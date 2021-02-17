#ifndef NOAFTODO_CUI_H
#define NOAFTODO_CUI_H

#include <functional>
#include <map>
#include <stack>
#include <string>
#include <variant>
#include <vector>

#ifdef __sun
#include <ncurses/curses.h>
# else
#include <curses.h>
#endif

#include <noaftodo.h>
#include <list.h>
#include <core/time.h>

// mode declaration macro
#define NOAFTODO_START_MODE(name, init_name, painter_name, input_handler_name) namespace cui::modes::mode_##name {\
	void init_name(); \
	void painter_name();\
	void input_handler_name(const keystroke_s& key, const bool& bind_fired);\
	std::nullptr_t creator = ([] () { init_mode(#name, {\
			init_name, \
			painter_name,\
			input_handler_name,\
			});\
		return nullptr;\
	})();

#define NOAFTODO_END_MODE }


namespace cui {

struct keystroke_s {
	wchar_t key;
	bool mod_alt;

	bool operator==(const keystroke_s& op) { return (this->key == op.key) && (this->mod_alt == op.mod_alt); }
	bool operator==(const keystroke_s& op) const { return (this->key == op.key) && (this->mod_alt == op.mod_alt); }
};

struct mode_s {
	std::function<void()> init_f;
	std::function<void()> paint_f;
	std::function<void(const keystroke_s& key, const bool& bind_fired)> input_f;

	inline void paint() { paint_f(); }
	inline void input(const keystroke_s& key, const bool& bind_fired = false) { input_f(key, bind_fired); }
};

struct bind_s {
	keystroke_s key;
	std::string command;
	int mode;
	bool autoexec;
};

namespace vargs {
	namespace cols {
		typedef struct { const li::entry& e; const int& id; } normal;
		typedef struct { const int& l_id; } lview;
		typedef std::variant<normal, lview> varg;
	}
}

struct col_s {
	std::string title;

	std::function<int(const int& w, const int& free, const int& col_n)> width;
	std::function<std::string(const vargs::cols::varg& args)> contents;
};

namespace modes {
	// modes
	std::map<std::string, mode_s>& modes();

	void init_mode(const std::string& alias, const mode_s& mode);
	mode_s mode(const std::string& alias);
}

// columns
extern std::map<char, col_s> lview_columns;
extern std::map<char, col_s> columns;

// status fields
extern std::map<char, std::function<std::string()>> status_fields;

// modes
constexpr int MODE_EXIT = 0;
constexpr int MODE_ALL = 0b11111111;
constexpr int MODE_NORMAL = 0b1;		// 1
constexpr int MODE_LISTVIEW = 0b10000;	// 16
constexpr int MODE_TIMELINE = 0b100000; // 32
constexpr int MODE_DETAILS = 0b1000;	// 8
constexpr int MODE_COMMAND = 0b10;		// 2
constexpr int MODE_HELP = 0b100;		// 4

// filters
// normal mode
constexpr int FILTER_UNCAT = 0b1; 		// 1 - uncategorized
constexpr int FILTER_COMPLETE = 0b10; 	// 2 - complete
constexpr int FILTER_COMING = 0b100; 	// 4 - upcoming
constexpr int FILTER_FAILED = 0b1000; 	// 8 - failed
constexpr int FILTER_NODUE = 0b10000; 	// 16 - nodue
constexpr int FILTER_EMPTY = 0b100000;	// 32 - empty lists

constexpr int TAG_ALL = -1;

// last screen updated time
extern time_s last_scr_upd;

// current mode
extern int mode;
extern std::stack<int> prev_modes;

extern int halfdelay_time;

// filter values
extern int filter;
extern int tag_filter;
extern std::string normal_regex_filter;
extern std::string listview_regex_filter;

extern std::string contexec_regex_filter;

// colors / appearance
extern bool shift_multivars;

extern int color_title;
extern int color_status;
extern int color_complete;
extern int color_coming;
extern int color_due;
extern int color_failed;

// status fields
extern std::string normal_status_fields;
extern std::string listview_status_fields;

// characters
extern multistr_c separators;
extern multistr_c box_strong;
extern multistr_c box_light;

extern int row_separator_offset;

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
extern std::string normal_all_cols;
extern std::string normal_cols;
extern std::string details_cols;
extern std::string listview_cols;

// binds
extern std::vector<bind_s> binds;

// markdown style switches
const std::vector<std::pair<int, std::wstring>> md_switches = {
	{ A_BOLD,	L"**"	},
	{ A_ITALIC,	L"*"	},
	{ A_UNDERLINE,	L"~~"	}
};

// symbols that break words
const std::wstring wordbreakers = L"!@#$%^&*()-+=`~\"';:,.<>/?\\|[]{} ";

// interface size
extern int w, h;

// status
extern std::string status;

// normal mode data
extern int s_line;
extern int delta;
extern int numbuffer;

// command mode data
extern std::vector<std::wstring> command_history;
extern std::wstring command;
extern std::wstring command_t;
extern int command_cursor;
extern int command_index;

// is cui started
extern bool active;

void init();
void construct();
void destroy();

void run();

void set_mode(const int& new_mode);

void bind(const bind_s& bind);
void bind(const keystroke_s& key, const std::string& command, const int& mode, const bool& autoexec);

bool fire_bind(const keystroke_s& key);

bool is_visible(const int& entryID);
bool l_is_visible(const int& list_id);

void safemode_box();

std::string prompt(const std::string& message = ">");

void filter_history();

keystroke_s key_from_str(std::string str);
int pair_from_str(const std::string& str);

typedef struct { attr_t attrs; int pair; } attrs;
inline void attrset_ext(const attr_t& a, const int& p = 0) { attr_set(a, p, NULL); }
inline void attrset_ext(const attrs& a) { attr_set(a.attrs, a.pair, NULL); }

inline void attron_ext(const attr_t& a1) { int a; int p; attr_get(&a, &p, NULL); attr_set(a1 | a, p, NULL); }
inline void attron_ext(const attr_t& a1, const int& p1) { int a; int p; attr_get(&a, &p, NULL); attr_set(a1 | a, p1, NULL); }

// drawer functions
int draw_table(const int& x, const int& y,
		const int& w, const int& h,
		const std::function<vargs::cols::varg(const int& item)>& colarg_f,
		const std::function<bool(const int& item)>& vis_f,
		const std::function<bool(const int& item)>& cont_f,
		const std::function<attrs(const int& item)>& attrs_f,
		const int& start, int& sel,
		const std::string& down_cmd,
		const std::string& cols, const std::map<char, col_s>& colmap);

void draw_status(const std::string& fields);

void clear_box(const int& x, const int& y,
		const int& w, const int& h);

void draw_border(const int& x, const int& y,
		const int& w, const int& h,
		multistr_c& chars);

typedef struct { int w; int h; } box_dim;

box_dim text_box(const int& x, const int& y,
		const int& w, const int& h,
		const std::string& str,
		const bool& show_md = true,
		const int& line_offset = 0,
		const bool& nullout = false);

}
#endif
