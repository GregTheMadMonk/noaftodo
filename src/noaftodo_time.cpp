#include "noaftodo_time.h"

using namespace std;
using namespace chrono;

long ti_to_long(const tm& t_tm)
{
	return t_tm.tm_min + t_tm.tm_hour * 1e2 + t_tm.tm_mday * 1e4 + t_tm.tm_mon * 1e6 + t_tm.tm_year * 1e8;
}

long ti_to_long(const string& t_str)
{
	return ti_to_long(ti_to_tm(t_str));
}

tm ti_to_tm(const string& t_str)
{
	tm ti = {0};
	const time_t t = system_clock::to_time_t(system_clock::now());
	tm l_ti = *localtime(&t);
	l_ti.tm_mon += 1;
	l_ti.tm_year += 1900;

	int year = 0;
	int month = 0;
	int day = 0;
	int hour = 0;
	int minute = 0;

	bool c_year = false;
	bool c_mon = false;
	bool c_day = false;
	bool c_hour = false;
	bool c_min = false;

	bool absolute = true;

	for (int i = 0; i <= t_str.length(); i++)
	{
		const char c = (i == t_str.length()) ? 'a' : t_str.at(i);

		if (isdigit(c)) { minute = minute * 10 + (c - '0'); c_min = true; }
		else 
		{
			switch (c)
			{
				case 'a':
					if (absolute) 
					{
						if (!c_year) ti.tm_year = l_ti.tm_year;
						else ti.tm_year = year;

						if (!c_mon) ti.tm_mon = l_ti.tm_mon;
						else ti.tm_mon = month;

						if (!c_day) ti.tm_mday = l_ti.tm_mday;
						else ti.tm_mday = day;

						if (!c_hour) ti.tm_hour = l_ti.tm_hour;
						else ti.tm_hour = hour;

						if (!c_min) ti.tm_min = l_ti.tm_min;
						else ti.tm_min = minute;
					} else {
						ti.tm_year += year;
						ti.tm_mon += month;
						ti.tm_mday += day;
						ti.tm_hour += hour;
						ti.tm_min += minute;
					}

					hour = 0;
					day = 0;
					month = 0;
					year = 0;
					
					c_hour = false;
					c_day = false;
					c_mon = false;
					c_year = false;

					absolute = !absolute;
					break;
				case 'h':
					hour = minute;
					c_hour = true;

					break;
				case 'd':
					day = minute;
					c_day = true;

					break;
				case 'm':
					month = minute;
					c_mon = true;

					break;
				case 'y':
					year = minute;
					c_year = true;

					break;
			}

			c_min = false;
			minute = 0;
		}
	}

	ti.tm_sec = 0;

	if (ti.tm_min >= 60) { ti.tm_hour += ti.tm_min / 60; ti.tm_min = ti.tm_min % 60; }
	if (ti.tm_hour >= 24) { ti.tm_mday += ti.tm_hour / 24; ti.tm_hour = ti.tm_hour % 24; }
	int days_in_month = (ti.tm_mon == 2) ? ((ti.tm_year % 4 == 0) ? 29 : 28) : ((ti.tm_mon <= 7) ? (30 + (ti.tm_mon % 2)) : (31 - (ti.tm_mon % 2)));
	while (ti.tm_mday > days_in_month)
	{
		ti.tm_mday -= days_in_month;
		ti.tm_mon++;

		days_in_month = (ti.tm_mon == 2) ? ((ti.tm_year % 4 == 0) ? 29 : 28) : ((ti.tm_mon <= 7) ? (30 + (ti.tm_mon % 2)) : (31 - (ti.tm_mon % 2)));
	}

	if (ti.tm_mon > 12) { ti.tm_year += ti.tm_mon / 12; ti.tm_mon = ti.tm_mon % 12; }

	return ti;
}

tm ti_to_tm(const long& t_long)
{
	tm ret = { 0 };

	ret.tm_sec = 0;
	ret.tm_min = t_long % (long)1e2;
	ret.tm_hour = t_long % (long)1e4 / 1e2;
	ret.tm_mday = t_long % (long)1e6 / 1e4;
	ret.tm_mon = t_long % (long)1e8 / 1e6;
	ret.tm_year = t_long / (long)1e8;

	return ret;
}

string ti_f_str(const tm& t_tm)
{
	const auto t_str = [](const int& integer)
	{
		return ((integer < 10) ? "0" : "") + to_string(integer);
	};

	return to_string(t_tm.tm_year) + "/" + t_str(t_tm.tm_mon) + "/" + t_str(t_tm.tm_mday) + " " + t_str(t_tm.tm_hour) + ":" + t_str(t_tm.tm_min);
}

string ti_f_str(const long& t_long)
{
	return ti_f_str(ti_to_tm(t_long));
}
