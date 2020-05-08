#include "noaftodo_config.h"

#include <curses.h>
#include <fstream>
#include <vector>

#include "noaftodo.h"
#include "noaftodo_cmd.h"
#include "noaftodo_output.h"

using namespace std;

string conf_filename = "noaftodo.conf";

map<string, string> conf_cvars;
map<string, string> conf_predefined_cvars;

extern char _binary_noaftodo_conf_template_start;
extern char _binary_noaftodo_conf_template_end;

void conf_load()
{
	conf_load(conf_filename);
}

void conf_load(const string& conf_file)
{
	log("Loading config from " + conf_file + "...");

	ifstream iconf(conf_file);
	if (!iconf.good())
	{	// create a new config from a template
		log("Config file does not exist!", LP_ERROR);
		log("Creating a new one from template...");

		ofstream oconf(conf_file);
		// there must be a smarter way to do this
		oconf << "# " << TITLE << " v." << VERSION << " auto-generated config file" << endl;

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

	conf_predefined_cvars = conf_cvars;
}

void conf_set_cvar(const string& name, const string& value)
{
	log("Set " + name + "=" + value);
	conf_cvars[name] = value;
}

string conf_get_cvar(const string& name)
{
	try
	{
		return conf_cvars.at(name);
	} catch (const out_of_range& err) {
		log("No cvar with name " + name + " defined. Returning \"\".", LP_ERROR);
		return "";
	}
}

string conf_get_predefined_cvar(const string& name)
{
	try
	{
		return conf_predefined_cvars.at(name);
	} catch (const out_of_range& err) {
		log("No cvar with name " + name + " predefined. Returning \"\".", LP_ERROR);
		return "";
	}
}

void conf_set_cvar_int(const string& name, const int& value)
{
	log("Set " + name + "=" + to_string(value));
	conf_cvars[name] = to_string(value);
}

int conf_get_cvar_int(const string& name)
{
	try
	{
		return stoi(conf_cvars.at(name));
	} catch (const out_of_range& err) {
		log("No cvar with name " + name + " defined. Returning \"\".", LP_ERROR);
		return 0;
	} catch (const invalid_argument& err) {
		log("Cannot convert variable value to integer (" + name + "=" + conf_cvars.at(name) + ")", LP_ERROR);
		return 0;
	}
}

int conf_get_predefined_cvar_int(const string& name)
{
	try
	{
		return stoi(conf_predefined_cvars.at(name));
	} catch (const out_of_range& err) {
		log("No cvar with name " + name + " predefined. Returning \"\".", LP_ERROR);
		return 0;
	} catch (const invalid_argument& err) {
		log("Cannot convert variable value to integer (" + name + "=" + conf_predefined_cvars.at(name) + ")", LP_ERROR);
		return 0;
	}
}
