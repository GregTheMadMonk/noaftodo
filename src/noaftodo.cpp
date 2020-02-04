#include "noaftodo.h"

#include <cstring>
#include <iostream>
#include <libnotify/notify.h>
#include <pwd.h>
#include <string>
#include <sys/types.h>
#include <unistd.h>

#include "noaftodo_cui.h"
#include "noaftodo_daemon.h"
#include "noaftodo_list.h"
#include "noaftodo_output.h"

using namespace std;

int main(int argc, char* argv[])
{
	setlocale(LC_ALL, "");

	log(string(TITLE) + " v." + string(VERSION));

	int mode = PM_DEFAULT;

	li_filename = string(getpwuid(getuid())->pw_dir) + "/.noaftodo-list";

	// parse arguments
	for (int i = 1; i < argc; i++)
	{
		if (strcmp(argv[i], "-h") * strcmp(argv[i], "--help") == 0) mode = PM_HELP;
		if (strcmp(argv[i], "-d") * strcmp(argv[i], "--daemon") == 0) mode = PM_DAEMON;
		if (strcmp(argv[i], "-k") * strcmp(argv[i], "--kill-daemon") == 0)
		{
			da_kill();
			return 0;
		}
	}

	if (mode == PM_HELP) 
	{
		print_help();
		return 0;
	}

	// load the list
	li_load();

	if (mode == PM_DEFAULT)
	{
		cui_run();
	}

	if (mode == PM_DAEMON)
	{
		notify_init(TITLE);
		da_run();
	}

	return 0;
}

void print_help()
{
	cout << "Usage: noaftodo [OPTIONS]" << endl;
	cout << "Command-line options:" << endl;
	cout << "\t-h, --help - print this message" << endl;
	cout << "\t-d, --daemon - start " << TITLE << " daemon" << endl;
	cout << "\t-k, --kill-daemon - kill " << TITLE << " daemon" << endl;
}
