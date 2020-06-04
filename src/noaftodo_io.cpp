#include "noaftodo_io.h"

#include <iostream>

#include "noaftodo.h"
#include "noaftodo_cui.h"
#include "noaftodo_time.h"

using namespace std;

bool verbose = false;

void log(const string& message, const char& prefix)
{	const bool wcui = cui_active;

	if (wcui) cui_destroy();

	if ((prefix != LP_DEFAULT) || verbose)
		cout << "[" << prefix << "] " << message << endl;

	if (wcui) cui_construct();
}

string format_str(const string& str, const noaftodo_entry& li_entry, const bool& renotify)
{
	string ret = str;
	int index = -1;
	while ((index = ret.find("%T%")) 	!= string::npos) ret.replace(index, 3, li_entry.title);
	while ((index = ret.find("%D%")) 	!= string::npos) ret.replace(index, 3, li_entry.description);
	while ((index = ret.find("%due%")) 	!= string::npos) ret.replace(index, 5, ti_cmd_str(li_entry.due));
	if (cui_active)
		while ((index = ret.find("%prompt%")) 	!= string::npos) ret.replace(index, 8, cui_prompt());

	while ((index = ret.find("%meta%"))	!= string::npos) ret.replace(index, 6, li_entry.meta_str());
	while ((index = ret.find("%id%")) 	!= string::npos) ret.replace(index, 4, to_string(cui_s_line));
	while ((index = ret.find("%VER%")) 	!= string::npos) ret.replace(index, 5, VERSION);
	while ((index = ret.find("%N%")) 	!= string::npos) ret.replace(index, 3, renotify ? "false" : "true");
	return ret;
}
