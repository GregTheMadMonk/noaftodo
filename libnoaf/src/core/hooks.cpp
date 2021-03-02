#include "hooks.hpp"

using namespace std;

namespace noaf {
	function<void(const int&)> on_exit = [] (const int& code) {
		::exit(code);
	};

	void exit(int code) { on_exit(code); }
}
