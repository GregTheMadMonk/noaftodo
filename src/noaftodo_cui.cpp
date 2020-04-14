#include "noaftodo_cui.h"

#include <codecvt>
#include <curses.h>
#include <locale>

#include "noaftodo.h"
#include "noaftodo_cmd.h"
#include "noaftodo_config.h"
#include "noaftodo_list.h"
#include "noaftodo_output.h"
#include "noaftodo_time.h"

using namespace std;

int cui_mode;

vector<cui_bind_s> binds;

int cui_w, cui_h;

string cui_status = "Welcome to " + string(TITLE) + "!";

int cui_s_line;
int cui_delta;
int cui_numbuffer = -1;

vector<wstring> cui_commands;
int cui_command_cursor = 0;
int cui_command_index = 0;

wstring_convert<codecvt_utf8<wchar_t>, wchar_t> w_converter;

void cui_init()
{
	log("Initializing console UI...");
	cui_commands.push_back(w_converter.from_bytes(""));

	initscr();
	start_color();
	use_default_colors();
	init_pair(CUI_CP_TITLE, conf_get_cvar_int("colors.title"), conf_get_cvar_int("colors.background"));
	init_pair(CUI_CP_GREEN_ENTRY, conf_get_cvar_int("colors.entry_completed"), conf_get_cvar_int("colors.background"));
	init_pair(CUI_CP_YELLOW_ENTRY, conf_get_cvar_int("colors.entry_coming"), conf_get_cvar_int("colors.background"));
	init_pair(CUI_CP_RED_ENTRY, conf_get_cvar_int("colors.entry_failed"), conf_get_cvar_int("colors.background"));

	cbreak();
	set_escdelay(0);
	curs_set(0);
	noecho();
	keypad(stdscr, true);

	cui_w = getmaxx(stdscr);
	cui_h = getmaxy(stdscr);
}

void cui_destroy()
{
	endwin();
}

void cui_run()
{
	cui_init();
	cui_set_mode(CUI_MODE_NORMAL);

	for (wint_t c = 0; ; get_wch(&c))
	{
		bool bind_fired = false;
		for (const auto& bind : binds)
			if ((bind.mode & cui_mode) && (bind.key == c))
			{
				if (bind.autoexec) cmd_exec(bind.command);
				else 
				{
					cui_commands[cui_commands.size() - 1] = w_converter.from_bytes(bind.command);
					cui_set_mode(CUI_MODE_COMMAND);
				}

				bind_fired = true;
			}

		if (bind_fired) cui_numbuffer = -1;

		if (!bind_fired) switch (cui_mode)
		{
			case CUI_MODE_NORMAL:
				cui_normal_input(c);
				break;
			case CUI_MODE_COMMAND:
				cui_command_input(c);
				break;
			case CUI_MODE_HELP:
				cui_help_input(c);
		}

		cui_w = getmaxx(stdscr);
		cui_h = getmaxy(stdscr);

		clear();

		switch (cui_mode)
		{
			case CUI_MODE_NORMAL:
				cui_normal_paint();
				break;
			case CUI_MODE_COMMAND:
				cui_command_paint();
				break;
			case CUI_MODE_HELP:
				cui_help_paint();
				break;
		}

		if (cui_mode == CUI_MODE_EXIT) break;
	}

	li_save();

	cui_destroy();
}

void cui_set_mode(const int& mode)
{
	cui_mode = mode;

	switch (mode)
	{
		case CUI_MODE_NORMAL:
			curs_set(0);
			break;
		case CUI_MODE_COMMAND:
			curs_set(1);
			cui_command_index = cui_commands.size() - 1;
			cui_command_cursor = cui_commands[cui_commands.size() - 1].length();
			break;
		case CUI_MODE_HELP:
			curs_set(0);
			break;
	}
}

void cui_bind(const cui_bind_s& bind)
{
	binds.push_back(bind);
}

void cui_bind(const wchar_t& key, const string& command, const int& mode, const bool& autoexec)
{
	cui_bind({ key, command, mode, autoexec });
}

bool cui_is_visible(const int& entryID)
{
	if (t_list.size() == 0) return false;
	const auto& entry = t_list.at(entryID);

	const int tag_filter = conf_get_cvar_int("tag_filter");
	const int filter = conf_get_cvar_int("filter");
	const bool tag_check = ((tag_filter == CUI_TAG_ALL) || (tag_filter == entry.tag));

	if (entry.completed) return tag_check && (filter & CUI_FILTER_COMPLETE);
	if (entry.due <= ti_to_long("a0d")) return tag_check && (filter & CUI_FILTER_FAILED);
	if (entry.due <= ti_to_long("a1d")) return tag_check && (filter & CUI_FILTER_COMING);

	return tag_check && (filter & CUI_FILTER_UNCAT);
}

void cui_normal_paint()
{
	const int tag_filter = conf_get_cvar_int("tag_filter");
	const int filter = conf_get_cvar_int("filter");
	const int id_chars = (tag_filter == CUI_TAG_ALL) ? 20 : 4;
	const int date_chars = 18;
	const int title_chars = cui_w / 5;
	const int desc_chars = cui_w - title_chars - date_chars - id_chars - 3;

	// draw table title
	move(0, 0);
	for (int i = 0; i < cui_w; i++) 
	{
		if (i < id_chars) attrset(A_STANDOUT | A_BOLD | COLOR_PAIR(CUI_CP_TITLE));
		else if (i < id_chars + 1 + date_chars) attrset(A_STANDOUT | A_BOLD | COLOR_PAIR(CUI_CP_TITLE));
		else if (i < id_chars + 1 + date_chars + 1 + title_chars) attrset(A_STANDOUT | A_BOLD | COLOR_PAIR(CUI_CP_TITLE));
		else attrset(A_STANDOUT | A_BOLD | COLOR_PAIR(CUI_CP_TITLE));
		addch(' ');
	}

	move(0, 0);
	attrset(A_STANDOUT | A_BOLD | COLOR_PAIR(CUI_CP_TITLE));
	addnstr((tag_filter == CUI_TAG_ALL) ? "ID: List" : "ID", id_chars - 1);
	move(0, id_chars - 1);
	attrset(A_STANDOUT | A_BOLD | COLOR_PAIR(CUI_CP_TITLE));
	addnstr((conf_get_cvar("charset.row_separator") + " Task due").c_str(), date_chars);
	move(0, id_chars + date_chars);
	attrset(A_STANDOUT | A_BOLD | COLOR_PAIR(CUI_CP_TITLE));
	addnstr((conf_get_cvar("charset.row_separator") + " Task title").c_str(), title_chars);
	move(0, id_chars + date_chars + 1 + title_chars);
	attrset(A_STANDOUT | A_BOLD | COLOR_PAIR(CUI_CP_TITLE));
	addnstr((conf_get_cvar("charset.row_separator") + " Task description").c_str(), desc_chars);
	attrset(A_NORMAL);

	cui_delta = 0;
	if (cui_s_line - cui_delta >= cui_h - 2) cui_delta = cui_s_line - cui_h + 3;
	if (cui_s_line - cui_delta < 0) cui_delta = cui_s_line;

	bool is_there_visible = false;
	for (int l = 0; l < t_list.size(); l++) is_there_visible |= cui_is_visible(l);

	int last_string = 0;
	if (is_there_visible) for (int l = 0; l < t_list.size(); l++)
	{
		if (!cui_is_visible(l))
		{
			cui_delta++;
			if (l == cui_s_line) if (l != t_list.size() - 1) cmd_exec("down");
		} else {
			if (l - cui_delta >= cui_h - 2) break;
			if (l >= cui_delta)    
			{
				if (l == cui_s_line) attron(A_STANDOUT);
				if (t_list.at(l).completed) attron(COLOR_PAIR(CUI_CP_GREEN_ENTRY) | A_BOLD);	// a completed entry
				else if (t_list.at(l).due <= ti_to_long("a0d")) attron(COLOR_PAIR(CUI_CP_RED_ENTRY) | A_BOLD);	// a failed entry
				else if (t_list.at(l).due <= ti_to_long("a1d")) attron(COLOR_PAIR(CUI_CP_YELLOW_ENTRY) | A_BOLD);	// an upcoming entry

				int x = 0;
				move(l - cui_delta + 1, x);
				for (int i = 0; i < cui_w; i++) addch(' ');
				move(l - cui_delta + 1, x);
				string id_string = to_string(l);
				if (tag_filter == CUI_TAG_ALL)
				{
					id_string += ": " + to_string(t_list.at(l).tag);
					if (t_list.at(l).tag < t_tags.size())
						if (t_tags.at(t_list.at(l).tag) != to_string(t_list.at(l).tag))
							id_string += "(" + t_tags.at(t_list.at(l).tag) + ")";
				}
				addnstr(id_string.c_str(), id_chars - 2);
				x += id_chars - 1;
				move(l - cui_delta + 1, x);
				addnstr((conf_get_cvar("charset.row_separator") + " " + ti_f_str(t_list.at(l).due)).c_str(), date_chars + 2);
				x += date_chars + 1;
				move(l - cui_delta + 1, x);
				addnstr((conf_get_cvar("charset.row_separator") + " " + t_list.at(l).title).c_str(), title_chars + 2);
				x += title_chars + 1;
				move(l - cui_delta + 1, x);
				wstring next = w_converter.from_bytes(conf_get_cvar("charset.row_separator") + " " + t_list.at(l).description);
				for (int i = 0; i < next.length(); i++)
				{
					addstr(w_converter.to_bytes(next.at(i)).c_str());
					if (i != 0) if (i % (cui_w - x - 2) == 0) if (i != next.length() - 1)
					{
						cui_delta--;
						x = 0;
						move(l - cui_delta + 1, x);
						for (int i = 0; i < cui_w; i++) addch(' ');
						x += id_chars - 1;
						move(l - cui_delta + 1, x);
						addnstr((conf_get_cvar("charset.row_separator") + " ").c_str(), date_chars + 2);
						x += date_chars + 1;
						move(l - cui_delta + 1, x);
						addnstr((conf_get_cvar("charset.row_separator") + " ").c_str(), title_chars + 2);
						x += title_chars + 1;
						move(l - cui_delta + 1, x);
						addstr((conf_get_cvar("charset.row_separator") + " ").c_str());
					}
				}

				attrset(A_NORMAL);
				last_string = l - cui_delta + 2;
			}
		}
	}

	for (int s = last_string; s < cui_h; s++) { move(s, 0); clrtoeol(); }

	cui_status = 	((tag_filter == CUI_TAG_ALL) ?
				"All lists" :
				("List " + to_string(tag_filter) + (((tag_filter < t_tags.size()) && (t_tags.at(tag_filter) != to_string(tag_filter))) ? (": " + t_tags.at(tag_filter)) : ""))) +
			" " + conf_get_cvar("charset.status_separator") + " " +
			string((filter & CUI_FILTER_UNCAT) ? "U" : "_") +
			string((filter & CUI_FILTER_COMPLETE) ? "V" : "_") +
			string((filter & CUI_FILTER_COMING) ? "C" : "_") +
			string((filter & CUI_FILTER_FAILED) ? "F" : "_") +
			((t_list.size() == 0) ? "" : " " + conf_get_cvar("charset.status_separator") + " " + to_string(cui_s_line) + "/" + to_string(t_list.size() - 1)) +
			string((cui_status != "") ? (" " + conf_get_cvar("charset.status_separator") + " " + cui_status) : "");

	move(cui_h - 1, cui_w - 1 - cui_status.length());
	addstr(cui_status.c_str());
	cui_status = "";
}

void cui_normal_input(const wchar_t& key)
{
	switch (key)
	{
		case '0': case '1': case '2': case '3': case '4':
		case '5': case '6': case '7': case '8': case '9':
			if (cui_numbuffer == -1) cui_numbuffer = 0;
			cui_numbuffer = cui_numbuffer * 10 + (key - '0');
			cui_status = to_string(cui_numbuffer);
			break;
		case 'g':
			if (cui_numbuffer == -1) 
			{
				cui_numbuffer = 0;
				cui_status = 'g';
			} else {
				cmd_exec("g " + to_string(cui_numbuffer));
				cui_numbuffer = -1;
			}
			break;
		case 'l':
			if (cui_numbuffer == -1)
			{
				cui_numbuffer = -2;
				cui_status = 'l';
			} else {
				if (cui_numbuffer == -2) cmd_exec("list all");
				else cmd_exec("list " + to_string(cui_numbuffer));
				cui_numbuffer = -1;
			}
			break;
		case 'G':
			cui_numbuffer = t_list.size() - 1;
			cui_status = 'G';
			break;
		default:
			cui_numbuffer = -1;
			break;
	}
}

void cui_command_paint()
{
	cui_normal_paint();

	move(cui_h - 1, 0);
	clrtoeol();
	move(cui_h - 1, 0);
	int offset = cui_command_cursor - cui_w + 3;
	if (offset < 0) offset = 0;
	addstr(w_converter.to_bytes(L":" + cui_commands[cui_commands.size() - 1].substr(offset)).c_str());
	move(cui_h - 1, 1 + cui_command_cursor - offset);
}

void cui_command_input(const wchar_t& key)
{
	// for some reason, some keys work as if they were get through getch
	// and others - with their KEY_* values
	// Guess, I won't touch it then and let it work as it works.
	switch (key)
	{
		case 10:
			if (cui_commands[cui_commands.size() - 1] != w_converter.from_bytes(""))
			{
				cmd_exec(w_converter.to_bytes(cui_commands[cui_commands.size() - 1]));
				if (cui_command_index != cui_commands.size() - 1)
				{
					wstring temp = cui_commands[cui_commands.size() - 1];
					cui_commands[cui_command_index] = cui_commands[cui_commands.size() - 1];
					cui_commands[cui_commands.size() - 1] = temp;
				}

				cui_commands.push_back(w_converter.from_bytes(""));
				cui_command_index = cui_commands.size() - 1;
			}
		case 27:
			cui_commands[cui_commands.size() - 1] = L"";
			if (cui_mode == CUI_MODE_COMMAND) cui_set_mode(CUI_MODE_NORMAL);
			cui_filter_history();
			break;
		case 127:
			if (cui_command_index != cui_commands.size() - 1)
			{
				wstring temp = cui_commands[cui_commands.size() - 1];
				cui_commands[cui_commands.size() - 1] = cui_commands[cui_command_index];
				cui_commands[cui_command_index] = temp;
				cui_commands.push_back(temp);
				cui_command_index = cui_commands.size() - 1;
			}

			if (cui_command_cursor > 0)
			{
				cui_commands[cui_commands.size() - 1] = cui_commands[cui_commands.size() - 1].substr(0, cui_command_cursor - 1) + cui_commands[cui_commands.size() - 1].substr(cui_command_cursor, cui_commands[cui_commands.size() - 1].length() - cui_command_cursor);
				cui_command_cursor--;
			}
			break;
		case KEY_DC:
			if (cui_command_index != cui_commands.size() - 1)
			{
				wstring temp = cui_commands[cui_commands.size() - 1];
				cui_commands[cui_commands.size() - 1] = cui_commands[cui_command_index];
				cui_commands[cui_command_index] = temp;
				cui_commands.push_back(temp);
				cui_command_index = cui_commands.size() - 1;
			}

			if (cui_command_cursor < cui_commands[cui_commands.size() - 1].length())
				cui_commands[cui_commands.size() - 1] = cui_commands[cui_commands.size() - 1].substr(0, cui_command_cursor) + cui_commands[cui_commands.size() - 1].substr(cui_command_cursor + 1, cui_commands[cui_commands.size() - 1].length() - cui_command_cursor - 1);
			break;
		case KEY_LEFT:
			if (cui_command_cursor > 0) cui_command_cursor--;
			break;
		case KEY_RIGHT:
			if (cui_command_cursor < cui_commands[cui_commands.size() - 1].length()) cui_command_cursor++;
			break;
		case KEY_UP:
			// go up the history
			if (cui_command_index > 0)
			{
				wstring temp = cui_commands[cui_command_index];
				cui_commands[cui_command_index] = cui_commands[cui_commands.size() - 1];
				cui_commands[cui_commands.size() - 1] = cui_commands[cui_command_index - 1];
				cui_commands[cui_command_index - 1] = temp;
				cui_command_index--;
				if (cui_command_cursor > cui_commands[cui_commands.size() - 1].length())
					cui_command_cursor = cui_commands[cui_commands.size() - 1].length();
			}
			break;
		case KEY_DOWN:
			// go down the history
			if (cui_command_index < cui_commands.size() - 1)
			{
				wstring temp = cui_commands[cui_command_index];
				cui_commands[cui_command_index] = cui_commands[cui_commands.size() - 1];
				cui_commands[cui_commands.size() - 1] = cui_commands[cui_command_index + 1];
				cui_commands[cui_command_index + 1] = temp;
				cui_command_index++;
				if (cui_command_cursor > cui_commands[cui_commands.size() - 1].length())
					cui_command_cursor = cui_commands[cui_commands.size() - 1].length();
			}	
			break;
		case KEY_HOME:
			cui_command_cursor = 0;
			break;
		case KEY_END:
			cui_command_cursor = cui_commands[cui_commands.size() - 1].length();
			break;
		default:
			if (cui_command_index != cui_commands.size() - 1)
			{
				wstring temp = cui_commands[cui_commands.size() - 1];
				cui_commands[cui_commands.size() - 1] = cui_commands[cui_command_index];
				cui_commands[cui_command_index] = temp;
				cui_commands.push_back(temp);
				cui_command_index = cui_commands.size() - 1;
			}

			cui_filter_history();

			cui_commands[cui_commands.size() - 1] = cui_commands[cui_commands.size() - 1].substr(0, cui_command_cursor) + key + cui_commands[cui_commands.size() - 1].substr(cui_command_cursor, cui_commands[cui_commands.size() - 1].length() - cui_command_cursor);
			cui_command_cursor++;
	}
}

void cui_help_paint()
{
	move(1, 1);
	addstr((string(TITLE) + " v." + VERSION + " help.").c_str());
	move(3, 1);
	addstr("Command mode (activates with \':\' key) commands:\n");
	addstr("\t:q - exit program\n");
	addstr("\t:? - show this message\n");
	addstr("\t:g <number> - got to entry with ID <number>\n");
	addstr("\t:up, :down - navigate up and down in the list\n");
	addstr("\t:a <due> <title> <description> - add entry to list. Due format: \"<year>y<month>m<day>d<hour>h<minute>\". If \"a\" is added before the due string, time is counted from current moment.\n");
	addstr("\t:c - toggle selected entry completion\n");
	addstr("\t:d - remove selected entry\n");
	addstr("\t:vtoggle uncat|completed|coming|failed - toggle visibility for entry category (\"uncat\" = uncategorized)\n");

	move(cui_h - 1, 1);
	addstr("Press 'q' or <esc> to quit.");
}

void cui_help_input(const wchar_t& key)
{
	switch (key)
	{
		case 'q': case 27:
			cui_set_mode(CUI_MODE_NORMAL);
			break;
	}
}

void cui_filter_history()
{
	for (int i = 0; i < cui_commands.size() - 1; i++)
		if (cui_commands[i] == w_converter.from_bytes("")) 
		{
			cui_commands.erase(cui_commands.begin() + i);
			i--;
		}
}
