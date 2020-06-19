#ifndef NOAFTODO_CVAR_H
#define NOAFTODO_CVAR_H

#include <functional>
#include <map>
#include <memory>
#include <string>

#include "noaftodo.h"

struct cvar_base_s	// a base structure for cvars
{
	virtual ~cvar_base_s() = default;

	std::string predef_val; // predefined value

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

	void reset();		// set value = predefined value
	void predefine();	// set predefined value = value

	bool changed();

	operator int();
	operator std::string();
};

struct cvar_s : public cvar_base_s // a regular cvar
{
	std::string value;

	cvar_s(const std::string& def_val = "");
};

extern std::map<std::string, std::unique_ptr<cvar_base_s>> cvars;

cvar_base_s& cvar(const std::string& name);
void cvar_reset(const std::string& name);
void cvar_erase(const std::string& name);

bool cvar_is_deletable(const std::string& name);

void cvar_wrap_string(const std::string& name, std::string& var, const bool& ronly = false);
void cvar_wrap_multistr(const std::string& name, multistr_c& var, const int& element_length = 1, const bool& ronly = false);
void cvar_wrap_int(const std::string& name, int& var, const bool& ronly = false);
void cvar_wrap_bool(const std::string& name, bool& var, const bool& ronly = false);
void cvar_wrap_maskflag(const std::string& name, int& mask, const int& flag, const bool& ronly = false);

#endif
