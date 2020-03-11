#include "noaftodo_cui.h"

#include <codecvt>
#include <curses.h>
#include <locale>

#include "noaftodo.h"
#include "noaftodo_list.h"
#include "noaftodo_output.h"
#include "noaftodo_time.h"

using namespace std;

int cui_filter = 0b1111;

cui_charset_s cui_charset;

int cui_mode;

vector<cui_bind_s> binds;

int cui_w, cui_h;

string cui_status = "Welcome to " + string(TITLE) + "!";

int cui_s_line;
int cui_delta;
int cui_numbuffer = -1;

wstring cui_command;
int cui_command_cursor = 0;

wstring_convert<codecvt_utf8<wchar_t>, wchar_t> w_converter;

void cui_init()
{
	log("Initializing console UI...");
	initscr();
	start_color();
	use_default_colors();
	init_pair(CUI_CP_TITLE_1, COLOR_BLUE, -1);
	init_pair(CUI_CP_TITLE_2, COLOR_GREEN, -1);
	init_pair(CUI_CP_GREEN_ENTRY, COLOR_GREEN, -1);
	init_pair(CUI_CP_YELLOW_ENTRY, COLOR_YELLOW, -1);
	init_pair(CUI_CP_RED_ENTRY, COLOR_RED, -1);

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
				if (bind.autoexec) cui_exec(bind.command);
				else 
				{
					cui_command = w_converter.from_bytes(bind.command);
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
			cui_command_cursor = cui_command.length();
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

void cui_exec(const string& command)
{
	vector<string> words;
	string word = "";
	bool inquotes = false;

	for (int i = 0; i < command.length(); i++)
	{
		const char c = command.at(i);

		switch (c)
		{
			case ' ':
				if (inquotes) word += c;
				else 
					if (word != "")
					{
						words.push_back(word);
						word = "";
					}
				break;
			case ';':
				if (inquotes) word += c;
				else
				{
					if (word != "")
					{
						words.push_back(word);
						word = "";
					}
					words.push_back(";");
				}
				break;
			case '"':
				inquotes = !inquotes;
				break;
			default:
				word += c;
		}
	}
	if (word != "") words.push_back(word);

	// trim words
	for (string& t_word : words)
	{
		while (t_word.at(0) == ' ') t_word = t_word.substr(1);
		while (t_word.at(t_word.length() - 1) == ' ') t_word = t_word.substr(0, t_word.length() - 1);
	}
	
	int offset = 0;
	for (int i = 0; i < words.size(); i++)
	{
		if (words.at(i) == ";") // command separator
			offset = i + 1;
		else if (i == offset)
		{
			if (words.at(i) == "q")
				cui_mode = CUI_MODE_EXIT;
			else if (words.at(i) == ":")
				cui_set_mode(CUI_MODE_COMMAND);
			else if (words.at(i) == "?")
				cui_set_mode(CUI_MODE_HELP);
			else if (words.at(i) == "down")
			{
				if (t_list.size() != 0)
				{
					if (cui_s_line < t_list.size() - 1) cui_s_line++;
					else cui_s_line = 0;

					if (!cui_is_visible(cui_s_line)) cui_exec("down");
				}
			} else if (words.at(i) == "up")
			{
				if (t_list.size() != 0)
				{
					if (cui_s_line > 0) cui_s_line--;
					else cui_s_line = t_list.size() - 1;

					if (!cui_is_visible(cui_s_line)) cui_exec("up");
				}
			} else if (words.at(i) == "c")
			{
				if (t_list.size() == 0)
					cui_status = "There's nothing in the list!";
				else
					li_comp(cui_s_line);
			} else if (words.at(i) == "d")
			{
				if (t_list.size() == 0)
					cui_status = "There's nothing to delete!";
				else
				{
					li_rem(cui_s_line);
					if (t_list.size() != 0) if (cui_s_line >= t_list.size()) cui_exec("up");
				}
			} else if (words.at(i) == "a")
			{
				if (words.size() >= i + 4)
				{
					noaftodo_entry new_entry;
					new_entry.completed = false;
					if (words.at(i + 1) != "")
					{
						new_entry.due = ti_to_long(words.at(i + 1));
						if (words.at(i + 2) != "")
						{
							new_entry.title = words.at(i + 2);
							if (words.at(i + 3) != "")
							{
								new_entry.description = words.at(i + 3);

								li_add(new_entry);
							}
						}
					}
				} else { 
					cui_status = "Not enough arguments for \":a\""; 
				}
			} else if (words.at(i) == "vtoggle")
			{
				if (words.size() >= i + 2)
				{
					if (words.at(i + 1) == "uncat")
						cui_filter ^= CUI_FILTER_UNCAT;
					if (words.at(i + 1) == "complete")
						cui_filter ^= CUI_FILTER_COMPLETE;
					if (words.at(i + 1) == "coming")
						cui_filter ^= CUI_FILTER_COMING;
					if (words.at(i + 1) == "failed")
						cui_filter ^= CUI_FILTER_FAILED;
				} else { 
					cui_status = "Not enough arguments for \":vtoggle\""; 
				}
			} else if (words.at(i) == "g")
			{
				if (words.size() >= i + 2)
				{
					const int target = stoi(words.at(i + 1));

					if ((target >= 0) && (target < t_list.size()))
						cui_s_line = target;
				} else {
					cui_status = "Not enough arguments for \":g\"";
				}
			}
		}
	}
}

bool cui_is_visible(const int& entryID)
{
	if (t_list.size() == 0) return false;
	const auto& entry = t_list.at(entryID);

	if (entry.completed) return (cui_filter & CUI_FILTER_COMPLETE);
	if (entry.due <= ti_to_long("a0d")) return (cui_filter & CUI_FILTER_FAILED);
	if (entry.due <= ti_to_long("a1d")) return (cui_filter & CUI_FILTER_COMING);

	return (cui_filter & CUI_FILTER_UNCAT);
}

void cui_normal_paint()
{
	const int id_chars = 4;
	const int date_chars = 18;
	const int title_chars = cui_w / 5;
	const int desc_chars = cui_w - title_chars - date_chars - id_chars - 3;

	// draw table title
	move(0, 0);
	for (int i = 0; i < cui_w; i++) 
	{
		if (i < id_chars) attrset(A_STANDOUT | A_BOLD | COLOR_PAIR(CUI_CP_TITLE_1));
		else if (i < id_chars + 1 + date_chars) attrset(A_STANDOUT | A_BOLD | COLOR_PAIR(CUI_CP_TITLE_2));
		else if (i < id_chars + 1 + date_chars + 1 + title_chars) attrset(A_STANDOUT | A_BOLD | COLOR_PAIR(CUI_CP_TITLE_1));
		else attrset(A_STANDOUT | A_BOLD | COLOR_PAIR(CUI_CP_TITLE_2));
		addch(' ');
	}

	move(0, 0);
	attrset(A_STANDOUT | A_BOLD | COLOR_PAIR(CUI_CP_TITLE_1));
	addnstr("ID", id_chars - 1);
	move(0, id_chars - 1);
	attrset(A_STANDOUT | A_BOLD | COLOR_PAIR(CUI_CP_TITLE_2));
	addnstr((cui_charset.row_separator + " Task due").c_str(), date_chars);
	move(0, id_chars + date_chars);
	attrset(A_STANDOUT | A_BOLD | COLOR_PAIR(CUI_CP_TITLE_1));
	addnstr((cui_charset.row_separator + " Task title").c_str(), title_chars);
	move(0, id_chars + date_chars + 1 + title_chars);
	attrset(A_STANDOUT | A_BOLD | COLOR_PAIR(CUI_CP_TITLE_2));
	addnstr((cui_charset.row_separator + " Task description").c_str(), desc_chars);
	attrset(A_NORMAL);

	cui_delta = 0;
	if (cui_s_line - cui_delta >= cui_h - 2) cui_delta = cui_s_line - cui_h + 3;
	if (cui_s_line - cui_delta < 0) cui_delta = cui_s_line;

	bool is_there_visible = false;
	for (int l = 0; l < t_list.size(); l++) is_there_visible |= cui_is_visible(l);

	if (is_there_visible) for (int l = 0; l < t_list.size(); l++)
	{
		if (!cui_is_visible(l))
		{
			cui_delta++;
			if (l == cui_s_line) if (l != t_list.size() - 1) cui_exec("down");
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
				addstr(to_string(l).c_str());
				x += id_chars - 1;
				move(l - cui_delta + 1, x);
				addstr((cui_charset.row_separator + " " + ti_f_str(t_list.at(l).due)).c_str());
				x += date_chars + 1;
				move(l - cui_delta + 1, x);
				addstr((cui_charset.row_separator + " " + t_list.at(l).title).c_str());
				x += title_chars + 1;
				move(l - cui_delta + 1, x);
				addstr((cui_charset.row_separator + " " + t_list.at(l).description).c_str());

				attrset(A_NORMAL);
			}
		}
	}


	cui_status = 	string((cui_filter & CUI_FILTER_UNCAT) ? "U" : "_") +
			string((cui_filter & CUI_FILTER_COMPLETE) ? "V" : "_") +
			string((cui_filter & CUI_FILTER_COMING) ? "C" : "_") +
			string((cui_filter & CUI_FILTER_FAILED) ? "F" : "_") +
			((t_list.size() == 0) ? "" : " " + cui_charset.status_separator + " " + to_string(cui_s_line) + "/" + to_string(t_list.size() - 1)) +
			string((cui_status != "") ? (" " + cui_charset.status_separator + " " + cui_status) : "");

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
				cui_exec("g " + to_string(cui_numbuffer));
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
	addstr(w_converter.to_bytes(L":" + cui_command).c_str());
	move(cui_h - 1, 1 + cui_command_cursor);
}

void cui_command_input(const wchar_t& key)
{
	// for some reason, some keys work as if they were get through getch
	// and others - with their KEY_* values
	// Guess, I won't touch it then and let it work as it works.
	switch (key)
	{
		case 10:
			cui_exec(w_converter.to_bytes(cui_command));
		case 27:
			cui_command = L"";
			if (cui_mode == CUI_MODE_COMMAND) cui_set_mode(CUI_MODE_NORMAL);
			break;
		case 127:
			if (cui_command_cursor > 0)
			{
				cui_command = cui_command.substr(0, cui_command_cursor - 1) + cui_command.substr(cui_command_cursor, cui_command.length() - cui_command_cursor);
				cui_command_cursor--;
			}
			break;
		case KEY_DC:
			if (cui_command_cursor < cui_command.length())
				cui_command = cui_command.substr(0, cui_command_cursor) + cui_command.substr(cui_command_cursor + 1, cui_command.length() - cui_command_cursor - 1);
			break;
		case KEY_LEFT:
			if (cui_command_cursor > 0) cui_command_cursor--;
			break;
		case KEY_RIGHT:
			if (cui_command_cursor < cui_command.length()) cui_command_cursor++;
			break;
		default:
			cui_command = cui_command.substr(0, cui_command_cursor) + key + cui_command.substr(cui_command_cursor, cui_command.length() - cui_command_cursor);
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
