#include "noaftodo_list.h"

#include <algorithm>
#include <fstream>
#include <unistd.h>

#include "noaftodo.h"
#include "noaftodo_cmd.h"
#include "noaftodo_config.h"
#include "noaftodo_cvar.h"
#include "noaftodo_cui.h"
#include "noaftodo_daemon.h"
#include "noaftodo_entry_flags.h"
#include "noaftodo_time.h"

using namespace std;

using namespace li::entry_flags;

namespace li {

vector<entry> t_list;
vector<string> t_tags;

struct stat file_stat;

void load(const bool& load_workspace) {
	log("=====================================================");
	log("Loading list file " + filename);
	log("=====================================================");

	bool safemode = true;

	if (load_workspace) // clear cvars
		for (int offset = 0; offset < cvar_base_s::cvars.size(); offset++) {
			auto it = cvar_base_s::cvars.begin();
			for (int i = 0; i < offset; i++) it++;

			if (cvar(it->first).flags & CVAR_FLAG_WS_IGNORE) continue;

			log("Resetting cvar " + it->first + " from value " + cvar(it->first).getter());

			if (cvar(it->first).predef_val == "") {
				cvar_base_s::erase(it->first);
				if (cvar_base_s::is_deletable(it->first)) offset--;
			} else cvar(it->first).reset();

			log("to value " + cvar(it->first).getter());
		}

	auto t_list_copy = t_list;
	auto t_tags_copy = t_tags;
	t_list.clear();
	t_tags.clear();

	// check if file exists
	ifstream ifile(filename);
	if (!ifile.good()) {
		// create list file
		log("File does not exist!", LP_ERROR);
		log("Creating...");
		ofstream ofile(filename);
		ofile << "# noaftodo list file" << endl << "[ver]" << endl << LIST_V << endl;
		safemode = false;
		if (ofile.good()) log("Created");
		else log("Uh oh file not created", LP_ERROR);
		ofile.close();
	} else {
		// load the list
		string line;
		int mode = 0; 	// -1 - nothing
				// 0 - list tags read
				// 1 - lists read
				// 2 - workspace read
				// 3 - list version read

		while (getline(ifile, line)) {
			if (line == "") continue; // only non-empty lines

			// trim string
			while ((line.at(0) == ' ') || (line.at(0) == '\t')) {
				line = line.substr(1);
				if (line == "") break;
			}

			if (line == "") 	continue; // same precautions
			if (line.at(0) == '#') 	continue; // comment?

			log(line);

			int token = 0;
			entry e;
			string temp = "";
			string metaname = "";

			if (line.at(0) == '[') {
				if (line == "[tags]") 		mode = 0;
				if (line == "[list]") 		mode = 1;
				if (line == "[workspace]") 	mode = 2;
				if (line == "[ver]") 		mode = 3;

				if (mode != 2) cmd::terminate();
			} else switch (mode) {
				case 0: // tags
					log("Added tag \"" + line + "\" with index " + to_string(t_tags.size()));
					t_tags.push_back(line);
					break;

				case 1: // list
					{
					bool inquotes = false;
					bool skip_special = false;

					for (int i = 0; i < line.length(); i++) {
						const char c = line.at(i);

						if (skip_special) { temp += c; skip_special = false; }
						else switch (c)  {
							case '\\':
								if (inquotes) {
									skip_special = true;
								} else {
									switch (token) {
										case 0:
											e.completed = (temp == "v");
											break;
										case 1:
											if (temp != "") if (temp.at(temp.length() - 1) == 'e') {
												// seconds since epoch (GMT)
												e.due = time_s(temp);
												break;
											}
											// load old time format that didn't handle timezones
											// DEPRECATED no lits should save the time like this
											// this is for loading only
											// to-be REMOVED in 2.x.x
											e.due = time_s(stol(temp));
											break;
										case 2:
											e.title = temp;
											log("Entry: " + e.title);
											break;
										case 3:
											e.description = temp;
											break;
										case 4:
											e.tag = stoi(temp);
											break;
										default:
											// metadata
											if (metaname == "") metaname = temp;
											else
											{
												e.meta[metaname] = temp;
												log("Meta prop: " + metaname + " <=> " + temp);
												metaname = "";
											}
											break;
									}

									temp = "";
									token++;
								}

								break;
							case '\"':
								inquotes = !inquotes;
								break;
							default:
								temp += c;
								break;
						}
					}

					t_list.push_back(e);
					}
					break;
				
				case 2: // workspace
					if (load_workspace) {
						while (line.at(0) == ' ') {
							line = line.substr(1);
							if (line == "") break;
						}

						if (line != "") if (line.at(0) != '#')
							cmd::exec(line);
					}
					break;

				case 3: // list version verification
					safemode &= (line != to_string(LIST_V));
					break;
			}
		}
	}

	if (safemode) {
		errors |= ERR_LIST_V;

		if (run_mode == PM_DAEMON)  {
			t_list = t_list_copy;
			t_tags = t_tags_copy;
		} else log("List version mismatch. "
			"Consult \"TROUBLESHOOTING\" manpage section (\"noaftodo -h\"). ", LP_ERROR);
	}

	autosave = (errors == 0) && (!safemode) && (run_mode != PM_DAEMON); // don't allow daemon to save stuff

	upd_stat();

	prepare(t_list);
	sort();
}

void load(const string& load_filename, const bool& load_workspace) {
	filename = load_filename;
	load(load_workspace);
}

int save(const string& save_filename) {
	if ((filename == save_filename) && has_changed()) {
		log("File was modified since its last recorded change. Skipping save!", LP_ERROR);
		return 1;
	}

	filename = save_filename;

	log("Saving...");

	identify_all();
	auto t_list_copy = t_list;
	prepare(t_list_copy);

	ofstream ofile;
	ofile.open(filename, ios::out | ios::trunc);

	ofile << "# naftodo list file" << endl;

	ofile << "[tags]" << endl << "# tags start at index 0 and go on" << endl;
	for (const auto& tag : t_tags)
		ofile << tag << endl;

	ofile << endl << "[list]" << endl;
	for (const auto& entry : t_list_copy) {
		ofile << "\"" << (entry.completed ? 'v' : '-')
			<< "\"\\\"" << to_string(entry.due.time) << "e"
			<< "\"\\\"" << replace_special(entry.title)
			<< "\"\\\"" << replace_special(entry.description)
			<< "\"\\\"" << entry.tag << "\"";

		for (auto it = entry.meta.begin(); it != entry.meta.end(); it++) {
			ofile << "\\\"" << replace_special(it->first)
				<< "\"\\\"" << replace_special(it->second) << "\"";
		}

		ofile << '\\' << endl;
	}

	ofile << endl << "[workspace]" << endl;
	ofile << "ver " << CONF_V << endl;
	for (auto cvar_i = cvar_base_s::cvars.begin(); cvar_i != cvar_base_s::cvars.end(); cvar_i++) {
		const string key = cvar_i->first;
		if ((cvar(key).flags & CVAR_FLAG_WS_IGNORE) == 0) if (cvar(key).changed()) {
			log("Setting workspace var " + key + " (" + to_string(cvar(key).flags) + " | " + to_string(cvar(key).flags & CVAR_FLAG_WS_IGNORE) + ")");
			ofile << "set \"" << key << "\" \"" << cvar(key).getter() << "\"" << endl;
		}
	}

	ofile << endl << "[ver]" << endl << LIST_V << endl;

	ofile.close();

	upd_stat();

	log("Changes written to file " + filename);

	return 0;
}

int save() {
	if (errors != 0) {
		log("Aborting save() in safe mode: saving in safe mode is prohibited without an explicit path specifition", LP_ERROR);
		return 1;
	}
	return save(filename);
}

void upd_stat() {
	if (stat(filename.c_str(), &file_stat) != 0)
		log("Can't stat(...) file", LP_ERROR);
}

bool has_changed() {
	struct stat new_stat;

	if (stat(filename.c_str(), &new_stat) != 0)
	{
		log("Can't stat(...) file", LP_ERROR);
		return false;
	}

	return (new_stat.st_mtime != file_stat.st_mtime) || (new_stat.st_mtim.tv_nsec != file_stat.st_mtim.tv_nsec);
}

void add(const entry& entry) {
	log("Adding " + entry.title + "...");
	t_list.push_back(entry);
	t_list[t_list.size() - 1].name();

	sort();
	if (autosave) save();

	da::send("A");
}

void comp(const int& entryID) {
	if ((entryID < 0) || (entryID >= t_list.size())) {
		log("comp: entry ID out of list bounds. Operation aborted", LP_ERROR);
		return;
	}

	t_list.at(entryID).completed = !t_list.at(entryID).completed;

	if (autosave) save();

	da::send("C");
}

void rem(const int& entryID) {
	if ((entryID < 0) || (entryID >= t_list.size())) {
		log("rem: entry ID out of list bounds. Operation aborted", LP_ERROR);
		return;
	}

	log("Removing " + t_list.at(entryID).title + "...");
	t_list.erase(t_list.begin() + entryID);

	if (autosave) save();

	da::send("R");
}

void sort() {
	std::sort(t_list.begin(), t_list.end(), less_than_entry_order());
}

void identify_all() {
	// identify entries
	for (auto& entry : t_list)
		if (entry.get_meta("eid") == "") entry.name();
}

void prepare(vector<entry>& list) {
	// sort entries
	std::sort(list.begin(), list.end(), less_than_entry());

	int max_tag = t_tags.size() - 1;
	for (const auto& e : t_list) if (e.tag > max_tag) max_tag = e.tag;
	while (max_tag >= (int)t_tags.size()) t_tags.push_back(to_string(t_tags.size()));

	// clean useless tags
	for (int i = t_tags.size() - 1; i >= 0; i--) {
		if (t_tags.at(i) != to_string(i)) return;
		for (const auto& e : t_list) if (e.tag == i) return;
		t_tags.erase(t_tags.begin() + i);
	}
}

int find(const string& eid) {
	for (int i = 0; i < t_list.size(); i++)
		if (t_list.at(i).get_meta("eid") == eid) return i;

	return -1;
}

bool tag_completed(const int& tagID) {
	for (const auto& e : t_list)
		if (((tagID == cui::TAG_ALL) || (e.tag == tagID)) && (!is_completed(e))) return false;

	return true;
}

bool tag_coming(const int& tagID) {
	for (const auto& e : t_list)
		if (((tagID == cui::TAG_ALL) || (e.tag == tagID)) && (is_coming(e))) return true;

	return false;
}

bool tag_failed(const int& tagID) {
	for (const auto& e : t_list)
		if (((tagID == cui::TAG_ALL) || (e.tag == tagID)) && (is_failed(e))) return true;

	return false;
}

}
