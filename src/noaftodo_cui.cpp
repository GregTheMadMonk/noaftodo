#include "noaftodo_cui.h"

#include <regex>

#include "noaftodo_cmd.h"
#include "noaftodo_config.h"
#include "noaftodo_cvar.h"
#include "noaftodo_daemon.h"
#include "noaftodo_time.h"

using namespace std;

using li::t_list;
using li::t_tags;

extern string CMDS_HELP;

namespace cui {

void init() {
	log("Initializing console UI...");

	// construct UI
	command_history.push_back(w_converter.from_bytes(""));

	construct();
}

void construct() {
	initscr();
	start_color();
	use_default_colors();

	// initialize color pairs
	for (short f = -1; f < 16; f++)
		for (short b = -1; b < 16; b++) {
			init_pair(f + 1 + (b + 1) * 17, f, b);
		}

	if (halfdelay_time == 0) cbreak();
	else halfdelay(halfdelay_time);
	set_escdelay(0);
	curs_set(0);
	noecho();
	keypad(stdscr, true);

	w = getmaxx(stdscr);
	h = getmaxy(stdscr);

	active = true;
}

void destroy() {
	endwin();
	active = false;
}

void run() {
	init();
	set_mode(MODE_NORMAL);

	for (wint_t c = -1; ; (get_wch(&c) != ERR) ? : (c = 0)) {
		if (li::has_changed()) li::load(false); // load only list contents, not the workspace
		else if ((c == 0) && (!shift_multivars)) continue;

		if (c > 0) {
			const int old_numbuffer = numbuffer;
			status = "";
			bool bind_fired = false;
			for (const auto& bind : binds)
				if ((bind.mode & mode) && (bind.key == c)) {
					if (bind.autoexec) cmd::exec(bind.command);
					else  {
						if ((s_line >= 0) && (s_line < t_list.size()))
							command = w_converter.from_bytes(format_str(bind.command, &t_list.at(s_line)));
						else
							command = w_converter.from_bytes(bind.command);
						set_mode(MODE_COMMAND);
					}

					bind_fired = true;
					break;
				}

			if (!bind_fired) switch (mode) {
				case MODE_NORMAL:
					normal_input(c);
					break;
				case MODE_LISTVIEW:
					listview_input(c);
					break;
				case MODE_DETAILS:
					details_input(c);
					break;
				case MODE_COMMAND:
					command_input(c);
					break;
				case MODE_HELP:
					help_input(c);
			}

			if (old_numbuffer == numbuffer) numbuffer = -1;
		}

		w = getmaxx(stdscr);
		h = getmaxy(stdscr);

		if (shift_multivars) {
			box_strong.shift();
			box_light.shift();
			separators.shift();
		}

		box_strong.drop();
		box_light.drop();
		separators.drop();

		switch (mode) {
			case MODE_NORMAL:
				normal_paint();
				break;
			case MODE_LISTVIEW:
				listview_paint();
				break;
			case MODE_DETAILS:
				details_paint();
				break;
			case MODE_COMMAND:
				command_paint();
				break;
			case MODE_HELP:
				help_paint();
				break;
		}

		if (errors != 0) {
			attrset_ext(A_NORMAL);
			int old_cx, old_cy;
			getyx(stdscr, old_cy, old_cx);
			safemode_box();
			move(old_cy, old_cx);
		}

		if (mode == MODE_EXIT) break;
	}

	if (li::autosave) li::save();

	destroy();

	da::send("D");
}

void set_mode(const int& new_mode) {
	if (new_mode == -1) {
		if (prev_modes.size() == 0) return;
		mode = prev_modes.top();
		prev_modes.pop();
	} else {
		if ((mode != MODE_COMMAND) && (new_mode != mode)) prev_modes.push(mode);
		mode = new_mode;
	}

	delta = 0;

	switch (mode) {
		case MODE_NORMAL:
			if (cvar("tag_filter_copy") != "") {
				tag_filter = cvar("tag_filter_copy");
				cvar_base_s::erase("tag_filter_copy");
			}
			curs_set(0);
			break;
		case MODE_LISTVIEW:
			curs_set(0);
			cvar("tag_filter_copy") = tag_filter;
			break;
		case MODE_DETAILS:
			if ((s_line < 0) || (s_line >= t_list.size())) {
				set_mode(-1);
				return;
			}
			delta = 0;
			curs_set(0);
			break;
		case MODE_COMMAND:
			curs_set(1);
			command_index = command_history.size();
			command_cursor = command.length();
			break;
		case MODE_HELP:
			delta = 0;
			curs_set(0);
			break;
	}
}

void bind(const bind_s& bind) {
	binds.push_back(bind);
}

void bind(const wchar_t& key, const string& command, const int& mode, const bool& autoexec) {
	bind({ key, command, mode, autoexec });
}

bool is_visible(const int& entryID) {
	if (t_list.size() == 0) return false;
	if ((entryID < 0) && (entryID >= t_list.size())) return false;

	const auto& entry = t_list.at(entryID);

	bool ret = ((tag_filter == TAG_ALL) || (tag_filter == entry.tag) || (mode == MODE_LISTVIEW));

	if (entry.completed) 	ret = ret && (filter & FILTER_COMPLETE);
	if (entry.is_failed()) 	ret = ret && (filter & FILTER_FAILED);
	if (entry.is_coming()) 	ret = ret && (filter & FILTER_COMING);

	if (entry.get_meta("nodue") == "true") ret = ret && (filter & FILTER_NODUE);

	if (entry.is_uncat())	ret = ret && (filter & FILTER_UNCAT);

	// fit regex
	if (normal_regex_filter != "") {
		regex rf_regex(normal_regex_filter);

		ret = ret && (regex_search(entry.title, rf_regex) || regex_search(entry.description, rf_regex));
	}

	return ret;
}

bool l_is_visible(const int& list_id) {
	if (list_id == TAG_ALL) return true;
	if ((list_id < 0) || (list_id >= t_tags.size())) return false;

	bool ret = false;

	if ((filter & FILTER_EMPTY) == 0)
		for (int i = 0; i < t_list.size(); i++)
			ret |= ((t_list.at(i).tag == list_id) && is_visible(i));
	else ret = true;

	// fit regex
	if (listview_regex_filter != "") {
		regex rf_regex(listview_regex_filter);

		ret = ret && regex_search(t_tags.at(list_id), rf_regex);
	}

	return ret;
}

void listview_paint() {
	const int last_string = draw_table(0, 0, w, h - 2,
			[] (const int& item) {
				return vargs::cols::varg(vargs::cols::lview { item });
			},
			l_is_visible,
			[] (const int& item) {
				return ((item >= -1) && (item < (int)t_tags.size()));
			},
			[] (const int& item) -> attrs {
				if (li::tag_completed(item)) return { A_BOLD, color_complete };
				if (li::tag_failed(item)) return { A_BOLD, color_failed };
				if (li::tag_coming(item)) return { A_BOLD, color_coming };

				return { A_NORMAL, 0 };
			},
			-1, tag_filter,
			"math %tag_filter% + 1 tag_fitler",
			listview_cols, lview_columns);

	for (int s = last_string; s < h; s++) { move(s, 0); clrtoeol(); }

	draw_status(listview_status_fields);
}

void listview_input(const wchar_t& key) { }

void normal_paint() {
	const int last_string =
		draw_table(0, 0, w, h - 2,
				[] (const int& item) {
					return vargs::cols::varg(vargs::cols::normal {
							t_list.at(item),
							item
							});
				},
				is_visible,
				[] (const int& item) {
					return ((item >= 0) && (item < t_list.size()));
				},
				[] (const int& item) -> attrs {
					const auto& e = t_list.at(item);

					if (e.completed) return { A_BOLD, color_complete };
					if (e.is_failed()) return { A_BOLD, color_failed };
					if (e.is_coming()) return { A_BOLD, color_coming };

					return { A_NORMAL, 0 };
				},
				0, s_line,
				"math %id% + 1 id",
				(tag_filter == TAG_ALL) ? normal_all_cols : normal_cols,
				columns);


	for (int s = last_string; s < h; s++) { move(s, 0); clrtoeol(); }

	draw_status(normal_status_fields);
}

void normal_input(const wchar_t& key) { }

void details_paint() {
	normal_paint();

	draw_border(3, 2, w - 6, h - 4, box_strong);
	clear_box(4, 3, w - 8, h - 6);
	// fill the box with details
	// Title
	const li::entry& entry = t_list.at(s_line);
	text_box(5, 4, w - 10, 1, entry.title);

	for (int i = 4; i < w - 4; i++) {
		move(6, i);
		addstr(box_light.s_get(CHAR_HLINE).c_str());
		box_light.drop();
	}

	string tag = "";
	if (entry.tag < t_tags.size()) if (t_tags.at(entry.tag) != to_string(entry.tag))
		tag = ": " + t_tags.at(entry.tag);

	string info_str = "";
	for (int coln = 0; coln < details_cols.length(); coln++) {
		try {
			const char& col = details_cols.at(coln);

			info_str += columns.at(col).contents(vargs::cols::normal { entry, s_line });
			if (coln < details_cols.length() - 1) info_str += " " + separators.s_get(CHAR_DET_SEP) + " ";
		} catch (const out_of_range& e) {}
	}

	text_box(5, 7, w - 10, 1, info_str);

	for (int i = 4; i < w - 4; i++) {
		move(8, i);
		addstr(box_light.s_get(CHAR_HLINE).c_str());
		box_light.drop();
	}

	// draw description
	// we want text wrapping here
	text_box(5, 10, w - 10, h - 14, entry.description, true, delta);
}

void details_input(const wchar_t& key) {
	switch (key) {
		case KEY_LEFT:
			if (delta > 0) delta--;
			break;
		case KEY_RIGHT:
			delta++;
			break;
		case '=':
			delta = 0;
			break;
		default:
			break;
	}
}

void command_paint() {
	switch (prev_modes.top()) {
		case MODE_NORMAL:
			normal_paint();
			break;
		case MODE_LISTVIEW:
			listview_paint();
			break;
		case MODE_DETAILS:
			details_paint();
			break;
		case MODE_HELP:
			help_paint();
			break;
	}

	move(h - 1, 0);
	attron_ext(0, color_status);
	for (int x = 0; x < w; x++) addch(' ');
	move(h - 1, 0);
	int offset = command_cursor - w + 3;
	if (offset < 0) offset = 0;
	addstr(w_converter.to_bytes(L":" + command.substr(offset)).c_str());
	move(h - 1, 1 + command_cursor - offset);
}

void command_input(const wchar_t& key) {
	command_index = command_history.size();

	command = command.substr(0, command_cursor) + key + command.substr(command_cursor, command.length() - command_cursor);
	command_cursor++;

	// continious command execution
	if (contexec_regex_filter != "") {
		regex ce_regex(contexec_regex_filter);

		if (regex_search(w_converter.to_bytes(command), ce_regex))
			cmd::exec(w_converter.to_bytes(command));
	}
}

void help_paint() {
	normal_paint();

	draw_border(3, 2, w - 6, h - 4, box_strong);
	clear_box(4, 3, w - 8, h - 6);

	// fill the box
	move(4, 5);
	addstr((string(TITLE) + " v." + VERSION).c_str());

	for (int i = 4; i < w - 4; i++) {
		move(6, i);
		addstr(box_light.s_get(CHAR_HLINE).c_str());
		box_light.drop();
	}

	// draw description
	// we want text wrapping here
	string help;
	for (char c : CMDS_HELP)
		help += string(1, c);

	text_box(5, 8, w - 10, h - 12, help, true, delta);
}

void help_input(const wchar_t& key) {
	switch (key) {
		case KEY_LEFT:
			if (delta > 0) delta--;
			break;
		case KEY_RIGHT:
			delta++;
			break;
		case '=':
			delta = 0;
			break;
	}
}

void safemode_box() {
	box_strong.drop();

	vector<string> mes_lines = { "SAFE MODE", "List won't autosave" };

	if ((errors & ERR_CONF_V) != 0) mes_lines.push_back("Config version mismatch");
	if ((errors & ERR_LIST_V) != 0) mes_lines.push_back("List version mismatch");

	int maxlen = 0;

	for (auto s : mes_lines) if (maxlen < s.length()) maxlen = s.length();

	int x0 = w - maxlen - 3;
	int y0 = 2;

	// draw message box
	draw_border(x0, y0, maxlen + 2, mes_lines.size() + 2, box_strong);
	clear_box(x0 + 1, y0 + 1, maxlen, mes_lines.size());

	for (int i = 0; i < mes_lines.size(); i++) {
		move(y0 + 1 + i, x0 + 1);
		addstr(mes_lines.at(i).c_str());
	}
}

string prompt(const string& message) {
	command = L"";
	curs_set(1);
	command_index = command_history.size();
	command_cursor = command.length();

	for (wint_t c = 0; ; get_wch(&c)) {
		switch (c) {
			case 10:
				return w_converter.to_bytes(command);
				break;
			case 27:
				return "";
				break;
			case 0:
				break;
			default:
				command_input(c);
				break;
		}

		move(h - 1, 0);
		attron_ext(0, color_status);
		for (int x = 0; x < w; x++) addch(' ');
		move(h - 1, 0);
		int offset = command_cursor - w + 3;
		if (offset < 0) offset = 0;
		addstr((message + w_converter.to_bytes(command.substr(offset))).c_str());
		move(h - 1, 1 + command_cursor - offset);
	}

	return "";
}

void filter_history() {
	for (int i = 0; i < command_history.size() - 1; i++)
		if (command_history[i] == L"")  {
			command_history.erase(command_history.begin() + i);
			i--;
		}
}

wchar_t key_from_str(const string& str) {
	if (str.length() == 1)
		return str.at(0);

	if (str == "up")	return KEY_UP;
	if (str == "down")	return KEY_DOWN;
	if (str == "left")	return KEY_LEFT;
	if (str == "right")	return KEY_RIGHT;
	if (str == "esc")	return 27;
	if (str == "enter")	return 10;
	if (str == "tab")	return 9;
	if (str == "home")	return KEY_HOME;
	if (str == "end")	return KEY_END;
	if (str == "backspace")	return KEY_BACKSPACE;
	if (str == "delete")	return KEY_DC;

	if (str.find("code") == 0)
		try {
			return stoi(str.substr(4));
		} catch (const invalid_argument& e) {}

	return 0;
}

int pair_from_str(const string& str) {
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

int draw_table(const int& x, const int& y,
		const int& w, const int& h,
		const std::function<vargs::cols::varg(const int& item)>& colarg_f,
		const std::function<bool(const int& item)>& vis_f,
		const std::function<bool(const int& item)>& cont_f,
		const std::function<attrs(const int& item)>& attrs_f,
		const int& start, int& sel,
		const std::string& down_cmd,
		const std::string& cols, const std::map<char, col_s>& colmap) {
	int t_x = x;
	int t_y = y;

	// draw table title
	move(t_y, t_x);
	attrset_ext(A_STANDOUT | A_BOLD, color_title);
	for (int i = 0; i < w; i++) addch(' ');

	for (int coln = 0; coln < cols.length(); coln++) {
		try {
			const char& col = cols.at(coln);
			if (t_x >= x + w) break;
			const int& cw = colmap.at(col).width(w, x + w - t_x, cols.length());
			text_box(t_x, t_y, cw, 1, colmap.at(col).title);

			if ((coln < cols.length() - 1) && (t_x + cw < x + w)) {
				move(t_y, t_x + cw);
				addstr((" " + separators.s_get(CHAR_ROW_SEP) + " ").c_str());
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
		separators.drop();
		separators.shift_at(CHAR_ROW_SEP, row_separator_offset * (l + 1));
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
				text_box(t_x, t_y, cw, 1, colmap.at(col).contents(colarg_f(v_list.at(l))));

				if ((coln < cols.length() - 1) && (t_x + cw < x + w)) {
					move(t_y, t_x + cw);
					addstr((" " + separators.s_get(CHAR_ROW_SEP) + " ").c_str());
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

void draw_status(const string& fields) {
	string status_l = "";

	for (int i = fields.length() - 1; i >= 0; i--) {
		const char& c = fields.at(i);
		try {
			const string field = (status_fields.at(c))();
			if (field != "") {
				if (status_l != "") status_l = " " + separators.s_get(CHAR_STA_SEP) + " " + status_l;
		       		status_l = field + status_l;
			}
		} catch (const out_of_range& e) {}
	}

	status_l += " ";

	attron_ext(0, color_status);
	move(h - 1, 0);
	for (int x = 0; x < w; x++) addch(' ');
	const auto len = text_box(w - 1 - w_converter.from_bytes(status_l).length(), h - 1, w, 1, status_l, true, 0, true).w;
	text_box(w - 1 - len, h - 1, w, 1, status_l);
	attrset_ext(A_NORMAL);
}

void clear_box(const int& x, const int& y, const int& w, const int& h) {
	for (int j = x; j < x + w; j++)
		for (int i = y; i < y + h; i++) {
			move(i, j);
			addch(' ');
		}
}

void draw_border(const int& x, const int& y, const int& w, const int& h, multistr_c& chars) {
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

box_dim text_box(const int& x, const int& y, const int& w, const int& h, const string& str, const bool& show_md, const int& line_offset, const bool& nullout) {
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
					pair = pair_from_str(w_converter.to_bytes(buffer));
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

		for (const auto& sw : md_switches)
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

}
