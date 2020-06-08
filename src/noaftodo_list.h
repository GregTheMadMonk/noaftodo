#ifndef NOAFTODO_LIST_H
#define NOAFTODO_LIST_H

#include <stdexcept>
#include <map>
#include <string>
#include <vector>

struct noaftodo_entry
{
	bool completed;
	long due;
	std::string title;
	std::string description;
	int tag;

	std::map<std::string, std::string> meta;
	std::string get_meta(const std::string& str);
	std::string get_meta(const std::string& str) const;

	bool sim(const noaftodo_entry& e2);
	bool operator==(const noaftodo_entry& comp);

	std::string meta_str();
	std::string meta_str() const;

	bool is_failed();
	bool is_failed() const;

	bool is_coming();
	bool is_coming() const;
};

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

					return false;
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

void li_load();
void li_load(const std::string& filename);

void li_save();
void li_save(const std::string& filename);

void li_add(const noaftodo_entry& li_entry);
void li_comp(const int& entryID);
void li_rem(const int& entryID);

void li_sort();

#endif
