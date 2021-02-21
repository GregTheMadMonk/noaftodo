#pragma once
#ifdef __linux__

#ifndef NOAF_UI_FB_H
#define NOAF_UI_FB_H

#include <string>

#include <ui.hpp>

namespace noaf {

	class backend_framebuffer : public backend {
			int dev = -1;

			int w, h;

			int bytes;

			bool running = false;

			uint32_t fg = 0xffffffff;
			uint32_t bg = 0xff000000;

			uint32_t* data = nullptr;
		public:
			backend_framebuffer();

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

			// stuff specific to this backend
			std::string dev_name;

			int halfdelay_time = 0;

			void blank();	// clear framebuffer
	};

}

#endif

#else
#warning "framebuffer backend is only provided for Linux platform and you don't seem to be running one :("
#endif
