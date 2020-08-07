#include "noaftodo_cvar.h"

#include <stdexcept>
#include <typeinfo>

using namespace std;

map<string, unique_ptr<cvar_base_s>> cvar_base_s::cvars;

cvar_base_s& cvar_base_s::operator=(const string& rval) {
	this->setter(rval);
	return *this;
}

cvar_base_s& cvar_base_s::operator=(const int& rval) {
	this->setter(to_string(rval));
	return *this;
}

cvar_base_s& cvar_base_s::operator=(const cvar_base_s& rval) {
	if (this != &rval) this->setter(rval.getter());

	return *this;
}

bool cvar_base_s::operator==(const string& rval) {
	return (this->getter() == rval);
}

bool cvar_base_s::operator==(const int& rval) {
	return (this->getter() == to_string(rval));
}

bool cvar_base_s::operator==(const cvar_base_s& rval) {
	return (this->getter() == rval.getter());
}

bool cvar_base_s::operator!=(const string& rval) {
	return (this->getter() != rval);
}

bool cvar_base_s::operator!=(const int& rval) {
	return (this->getter() != to_string(rval));
}

bool cvar_base_s::operator!=(const cvar_base_s& rval) {
	return (this->getter() != rval.getter());
}

void cvar_base_s::reset() {
	this->setter(this->predef_val);
}

void cvar_base_s::predefine(const string& val) {
	if (val != "") this->predef_val = val;
	else this->predef_val = this->getter();
}

bool cvar_base_s::changed() {
	return (this->getter() != this->predef_val);
}

cvar_base_s::operator int() {
	try {
		return stoi(this->getter());
	} catch (const invalid_argument& e) {
		return 0;
	}
}

cvar_base_s::operator string() {
	return this->getter();
}

cvar_s::cvar_s(const string& def_val) {
	this->value = def_val;

	this->getter = [this] () { return this->value; };
	this->setter = [this] (const string& val) { this->value = val; };
}

cvar_base_s& cvar(const string& name) {
	try {
		return *cvar_base_s::cvars.at(name);
	} catch (const out_of_range& e) {
		return *(cvar_base_s::cvars[name] = make_unique<cvar_s>()); // otherwise cvars[name] would automatically create
							// cvar_base_s, which is basically empty
	}
}

void cvar_base_s::reset(const string& name) {
	try { cvars.at(name); } catch (const out_of_range& e) { return; }

	if (cvar(name).predef_val != "") cvar(name).reset();
	else erase(name);
}

void cvar_base_s::erase(const string& name) {
	if (is_deletable(name)) cvars.erase(name);
	else cvar(name) = "";
}

bool cvar_base_s::is_deletable(const string& name) { // allow deleting only cvar_s cvars
	try { cvars.at(name); } catch (const out_of_range& e) { return false; }
	return (dynamic_cast<cvar_s*>(cvars.at(name).get()) != 0);
}

void cvar_base_s::wrap_string(const string& name, string& var, const int& flags) {
	cvars[name] = make_unique<cvar_base_s>();
	cvars[name]->getter = [&var] () { return var; };

	if (flags & CVAR_FLAG_RO) cvars[name]->setter = [] (const string& val) { };
	else cvars[name]->setter = [&var] (const string& val) { var = val; };

	cvars[name]->flags = flags;
}

void cvar_base_s::wrap_multistr(const string& name, multistr_c& var, const int& length, const int& flags) {
	cvars[name] = make_unique<cvar_base_s>();
	cvars[name]->getter = [&var] () { return w_converter.to_bytes(var.str()); };
	if (flags & CVAR_FLAG_RO) cvars[name]->setter = [] (const string& val) { };
	else cvars[name]->setter = [name, &var, length] (const string& val) {
		int element_length = 1;
		if (val.length() > 0) if (val.at(0) == '{') {
			string buffer = "";
			for (int i = 1; i < val.length(); i++) {
				if (val.at(i) == '}') break;
				buffer += val.at(i);
			}

			try {
				element_length = stoi(buffer);
			} catch (const out_of_range& e) { }
		}
		var = multistr_c(val, element_length, length);
	};

	cvars[name]->flags = flags | CVAR_FLAG_WS_IGNORE; // otherwise it breaks a cvar
}

void cvar_base_s::wrap_multistr_element(const std::string& name, multistr_c& var, const int& index, const int& flags) {
	cvars[name] = make_unique<cvar_base_s>();
	cvars[name]->getter = [&var, index] () { return w_converter.to_bytes(var.at(index)); };
	if (flags & CVAR_FLAG_RO) cvars[name]->setter = [] (const string& val) { };
	else cvars[name]->setter = [&var, index] (const string& val) {
		int element_length = 1;
		int start_from = 0;
		if (val.length() > 0) if (val.at(0) == '{') {
			string buffer = "";
			for (int i = 1; i < val.length(); i++) {
				if (val.at(i) == '}') { start_from = i + 1; break; }
				buffer += val.at(i);
			}

			try {
				element_length = stoi(buffer);
			} catch (const invalid_argument& e) { }
		}

		vector<wstring> newval;

		wstring buffer = L"";
		wstring wval = w_converter.from_bytes(val.substr(start_from));
		for (int i = 0; i < wval.length(); i++) {
			buffer += wval.at(i);

			if (i % element_length == element_length - 1) {
				newval.push_back(buffer);
				buffer = L"";
			}
		}

		var.v_at(index) = newval;
	};

	cvars[name]->flags = flags | CVAR_FLAG_WS_IGNORE; // predefined multistr_c's are broken :)
}

void cvar_base_s::wrap_int(const string& name, int& var, const int& flags) {
	cvars[name] = make_unique<cvar_base_s>();
	cvars[name]->getter = [&var] () { return to_string(var); };

	if (flags & CVAR_FLAG_RO) cvars[name]->setter = [] (const string& val) { };
	else cvars[name]->setter = [&var] (const string& val) {
			try { var = stoi(val); } catch (const invalid_argument& e) {}
		};

	cvars[name]->flags = flags;
}

void cvar_base_s::wrap_bool(const string& name, bool& var, const int& flags) {
	cvars[name] = make_unique<cvar_base_s>();
	cvars[name]->getter = [&var] () { return var ? "true" : "false"; };

	if (flags & CVAR_FLAG_RO) cvars[name]->setter = [] (const string& val) { };
	else cvars[name]->setter = [&var] (const string& val) {
		if (((val == "true") && !var) || ((val == "false") && var))
			var = !var;
	};

	cvars[name]->flags = flags;
}

void cvar_base_s::wrap_maskflag(const string& name, int& mask, const int& flag, const int& flags) {
	cvars[name] = make_unique<cvar_base_s>();
	cvars[name]->getter = [&mask, flag] () { return (mask & flag) ? "true" : "false"; };

	if (flags & CVAR_FLAG_RO) cvars[name]->setter = [] (const string& val) { };
	else cvars[name]->setter = [&mask, flag] (const string& val) {
			if (((val == "true") && (mask ^ flag)) || ((val != "true") && (mask & flag)))
				mask ^= flag;
		};

	cvars[name]->flags = flags;
}
