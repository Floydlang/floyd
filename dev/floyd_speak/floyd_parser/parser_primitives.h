//
//  parser_primitives.h
//  FloydSpeak
//
//  Created by Marcus Zetterquist on 26/07/16.
//  Copyright © 2016 Marcus Zetterquist. All rights reserved.
//

/*
	These functions are building blocks for parsing Floyd program syntax.
*/

#ifndef parser_primitives_hpp
#define parser_primitives_hpp

#include "quark.h"
#include "text_parser.h"
#include "json_support.h"

#include <string>
#include <vector>
#include <map>
#include <algorithm>

namespace floyd {
struct typeid_t;
struct member_t;


////////////////////////////////		TEXT PARSING PRIMITIVES

//	Also skips
//		- one-line comments starting with "//"
//		- multiline comments withint /* ... */. Any number of nesting of comments allowed.
std::string skip_whitespace(const std::string& s);
seq_t skip_whitespace(const seq_t& s);

std::pair<std::string, seq_t> skip_whitespace2(const seq_t& s);

//	Removes whitespace before AND AFTER.
std::string skip_whitespace_ends(const std::string& s);

bool is_whitespace(char ch);


////////////////////////////////		BALANCING PARANTHESES, BRACKETS

/*
	First char is the start char, like '(' or '{'.
	Checks *all* balancing-chars
	Is recursive and not just checking intermediate chars, also pair match them.
*/
std::pair<std::string, seq_t> get_balanced(const seq_t& s);

/*
	()	=> ""
	(x)	=>	"x"
	(int x, int y) => "int x, int y"
*/
std::pair<std::string, seq_t> read_enclosed_in_parantheses(const seq_t& pos);


/*
	??? Use this is all places we scan.
	Understands nested parantheses and brackets and skips those.
	Does NOT skip leading whitespace.
	If none are found, returns { s.str(), seq_t("") }
*/
std::pair<std::string, seq_t> read_until_toplevel_match(const seq_t& s, const std::string& match_chars);


////////////////////////////////		BASIC STRING


std::string reverse(const std::string& s);


////////////////////////////////		IDENTIFIER


std::pair<std::string, seq_t> read_identifier(const seq_t& s);

/*
	Reads an identifier, like a variable name or function name.
	DOES NOT struct members.
		"hello xxx"
		"hello()xxx"
		"hello+xxx"
*/
std::pair<std::string, seq_t> read_required_identifier(const seq_t& s);


////////////////////////////////		TYPES
/*
??? move this out of this file -- parser should not know about typeid_t.
	Skip leading whitespace, get string while type-char.
	See language reference
	Validates that this is a legal string, with legal characters. Exception.
	Does NOT make sure this a known type-identifier.
	String must not be empty.
*/
std::pair<std::shared_ptr<typeid_t>, seq_t> read_type(const seq_t& s);
std::pair<typeid_t, seq_t> read_required_type(const seq_t& s);


////////////////////////////////		HIGH LEVEL
/*
	FUNCTION ARGUMENTS

	int (double, string)	--	function-type
	func int f(double x, string y)		-- function definition
	f(a, b)		-- function call


	s: starts and ends with parantheses. Has no other data at end.
	Argument names are optional.

	()
	(int a)
	(int x, int y)

	(int, int)
*/
std::pair<std::vector<member_t>, seq_t> read_functiondef_arg_parantheses(const seq_t& s);

//	Member names may be left blank.
std::pair<std::vector<member_t>, seq_t> read_function_type_args(const seq_t& s);

std::pair<std::vector<member_t>, seq_t> read_call_args(const seq_t& s);


////////////////////////////////		parse_result_t

//	Used by the parser functions to return both its result and the pos where it stopped reading.

struct parse_result_t {
	json_t parse_tree;
	seq_t pos;
};


}	//	floyd


#endif /* parser_primitives_hpp */
