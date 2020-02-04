#ifndef NOAFTODO_OUTPUT_H
#define NOAFTOFO_OUTPUT_H

#include <string>

// log prefixes
constexpr char LP_DEFAULT = 'i';
constexpr char LP_ERROR = '!';

// notification priorities
constexpr int NP_LOW = 0;
constexpr int NP_MID = 1;
constexpr int NP_HIGH = 2;

void log(const std::string& message, const char& prefix = LP_DEFAULT);
void notify(const std::string& title, const std::string& description, const int& priority = NP_LOW);

#endif
