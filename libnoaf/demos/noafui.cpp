#include <dlfcn.h>
#include <csignal>

#include <conarg.hpp>
#include <hooks.hpp>
#include <log.hpp>
#include <ui/qt.hpp>
#include <ui/nc.hpp>

using namespace std;
using namespace noaf;

int main(int argc, char* argv[]) {
	setlocale(LC_ALL, "");

	signal(SIGINT, noaf::exit);
	signal(SIGTERM, noaf::exit);

	bool qt = false;

	conarg::args()[{ "--qt" }] = {
		[&] (const vector<string>& params) { qt = true; },
		0,
		""
	};

	conarg::parse(argc, argv);

	void* handle = nullptr;

	if (qt) {
		handle = dlopen("libnoafgui.so", RTLD_LAZY);
	} else {
		handle = dlopen("libnoafcui.so", RTLD_LAZY);
	}

	backend_creator creator = (backend_creator) dlsym(handle, "backend_create");
	ui = creator(argc, argv);

	if (qt) {
	} else {
		((backend_ncurses*)ui)->charset = L"|-/\\\\/";
	}

	ui->init();
	ui->fill(true);

	int frame = 0;

	std::string input_name = "";

	on_paint = [&] () {
		frame++;
		ui->clear();
		ui->set_fg(0xffff0000);
		ui->set_bg(0xffffffff);
		ui->draw_text(1, ucvt(50, 'h'), "**Frame:** " + to_string(frame) + " : " + to_string(frame % 16));
		ui->set_fg(0xff00ff00);
		ui->draw_text(1, ucvt(50, 'h') + 1, "**Last input: **" + input_name);
		ui->set_fg(0xffffffff);
		ui->set_bg(0xffffcc00);
		ui->draw_box(ucvt(50, 'w'), ucvt(50, 'h'), ucvt(100, 'w') - 2, ucvt(100, 'h') - 2);
		ui->set_bg(col::to_true(frame % 16));
		ui->draw_box(10, 10, 20, 20);
		ui->set_bg(0xaaff00cc);
		ui->draw_box(ucvt(25, 'w'), ucvt(25, 'h'), ucvt(75, 'w'), ucvt(75, 'h'));
	};

	on_input = [&] (const input_event& e) {
		input_name = e.name;
		if (e.key == 'q') noaf::exit();
	};

	noaf::on_exit = [&] (const int& code) {
		ui->kill();
	};

	ui->run();

	delete ui;
	dlclose(handle);

	return 0;
}
