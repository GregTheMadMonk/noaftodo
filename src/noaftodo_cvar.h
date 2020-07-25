#ifndef NOAFTODO_CVAR_H
#define NOAFTODO_CVAR_H

#include <functional>
#include <map>
#include <memory>
#include <string>

#include "noaftodo.h"

constexpr int CVAR_FLAG_RO = 0b1;
constexpr int CVAR_FLAG_WS_IGNORE = 0b10;
constexpr int CVAR_FLAG_NO_PREDEF = 0b100;

struct cvar_base_s {	// a base structure for cvars
	int flags = 0;

	virtual ~cvar_base_s() = default;

	std::string predef_val = ""; // predefined value

	std::function<std::string()> getter = [] () { return std::string(""); };
	std::function<void(const std::string& val)> setter = [] (const std::string& val) { };

	cvar_base_s& operator=(const std::string& rval);
	cvar_base_s& operator=(const int& rval);
	cvar_base_s& operator=(const cvar_base_s& rval);

	bool operator==(const std::string& rval);
	bool operator==(const int& rval);
	bool operator==(const cvar_base_s& rval);

	bool operator!=(const std::string& rval);
	bool operator!=(const int& rval);
	bool operator!=(const cvar_base_s& rval);

	void reset();					// set value = predefined value
	void predefine(const std::string& val = "");	// set predefined value = value

	bool changed();

	operator int();
	operator std::string();

	// static members
	static std::map<std::string, std::unique_ptr<cvar_base_s>> cvars;

	static cvar_base_s& cvar(const std::string& name);
	static void reset(const std::string& name);
	static void erase(const std::string& name);

	static bool is_deletable(const std::string& name);

	static void wrap_string(const std::string& name, std::string& var, const int& flags = 0);
	static void wrap_multistr(const std::string& name, multistr_c& var, const int& length = 1, const int& flags = 0);
	static void wrap_multistr_element(const std::string& name, multistr_c& var, const int& index, const int& flags = 0);
	static void wrap_int(const std::string& name, int& var, const int& flags = 0);
	static void wrap_bool(const std::string& name, bool& var, const int& flags = 0);
	static void wrap_maskflag(const std::string& name, int& mask, const int& flag, const int& flags = 0);
};

struct cvar_s : public cvar_base_s { // a regular cvar
	std::string value;

	cvar_s(const std::string& def_val = "");
};

#endif
