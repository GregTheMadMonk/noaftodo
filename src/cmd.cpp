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

#include <noaftodo.h>
#include <config.h>
#include <cui.h>
#include <cvar.h>
#include <daemon.h>
#include <time.h>

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

	init_cvars();
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

void upd_cvars() {
	for (auto it = cvar_base_s::cvars.begin(); it != cvar_base_s::cvars.end(); )
		if (it->first.find("alias.") == 0)
			it = cvar_base_s::cvars.erase(it);
		else
			it++;

	for (auto it = aliases.begin(); it != aliases.end(); it++) {
		const string name = it->first;

		cvar_base_s::cvars["alias." + name] = make_unique<cvar_base_s>(
				[name] () { return aliases[name]; },
				[] (const string& val) {},
				CVAR_FLAG_NO_PREDEF | CVAR_FLAG_WS_IGNORE
				);
	}

	for (auto it = cvar_base_s::cvars.begin(); it != cvar_base_s::cvars.end(); )
		if (it->first.find("status_field.") == 0)
			it = cvar_base_s::cvars.erase(it);
		else
			it++;

	for (auto it = cui::status_fields.begin(); it != cui::status_fields.end(); it++) {
		const char name = it->first;

		cvar_base_s::cvars[string("fields.status.") + name] = make_unique<cvar_base_s>(
				[name] () { return cui::status_fields[name](); },
				[name] (const string& val) {
					cui::status_fields[name] = [val] () {
						return format_str(val, sel_entry);
					};
				},
				CVAR_FLAG_NO_PREDEF | CVAR_FLAG_WS_IGNORE
				);
	}
}

}
