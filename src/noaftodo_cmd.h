#ifndef NOAFTODO_CMD_H
#define NOAFTODO_CMD_H

#include <functional>
#include <map>
#include <string>
#include <vector>

constexpr int CMD_ERR_ARG_COUNT 	= 1;
constexpr int CMD_ERR_ARG_TYPE 		= 2;
constexpr int CMD_ERR_EXTERNAL		= -1;

extern std::map<std::string, std::function<int(const std::vector<std::string>& args)>> cmd_cmds;
extern std::map<std::string, std::string> cmd_aliases;

extern std::string cmd_buffer;

void cmd_init();
std::vector<std::string> cmd_break(const std::string& cmdline); // breaks the line into single commands
								// respecting quotes and '\'

void cmd_run(std::string command);	// if string consists of only one command, executes it,
					// otherwise calls inself on each element of cmd_break output

void cmd_exec(const std::string& command);	// initializes and starts execution of a command.
						// respects non-null cmd_buffer
void cmd_terminate();

#endif
