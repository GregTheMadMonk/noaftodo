#include "cvar.h"

#include <stdexcept>
#include <typeinfo>

using namespace std;

map<string, unique_ptr<cvar_base_s>> cvar_base_s::cvars;

cvar_base_s::cvar_base_s(const cvar_getter& c_getter,
			const cvar_setter& c_setter,
			const int& c_flags,
			const string& predefine_val) : getter(c_getter), setter(c_setter), flags(c_flags) {
	if (predefine_val != "")
		this->predefine(predefine_val);
}

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

unique_ptr<cvar_base_s> cvar_base_s::wrap_string(string& var,
							const int& flags,
							const optional<cvar_getter>& getter_override,
							const optional<cvar_setter>& setter_override,
							const string& predefine_value) {
	auto ret = make_unique<cvar_base_s>(
			[&var] () { return var; },

			(flags & CVAR_FLAG_RO) ? (
				(cvar_setter) [] (const string& val) { }
			) : (
				(cvar_setter) [&var] (const string& val) { var = val; }
			),

			flags
		);

	if (getter_override != nullopt) ret->getter = getter_override.value();
	if (setter_override != nullopt) ret->setter = setter_override.value();
	if (predefine_value != "") ret->predefine(predefine_value);

	return ret;
}

unique_ptr<cvar_base_s> cvar_base_s::wrap_multistr(multistr_c& var, const int& length,
							const int& flags,
							const optional<cvar_getter>& getter_override,
							const optional<cvar_setter>& setter_override,
							const string& predefine_value) {
	auto ret = make_unique<cvar_base_s>(
			[&var] () { return w_converter.to_bytes(var.str()); },

			(flags & CVAR_FLAG_RO) ? (
				(cvar_setter) [] (const string& val) { }
			) : (
				(cvar_setter) [&var, length] (const string& val) {
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
				}
			),

			flags | CVAR_FLAG_WS_IGNORE // otherwise it breaks a cvar
		);

	if (getter_override != nullopt) ret->getter = getter_override.value();
	if (setter_override != nullopt) ret->setter = setter_override.value();
	if (predefine_value != "") ret->predefine(predefine_value);

	return ret;
}

unique_ptr<cvar_base_s> cvar_base_s::wrap_multistr_element(multistr_c& var, const int& index,
							const int& flags,
							const optional<cvar_getter>& getter_override,
							const optional<cvar_setter>& setter_override,
							const string& predefine_value) {
	auto ret = make_unique<cvar_base_s>(
			[&var, index] () { return w_converter.to_bytes(var.at(index)); },

			(flags & CVAR_FLAG_RO) ? (
				(cvar_setter) [] (const string& val) { }
			) : (
				(cvar_setter) [&var, index] (const string& val) {
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
				}
			),

			flags | CVAR_FLAG_WS_IGNORE // predefined multistr_c's are broken :)
		);

	if (getter_override != nullopt) ret->getter = getter_override.value();
	if (setter_override != nullopt) ret->setter = setter_override.value();
	if (predefine_value != "") ret->predefine(predefine_value);

	return ret;
}

unique_ptr<cvar_base_s> cvar_base_s::wrap_int(int& var,
							const int& flags,
							const optional<cvar_getter>& getter_override,
							const optional<cvar_setter>& setter_override,
							const string& predefine_value) {
	auto ret = make_unique<cvar_base_s>(
			[&var] () { return to_string(var); },

			(flags & CVAR_FLAG_RO) ? (
				(cvar_setter) [] (const string& val) { }
			) : (
				(cvar_setter) [&var] (const string& val) {
					try { var = stoi(val); } catch (const invalid_argument& e) {}
				}
			),

			flags
		);

	if (getter_override != nullopt) ret->getter = getter_override.value();
	if (setter_override != nullopt) ret->setter = setter_override.value();
	if (predefine_value != "") ret->predefine(predefine_value);

	return ret;
}

unique_ptr<cvar_base_s> cvar_base_s::wrap_bool(bool& var,
							const int& flags,
							const optional<cvar_getter>& getter_override,
							const optional<cvar_setter>& setter_override,
							const string& predefine_value) {
	auto ret = make_unique<cvar_base_s>(
			[&var] () { return var ? "true" : "false"; },
			(flags & CVAR_FLAG_RO) ? (
				(cvar_setter) [] (const string& val) { }
			) : (
				(cvar_setter) [&var] (const string& val) {
					if (((val == "true") && !var) || ((val == "false") && var))
						var = !var;
				}
			),

			flags
		);

	if (getter_override != nullopt) ret->getter = getter_override.value();
	if (setter_override != nullopt) ret->setter = setter_override.value();
	if (predefine_value != "") ret->predefine(predefine_value);

	return ret;
}

unique_ptr<cvar_base_s> cvar_base_s::wrap_maskflag(int& mask, const int& flag,
							const int& flags,
							const optional<cvar_getter>& getter_override,
							const optional<cvar_setter>& setter_override,
							const string& predefine_value) {
	auto ret = make_unique<cvar_base_s>(
			[&mask, flag] () { return (mask & flag) ? "true" : "false"; },
			
			(flags & CVAR_FLAG_RO) ? (
				(cvar_setter) [] (const string& val) { }
			) : (
				(cvar_setter) [&mask, flag] (const string& val) {
					if (((val == "true") && (mask ^ flag)) || ((val != "true") && (mask & flag)))
						mask ^= flag;
				}
			),

			flags
		);

	if (getter_override != nullopt) ret->getter = getter_override.value();
	if (setter_override != nullopt) ret->setter = setter_override.value();
	if (predefine_value != "") ret->predefine(predefine_value);

	return ret;
}
