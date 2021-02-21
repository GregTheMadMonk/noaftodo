#pragma once
#ifdef __linux__

#ifndef NOAF_UI_FB_H
#define NOAF_UI_FB_H

#include <string>

#include <ui.hpp>

namespace noaf {

	class backend_framebuffer : public backend {
			int dev;

			int w, h;

			int bytes;

			bool running = false;

			uint32_t* data;
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

			// stuff specific to this backend
			std::string dev_name;

			void blank();	// clear framebuffer
	};

}

#endif

#else
#warning "framebuffer backend is only provided for Linux platform and you don't seem to be running one :("
#endif
