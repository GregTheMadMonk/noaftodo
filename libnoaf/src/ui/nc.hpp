#pragma once
#ifndef NOAF_UI_NC_H
#define NOAF_UI_NC_H

#include <string>

#include <ui.hpp>

namespace noaf {

	class backend_ncurses : public backend {
			bool initialized;
			bool running;

			int fg;	// color code or -1 for default
			int bg;	// -- // --

			void set_attrs();
		public:
			backend_ncurses();

			void init();
			void pause();
			void resume();
			void kill();

			void run();

			int width();
			int height();

			void draw_line(const int& x1, const int& y1, const int& x2, const int& y2);
			void draw_box(const int& x1, const int& y1, const int& x2, const int& y2);
			void draw_text(const int& x, const int& y, const std::string& text);

			void set_fg(const uint32_t& color);
			void set_bg(const uint32_t& color);

			int halfdelay_time;
			std::wstring charset;
			std::string charset_get(const int& position);
	};

}

#endif
