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

#ifdef __sun
#include <ncurses/curses.h>
# else
#include <curses.h>
#endif

#include "noaftodo.h"
#include "noaftodo_config.h"
#include "noaftodo_cui.h"
#include "noaftodo_daemon.h"
#include "noaftodo_list.h"
#include "noaftodo_io.h"
#include "noaftodo_time.h"

using namespace std;

map<string, function<int(const vector<string>& args)>> cmds;
map<string, vector<string>> aliases;

void cmd_init()
{
	// command "q" - exit the program
	cmds["q"] = [] (const vector<string>& args)
	{
		cui_mode = CUI_MODE_EXIT;
		return 0;
	};

	// command ":" - enter the command mode
	cmds[":"] = [] (const vector<string>& args)
	{
		cui_set_mode(CUI_MODE_COMMAND);
		return 0;
	};

	// command "?" - show the help message
	cmds["?"] = [] (const vector<string>& args)
	{
		cui_set_mode(CUI_MODE_HELP);
		return 0;
	};

	// command "details" - view task details
	cmds["details"] = [] (const vector<string>& args)
	{
		cui_set_mode(CUI_MODE_DETAILS);
		return 0;
	};

	// command "alias" - create an alias for command
	cmds["alias"] = [] (const vector<string>& args)
	{
		if (args.size() < 1) return CMD_ERR_ARG_COUNT;

		vector<string> alargs;
		for (int i = 1; i < args.size(); i++) alargs.push_back(args.at(i));

		aliases[args.at(0)] = alargs;

		return 0;
	};

	// command "list" - navigate to list (":list all" to view tasks from all lists)
	cmds["list"] = [] (const vector<string>& args)
	{
		if (args.size() != 1) return CMD_ERR_ARG_COUNT;
		
		if (args.at(0) == "all") conf_set_cvar_int("tag_filter", CUI_TAG_ALL);
		else try {
			const int new_filter = stoi(args.at(0));
			const int tag_filter = conf_get_cvar_int("tag_filter");

			if (new_filter == tag_filter) conf_set_cvar_int("tag_filter", CUI_TAG_ALL);
			else conf_set_cvar_int("tag_filter", new_filter);
		} catch (const invalid_argument& e) {
			return CMD_ERR_ARG_TYPE;
		}

		return 0;
	};

	// command "down" - navigate down the list
	cmds["down"] = [] (const vector<string>& args)
	{
		bool has_visible = false;
		for (int i = 0; i < t_list.size(); i++) has_visible |= cui_is_visible(i);

		if (!has_visible) return CMD_ERR_EXTERNAL;

		if (cui_s_line < t_list.size() - 1) cui_s_line++;
		else cui_s_line = 0;

		if (!cui_is_visible(cui_s_line)) cmd_exec("down");

		cui_delta = 0;

		return 0;
	};

	// command "up" - navigate up the list
	cmds["up"] = [] (const vector<string>& args)
	{
		bool has_visible = false;
		for (int i = 0; i < t_list.size(); i++) has_visible |= cui_is_visible(i);

		if (!has_visible) return CMD_ERR_EXTERNAL;

		if (cui_s_line > 0) cui_s_line--;
		else cui_s_line = t_list.size() - 1;

		if (!cui_is_visible(cui_s_line)) cmd_exec("up");

		cui_delta = 0;

		return 0;
	};

	// command "c" - toggle selected task's "completed" property
	cmds["c"] = [] (const vector<string>& args)
	{
		if (t_list.size() == 0) return CMD_ERR_EXTERNAL;

		li_comp(cui_s_line);
		return 0;
	};

	// command "d" - remove selected task
	cmds["d"] = [] (const vector<string>& args)
	{
		if (t_list.size() == 0) return CMD_ERR_EXTERNAL;

		li_rem(cui_s_line);
		if (t_list.size() != 0) if (cui_s_line >= t_list.size()) cmd_exec("up");

		return 0;
	};

	// command "a" - add a task
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

			const int tag_filter = conf_get_cvar_int("tag_filter");
			new_entry.tag = (tag_filter == CUI_TAG_ALL) ? 0 : tag_filter;
			
			if (args.size() < 4) li_add(new_entry);
			else
			{
				const int id = stoi(args.at(3));

				if ((id >= 0) && (id < t_list.size())) 
				{
					new_entry.meta = t_list.at(id).meta;
					new_entry.tag = t_list.at(id).tag;
					t_list[id] = new_entry;
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

	// command "setmeta" - set task property
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

	// command "clrmeta" - erase task metadata
	cmds["clrmeta"] = [] (const vector<string>& args)
	{
		if ((cui_s_line < 0) || (cui_s_line >= t_list.size())) return CMD_ERR_EXTERNAL;

		for (auto it = t_list[cui_s_line].meta.begin(); it != t_list[cui_s_line].meta.end(); it = t_list[cui_s_line].meta.begin())
			t_list[cui_s_line].meta.erase(it->first);

		return 0;
	};

	// command "vtoggle" - toggle filters. Supported filters: uncat, complete, coming, failed
	cmds["vtoggle"] = [] (const vector<string>& args)
	{
		if (args.size() < 1) return CMD_ERR_ARG_COUNT;

		int filter = conf_get_cvar_int("filter");
		if (args.at(0) == "uncat") 	filter ^= CUI_FILTER_UNCAT;
		if (args.at(0) == "complete") 	filter ^= CUI_FILTER_COMPLETE;
		if (args.at(0) == "coming") 	filter ^= CUI_FILTER_COMING;
		if (args.at(0) == "failed") 	filter ^= CUI_FILTER_FAILED;
		if (args.at(0) == "nodue")	filter ^= CUI_FILTER_NODUE;

		conf_set_cvar_int("filter", filter);

		return 0;
	};

	// command "g" - go to task
	cmds["g"] = [] (const vector<string>& args)
	{
		if (args.size() < 1) return CMD_ERR_ARG_COUNT;

		try {
			const int target = stoi(args.at(0));

			if ((target >= 0) && (target < t_list.size()))
				cui_s_line = target;
			else return CMD_ERR_EXTERNAL;

			return 0;
		} catch (const invalid_argument& e) {
			return CMD_ERR_ARG_TYPE;
		}
	};


	// command "lrename" - rename list
	cmds["lrename"] = [] (const vector<string>& args)
	{
		if (args.size() < 1) return CMD_ERR_ARG_COUNT;

		const int tag_filter = conf_get_cvar_int("tag_filter");
		if (tag_filter == CUI_TAG_ALL) return CMD_ERR_EXTERNAL;

		while (tag_filter >= t_tags.size()) t_tags.push_back(to_string(t_tags.size()));

		t_tags[tag_filter] = args.at(0);
		if (li_autosave) li_save();

		return 0;
	};

	// command "lclear" - clear list
	cmds["lclear"] = [] (const vector<string>& args)
	{
		const int tag_filter = conf_get_cvar_int("tag_filter");

		if (tag_filter == CUI_TAG_ALL) return CMD_ERR_EXTERNAL;

		for (int i = 0; i < t_list.size(); )
			if (t_list.at(i).tag == tag_filter) li_rem(i);
			else i++;

		return 0;
	};

	// command "lmv" - move selected task to a list
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

	// command "bind" - bind a key
	cmds["bind"] = [] (const vector<string>& args)
	{
		if (args.size() < 4) return CMD_ERR_ARG_COUNT;

		try {
			const string skey = args.at(0);
			const string scomm = args.at(1);
			const int smode = stoi(args.at(2));
			const bool sauto = (args.at(3) == "true");

			log("Binding " + skey + " to \"" + scomm + "\"");

			wchar_t key;
			if (skey.length() == 1)
				key = skey.at(0);
			else
			{
				if (skey == "up") 	key = KEY_UP;
				if (skey == "down") 	key = KEY_DOWN;
				if (skey == "left")	key = KEY_LEFT;
				if (skey == "right")	key = KEY_RIGHT;
				if (skey == "esc")	key = 27;
				if (skey == "enter")	key = 10;
			}

			cui_bind(key, scomm, smode, sauto);

			return 0;
		} catch (const invalid_argument& e) {
			return CMD_ERR_ARG_TYPE;
		}
	};

	// command "set" - set cvar value
	cmds["set"] = [] (const vector<string>& args)
	{
		if (args.size() < 1) return CMD_ERR_ARG_COUNT;

		if (args.size() < 2)
		{
			cmd_exec("reset " + args.at(0));
			return 0;
		}

		conf_set_cvar(args.at(0), args.at(1));
		return 0;
	};

	// command "reset" - reset cvar value to default
	cmds["reset"] = [] (const vector<string>& args)
	{
		if (args.size() < 1) return CMD_ERR_ARG_COUNT;

		if (conf_get_predefined_cvar(args.at(0)) != "")
			conf_set_cvar(args.at(0), conf_get_predefined_cvar(args.at(0)));
		else
			conf_cvars.erase(args.at(0));

		return 0;
	};

	// command "exec" - execute a config file
	cmds["exec"] = [] (const vector<string>& args)
	{
		if (args.size() < 1) return CMD_ERR_ARG_COUNT;

		conf_load(args.at(0));

		return 0;
	};

	// command "ver" - is used to specify config version to notify about possible outdated config files
	cmds["ver"] = [] (const vector<string>& args)
	{
		if (args.size() < 1) return CMD_ERR_ARG_COUNT;

		if (args.at(0) != to_string(CONF_V))
		{
			log("File you are trying to load is declared to be for an outdated version of NOAFtodo (CONF_V " + args.at(0) + " != " + to_string(CONF_V) + ") and might not work as expected. Fix it and restart " + TITLE + ". Exiting program now.", LP_ERROR);

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

	// command "save" - force the list save
	cmds["save"] = [] (const vector<string>& args)
	{
		if (args.size() < 1)	li_save();
		else 			li_save(args.at(0));
		return 0;
	};

	// command "echo" - print the following in status
	cmds["echo"] = [] (const vector<string>& args)
	{
		string message = "";
		for (int i = 0; i < args.size(); i++) message += args.at(i) + " ";
		cui_status = message;
		return 0;
	};
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
		} else if (skip_special) 
		{
			word += c;
			skip_special = false;
		} else switch (c) {
			case '\\':
				skip_special = true;
				break;
			case ' ':
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

	int offset = 0;
	for (int i = 0; i < words.size(); i++)
	{
		if (words.at(i) == ";") // command separator
			offset = i + 1;
		else if (i == offset)
		{
			if (words.at(i).at(0) == '!')
			{
				string shell_command = words.at(i).substr(1);

				log("Executing shell command: `" + shell_command + "`...", LP_IMPORTANT);

				system(shell_command.c_str());
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
					vector<string> oargs = aliases.at(words.at(i));

					vector<string> newargs;

					for (int j = 1; j < oargs.size(); j++) 
					{
						if ((cui_s_line >= 0) && (cui_s_line < t_list.size())) newargs.push_back(format_str(oargs.at(j), t_list.at(cui_s_line)));
						else newargs.push_back(oargs.at(j));
					}
					for (int j = 0; j < cmdarg.size(); j++) 
					{
						bool replaced = false;

						for (int k = 0; k < oargs.size() - 1; k++)
						{
							int index = -1;
							string varname = "%arg" + to_string(j + 1) + "%";
							while ((index = newargs.at(k).find(varname)) != string::npos)
							{
								newargs[k].replace(index, varname.length(), cmdarg.at(j));
								replaced = true;
							}
						}

						if (!replaced) newargs.push_back(cmdarg.at(j));
					}

					const int ret = (cmds.at(oargs.at(0)))(newargs);

					if (ret == CMD_ERR_ARG_COUNT)	cui_status = "Wrong argument count";
					if (ret == CMD_ERR_ARG_TYPE)	cui_status = "Wrong argument type";
					if (ret == CMD_ERR_EXTERNAL)	cui_status = "Cannot execute command";
				} catch (const out_of_range& e) { // alias not found, try to execute a command
					try {
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
