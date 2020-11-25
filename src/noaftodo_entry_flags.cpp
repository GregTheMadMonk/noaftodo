#include "noaftodo_entry_flags.h"

using namespace std;

namespace li::entry_flags {

std::map<string, entry_flag::checker_f>& entry_flag::flags() {
	static std::map<string, entry_flag::checker_f> entry_flag_flags {};
	return entry_flag_flags;
}

entry_flag is_failed("F", [] (const entry& e, const time_s& t) {
	return !e.completed && (e.get_meta("nodue") != "true") && (e.due <= t);
});

entry_flag is_coming("C", [] (const entry& e, const time_s& t) {
	return !e.completed && !is_failed(e, t) &&
		(e.get_meta("nodue") != "true") &&
		(e.due <= time_s(t.fmt_cmd() + "a" + e.get_meta("warn_time", "1d")));
});

entry_flag is_uncat("", [] (const entry& e, const time_s& t) {
	return !e.completed &&
		!is_failed(e, t) &&
		!is_coming(e, t) &&
		(e.get_meta("nodue") != "true");
});

}
