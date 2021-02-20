#include <iostream>

#include <cmd.hpp>
#include <conarg.hpp>
#include <log.hpp>
#include <ui/fb.hpp>
#include <ui/nc.hpp>

using namespace std;
using namespace noaf;

int main(int argc, char* argv[]) {
	setlocale(LC_ALL, "");

	log << lver(0);

	bool fb = false;

	conarg::args()[{ "-h", "--help"}] = {
		[] (const vector<string>& params) {
			log << llev(2) << "Help message!!!" << lend;
		},
		0,
		""
	};
#ifdef __linux__
	conarg::args()[{ "-f" }] = {
		[&] (const vector<string>& params) { fb = true; },
		0,
		""
	};
#endif

	conarg::parse(argc, argv);

#ifdef __linux__
	if (fb) {
		ui = make_shared<backend_framebuffer>();

		ui_as<backend_framebuffer>()->dev_name = "/dev/fb0";
	} else {
#endif
		ui = make_shared<backend_ncurses>();

		ui_as<backend_ncurses>()->charset = L"|-/\\\\/";
		ui_as<backend_ncurses>()->halfdelay_time = 1;
#ifdef __linux__
	}
#endif

	ui->init();

	int frame = 0;
	on_paint = [&] () {
		frame++;
		ui->draw_text(1, 1, to_string(frame));
		ui->draw_box(ucvt(50, 'w'), ucvt(50, 'h'), ucvt(100, 'w') - 2, ucvt(100, 'h') - 2);
		ui->draw_box(10, 10, 20, 20);
		ui->draw_box(ucvt(25, 'w'), ucvt(25, 'h'), ucvt(75, 'w'), ucvt(75, 'h'));
	};

	ui->run();

	ui->kill();

	return 0;

	string s;
	while (getline(cin, s)) {
		cmd::exec(s);
	}
	return 0;
}
