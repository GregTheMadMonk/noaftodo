#include "noaftodo.h"

#include <csignal>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <memory>
#include <pwd.h>
#include <sys/types.h>
#include <unistd.h>

#include <cmd.h>
#include <config.h>
#include <cui.h>
#include <cvar.h>
#include <daemon.h>
#include <list.h>
#include <core/time.h>

using namespace std;

int run_mode;

int errors = 0;

bool allow_root = false;

bool verbose = false;
bool pure = false;
bool enable_log = true;
int log_offset = 0;

int exit_value = 0;

extern string HELP;

int main(int argc, char* argv[]) {
	setlocale(LC_ALL, "");

	auto handler = [] (int signum) {
		log("Exit requested by signal: " + to_string(signum), LP_IMPORTANT);
		noaftodo_exit(signum);
	};

	signal(SIGINT, handler);
	signal(SIGTERM, handler);

	run_mode = PM_DEFAULT;

	const char* home = getenv("HOME");
	const char* xdg_conf = getenv("XDG_CONFIG_HOME");

	li::filename = string((home == nullptr) ? "." : home) + "/.noaftodo-list";
	conf::filename = string((xdg_conf == nullptr) ? ((home == nullptr) ? "." : home) : xdg_conf) + "/noaftodo.conf";

	// init the command-line interpreter
	cmd::init();

	// parse arguments
	for (int i = 1; i < argc; i++) {
		// @argument "-h, --help" - print help message
		if (strcmp(argv[i], "-h") * strcmp(argv[i], "--help") == 0) run_mode = PM_HELP;
		// @argument "-V, --version" - print program version
		else if (strcmp(argv[i], "-V") * strcmp(argv[i], "--version") == 0) {
			cout << TITLE << " v." << VERSION << endl;
			noaftodo_exit();
		}
		// @argument "-d, --daemon" - start daemon
		else if (strcmp(argv[i], "-d") * strcmp(argv[i], "--daemon") == 0) run_mode = PM_DAEMON;
		// @argument "-I, --interpreter" - start in interpreter mode
		else if (strcmp(argv[i], "-I") * strcmp(argv[i], "--interpreter") == 0) run_mode = PM_INTERP;
		// @argument "-C, --command" - execute a command
		else if (strcmp(argv[i], "-C") * strcmp(argv[i], "--command") == 0) {
			if (i < argc - 1) {
				cmd::exec(string(argv[i + 1]));
				i++;
			} else {
				log("No command specified!", LP_ERROR);
				noaftodo_exit(1);
			}
		}
		// @argument "-k, --kill-daemon" - kill daemon
		else if (strcmp(argv[i], "-k") * strcmp(argv[i], "--kill-daemon") == 0) {
			da::kill();
			noaftodo_exit();
		}
		// @argument "-r, --refire" - if daemon is running, re-fire startup events (like, notifications)
		else if (strcmp(argv[i], "-r") * strcmp(argv[i], "--refire") == 0) {
			da::send("N");
			noaftodo_exit();
		}
		// @argument "-c, --config" - specify config file
		else if (strcmp(argv[i], "-c") * strcmp(argv[i], "--config") == 0) {
			if (i < argc - 1) {
				conf::filename = string(argv[i + 1]);
				i++;
			} else {
				log("Config not specified after " + string(argv[i]), LP_ERROR);
				noaftodo_exit(1);
			}
		}
		// @argument "-l, --list" - specify list file
		else if (strcmp(argv[i], "-l") * strcmp(argv[i], "--list") == 0) {
			if (i < argc - 1) {
				li::filename = string(argv[i + 1]);
				i++;
			} else {
				log("List file not specified after " + string(argv[i]), LP_ERROR);
				noaftodo_exit(1);
			}
		}
		// @argument "-v, --verbose" - print all messages
		else if (strcmp(argv[i], "-v") * strcmp(argv[i], "--verbose") == 0) {
			verbose = true;
		}
		// @argument "-p, --pure" - use pure output. No intents, no time, just text
		else if (strcmp(argv[i], "-p") * strcmp(argv[i], "--pure") == 0) {
			pure = true;
		} else log("Unrecognized parameter \"" + string(argv[i]), LP_ERROR);
	}

	log(string(TITLE) + " v." + string(VERSION));

	if (run_mode == PM_HELP)  {
		print_help();
		noaftodo_exit();
	}

	// interpreter starts without preloading anything
	if (run_mode == PM_INTERP) {
		string line;
		while (getline(cin, line)) cmd::exec(line);
	}

	// initialize CUI even if we are not going
	// to launch it. Will create per-mode init
	// functions and allow to set mode variables
	cui::init();

	// load the config
	conf::load(true, true);

#ifndef NO_ROOT_CHECK
	if ((geteuid() == 0) && !allow_root) {
		log("Can't run as root! To disable this check, set \"allow_root\" to \"true\" in your config", LP_ERROR);
		noaftodo_exit(1);
	}
#endif

	// load the list
	li::load();

	if (run_mode == PM_DEFAULT)	 {
		// if this option is enabled, start daemon in the forked process
		if (da::fork_autostart && !da::check_lockfile())
			switch (fork()) {
				case -1:
					log("Error creating fork for daemon", LP_ERROR);
					break;
				case 0: // child - daemon
					run_mode = PM_DAEMON;
					enable_log = false;
					da::clients = 1; // care about clients, shut down when there's none
					break;
				default:
					cui::construct();
					cui::run();
					break;
			}
		else
		{
			da::send("S");
			cui::construct();
			cui::run();
		}
	}

	if (run_mode == PM_DAEMON)	da::run();

	return exit_value;
}

void print_help() {
	cout << TITLE << endl;
#ifdef NO_MQUEUE
	cout << "Built with NO_MQUEUE" << endl;
#endif
	cout << "Usage:" << endl <<
		"\tnoaftodo [OPTIONS]" << endl;
	cout << HELP << endl;
	cout << "For more information, see " << TITLE << " manpage." << endl;
}

void log(const string& message, const char& prefix, const int& sleep_sec)
{	
	if (!enable_log) return;

	const bool wcui = cui::active;

	if ((prefix != LP_DEFAULT) || verbose) {
		if (wcui) cui::destroy();

		// reset formatting (prevents colorful, but annoying mess in the terminal)
		cout << "[0m";

		if (!pure) {
			cout << "[" << time_s().fmt_log() << "][" << prefix << "] ";
			for (int i = 0; i < log_offset; i++) cout << "  ";
		}

		cout << message << endl;

		if (sleep_sec != 0) sleep(sleep_sec);

		if (wcui) cui::construct();
	}
}

void noaftodo_exit(const int& val) {
	log("Exiting with value: " + to_string(val));

	// if UI or daemon were not started yet, just exit
	// otherwise, notify components: they have to finish
	// their job first
	if (cui::active) { cui::mode = cui::MODE_EXIT; exit_value = val; }
	else if ((run_mode == PM_DAEMON) && da::running) { da::running = false; exit_value = val; }
	else exit(val);
}

void select_entry(li::entry* const list_entry) {
	for (auto it = cvar_base_s::cvars.begin(); it != cvar_base_s::cvars.end(); )
		if (it->first.find("meta.") == 0) // clean up old %meta.prop_name% cvars
			it = cvar_base_s::cvars.erase(it);
		else it++;

	cmd::sel_entry = list_entry;

	// and populate new
	if (cmd::sel_entry != nullptr)
		for (auto it = cmd::sel_entry->meta.begin(); it != cmd::sel_entry->meta.end(); it++) {
			const string name = it->first;
			cvar_base_s::cvars["meta." + name] = make_unique<cvar_base_s>(
				[name] () {
					return cmd::sel_entry->get_meta(name);
				},
				[name] (const string& val) {
					if (name != "eid") // eid is a read-only property
						cmd::sel_entry->meta[name] = val;

					li::save();
				},
				CVAR_FLAG_NO_PREDEF | CVAR_FLAG_WS_IGNORE
				);
		}
}

string format_str(string str, li::entry* const list_entry, const bool& renotify) {
	if (list_entry != cmd::sel_entry)
		select_entry(list_entry);

	int index = -1;

	// fire all prompts
	while ((index = str.find("%prompt%")) 	!= string::npos) str.replace(index, 8, cui::active ? cui::prompt() : [] () {
		string input;
		if (!pure) cout << "prompt :: ";
		getline(cin, input);
		log("Input :" + input);
		return input;
	}());

	while ((index = str.find("%N%")) 	!= string::npos) str.replace(index, 3, renotify ? "false" : "true");

	// replace %cvars% with their values
	for (auto it = cvar_base_s::cvars.begin(); it != cvar_base_s::cvars.end(); it++) {
		while ((index = str.find("%%" + it->first + "%%")) != string::npos)
			str.replace(index, 4 + it->first.length(), replace_special(*it->second));
		while ((index = str.find("%" + it->first + "%")) != string::npos)
			str.replace(index, 2 + it->first.length(), *it->second);
	}

	return str;
}

string replace_special(string str) {
	for (auto c : SPECIAL) {
		int index = 0;
		while ((index = str.find(c, index)) != string::npos) { str.replace(index, 1, "\\" + c); index += 2; }
	}

	return str;
}

// multistr_c functions
multistr_c::multistr_c(const string& str, const int& elen, const int& len) {
	wstring wstr = w_converter().from_bytes(str);
	this->init(wstr, elen, len);
}

multistr_c::multistr_c(const wstring& str, const int& elen, const int& len) {
	this->init(str, elen, len);
}

void multistr_c::init(const wstring& istr, const int& elen, int len) {
	if (len == -1) len = istr.length();
			
	this->data = vector<vector<wstring>>(len, vector<wstring>());
	this->shifts = vector<int>(len, 0);

	wstring buffer = L"";
	int iterator = 0;

	for (int i = 0; i < istr.length(); i++) {
		buffer += istr.at(i);

		if (buffer.length() == elen) {
			this->data.at(iterator % len).push_back(buffer);
			buffer = L"";
			iterator++;
		}
	}
}

void multistr_c::drop() {
	for (int& s : this->shifts) s = 0;
}

void multistr_c::reset() {
	this->drop();
	this->offset = 0;
}

int multistr_c::pos(const int& i) {
	return (this->offset + this->shifts.at(i)) % this->data.at(i).size();
}

wstring multistr_c::at(const int& i) {
	return this->data.at(i).at(this->pos(i));
}

wstring multistr_c::get(const int& i) {
	wstring ret = this->at(i);
	this->shift_at(i);

	return ret;
}

string multistr_c::s_get(const int& i) {
	return w_converter().to_bytes(this->get(i));
}

vector<wstring>& multistr_c::v_at(const int& i) {
	return this->data.at(i);
}

wstring multistr_c::str() {
	wstring ret = L"";

	for (int i = 0; i < this->length(); i++) ret += this->at(i);

	return ret;
}

void multistr_c::shift(const int& steps) { this->offset += steps; }

void multistr_c::shift_at(const int& index, const int& steps) {
	this->shifts.at(index) = (this->shifts.at(index) + steps) % this->data.at(index).size();
}

int multistr_c::length() { return this->data.size(); }

void multistr_c::append(const vector<wstring>& app) {
	this->data.push_back(app);
	this->shifts.push_back(0);
}
