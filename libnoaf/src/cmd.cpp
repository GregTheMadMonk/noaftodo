#include "cmd.hpp"

#include <regex>

#include <cvar.hpp>
#include <hooks.hpp>
#include <log.hpp>

using namespace std;

namespace noaf::cmd {

	command::command(const cmd_cb& cb, const string& u, const string& t) :
		callback(cb), usage(u), tooltip(t) {}

	map<string, command>& cmds() {
		static map<string, command> _cmds = {
			{
				"echo",
				command(
					[] (const vector<string>& args) {
						string ret = "";
						for (int i = 0; i < args.size(); i++) {
							ret += args.at(i);
							if (i < args.size() - 1) ret += " ";
						}

						log << llev(VCAT) << ret << lend;

						return ret;
					},
					"echo [arguments...]",
					"Outputs its arguments"
				)
			},
			{
				"q",
				command(
					[] (const vector<string>& args) {
						if (args.empty()) noaf::exit();
						else noaf::exit(stoi(args.at(0)));

						return "";
					},
					"q [code = 0]",
					"Exits the program with specified code"
				)
			},
		};

		return _cmds;
	}

	vector<string> read_list(stack<token>& dump) {
		vector<string> items = { };	// elements
		bool through = false;		// detects 1..2
		while (dump.top().type != LIST_START) {
			const token lt = dump.top();
			dump.pop();

			switch (lt.type) {
				case VALUE:
					if (items.empty()) items.push_back("");
					if (!through) {
						items.at(0) = lt.value + items.at(0);
					} else {
						float a = stof(items.at(0));
						float b = stof(lt.value);

						if (a < b) for (float c = a + 1; c <= b; c++)
							items.insert(items.begin(), to_string(c));
						else for (float c = a - 1; c >= b; c--)
							items.insert(items.begin(), to_string(c));
					}
					break;
				case LIST_DELIMITER:
					through = false;
					items.insert(items.begin(), "");
					break;
				case LIST_THROUGH:
					through = true;
					break;
			}
		}
		return items;
	}

	substitutor get_subst(const list_indexer& li, const list_indexer::iterator& where, const listdb& what) {
		if (where == li.end()) return {};
		substitutor ret = {};

		if (next(where) == li.end()) { // last iterator
			for (const auto l_item : what.at(where->second))
				ret.push_back({ { where->first, l_item } });

			return ret;
		}

		for (const auto l_item : what.at(where->second))
			for (auto l_vec : get_subst(li, next(where), what)) {
				l_vec.insert(l_vec.begin(), { where->first, l_item });
				ret.push_back(l_vec);
			}

		return ret;
	}

	vector<track> read_command(stack<token>& dump, const listdb& lists, const stack<mode_type>& mode) {
		track items;
		map<size_t, size_t> i_lists = {}; // where to have lists and which ones (offset from the end)

		const auto is_reading = [&] () {
			if (dump.empty()) return false;
			return (dump.top().type != INLINE_START);
		};

		bool delim = false;

		while (is_reading()) {
			auto bt = dump.top();
			dump.pop();

			switch (bt.type) {
				case QUOT1: case QUOT2: case QUOT3:
					bt.value = bt.value.substr(0, bt.value.length() - 1);
					bt.value = regex_replace(bt.value,
							regex("\\\\(.)"),
							"$1");
				case VALUE_SPECIAL:
					bt.value = bt.value.substr(1);
				case VALUE:
					if (items.empty()) items.push_back({ VALUE, "" });
					if ((items.at(0).type != VALUE) || delim)
						items.insert(items.begin(), { VALUE, bt.value });
					else items.at(0).value = bt.value + items.at(0).value;
					delim = false;
					break;
				case LIST_REFERENCE:
					items.insert(items.begin(), bt);
					i_lists[items.size()] = (size_t)bt.value.at(0);
					delim = false;
					break;
				case BASE_DELIMITER:
					items.insert(items.begin(), bt);
					delim = true;
					break;
				case BASE_START: case EXPR_START:
					break;
				default:
					items.insert(items.begin(), bt);
					delim = false;
			}
		}

		if (!dump.empty()) dump.pop();

		// now, form the track expaning lists
		const auto substs = get_subst(i_lists, i_lists.begin(), lists);

		switch (mode.top()) {
			case BASE:
				items.push_back({ CALL, "" });
				break;
			case EXPRESSION:
				items.push_back({ EVAL, "" });
				break;
		}

		if (substs.empty()) return { items };

		vector<track> items_final = {};
		for (const auto& subst : substs) {
			// replace list references with list values
			for (const auto& rep : subst) items.at(items.size() - 1 - rep.first) = { VALUE, rep.second };
			items_final.push_back(items);
		}

		return items_final;
	}

	vector<track> tracks_append(const vector<track>& base, const vector<track>& more) {
		if (base.empty()) return more;

		vector<track> ret;
		for (const auto& b : base)
			for (const auto& m : more) {
				ret.push_back(b);
				ret.back().insert(ret.back().end(), m.begin(), m.end());
			}

		return ret;
	}

	string call(const track& cmdline) {
		if (cmdline.empty()) return "";

		string name;
		vector<string> args;

		token_type last = TNONE;

		for (const auto& t : cmdline) {
			switch (t.type) {
				case VALUE: // VALUE_SPECIAL is not presentby this point
					if (last == VALUE) {
						args.back() += t.value;
					} else if (name.empty()) {
						name = t.value;
					} else args.push_back(t.value);
					break;
			}

			last = t.type;
		}

		return cmds().at(name).callback(args);
	}

	string eval(track cmdline) {
		if (cmdline.empty()) return "";

		if (cmdline.size() == 1) return cmdline.at(0).value;

		// finds the first token of specified type in a track
		const auto fd_t = [] (const track& trck, const token_type& tt) {
			for (auto it = trck.begin(); it != trck.end(); it++)
				if ((*it).type == tt) return it;

			return trck.end();
		};

		for (const auto& op_t : op_order)
			for (auto it = fd_t(cmdline, op_t); it != cmdline.end(); it = fd_t(cmdline, op_t)) {
				token res;
				res.type = VALUE;

				switch ((*it).type) {
					case OP_SUB: // - before a number without a left operand negates it
						{

						bool negate = false;
						if (it == cmdline.begin()) {
							negate = true;
						} else {
							const auto ta = *prev(it);
							negate = (ta.type != VALUE);
						}

						if (negate) {
							const auto tb = *next(it);
							if (tb.type != VALUE) throw wrong_token;

							cmdline.erase(it, next(next(it)));
							cmdline.insert(it, { VALUE, to_string(-stof(tb.value)) });
							break;
						}

						}
					case OP_DIV: case OP_MUL: case OP_ADD:
						{
						const auto ta = *prev(it);
						const auto tb = *next(it);
						if ((ta.type != VALUE) || (tb.type != VALUE)) throw wrong_token;

						const float a = stof(ta.value);
						const float b = stof(tb.value);

						switch ((*it).type) {
							case OP_DIV:
								res.value = to_string(a / b);
								break;
							case OP_MUL:
								res.value = to_string(a * b);
								break;
							case OP_ADD:
								res.value = to_string(a + b);
								break;
							case OP_SUB:
								res.value = to_string(a - b);
								break;
						}

						cmdline.erase(prev(it), next(next(it)));
						cmdline.insert(prev(it), res);
						}
						break;
					case OP_EQ:
						{
						const auto ta = *prev(it);
						const auto tb = *next(it);

						if ((ta.type != VALUE) || (tb.type != VALUE)) throw wrong_token;

						cvar::get(ta.value) = tb.value;

						cmdline.erase(prev(it), next(it));
						}
						break;
				}
			}

		return cmdline.at(0).value;
	}

	track lex(const std::string& s) {
		track tokens;	// token list to be returned
		stack<mode_type> mode;	// modes
		mode.push(MNONE);

		smatch match;		// match variable
		int progress = 0;	// "cursor" that we move as wwe progress parsing the string

		while (progress < s.size()) {
			const string command = s.substr(progress);
			token_type found = TNONE;

			// if a mode is undetermined, try and detect it
			if (mode.top() == MNONE) {
				// if we fail to find a command, default to expression mode
				mode_type new_mode = EXPRESSION;
				// search if there is a command at the beginning of a string
				for (auto it = cmds().begin(); it != cmds().end(); it++) {
					regex cmdsearch(it->first +
						       ((command.length() == it->first.length()) ? "" : " ")
						       );
					if (regex_search(command.begin(), command.end(), match, cmdsearch)) {
						// command found, use BASE mode
						if (match.position() == 0) new_mode = BASE;
						break;
					}
				}

				mode.pop();		// pop MNONE
				mode.push(new_mode);	// push actual mode

				// store info about modes so we don't have to repeat the process
				// of command searching extra times
				token ms;
				switch (new_mode) {
					case BASE:
						ms.type = BASE_START;
						break;
					case EXPRESSION:
						ms.type = EXPR_START;
						break;
				}
				tokens.push_back(ms);
			}

			// choose a relevant token set
			const auto& tokenlist = [&] () -> map<token_type, string>& {
				switch (mode.top()) {
					case BASE:
						return tokens_base;
					case EXPRESSION:
						return tokens_expr;
					case LIST:
						return tokens_list;
					default:
						throw wrong_mode;
				}
			} ();

			// search for the token closest to the cursor (also need the longest one)
			for (auto it = tokenlist.begin(); it != tokenlist.end(); it++) {
				regex r(it->second);

				smatch new_match;
				if (regex_search(command.begin(), command.end(), new_match, r)) {
					if ((found != 0) && ((new_match.position() > match.position()) ||
						((new_match.position() == match.position()) &&
						 (new_match[0].length() <= match[0].length()))
							    )) // sorry for *this*
						// token found, but it's not the closest or the longest match
						continue;

					// closest match so far, remember it
					match = new_match;
					found = it->first;
				}
			}

			token t; // token to push

			if (found == TNONE) { // nothing was found, end parsing
				t.type = VALUE;
				t.value = command;
				tokens.push_back(t);
				break;
			}

			if (match.position() != 0) { // a token was found, but not at the cursor
				// everything between tokens is stored as a simple value
				t.type = VALUE;
				t.value = command.substr(0, match.position());
				tokens.push_back(t);
			}

			// defaults for the token
			t.type = found;
			t.value = match[0];

			// specific stuff depending on found token type
			switch (t.type) {
				case LIST_START:
					mode.push(LIST);
					break;
				case LIST_END:
					mode.pop();
					break;
				case INLINE_START:
					mode.push(MNONE);
					break;
				case INLINE_END:
					mode.pop();
					break;
				default:
					break;
			}

			progress += match.position() + match[0].length();

			tokens.push_back(t);
		}

		if (mode.size() > 1) throw unterminated_mode;

		return tokens;
	}

	track precomp(track tokens) {
		if (!tokens.empty()) if (tokens.back().type != NEXTCMD) tokens.push_back({ NEXTCMD, ";" });

		stack<mode_type> mode;
		stack<token> dump;
		vector<track> tracks;
		listdb lists;
		track ret = {};

		// "compilation"?
		for (size_t p = 0; p < tokens.size(); p++) {
			const auto& t = tokens.at(p);
			switch (t.type) {
				case LIST_START:
					mode.push(LIST);
					dump.push(t);
					break;
				case BASE_START:
					mode.push(BASE);
					dump.push(t);
					break;
				case EXPR_START:
					mode.push(EXPRESSION);
					dump.push(t);
					break;
				case INLINE_END:
					if (mode.top() == LIST) throw wrong_token;
					tracks = tracks_append(tracks, read_command(dump, lists, mode));
					dump.push({ REFERENCE, "" });
					mode.pop();
					break;
				case NEXTCMD:
					tracks = tracks_append(tracks, read_command(dump, lists, mode));
					if (mode.size() == 1) { // top level commands are not chain-expanded
						// squash the tracks into a token sequence
						for (const auto& trck : tracks) {
							ret.insert(ret.end(), { CLRRET, "" });
							ret.insert(ret.end(), trck.begin(), trck.end());
						}

						tracks = {};
					}
					break;
				case LIST_END:
					if (mode.top() != LIST) throw wrong_token;
					lists.push_back(read_list(dump));
					dump.pop();
					dump.push({
						LIST_REFERENCE,
						string(1, (char)(lists.size() - 1))
					});
					mode.pop();
					break;
				case TNONE: // empty token
					break;
				default:
					dump.push(t);
			}
		}

		return ret;
	}

	string run(const track& data) {
		stack<string>	retvals; // return values
		stack<token>	pending; // next command pending execution

		const auto form = [&] () {
			// implace references
			// we need to do it in a specific function so that it doesn't mess
			// up the implacement order
			track result;

			while (!pending.empty()) {
				switch (pending.top().type) {
					case REFERENCE:
						if (retvals.empty()) throw lose_reference;

						result.insert(result.begin(), { VALUE, retvals.top() });
						retvals.pop();
						break;
					case VAR_REFERENCE:
						{
						string var = pending.top().value;
						var = var.substr(1, var.length() - 2);
						result.insert(result.begin(), { VALUE, (string)cvar::get(var) });
						}
						break;
					default:
						result.insert(result.begin(), pending.top());
				}
				pending.pop();
			}

			return result;
		};

		for (const auto& t : data) {
			switch (t.type) {
				case CALL:
					retvals.push(call(form()));
					pending = {};
					break;
				case EVAL:
					retvals.push(eval(form()));
					pending = {};
					break;
				case CLRRET:
					{
						stack<string> emp;
						swap(retvals, emp);
					}
					break;
				default:
					pending.push(t);
			}
		}

		return retvals.top();
	}

	void exec(const std::string& s) {
		auto tokens = lex(s);
		tokens = precomp(tokens);
		const auto ret = run(tokens);

		log << ">>> " << ret << lend;
	}

}
