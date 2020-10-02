#ifndef NOAFTODO_TIME_H
#define NOAFTODO_TIME_H

#include <chrono>
#include <functional>
#include <string>

class time_s {
	time_t time;
public:
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
		l_ti.tm_mon	= t_long % (long)1e8 / 1e6;
		l_ti.tm_year	= t_long / (long)1e8;

		this->time = mktime(&l_ti);
	}

	// construct from cmd string
	time_s(const std::string& t_str) {
		const time_t now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
		tm ti = *localtime(&now);

		ti.tm_mon += 1;
		ti.tm_year += 1900;

		int buffer = 0;
		bool input = false;
		bool absolute = true;

		const auto put_field = [&buffer, &input, &absolute] (int& to) {
			if (input) {
				if (absolute) to = buffer;
				else to += buffer;
			}

			buffer = 0;
			input = false;
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
			}
		}

		ti.tm_mon -= 1;
		ti.tm_year -= 1900;

		this->time = mktime(&ti);
	}

	// construct long value of format YYYYMMDDHHMM. To be deprecated in 2.0.0
	long to_long() {
		const tm l_ti = *localtime(&(this->time));
		return l_ti.tm_min +
			l_ti.tm_hour * 1e2 +
			l_ti.tm_mday * 1e4 +
			l_ti.tm_mon * 1e6 +
			l_ti.tm_year * 1e8;
	}

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
	std::string fmt(const fmt_f_t& fmt_f) {
		const tm l_ti = *localtime(&(this->time));

		return fmt_f(l_ti.tm_year + 1900, // tm_year = current_year - 1900
			l_ti.tm_mon + 1, // tm_mon = month - 1
			l_ti.tm_mday,
			l_ti.tm_hour,
			l_ti.tm_min,
			l_ti.tm_sec);
	}

	// formatters
	// to command interpreter date format
	std::string fmt_cmd() {
		return this->fmt([] (FMT_F_ARGS) {
			return std::to_string(y) + "y" +
				std::to_string(M) + "m" +
				std::to_string(d) + "d" +
				std::to_string(h) + "h" +
				std::to_string(m);
		});
	}

	// to be printed on UI: (YYYY/MM/DD HH:MM)
	std::string fmt_ui() {
		return this->fmt([] (FMT_F_ARGS) {
			return std::to_string(y) + "/" +
				((M < 10) ? "0" : "") + std::to_string(M) + "/" +
				((d < 10) ? "0" : "") + std::to_string(d) + " " +
				((h < 10) ? "0" : "") + std::to_string(h) + ":" +
				((m < 10) ? "0" : "") + std::to_string(m);
		});
	}

	// to be printed in log
	std::string fmt_log() {
		return this->fmt([] (FMT_F_ARGS) {
			return ((h < 10) ? "0" : "") + std::to_string(h) + ":" +
				((m < 10) ? "0" : "") + std::to_string(m) + ":" +
				((s < 10) ? "0" : "") + std::to_string(s);
		});
	}
};

std::string ti_log_time();

long ti_to_long(const tm& t_tm);
long ti_to_long(const std::string& t_str);
tm ti_to_tm(const std::string& t_str);
tm ti_to_tm(const long& t_long);

std::string ti_f_str(const tm& t_tm);
std::string ti_f_str(const long& t_long);

std::string ti_cmd_str(const tm& t_tm);
std::string ti_cmd_str(const long& t_long);

#endif
