#include "../noaftodo_cui.h"

#include <regex>

#include "../noaftodo_cmd.h"

using namespace std;

using li::t_list;

namespace cui::modes::COMMAND {

void paint() {
	switch (prev_modes.top()) {
		case MODE_NORMAL:
			modes::mode("normal").paint();
			break;
		case MODE_LISTVIEW:
			modes::mode("liview").paint();
			break;
		case MODE_DETAILS:
			modes::mode("details").paint();
			break;
		case MODE_HELP:
			modes::mode("help").paint();
			break;
	}

	move(h - 1, 0);
	attron_ext(0, color_status);
	for (int x = 0; x < w; x++) addch(' ');
	move(h - 1, 0);
	int offset = command_cursor - w + 3;
	if (offset < 0) offset = 0;
	addstr(w_converter.to_bytes(L":" + command.substr(offset)).c_str());
	move(h - 1, 1 + command_cursor - offset);
}

void input(const keystroke_s& key, const bool& bind_fired) {
	if (!bind_fired) {
		command_index = command_history.size();

		command = command.substr(0, command_cursor) + key.key + command.substr(command_cursor, command.length() - command_cursor);
		command_cursor++;
	}

	// continious command execution
	if (contexec_regex_filter != "") {
		regex ce_regex(contexec_regex_filter);

		if (regex_search(w_converter.to_bytes(command), ce_regex))
			cmd::exec(w_converter.to_bytes(command));
	}
}

nullptr_t creator = ([] () { init_mode("command", {
		paint,
		input
		});
		
		return nullptr;
		})();

}
