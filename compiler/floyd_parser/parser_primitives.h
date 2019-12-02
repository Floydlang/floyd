//
//  parser_primitives.h
//  Floyd
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
#include "types.h"

#include <string>
#include <vector>
#include <map>
#include <algorithm>

namespace floyd {

struct value_t;
struct location_t;
struct member_t;
struct types_t;

namespace parser {

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


////////////////////////////////		STRUCT


struct struct_def_t {
	std::string optional_name;
	std::vector<member_t> members_optional_names;
	type_t struct_type;
};


/*
	struct NAME { TYPE NAME; TYPE NAME .. }
	struct { TYPE NAME; TYPE NAME .. }
*/
std::pair<struct_def_t, seq_t> parse_struct_def(types_t& types, const seq_t& p, const location_t& location);


////////////////////////////////		TYPES
/*
??? move this out of this file -- parser should not know about type_t.
	Skip leading whitespace, get string while type-char.
	See language reference
	Validates that this is a legal string, with legal characters. Exception.
	Does NOT make sure this a known type-identifier.
	String must not be empty.
*/
std::pair<std::shared_ptr<type_t>, seq_t> read_type(types_t& types, const seq_t& s);
std::pair<type_t, seq_t> read_required_type(types_t& types, const seq_t& s);




////////////////////////////////		FUNCTION TYPES



struct function_desc_t {
	std::string optional_name;
	type_t return_type;
	std::vector<member_t> args_with_optional_names;
	bool impure;

	type_t function_type;
};

/*
	FUNCTION PROTOYPES

	func int (double, string) impure
	func int f(double x, string y)
	func int f(double, string)

	Name of function and arguments are optional. Does not parse body if one is present.
*/
std::pair<function_desc_t, seq_t> parse_function_prototype(
	types_t& types,
	const seq_t& pos,
	const location_t& location
);



////////////////////////////////		parse_result_t

//	Used by the parser functions to return both its result and the pos where it stopped reading.

struct parse_result_t {
	json_t parse_tree;
	seq_t pos;
};


////////////////////////////////		parse_tree_expression_opcode_t


namespace parse_tree_expression_opcode_t {
	const std::string k_literal = "k";
	const std::string k_call = "call";
	const std::string k_load = "@";
	const std::string k_resolve_member = "->";
	const std::string k_arithmetic_unary_minus = "unary-minus";
	const std::string k_conditional_operator = "?:";
	const std::string k_struct_def = "struct-def";
	const std::string k_function_def = "function-def";
	const std::string k_value_constructor = "value-constructor";
	const std::string k_lookup_element = "[]";
	const std::string k_benchmark = "benchmark";


	const std::string k_arithmetic_add = "+";
	const std::string k_arithmetic_subtract = "-";
	const std::string k_arithmetic_multiply = "*";
	const std::string k_arithmetic_divide = "/";
	const std::string k_arithmetic_remainder = "%";

	const std::string k_logical_and = "&&";
	const std::string k_logical_or = "||";

	const std::string k_comparison_smaller_or_equal = "<=";
	const std::string k_comparison_smaller = "<";
	const std::string k_comparison_larger_or_equal = ">=";
	const std::string k_comparison_larger = ">";

	const std::string k_logical_equal = "==";
	const std::string k_logical_nonequal = "!=";
};


////////////////////////////////		parse_tree_statement_opcode


namespace parse_tree_statement_opcode {
	const std::string k_return = "return";

	//??? Make init-local-const, init-local-mut
	const std::string k_init_local = "init-local";
	const std::string k_assign = "assign";
	const std::string k_block = "block";

	const std::string k_def_struct = "def-struct";

	const std::string k_if = "if";
	const std::string k_for = "for";
	const std::string k_while = "while";

	const std::string k_expression_statement = "expression-statement";

	const std::string k_software_system_def = "software-system-def";
	const std::string k_container_def = "container-def";
	const std::string k_benchmark_def = "benchmark-def";
	const std::string k_test_def = "test-def";
};



json_t make_parser_node(const location_t& location, const std::string& opcode, const std::vector<json_t>& params);

json_t parser__make_literal(const value_t& value);

}	//	 parser
}	//	floyd


#endif /* parser_primitives_hpp */
