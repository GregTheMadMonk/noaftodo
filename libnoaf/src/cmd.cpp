#include "cmd.hpp"

#include <iostream>
#include <stdexcept>

#include <hooks.hpp>
#include <log.hpp>

using namespace std;

namespace noaf::cmd {

	bool allow_system = true;

	map<string, cmd>& cmds() {
		// commands index is initialized with essential commands EVERY noaf app should have
		static map<string, cmd> _cmds = {
			{ "q",
				{
					[] (const vector<string>& args) {
						int code = 0;
						if (args.size() > 0) code = stoi(args.at(0));
						noaf::exit(code);
						return "";
					},
					"Exits program with code from its argument or, if unspecified, 0"
				}
			},
			{ "!",
				{
					[] (const vector<string>& args) {
						if (!allow_system)
							throw runtime_error("Execution of system commands is not allowed!");

						string cmdline = "";

						for (const auto& arg : args) cmdline += arg + " ";

						ui::pause();

						char buffer[128];
						string result = "";
						FILE* pipe = popen(cmdline.c_str(), "r");
						if (!pipe)
							throw runtime_error("Can't execute command: popen() failed!");

						try {
							while (fgets(buffer, sizeof buffer, pipe) != NULL) {
								cout << buffer;
								result += buffer;
							}
						} catch (...) {
							pclose(pipe);
							throw runtime_error("Error dduring process execution!");
						}

						ui::resume();

						pclose(pipe);

						result.erase(remove(result.begin(), result.end(), '\n'), result.end());

						return result;
					},
					"Executes shell command"
				}
			},
			{ "!!",
				{
					[] (const vector<string>& args) {
						if (!allow_system)
							throw runtime_error("Execution of system commands is not allowed!");

						string cmdline = "";

						for (const auto& arg : args) cmdline += arg + " ";

						ui::pause();

						system(cmdline.c_str());

						ui::resume();

						return "";
					},
					"Execute shell command, but don't capture its output. Useful when calling an app that has UI."
				}
			},
			{ "echo",
				{
					[] (const vector<string>& args) {
						string ret = "";

						for (int i = 0; i < args.size(); i++) {
							ret += args.at(i);
							if (i < args.size() - 1) ret += " ";
						}

						log << ret << lend;

						return ret;
					},
					"It's an echo command. IDK how to explain it if you don't know what it does"
				}
			},
		};
		return _cmds;
	}

	map<string, string>& aliases() {
		static map<string, string> _aliases;
		return _aliases;
	}

	string buffer = "";

	string msg = "";
	string ret = "";

	void exec(const string& command) {
		log << "Attempting execution: " << command << lend << lr;
		
		vector<string> queue = { "" };

		// get last argument of current queue item
		const auto& last = [&] () -> string& {
			return queue.at(queue.size() - 1);
		};

		// run command on current level
		const auto& flush = [&] () {
			if (queue.size() == 0) return;

			log << llev(VERR) << "Executing ";
			for (const auto& a : queue) log << a << " ";
			log << lend;

			vector<string> args;
			for (int i = 1; i < queue.size(); i++)
				args.push_back(queue.at(i));

			try {
				ret = cmds().at(queue.at(0)).cmd_f(args);
			} catch (const out_of_range& e) {
				log << llev(VERR) << "Command not found: \"" << queue.at(0) << "\"" << lend;
			}

			queue = { };
			buffer = "";
		};

		bool skip_special = false;	// skip next special character
		bool inquotes = false;		// take input inquotesly
		char mode = 0;			// input mode
		int curly_owo = 0;		// how deep are we inside curly brackets
		string temp = "";

		for (const char& c : command) {
			buffer += c;

			if (curly_owo > 0) {
				if (c == '{') curly_owo++;
				if (c == '}') {
					curly_owo--;
				}
				if (curly_owo > 0) last() += c;
				continue;
			}

			if (skip_special) {
				if (mode == '`') temp += c;
				else last() += c;
				skip_special = false;
				continue;
			}

			if (inquotes && (mode != c)) {
				if (c == '\\') skip_special = true;
				else {
					if (mode == '`') temp += c;
					else last() += c;
				}

				continue;
			}

			switch (c) {
				case '{':
					curly_owo++;
					break;
				case '\\':
					skip_special = true;
					break;
				case ';':
					if (mode == 0) flush();
					else last() += c;
					break;
				case '`':
					if (!inquotes) {
						mode = '`';
						inquotes = true;
						temp = "";
					} else {
						mode = 0;
						exec(temp);
						last() += ret;
					}
					break;
				case '\'': case '\"':
					if (!inquotes) {
						mode = c;
						inquotes = true;
					} else if (mode == c) mode = 0;
					break;
				case ' ': case '\t':
					if (!inquotes) {
						if (last() != "") queue.push_back("");
						break;
					}
				default:
					last() += c;
			}
		}

		if (!skip_special) flush();

		log << ll << "Execution done with return value " << ret << lend;
	}

	void terminate() {
		if (buffer == "") return;

		log << llev(VERR) << "Unterminated command at the end of execution. Skipping..." << lend;

		buffer = "";
	}
}
