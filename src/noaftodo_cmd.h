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
extern std::map<std::string, std::string> aliases;

extern std::string cmd_buffer;

void cmd_init();
int cmd_exec(std::string command);
void cmd_terminate();

#endif
