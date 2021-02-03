#include <cui.h>

NOAFTODO_START_MODE(details, init, paint, input)

using namespace std;

using li::t_list;
using li::t_tags;

void init() {
	// @cvar "det.cols" - "columns" (or better fields) to display as entry information in DETAILS mode
	cvar_base_s::cvars["det.cols"] = cvar_base_s::wrap_string(cui::details_cols);
	// @cvar "charset.separators.details" - separator between DETAILS fields
	cvar_base_s::cvars["charset.separators.details"] = cvar_base_s::wrap_multistr_element(cui::separators, cui::CHAR_DET_SEP);
}

void paint() {
	modes::mode("normal").paint();

	draw_border(3, 2, w - 6, h - 4, box_strong);
	clear_box(4, 3, w - 8, h - 6);
	// fill the box with details
	// Title
	const li::entry& entry = t_list.at(s_line);
	text_box(5, 4, w - 10, 1, entry.title);

	for (int i = 4; i < w - 4; i++) {
		move(6, i);
		addstr(box_light.s_get(CHAR_HLINE).c_str());
		box_light.drop();
	}

	string tag = "";
	if (entry.tag < t_tags.size()) if (t_tags.at(entry.tag) != to_string(entry.tag))
		tag = ": " + t_tags.at(entry.tag);

	string info_str = "";
	for (int coln = 0; coln < details_cols.length(); coln++) {
		try {
			const char& col = details_cols.at(coln);

			info_str += columns.at(col).contents(vargs::cols::normal { entry, s_line });
			if (coln < details_cols.length() - 1) info_str += " " + separators.s_get(CHAR_DET_SEP) + " ";
		} catch (const out_of_range& e) {}
	}

	text_box(5, 7, w - 10, 1, info_str);

	for (int i = 4; i < w - 4; i++) {
		move(8, i);
		addstr(box_light.s_get(CHAR_HLINE).c_str());
		box_light.drop();
	}

	// draw description
	// we want text wrapping here
	text_box(5, 10, w - 10, h - 14, entry.description, true, delta);
}

void input(const keystroke_s& key, const bool& bind_fired) {
	mode("help").input(key, bind_fired);
}

NOAFTODO_END_MODE
