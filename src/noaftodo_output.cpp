#include "noaftodo_output.h"

#include <iostream>

#include "noaftodo.h"

using namespace std;

void log(const string& message, const char& prefix)
{
	cout << "[" << prefix << "] " << message << endl;
}

string format_str(const string& str, const noaftodo_entry& li_entry, const bool& renotify)
{
	string ret = str;
	int index = -1;
	while ((index = ret.find("%T%")) != string::npos) ret.replace(index, 3, li_entry.title);
	while ((index = ret.find("%D%")) != string::npos) ret.replace(index, 3, li_entry.description);
	while ((index = ret.find("%VER%")) != string::npos) ret.replace(index, 5, VERSION);
	while ((index = ret.find("%N%")) != string::npos) ret.replace(index, 3, renotify ? "false" : "true");
	return ret;
}
