#pragma once
#ifndef NOAF_CVAR_H
#define NOAF_CVAR_H

#include <functional>
#include <map>
#include <memory>
#include <string>

namespace noaf {

	struct cvar {
		// smart cvar pointer
		typedef std::unique_ptr<cvar> cvar_p;
		// getter and setter function types
		typedef std::function<std::string()>			get_f;
		typedef std::function<void(const std::string& val)>	set_f;

		int flags = 0; // cvar flags

		// cvar flag masks
		constexpr static int READONLY	= 0b1;
		constexpr static int WS_IGNORE	= 0b10;
		constexpr static int NO_PREDEF	= 0b100;

		std::string predef_val = ""; // predefined value

		// setter and getter
		get_f getter = [] () { return std::string(""); };
		set_f setter = [] (const std::string& val) {};

		// variable description/help
		std::string tooltip = "";

		// constructors
		cvar() = default;
		cvar(const get_f& c_getter,
			const set_f& c_setter,
			const int& c_flags = 0,
			const std::string& predefine_val = "");
		// destructor
		virtual ~cvar() = default;

		// assignment
		cvar& operator=(const std::string& rval);
		cvar& operator=(const int& rval);
		cvar& operator=(const cvar& rval);

		// comparsement
		bool operator==(const std::string& rval);
		bool operator==(const int& rval);
		bool operator==(const cvar& rval);
		bool operator!=(const std::string& rval);
		bool operator!=(const int& rval);
		bool operator!=(const cvar& rval);

		void reset();					// reset cvar to a predefined value
		void predefine(const std::string& val = "");	// predefine a variable

		bool changed();	// is cvar value different from predefined

		// casters
		operator int();
		operator std::string();

		// returns an array of cvars
		static std::map<std::string, cvar_p>& cvars();
		static cvar& get(const std::string& name);

		// static members
		static void reset(const std::string& name);	// reset a named cvar
		static void erase(const std::string& name);	// removes cvar completely

		static bool deletable(const std::string& name);	// can a cvar be deleted?

		static cvar_p wrap_string(std::string& var,
					const int& flags = 0,
					const std::optional<get_f>& getter_override = {},
					const std::optional<set_f>& setter_override = {},
					const std::string& predefine_value = "");
		static cvar_p wrap_int(int& var,
					const int& flags = 0,
					const std::optional<get_f>& getter_override = {},
					const std::optional<set_f>& setter_override = {},
					const std::string& predefine_value = "");
		static cvar_p wrap_bool(bool& var,
					const int& flags = 0,
					const std::optional<get_f>& getter_override = {},
					const std::optional<set_f>& setter_override = {},
					const std::string& predefine_value = "");
		static cvar_p wrap_maskflag(int& mask, const int& flag,
					const int& flags = 0,
					const std::optional<get_f>& getter_override = {},
					const std::optional<set_f>& setter_override = {},
					const std::string& predefine_value = "");
	};

	class runtime_cvar : public cvar {
		std::string value;

		public:
		runtime_cvar(const std::string& define_val = "");
	};

}

#endif
