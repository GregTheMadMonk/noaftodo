#include <csignal>
#include <iostream>

#include <cmd.hpp>
#include <hooks.hpp>
#include <log.hpp>

using namespace std;
using namespace noaf;

int main(int argc, char* argv[]) {
	setlocale(LC_ALL, "");

	signal(SIGINT, noaf::exit);
	signal(SIGTERM, noaf::exit);

	log << lver(0);

	cmd::cmds()["q"] = cmd::command([] (const vector<string>& args) { noaf::exit(); return ""; });

	string s;
	while (getline(cin, s)) {
		cmd::exec(s);
	}
	return 0;
}
