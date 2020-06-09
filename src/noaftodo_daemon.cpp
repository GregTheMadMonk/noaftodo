#include "noaftodo_daemon.h"

#include <ctime>
#include <fstream>
#include <string>
#include <mqueue.h>
#include <unistd.h>
#include <sys/stat.h>

#include "noaftodo.h"
#include "noaftodo_cmd.h"
#include "noaftodo_cui.h"
#include "noaftodo_config.h"
#include "noaftodo_io.h"
#include "noaftodo_time.h"

using namespace std;

bool da_running = false;

int da_interval = 1;

vector<noaftodo_entry> da_cache;
long da_cached_time = 0;

void da_run()
{
	// init cache
	if (da_check_lockfile())
	{
		log("Lockfile " + string(DA_LOCK_FILE) + " exists. If daemon is not running, you can delete it or run noaftodo -k.", LP_IMPORTANT);
		return;
	} else {
		da_lock();
	}

	da_cache.clear();

	log("Opening a message queue...");
	mq_attr attr;
	attr.mq_maxmsg = 10;
	attr.mq_msgsize = DA_MSGSIZE;
	attr.mq_flags = 0;
	mqd_t mq = mq_open(DA_MQ_NAME, O_CREAT | O_RDWR, S_IRUSR | S_IWUSR, &attr);

	if (mq == -1)
	{
		log("Failed!", LP_ERROR);
		return;
	}
	log("OK");

	cmd_exec(format_str(conf_get_cvar("on_daemon_launch_action"), {}));

	da_running = true;
	timespec tout;

	da_upd_cache(true);
	da_cached_time = ti_to_long("a0d");

	for (int ticks = 0; da_running; ticks++)
	{
		if (ticks == conf_get_cvar_int("daemon.list_upd_ticks")) 
		{
			ticks = 0;
			da_upd_cache();
		}

		da_check_dues();

		da_cached_time = ti_to_long("a0d");

		char msg[DA_MSGSIZE];

		clock_gettime(CLOCK_REALTIME, &tout);
		tout.tv_sec += da_interval;

		int status = mq_timedreceive(mq, msg, DA_MSGSIZE, NULL, &tout);

		clock_gettime(CLOCK_REALTIME, &tout);
		while (status >= 0)
		{
			log(msg);

			switch (msg[0])
			{
				case 'K':
					da_running = false;
					break;
				case 'N':
					da_check_dues(true);
					break;
				case 'C':
					da_upd_cache();
					ticks = 0;
					break;
			}

			status = mq_timedreceive(mq, msg, DA_MSGSIZE, NULL, &tout);
		}
	}

	log("Closing message queue...");
	mq_close(mq);
	mq_unlink(DA_MQ_NAME);

	da_unlock();
}

void da_upd_cache(const bool& is_first_load)
{
	li_load();

	for (int i = 0; i < t_list.size(); i++)
	{
		int cached_id = -1;

		for (int j = 0; j < da_cache.size(); j++)
			if (da_cache.at(j).sim(t_list.at(i)))
			{
				cached_id  = j;
				break;
			}

		const noaftodo_entry li_e = t_list.at(i);

		if (cached_id == -1) 
		{	// add to cache
			da_cache.push_back(li_e);

			if (li_e.completed)
				cmd_exec(format_str(conf_get_cvar("on_task_completed_action"), li_e, is_first_load));
			else if (li_e.is_failed())
				cmd_exec(format_str(conf_get_cvar("on_task_failed_action"), li_e, is_first_load));
			else if (li_e.is_coming())
				cmd_exec(format_str(conf_get_cvar("on_task_coming_action"), li_e, is_first_load));
			else if (!is_first_load)
				cmd_exec(format_str(conf_get_cvar("on_task_new_action"), li_e, is_first_load));

			continue;
		}

		const noaftodo_entry ca_e = da_cache.at(cached_id);
		da_cache[cached_id] = li_e;

		if (ca_e == li_e) continue; // skip the unchanged entries

		// entry completion switched
		if (ca_e.completed != li_e.completed)
		{
			if (li_e.completed)
				cmd_exec(format_str(conf_get_cvar("on_task_completed_action"), li_e, false));
			else if (li_e.is_failed())
				cmd_exec(format_str(conf_get_cvar("on_task_failed_action"), li_e, true));
			else if (li_e.is_coming())
				cmd_exec(format_str(conf_get_cvar("on_task_coming_action"), li_e, true));

			continue;
		}

		// something else has changed
		//
		// ...
		//
		// we don't care yet lol
	}

	cui_s_line = -1;

	// clear cache from deleted tasks
	for (int i = 0; i < da_cache.size();)
	{
		bool removed = true;
		for (int j = 0; j < t_list.size(); j++)
			removed &= !t_list.at(j).sim(da_cache.at(i));

		if (removed)
		{
			cmd_exec(format_str(conf_get_cvar("on_task_removed_action"), da_cache.at(i)));
			da_cache.erase(da_cache.begin() + i);
		} else i++;
	}
}

void da_check_dues(const bool& renotify)
{
	// da_check_dues supposes that the cache is up to date with the list
	// and da_cache and t_list contain the same entries
	for (cui_s_line = 0; cui_s_line < t_list.size(); cui_s_line++)	
	{
		if ((t_list.at(cui_s_line).is_failed()) && (renotify || (t_list.at(cui_s_line).due > da_cached_time)))
			cmd_exec(format_str(conf_get_cvar("on_task_failed_action"), t_list.at(cui_s_line), renotify));
		else if ((t_list.at(cui_s_line).is_coming()) && (renotify || (t_list.at(cui_s_line).due > ti_to_long(ti_cmd_str(da_cached_time) + "a1d"))))
			cmd_exec(format_str(conf_get_cvar("on_task_coming_action"), t_list.at(cui_s_line), renotify));
	}

	cui_s_line = -1;
}

void da_kill()
{
	log("Killing the daemon...");
	da_send("K");
	da_unlock();
	mq_unlink(DA_MQ_NAME);
}

void da_send(const char message[])
{
	if (!da_check_lockfile())
	{
		log("Lock file not found. Run or restart the daemon. Message not sent!", LP_ERROR);
		return;
	}

	log("Opening a message queue...");
	mq_attr attr;
	attr.mq_maxmsg = 10;
	attr.mq_msgsize = DA_MSGSIZE;
	attr.mq_flags = 0;
	mqd_t mq = mq_open(DA_MQ_NAME, O_CREAT | O_RDWR, S_IRUSR | S_IWUSR, &attr);

	if (mq == -1)
	{
		log("Failed!", LP_ERROR);
		return;
	}
	log("OK");

	int status = mq_send(mq, message, DA_MSGSIZE, 1);

	if (status == -1)
		log("Uh oh no success :(", LP_ERROR);

	log("Closing message queue...");
	mq_close(mq);
}

void da_lock()
{
	log("Creating lock file...");
	ofstream l_file(DA_LOCK_FILE);

	l_file << getpid() << endl;

	l_file.close();
	if (l_file.good()) log("OK");
	else log("Uh oh error", LP_ERROR);
}

void da_unlock()
{
	log("Removing lock file...");
	remove(DA_LOCK_FILE);
}

bool da_check_lockfile()
{
	ifstream l_file(DA_LOCK_FILE);
	return l_file.good();
}
