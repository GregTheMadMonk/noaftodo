#include "noaftodo_cvar.h"

#include <stdexcept>
#include <typeinfo>

using namespace std;

map<string, unique_ptr<cvar_base_s>> cvars;
map<string, unique_ptr<cvar_base_s>> cvars_predefined;

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

void cvar_base_s::reset()
{
	this->setter(this->predef_val);
}

void cvar_base_s::predefine()
{
	this->predef_val = this->getter();
}

bool cvar_base_s::changed()
{
	return (this->getter() != this->predef_val);
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
		return *(cvars[name] = make_unique<cvar_s>()); // otherwise cvars[name] would automatically create
							// cvar_base_s, which is basically empty
	}
}

void cvar_reset(const string& name)
{
	try { cvars.at(name); } catch (const out_of_range& e) { return; }

	if (cvar(name).predef_val != "")
		cvar(name).reset();
	else cvar_erase(name);
}

void cvar_erase(const string& name)
{
	if (cvar_is_deletable(name))
		cvars.erase(name);
	else
		cvar(name) = "";
}

bool cvar_is_deletable(const string& name) // allow deleting only cvar_s cvars
{
	try { cvars.at(name); } catch (const out_of_range& e) { return false; }
	return (dynamic_cast<cvar_s*>(cvars.at(name).get()) != 0);
}
