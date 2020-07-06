#ifndef NOAFTODO_LIST_H
#define NOAFTODO_LIST_H

#include <cmath>
#include <stdexcept>
#include <map>
#include <string>
#include <sys/stat.h>
#include <sys/types.h>
#include <vector>

struct noaftodo_entry {
	bool completed;
	long due;
	std::string title;
	std::string description;
	int tag; // also tag = -1 indicates a null entry

	std::map<std::string, std::string> meta;
	std::string get_meta(const std::string& str, const std::string& def = "");
	std::string get_meta(const std::string& str, const std::string& def = "") const;

	bool sim(const noaftodo_entry& e2);
	bool sim(const noaftodo_entry& e2) const;
	bool operator==(const noaftodo_entry& comp);
	bool operator==(const noaftodo_entry& comp) const;
	bool operator!=(const noaftodo_entry& comp);
	bool operator!=(const noaftodo_entry& comp) const;

	std::string meta_str();
	std::string meta_str() const;

	bool is_failed();
	bool is_failed() const;

	bool is_coming();
	bool is_coming() const;

	bool is_uncat();
	bool is_uncat() const;

	void name();
};

extern std::string li_sort_order;

struct less_than_noaftodo_entry_order {
	inline int comp_due(const noaftodo_entry& e1, const noaftodo_entry& e2) {
		if ((e1.get_meta("nodue") == "true") && (e2.get_meta("nodue") == "true")) return 0;
		if ((e1.get_meta("nodue") == "true") && (e2.get_meta("nodue") != "true")) return -1;
		if ((e2.get_meta("nodue") == "true") && (e1.get_meta("nodue") != "true")) return 1;

		return (e1.due < e2.due) ? 1 : -1;
	}

	inline int comp_tag(const noaftodo_entry& e1, const noaftodo_entry& e2) {
		if (e1.tag == e2.tag) return 0;

		return (e1.tag < e2.tag) ? 1 : -1;
	}

	inline int comp_title(const noaftodo_entry& e1, const noaftodo_entry& e2) {
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

	inline int comp_desc(const noaftodo_entry& e1, const noaftodo_entry& e2) {
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

	inline bool operator() (const noaftodo_entry& e1, const noaftodo_entry& e2) {
		float comparator = 0.0;
		for (int i = 0; i < li_sort_order.length(); i++) {
			int field_comp = 0;

			switch (li_sort_order.at(i)) {
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

struct less_than_noaftodo_entry {
	inline bool operator() (const noaftodo_entry& e1, const noaftodo_entry& e2) {
		if (e1.get_meta("eid") == "") return true;
		if (e2.get_meta("eid") == "") return false;

		try {
			const int i1 = stoi(e1.get_meta("eid").substr(1));
			const int i2 = stoi(e2.get_meta("eid").substr(1));

			return i1 < i2;
		} catch (const std::invalid_argument& e) { return false; }
	}
};

extern std::vector<noaftodo_entry> t_list;	// the list itself
extern std::vector<std::string> t_tags;		// list tags
extern std::string li_filename;		// the list filename
extern bool li_autosave;

extern struct stat li_file_stat;

void li_load(const bool& load_workspace = true);
void li_load(const std::string& filename, const bool& load_workspace = true);

int li_save();
int li_save(const std::string& filename);

void li_upd_stat();
bool li_has_changed();

void li_add(const noaftodo_entry& li_entry);
void li_comp(const std::string& eid);
void li_comp(const int& entryID);
void li_rem(const std::string& eid);
void li_rem(const int& entryID);

void li_sort();
void li_prepare(std::vector<noaftodo_entry>& list);

int li_find(const std::string& eid);

#endif
