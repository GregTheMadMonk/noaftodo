#include "noaftodo_cmd.h"

/*
 * [Almost] everything responsible for NOAFtodo
 * built-in command interpreter.
 *
 * I am losing control of it.
 * If hell exists, this is my ticket there.
 */

#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <regex>
#include <stdexcept>

#include "noaftodo.h"
#include "noaftodo_config.h"
#include "noaftodo_cui.h"
#include "noaftodo_cvar.h"
#include "noaftodo_daemon.h"
#include "noaftodo_time.h"

using namespace std;

using li::t_list;
using li::t_tags;

using cui::status;
using cui::s_line;

namespace cmd {

string retval = "";
string buffer;

li::entry* sel_entry = nullptr;

void init() {
	buffer = "";

	// "fake" cvar_base_s::cvars
	// CORE CVARS
	cvar_base_s::wrap_bool("allow_root", allow_root);
	cvar_base_s::wrap_bool("autorun_daemon", da::fork_autostart);

	cvar_base_s::wrap_string("cmd.contexec", cui::contexec_regex_filter);

	cvar_base_s::wrap_int("mode", cui::mode, CVAR_FLAG_NO_PREDEF | CVAR_FLAG_WS_IGNORE);
	cvar_base_s::cvars["mode"]->setter = [] (const string& val) {
		try {
			cui::set_mode(stoi(val));
			return 0;
		} catch (const invalid_argument& e) { return ERR_ARG_TYPE; }
	};
	cvar_base_s::cvars["mode"]->predefine("-1");

	// ENTRY FIELDS
	cvar_base_s::wrap_int("id", s_line, CVAR_FLAG_NO_PREDEF);
	cvar_base_s::cvars["id"]->setter = [] (const string& val) {
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
		} catch (const invalid_argument& e) {}
	};
	cvar_base_s::cvars["id"]->predefine("0");

	cvar_base_s::cvars["title"] = make_unique<cvar_base_s>();
	cvar_base_s::cvars["title"]->getter = [] () {
		if (sel_entry == nullptr)  return string("");
		return sel_entry->title;
	};
	cvar_base_s::cvars["title"]->setter = [] (const string& val) {
		if (sel_entry == nullptr) return;
		sel_entry->title = val;
		if (li::autosave) li::save();
	};
	cvar_base_s::cvars["title"]->flags = CVAR_FLAG_NO_PREDEF | CVAR_FLAG_WS_IGNORE;

	cvar_base_s::cvars["desc"] = make_unique<cvar_base_s>();
	cvar_base_s::cvars["desc"]->getter = [] () {
		if (sel_entry == nullptr)  return string("");
		return sel_entry->description;
	};
	cvar_base_s::cvars["desc"]->setter = [] (const string& val) {
		if (sel_entry == nullptr) return;
		sel_entry->description = val;
		if (li::autosave) li::save();
	};
	cvar_base_s::cvars["desc"]->flags = CVAR_FLAG_NO_PREDEF | CVAR_FLAG_WS_IGNORE;

	cvar_base_s::cvars["due"] = make_unique<cvar_base_s>();
	cvar_base_s::cvars["due"]->getter = [] () {
		if (sel_entry == nullptr)  return string("");
		return ti_cmd_str(sel_entry->due);
	};
	cvar_base_s::cvars["due"]->setter = [] (const string& val) {
		if (sel_entry == nullptr) return;
		sel_entry->due = ti_to_long(val);
		if (li::autosave) li::save();
	};
	cvar_base_s::cvars["due"]->flags = CVAR_FLAG_NO_PREDEF | CVAR_FLAG_WS_IGNORE;

	cvar_base_s::cvars["meta"] = make_unique<cvar_base_s>();
	cvar_base_s::cvars["meta"]->getter = [] () {
		if (sel_entry == nullptr)  return string("");
		return sel_entry->meta_str();
	};
	cvar_base_s::cvars["meta"]->flags = CVAR_FLAG_NO_PREDEF | CVAR_FLAG_WS_IGNORE;

	cvar_base_s::cvars["comp"] = make_unique<cvar_base_s>();
	cvar_base_s::cvars["comp"]->getter = [] ()  {
		return (sel_entry == nullptr) ? "false" : (sel_entry->completed ? "true" : "false" );
	};
	cvar_base_s::cvars["comp"]->setter = [] (const string& val) {
		if (sel_entry == nullptr) return;

		if ((sel_entry->completed && (val != "true")) ||
			(!sel_entry->completed && (val == "true")))
				li::comp(s_line);
	};
	cvar_base_s::cvars["comp"]->flags = CVAR_FLAG_NO_PREDEF | CVAR_FLAG_WS_IGNORE;
	cvar_base_s::cvars["comp"]->predefine("false");

	cvar_base_s::cvars["parent"] = make_unique<cvar_base_s>();
	cvar_base_s::cvars["parent"]->getter = [] () {
		if (sel_entry == nullptr)  return string("");
		return to_string(sel_entry->tag);
	};
	cvar_base_s::cvars["parent"]->setter = [] (const string& val) {
		if (sel_entry == nullptr) return;
		try {
			sel_entry->tag = stoi(val);
			if (li::autosave) li::save();
		} catch (const invalid_argument& e) { }
	};
	cvar_base_s::cvars["parent"]->flags = CVAR_FLAG_NO_PREDEF | CVAR_FLAG_WS_IGNORE;

	cvar_base_s::cvars["pname"] = make_unique<cvar_base_s>(); // parent [list] name
	cvar_base_s::cvars["pname"]->getter = [] ()  {
		if (cui::tag_filter == cui::TAG_ALL) return string("All lists");

		if (cui::tag_filter < t_tags.size()) return t_tags.at(cui::tag_filter);

		return to_string(cui::tag_filter);
	};
	cvar_base_s::cvars["pname"]->setter = [] (const string& val) {
		if (cui::tag_filter < 0) return;

		while (cui::tag_filter >= t_tags.size()) t_tags.push_back(to_string(t_tags.size()));

		t_tags[cui::tag_filter] = val;

		if (li::autosave) li::save();
	};
	cvar_base_s::cvars["pname"]->flags = CVAR_FLAG_NO_PREDEF | CVAR_FLAG_WS_IGNORE;

	// FILTERS
	cvar_base_s::wrap_string("norm.regex_filter", cui::normal_regex_filter);
	cvar_base_s::wrap_string("livi.regex_filter", cui::listview_regex_filter);

	cvar_base_s::wrap_int("tag_filter", cui::tag_filter);
	cvar_base_s::cvars["tag_filter"]->setter = [] (const string& val) {
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
	};
	cvar_base_s::cvars["tag_filter"]->flags |= CVAR_FLAG_NO_PREDEF;
	cvar_base_s::cvars["tag_filter"]->predefine("all");

	cvar_base_s::wrap_int("tag_filter_v", cui::tag_filter);
	cvar_base_s::cvars["tag_filter_v"]->setter = [] (const string& val) {
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
	};
	cvar_base_s::cvars["tag_filter_v"]->flags |= CVAR_FLAG_NO_PREDEF | CVAR_FLAG_WS_IGNORE;
	cvar_base_s::cvars["tag_filter_v"]->predefine("-1");

	cvar_base_s::wrap_int("filter", cui::filter);

	cvar_base_s::wrap_string("sort_by", li::sort_order);
	cvar_base_s::cvars["sort_by"]->setter = [] (const string& val) {
		li::sort_order = "";

		for (const char& c : val)
			if (li::sort_order.find(c) == string::npos) li::sort_order += c;

		//li::sort_order = string(li::sort_order.begin(), unique(li::sort_order.begin(), li::sort_order.end()));
		if (li::sort_order.length() > 4) li::sort_order = li::sort_order.substr(0, 4);
		li::sort();
	};

	// FILTER BITS
	cvar_base_s::wrap_maskflag("filter.uncat", cui::filter, cui::FILTER_UNCAT, CVAR_FLAG_WS_IGNORE);
	cvar_base_s::wrap_maskflag("filter.complete", cui::filter, cui::FILTER_COMPLETE, CVAR_FLAG_WS_IGNORE);
	cvar_base_s::wrap_maskflag("filter.coming", cui::filter, cui::FILTER_COMING, CVAR_FLAG_WS_IGNORE);
	cvar_base_s::wrap_maskflag("filter.failed", cui::filter, cui::FILTER_FAILED, CVAR_FLAG_WS_IGNORE);
	cvar_base_s::wrap_maskflag("filter.nodue", cui::filter, cui::FILTER_NODUE, CVAR_FLAG_WS_IGNORE);
	cvar_base_s::wrap_maskflag("filter.empty", cui::filter, cui::FILTER_EMPTY);

	// UI SETUP
	cvar_base_s::wrap_int("halfdelay_time", cui::halfdelay_time);

	cvar_base_s::wrap_bool("frameshift_multistr", cui::shift_multivars);

	cvar_base_s::wrap_string("norm.cols.all", cui::normal_all_cols);
	cvar_base_s::wrap_string("norm.cols", cui::normal_cols);
	cvar_base_s::wrap_string("livi.cols", cui::listview_cols);
	cvar_base_s::wrap_string("det.cols", cui::details_cols);

	cvar_base_s::wrap_string("norm.status_fields", cui::normal_status_fields);
	cvar_base_s::wrap_string("livi.status_fields", cui::listview_status_fields);

	// CHARACTER SET
	cvar_base_s::wrap_multistr("charset.separators", cui::separators, 3);
	cvar_base_s::wrap_multistr("charset.box_strong", cui::box_strong, 6);
	cvar_base_s::wrap_multistr("charset.box_light", cui::box_light, 6);

	// SINGLE CHARACTER WRAPPERS
	cvar_base_s::wrap_multistr_element("charset.separators.row", cui::separators, cui::CHAR_ROW_SEP);
	cvar_base_s::wrap_multistr_element("charset.separators.status", cui::separators, cui::CHAR_STA_SEP);
	cvar_base_s::wrap_multistr_element("charset.separators.details", cui::separators, cui::CHAR_DET_SEP);

	cvar_base_s::wrap_multistr_element("charset.box_strong.v", cui::box_strong, cui::CHAR_VLINE);
	cvar_base_s::wrap_multistr_element("charset.box_strong.h", cui::box_strong, cui::CHAR_HLINE);
	cvar_base_s::wrap_multistr_element("charset.box_strong.corn1", cui::box_strong, cui::CHAR_CORN1);
	cvar_base_s::wrap_multistr_element("charset.box_strong.corn2", cui::box_strong, cui::CHAR_CORN2);
	cvar_base_s::wrap_multistr_element("charset.box_strong.corn3", cui::box_strong, cui::CHAR_CORN3);
	cvar_base_s::wrap_multistr_element("charset.box_strong.corn4", cui::box_strong, cui::CHAR_CORN4);
	cvar_base_s::wrap_multistr_element("charset.box_light.v", cui::box_light, cui::CHAR_VLINE);
	cvar_base_s::wrap_multistr_element("charset.box_light.h", cui::box_light, cui::CHAR_HLINE);
	cvar_base_s::wrap_multistr_element("charset.box_light.corn1", cui::box_light, cui::CHAR_CORN1);
	cvar_base_s::wrap_multistr_element("charset.box_light.corn2", cui::box_light, cui::CHAR_CORN2);
	cvar_base_s::wrap_multistr_element("charset.box_light.corn3", cui::box_light, cui::CHAR_CORN3);
	cvar_base_s::wrap_multistr_element("charset.box_light.corn4", cui::box_light, cui::CHAR_CORN4);

	// CHARACTER SET: MISC
	cvar_base_s::wrap_int("charset.separators.row.offset", cui::row_separator_offset);

	// COLORSCHEME
	cvar_base_s::wrap_int("colors.title", cui::color_title);
	cvar_base_s::wrap_int("colors.status", cui::color_status);
	cvar_base_s::wrap_int("colors.entry_completed", cui::color_complete);
	cvar_base_s::wrap_int("colors.entry_coming", cui::color_coming);
	cvar_base_s::wrap_int("colors.entry_failed", cui::color_failed);

	// DAEMON ACTIONS
	cvar_base_s::wrap_string("on_daemon_launch_action", da::launch_action);
	cvar_base_s::wrap_string("on_task_failed_action", da::task_failed_action);
	cvar_base_s::wrap_string("on_task_coming_action", da::task_coming_action);
	cvar_base_s::wrap_string("on_task_completed_action", da::task_completed_action);
	cvar_base_s::wrap_string("on_task_uncompleted_action", da::task_uncompleted_action);
	cvar_base_s::wrap_string("on_task_new_action", da::task_new_action);
	cvar_base_s::wrap_string("on_task_edited_action", da::task_edited_action);
	cvar_base_s::wrap_string("on_task_removed_action", da::task_removed_action);

	// HELPER CVARS
	cvar_base_s::cvars["last_v_id"] = make_unique<cvar_base_s>();
	cvar_base_s::cvars["last_v_id"]->getter = [] ()  {
		int ret = -1;
		for (int i = 0; i < t_list.size(); i++)
			if (cui::is_visible(i)) ret = i;

		return to_string(ret);
	};
	cvar_base_s::cvars["last_v_id"]->flags = CVAR_FLAG_RO | CVAR_FLAG_WS_IGNORE | CVAR_FLAG_NO_PREDEF;

	cvar_base_s::cvars["first_v_id"] = make_unique<cvar_base_s>();
	cvar_base_s::cvars["first_v_id"]->getter = [] ()  {
		for (int i = 0; i < t_list.size(); i++)
			if (cui::is_visible(i)) return to_string(i);

		return string("-1");
	};
	cvar_base_s::cvars["first_v_id"]->flags = CVAR_FLAG_RO | CVAR_FLAG_WS_IGNORE | CVAR_FLAG_NO_PREDEF;

	cvar_base_s::cvars["last_v_list"] = make_unique<cvar_base_s>();
	cvar_base_s::cvars["last_v_list"]->getter = [] ()  {
		int ret = -1;
		for (int i = 0; i < t_tags.size(); i++)
			if (cui::l_is_visible(i)) ret = i;

		return to_string(ret);
	};
	cvar_base_s::cvars["last_v_list"]->flags = CVAR_FLAG_RO | CVAR_FLAG_WS_IGNORE | CVAR_FLAG_NO_PREDEF;

	cvar_base_s::cvars["first_v_list"] = make_unique<cvar_base_s>();
	cvar_base_s::cvars["first_v_list"]->getter = [] ()  {
		for (int i = 0; i < t_tags.size(); i++)
			if (cui::l_is_visible(i)) return to_string(i);

		return string("-1");
	};
	cvar_base_s::cvars["first_v_list"]->flags = CVAR_FLAG_RO | CVAR_FLAG_WS_IGNORE | CVAR_FLAG_NO_PREDEF;

	cvar_base_s::wrap_int("numbuffer", cui::numbuffer, CVAR_FLAG_WS_IGNORE | CVAR_FLAG_NO_PREDEF);
	cvar_base_s::cvars["numbuffer"]->predefine("-1");

	cvar_base_s::wrap_string("ret", retval, CVAR_FLAG_WS_IGNORE | CVAR_FLAG_NO_PREDEF);

	cvar_base_s::cvars["VER"] = make_unique<cvar_base_s>();
	cvar_base_s::cvars["VER"]->getter = [] () { return VERSION; };
	cvar_base_s::cvars["VER"]->flags = CVAR_FLAG_RO | CVAR_FLAG_WS_IGNORE | CVAR_FLAG_NO_PREDEF;
}

vector<string> cmdbreak(const string& cmdline) {
	vector<string> cmdqueue;
	string temp = "";

	bool inquotes = false;
	bool skip_special = false;
	for (int i = 0; i < cmdline.length(); i++) {
		const char c = cmdline.at(i);

		if (skip_special) {
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
				if (!inquotes) {
					if (temp != "") {
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

	if (inquotes || skip_special) { // unterminated command (unterminated quotes or ends with '\')
		buffer = cmdline;
		return vector<string>();
	}

	if (temp != "") cmdqueue.push_back(temp);

	return cmdqueue;
}

void run(string command) {
	log_offset++;
	// replace variables
	if ((s_line >= 0) && (s_line < t_list.size()))
		command = format_str(command, &t_list.at(s_line));
	else
		command = format_str(command, nullptr);

	// search for shell variables
	regex shr("\\$[[:alpha:]]\\w*\\b"); // I <3 StackOverflow

	sregex_iterator it(command.begin(), command.end(), shr);
	sregex_iterator end;

	{
		string newcom = command;
		int index = -1;

		for (; it != end; it++) {
			for (int i = 0; i < it->size(); i++) {
				const string var = (*it)[i];
				const char* rep = getenv(var.substr(1).c_str());
				if (rep != nullptr) {
					log("Replacing environment variable " + var + " with \"" + rep + "\"", LP_IMPORTANT);
					while ((index = newcom.find(var)) != string::npos) // not the best solution but I'm fine with it
						newcom.replace(index, var.length(), rep);
				}
			}
		}

		command = newcom;

		if (getenv("HOME") != nullptr)
			while ((index = command.find("~/")) != string::npos)
				command.replace(index, 2, getenv("HOME") + string("/"));
	}

	vector<string> new_commands = cmdbreak(command);

	if (new_commands.size() == 0) { log_offset--; return; } // basiacally, an empty string, or just ';'s

	if (new_commands.size() > 1) {
		// more than one command in a string. Each can contain some more after
		// variables are replaced with their values
		log("Breaking " + command);
		for (const auto& cmd : new_commands) run(cmd);
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

	auto put = [&name, &args] (const string& val) {
		if (name == "") name = val;
		else args.push_back(val);
	};

	int start = 0;	// to respect tabs and spaces at the line start

	for (int i = 0; i < command.length(); i++) {
		const char c = command.at(i);

		if ((i == start) && (c == '!'))	{ // "!" or "!!" - shell command
			if (i < command.length() - 1) if (command.at(i + 1) == '!') {
				put("!!");
				i++;
				shellcmd = true;
				continue;
			}

			put("!");

			shellcmd = true;
			continue;
		}

		if (shellcmd) { // everything between "!" and ";" is passed to system() as-it-is
			word += c;
		} else if (skip_special) { // symbol after '\' is added as-it-is
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
				if (!inquotes) { // in quotes - just a symbol
					if (word != "") {
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

	buffer = ""; // if we have reached this far, a command is at least properly terminated

	if (word != "") put(word);

	if (name == "") { log_offset--; return; } // null command

	try
	{	// search for aliases, prioritize
		string alstr = aliases.at(name);

		// insert alias arguments. Add unused ones to command line
		for (int j = 0; j < args.size(); j++) {
			bool replaced = false;

			int index = -1;
			const string varname = "%arg" + to_string(j + 1) + "%";
			while ((index = alstr.find(varname)) != string::npos) {
				alstr.replace(index, varname.length(), args.at(j));
				replaced = true;
			}

			if (!replaced) alstr += " \"" + replace_special(args.at(j)) + "\"";
		}

		exec(alstr); // try to run
	} catch (const out_of_range& e) { // no such alias
		try {
			const int ret = (cmds.at(name))(args); // try to run command

			if (ret != 0) switch (ret) { // handle error return values
				case ERR_ARG_COUNT:
					retval = "Wrong argument count!";
					break;
				case ERR_ARG_TYPE:
					retval = "Wrog argument type!";
					break;
				case ERR_EXTERNAL:
					retval = "Cannot execute command!";
					break;
				default:
					retval = "Unknown execution error!";
			}
		} catch (const out_of_range& e) { // command not found
			log("Command not found! (" + command + ")", LP_ERROR);
			retval = "Command not found!";
		}
	}

	status = retval;

	log_offset--;
}

void exec(const string& command) {
	log ("Attempting execution: " + command);
	vector<string> cmdq = cmdbreak(buffer + command);

	for (const auto& cmd : cmdq) run(cmd);
}

void terminate() {
	if (buffer == "") return;

	status = "Unterminated command";
	log("Unterminated command at the end of execution. Skipping...", LP_ERROR);

	buffer = "";
}

}
