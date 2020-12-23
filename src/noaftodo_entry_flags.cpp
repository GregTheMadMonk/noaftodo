#include "noaftodo_entry_flags.h"

using namespace std;

namespace li::entry_flags {

std::map<char, entry_flag::checker_f>& entry_flag::flags() {
	static std::map<char, entry_flag::checker_f> entry_flag_flags {};
	return entry_flag_flags;
}

entry_flag is_failed('F', [] (const entry& e, const time_s& t) {
	return !is_completed(e, t) && !is_nodue(e, t) && (e.due <= t);
});

entry_flag is_coming('C', [] (const entry& e, const time_s& t) {
	return !is_completed(e, t) && !is_failed(e, t) && !is_nodue(e, t) &&
		(e.due <= time_s(t.fmt_cmd() + "a" + e.get_meta("warn_time", "1d")));
});

entry_flag is_nodue('N', [] (const entry& e, const time_s& t) {
	return e.get_meta("nodue") == "true";
});

entry_flag is_completed('V', [] (const entry& e, const time_s& t) {
	return e.completed;
});

entry_flag is_uncat(' ', [] (const entry& e, const time_s& t) {
	return !is_completed(e, t) && !is_failed(e, t) && !is_coming(e, t) && !is_nodue(e, t);
});

}
