#include "noaftodo.h"

#include <cstring>
#include <iostream>
#include <pwd.h>
#include <sys/types.h>
#include <unistd.h>

#include "noaftodo_cmd.h"
#include "noaftodo_config.h"
#include "noaftodo_cui.h"
#include "noaftodo_cvar.h"
#include "noaftodo_daemon.h"
#include "noaftodo_list.h"
#include "noaftodo_time.h"

using namespace std;

int run_mode;

bool allow_root = false;

bool verbose = false;
bool enable_log = true;

wstring_convert<codecvt_utf8<wchar_t>, wchar_t> w_converter;

extern char _binary_doc_doc_gen_start;
extern char _binary_doc_doc_gen_end;

int main(int argc, char* argv[])
{
	setlocale(LC_ALL, "");

	run_mode = PM_DEFAULT;

	li_filename = string(getpwuid(getuid())->pw_dir) + "/.noaftodo-list";
	conf_filename = string(getpwuid(getuid())->pw_dir) + "/.config/noaftodo.conf";

	// parse arguments
	for (int i = 1; i < argc; i++)
	{
		// argument "-h, --help" - print help message
		if (strcmp(argv[i], "-h") * strcmp(argv[i], "--help") == 0) run_mode = PM_HELP;
		// argument "-d, --daemon" - start daemon
		else if (strcmp(argv[i], "-d") * strcmp(argv[i], "--daemon") == 0) run_mode = PM_DAEMON;
		// argument "-k, --kill-daemon" - kill daemon
		else if (strcmp(argv[i], "-k") * strcmp(argv[i], "--kill-daemon") == 0)
		{
			da_kill();
			return 0;
		}
		// argument "-r, --refire" - if daemon is running, re-fire startup events (like, notifications)
		else if (strcmp(argv[i], "-r") * strcmp(argv[i], "--refire") == 0)
		{
			da_send("N");
			return 0;
		} 
		// argument "-c, --config" - specify config file
		else if (strcmp(argv[i], "-c") * strcmp(argv[i], "--config") == 0)
		{
			if (i < argc - 1)
			{
				conf_filename = string(argv[i + 1]);
				i++;
			} else {
				log("Config not specified after " + string(argv[i]), LP_ERROR);
				return 1;
			}
		}
		// argument "-l, --list" - specify list file
		else if (strcmp(argv[i], "-l") * strcmp(argv[i], "--list") == 0)
		{
			if (i < argc - 1)
			{
				li_filename = string(argv[i + 1]);
				i++;
			} else {
				log("List file not specified after " + string(argv[i]), LP_ERROR);
				return 1;
			}
		}
		// argument "-v, --verbose" - print all messages
		else if (strcmp(argv[i], "-v") * strcmp(argv[i], "--verbose") == 0)
		{
			verbose = true;
		} else log("Unrecognized parameter \"" + string(argv[i]), LP_ERROR);
	}

	log(string(TITLE) + " v." + string(VERSION));

	if (run_mode == PM_HELP) 
	{
		print_help();
		return 0;
	}

	// init the command-line interpreter
	cmd_init();

	// load the config
	conf_load();

#ifndef NO_ROOT_CHECK
	if ((geteuid() == 0) && !allow_root)
	{
		log("Can't run as root! To disable this check, set \"allow_root\" to \"true\" in your config", LP_ERROR);
		return 0;
	}
#endif

	// load the list
	li_load();

	if (run_mode == PM_DEFAULT)	
	{
		// if this option is enabled, start daemon in the forked process
		if (da_fork_autostart && !da_check_lockfile())
			switch (fork())
			{
				case -1:
					log("Error creating fork for daemon", LP_ERROR);
					break;
				case 0: // child - daemon
					run_mode = PM_DAEMON;
					enable_log = false;
					da_clients = 1; // care about clients, shut down when there's none
					break;
				default:
					cui_run();
					break;
			}
		else 
		{ 
			da_send("S");
			cui_run(); 
		}
	}

	if (run_mode == PM_DAEMON)	da_run();

	return 0;
}

void print_help()
{
	vector<string> lines;
	lines.push_back("");

	const int TAB_W = 25;
	int tabs = 0;

	for (char *c = &_binary_doc_doc_gen_start; c < &_binary_doc_doc_gen_end; c++)
		switch(*c)
		{
			case '\n':
				lines.push_back("");
				tabs = 0;
				break;
			case '\t':
				tabs++;

				while (lines.at(lines.size() - 1).length() < TAB_W * tabs) lines[lines.size() - 1] += " ";
				break;
			default:
				lines[lines.size() - 1] += *c;
				break;
		}

#ifdef NO_MQUEUE
	cout << "Built with NO_MQUEUE" << endl;
#endif
	for (auto line : lines) cout << line << endl;
}

void log(const string& message, const char& prefix)
{	
	if (!enable_log) return;

	const bool wcui = cui_active;

	if ((prefix != LP_DEFAULT) || verbose)
	{
		if (wcui) cui_destroy();

		cout << "[" << ti_log_time() << "][" << prefix << "] " << message << endl;

		if (wcui) cui_construct();
	}
}

string format_str(string str, const noaftodo_entry& li_entry, const bool& renotify)
{
	int index = -1;

	// fire all prompts
	if (cui_active)
		while ((index = str.find("%prompt%")) 	!= string::npos) str.replace(index, 8, cui_prompt());

	// replace entry data
	if (li_entry != NULL_ENTRY)
	{
		while ((index = str.find("%T%")) 	!= string::npos) str.replace(index, 3, replace_special(li_entry.title));
		while ((index = str.find("%D%")) 	!= string::npos) str.replace(index, 3, replace_special(li_entry.description));
		while ((index = str.find("%due%")) 	!= string::npos) str.replace(index, 5, ti_cmd_str(li_entry.due));
		while ((index = str.find("%meta%"))	!= string::npos) str.replace(index, 6, li_entry.meta_str());
	}

	while ((index = str.find("%N%")) 	!= string::npos) str.replace(index, 3, renotify ? "false" : "true");

	// replace %cvars% with their values
	for (auto it = cvars.begin(); it != cvars.end(); it++)
		while ((index = str.find("%" + it->first + "%")) != string::npos)
			str.replace(index, 2 + it->first.length(), *it->second);

	return str;
}

string replace_special(string str)
{
	for (auto c : SPECIAL)
	{
		int index = 0;
		while ((index = str.find(c, index)) != string::npos) { str.replace(index, 1, "\\" + c); index += 2; }
	}

	return str;
}

// multistr_c functions
multistr_c::multistr_c(const vector<string>& init_list)
{
	this->vals = init_list;
}

string multistr_c::get(const int& position)
{
	if (position == -1)
	{
		const string retval = this->vals.at((this->offset + this->const_offset) % this->vals.size());

		this->shift();

		return retval;
	} else return this->vals.at(position % this->vals.size());
}

string multistr_c::full_str()
{
	string ret = "";

	for (auto s : this->vals) ret += s;

	return ret;
}

void multistr_c::shift(const int& steps)
{
	this->offset += steps;
	if (this->offset >= this->vals.size()) this->offset %= this->vals.size();
}

void multistr_c::shift_const(const int& steps)
{
	this->const_offset += steps;
	if (this->const_offset >= this->vals.size()) this->const_offset %= this->vals.size();
}

void multistr_c::reset()
{
	this->offset = 0;
}
