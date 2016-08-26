//
//  parse_expression.cpp
//  FloydSpeak
//
//  Created by Marcus Zetterquist on 26/07/16.
//  Copyright © 2016 Marcus Zetterquist. All rights reserved.
//

#include "parse_expression.h"

#include "text_parser.h"
#include "statements.h"
#include "parser_value.h"
#include "parse_function_def.h"
#include "parser_ast.h"
#include "parser_primitives.h"
#include "parser2.h"

namespace floyd_parser {


using std::pair;
using std::string;
using std::vector;
using std::shared_ptr;
using std::make_shared;




seq to_seq(const pair<expression_t, string>& p){
	return seq(expression_to_json_string(p.first), p.second);
}



/*
	[<expression>]...

	[10]
	[f(3)]
	["troll"]

	Returns a lookup_element_expr_t, including any expression within the brackets.
*/
pair<expression_t, string> parse_lookup(const expression_t& leftside, const string& s) {
	QUARK_ASSERT(s.size() > 2);

	const auto pos = skip_whitespace(s);
	const auto body = get_balanced(pos);
	if(body.first.empty()){
		throw std::runtime_error("Illegal index key");
	}

	const auto key_expression_s = trim_ends(body.first);
	expression_t key_expression = parse_expression(key_expression_s);
	return { expression_t::make_lookup(leftside, key_expression), body.second };
}

/*
	"f()"
	"hello(a + b)"
*/
pair<expression_t, string> parse_function_call(const std::shared_ptr<expression_t>& leftside, const string& s) {
	QUARK_ASSERT(s.size() > 0);

	const auto identifier_pos = read_required_single_symbol(s);

	string p2 = skip_whitespace(identifier_pos.second);

	//	Function call?
	QUARK_ASSERT(!p2.empty() && p2[0] == '(');

	const auto arg_list_pos = get_balanced(p2);
	const auto args = trim_ends(arg_list_pos.first);

	p2 = args;
	vector<shared_ptr<expression_t>> args_expressions;
	while(!p2.empty()){
		const auto p3 = read_until(skip_whitespace(p2), ",");
		expression_t arg_expr = parse_expression(p3.first);
		args_expressions.push_back(make_shared<expression_t>(arg_expr));
		p2 = p3.second[0] == ',' ? p3.second.substr(1) : p3.second;
	}

	return {
		expression_t::make_function_call(type_identifier_t::make(identifier_pos.first), args_expressions, type_identifier_t()),
		arg_list_pos.second
	};
}

pair<expression_t, string> parse_path_node(const std::shared_ptr<expression_t>& leftside, const string& s) {
	QUARK_ASSERT(s.size() > 0);

	const auto pos = skip_whitespace(s);

	//	Lookup? [expression]xxx
	if(peek_compare_char(pos, '[')){
		if(!leftside){
			throw std::runtime_error("[] needs to operate on a value");
		}
		const auto lookup = parse_lookup(*leftside.get(), pos);
		return lookup;
	}

	//	variable name || function call
	else{
		const auto identifier_pos = read_required_single_symbol(pos);

		string p2 = skip_whitespace(identifier_pos.second);

		//	Function call?
		if(!p2.empty() && p2[0] == '('){
			return parse_function_call(leftside, pos);
		}

		//	Variable.
		else{
			if(!leftside){
				return { expression_t::make_resolve_variable(identifier_pos.first, type_identifier_t()), identifier_pos.second };
			}
			else{
				return { expression_t::make_resolve_member(leftside, identifier_pos.first), identifier_pos.second };
			}
		}
	}
}

QUARK_UNIT_TESTQ("parse_path_node()", ""){
	quark::ut_compare(to_seq(parse_path_node({}, "hello xxx")), seq(R"(["@", "hello"])", " xxx"));
}

QUARK_UNIT_TESTQ("parse_path_node()", ""){
	quark::ut_compare(to_seq(parse_path_node({}, "f () xxx")), seq(R"(["call", "f", []])", " xxx"));
}

QUARK_UNIT_TESTQ("parse_path_node()", ""){
	quark::ut_compare(to_seq(parse_path_node({}, "f (x + 10) xxx")), seq(R"(["call", "f", [["+", ["load", ["@", "x"]], ["k", "<int>", 10]]]])", " xxx"));
}

QUARK_UNIT_TESTQ("parse_path_node()", ""){
//	quark::ut_compare(to_seq(parse_path_node({}, "[10] xxx")), seq("(@lookup nullptr (@k <int>10))", " xxx"));
}


pair<expression_t, string> parse_calculated_value_recursive(const std::shared_ptr<expression_t>& leftside, const string& s) {
	QUARK_ASSERT(s.size() > 0);

	//	Read leftmost element in path. "hello" or "f(1 + 2)" or "[1 + 2]".
	const auto a = parse_path_node(leftside, s);

	//	Is there a chain of resolves, like "kitty" in "hello.kitty"?
	const auto pos = skip_whitespace(a.second);
	if(peek_compare_char(pos, '.')){
		return parse_calculated_value_recursive(make_shared<expression_t>(a.first), pos.substr(1));
	}
	else if(peek_compare_char(pos, '[')){
		return parse_calculated_value_recursive(make_shared<expression_t>(a.first), pos);
	}
	else{
		return a;
	}
}



/*
	Parse non-constant value.
	This is a recursive function since you can use lookups, function calls, structure members - all nested.

	Each step in the path can be one of these:
	1) *read* from a variable / constant, structure member.
	2) Function call
	3) Looking up using []

	Returns either a variable_read_expr_t or a function_call_expr_t. The hold potentially many levels of nested lookups, function calls and member reads.

	load "[4]"
	call "f()"
	load "my_global"
	load "my_local"


	Examples:
		"hello xxx"
		"hello.kitty xxx"
		"f ()"
		"f(x + 10)"
		"hello[10] xxx"
		"hello["troll"] xxx"
		"hello["troll"].kitty[10].cat xxx"
*/

//??? BREAK OUT PARSE_ADDRESS, WITHOUT THE LOAD.

pair<expression_t, string> parse_calculated_value(const string& s) {
	const auto a = parse_calculated_value_recursive(shared_ptr<expression_t>(), s);
	if(a.first._resolve_variable){
		return { expression_t::make_load(a.first, type_identifier_t()), a.second };
	}
	else if(a.first._resolve_member){
		return { expression_t::make_load(a.first, type_identifier_t()), a.second };
	}
	else if(a.first._lookup_element){
		return { expression_t::make_load(a.first, type_identifier_t()), a.second };
	}
	else if(a.first._call){
		return a;
	}
	else{
		QUARK_ASSERT(false);
	}
}

/*
??? Is this a test?
	struct pixel { int red; int green; int blue; };
	struct image { pixel background_color; int width; int height; };

	int main(int magic){
		image i = image_constructor();

		//	Read int-member of struct
		int width = i.width;

		//	Read int-member of a struct inside another struct.
		int red = image.background_color.red;

		//	Read local variable.
		int width2 = width * 2;

		//	Read argument.
		int magic2 = magic;
		return a;
	}
*/



QUARK_UNIT_TESTQ("parse_calculated_value()", ""){
	quark::ut_compare(to_seq(parse_calculated_value("hello xxx")), seq(R"(["load", ["@", "hello"]])", " xxx"));
}

QUARK_UNIT_TESTQ("parse_calculated_value()", ""){
	quark::ut_compare(to_seq(parse_calculated_value("hello.kitty xxx")), seq(R"(["load", ["->", ["@", "hello"], "kitty"]])", " xxx"));
}

QUARK_UNIT_TESTQ("parse_calculated_value()", ""){
	quark::ut_compare(to_seq(parse_calculated_value("hello.kitty.cat xxx")), seq(R"(["load", ["->", ["->", ["@", "hello"], "kitty"], "cat"]])", " xxx"));
}

QUARK_UNIT_TESTQ("parse_calculated_value()", ""){
	quark::ut_compare(to_seq(parse_calculated_value("f () xxx")), seq(R"(["call", "f", []])", " xxx"));
}

QUARK_UNIT_TESTQ("parse_calculated_value()", ""){
	quark::ut_compare(to_seq(parse_calculated_value("f (x + 10) xxx")), seq(R"(["call", "f", [["+", ["load", ["@", "x"]], ["k", "<int>", 10]]]])", " xxx"));
}

QUARK_UNIT_TESTQ("parse_calculated_value()", ""){
	quark::ut_compare(to_seq(parse_calculated_value("hello[10] xxx")), seq(R"(["load", ["[-]", ["@", "hello"], ["k", "<int>", 10]]])", " xxx"));
}

QUARK_UNIT_TESTQ("parse_calculated_value()", ""){
	quark::ut_compare(to_seq(parse_calculated_value("hello[\"troll\"] xxx")), seq(R"(["load", ["[-]", ["@", "hello"], ["k", "<string>", "troll"]]])", " xxx"));
}

//### allow nl and tab when writing result strings.
QUARK_UNIT_TESTQ("parse_calculated_value()", ""){
	quark::ut_compare(to_seq(parse_calculated_value("hello[\"troll\"].kitty[10].cat xxx")), seq(R"(["load", ["->", ["[-]", ["->", ["[-]", ["@", "hello"], ["k", "<string>", "troll"]], "kitty"], ["k", "<int>", 10]], "cat"]])", " xxx"));
}

/*
	??? more tests

	"hello["troll"][10].cat xxx"
	"hello[10].func(3).cat xxx"

	"my_global[
		f(
			g(
				test[3 + f()]
			)
		)
	].next["hello"].tail[10]"
*/


pair<expression_t, string> parse_expression(string expression, int depth);


expression_t negate_expression(const expression_t& e){
	//	Shortcut: directly negate numeric constants. This makes parse tree cleaner and is non-lossy.
	if(e._constant){
		const value_t& value = *e._constant;
		if(value.get_type() == type_identifier_t::make_int()){
			return expression_t::make_constant(-value.get_int());
		}
		else if(value.get_type() == type_identifier_t::make_float()){
			return expression_t::make_constant(-value.get_float());
		}
	}

	return expression_t::make_math_operation1(math_operation1_expr_t::negate, e, type_identifier_t());
}

// [0-9] and "."  => numeric constant.
pair<value_t, string> parse_numeric_constant(const string& s) {
	QUARK_ASSERT(s.size() > 0);
	QUARK_ASSERT((number_chars + ".").find(s[0]) != string::npos);

	const auto number_pos = read_while(s, number_chars + ".");
	if(number_pos.first.empty()){
		throw std::runtime_error("EEE_WRONG_CHAR");
	}

	//	If it contains a "." its a float, else an int.
	if(number_pos.first.find('.') != string::npos){
		const auto pos = s.substr(number_pos.first.size());
		float value = parse_float(number_pos.first);
		return { value_t(value), pos };
	}
	else{
		const auto pos = s.substr(number_pos.first.size());
		int value = atoi(number_pos.first.c_str());
		return { value_t(value), pos };
	}
}

QUARK_UNIT_TESTQ("parse_numeric_constant()", ""){
	quark::ut_compare(parse_numeric_constant("0 xxx"), pair<value_t, string>(value_t(0), " xxx"));
}

QUARK_UNIT_TESTQ("parse_numeric_constant()", ""){
	quark::ut_compare(parse_numeric_constant("1234 xxx"), pair<value_t, string>(value_t(1234), " xxx"));
}

QUARK_UNIT_TESTQ("parse_numeric_constant()", ""){
	quark::ut_compare(parse_numeric_constant(".5 xxx"), pair<value_t, string>(value_t(0.5f), " xxx"));
}




/*
	Constant literal
		"3"
		"3.0"
		"\"three\""
		"true"
		"false"
	Function call
		"f ()"
		"f(g())"
		"f(a + "xyz")"
		"f(a + "xyz", 1000 * 3)"
	Variable read
		"x1"
		"hello2"
		"hello.member"

		x[10 + f()]
*/
pair<expression_t, string> parse_single(const string& s) {
	QUARK_ASSERT(s.size() > 0);

	//	" => string constant.
	if(peek_string(s, "\"")){
		const auto a = parse_string_literal(seq_t(s));
		return { expression_t::make_constant(a.first), a.second.get_all() };
	}

	// [0-9] and "."  => numeric constant.
	else if((number_chars + ".").find(s[0]) != string::npos){
		const auto n = parse_numeric_constant(s);
		return { expression_t::make_constant(n.first), n.second };
	}

	else if(peek_string(s, "true")){
		return { expression_t::make_constant(true), read_required_string(s, "true") };
	}
	else if(peek_string(s, "false")){
		return { expression_t::make_constant(false), read_required_string(s, "false") };
	}
	else{
		return parse_calculated_value(s);
	}
}

QUARK_UNIT_TESTQ("parse_single", ""){
	QUARK_TEST_VERIFY((parse_single("truexyz") == pair<expression_t, string>(expression_t::make_constant(true), "xyz")));
}
QUARK_UNIT_TESTQ("parse_single", ""){
	QUARK_TEST_VERIFY((parse_single("falsexyz") == pair<expression_t, string>(expression_t::make_constant(false), "xyz")));
}

QUARK_UNIT_TESTQ("parse_single", ""){
	QUARK_TEST_VERIFY((parse_single("\"\"") == pair<expression_t, string>(expression_t::make_constant(""), "")));
}
QUARK_UNIT_TESTQ("parse_single", ""){
	QUARK_TEST_VERIFY((parse_single("\"abcd\"") == pair<expression_t, string>(expression_t::make_constant("abcd"), "")));
}

QUARK_UNIT_TESTQ("parse_single", "number"){
	QUARK_TEST_VERIFY((parse_single("9.0") == pair<expression_t, string>(expression_t::make_constant(9.0f), "")));
}

QUARK_UNIT_TESTQ("parse_single", "function call"){
	const auto a = parse_single("log(34.5)");
	QUARK_TEST_VERIFY(a.first._call->_function.to_string() == "log");
	QUARK_TEST_VERIFY(a.first._call->_inputs.size() == 1);
	QUARK_TEST_VERIFY(*a.first._call->_inputs[0]->_constant == value_t(34.5f));
	QUARK_TEST_VERIFY(a.second == "");
}

QUARK_UNIT_TESTQ("parse_single", "function call with two args"){
	const auto a = parse_single("log2(\"123\" + \"xyz\", 1000 * 3)");
	QUARK_TEST_VERIFY(a.first._call->_function.to_string() == "log2");
	QUARK_TEST_VERIFY(a.first._call->_inputs.size() == 2);
	QUARK_TEST_VERIFY(a.first._call->_inputs[0]->_math2);
	QUARK_TEST_VERIFY(a.first._call->_inputs[1]->_math2);
	QUARK_TEST_VERIFY(a.second == "");
}

QUARK_UNIT_TESTQ("parse_single", "nested function calls"){
	const auto a = parse_single("log2(2.1, f(3.14))");
	QUARK_TEST_VERIFY(a.first._call->_function.to_string() == "log2");
	QUARK_TEST_VERIFY(a.first._call->_inputs.size() == 2);
	QUARK_TEST_VERIFY(a.first._call->_inputs[0]->_constant);
	QUARK_TEST_VERIFY(a.first._call->_inputs[1]->_call->_function.to_string() == "f");
	QUARK_TEST_VERIFY(a.first._call->_inputs[1]->_call->_inputs.size() == 1);
	QUARK_TEST_VERIFY(*a.first._call->_inputs[1]->_call->_inputs[0] == expression_t::make_constant(3.14f));
	QUARK_TEST_VERIFY(a.second == "");
}

QUARK_UNIT_TESTQ("parse_single", "variable read"){
	const auto a = pair<expression_t, string>(expression_t::make_load_variable("k_my_global"), "");
	const auto b = parse_single("k_my_global");
	QUARK_TEST_VERIFY(a == b);
}

QUARK_UNIT_TESTQ("parse_single", "read struct member"){
	quark::ut_compare(to_seq(parse_single("k_my_global.member")),  seq(R"(["load", ["->", ["@", "k_my_global"], "member"]])", ""));
}


/*
	Parse a single constant or an expression in parenthesis

	"123"
	"-123"
	"--+-123"
	"(123 + 123 * x + f(y*3))"
*/

pair<expression_t, string> parse_atom(const string& s, int depth) {
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

		auto res = parse_expression(pos, depth);
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
		const auto single_pos = parse_single(pos);
		return { negative ? negate_expression(single_pos.first) : single_pos.first, single_pos.second };
	}

	throw std::runtime_error("Expected number");
}

//### more tests here!
QUARK_UNIT_TESTQ("parse_atom", ""){
	QUARK_TEST_VERIFY((parse_atom("0.0", 0) == pair<expression_t, string>(expression_t::make_constant(0.0f), "")));
}

QUARK_UNIT_TESTQ("parse_atom", ""){
	QUARK_TEST_VERIFY((parse_atom("9.0", 0) == pair<expression_t, string>(expression_t::make_constant(9.0f), "")));
}

QUARK_UNIT_TESTQ("parse_atom", ""){
	QUARK_TEST_VERIFY((parse_atom("12345.0", 0) == pair<expression_t, string>(expression_t::make_constant(12345.0f), "")));
}

QUARK_UNIT_TESTQ("parse_atom", ""){
	QUARK_TEST_VERIFY((parse_atom("10.0", 0) == pair<expression_t, string>(expression_t::make_constant(10.0f), "")));
}

QUARK_UNIT_TESTQ("parse_atom", ""){
	QUARK_TEST_VERIFY((parse_atom("-10.0", 0) == pair<expression_t, string>(expression_t::make_constant(-10.0f), "")));
}

QUARK_UNIT_TESTQ("parse_atom", ""){
	QUARK_TEST_VERIFY((parse_atom("+10.0", 0) == pair<expression_t, string>(expression_t::make_constant( 10.0f), "")));
}

QUARK_UNIT_TESTQ("parse_atom", ""){
	QUARK_TEST_VERIFY((parse_atom("4.0+", 0) == pair<expression_t, string>(expression_t::make_constant(4.0f), "+")));
}

QUARK_UNIT_TESTQ("parse_atom", ""){
	QUARK_TEST_VERIFY((parse_atom("\"hello\"", 0) == pair<expression_t, string>(expression_t::make_constant("hello"), "")));
}
//??? check function calls with paths.


pair<expression_t, string> parse_factors(const string& s, int depth) {
	const auto num1_pos = parse_atom(s, depth);
	auto result_expression = num1_pos.first;
	string pos = skip_whitespace(num1_pos.second);
	while(!pos.empty() && (pos[0] == '*' || pos[0] == '/')){
		const auto op_pos = read_char(pos);
		const auto expression2_pos = parse_atom(op_pos.second, depth);
		if(op_pos.first == '/') {
			result_expression = expression_t::make_math_operation2(math_operation2_expr_t::divide, result_expression, expression2_pos.first);
		}
		else{
			result_expression = expression_t::make_math_operation2(math_operation2_expr_t::multiply, result_expression, expression2_pos.first);
		}
		pos = skip_whitespace(expression2_pos.second);
	}
	return { result_expression, pos };
}

pair<expression_t, string> parse_summands(const string& s, int depth) {
	QUARK_ASSERT(depth >= 0);

	const auto num1_pos = parse_factors(s, depth);
	auto result_expression = num1_pos.first;
	string pos = num1_pos.second;
	while(!pos.empty() && (pos[0] == '-' || pos[0] == '+')) {
		const auto op_pos = read_char(pos);

		const auto expression2_pos = parse_factors(op_pos.second, depth);
		if(op_pos.first == '-'){
			result_expression = expression_t::make_math_operation2(math_operation2_expr_t::subtract, result_expression, expression2_pos.first);
		}
		else{
			result_expression = expression_t::make_math_operation2(math_operation2_expr_t::add, result_expression, expression2_pos.first);
		}

		pos = skip_whitespace(expression2_pos.second);
	}
	return { result_expression, pos };
}


pair<expression_t, string> parse_expression(string expression, int depth){
	QUARK_ASSERT(depth >= 0);

	if(expression.empty()){
		throw std::runtime_error("EEE_WRONG_CHAR");
	}
	auto result = parse_summands(expression, 0);

	QUARK_TRACE("Expression: \"" + expression + "\"");
	trace(result.first);
	return result;
}


expression_t parse_expression(string expression){
	const auto result = parse_expression(expression, 0);
	if(!result.second.empty()){
		throw std::runtime_error("EEE_WRONG_CHAR");
	}
	return result.first;
}

QUARK_UNIT_TESTQ("parse_expression()", ""){
	const auto a = parse_expression("pixel( \"hiya\" )");
	QUARK_TEST_VERIFY(a._call);
}

QUARK_UNIT_TESTQ("parse_expression()", ""){
	quark::ut_compare(expression_to_json_string(parse_expression("pixel.red")), R"(["load", ["->", ["@", "pixel"], "red"]])");
}


#if false
QUARK_UNIT_TESTQ("parse_expression()", ""){
	quark::ut_compare(expression_to_json_string(parse_expression("input_flag ? 100 + 10 * 2 : 1000 - 3 * 4")), R"(["load", ["->", ["@", "pixel"], "red"]])");
}

QUARK_UNIT_TESTQ("parse_expression()", ""){
	quark::ut_compare(expression_to_json_string(parse_expression("input_flag ? \"123\" : \"456\"")), R"(["load", ["->", ["@", "pixel"], "red"]])");
}
#endif





//////////////////////////////////////////////////		test rig



ast_t make_test_ast(){
	ast_t result;
	result._global_scope = result._global_scope->set_types(define_function_type(result._global_scope->_types_collector, "log", make_log_function(result._global_scope)));
	result._global_scope = result._global_scope->set_types(define_function_type(result._global_scope->_types_collector, "log2", make_log2_function(result._global_scope)));
	result._global_scope = result._global_scope->set_types(define_function_type(result._global_scope->_types_collector, "f", make_log_function(result._global_scope)));
	result._global_scope = result._global_scope->set_types(define_function_type(result._global_scope->_types_collector, "return5", make_return5(result._global_scope)));

	result._global_scope = result._global_scope->set_types(define_struct_type(result._global_scope->_types_collector, "test_struct0", make_struct0(result._global_scope)));
	result._global_scope = result._global_scope->set_types(define_struct_type(result._global_scope->_types_collector, "test_struct1", make_struct1(result._global_scope)));
	return result;
}

QUARK_UNIT_TESTQ("make_test_ast()", ""){
	auto a = make_test_ast();
	QUARK_TEST_VERIFY(*resolve_struct_type(a._global_scope->_types_collector, "test_struct0") == *make_struct0(a._global_scope));
	QUARK_TEST_VERIFY(*resolve_struct_type(a._global_scope->_types_collector, "test_struct1") == *make_struct1(a._global_scope));
}






}	//	floyd_parser

