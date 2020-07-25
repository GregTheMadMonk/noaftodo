#include "noaftodo_daemon.h"

#include <ctime>
#include <fstream>
#include <string>

#ifndef NO_MQUEUE
#include <mqueue.h>
#endif

#include <unistd.h>
#include <sys/stat.h>

#include "noaftodo.h"
#include "noaftodo_cmd.h"
#include "noaftodo_cui.h"
#include "noaftodo_cvar.h"
#include "noaftodo_config.h"
#include "noaftodo_time.h"

using namespace std;

using li::t_list;
using li::t_tags;

using cmd::exec;

using cui::s_line;

bool da_fork_autostart = true;

string da_launch_action;
string da_task_failed_action;
string da_task_coming_action;
string da_task_completed_action;
string da_task_uncompleted_action;
string da_task_new_action;
string da_task_edited_action;
string da_task_removed_action;

bool da_running = false;
int da_clients = -1; // -1 if we don't care. Other numbers indicate
			// amount of active noaftodo clients

int da_interval = 1;

vector<li::entry> da_cache;
long da_cached_time = 0;

void da_run() {
	// init cache
	if (da_check_lockfile()) {
		log("Lockfile " + string(DA_LOCK_FILE) + " exists. If daemon is not running, you can delete it or run noaftodo -k.", LP_ERROR);
		return;
	} else {
		da_lock();
	}

	da_cache.clear();

#ifndef NO_MQUEUE
	log("Opening a message queue...");
	mq_attr attr;
	attr.mq_maxmsg = 10;
	attr.mq_msgsize = DA_MSGSIZE;
	attr.mq_flags = 0;
	mqd_t mq = mq_open(DA_MQ_NAME, O_CREAT | O_RDWR, S_IRUSR | S_IWUSR, &attr);

	if (mq == -1) {
		log("Failed!", LP_ERROR);
		return;
	}
	log("OK");
#endif

	exec(format_str(da_launch_action, nullptr, true));

	da_running = true;
	timespec tout;

	da_upd_cache(true);
	da_cached_time = ti_to_long("a0d");

	while (da_running) {
		if (li::has_changed()) da_upd_cache();

		da_check_dues();

		da_cached_time = ti_to_long("a0d");

#ifdef NO_MQUEUE
		string msg;
		vector<string> msgs;
		auto getmsg = [&msgs] () {
			ifstream q_in(DA_MQ_NAME);

			if (!q_in.good()) return;

			string mes;

			while (getline(q_in, mes)) msgs.push_back(mes);

			q_in.close();

			remove(DA_MQ_NAME);
		};

		getmsg();
#else
		char msg[DA_MSGSIZE];

		clock_gettime(CLOCK_REALTIME, &tout);
		tout.tv_sec += da_interval;

		int status = mq_timedreceive(mq, msg, DA_MSGSIZE, NULL, &tout);

		clock_gettime(CLOCK_REALTIME, &tout);
#endif

#ifdef NO_MQUEUE
		while (msgs.size() > 0) {
#else
		while (status >= 0) {
#endif
#ifdef NO_MQUEUE
			msg = msgs.at(0);
			msgs.erase(msgs.begin());
#endif
			log(msg, LP_IMPORTANT);

			switch (msg[0]) {
				case 'K':
					da_running = false;
					break;
				case 'N':
					da_check_dues(true);
					break;
				case 'C':
					da_upd_cache();
					break;
				case 'S':
					if (da_clients != -1) da_clients++;
					break;
				case 'D':
					if (da_clients != -1) {
						da_clients--;
						if (da_clients == 0) da_running = false;
					}
					break;
			}

#ifdef NO_MQUEUE
			getmsg();
#else
			status = mq_timedreceive(mq, msg, DA_MSGSIZE, NULL, &tout);
#endif
		}

#ifdef NO_MQUEUE
		sleep(da_interval);
#endif
	}

#ifdef NO_MQUEUE
	log("Removing queue file (NO_MQUEUE)...");
	remove(DA_MQ_NAME);
#else
	log("Closing message queue...");
	mq_close(mq);
	mq_unlink(DA_MQ_NAME);
#endif

	da_unlock();
}

void da_upd_cache(const bool& is_first_load) {
	log("Updating daemon cache...", LP_IMPORTANT);
	li::load();

	const auto t_list_copy = t_list;

	for (int i = 0; i < t_list_copy.size(); i++) {
		s_line = i;
		int cached_id = -1;

		for (int j = 0; j < da_cache.size(); j++)
			if (da_cache.at(j).get_meta("eid") == t_list_copy.at(i).get_meta("eid")) {
				cached_id  = j;
				break;
			}

		li::entry li_e = t_list_copy.at(i);

		if (cached_id == -1)
		{	// add to cache
			da_cache.push_back(li_e);

			if (li_e.completed) {
				if (li_e.get_meta("ignore_global_on_completed") != "true")
					exec(format_str(da_task_completed_action, &li_e, is_first_load));
			} else if (li_e.is_failed()) {
				if (li_e.get_meta("ignore_global_on_failed") != "true")
					exec(format_str(da_task_failed_action, &li_e, is_first_load));
			} else if (li_e.is_coming()) {
				if (li_e.get_meta("ignore_global_on_coming") != "true")
					exec(format_str(da_task_coming_action, &li_e, is_first_load));
			} else if (!is_first_load) {
					exec(format_str(da_task_new_action, &li_e, is_first_load));
			}

			continue;
		}

		const li::entry ca_e = da_cache.at(cached_id);
		da_cache[cached_id] = li_e;

		if (ca_e == li_e) continue; // skip the unchanged entries

		// entry completion switched
		if (ca_e.completed != li_e.completed) {
			if (li_e.completed) {
				if (li_e.get_meta("ignore_global_on_completed") != "true")
					exec(format_str(da_task_completed_action, &li_e, false));
				exec(format_str(li_e.get_meta("on_completed"), &li_e, false));
			} else if (li_e.is_failed()) {
				if (li_e.get_meta("ignore_global_on_failed") != "true")
					exec(format_str(da_task_failed_action, &li_e, true));
			} else if (li_e.is_coming()) {
				if (li_e.get_meta("ignore_global_on_coming") != "true")
					exec(format_str(da_task_coming_action, &li_e, true));
			} else {
				if (li_e.get_meta("ignore_global_on_uncompleted") != "true")
					exec(format_str(da_task_uncompleted_action, &li_e, true));
				exec(format_str(li_e.get_meta("on_uncompleted"), &li_e, true));
			}
		}

		// a task was edited
		if ((ca_e.due != li_e.due) || (ca_e.tag != li_e.tag)
			       || (ca_e.title != li_e.title) || (ca_e.description != li_e.description))
		{
			if (li_e.get_meta("ignore_global_on_edited") != "true")
				exec(format_str(da_task_edited_action, &li_e, true));
			exec(format_str(li_e.get_meta("on_edited"), &li_e, true));
		}

		// something else has changed
		//
		// ...
		//
		// we don't care yet lol
	}

	s_line = -1;

	// clear cache from deleted tasks
	for (int i = 0; i < da_cache.size();) {
		bool removed = true;
		for (int j = 0; j < t_list.size(); j++)
			removed &= !t_list.at(j).sim(da_cache.at(i));

		if (removed) {
			if (da_cache.at(i).get_meta("ignore_global_on_removed") != "true")
				exec(format_str(da_task_removed_action, &da_cache.at(i)));
			da_cache.erase(da_cache.begin() + i);
		} else i++;
	}

	if (t_list != t_list_copy)  {
		li::save();
		da_upd_cache(); // I'm sorry
	}
}

void da_check_dues(const bool& renotify) {
	const auto t_list_copy = t_list;
	// da_check_dues supposes that the cache is up to date with the list
	// and da_cache and t_list contain the same entries
	for (s_line = 0; s_line < t_list.size(); s_line++)	 {
		if ((t_list.at(s_line).is_failed()) && (renotify || (t_list.at(s_line).due > da_cached_time))) {
			if (t_list.at(s_line).get_meta("ignore_global_on_failed") != "true")
				exec(format_str(da_task_failed_action, &t_list.at(s_line), renotify));
			if (!renotify)
				exec(format_str(t_list.at(s_line).get_meta("on_failed"), &t_list.at(s_line)));
		}
		else if ((t_list.at(s_line).is_coming()) && (renotify || (t_list.at(s_line).due > ti_to_long(ti_cmd_str(da_cached_time) + "a" + t_list.at(s_line).get_meta("warn_time", "1d"))))) {
			if (t_list.at(s_line).get_meta("ignore_global_on_coming") != "true")
				exec(format_str(da_task_coming_action, &t_list.at(s_line), renotify));
			if (!renotify)
				exec(format_str(t_list.at(s_line).get_meta("on_coming"), &t_list.at(s_line)));
		}
	}

	s_line = -1;

	if (t_list != t_list_copy)  {
		li::save();
		da_upd_cache(); // I'm sorry
	}
}

void da_kill() {
	log("Killing the daemon...");
	da_send("K");
	da_unlock();
#ifndef NO_MQUEUE
	mq_unlink(DA_MQ_NAME);
#endif
}

void da_send(const char message[]) {
	if (!da_check_lockfile()) {
		log("Lock file not found. Run or restart the daemon. Message not sent!", LP_ERROR);
		return;
	}

#ifdef NO_MQUEUE
	log("Sending a message (NO_MQUEUE)...");
	ofstream q_file(DA_MQ_NAME, std::ios_base::app);

	q_file << message << endl;

	if (q_file.good()) log("OK!");
	else log("Uh oh error", LP_ERROR);

	q_file.close();
#else
	log("Opening a message queue...");
	mq_attr attr;
	attr.mq_maxmsg = 10;
	attr.mq_msgsize = DA_MSGSIZE;
	attr.mq_flags = 0;
	mqd_t mq = mq_open(DA_MQ_NAME, O_CREAT | O_RDWR, S_IRUSR | S_IWUSR, &attr);

	if (mq == -1) {
		log("Failed!", LP_ERROR);
		return;
	}
	log("OK");

	const timespec timeout = { .tv_sec = 1 };
	int status = mq_timedsend(mq, message, DA_MSGSIZE, 1, &timeout);

	if (status == -1)
		log("Uh oh no success :(", LP_ERROR);

	log("Closing message queue...");
	mq_close(mq);
#endif
}

void da_lock() {
	log("Creating lock file...");
	ofstream l_file(DA_LOCK_FILE);

	l_file << getpid() << endl;

	if (l_file.good()) log("OK");
	else log("Uh oh error", LP_ERROR);

	l_file.close();
}

void da_unlock() {
	log("Removing lock file...");
	remove(DA_LOCK_FILE);
}

bool da_check_lockfile() {
	ifstream l_file(DA_LOCK_FILE);
	return l_file.good();
}
