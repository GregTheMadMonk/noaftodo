#include "../noaftodo_cui.h"

using li::t_list;

namespace cui::modes::NORMAL {

void paint() {
	const int last_string =
		draw_table(0, 0, w, h - 1,
				[] (const int& item) {
					return vargs::cols::varg(vargs::cols::normal {
							t_list.at(item),
							item
							});
				},
				is_visible,
				[] (const int& item) {
					return ((item >= 0) && (item < t_list.size()));
				},
				[] (const int& item) -> attrs {
					const auto& e = t_list.at(item);

					if (e.completed) return { A_BOLD, color_complete };
					if (e.is_failed()) return { A_BOLD, color_failed };
					if (e.is_coming()) return { A_BOLD, color_coming };

					return { A_NORMAL, 0 };
				},
				0, s_line,
				"math %id% + 1 id",
				(tag_filter == TAG_ALL) ? normal_all_cols : normal_cols,
				columns);


	for (int s = last_string; s < h; s++) { move(s, 0); clrtoeol(); }

	draw_status(normal_status_fields);
}

std::nullptr_t creator = ([] () { init_mode("normal", {
		paint,
		[] (const keystroke_s& key, const bool& bind_fired) {}
		});
		
		return nullptr;
		})();

}