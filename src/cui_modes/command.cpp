#include <regex>

#include <cui.h>
#include <cmd.h>
#include <cvar.h>

NOAFTODO_START_MODE(command, init, paint, input)

using namespace std;

using li::t_list;

void init() {
	using namespace cmd;
	// command mode cvars
	// @cvar "cmd.contexec" - regex filter for commands to be executed continiously. It means that if the command in the input matches the filter, it will be executed without user action. Useful for filtering commands or everywhere else where it is needed to see the "preview" of what the user is typing.
	cvar_base_s::cvars["cmd.contexec"] = cvar_base_s::wrap_string(cui::contexec_regex_filter);

	// command mode commands
	// @command "cmd.curs.left" - move command line curor to the left
	cmd::cmds["cmd.curs.left"] = [] (const vector<string>& args) {
		if (cui::command_cursor > 0) cui::command_cursor--;
		return 0;
	};

	// @command "cmd.curs.right" - move command line cursor to the right
	cmd::cmds["cmd.curs.right"] = [] (const vector<string>& args) {
		if (cui::command_cursor < cui::command.length()) cui::command_cursor++;
		return 0;
	};

	// @command "cmd.curs.home" - move command line cursor to the beginning of the line
	cmd::cmds["cmd.curs.home"] = [] (const vector<string>& args) {
		cui::command_cursor = 0;
		return 0;
	};

	// @command "cmd.curs.end" - move command line cursor to the end of the line
	cmd::cmds["cmd.curs.end"] = [] (const vector<string>& args) {
		cui::command_cursor = cui::command.length();
		return 0;
	};

	// @command "cmd.curs.word.home" - move command line cursor to the beginning of the word
	cmd::cmds["cmd.curs.word.home"] = [] (const vector<string>& args) {
		while (cui::command_cursor > 0) {
			cui::command_cursor--;
			const auto c = cui::command.at(cui::command_cursor);
			if (cui::wordbreakers.find(c) != wstring::npos) break;
		}

		return 0;
	};

	// @command "cmd.curs.word.end" - move command line cursor to the end of the word
	cmd::cmds["cmd.curs.word.end"] = [] (const vector<string>& args) {
		while (cui::command_cursor < cui::command.length()) {
			cui::command_cursor++;
			if (cui::command_cursor >= cui::command.length()) break;
			const auto c = cui::command.at(cui::command_cursor);
			if (cui::wordbreakers.find(c) != wstring::npos) break;
		}

		return 0;
	};

	// @command "cmd.history.up" - go one command up (earlier) in command history
	cmd::cmds["cmd.history.up"] = [] (const vector<string>& args) {
		if (cui::command_index > 0) {
			if (cui::command_index == cui::command_history.size())
				cui::command_t = cui::command;

			cui::command_index--;
			cui::command = cui::command_history[cui::command_index];
		}

		if (cui::command_cursor > cui::command.length())
			cui::command_cursor = cui::command.length();

		return 0;
	};

	// @command "cmd.history.down" - go one command down (later) in command history
	cmd::cmds["cmd.history.down"] = [] (const vector<string>& args) {
		if (cui::command_index < cui::command_history.size() - 1) {
			cui::command_index++;
			cui::command = cui::command_history[cui::command_index];
		} else if (cui::command_index == cui::command_history.size() - 1) {
			cui::command_index++;
			cui::command = cui::command_t;
		}

		if (cui::command_cursor > cui::command.length())
			cui::command_cursor = cui::command.length();

		return 0;
	};

	// @command "cmd.backspace" - removes character before command line cursor
	cmd::cmds["cmd.backspace"] = [] (const vector<string>& args) {
		cui::command_index = cui::command_history.size();

		if (cui::command_cursor > 0) {
			cui::command = cui::command.substr(0, cui::command_cursor - 1) +
				cui::command.substr(cui::command_cursor);

			cui::command_cursor--;
		}

		return 0;
	};

	// @command "cmd.delete" - removes the character under command line cursor
	cmd::cmds["cmd.delete"] = [] (const vector<string>& args) {
		cui::command_index = cui::command_history.size();

		if (cui::command_cursor < cui::command.length())
			cui::command = cui::command.substr(0, cui::command_cursor) +
				cui::command.substr(cui::command_cursor + 1);

		return 0;
	};

	// @command "cmd.send" - sends input from command line for execution
	cmd::cmds["cmd.send"] = [] (const vector<string>& args) {
		if (cui::command != L"") {
			exec(w_converter().to_bytes(cui::command));
			cui::command_history.push_back(cui::command);
			cui::command_index = cui::command_history.size();
			cmd::terminate();
		}

		return 0;
	};

	// @command "cmd.clear" - clears command line
	cmd::cmds["cmd.clear"] = [] (const vector<string>& args) {
		cui::command = L"";
		cui::filter_history();

		return 0;
	};
}

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
	addstr(w_converter().to_bytes(L":" + command.substr(offset)).c_str());
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

		if (regex_search(w_converter().to_bytes(command), ce_regex))
			cmd::exec(w_converter().to_bytes(command));
	}
}

NOAFTODO_END_MODE
