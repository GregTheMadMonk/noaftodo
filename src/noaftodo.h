#ifndef NOAFTODO_H
#define NOAFTODO_H

// progrm title and version
constexpr char TITLE[] = "NOAFtodo";

constexpr int LIST_V = 1;
constexpr int CONF_V = 2;
constexpr int MINOR_V = 2;
#define VERSION to_string(LIST_V) + "." + to_string(CONF_V) + "." + to_string(MINOR_V)

// program modes
constexpr int PM_DEFAULT = 0;
constexpr int PM_HELP = 1;
constexpr int PM_DAEMON = 2;

extern int run_mode;

void print_help();

#endif
