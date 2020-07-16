#ifndef NOAFTODO_CUI_H
#define NOAFTODO_CUI_H

#include <functional>
#include <map>
#include <stack>
#include <string>
#include <vector>

#ifdef __sun
#include <ncurses/curses.h>
# else
#include <curses.h>
#endif

#include "noaftodo.h"
#include "noaftodo_list.h"

struct cui_bind_s {
	wchar_t key;
	std::string command;
	int mode;
	bool autoexec;
};

struct cui_col_s {
	std::string title;

	std::function<int(const int& w, const int& free, const int& col_n)> width;
	std::function<std::string(const noaftodo_entry& entry, const int& id)> contents;
};

struct cui_lview_col_s {
	std::string title;

	std::function<int(const int& w, const int& free, const int& col_n)> width;
	std::function<std::string(const int& list_id)> contents;
};

// columns
extern std::map<char, cui_lview_col_s> cui_lview_columns;
extern std::map<char, cui_col_s> cui_columns;

// status fields
extern std::map<char, std::function<std::string()>> cui_status_fields;

// color pair indexes
constexpr int CUI_CP_TITLE = 1;
constexpr int CUI_CP_GREEN_ENTRY = 2;
constexpr int CUI_CP_YELLOW_ENTRY = 3;
constexpr int CUI_CP_RED_ENTRY = 4;
constexpr int CUI_CP_STATUS = 5;

// modes
constexpr int CUI_MODE_EXIT = 0;
constexpr int CUI_MODE_ALL = 0b11111111;
constexpr int CUI_MODE_NORMAL = 0b1;		// 1
constexpr int CUI_MODE_LISTVIEW = 0b10000;	// 16
constexpr int CUI_MODE_DETAILS = 0b1000;	// 8
constexpr int CUI_MODE_COMMAND = 0b10;		// 2
constexpr int CUI_MODE_HELP = 0b100;		// 4

// filters
// normal mode
constexpr int CUI_FILTER_UNCAT = 0b1; 		// 1 - uncategorized
constexpr int CUI_FILTER_COMPLETE = 0b10; 	// 2 - complete
constexpr int CUI_FILTER_COMING = 0b100; 	// 4 - upcoming
constexpr int CUI_FILTER_FAILED = 0b1000; 	// 8 - failed
constexpr int CUI_FILTER_NODUE = 0b10000; 	// 16 - nodue
constexpr int CUI_FILTER_EMPTY = 0b100000;	// 32 - empty lists

constexpr int CUI_TAG_ALL = -1;

// current mode
extern int cui_mode;
extern std::stack<int> cui_prev_modes;

extern int cui_halfdelay_time;

// filter values
extern int cui_filter;
extern int cui_tag_filter;
extern std::string cui_normal_regex_filter;
extern std::string cui_listview_regex_filter;

extern std::string cui_contexec_regex_filter;

// colors / appearance
extern bool cui_status_standout;

extern bool cui_shift_multivars;

extern int cui_color_bg;
extern int cui_color_title;
extern int cui_color_status;
extern int cui_color_complete;
extern int cui_color_coming;
extern int cui_color_failed;

// status fields
extern std::string cui_normal_status_fields;
extern std::string cui_listview_status_fields;

// characters
extern multistr_c cui_separators;
extern multistr_c cui_box_strong;
extern multistr_c cui_box_light;

extern int cui_row_separator_offset;

constexpr int CHAR_ROW_SEP = 0;
constexpr int CHAR_STA_SEP = 1;
constexpr int CHAR_DET_SEP = 2;

constexpr int CHAR_VLINE = 0;
constexpr int CHAR_HLINE = 1;
constexpr int CHAR_CORN1 = 2;
constexpr int CHAR_CORN2 = 3;
constexpr int CHAR_CORN3 = 4;
constexpr int CHAR_CORN4 = 5;

// mode columns
extern std::string cui_normal_all_cols;
extern std::string cui_normal_cols;
extern std::string cui_details_cols;
extern std::string cui_listview_cols;

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
extern std::vector<std::wstring> cui_command_history;
extern std::wstring cui_command;
extern std::wstring cui_command_t;
extern int cui_command_cursor;
extern int cui_command_index;

// is cui started
extern bool cui_active;

void cui_init();
void cui_construct();
void cui_destroy();

void cui_run();

void cui_set_mode(const int& mode);

void cui_bind(const cui_bind_s& bind);
void cui_bind(const wchar_t& key, const std::string& command, const int& mode, const bool& autoexec);

bool cui_is_visible(const int& entryID);
bool cui_l_is_visible(const int& list_id);

// mode-specific painters and input handlers
void cui_normal_paint();
void cui_normal_input(const wchar_t& key);

void cui_listview_paint();
void cui_listview_input(const wchar_t& key);

void cui_details_paint();
void cui_details_input(const wchar_t& key);

void cui_command_paint();
void cui_command_input(const wchar_t& key);

void cui_help_paint();
void cui_help_input(const wchar_t& key);

void cui_safemode_box();

std::string cui_prompt(const std::string& message = ">");

void cui_filter_history();

wchar_t cui_key_from_str(const std::string& str);

// drawer functions
void cui_draw_status(const std::string& fields);

void cui_clear_box(const int& x, const int& y, const int& w, const int& h);
void cui_draw_border(const int& x, const int& y, const int& w, const int& h, multistr_c& chars);
void cui_text_box(const int& x, const int& y, const int& w, const int& h, const std::string& str, const bool& show_md = true);
#endif
