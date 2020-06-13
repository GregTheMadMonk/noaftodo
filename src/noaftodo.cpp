#include "noaftodo.h"

#include <cstring>
#include <iostream>
#include <pwd.h>
#include <string>
#include <sys/types.h>
#include <unistd.h>

#include "noaftodo_cmd.h"
#include "noaftodo_config.h"
#include "noaftodo_cui.h"
#include "noaftodo_daemon.h"
#include "noaftodo_list.h"
#include "noaftodo_io.h"

using namespace std;

int run_mode;

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

	// load the list
	li_load();

	if (run_mode == PM_DEFAULT)	
	{
		// if this option is enabled, start daemon in the forked process
		if ((conf_get_cvar("daemon.fork_autostart") == "true") &&
		   !da_check_lockfile())
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
