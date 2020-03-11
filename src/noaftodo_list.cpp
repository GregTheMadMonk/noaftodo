#include "noaftodo_list.h"

#include <algorithm>
#include <fstream>

#include "noaftodo_daemon.h"
#include "noaftodo_output.h"

using namespace std;

vector<noaftodo_entry> t_list;
string li_filename = ".noaftodo-list";
bool li_autosave = true;

bool noaftodo_entry::sim(const noaftodo_entry& e2)
{
	return (this->due == e2.due) && (this->title == e2.title) && (this->description == e2.description);
}

void li_load()
{
	log("Loading list file " + li_filename);

	t_list.clear();

	// check if file exists
	ifstream ifile(li_filename);
	if (!ifile.good())
	{
		// create list file
		log("File does not exist!", LP_ERROR);
		log("Creating...");
		ofstream ofile(li_filename);
		ofile << "# noaftodo list file" << endl;
		if (ofile.good()) log("Created");
		else log("Uh oh file not created", LP_ERROR);
		ofile.close();
	} else {
		// load the list
		string entry;

		while (getline(ifile, entry))
		{
			int token = 0;
			noaftodo_entry li_entry;
			string temp = "";

			if (entry != "") // only non-empty lines
			{
				// trim string
				while (entry.at(0) == ' ') 
				{
					entry = entry.substr(1);
					if (entry == "") break;
				}

				if (entry != "") if (entry.at(0) != '#')
				{
					log(entry);
					for (int i = 0; i < entry.length(); i++)
					{
						if (entry.at(i) == '\\')
						{
							switch (token)
							{
								case 0:
									li_entry.completed = (temp == "v");
									break;
								case 1:
									li_entry.due = stol(temp);
									break;
								case 2:
									li_entry.title = temp;
									break;
								case 3:
									li_entry.description = temp;
									break;
							}

							temp = "";
							token++;
						} else temp += entry.at(i);
					}

					t_list.push_back(li_entry);
				}
			}
		}
	}

	li_sort();
}

void li_load(const string& filename)
{
	li_filename = filename;
	li_load();
}

void li_save()
{
	ofstream ofile;
	ofile.open(li_filename, ios::out | ios::trunc);

	ofile << "# naftodo list file" << endl;

	for (const auto& entry : t_list)
		ofile << (entry.completed ? 'v' : '-') << '\\' << entry.due << '\\' << entry.title << '\\' << entry.description << '\\' << endl;

	log("Changes written to file " + li_filename);
}

void li_save(const string& filename)
{
	li_filename = filename;
	li_save();
}

void li_add(const noaftodo_entry& li_entry)
{
	log("Adding " + li_entry.title + "...");
	t_list.push_back(li_entry);

	li_sort(); // will also autosave

	da_send("A");
}

void li_comp(const int& entryID)
{
	if ((entryID < 0) || (entryID >= t_list.size()))
	{
		log("li_comp: entry ID out of list bounds. Operation aborted", LP_ERROR);
		return;
	}

	t_list.at(entryID).completed = !t_list.at(entryID).completed;

	if (li_autosave) li_save();

	da_send("C");
}

void li_rem(const int& entryID)
{
	if ((entryID < 0) || (entryID >= t_list.size()))
	{
		log("li_rem: entry ID out of list bounds. Operation aborted", LP_ERROR);
		return;
	}

	log("Removing " + t_list.at(entryID).title + "...");
	t_list.erase(t_list.begin() + entryID);

	li_sort();

	da_send("R");
}

void li_sort()
{
	std::sort(t_list.begin(), t_list.end(), less_than_noaftodo_entry());

	if (li_autosave) li_save();
}
