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
#include "noaftodo_list.h"
#include "noaftodo_time.h"

using namespace std;

map<string, function<int(const vector<string>& args)>> cmds;
map<string, string> aliases;

string cmd_buffer = "";

void cmd_init()
{
	// command "q" - exit the program.
	cmds["q"] = [] (const vector<string>& args)
	{
		cui_mode = CUI_MODE_EXIT;
		return 0;
	};

	// command ":" - enter the command mode.
	cmds[":"] = [] (const vector<string>& args)
	{
		cui_set_mode(CUI_MODE_COMMAND);
		return 0;
	};

	// command "?" - show the help message.
	cmds["?"] = [] (const vector<string>& args)
	{
		cui_set_mode(CUI_MODE_HELP);
		return 0;
	};

	// command "back" - go to previous mode.
	cmds["back"] = [] (const vector<string>& args)
	{
		cui_set_mode(-1);
		return 0;
	};

	// command "details" - view task details.
	cmds["details"] = [] (const vector<string>& args)
	{
		if ((cui_s_line < 0) || (cui_s_line >= t_list.size()))
			return CMD_ERR_EXTERNAL;
		cui_set_mode(CUI_MODE_DETAILS);
		return 0;
	};

	// command "listview" - go to the list view
	cmds["listview"] = [] (const vector<string>& args)
	{
		cui_set_mode(CUI_MODE_LISTVIEW);
		return 0;
	};

	// command "alias <command>" - create an alias for command.
	cmds["alias"] = [] (const vector<string>& args)
	{
		if (args.size() < 1) return CMD_ERR_ARG_COUNT;

		string alargs;
		for (int i = 1; i < args.size(); i++) 
		{
			if (alargs != "") alargs += " ";
			alargs += args.at(i);
		}

		log("Alias " + args.at(0) + " => " + alargs);

		aliases[args.at(0)] = alargs;

		return 0;
	};

	// command "next" - go to the next list
	cmds["next"] = [] (const vector<string>& args)
	{
		cui_tag_filter++;

		if (cui_tag_filter >= t_tags.size()) cui_tag_filter = CUI_TAG_ALL;

		if (!cui_l_is_visible(cui_tag_filter)) cmd_exec("next");

		return 0;
	};

	// command "prev" - go to the previous list
	cmds["prev"] = [] (const vector<string>& args)
	{
		cui_tag_filter--;

		if (cui_tag_filter < CUI_TAG_ALL) cui_tag_filter = t_tags.size() - 1;

		if (!cui_l_is_visible(cui_tag_filter)) cmd_exec("prev");

		return 0;
	};

	// command "c" - toggle selected task's "completed" property.
	cmds["c"] = [] (const vector<string>& args)
	{
		if (t_list.size() == 0) return CMD_ERR_EXTERNAL;

		li_comp(cui_s_line);
		return 0;
	};

	// command "d" - remove selected task.
	cmds["d"] = [] (const vector<string>& args)
	{
		if (t_list.size() == 0) return CMD_ERR_EXTERNAL;

		li_rem(cui_s_line);
		if (t_list.size() != 0) if (cui_s_line >= t_list.size()) cmd_exec("up");

		return 0;
	};

	// command "a <due> <title> <description>[ <id>]" - add or override a task. If <id> is not specified, a new task is created. If not, a task with <id> will be overriden.
	cmds["a"] = [] (const vector<string>& args)
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
				if (t_list.at(i) == new_entry) cmd_exec("g " + to_string(i));

			return 0;
		} catch (const invalid_argument& e) {
			return CMD_ERR_ARG_TYPE;
		}
	};

	// command "setmeta <name>[ <value>]" - set task meta property. If <value> is not specified, property with name <name> is reset.
	cmds["setmeta"] = [] (const vector<string>& args)
	{
		if (args.size() < 1) return CMD_ERR_ARG_COUNT;

		if ((cui_s_line < 0) || (cui_s_line >= t_list.size())) return CMD_ERR_EXTERNAL;

		if (args.size() < 2)
			t_list[cui_s_line].meta.erase(args.at(0));
		else 
		{
			if (args.size() % 2 == 1) return CMD_ERR_ARG_COUNT;
			for (int i = 0; i < args.size(); i += 2)
				t_list[cui_s_line].meta[args.at(i)] = args.at(i + 1);
		}

		if (li_autosave) li_save();

		return 0;
	};

	// command "clrmeta" - erase task metadata.
	cmds["clrmeta"] = [] (const vector<string>& args)
	{
		if ((cui_s_line < 0) || (cui_s_line >= t_list.size())) return CMD_ERR_EXTERNAL;

		for (auto it = t_list[cui_s_line].meta.begin(); it != t_list[cui_s_line].meta.end(); it = t_list[cui_s_line].meta.begin())
			t_list[cui_s_line].meta.erase(it->first);

		return 0;
	};

	// command "lrename[ <new_name>]" - rename list. If <new_name> is not specified or is the same as list id, list name is reset.
	cmds["lrename"] = [] (const vector<string>& args)
	{
		if (cui_tag_filter == CUI_TAG_ALL) return CMD_ERR_EXTERNAL;

		if ((args.size() < 1) && (cui_tag_filter < t_tags.size())) 
		{ 
			t_tags[cui_tag_filter] = to_string(cui_tag_filter);
			return 0;
		}

		while (cui_tag_filter >= t_tags.size()) t_tags.push_back(to_string(t_tags.size()));

		t_tags[cui_tag_filter] = args.at(0);
		if (li_autosave) li_save();

		return 0;
	};

	// command "lclear" - clear list.
	cmds["lclear"] = [] (const vector<string>& args)
	{
		if (cui_tag_filter == CUI_TAG_ALL) return CMD_ERR_EXTERNAL;

		for (int i = 0; i < t_list.size(); )
			if (t_list.at(i).tag == cui_tag_filter) li_rem(i);
			else i++;

		return 0;
	};

	// command "lmv <list_id>" - move selected task to a list.
	cmds["lmv"] = [] (const vector<string>& args)
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
	cmds["bind"] = [] (const vector<string>& args)
	{
		if (args.size() < 4) return CMD_ERR_ARG_COUNT;

		try {
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
	};

	// command "unbind <key>" - remove bind from key
	cmds["unbind"] = [] (const vector<string>& args)
	{
		if (args.size() < 1) return CMD_ERR_ARG_COUNT;

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

	// command "math <num1> <op> <num2> <name>" - calculate math expression (+,-,/,*) and write to cvar <name>. If <name> is not specifed, just print the result out
	cmds["math"] = [] (const vector<string>& args)
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
			}

			if (args.size() < 4) cui_status = to_string(result);
			else cvar(args.at(3)).setter(to_string(result));
		} catch (const invalid_argument& e) { return CMD_ERR_ARG_TYPE; }

		return 0;
	};

	// command "set <name>[ <value>]" - set cvar value. If <value> is not specified, reset cvar to default value.
	cmds["set"] = [] (const vector<string>& args)
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

	// command "toggle <name>" - toggles variable value between "true" and "false"
	cmds["toggle"] = [] (const vector<string>& args)
	{
		if (args.size() < 1) return CMD_ERR_ARG_COUNT;

		if (cvar(args.at(0)) == "true") 
			cvar(args.at(0)) = "false"; // cannot just do cvars.erase(args.at(0)) because
							// someone might've set it to true in their default config
		else cvar(args.at(0)) = "true";

		return 0;
	};

	// command "exec <filename>[ script]" - execute a config file. Execute default config with "exec default". With "script" cvars from config are not set as default
	cmds["exec"] = [] (const vector<string>& args)
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
	cmds["ver"] = [] (const vector<string>& args)
	{
		if (args.size() < 1) return CMD_ERR_ARG_COUNT;

		if (args.at(0) != to_string(CONF_V))
		{
			log("File you are trying to load is declared to be for an outdated version of NOAFtodo (CONF_V " + args.at(0) + " != " + to_string(CONF_V) + ") and might not work as expected. Fix it and restart " + TITLE + ". Config file: " + conf_filename + ", list file: " + li_filename + ". Exiting program now.", LP_ERROR);

			switch (run_mode)
			{
				case PM_DEFAULT:
					if (cui_active) cui_set_mode(CUI_MODE_EXIT);
					else exit(1);
					break;
				case PM_DAEMON:
					if (da_running) da_kill();
					else exit(1);
					break;
				default:
					exit(1);
					break;
			}
		}

		return 0;
	};

	// command "save[ <filename>]" - force the list save. If <filename> is not specified, override opened file.
	cmds["save"] = [] (const vector<string>& args)
	{
		if (args.size() < 1)	li_save();
		else 			li_save(args.at(0));
		return 0;
	};

	// command "echo[ args...]" - print the following in status.
	cmds["echo"] = [] (const vector<string>& args)
	{
		string message = "";
		for (int i = 0; i < args.size(); i++) message += args.at(i) + " ";
		cui_status = message;
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
		} catch (const invalid_argument& e) {}
	};
	cvars["tag_filter"]->flags |= CVAR_FLAG_NO_PREDEF;
	cvars["tag_filter"]->predefine("all");

	cvar_wrap_int("filter", cui_normal_filter);

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

	cvar_wrap_int("id", cui_s_line, CVAR_FLAG_NO_PREDEF);
	cvars["id"]->setter = [] (const string& val)
	{
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

	cvar_wrap_maskflag("lview_show_empty", cui_listview_filter, CUI_FILTER_EMPTY);

	cvar_wrap_maskflag("show_uncat", cui_normal_filter, CUI_FILTER_UNCAT, CVAR_FLAG_WS_IGNORE);
	cvar_wrap_maskflag("show_complete", cui_normal_filter, CUI_FILTER_COMPLETE, CVAR_FLAG_WS_IGNORE);
	cvar_wrap_maskflag("show_coming", cui_normal_filter, CUI_FILTER_COMING, CVAR_FLAG_WS_IGNORE);
	cvar_wrap_maskflag("show_failed", cui_normal_filter, CUI_FILTER_FAILED, CVAR_FLAG_WS_IGNORE);
	cvar_wrap_maskflag("show_nodue", cui_normal_filter, CUI_FILTER_NODUE, CVAR_FLAG_WS_IGNORE);

	cvar_wrap_bool("allow_root", allow_root);
	cvar_wrap_bool("daemon.fork_autostart", da_fork_autostart);

	cvar_wrap_string("on_daemon_launch_action", da_launch_action);
	cvar_wrap_string("on_task_failed_action", da_task_failed_action);
	cvar_wrap_string("on_task_coming_action", da_task_coming_action);
	cvar_wrap_string("on_task_completed_action", da_task_completed_action);
	cvar_wrap_string("on_task_uncompleted_action", da_task_uncompleted_action);
	cvar_wrap_string("on_task_new_action", da_task_new_action);
	cvar_wrap_string("on_task_removed_action", da_task_removed_action);
}

int cmd_exec(string command)
{
	vector<string> words;
	string word = "";
	bool inquotes = false;
	bool skip_special = false;
	bool shell_cmd = false;

	if ((cui_s_line >= 0) && (cui_s_line < t_list.size()))
		command = format_str(command, t_list.at(cui_s_line));
	else
		command = format_str(command, NULL_ENTRY);

	if (cmd_buffer != "")
	{
		command = cmd_buffer + command;
		cmd_buffer = "";
	}

	for (int i = 0; i < command.length(); i++)
	{
		const char c = command.at(i);

		if (shell_cmd)
		{
			if (!skip_special && (c == ';')) 
			{
				words.push_back(word);
				word = "";
				words.push_back(";");
				shell_cmd = false;
			} else {
				if (skip_special) word += '\\';
				
				if (c == '\\') skip_special = true;
				else { word += c; skip_special = false; }
			}
		} else if (skip_special) {
			word += c;
			skip_special = false;
		} else switch (c) {
			case '\\':
				skip_special = true;
				break;
			case ' ': case '\t':
				if (inquotes) word += c;
				else 
					if (word != "")
					{
						words.push_back(word);
						word = "";
					}
				break;
			case ';':
				if (inquotes) word += c;
				else
				{
					if (word != "")
					{
						words.push_back(word);
						word = "";
					}
					words.push_back(";");
				}
				break;
			case '"':
				inquotes = !inquotes;
				break;
			case '!':
				if (!inquotes) shell_cmd = true;
				word += c;
				break;
			default:
				word += c;
		}
	}
	if (word != "") words.push_back(word);

	if (inquotes) // line break inside quotes
	{
		cmd_buffer = command;
		return 0;
	}

	int offset = 0;
	for (int i = 0; i < words.size(); i++)
	{
		if (words.at(i) == ";") // command separator
		{
			offset = i + 1;

			for (int k = offset; k < words.size(); k++) // update rest of the commands
			{
				if ((cui_s_line >= 0) && (cui_s_line < t_list.size()))
					words.at(k) = format_str(words.at(k), t_list.at(cui_s_line));
				else
					words.at(k) = format_str(words.at(k), NULL_ENTRY);
			}
		} else if (i == offset) {
			if (words.at(i).at(0) == '!')
			{
				const bool wcui = cui_active;

				if (wcui) cui_destroy();

				string shell_command = words.at(i).substr(1);

				log("Executing shell command: '" + shell_command + "'...");

				system(shell_command.c_str());

				if (wcui) cui_construct();
			} else {
				// search command in cmds

				vector<string> cmdarg;

				int j;
				for (j = 1; i + j < words.size(); j++)
				{
					if (words.at(i + j) == ";") break;
					cmdarg.push_back(words.at(i + j));
				}

				try {	// search for alias, prioritize
					string alstr = aliases.at(words.at(i));

					if ((cui_s_line >= 0) && (cui_s_line < t_list.size())) alstr = format_str(alstr, t_list.at(cui_s_line));
					else alstr = format_str(alstr, NULL_ENTRY);

					for (int j = 0; j < cmdarg.size(); j++)
					{
						bool replaced = false;

						int index = -1;
						const string varname = "%arg" + to_string(j + 1) + "%";
						while ((index = alstr.find(varname)) != string::npos)
						{
							alstr.replace(index, varname.length(), cmdarg.at(j));
							replaced = true;
						}

						if (!replaced) alstr += " " + cmdarg.at(j);
					}

					log("Executing alias " + words.at(i) + " => " + alstr);
					cmd_exec(alstr);
				} catch (const out_of_range& e) { // alias not found, try to execute a command
					try {
						string l = words.at(i) + " ";
						for (int k = 0; k < cmdarg.size(); k++) l += "\"" + cmdarg.at(k) + "\" ";
						log(l);

						const int ret = (cmds.at(words.at(i)))(cmdarg);

						if (ret == CMD_ERR_ARG_COUNT)	cui_status = "Wrong argument count";
						if (ret == CMD_ERR_ARG_TYPE)	cui_status = "Wrong argument type";
						if (ret == CMD_ERR_EXTERNAL)	cui_status = "Cannot execute command";
					} catch (const out_of_range& e)
					{
						log("Command not found! (" + command + ")", LP_ERROR);
						cui_status = "Command not found!";
					}
				}

				offset += j;
			}
		}
	}

	return 0;
}

void cmd_terminate()
{
	if (cmd_buffer == "") return;

	cui_status = "Unterminated command";
	log("Unterminated command at the end of execution. Skipping...", LP_ERROR);

	cmd_buffer = "";
}
