#pragma once
#ifndef NOAF_CARG_H
#define NOAF_CARG_H

#include <functional>
#include <map>
#include <string>
#include <vector>

namespace noaf {

	// Console argument struct. Couldd've been a template struct with
	// param_count being a template parameter and callback accepting
	// std::array<std::string, param_count>, but wwe want to tore all
	// these in a vector, so yeah here we are.
	struct conarg { // console argument
		std::function<void(const std::vector<std::string>&)> callback; // argument action
		int param_count; // expected number of parameters
		std::string tooltip; // argument tooltip

		static std::map<std::vector<std::string>, conarg>& args();
		static void parse(const int& argc, char* argv[]);
	};

}

#endif
