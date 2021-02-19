#include <iostream>

#include <cmd.hpp>
#include <conarg.hpp>
#include <log.hpp>
#include <ui/nc.hpp>

#include <curses.h>

using namespace std;
using namespace noaf;

int main(int argc, char* argv[]) {
	setlocale(LC_ALL, "");

	log << lver(0);

	ui = make_unique<backend_ncurses>();

	ui->init();

	for (wint_t c = -1; ; (get_wch(&c) != ERR) ? : (c = 0)) {
		ui->draw_box(ucvt(50, 'w'), ucvt(50, 'h'), ucvt(100, 'w') - 2, ucvt(100, 'h') - 2);
		ui->draw_box(10, 10, 20, 20);
	}

	ui->kill();

	return 0;

	conarg::args()[{ "-h", "--help"}] = {
		[] (const vector<string>& params) {
			log << llev(2) << "Help message!!!" << lend;
		},
		0,
		""
	};

	conarg::parse(argc, argv);

	string s;
	while (getline(cin, s)) {
		cmd::exec(s);
	}
	return 0;
}
