#ifndef NOAFTODO_ENTRY_FLAGS
#define NOAFTODO_ENTRY_FLAGS

#include "noaftodo_list.h"

namespace li::entry_flags {

class entry_flag {
	typedef std::function<bool(const entry& e, const time_s& time)> checker_f;
	char display;

	public:
	entry_flag(const char& d, const checker_f& ch) : display(d) {
		flags()[d] = ch;
	}

	static std::map<char, checker_f>& flags();

	CONST_DPL(bool operator()(const entry& e), {
		return entry_flag::flags().at(this->display)(e, time_s());
	})
	CONST_DPL(bool operator()(const entry& e, const time_s& t), {
		return entry_flag::flags().at(this->display)(e, t);
	})
};

extern entry_flag is_failed;
extern entry_flag is_coming;
extern entry_flag is_uncat;

}

#endif
