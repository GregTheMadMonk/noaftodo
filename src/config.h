#ifndef NOAFTODO_CONFIG_H
#define NOAFTODO_CONFIG_H

#include <map>
#include <string>

namespace conf {

extern std::string filename;

void load(const bool& predefine_cvars = true, const bool& create_template = false);
void load(const std::string& conf_file, const bool& predefine_cvars = true, const bool& create_template = false);

}

#endif
