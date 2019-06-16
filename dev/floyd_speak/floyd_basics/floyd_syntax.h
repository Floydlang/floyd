//
//  floyd_syntax.hpp
//  floyd_speak
//
//  Created by Marcus Zetterquist on 2019-02-05.
//  Copyright © 2019 Marcus Zetterquist. All rights reserved.
//

#ifndef floyd_syntax_hpp
#define floyd_syntax_hpp

/*
	Types and constants that describe the syntax of floyd program texts.
	Keywords, the different types in the language, what special characters there are.
*/

#include <string>
#include <map>
#include "quark.h"

namespace floyd {

/*
	PEG
	https://en.wikipedia.org/wiki/Parsing_expression_grammar
	http://craftinginterpreters.com/representing-code.html

	AST ABSTRACT SYNTAX TREE

	https://en.wikipedia.org/wiki/Abstract_syntax_tree

	https://en.wikipedia.org/wiki/Parsing_expression_grammar
	https://en.wikipedia.org/wiki/Parsing
*/

/*
	C99-language constants.
	http://en.cppreference.com/w/cpp/language/operator_precedence
*/
const std::string k_c99_number_chars = "0123456789.";
const std::string k_c99_whitespace_chars = " \n\t\r";
const std::string k_c99_identifier_chars = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_";


const std::string whitespace_chars = " \n\t";
const std::string identifier_chars = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_";
const std::string bracket_pairs = "(){}[]";

const std::string valid_expression_chars = k_c99_identifier_chars + k_c99_number_chars + k_c99_whitespace_chars + "+-*/%" + "\"[](){}.?:=!<>&,|#$\\;\'";



//////////////////////////////////////		base_type

/*
	The atomic building block of all types.
	Some of the types are ready as-is, like bool or double.
	Some types needs further information to be 100% defined, like struct (needs its members), vector needs its element-type.
*/

enum class base_type {
	//	k_undefined is never exposed in code, only used internally in compiler.
	//	??? Maybe we can use void for this and remove k_undefined?
	k_undefined,

	//	Used by host functions arguments / returns to tell this is a dynamic value, not static type.
	k_any,

	//	Means no value. Used as return type for print() etc.
	k_void,

	k_bool,
	k_int,
	k_double,
	k_string,
	k_json_value,

	//	This is a type that specifies any other type at runtime.
	k_typeid,

	k_struct,
	k_vector,
	k_dict,
	k_function,

	//	We have an identifier, like "pixel" or "print" but haven't resolved it to an actual type yet.
	//	Keep the identifier so it can be resolved later.
	k_unresolved
};

std::string base_type_to_string(const base_type t);

void ut_verify(const quark::call_context_t& context, const base_type& result, const base_type& expected);


//??? use lookup for statements vs their JSON-strings: k_assign2, "assign" and "def-struct".



///////////////////////////////////			eoperator_precedence


/*
	Operator precedence is the same as C99.
	Lower number gives stronger precedence.
	Important: we use < and > to compare these.
*/
enum class eoperator_precedence {
	k_super_strong = 0,

	//	(xyz)
	k_parentesis = 0,

	//	a()
	k_function_call = 2,

	//	a[], aka subscript
	k_lookup = 2,

	//	a.b
	k_member_access = 2,


	k_multiply_divider_remainder = 5,

	k_add_sub = 6,

	//	<   <=	For relational operators < and ≤ respectively
	//	>   >=
	k_larger_smaller = 8,


	k_equal__not_equal = 9,

	k_logical_and = 13,
	k_logical_or = 14,

	k_comparison_operator = 15,

	k_super_weak
};



///////////////////////////////////			syntax_expression_type


/*
	Types of expressions that can be expressed in the Floyd syntax
	The order of constants inside enum NOT important.
	The number in the names tells how many operands it needs.
*/
//??? Use expression_type instead.
enum class syntax_expression_type {
	k_0_number_constant = 100,

	//	This is string specifying a local variable, member variable, argument, global etc. Only the first entry in a chain.
	k_0_resolve,

	k_0_string_literal,

	k_x_member_access,

	k_2_lookup,

	k_2_add,
	k_2_subtract,
	k_2_multiply,
	k_2_divide,
	k_2_remainder,

	k_2_smaller_or_equal,
	k_2_smaller,

	k_2_larger_or_equal,
	k_2_larger,

	//	a == b
	k_2_logical_equal,

	//	a != b
	k_2_logical_nonequal,

	//	a && b
	k_2_logical_and,

	//	a || b
	k_2_logical_or,

	//	cond ? a : b
	k_3_conditional_operator,

	k_n_call,

	//	!a
//	k_1_logical_not

	//	-a
	k_1_unary_minus,


//	k_0_identifier

//	k_1_construct_value

	k_1_vector_definition,
	k_1_dict_definition
};

std::string k_2_operator_to_string__func(syntax_expression_type op);



//////////////////////////////////////		expression_type



//??? Split and categories better. Logic vs equality vs math.

//	Number at end of name tells number of input expressions operation has.
enum class expression_type {

	//	c99: a + b			token: "+"
	k_arithmetic_add__2 = 10,

	//	c99: a - b			token: "-"
	k_arithmetic_subtract__2,

	//	c99: a * b			token: "*"
	k_arithmetic_multiply__2,

	//	c99: a / b			token: "/"
	k_arithmetic_divide__2,

	//	c99: a % b			token: "%"
	k_arithmetic_remainder__2,


	//	c99: a <= b			token: "<="
	k_comparison_smaller_or_equal__2,

	//	c99: a < b			token: "<"
	k_comparison_smaller__2,

	//	c99: a >= b			token: ">="
	k_comparison_larger_or_equal__2,

	//	c99: a > b			token: ">"
	k_comparison_larger__2,


	//	c99: a == b			token: "=="
	k_logical_equal__2,

	//	c99: a != b			token: "!="
	k_logical_nonequal__2,


	//	c99: a && b			token: "&&"
	k_logical_and__2,

	//	c99: a || b			token: "||"
	k_logical_or__2,

	//	c99: !a				token: "!"
//			k_logical_not,

	//	c99: 13				token: "k"
	k_literal,

	//	c99: -a				token: "unary-minus"
	k_arithmetic_unary_minus__1,

	//	c99: cond ? a : b	token: "?:"
	k_conditional_operator3,

	//	c99: a(b, c)		token: "call"
	k_call,

	k_corecall,

	//	c99: a				token: "@"
	k_load,

	//	c99: a				token: "@i"
	k_load2,

	//	c99: a.b			token: "->"
	k_resolve_member,

	//						token: "update"
	k_update,

	//						token: "<-"
	k_update_member,

	//	c99: a[b]			token: "[]"
	k_lookup_element,

	//	"def-struct"
	k_struct_def,

	//???	use k_literal for function values?
	//	"def-func"
	k_function_def,

	//	"construct-value"
	k_value_constructor,
};



inline bool is_arithmetic_expression(expression_type op){
	return false
		|| op == expression_type::k_arithmetic_add__2
		|| op == expression_type::k_arithmetic_subtract__2
		|| op == expression_type::k_arithmetic_multiply__2
		|| op == expression_type::k_arithmetic_divide__2
		|| op == expression_type::k_arithmetic_remainder__2
		|| op == expression_type::k_logical_and__2
		|| op == expression_type::k_logical_or__2
		;
}

inline bool is_comparison_expression(expression_type op){
	return false
		|| op == expression_type::k_comparison_smaller_or_equal__2
		|| op == expression_type::k_comparison_smaller__2
		|| op == expression_type::k_comparison_larger_or_equal__2
		|| op == expression_type::k_comparison_larger__2

		|| op == expression_type::k_logical_equal__2
		|| op == expression_type::k_logical_nonequal__2
		;
}

//	Token = as in the source code syntax.
expression_type token_to_expression_type(const std::string& op);
std::string expression_type_to_token(const expression_type& op);


//	"+", "<=", "&&" etc.
inline bool is_simple_expression__2(const std::string& op){
	return
		op == "+" || op == "-" || op == "*" || op == "/" || op == "%"
		|| op == "<=" || op == "<" || op == ">=" || op == ">"
		|| op == "==" || op == "!=" || op == "&&" || op == "||";
}


//	Keywords in source code.
namespace keyword_t {
	const std::string k_update = "update";
	const std::string k_return = "return";
	const std::string k_while = "while";
	const std::string k_for = "for";
	const std::string k_if = "if";
	const std::string k_else = "else";
	const std::string k_func = "func";
	const std::string k_impure = "impure";

	const std::string k_undefined = "**undef**";
	const std::string k_any = "any";
	const std::string k_void = "void";
	const std::string k_false = "false";
	const std::string k_true = "true";
	const std::string k_bool = "bool";

	const std::string k_int = "int";
	const std::string k_double = "double";
	const std::string k_string = "string";
	const std::string k_typeid = "typeid";
	const std::string k_json_value = "json_value";
	const std::string k_struct = "struct";

	const std::string k_mutable = "mutable";
	const std::string k_let = "let";

	const std::string k_software_system = "software-system";
	const std::string k_container_def = "container-def";

	const std::string k_json_object = "json_object";
	const std::string k_json_array = "json_array";
	const std::string k_json_string = "json_string";
	const std::string k_json_number = "json_number";
	const std::string k_json_true = "json_true";
	const std::string k_json_false = "json_false";
	const std::string k_json_null = "json_null";

/*
	"assert",
	"print",
	"to_string",
	"update",
	"size",
*/

/*
	"catch",
	"deserialize()",
	"diff()",
	"ensure",
	"foreach",
	"hash()",
	"invariant",
	"log",
	"namespace",
	"private",
	"property",
	"prove",
	"require",
	"serialize()",
	"swap",
	"switch",
	"tag",
	"test",
	"this",
	"try",
	"typecast",
	"typeof",

	"char",
	code_point",
	"float",
	"float32",
	"float80",
	hash",
	"int16",
	"int32",
	"int64",
	"int8",
	path",
	text"

	clock",
	defect_exception",
	dyn",
	dyn**<>",
	enum",
	exception",
	"dict",
	protocol",
	rights",
	runtime_exception",
	"seq",
	typedef",
*/
};


}	//	floyd

#endif /* floyd_syntax_hpp */
