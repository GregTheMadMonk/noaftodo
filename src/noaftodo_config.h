#ifndef NOAFTODO_CONFIG_H
#define NOAFTODO_CONFIG_H

#include <map>
#include <string>

extern std::string conf_filename;

void conf_load(const bool& predefine_cvars = true);
void conf_load(const std::string& conf_file, const bool& predefine_cvars = true);
#endif
