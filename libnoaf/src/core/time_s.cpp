#include "time_s.hpp"

#include <chrono>

using namespace std;

namespace noaf {
	time_s::time_s() {
		time = chrono::system_clock::to_time_t(chrono::system_clock::now());
	}

	time_s::time_s(const long& t_long) {
		tm l_ti = { 0 };

		l_ti.tm_sec	= 0;
		l_ti.tm_min	= t_long % (long)1e2;
		l_ti.tm_hour	= t_long % (long)1e4 / 1e2;
		l_ti.tm_mday	= t_long % (long)1e6 / 1e4;
		l_ti.tm_mon	= t_long % (long)1e8 / 1e6 - 1;
		l_ti.tm_year	= t_long / (long)1e8 - 1900;

		time = mktime(&l_ti);
	}

	time_s::time_s(const string& t_str) {
		const time_t now = chrono::system_clock::to_time_t(chrono::system_clock::now());
		tm ti = *localtime(&now);

		ti.tm_mon += 1;
		ti.tm_year += 1900;

		time_t buffer = 0;
		bool input = false;
		bool absolute = true;
		bool negative = false;

		const auto put_field = [&buffer, &input, &absolute, &negative] (int& to) {
			if (negative) buffer = -buffer;
			if (input) {
				if (absolute) to = buffer;
				else to += buffer;
			}

			buffer = 0;
			input = false;
			negative = false;
		};

		for (int i = 0; i <= t_str.length(); i++) {
			const char c = (i == t_str.length()) ? 'a' : t_str.at(i);
			if (isdigit(c)) {
				buffer = buffer * 10 + (c - '0');
				input = true;
			} else switch (c) {
				case 'a':
					// 'a' also finishes minutes input
					put_field(ti.tm_min);

					// switch between absolute and relative modes
					absolute = !absolute;
					break;
				case '-':
					negative = !negative;
					break;
				case 'h':
					put_field(ti.tm_hour);
					break;
				case 'd':
					put_field(ti.tm_mday);
					break;
				case 'm':
					put_field(ti.tm_mon);
					break;
				case 'y':
					put_field(ti.tm_year);
					break;
				case 'e':
					// 'e' means that previous input should be interpreted
					// as seconds since epoch began
					time_t secs = mktime(&ti);
					if (negative) buffer = -buffer;
					if (input) {
						if (absolute) secs = buffer;
						else secs += buffer;
					}

					buffer = 0;
					input = false;
					negative = false;

					ti = *localtime(&secs);
					ti.tm_mon += 1;
					ti.tm_year += 1900;
					break;
			}
		}

		ti.tm_mon -= 1;
		ti.tm_year -= 1900;

		ti.tm_sec = 0; // command-line time format doesm't include seconds

		time = mktime(&ti);
	}

	tm time_s::to_tm() {
		return *localtime(&time);
	}
	tm time_s::to_tm() const {
		return *localtime(&time);
	}

	string time_s::fmt(string format) const {
		const tm l_ti = *localtime(&time);
		int index = -1;

		const auto to_string2 = [] (const int& i) {
			return ((i < 10) ? "0" : "") + to_string(i);
		};

		while ((index = format.find("%H")) != string::npos)
			format.replace(index, 2, to_string2(l_ti.tm_hour));

		while ((index = format.find("%M")) != string::npos)
			format.replace(index, 2, to_string2(l_ti.tm_min));

		while ((index = format.find("%S")) != string::npos)
			format.replace(index, 2, to_string2(l_ti.tm_sec));

		while ((index = format.find("%d")) != string::npos)
			format.replace(index, 2, to_string2(l_ti.tm_mday));

		while ((index = format.find("%m")) != string::npos)
			format.replace(index, 2, to_string2(l_ti.tm_mon + 1));

		while ((index = format.find("%Y")) != string::npos)
			format.replace(index, 2, to_string2(l_ti.tm_year + 1900));

		return format;
	}
	string time_s::fmt(string format) {
		return static_cast<const time_s&>(*this).fmt(format);
	}

	bool time_s::operator<(const time_s& rval) const	{ return time < rval.time; }
	bool time_s::operator<(const time_s& rval)		{ return time < rval.time; }
	bool time_s::operator>(const time_s& rval) const	{ return time > rval.time; }
	bool time_s::operator>(const time_s& rval)		{ return time > rval.time; }
	bool time_s::operator<=(const time_s& rval) const	{ return time <= rval.time; }
	bool time_s::operator<=(const time_s& rval)		{ return time <= rval.time; }
	bool time_s::operator>=(const time_s& rval) const	{ return time >= rval.time; }
	bool time_s::operator>=(const time_s& rval)		{ return time >= rval.time; }
	bool time_s::operator==(const time_s& rval) const	{ return time == rval.time; }
	bool time_s::operator==(const time_s& rval)		{ return time == rval.time; }
	bool time_s::operator!=(const time_s& rval) const	{ return time != rval.time; }
	bool time_s::operator!=(const time_s& rval)		{ return time != rval.time; }

	time_s::time_diff_s time_s::operator-(const time_s& rval) const {
		time_diff_s ret;

		ret.sign = (time >= rval.time);

		const auto tm_to	= ret.sign ? to_tm() : rval.to_tm();
		const auto tm_from	= ret.sign ? rval.to_tm() : to_tm();

		// amount of days in a month
		auto mdays = [] (const int& month, const int& year) {
			if (month == 2) {
				if ((year % 400 == 0) || ((year % 4 == 0) && (year % 100 != 0)))
					return 29;
				return 28;
			}

			// higher forces forgive me for this
			for (const auto i : { 1, 3, 5, 7, 8, 10, 12 })
				if (i == month) return 31;

			return 30;
		};

		ret.diff.tm_year= tm_to.tm_year - tm_from.tm_year;
		ret.diff.tm_mon	= tm_to.tm_mon - tm_from.tm_mon;
		ret.diff.tm_mday= tm_to.tm_mday - tm_from.tm_mday;
		ret.diff.tm_hour= tm_to.tm_hour - tm_from.tm_hour;
		ret.diff.tm_min	= tm_to.tm_min - tm_from.tm_min;	

		if (ret.diff.tm_min < 0) {
			ret.diff.tm_hour--;
			ret.diff.tm_min += 60;
		}
		if (ret.diff.tm_hour < 0) {
			ret.diff.tm_mday--;
			ret.diff.tm_hour += 24;
		}
		if (ret.diff.tm_mday < 0) {
			ret.diff.tm_mon--;
			ret.diff.tm_mday += mdays(tm_from.tm_mon + 1, tm_from.tm_year + 1900);
		}
		if (ret.diff.tm_mon < 0) {
			ret.diff.tm_year--;
			ret.diff.tm_mon += 12;
		}

		return ret;
	}
}
