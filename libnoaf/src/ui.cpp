#include "ui.hpp"

using namespace std;

namespace noaf {

	shared_ptr<backend> ui;

	bool backend::can(const feature& req) {
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

}
