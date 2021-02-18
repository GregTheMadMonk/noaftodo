#include "log.hpp"

#include <iostream>

#include <cmd.hpp>
#include <hooks.hpp>
#include <time_s.hpp>

using namespace std;

namespace noaf {
	log_stream::log_stream() : enabled(true),
			pure(false),
			sendreset(true),
			verbosity(1),
			offset(0),
			level(0),
			flusher([] (const string& message) {
				ui::pause();
				cout << message;
				ui::resume();
				})
	{}

	void log_stream::flush() {
		if (level >= verbosity) {
			if (pure) flusher(buffer.str());
			else {
				string prefix = "[" + time_s().fmt("%H:%M:%S") + "][" + to_string(level) + "][";

				for (int i = 0; i < 10; i++)
					if (i < cmd::ret.length()) prefix += cmd::ret.at(i);
					else prefix += " ";

				prefix += "]\t";

				for (int i = 0; i < offset; i++) prefix += "\t";
				flusher(prefix + buffer.str());
			}
		}
		if (sendreset) level = 0;
		buffer.str("");
	}

	log_stream log;

	log_modifier lend= [] (log_stream& s) {
		s << "\n";
		s.flush();
	};

	log_modifier ll = [] (log_stream& s) {
		if (s.offset > 0) s.offset --;
	};

	log_modifier lr = [] (log_stream& s) {
		s.offset++;
	};

	log_modifier llev(const int& new_level) {
		return [&new_level] (log_stream& s) {
			s.level = new_level;
		};
	}

	log_modifier lver(const int& new_verbosity) {
		return [&new_verbosity] (log_stream& s) {
			s.verbosity = new_verbosity;
		};
	}
}
