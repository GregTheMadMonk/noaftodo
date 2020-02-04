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

	int year = 0;
	int month = 0;
	int day = 0;
	int hour = 0;
	int minute = 0;

	int start = 0;
	if (t_str.at(0) == 'a')
	{
		const time_t t = system_clock::to_time_t(system_clock::now());
		ti = *localtime(&t);
		ti.tm_mon += 1;
		ti.tm_year += 1900;

		start = 1;
	}

	for (int i = start; i < t_str.length(); i++)
	{
		const char c = t_str.at(i);

		if (isdigit(c)) minute = minute * 10 + (c - '0');
		else 
		{
			switch (c)
			{
				case 'h':
					hour = minute;
					break;
				case 'd':
					day = minute;
					break;
				case 'm':
					month = minute;
					break;
				case 'y':
					year = minute;
					break;
			}

			minute = 0;
		}
	}

	ti.tm_year += year;
	ti.tm_mon += month;
	ti.tm_mday += day;
	ti.tm_hour += hour;
	ti.tm_min += minute;
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
