#ifndef NOAFTODO_H
#define NOAFTODO_H

#include <string>
#include <vector>

#include "noaftodo_list.h"

// program title and version
constexpr char TITLE[] = "NOAFtodo";

constexpr int LIST_V = 1;
constexpr int CONF_V = 3;
constexpr int MINOR_V = 4;
#define VERSION to_string(LIST_V) + "." + to_string(CONF_V) + "." + to_string(MINOR_V)

// program modes
constexpr int PM_DEFAULT = 0;
constexpr int PM_HELP = 1;
constexpr int PM_DAEMON = 2;

// log prefixes
constexpr char LP_DEFAULT = 'i';
constexpr char LP_IMPORTANT = 'I';
constexpr char LP_ERROR = '!';

// special characters
const std::vector<std::string> SPECIAL = 
{ 
	"\\",
	"\"", 
	";"
};

// program mode
extern int run_mode;

// output options
extern bool enable_log;
extern bool verbose;

void print_help();

void log(const std::string& message, const char& prefix = LP_DEFAULT);

std::string format_str(std::string str, const noaftodo_entry& li_entry, const bool& renotify = false);

std::string replace_special(std::string str);

#endif
