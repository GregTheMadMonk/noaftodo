#pragma once
#ifndef NOAF_UI_NC_H
#define NOAF_UI_NC_H

#include <codecvt>
#include <locale>
#include <string>

#include <ui.hpp>

namespace noaf {

	class backend_ncurses : public backend {
			bool initialized = false;
			bool running = false;

			int fg = -1;		// color code or -1 for default
			int bg = -1;		// -- // --

			bool mod_alt = false;	// alt+ combination incoming

			std::wstring_convert<std::codecvt_utf8<wchar_t>, wchar_t> wc;

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
			std::wstring charset = L"|-++++";
			std::string charset_get(const int& position);
	};

}

#endif
