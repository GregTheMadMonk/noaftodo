#pragma once
#ifndef NOAF_CMD_TOKEN_H
#define NOAF_CMD_TOKEN_H

#include <string>

namespace noaf::cmd {

	// all the different types of tokens
	enum token_type : char {
		// empty token
		TNONE = 0,

		// command termination
		NEXTCMD = ';',

		// start and end of an inline block (...)
		INLINE_START = '(',
		INLINE_END = ')',

		// start and end of a {...} block (TODO)
		BLOCK_START = '{',
		BLOCK_END = '}',

		// mode start tokens
		BASE_START = 'b',
		EXPR_START = 'E',
		LIST_START = 'l',

		// base mode tokens
		BASE_DELIMITER = ' ',


		// expression mode tokens
		// expression operators
		OP_ADD = '+',
		OP_SUB = '-',
		OP_MUL = '*',
		OP_DIV = '/',
		OP_EQ = '=',

		// list mode tokens
		LIST_END = 'e',
		LIST_DELIMITER = ',',
		LIST_THROUGH = '.',

		// unversal tokens for values
		VALUE = 'V',
		VALUE_SPECIAL = '\\',

		// references
		REFERENCE = 'R',	// references the top of previously executed commands return values
		LIST_REFERENCE = 'L',	// references the list

		// execution flow control
		CALL = 'C',	// call the command
		EVAL = 'v',	// evaluate the expression
		CLRRET = 'c',	// clear the return values queue
	};

	// structure for token: some tokens contain string data
	struct token {
		token_type type;
		std::string value;
	};

}
#endif
