#include "noaftodo_cmd.h"

#include <curses.h>

#include "noaftodo_config.h"
#include "noaftodo_cui.h"
#include "noaftodo_output.h"
#include "noaftodo_time.h"

using namespace std;

int cmd_exec(const string& command)
{
	vector<string> words;
	string word = "";
	bool inquotes = false;
	bool skip_special = false;

	for (int i = 0; i < command.length(); i++)
	{
		const char c = command.at(i);

		if (skip_special) 
		{
			word += c;
			skip_special = false;
		} else switch (c) {
			case '\\':
				skip_special = true;
				break;
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
			else if (words.at(i) == "list")
			{
				if (words.size() >= i + 2)
				{
					if (words.at(i + 1) == "all") conf_set_cvar_int("tag_filter", CUI_TAG_ALL);
					else
					{
						const int new_filter = stoi(words.at(1));
						const int tag_filter = conf_get_cvar_int("tag_filter");
						if (new_filter == tag_filter) conf_set_cvar_int("tag_filter", CUI_TAG_ALL);
						else conf_set_cvar_int("tag_filter", new_filter);
					}
				} else return 1;
			} else if (words.at(i) == "down")
			{
				if (t_list.size() != 0)
				{
					if (cui_s_line < t_list.size() - 1) cui_s_line++;
					else cui_s_line = 0;

					if (!cui_is_visible(cui_s_line)) cmd_exec("down");
				} else return 2;
			} else if (words.at(i) == "up")
			{
				if (t_list.size() != 0)
				{
					if (cui_s_line > 0) cui_s_line--;
					else cui_s_line = t_list.size() - 1;

					if (!cui_is_visible(cui_s_line)) cmd_exec("up");
				} else return 2;
			} else if (words.at(i) == "c")
			{
				if (t_list.size() == 0)
					return 2;
				else
					li_comp(cui_s_line);
			} else if (words.at(i) == "d")
			{
				if (t_list.size() == 0)
					return 2;
				else
				{
					li_rem(cui_s_line);
					if (t_list.size() != 0) if (cui_s_line >= t_list.size()) cmd_exec("up");
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
								int tag_filter = conf_get_cvar_int("tag_filter");
								new_entry.tag = (tag_filter == CUI_TAG_ALL) ? 0 : tag_filter;

								li_add(new_entry);
							}
						}
					}
				} else return 1;
			} else if (words.at(i) == "vtoggle")
			{
				if (words.size() >= i + 2)
				{
					int filter = conf_get_cvar_int("filter");
					if (words.at(i + 1) == "uncat")
						filter ^= CUI_FILTER_UNCAT;
					if (words.at(i + 1) == "complete")
						filter ^= CUI_FILTER_COMPLETE;
					if (words.at(i + 1) == "coming")
						filter ^= CUI_FILTER_COMING;
					if (words.at(i + 1) == "failed")
						filter ^= CUI_FILTER_FAILED;
					conf_set_cvar_int("filter", filter);
				} else return 1;
			} else if (words.at(i) == "g")
			{
				if (words.size() >= i + 2)
				{
					const int target = stoi(words.at(i + 1));

					if ((target >= 0) && (target < t_list.size()))
						cui_s_line = target;
				} else return 1;
			} else if (words.at(i) == "lrename")
			{
				if (words.size() >= i + 2)
				{
					const int tag_filter = conf_get_cvar_int("tag_filter");
					if (tag_filter == CUI_TAG_ALL) cui_status = "No specific list selected";
					else 
					{
						while (tag_filter >= t_tags.size()) t_tags.push_back(to_string(t_tags.size()));

						t_tags[tag_filter] = words.at(i + 1);
						li_save();
					}
				} else return 1;
			}
		       	else if (words.at(i) == "lmv")
			{
				if (words.size() >= i + 2)
				{
					t_list[cui_s_line].tag = stoi(words.at(i + 1));
					li_save();
				} else return 1;
			}
			else if (words.at(i) == "get")
			{
				if (words.size() >= i + 2)
				{
					cui_status = conf_get_cvar(words.at(i + 1));
				} else return 1;
			} else if (words.at(i) == "bind")
			{
				if (words.size() == i + 5)
				{
					const string skey = words.at(i + 1);
					const string scomm = words.at(i + 2);
					const int smode = stoi(words.at(i + 3));
					const bool sauto = (words.at(i + 4) == "true");

					log("Binding " + skey + " to '" + scomm + "'");
					if (skey.length() == 1)
						cui_bind(skey.at(0), scomm, smode, sauto);
					else
					{
						if (skey == "up")
							cui_bind(KEY_UP, scomm, smode, sauto);
						if (skey == "down")
							cui_bind(KEY_DOWN, scomm, smode, sauto);
						if (skey == "left")
							cui_bind(KEY_LEFT, scomm, smode, sauto);
						if (skey == "right")
							cui_bind(KEY_RIGHT, scomm, smode, sauto);
						if (skey == "esc")
							cui_bind(27, scomm, smode, sauto);
					}
				} else return 1;
			} else if (words.at(i) == "set")
			{
				if (words.size() == i + 3)
				{
					conf_set_cvar(words.at(i + 1), words.at(i + 2));
				} else return 1;
			} else if (words.at(i) == "reset")
			{
				if (words.size() == i + 2)
				{
					if (conf_get_predefined_cvar(words.at(i + 1)) != "")
						conf_set_cvar(words.at(i + 1), conf_get_predefined_cvar(words.at(i + 1)));
					else
						conf_cvars.erase(words.at(i + 1));
				}
			}
		}
	}

	return 0;
}
