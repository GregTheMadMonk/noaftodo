#ifndef NOAFTODO_CONFIG_H
#define NOAFTODO_CONFIG_H

#include <map>
#include <string>

extern std::string conf_filename;

extern std::map<std::string, std::string> conf_cvars;
extern std::map<std::string, std::string> conf_predefined_cvars;

void conf_load();
void conf_load(const std::string& conf_file);

void conf_set_cvar(const std::string& name, const std::string& value);
std::string conf_get_cvar(const std::string& name);
std::string conf_get_predefined_cvar(const std::string& name);

#endif
