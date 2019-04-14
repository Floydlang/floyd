//
//  floyd_basics.hpp
//  FloydSpeak
//
//  Created by Marcus Zetterquist on 2017-08-09.
//  Copyright © 2017 Marcus Zetterquist. All rights reserved.
//

#ifndef ast_json_h
#define ast_json_h

/*
	The ast_json_t type hold an AST, encoded as a JSON.

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


	NOTICE: Right now ast_json_t is used a little sloppyly.

	1. The parse tree
	2. The input AST (no types and resolve)
	3. The semantically correct AST

	Future: make separate types, optimally separate constants for statement/expression opcodes.
*/

#include <string>
#include <vector>
#include "json_support.h"
#include "compiler_basics.h"

struct seq_t;

namespace floyd {

struct value_t;


////////////////////////////////////////		ast_json_t


struct ast_json_t {
	private: ast_json_t(const json_t& v) :
		_value(v)
	{
	}

	public: static ast_json_t make(const json_t& v){
		return { v };
	}
	public: static ast_json_t make(const std::vector<json_t>& v){
		return { json_t::make_array(v) };
	}


	////////////////////////////////////////		STATE
	json_t _value;
};



////////////////////////////////////////		statement_opcode_t

//	String keys use to specify statement type inside the ast_json_t.

namespace statement_opcode_t {
	const std::string k_return = "return";

	const std::string k_bind = "bind";
	const std::string k_store = "store";
	const std::string k_store2 = "store2";
	const std::string k_block = "block";

	const std::string k_def_struct = "def-struct";
	const std::string k_def_func = "def-func";


	const std::string k_if = "if";
	const std::string k_for = "for";
	const std::string k_while = "while";

	const std::string k_expression_statement = "expression-statement";
	const std::string k_software_system = "software-system";
	const std::string k_container_def = "container-def";
};


////////////////////////////////////////		expression_opcode_t

//	String keys use to specify statement type inside the ast_json_t.

namespace expression_opcode_t {
	const std::string k_literal = "k";
	const std::string k_call = "call";
	const std::string k_load = "@";
	const std::string k_load2 = "@i";
	const std::string k_resolve_member = "->";
	const std::string k_unary_minus = "unary-minus";
	const std::string k_conditional_operator = "?:";
	const std::string k_struct_def = "struct-def";
	const std::string k_function_def = "function-def";
	const std::string k_value_constructor = "value-constructor";
	const std::string k_lookup_element = "[]";
};




////////////////////////////////////////		make_statement*(), make_expression*()

//	Creates json values for different AST constructs like expressions and statements.

json_t make_statement_n(const location_t& location, const std::string& opcode, const std::vector<json_t>& params);
json_t make_statement1(const location_t& location, const std::string& opcode, const json_t& params);
json_t make_statement2(const location_t& location, const std::string& opcode, const json_t& param1, const json_t& param2);
json_t make_statement3(const location_t& location, const std::string& opcode, const json_t& param1, const json_t& param2, const json_t& param3);
json_t make_statement4(const location_t& location, const std::string& opcode, const json_t& param1, const json_t& param2, const json_t& param3, const json_t& param4);

json_t make_expression_n(const location_t& location, const std::string& opcode, const std::vector<json_t>& params);
json_t make_expression1(const location_t& location, const std::string& opcode, const json_t& param);
json_t make_expression2(const location_t& location, const std::string& opcode, const json_t& param1, const json_t& param2);
json_t make_expression3(const location_t& location, const std::string& opcode, const json_t& param1, const json_t& param2, const json_t& param3);


json_t maker__make_identifier(const std::string& s);
json_t maker__make_unary_minus(const json_t& expr);
json_t maker__make2(const std::string op, const json_t& lhs, const json_t& rhs);
json_t maker__make_conditional_operator(const json_t& e1, const json_t& e2, const json_t& e3);
json_t maker__call(const json_t& f, const std::vector<json_t>& args);
json_t maker_vector_definition(const std::string& element_type, const std::vector<json_t>& elements);
json_t maker_dict_definition(const std::string& value_type, const std::vector<json_t>& elements);
json_t maker__member_access(const json_t& address, const std::string& member_name);
json_t maker__make_constant(const value_t& value);



//	Reads a location_t from a statement, if one exists. Else it returns k_no_location.
//	INPUT: [2, "bind", "^double", "cmath_pi", ["k", 3.14159, "^double"]]
location_t unpack_loc2(const json_t& s);


//??? move somewhere else
void ut_verify_json_and_rest(const quark::call_context_t& context, const std::pair<json_t, seq_t>& result_pair, const std::string& expected_json, const std::string& expected_rest);


void ut_verify(const quark::call_context_t& context, const std::pair<std::string, seq_t>& result, const std::pair<std::string, seq_t>& expected);


}	//	floyd

#endif /* ast_json_h */
