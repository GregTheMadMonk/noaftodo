#ifndef NOAFTODO_CVAR_H
#define NOAFTODO_CVAR_H

#include <functional>
#include <map>
#include <string>

struct cvar_base_s	// a base structure for cvars
{
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

	operator int();
	operator std::string();
};

struct cvar_s : public cvar_base_s // a regular cvar
{
	std::string value;

	cvar_s(const std::string& def_val = "");
};

extern std::map<std::string, cvar_base_s*> cvars;
extern std::map<std::string, cvar_base_s*> cvars_predefined;

cvar_base_s& cvar(const std::string& name);
void cvar_erase(const std::string& name);
cvar_base_s& cvar_predefined(const std::string& name);

#endif
