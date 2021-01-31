#include "noaftodo_daemon.h"

#include <ctime>
#include <fstream>
#include <string>

#ifndef NO_MQUEUE
#include <mqueue.h>
#endif

#include <unistd.h>
#include <sys/stat.h>

#include <noaftodo.h>
#include <cmd.h>
#include <cui.h>
#include <cvar.h>
#include <config.h>
#include <entry_flags.h>

using namespace std;

using li::t_list;
using li::t_tags;

using namespace li::entry_flags;

using cmd::exec;

using cui::s_line;

namespace da {

bool running = false;
int clients = -1; // -1 if we don't care. Other numbers indicate
			// amount of active noaftodo clients

int interval = 1;

vector<li::entry> cache;
time_s cached_time;

void run() {
	// init cache
	if (check_lockfile()) {
		log("Lockfile " + string(LOCK_FILE) + " exists. If daemon is not running, you can delete it or run noaftodo -k.", LP_ERROR);
		return;
	} else {
		lock();
	}

	cache.clear();

#ifndef NO_MQUEUE
	log("Opening a message queue...");
	mq_attr attr;
	attr.mq_maxmsg = 10;
	attr.mq_msgsize = MSGSIZE;
	attr.mq_flags = 0;
	mqd_t mq = mq_open(MQ_NAME, O_CREAT | O_RDWR, S_IRUSR | S_IWUSR, &attr);

	if (mq == -1) {
		log("Failed!", LP_ERROR);
		return;
	}
	log("OK");
#endif

	exec(format_str(launch_action, nullptr, true));

	running = true;
	timespec tout;

	upd_cache(true);
	cached_time = time_s();

	while (running) {
		if (li::has_changed()) upd_cache();

		check_dues();

		cached_time = time_s();

#ifdef NO_MQUEUE
		string msg;
		vector<string> msgs;
		auto getmsg = [&msgs] () {
			ifstream q_in(MQ_NAME);

			if (!q_in.good()) return;

			string mes;

			while (getline(q_in, mes)) msgs.push_back(mes);

			q_in.close();

			remove(MQ_NAME);
		};

		getmsg();
#else
		char msg[MSGSIZE];

		clock_gettime(CLOCK_REALTIME, &tout);
		tout.tv_sec += interval;

		int status = mq_timedreceive(mq, msg, MSGSIZE, NULL, &tout);

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
					running = false;
					break;
				case 'N':
					check_dues(true);
					break;
				case 'C':
					upd_cache();
					break;
				case 'S':
					if (clients != -1) clients++;
					break;
				case 'D':
					if (clients != -1) {
						clients--;
						if (clients == 0) running = false;
					}
					break;
			}

#ifdef NO_MQUEUE
			getmsg();
#else
			status = mq_timedreceive(mq, msg, MSGSIZE, NULL, &tout);
#endif
		}

#ifdef NO_MQUEUE
		sleep(interval);
#endif
	}

#ifdef NO_MQUEUE
	log("Removing queue file (NO_MQUEUE)...");
	remove(MQ_NAME);
#else
	log("Closing message queue...");
	mq_close(mq);
	mq_unlink(MQ_NAME);
#endif

	unlock();
}

void upd_cache(const bool& is_first_load) {
	log("Updating daemon cache...", LP_IMPORTANT);
	li::load();

	const auto t_list_copy = t_list;

	for (int i = 0; i < t_list_copy.size(); i++) {
		s_line = i;
		int cached_id = -1;

		for (int j = 0; j < cache.size(); j++)
			if (cache.at(j).get_meta("eid") == t_list_copy.at(i).get_meta("eid")) {
				cached_id  = j;
				break;
			}

		li::entry li_e = t_list_copy.at(i);

		if (cached_id == -1)
		{	// add to cache
			cache.push_back(li_e);

			if (is_completed(li_e)) {
				if (li_e.get_meta("ignore_global_on_completed") != "true")
					exec(format_str(task_completed_action, &li_e, is_first_load));
			} else if (is_failed(li_e)) {
				if (li_e.get_meta("ignore_global_on_failed") != "true")
					exec(format_str(task_failed_action, &li_e, is_first_load));
			} else if (is_due(li_e)) {
				if (li_e.get_meta("ignore_global_on_due") != "true")
					exec(format_str(task_due_action, &li_e, is_first_load));
			} else if (is_coming(li_e)) {
				if (li_e.get_meta("ignore_global_on_coming") != "true")
					exec(format_str(task_coming_action, &li_e, is_first_load));
			} else if (!is_first_load) {
					exec(format_str(task_new_action, &li_e, is_first_load));
			}

			continue;
		}

		const li::entry ca_e = cache.at(cached_id);
		cache[cached_id] = li_e;

		if (ca_e == li_e) continue; // skip the unchanged entries

		// entry completion switched
		if (is_completed(ca_e) != is_completed(li_e)) {
			if (is_completed(li_e)) {
				if (li_e.get_meta("ignore_global_on_completed") != "true")
					exec(format_str(task_completed_action, &li_e, false));
				exec(format_str(li_e.get_meta("on_completed"), &li_e, false));
			} else if (is_failed(li_e)) {
				if (li_e.get_meta("ignore_global_on_failed") != "true")
					exec(format_str(task_failed_action, &li_e, true));
			} else if (is_due(li_e)) {
				if (li_e.get_meta("ignore_global_on_due") != "true")
					exec(format_str(task_due_action, &li_e, true));
			} else if (is_coming(li_e)) {
				if (li_e.get_meta("ignore_global_on_coming") != "true")
					exec(format_str(task_coming_action, &li_e, true));
			} else {
				if (li_e.get_meta("ignore_global_on_uncompleted") != "true")
					exec(format_str(task_uncompleted_action, &li_e, true));
				exec(format_str(li_e.get_meta("on_uncompleted"), &li_e, true));
			}
		}

		// a task was edited
		if ((ca_e.due != li_e.due) || (ca_e.tag != li_e.tag)
			       || (ca_e.title != li_e.title) || (ca_e.description != li_e.description))
		{
			if (li_e.get_meta("ignore_global_on_edited") != "true")
				exec(format_str(task_edited_action, &li_e, true));
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
	for (int i = 0; i < cache.size();) {
		bool removed = true;
		for (int j = 0; j < t_list.size(); j++)
			removed &= !t_list.at(j).sim(cache.at(i));

		if (removed) {
			if (cache.at(i).get_meta("ignore_global_on_removed") != "true")
				exec(format_str(task_removed_action, &cache.at(i)));
			cache.erase(cache.begin() + i);
		} else i++;
	}

	if (t_list != t_list_copy)  {
		li::save();
		upd_cache(); // I'm sorry
	}
}

void check_dues(const bool& renotify) {
	const auto t_list_copy = t_list;
	// check_dues supposes that the cache is up to date with the list
	// and cache and t_list contain the same entries
	for (s_line = 0; s_line < t_list.size(); s_line++)	 {
		if ((is_failed(t_list.at(s_line))) && (renotify || (!is_failed(t_list.at(s_line), cached_time)))) {
			if (t_list.at(s_line).get_meta("ignore_global_on_failed") != "true")
				exec(format_str(task_failed_action, &t_list.at(s_line), renotify));
			if (!renotify)
				exec(format_str(t_list.at(s_line).get_meta("on_failed"), &t_list.at(s_line)));
		}
		else if ((is_due(t_list.at(s_line))) && (renotify || (t_list.at(s_line).due > cached_time))) {
			if (t_list.at(s_line).get_meta("ignore_global_on_due") != "true")
				exec(format_str(task_due_action, &t_list.at(s_line), renotify));
			if (!renotify)
				exec(format_str(t_list.at(s_line).get_meta("on_due"), &t_list.at(s_line)));
		}
		else if ((is_coming(t_list.at(s_line))) && (renotify || (t_list.at(s_line).due > time_s(cached_time.fmt_cmd() + "a" + t_list.at(s_line).get_meta("warn_time", "1d"))))) {
			if (t_list.at(s_line).get_meta("ignore_global_on_coming") != "true")
				exec(format_str(task_coming_action, &t_list.at(s_line), renotify));
			if (!renotify)
				exec(format_str(t_list.at(s_line).get_meta("on_coming"), &t_list.at(s_line)));
		}
	}

	s_line = -1;

	if (t_list != t_list_copy)  {
		li::save();
		upd_cache(); // I'm sorry
	}
}

void kill() {
	log("Killing the daemon...");
	send("K");
	unlock();
#ifndef NO_MQUEUE
	mq_unlink(MQ_NAME);
#endif
}

void send(const char message[]) {
	if (!check_lockfile()) {
		log("Lock file not found. Run or restart the daemon. Message not sent!", LP_ERROR);
		return;
	}

#ifdef NO_MQUEUE
	log("Sending a message (NO_MQUEUE)...");
	ofstream q_file(MQ_NAME, std::ios_base::app);

	q_file << message << endl;

	if (q_file.good()) log("OK!");
	else log("Uh oh error", LP_ERROR);

	q_file.close();
#else
	log("Opening a message queue...");
	mq_attr attr;
	attr.mq_maxmsg = 10;
	attr.mq_msgsize = MSGSIZE;
	attr.mq_flags = 0;
	mqd_t mq = mq_open(MQ_NAME, O_CREAT | O_RDWR, S_IRUSR | S_IWUSR, &attr);

	if (mq == -1) {
		log("Failed!", LP_ERROR);
		return;
	}
	log("OK");

	const timespec timeout = { .tv_sec = 1 };
	int status = mq_timedsend(mq, message, MSGSIZE, 1, &timeout);

	if (status == -1)
		log("Uh oh no success :(", LP_ERROR);

	log("Closing message queue...");
	mq_close(mq);
#endif
}

void lock() {
	log("Creating lock file...");
	ofstream l_file(LOCK_FILE);

	l_file << getpid() << endl;

	if (l_file.good()) log("OK");
	else log("Uh oh error", LP_ERROR);

	l_file.close();
}

void unlock() {
	log("Removing lock file...");
	remove(LOCK_FILE);
}

bool check_lockfile() {
	ifstream l_file(LOCK_FILE);
	return l_file.good();
}

}
