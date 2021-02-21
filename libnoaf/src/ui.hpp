#pragma once
#ifndef NOAF_UI_H
#define NOAF_UI_H

#include <functional>
#include <memory>
#include <vector>

namespace noaf {

	class backend {
		protected:
		static constexpr int MARKDOWN		= 0b1;
		static constexpr int COLOR		= 0b10;
		static constexpr int TRUECOLOR		= 0b100;
		static constexpr int OPACITY		= 0b1000;
		static constexpr int IMAGES		= 0b10000;
		int features = 0; // all backends can handle primitives/text, other is optional

		// what to draw
		bool draw_fill	= false;
		bool draw_stroke= false;
		public:
			// backend hooks
			// basic functionality
			virtual void init() = 0;
			virtual void pause() = 0;
			virtual void resume() = 0;
			virtual void kill() = 0;

			virtual void run() = 0; // backend life cycle

			virtual int width() = 0;
			virtual int height() = 0;

			virtual void draw_line(const int& x1, const int& y1, const int& x2, const int& y2) = 0;
			virtual void draw_box(const int& x1, const int& y1, const int& x2, const int& y2) = 0;
			virtual void draw_text(const int& x, const int& y, const std::string& text) = 0;

			// switch drawing fill and stroke
			virtual void fill();			// toggle
			virtual void fill(const bool& val);	// set
			virtual void stroke();			// toggle
			virtual void stroke(const bool& val);	// set

			// color management
			virtual void set_fg(const uint32_t& color) {}
			virtual void set_bg(const uint32_t& color) {}

			// image drawing
			// TODO

			// feature request
			bool can(const int& req);
	};

	extern std::shared_ptr<backend> ui;
	template <class b> std::shared_ptr<b> ui_as() {
		return std::dynamic_pointer_cast<b>(ui);
	}

	int ucvt(const int& val, const char& unit = 0);

	extern std::function<void()> on_paint;

	// cast colors from different palettes
	namespace col {
		uint32_t to_true(const int& col);
		int to_16(const uint32_t& col);
	}

}

#endif
