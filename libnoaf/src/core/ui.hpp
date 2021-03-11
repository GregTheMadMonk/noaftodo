#pragma once
#ifndef NOAF_UI_H
#define NOAF_UI_H

#include <functional>
#include <string>
#include <vector>

namespace noaf {

	class backend {
		protected:
		static constexpr int MARKDOWN		= 0b1;
		static constexpr int COLOR		= 0b10;
		static constexpr int OPACITY		= 0b100;
		static constexpr int IMAGES		= 0b1000;
		int features = 0; // all backends can handle primitives/text, other is optional

		// what to draw
		bool draw_fill	= false;
		bool draw_stroke= true;
		public:
			int frame_time = 300; // time one frame takes

			// backend hooks
			// basic functionality
			virtual void init() = 0;
			virtual void pause() = 0;
			virtual void resume() = 0;
			virtual void kill() = 0;

			virtual void run() = 0; // backend life cycle

			virtual int width() = 0;
			virtual int height() = 0;

			virtual void clear() = 0;

			virtual void draw_line(const int& x1, const int& y1, const int& x2, const int& y2) = 0;
			virtual void draw_box(const int& x1, const int& y1, const int& x2, const int& y2) = 0;
			virtual void draw_rnd_box(const int& x1, const int& y1, const int& x2, const int& y2, const int& radius);
			virtual void draw_text(const int& x, const int& y, const std::string& text) = 0;

			// switch drawing fill and stroke
			virtual void fill();			// toggle
			virtual void fill(const bool& val);	// set
			virtual void stroke();			// toggle
			virtual void stroke(const bool& val);	// set

			// color management
			virtual void set_fg(const uint32_t& color = 0xffffffff) {}
			virtual void set_bg(const uint32_t& color = 0) {}

			// image drawing
			// TODO

			// feature request
			bool can(const int& req);
	};

	extern backend* ui;

	int ucvt(const int& val, const char& unit = 0);

	// cast colors from different palettes
	namespace col {
		uint32_t to_true(const int& col);
		int to_16(const uint32_t& col);
	}

	// UI input event structure
	struct input_event {
		static constexpr int KEY= 0;	// keyboard event
		int type = KEY;
		wchar_t key = 0;	// keycode
		std::string name = "";	// key name on a physical keyboard
	};

	// callbacks
	extern std::function<void()> on_paint;
	extern std::function<void(const input_event& event)> on_input;

	typedef backend* (*backend_creator)(const int& argc, char**& argv);

}

#endif