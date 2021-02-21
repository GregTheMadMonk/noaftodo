#include "nc.hpp"

#ifdef __sun
#include <ncurses/curses.h>
#else
#include <curses.h>
#endif

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

		if (halfdelay_time == 0) cbreak();
		else halfdelay(halfdelay_time);
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
			if (c > 0) {
				input_event event;
				event.name = keyname(c);
				event.mod_alt = mod_alt;
				mod_alt = false;
				if (c == 27) { // alt+ combination
					mod_alt = true;
				} else if ((c & 0x1f) == c) { // ctrl key performs an AND on keycode and 0x1f
					event.mod_ctrl = true;
					event.key = wc.from_bytes(event.name).at(1);
				} else event.key = c;

				if (event.key != 0) {
					log << event.mod_alt << " - " <<
						event.mod_ctrl << " - " <<
						wc.to_bytes(event.key) << " - " <<
						event.name << lend;
					on_input(event);
				}
			}

			// paint UI
			on_paint();

			if (!running) break;
		}
	}

	int backend_ncurses::width()	{ return getmaxx(stdscr); }
	int backend_ncurses::height()	{ return getmaxy(stdscr); }

	void backend_ncurses::draw_line(const int& x1, const int& y1, const int& x2, const int& y2) {
	}

	void backend_ncurses::draw_box(const int& x1, const int& y1, const int& x2, const int& y2) {
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

	void backend_ncurses::draw_text(const int& x, const int& y, const std::string& text) {
		move(y, x);
		addstr(text.c_str());
	}

	void backend_ncurses::set_fg(const uint32_t& color) {
	}

	void backend_ncurses::set_bg(const uint32_t& color) {
	}

	string backend_ncurses::charset_get(const int& position) {
		return string(charset.begin() + position, charset.begin() + position + 1);
	}

}
