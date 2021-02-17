#pragma once
#ifndef NOAF_LOG_H
#define NOAF_LOG_H

#include <functional>
#include <sstream>
#include <string>

namespace noaf {

	constexpr auto TIME_FMT = "%H:%M:%S";

	class log_stream;
	typedef std::function<void(log_stream&)> log_modifier;
	class log_stream {
		std::stringstream buffer;
		public:
		bool	enabled;	// enable output
		bool	pure;		// pure output (not timestamp/log level)
		bool	sendreset;	// reset verboity level to default value after flushing the stream
		int	verbosity;	// verbosity threshold
		int	offset;		// log offset

		int	level;		// current message verbosity level

		log_stream();

		std::function<void(const std::string& message)> flusher;

		void flush();

		template<typename T> friend log_stream& operator<<(log_stream& stream, const T& value) {
			stream.buffer << value;
			return stream;
		}

		friend log_stream& operator<<(log_stream& stream, const log_modifier& modifier) {
			modifier(stream);
			return stream;
		}
	};

	extern log_stream log;
	
	// log stream modifiers
	extern log_modifier lend;
	extern log_modifier ll;
	extern log_modifier lr;
	log_modifier llev(const int& new_level);
	log_modifier lver(const int& new_verbosity);

	// default noaf verbosities
	constexpr int VMES = 0;	// a standart message
	constexpr int VIMP = 1;	// an important message
	constexpr int VWAR = 2;	// a warning message
	constexpr int VERR = 3;	// an error message
}

#endif
