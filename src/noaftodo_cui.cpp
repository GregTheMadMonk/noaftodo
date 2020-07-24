#include "noaftodo_cui.h"

#include <regex>

#include "noaftodo_cmd.h"
#include "noaftodo_config.h"
#include "noaftodo_cvar.h"
#include "noaftodo_daemon.h"
#include "noaftodo_time.h"

using namespace std;

map<char, cui_col_s> cui_lview_columns;
map<char, cui_col_s> cui_columns;
map<char, function<string()>> cui_status_fields;

int cui_halfdelay_time = 2;

int cui_filter;
int cui_tag_filter;
string cui_normal_regex_filter;
string cui_listview_regex_filter;

string cui_contexec_regex_filter;

bool cui_shift_multivars = false;

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

extern string CMDS_HELP;

void cui_init() {
	log("Initializing console UI...");

	using namespace vargs::cols;

	// initialize columns
	cui_columns = {
		// NORMAL column "t" - shows task title
		{ 't',	{
				"Title",
				[](const int& w, const int& free, const int& cols) {
					return free / 4;
				},
				[](const varg& args) {
					return get<normal>(args).e.title;
				}
			}
		},
		// NORMAL column "f" - shows task flags
		{ 'f',	{
				"Flags",
				[](const int& w, const int& free, const int& cols) {
					return 5;
				},
				[](const varg& args) {
					string ret = "";
					const auto& e = get<normal>(args).e;
					if (e.completed) ret += "V";	// a completed entry
					if (e.is_failed()) ret += "F";	// a failed entry
					if (e.is_coming()) ret += "C";	// an upcoming entry
					if (e.get_meta("nodue") == "true") ret += "N";	// a "nodue" entry
					return ret;
				}
			}
		},
		// NORMAL column "l" - shows what list the task is attached to
		{ 'l',	{
				"List",
				[](const int& w, const int& free, const int& cols) {
					return free / 10;
				},
				[](const varg& args) {
					const auto& e = get<normal>(args).e;
					if (e.tag < t_tags.size())
					       if (t_tags.at(e.tag) != to_string(e.tag))
						       return to_string(e.tag) + ": " + t_tags.at(e.tag);

					return to_string(e.tag);
				}
			}
		},
		// NORMAL column "d" - shows task due
		{ 'd',	{
				"Due",
				[](const int& w, const int& free, const int& cols) {
					return 16;
				},
				[](const varg& args) {
					const auto& e = get<normal>(args).e;
					if (e.get_meta("nodue") == "true") return string("----------------");
					else return ti_f_str(e.due);
				}
			}
		},
		// NORMAL column "D" - shows task description
		{ 'D',	{
				"Description",
				[](const int& w, const int& free, const int& cols) {
					return free;
				},
				[](const varg& args) {
					return get<normal>(args).e.description;
				}
			}
		},
		// NORMAL column "i" - shows task ID (as referred by UI, don't confuse with task unique ID)
		{ 'i',	{
				"ID",
				[](const int& w, const int& free, const int& cols) {
					return 3;
				},
				[](const varg& args) {
					return to_string(get<normal>(args).id);
				}
			}
		}
	};
	
	// initialize listview columns
	cui_lview_columns = {
		// LISTVIEW column "i" - shows list number (aka ID)
		{ 'i',	{
				"ID",
				[](const int& w, const int& free, const int& cols) {
					return 3;
				},
				[](const varg& args) {
					const auto& list_id = get<lview>(args).l_id;
					if (list_id == -1) return string(" ");
					return to_string(list_id);
				}
			}
		},
		// LISTVIEW column "f" - shows list flags
		{ 'f',	{
				"Flags",
				[] (const int& w, const int& free, const int& cols) {
					return 5;
				},
				[] (const varg& args) {
					const auto& list_id = get<lview>(args).l_id;
					return string(li_tag_completed(list_id) ? "V" : "") +
						string(li_tag_coming(list_id) ? "C" : "") +
						string(li_tag_failed(list_id) ? "F" : "");
				}
			}
		},
		// LISTVIEW column "t" - shows list title
		{ 't',	{
				"Title",
				[](const int& w, const int& free, const int& cols) {
					return free / 4;
				},
				[](const varg& args) {
					const auto& list_id = get<lview>(args).l_id;
					if (list_id == CUI_TAG_ALL) return string("All lists");
					return t_tags.at(list_id);
				}
			}
		},
		// LISTVIEW column "e" - shows the amount of entries attached to the list
		{ 'e',	{
				"Entries",
				[](const int& w, const int& free, const int& cols) {
					return 7;
				},
				[](const varg& args) {
					const auto& list_id = get<lview>(args).l_id;
					if (list_id == CUI_TAG_ALL) return to_string(t_list.size());

					int ret = 0;
					for (auto e : t_list) if (e.tag == list_id) ret++;

					return to_string(ret);
				}
			}
		},
		// LISTVIEW column "p" - shows the precentge of completed entries on the list
		{ 'p',	{
				"%",
				[](const int& w, const int& free, const int& cols) {
					return 4;
				},
				[](const varg& args) {
					const auto& list_id = get<lview>(args).l_id;
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
			}
		}
	};

	// initialize status fields
	cui_status_fields = {
		// status field "s" - shows cui_status value (output of commands)
		{ 's',	[] () {
				return cui_status;
			}
		},
		// status field "l" - shows current list name
		{ 'l',	[] () {
				if (cui_tag_filter == CUI_TAG_ALL) return string("All lists");

				return "List " + to_string(cui_tag_filter) + (((cui_tag_filter < t_tags.size()) && (t_tags.at(cui_tag_filter) != to_string(cui_tag_filter))) ? (": " + t_tags.at(cui_tag_filter)) : "");
			}
		},
		// status field "m" - shows current mode (NORMAL|LISTVIEW|DETAILS|HELP)
		{ 'm',	[] () {
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
			}
		},
		// status field "f" - shows filters
		{ 'f',	[] () {
				return string((cui_filter & CUI_FILTER_UNCAT) ? "U" : "_") +
					string((cui_filter & CUI_FILTER_COMPLETE) ? "V" : "_") +
					string((cui_filter & CUI_FILTER_COMING) ? "C" : "_") +
					string((cui_filter & CUI_FILTER_FAILED) ? "F" : "_") +
					string((cui_filter & CUI_FILTER_NODUE) ? "N" : "_") +
					string((cui_filter & CUI_FILTER_EMPTY) ? "0" : "_") +
					((cui_normal_regex_filter == "") ? "" : (" [e " + cui_normal_regex_filter + "]")) +
					((cui_listview_regex_filter == "") ? "" : (" [l " + cui_listview_regex_filter + "]"));
			}
		},
		// status field "i" - shows selected task ID and a total amount of entries
		{ 'i',	[] () {
				bool has_visible = false;

				for (int i = 0; i < t_list.size(); i++) has_visible |= cui_is_visible(i);

				if (!has_visible) return string("");

				return to_string(cui_s_line) + "/" + to_string(t_list.size() - 1);
			}
		},
		// status field "p" - shows the percentage of completed entries on a current list
		{ 'p',	[] () {
				int total = 0;
				int comp = 0;

				for (int i = 0; i < t_list.size(); i++)
					if ((cui_tag_filter == CUI_TAG_ALL) || (cui_tag_filter == t_list.at(i).tag)) {
						total++;
						if (t_list.at(i).completed) comp++;
					}

				if (total == 0) return string("");

				return to_string((int)(100.0 * comp / total)) + "%";
			}
		},
		// status field "P" - shows the amount of completed entries on the list against the total amount of attached entries
		{ 'P', [] () {
				int total = 0;
				int comp = 0;

				for (int i = 0; i < t_list.size(); i++)
					if ((cui_tag_filter == CUI_TAG_ALL) || (cui_tag_filter == t_list.at(i).tag)) {
						total++;
						if (t_list.at(i).completed) comp++;
					}

				if (total == 0) return string("");

				return to_string(comp) + "/" + to_string(total) + " COMP";
			}
		}
	};
	
	// construct UI
	cui_command_history.push_back(w_converter.from_bytes(""));

	cui_construct();
}

void cui_construct() {
	initscr();
	start_color();
	use_default_colors();

	// initialize color pairs
	for (short f = -1; f < 16; f++)
		for (short b = -1; b < 16; b++) {
			init_pair(f + 1 + (b + 1) * 17, f, b);
		}

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
					if (bind.autoexec) cmd::exec(bind.command);
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
			attrset_ext(A_NORMAL);
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
	const int last_string = cui_draw_table(0, 0, cui_w, cui_h - 2,
			[] (const int& item) {
				return vargs::cols::varg(vargs::cols::lview { item });
			},
			cui_l_is_visible,
			[] (const int& item) {
				return ((item >= -1) && (item < (int)t_tags.size()));
			},
			[] (const int& item) -> cui_attrs {
				if (li_tag_completed(item)) return { A_BOLD, cui_color_complete };
				if (li_tag_failed(item)) return { A_BOLD, cui_color_failed };
				if (li_tag_coming(item)) return { A_BOLD, cui_color_coming };

				return { A_NORMAL, 0 };
			},
			-1, cui_tag_filter,
			"math %tag_filter% + 1 tag_fitler",
			cui_listview_cols, cui_lview_columns);

	for (int s = last_string; s < cui_h; s++) { move(s, 0); clrtoeol(); }

	cui_draw_status(cui_listview_status_fields);
}

void cui_listview_input(const wchar_t& key) { }

void cui_normal_paint() {
	const int last_string =
		cui_draw_table(0, 0, cui_w, cui_h - 2,
				[] (const int& item) {
					return vargs::cols::varg(vargs::cols::normal {
							t_list.at(item),
							item
							});
				},
				cui_is_visible,
				[] (const int& item) {
					return ((item >= 0) && (item < t_list.size()));
				},
				[] (const int& item) -> cui_attrs {
					const auto& e = t_list.at(item);

					if (e.completed) return { A_BOLD, cui_color_complete };
					if (e.is_failed()) return { A_BOLD, cui_color_failed };
					if (e.is_coming()) return { A_BOLD, cui_color_coming };

					return { A_NORMAL, 0 };
				},
				0, cui_s_line,
				"math %id% + 1 id",
				(cui_tag_filter == CUI_TAG_ALL) ? cui_normal_all_cols : cui_normal_cols,
				cui_columns);


	for (int s = last_string; s < cui_h; s++) { move(s, 0); clrtoeol(); }

	cui_draw_status(cui_normal_status_fields);
}

void cui_normal_input(const wchar_t& key) { }

void cui_details_paint() {
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

			info_str += cui_columns.at(col).contents(vargs::cols::normal { entry, cui_s_line });
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
	cui_text_box(5, 10, cui_w - 10, cui_h - 14, entry.description, true, cui_delta);
}

void cui_details_input(const wchar_t& key) {
	switch (key) {
		case KEY_LEFT:
			if (cui_delta > 0) cui_delta--;
			break;
		case KEY_RIGHT:
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
	attron_ext(0, cui_color_status);
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
				cmd::exec(w_converter.to_bytes(cui_command));
				cui_command_history.push_back(cui_command);
				cui_command_index = cui_command_history.size();
				cmd::terminate();
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
					cmd::exec(w_converter.to_bytes(cui_command));
			}
	}
}

void cui_help_paint() {
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
	string cui_help;
	for (char c : CMDS_HELP)
		cui_help += string(1, c);

	cui_text_box(5, 8, cui_w - 10, cui_h - 12, cui_help, true, cui_delta);
}

void cui_help_input(const wchar_t& key) {
	switch (key) {
		case KEY_LEFT:
			if (cui_delta > 0) cui_delta--;
			break;
		case KEY_RIGHT:
			cui_delta++;
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
		attron_ext(0, cui_color_status);
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
		if (cui_command_history[i] == L"")  {
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

int cui_pair_from_str(const string& str) {
	short fg = -1;
	short bg = -1;

	bool last = false; // false -> fg, true -> bg

	string buffer = "";

	for (const auto& c : (str + (char)0))
		switch (c) {
			case ';': case 0:
				try {
					const int& val = stoi(buffer);
					buffer = "";
					switch (val / 10) {
						case 3:
							fg = val % 10;
							last = false;
							break;
						case 4:
							bg = val % 10;
							last = true;
							break;
						case 0:
							if (val == 1) {
								if (last) { if (bg >= 0) bg += 8; }
								else { if (fg >= 0) fg += 8; }
							}
							break;
					}
				} catch (const invalid_argument& e) { }
				break;
			default:
				buffer += c;
		}

	return fg + 1 + (bg + 1) * 17;
}

int cui_draw_table(const int& x, const int& y,
		const int& w, const int& h,
		const std::function<vargs::cols::varg(const int& item)>& colarg_f,
		const std::function<bool(const int& item)>& vis_f,
		const std::function<bool(const int& item)>& cont_f,
		const std::function<cui_attrs(const int& item)>& attrs_f,
		const int& start, int& sel,
		const std::string& down_cmd,
		const std::string& cols, const std::map<char, cui_col_s>& colmap) {
	int t_x = x;
	int t_y = y;

	// draw table title
	move(t_y, t_x);
	attrset_ext(A_STANDOUT | A_BOLD, cui_color_title);
	for (int i = 0; i < w; i++) addch(' ');

	for (int coln = 0; coln < cols.length(); coln++) {
		try {
			const char& col = cols.at(coln);
			if (t_x >= x + w) break;
			const int& cw = colmap.at(col).width(w, x + w - t_x, cols.length());
			cui_text_box(t_x, t_y, cw, 1, colmap.at(col).title);

			if ((coln < cols.length() - 1) && (t_x + cw < x + w)) {
				move(t_y, t_x + cw);
				addstr((" " + cui_separators.s_get(CHAR_ROW_SEP) + " ").c_str());
			}

			t_x += cw + 3;
		} catch (const out_of_range& e) { }
	}
	attrset_ext(A_NORMAL);

	// form a list of displayed elements
	vector<int> v_list;
	int v_sel = -1;

	for (int l = start; cont_f(l); l++)
		if (vis_f(l)) {
			v_list.push_back(l);
			if (l == sel) v_sel = v_list.size() - 1;
		}

	if (v_list.size() != 0) {
		while (!vis_f(sel)) cmd::exec(down_cmd);
		for (int i = 0; i < v_list.size(); i++) if (v_list.at(i) == sel) { v_sel = i; break; }
	}

	// calculate list offset
	int l_offset = 0;
	if (v_sel - l_offset >= h) l_offset = v_sel - h + 3;
	if (v_sel - l_offset < 0) l_offset = v_sel;

	if (v_list.size() == 0) {
		sel = -1;
		return t_y + 1;
	}

	// draw list
	for (int l = 0; l < v_list.size(); l++) {
		cui_separators.drop();
		cui_separators.shift_at(CHAR_ROW_SEP, cui_row_separator_offset * (l + 1));
		if (l - l_offset >= h - 2) break;
		if (l < l_offset) continue;

		const auto a_f = attrs_f(v_list.at(l));
		attrset_ext(a_f.attrs | ((l == v_sel) ? A_STANDOUT : 0), a_f.pair);
		t_x = x;
		t_y++;

		move(t_y, t_x);
		for (int i = 0; i < w; i++) addch(' ');

		for (int coln = 0; coln < cols.length(); coln++) {
			try {
				const char& col = cols.at(coln);
				if (t_x >= x + w) break;
				const int cw = colmap.at(col).width(w, x + w - t_x, cols.length());
				cui_text_box(t_x, t_y, cw, 1, colmap.at(col).contents(colarg_f(v_list.at(l))));

				if ((coln < cols.length() - 1) && (t_x + cw < x + w)) {
					move(t_y, t_x + cw);
					addstr((" " + cui_separators.s_get(CHAR_ROW_SEP) + " ").c_str());
				}

				t_x += cw + 3;
			} catch (const out_of_range& e) { }
		}

		move(t_y, x + w - 1);
		addstr(" ");

		attrset_ext(A_NORMAL);
	}

	return t_y + 1;
}

void cui_draw_status(const string& fields) {
	string cui_status_l = "";

	for (int i = fields.length() - 1; i >= 0; i--) {
		const char& c = fields.at(i);
		try {
			const string field = (cui_status_fields.at(c))();
			if (field != "") {
				if (cui_status_l != "") cui_status_l = " " + cui_separators.s_get(CHAR_STA_SEP) + " " + cui_status_l;
		       		cui_status_l = field + cui_status_l;
			}
		} catch (const out_of_range& e) {}
	}

	cui_status_l += " ";

	attron_ext(0, cui_color_status);
	move(cui_h - 1, 0);
	for (int x = 0; x < cui_w; x++) addch(' ');
	const auto len = cui_text_box(cui_w - 1 - w_converter.from_bytes(cui_status_l).length(), cui_h - 1, cui_w, 1, cui_status_l, true, 0, true).w;
	cui_text_box(cui_w - 1 - len, cui_h - 1, cui_w, 1, cui_status_l);
	attrset_ext(A_NORMAL);
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

box_dim cui_text_box(const int& x, const int& y, const int& w, const int& h, const string& str, const bool& show_md, const int& line_offset, const bool& nullout) {
	int t_x = x;
	int t_y = y - line_offset;

	bool c033 = false;

	wstring buffer = L"";

	int attrs = 0;
	int pair = 0;

	int rw = 0;
	int rh = 0;

	auto putwch = [&t_x, &t_y, &w, &h, &x, &y, &line_offset, &nullout, &c033, &buffer, &attrs, &pair, &rh, &rw] (const wchar_t& wch) {
		switch (wch) {
			case L'\n':
				t_x = x;
				t_y++;
				return;
			case L'\t':
				t_x += 4;
				if (t_x >= x + w) {
					t_x = x;
					t_y++;
				}
				return;
			case L'\033':
				c033 = true;
				return;
			case L'[':
				if (c033) return;
			case L'm':
				if (!c033) break;

				c033 = false;

				try {
					pair = cui_pair_from_str(w_converter.to_bytes(buffer));
					attrset_ext(attrs, pair);
				} catch (const invalid_argument& e) { }

				buffer = L"";
				return;
		}

		if (c033) { buffer += wch; return; }

		if (t_x >= x + w) {
			t_x = x;
			t_y++;
		}
		if (t_y >= y + h) return;

		if ((t_y >= y) && !nullout) {
			move(t_y, t_x);
			addstr(w_converter.to_bytes(wch).c_str());
		}

		if (t_x - x > rw) rw = t_x - x;
		if (t_y - y > rh) rh = t_y - y;

		t_x++;
	};

	wstring wstr = w_converter.from_bytes(str);
	if (!show_md) {
		for (const auto& wch : wstr) {
			putwch(wch);
			if (t_y >= y + h) break;
		}
		return { rw, rh };
	}

	int text_attrs = A_NORMAL;
	int text_pair;
	attr_get(&text_attrs, &text_pair, NULL);
	attrs = text_attrs & ~COLOR_PAIR(text_pair);
	pair = text_pair;

	int i = 0;
	bool skip_special = false;
	for (; i < wstr.length(); i++) {
		if (skip_special) {
			putwch(wstr.at(i));
			skip_special = false;
			continue;
		}

		for (const auto& sw : cui_md_switches)
			if (wstr.substr(i, sw.second.length()) == sw.second) {
				attrs ^= sw.first;
				i += sw.second.length() - 1;
				attrset_ext(attrs, pair);
				goto skip_putwch;
			}

		if (wstr.at(i) == L'\\') skip_special = true;
		else putwch(wstr.at(i));

		skip_putwch:;
	}

	attrset_ext(text_attrs, text_pair);

	text_box_ret:
	return { rw, rh };
}
