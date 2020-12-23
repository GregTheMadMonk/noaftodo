#ifndef NOAFTODO_LIST_H
#define NOAFTODO_LIST_H

#include <cmath>
#include <functional>
#include <map>
#include <stdexcept>
#include <string>
#include <sys/stat.h>
#include <sys/types.h>
#include <vector>

#include "noaftodo_macro.h"
#include "noaftodo_time.h"

namespace li {

struct entry;

extern std::vector<entry> t_list;	// the list itself
extern std::vector<std::string> t_tags;		// list tags
extern std::string filename;		// the list filename
extern bool autosave;

extern struct stat file_stat;

struct entry {
	bool completed;
	time_s due;
	std::string title;
	std::string description;
	int tag; // also tag = -1 indicates a null entry

	std::map<std::string, std::string> meta;

	CONST_DPL(std::string get_meta(const std::string& str, const std::string& def = ""), {
		try  {
			return (this->meta.at(str) == "") ? def : this->meta.at(str);
		} catch (const std::out_of_range& e) { return def; }
	})

	CONST_DPL(bool sim(const entry& e2), {
		return (this->due == e2.due) &&
			(this->title == e2.title) &&
			(this->description == e2.description) &&
			(this->tag == e2.tag);
	})
	CONST_DPL(bool operator==(const entry& comp), {
		return this->sim(comp) &&
			(this->completed == comp.completed) &&
			(this->meta == comp.meta);
	})
	CONST_DPL(bool operator!=(const entry& comp), { return !(*this == comp); })

	CONST_DPL(std::string meta_str(), {
		std::string meta = "";

		for (auto it = this->meta.begin(); it != this->meta.end(); it++) {
			if (meta != "") meta += " ";
			meta += "\"" + it->first + "\" \"" + it->second + "\"";
		}

		return meta;
	})

	void name() {
		if (this->get_meta("eid") == "") for (int i = 0; ; i++)
			// if (!found)
			if ( ([&i] () -> bool {
				for (const auto& e : t_list) if (e.get_meta("eid") == "e" + std::to_string(i))
					return false;

				return true;
			})() ) {
				this->meta["eid"] = "e" + std::to_string(i);
				break;
			}
	}
};

extern std::string sort_order;

struct less_than_entry_order {
	inline int comp_due(const entry& e1, const entry& e2) {
		// can't use is_nodue entry flag here because noaftodo_entry_flags.h includes this file
		if ((e1.get_meta("nodue") == "true") && (e2.get_meta("nodue") == "true")) return 0;
		if ((e1.get_meta("nodue") == "true") && (e2.get_meta("nodue") != "true")) return -1;
		if ((e2.get_meta("nodue") == "true") && (e1.get_meta("nodue") != "true")) return 1;

		return (e1.due < e2.due) ? 1 : -1;
	}

	inline int comp_tag(const entry& e1, const entry& e2) {
		if (e1.tag == e2.tag) return 0;

		return (e1.tag < e2.tag) ? 1 : -1;
	}

	inline int comp_title(const entry& e1, const entry& e2) {
		try { // sort by number in title
			const double d1 = stod(e1.title);
			const double d2 = stod(e2.title);
			
			if (d1 == d2) return 0;
			
			return (d1 < d2) ? 1 : -1;
		} catch (const std::invalid_argument& e) {
			// sort by title
			for (int i = 0; (i < e1.title.length()) && (i < e2.title.length()); i++)
				if (e1.title.at(i) != e2.title.at(i)) return (e1.title.at(i) < e2.title.at(i)) ? 1 : -1;
		}

		return 0;
	}

	inline int comp_desc(const entry& e1, const entry& e2) {
		try { // sort by number in description
			const double d1 = stod(e1.description);
			const double d2 = stod(e2.description);

			if (d1 == d2) return 0;

			return (d1 < d2) ? 1 : -1;
		} catch (const std::invalid_argument& e) {
			// sort by title
			for (int i = 0; (i < e1.description.length()) && (i < e2.description.length()); i++)
				if (e1.description.at(i) != e2.description.at(i)) return (e1.description.at(i) < e2.description.at(i)) ? 1 : -1;
		}

		return 0;
	}

	inline bool operator() (const entry& e1, const entry& e2) {
		float comparator = 0.0;
		for (int i = 0; i < sort_order.length(); i++) {
			int field_comp = 0;

			switch (sort_order.at(i)) {
				case 'd':
					field_comp = comp_due(e1, e2);
					break;
				case 'l':
					field_comp = comp_tag(e1, e2);
					break;
				case 't':
					field_comp = comp_title(e1, e2);
					break;
				case 'D':
					field_comp = comp_desc(e1, e2);
					break;
			}

			comparator += pow(0.25, i) * field_comp;
		}

		return comparator > 0;
	}
};

struct less_than_entry {
	inline bool operator() (const entry& e1, const entry& e2) {
		if (e1.get_meta("eid") == "") return true;
		if (e2.get_meta("eid") == "") return false;

		try {
			const int i1 = stoi(e1.get_meta("eid").substr(1));
			const int i2 = stoi(e2.get_meta("eid").substr(1));

			return i1 < i2;
		} catch (const std::invalid_argument& e) { return false; }
	}
};

void load(const bool& load_workspace = true);
void load(const std::string& load_filename, const bool& load_workspace = true);

int save();
int save(const std::string& save_filename);

void upd_stat();
bool has_changed();

void add(const entry& entry);
void comp(const std::string& eid);
void comp(const int& entryID);
void rem(const std::string& eid);
void rem(const int& entryID);

void sort();
void identify_all();
void prepare(std::vector<entry>& list);

int find(const std::string& eid);

bool tag_completed(const int& tagID);
bool tag_coming(const int& tagID);
bool tag_failed(const int& tagID);

}

#endif
