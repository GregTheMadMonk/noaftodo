#include <iostream>

#include <noaftodo.h>
#include <cmd.h>
#include <config.h>
#include <cui.h>
#include <cvar.h>
#include <daemon.h>
#include <entry_flags.h>
#include <core/time.h>

using namespace std;

using cui::s_line;
using cui::tag_filter;

// CMD DEFINITIONS
namespace cmd {

using li::t_list;
using li::t_tags;

bool allow_system_commands = true;

// initialize commands
map<string, function<int(const vector<string>& args)>> cmds = {
/*	{ command, int cmd_func(const vector<string>& args) },	*/
	// @command "q" - exit the program.
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

	// @command "!<command>" - execute shell command
	{ "!", [] (const vector<string>& args) {
			if (!allow_system_commands) {
				log("%allow_system_commands% is false. Skipping...", LP_ERROR);
				return ERR_EXTERNAL;
			}

			string cmdline = "";

			for (const auto& arg : args) cmdline += arg + " ";

			const bool wcui = cui::active;
			if (wcui) cui::destroy();

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

			if (wcui) cui::construct();

			pclose(pipe);

			result.erase(remove(result.begin(), result.end(), '\n'), result.end());
			ret(result);

			return 0;
		}
	},

	// @command "!!<command>" - execute shell command, don't track output (for lauching programs that have a TUI or something
	{ "!!", [] (const vector<string>& args) {
			if (!allow_system_commands) {
				log("%allow_system_commands% is false. Skipping...", LP_ERROR);
				return ERR_EXTERNAL;
			}

			string cmdline = "";
			
			for (const auto& arg : args) cmdline += arg + " ";

			const bool wcui = cui::active;

			if (wcui) cui::destroy();

			log("Executing shell command: '" + cmdline + "'...");
			system(cmdline.c_str());

			if (wcui) cui::construct();

			return 0;
		}
	},

	// @command "alias <command>" - create an alias for command.
	{ "alias", [] (const vector<string>& args) {
			if (args.size() < 1) return ERR_ARG_COUNT;

			if (args.size() == 1) {
				aliases.erase(args.at(0));
				upd_cvars();
				return 0;
			}

			string alargs;
			for (int i = 1; i < args.size(); i++)  {
				if (alargs != "") alargs += " ";
				alargs += args.at(i);
			}

			log("Alias " + args.at(0) + " => " + alargs);

			aliases[args.at(0)] = alargs;

			upd_cvars();
			return 0;
		}
	},

	// @command "d" - remove selected task.
	{ "d", [] (const vector<string>& args) {
			if (t_list.size() == 0) return ERR_EXTERNAL;

			li::rem(s_line);
			if (t_list.size() != 0) if (s_line >= t_list.size()) exec("math %id% - 1 id");

			return 0;
		}
	},

	// @command "a <due> <title> <description>[ <id>]" - add or override a task. If <id> is not specified, a new task is created. If not, a task with <id> will be overriden.
	{ "a", [] (const vector<string>& args) {
			if (args.size() < 3) return ERR_ARG_COUNT;

			try {
				li::entry new_entry;
				new_entry.completed = false;

				if (args.at(0) == "-") {
					new_entry.due = time_s("a10000y");
					new_entry.meta["nodue"] = "true";
				}
				else {
					const auto del = args.at(0).find("+");
					if (del == string::npos) new_entry.due = time_s(args.at(0));
					else {
						new_entry.due = time_s(args.at(0).substr(0, del));
						new_entry.meta["duration"] = args.at(0).substr(del + 1);
					}
				}
				new_entry.title = args.at(1);
				new_entry.description = args.at(2);

				new_entry.tag = (tag_filter == cui::TAG_ALL) ? 0 : tag_filter;
				
				if (args.size() < 4) li::add(new_entry);
				else {
					const int id = stoi(args.at(3));

					if ((id >= 0) && (id < t_list.size()))  {
						new_entry.meta = t_list.at(id).meta;
						new_entry.tag = t_list.at(id).tag;
						t_list[id] = new_entry;

						if (li::autosave) li::save();
					}
					else return ERR_EXTERNAL;
				}

				// go to created task
				for (int i = 0; i < t_list.size(); i++)
					if (t_list.at(i) == new_entry) s_line = i;

				return 0;
			} catch (const invalid_argument& e) {
				return ERR_ARG_TYPE;
			}
		}
	},

	// @command "setmeta[ <name1> <value1>[ <name2> <value2>[ ...]]]" - set task meta. If no arguments are specified, clear task meta. To add properties to meta, use "setmeta %meta% <name1> <value1>...". "setmeta <name>" will erase only <name> meta property
	{ "setmeta", [] (const vector<string>& args) {
			if ((s_line < 0) || (s_line >= t_list.size())) return ERR_EXTERNAL;

			if (args.size() == 1) {
				if (args.at(0) == "eid") return ERR_EXTERNAL;
				t_list[s_line].meta.erase(args.at(0));
				return 0;
			}

			const string eid = t_list[s_line].get_meta("eid");
			t_list[s_line].meta.clear();
			t_list[s_line].meta["eid"] = eid;

			for (int i = 0; i + 1 < args.size(); i += 2)
				t_list[s_line].meta[args.at(i)] = args.at(i + 1);

			if (li::autosave) li::save();

			return 0;
		}
	},

	// @command "lclear" - clear list.
	{ "lclear", [] (const vector<string>& args) {
			if (tag_filter == cui::TAG_ALL) return ERR_EXTERNAL;

			for (int i = 0; i < t_list.size(); )
				if (t_list.at(i).tag == tag_filter) li::rem(i);
				else i++;

			return 0;
		}
	},

	// @command "bind <key> <command> <mode> <autoexec>" - bind <key> to <command>. <mode> speciifes, which modes use this bind (mask, see noaftodo_cui.h for MODE_* values). If <autoexec> is "true", execute command on key hit, otherwise just go into command mode with it. Running as just "bind <key>" removes a bind.
	{ "bind", [] (const vector<string>& args) {
			if (args.size() < 1) return ERR_ARG_COUNT;

			if (args.size() >= 4) try {
				const string skey = args.at(0);
				const string scomm = args.at(1);
				const int smode = stoi(args.at(2));
				const bool sauto = (args.at(3) == "true");

				log("Binding " + skey + " to \"" + scomm + "\"");

				cui::bind(cui::key_from_str(skey), scomm, smode, sauto);

				return 0;
			} catch (const invalid_argument& e) {
				return ERR_ARG_TYPE;
			}

			if (args.size() > 1) return ERR_ARG_COUNT;

			log("Unbinding " + args.at(0));

			bool removed = false;

			for (int i = 0; i < cui::binds.size(); i++)
				if (cui::key_from_str(args.at(0)) == cui::binds.at(i).key) {
					cui::binds.erase(cui::binds.begin() + i);
					i--;
					removed = true;
				}

			return removed ? 0 : ERR_EXTERNAL;
		}
	},

	// @command "math <num1> <op> <num2>[ <name>]" - calculate math expression (+,-,/,*,=,<,>,min,max) and write to cvar <name>. If <name> is not specifed, just print the result out
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
						if (args.size() < 4) ret((a == b) ? "true" : "false");
						else cvar(args.at(3)).setter((a == b) ? "true" : "false");
						return 0;
					case '<':
						if (args.size() < 4) ret((a < b) ? "true" : "false");
						else cvar(args.at(3)).setter((a < b) ? "true" : "false");
						return 0;
					case '>':
						if (args.size() > 4) ret((a > b) ? "true" : "false");
						else cvar(args.at(3)).setter((a > b) ? "true" : "false");
						return 0;
				}

				if (args.size() < 4) ret(to_string(result));
				else cvar(args.at(3)).setter(to_string(result));
			} catch (const invalid_argument& e) { return ERR_ARG_TYPE; }

			return 0;
		}
	},

	// @command "if <true|false> <do-if-true>[ <do-if-false>]" - simple if expression
	{ "if", [] (const vector<string>& args) {
			if (args.size() < 2) return ERR_ARG_COUNT;
			
			if (args.at(0) == "true") exec(args.at(1));
			else if (args.size() > 2) exec(args.at(2));

			return 0;
		}
	},

	// @command "set <name>[ <value>]" - set cvar value. If <value> is not specified, reset cvar to default value.
	{ "set", [] (const vector<string>& args) {
			if (args.size() < 1) return ERR_ARG_COUNT;

			if (args.size() < 2) {
				cvar_base_s::reset(args.at(0));

				return 0;
			}

			if (args.at(0).find("meta.") == 0) {
				try {
					cvar_base_s::cvars.at(args.at(0))->setter(args.at(1));
				} catch (const out_of_range& e) {
					sel_entry->meta[args.at(0).substr(5)] = args.at(1);
					select_entry(sel_entry); // will regenerate meta variables
					li::save();
				}
			} else if (args.at(0).find("fields.status.") == 0) {
				try {
					cvar_base_s::cvars.at(args.at(0))->setter(args.at(1));
				} catch (const out_of_range& e) {
					cui::status_fields[args.at(0).substr(14).at(0)] = [args] () {
						return format_str(args.at(1), sel_entry);
					};
					upd_cvars();
				}
			} else cvar(args.at(0)) = args.at(1);

			return 0;
		}
	},

	// @command "exec <filename>[ script]" - execute a config file. Execute default config with "exec default". With "script" cvars from config are not set as default
	{ "exec", [] (const vector<string>& args) {
			if (args.size() < 1) return ERR_ARG_COUNT;

			bool predef_cvars = true;

			for (int i = 1; i < args.size(); i++) {
				if (args.at(i) == "script") predef_cvars = false;
			}

			conf::load(args.at(0), predef_cvars);

			return 0;
		}
	},

	// @command "ver <VERSION>" - is used to specify config version to notify about possible outdated config files.
	{ "ver", [] (const vector<string>& args) {
			if (args.size() < 1) return ERR_ARG_COUNT;

			if (args.at(0) != to_string(CONF_V)) {
				log("Config version mismatch (CONF_V " + args.at(0) + " != " + to_string(CONF_V) + "). "
					"Consult \"TROUBLESHOOTING\" manpage section (\"noaftodo -h\").", LP_ERROR);
				errors |= ERR_CONF_V;
				li::autosave = false;
			}

			return 0;
		}
	},

	// @command "save[ <filename>]" - force the list save. If <filename> is not specified, override opened file.
	{ "save", [] (const vector<string>& args) {
			if (args.size() < 1) return li::save();
			else return li::save(args.at(0));
			return 0;
		}
	},

	// @command "load <filename>" - load the list file
	{ "load", [] (const vector<string>& args) {
			if (args.size() < 1) li::load(true);
			else li::load(args.at(0), true);
			return 0;
		}
	},

	// @command "echo[ args...]" - print the following in status.
	{ "echo", [] (const vector<string>& args) {
			string message = "";
			for (int i = 0; i < args.size(); i++) message += args.at(i) + " ";

			ret(message);
			if (!cui::active) log((pure ? "" : "echo :: ") + message, LP_IMPORTANT);
			return 0;
		}
	},
};

map<string, string> aliases = {
/*	{ alias_name, alias_command },	*/
};

// INIT CVARS
void init_cvars() {
	// This function cannot be replaced with an initializer list because
	// you cant create uniue_ptr's in them, and I can't be bothered with
	// figuring out how to make it work. It's ust not worth it

	// CORE CVARS
	// @cvar "allow_root" - allows program to be run as root user on UNIX systems. Will do nothing on single-user systems like Haiku (check is disabled there)
	cvar_base_s::cvars["allow_root"] = cvar_base_s::wrap_bool(allow_root);
	// @cvar "allow_system_commands" - allow host system commands to be executed via "!" and "!!" commands
	cvar_base_s::cvars["allow_system_commands"] = cvar_base_s::wrap_bool(allow_system_commands);
	// @cvar "autorun_daemon" - automatically start/stop a background daemon (if not already runnning) on program start/quit
	cvar_base_s::cvars["autorun_daemon"] = cvar_base_s::wrap_bool(da::fork_autostart);

	// @cvar "mode" - program UI mode integer
	cvar_base_s::cvars["mode"] =
		cvar_base_s::wrap_int(cui::mode, CVAR_FLAG_NO_PREDEF | CVAR_FLAG_WS_IGNORE,
			{},
			[] (const string& val) {
				try {
					cui::set_mode(stoi(val));
					return 0;
				} catch (const invalid_argument& e) { return ERR_ARG_TYPE; }
			},
			"-1");

	// ENTRY FIELDS
	// @cvar "id" - selected entry ID (as displayed in NORMAL mode, don't confuse with entry EIDs)
	cvar_base_s::cvars["id"] =
		cvar_base_s::wrap_int(s_line, CVAR_FLAG_NO_PREDEF,
			{},
			[] (const string& val) {
				bool is_visible = false;
				for (int i = 0; i < t_list.size(); i++) is_visible |= cui::is_visible(i);
				if (!is_visible) return; // don't try to set the ID when there's nothing on the screen

				try {
					const int target = stoi(val);

					const int dir = (target - s_line >= 0) ? 1 : -1;

					if ((target >= 0) && (target < t_list.size()))
						s_line = target;
					else if (target < 0) s_line = t_list.size() - 1;
					else s_line = 0;

					if (cui::active)	while (!cui::is_visible(s_line))  {
						s_line += dir;

						if (s_line < 0) s_line = t_list.size() - 1;
						else if (s_line >= t_list.size()) s_line = 0;
					}

					sel_entry = &t_list.at(s_line);
				} catch (const invalid_argument& e) {}
			},
			"0");

	// @cvar "title" - selected entry's title
	cvar_base_s::cvars["title"] = make_unique<cvar_base_s>(
		[] () {
			if (sel_entry == nullptr)  return string("");
			return sel_entry->title;
		},
		[] (const string& val) {
			if (sel_entry == nullptr) return;
			sel_entry->title = val;
			if (li::autosave) li::save();
		},
		CVAR_FLAG_NO_PREDEF | CVAR_FLAG_WS_IGNORE
	);

	// @cvar "desc" - selected entry's description
	cvar_base_s::cvars["desc"] = make_unique<cvar_base_s>(
		[] () {
			if (sel_entry == nullptr)  return string("");
			return sel_entry->description;
		},
		[] (const string& val) {
			if (sel_entry == nullptr) return;
			sel_entry->description = val;
			if (li::autosave) li::save();
		},
		CVAR_FLAG_NO_PREDEF | CVAR_FLAG_WS_IGNORE
	);

	// @cvar "due" - selected entry's due (in command-line format)
	cvar_base_s::cvars["due"] = make_unique<cvar_base_s>(
		[] () {
			if (sel_entry == nullptr)  return string("");
			return sel_entry->due.fmt_cmd();
		},
		[] (const string& val) {
			if (sel_entry == nullptr) return;
			sel_entry->due = time_s(val);
			if (li::autosave) li::save();
		},
		CVAR_FLAG_NO_PREDEF | CVAR_FLAG_WS_IGNORE
	);

	// @cvar "meta" - selected entry's meta string. Read-only. Access individual meta properties via%meta.property_name% cvars and :setmeta command
	cvar_base_s::cvars["meta"] = make_unique<cvar_base_s>(
		[] () {
			if (sel_entry == nullptr)  return string("");
			return sel_entry->meta_str();
		},
		[] (const string& val) { },
		CVAR_FLAG_NO_PREDEF | CVAR_FLAG_WS_IGNORE
	);

	// @cvar "comp" - selected entrypletion (true or false)
	cvar_base_s::cvars["comp"] = make_unique<cvar_base_s>(
		[] ()  {
			return (sel_entry == nullptr) ? "false" : (sel_entry->completed ? "true" : "false" );
		},
		[] (const string& val) {
			if (sel_entry == nullptr) return;

			if ((sel_entry->completed && (val != "true")) ||
				(!sel_entry->completed && (val == "true")))
					li::comp(s_line);
		},
		CVAR_FLAG_NO_PREDEF | CVAR_FLAG_WS_IGNORE,
		"false");

	// @cvar "parent" - selected entry's parent list index
	cvar_base_s::cvars["parent"] = make_unique<cvar_base_s>(
		[] () {
			if (sel_entry == nullptr)  return string("");
			return to_string(sel_entry->tag);
		},
		[] (const string& val) {
			if (sel_entry == nullptr) return;
			try {
				sel_entry->tag = stoi(val);
				if (li::autosave) li::save();
			} catch (const invalid_argument& e) { }
		},
		CVAR_FLAG_NO_PREDEF | CVAR_FLAG_WS_IGNORE
	);

	// @cvar "pname" - selected list's name. If no list is selected, read-only and returns "All lists"
	cvar_base_s::cvars["pname"] = make_unique<cvar_base_s>( // parent [list] name
		[] ()  {
			if (cui::tag_filter == cui::TAG_ALL) return string("All lists");

			if (cui::tag_filter < t_tags.size()) return t_tags.at(cui::tag_filter);

			return to_string(cui::tag_filter);
		},
		[] (const string& val) {
			if (cui::tag_filter < 0) return;

			while (cui::tag_filter >= t_tags.size()) t_tags.push_back(to_string(t_tags.size()));

			t_tags[cui::tag_filter] = val;

			if (li::autosave) li::save();
		},
		CVAR_FLAG_NO_PREDEF | CVAR_FLAG_WS_IGNORE
	);

	// @cvar "tag_filter" - current list's index. -1 for all lists
	cvar_base_s::cvars["tag_filter"] = cvar_base_s::wrap_int(cui::tag_filter, CVAR_FLAG_NO_PREDEF,
			{},
			[] (const string& val) {
				try {
					if (val == "all")  {
						cui::tag_filter = cui::TAG_ALL;
						return;
					}

					const int new_filter = stoi(val);

					if (new_filter == cui::tag_filter) cui::tag_filter = cui::TAG_ALL;
					else cui::tag_filter = new_filter;

					cvar_base_s::cvars["pname"]->predefine(to_string(cui::tag_filter));
				} catch (const invalid_argument& e) {}
			},
			"all");

	// @cvar "tag_filter_v" - same as %tag_filter%, but if set to an invisible list's index, will cycle through the lists until the first visible one
	cvar_base_s::cvars["tag_filter_v"] = cvar_base_s::wrap_int(cui::tag_filter, CVAR_FLAG_NO_PREDEF | CVAR_FLAG_WS_IGNORE,
			{},
			[] (const string& val) {
				if (val == "all") {
					cui::tag_filter = cui::TAG_ALL;
					return;
				}

				try {
					const int new_filter = stoi(val);
					const int dir = (new_filter >= cui::tag_filter) ? 1 : -1;

					cui::tag_filter = new_filter;

					while (!cui::l_is_visible(cui::tag_filter)) {
						cui::tag_filter += dir;

						if (cui::tag_filter >= (int)t_tags.size()) {
							// without (int) returns true
							// with any negative cui::tag_filter
							cui::tag_filter = cui::TAG_ALL;
							return;
						}

						if (cui::tag_filter < cui::TAG_ALL)
							cui::tag_filter = t_tags.size() - 1;
					}

					cvar_base_s::cvars["pname"]->predefine(to_string(cui::tag_filter));
				} catch (const invalid_argument& e) {}
			},
			"-1");

	// @cvar "filter" - entry flag filter integer. Will be deprecated soon
	cvar_base_s::cvars["filter"] = cvar_base_s::wrap_int(cui::filter);

	// @cvar "sort_by" - string indicating enrtry sort order for NORMAL mode
	cvar_base_s::cvars["sort_by"] = cvar_base_s::wrap_string(li::sort_order, 0,
			{},
			[] (const string& val) {
				li::sort_order = "";

				for (const char& c : val)
					if (li::sort_order.find(c) == string::npos) li::sort_order += c;

				if (li::sort_order.length() > 4) li::sort_order = li::sort_order.substr(0, 4);
				li::sort();
			}
		);

	// FILTER BITS
	// @cvar "filter.uncat" - show/hide uncategorized entries
	cvar_base_s::cvars["filter.uncat"] = cvar_base_s::wrap_maskflag(cui::filter, cui::FILTER_UNCAT, CVAR_FLAG_WS_IGNORE);
	// @cvar "filter.complete" - show/hide completed enties
	cvar_base_s::cvars["filter.complete"] = cvar_base_s::wrap_maskflag(cui::filter, cui::FILTER_COMPLETE, CVAR_FLAG_WS_IGNORE);
	// @cvar "filter.coming" - show/hide upcoming enties
	cvar_base_s::cvars["filter.coming"] = cvar_base_s::wrap_maskflag(cui::filter, cui::FILTER_COMING, CVAR_FLAG_WS_IGNORE);
	// @cvar "filter.failed" - show/hide failed enties
	cvar_base_s::cvars["filter.failed"] = cvar_base_s::wrap_maskflag(cui::filter, cui::FILTER_FAILED, CVAR_FLAG_WS_IGNORE);
	// @cvar "filter.nodue" - show/hide nodue enties
	cvar_base_s::cvars["filter.nodue"] = cvar_base_s::wrap_maskflag(cui::filter, cui::FILTER_NODUE, CVAR_FLAG_WS_IGNORE);
	// @cvar "filter.empty" - show/hide lists without visible entries
	cvar_base_s::cvars["filter.empty"] = cvar_base_s::wrap_maskflag(cui::filter, cui::FILTER_EMPTY);

	// UI SETUP
	// @cvar "halfdelay_time" - screen update time. Can be sets to 0 for no automatic screen updates
	cvar_base_s::cvars["halfdelay_time"] = cvar_base_s::wrap_int(cui::halfdelay_time);

	// @cvar "frameshift_multistr" - animate multistr cvars (even I don't really know how this works)
	cvar_base_s::cvars["frameshift_multistr"] = cvar_base_s::wrap_bool(cui::shift_multivars);


	// CHARACTER SET
	// @cvar "charset.separators" - separators (3 characters - between rows, status fields, DETAILS mode fields)
	cvar_base_s::cvars["charset.separators"] = cvar_base_s::wrap_multistr(cui::separators, 3);
	// @cvar "charset.box_strong" - thick box drawing characters (vertical line, horizontal line, left top, right top, left bottom, right bottom corners)
	cvar_base_s::cvars["charset.box_strong"] = cvar_base_s::wrap_multistr(cui::box_strong, 6);
	// @cvar "charset.box_light" - thin box drawing characters (vertical line, horizontal line, left top, right top, left bottom, right bottom corners)
	cvar_base_s::cvars["charset.box_light"] = cvar_base_s::wrap_multistr(cui::box_light, 6);

	// SINGLE CHARACTER WRAPPERS
	// @cvar "charset.separators.row" - NORMAL mode separator between rows
	cvar_base_s::cvars["charset.separators.row"] = cvar_base_s::wrap_multistr_element(cui::separators, cui::CHAR_ROW_SEP);
	// @cvar "charset.separators.status" - separator between status fields
	cvar_base_s::cvars["charset.separators.status"] = cvar_base_s::wrap_multistr_element(cui::separators, cui::CHAR_STA_SEP);

	// @cvar "charset.box_strong.v" - thick box vertical line character
	cvar_base_s::cvars["charset.box_strong.v"] = cvar_base_s::wrap_multistr_element(cui::box_strong, cui::CHAR_VLINE);
	// @cvar "charset.box_strong.h" - thick box horizontal line character
	cvar_base_s::cvars["charset.box_strong.h"] = cvar_base_s::wrap_multistr_element(cui::box_strong, cui::CHAR_HLINE);
	// @cvar "charset.box_strong.corn1" - thick box top left corner character
	cvar_base_s::cvars["charset.box_strong.corn1"] = cvar_base_s::wrap_multistr_element(cui::box_strong, cui::CHAR_CORN1);
	// @cvar "charset.box_strong.corn2" - thick box top right corner character
	cvar_base_s::cvars["charset.box_strong.corn2"] = cvar_base_s::wrap_multistr_element(cui::box_strong, cui::CHAR_CORN2);
	// @cvar "charset.box_strong.corn3" - thick box bottom left corner character
	cvar_base_s::cvars["charset.box_strong.corn3"] = cvar_base_s::wrap_multistr_element(cui::box_strong, cui::CHAR_CORN3);
	// @cvar "charset.box_strong.corn4" - thick box bottom right corner character
	cvar_base_s::cvars["charset.box_strong.corn4"] = cvar_base_s::wrap_multistr_element(cui::box_strong, cui::CHAR_CORN4);
	// @cvar "charset.box_light.v" - thin box vertical line character
	cvar_base_s::cvars["charset.box_light.v"] = cvar_base_s::wrap_multistr_element(cui::box_light, cui::CHAR_VLINE);
	// @cvar "charset.box_light.h" - thin box horizontal line character
	cvar_base_s::cvars["charset.box_light.h"] = cvar_base_s::wrap_multistr_element(cui::box_light, cui::CHAR_HLINE);
	// @cvar "charset.box_ligth.corn1" - thin box top left corner character
	cvar_base_s::cvars["charset.box_light.corn1"] = cvar_base_s::wrap_multistr_element(cui::box_light, cui::CHAR_CORN1);
	// @cvar "charset.box_ligth.corn2" - thin box top right corner character
	cvar_base_s::cvars["charset.box_light.corn2"] = cvar_base_s::wrap_multistr_element(cui::box_light, cui::CHAR_CORN2);
	// @cvar "charset.box_ligth.corn3" - thin box bottom left corner character
	cvar_base_s::cvars["charset.box_light.corn3"] = cvar_base_s::wrap_multistr_element(cui::box_light, cui::CHAR_CORN3);
	// @cvar "charset.box_ligth.corn4" - thin box bottom right corner character
	cvar_base_s::cvars["charset.box_light.corn4"] = cvar_base_s::wrap_multistr_element(cui::box_light, cui::CHAR_CORN4);

	// CHARACTER SET: MISC
	// @cvar "charset.separators.row.offset" - how much to shift %charset.separators.row% offset each time it is printed
	cvar_base_s::cvars["charset.separators.row.offset"] = cvar_base_s::wrap_int(cui::row_separator_offset);

	// COLORSCHEME
	// @cvar "colors.title" - table title color
	cvar_base_s::cvars["colors.title"] = cvar_base_s::wrap_int(cui::color_title);
	// @cvar "colors.status" - status bar color
	cvar_base_s::cvars["colors.status"] = cvar_base_s::wrap_int(cui::color_status);
	// @cvar "colors.entry_completed" - completed entry color
	cvar_base_s::cvars["colors.entry_completed"] = cvar_base_s::wrap_int(cui::color_complete);
	// @cvar "colors.entry_coming" - upcoming entry color
	cvar_base_s::cvars["colors.entry_coming"] = cvar_base_s::wrap_int(cui::color_coming);
	// @cvar "colors.entry_due" - due entry color
	cvar_base_s::cvars["colors.entry_due"] = cvar_base_s::wrap_int(cui::color_due);
	// @cvar "colors.entry_failed" - failed entry color
	cvar_base_s::cvars["colors.entry_failed"] = cvar_base_s::wrap_int(cui::color_failed);

	// DAEMON ACTIONS
	// @cvar "on_daemon_launch_action" - command to run when daemon is started
	cvar_base_s::cvars["on_daemon_launch_action"] = cvar_base_s::wrap_string(da::launch_action);
	// @cvar "on_task_failed_action" - command to run when a task becomes failed
	cvar_base_s::cvars["on_task_failed_action"] = cvar_base_s::wrap_string(da::task_failed_action);
	// @cvar "on_task_due_action" - command to run when a task due comes
	cvar_base_s::cvars["on_task_due_action"] = cvar_base_s::wrap_string(da::task_due_action);
	// @cvar "on_task_coming_action" - command to run when task becomes upcoming
	cvar_base_s::cvars["on_task_coming_action"] = cvar_base_s::wrap_string(da::task_coming_action);
	// @cvar "on_task_completed_action" - command to run when a task becomes completed
	cvar_base_s::cvars["on_task_completed_action"] = cvar_base_s::wrap_string(da::task_completed_action);
	// @cvar "on_task_uncompleted_action" - command to run when task stops being completed
	cvar_base_s::cvars["on_task_uncompleted_action"] = cvar_base_s::wrap_string(da::task_uncompleted_action);
	// @cvar "on_task_new_action" - command to run when a new task is added
	cvar_base_s::cvars["on_task_new_action"] = cvar_base_s::wrap_string(da::task_new_action);
	// @cvar "on_task_edited_action" - command to run if a task was edited
	cvar_base_s::cvars["on_task_edited_action"] = cvar_base_s::wrap_string(da::task_edited_action);
	// @cvar "on_task_removed_action" - command to run when a task is removed
	cvar_base_s::cvars["on_task_removed_action"] = cvar_base_s::wrap_string(da::task_removed_action);

	// HELPER CVARS
	// @cvar "lat_v_id" - the id of the last entry that is visible. Read-only
	cvar_base_s::cvars["last_v_id"] = make_unique<cvar_base_s>(
		[] ()  {
			int ret = -1;
			for (int i = 0; i < t_list.size(); i++)
				if (cui::is_visible(i)) ret = i;

			return to_string(ret);
		},
		[] (const string& val) {},
		CVAR_FLAG_RO | CVAR_FLAG_WS_IGNORE | CVAR_FLAG_NO_PREDEF
	);

	// @cvar "first_v_id" - the id of the first entry that is visible. Read-only
	cvar_base_s::cvars["first_v_id"] = make_unique<cvar_base_s>(
		[] () {
			for (int i = 0; i < t_list.size(); i++)
				if (cui::is_visible(i)) return to_string(i);

			return string("-1");
		},
		[] (const string& val) {},
		CVAR_FLAG_RO | CVAR_FLAG_WS_IGNORE | CVAR_FLAG_NO_PREDEF
	);

	// @cvar "last_v_list" - the index of the last visible list
	cvar_base_s::cvars["last_v_list"] = make_unique<cvar_base_s>(
		[] () {
			int ret = -1;
			for (int i = 0; i < t_tags.size(); i++)
				if (cui::l_is_visible(i)) ret = i;

			return to_string(ret);
		},
		[] (const string& val) {},
		CVAR_FLAG_RO | CVAR_FLAG_WS_IGNORE | CVAR_FLAG_NO_PREDEF
	);

	// @cvar "first_v_list" - the index of the first visible list
	cvar_base_s::cvars["first_v_list"] = make_unique<cvar_base_s>(
		[] ()  {
			for (int i = 0; i < t_tags.size(); i++)
				if (cui::l_is_visible(i)) return to_string(i);

			return string("-1");
		},
		[] (const string& val) {},
		CVAR_FLAG_RO | CVAR_FLAG_WS_IGNORE | CVAR_FLAG_NO_PREDEF
	);

	// @cvar "numbuffer" - service number buffer
	cvar_base_s::cvars["numbuffer"] = cvar_base_s::wrap_int(cui::numbuffer, CVAR_FLAG_WS_IGNORE | CVAR_FLAG_NO_PREDEF,
		{},
		{},
		"-1");

	// @cvar "ret" - previous command return value
	cvar_base_s::cvars["ret"] = cvar_base_s::wrap_string(retval, CVAR_FLAG_WS_IGNORE | CVAR_FLAG_NO_PREDEF);

	// @cvar "VER" - program version
	cvar_base_s::cvars["VER"] = make_unique<cvar_base_s>(
		[] () { return VERSION; },
		[] (const string& val) {},
		CVAR_FLAG_RO | CVAR_FLAG_WS_IGNORE | CVAR_FLAG_NO_PREDEF
	);
}

}

namespace da {

bool fork_autostart = true;

string launch_action;
string task_failed_action;
string task_due_action;
string task_coming_action;
string task_completed_action;
string task_uncompleted_action;
string task_new_action;
string task_edited_action;
string task_removed_action;

}

// CUI DEFINITIONS
namespace cui {

using li::t_list;
using li::t_tags;

using namespace li::entry_flags;

using namespace vargs::cols;

// NORMAL mode colums
map<char, col_s> columns = {
	/*	{ symbol, { title, width_func, content_func } }	*/
		// @NORMAL column "t" - shows task title
		{ 't',	{
				"Title",
				[](const int& w, const int& free, const int& cols) {
					return free / 4;
				},
				[](const varg& args) {
					return get<normal>(args).e.title;
				}
			}
		},
		// @NORMAL column "f" - shows task flags
		{ 'f',	{
				"Flags",
				[](const int& w, const int& free, const int& cols) {
					return 5;
				},
				[](const varg& args) {
					string ret = "";
					const auto& e = get<normal>(args).e;

					for (auto it = entry_flag::flags().begin(); it != entry_flag::flags().end(); it++)
						if (it->second(e, time_s())) ret += string(&it->first);

					return ret;
				}
			}
		},
		// @NORMAL column "l" - shows what list the task is attached to
		{ 'l',	{
				"List",
				[](const int& w, const int& free, const int& cols) {
					return free / 10;
				},
				[](const varg& args) {
					const auto& e = get<normal>(args).e;
					if (e.tag < t_tags.size())
					       if (t_tags.at(e.tag) != to_string(e.tag))
						       return to_string(e.tag) + ": " + t_tags.at(e.tag);

					return to_string(e.tag);
				}
			}
		},
		// @NORMAL column "d" - shows task due
		{ 'd',	{
				"Due",
				[](const int& w, const int& free, const int& cols) {
					return 16;
				},
				[](const varg& args) {
					const auto& e = get<normal>(args).e;
					if (is_nodue(e)) return string("----------------");
					return e.due.fmt_ui();
				}
			}
		},
		// @NORMAL column "r" - remaining time until task due
		{ 'r', {
			       "Due in",
			       [](const int& w, const int& free, const int& cols) {
				       return 18;
			       },
			       [](const varg& args) {
					const auto& e = get<normal>(args).e;
					if (is_nodue(e)) return string("------------------");

					const auto diff = e.due - time_s();

					char buffer[16];
					sprintf(buffer, "%04d/%02d/%02d %02d:%02d",
							diff.diff.tm_year,
							diff.diff.tm_mon,
							diff.diff.tm_mday,
							diff.diff.tm_hour,
							diff.diff.tm_min);
				       return (diff.sign ? "- " : "+ ") + string(buffer);
			       }
		       }
		},
		// @NORMAL column "D" - shows task description
		{ 'D',	{
				"Description",
				[](const int& w, const int& free, const int& cols) {
					return free;
				},
				[](const varg& args) {
					return get<normal>(args).e.description;
				}
			}
		},
		// @NORMAL column "i" - shows task ID (as referred by UI, don't confuse with task unique ID)
		{ 'i',	{
				"ID",
				[](const int& w, const int& free, const int& cols) {
					return 3;
				},
				[](const varg& args) {
					return to_string(get<normal>(args).id);
				}
			}
		}
	};

// LISTVIEW mode colums
map<char, col_s> lview_columns = {
	/*	{ symbol, { title, width_func, content_func } }	*/
		// @LISTVIEW column "i" - shows list number (aka ID)
		{ 'i',	{
				"ID",
				[](const int& w, const int& free, const int& cols) {
					return 3;
				},
				[](const varg& args) {
					const auto& list_id = get<lview>(args).l_id;
					if (list_id == -1) return string(" ");
					return to_string(list_id);
				}
			}
		},
		// @LISTVIEW column "f" - shows list flags
		{ 'f',	{
				"Flags",
				[] (const int& w, const int& free, const int& cols) {
					return 5;
				},
				[] (const varg& args) {
					const auto& list_id = get<lview>(args).l_id;
					return string(li::tag_completed(list_id) ? "V" : "") +
						string(li::tag_coming(list_id) ? "C" : "") +
						string(li::tag_failed(list_id) ? "F" : "");
				}
			}
		},
		// @LISTVIEW column "t" - shows list title
		{ 't',	{
				"Title",
				[](const int& w, const int& free, const int& cols) {
					return free / 4;
				},
				[](const varg& args) {
					const auto& list_id = get<lview>(args).l_id;
					if (list_id == TAG_ALL) return string("All lists");
					return t_tags.at(list_id);
				}
			}
		},
		// @LISTVIEW column "e" - shows the amount of entries attached to the list
		{ 'e',	{
				"Entries",
				[](const int& w, const int& free, const int& cols) {
					return 7;
				},
				[](const varg& args) {
					const auto& list_id = get<lview>(args).l_id;
					if (list_id == TAG_ALL) return to_string(t_list.size());

					int ret = 0;
					for (auto e : t_list) if (e.tag == list_id) ret++;

					return to_string(ret);
				}
			}
		},
		// @LISTVIEW column "p" - shows the precentge of completed entries on the list
		{ 'p',	{
				"%",
				[](const int& w, const int& free, const int& cols) {
					return 4;
				},
				[](const varg& args) {
					const auto& list_id = get<lview>(args).l_id;
					int ret = 0;
					int tot = 0;

					for (auto e : t_list)
						if ((e.tag == list_id) || (TAG_ALL == list_id)) {
							if (is_completed(e)) ret++;
							tot++;
						}

					if (tot == 0) return string("0%");
					return to_string(100 * ret / tot) + "%";
				}
			}
		}
	};

// status fields
map<char, function<string()>> status_fields = {
		// @status field "s" - shows status value (output of commands)
		{ 's',	[] () {
				return status;
			}
		},
		// @status field "l" - shows current list name
		{ 'l',	[] () {
				if (tag_filter == TAG_ALL) return string("All lists");

				return "List " + to_string(tag_filter) + (((tag_filter < t_tags.size()) && (t_tags.at(tag_filter) != to_string(tag_filter))) ? (": " + t_tags.at(tag_filter)) : "");
			}
		},
		// @status field "m" - shows current mode (NORMAL|LISTVIEW|DETAILS|HELP)
		{ 'm',	[] () {
				switch (mode) {
					case MODE_NORMAL:
						return string("NORMAL");
						break;
					case MODE_LISTVIEW:
						return string("LISTVIEW");
						break;
					case MODE_DETAILS:
						return string("DETAILS");
						break;
					case MODE_HELP:
						return string("HELP");
						break;
					case MODE_TIMELINE:
						return string("TIMELINE");
						break;
					default:
						return string("");
				}
			}
		},
		// @status field "f" - shows filters
		{ 'f',	[] () {
				return string((filter & FILTER_UNCAT) ? "U" : "_") +
					string((filter & FILTER_COMPLETE) ? "V" : "_") +
					string((filter & FILTER_COMING) ? "C" : "_") +
					string((filter & FILTER_FAILED) ? "F" : "_") +
					string((filter & FILTER_NODUE) ? "N" : "_") +
					string((filter & FILTER_EMPTY) ? "0" : "_") +
					((normal_regex_filter == "") ? "" : (" [e " + normal_regex_filter + "]")) +
					((listview_regex_filter == "") ? "" : (" [l " + listview_regex_filter + "]"));
			}
		},
		// @status field "i" - shows selected task ID and a total amount of entries
		{ 'i',	[] () {
				bool has_visible = false;

				for (int i = 0; i < t_list.size(); i++) has_visible |= is_visible(i);

				if (!has_visible) return string("");

				return to_string(s_line) + "/" + to_string(t_list.size() - 1);
			}
		},
		// @status field "p" - shows the percentage of completed entries on a current list
		{ 'p',	[] () {
				int total = 0;
				int comp = 0;

				for (int i = 0; i < t_list.size(); i++)
					if ((tag_filter == TAG_ALL) || (tag_filter == t_list.at(i).tag)) {
						total++;
						if (is_completed(t_list.at(i))) comp++;
					}

				if (total == 0) return string("");

				return to_string((int)(100.0 * comp / total)) + "%";
			}
		},
		// @status field "P" - shows the amount of completed entries on the list against the total amount of attached entries
		{ 'P', [] () {
				int total = 0;
				int comp = 0;

				for (int i = 0; i < t_list.size(); i++)
					if ((tag_filter == TAG_ALL) || (tag_filter == t_list.at(i).tag)) {
						total++;
						if (is_completed(t_list.at(i))) comp++;
					}

				if (total == 0) return string("");

				return to_string(comp) + "/" + to_string(total) + " COMP";
			}
		}
	};

int halfdelay_time = 2;

int filter;
int tag_filter;
string normal_regex_filter;
string listview_regex_filter;

string contexec_regex_filter;

bool shift_multivars = false;

int color_title;
int color_status;
int color_complete;
int color_coming;
int color_due;
int color_failed;

int row_separator_offset = 0;
multistr_c separators("|||");	// row, status, details separators
multistr_c box_strong("|-++++");	// vertical and horisontal line;
					// 1st, 2nd, 3rd, 4th corner;
					// more later
multistr_c box_light("|-++++");

string normal_all_cols;
string normal_cols;
string details_cols;
string listview_cols;

string normal_status_fields;
string listview_status_fields;

int mode = MODE_NORMAL;
stack<int> prev_modes;

vector<bind_s> binds;

int w, h;

string status = "Welcome to " + string(TITLE) + "!";

int s_line;
int delta;
int numbuffer = -1;

vector<wstring> command_history;
wstring command;
wstring command_t;
int command_cursor = 0;
int command_index = 0;

bool active = false;

}

namespace li {

string filename = ".noaftodo-list";
bool autosave = true;

string task_duration_default = "1h";

string sort_order = "ldtD";

}

namespace conf {

	string filename = "noaftodo.conf";

}
