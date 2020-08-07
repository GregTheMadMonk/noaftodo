#ifndef NOAFTODO_H
#define NOAFTODO_H

#include <codecvt>
#include <locale>
#include <string>
#include <vector>

#include "noaftodo_list.h"

// program title and version
constexpr char TITLE[] = "NOAFtodo";

constexpr int LIST_V = 1;
constexpr int CONF_V = 4;
constexpr int MINOR_V = 3;
#ifndef V_SUFFIX
#define VERSION to_string(LIST_V) + "." + to_string(CONF_V) + "." + to_string(MINOR_V)
#else
#define XMSTR(s) MSTR(s)
#define MSTR(s) #s
#define VERSION to_string(LIST_V) + "." + to_string(CONF_V) + "." + to_string(MINOR_V) + " " + XMSTR(V_SUFFIX)
#endif

// program modes
constexpr int PM_DEFAULT = 0;
constexpr int PM_HELP = 1;
constexpr int PM_DAEMON = 2;
constexpr int PM_INTERP = 3;

// log prefixes
constexpr char LP_DEFAULT = 'i';
constexpr char LP_IMPORTANT = 'I';
constexpr char LP_ERROR = '!';

// runtime errors
constexpr int ERR_CONF_V = 0b1;
constexpr int ERR_LIST_V = 0b10;

// "multistr" class
class multistr_c {
	/*
	 * data:
	 * 	a b c d
	 * 	e f g h
	 * 	i j
	 * 	k
	 * 	l
	 *
	 * shifts:
	 * 	3 1 0 1
	 *
	 * offset = 1:
	 * 	1 1 1 1
	 *
	 * resulting shifts:
	 * 	4 2 1 2 <=> 4 2 1 0 as the last set consists of only two charaters
	 *
	 * return value:
	 * 	l j g d
	 */

	std::vector<std::vector<std::wstring>> 	data;
	std::vector<int>		shifts;

	int offset;

	void init(const std::wstring& istr, const int& elen, int len);
public:
	multistr_c(const std::string& str, const int& elen = 1, const int& len = -1);
	multistr_c(const std::wstring& str, const int& elen = 1, const int& len = -1);

	void drop();	// shifts = 0
	void reset();	// shifts = 0 & offset = 0

	int pos(const int& i);

	std::wstring at(const int& i);
	std::wstring get(const int& i = 0); // default argument = 0 for old char variables
	std::string s_get(const int& i = 0);
	std::vector<std::wstring>& v_at(const int& i);
	std::wstring str();

	void shift(const int& steps = 1);
	void shift_at(const int& index, const int& steps = 1);

	int length();

	void append(const std::vector<std::wstring>& app);
};

// special characters
const std::vector<std::string> SPECIAL =
{
	"\\",
	"\"",
	";",
	" "
};

// string <-> wstring converter
extern std::wstring_convert<std::codecvt_utf8<wchar_t>, wchar_t> w_converter;

// program mode
extern int run_mode;

// error flag mask
extern int errors;

// params
extern bool allow_root;

// output options
extern bool enable_log;
extern bool verbose;
extern bool pure;
extern int log_offset;

void print_help();

void noaftodo_exit(const int& val = 0);

void log(const std::string& message, const char& prefix = LP_DEFAULT, const int& sleep_sec = 0);

void select_entry(li::entry* const list_entry);

std::string format_str(std::string str, li::entry* const list_entry, const bool& renotify = false);

std::string replace_special(std::string str);

#endif
