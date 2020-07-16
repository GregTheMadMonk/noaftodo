#include "noaftodo_cui.h"

#include <regex>

#include "noaftodo_cmd.h"
#include "noaftodo_config.h"
#include "noaftodo_cvar.h"
#include "noaftodo_daemon.h"
#include "noaftodo_time.h"

using namespace std;

map<char, cui_lview_col_s> cui_lview_columns;
map<char, cui_col_s> cui_columns;
map<char, function<string()>> cui_status_fields;

int cui_halfdelay_time = 2;

int cui_filter;
int cui_tag_filter;
string cui_normal_regex_filter;
string cui_listview_regex_filter;

string cui_contexec_regex_filter;

bool cui_status_standout;

bool cui_shift_multivars = false;

int cui_color_bg;
int cui_color_title;
int cui_color_status;
int cui_color_complete;
int cui_color_coming;
int cui_color_failed;

int cui_row_separator_offset = 0;
multistr_c cui_separators("|||");	// row, status, details separators
multistr_c cui_box_strong("|-++++");	// vertical and horisontal line;
					// 1st, 2nd, 3rd, 4th corner;
					// more later
multistr_c cui_box_light("|-++++");

string cui_normal_all_cols;
string cui_normal_cols;
string cui_details_cols;
string cui_listview_cols;

string cui_normal_status_fields;
string cui_listview_status_fields;

int cui_mode;
stack<int> cui_prev_modes;

vector<cui_bind_s> binds;

int cui_w, cui_h;

string cui_status = "Welcome to " + string(TITLE) + "!";

int cui_s_line;
int cui_delta;
int cui_numbuffer = -1;

vector<wstring> cui_command_history;
wstring cui_command;
wstring cui_command_t;
int cui_command_cursor = 0;
int cui_command_index = 0;

bool cui_active = false;

extern string DOC;

void cui_init() {
	log("Initializing console UI...");

	// initialize columns
	cui_columns['t'] = 
	{ 
		"Title", 
		[](const int& w, const int& free, const int& cols) {
			return free / 4;
		},
		[](const noaftodo_entry& e, const int& id) 
		{ 
			return e.title; 
		} 
	};
	cui_columns['f'] = 
	{ 
		"Flags", 
		[](const int& w, const int& free, const int& cols) {
			return 5;
		},
		[](const noaftodo_entry& e, const int& id) 
		{ 
			string ret = "";
			if (e.completed) ret += "V";	// a completed entry
			if (e.is_failed()) ret += "F";	// a failed entry
			if (e.is_coming()) ret += "C";	// an upcoming entry
			if (e.get_meta("nodue") == "true") ret += "N";	// a "nodue" entry
			return ret; 
		} 
	};
	cui_columns['l'] = 
	{ 
		"List", 
		[](const int& w, const int& free, const int& cols) {
			return free / 10;
		},
		[](const noaftodo_entry& e, const int& id) 
		{ 
			if (e.tag < t_tags.size())
			       if (t_tags.at(e.tag) != to_string(e.tag))
				       return to_string(e.tag) + ": " + t_tags.at(e.tag);

			return to_string(e.tag);
		} 
	};
	cui_columns['d'] = 
	{ 
		"Due", 
		[](const int& w, const int& free, const int& cols) {
			return 16;
		},
		[](const noaftodo_entry& e, const int& id) 
		{ 
			if (e.get_meta("nodue") == "true") return string("----------------");
			else return ti_f_str(e.due); 
		} 
	};
	cui_columns['D'] = 
	{ 
		"Description", 
		[](const int& w, const int& free, const int& cols) {
			return free;
		},
		[](const noaftodo_entry& e, const int& id) 
		{ 
			return e.description; 
		} 
	};
	cui_columns['i'] = 
	{ 
		"ID", 
		[](const int& w, const int& free, const int& cols) {
			return 3;
		},
		[](const noaftodo_entry& e, const int& id) 
		{ 
			return to_string(id); 
		} 
	};
	
	// initialize listview columns
	cui_lview_columns['i'] = 
	{ 
		"ID", 
		[](const int& w, const int& free, const int& cols) {
			return 3;
		},
		[](const int& list_id) 
		{ 
			if (list_id == -1) return string(" ");
			return to_string(list_id);
		} 
	};

	cui_lview_columns['f'] =
	{
		"Flags",
		[] (const int& w, const int& free, const int& cols) {
			return 5;
		},
		[] (const int& list_id) {
			return string(li_tag_completed(list_id) ? "V" : "") +
				string(li_tag_coming(list_id) ? "C" : "") +
				string(li_tag_failed(list_id) ? "F" : "");
		}
	};
	
	cui_lview_columns['t'] = 
	{ 
		"Title", 
		[](const int& w, const int& free, const int& cols) {
			return free / 4;
		},
		[](const int& list_id) 
		{ 
			if (list_id == CUI_TAG_ALL) return string("All lists");
			return t_tags.at(list_id);
		} 
	};

	cui_lview_columns['e'] = {
		"Entries",
		[](const int& w, const int& free, const int& cols) {
			return 7;
		},
		[](const int& list_id) {
			if (list_id == CUI_TAG_ALL) return to_string(t_list.size());

			int ret = 0;
			for (auto e : t_list) if (e.tag == list_id) ret++;

			return to_string(ret);
		}
	};	
	
	cui_lview_columns['p'] = 
	{ 
		"%", 
		[](const int& w, const int& free, const int& cols) {
			return 4;
		},
		[](const int& list_id) 
		{ 
			int ret = 0;
			int tot = 0;

			for (auto e : t_list)
				if ((e.tag == list_id) || (CUI_TAG_ALL == list_id)) {
					if (e.completed) ret++;
					tot++;
				}

			if (tot == 0) return string("0%");
			return to_string(100 * ret / tot) + "%";
		} 
	};

	// initialize status fields
	cui_status_fields['s'] = [] () {
		return cui_status;
	};

	cui_status_fields['l'] = [] () {
		if (cui_tag_filter == CUI_TAG_ALL) return string("All lists");

		return "List " + to_string(cui_tag_filter) + (((cui_tag_filter < t_tags.size()) && (t_tags.at(cui_tag_filter) != to_string(cui_tag_filter))) ? (": " + t_tags.at(cui_tag_filter)) : "");
	};

	cui_status_fields['m'] = [] () {
		switch (cui_mode) {
			case CUI_MODE_NORMAL:
				return string("NORMAL");
				break;
			case CUI_MODE_LISTVIEW:
				return string("LISTVIEW");
				break;
			case CUI_MODE_DETAILS:
				return string("DETAILS");
				break;
			case CUI_MODE_HELP:
				return string("HELP");
				break;
			default:
				return string("");
		}
	};

	cui_status_fields['f'] = [] () {
		return string((cui_filter & CUI_FILTER_UNCAT) ? "U" : "_") +
			string((cui_filter & CUI_FILTER_COMPLETE) ? "V" : "_") +
			string((cui_filter & CUI_FILTER_COMING) ? "C" : "_") + 
			string((cui_filter & CUI_FILTER_FAILED) ? "F" : "_") +
			string((cui_filter & CUI_FILTER_NODUE) ? "N" : "_") + 
			string((cui_filter & CUI_FILTER_EMPTY) ? "0" : "_") +
			((cui_normal_regex_filter == "") ? "" : (" [e " + cui_normal_regex_filter + "]")) +
			((cui_listview_regex_filter == "") ? "" : (" [l " + cui_listview_regex_filter + "]"));
	};

	cui_status_fields['i'] = [] () {
		bool has_visible = false;

		for (int i = 0; i < t_list.size(); i++) has_visible |= cui_is_visible(i);

		if (!has_visible) return string("");

		return to_string(cui_s_line) + "/" + to_string(t_list.size() - 1);
	};

	cui_status_fields['p'] = [] () {
		int total = 0;
		int comp = 0;

		for (int i = 0; i < t_list.size(); i++)
			if ((cui_tag_filter == CUI_TAG_ALL) || (cui_tag_filter == t_list.at(i).tag)) {
				total++;
				if (t_list.at(i).completed) comp++;
			}

		if (total == 0) return string("");

		return to_string((int)(100.0 * comp / total)) + "%";
	};

	cui_status_fields['P'] = [] () {
		int total = 0;
		int comp = 0;

		for (int i = 0; i < t_list.size(); i++)
			if ((cui_tag_filter == CUI_TAG_ALL) || (cui_tag_filter == t_list.at(i).tag)) {
				total++;
				if (t_list.at(i).completed) comp++;
			}

		if (total == 0) return string("");

		return to_string(comp) + "/" + to_string(total) + " COMP";
	};
	
	// construct UI
	cui_command_history.push_back(w_converter.from_bytes(""));

	cui_construct();
}

void cui_construct() {
	initscr();
	start_color();
	use_default_colors();

	init_pair(CUI_CP_TITLE, cui_color_title, cui_color_bg);
	init_pair(CUI_CP_GREEN_ENTRY, cui_color_complete, cui_color_bg);
	init_pair(CUI_CP_YELLOW_ENTRY, cui_color_coming, cui_color_bg);
	init_pair(CUI_CP_RED_ENTRY, cui_color_failed, cui_color_bg);
	init_pair(CUI_CP_STATUS, cui_color_status, cui_color_bg);

	if (cui_halfdelay_time == 0) cbreak();
	else halfdelay(cui_halfdelay_time);
	set_escdelay(0);
	curs_set(0);
	noecho();
	keypad(stdscr, true);

	cui_w = getmaxx(stdscr);
	cui_h = getmaxy(stdscr);

	cui_active = true;
}

void cui_destroy() {
	endwin();
	cui_active = false;
}

void cui_run() {
	cui_init();
	cui_set_mode(CUI_MODE_NORMAL);

	for (wint_t c = -1; ; (get_wch(&c) != ERR) ? : (c = 0)) {
		if (li_has_changed()) li_load(false); // load only list contents, not the workspace
		else if ((c == 0) && (!cui_shift_multivars)) continue;

		if (c > 0) {
			const int old_numbuffer = cui_numbuffer;
			cui_status = "";
			bool bind_fired = false;
			for (const auto& bind : binds)
				if ((bind.mode & cui_mode) && (bind.key == c)) {
					if (bind.autoexec) cmd_exec(bind.command);
					else  {
						if ((cui_s_line >= 0) && (cui_s_line < t_list.size()))
							cui_command = w_converter.from_bytes(format_str(bind.command, &t_list.at(cui_s_line)));
						else
							cui_command = w_converter.from_bytes(bind.command);
						cui_set_mode(CUI_MODE_COMMAND);
					}

					bind_fired = true;
					break;
				}

			if (!bind_fired) switch (cui_mode) {
				case CUI_MODE_NORMAL:
					cui_normal_input(c);
					break;
				case CUI_MODE_LISTVIEW:
					cui_listview_input(c);
					break;
				case CUI_MODE_DETAILS:
					cui_details_input(c);
					break;
				case CUI_MODE_COMMAND:
					cui_command_input(c);
					break;
				case CUI_MODE_HELP:
					cui_help_input(c);
			}

			if (old_numbuffer == cui_numbuffer) cui_numbuffer = -1;
		}

		cui_w = getmaxx(stdscr);
		cui_h = getmaxy(stdscr);

		if (cui_shift_multivars) {
			cui_box_strong.shift();
			cui_box_light.shift();
			cui_separators.shift();
		}

		cui_box_strong.drop();
		cui_box_light.drop();
		cui_separators.drop();

		switch (cui_mode) {
			case CUI_MODE_NORMAL:
				cui_normal_paint();
				break;
			case CUI_MODE_LISTVIEW:
				cui_listview_paint();
				break;
			case CUI_MODE_DETAILS:
				cui_details_paint();
				break;
			case CUI_MODE_COMMAND:
				cui_command_paint();
				break;
			case CUI_MODE_HELP:
				cui_help_paint();
				break;
		}

		if (errors != 0) {
			attrset(A_NORMAL);
			int old_cx, old_cy;
			getyx(stdscr, old_cy, old_cx);
			cui_safemode_box();
			move(old_cy, old_cx);
		}

		if (cui_mode == CUI_MODE_EXIT) break;
	}

	if (li_autosave) li_save();

	cui_destroy();

	da_send("D");
}

void cui_set_mode(const int& mode) {
	if (mode == -1) {
		if (cui_prev_modes.size() == 0) return;
		cui_mode = cui_prev_modes.top();
		cui_prev_modes.pop();
	} else {
		if ((cui_mode != CUI_MODE_COMMAND) && (cui_mode != mode)) cui_prev_modes.push(cui_mode);
		cui_mode = mode;
	}

	cui_delta = 0;

	switch (cui_mode) {
		case CUI_MODE_NORMAL:
			if (cvar("tag_filter_copy") != "") {
				cui_tag_filter = cvar("tag_filter_copy");
				cvar_erase("tag_filter_copy");
			}
			curs_set(0);
			break;
		case CUI_MODE_LISTVIEW:
			curs_set(0);
			cvar("tag_filter_copy") = cui_tag_filter;
			break;
		case CUI_MODE_DETAILS:
			if ((cui_s_line < 0) || (cui_s_line >= t_list.size())) {
				cui_set_mode(-1);
				return;
			}
			cui_delta = 0;
			curs_set(0);
			break;
		case CUI_MODE_COMMAND:
			curs_set(1);
			cui_command_index = cui_command_history.size();
			cui_command_cursor = cui_command.length();
			break;
		case CUI_MODE_HELP:
			cui_delta = 0;
			curs_set(0);
			break;
	}
}

void cui_bind(const cui_bind_s& bind) {
	binds.push_back(bind);
}

void cui_bind(const wchar_t& key, const string& command, const int& mode, const bool& autoexec) {
	cui_bind({ key, command, mode, autoexec });
}

bool cui_is_visible(const int& entryID) {
	if (t_list.size() == 0) return false;
	if ((entryID < 0) && (entryID >= t_list.size())) return false;

	const auto& entry = t_list.at(entryID);

	bool ret = ((cui_tag_filter == CUI_TAG_ALL) || (cui_tag_filter == entry.tag) || (cui_mode == CUI_MODE_LISTVIEW));

	if (entry.completed) 	ret = ret && (cui_filter & CUI_FILTER_COMPLETE);
	if (entry.is_failed()) 	ret = ret && (cui_filter & CUI_FILTER_FAILED);
	if (entry.is_coming()) 	ret = ret && (cui_filter & CUI_FILTER_COMING);

	if (entry.get_meta("nodue") == "true") ret = ret && (cui_filter & CUI_FILTER_NODUE);

	if (entry.is_uncat())	ret = ret && (cui_filter & CUI_FILTER_UNCAT);

	// fit regex
	if (cui_normal_regex_filter != "") {
		regex rf_regex(cui_normal_regex_filter);

		ret = ret && (regex_search(entry.title, rf_regex) || regex_search(entry.description, rf_regex));
	}

	return ret;
}

bool cui_l_is_visible(const int& list_id) {
	if (list_id == CUI_TAG_ALL) return true;
	if ((list_id < 0) || (list_id >= t_tags.size())) return false;

	bool ret = false;

	if ((cui_filter & CUI_FILTER_EMPTY) == 0)
		for (int i = 0; i < t_list.size(); i++)
			ret |= ((t_list.at(i).tag == list_id) && cui_is_visible(i));
	else ret = true;

	// fit regex
	if (cui_listview_regex_filter != "") {
		regex rf_regex(cui_listview_regex_filter);

		ret = ret && regex_search(t_tags.at(list_id), rf_regex);
	}

	return ret;
}

void cui_listview_paint() {
	// draw table title
	move(0, 0);
	attrset(A_STANDOUT | A_BOLD | COLOR_PAIR(CUI_CP_TITLE));
	for (int i = 0; i < cui_w; i++) addch(' ');

	int x = 0;
	for (int coln = 0; coln < cui_listview_cols.length(); coln++) {
		try {
			const char& col = cui_listview_cols.at(coln);
			if (x >= cui_w) break;
			move(0, x);
			const int w = cui_lview_columns.at(col).width(cui_w, cui_w - x, cui_listview_cols.length());
			addstr(cui_lview_columns.at(col).title.c_str());

			if (coln < cui_listview_cols.length() - 1) if (x + w < cui_w) {
				move(0, x + w);
				addstr((" " + cui_separators.s_get(CHAR_ROW_SEP) + " ").c_str());
			}
			x += w + 3;
			for (int x1 = x; x1 < cui_w; x1++) addch(' ');
		} catch (const out_of_range& e) {}
	}
	attrset(A_NORMAL);

	vector<int> v_list;
	v_list.push_back(-1);
	int cui_v_line = -2;
	for (int l = 0; l < t_tags.size(); l++)
		if (cui_l_is_visible(l))  {
			v_list.push_back(l);
			if (l == cui_tag_filter) cui_v_line = v_list.size() - 1;
		}

	if (v_list.size() != 0)  {
		while (!cui_l_is_visible(cui_tag_filter) && (cui_tag_filter < t_tags.size())) 
			cui_tag_filter++; 
		if (cui_tag_filter == t_tags.size()) cui_tag_filter = CUI_TAG_ALL;
		for (int i = 0; i < v_list.size(); i++) if (v_list.at(i) == cui_tag_filter) cui_v_line = i;
	}

	cui_delta = 0;
	if (cui_v_line - cui_delta >= cui_h - 2) cui_delta = cui_v_line - cui_h + 3;
	if (cui_v_line - cui_delta < 0) cui_delta = cui_v_line;

	int last_string = 1;
	if (v_list.size() == 0) cui_tag_filter = CUI_TAG_ALL;
	else {
		for (int l = 0; l < v_list.size(); l++) {
			cui_separators.drop();
			cui_separators.shift_at(CHAR_ROW_SEP, cui_row_separator_offset * (l + 1));
			if (l - cui_delta >= cui_h - 2) break;
			if (l >= cui_delta)     {
				if (l == cui_v_line) attron(A_STANDOUT);
				if (li_tag_completed(v_list.at(l))) attron(COLOR_PAIR(CUI_CP_GREEN_ENTRY) | A_BOLD);	// list consists of completed entries
				else if (li_tag_failed(v_list.at(l))) attron(COLOR_PAIR(CUI_CP_RED_ENTRY) | A_BOLD);	// list contains a failed entry
				else if (li_tag_coming(v_list.at(l))) attron(COLOR_PAIR(CUI_CP_YELLOW_ENTRY) | A_BOLD);	// list contains an upcoming entry

				x = 0;
				move(l - cui_delta + 1, x);
				for (int i = 0; i < cui_w; i++) addch(' ');

				for (int coln = 0; coln < cui_listview_cols.length(); coln++) {
					try {
						const char& col = cui_listview_cols.at(coln);
						if (x >= cui_w) break;
						move(l - cui_delta + 1, x);
						const int w = cui_lview_columns.at(col).width(cui_w, cui_w - x, cui_listview_cols.length());
						addstr((cui_lview_columns.at(col).contents(v_list.at(l))).c_str());

						if (coln < cui_listview_cols.length() - 1) if (x + w < cui_w) {
							move(l - cui_delta + 1, x + w);
							addstr((" " + cui_separators.s_get(CHAR_ROW_SEP) + " ").c_str());
						}
						x += w + 3;

						for (int x1 = x; x1 < cui_w; x1++) addch(' ');
					} catch (const out_of_range& e) {}
				}

				move(l - cui_delta + 1, cui_w - 1);
				addstr(" ");

				attrset(A_NORMAL);
				last_string = l - cui_delta + 2;
			}
		}
	}

	for (int s = last_string; s < cui_h; s++) { move(s, 0); clrtoeol(); }

	string cui_status_l = "";

	for (const char& c : cui_listview_status_fields) {
		try {
			const string field = (cui_status_fields.at(c))();
			if (field != "") {
				if (cui_status_l != "") cui_status_l = " " + cui_separators.s_get(CHAR_STA_SEP) + " " + cui_status_l;
		       		cui_status_l = field + cui_status_l;
			}
		} catch (const out_of_range& e) {}
	}

	if (cui_status_standout) attron(A_STANDOUT);
	attron(COLOR_PAIR(CUI_CP_STATUS));
	move(cui_h - 1, 0);
	for (int x = 0; x < cui_w; x++) addch(' ');
	move(cui_h - 1, cui_w - 1 - w_converter.from_bytes(cui_status_l).length());
	addstr(cui_status_l.c_str());
	attrset(A_NORMAL);
}

void cui_listview_input(const wchar_t& key) { }

void cui_normal_paint() {
	// draw table title
	move(0, 0);
	attrset(A_STANDOUT | A_BOLD | COLOR_PAIR(CUI_CP_TITLE));
	for (int i = 0; i < cui_w; i++) addch(' ');

	int x = 0;
	const string cols = (cui_tag_filter == CUI_TAG_ALL) ? cui_normal_all_cols : cui_normal_cols;
	for (int coln = 0; coln < cols.length(); coln++) {
		try {
			const char& col = cols.at(coln);
			if (x >= cui_w) break;
			move(0, x);
			const int w = cui_columns.at(col).width(cui_w, cui_w - x, cols.length());
			addstr(cui_columns.at(col).title.c_str());

			if (coln < cols.length() - 1) if (x + w < cui_w) {
				move(0, x + w);
				addstr((" " + cui_separators.s_get(CHAR_ROW_SEP) + " ").c_str());
			}
			x += w + 3;
			for (int x1 = x; x1 < cui_w; x1++) addch(' ');
		} catch (const out_of_range& e) {}
	}
	attrset(A_NORMAL);

	vector<int> v_list;
	int cui_v_line = -1;
	for (int l = 0; l < t_list.size(); l++)
		if (cui_is_visible(l))  {
			v_list.push_back(l);
			if (l == cui_s_line) cui_v_line = v_list.size() - 1;
		}

	if (v_list.size() != 0)  {
		while (!cui_is_visible(cui_s_line)) cmd_exec("math %id% + 1 id"); // sorry, it just includes all precautions I don't want to write again
		for (int i = 0; i < v_list.size(); i++) if (v_list.at(i) == cui_s_line) cui_v_line = i;
	}

	cui_delta = 0;
	if (cui_v_line - cui_delta >= cui_h - 2) cui_delta = cui_v_line - cui_h + 3;
	if (cui_v_line - cui_delta < 0) cui_delta = cui_v_line;

	int last_string = 1;
	if (v_list.size() == 0) cui_s_line = -1;
	else {
		for (int l = 0; l < v_list.size(); l++) {
			cui_separators.drop();
			cui_separators.shift_at(CHAR_ROW_SEP, cui_row_separator_offset * (l + 1));
			if (l - cui_delta >= cui_h - 2) break;
			if (l >= cui_delta)     {
				const noaftodo_entry& entry = t_list.at(v_list.at(l));
				
				if (l == cui_v_line) attron(A_STANDOUT);
				if (entry.completed) attron(COLOR_PAIR(CUI_CP_GREEN_ENTRY) | A_BOLD);	// a completed entry
				else if (entry.is_failed()) attron(COLOR_PAIR(CUI_CP_RED_ENTRY) | A_BOLD);	// a failed entry
				else if (entry.is_coming()) attron(COLOR_PAIR(CUI_CP_YELLOW_ENTRY) | A_BOLD);	// an upcoming entry

				x = 0;
				move(l - cui_delta + 1, x);
				for (int i = 0; i < cui_w; i++) addch(' ');

				for (int coln = 0; coln < cols.length(); coln++) {
					try {
						const char& col = cols.at(coln);
						if (x >= cui_w) break;
						move(l - cui_delta + 1, x);
						const int w = cui_columns.at(col).width(cui_w, cui_w - x, cols.length());
						cui_text_box(x, l - cui_delta + 1, w, 1, cui_columns.at(col).contents(entry, v_list.at(l)));

						if (coln < cols.length() - 1) if (x + w < cui_w) {
							move(l - cui_delta + 1, x + w);
							addstr((" " + cui_separators.s_get(CHAR_ROW_SEP) + " ").c_str());
						}
						x += w + 3;

						for (int x1 = x; x1 < cui_w; x1++) addch(' ');
					} catch (const out_of_range& e) {}
				}

				move(l - cui_delta + 1, cui_w - 1);
				addstr(" ");

				attrset(A_NORMAL);
				last_string = l - cui_delta + 2;
			}
		}
	}

	for (int s = last_string; s < cui_h; s++) { move(s, 0); clrtoeol(); }

	string cui_status_l = "";

	for (const char& c : cui_normal_status_fields) {
		try {
			const string field = (cui_status_fields.at(c))();
			if (field != "") {
				if (cui_status_l != "") cui_status_l = " " + cui_separators.s_get(CHAR_STA_SEP) + " " + cui_status_l;
		       		cui_status_l = field + cui_status_l;
			}
		} catch (const out_of_range& e) {}
	}

	if (cui_status_standout) attron(A_STANDOUT);
	attron(COLOR_PAIR(CUI_CP_STATUS));
	move(cui_h - 1, 0);
	for (int x = 0; x < cui_w; x++) addch(' ');
	move(cui_h - 1, cui_w - 1 - w_converter.from_bytes(cui_status_l).length());
	addstr(cui_status_l.c_str());
	attrset(A_NORMAL);
}

void cui_normal_input(const wchar_t& key) { }

void cui_details_paint() {
	const int tdelta = cui_delta;
	cui_normal_paint();

	cui_draw_border(3, 2, cui_w - 6, cui_h - 4, cui_box_strong);
	cui_clear_box(4, 3, cui_w - 8, cui_h - 6);
	// fill the box with details
	// Title
	const noaftodo_entry& entry = t_list.at(cui_s_line);
	cui_text_box(5, 4, cui_w - 10, 1, entry.title);

	for (int i = 4; i < cui_w - 4; i++) {
		move(6, i);
		addstr(cui_box_light.s_get(CHAR_HLINE).c_str());
		cui_box_light.drop();
	}

	string tag = "";
	if (entry.tag < t_tags.size()) if (t_tags.at(entry.tag) != to_string(entry.tag))
		tag = ": " + t_tags.at(entry.tag);

	string info_str = "";
	for (int coln = 0; coln < cui_details_cols.length(); coln++) {
		try {
			const char& col = cui_details_cols.at(coln);

			info_str += cui_columns.at(col).contents(entry, cui_s_line);
			if (coln < cui_details_cols.length() - 1) info_str += " " + cui_separators.s_get(CHAR_DET_SEP) + " ";
		} catch (const out_of_range& e) {}
	}

	cui_text_box(5, 7, cui_w - 10, 1, info_str);

	for (int i = 4; i < cui_w - 4; i++) {
		move(8, i);
		addstr(cui_box_light.s_get(CHAR_HLINE).c_str());
		cui_box_light.drop();
	}

	// draw description
	// we want text wrapping here
	cui_text_box(5, 10, cui_w - 10, cui_h - 14, entry.description);
}

void cui_details_input(const wchar_t& key) {
	switch (key) {
		case KEY_RIGHT:
			cui_delta--;
			break;
		case KEY_LEFT:
			cui_delta++;
			break;
		case '=':
			cui_delta = 0;
			break;
		default:
			break;
	}
}

void cui_command_paint() {
	switch (cui_prev_modes.top()) {
		case CUI_MODE_NORMAL:
			cui_normal_paint();
			break;
		case CUI_MODE_LISTVIEW:
			cui_listview_paint();
			break;
		case CUI_MODE_DETAILS:
			cui_details_paint();
			break;
		case CUI_MODE_HELP:
			cui_help_paint();
			break;
	}

	move(cui_h - 1, 0);
	if (cui_status_standout) attron(A_STANDOUT);
	attron(COLOR_PAIR(CUI_CP_STATUS));
	for (int x = 0; x < cui_w; x++) addch(' ');
	move(cui_h - 1, 0);
	int offset = cui_command_cursor - cui_w + 3;
	if (offset < 0) offset = 0;
	addstr(w_converter.to_bytes(L":" + cui_command.substr(offset)).c_str());
	move(cui_h - 1, 1 + cui_command_cursor - offset);
}

void cui_command_input(const wchar_t& key) {
	switch (key) {
		case 10:
			if (cui_command != L"") {
				cmd_exec(w_converter.to_bytes(cui_command));
				cui_command_history.push_back(cui_command);
				cui_command_index = cui_command_history.size();
				cmd_terminate();
			}
		case 27:
			cui_command = L"";
			if (cui_mode == CUI_MODE_COMMAND) cui_set_mode(-1);
			cui_filter_history();
			break;
		case 127: case KEY_BACKSPACE: 	// 127 is for, e.g., xfce4-terminal
						// KEY_BACKSPACE - e.g., alacritty
			cui_command_index = cui_command_history.size();

			if (cui_command_cursor > 0) {
				cui_command = cui_command.substr(0, cui_command_cursor - 1) + cui_command.substr(cui_command_cursor, cui_command.length() - cui_command_cursor);
				cui_command_cursor--;
			}

			goto contexec;
			break;
		case KEY_DC:
			cui_command_index = cui_command_history.size();

			if (cui_command_cursor < cui_command.length())
				cui_command = cui_command.substr(0, cui_command_cursor) + cui_command.substr(cui_command_cursor + 1, cui_command.length() - cui_command_cursor - 1);

			goto contexec;
			break;
		case KEY_LEFT:
			if (cui_command_cursor > 0) cui_command_cursor--;
			break;
		case KEY_RIGHT:
			if (cui_command_cursor < cui_command.length()) cui_command_cursor++;
			break;
		case KEY_UP:
			// go up the history
			if (cui_command_index > 0) {
				if (cui_command_index == cui_command_history.size()) cui_command_t = cui_command;
				cui_command_index--;
				cui_command = cui_command_history[cui_command_index];
			}

			if (cui_command_cursor > cui_command.length()) cui_command_cursor = cui_command.length();
			break;
		case KEY_DOWN:
			// go down the history
			if (cui_command_index < cui_command_history.size() - 1) {
				cui_command_index++;
				cui_command = cui_command_history[cui_command_index];
			} else if (cui_command_index == cui_command_history.size() - 1) {
				cui_command_index++;
				cui_command = cui_command_t;
			}

			if (cui_command_cursor > cui_command.length()) cui_command_cursor = cui_command.length();
			break;
		case KEY_HOME:
			cui_command_cursor = 0;
			break;
		case KEY_END:
			cui_command_cursor = cui_command.length();
			break;
		default:
			cui_command_index = cui_command_history.size();

			cui_command = cui_command.substr(0, cui_command_cursor) + key + cui_command.substr(cui_command_cursor, cui_command.length() - cui_command_cursor);
			cui_command_cursor++;

			// continious command execution
			contexec:
			if (cui_contexec_regex_filter != "") {
				regex ce_regex(cui_contexec_regex_filter);

				if (regex_search(w_converter.to_bytes(cui_command), ce_regex))
					cmd_exec(w_converter.to_bytes(cui_command));
			}
	}
}

void cui_help_paint() {
	const int tdelta = cui_delta;
	cui_normal_paint();

	cui_draw_border(3, 2, cui_w - 6, cui_h - 4, cui_box_strong);
	cui_clear_box(4, 3, cui_w - 8, cui_h - 6);

	// fill the box
	move(4, 5);
	addstr((string(TITLE) + " v." + VERSION).c_str());

	for (int i = 4; i < cui_w - 4; i++) {
		move(6, i);
		addstr(cui_box_light.s_get(CHAR_HLINE).c_str());
		cui_box_light.drop();
	}

	// draw description
	// we want text wrapping here
	int x = 5;
	int y = 8 + tdelta;
	int tabs = 0;
	string cui_help;
	for (char c : DOC)
		cui_help += string(1, c);

	for (int i = 0; i < cui_help.length(); i++) {
		if (x == cui_w - 5) {
			x = 5;
			y++;
		}
		
		if (y >= cui_h - 4)  {
			move(cui_h - 4, 5);
			addstr("<- ... ->");
			break;
		}

		const char c = cui_help.at(i);

		move(y, x);

		constexpr int TAB_W = 30;
		switch (c) {
			case '\n':
				y++;
				tabs = 0;
				x = 5;
				break;
			case '\t':
				tabs++;
				x = tabs * TAB_W;
				break;
			default:
				if (y >= 8) addch(c);
				x++;
		}
	}

	cui_delta = tdelta;
}

void cui_help_input(const wchar_t& key) {
	switch (key) {
		case KEY_RIGHT:
			cui_delta--;
			break;
		case KEY_LEFT:
			if (cui_delta < 0) cui_delta++;
			break;
		case '=':
			cui_delta = 0;
			break;
	}
}

void cui_safemode_box() {
	cui_box_strong.drop();

	vector<string> mes_lines = { "SAFE MODE", "List won't autosave" };

	if ((errors & ERR_CONF_V) != 0) mes_lines.push_back("Config version mismatch");
	if ((errors & ERR_LIST_V) != 0) mes_lines.push_back("List version mismatch");

	int maxlen = 0;

	for (auto s : mes_lines) if (maxlen < s.length()) maxlen = s.length();

	int x0 = cui_w - maxlen - 3;
	int y0 = 2;

	// draw message box
	cui_draw_border(x0, y0, maxlen + 2, mes_lines.size() + 2, cui_box_strong);
	cui_clear_box(x0 + 1, y0 + 1, maxlen, mes_lines.size());

	for (int i = 0; i < mes_lines.size(); i++) {
		move(y0 + 1 + i, x0 + 1);
		addstr(mes_lines.at(i).c_str());
	}
}

string cui_prompt(const string& message) {
	cui_command = L"";
	curs_set(1);
	cui_command_index = cui_command_history.size();
	cui_command_cursor = cui_command.length();

	for (wint_t c = 0; ; get_wch(&c)) {
		switch (c) {
			case 10:
				return w_converter.to_bytes(cui_command);
				break;
			case 27:
				return "";
				break;
			case 0:
				break;
			default:
				cui_command_input(c);
				break;
		}

		move(cui_h - 1, 0);
		if (cui_status_standout) attron(A_STANDOUT);
		attron(COLOR_PAIR(CUI_CP_STATUS));
		for (int x = 0; x < cui_w; x++) addch(' ');
		move(cui_h - 1, 0);
		int offset = cui_command_cursor - cui_w + 3;
		if (offset < 0) offset = 0;
		addstr((message + w_converter.to_bytes(cui_command.substr(offset))).c_str());
		move(cui_h - 1, 1 + cui_command_cursor - offset);
	}

	return "";
}

void cui_filter_history() {
	for (int i = 0; i < cui_command_history.size() - 1; i++)
		if (cui_command_history[i] == w_converter.from_bytes(""))  {
			cui_command_history.erase(cui_command_history.begin() + i);
			i--;
		}
}

wchar_t cui_key_from_str(const string& str) {
	if (str.length() == 1)
		return str.at(0);

	if (str == "up")	return KEY_UP;
	if (str == "down")	return KEY_DOWN;
	if (str == "left")	return KEY_LEFT;
	if (str == "right")	return KEY_RIGHT;
	if (str == "esc")	return 27;
	if (str == "enter")	return 10;
	if (str == "tab")	return 9;

	return 0;
}

void cui_clear_box(const int& x, const int& y, const int& w, const int& h) {
	for (int j = x; j < x + w; j++)
		for (int i = y; i < y + h; i++) {
			move(i, j);
			addch(' ');
		}
}

void cui_draw_border(const int& x, const int& y, const int& w, const int& h, multistr_c& chars) {
	// draw corners
	move(y, x);
	addstr(chars.s_get(CHAR_CORN1).c_str());
	move(y, x + w - 1);
	addstr(chars.s_get(CHAR_CORN2).c_str());
	move(y + h - 1, x);
	addstr(chars.s_get(CHAR_CORN3).c_str());
	move(y + h - 1, x + w - 1);
	addstr(chars.s_get(CHAR_CORN4).c_str());

	chars.drop();

	// draw vertical lines
	for (int i = y + 1; i < y + h - 1; i++) {
		move(i, x);
		addstr(chars.s_get(CHAR_VLINE).c_str());
		chars.drop();

		move(i, x + w - 1);
		addstr(chars.s_get(CHAR_VLINE).c_str());
		chars.drop();
	}

	// draw horizontal lines
	for (int j = x + 1; j < x + w - 1; j++) {
		move(y, j);
		addstr(chars.s_get(CHAR_HLINE).c_str());
		chars.drop();

		move(y + h - 1, j);
		addstr(chars.s_get(CHAR_HLINE).c_str());
		chars.drop();
	}
}

void cui_text_box(const int& x, const int& y, const int& w, const int& h, const string& str, const bool& show_md) {
	int t_x = x;
	int t_y = y;

	auto putwch = [&t_x, &t_y, &w, &h, &x, &y] (const wchar_t& wch) {
		if (t_x >= x + w) {
			t_x = x;
			t_y++;
		}
		if (t_y >= y + h) return;

		move(t_y, t_x);
		addstr(w_converter.to_bytes(wch).c_str());
		t_x++;
	};

	wstring wstr = w_converter.from_bytes(str);
	if (!show_md) {
		for (const auto& wch : wstr) {
			putwch(wch);
			if (t_y >= y + h) return;
		}
		return;
	}

	int text_attrs = A_NORMAL;
	int pair = 0;
	attr_get(&text_attrs, &pair, NULL);

	int attrs = 0;
	for (int i = 0; i < wstr.length(); i++) {
		const auto& wch = wstr.at(i);

		putwch(wch);

		if (wstr.substr(i + 1, 2) == L"**") { attrs ^= A_BOLD; i += 2; attrset(attrs | text_attrs); }
		else if (wstr.substr(i + 1, 1) == L"*") { attrs ^= A_ITALIC; i++; attrset(attrs | text_attrs); }
		else if (wstr.substr(i + 1, 2) == L"~~") { attrs ^= A_UNDERLINE; i += 2; attrset(attrs | text_attrs); }
	}

	attrset(text_attrs);
}
