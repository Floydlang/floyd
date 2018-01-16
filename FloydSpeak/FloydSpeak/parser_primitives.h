//
//  parser_primitives.h
//  FloydSpeak
//
//  Created by Marcus Zetterquist on 26/07/16.
//  Copyright © 2016 Marcus Zetterquist. All rights reserved.
//

/*
	These functions knows about the Floyd syntax.
*/

#ifndef parser_primitives_hpp
#define parser_primitives_hpp

#include "quark.h"
#include "text_parser.h"
#include "floyd_basics.h"

#include <string>
#include <vector>
#include <map>


struct json_t;

namespace floyd {

	const std::string whitespace_chars = " \n\t";
	const std::string identifier_chars = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_";
	const std::string brackets = "(){}[]";
	const std::string open_brackets = "({[";
	const std::string type_chars = identifier_chars;
	const std::string number_chars = "0123456789.";
	const std::string operator_chars = "+-*/.";


	//////////////////////////////////////////////////		Text parsing primitives, Floyd specific


	/*
		first: skipped whitespaces
		second: all / any text after whitespace.
	*/
	std::string skip_whitespace(const std::string& s);
	seq_t skip_whitespace(const seq_t& s);

	//	Removes whitespace before AND AFTER.
	std::string skip_whitespace_ends(const std::string& s);

	bool is_whitespace(char ch);


	//////////////////////////////////////////////////		BALANCING PARANTHESES, BRACKETS


	bool is_start_char(char c);
	bool is_end_char(char c);
	char start_char_to_end_char(char start_char);


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



	std::pair<std::string, seq_t> read_until_semicolor_or_seagull(const seq_t& pos0);


	/*
		Understands nested parantheses and brackets and skips those.
		Does NOT skip leading whitespace.
		If none are found, returns { "", s }
	*/
	std::pair<std::string, seq_t> read_until_toplevel_match(const seq_t& s, const std::string& match_chars);






	//////////////////////////////////////		BASIC STRING



	std::string reverse(const std::string& s);


	//////////////////////////////////////		IDENTIFIER


	std::pair<std::string, seq_t> read_single_identifier(const seq_t& s);

	/*
		Reads an identifier, like a variable name or function name.
		DOES NOT struct members.
			"hello xxx"
			"hello()xxx"
			"hello+xxx"

		Does not skip whitespace on the rest of the string.
			"\thello\txxx" => "hello" + "\txxx"
	*/
	std::pair<std::string, seq_t> read_required_single_identifier(const seq_t& s);



	//////////////////////////////////////		TYPES


	/*
		Skip leading whitespace, get string while type-char.
		See language reference
		Validates that this is a legal string, with legal characters. Exception.
		Does NOT make sure this a known type-identifier.
		String must not be empty.
	*/
	std::pair<std::shared_ptr<typeid_t>, seq_t> read_type_identifier2(const seq_t& s);
	std::pair<typeid_t, seq_t> read_required_type_identifier2(const seq_t& s);



	//////////////////////////////////////		HIGH LEVEL


	/*
		Used for:
			struct member variable
			function's local variable


		type is optional, can be "".
		expr is optional, can be null.
		member
		{
			"type": "int",
			"name": "my_local",
			"expr": "[\"k\", 1000]
		}
	*/
	json_t make_member_def(const std::string& type, const std::string& name, const json_t& expression);


	/*
		s2: starts and ends with parantheses. Has no other data at end.
		Argument names are optional.

		()
		(int a)
		(int x, int y)

		(int, int)
	*/
	std::vector<std::pair<typeid_t, std::string>> parse_functiondef_arguments2(const std::string& s);

}	//	floyd


#endif /* parser_primitives_hpp */
