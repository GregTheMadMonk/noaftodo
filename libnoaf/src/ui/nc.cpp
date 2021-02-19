#include "nc.hpp"

#ifdef __sun
#include <ncurses/curses.h>
#else
#include <curses.h>
#endif

#include <log.hpp>

using namespace std;

namespace noaf {

	backend_ncurses::backend_ncurses() :
		active(false),
		halfdelay_time(0),
		charset(L"|-++++") {
	}

	void backend_ncurses::init() {
		initscr();
		start_color();
		use_default_colors();

		if (halfdelay_time == 0) cbreak();
		else halfdelay(halfdelay_time);
		set_escdelay(0);
		curs_set(0);
		noecho();
		keypad(stdscr, true);

		active = true;
	}

	void backend_ncurses::resume() {
		if (!active) init();
	}

	void backend_ncurses::pause() {
		if (active) kill();
	}

	void backend_ncurses::kill() {
		endwin();
		active = false;
	}

	void backend_ncurses::run() {
		for (wint_t c = -1; ; (get_wch(&c) != ERR) ? : (c = 0)) {
			on_paint();
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

	string backend_ncurses::charset_get(const int& position) {
		return string(charset.begin() + position, charset.begin() + position + 1);
	}

}
