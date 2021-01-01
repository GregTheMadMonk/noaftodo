#include "../noaftodo_cui.h"
#include "../noaftodo_cvar.h"
#include "../noaftodo_entry_flags.h"
#include "../noaftodo_time.h"

using namespace std;
using namespace li::entry_flags;

NOAFTODO_START_MODE(timeline, init, paint, input)

using li::t_list;

int position = 0;

int unit = 60; // minutes per timeline unit (basically, scale)

std::string line	= "-";
std::string vline	= "|";
std::string marker	= "^";
std::string rarrow	= "~~~>";

void init() {
	cvar_base_s::cvars["timeline.position"] = cvar_base_s::wrap_int(position);
	cvar_base_s::cvars["timeline.unit"] = cvar_base_s::wrap_int(unit);
	cvar_base_s::cvars["charset.timeline.marker"] = cvar_base_s::wrap_string(marker);
	cvar_base_s::cvars["charset.timeline.line"] = cvar_base_s::wrap_string(line);
	cvar_base_s::cvars["charset.timeline.vline"] = cvar_base_s::wrap_string(vline);
	cvar_base_s::cvars["charset.timeline.rarrow"] = cvar_base_s::wrap_string(rarrow);
}

void paint() {
	clear();

	// draw the line
	move(h - 4, 0);
	for (int x = 0; x < w - 1; x++) addstr(line.c_str());
	move(h - 4, w - w_converter.from_bytes(rarrow).length());
	addstr(rarrow.c_str());

	const int rounder = time_s().to_tm().tm_min % unit;

	const int cpu = 12;	// characters per unit
	const int uoffset = 1;	// unit painting offset

	const auto draw_vline_at = [&cpu, &uoffset, &rounder] (const int& offset) {
		// draws a vertical unit line
		for (int y = 0; y < h - 1; y++) {
			move(y, (uoffset + offset) * cpu);
			addstr(vline.c_str());
		}

		// mark the line
		const auto t = time_s("a" + to_string((position + offset) * unit - rounder));
		move(h - 3, (uoffset + offset) * cpu + 1);
		addnstr(t.fmt_sprintf("%1$04d/%2$02d/%3$02d").c_str(), w - (uoffset + offset) * cpu - 1);
		move(h - 2, (uoffset + offset) * cpu + 1);
		addnstr(t.fmt_sprintf("%4$02d:%5$02d").c_str(), w - (uoffset + offset) * cpu - 1);
	};

	const auto time_coord = [] (const time_s& time) {
		const time_t zero = (time_s().time / 60) * cpu / unit - (-position + uoffset) * cpu;
		const time_t position = (time.time / 60) * cpu / unit;

		return position - zero;
	};

	// draw vertical unit lines
	for (int offset = 0; (uoffset - offset >= 0) || ((uoffset + offset) * cpu < w); offset++) {
		if (uoffset - offset >= 0)		draw_vline_at(-offset);
		if ((uoffset + offset) * cpu < w)	draw_vline_at(offset);
	}

	// draw tasks
	int y1 = h - 5;
	for (int i = 0; i < t_list.size(); i++) if (is_visible(i)) {
		int x = time_coord(t_list.at(i).due);
		int x1 = time_coord(time_s(t_list.at(i).due.fmt_cmd()
					+ "a" + t_list.at(i).get_meta("duration", li::task_duration_default)));
		if ((x >= w) || (x1 < 0)) continue;
		if (x < 0) x = 0;
		if (x1 >= w) x1 = w - 1;

		const auto& e = t_list.at(i);

		const int attr = (i == s_line) ? A_STANDOUT : A_NORMAL;
		if (is_completed(e)) attrset_ext(attr | A_BOLD, color_complete);
		else if (is_failed(e)) attrset_ext(attr | A_BOLD, color_failed);
		else if (is_coming(e)) attrset_ext(attr | A_BOLD, color_coming);
		else attrset_ext(attr);

		draw_border(x, y1 - 2, x1 - x, 3, box_strong);
		clear_box(x + 1, y1 - 1, x1 - x - 2, 1);
		if (x1 - x > 2) {
			move(y1 - 1, x + 1);
			addnstr((t_list.at(i).title + " - " + t_list.at(i).description).c_str(), x1 - x - 2);
		}
		y1 -= 3;
	}

	draw_status(normal_status_fields);
}

void input(const keystroke_s& key, const bool& bind_fired) {}

NOAFTODO_END_MODE
