#pragma once
#ifndef NOAF_TIME_H
#define NOAF_TIME_H

#include <functional>
#include <string>

namespace noaf {

	struct time_s {
		time_t time;

		time_s();				// default constructor: initialize with current time
		time_s(const long& t_long); 		// constructor from a long. OBSOLETE in 2.x.x
		time_s(const std::string& t_str);	// constructor from a cmd string

		// return localtime tm struct
		tm to_tm() const;
		tm to_tm();

		// format time
		// main format method
		std::string fmt(std::string format) const;
		std::string fmt(std::string format);

		// comparsement operators
		bool operator<(const time_s& rval) const;
		bool operator<(const time_s& rval);
		bool operator>(const time_s& rval) const;
		bool operator>(const time_s& rval);
		bool operator<=(const time_s& rval) const;
		bool operator<=(const time_s& rval);
		bool operator>=(const time_s& rval) const;
		bool operator>=(const time_s& rval);
		bool operator==(const time_s& rval) const;
		bool operator==(const time_s& rval);
		bool operator!=(const time_s& rval) const;
		bool operator!=(const time_s& rval);

		// difference between two time marks
		struct time_diff_s { tm diff; bool sign; /* sign = true -> positive */ };
		time_diff_s operator-(const time_s& rval) const;
		time_diff_s operator-(const time_s& rval);
	};

}

#endif
