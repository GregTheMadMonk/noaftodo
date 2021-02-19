#pragma once
#ifndef NOAF_UI_NC_H
#define NOAF_UI_NC_H

#include <string>

#include <ui.hpp>

namespace noaf {

	class backend_ncurses : public backend {
			bool initialized;
			bool running;
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

			// stuff specific to this backend
			int halfdelay_time;
			std::wstring charset;
			std::string charset_get(const int& position);
	};

}

#endif
