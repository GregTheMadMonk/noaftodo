#include <cmd.h>
#include <cui.h>
#include <cvar.h>
#include <entry_flags.h>
#include <core/time.h>

using namespace std;
using namespace li::entry_flags;

NOAFTODO_START_MODE(timeline, init, paint, input)

using li::t_list;

int position = 0;

int unit = 60; // minutes per timeline unit (basically, scale)
constexpr int units[] = { 1, 2, 5, 10, 15, 20, 30 };

std::string line	= "-";
std::string vline	= "|";
std::string vline_now	= "#";
std::string marker	= "^";
std::string marker_now	= "#";
std::string rarrow	= "~~~>";

inline int closest_id(const time_s& t0) {
	int dist = -1;
	const auto dist_f = [&t0] (const time_s& t) { return abs(t0.time - t.time); };
	int ret = s_line;
	for (int i = 0; i < t_list.size(); i++) {
		if (!is_visible(i)) continue;
		if ((dist == -1) || (dist > dist_f(t_list.at(i).due))) { dist = dist_f(t_list.at(i).due); ret = i; }
		const int d2 = dist_f(time_s(t_list.at(i).due.fmt_cmd() + "a" + t_list.at(i).get_meta("duration", li::task_duration_default)));
		if ((dist == -1) || (dist > d2)) { dist = d2; ret = i; }
	}

	return ret;
}

inline void focus() {
	position = (t_list.at(s_line).due.time - time_s().time) / 60 / unit;
}

void init() {
	// timeline mode cvars
	// @cvar "timeline.position" - position (in units, relative to current time) of the timeline
	cvar_base_s::cvars["timeline.position"] = cvar_base_s::wrap_int(position);
	// @cvar "timeline.unit" - length (in minutes) of a TIMELINE mode unit
	cvar_base_s::cvars["timeline.unit"] = cvar_base_s::wrap_int(unit);
	// @cvar "charset.timeline.marker" - TIMELINE marker (displayed where a vertical line crosses horizontal)
	cvar_base_s::cvars["charset.timeline.marker"] = cvar_base_s::wrap_string(marker);
	// @cvar "charset.timeline.marker_now" - timeline marker at current time
	cvar_base_s::cvars["charset.timeline.marker_now"] = cvar_base_s::wrap_string(marker_now);
	// @cvar "charset.timeline.line" - character for horizontal line in TIMELINE mode
	cvar_base_s::cvars["charset.timeline.line"] = cvar_base_s::wrap_string(line);
	// @cvar "charset.timeline.vline" - character to draw vertical lines in TIMELINE mode with
	cvar_base_s::cvars["charset.timeline.vline"] = cvar_base_s::wrap_string(vline);
	// @cvar "charset.timeline.vline_now" - character to draw vertical line indicating current time in TIMELINE mode with
	cvar_base_s::cvars["charset.timeline.vline_now"] = cvar_base_s::wrap_string(vline_now);
	// @cvar "charset.timeline.rarrow" - cute right arrow at the end of horizontal line indicating the flow of time
	cvar_base_s::cvars["charset.timeline.rarrow"] = cvar_base_s::wrap_string(rarrow);
	
	// timeline mode commands
	// @command "timeline.focus" - focus selected entry in TIMELINE mode
	cmd::cmds["timeline.focus"] = [] (const vector<string>& args) {
		focus();
		return 0;
	};

	// @command "timeline.focus_closest" - same as timeline.focus, but prior to it selects closest entry to current position on TIMELINE
	cmd::cmds["timeline.focus_closest"] = [] (const vector<string>& args) {
		const auto t = time_s("a" + to_string(position * unit));
		s_line = closest_id(t);
		focus();

		return 0;
	};

	// @command "timeline.scale_up" - scale up timeline
	cmd::cmds["timeline.scale_up"] = [] (const vector<string>& args) {
		for (const auto& u : units) if (u > unit) {
			position = position * unit / u;
			unit = u;
			return 0;
		}

		unit *= 2;
		position /= 2;

		return 0;
	};

	// @command "timeline.scale_down" - scale down timeline
	cmd::cmds["timeline.scale_down"] = [] (const vector<string>& args) {
		if (unit >= 60) {
			unit /= 2;
			position *= 2;
			return 0;
		}

		int n = unit;
		for (const auto& u : units) if (u < unit)
			n = u;

		position = position * unit / n;
		unit = n;
		return 0;
	};

	// @command "timeline.next" - select next entry on the timeline
	cmd::cmds["timeline.next"] = [] (const vector<string>& args) {
		int next_id = s_line;

		for (int i = 0; i < t_list.size(); i++) {
			if (is_visible(i) && ((t_list.at(i).due > t_list.at(s_line).due) || ((t_list.at(i).due == t_list.at(s_line).due) && (s_line < i))))
				if ((next_id == s_line) || (t_list.at(i).due < t_list.at(next_id).due)) next_id = i;
		}
		s_line = next_id;

		return 0;
	};

	// @command "timeline.prev" - select previous entry on the timeline
	cmd::cmds["timeline.prev"] = [] (const vector<string>& args) {
		int next_id = s_line;

		for (int i = t_list.size() - 1; i >= 0; i--) {
			if (is_visible(i) && ((t_list.at(i).due < t_list.at(s_line).due) || ((t_list.at(i).due == t_list.at(s_line).due) && (s_line > i))))
				if ((next_id == s_line) || (t_list.at(i).due > t_list.at(next_id).due)) next_id = i;
		}
		s_line = next_id;

		return 0;
	};
}

void paint() {
	clear();

	// draw title
	move(0, 0);
	attrset_ext(A_STANDOUT | A_BOLD, color_title);
	for (int i = 0; i < w; i++) addch(' ');

	move(0, 1);
	if (unit < 60) addstr(("Unit size: " + to_string(unit) + " minute(s).").c_str());
	else addstr(("Unit size: " + to_string(unit / 60) + " hour(s).").c_str());

	attrset_ext(A_NORMAL);

	// draw the line
	move(h - 4, 0);
	for (int x = 0; x < w - 1; x++) addstr(line.c_str());
	move(h - 4, w - w_converter().from_bytes(rarrow).length());
	addstr(rarrow.c_str());

	const int rounder = time_s().to_tm().tm_min % unit;

	const int cpu = 12;		// characters per unit
	const int uoffset = w / cpu / 2;// unit painting offset

	const auto draw_vline_at = [&cpu, &uoffset, &rounder] (const int& offset) {
		// draws a vertical unit line
		for (int y = 1; y < h - 1; y++) {
			move(y, (uoffset + offset) * cpu);
			addstr(((y == h - 4) ?
				((offset + position == 0) ? marker_now : marker)
				:
				((offset + position == 0) ? vline_now : vline)).c_str());
		}

		// mark the line
		const auto t = time_s("a" + to_string((position + offset) * unit));
		move(h - 3, (uoffset + offset) * cpu + 1);
		addnstr(t.fmt_str("\%Y/%m/%d").c_str(), w - (uoffset + offset) * cpu - 1);
		move(h - 2, (uoffset + offset) * cpu + 1);
		addnstr(t.fmt_str("%H:%M").c_str(), w - (uoffset + offset) * cpu - 1);
	};

	const auto time_coord = [&uoffset] (const time_s& time) {
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

		attrset_ext((i == s_line) ? A_STANDOUT : A_NORMAL);
		if (is_completed(e)) attron_ext(A_BOLD, color_complete);
		else if (is_failed(e)) attron_ext(A_BOLD, color_failed);
		else if (is_due(e)) attron_ext(A_BOLD, color_due);
		else if (is_coming(e)) attron_ext(A_BOLD, color_coming);

		draw_border(x, y1 - 2, x1 - x, 3, box_strong);
		clear_box(x + 1, y1 - 1, x1 - x - 2, 1);
		if (x1 - x > 2) {
			text_box(x + 2, y1 - 2, x1 - x - 4, 1, li::t_tags.at(t_list.at(i).tag));
			text_box(x + 1, y1 - 1, x1 - x - 2, 1, t_list.at(i).title + " - " + t_list.at(i).description);
			text_box(x + 2, y1, x1 - x - 4, 1, "#" + to_string(i));
		}
		y1 -= 3;
	}

	draw_status(normal_status_fields);
}

void input(const keystroke_s& key, const bool& bind_fired) {}

NOAFTODO_END_MODE
