#include <cui.h>
#include <cvar.h>

NOAFTODO_START_MODE(liview, init, paint, input)

using li::t_list;
using li::t_tags;

void init() {
	// @cvar "livi.regex_filter" - LISTVIEW mode list regex filter
	cvar_base_s::cvars["livi.regex_filter"] = cvar_base_s::wrap_string(cui::listview_regex_filter);
	// @cvar "livi.cols" - LISTVIEW mode columns
	cvar_base_s::cvars["livi.cols"] = cvar_base_s::wrap_string(cui::listview_cols);
	// @cvar "livi.status_fields" - status fields to display in LISTVIEW mode
	cvar_base_s::cvars["livi.status_fields"] = cvar_base_s::wrap_string(cui::listview_status_fields);
}

void paint() {
	const int last_string = draw_table(0, 0, w, h - 1,
			[] (const int& item) {
				return vargs::cols::varg(vargs::cols::lview { item });
			},
			l_is_visible,
			[] (const int& item) {
				return ((item >= -1) && (item < (int)t_tags.size()));
			},
			[] (const int& item) -> attrs {
				if (li::tag_completed(item))	return { A_BOLD, color_complete };
				if (li::tag_due(item))		return { A_BOLD, color_due };
				if (li::tag_failed(item))	return { A_BOLD, color_failed };
				if (li::tag_coming(item))	return { A_BOLD, color_coming };

				return { A_NORMAL, 0 };
			},
			-1, tag_filter,
			"math %tag_filter_v% + 1 tag_filter_v",
			listview_cols, lview_columns);

	for (int s = last_string; s < h; s++) { move(s, 0); clrtoeol(); }

	draw_status(listview_status_fields);
}

void input(const keystroke_s& key, const bool& bind_fired) {}

NOAFTODO_END_MODE
