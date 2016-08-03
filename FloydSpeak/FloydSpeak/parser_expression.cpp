//
//  parse_expression.cpp
//  FloydSpeak
//
//  Created by Marcus Zetterquist on 26/07/16.
//  Copyright © 2016 Marcus Zetterquist. All rights reserved.
//

#include "parser_expression.h"

#include "text_parser.h"
#include "parser_statement.h"
#include "parser_value.h"

#include <cmath>

namespace floyd_parser {


using std::pair;
using std::string;
using std::vector;
using std::shared_ptr;
using std::make_shared;



#if false
pair<symbol_path, string> read_required_symbol_path(const string& s){
	const auto a = read_required_single_symbol(s);
	if(peek_compare_char(a.second, '.')){
		const auto b = read_required_symbol_path(a.second.substr(1));
		symbol_path path = symbol_path({a.first}) + b.first;
		return pair<symbol_path, string>(path, b.second);
	}
	else{
		return pair<vector<string>, string>({a.first}, a.second);
	}
}
#endif

/*
	[10]
	[f(3)]
	["troll"]

	Returns a lookup_element_expr_t, including any expression within the brackets.
*/
pair<expression_t, string> parse_lookup(const parser_i& parser, const string& s) {
	QUARK_ASSERT(s.size() > 2);

	const auto pos = skip_whitespace(s);
	const auto body = get_balanced(pos);
	if(body.first.empty()){
		throw std::runtime_error("Illegal index key");
	}

	const auto key_expression_s = trim_ends(body.first);
	expression_t key_expression = parse_expression(parser, key_expression_s);
	return { make_lookup(key_expression), body.second };
}

/*
	"f()"
	"hello(a + b)"
*/
pair<expression_t, string> parse_function_call(const parser_i& parser, const string& s) {
	QUARK_ASSERT(s.size() > 0);

	const auto identifier_pos = read_required_single_symbol(s);

	string p2 = skip_whitespace(identifier_pos.second);

	//	Function call?
	QUARK_ASSERT(!p2.empty() && p2[0] == '(');

	const auto arg_list_pos = get_balanced(p2);
	const auto args = trim_ends(arg_list_pos.first);

	p2 = args;
	vector<expression_t> args_expressions;
	while(!p2.empty()){
		const auto p3 = read_until(skip_whitespace(p2), ",");
		expression_t arg_expr = parse_expression(parser, p3.first);
		args_expressions.push_back(arg_expr);
		p2 = p3.second[0] == ',' ? p3.second.substr(1) : p3.second;
	}

	return { make_function_call(identifier_pos.first, args_expressions), arg_list_pos.second };
}


/*
	Non-constant value.
	Read variable, from lookup, call function, from structure member -- and any mix of these.
	Returns a variable_read_expr_t or a function_call_expr_t -- with simple or complex address expression in it.


	load "[4]"

	call "f()"

	load "my_global"

	load "my_local"

	load "my_global[
		f(
			g(
				test[3 + f()]
			)
		)
	].next["hello"].tail[10]"
*/
/*
	"hello xxx"
	"hello.kitty xxx"
	"hello.kitty.cat xxx"
	"[10] xxx"
	"[10].cat xxx"
	"hello[10] xxx"
	"hello["troll"] xxx"
	"hello["troll"].kitty[10].cat xxx"
	"hello["troll"][10].cat xxx"

	"[x + f(10)].cat xxx"
	"hello[10].func(3).cat xxx"
*/

/*
	Start of identifier: can be variable access or function call.

	"hello2"
	"hello.member"
	"f ()"
	"f(x + 10)"
	"node.print(10)"
	??? can be any expression characters
*/

//??? make recursive!!
pair<expression_t, string> parse_calculated_value(const parser_i& parser, const string& s) {
	QUARK_ASSERT(s.size() > 0);

	const auto pos = skip_whitespace(s);

	//	Lookup? [expression]xxx
	if(peek_compare_char(pos, '[')){
		const auto lookup = parse_lookup(parser, pos);
		return { make_variable_read(lookup.first), lookup.second };
	}

	//	variable name || function call
	else{
		const auto identifier_pos = read_required_single_symbol(pos);

		string p2 = skip_whitespace(identifier_pos.second);

		//	Function call?
		if(!p2.empty() && p2[0] == '('){
			return parse_function_call(parser, pos);
		}

		//	Variable-read.
		else{
			return { make_variable_read_variable(identifier_pos.first), identifier_pos.second };
		}
	}
}

QUARK_UNIT_TESTQ("parse_calculated_value()", ""){
	QUARK_TEST_VERIFY((parse_calculated_value({}, "hello xxx") == pair<expression_t, string>{ make_variable_read_variable("hello"), " xxx" }));
}




pair<expression_t, string> parse_summands(const parser_i& parser, const string& s, int depth);


expression_t negate_expression(const expression_t& e){
	//	Shortcut: directly negate numeric constants. This makes parse tree cleaner and is non-lossy.
	if(e._constant){
		const value_t& value = *e._constant;
		if(value.get_type() == make_type_identifier("int")){
			return make_constant(-value.get_int());
		}
		else if(value.get_type() == make_type_identifier("float")){
			return make_constant(-value.get_float());
		}
	}

	return make_math_operation1(math_operation1_expr_t::negate, e);
}

float parse_float(const string& pos){
	size_t end = -1;
	auto res = std::stof(pos, &end);
	if(isnan(res) || end == 0){
		throw std::runtime_error("EEE_WRONG_CHAR");
	}
	return res;
}


/*
	Constant literal
		"3"
		"3.0"
		"\"three\""
	Function call
		"f ()"
		"f(g())"
		"f(a + "xyz")"
		"f(a + "xyz", 1000 * 3)"
	Variable read
		"x1"
		"hello2"
		"hello.member"

		??? []
	FUTURE
		- Add lambda / local function
*/
pair<expression_t, string> parse_single_internal(const parser_i& parser, const string& s) {
	QUARK_ASSERT(s.size() > 0);

	string pos = s;

	//	" => string constant.
	if(peek_string(pos, "\"")){
		pos = pos.substr(1);
		const auto string_constant_pos = read_until(pos, "\"");

		pos = string_constant_pos.second;
		pos = pos.substr(1);
		return { make_constant(string_constant_pos.first), pos };
	}

	// [0-9] and "."  => numeric constant.
	else if((number_chars + ".").find(pos[0]) != string::npos){
		const auto number_pos = read_while(pos, number_chars + ".");
		if(number_pos.first.empty()){
			throw std::runtime_error("EEE_WRONG_CHAR");
		}

		//	If it contains a "." its a float, else an int.
		if(number_pos.first.find('.') != string::npos){
			pos = pos.substr(number_pos.first.size());
			float value = parse_float(number_pos.first);
			return { make_constant(value), pos };
		}
		else{
			pos = pos.substr(number_pos.first.size());
			int value = atoi(number_pos.first.c_str());
			return { make_constant(value), pos };
		}
	}

	else{
		return parse_calculated_value(parser, pos);
	}
}

pair<expression_t, string> parse_single(const parser_i& parser, const string& s){
	const auto result = parse_single_internal(parser, s);
	trace(result.first);
	return result;
}



struct test_parser : public parser_i {
	public: virtual bool parser_i__is_declared_function(const std::string& s) const{
		return s == "log" || s == "log2" || s == "f" || s == "return5";
	}
	public: virtual bool parser_i__is_declared_constant_value(const std::string& s) const{
		return false;
	}

	bool parser_i__is_known_type(const std::string& s) const{
		return true;
	}
};


QUARK_UNIT_TESTQ("parse_single", "number"){
	test_parser parser;
	QUARK_TEST_VERIFY((parse_single(parser, "9.0") == pair<expression_t, string>(make_constant(9.0f), "")));
}

QUARK_UNIT_TESTQ("parse_single", "function call"){
	test_parser parser;
	const auto a = parse_single(parser, "log(34.5)");
	QUARK_TEST_VERIFY(a.first._call_function_expr->_function_name == "log");
	QUARK_TEST_VERIFY(a.first._call_function_expr->_inputs.size() == 1);
	QUARK_TEST_VERIFY(*a.first._call_function_expr->_inputs[0]->_constant == value_t(34.5f));
	QUARK_TEST_VERIFY(a.second == "");
}

QUARK_UNIT_TESTQ("parse_single", "function call with two args"){
	test_parser parser;
	const auto a = parse_single(parser, "log2(\"123\" + \"xyz\", 1000 * 3)");
	QUARK_TEST_VERIFY(a.first._call_function_expr->_function_name == "log2");
	QUARK_TEST_VERIFY(a.first._call_function_expr->_inputs.size() == 2);
	QUARK_TEST_VERIFY(a.first._call_function_expr->_inputs[0]->_math_operation2_expr);
	QUARK_TEST_VERIFY(a.first._call_function_expr->_inputs[1]->_math_operation2_expr);
	QUARK_TEST_VERIFY(a.second == "");
}

QUARK_UNIT_TESTQ("parse_single", "nested function calls"){
	test_parser parser;
	const auto a = parse_single(parser, "log2(2.1, f(3.14))");
	QUARK_TEST_VERIFY(a.first._call_function_expr->_function_name == "log2");
	QUARK_TEST_VERIFY(a.first._call_function_expr->_inputs.size() == 2);
	QUARK_TEST_VERIFY(a.first._call_function_expr->_inputs[0]->_constant);
	QUARK_TEST_VERIFY(a.first._call_function_expr->_inputs[1]->_call_function_expr->_function_name == "f");
	QUARK_TEST_VERIFY(a.first._call_function_expr->_inputs[1]->_call_function_expr->_inputs.size() == 1);
	QUARK_TEST_VERIFY(*a.first._call_function_expr->_inputs[1]->_call_function_expr->_inputs[0] == make_constant(3.14f));
	QUARK_TEST_VERIFY(a.second == "");
}

QUARK_UNIT_TESTQ("parse_single", "variable read"){
	test_parser parser;
	const auto a = pair<expression_t, string>(make_variable_read_variable("k_my_global"), "");
	const auto b = parse_single({}, "k_my_global");
	QUARK_TEST_VERIFY(a == b);
}

#if false
QUARK_UNIT_TESTQ("parse_single", "read struct member"){
	test_parser parser;
	QUARK_TEST_VERIFY((parse_single(parser, "k_my_global.member") == pair<expression_t, string>(make_variable_read("k_my_global.member"), "")));
}
#endif


// Parse a constant or an expression in parenthesis
pair<expression_t, string> parse_atom(const parser_i& parser, const string& s, int depth) {
	string pos = skip_whitespace(s);

	//	Handle the sign before parenthesis (or before number)
	auto negative = false;
	while(pos.size() > 0 && (is_whitespace(pos[0]) || (pos[0] == '-' || pos[0] == '+'))){
		if(pos[0] == '-') {
			negative = !negative;
		}
		pos = pos.substr(1);
	}

	// Check if there is parenthesis
	if(pos.size() > 0 && pos[0] == '(') {
		pos = pos.substr(1);

		auto res = parse_summands(parser, pos, depth);
		pos = skip_whitespace(res.second);
		if(!(res.second.size() > 0 && res.second[0] == ')')) {
			// Unmatched opening parenthesis
			throw std::runtime_error("EEE_PARENTHESIS");
		}
		pos = pos.substr(1);
		const auto expr = res.first;
		return { negative ? negate_expression(expr) : expr, pos };
	}

	//	Parse constant literal / function call / variable-access.
	if(pos.size() > 0){
		const auto single_pos = parse_single(parser, pos);
		return { negative ? negate_expression(single_pos.first) : single_pos.first, single_pos.second };
	}

	throw std::runtime_error("Expected number");
}



//### more tests here!
QUARK_UNIT_TEST("", "parse_atom", "", ""){
	test_parser parser;

	QUARK_TEST_VERIFY((parse_atom(parser, "0.0", 0) == pair<expression_t, string>(make_constant(0.0f), "")));
	QUARK_TEST_VERIFY((parse_atom(parser, "9.0", 0) == pair<expression_t, string>(make_constant(9.0f), "")));
	QUARK_TEST_VERIFY((parse_atom(parser, "12345.0", 0) == pair<expression_t, string>(make_constant(12345.0f), "")));

	QUARK_TEST_VERIFY((parse_atom(parser, "10.0", 0) == pair<expression_t, string>(make_constant(10.0f), "")));
	QUARK_TEST_VERIFY((parse_atom(parser, "-10.0", 0) == pair<expression_t, string>(make_constant(-10.0f), "")));
	QUARK_TEST_VERIFY((parse_atom(parser, "+10.0", 0) == pair<expression_t, string>(make_constant( 10.0f), "")));

	QUARK_TEST_VERIFY((parse_atom(parser, "4.0+", 0) == pair<expression_t, string>(make_constant(4.0f), "+")));


	QUARK_TEST_VERIFY((parse_atom(parser, "\"hello\"", 0) == pair<expression_t, string>(make_constant("hello"), "")));
}


pair<expression_t, string> parse_factors(const parser_i& parser, const string& s, int depth) {
	const auto num1_pos = parse_atom(parser, s, depth);
	auto result_expression = num1_pos.first;
	string pos = skip_whitespace(num1_pos.second);
	while(!pos.empty() && (pos[0] == '*' || pos[0] == '/')){
		const auto op_pos = read_char(pos);
		const auto expression2_pos = parse_atom(parser, op_pos.second, depth);
		if(op_pos.first == '/') {
			result_expression = make_math_operation2(math_operation2_expr_t::divide, result_expression, expression2_pos.first);
		}
		else{
			result_expression = make_math_operation2(math_operation2_expr_t::multiply, result_expression, expression2_pos.first);
		}
		pos = skip_whitespace(expression2_pos.second);
	}
	return { result_expression, pos };
}

pair<expression_t, string> parse_summands(const parser_i& parser, const string& s, int depth) {
	const auto num1_pos = parse_factors(parser, s, depth);
	auto result_expression = num1_pos.first;
	string pos = num1_pos.second;
	while(!pos.empty() && (pos[0] == '-' || pos[0] == '+')) {
		const auto op_pos = read_char(pos);

		const auto expression2_pos = parse_factors(parser, op_pos.second, depth);
		if(op_pos.first == '-'){
			result_expression = make_math_operation2(math_operation2_expr_t::subtract, result_expression, expression2_pos.first);
		}
		else{
			result_expression = make_math_operation2(math_operation2_expr_t::add, result_expression, expression2_pos.first);
		}

		pos = skip_whitespace(expression2_pos.second);
	}
	return { result_expression, pos };
}



expression_t parse_expression(const parser_i& parser, string expression){
	if(expression.empty()){
		throw std::runtime_error("EEE_WRONG_CHAR");
	}
	auto result = parse_summands(parser, expression, 0);
	if(!result.second.empty()){
		throw std::runtime_error("EEE_WRONG_CHAR");
	}

	QUARK_TRACE("Expression: \"" + expression + "\"");
	trace(result.first);
	return result.first;
}


QUARK_UNIT_TESTQ("parse_expression()", ""){
	const auto a = parse_expression({}, "pixel( \"hiya\" )");
	QUARK_TEST_VERIFY(a._call_function_expr);
}

#if false
QUARK_UNIT_TESTQ("parse_expression()", ""){
	const auto a = parse_expression({}, "pixel.red");
	QUARK_TEST_VERIFY(a._variable_read_expr);
}
#endif



//////////////////////////////////////////////////		test rig




ast_t make_test_ast(){
	ast_t result;
	result._types_collector = result._types_collector.define_function_type("log", make_log_function());
	result._types_collector = result._types_collector.define_function_type("log2", make_log2_function());
	result._types_collector = result._types_collector.define_function_type("f", make_log_function());
	result._types_collector = result._types_collector.define_function_type("return5", make_return5());

	result._types_collector = result._types_collector.define_struct_type("test_struct0", make_struct0());
	result._types_collector = result._types_collector.define_struct_type("test_struct1", make_struct1());
	return result;
}

QUARK_UNIT_TESTQ("make_test_ast()", ""){
	auto a = make_test_ast();
	QUARK_TEST_VERIFY(*a._types_collector.resolve_struct_type("test_struct0") == make_struct0());
	QUARK_TEST_VERIFY(*a._types_collector.resolve_struct_type("test_struct1") == make_struct1());
}




}	//	floyd_parser




