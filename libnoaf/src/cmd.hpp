#pragma once
#ifndef NOAF_CMD_H
#define NOAF_CMD_H

#include <functional>
#include <map>
#include <string>

namespace noaf::cmd {

	extern bool allow_system; // should sytem commands be allowed

	struct cmd {
		std::function<std::string(const std::vector<std::string>& args)> cmd_f; // command function
		std::string tooltip;
	};

	std::map<std::string, cmd>&		cmds();		// commands index
	std::map<std::string, std::string>&	aliases();	// aliases index

	extern std::string buffer; // buffer to store unterminated command e.g. when
					// command is spread across multiple lines
	
	extern std::string msg;	// message with current command interpreter status (usually just command output)
	extern std::string ret;	// last command return value

	void exec(const std::string& command); // initalize command execution (complex command string)
	
	void terminate(); // signals the end of command input. Resets buffer

}

#endif
