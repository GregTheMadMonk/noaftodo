#include "noaftodo_cvar.h"

#include <stdexcept>

using namespace std;

map<string, cvar_base_s*> cvars;
map<string, cvar_base_s*> cvars_predefined;

cvar_base_s& cvar_base_s::operator=(const string& rval)
{
	this->setter(rval);
	return *this;
}

cvar_base_s& cvar_base_s::operator=(const int& rval)
{
	this->setter(to_string(rval));
	return *this;
}

cvar_base_s& cvar_base_s::operator=(const cvar_base_s& rval)
{
	if (this != &rval) this->setter(rval.getter());

	return *this;
}

bool cvar_base_s::operator==(const string& rval)
{
	return (this->getter() == rval);
}

bool cvar_base_s::operator==(const int& rval)
{
	return (this->getter() == to_string(rval));
}

bool cvar_base_s::operator==(const cvar_base_s& rval)
{
	return (this->getter() == rval.getter());
}

bool cvar_base_s::operator!=(const string& rval)
{
	return (this->getter() != rval);
}

bool cvar_base_s::operator!=(const int& rval)
{
	return (this->getter() != to_string(rval));
}

bool cvar_base_s::operator!=(const cvar_base_s& rval)
{
	return (this->getter() != rval.getter());
}

cvar_base_s::operator int()
{
	try
	{
		return stoi(this->getter());
	} catch (const invalid_argument& e) {
		return 0;
	}
}

cvar_base_s::operator string()
{
	return this->getter();
}

cvar_s::cvar_s(const string& def_val)
{
	this->value = def_val;

	this->getter = [this] () { return this->value; };
	this->setter = [this] (const string& val) { this->value = val; };
}

cvar_base_s& cvar(const string& name)
{
	try
	{
		return *cvars.at(name);
	} catch (const out_of_range& e) {
		return *(cvars[name] = new cvar_s()); // otherwise cvars[name] would automatically create
							// cvar_base_s, which is basically empty
	}
}

void cvar_erase(const string& name)
{
	try
	{
		delete cvars.at(name);
		cvars.erase(name);
	} catch (const out_of_range& e) {}
}

cvar_base_s& cvar_predefined(const string& name)
{
	try
	{
		return *cvars_predefined.at(name);
	} catch (const out_of_range& e) {
		return *(cvars_predefined[name] = new cvar_s()); // otherwise cvars[name] would automatically create
							// cvar_base_s, which is basically empty
	}
}
