#pragma once
#ifndef NOAF_CMD_H
#define NOAF_CMD_H

#include <map>
#include <string>

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
	 * Peace
	 */
	extern std::map<char, std::string> tokens_base;
	extern std::map<char, std::string> tokens_expr;

}

#endif
