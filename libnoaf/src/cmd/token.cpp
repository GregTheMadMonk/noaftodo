#include "token.hpp"

using namespace std;

namespace noaf::cmd {

	map<token_type, string> tokens_base = {
		{ NEXTCMD,		"\\s*;\\s*" },
		{ INLINE_START,		"\\(\\s*" },
		{ INLINE_END,		"\\s*\\)" },
		{ LIST_START,		"\\[\\s*" },
		{ BASE_DELIMITER,	"\\s+" },
		{ VALUE_SPECIAL,	"\\\\." },
	};

	map<token_type, string> tokens_expr = {
		{ NEXTCMD,		"\\s*;\\s*" },
		{ INLINE_START,		"\\s*\\(\\s*" },
		{ INLINE_END,		"\\s*\\)" },
		{ LIST_START,		"\\s*\\[\\s*" },
		{ VALUE_SPECIAL,	"\\\\." },

		{ OP_ADD,		"\\s*\\+\\s*" },
		{ OP_SUB,		"\\s*-\\s*" },
		{ OP_MUL,		"\\s*\\*\\s*" },
		{ OP_DIV,		"\\s*\\/\\s*" },
		{ OP_EQ,		"\\s*=\\s*" },

		{ TNONE,		"\\s+" },	// if for some reason there are lose whitspaces in an
							// expression
	};

	map<token_type, string> tokens_list = {
		{ LIST_DELIMITER,	"\\s*,\\s*" },
		{ LIST_THROUGH,		"\\s*\\.\\.\\s*" },
		{ LIST_END,		"\\s*]" },
		{ VALUE_SPECIAL,	"\\\\." },
		{ INLINE_START,		"\\s*\\(\\s*" },
		{ INLINE_END,		"\\s*\\)\\s*" },
	};

	vector<token_type> op_order = {
		// priority from most prioritized to least
		OP_DIV,
		OP_MUL,
		OP_SUB,
		OP_ADD,
		OP_EQ,
	};

}
