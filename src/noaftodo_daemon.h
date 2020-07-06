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
#ifdef NO_MQUEUE
constexpr char DA_MQ_NAME[] = "/tmp/.noaftodo-queue";
#else
constexpr char DA_MQ_NAME[] = "/noaftodo-msg-queue";
#endif

// lock file name
constexpr char DA_LOCK_FILE[] = "/tmp/.noaftodo-dlock";

// message size
constexpr int DA_MSGSIZE = 256;

// should daemon be automatically started when noaftodo is launched?
extern bool da_fork_autostart;

// daemon actions
extern std::string da_launch_action;
extern std::string da_task_failed_action;
extern std::string da_task_coming_action;
extern std::string da_task_completed_action;
extern std::string da_task_uncompleted_action;
extern std::string da_task_new_action;
extern std::string da_task_edited_action;
extern std::string da_task_removed_action;

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
