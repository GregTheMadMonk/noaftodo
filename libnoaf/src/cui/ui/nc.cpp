#include "nc.hpp"

#ifdef __sun
#include <ncurses/curses.h>
#else
#include <curses.h>
#endif

#include <cstdlib>
#include <cstring>

#include <log.hpp>

using namespace std;

namespace noaf {

	backend_ncurses::backend_ncurses() {
		features = MARKDOWN | COLOR;

	}

	void backend_ncurses::init() {
		log << "Initializing the \"ncurses\" backend..." << lend;
		resume();
	}

	void backend_ncurses::resume() {
		if (initialized) return;
		initscr();
		start_color();
		use_default_colors();

		// initialize color pairs
		if (COLOR_PAIRS < 17 * 17) for (int f = 0; f < 8; f++) for (int b = 0; b < 8; b++)
			init_pair(f + b * 8 + 1, f, b);
		else for (int f = -1; f < 16; f++) for (int b = -1; b < 16; b++)
			init_pair(f + 1 + (b + 1) * 17 + 1, f % 8, b % 8);

		if (frame_time == 0) cbreak();
		else halfdelay(frame_time / 100);
		set_escdelay(0);
		curs_set(0);
		noecho();
		keypad(stdscr, true);

		initialized = true;
	}

	void backend_ncurses::pause() {
		if (!initialized) return;
		endwin();
		initialized = false;
	}

	void backend_ncurses::kill() {
		running = false; // will cause run() to end
		pause();
	}

	void backend_ncurses::run() {
		running = true;
		for (wint_t c = 0; running; (get_wch(&c) != ERR) ? : (c = 0)) {
			// process input
			if (c != 0) on_input(nc_process_input(c));

			if (!running) break;

			// paint UI
			on_paint();
		}
	}

	int backend_ncurses::width()	{ return getmaxx(stdscr); }
	int backend_ncurses::height()	{ return getmaxy(stdscr); }

	void backend_ncurses::clear() {
			::clear();
	}

	void backend_ncurses::draw_line(const int& x1, const int& y1, const int& x2, const int& y2) {
		set_attrs();
	}

	void backend_ncurses::draw_box(const int& x1, const int& y1, const int& x2, const int& y2) {
		set_attrs();
		if (draw_stroke) {
			// draw corners
			move(y1, x1);
			addstr(charset_get(2).c_str());
			move(y1, x2);
			addstr(charset_get(3).c_str());
			move(y2, x1);
			addstr(charset_get(4).c_str());
			move(y2, x2);
			addstr(charset_get(5).c_str());

			// draw vertical lines
			for (int i = y1 + 1; i < y2; i++) {
				move(i, x1);
				addstr(charset_get(0).c_str());
				move(i, x2);
				addstr(charset_get(0).c_str());
			}

			// draw horizontal lines
			for (int i = x1 + 1; i < x2; i++) {
				move(y1, i);
				addstr(charset_get(1).c_str());
				move(y2, i);
				addstr(charset_get(1).c_str());
			}
		}

		if (draw_fill) for (int x = x1 + 1; x < x2; x++) for (int y = y1 + 1; y < y2; y++) {
			move(y, x);
			addstr(" ");
		}
	}

	void backend_ncurses::draw_text(const int& x, const int& y, const std::string& text) {
		set_attrs();
		move(y, x);
		addstr(text.c_str());
	}

	void backend_ncurses::set_fg(const uint32_t& color) {
		fg = col::to_16(color);
	}

	void backend_ncurses::set_bg(const uint32_t& color) {
		bg = col::to_16(color);
	}

	void backend_ncurses::set_attrs() {
		if (COLOR_PAIRS < 17 * 17) {
			attr_set(A_NORMAL,
					((fg == -1) ? 7 : (fg % 8)) +
					((bg == -1) ? 7 : (bg % 8)) * 8 + 1, NULL);
		} else attr_set(A_NORMAL, fg + 1 + (bg + 1) * 17 + 1, NULL);
	}

	string backend_ncurses::charset_get(const int& position) {
		return wc.to_bytes(charset.at(position));
	}

	map<string, string> nc_keyname_lookup = {
		{ "KEY_LEFT",		"left" },
		{ "KEY_RIGHT",		"right" },
		{ "KEY_UP",		"up" },
		{ "KEY_DOWN",		"down" },
		{ "kLFT3",		"a^left" },
		{ "kRIT3",		"a^right" },
		{ "kUP3",		"a^up" },
		{ "kDN3",		"a^down" },
		{ "kLFT5",		"c^left" },
		{ "kRIT5",		"c^right" },
		{ "kUP5",		"c^up" },
		{ "kDN5",		"c^down" },
		{ "kLFT7",		"a^c^left" },
		{ "kRIT7",		"a^c^right" },
		{ "kUP7",		"a^c^up" },
		{ "kDN7",		"a^c^down" },
		{ "KEY_SLEFT",		"sleft" },
		{ "KEY_SRIGHT",		"sright" },
		{ "KEY_SR",		"sup" }, // not much how about you ahahaahaahhhhaahaahahahahaha
		{ "KEY_SF",		"sdown" },
		{ "KEY_BACKSPACE",	"backspace" },
		{ "KEY_DC",		"delete" },
		{ "^J",			"enter" },
		{ "^I",			"tab" },
		{ "KEY_HOME",		"home" },
		{ "KEY_END",		"end" },
		{ "KEY_PPAGE",		"pgup" },
		{ "KEY_NPAGE",		"pgdown" },
	};

	input_event backend_ncurses::nc_process_input(wint_t c) {
		input_event event;
		bool mod_alt = false;
		bool mod_ctrl = false;
		if (c == 27) { // alt+ combination
			mod_alt = true;
			nodelay(stdscr, true);
			if (get_wch(&c) == ERR) {
				event.key = 27;
				event.name = "esc";
				mod_alt = false;
				mod_ctrl = false;
			} else mod_alt = true;
			nodelay(stdscr, false);
		}

		if (event.key == 0) {
			if (keyname(c) != nullptr) event.name = keyname(c);
			else event.name = wc.to_bytes(c);

			if ((c & 0x1f) == c) { // ctrl key performs an AND on keycode and 0x1f
				mod_ctrl = true;
				event.key = wc.from_bytes(event.name).at(1);
			} else event.key = c;
		}

		if (event.key != 0) {
			if (nc_keyname_lookup.find(event.name) != nc_keyname_lookup.end())
				event.name = nc_keyname_lookup.at(event.name);
			else {
				if (mod_ctrl)	event.name = tolower(event.name.at(1));
				if (mod_ctrl)	event.name = "c^" + event.name;
				if (mod_alt)	event.name = "a^" + event.name;
			}

			return event;
		}

		return input_event();
	}

	extern "C" backend* backend_create(const int& ac, char**& av) {
		return new backend_ncurses();
	}

}
