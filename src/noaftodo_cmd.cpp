#include "noaftodo_cmd.h"

/*
 * [Almost] everything responsible for NOAFtodo
 * built-in command interpreter.
 *
 * I am losing control of it.
 * If hell exists, this is my ticket there.
 */

#include <cstdlib>
#include <stdexcept>

#include "noaftodo.h"
#include "noaftodo_config.h"
#include "noaftodo_cui.h"
#include "noaftodo_cvar.h"
#include "noaftodo_daemon.h"
#include "noaftodo_time.h"

using namespace std;

map<string, function<int(const vector<string>& args)>> cmd_cmds;
map<string, string> cmd_aliases;

string cmd_buffer;

noaftodo_entry* cmd_sel_entry = nullptr;

void cmd_init()
{
	cmd_buffer = "";
	cmd_aliases.clear();
	cmd_cmds.clear();

	// command "q" - exit the program.
	cmd_cmds["q"] = [] (const vector<string>& args)
	{
		if (cui_active) cui_mode = CUI_MODE_EXIT;
		else if ((run_mode == PM_DAEMON) && da_running) da_running = false;
		else exit(0);
		return 0;
	};

	// command "!<command>" - execute shell command
	cmd_cmds["!"] = [] (const vector<string>& args)
	{
		string cmdline = "";

		for (const auto& arg : args) cmdline += arg + " ";

		const bool wcui = cui_active;

		if (wcui) cui_destroy();

		log("Executing shell command: '" + cmdline + "'...");
		system(cmdline.c_str());

		if (wcui) cui_construct();

		return 0;
	};

	// command "alias <command>" - create an alias for command.
	cmd_cmds["alias"] = [] (const vector<string>& args)
	{
		if (args.size() < 1) return CMD_ERR_ARG_COUNT;

		if (args.size() == 1)
		{
			cmd_aliases.erase(args.at(0));
			return 0;
		}

		string alargs;
		for (int i = 1; i < args.size(); i++) 
		{
			if (alargs != "") alargs += " ";
			alargs += args.at(i);
		}

		log("Alias " + args.at(0) + " => " + alargs);

		cmd_aliases[args.at(0)] = alargs;

		return 0;
	};

	// command "c" - toggle selected task's "completed" property.
	cmd_cmds["c"] = [] (const vector<string>& args)
	{
		if (t_list.size() == 0) return CMD_ERR_EXTERNAL;

		li_comp(cui_s_line);
		return 0;
	};

	// command "d" - remove selected task.
	cmd_cmds["d"] = [] (const vector<string>& args)
	{
		if (t_list.size() == 0) return CMD_ERR_EXTERNAL;

		li_rem(cui_s_line);
		if (t_list.size() != 0) if (cui_s_line >= t_list.size()) cmd_exec("math %id% - 1 id");

		return 0;
	};

	// command "a <due> <title> <description>[ <id>]" - add or override a task. If <id> is not specified, a new task is created. If not, a task with <id> will be overriden.
	cmd_cmds["a"] = [] (const vector<string>& args)
	{
		if (args.size() < 3) return CMD_ERR_ARG_COUNT;

		try {
			noaftodo_entry new_entry;
			new_entry.completed = false;

			if (args.at(0) == "-")
			{
				new_entry.due = ti_to_long("a10000y");
				new_entry.meta["nodue"] = "true";
			}
			else new_entry.due = ti_to_long(args.at(0));
			new_entry.title = args.at(1);
			new_entry.description = args.at(2);

			new_entry.tag = (cui_tag_filter == CUI_TAG_ALL) ? 0 : cui_tag_filter;
			
			if (args.size() < 4) li_add(new_entry);
			else
			{
				const int id = stoi(args.at(3));

				if ((id >= 0) && (id < t_list.size())) 
				{
					new_entry.meta = t_list.at(id).meta;
					new_entry.tag = t_list.at(id).tag;
					t_list[id] = new_entry;

					if (li_autosave) li_save();
				}
				else return CMD_ERR_EXTERNAL;
			}

			// go to created task
			for (int i = 0; i < t_list.size(); i++)
				if (t_list.at(i) == new_entry) cui_s_line = i;

			return 0;
		} catch (const invalid_argument& e) {
			return CMD_ERR_ARG_TYPE;
		}
	};

	// command "setmeta[ <name1> <value1>[ <name2> <value2>[ ...]]]" - set task meta. If no arguments are specified, clear task meta. To add properties to meta, use "setmeta %meta% <name1> <value1>...". "setmeta <name>" will erase only <name> meta property
	cmd_cmds["setmeta"] = [] (const vector<string>& args)
	{
		if ((cui_s_line < 0) || (cui_s_line >= t_list.size())) return CMD_ERR_EXTERNAL;

		if (args.size() == 1)
		{
			t_list[cui_s_line].meta.erase(args.at(0));
			return 0;
		}

		t_list[cui_s_line].meta.clear();

		for (int i = 0; i + 1 < args.size(); i += 2)
			t_list[cui_s_line].meta[args.at(i)] = args.at(i + 1);

		if (li_autosave) li_save();

		return 0;
	};

	// command "lclear" - clear list.
	cmd_cmds["lclear"] = [] (const vector<string>& args)
	{
		if (cui_tag_filter == CUI_TAG_ALL) return CMD_ERR_EXTERNAL;

		for (int i = 0; i < t_list.size(); )
			if (t_list.at(i).tag == cui_tag_filter) li_rem(i);
			else i++;

		return 0;
	};

	// command "lmv <list_id>" - move selected task to a list.
	cmd_cmds["lmv"] = [] (const vector<string>& args)
	{
		if (args.size() < 1) return CMD_ERR_ARG_COUNT;

		try {
			t_list[cui_s_line].tag = stoi(args.at(0));
			if (li_autosave) li_save();

			return 0;
		} catch (const invalid_argument& e) {
			return CMD_ERR_ARG_TYPE;
		}
	};

	// command "bind <key> <command> <mode> <autoexec>" - bind <key> to <command>. <mode> speciifes, which modes use this bind (mask, see noaftodo_cui.h for CUI_MODE_* values). If <autoexec> is "true", execute command on key hit, otherwise just go into command mode with it.
	cmd_cmds["bind"] = [] (const vector<string>& args)
	{
		if (args.size() < 1) return CMD_ERR_ARG_COUNT;

		if (args.size() >= 4) try {
			const string skey = args.at(0);
			const string scomm = args.at(1);
			const int smode = stoi(args.at(2));
			const bool sauto = (args.at(3) == "true");

			log("Binding " + skey + " to \"" + scomm + "\"");

			cui_bind(cui_key_from_str(skey), scomm, smode, sauto);

			return 0;
		} catch (const invalid_argument& e) {
			return CMD_ERR_ARG_TYPE;
		}

		if (args.size() > 1) return CMD_ERR_ARG_COUNT;

		log("Unbinding " + args.at(0));

		bool removed = false;

		for (int i = 0; i < binds.size(); i++)
			if (cui_key_from_str(args.at(0)) == binds.at(i).key)
			{
				binds.erase(binds.begin() + i);
				i--;
				removed = true;
			}

		return removed ? 0 : CMD_ERR_EXTERNAL;
	};

	// command "math <num1> <op> <num2> <name>" - calculate math expression (+,-,/,*,=,<,>,min,max) and write to cvar <name>. If <name> is not specifed, just print the result out
	cmd_cmds["math"] = [] (const vector<string>& args)
	{
		if (args.size() < 3) return CMD_ERR_ARG_COUNT;

		try
		{
			const double a = stod(args.at(0));
			const double b = stod(args.at(2));

			double result = 0;

			switch (args.at(1).at(0))
			{
				case '+':
					result = a + b;
					break;
				case '-':
					result = a - b;
					break;
				case '*':
					result = a * b;
					break;
				case '/':
					if (b == 0) return CMD_ERR_ARG_TYPE;
					result = a / b;
					break;
				case 'm':
					if (args.at(1) == "max") result = (a >= b) ? a : b;
					else if (args.at(1) == "min") result = (a < b) ? a : b;
					break;
				case '=':
					if (args.size() < 4) cui_status = (a == b) ? "true" : "false";
					else cvar(args.at(3)).setter((a == b) ? "true" : "false");
					return 0;
				case '<':
					if (args.size() < 4) cui_status = (a < b) ? "true" : "false";
					else cvar(args.at(3)).setter((a < b) ? "true" : "false");
					return 0;
				case '>':
					if (args.size() > 4) cui_status = (a > b) ? "true" : "false";
					else cvar(args.at(3)).setter((a > b) ? "true" : "false");
					return 0;
			}

			if (args.size() < 4) cui_status = to_string(result);
			else cvar(args.at(3)).setter(to_string(result));
		} catch (const invalid_argument& e) { return CMD_ERR_ARG_TYPE; }

		return 0;
	};

	// command "if <true|false> <do-if-true>[ <do-if-false>]" - simple if expression
	cmd_cmds["if"] = [] (const vector<string>& args)
	{
		if (args.size() < 2) return CMD_ERR_ARG_COUNT;
		
		if (args.at(0) == "true") cmd_exec(args.at(1));
		else if (args.size() > 2) cmd_exec(args.at(2));

		return 0;
	};

	// command "set <name>[ <value>]" - set cvar value. If <value> is not specified, reset cvar to default value.
	cmd_cmds["set"] = [] (const vector<string>& args)
	{
		if (args.size() < 1) return CMD_ERR_ARG_COUNT;

		if (args.size() < 2)
		{
			cvar_reset(args.at(0));

			return 0;
		}

		cvar(args.at(0)) = args.at(1);
		return 0;
	};

	// command "exec <filename>[ script]" - execute a config file. Execute default config with "exec default". With "script" cvars from config are not set as default
	cmd_cmds["exec"] = [] (const vector<string>& args)
	{
		if (args.size() < 1) return CMD_ERR_ARG_COUNT;

		bool predef_cvars = true;

		for (int i = 1; i < args.size(); i++)
		{
			if (args.at(i) == "script") predef_cvars = false;
		}

		conf_load(args.at(0), predef_cvars);

		return 0;
	};

	// command "ver <VERSION>" - is used to specify config version to notify about possible outdated config files.
	cmd_cmds["ver"] = [] (const vector<string>& args)
	{
		if (args.size() < 1) return CMD_ERR_ARG_COUNT;

		if (args.at(0) != to_string(CONF_V))
		{
			log("Config version mismatch (CONF_V " + args.at(0) + " != " + to_string(CONF_V) + "). "
				"Consult \"Troubleshooting\" section of help (\"noaftodo -h\").", LP_ERROR);
			errors |= ERR_CONF_V;
			li_autosave = false;
		}

		return 0;
	};

	// command "save[ <filename>]" - force the list save. If <filename> is not specified, override opened file.
	cmd_cmds["save"] = [] (const vector<string>& args)
	{
		if (args.size() < 1) return li_save();
		else return li_save(args.at(0));
		return 0;
	};

	// command "echo[ args...]" - print the following in status.
	cmd_cmds["echo"] = [] (const vector<string>& args)
	{
		string message = "";
		for (int i = 0; i < args.size(); i++) message += args.at(i) + " ";

		if (cui_active) cui_status = message;
		else log("echo :: " + message, LP_IMPORTANT);
		return 0;
	};

	// "fake" cvars
	cvar_wrap_int("halfdelay_time", cui_halfdelay_time);

	cvar_wrap_int("tag_filter", cui_tag_filter);
	cvars["tag_filter"]->setter = [] (const string& val)
	{
		try
		{
			if (val == "all") 
			{
				cui_tag_filter = CUI_TAG_ALL;
				return;
			}

			const int new_filter = stoi(val);

			if (new_filter == cui_tag_filter) cui_tag_filter = CUI_TAG_ALL;
			else cui_tag_filter = new_filter;

			cvars["pname"]->predefine(to_string(cui_tag_filter));
		} catch (const invalid_argument& e) {}
	};
	cvars["tag_filter"]->flags |= CVAR_FLAG_NO_PREDEF;
	cvars["tag_filter"]->predefine("all");

	cvar_wrap_int("tag_filter_v", cui_tag_filter);
	cvars["tag_filter_v"]->setter = [] (const string& val)
	{
		if (val == "all")
		{
			cui_tag_filter = CUI_TAG_ALL;
			return;
		}

		try
		{
			const int new_filter = stoi(val);
			const int dir = (new_filter >= cui_tag_filter) ? 1 : -1;

			cui_tag_filter = new_filter;

			while (!cui_l_is_visible(cui_tag_filter))
			{
				cui_tag_filter += dir;

				if (cui_tag_filter >= (int)t_tags.size()) // wwithout (int) returns true
									// with any negative cui_tag_filter
				{
					cui_tag_filter = CUI_TAG_ALL;
					return;
				}

				if (cui_tag_filter < CUI_TAG_ALL)
					cui_tag_filter = t_tags.size() - 1;
			}

			cvars["pname"]->predefine(to_string(cui_tag_filter));
		} catch (const invalid_argument& e) {}
	};
	cvars["tag_filter_v"]->flags |= CVAR_FLAG_NO_PREDEF | CVAR_FLAG_WS_IGNORE;
	cvars["tag_filter_v"]->predefine("-1");

	cvar_wrap_int("filter", cui_filter);

	cvar_wrap_string("regex_filter", cui_normal_regex_filter);
	cvar_wrap_string("list_regex_filter", cui_listview_regex_filter);

	cvar_wrap_string("contexec_cmd_regex", cui_contexec_regex_filter);

	cvar_wrap_bool("colors.status_standout", cui_status_standout);

	cvar_wrap_bool("shift_multivars", cui_shift_multivars);

	cvar_wrap_int("colors.background", cui_color_bg);
	cvar_wrap_int("colors.title", cui_color_title);
	cvar_wrap_int("colors.status", cui_color_status);
	cvar_wrap_int("colors.entry_completed", cui_color_complete);
	cvar_wrap_int("colors.entry_coming", cui_color_coming);
	cvar_wrap_int("colors.entry_failed", cui_color_failed);

	cvar_wrap_int("mode", cui_mode, CVAR_FLAG_NO_PREDEF);
	cvars["mode"]->setter = [] (const string& val)
	{
		try
		{
			cui_set_mode(stoi(val));
			return 0;
		} catch (const invalid_argument& e) { return CMD_ERR_ARG_TYPE; }
	};
	cvars["mode"]->predefine("-1");

	cvar_wrap_int("id", cui_s_line, CVAR_FLAG_NO_PREDEF);
	cvars["id"]->setter = [] (const string& val)
	{
		bool is_visible = false;
		for (int i = 0; i < t_list.size(); i++) is_visible |= cui_is_visible(i);
		if (!is_visible) return; // don't try to set the ID when there's nothing on the screen

		try
		{
			const int target = stoi(val);

			const int dir = (target - cui_s_line >= 0) ? 1 : -1;

			if ((target >= 0) && (target < t_list.size()))
				cui_s_line = target;
			else if (target < 0) cui_s_line = t_list.size() - 1;
			else cui_s_line = 0;

			if (cui_active)	while (!cui_is_visible(cui_s_line)) 
			{
				cui_s_line += dir;

				if (cui_s_line < 0) cui_s_line = t_list.size() - 1;
				else if (cui_s_line >= t_list.size()) cui_s_line = 0;
			}
		} catch (const invalid_argument& e) {}
	};
	cvars["id"]->predefine("0");

	cvars["T"] = make_unique<cvar_base_s>();
	cvars["T"]->getter = [] ()
	{
		if (cmd_sel_entry == nullptr)  return string();
		return cmd_sel_entry->title;
	};
	cvars["T"]->setter = [] (const string& val)
	{
		if (cmd_sel_entry == nullptr) return;
		cmd_sel_entry->title = val;
		if (li_autosave) li_save();
	};
	cvars["T"]->flags = CVAR_FLAG_NO_PREDEF | CVAR_FLAG_WS_IGNORE;

	cvars["D"] = make_unique<cvar_base_s>();
	cvars["D"]->getter = [] ()
	{
		if (cmd_sel_entry == nullptr)  return string();
		return cmd_sel_entry->description;
	};
	cvars["D"]->setter = [] (const string& val)
	{
		if (cmd_sel_entry == nullptr) return;
		cmd_sel_entry->description = val;
		if (li_autosave) li_save();
	};
	cvars["D"]->flags = CVAR_FLAG_NO_PREDEF | CVAR_FLAG_WS_IGNORE;

	cvars["due"] = make_unique<cvar_base_s>();
	cvars["due"]->getter = [] ()
	{
		if (cmd_sel_entry == nullptr)  return string();
		return ti_cmd_str(cmd_sel_entry->due);
	};
	cvars["due"]->setter = [] (const string& val)
	{
		if (cmd_sel_entry == nullptr) return;
		cmd_sel_entry->due = ti_to_long(val);
		if (li_autosave) li_save();
	};
	cvars["due"]->flags = CVAR_FLAG_NO_PREDEF | CVAR_FLAG_WS_IGNORE;

	cvars["meta"] = make_unique<cvar_base_s>();
	cvars["meta"]->getter = [] ()
	{
		if (cmd_sel_entry == nullptr)  return string();
		return replace_special(cmd_sel_entry->meta_str());
	};
	cvars["meta"]->flags = CVAR_FLAG_NO_PREDEF | CVAR_FLAG_WS_IGNORE;

	cvars["pname"] = make_unique<cvar_base_s>(); // parent [list] name
	cvars["pname"]->getter = [] () 
	{
		if (cui_tag_filter == CUI_TAG_ALL) return string("All lists");

		if (cui_tag_filter < t_tags.size()) return t_tags.at(cui_tag_filter);

		return to_string(cui_tag_filter);
	};
	cvars["pname"]->setter = [] (const string& val)
	{
		if (cui_tag_filter < 0) return;

		while (cui_tag_filter >= t_tags.size()) t_tags.push_back(to_string(t_tags.size()));

		t_tags[cui_tag_filter] = val;

		if (li_autosave) li_save();
	};
	cvars["pname"]->flags = CVAR_FLAG_NO_PREDEF | CVAR_FLAG_WS_IGNORE;

	cvars["last_visible_id"] = make_unique<cvar_base_s>();
	cvars["last_visible_id"]->getter = [] () 
	{
		int ret = -1;
		for (int i = 0; i < t_list.size(); i++)
			if (cui_is_visible(i)) ret = i;

		return to_string(ret);
	};
	cvars["last_visible_id"]->flags = CVAR_FLAG_RO | CVAR_FLAG_WS_IGNORE | CVAR_FLAG_NO_PREDEF;

	cvars["first_visible_id"] = make_unique<cvar_base_s>();
	cvars["first_visible_id"]->getter = [] () 
	{
		for (int i = 0; i < t_list.size(); i++)
			if (cui_is_visible(i)) return to_string(i);

		return string("-1");
	};
	cvars["first_visible_id"]->flags = CVAR_FLAG_RO | CVAR_FLAG_WS_IGNORE | CVAR_FLAG_NO_PREDEF;

	cvars["last_visible_list"] = make_unique<cvar_base_s>();
	cvars["last_visible_list"]->getter = [] () 
	{
		int ret = -1;
		for (int i = 0; i < t_tags.size(); i++)
			if (cui_l_is_visible(i)) ret = i;

		return to_string(ret);
	};
	cvars["last_visible_list"]->flags = CVAR_FLAG_RO | CVAR_FLAG_WS_IGNORE | CVAR_FLAG_NO_PREDEF;

	cvars["first_visible_list"] = make_unique<cvar_base_s>();
	cvars["first_visible_list"]->getter = [] () 
	{
		for (int i = 0; i < t_tags.size(); i++)
			if (cui_l_is_visible(i)) return to_string(i);

		return string("-1");
	};
	cvars["first_visible_list"]->flags = CVAR_FLAG_RO | CVAR_FLAG_WS_IGNORE | CVAR_FLAG_NO_PREDEF;

	cvars["VER"] = make_unique<cvar_base_s>();
	cvars["VER"]->getter = [] () { return VERSION; };
	cvars["VER"]->flags = CVAR_FLAG_RO | CVAR_FLAG_WS_IGNORE | CVAR_FLAG_NO_PREDEF;

	cvar_wrap_string("all_cols", cui_normal_all_cols);
	cvar_wrap_string("cols", cui_normal_cols);
	cvar_wrap_string("listview_cols", cui_listview_cols);
	cvar_wrap_string("details_cols", cui_details_cols);

	cvar_wrap_string("status_fields", cui_normal_status_fields);
	cvar_wrap_string("listview_status_fields", cui_listview_status_fields);

	cvar_wrap_multistr("charset.separators", cui_separators, 3);
	cvar_wrap_multistr_element("charset.row_separator", cui_separators, CHAR_ROW_SEP);
	cvar_wrap_int("charset.row_separator.offset", cui_row_separator_offset);
	cvar_wrap_multistr_element("charset.status_separator", cui_separators, CHAR_STA_SEP);
	cvar_wrap_multistr_element("charset.details_separator", cui_separators, CHAR_DET_SEP);

	cvar_wrap_multistr("charset.box_strong", cui_box_strong, 6);
	cvar_wrap_multistr("charset.box_light", cui_box_light, 6);
	cvar_wrap_multistr_element("charset.box_border_v", cui_box_strong, CHAR_VLINE);
	cvar_wrap_multistr_element("charset.box_border_h", cui_box_strong, CHAR_HLINE);
	cvar_wrap_multistr_element("charset.box_corner_1", cui_box_strong, CHAR_CORN1);
	cvar_wrap_multistr_element("charset.box_corner_2", cui_box_strong, CHAR_CORN2);
	cvar_wrap_multistr_element("charset.box_corner_3", cui_box_strong, CHAR_CORN3);
	cvar_wrap_multistr_element("charset.box_corner_4", cui_box_strong, CHAR_CORN4);
	cvar_wrap_multistr_element("charset.box_ui_line_h", cui_box_light, CHAR_HLINE);

	cvar_wrap_maskflag("lview_show_empty", cui_filter, CUI_FILTER_EMPTY);

	cvar_wrap_maskflag("show_uncat", cui_filter, CUI_FILTER_UNCAT, CVAR_FLAG_WS_IGNORE);
	cvar_wrap_maskflag("show_complete", cui_filter, CUI_FILTER_COMPLETE, CVAR_FLAG_WS_IGNORE);
	cvar_wrap_maskflag("show_coming", cui_filter, CUI_FILTER_COMING, CVAR_FLAG_WS_IGNORE);
	cvar_wrap_maskflag("show_failed", cui_filter, CUI_FILTER_FAILED, CVAR_FLAG_WS_IGNORE);
	cvar_wrap_maskflag("show_nodue", cui_filter, CUI_FILTER_NODUE, CVAR_FLAG_WS_IGNORE);

	cvar_wrap_bool("allow_root", allow_root);
	cvar_wrap_bool("daemon.fork_autostart", da_fork_autostart);

	cvar_wrap_string("on_daemon_launch_action", da_launch_action);
	cvar_wrap_string("on_task_failed_action", da_task_failed_action);
	cvar_wrap_string("on_task_coming_action", da_task_coming_action);
	cvar_wrap_string("on_task_completed_action", da_task_completed_action);
	cvar_wrap_string("on_task_uncompleted_action", da_task_uncompleted_action);
	cvar_wrap_string("on_task_new_action", da_task_new_action);
	cvar_wrap_string("on_task_removed_action", da_task_removed_action);

	cvar_wrap_int("numbuffer", cui_numbuffer, CVAR_FLAG_WS_IGNORE | CVAR_FLAG_NO_PREDEF);
	cvars["numbuffer"]->predefine("-1");
}

vector<string> cmd_break(const string& cmdline)
{
	vector<string> cmdqueue;
	string temp = "";

	bool inquotes = false;
	bool skip_special = false;
	for (int i = 0; i < cmdline.length(); i++)
	{
		const char c = cmdline.at(i);

		if (skip_special)
		{
			temp += c;
			skip_special = false;
		} else switch (c) {
			case '\\':
				skip_special = true;
				temp += c;
				break;
			case '"':
				inquotes = !inquotes;
				temp += c;
				break;
			case ';':
				if (!inquotes)
				{
					if (temp != "")
					{
						cmdqueue.push_back(temp);
						temp = "";
					}
					break;
				}
			default:
				temp += c;
				break;
		}
	}

	if (inquotes || skip_special) // unterminated command (unterminated quotes or ends with '\')
	{
		cmd_buffer = cmdline;
		return vector<string>();
	}

	if (temp != "") cmdqueue.push_back(temp);

	return cmdqueue;
}

void cmd_run(string command)
{
	log_offset++;
	// replace variables
	if ((cui_s_line >= 0) && (cui_s_line < t_list.size()))
		command = format_str(command, t_list.at(cui_s_line));
	else
		command = format_str(command, NULL_ENTRY);

	vector<string> new_commands = cmd_break(command);

	if (new_commands.size() == 0) { log_offset--; return; } // basiacally, an empty string, or just ';'s

	if (new_commands.size() > 1) // more than one command in a string. Each can contain some more after
					// variables are replaced with their values
	{
		log("Breaking " + command);
		for (const auto& cmd : new_commands) cmd_run(cmd);
		log_offset--;
		return;
	}

	log(command + " is elementary. Executing..");

	// at this point, we know for sure that there's only one command in a string
	// break it into tokens and execute
	string name = "";	// command name (first word)
	vector<string> args;	// command arguments
	string word = "";	// buffer

	bool inquotes = false;
	bool skip_special = false;
	bool shellcmd = false;

	auto put = [&name, &args] (const string& val)
	{
		if (name == "") name = val;
		else args.push_back(val);
	};

	int start = 0;	// to respect tabs and spaces at the line start

	for (int i = 0; i < command.length(); i++)
	{
		const char c = command.at(i);

		if ((i == start) && (c == '!'))	// "!" - shell command
		{
			put("!");

			shellcmd = true;
			continue;
		}

		if (shellcmd) { // everything between "!" and ";" is passed to system() as-it-is
			word += c;
		} else if (skip_special) // symbol after '\' is added as-it-is
		{
			word += c;
			skip_special = false;
		} else switch (c) {
			case '#': // comment. Stop execution
				i = command.length();
				break;
			case '\\':
				skip_special = true;
				break;
			case '"':
				inquotes = !inquotes;
				break;
			case ' ': case '\t': // token separator
				if (!inquotes) // in quotes - just a symbol
				{
					if (word != "")
					{
						put(word);
						word = "";
					}
					
					if (name == "") start = i + 1; // space/tab at the beginning of a line.
									// Command starts not earlier than at
									// next symbol
					break;
				}
			default:
				word += c;
		}
	}

	cmd_buffer = ""; // if we have reached this far, a command is at least properly terminated

	if (word != "") put(word);

	if (name == "") { log_offset--; return; } // null command

	try
	{	// search for cmd_aliases, prioritize
		string alstr = cmd_aliases.at(name);

		// insert alias arguments. Add unused ones to command line 
		for (int j = 0; j < args.size(); j++)
		{
			bool replaced = false;

			int index = -1;
			const string varname = "%arg" + to_string(j + 1) + "%";
			while ((index = alstr.find(varname)) != string::npos)
			{
				alstr.replace(index, varname.length(), args.at(j));
				replaced = true;
			}

			if (!replaced) alstr += " \"" + replace_special(args.at(j)) + "\"";
		}

		cmd_exec(alstr); // try to run
	} catch (const out_of_range& e) { // no such alias
		try
		{
			const int ret = (cmd_cmds.at(name))(args); // try to run command

			if (ret != 0) switch (ret) // handle error return values
			{
				case CMD_ERR_ARG_COUNT:
					cui_status = "Wrong argument count!";
					break;
				case CMD_ERR_ARG_TYPE:
					cui_status = "Wrog argument type!";
					break;
				case CMD_ERR_EXTERNAL:
					cui_status = "Cannot execute command!";
					break;
				default:
					cui_status = "Unknown execution error!";
			}
		} catch (const out_of_range& e) { // command not found
			log("Command not found! (" + command + ")", LP_ERROR);
			cui_status = "Command not found!";
		}
	}

	log_offset--;
}

void cmd_exec(const string& command)
{
	log ("Attempting execution: " + command);
	vector<string> cmdq = cmd_break(cmd_buffer + command);

	for (const auto& cmd : cmdq) cmd_run(cmd);
}

void cmd_terminate()
{
	if (cmd_buffer == "") return;

	cui_status = "Unterminated command";
	log("Unterminated command at the end of execution. Skipping...", LP_ERROR);

	cmd_buffer = "";
}
