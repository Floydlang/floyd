//
//  floyd_syntax.hpp
//  Floyd
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

#include "quark.h"

#include <string>
#include <map>

namespace floyd {

namespace parser {

/*
	C99-language constants.
	http://en.cppreference.com/w/cpp/language/operator_precedence
*/
const std::string k_c99_number_chars = "0123456789.";
const std::string k_c99_whitespace_chars = " \n\t\r";
const std::string k_c99_identifier_chars = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_";


const std::string k_whitespace_chars = " \n\t";
const std::string k_identifier_chars = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_";
const std::string k_bracket_pairs = "(){}[]";

const std::string k_valid_expression_chars = k_c99_identifier_chars + k_c99_number_chars + k_c99_whitespace_chars + "+-*/%" + "\"[](){}.?:=!<>&,|#$\\;\'" + "@";




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



///////////////////////////////////			keyword_t


//	Keywords in source code.
namespace keyword_t {
	const std::string k_return = "return";
	const std::string k_while = "while";
	const std::string k_for = "for";
	const std::string k_if = "if";
	const std::string k_else = "else";
	const std::string k_func = "func";
	const std::string k_impure = "impure";

	const std::string k_undefined = "undef";
	const std::string k_any = "any";
	const std::string k_void = "void";
	const std::string k_false = "false";
	const std::string k_true = "true";
	const std::string k_bool = "bool";

	const std::string k_int = "int";
	const std::string k_double = "double";
	const std::string k_string = "string";
	const std::string k_typeid = "typeid";
	const std::string k_json = "json";
	const std::string k_struct = "struct";

	const std::string k_mutable = "mutable";
	const std::string k_let = "let";

	const std::string k_software_system = "software-system-def";
	const std::string k_container_def = "container-def";
	const std::string k_benchmark_def = "benchmark-def";
	const std::string k_benchmark = "benchmark";

	const std::string k_json_object = "json_object";
	const std::string k_json_array = "json_array";
	const std::string k_json_string = "json_string";
	const std::string k_json_number = "json_number";
	const std::string k_json_true = "json_true";
	const std::string k_json_false = "json_false";
	const std::string k_json_null = "json_null";
};

//	Future keywords?
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


}	//	parser
}	//	floyd

#endif /* floyd_syntax_hpp */
