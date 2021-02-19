#include "ui.hpp"

using namespace std;

namespace noaf {

	unique_ptr<backend> ui;

	bool backend::can(const feature& req) {
		for (const auto& f : features) if (f == req) return true;
		return false;
	}

	int ucvt(const int& val, const char& unit) {
		switch (unit) {
			case 0:
				return val;
			case 'w':
				return ui->width() * val  / 100;
			case 'h':
				return ui->height() * val / 100;
			default:
				return 0;
		}
	}

}
