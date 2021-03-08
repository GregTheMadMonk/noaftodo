#pragma once
#ifndef NOAF_UI_QT_H
#define NOAF_UI_QT_H

#include <memory>

#include <ui.hpp>

class QApplication;

namespace noaf {

	class QNOAFWindow;

	class backend_qt : public backend {
			QApplication* app = nullptr;
			QNOAFWindow* win = nullptr;

			int argc;
			char** argv;
			#ifdef __linux__
			int k_mode = -1;
			#endif
		public:
			#ifdef __linux__
			bool linuxfb;
			#endif

			backend_qt(const int& ac, char**& av);
			~backend_qt();

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
	};

}

#endif
