#include "noaftodo_daemon.h"

#include <ctime>
#include <fstream>
#include <string>
#include <mqueue.h>
#include <unistd.h>

#include "noaftodo.h"
#include "noaftodo_output.h"
#include "noaftodo_time.h"

using namespace std;

int da_interval = 1;

vector<noaftodo_entry> da_cache;
long da_cached_time = 0;

void da_run()
{
	// init cache
	if (da_check_lockfile())
	{
		log("Lockfile " + string(DA_LOCK_FILE) + " exists. If daemon is not running, you can delete it or run noaftodo -k.");
		return;
	} else {
		da_lock();
	}

	da_cache.clear();
	li_autosave = false;

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

	notify(string(TITLE) + " v." + VERSION, "Daemon is up and running");

	bool running = true;
	timespec tout;
	bool first = true;
	while (running)
	{
		// update cache
		li_load();
		for (int i = 0; i < t_list.size(); i++)
		{
			bool cached = false;
			int cached_id = -1;
			for (int j = 0; j < da_cache.size(); j++)
				if (da_cache.at(j).sim(t_list.at(i)))
				{
					cached = true;
					cached_id = j;
				}

			const noaftodo_entry e1 = t_list.at(i);

			if (!cached)
			{	// add to cache
				da_cache.push_back(t_list.at(i));

				if (e1.completed)
					notify(DA_M_TCOMPLETE, e1.title + ": " + e1.description, NP_LOW);
				else if (e1.due <= ti_to_long("a0d"))
					notify(DA_M_TFAILED, e1.title + ": " + e1.description, NP_HIGH);
				else if (e1.due <= ti_to_long("a1d"))
					notify(DA_M_TCOMING, e1.title + ": " + e1.description, NP_MID);
				else if (!first)
					notify(DA_M_TNEW, e1.title + ": " + e1.description);
			} else {
				const noaftodo_entry e2 = da_cache.at(cached_id);

				if (e1.completed != e2.completed)
				{
					if (e1.completed)
						notify(DA_M_TCOMPLETE, e1.title + ": " + e1.description, NP_LOW);
					else if (e1.due <= ti_to_long("a0d"))
						notify(DA_M_TFAILED, e1.title + ": " + e1.description, NP_HIGH);
					else if (e1.due <= ti_to_long("a1d"))\
						notify(DA_M_TCOMING, e1.title + ": " + e1.description, NP_MID);
					else
						notify(DA_M_TUNCOMPLETE, e1.title + ": " + e1.description, NP_MID);
				} else if (!e1.completed)
				{
					if (e1.due <= ti_to_long("a0d"))
					{
						if (e1.due > da_cached_time)
							notify(DA_M_TFAILED, e1.title + ": " + e1.description, NP_HIGH);
					} else if (e1.due <= ti_to_long("a1d"))
					{
						if (e1.due > ti_to_long(ti_to_tm(da_cached_time + ti_to_long("1d"))))
							notify(DA_M_TCOMING, e1.title + ": " + e1.description, NP_MID);
					}
				}

				da_cache[cached_id] = e1;
			}
		}

		// clear cache
		for (int i = 0; i < da_cache.size(); i++)
		{
			bool deleted = true;
			for (int j = 0; j < t_list.size(); j++)
				if (t_list.at(j).sim(da_cache.at(i))) deleted = false;

			if (deleted) 
			{
				notify(DA_M_TREM, da_cache.at(i).title + ": " + da_cache.at(i).description, NP_MID);
				da_cache.erase(da_cache.begin() + i);
				i--;
			}
		}

		first = false;
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
					running = false;
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
