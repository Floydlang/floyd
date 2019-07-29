//
//  floyd_basics.hpp
//  Floyd
//
//  Created by Marcus Zetterquist on 2017-08-09.
//  Copyright © 2017 Marcus Zetterquist. All rights reserved.
//

#ifndef ast_json_h
#define ast_json_h

/*
	The AST JSON type hold an AST, encoded as a JSON.

	It's a JSON array with each program statement in order.
	Each statement is a JSON array like this:

		["return", EXPRESSION_JSON_OBJECT ]
		["bind", "string", "local_name", EXPRESSION_JSON_OBJECT ]
		["def_struct", STRUCT_DEF_JSON_OBJECT ]
		["define_function", FUNCTION_DEF_JSON_OBJECT ]

	The first element, element 0, of each statement is optionally a byte-offset where this statement is defined in the source text.
	Then comes the statement opcode, like "return" and its parameters.
		[ 135500, "bind", "^double", "cmath_pi", ["k", 3.14159, "^double"] ],
		[ "bind", "^double", "cmath_pi", ["k", 3.14159, "^double"] ],
*/

#include <string>
#include <vector>
#include "json_support.h"
#include "compiler_basics.h"

struct seq_t;

namespace floyd {

struct value_t;


////////////////////////////////////////		statement_opcode_t

//	String keys used to specify statement type inside the AST JSON.

namespace statement_opcode_t {
	const std::string k_return = "return";

	const std::string k_bind = "bind";
	const std::string k_assign = "assign";
	const std::string k_assign2 = "assign2";
	const std::string k_init2 = "init2";
	const std::string k_block = "block";

	const std::string k_def_struct = "def-struct";
	const std::string k_def_func = "def-func";


	const std::string k_if = "if";
	const std::string k_for = "for";
	const std::string k_while = "while";

	const std::string k_expression_statement = "expression-statement";

	const std::string k_software_system_def = "software-system-def";
	const std::string k_container_def = "container-def";
	const std::string k_benchmark_def = "benchmark-def";
	const std::string k_benchmark = "benchmark";
};


////////////////////////////////////////		expression_opcode_t

//	String keys used to specify statement type inside the AST JSON.

namespace expression_opcode_t {
	const std::string k_literal = "k";
	const std::string k_call = "call";
	const std::string k_corecall = "corecall";
	const std::string k_load = "@";
	const std::string k_load2 = "@i";
	const std::string k_resolve_member = "->";
	const std::string k_update_member = "<-";
	const std::string k_unary_minus = "unary-minus";
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





////////////////////////////////////////		make_statement*(), make_expression*()

//	Creates json values for different AST constructs like expressions and statements.

json_t make_ast_node(const location_t& location, const std::string& opcode, const std::vector<json_t>& params);

json_t make_statement1(const location_t& location, const std::string& opcode, const json_t& params);
json_t make_statement2(const location_t& location, const std::string& opcode, const json_t& param1, const json_t& param2);

json_t make_expression1(const location_t& location, const std::string& opcode, const json_t& param);
json_t make_expression2(const location_t& location, const std::string& opcode, const json_t& param1, const json_t& param2);


json_t maker__make2(const std::string op, const json_t& lhs, const json_t& rhs);
json_t maker__corecall(const std::string& name, const std::vector<json_t>& args);
json_t maker_vector_definition(const std::string& element_type, const std::vector<json_t>& elements);
json_t maker_dict_definition(const std::string& value_type, const std::vector<json_t>& elements);
json_t maker__member_access(const json_t& address, const std::string& member_name);
json_t maker__make_constant(const value_t& value);
json_t maker_benchmark_definition(const json_t& body);


//	INPUT: [2, "bind", "^double", "cmath_pi", ["k", 3.14159, "^double"]]
std::pair<json_t, location_t> unpack_loc(const json_t& s);

//	Reads a location_t from a statement, if one exists. Else it returns k_no_location.
//	INPUT: [2, "bind", "^double", "cmath_pi", ["k", 3.14159, "^double"]]
location_t unpack_loc2(const json_t& s);



}	//	floyd

#endif /* ast_json_h */
