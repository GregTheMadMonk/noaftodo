#include "ui.hpp"

using namespace std;

namespace noaf {

	backend* ui;

	void backend::fill() {
		draw_fill = !draw_fill;
	}

	void backend::fill(const bool& val) {
		draw_fill = val;
	}

	void backend::stroke() {
		draw_stroke = !draw_stroke;
	}

	void backend::stroke(const bool& val) {
		draw_stroke = val;
	}

	bool backend::can(const int& req) {
		return features & req;
	}

	int ucvt(const int& val, const char& unit) {
		switch (unit) {
			case 0:
				return val;
			case 'w':
				return (ui->width() - 1) * val  / 100;
			case 'h':
				return (ui->height() - 1) * val / 100;
			default:
				return 0;
		}
	}

	namespace col {

		// TODO: this two functions don't appear to handle white/bright white
		// really well. Fix it!
		uint32_t to_true(const int& col) {
			if (col == -1) return 0;

			const auto extract = [&] (const unsigned& offset) -> uint32_t {
				const int ec = (col % 8) & (1 << offset);

				return ((ec > 0) ? 0xff : ((col >= 8) ? 0x80 : 0)) << ((2 - offset) * 8);
			};

			return (0xff << 24) | extract(0) | extract(1) | extract (2);
		}

		int to_16(const uint32_t& col) {
			if (col & 0xff000000 == 0) return -1;

			bool bright = false;

			const auto extract = [&] (const unsigned& offset) -> int {
				int ec = ((col & (0xff << offset * 8)) >> (offset * 8));

				if (ec >= 64) {
					if (ec >= 192) return (1 << (2 - offset)); // 192 = 128 + 64
					else bright = true;
				}

				return 0;
			};

			return (extract(0) | extract(1) | extract(2)) + (bright ? 8 : 0);
		}

	}

	function<void()> on_paint = [] () {};
	function<void(const input_event&)> on_input = [] (const input_event& e) {};

}
