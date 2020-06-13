#include "noaftodo_io.h"

#include <iostream>

#include "noaftodo.h"
#include "noaftodo_config.h"
#include "noaftodo_cui.h"
#include "noaftodo_time.h"

using namespace std;

bool verbose = false;
bool enable_log = true;

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
	while ((index = str.find("%T%")) 	!= string::npos) str.replace(index, 3, replace_special(li_entry.title));
	while ((index = str.find("%D%")) 	!= string::npos) str.replace(index, 3, replace_special(li_entry.description));
	while ((index = str.find("%due%")) 	!= string::npos) str.replace(index, 5, ti_cmd_str(li_entry.due));
	while ((index = str.find("%meta%"))	!= string::npos) str.replace(index, 6, li_entry.meta_str());
	while ((index = str.find("%id%")) 	!= string::npos) str.replace(index, 4, to_string(cui_s_line));

	while ((index = str.find("%VER%")) 	!= string::npos) str.replace(index, 5, VERSION);
	while ((index = str.find("%N%")) 	!= string::npos) str.replace(index, 3, renotify ? "false" : "true");

	// replace %cvars% with their values
	for (auto it = conf_cvars.begin(); it != conf_cvars.end(); it++)
		while ((index = str.find("%" + it->first + "%")) != string::npos)
			str.replace(index, 2 + it->first.length(), it->second);

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
