#pragma once
#ifndef NOAF_HOOKS_CPP
#define NOAF_HOOKS_CPP

#include <functional>

namespace noaf {
	// program exit callback
	extern std::function<void(const int& code)> exit;
}

#endif
