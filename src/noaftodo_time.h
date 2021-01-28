#ifndef NOAFTODO_TIME_H
#define NOAFTODO_TIME_H

#include <chrono>
#include <functional>
#include <string>

#include "noaftodo_macro.h"

struct time_s {
	time_t time;

	// default constructor initializes time structure with current time
	time_s() {
		this->time = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
	}

	// constructor from a long
	time_s(const long& t_long) {
		tm l_ti = { 0 };

		l_ti.tm_sec	= 0;
		l_ti.tm_min	= t_long % (long)1e2;
		l_ti.tm_hour	= t_long % (long)1e4 / 1e2;
		l_ti.tm_mday	= t_long % (long)1e6 / 1e4;
		l_ti.tm_mon	= t_long % (long)1e8 / 1e6 - 1;
		l_ti.tm_year	= t_long / (long)1e8 - 1900;

		this->time = mktime(&l_ti);
	}

	// construct from cmd string
	time_s(const std::string& t_str) {
		const time_t now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
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

		this->time = mktime(&ti);
	}

	// construct long value of format YYYYMMDDHHMM. To be deprecated in 2.0.0
	CONST_DPL(long to_long(), {
		const tm l_ti = *localtime(&(this->time));
		return l_ti.tm_min +
			l_ti.tm_hour * 1e2 +
			l_ti.tm_mday * 1e4 +
			(l_ti.tm_mon + 1) * 1e6 +
			(l_ti.tm_year + 1900) * 1e8;
	})

	// return localtime tm struct
	CONST_DPL(auto to_tm(), { return *localtime(&this->time); })

	// format function argument list
	#define FMT_F_ARGS const unsigned& y, \
		const unsigned& M, \
		const unsigned& d, \
		const unsigned& h, \
		const unsigned& m, \
		const unsigned& s
	// format function type
	typedef std::function<std::string(FMT_F_ARGS)> fmt_f_t;
	// format string
	CONST_DPL(std::string fmt(const fmt_f_t& fmt_f), {
		const tm l_ti = *localtime(&(this->time));

		return fmt_f(l_ti.tm_year + 1900, // tm_year = current_year - 1900
			l_ti.tm_mon + 1, // tm_mon = month - 1
			l_ti.tm_mday,
			l_ti.tm_hour,
			l_ti.tm_min,
			l_ti.tm_sec);
	})

	// formatters
	// to command interpreter date format
	CONST_DPL(std::string fmt_cmd(), {
		return this->fmt([] (FMT_F_ARGS) {
			char buffer[64];
			sprintf(buffer, "%dy%dm%dd%dh%d", y, M, d, h, m);
			return std::string(buffer);
		});
	})

	// to be printed on UI: (YYYY/MM/DD HH:MM)
	CONST_DPL(std::string fmt_ui(), {
		return this->fmt([] (FMT_F_ARGS) {
			char buffer[64];
			sprintf(buffer, "%04d/%02d/%02d %02d:%02d", y, M, d, h, m);
			return std::string(buffer);
		});
	})

	// to be printed in log
	CONST_DPL(std::string fmt_log(), {
		return this->fmt([] (FMT_F_ARGS) {
			char buffer[64];
			sprintf(buffer, "%02d:%02d:%02d", h, m, s);
			return std::string(buffer);
		});
	})

	// using a custom sprintf expression
	CONST_DPL(std::string fmt_sprintf(const std::string& format), {
		return this->fmt([&format] (FMT_F_ARGS) {
			char buffer[64];
			sprintf(buffer, format.c_str(), y, M, d, h, m, s);
			return std::string(buffer);
		});
	})

	// comparsement
	CONST_DPL(bool operator<(const time_s& op2), { return this->time < op2.time; })
	CONST_DPL(bool operator>(const time_s& op2), { return this->time > op2.time; })
	CONST_DPL(bool operator<=(const time_s& op2), { return this->time <= op2.time; })
	CONST_DPL(bool operator>=(const time_s& op2), { return this->time >= op2.time; })
	CONST_DPL(bool operator==(const time_s& op2), { return this->time == op2.time; })
	CONST_DPL(bool operator!=(const time_s& op2), { return this->time != op2.time; })

	// difference between two time marks
	struct time_diff_s { tm diff; bool sign; /* sign = true - positive, false - negative */ };
	CONST_DPL(time_diff_s operator-(const time_s& op), {
			time_diff_s ret;

			ret.sign = (this->time >= op.time);

			const auto tm_to	= ret.sign ? this->to_tm() : op.to_tm();
			const auto tm_from	= ret.sign ? op.to_tm() : this->to_tm();

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
	});
};

#endif
