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

extern char _binary_noaftodo_conf_template_start;
extern char _binary_noaftodo_conf_template_end;

void conf_load(const bool& predefine_cvars)
{
	conf_load(conf_filename, predefine_cvars);
}

void conf_load(const string& conf_file, const bool& predefine_cvars)
{
	if (conf_file == "default")
	{
		log("Loading default config...");

		string entry = "";
		for (char* c = &_binary_noaftodo_conf_template_start; c < &_binary_noaftodo_conf_template_end; c++)
		{
			if (*c == '\n')
			{
				// new line
				if (entry != "")
				{
					while (entry.at(0) == ' ') 
					{ 
						entry = entry.substr(1);
						if (entry == "") break;
					}

					if (entry != "") if (entry.at(0) != '#')
						cmd_exec(entry);
				}

				entry = "";
			} else entry += *c;
		}
	} else {
		log("Loading config from " + conf_file + "...");

		ifstream iconf(conf_file);
		if (!iconf.good())
		{	// create a new config from a template
			log("Config file does not exist!", LP_ERROR);
			log("Creating a new one from template...");

			ofstream oconf(conf_file);
			// there must be a smarter way to do this
			oconf << "# " << TITLE << " v." << VERSION << " auto-generated config file" << endl;

			oconf << "ver " << CONF_V << endl;

			for (char* c = &_binary_noaftodo_conf_template_start; c < &_binary_noaftodo_conf_template_end; c++)
				oconf << *c;
		}

		iconf = ifstream(conf_file);
		if (!iconf.good())
		{
			log("Something went wrong!", LP_ERROR);
			return;
		}

		string entry;

		while (getline(iconf, entry))
		{
			if (entry != "")
			{
				while (entry.at(0) == ' ') 
				{ 
					entry = entry.substr(1);
					if (entry == "") break;
				}

				if (entry != "") if (entry.at(0) != '#')
					cmd_exec(entry);
			}
		}
	}

	cmd_terminate();

	if (!predefine_cvars) return;

	for (auto it = cvars.begin(); it != cvars.end(); it++)
		cvar(it->first).predefine();
}
