#ifndef NOAFTODO_DAEMON_H
#define NOAFTODO_DAEMON_H

#include <vector>

#include "noaftodo_list.h"

namespace da {

// notification messages titles
constexpr char M_TFAILED[] = "You have a failed task! Come on, it's never too late!";
constexpr char M_TCOMING[] = "You have an upcoming task!";
constexpr char M_TCOMPLETE[] = "You have completed a task!";
constexpr char M_TUNCOMPLETE[] = "Task is marked \"Not completed\" again!";
constexpr char M_TNEW[] = "You have a new task!";
constexpr char M_TREM[] = "Task removed!";

// message queue name
#ifdef NO_MQUEUE
constexpr char MQ_NAME[] = "/tmp/.noaftodo-queue";
#else
constexpr char MQ_NAME[] = "/noaftodo-msg-queue";
#endif

// lock file name
constexpr char LOCK_FILE[] = "/tmp/.noaftodo-dlock";

// message size
constexpr int MSGSIZE = 256;

// should daemon be automatically started when noaftodo is launched?
extern bool fork_autostart;

// daemon actions
extern std::string launch_action;
extern std::string task_failed_action;
extern std::string task_coming_action;
extern std::string task_completed_action;
extern std::string task_uncompleted_action;
extern std::string task_new_action;
extern std::string task_edited_action;
extern std::string task_removed_action;

// is daemon running?
extern bool running;

// running clients
extern int clients;

// cache
extern std::vector<li::entry> cache;
extern long cached_time;

// check interval
extern int interval;

void run();

void upd_cache(const bool& is_first_load = false);
void check_dues(const bool& renotify = false);

void kill();

void send(const char message[]);

void lock();
void unlock();
bool check_lockfile();

}
#endif
