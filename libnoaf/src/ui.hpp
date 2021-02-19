#pragma once
#ifndef NOAF_UI_H
#define NOAF_UI_H

#include <memory>
#include <vector>

namespace noaf::ui {

	class backend {
		enum feature {
			color_table,
			color,
			opacity,
			images
		};
		std::vector<feature> features = {}; // all backends can handle primitives/text, other is optional
		public:
			// backend hooks
			// basic functionality
			virtual void init() = 0;
			virtual void pause() = 0;
			virtual void resume() = 0;
			virtual void kill() = 0;

			virtual int width() = 0;
			virtual int height() = 0;

			virtual void draw_line(const int& x1, const int& y1, const int& x2, const int& y2) = 0;
			virtual void draw_box(const int& x1, const int& y1, const int& x2, const int& y2) = 0;

			// color management (color table)
			// TODO
			virtual void set_color(const int& index) {}
			// color management (256-color)
			// TODO
			virtual void set_color(const int& r, const int& g, const int& b) {}
			// opacity management
			// TODO
			virtual void set_opacity(const float& a) {}
			// image drawing
			// TODO

			// feature request
			bool can(const feature& req);
	};

	extern std::unique_ptr<backend> ui;

	int ucvt(const int& val, const char& unit = 0);

}

#endif
