#include "noaftodo_config.h"

#ifdef __sun
#include <ncurses/curses.h>
# else
#include <curses.h>
#endif
#include <fstream>
#include <vector>

#include "noaftodo.h"
#include "noaftodo_cmd.h"
#include "noaftodo_cvar.h"

using namespace std;

string conf_filename = "noaftodo.conf";

extern string TCONF;

void conf_load(const bool& predefine_cvars, const bool& create_template)
{
	conf_load(conf_filename, predefine_cvars, create_template);
}

void conf_load(const string& conf_file, const bool& predefine_cvars, const bool& create_template)
{
	if (conf_file == "default")
	{
		log("Loading default config...");

		string entry = "";
		for (char c : TCONF)
		{
			if (c == '\n')
			{
				cmd_exec(entry);
				entry = "";
			} else entry += c;
		}
		cmd_exec(entry);
	} else {
		log("Loading config from " + conf_file + "...");

		ifstream iconf(conf_file);
		if (!iconf.good())
		{	// create a new config from a template
			log("Config file does not exist (" + conf_file + ")!", LP_ERROR);
			if (!create_template) return;
			log("Creating a new one from template...");

			ofstream oconf(conf_file);
			// there must be a smarter way to do this
			oconf << "# " << TITLE << " v." << VERSION << " auto-generated config file" << endl;

			oconf << "ver " << CONF_V << endl;

			for (char c : TCONF) oconf << c;
		}

		iconf = ifstream(conf_file);
		if (!iconf.good())
		{
			log("Something went wrong!", LP_ERROR);
			return;
		}

		string entry;
		while (getline(iconf, entry))
			cmd_exec(entry);
	}

	cmd_terminate();

	if (!predefine_cvars) return;

	for (auto it = cvars.begin(); it != cvars.end(); it++)
		if ((cvar(it->first).flags & CVAR_FLAG_NO_PREDEF) == 0)
			cvar(it->first).predefine();
}
