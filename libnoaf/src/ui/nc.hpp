#pragma once
#ifndef NOAF_UI_NC_H
#define NOAF_UI_NC_H

#include <codecvt>
#include <locale>
#include <map>
#include <string>

#include <ui.hpp>

namespace noaf {

	class backend_ncurses : public backend {
			bool initialized = false;
			bool running = false;

			int fg = -1;		// color code or -1 for default
			int bg = -1;		// -- // --

			void set_attrs(); // set ncurses attributes according to backend state
		public:
			backend_ncurses();

			void init();
			void pause();
			void resume();
			void kill();

			void run();

			int width();
			int height();

			void clear();

			void draw_line(const int& x1, const int& y1, const int& x2, const int& y2);
			void draw_box(const int& x1, const int& y1, const int& x2, const int& y2);
			void draw_text(const int& x, const int& y, const std::string& text);

			void set_fg(const uint32_t& color);
			void set_bg(const uint32_t& color);

			int halfdelay_time;
			std::wstring charset = L"|-++++";
			std::string charset_get(const int& position);
	};

	extern std::map<std::string, std::string> nc_keyname_lookup;
	input_event nc_process_input(wint_t c);

}

#endif
