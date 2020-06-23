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

void cvar_wrap_string(const string& name, string& var, const bool& ronly)
{
	cvars[name] = make_unique<cvar_base_s>();
	cvars[name]->getter = [&var] () { return var; };

	if (ronly) cvars[name]->setter = [] (const string& val) { };
	else cvars[name]->setter = [&var] (const string& val) { var = val; };
}

void cvar_wrap_multistr(const string& name, multistr_c& var, const int& length, const bool& ronly)
{
	cvars[name] = make_unique<cvar_base_s>();
	cvars[name]->getter = [&var] () { return w_converter.to_bytes(var.str()); };
	if (ronly) cvars[name]->setter = [] (const string& val) { };
	else cvars[name]->setter = [name, &var, length] (const string& val)
	{
		var = multistr_c(val, length);
	};

	cvars[name]->ws_ignore = true; // otherwise it breaks a cvar
}

void cvar_wrap_multistr_element(const std::string& name, multistr_c& var, const int& index, const bool& ronly)
{
	cvars[name] = make_unique<cvar_base_s>();
	cvars[name]->getter = [&var, index] () { return w_converter.to_bytes(var.at(index)); };
	if (ronly) cvars[name]->setter = [] (const string& val) { };
	else cvars[name]->setter = [&var, index] (const string& val)
	{
		vector<wchar_t> newval;

		for (wchar_t c : w_converter.from_bytes(val))
			newval.push_back(c);

		var.v_at(index) = newval;
	};

	cvars[name]->ws_ignore = true; // predefined multistr_c's are broken :)
}

void cvar_wrap_int(const string& name, int& var, const bool& ronly)
{
	cvars[name] = make_unique<cvar_base_s>();
	cvars[name]->getter = [&var] () { return to_string(var); };

	if (ronly) cvars[name]->setter = [] (const string& val) { };
	else cvars[name]->setter = [&var] (const string& val)
		{
			try { var = stoi(val); } catch (const invalid_argument& e) {}
		};
}

void cvar_wrap_bool(const string& name, bool& var, const bool& ronly)
{
	cvars[name] = make_unique<cvar_base_s>();
	cvars[name]->getter = [&var] () { return var ? "true" : "false"; };

	if (ronly) cvars[name]->setter = [] (const string& val) { };
	else cvars[name]->setter = [&var] (const string& val)
	{
		if (((val == "true") && !var) || ((val == "false") && var))
			var = !var;
	};
}

void cvar_wrap_maskflag(const string& name, int& mask, const int& flag, const bool& ronly)
{
	cvars[name] = make_unique<cvar_base_s>();
	cvars[name]->getter = [&mask, flag] () { return (mask & flag) ? "true" : "false"; };

	if (ronly) cvars[name]->setter = [] (const string& val) { };
	else cvars[name]->setter = [&mask, flag] (const string& val)
		{
			if (((val == "true") && (mask ^ flag)) || ((val != "true") && (mask & flag)))
				mask ^= flag;
		};
}
