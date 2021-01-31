#include <noaftodo_cui.h>

using namespace std;

extern string CMDS_HELP;

NOAFTODO_START_MODE(help, init, paint, input)

using li::t_list;

void init() {}

void paint() {
	modes::mode("normal").paint();

	draw_border(3, 2, w - 6, h - 4, box_strong);
	clear_box(4, 3, w - 8, h - 6);

	// fill the box
	move(4, 5);
	addstr((string(TITLE) + " v." + VERSION).c_str());

	for (int i = 4; i < w - 4; i++) {
		move(6, i);
		addstr(box_light.s_get(CHAR_HLINE).c_str());
		box_light.drop();
	}

	// draw description
	// we want text wrapping here
	string help;
	for (char c : CMDS_HELP)
		help += string(1, c);

	text_box(5, 8, w - 10, h - 12, help, true, delta);
}

void input(const keystroke_s& key, const bool& bind_fired) {
	if (bind_fired) return;
	switch (key.key) {
		case KEY_LEFT:
			if (delta > 0) delta--;
			break;
		case KEY_RIGHT:
			delta++;
			break;
		case '=':
			delta = 0;
			break;
	}
}

NOAFTODO_END_MODE
