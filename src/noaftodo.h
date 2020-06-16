#ifndef NOAFTODO_H
#define NOAFTODO_H

#include <codecvt>
#include <locale>
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

// "multistr" class
class multistr_c
{
	std::vector<std::string> vals;
	int offset = 0;
public:
	multistr_c(const std::vector<std::string>& init_list);

	std::string get();
	std::string full_str();
	void shift();
	void reset();
};

// special characters
const std::vector<std::string> SPECIAL = 
{ 
	"\\",
	"\"", 
	";"
};

// string <-> wstring converter
extern std::wstring_convert<std::codecvt_utf8<wchar_t>, wchar_t> w_converter;

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
