#include "conarg.hpp"

#include <stdexcept>

using namespace std;

namespace noaf {

	map<vector<string>, conarg>& conarg::args() {
		static map<vector<string>, conarg> _conargs;
		return _conargs;
	}

	void conarg::parse(const int& argc, char* argv[]) {
		// scan the arguments
		for (int i = 0; i < argc; i++) for (auto it = args().begin(); it != args().end(); it++)
			// try if argument matches
			for (const auto& caller : it->first) if (caller == argv[i]) {
				// yep, it does - call a corresponding function
				vector<string> params;
				for (int j = 1; j <= it->second.param_count; j++)
					if (i + j >= argc) throw(std::out_of_range("Not enough arguments provided"));
					else params.push_back(argv[i + j]);

				it->second.callback(params);
				i += it->second.param_count;

				break;
			}
	}

}
