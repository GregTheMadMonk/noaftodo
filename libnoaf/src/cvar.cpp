#include "cvar.hpp"

#include <stdexcept>

using namespace std;

namespace noaf {
	cvar::cvar(const get_f& c_getter,
		const set_f& c_setter,
		const int& c_flags,
		const std::string& predefine_val) :
		getter(c_getter),
		setter(c_setter),
		flags(c_flags),
		predef_val(predefine_val)
	{}

	cvar& cvar::operator=(const std::string& rval) {
		setter(rval);
		return *this;
	}
	cvar& cvar::operator=(const int& rval) {
		setter(to_string(rval));
		return *this;
	}
	cvar& cvar::operator=(const cvar& rval) {
		if (this != &rval) setter(rval.getter());
		return *this;
	}

	bool cvar::operator==(const string& rval) {
		return (getter() == rval);
	}
	bool cvar::operator==(const int& rval) {
		return (getter() == to_string(rval));
	}
	bool cvar::operator==(const cvar& rval) {
		return (getter() == rval.getter());
	}

	bool cvar::operator!=(const string& rval) {
		return (getter() != rval);
	}
	bool cvar::operator!=(const int& rval) {
		return (getter() != to_string(rval));
	}
	bool cvar::operator!=(const cvar& rval) {
		return (getter() != rval.getter());
	}

	void cvar::reset() {
		setter(predef_val);
	}

	void cvar::predefine(const string& val) {
		if (val != "") predef_val = val;
		else predef_val = getter();
	}

	bool cvar::changed() {
		return (getter() != predef_val);
	}

	cvar::operator int() {
		try {
			return stoi(getter());
		} catch (const invalid_argument& e) {
			return 0;
		}
	}

	cvar::operator string() {
		return getter();
	}

	map<string, cvar::cvar_p>& cvar::cvars() {
		static map<string, cvar::cvar_p> _cvars;
		return _cvars;
	}

	cvar& cvar::get(const string& name) {
		try {
			return *cvars().at(name);
		} catch (const out_of_range& e) {
			return *(cvars()[name] = make_unique<runtime_cvar>());
			// (have to explicitly set the value, or cvars()[name] would create
			// a `cvar` cvar instead of `runtime_cvar`)
		}
	}

	void cvar::reset(const string& name) {
		try {
			if (cvars().at(name)->predef_val != "") get(name).reset();
			else erase(name);
		} catch (const out_of_range& e) {}
	}

	void cvar::erase(const string& name) {
		if (deletable(name)) cvars().erase(name);
		else get(name) = "";
	}

	bool cvar::deletable(const string& name) {
		try {
			return (dynamic_cast<cvar*>(cvars().at(name).get()) != 0);
		} catch (const out_of_range& e) {
			return false;
		}
	}

	cvar::cvar_p cvar::wrap_string(string& var,
					const int& flags,
					const optional<cvar::get_f>& getter_override,
					const optional<cvar::set_f>& setter_override,
					const string& predefine_value) {
		auto ret = make_unique<cvar>(
				[&var] () { return var; },
				(flags & READONLY) ? (
					(set_f) [] (const string& val) { }
				) : (
					(set_f) [&var] (const string& val) { var = val; }
				),
				flags
			);

		if (getter_override != nullopt) ret->getter = getter_override.value();
		if (setter_override != nullopt) ret->setter = setter_override.value();
		if (predefine_value != "") ret->predefine(predefine_value);

		return ret;
	}

	cvar::cvar_p cvar::wrap_int(int& var,
					const int& flags,
					const optional<cvar::get_f>& getter_override,
					const optional<cvar::set_f>& setter_override,
					const string& predefine_value) {
		auto ret = make_unique<cvar>(
				[&var] () { return to_string(var); },
				(flags & READONLY) ? (
					(set_f) [] (const string& val) { }
				) : (
					(set_f) [&var] (const string& val) {
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

	cvar::cvar_p cvar::wrap_bool(bool& var,
					const int& flags,
					const optional<cvar::get_f>& getter_override,
					const optional<cvar::set_f>& setter_override,
					const string& predefine_value) {
		auto ret = make_unique<cvar>(
				[&var] () { return var ? "true" : "false"; },
				(flags & READONLY) ? (
					(set_f) [] (const string& val) { }
				) : (
					(set_f) [&var] (const string& val) {
						var = (val == "true");
					}
				),
				flags
			);

		if (getter_override != nullopt) ret->getter = getter_override.value();
		if (setter_override != nullopt) ret->setter = setter_override.value();
		if (predefine_value != "") ret->predefine(predefine_value);

		return ret;
	}

	cvar::cvar_p cvar::wrap_maskflag(int& mask, const int& flag,
					const int& flags,
					const optional<cvar::get_f>& getter_override,
					const optional<cvar::set_f>& setter_override,
					const string& predefine_value) {
		auto ret = make_unique<cvar>(
				[&mask, flag] () { return (mask & flag) ? "true" : "false"; },
				(flags & READONLY) ? (
					(set_f) [] (const string& val) { }
				) : (
					(set_f) [&mask, flag] (const string& val) {
						if (val == "true") mask |= flag;
						else mask -= mask & flag;
					}
				),
				flags
			);

		if (getter_override != nullopt) ret->getter = getter_override.value();
		if (setter_override != nullopt) ret->setter = setter_override.value();
		if (predefine_value != "") ret->predefine(predefine_value);

		return ret;
	}

	runtime_cvar::runtime_cvar(const string& define_val) {
		value = define_val;

		getter = [this] () { return this->value; };
		setter = [this] (const string& val) { this->value = val; };
	}
}
