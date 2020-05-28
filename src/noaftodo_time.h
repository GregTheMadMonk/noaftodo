#ifndef NOAFTODO_TIME_H
#define NOAFTODO_TIME_H

#include <chrono>
#include <string>

long ti_to_long(const tm& t_tm);
long ti_to_long(const std::string& t_str);
tm ti_to_tm(const std::string& t_str);
tm ti_to_tm(const long& t_long);

std::string ti_f_str(const tm& t_tm);
std::string ti_f_str(const long& t_long);

std::string ti_cmd_str(const tm& t_tm);
std::string ti_cmd_str(const long& t_long);

#endif
