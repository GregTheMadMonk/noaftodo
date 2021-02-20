#include "ui.hpp"

using namespace std;

namespace noaf {

	shared_ptr<backend> ui;

	bool backend::can(const int& req) {
		for (const auto& f : features) if (f == req) return true;
		return false;
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

	function<void()> on_paint = [] () {};

	namespace col {

		uint32_t to_true(const int& col) {
			if (col == -1) return 0;

			uint32_t ret = 0xff << 24; // opaque

			int r = (col % 8) & 0b1;
			int g = (col % 8) & 0b10;
			int b = (col % 8) & 0b100;

			ret |= (r > 0) ? 0xff0000 : ((col >= 8) ? 0x800000 : 0);
			ret |= (g > 0) ? 0xff00 : ((col >= 8) ? 0x8000 : 0);
			ret |= (b > 0) ? 0xff : ((col >= 8) ? 0x80 : 0);
			
			return ret;
		}

		int to_16(const uint32_t& col) {
			if (col & 0xff000000 == 0) return -1;
			int r = ((col & 0xff0000) >> 16);
			int g = ((col & 0xff00) >> 8);
			int b = col & 0xff;

			bool bright = false;;
			int ret = 0;

			if (r >= 64) {
				if (r >= 192) ret += 0b1; // 192 = 128 + 64
				else bright = true;
			}

			if (g >= 64) {
				if (g >= 192) ret += 0b10; // 192 = 128 + 64
				else bright = true;
			}

			if (b >= 64) {
				if (b >= 192) ret += 0b100; // 192 = 128 + 64
				else bright = true;
			}

			if (bright) ret += 8;
			return ret;
		}

	}

}
