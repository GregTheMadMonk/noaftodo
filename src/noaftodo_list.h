#ifndef NOAFTODO_LIST_H
#define NOAFTODO_LIST_H

#include <stdexcept>
#include <map>
#include <string>
#include <sys/stat.h>
#include <sys/types.h>
#include <vector>

struct noaftodo_entry
{
	bool completed;
	long due;
	std::string title;
	std::string description;
	int tag; // also tag = -1 indicates a null entry

	std::map<std::string, std::string> meta;
	std::string get_meta(const std::string& str);
	std::string get_meta(const std::string& str) const;

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
};

const noaftodo_entry NULL_ENTRY = { .tag = -1 };

struct less_than_noaftodo_entry
{
	inline bool operator() (const noaftodo_entry& e1, const noaftodo_entry& e2)
	{
		// if only one of entries is "nodue" - it comes after
		if ((e1.get_meta("nodue") == "true") &&  (e2.get_meta("nodue") != "true")) return false;
		if ((e2.get_meta("nodue") == "true") &&  (e1.get_meta("nodue") != "true")) return true;

		// if both are "nodue"
		if ((e1.get_meta("nodue") == "true") &&  (e2.get_meta("nodue") == "true"))
		{
			if (e1.tag != e2.tag) return e1.tag < e2.tag; // sort by tag
			else	// if tags are equal, sort by title
			{
				try { // sort by number in title
					return stod(e1.title) < stod(e2.title);
				} catch (const std::invalid_argument& e) {
					// sort by title
					for (int i = 0; (i < e1.title.length()) && (i < e2.title.length()); i++)
						if (e1.title.at(i) != e2.title.at(i)) return e1.title.at(i) < e2.title.at(i);
						
					// try to sort by description
					try { // sort by number in description
						return stod(e1.description) < stod(e2.description);
					} catch (const std::invalid_argument& e) {
						// sort by title
						for (int i = 0; (i < e1.description.length()) && (i < e2.description.length()); i++)
							if (e1.description.at(i) != e2.description.at(i)) return e1.description.at(i) < e2.description.at(i);

						return false;
					}
				}
			}
		}
		return e1.due < e2.due;
	}
};

extern std::vector<noaftodo_entry> t_list;	// the list itself
extern std::vector<std::string> t_tags;		// list tags
extern std::string li_filename;		// the list filename
extern bool li_autosave;

extern struct stat li_file_stat;

void li_load(const bool& load_workspace = true);
void li_load(const std::string& filename, const bool& load_workspace = true);

void li_save();
void li_save(const std::string& filename);

void li_upd_stat();
bool li_has_changed();

void li_add(const noaftodo_entry& li_entry);
void li_comp(const int& entryID);
void li_rem(const int& entryID);

void li_sort();

#endif
