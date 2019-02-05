//
//  floyd_basics.hpp
//  FloydSpeak
//
//  Created by Marcus Zetterquist on 2017-08-09.
//  Copyright © 2017 Marcus Zetterquist. All rights reserved.
//

#ifndef ast_json_h
#define ast_json_h

#include <string>
#include <vector>
#include "json_support.h"

struct seq_t;

namespace floyd {

struct value_t;


////////////////////////////////////////		ast_json_t

/*
Used to hold an AST encoded as a JSON.

It's a JSON array with each program statement in order.
Each statement is a JSON array like this:

	["return", EXPRESSION_JSON_OBJECT ]
	["bind", "string", "local_name", EXPRESSION_JSON_OBJECT ]
	["def_struct", STRUCT_DEF_JSON_OBJECT ]
	["define_function", FUNCTION_DEF_JSON_OBJECT ]

Element 0: if its a number, this is an extra field before the opcode and it contains the byte position inside compilation unit text.
If a string, this is the node opcode.
	[ 135500, "bind", "^double", "cmath_pi", ["k", 3.14159, "^double"] ],
	[ "bind", "^double", "cmath_pi", ["k", 3.14159, "^double"] ],

- Record byte position inside source file: requires source file to figure out line number.
- Offset is text position inside compilaton unit source -- different ranges maps to different source files, like preheader and includes. location_t.
*/

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


	json_t _value;
};



////////////////////////////////////////		statement_opcode_t

//	String keys use to specifiy statement type inside the ast_json_t.

namespace statement_opcode_t {
	const std::string k_return = "return";

	const std::string k_bind = "bind";
	const std::string k_store = "store";
	const std::string k_store2 = "store2";
	const std::string k_block = "block";

	const std::string k_def_struct = "def-struct";
	const std::string k_def_protocol = "def-protocol";
	const std::string k_def_func = "def-func";


	const std::string k_if = "if";
	const std::string k_for = "for";
	const std::string k_while = "while";

	const std::string k_expression_statement = "expression-statement";
	const std::string k_software_system = "software-system";
	const std::string k_container_def = "container-def";
};


////////////////////////////////////////		expression_opcode_t

//	String keys use to specifiy statement type inside the ast_json_t.

namespace expression_opcode_t {
	const std::string k_literal = "k";
	const std::string k_call = "call";
	const std::string k_load = "@";
	const std::string k_load2 = "@i";
	const std::string k_resolve_member = "->";
	const std::string k_unary_minus = "unary-minus";
	const std::string k_conditional_operator = "?:";
	const std::string k_struct_def = "def-struct";
	const std::string k_function_def = "def-func";
	const std::string k_value_constructor = "construct-value";
	const std::string k_lookup_element = "[]";
};




////////////////////////////////////////		location2_t


struct location2_t {
	location2_t(const std::string& source_file_path, int line_number, int column, std::size_t start, std::size_t end, const std::string& line) :
		source_file_path(source_file_path),
		line_number(line_number),
		column(column),
		start(start),
		end(end),
		line(line)
	{
	}

	std::string source_file_path;
	int line_number;
	int column;
	std::size_t start;
	std::size_t end;
	std::string line;
};


////////////////////////////////////////		location_t


//	Specifies character offset inside source code.

struct location_t {
	explicit location_t(std::size_t offset) :
		offset(offset)
	{
	}

	std::size_t offset;
};

inline bool operator==(const location_t& lhs, const location_t& rhs){
	return lhs.offset == rhs.offset;
}
extern const location_t k_no_location;



////////////////////////////////////////		Creates json values for different AST constructs like expressions and statements.


ast_json_t make_statement_n(const location_t& location, const std::string& opcode, const std::vector<json_t>& params);
ast_json_t make_statement1(const location_t& location, const std::string& opcode, const json_t& params);
ast_json_t make_statement2(const location_t& location, const std::string& opcode, const json_t& param1, const json_t& param2);
ast_json_t make_statement3(const location_t& location, const std::string& opcode, const json_t& param1, const json_t& param2, const json_t& param3);
ast_json_t make_statement4(const location_t& location, const std::string& opcode, const json_t& param1, const json_t& param2, const json_t& param3, const json_t& param4);

ast_json_t make_expression_n(const location_t& location, const std::string& opcode, const std::vector<json_t>& params);
ast_json_t make_expression1(const location_t& location, const std::string& opcode, const json_t& param);
ast_json_t make_expression2(const location_t& location, const std::string& opcode, const json_t& param1, const json_t& param2);
ast_json_t make_expression3(const location_t& location, const std::string& opcode, const json_t& param1, const json_t& param2, const json_t& param3);


ast_json_t maker__make_identifier(const std::string& s);
ast_json_t maker__make_unary_minus(const json_t& expr);
ast_json_t maker__make2(const std::string op, const json_t& lhs, const json_t& rhs);
ast_json_t maker__make_conditional_operator(const json_t& e1, const json_t& e2, const json_t& e3);
ast_json_t maker__call(const json_t& f, const std::vector<json_t>& args);
ast_json_t maker_vector_definition(const std::string& element_type, const std::vector<json_t>& elements);
ast_json_t maker_dict_definition(const std::string& value_type, const std::vector<json_t>& elements);
ast_json_t maker__member_access(const json_t& address, const std::string& member_name);
ast_json_t maker__make_constant(const value_t& value);




//??? move somewhere else
void ut_verify_json_and_rest(const quark::call_context_t& context, const std::pair<ast_json_t, seq_t>& result_pair, const std::string& expected_json, const std::string& expected_rest);

void ut_verify_values(const quark::call_context_t& context, const value_t& result, const value_t& expected);

void ut_verify(const quark::call_context_t& context, const std::pair<std::string, seq_t>& result, const std::pair<std::string, seq_t>& expected);


}	//	floyd

#endif /* ast_json_h */
