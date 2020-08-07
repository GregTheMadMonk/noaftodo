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

using cmd::exec;

extern string TCONF;

namespace conf {

void load(const bool& predefine_cvars, const bool& create_template) {
	load(filename, predefine_cvars, create_template);
}

void load(const string& file, const bool& predefine_cvars, const bool& create_template) {
	if (file == "default") {
		log("Loading default config...");

		string entry = "";
		for (char c : TCONF) {
			if (c == '\n') {
				exec(entry);
				entry = "";
			} else entry += c;
		}
		exec(entry);
	} else {
		log("Loading config from " + file + "...");

		ifstream iconf(file);
		if (!iconf.good())
		{	// create a new config from a template
			log("Config file does not exist (" + file + ")!", LP_ERROR);
			if (!create_template) return;
			log("Creating a new one from template...");

			ofstream oconf(file);
			// there must be a smarter way to do this
			oconf << "# " << TITLE << " v." << VERSION << " auto-generated config file" << endl;

			oconf << "ver " << CONF_V << endl;

			for (char c : TCONF) oconf << c;
		}

		iconf = ifstream(file);
		if (!iconf.good()) {
			log("Something went wrong!", LP_ERROR);
			return;
		}

		string entry;
		while (getline(iconf, entry))
			exec(entry);
	}

	cmd::terminate();

	if (!predefine_cvars) return;

	for (auto it = cvar_base_s::cvars.begin(); it != cvar_base_s::cvars.end(); it++)
		if ((cvar(it->first).flags & CVAR_FLAG_NO_PREDEF) == 0)
			cvar(it->first).predefine();
}

}
