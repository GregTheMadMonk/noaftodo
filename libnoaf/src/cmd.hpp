#pragma once
#ifndef NOAF_CMD_H
#define NOAF_CMD_H

#include <functional>
#include <map>
#include <stack>
#include <stdexcept>
#include <string>
#include <vector>

#include <cmd/token.hpp>

namespace noaf::cmd {

	/*
	 * *in drunk voice*
	 * OK so HEAR ME OUT EVERYONE I've got this HUGE idea
	 * for NOAF interpreter that's NOT gonna suck as all my
	 * previous attempts to do this did so yeahh sooooo
	 * baasically there's gonna be to moddes for the interpreter
	 * with their own tokens and rules so one is command mode
	 * which reads your command from the first "word" of input
	 * and *burp* passes the arguments to it but there's another
	 * that's called expression mode that's instead of running a
	 * command performs arithmetic operations and such and basically
	 * there's gonna be operators that perform stuff and which mode
	 * to use is determined by the first word of input like if you
	 * write a command nme it's command mode and if you write a
	 * number or operator it's eval mode and if you write something
	 * that's neither you get a message like wtf are you like 8 and
	 * like not years but months because I can't understand wwhat
	 * you're typing an its an error and user pc explodes (ideally)
	 * and theres also ZOMBIES and you can SHOOT them with a
	 * SHOGUN and it's all gonna be soooo awesome can't wait to spend
	 * a couple of weeks slowly turning this iddea into an abomination
	 * because I've never studied CS and not going to in my next
	 * several lifetimes
	 * There's also a list mode, but it's not as cool it just makes
	 * the same command execute for each item in the list basically a
	 * for loop but without writing for and shit since we're making
	 * this language primarily for typing commands in the program and
	 * you want them to be as short and straight-forward as possible
	 * Peace
	 */

	// interpreter modes
	enum mode_type {
		MNONE,
		BASE,
		EXPRESSION,
		LIST
	};

	// interpreter runtime exceptions
	const std::runtime_error wrong_mode("Invalid mode!");
	const std::runtime_error wrong_token("Unexpected token!");
	const std::runtime_error token_not_found("Can't find next token in the string!");
	const std::runtime_error lose_reference("Lose REFERENCE token!");
	const std::runtime_error few_arguments("Not enoguh arguments!");
	const std::runtime_error unterminated_mode("Unterminated mode!");
	const std::runtime_error wrong_arg_type("Wrong argument type!");

	// command struct
	struct command {
		typedef std::function<std::string(const std::vector<std::string>& args)> cmd_cb;
		cmd_cb callback;	// command callback
		std::string usage;	// usage tempalte
		std::string tooltip;	// command tooltip

		command() = default;
		command(const cmd_cb& cb, const std::string& u = "", const std::string& t = "");
	};
	// commands list
	std::map<std::string, command>& cmds();

	// list expansion from the dump
	std::vector<std::string> read_list(std::stack<token>& dump);

	// reads the last command
	typedef std::vector<std::vector<std::string>> listdb;
	typedef std::vector<token> track;

	// track expander. All lists in the track shoul be scanned to determine all
	// possible substitutions
	typedef std::vector<std::vector<std::pair<size_t, std::string>>> substitutor;
	typedef std::map<size_t, size_t> list_indexer;
	substitutor get_subst(const list_indexer& li, const list_indexer::iterator& where, const listdb& what);

	// reads the command from the end to the first command starting token + implaces lists
	std::vector<track> read_command(std::stack<token>& dump, const listdb& lists, const std::stack<mode_type>& mode);

	// append a set of "tracks" to an existing one
	std::vector<track> tracks_append(const std::vector<track>& base, const std::vector<track>& more);

	/*
	 * A little remark: yes, I know that read_list, read_command and get_subst
	 * could've been a lambda expression inide precomp, but they're just
		 ___  ___    _ __ ___   __ _ ___ ___(_)_   _____
		/ __|/ _ \  | '_ ` _ \ / _` / __/ __| \ \ / / _ \
		\__ \ (_) | | | | | | | (_| \__ \__ \ |\ V /  __/
		|___/\___/  |_| |_| |_|\__,_|___/___/_| \_/ \___|
	* that I really don't think that putting them iside another method is a good
	* idea even at cost of passing some arguments that would've been captured
	* in a lambda
	*/

	std::string call(const track& cmdline);
	std::string eval(track cmdline);

	// lexer (forgive me my terminology, I don't know shit about CS)
	track lex(const std::string& s);
	// "compiler"
	track precomp(std::vector<token> ts);
	// actually run the mostrocity from precomp
	std::string run(const track& data);

	void exec(const std::string& s);

}

#endif
