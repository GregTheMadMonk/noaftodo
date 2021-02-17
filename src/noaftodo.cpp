#include <iostream>

#include <cmd.hpp>
#include <conarg.hpp>
#include <log.hpp>

using namespace std;
using namespace noaf;

int main(int argc, char* argv[]) {
	conarg::args()[{ "-h", "--help"}] = {
		[] (const vector<string>& params) {
			log << llev(2) << "Help message!!!" << lend;
		},
		0,
		""
	};

	log << lver(0);

	conarg::parse(argc, argv);

	string s;
	while (getline(cin, s)) {
		cmd::exec(s);
	}
	return 0;
}
