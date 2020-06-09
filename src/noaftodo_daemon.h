#ifndef NOAFTODO_DAEMON_H
#define NOAFTODO_DAEMON_H

#include <vector>

#include "noaftodo_list.h"

// notification messages titles
constexpr char DA_M_TFAILED[] = "You have a failed task! Come on, it's never too late!";
constexpr char DA_M_TCOMING[] = "You have an upcoming task!";
constexpr char DA_M_TCOMPLETE[] = "You have completed a task!";
constexpr char DA_M_TUNCOMPLETE[] = "Task is marked \"Not completed\" again!";
constexpr char DA_M_TNEW[] = "You have a new task!";
constexpr char DA_M_TREM[] = "Task removed!";

// message queue name
constexpr char DA_MQ_NAME[] = "/noaftodo-msg-queue";

// lock file name
constexpr char DA_LOCK_FILE[] = "/tmp/.noaftodo-dlock";

// message size
constexpr int DA_MSGSIZE = 256;

// is daemon running?
extern bool da_running;

// running clients
extern int da_clients;

// cache
extern std::vector<noaftodo_entry> da_cache;
extern long da_cached_time;

// check interval
extern int da_interval;

void da_run();

void da_upd_cache(const bool& is_first_load = false);
void da_check_dues(const bool& renotify = false);

void da_kill();

void da_send(const char message[]);

void da_lock();
void da_unlock();
bool da_check_lockfile();
#endif
