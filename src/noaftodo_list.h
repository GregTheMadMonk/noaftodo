#ifndef NOAFTODO_LIST_H
#define NOAFTODO_LIST_H

#include <string>
#include <vector>

struct noaftodo_entry
{
	bool completed;
	long due;
	std::string title;
	std::string description;

	bool sim(const noaftodo_entry& e2);
};

struct less_than_noaftodo_entry
{
	inline bool operator() (const noaftodo_entry& e1, const noaftodo_entry& e2)
	{
		return e1.due < e2.due;
	}
};

extern std::vector<noaftodo_entry> t_list;	// the list itself
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
