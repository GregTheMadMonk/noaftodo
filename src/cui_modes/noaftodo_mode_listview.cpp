#include "../noaftodo_cui.h"

using li::t_list;
using li::t_tags;

namespace cui::modes::LISTVIEW {

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
				if (li::tag_completed(item)) return { A_BOLD, color_complete };
				if (li::tag_failed(item)) return { A_BOLD, color_failed };
				if (li::tag_coming(item)) return { A_BOLD, color_coming };

				return { A_NORMAL, 0 };
			},
			-1, tag_filter,
			"math %tag_filter% + 1 tag_fitler",
			listview_cols, lview_columns);

	for (int s = last_string; s < h; s++) { move(s, 0); clrtoeol(); }

	draw_status(listview_status_fields);
}

std::nullptr_t creator = ([] () { init_mode("liview", {
		paint,
		[] (const keystroke_s& key, const bool& bind_fired) {}
		});
		
		return nullptr;
		})();

}
