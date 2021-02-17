#pragma once
#ifndef NOAF_HOOKS_CPP
#define NOAF_HOOKS_CPP

#include <functional>

namespace noaf {
	// program exit callback
	extern std::function<void(const int& code)> exit;

	// UI pause/resume functions
	namespace ui {
		extern std::function<void()> pause;
		extern std::function<void()> resume;
	}
}

#endif
