#ifndef NOAFTODO_CMD_H
#define NOAFTODO_CMD_H

#include <functional>
#include <map>
#include <string>
#include <vector>

constexpr int CMD_ERR_ARG_COUNT 	= 1;
constexpr int CMD_ERR_ARG_TYPE 		= 2;
constexpr int CMD_ERR_EXTERNAL		= -1;

extern std::map<std::string, std::function<int(const std::vector<std::string>& args)>> cmds;

void cmd_init();
int cmd_exec(const std::string& command);

#endif
