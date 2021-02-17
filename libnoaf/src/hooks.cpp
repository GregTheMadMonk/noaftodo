#include "hooks.hpp"

using namespace std;

namespace noaf {
	function<void(const int&)> exit = [] (const int& code) {
		::exit(code);
	};

	namespace ui {
		function<void()> pause = [] () {};
		function<void()> resume = [] () {};
	}
}
