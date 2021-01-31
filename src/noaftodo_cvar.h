#ifndef NOAFTODO_CVAR_H
#define NOAFTODO_CVAR_H

#include <functional>
#include <map>
#include <memory>
#include <optional>
#include <string>

#include <noaftodo.h>

constexpr int CVAR_FLAG_RO = 0b1;
constexpr int CVAR_FLAG_WS_IGNORE = 0b10;
constexpr int CVAR_FLAG_NO_PREDEF = 0b100;

typedef std::function<std::string()> cvar_getter;
typedef std::function<void(const std::string& val)> cvar_setter;

struct cvar_base_s {	// a base structure for cvars
	int flags = 0;

	cvar_base_s() = default;
	cvar_base_s(const cvar_getter& c_getter,
			const cvar_setter& c_setter,
			const int& c_flags = 0,
			const std::string& predefine_val = "");
	virtual ~cvar_base_s() = default;

	std::string predef_val = ""; // predefined value

	cvar_getter getter = [] () { return std::string(""); };
	cvar_setter setter = [] (const std::string& val) { };

	cvar_base_s& operator=(const std::string& rval);
	cvar_base_s& operator=(const int& rval);
	cvar_base_s& operator=(const cvar_base_s& rval);

	bool operator==(const std::string& rval);
	bool operator==(const int& rval);
	bool operator==(const cvar_base_s& rval);

	bool operator!=(const std::string& rval);
	bool operator!=(const int& rval);
	bool operator!=(const cvar_base_s& rval);

	void reset();	// set value = predefined value
	void predefine(const std::string& val = "");	// set predefined value = value

	bool changed();

	operator int();
	operator std::string();

	// static members
	static std::map<std::string, std::unique_ptr<cvar_base_s>> cvars;

	static void reset(const std::string& name);
	static void erase(const std::string& name);

	static bool is_deletable(const std::string& name);

	static std::unique_ptr<cvar_base_s> wrap_string(std::string& var,
					const int& flags = 0,
					const std::optional<cvar_getter>& getter_override = {},
					const std::optional<cvar_setter>& setter_override = {},
					const std::string& predefine_value = "");
	static std::unique_ptr<cvar_base_s> wrap_multistr(multistr_c& var, const int& length = 1,
					const int& flags = 0,
					const std::optional<cvar_getter>& getter_override = {},
					const std::optional<cvar_setter>& setter_override = {},
					const std::string& predefine_value = "");
	static std::unique_ptr<cvar_base_s> wrap_multistr_element(multistr_c& var, const int& index,
					const int& flags = 0,
					const std::optional<cvar_getter>& getter_override = {},
					const std::optional<cvar_setter>& setter_override = {},
					const std::string& predefine_value = "");
	static std::unique_ptr<cvar_base_s> wrap_int(int& var,
					const int& flags = 0,
					const std::optional<cvar_getter>& getter_override = {},
					const std::optional<cvar_setter>& setter_override = {},
					const std::string& predefine_value = "");
	static std::unique_ptr<cvar_base_s> wrap_bool(bool& var,
					const int& flags = 0,
					const std::optional<cvar_getter>& getter_override = {},
					const std::optional<cvar_setter>& setter_override = {},
					const std::string& predefine_value = "");
	static std::unique_ptr<cvar_base_s> wrap_maskflag(int& mask, const int& flag,
					const int& flags = 0,
					const std::optional<cvar_getter>& getter_override = {},
					const std::optional<cvar_setter>& setter_override = {},
					const std::string& predefine_value = "");
};

struct cvar_s : public cvar_base_s { // a regular cvar
	std::string value;

	cvar_s(const std::string& def_val = "");
};

cvar_base_s& cvar(const std::string& name);

#endif
