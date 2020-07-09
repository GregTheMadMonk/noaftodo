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
#include "noaftodo_time.h"

using namespace std;

vector<noaftodo_entry> t_list;
vector<string> t_tags;
string li_filename = ".noaftodo-list";
bool li_autosave = true;

string li_sort_order = "ldtD";

struct stat li_file_stat;

string noaftodo_entry::get_meta(const string& str, const string& def) const {
	try  {
		return (this->meta.at(str) == "") ? def : this->meta.at(str);
	} catch (const out_of_range& e) { return def; }
}

string noaftodo_entry::get_meta(const string& str, const string& def) {
	return const_cast<const noaftodo_entry*>(this)->get_meta(str, def);
}

bool noaftodo_entry::operator==(const noaftodo_entry& comp) {
	return this->sim(comp) &&
		(this->completed == comp.completed) &&
		(this->meta == comp.meta);
}

bool noaftodo_entry::operator==(const noaftodo_entry& comp) const {
	return this->sim(comp) &&
		(this->completed == comp.completed) &&
		(this->meta == comp.meta);
}

bool noaftodo_entry::operator!=(const noaftodo_entry& comp) {
	return !(*this == comp);
}

bool noaftodo_entry::operator!=(const noaftodo_entry& comp) const {
	return !(*this == comp);
}

bool noaftodo_entry::sim(const noaftodo_entry& e2) const {
	return (this->due == e2.due) && 
		(this->title == e2.title) && 
		(this->description == e2.description) &&
		(this->tag == e2.tag);
}

bool noaftodo_entry::sim(const noaftodo_entry& e2) {
	return const_cast<const noaftodo_entry*>(this)->sim(e2); 
}

string noaftodo_entry::meta_str() const {
	string meta = "";

	for (auto it = this->meta.begin(); it != this->meta.end(); it++) {
		if (meta != "") meta += " ";
		meta += it->first + " " + it->second;
	}

	return meta;
}

string noaftodo_entry::meta_str() {
	return const_cast<const noaftodo_entry*>(this)->meta_str();
}

bool noaftodo_entry::is_failed() const {
	return !this->completed && (this->get_meta("nodue") != "true") && (this->due <= ti_to_long("a0d"));
}

bool noaftodo_entry::is_failed() {
	return const_cast<const noaftodo_entry*>(this)->is_failed();
}

bool noaftodo_entry::is_coming() const {
	return !this->completed && !this->is_failed() && (this->get_meta("nodue") != "true") && (this->due <= ti_to_long("a" + this->get_meta("warn_time", "1d")));
}

bool noaftodo_entry::is_coming() {
	return const_cast<const noaftodo_entry*>(this)->is_coming();
}

bool noaftodo_entry::is_uncat() const {
	return !this->completed &&
		!this->is_failed() &&
		!this->is_coming() &&
		(this->get_meta("nodue") != "true");
}

bool noaftodo_entry::is_uncat() {
	return const_cast<const noaftodo_entry*>(this)->is_uncat();
}

void noaftodo_entry::name() {
	if (this->get_meta("eid") == "")
		for (int i = 0; ; i++) {
			bool found = false;
			for (const auto& e : t_list) found |= (e.get_meta("eid") == "e" + to_string(i));

			if (!found) {
				this->meta["eid"] = "e" + to_string(i);
				break;
			}
		}
}

void li_load(const bool& load_workspace) {
	log("=====================================================");
	log("Loading list file " + li_filename);
	log("=====================================================");

	bool safemode = true;

	if (load_workspace) // clear cvars
		for (int offset = 0; offset < cvars.size(); offset++) {
			auto it = cvars.begin();
			for (int i = 0; i < offset; i++) it++;

			if (cvar(it->first).flags & CVAR_FLAG_WS_IGNORE) continue;

			log("Resetting cvar " + it->first + " from value " + cvar(it->first).getter());

			if (cvar(it->first).predef_val == "") {
				cvar_erase(it->first);
				if (cvar_is_deletable(it->first)) offset--;
			} else cvar(it->first).reset();

			log("to value " + cvar(it->first).getter());
		}

	auto t_list_copy = t_list;
	auto t_tags_copy = t_tags;
	t_list.clear();
	t_tags.clear();

	// check if file exists
	ifstream ifile(li_filename);
	if (!ifile.good()) {
		// create list file
		log("File does not exist!", LP_ERROR);
		log("Creating...");
		ofstream ofile(li_filename);
		ofile << "# noaftodo list file" << endl << "[ver]" << endl << LIST_V << endl;
		safemode = false;
		if (ofile.good()) log("Created");
		else log("Uh oh file not created", LP_ERROR);
		ofile.close();
	} else {
		// load the list
		string entry;
		int mode = 0; 	// -1 - nothing
				// 0 - list tags read
				// 1 - lists read
				// 2 - workspace read
				// 3 - list version read

		while (getline(ifile, entry)) {
			int token = 0;
			noaftodo_entry li_entry;
			string temp = "";
			string metaname = "";

			if (entry == "") continue; // only non-empty lines

			// trim string
			while ((entry.at(0) == ' ') || (entry.at(0) == '\t')) {
				entry = entry.substr(1);
				if (entry == "") break;
			}

			if (entry == "") 	continue; // same precautions
			if (entry.at(0) == '#') continue; // comment?

			log(entry);

			if (entry.at(0) == '[') {
				if (entry == "[tags]") 		mode = 0;
				if (entry == "[list]") 		mode = 1;
				if (entry == "[workspace]") 	mode = 2;
				if (entry == "[ver]") 		mode = 3;

				if (mode != 2) cmd_terminate();
			} else switch (mode) {
				case 0: // tags
					log("Added tag \"" + entry + "\" with index " + to_string(t_tags.size()));
					t_tags.push_back(entry);
					break;

				case 1: // list
					{
					bool inquotes = false;
					bool skip_special = false;

					for (int i = 0; i < entry.length(); i++) {
						const char c = entry.at(i);

						if (skip_special) { temp += c; skip_special = false; }
						else switch (c)  {
							case '\\':
								if (inquotes) {
									skip_special = true;
								} else {
									switch (token) {
										case 0:
											li_entry.completed = (temp == "v");
											break;
										case 1:
											li_entry.due = stol(temp);
											break;
										case 2:
											li_entry.title = temp;
											log("Entry: " + li_entry.title);
											break;
										case 3:
											li_entry.description = temp;
											break;
										case 4:
											li_entry.tag = stoi(temp);
											break;
										default:
											// metadata
											if (metaname == "") metaname = temp;
											else 
											{ 
												li_entry.meta[metaname] = temp; 
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

					t_list.push_back(li_entry);
					}
					break;
				
				case 2: // workspace
					if (load_workspace) {
						while (entry.at(0) == ' ') {
							entry = entry.substr(1);
							if (entry == "") break;
						}

						if (entry != "") if (entry.at(0) != '#')
							cmd_exec(entry);
					}
					break;

				case 3: // list version verification
					safemode &= (entry != to_string(LIST_V));
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
			"Consult \"Troubleshooting\" section of help (\"noaftodo -h\"). ", LP_ERROR);
	}

	li_autosave = (errors == 0) && (!safemode) && (run_mode != PM_DAEMON); // don't allow daemon to save stuff

	li_upd_stat();

	li_sort();
}

void li_load(const string& filename, const bool& load_workspace) {
	li_filename = filename;
	li_load(load_workspace);
}

int li_save(const string& filename) {
	if ((filename == li_filename) && li_has_changed()) {
		log("File was modified since its last recorded change. Skipping save!", LP_ERROR);
		return 1;
	}

	li_filename = filename;

	log("Saving...");

	auto t_list_copy = t_list;
	li_prepare(t_list_copy);

	ofstream ofile;
	ofile.open(li_filename, ios::out | ios::trunc);

	ofile << "# naftodo list file" << endl;

	ofile << "[tags]" << endl << "# tags start at index 0 and go on" << endl;
	for (const auto& tag : t_tags)
		ofile << tag << endl;

	ofile << endl << "[list]" << endl;
	for (const auto& entry : t_list_copy) {
		ofile << "\"" << (entry.completed ? 'v' : '-') 
			<< "\"\\\"" << entry.due 
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
	for (auto cvar_i = cvars.begin(); cvar_i != cvars.end(); cvar_i++) {
		const string key = cvar_i->first;
		if ((cvar(key).flags & CVAR_FLAG_WS_IGNORE) == 0) if (cvar(key).changed()) {
			log("Setting workspace var " + key + " (" + to_string(cvar(key).flags) + " | " + to_string(cvar(key).flags & CVAR_FLAG_WS_IGNORE) + ")");
			ofile << "set \"" << key << "\" \"" << cvar(key).getter() << "\"" << endl;
		}
	}

	ofile << endl << "[ver]" << endl << LIST_V << endl;

	ofile.close();

	li_upd_stat();

	log("Changes written to file " + li_filename);

	return 0;
}

int li_save() {
	if (errors != 0) {
		log("Aborting li_save() in safe mode: saving in safe mode is prohibited without an explicit path specifition", LP_ERROR);
		return 1;
	}
	return li_save(li_filename);
}

void li_upd_stat() {
	if (stat(li_filename.c_str(), &li_file_stat) != 0)
		log("Can't stat(...) file", LP_ERROR);
}

bool li_has_changed() {
	struct stat new_stat;

	if (stat(li_filename.c_str(), &new_stat) != 0) 
	{ 
		log("Can't stat(...) file", LP_ERROR);
		return false;
	}

	return (new_stat.st_mtime != li_file_stat.st_mtime) || (new_stat.st_mtim.tv_nsec != li_file_stat.st_mtim.tv_nsec);
}

void li_add(const noaftodo_entry& li_entry) {
	log("Adding " + li_entry.title + "...");
	t_list.push_back(li_entry);

	li_sort();
	if (li_autosave) li_save();

	da_send("A");
}

void li_comp(const int& entryID) {
	if ((entryID < 0) || (entryID >= t_list.size())) {
		log("li_comp: entry ID out of list bounds. Operation aborted", LP_ERROR);
		return;
	}

	t_list.at(entryID).completed = !t_list.at(entryID).completed;

	if (li_autosave) li_save();

	da_send("C");
}

void li_rem(const int& entryID) {
	if ((entryID < 0) || (entryID >= t_list.size())) {
		log("li_rem: entry ID out of list bounds. Operation aborted", LP_ERROR);
		return;
	}

	log("Removing " + t_list.at(entryID).title + "...");
	t_list.erase(t_list.begin() + entryID);

	if (li_autosave) li_save();

	da_send("R");
}

void li_sort() {
	std::sort(t_list.begin(), t_list.end(), less_than_noaftodo_entry_order());
}

void li_prepare(vector<noaftodo_entry>& list) {
	// identify entries
	for (auto& entry : list)
		if (entry.get_meta("eid") == "") entry.name();
	// sort entries
	std::sort(list.begin(), list.end(), less_than_noaftodo_entry());

	// clean useless tags
	for (int i = t_tags.size() - 1; i >= 0; i--) {
		if (t_tags.at(i) != to_string(i)) break;
		t_tags.erase(t_tags.begin() + i);
	}
}

int li_find(const string& eid) {
	for (int i = 0; i < t_list.size(); i++)
		if (t_list.at(i).get_meta("eid") == eid) return i;

	return -1;
}

bool li_tag_completed(const int& tagID) {
	for (const auto& e : t_list)
		if (((tagID == CUI_TAG_ALL) || (e.tag == tagID)) && (!e.completed)) return false;

	return true;
}

bool li_tag_coming(const int& tagID) {
	for (const auto& e : t_list)
		if (((tagID == CUI_TAG_ALL) || (e.tag == tagID)) && (e.is_coming())) return true;

	return false;
}

bool li_tag_failed(const int& tagID) {
	for (const auto& e : t_list)
		if (((tagID == CUI_TAG_ALL) || (e.tag == tagID)) && (e.is_failed())) return true;

	return false;
}
