#include <csignal>

#include <conarg.hpp>
#include <hooks.hpp>
#include <log.hpp>
#include <ui/fb.hpp>
#include <ui/nc.hpp>

using namespace std;
using namespace noaf;

int main(int argc, char* argv[]) {
	setlocale(LC_ALL, "");

	signal(SIGINT, noaf::exit);
	signal(SIGTERM, noaf::exit);

#ifdef __linux__

	bool fb = false;

	conarg::args()[{ "-f" }] = {
		[&] (const vector<string>& params) { fb = true; },
		0,
		""
	};

	conarg::parse(argc, argv);

	if (fb) {
		ui = make_shared<backend_framebuffer>();

		ui_as<backend_framebuffer>()->dev_name = "fb0";
		ui_as<backend_framebuffer>()->halfdelay_time = 3;
	} else
#endif
	{
		ui = make_shared<backend_ncurses>();

		ui_as<backend_ncurses>()->charset = L"|-/\\\\/";
		ui_as<backend_ncurses>()->halfdelay_time = 3;
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
		ui->draw_text(1, 1, "**Frame:** " + to_string(frame) + " : " + to_string(frame % 16));
		ui->set_fg(0xff00ff00);
		ui->draw_text(1, 2, "**Last input: **" + input_name);
		ui->set_fg(0xffffffff);
		ui->set_bg(0xffffcc00);
		ui->draw_box(ucvt(50, 'w'), ucvt(50, 'h'), ucvt(100, 'w') - 2, ucvt(100, 'h') - 2);
		ui->set_bg(col::to_true(frame % 16));
		ui->draw_box(10, 10, 20, 20);
		ui->set_bg(0xffff00cc);
		ui->draw_box(ucvt(25, 'w'), ucvt(25, 'h'), ucvt(75, 'w'), ucvt(75, 'h'));
	};

	on_input = [&] (const input_event& e) {
		input_name = e.name;
		if (e.key == 'q') noaf::exit();
	};

	noaf::on_exit = [] (const int& code) {
		ui->kill();
	};

	ui->run();

	ui->kill();

	return 0;
}
