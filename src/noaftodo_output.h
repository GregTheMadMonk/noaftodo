#ifndef NOAFTODO_OUTPUT_H
#define NOAFTODO_OUTPUT_H

#include <string>

#include "noaftodo_list.h"

extern bool verbose;

// log prefixes
constexpr char LP_DEFAULT = 'i';
constexpr char LP_ERROR = '!';

void log(const std::string& message, const char& prefix = LP_DEFAULT);

std::string format_str(const std::string& str, const noaftodo_entry& li_entry, const bool& renotify = false);

#endif
