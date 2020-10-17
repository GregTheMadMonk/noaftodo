#include "../noaftodo_cui.h"
#include "../noaftodo_cvar.h"
#include "../noaftodo_time.h"

using namespace std;

NOAFTODO_START_MODE(timeline, init, paint, input)

using li::t_list;

int position = 0;

int scale = 6; // characters per hour

std::string line	= "_";
std::string vline	= "|";
std::string marker	= "^";

void init() {
	cvar_base_s::cvars["timeline.position"] = cvar_base_s::wrap_int(position);
	cvar_base_s::cvars["timeline.scale"] = cvar_base_s::wrap_int(scale);
	cvar_base_s::cvars["charset.timeline.marker"] = cvar_base_s::wrap_string(marker);
	cvar_base_s::cvars["charset.timeline.line"] = cvar_base_s::wrap_string(line);
	cvar_base_s::cvars["charset.timeline.vline"] = cvar_base_s::wrap_string(vline);
}

void paint() {
	clear();

	const auto now = time_s();
	const int now_hr = now.to_tm().tm_hour;

	// draw the timeline
	for (int x = 0; x < w; x++) { // scan horizontally
		int hr = (now_hr + (position + x) / scale) % 24;
		while (hr < 0) hr += 24;

		move(h - 2, x);
		addstr(line.c_str());
		if ((position + x) % scale == 0) {
			move(h - 2, x);
			addstr(marker.c_str());
			for (int y = 0; y < h - 2; y++) {
				move(y, x);
				addstr(vline.c_str());
			}
		}
	}

	// label timelien
	for (int x = 0; x < w; x++) { // scan horizontally
		int hr = (now_hr + (position + x) / scale) % 24;
		while (hr < 0) hr += 24;

		if ((position + x) % scale == 0) {
			move(h - 5, x + 1);
			addstr(to_string(hr).c_str());

			if (hr == 0) { // a new day begins
				move(h - 6, x + 1);
				addstr((to_string(now.to_tm().tm_mday) + "/" + to_string(now.to_tm().tm_mon + 1)).c_str());
			}
		}
	}

	draw_status(normal_status_fields);
}

void input(const keystroke_s& key, const bool& bind_fired) {}

NOAFTODO_END_MODE
