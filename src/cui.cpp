#include "cui.h"

#include <regex>

#include <cmd.h>
#include <config.h>
#include <cvar.h>
#include <daemon.h>
#include <entry_flags.h>

using namespace std;

using li::t_list;
using li::t_tags;

using namespace li::entry_flags;

namespace cui {

time_s last_scr_upd;

map<string, mode_s>& modes::modes() {
	static map<string, mode_s> modes_ {};
	return modes_;
}

void modes::init_mode(const string& alias, const mode_s& mode) {
	log("Initializing mode " + alias);
	modes::modes()[alias] = mode;
}

mode_s modes::mode(const string& alias) {
	try {
		return modes::modes().at(alias);
	} catch (const out_of_range& e) {
		return {
			[] () {},
			[&alias] () {
				move(1, 1);
				addstr(("Mode not found: \"" + alias + "\" :(").c_str());
			},
			[] (const keystroke_s& key, const bool& bind_fired) {
				if (key.key == L'q') cmd::exec("exit");
			}
		};
	}
}

void init() {
	log("Initializing console UI...");

	log("Running modes init() functions");

	for (auto it = modes::modes().begin();
			it != modes::modes().end();
			it++)
		it->second.init_f();

	// construct UI
	command_history.push_back(w_converter().from_bytes(""));
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
	for (auto it = entry_flag::flags().begin(); it != entry_flag::flags().end(); it++)
		log("Entry flag! -> " + string(1, it->first));

	for (wint_t c = -1; ; (get_wch(&c) != ERR) ? : (c = 0)) {
		if (li::has_changed()) li::load(false); // load only list contents, not the workspace

		// check for updates in task state
		else if ( !([] () -> bool {
				for (const auto& entry : t_list)
					if ((is_failed(entry) && (entry.due > last_scr_upd)) ||
					(is_coming(entry) && (entry.due > time_s(last_scr_upd.fmt_cmd() + "a" + entry.get_meta("warn_time", "1d")))))
						return true;

				return false;
		})() && (c == 0) && (!shift_multivars) ) continue;

		last_scr_upd = time_s();

		if (c > 0) {
			const int old_numbuffer = numbuffer;
			status = "";

			keystroke_s k = { (wchar_t)c, false };

			if (c == 27) {
				wint_t c2;
				nodelay(stdscr, true);
				if (get_wch(&c2) != ERR)
					k = { (wchar_t)c2, true };
				nodelay(stdscr, false);
			}

			switch (mode) {
				case MODE_NORMAL:
					modes::mode("normal").input(k, fire_bind(k));
					break;
				case MODE_LISTVIEW:
					modes::mode("liview").input(k, fire_bind(k));
					break;
				case MODE_DETAILS:
					modes::mode("details").input(k, fire_bind(k));
					break;
				case MODE_COMMAND:
					modes::mode("command").input(k, fire_bind(k));
					break;
				case MODE_HELP:
					modes::mode("help").input(k, fire_bind(k));
					break;
				case MODE_TIMELINE:
					modes::mode("timeline").input(k, fire_bind(k));
					break;
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
				modes::mode("normal").paint();
				break;
			case MODE_LISTVIEW:
				modes::mode("liview").paint();
				break;
			case MODE_DETAILS:
				modes::mode("details").paint();
				break;
			case MODE_COMMAND:
				modes::mode("command").paint();
				break;
			case MODE_HELP:
				modes::mode("help").paint();
				break;
			case MODE_TIMELINE:
				modes::mode("timeline").paint();
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
	if (mode == 0) return; // exit was requested, we don't want to go back to anything

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

void bind(const keystroke_s& key, const string& command, const int& mode, const bool& autoexec) {
	bind({ key, command, mode, autoexec });
}

bool fire_bind(const keystroke_s& key) {
	bool bind_fired = false;
	for (const auto& bind : binds)
		if ((bind.mode & mode) && (bind.key == key)) {
			if (bind.autoexec) cmd::exec(bind.command);
			else  {
				if ((s_line >= 0) && (s_line < t_list.size()))
					command = w_converter().from_bytes(format_str(bind.command, &t_list.at(s_line)));
				else
					command = w_converter().from_bytes(bind.command);
				set_mode(MODE_COMMAND);
			}

			bind_fired = true;
			break;
		}

	return bind_fired;
}

bool is_visible(const int& entryID) {
	if (t_list.size() == 0) return false;
	if ((entryID < 0) && (entryID >= t_list.size())) return false;

	const auto& entry = t_list.at(entryID);

	bool ret = ((tag_filter == TAG_ALL) || (tag_filter == entry.tag) || (mode == MODE_LISTVIEW));

	if (is_completed(entry))	ret = ret && (filter & FILTER_COMPLETE);
	if (is_failed(entry)) 		ret = ret && (filter & FILTER_FAILED);
	if (is_coming(entry)) 		ret = ret && (filter & FILTER_COMING);

	if (is_nodue(entry))		ret = ret && (filter & FILTER_NODUE);

	if (is_uncat(entry))		ret = ret && (filter & FILTER_UNCAT);

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
		keystroke_s k = { (wchar_t)c, false };

		switch (c) {
			case 10:
				return w_converter().to_bytes(command);
				break;
			case 27:
				wint_t c2;
				nodelay(stdscr, true);
				if (get_wch(&c2) != ERR) {
					k = { (wchar_t)c2, true };
					nodelay(stdscr, false);
				} else {
					nodelay(stdscr, false);
					return "";
				}
			case 0:
				break;
			default:
				modes::mode("command").input(k, fire_bind(k));
				break;
		}

		move(h - 1, 0);
		attron_ext(0, color_status);
		for (int x = 0; x < w; x++) addch(' ');
		move(h - 1, 0);
		int offset = command_cursor - w + 3;
		if (offset < 0) offset = 0;
		addstr((message + w_converter().to_bytes(command.substr(offset))).c_str());
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

keystroke_s key_from_str(string str) {
	int mod = 0; // 0b1 CTRL bit

	bool modfound = true;
	while (modfound) {
		modfound = false;
		if (str.find("c^") == 0) {
			mod |= 0b1;
			modfound = true;
			str = str.substr(2);
		} else if (str.find("a^") == 0) {
			mod |= 0b10;
			modfound = true;
			str = str.substr(2);
		}
	}

	int ret = 0;

	if (str.length() == 1)
		ret = str.at(0);

	if (str == "up")	ret = KEY_UP;
	if (str == "down")	ret = KEY_DOWN;
	if (str == "left")	ret = KEY_LEFT;
	if (str == "right")	ret = KEY_RIGHT;
	if (str == "esc")	ret = 27;
	if (str == "enter")	ret = 10;
	if (str == "tab")	ret = 9;
	if (str == "home")	ret = KEY_HOME;
	if (str == "end")	ret = KEY_END;
	if (str == "backspace")	ret = KEY_BACKSPACE;
	if (str == "delete")	ret = KEY_DC;
	if (str == "pgup")	ret = KEY_PPAGE;
	if (str == "pgdn")	ret = KEY_NPAGE;

	if (str.find("code") == 0)
		try {
			ret = stoi(str.substr(4));
		} catch (const invalid_argument& e) {}

	if ((mod & 0b1) != 0)
		ret &= 0x1f;

	return { (wchar_t)ret, ((mod & 0b10) != 0) };
}

int pair_from_str(const string& str) {
	int text_attrs = A_NORMAL;
	int text_pair;
	attr_get(&text_attrs, &text_pair, NULL);

	short bg = text_pair / 17 - 1;
	short fg = text_pair % 17 - 1;

	bool last = false; // false -> fg, true -> bg

	string buffer = "";

	for (const auto& c : (str + (char)0))
		switch (c) {
			case ';': case 0:
				try {
					const int& val = stoi(buffer);
					buffer = "";
					switch (val / 10) {
						case 3: case -3:
							fg = val % 10;
							last = false;
							break;
						case 4: case -4:
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
	if (v_sel - l_offset >= h - 2) l_offset = v_sel - h + 3;
	if (v_sel - l_offset < 0) l_offset = v_sel;

	if (v_list.size() == 0) {
		sel = -1;
		return t_y + 1;
	}

	// draw list
	for (int l = 0; l < v_list.size(); l++) {
		separators.drop();
		separators.shift_at(CHAR_ROW_SEP, row_separator_offset * (l + 1));
		if (l - l_offset >= h - 1) break;
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
	attrset_ext(A_NORMAL);
	string status_l = "";

	for (int i = fields.length() - 1; i >= 0; i--) {
		const char& c = fields.at(i);
		try {
			const string field = (status_fields.at(c))();
			if (field != "") {
				if (status_l != "") status_l = separators.s_get(CHAR_STA_SEP) + status_l;
		       		status_l = field + status_l;
			}
		} catch (const out_of_range& e) {}
	}

	status_l += " ";

	attron_ext(0, color_status);
	move(h - 1, 0);
	for (int x = 0; x < w; x++) addch(' ');
	const auto len = text_box(w - 1 - w_converter().from_bytes(status_l).length(), h - 1, w, 1, status_l, true, 0, true).w;
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
					pair = pair_from_str(w_converter().to_bytes(buffer));
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
			addstr(w_converter().to_bytes(wch).c_str());
		}

		if (t_x - x > rw) rw = t_x - x;
		if (t_y - y > rh) rh = t_y - y;

		t_x++;
	};

	wstring wstr = w_converter().from_bytes(str);
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
