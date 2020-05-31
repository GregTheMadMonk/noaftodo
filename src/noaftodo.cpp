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

int main(int argc, char* argv[])
{
	setlocale(LC_ALL, "");

	log(string(TITLE) + " v." + string(VERSION));

	int mode = PM_DEFAULT;

	li_filename = string(getpwuid(getuid())->pw_dir) + "/.noaftodo-list";
	conf_filename = string(getpwuid(getuid())->pw_dir) + "/.config/noaftodo.conf";

	// parse arguments
	for (int i = 1; i < argc; i++)
	{
		if (strcmp(argv[i], "-h") * strcmp(argv[i], "--help") == 0) mode = PM_HELP;
		else if (strcmp(argv[i], "-d") * strcmp(argv[i], "--daemon") == 0) mode = PM_DAEMON;
		else if (strcmp(argv[i], "-k") * strcmp(argv[i], "--kill-daemon") == 0)
		{
			da_kill();
			return 0;
		}
		else if (strcmp(argv[i], "-r") * strcmp(argv[i], "--refire") == 0)
		{
			da_send("N");
			return 0;
		} 
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
		else if (strcmp(argv[i], "-v") * strcmp(argv[i], "--verbose") == 0)
		{
			verbose = true;
		} else log("Unrecognized parameter \"" + string(argv[i]), LP_ERROR);
	}

	if (mode == PM_HELP) 
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

	if (mode == PM_DEFAULT)	cui_run();

	if (mode == PM_DAEMON)	da_run();

	return 0;
}

void print_help()
{
	cout << "Usage: noaftodo [OPTIONS]" << endl;
	cout << "Command-line options:" << endl;
	cout << "\t-h, --help - print this message" << endl;
	cout << "\t-c, --config - specify config file after this parameter" << endl;
	cout << "\t-l, --list - specify list file after this parameter" << endl;
	cout << "\t-d, --daemon - start " << TITLE << " daemon" << endl;
	cout << "\t-k, --kill-daemon - kill " << TITLE << " daemon" << endl;
	cout << "\t-r, --refire - if daemon is running, re-fire startup events" << endl;
	cout << "\t-v, --verbose - print all messages" << endl;
}
