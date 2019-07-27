//
//  compiler_basics.hpp
//  floyd_speak
//
//  Created by Marcus Zetterquist on 2019-02-05.
//  Copyright © 2019 Marcus Zetterquist. All rights reserved.
//

#ifndef compiler_basics_hpp
#define compiler_basics_hpp

/*
	Infrastructure primitives under the compiler.
*/

#include "quark.h"

struct seq_t;
struct json_t;


namespace floyd {


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
	k_json,

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
base_type string_to_base_type(const std::string& s);

void ut_verify(const quark::call_context_t& context, const base_type& result, const base_type& expected);






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

	k_benchmark
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




////////////////////////////////////////		function_id_t


struct function_id_t {
	std::string name;
};

inline bool operator==(const function_id_t& lhs, const function_id_t& rhs){
	return lhs.name == rhs.name;
}
inline bool operator!=(const function_id_t& lhs, const function_id_t& rhs){
	return lhs.name != rhs.name;
}
inline bool operator<(const function_id_t& lhs, const function_id_t& rhs){
	return lhs.name < rhs.name;
}

const function_id_t k_no_function_id = function_id_t { "" };



////////////////////////////////////////		location_t


//	Specifies character offset inside source code.

struct location_t {
	explicit location_t(std::size_t offset) :
		offset(offset)
	{
	}

	location_t(const location_t& other) = default;
	location_t& operator=(const location_t& other) = default;

	std::size_t offset;
};

inline bool operator==(const location_t& lhs, const location_t& rhs){
	return lhs.offset == rhs.offset;
}
extern const location_t k_no_location;



////////////////////////////////////////		location2_t


struct location2_t {
	location2_t(const std::string& source_file_path, int line_number, int column, std::size_t start, std::size_t end, const std::string& line, const location_t& loc) :
		source_file_path(source_file_path),
		line_number(line_number),
		column(column),
		start(start),
		end(end),
		line(line),
		loc(loc)
	{
	}

	std::string source_file_path;
	int line_number;
	int column;
	std::size_t start;
	std::size_t end;
	std::string line;
	location_t loc;
};


////////////////////////////////////////		compilation_unit_t


struct compilation_unit_t {
	std::string prefix_source;
	std::string program_text;
	std::string source_file_path;
};



////////////////////////////////////////		compiler_error


class compiler_error : public std::runtime_error {
	public: compiler_error(const location_t& location, const location2_t& location2, const std::string& message) :
		std::runtime_error(message),
		location(location),
		location2(location2)
	{
	}

	public: location_t location;
	public: location2_t location2;
};



////////////////////////////////////////		throw_compiler_error()


void throw_compiler_error_nopos(const std::string& message) __dead2;
inline void throw_compiler_error_nopos(const std::string& message){
//	location2_t(const std::string& source_file_path, int line_number, int column, std::size_t start, std::size_t end, const std::string& line) :
	throw compiler_error(k_no_location, location2_t("", 0, 0, 0, 0, "", k_no_location), message);
}



void throw_compiler_error(const location_t& location, const std::string& message) __dead2;
inline void throw_compiler_error(const location_t& location, const std::string& message){
//	location2_t(const std::string& source_file_path, int line_number, int column, std::size_t start, std::size_t end, const std::string& line) :
	throw compiler_error(location, location2_t("", 0, 0, 0, 0, "", k_no_location), message);
}

void throw_compiler_error(const location2_t& location2, const std::string& message) __dead2;
inline void throw_compiler_error(const location2_t& location2, const std::string& message){
	throw compiler_error(location2.loc, location2, message);
}


location2_t find_source_line(const std::string& program, const std::string& file, bool corelib, const location_t& loc);


////////////////////////////////////////		refine_compiler_error_with_loc2()


std::pair<location2_t, std::string> refine_compiler_error_with_loc2(const compilation_unit_t& cu, const compiler_error& e);




////////////////////////////////	MISSING FEATURES



void NOT_IMPLEMENTED_YET() __dead2;
void UNSUPPORTED() __dead2;



void ut_verify_json_and_rest(const quark::call_context_t& context, const std::pair<json_t, seq_t>& result_pair, const std::string& expected_json, const std::string& expected_rest);

void ut_verify(const quark::call_context_t& context, const std::pair<std::string, seq_t>& result, const std::pair<std::string, seq_t>& expected);



}	// floyd

#endif /* compiler_basics_hpp */
