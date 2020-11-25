#include "../noaftodo_cui.h"
#include "../noaftodo_cvar.h"
#include "../noaftodo_time.h"

using namespace std;

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

	// draw vertical unit lines
	for (int offset = 0; (uoffset - offset >= 0) || ((uoffset + offset) * cpu < w); offset++) {
		if (uoffset - offset >= 0)		draw_vline_at(-offset);
		if ((uoffset + offset) * cpu < w)	draw_vline_at(offset);
	}

	draw_status(normal_status_fields);
}

void input(const keystroke_s& key, const bool& bind_fired) {}

NOAFTODO_END_MODE
