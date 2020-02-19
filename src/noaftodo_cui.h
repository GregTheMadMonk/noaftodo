#ifndef NOAFTODO_CUI_H
#define NOAFTODO_CUI_H

#include <string>
#include <vector>

struct cui_bind_s
{
	wchar_t key;
	std::string command;
	int mode;
	bool autoexec;
};

// color pair indexes
constexpr int CUI_CP_TITLE = 1;
constexpr int CUI_CP_GREEN_ENTRY = 2;
constexpr int CUI_CP_YELLOW_ENTRY = 3;
constexpr int CUI_CP_RED_ENTRY = 4;

// modes
constexpr int CUI_MODE_EXIT = 0;
constexpr int CUI_MODE_ALL = 0b1111;
constexpr int CUI_MODE_NORMAL = 0b1;
constexpr int CUI_MODE_COMMAND = 0b10;
constexpr int CUI_MODE_HELP = 0b100;

// filters
constexpr int CUI_FILTER_UNCAT = 0b1; // uncategorized
constexpr int CUI_FILTER_COMPLETE = 0b10; // complete
constexpr int CUI_FILTER_COMING = 0b100; // upcoming
constexpr int CUI_FILTER_FAILED = 0b1000; // failed

extern int cui_filter;

// current mode
extern int cui_mode;

// binds
extern std::vector<cui_bind_s> binds;

// interface size
extern int cui_w, cui_h;

// status
extern std::string cui_status;

// normal mode data
extern int cui_s_line;
extern int cui_delta;
extern int cui_numbuffer;

// command mode data
extern std::wstring cui_command;
extern int cui_command_cursor;

void cui_init();
void cui_destroy();

void cui_run();

void cui_set_mode(const int& mode);

void cui_bind(const cui_bind_s& bind);
void cui_bind(const wchar_t& key, const std::string& command, const int& mode, const bool& autoexec);

void cui_exec(const std::string& command);

bool cui_is_visible(const int& entryID);

// mode-specific painters and input handlers
void cui_normal_paint();
void cui_normal_input(const wchar_t& key);

void cui_command_paint();
void cui_command_input(const wchar_t& key);

void cui_help_paint();
void cui_help_input(const wchar_t& key);
#endif
