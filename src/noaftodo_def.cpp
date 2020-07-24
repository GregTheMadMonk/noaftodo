#include <iostream>

#include "noaftodo.h"
#include "noaftodo_cmd.h"
#include "noaftodo_config.h"
#include "noaftodo_cui.h"
#include "noaftodo_cvar.h"
#include "noaftodo_time.h"

using namespace std;

namespace cmd {
	// initialize commands
	map<string, function<int(const vector<string>& args)>> cmds = {
	/*	{ command, int cmd_func(const vector<string>& args) },	*/
		// command "q" - exit the program.
		{ "q", [] (const vector<string>& args) {
				try {
					noaftodo_exit(stoi(args.at(0)));
					return 0;
				} catch (const invalid_argument& e) {
				} catch (const out_of_range& e) { }

				noaftodo_exit();
				return 0;
			}
		},

		// command "!<command>" - execute shell command
		{ "!", [] (const vector<string>& args) {
				string cmdline = "";

				for (const auto& arg : args) cmdline += arg + " ";

				const bool wcui = cui_active;
				if (wcui) cui_destroy();

				char buffer[128];
				string result = "";
				FILE* pipe = popen(cmdline.c_str(), "r");
				if (!pipe) {
					log("popen() failed", LP_ERROR);
					return ERR_EXTERNAL;
				}

				try {
					while (fgets(buffer, sizeof buffer, pipe) != NULL) {
						cout << buffer;
						result += buffer;
					}
				} catch (...) {
					pclose(pipe);
					return ERR_EXTERNAL;
				}

				if (wcui) cui_construct();

				pclose(pipe);

				result.erase(remove(result.begin(), result.end(), '\n'), result.end());
				retval = result;

				return 0;
			}
		},

		// command "!!<command>" - execute shell command, don't track output (for lauching programs that have a TUI or something
		{ "!!", [] (const vector<string>& args) {
				string cmdline = "";
				
				for (const auto& arg : args) cmdline += arg + " ";

				const bool wcui = cui_active;

				if (wcui) cui_destroy();

				log("Executing shell command: '" + cmdline + "'...");
				system(cmdline.c_str());

				if (wcui) cui_construct();

				return 0;
			}
		},

		// command "alias <command>" - create an alias for command.
		{ "alias", [] (const vector<string>& args) {
				if (args.size() < 1) return ERR_ARG_COUNT;

				if (args.size() == 1) {
					aliases.erase(args.at(0));
					return 0;
				}

				string alargs;
				for (int i = 1; i < args.size(); i++)  {
					if (alargs != "") alargs += " ";
					alargs += args.at(i);
				}

				log("Alias " + args.at(0) + " => " + alargs);

				aliases[args.at(0)] = alargs;

				return 0;
			}
		},

		// command "d" - remove selected task.
		{ "d", [] (const vector<string>& args) {
				if (t_list.size() == 0) return ERR_EXTERNAL;

				li_rem(cui_s_line);
				if (t_list.size() != 0) if (cui_s_line >= t_list.size()) exec("math %id% - 1 id");

				return 0;
			}
		},

		// command "a <due> <title> <description>[ <id>]" - add or override a task. If <id> is not specified, a new task is created. If not, a task with <id> will be overriden.
		{ "a", [] (const vector<string>& args) {
				if (args.size() < 3) return ERR_ARG_COUNT;

				try {
					noaftodo_entry new_entry;
					new_entry.completed = false;

					if (args.at(0) == "-") {
						new_entry.due = ti_to_long("a10000y");
						new_entry.meta["nodue"] = "true";
					}
					else new_entry.due = ti_to_long(args.at(0));
					new_entry.title = args.at(1);
					new_entry.description = args.at(2);

					new_entry.tag = (cui_tag_filter == CUI_TAG_ALL) ? 0 : cui_tag_filter;
					
					if (args.size() < 4) li_add(new_entry);
					else {
						const int id = stoi(args.at(3));

						if ((id >= 0) && (id < t_list.size()))  {
							new_entry.meta = t_list.at(id).meta;
							new_entry.tag = t_list.at(id).tag;
							t_list[id] = new_entry;

							if (li_autosave) li_save();
						}
						else return ERR_EXTERNAL;
					}

					// go to created task
					for (int i = 0; i < t_list.size(); i++)
						if (t_list.at(i) == new_entry) cui_s_line = i;

					return 0;
				} catch (const invalid_argument& e) {
					return ERR_ARG_TYPE;
				}
			}
		},

		// command "setmeta[ <name1> <value1>[ <name2> <value2>[ ...]]]" - set task meta. If no arguments are specified, clear task meta. To add properties to meta, use "setmeta %meta% <name1> <value1>...". "setmeta <name>" will erase only <name> meta property
		{ "setmeta", [] (const vector<string>& args) {
				if ((cui_s_line < 0) || (cui_s_line >= t_list.size())) return ERR_EXTERNAL;

				if (args.size() == 1) {
					if (args.at(0) == "eid") return ERR_EXTERNAL;
					t_list[cui_s_line].meta.erase(args.at(0));
					return 0;
				}

				const string eid = t_list[cui_s_line].get_meta("eid");
				t_list[cui_s_line].meta.clear();
				t_list[cui_s_line].meta["eid"] = eid;

				for (int i = 0; i + 1 < args.size(); i += 2)
					t_list[cui_s_line].meta[args.at(i)] = args.at(i + 1);

				if (li_autosave) li_save();

				return 0;
			}
		},

		// command "lclear" - clear list.
		{ "lclear", [] (const vector<string>& args) {
				if (cui_tag_filter == CUI_TAG_ALL) return ERR_EXTERNAL;

				for (int i = 0; i < t_list.size(); )
					if (t_list.at(i).tag == cui_tag_filter) li_rem(i);
					else i++;

				return 0;
			}
		},

		// command "bind <key> <command> <mode> <autoexec>" - bind <key> to <command>. <mode> speciifes, which modes use this bind (mask, see noaftodo_cui.h for CUI_MODE_* values). If <autoexec> is "true", execute command on key hit, otherwise just go into command mode with it. Running as just "bind <key>" removes a bind.
		{ "bind", [] (const vector<string>& args) {
				if (args.size() < 1) return ERR_ARG_COUNT;

				if (args.size() >= 4) try {
					const string skey = args.at(0);
					const string scomm = args.at(1);
					const int smode = stoi(args.at(2));
					const bool sauto = (args.at(3) == "true");

					log("Binding " + skey + " to \"" + scomm + "\"");

					cui_bind(cui_key_from_str(skey), scomm, smode, sauto);

					return 0;
				} catch (const invalid_argument& e) {
					return ERR_ARG_TYPE;
				}

				if (args.size() > 1) return ERR_ARG_COUNT;

				log("Unbinding " + args.at(0));

				bool removed = false;

				for (int i = 0; i < binds.size(); i++)
					if (cui_key_from_str(args.at(0)) == binds.at(i).key) {
						binds.erase(binds.begin() + i);
						i--;
						removed = true;
					}

				return removed ? 0 : ERR_EXTERNAL;
			}
		},

		// command "math <num1> <op> <num2>[ <name>]" - calculate math expression (+,-,/,*,=,<,>,min,max) and write to cvar <name>. If <name> is not specifed, just print the result out
		{ "math", [] (const vector<string>& args) {
				if (args.size() < 3) return ERR_ARG_COUNT;

				try {
					const double a = stod(args.at(0));
					const double b = stod(args.at(2));

					double result = 0;

					switch (args.at(1).at(0)) {
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
							if (b == 0) return ERR_ARG_TYPE;
							result = a / b;
							break;
						case 'm':
							if (args.at(1) == "max") result = (a >= b) ? a : b;
							else if (args.at(1) == "min") result = (a < b) ? a : b;
							break;
						case '=':
							if (args.size() < 4) retval = (a == b) ? "true" : "false";
							else cvar(args.at(3)).setter((a == b) ? "true" : "false");
							return 0;
						case '<':
							if (args.size() < 4) retval = (a < b) ? "true" : "false";
							else cvar(args.at(3)).setter((a < b) ? "true" : "false");
							return 0;
						case '>':
							if (args.size() > 4) retval = (a > b) ? "true" : "false";
							else cvar(args.at(3)).setter((a > b) ? "true" : "false");
							return 0;
					}

					if (args.size() < 4) retval = to_string(result);
					else cvar(args.at(3)).setter(to_string(result));
				} catch (const invalid_argument& e) { return ERR_ARG_TYPE; }

				return 0;
			}
		},

		// command "if <true|false> <do-if-true>[ <do-if-false>]" - simple if expression
		{ "if", [] (const vector<string>& args) {
				if (args.size() < 2) return ERR_ARG_COUNT;
				
				if (args.at(0) == "true") exec(args.at(1));
				else if (args.size() > 2) exec(args.at(2));

				return 0;
			}
		},

		// command "set <name>[ <value>]" - set cvar value. If <value> is not specified, reset cvar to default value.
		{ "set", [] (const vector<string>& args) {
				if (args.size() < 1) return ERR_ARG_COUNT;

				if (args.size() < 2) {
					cvar_reset(args.at(0));

					return 0;
				}

				cvar(args.at(0)) = args.at(1);
				return 0;
			}
		},

		// command "exec <filename>[ script]" - execute a config file. Execute default config with "exec default". With "script" cvars from config are not set as default
		{ "exec", [] (const vector<string>& args) {
				if (args.size() < 1) return ERR_ARG_COUNT;

				bool predef_cvars = true;

				for (int i = 1; i < args.size(); i++) {
					if (args.at(i) == "script") predef_cvars = false;
				}

				conf_load(args.at(0), predef_cvars);

				return 0;
			}
		},

		// command "ver <VERSION>" - is used to specify config version to notify about possible outdated config files.
		{ "ver", [] (const vector<string>& args) {
				if (args.size() < 1) return ERR_ARG_COUNT;

				if (args.at(0) != to_string(CONF_V)) {
					log("Config version mismatch (CONF_V " + args.at(0) + " != " + to_string(CONF_V) + "). "
						"Consult \"TROUBLESHOOTING\" manpage section (\"noaftodo -h\").", LP_ERROR);
					errors |= ERR_CONF_V;
					li_autosave = false;
				}

				return 0;
			}
		},

		// command "save[ <filename>]" - force the list save. If <filename> is not specified, override opened file.
		{ "save", [] (const vector<string>& args) {
				if (args.size() < 1) return li_save();
				else return li_save(args.at(0));
				return 0;
			}
		},

		// command "load <filename>" - load the list file
		{ "load", [] (const vector<string>& args) {
				if (args.size() < 1) li_load(true);
				else li_load(args.at(0), true);
				return 0;
			}
		},

		// command "echo[ args...]" - print the following in status.
		{ "echo", [] (const vector<string>& args) {
				string message = "";
				for (int i = 0; i < args.size(); i++) message += args.at(i) + " ";

				retval = message;
				if (!cui_active) log((pure ? "" : "echo :: ") + message, LP_IMPORTANT);
				return 0;
			}
		}
	};

	map<string, string> aliases = {
	/*	{ alias_name, alias_command },	*/
	};
}
