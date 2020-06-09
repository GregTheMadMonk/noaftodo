#ifndef NOAFTODO_OUTPUT_H
#define NOAFTODO_OUTPUT_H

#include <string>
#include <vector>

#include "noaftodo_list.h"

extern bool enable_log;
extern bool verbose;

// special characters
const std::vector<std::string> SPECIAL = 
{ 
	"\\",
	"\"", 
	";"
};

// log prefixes
constexpr char LP_DEFAULT = 'i';
constexpr char LP_IMPORTANT = 'I';
constexpr char LP_ERROR = '!';

void log(const std::string& message, const char& prefix = LP_DEFAULT);

std::string format_str(std::string str, const noaftodo_entry& li_entry, const bool& renotify = false);

std::string replace_special(std::string str);

#endif
