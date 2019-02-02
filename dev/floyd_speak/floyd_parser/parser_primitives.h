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


struct ast_json_t;

namespace floyd {
	//??? move this out of this file -- parser should not know about typeid_t.
	struct typeid_t;

	//??? move this out of this file -- parser should not know about member_t.
	struct member_t;


	const std::string whitespace_chars = " \n\t";
	const std::string identifier_chars = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_";
	const std::string bracket_pairs = "(){}[]";


	//////////////////////////////////////////////////		Text parsing primitives, Floyd specific


	//	Also skips
	//		- one-line comments starting with "//"
	//		- multiline comments withint /* ... */. Any number of nesting of comments allowed.
	std::string skip_whitespace(const std::string& s);
	seq_t skip_whitespace(const seq_t& s);
	
	std::pair<std::string, seq_t> skip_whitespace2(const seq_t& s);

	//	Removes whitespace before AND AFTER.
	std::string skip_whitespace_ends(const std::string& s);

	bool is_whitespace(char ch);



	//////////////////////////////////////////////////		BALANCING PARANTHESES, BRACKETS


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


	//////////////////////////////////////		BASIC STRING


	std::string reverse(const std::string& s);


	//////////////////////////////////////		IDENTIFIER


	std::pair<std::string, seq_t> read_identifier(const seq_t& s);

	/*
		Reads an identifier, like a variable name or function name.
		DOES NOT struct members.
			"hello xxx"
			"hello()xxx"
			"hello+xxx"
	*/
	std::pair<std::string, seq_t> read_required_identifier(const seq_t& s);


	//////////////////////////////////////		TYPES


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

	std::pair<bool, seq_t> read_type_verify(const seq_t& s);


	//////////////////////////////////////		HIGH LEVEL


	/*
		s2: starts and ends with parantheses. Has no other data at end.
		Argument names are optional.

		()
		(int a)
		(int x, int y)

		(int, int)
	*/
	std::vector<member_t> parse_functiondef_arguments2(const std::string& s);

	std::pair<std::vector<member_t>, seq_t> read_function_arg_parantheses(const seq_t& s);





inline int count_lines(const std::string& range){
	const auto lf_count = std::count(range.begin(), range.end(), '\r');
	const auto nl_count = std::count(range.begin(), range.end(), '\n');
	return static_cast<int>(lf_count + nl_count);
}

inline int count_lines(const seq_t& start, const seq_t& end){
	const auto range = get_range(start, end);
	return count_lines(range);
}


struct location_t {
	location_t(const std::string& source_file_path, int line_number, int column) :
		source_file_path(source_file_path),
		line_number(line_number),
		column(column)
	{
	}

	std::string source_file_path;
	int line_number;
	int column;
};

ast_json_t make_statement_n(int64_t location, const std::string& opcode, const std::vector<json_t>& params);
ast_json_t make_statement1(int64_t location, const std::string& opcode, const json_t& params);
ast_json_t make_statement2(int64_t location, const std::string& opcode, const json_t& param1, const json_t& param2);
ast_json_t make_statement3(int64_t location, const std::string& opcode, const json_t& param1, const json_t& param2, const json_t& param3);
ast_json_t make_statement4(int64_t location, const std::string& opcode, const json_t& param1, const json_t& param2, const json_t& param3, const json_t& param4);

ast_json_t make_expression_n(int64_t location, const std::string& opcode, const std::vector<json_t>& params);
ast_json_t make_expression1(int64_t location, const std::string& opcode, const json_t& param);
ast_json_t make_expression2(int64_t location, const std::string& opcode, const json_t& param1, const json_t& param2);
ast_json_t make_expression3(int64_t location, const std::string& opcode, const json_t& param1, const json_t& param2, const json_t& param3);

/*
#include "floyd_basics.h"
#include "text_parser.h"
*/
	struct parse_result_t {
		ast_json_t ast;
		seq_t pos;
		std::vector<int> line_numbers;
	};

	
}	//	floyd


#endif /* parser_primitives_hpp */
