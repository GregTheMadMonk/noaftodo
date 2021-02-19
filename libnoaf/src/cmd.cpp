#include "cmd.hpp"

#include <iostream>
#include <stdexcept>

#include <cvar.hpp>
#include <hooks.hpp>
#include <log.hpp>
#include <ui.hpp>

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
						return ret;
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

						if (ui) ui->pause();

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

						if (ui) ui->resume();

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

						if (ui) ui->pause();

						system(cmdline.c_str());

						if (ui) ui->resume();

						return ret;
					},
					"Execute shell command, but don't capture its output. Useful when calling an app that has UI."
				}
			},
			{ "alias",
				{
					[] (const vector<string>& args) {
						if (args.size() < 2)
							throw runtime_error("Not enough arguments!");

						vector<string> a_args;

						for (int i = 1; i < args.size() - 1; i++)
							a_args.push_back(args.at(i));

						aliases()[args.at(0)] = {
							a_args,
							args.at(args.size() - 1)
						};

						return args.at(0);
					},
					"Create an alias (basically, NOAFScript functions)"
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

						log << llev(VCAT) << ret << lend;

						return ret;
					},
					"It's an echo command. IDK how to explain it if you don't know what it does"
				}
			},
			{ "if",
				{
					[] (const vector<string>& args) {
						if (args.size() < 2)
							throw runtime_error("Not enough arguments!");

						if (args.at(0) == "true")
							exec(args.at(1), true);
						else if (args.size() > 2)
							exec(args.at(2), true);

						return ret; // don't modify a return value
					},
					"It's an if statement. :if condition do-if-true[ do-if-false]"
				}
			},
			{ "math",
				{
					[] (const vector<string>& args) -> string {
						if (args.size() < 3)
							throw runtime_error("Not enough arguments!");

						const float a = stod(args.at(0));
						const float b = stod(args.at(2));

						// I wish there was a better way...
						// Or, if there is, I knew it...
						if (args.at(1) == "==") {
							return (a == b) ? "true" : "false";
						} else if (args.at(1) == "!=") {
							return (a != b) ? "true" : "false";
						} else if (args.at(1) == "<") {
							return (a < b) ? "true" : "false";
						} else if (args.at(1) == "<=") {
							return (a <= b) ? "true" : "false";
						} else if (args.at(1) == ">") {
							return (a > b) ? "true" : "false";
						} else if (args.at(1) == ">=") {
							return (a >= b) ? "true" : "false";
						} else if (args.at(1) == "+") {
							return to_string(a + b);
						} else if (args.at(1) == "-") {
							return to_string(a - b);
						} else if (args.at(1) == "*") {
							return to_string(a * b);
						} else if (args.at(1) == "/") {
							if (b == 0)
								throw runtime_error("No, you can't divide by zero");
							return to_string(a / b);
						} else if (args.at(1) == "%") {
							if (stoi(args.at(2)) == 0)
								throw runtime_error("No, you can't divide by zero");
							return to_string(stoi(args.at(0)) % stoi(args.at(2)));
						}

						return "";
					},
					"Perform a mathematical operation on operands"
				}
			},
			{ "set",
				{
					[] (const vector<string>& args) -> string {
						if (args.size() < 1)
							throw runtime_error("Cvar name is not given!");
						if (args.size() < 2) {
							cvar::reset(args.at(0));
							return ret;
						}

						cvar::get(args.at(0)) = args.at(1);
						return args.at(1);
					},
					"Sets a console variable value. If no value is given, a console variable is reset"
				}
			},
		};
		return _cmds;
	}

	map<string, alias>& aliases() {
		static map<string, alias> _aliases;
		return _aliases;
	}

	string buffer = "";

	string msg = "";
	string ret = "";

	void exec(const string& command, const bool& oneliner) {
		// but before we get to funs stuff, it's needed to be sure that some needed cvars are
		// properly initialized.
		if (cvar::cvars().find("ret") == cvar::cvars().end())
			cvar::cvars()["ret"] = cvar::wrap_string(ret, cvar::READONLY || cvar::NO_PREDEF);
		
		// interpreter state variables
		vector<string> queue = { "" };	// arguments list

		bool skip_special = false;	// skip next special character
		bool inquotes = false;		// take input inquotesly
		char mode = 0;			// input mode
		int curly_owo = 0;		// how deep are we inside curly brackets
		string temp = "";		// we read some stuff here, e.g. variable names

		// get last argument of current queue item
		const auto& last = [&] () -> string& {
			if (queue.size() == 0) queue.push_back("");
			if ((mode == '%') || (mode == '`') || (mode == '(') || (mode == '$'))
				return temp;
			return queue.at(queue.size() - 1);
		};

		// run command on current level
		const auto& flush = [&] () {
			if (queue.size() == 0) return;

			// work out special cases: var=value, arithmetics
			// replace :var = value with :set var value
			if (queue.size() == 3) if (queue.at(1) == "=")
				queue = { "set", queue.at(0), queue.at(2) };
			// call :math command if first argument is a number
			try {
				stod(queue.at(0));

				queue.insert(queue.begin(), "math");
			} catch (const invalid_argument& e) {}

			log << "Executing ";
			for (const auto& a : queue) log << "\"" << a << "\" ";
			log << lend;

			// search for an alias
			if (aliases().find(queue.at(0)) != aliases().end()) {
				const auto& alias = aliases().at(queue.at(0));

				if (queue.size() - 1 < alias.args.size()) {
					log << llev(VERR) << "Not enough arguments for an alias!" << lend;
					ret = "";
					buffer = "";
					return;
				}

				for (int i = 0; i < alias.args.size(); i++)
					cvar::get("args." + alias.args.at(i)) = queue.at(1 + i);

				exec(alias.body, true);

				for (const auto& arg : alias.args)
					cvar::erase("args." + arg);

				buffer = "";

				return;
			}

			// execute a command
			vector<string> args;
			for (int i = 1; i < queue.size(); i++)
				args.push_back(queue.at(i));

			try {
				ret = cmds().at(queue.at(0)).cmd_f(args);
			} catch (const out_of_range& e) {
				log << llev(VERR) << "Command not found: \"" << queue.at(0) << "\"" << lend;
				ret = "";
			} catch (const exception& e) {
				log << llev(VERR) << "Command execution error: " << e.what() << lend;
				ret = "";
			}

			queue = { };
			if (!oneliner) buffer = "";
		};

		const auto cmds = (oneliner ? "" : buffer) + command;
		if (!oneliner) buffer = "";
		log << "Attempting execution: " << cmds << lend << lr;

		for (const char& c : cmds) {
			if (!oneliner) buffer += c;

			if (mode == '$') {
				// reading a shell variable name
				if (((c <= 'z') && (c >= 'a')) ||
					((c <= 'Z') && (c >= 'A')) ||
					((c <= '9') && (c >= '0')) ||
					(c == '_'))
					last() += c;
				else {
					mode = 0;
					log << "Trying to replace $" << temp << lend;
					if (getenv(temp.c_str()) != nullptr)
						last() += getenv(temp.c_str());
				}
				continue;
			}

			if (curly_owo > 0) {
				if (c == mode) curly_owo++;
				if (((mode == '{') && (c == '}')) || ((mode == '(') && (c == ')'))) {
					curly_owo--;
				}

				if (curly_owo == 0) {
					if (mode == '(') {
						const auto ret_copy = ret;
						exec(temp, true);
						mode = 0;
						last() += ret;
						ret = ret_copy;
					}
					mode = 0;
				} else last() += c;
				continue;
			}

			if (skip_special) {
				last() += c;
				skip_special = false;
				continue;
			}

			if (((mode == '%') || inquotes) && (mode != c)) {
				if (c == '\\') skip_special = true;
				else last() += c;

				continue;
			}

			// ! and !! commands should not require a space after
			if ((c == '!') && (queue.size() == 1)) {
				mode = '!';
			} else if (mode == '!') {
				queue.push_back("");
				mode = 0;
			}

			switch (c) {
				case '~': // replace ~ with $HOME value
					if (getenv("HOME") != nullptr)
						last() += getenv("HOME");
					else last() += c;
					break;
				case '$': // read system environment variables
					mode = c;
					temp = "";
					break;
				case '(':
					temp = "";
				case '{':
					curly_owo++;
					mode = c;
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
						inquotes = false;
						const auto ret_copy = ret; // back up return value
						exec(temp, true);
						last() += ret;
						ret  = ret_copy;
					}
					break;
				case '%':
					if (mode != c) {
						mode = c;
						temp = "";
					} else if (mode == c) {
						// var name = temp
						mode = 0;
						const auto ret_copy = ret;
						log << "Resolving variable name for " << temp << lend;
						exec("echo " + temp, true);
						temp = ret;
						ret = ret_copy;
						log << "Replacing variable " << temp << lend;
						last() += (string)cvar::get(temp);
					}
					break;
				case '\'': case '\"':
					if (!inquotes) {
						mode = c;
						inquotes = true;
					} else if (mode == c) {
						mode = 0;
						inquotes = false;
					}
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

		// line of input could've terminated here, or we
		// might've reached a line wrap
		if (skip_special) {
			buffer.at(buffer.length() - 1) = ' ';
		} else switch (mode) {
			case '$':
				// end reading of an envvar name
				mode = 0;
				log << "Trying to replace $" << temp << lend;
				if (getenv(temp.c_str()) != nullptr)
					last() += getenv(temp.c_str());
			case 0:
				flush();
				log << "Execution done with return value " << ret << lend;
				break;
		}
		log << ll;
	}

	void terminate() {
		if (buffer == "") return;

		log << llev(VERR) << "Unterminated command at the end of execution. Skipping..." << lend;

		buffer = "";
	}
}
