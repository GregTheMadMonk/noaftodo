#ifndef NOAFTODO_CMD_H
#define NOAFTODO_CMD_H

#include <functional>
#include <map>
#include <string>
#include <vector>

#include "noaftodo_list.h"

namespace cmd {

constexpr int ERR_EXTERNAL	= 1;
constexpr int ERR_ARG_COUNT 	= 2;
constexpr int ERR_ARG_TYPE 	= 3;

extern std::map<std::string, std::function<int(const std::vector<std::string>& args)>> cmds;
extern std::map<std::string, std::string> aliases;

extern std::string retval;
extern std::string buffer;

extern li::entry* sel_entry;

void init();
std::vector<std::string> cmdbreak(const std::string& cmdline); // breaks the line into single commands
								// respecting quotes and '\'

void run(std::string command);	// if string consists of only one command, executes it,
				// otherwise calls inself on each element of cmd_break output

void exec(const std::string& command);	// initializes and starts execution of a command.
					// respects non-null cmd_buffer
void terminate();

}

#endif
