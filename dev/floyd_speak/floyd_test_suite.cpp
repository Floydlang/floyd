//
//  floyd_test_suite.cpp
//  FloydSpeak
//
//  Created by Marcus Zetterquist on 2018-01-21.
//  Copyright © 2018 Marcus Zetterquist. All rights reserved.
//

#include "floyd_test_suite.hpp"


#include "bytecode_interpreter/floyd_interpreter.h"
#include "ast_value.h"
#include "expression.h"
#include "json_parser.h"
#include "text_parser.h"
#include "host_functions.h"
#include "FileHandling.h"

#include <string>
#include <vector>
#include <iostream>


using std::vector;
using std::string;

using namespace floyd;




std::vector<std::string> program_recording;

///////////////////////////////////////////////////////////////////////////////////////
//	FLOYD LANGUAGE TEST SUITE
///////////////////////////////////////////////////////////////////////////////////////


value_t test__run_return_result(const std::string& program, const std::vector<value_t>& args){
	program_recording.push_back(program);

	const auto r = run_main(program, args);
	print_vm_printlog(*r.first);
	const auto result = get_global(*r.first, "result");
	return result;
}

void test__run_init__check_result(const std::string& program, const value_t& expected_result){
	const auto result = test__run_return_result(program, {});
	ut_compare_jsons(
		expression_to_json(expression_t::make_literal(result))._value,
		expression_to_json(expression_t::make_literal(expected_result))._value
	);
}

std::shared_ptr<interpreter_t> test__run_global(const std::string& program){
	program_recording.push_back(program);
	const auto result = run_global(program);
	return result;
}

void test__run_main(const std::string& program, const vector<floyd::value_t>& args, const value_t& expected_return){
	program_recording.push_back(program);

	const auto result = run_main(program, args);
	ut_compare_jsons(
		expression_to_json(expression_t::make_literal(result.second))._value,
		expression_to_json(expression_t::make_literal(expected_return))._value
	);
}

void test_result(const std::string& program, const std::string& expected_json){
	const auto result = test__run_return_result(program, {});
	const auto expected_json2 = parse_json(seq_t(expected_json));
	const auto result_json = value_and_type_to_ast_json(result)._value;
	ut_compare_jsons(result_json, expected_json2.first);
}




void ut_compare_stringvects(const vector<string>& result, const vector<string>& expected){
	if(result != expected){
		if(result.size() != expected.size()){
			QUARK_TRACE("Vector are different sizes!");
		}
		const auto count = std::min(result.size(), expected.size());
		for(int i = 0 ; i < count ; i++){
			QUARK_SCOPED_TRACE(std::to_string(i));

			quark::ut_compare_strings(result[i], expected[i]);
		}

		::quark::on_unit_test_failed_hook(
			::quark::get_runtime(),
			::quark::source_code_location(__FILE__, __LINE__),
			""
		);
	}
}

void ut_compare_values(const value_t& result, const value_t& expected){
	if(result != expected){
		QUARK_TRACE("result:");
		QUARK_TRACE(json_to_pretty_string(value_and_type_to_ast_json(result)._value));
		QUARK_TRACE("expected:");
		QUARK_TRACE(json_to_pretty_string(value_and_type_to_ast_json(expected)._value));

		QUARK_UT_VERIFY(false);
	}
}


//////////////////////////////////////////		TEST GLOBAL CONSTANTS


QUARK_UNIT_TEST("Floyd test suite", "Global int variable", "", ""){
	test__run_init__check_result("let int result = 123", value_t::make_int(123));
}

QUARK_UNIT_TEST("Floyd test suite", "bool constant expression", "", ""){
	test__run_init__check_result("let bool result = true", value_t::make_bool(true));
}
QUARK_UNIT_TESTQ("Floyd test suite", "bool constant expression"){
	test__run_init__check_result("let bool result = false", value_t::make_bool(false));
}

QUARK_UNIT_TESTQ("Floyd test suite", "int constant expression"){
	test__run_init__check_result("let int result = 123", value_t::make_int(123));
}

QUARK_UNIT_TESTQ("Floyd test suite", "double constant expression"){
	test__run_init__check_result("let double result = 3.5", value_t::make_double(double(3.5)));
}

QUARK_UNIT_TEST("Floyd test suite", "string constant expression", "", ""){
	test__run_init__check_result("let string result = \"xyz\"", value_t::make_string("xyz"));
}


//////////////////////////////////////////		BASIC EXPRESSIONS


QUARK_UNIT_TEST("Floyd test suite", "+", "", "") {
	test__run_init__check_result("let int result = 1 + 2", value_t::make_int(3));
}
QUARK_UNIT_TESTQ("Floyd test suite", "+") {
	test__run_init__check_result("let int result = 1 + 2 + 3", value_t::make_int(6));
}
QUARK_UNIT_TESTQ("Floyd test suite", "*") {
	test__run_init__check_result("let int result = 3 * 4", value_t::make_int(12));
}

QUARK_UNIT_TESTQ("Floyd test suite", "parant") {
	test__run_init__check_result("let int result = (3 * 4) * 5", value_t::make_int(60));
}

//??? test all types, like [int] etc.

QUARK_UNIT_TEST("Floyd test suite", "Expression statement", "", "") {
	const auto r = test__run_global("print(5)");
	QUARK_UT_VERIFY((r->_print_output == vector<string>{ "5" }));
}

QUARK_UNIT_TEST("Floyd test suite", "Deduced bind", "", "") {
	const auto r = test__run_global("let a = 10;print(a)");
	QUARK_UT_VERIFY((r->_print_output == vector<string>{ "10" }));
}


//////////////////////////////////////////		BASIC EXPRESSIONS - CONDITIONAL EXPRESSION


QUARK_UNIT_TEST("run_main()", "conditional expression", "", ""){
	test__run_init__check_result("let int result = true ? 1 : 2", value_t::make_int(1));
}
QUARK_UNIT_TEST("run_main()", "conditional expression", "", ""){
	test__run_init__check_result("let int result = false ? 1 : 2", value_t::make_int(2));
}

//??? Test truthness off all variable types: strings, doubles

QUARK_UNIT_TEST("run_main()", "conditional expression", "", ""){
	test__run_init__check_result("let string result = true ? \"yes\" : \"no\"", value_t::make_string("yes"));
}
QUARK_UNIT_TEST("run_main()", "conditional expression", "", ""){
	test__run_init__check_result("let string result = false ? \"yes\" : \"no\"", value_t::make_string("no"));
}


//////////////////////////////////////////		BASIC EXPRESSIONS - PARANTHESES


QUARK_UNIT_TESTQ("execute_expression()", "Parentheses") {
	test__run_init__check_result("let int result = 5*(4+4+1)", value_t::make_int(45));
}
QUARK_UNIT_TESTQ("execute_expression()", "Parentheses") {
	test__run_init__check_result("let int result = 5*(2*(1+3)+1)", value_t::make_int(45));
}
QUARK_UNIT_TESTQ("execute_expression()", "Parentheses") {
	test__run_init__check_result("let int result = 5*((1+3)*2+1)", value_t::make_int(45));
}

QUARK_UNIT_TESTQ("execute_expression()", "Sign before parentheses") {
	test__run_init__check_result("let int result = -(2+1)*4", value_t::make_int(-12));
}
QUARK_UNIT_TESTQ("execute_expression()", "Sign before parentheses") {
	test__run_init__check_result("let int result = -4*(2+1)", value_t::make_int(-12));
}


//////////////////////////////////////////		BASIC EXPRESSIONS - SPACES


QUARK_UNIT_TESTQ("execute_expression()", "Spaces") {
	test__run_init__check_result("let int result = 5 * ((1 + 3) * 2 + 1)", value_t::make_int(45));
}
QUARK_UNIT_TESTQ("execute_expression()", "Spaces") {
	test__run_init__check_result("let int result = 5 - 2 * ( 3 )", value_t::make_int(-1));
}
QUARK_UNIT_TESTQ("execute_expression()", "Spaces") {
	test__run_init__check_result("let int result =  5 - 2 * ( ( 4 )  - 1 )", value_t::make_int(-1));
}


//////////////////////////////////////////		BASIC EXPRESSIONS - double


QUARK_UNIT_TESTQ("execute_expression()", "Fractional numbers") {
	test__run_init__check_result("let double result = 5.5/5.0", value_t::make_double(1.1f));
}
QUARK_UNIT_TESTQ("execute_expression()", "Fractional numbers") {
//	test__run_init__check_result("int result = 1/5e10") == 2e-11);
}
QUARK_UNIT_TEST("execute_expression()", "Fractional numbers", "", "") {
	test__run_init__check_result("let double result = (4.0-3.0)/(4.0*4.0)", value_t::make_double(0.0625f));
}
QUARK_UNIT_TESTQ("execute_expression()", "Fractional numbers") {
	test__run_init__check_result("let double result = 1.0/2.0/2.0", value_t::make_double(0.25f));
}
QUARK_UNIT_TESTQ("execute_expression()", "Fractional numbers") {
	test__run_init__check_result("let double result = 0.25 * .5 * 0.5", value_t::make_double(0.0625f));
}
QUARK_UNIT_TESTQ("execute_expression()", "Fractional numbers") {
	test__run_init__check_result("let double result = .25 / 2.0 * .5", value_t::make_double(0.0625f));
}

//////////////////////////////////////////		BASIC EXPRESSIONS - EDGE CASES


QUARK_UNIT_TESTQ("execute_expression()", "Repeated operators") {
	test__run_init__check_result("let int result = 1+-2", value_t::make_int(-1));
}
QUARK_UNIT_TESTQ("execute_expression()", "Repeated operators") {
	test__run_init__check_result("let int result = --2", value_t::make_int(2));
}
QUARK_UNIT_TESTQ("execute_expression()", "Repeated operators") {
	test__run_init__check_result("let int result = 2---2", value_t::make_int(0));
}
QUARK_UNIT_TESTQ("execute_expression()", "Repeated operators") {
	test__run_init__check_result("let int result = 2-+-2", value_t::make_int(4));
}


//////////////////////////////////////////		BASIC EXPRESSIONS - BOOL


QUARK_UNIT_TESTQ("execute_expression()", "Bool") {
	test__run_init__check_result("let bool result = true", value_t::make_bool(true));
}
QUARK_UNIT_TESTQ("execute_expression()", "Bool") {
	test__run_init__check_result("let bool result = false", value_t::make_bool(false));
}


//////////////////////////////////////////		BASIC EXPRESSIONS - CONDITIONAL EXPRESSION


QUARK_UNIT_TESTQ("execute_expression()", "?:") {
	test__run_init__check_result("let int result = true ? 4 : 6", value_t::make_int(4));
}
QUARK_UNIT_TESTQ("execute_expression()", "?:") {
	test__run_init__check_result("let int result = false ? 4 : 6", value_t::make_int(6));
}

QUARK_UNIT_TEST("execute_expression()", "?:", "", "") {
	test__run_init__check_result("let int result = 1==3 ? 4 : 6", value_t::make_int(6));
}
QUARK_UNIT_TESTQ("execute_expression()", "?:") {
	test__run_init__check_result("let int result = 3==3 ? 4 : 6", value_t::make_int(4));
}

QUARK_UNIT_TESTQ("execute_expression()", "?:") {
	test__run_init__check_result("let int result = 3==3 ? 2 + 2 : 2 * 3", value_t::make_int(4));
}

QUARK_UNIT_TESTQ("execute_expression()", "?:") {
	test__run_init__check_result("let int result = 3==1+2 ? 2 + 2 : 2 * 3", value_t::make_int(4));
}


//////////////////////////////////////////		BASIC EXPRESSIONS - OPERATOR ==


QUARK_UNIT_TESTQ("execute_expression()", "==") {
	test__run_init__check_result("let bool result = 1 == 1", value_t::make_bool(true));
}
QUARK_UNIT_TESTQ("execute_expression()", "==") {
	test__run_init__check_result("let bool result = 1 == 2", value_t::make_bool(false));
}
QUARK_UNIT_TESTQ("execute_expression()", "==") {
	test__run_init__check_result("let bool result = 1.3 == 1.3", value_t::make_bool(true));
}
QUARK_UNIT_TEST("execute_expression()", "==", "", "") {
	test__run_init__check_result("let bool result = \"hello\" == \"hello\"", value_t::make_bool(true));
}
QUARK_UNIT_TESTQ("execute_expression()", "==") {
	test__run_init__check_result("let bool result = \"hello\" == \"bye\"", value_t::make_bool(false));
}


//////////////////////////////////////////		BASIC EXPRESSIONS - OPERATOR <


QUARK_UNIT_TEST("execute_expression()", "<", "", "") {
	test__run_init__check_result("let bool result = 1 < 2", value_t::make_bool(true));
}
QUARK_UNIT_TESTQ("execute_expression()", "<") {
	test__run_init__check_result("let bool result = 5 < 2", value_t::make_bool(false));
}
QUARK_UNIT_TESTQ("execute_expression()", "<") {
	test__run_init__check_result("let bool result = 0.3 < 0.4", value_t::make_bool(true));
}
QUARK_UNIT_TESTQ("execute_expression()", "<") {
	test__run_init__check_result("let bool result = 1.5 < 0.4", value_t::make_bool(false));
}
QUARK_UNIT_TESTQ("execute_expression()", "<") {
	test__run_init__check_result("let bool result = \"adwark\" < \"boat\"", value_t::make_bool(true));
}
QUARK_UNIT_TESTQ("execute_expression()", "<") {
	test__run_init__check_result("let bool result = \"boat\" < \"adwark\"", value_t::make_bool(false));
}

//////////////////////////////////////////		BASIC EXPRESSIONS - OPERATOR &&

QUARK_UNIT_TEST("execute_expression()", "&&", "", "") {
	test__run_init__check_result("let bool result = false && false", value_t::make_bool(false));
}
QUARK_UNIT_TESTQ("execute_expression()", "&&") {
	test__run_init__check_result("let bool result = false && true", value_t::make_bool(false));
}
QUARK_UNIT_TESTQ("execute_expression()", "&&") {
	test__run_init__check_result("let bool result = true && false", value_t::make_bool(false));
}
QUARK_UNIT_TESTQ("execute_expression()", "&&") {
	test__run_init__check_result("let bool result = true && true", value_t::make_bool(true));
}

QUARK_UNIT_TESTQ("execute_expression()", "&&") {
	test__run_init__check_result("let bool result = false && false && false", value_t::make_bool(false));
}
QUARK_UNIT_TESTQ("execute_expression()", "&&") {
	test__run_init__check_result("let bool result = false && false && true", value_t::make_bool(false));
}
QUARK_UNIT_TESTQ("execute_expression()", "&&") {
	test__run_init__check_result("let bool result = false && true && false", value_t::make_bool(false));
}
QUARK_UNIT_TESTQ("execute_expression()", "&&") {
	test__run_init__check_result("let bool result = false && true && true", value_t::make_bool(false));
}
QUARK_UNIT_TESTQ("execute_expression()", "&&") {
	test__run_init__check_result("let bool result = true && false && false", value_t::make_bool(false));
}
QUARK_UNIT_TESTQ("execute_expression()", "&&") {
	test__run_init__check_result("let bool result = true && true && false", value_t::make_bool(false));
}
QUARK_UNIT_TESTQ("execute_expression()", "&&") {
	test__run_init__check_result("let bool result = true && true && true", value_t::make_bool(true));
}

//////////////////////////////////////////		BASIC EXPRESSIONS - OPERATOR ||

QUARK_UNIT_TESTQ("execute_expression()", "||") {
	test__run_init__check_result("let bool result = false || false", value_t::make_bool(false));
}
QUARK_UNIT_TESTQ("execute_expression()", "||") {
	test__run_init__check_result("let bool result = false || true", value_t::make_bool(true));
}
QUARK_UNIT_TESTQ("execute_expression()", "||") {
	test__run_init__check_result("let bool result = true || false", value_t::make_bool(true));
}
QUARK_UNIT_TESTQ("execute_expression()", "||") {
	test__run_init__check_result("let bool result = true || true", value_t::make_bool(true));
}


QUARK_UNIT_TESTQ("execute_expression()", "||") {
	test__run_init__check_result("let bool result = false || false || false", value_t::make_bool(false));
}
QUARK_UNIT_TESTQ("execute_expression()", "||") {
	test__run_init__check_result("let bool result = false || false || true", value_t::make_bool(true));
}
QUARK_UNIT_TESTQ("execute_expression()", "||") {
	test__run_init__check_result("let bool result = false || true || false", value_t::make_bool(true));
}
QUARK_UNIT_TESTQ("execute_expression()", "||") {
	test__run_init__check_result("let bool result = false || true || true", value_t::make_bool(true));
}
QUARK_UNIT_TESTQ("execute_expression()", "||") {
	test__run_init__check_result("let bool result = true || false || false", value_t::make_bool(true));
}
QUARK_UNIT_TESTQ("execute_expression()", "||") {
	test__run_init__check_result("let bool result = true || false || true", value_t::make_bool(true));
}
QUARK_UNIT_TESTQ("execute_expression()", "||") {
	test__run_init__check_result("let bool result = true || true || false", value_t::make_bool(true));
}
QUARK_UNIT_TESTQ("execute_expression()", "||") {
	test__run_init__check_result("let bool result = true || true || true", value_t::make_bool(true));
}


//////////////////////////////////////////		BASIC EXPRESSIONS - ERRORS


QUARK_UNIT_TEST("execute_expression()", "Type mismatch", "", "") {
	try{
		test__run_init__check_result("let int result = true", value_t::make_int(1));
		QUARK_TEST_VERIFY(false);
	}
	catch(const std::runtime_error& e){
		QUARK_TEST_VERIFY(string(e.what()) == "Expression type mismatch. Cannot convert 'bool' to 'int.");
	}
}


QUARK_UNIT_TESTQ("execute_expression()", "Division by zero") {
	try{
		test__run_init__check_result("let int result = 2/0", value_t::make_undefined());
		QUARK_TEST_VERIFY(false);
	}
	catch(const std::runtime_error& e){
		QUARK_TEST_VERIFY(string(e.what()) == "EEE_DIVIDE_BY_ZERO");
	}
}

QUARK_UNIT_TESTQ("execute_expression()", "Division by zero"){
	try{
		test__run_init__check_result("let int result = 3+1/(5-5)+4", value_t::make_undefined());
		QUARK_TEST_VERIFY(false);
	}
	catch(const std::runtime_error& e){
		QUARK_TEST_VERIFY(string(e.what()) == "EEE_DIVIDE_BY_ZERO");
	}
}

#if false

QUARK_UNIT_TESTQ("execute_expression()", "Errors") {
		//	Multiple errors not possible/relevant now that we use exceptions for errors.
/*
	//////////////////////////		Only one error will be detected (in this case, the last one)
	QUARK_TEST_VERIFY(test__execute_expression("3+1/0+4$") == EEE_WRONG_CHAR);

	QUARK_TEST_VERIFY(test__execute_expression("3+1/0+4") == EEE_DIVIDE_BY_ZERO);

	// ...or the first one
	QUARK_TEST_VERIFY(test__execute_expression("q+1/0)") == EEE_WRONG_CHAR);
	QUARK_TEST_VERIFY(test__execute_expression("+1/0)") == EEE_PARENTHESES);
	QUARK_TEST_VERIFY(test__execute_expression("+1/0") == EEE_DIVIDE_BY_ZERO);
*/
}

#endif


QUARK_UNIT_TEST("execute_expression()", "-true", "", "") {
	try{
		test__run_init__check_result("let int result = -true", value_t::make_int(0));
		QUARK_TEST_VERIFY(false);
	}
	catch(const std::runtime_error& e){
		QUARK_TEST_VERIFY(string(e.what()) == "Unary minus won't work on expressions of type \"\"bool\"\".");
	}
}


//////////////////////////////////////////		MINIMAL PROGRAMS


QUARK_UNIT_TEST("run_main", "Forgot let or mutable", "", "Exception"){
	try{
		test__run_init__check_result("int test = 123", value_t::make_int(0));
		QUARK_TEST_VERIFY(false);
	}
	catch(const std::runtime_error& e){
		QUARK_TEST_VERIFY(string(e.what()) == "Use 'mutable' or 'let' syntax.");
	}
}

QUARK_UNIT_TEST("run_main", "Can make and read global int", "", ""){
	test__run_main(
		R"(
			let int test = 123
			func int main(){
				return test
			}
		)",
		{},
		value_t::make_int(123)
	);
}

QUARK_UNIT_TEST("run_main()", "minimal program 2", "", ""){
	test__run_main(
		R"(
			func string main(string args){
				return "123" + "456"
			}
		)",
		vector<value_t>{value_t::make_string("program_name 1 2 3 4")},
		value_t::make_string("123456")
	);
}

QUARK_UNIT_TESTQ("run_main()", ""){
	test__run_main("func bool main(){ return 4 < 5 }", {}, value_t::make_bool(true));
}
QUARK_UNIT_TESTQ("run_main()", ""){
	test__run_main("func bool main(){ return 5 < 4 }", {}, value_t::make_bool(false));
}
QUARK_UNIT_TESTQ("run_main()", ""){
	test__run_main("func bool main(){ return 4 <= 4 }", {}, value_t::make_bool(true));
}

QUARK_UNIT_TESTQ("call_function()", "minimal program"){
	auto ast = compile_to_bytecode(
		R"(
			func int main(string args){
				return 3 + 4
			}
		)"
	);
	interpreter_t vm(ast);
	const auto f = find_global_symbol(vm, "main");
	const auto result = call_function(vm, f, vector<value_t>{ value_t::make_string("program_name 1 2 3") });
	QUARK_TEST_VERIFY(result == value_t::make_int(7));
}

QUARK_UNIT_TESTQ("call_function()", "minimal program 2"){
	auto ast = compile_to_bytecode(R"(

		func string main(string args){
			return "123" + "456"
		}

	)");
	interpreter_t vm(ast);
	const auto f = find_global_symbol(vm, "main");
	const auto result = call_function(vm, f, vector<value_t>{ value_t::make_string("program_name 1 2 3") });
	QUARK_TEST_VERIFY(result == value_t::make_string("123456"));
}


//////////////////////////////////////////		TEST CONSTRUCTOR FOR ALL TYPES


QUARK_UNIT_TEST("", "bool()", "", ""){
	test__run_init__check_result("let result = bool(false)", value_t::make_bool(false));
}
QUARK_UNIT_TEST("", "bool()", "", ""){
	test__run_init__check_result("let result = bool(true)", value_t::make_bool(true));
}
QUARK_UNIT_TEST("", "int()", "", ""){
	test__run_init__check_result("let result = int(123)", value_t::make_int(123));
}
QUARK_UNIT_TEST("", "double()", "", ""){
	test__run_init__check_result("let result = double(0.0)", value_t::make_double(0.0));
}
QUARK_UNIT_TEST("", "double()", "", ""){
	test__run_init__check_result("let result = double(123.456)", value_t::make_double(123.456f));
}

QUARK_UNIT_TEST("", "string()", "", ""){
	try{
		test__run_init__check_result("let result = string()", value_t::make_string(""));
		QUARK_UT_VERIFY(false);
	}
	catch(const std::runtime_error& e){
		QUARK_TEST_VERIFY(string(e.what()) == "Wrong number of arguments in function call.");
	}
}

QUARK_UNIT_TEST("", "string()", "", ""){
	test__run_init__check_result("let result = string(\"ABCD\")", value_t::make_string("ABCD"));
}

QUARK_UNIT_TEST("", "json_value()", "", ""){
	test__run_init__check_result("let result = json_value(123)", value_t::make_json_value(json_t(123.0)));
}
QUARK_UNIT_TEST("", "json_value()", "", ""){
	test__run_init__check_result("let result = json_value(\"hello\")", value_t::make_json_value(json_t("hello")));
}
QUARK_UNIT_TEST("", "json_value()", "", ""){
	test__run_init__check_result("let result = json_value([1,2,3])", value_t::make_json_value(json_t::make_array({1,2,3})));
}
QUARK_UNIT_TEST("", "pixel_t()", "", ""){
	const auto pixel_t__def = std::vector<member_t>{
		member_t(typeid_t::make_int(), "red"),
		member_t(typeid_t::make_int(), "green"),
		member_t(typeid_t::make_int(), "blue")
	};

	test__run_init__check_result(

		"struct pixel_t { int red int green int blue } result = pixel_t(1,2,3)",

		value_t::make_struct_value(
			typeid_t::make_struct2(pixel_t__def),
			vector<value_t>{ value_t::make_int(1), value_t::make_int(2), value_t::make_int(3) }
		)
	);
}

//??? typeid()

/*
unsupported syntax
QUARK_UNIT_TEST("", "[int]()", "", ""){
	test__run_init__check_result(
		"result = [int](1,2,3);",
		value_t::make_vector_value(typeid_t::make_int(), {
			value_t::make_int(1),
			value_t::make_int(2),
			value_t::make_int(3)
		})
	);
}

unsupported syntax
QUARK_UNIT_TEST("", "[[int]]()", "", ""){
	test__run_init__check_result(
		"result = [[int]]([1,2,3], [4,5,6]);",
		value_t::make_vector_value(
			typeid_t::make_vector(typeid_t::make_int()),
			{
				value_t::make_vector_value(typeid_t::make_int(), {
					value_t::make_int(1),
					value_t::make_int(2),
					value_t::make_int(3)
				}),
				value_t::make_vector_value(typeid_t::make_int(), {
					value_t::make_int(4),
					value_t::make_int(5),
					value_t::make_int(6)
				})
			}
		)
	);
}

unsupported syntax
QUARK_UNIT_TEST("", "[pixel_t]()", "", ""){
	const auto pixel_t__def = std::make_shared<floyd::struct_definition_t>(
		std::vector<member_t>{
			member_t(typeid_t::make_int(), "red"),
			member_t(typeid_t::make_int(), "green"),
			member_t(typeid_t::make_int(), "blue")
		}
	);
	const auto pixel_t_typeid = typeid_t::make_struct(pixel_t__def);

	test__run_init__check_result(
		"struct pixel_t { int red; int green; int blue; } result = [pixel_t](pixel_t(1,2,3),pixel_t(4,5,6));",
		value_t::make_vector_value(
			pixel_t_typeid,
			{
				value_t::make_struct_value(pixel_t_typeid, vector<value_t>{value_t::make_int(1), value_t::make_int(2), value_t::make_int(3)}),
				value_t::make_struct_value(pixel_t_typeid, vector<value_t>{value_t::make_int(4), value_t::make_int(5), value_t::make_int(6)})
			}
		)
	);
}
*/

/*
QUARK_UNIT_TEST("", "[[string: int]]()", "", ""){
	test__run_init__check_result(
		R"(result = [{string: int}]({"a":1,"b":2,"c":3}, {"d":4,"e":5,"f":6});)",
		value_t::make_vector_value(
			typeid_t::make_vector(typeid_t::make_int()),
			{
				value_t::make_vector_value(typeid_t::make_int(), {
					value_t::make_int(1),
					value_t::make_int(2),
					value_t::make_int(3)
				}),
				value_t::make_vector_value(typeid_t::make_int(), {
					value_t::make_int(4),
					value_t::make_int(5),
					value_t::make_int(6)
				})
			}
		)
	);
}
*/


//////////////////////////////////////////		CALL FUNCTIONS


QUARK_UNIT_TEST("call_function()", "define additional function, call it several times", "", ""){
	auto ast = compile_to_bytecode(R"(

		func int myfunc(){ return 5 }
		func int main(string args){
			return myfunc() + myfunc() * 2
		}

	)");
	interpreter_t vm(ast);
	const auto f = find_global_symbol(vm, "main");
	const auto result = call_function(vm, f, vector<value_t>{ value_t::make_string("program_name 1 2 3") });
	QUARK_TEST_VERIFY(result == value_t::make_int(15));
}

QUARK_UNIT_TEST("call_function()", "use function inputs", "", ""){
	auto ast = compile_to_bytecode(R"(

		func string main(string args){
			return "-" + args + "-"
		}

	)");
	interpreter_t vm(ast);
	const auto f = find_global_symbol(vm, "main");
	const auto result = call_function(vm, f, vector<value_t>{ value_t::make_string("xyz") });
	QUARK_TEST_VERIFY(result == value_t::make_string("-xyz-"));

	const auto result2 = call_function(vm, f, vector<value_t>{ value_t::make_string("Hello, world!") });
	QUARK_TEST_VERIFY(result2 == value_t::make_string("-Hello, world!-"));
}


//////////////////////////////////////////		USE LOCAL VARIABLES IN FUNCTION


QUARK_UNIT_TEST("call_function()", "use local variables", "", ""){
	auto ast = compile_to_bytecode(R"(

		func string myfunc(string t){ return "<" + t + ">" }
		func string main(string args){
			 let string a = "--"
			 let string b = myfunc(args)
			 return a + args + b + a
		}

	)");
	interpreter_t vm(ast);
	const auto f = find_global_symbol(vm, "main");
	const auto result = call_function(vm, f, vector<value_t>{ value_t::make_string("xyz") });

	QUARK_TEST_VERIFY(result == value_t::make_string("--xyz<xyz>--"));

	const auto result2 = call_function(vm, f, vector<value_t>{ value_t::make_string("123") });

	QUARK_TEST_VERIFY(result2 == value_t::make_string("--123<123>--"));
}


//////////////////////////////////////////		MUTATE VARIABLES


QUARK_UNIT_TEST("call_function()", "local variable without let", "", ""){
	auto r = test__run_global(R"(

		a = 7
		print(a)

	)");
	QUARK_UT_VERIFY((r->_print_output == vector<string>{ "7" }));
}



//////////////////////////////////////////		MUTATE VARIABLES


QUARK_UNIT_TEST("call_function()", "mutate local", "", ""){
	auto r = test__run_global(R"(

		mutable a = 1
		a = 2
		print(a)

	)");
	QUARK_UT_VERIFY((r->_print_output == vector<string>{ "2" }));
}

QUARK_UNIT_TESTQ("run_init()", "increment a mutable"){
	const auto r = test__run_global(R"(

		mutable a = 1000
		a = a + 1
		print(a)

	)");
	QUARK_UT_VERIFY((r->_print_output == vector<string>{ "1001" }));
}

QUARK_UNIT_TESTQ("run_main()", "test locals are immutable"){
	try {
		const auto vm = test__run_global(R"(

			let a = 3
			a = 4

		)");
		QUARK_UT_VERIFY(false);
	}
	catch(const std::runtime_error& e){
		QUARK_UT_VERIFY(string(e.what()) == "Cannot assign to immutable identifier.");
	}
}

QUARK_UNIT_TESTQ("run_main()", "test function args are always immutable"){
	try {
		const auto vm = test__run_global(R"(

			func int f(int x){
				x = 6
			}
			f(5)

		)");
		QUARK_UT_VERIFY(false);
	}
	catch(const std::runtime_error& e){
		QUARK_UT_VERIFY(string(e.what()) == "Cannot assign to immutable identifier.");
	}
}

QUARK_UNIT_TEST("run_main()", "test mutating from a subscope", "", ""){
	const auto r = test__run_global(R"(

		mutable a = 7
		a = 8
		{
			print(a)
		}

	)");
	QUARK_UT_VERIFY((r->_print_output == vector<string>{ "8" }));
}

QUARK_UNIT_TEST("run_main()", "test mutating from a subscope", "", ""){
	const auto r = test__run_global(R"(

		let a = 7
		print(a)

	)");
	QUARK_UT_VERIFY((r->_print_output == vector<string>{ "7" }));
}


QUARK_UNIT_TEST("run_main()", "test mutating from a subscope", "", ""){
	const auto r = test__run_global(R"(

		let a = 7
		{
		}
		print(a)

	)");
	QUARK_UT_VERIFY((r->_print_output == vector<string>{ "7" }));
}

QUARK_UNIT_TEST("run_main()", "test mutating from a subscope", "", ""){
	const auto r = test__run_global(R"(

		mutable a = 7
		{
			a = 8
		}
		print(a)

	)");
	QUARK_UT_VERIFY((r->_print_output == vector<string>{ "8" }));
}


//////////////////////////////////////////		RETURN STATEMENT - ADVANCED USAGE



QUARK_UNIT_TEST("call_function()", "return from middle of function", "", ""){
	auto r = test__run_global(R"(

		func string f(){
			print("A")
			return "B"
			print("C")
			return "D"
		}
		let string x = f()
		print(x)

	)");
	QUARK_UT_VERIFY((r->_print_output == vector<string>{ "A", "B" }));
}

QUARK_UNIT_TEST("call_function()", "return from within IF block", "", ""){
	auto r = test__run_global(R"(

		func string f(){
			if(true){
				print("A")
				return "B"
				print("C")
			}
			print("D")
			return "E"
		}
		let string x = f()
		print(x)

	)");
	QUARK_UT_VERIFY((r->_print_output == vector<string>{ "A", "B" }));
}

QUARK_UNIT_TEST("call_function()", "return from within FOR block", "", ""){
	auto r = test__run_global(R"(

		func string f(){
			for(e in 0...3){
				print("A")
				return "B"
				print("C")
			}
			print("D")
			return "E"
		}
		let string x = f()
		print(x)

	)");
	QUARK_UT_VERIFY((r->_print_output == vector<string>{ "A", "B" }));
}

// ??? add test for: return from ELSE

QUARK_UNIT_TESTQ("call_function()", "return from within BLOCK"){
	auto r = test__run_global(R"(

		func string f(){
			{
				print("A")
				return "B"
				print("C")
			}
			print("D")
			return "E"
		}
		let string x = f()
		print(x)

	)");
	QUARK_UT_VERIFY((r->_print_output == vector<string>{ "A", "B" }));
}


QUARK_UNIT_TEST("", "Make sure returning wrong type => error", "", ""){
	try {
	run_container2(R"(

func int f(){
	return "x"
}

	)", {}, "");


	}
	catch(const std::runtime_error& e){
		QUARK_UT_VERIFY(string(e.what()) == "Expression type mismatch. Cannot convert 'string' to 'int.");
	}
}




//////////////////////////////////////////		HOST FUNCTION - to_string()


QUARK_UNIT_TESTQ("run_init()", ""){
	test__run_init__check_result(
		R"(
			let string result = to_string(145)
		)",
		value_t::make_string("145")
	);
}
QUARK_UNIT_TESTQ("run_init()", ""){
	test__run_init__check_result(
		R"(
			let string result = to_string(3.1)
		)",
		value_t::make_string("3.1")
	);
}

QUARK_UNIT_TESTQ("run_init()", ""){
	test__run_init__check_result(
		R"(
			let string result = to_string(3.0)
		)",
		value_t::make_string("3.0")
	);
}


//////////////////////////////////////////		HOST FUNCTION - typeof()


QUARK_UNIT_TEST("", "typeof()", "", ""){
	const auto result = test__run_return_result(
		R"(
			let result = typeof(145)
		)", {}
	);
	ut_compare_values(result, value_t::make_typeid_value(typeid_t::make_int()));
}
QUARK_UNIT_TEST("", "typeof()", "", ""){
	const auto result = test__run_return_result(
		R"(
			let result = to_string(typeof(145))
		)", {}
	);
	ut_compare_values(result, value_t::make_string("int"));
}

QUARK_UNIT_TEST("", "typeof()", "", ""){
	const auto result = test__run_return_result(
		R"(
			let result = typeof("hello")
		)", {}
	);
	ut_compare_values(result, value_t::make_typeid_value(typeid_t::make_string()));
}

QUARK_UNIT_TEST("", "typeof()", "", ""){
	const auto result = test__run_return_result(
		R"(
			let result = to_string(typeof("hello"))
		)", {}
	);
	ut_compare_values(result, value_t::make_string("string"));
}

QUARK_UNIT_TEST("", "typeof()", "", ""){
	const auto result = test__run_return_result(
		R"(
			let result = typeof([1,2,3])
		)", {}
	);
	ut_compare_values(result, value_t::make_typeid_value(typeid_t::make_vector(typeid_t::make_int())));
}
QUARK_UNIT_TEST("", "typeof()", "", ""){
	const auto result = test__run_return_result(
		R"(
			let result = to_string(typeof([1,2,3]))
		)", {}
	);
	ut_compare_values(result, value_t::make_string("[int]"));
}

/*
//??? add support for typeof(int)
QUARK_UNIT_TEST("", "typeof()", "", ""){
	const auto result = test__run_return_result(
		R"(
			result = typeof(int);
		)", {}
	);
	ut_compare_values(result, value_t::make_string("int"));
}
*/



//////////////////////////////////////////		HOST FUNCTION - assert()

QUARK_UNIT_TESTQ("run_global()", ""){
	try{
		const auto r = test__run_global(
			R"(
				assert(1 == 2)
			)"
		);
		QUARK_UT_VERIFY(false);
	}
	catch(...){
//		QUARK_UT_VERIFY((r->_print_output == vector<string>{ "Assertion failed.", "A" }));
	}
}

QUARK_UNIT_TESTQ("run_global()", ""){
	const auto r = test__run_global(
		R"(
			assert(1 == 1)
			print("A")
		)"
	);
	QUARK_UT_VERIFY((r->_print_output == vector<string>{ "A" }));
}




//////////////////////////////////////////		BLOCKS AND SCOPING


QUARK_UNIT_TEST("run_init()", "Empty block", "", ""){
	test__run_global(
		"{}"
	);
}

QUARK_UNIT_TESTQ("run_init()", "Block with local variable, no shadowing"){
	test__run_global(
		"{ let int x = 4 }"
	);
}

QUARK_UNIT_TEST("run_init()", "Block with local variable, no shadowing", "", ""){
	const auto r = test__run_global(
		R"(
			let int x = 3
			print("B:" + to_string(x))
			{
				print("C:" + to_string(x))
				let int x = 4
				print("D:" + to_string(x))
			}
			print("E:" + to_string(x))
		)"
	);
	QUARK_UT_VERIFY((r->_print_output == vector<string>{ "B:3", "C:3", "D:4", "E:3" }));
}


//////////////////////////////////////////		IF STATEMENT


QUARK_UNIT_TEST("run_init()", "if(true){}", "", ""){
	const auto r = test__run_global(
		R"(
			if(true){
				print("Hello!")
			}
			print("Goodbye!")
		)"
	);
	QUARK_UT_VERIFY((r->_print_output == vector<string>{ "Hello!", "Goodbye!" }));
}

QUARK_UNIT_TESTQ("run_init()", "if(false){}"){
	const auto r = test__run_global(
		R"(
			if(false){
				print("Hello!")
			}
			print("Goodbye!")
		)"
	);
	QUARK_UT_VERIFY((r->_print_output == vector<string>{ "Goodbye!" }));
}

QUARK_UNIT_TESTQ("run_init()", "if(true){}else{}"){
	const auto r = test__run_global(
		R"(
			if(true){
				print("Hello!")
			}
			else{
				print("Goodbye!")
			}
		)"
	);
	QUARK_UT_VERIFY((r->_print_output == vector<string>{ "Hello!" }));
}

QUARK_UNIT_TESTQ("run_init()", "if(false){}else{}"){
	const auto r = test__run_global(
		R"(
			if(false){
				print("Hello!")
			}
			else{
				print("Goodbye!")
			}
		)"
	);
	QUARK_UT_VERIFY((r->_print_output == vector<string>{ "Goodbye!" }));
}

QUARK_UNIT_TESTQ("run_init()", "if"){
	const auto r = test__run_global(
		R"(
			if(1 == 1){
				print("one")
			}
			else if(2 == 0){
				print("two")
			}
			else if(3 == 0){
				print("three")
			}
			else{
				print("four")
			}
		)"
	);
	QUARK_UT_VERIFY((r->_print_output == vector<string>{ "one" }));
}

QUARK_UNIT_TESTQ("run_init()", "if"){
	const auto r = test__run_global(
		R"(
			if(1 == 0){
				print("one")
			}
			else if(2 == 2){
				print("two")
			}
			else if(3 == 0){
				print("three")
			}
			else{
				print("four")
			}
		)"
	);
	QUARK_UT_VERIFY((r->_print_output == vector<string>{ "two" }));
}

QUARK_UNIT_TESTQ("run_init()", "if"){
	const auto r = test__run_global(
		R"(
			if(1 == 0){
				print("one")
			}
			else if(2 == 0){
				print("two")
			}
			else if(3 == 3){
				print("three")
			}
			else{
				print("four")
			}
		)"
	);
	QUARK_UT_VERIFY((r->_print_output == vector<string>{ "three" }));
}

QUARK_UNIT_TESTQ("run_init()", "if"){
	const auto r = test__run_global(
		R"(
			if(1 == 0){
				print("one")
			}
			else if(2 == 0){
				print("two")
			}
			else if(3 == 0){
				print("three")
			}
			else{
				print("four")
			}
		)"
	);
	QUARK_UT_VERIFY((r->_print_output == vector<string>{ "four" }));
}


QUARK_UNIT_TEST("", "function calling itself by name", "", ""){
	const auto vm = test__run_global(
		R"(
			func int fx(int a){
				return fx(a + 1)
			}
		)"
	);
}


QUARK_UNIT_TEST("run_init()", "Make sure a function can access global independent on how it's called in callstack", "", ""){
	const auto vm = test__run_global(
		R"(
			let int x = 13

			func int add(bool t){
				if(t == false){
					return x + 1
				}
				else{
					return add(false) + 1000
				}
			}

			print(x)
			print(add(false))
			print(add(true))
		)"
	);
	QUARK_UT_VERIFY((
		vm->_print_output == vector<string>{
			"13", "14", "1014"
		})
	);
}


//////////////////////////////////////////		FOR STATEMENT


QUARK_UNIT_TEST("run_init()", "for", "", ""){
	const auto r = test__run_global(
		R"(
			for (i in 0...2) {
				print("xyz")
			}
		)"
	);
	QUARK_UT_VERIFY((r->_print_output == vector<string>{ "xyz", "xyz", "xyz" }));
}
QUARK_UNIT_TEST("run_init()", "for", "", ""){
	const auto r = test__run_global(
		R"(
			for (i in 0...2) {
				to_string(i)
			}
		)"
	);
//	QUARK_UT_VERIFY((r->_print_output == vector<string>{ "Iteration: 0", "Iteration: 1", "Iteration: 2" }));
}


QUARK_UNIT_TEST("run_init()", "for", "", ""){
	const auto r = test__run_global(
		R"(
			for (i in 0...2) {
				print("Iteration: " + to_string(i))
			}
		)"
	);
	QUARK_UT_VERIFY((r->_print_output == vector<string>{ "Iteration: 0", "Iteration: 1", "Iteration: 2" }));
}
QUARK_UNIT_TEST("run_init()", "for", "", ""){
	const auto r = test__run_global(
		R"(
			for (i in 0..<2) {
				print("Iteration: " + to_string(i))
			}
		)"
	);
	QUARK_UT_VERIFY((r->_print_output == vector<string>{ "Iteration: 0", "Iteration: 1" }));
}

QUARK_UNIT_TEST("run_init()", "fibonacci", "", ""){
	const auto vm = test__run_global(R"(
		func int fibonacci(int n) {
			if (n <= 1){
				return n
			}
			return fibonacci(n - 2) + fibonacci(n - 1)
		}

		for (i in 0...10) {
			print(fibonacci(i))
		}
	)");

	QUARK_UT_VERIFY((
		vm->_print_output == vector<string>{
			"0", "1", "1", "2", "3", "5", "8", "13", "21", "34",
			"55" //, "89", "144", "233", "377", "610", "987", "1597", "2584", "4181"
		})
	);
}


//////////////////////////////////////////		WHILE STATEMENT


QUARK_UNIT_TEST("run_init()", "for", "", ""){
	const auto r = test__run_global(R"(
		mutable a = 100
		while(a < 105){
			print(to_string(a))
			a = a + 1
		}
	)");
	QUARK_UT_VERIFY((r->_print_output == vector<string>{ "100", "101", "102", "103", "104" }));
}


//////////////////////////////////////////		TYPEID - TYPE

//	??? Test converting different types to jsons


//////////////////////////////////////////		NULL - TYPE


QUARK_UNIT_TEST("null", "", "", "0"){
//	try{
		const auto result = test__run_return_result(R"(
			let result = null
		)", {});
/*
		QUARK_UT_VERIFY(false);

 }
	catch(const std::runtime_error& e){
		QUARK_UT_VERIFY(string(e.what()) == "Undefined variable \"null\".");
	}
*/

}


//////////////////////////////////////////		STRING - TYPE


//??? add tests for equality.

QUARK_UNIT_TEST("string", "[]", "string", "0"){
	const auto vm = test__run_global(R"(
		assert("hello"[0] == 104)
	)");
}
QUARK_UNIT_TEST("string", "[]", "string", "0"){
	const auto vm = test__run_global(R"(
		assert("hello"[4] == 111)
	)");
}

QUARK_UNIT_TEST("string", "size()", "string", "0"){
	const auto vm = test__run_global(R"(
		assert(size("") == 0)
	)");
}
QUARK_UNIT_TEST("string", "size()", "string", "24"){
	const auto vm = test__run_global(R"(
		assert(size("How long is this string?") == 24)
	)");
}

QUARK_UNIT_TEST("string", "push_back()", "string", "correct final vector"){
	const auto vm = test__run_global(R"(
		a = push_back("one", 111)
		assert(a == "oneo")
	)");
}

QUARK_UNIT_TEST("string", "update()", "string", "correct final string"){
	const auto vm = test__run_global(R"(
		a = update("hello", 1, 98)
		assert(a == "hbllo")
	)");
}


QUARK_UNIT_TEST("string", "subset()", "string", "correct final vector"){
	const auto vm = test__run_global(R"(
		assert(subset("abc", 0, 3) == "abc")
		assert(subset("abc", 1, 3) == "bc")
		assert(subset("abc", 0, 0) == "")
	)");
}

QUARK_UNIT_TEST("vector", "replace()", "combo", ""){
	const auto vm = test__run_global(R"(
		assert(replace("One ring to rule them all", 4, 8, "rabbit") == "One rabbit to rule them all")
	)");
}
// ### test pos limiting and edge cases.




//////////////////////////////////////////		VECTOR - TYPE




QUARK_UNIT_TEST("vector", "[]-constructor", "deduce type", "valid vector"){
	const auto vm = test__run_global(R"(
		let a = ["one", "two"]
		print(a)
	)");
	ut_compare_stringvects(vm->_print_output, vector<string>{
		R"(["one", "two"])"
	});
}

QUARK_UNIT_TEST("vector", "[]-constructor", "cannot be deducted", "error"){
	try{
		const auto vm = test__run_global(R"(
			let a = []
			print(a)
		)");
		QUARK_UT_VERIFY(false);
	}
	catch(const std::runtime_error& e){
		QUARK_UT_VERIFY(string(e.what()) == "Cannot resolve type.");
	}
}
/*
???
		[string] a = ["one", "two"]

		Parsed as:
			[string]		//	Expression statement that creates [typeid] = [string]
			a = ["one", "two"]	//	Make vector with type deduced from "one".
*/

QUARK_UNIT_TEST("vector", "explit bind, is []", "deduce type", "valid vector"){
	const auto vm = test__run_global(R"(
		let [string] a = ["one", "two"]
		print(a)
	)");
	ut_compare_stringvects(vm->_print_output, vector<string>{
		R"(["one", "two"])"
	});
}

QUARK_UNIT_TEST("vector", "", "empty vector", "valid vector"){
	const auto vm = test__run_global(R"(

		let [string] a = []
		print(a)

	)");
	ut_compare_stringvects(vm->_print_output, vector<string>{
		R"([])"
	});
}

//	We could support this if we had special type for empty-vector and empty-dict.
QUARK_UNIT_TEST("vector", "==", "lhs and rhs are empty-typeless", ""){
	try{
		const auto vm = test__run_global(R"(

			assert(([] == []) == true)

		)");
		QUARK_UT_VERIFY(false);
	}
	catch(const std::runtime_error& e){
		QUARK_TEST_VERIFY(string(e.what()) == "Cannot resolve type.");
	}
}

QUARK_UNIT_TEST("vector", "+", "add empty vectors", ""){
	try{
		test_result(R"(

			let [int] a = [] + [] result = a == []

		)", R"(	)");
		QUARK_UT_VERIFY(false);
	}
	catch(const std::runtime_error& e){
		QUARK_TEST_VERIFY(string(e.what()) == "Cannot resolve type.");
	}
}

//??? This fails but should not. This code becomes a constructor call to [int] with more than 16 arguments. Byte code interpreter has 16 argument limit.
OFF_QUARK_UNIT_TEST("vector", "[]-constructor", "32 elements initialization", ""){
	const auto vm = test__run_global(R"(

		let a = [ 0,0,1,1,0,0,0,0,	0,0,1,1,0,0,0,0,	0,0,1,1,1,0,0,0,	0,0,1,1,1,1,0,0 ]
		print(a)

	)");
}




//////////////////////////////////////////		vector-string


QUARK_UNIT_TEST("vector-string", "literal expression", "", ""){
	test_result(R"(

		let [string] result = ["alpha", "beta"]

	)", R"(		[[ "vector", "^string" ], ["alpha","beta"]]		)");
}
QUARK_UNIT_TEST("vector-string", "literal expression, computed element", "", ""){
	test_result(R"(		func string get_beta(){ return "beta" } 	let [string] result = ["alpha", get_beta()]		)", R"(		[[ "vector", "^string" ], ["alpha","beta"]]		)");
}
QUARK_UNIT_TEST("vector-string", "=", "copy", ""){
	const auto vm = test__run_global(R"(

		let a = ["one", "two"]
		let b = a
		assert(a == b)
		print(a)
		print(b)

	)");
	ut_compare_stringvects(vm->_print_output, vector<string>{
		R"(["one", "two"])",
		R"(["one", "two"])"
	});
}
QUARK_UNIT_TEST("vector-string", "==", "same values", ""){
	const auto vm = test__run_global(R"(

		assert((["one", "two"] == ["one", "two"]) == true)

	)");
}

QUARK_UNIT_TEST("vector-string", "==", "different values", ""){
	const auto vm = test__run_global(R"(

		assert((["one", "three"] == ["one", "two"]) == false)

	)");
}

QUARK_UNIT_TEST("vector-string", "<", "same values", ""){
	const auto vm = test__run_global(R"(

		assert((["one", "two"] < ["one", "two"]) == false)

	)");
}

QUARK_UNIT_TEST("vector-string", "<", "different values", ""){
	const auto vm = test__run_global(R"(

		assert((["one", "a"] < ["one", "two"]) == true)

	)");
}
QUARK_UNIT_TEST("vector-string", "size()", "empty", "0"){
	const auto vm = test__run_global(R"(

		let [string] a = []
		assert(size(a) == 0)

	)");
}
QUARK_UNIT_TEST("vector-string", "size()", "2", ""){
	const auto vm = test__run_global(R"(

		let [string] a = ["one", "two"]
		assert(size(a) == 2)

	)");
}
QUARK_UNIT_TEST("vector-string", "+", "non-empty vectors", ""){
	const auto vm = test__run_global(R"(

		let [string] a = ["one"] + ["two"]
		assert(a == ["one", "two"])

	)");
}
QUARK_UNIT_TEST("vector-string", "push_back()", "", ""){
	const auto vm = test__run_global(R"(

		let [string] a = push_back(["one"], "two")
		assert(a == ["one", "two"])

	)");
}


//////////////////////////////////////////		vector-bool


QUARK_UNIT_TEST("vector-bool", "literal expression", "", ""){
	test_result(R"(		let [bool] result = [true, false, true]		)", R"(		[[ "vector", "^bool" ], [true, false, true]]		)");
}
QUARK_UNIT_TEST("vector-bool", "=", "copy", ""){
	test_result(R"(		let a = [true, false, true] result = a		)", R"(		[[ "vector", "^bool" ], [true, false, true]]		)");
}
QUARK_UNIT_TEST("vector-bool", "==", "same values", ""){
	test_result(R"(		let result = [true, false] == [true, false]		)", R"(		[ "^bool", true]		)");
}
QUARK_UNIT_TEST("vector-bool", "==", "different values", ""){
	test_result(R"(		let result = [false, false] == [true, false]		)", R"(		[ "^bool", false]		)");
}
QUARK_UNIT_TEST("vector-bool", "<", "", ""){
	test_result(R"(		let result = [true, true] < [true, true]		)", R"(		[ "^bool", false]	)");
}
QUARK_UNIT_TEST("vector-bool", "<", "different values", ""){
	test_result(R"(		let result = [true, false] < [true, true]		)", R"(		[ "^bool", true]		)");
}
QUARK_UNIT_TEST("vector-bool", "size()", "empty", "0"){
	test_result(R"(		let [bool] a = [] result = size(a)		)", R"(		[ "^int", 0]		)");
}
QUARK_UNIT_TEST("vector-bool", "size()", "2", ""){
	test_result(R"(		let [bool] a = [true, false, true] result = size(a)		)", R"(		[ "^int", 3]		)");
}
QUARK_UNIT_TEST("vector-bool", "+", "non-empty vectors", ""){
	test_result(R"(		let [bool] result = [true, false] + [true, true]		)", R"(		[[ "vector", "^bool" ], [true, false, true, true]]		)");
}
QUARK_UNIT_TEST("vector-bool", "push_back()", "", ""){
	test_result(R"(		let [bool] result = push_back([true, false], true)		)", R"(		[[ "vector", "^bool" ], [true, false, true]]		)");
}


//////////////////////////////////////////		vector-int


QUARK_UNIT_TEST("vector-int", "literal expression", "", ""){
	test_result(R"(		let [int] result = [10, 20, 30]		)", R"(		[[ "vector", "^int" ], [10, 20, 30]]		)");
}
QUARK_UNIT_TEST("vector-int", "=", "copy", ""){
	test_result(R"(		let a = [10, 20, 30] result = a;		)", R"(		[[ "vector", "^int" ], [10, 20, 30]]		)");
}
QUARK_UNIT_TEST("vector-int", "==", "same values", ""){
	test_result(R"(		let result = [1, 2] == [1, 2]		)", R"(		[ "^bool", true]		)");
}
QUARK_UNIT_TEST("vector-int", "==", "different values", ""){
	test_result(R"(		let result = [1, 3] == [1, 2]		)", R"(		[ "^bool", false]		)");
}
QUARK_UNIT_TEST("vector-int", "<", "", ""){
	test_result(R"(		let result = [1, 2] < [1, 2]		)", R"(		[ "^bool", false]	)");
}
QUARK_UNIT_TEST("vector-int", "<", "different values", ""){
	test_result(R"(		let result = [1, 2] < [1, 3]		)", R"(		[ "^bool", true]		)");
}
QUARK_UNIT_TEST("vector-int", "size()", "empty", "0"){
	test_result(R"(		let [int] a = [] result = size(a)		)", R"(		[ "^int", 0]		)");
}
QUARK_UNIT_TEST("vector-int", "size()", "2", ""){
	test_result(R"(		let [int] a = [1, 2, 3] result = size(a)		)", R"(		[ "^int", 3]		)");
}
QUARK_UNIT_TEST("vector-int", "+", "non-empty vectors", ""){
	test_result(R"(		let [int] result = [1, 2] + [3, 4]		)", R"(		[[ "vector", "^int" ], [1, 2, 3, 4]]		)");
}
QUARK_UNIT_TEST("vector-int", "push_back()", "", ""){
	test_result(R"(		let [int] result = push_back([1, 2], 3)		)", R"(		[[ "vector", "^int" ], [1, 2, 3]]		)");
}


//////////////////////////////////////////		vector-double


QUARK_UNIT_TEST("vector-double", "literal expression", "", ""){
	test_result(R"(		let [double] result = [10.5, 20.5, 30.5]		)", R"(		[[ "vector", "^double" ], [10.5, 20.5, 30.5]]		)");
}
QUARK_UNIT_TEST("vector-double", "=", "copy", ""){
	test_result(R"(		let a = [10.5, 20.5, 30.5] result = a		)", R"(		[[ "vector", "^double" ], [10.5, 20.5, 30.5]]		)");
}
QUARK_UNIT_TEST("vector-double", "==", "same values", ""){
	test_result(R"(		let result = [1.5, 2.5] == [1.5, 2.5]		)", R"(		[ "^bool", true]		)");
}
QUARK_UNIT_TEST("vector-double", "==", "different values", ""){
	test_result(R"(		let result = [1.5, 3.5] == [1.5, 2.5]		)", R"(		[ "^bool", false]		)");
}
QUARK_UNIT_TEST("vector-double", "<", "", ""){
	test_result(R"(		let result = [1.5, 2.5] < [1.5, 2.5]		)", R"(		[ "^bool", false]	)");
}
QUARK_UNIT_TEST("vector-double", "<", "different values", ""){
	test_result(R"(		let result = [1.5, 2.5] < [1.5, 3.5]		)", R"(		[ "^bool", true]		)");
}
QUARK_UNIT_TEST("vector-double", "size()", "empty", "0"){
	test_result(R"(		let [double] a = [] result = size(a)		)", R"(		[ "^int", 0]		)");
}
QUARK_UNIT_TEST("vector-double", "size()", "2", ""){
	test_result(R"(		let [double] a = [1.5, 2.5, 3.5] result = size(a)		)", R"(		[ "^int", 3]		)");
}
QUARK_UNIT_TEST("vector-double", "+", "non-empty vectors", ""){
	test_result(R"(		let [double] result = [1.5, 2.5] + [3.5, 4.5]		)", R"(		[[ "vector", "^double" ], [1.5, 2.5, 3.5, 4.5]]		)");
}
QUARK_UNIT_TEST("vector-double", "push_back()", "", ""){
	test_result(R"(		let [double] result = push_back([1.5, 2.5], 3.5)		)", R"(		[[ "vector", "^double" ], [1.5, 2.5, 3.5]]		)");
}




//////////////////////////////////////////		FIND()




QUARK_UNIT_TEST("", "find()", "int", ""){
	const auto vm = test__run_global(R"(
		assert(find([1,2,3], 4) == -1)
		assert(find([1,2,3], 1) == 0)
		assert(find([1,2,2,2,3], 2) == 1)
	)");
}

QUARK_UNIT_TEST("", "find()", "string", ""){
	const auto vm = test__run_global(R"(
		assert(find("hello, world", "he") == 0)
		assert(find("hello, world", "e") == 1)
		assert(find("hello, world", "x") == -1)
	)");
}


//////////////////////////////////////////		SUBSET()


QUARK_UNIT_TEST("", "subset()", "int", ""){
	const auto vm = test__run_global(R"(
		assert(subset([10,20,30], 0, 3) == [10,20,30])
	)");
}
QUARK_UNIT_TEST("", "subset()", "int", ""){
	const auto vm = test__run_global(R"(
		assert(subset([10,20,30], 1, 3) == [20,30])
	)");
}
QUARK_UNIT_TEST("", "subset()", "int", ""){
	const auto vm = test__run_global(R"(
		result = (subset([10,20,30], 0, 0) == [])
	)");
}
QUARK_UNIT_TEST("", "subset()", "int", ""){
	const auto vm = test__run_global(R"(
		assert(subset([10,20,30], 0, 0) == [])
	)");
}


//////////////////////////////////////////		REPLACE()


QUARK_UNIT_TEST("", "replace()", "int", ""){
	const auto vm = test__run_global(R"(
		assert(replace([ 1, 2, 3, 4, 5, 6 ], 2, 5, [20, 30]) == [1, 2, 20, 30, 6])
	)");
}
// ### test pos limiting and edge cases.


//////////////////////////////////////////		UPDATE()


QUARK_UNIT_TEST("", "update()", "mutate element", "valid vector, without side effect on original vector"){
	const auto vm = test__run_global(R"(
		a = [ "one", "two", "three"]
		b = update(a, 1, "zwei")
		print(a)
		print(b)
		assert(a == ["one","two","three"])
		assert(b == ["one","zwei","three"])
	)");
	ut_compare_stringvects(vm->_print_output, vector<string>{
		R"(["one", "two", "three"])",
		R"(["one", "zwei", "three"])"
	});
}





//////////////////////////////////////////		DICT - TYPE


QUARK_UNIT_TEST("dict", "construct", "", ""){
	const auto vm = test__run_global(R"(
		let [string: int] a = {"one": 1, "two": 2}
		assert(size(a) == 2)
	)");
}
QUARK_UNIT_TEST("dict", "[]", "", ""){
	const auto vm = test__run_global(R"(
		let [string: int] a = {"one": 1, "two": 2}
		print(a["one"])
		print(a["two"])
	)");
	ut_compare_stringvects(vm->_print_output, vector<string>{
		"1",
		"2"
	});
}

QUARK_UNIT_TEST("dict", "", "", ""){
	try{
		const auto vm = test__run_global(R"(
			mutable a = {}
			a = {"hello": 1}
			print(a)
		)");
		QUARK_UT_VERIFY(false);
	}
	catch(const std::runtime_error& e){
		QUARK_UT_VERIFY(string(e.what()) == "Cannot resolve type.");
	}
}

QUARK_UNIT_TEST("dict", "[:]", "", ""){
	try {
		const auto vm = test__run_global(R"(
			let a = {}
			print(a)
		)");
		QUARK_UT_VERIFY(false);
	}
	catch(const std::runtime_error& e){
		QUARK_UT_VERIFY(string(e.what()) == "Cannot resolve type.");
	}
}


QUARK_UNIT_TEST("dict", "deduced type ", "", ""){
	const auto vm = test__run_global(R"(
		let a = {"one": 1, "two": 2}
		print(a)
	)");
	ut_compare_stringvects(vm->_print_output, vector<string>{
		R"({"one": 1, "two": 2})",
	});
}

QUARK_UNIT_TEST("dict", "{}", "", ""){
	const auto vm = test__run_global(R"(
		mutable [string:int] a = {}
		a = {}
		print(a)
	)");
	ut_compare_stringvects(vm->_print_output, vector<string>{
		R"({})",
	});
}

QUARK_UNIT_TEST("dict", "==", "", ""){
	const auto vm = test__run_global(R"(
		assert(({"one": 1, "two": 2} == {"one": 1, "two": 2}) == true)
	)");
}
QUARK_UNIT_TEST("dict", "==", "", ""){
	const auto vm = test__run_global(R"(
		assert(({"one": 1, "two": 2} == {"two": 2}) == false)
	)");
}
QUARK_UNIT_TEST("dict", "==", "", ""){
	const auto vm = test__run_global(R"(
		assert(({"one": 2, "two": 2} == {"one": 1, "two": 2}) == false)
	)");
}
QUARK_UNIT_TEST("dict", "==", "", ""){
	const auto vm = test__run_global(R"(
		assert(({"one": 1, "two": 2} < {"one": 1, "two": 2}) == false)
	)");
}
QUARK_UNIT_TEST("dict", "==", "", ""){
	const auto vm = test__run_global(R"(
		assert(({"one": 1, "two": 1} < {"one": 1, "two": 2}) == true)
	)");
}

QUARK_UNIT_TEST("dict", "size()", "[:]", "correct size"){
	try{
		const auto vm = test__run_global(R"(
			assert(size({}) == 0)
		)");
		QUARK_UT_VERIFY(false);
	}
	catch(const std::runtime_error& e){
		QUARK_TEST_VERIFY(string(e.what()) == "Cannot resolve type.");
	}
}

QUARK_UNIT_TEST("dict", "size()", "[:]", "correct type"){
	try{
		const auto vm = test__run_global(R"(
			print({})
		)");
		QUARK_UT_VERIFY(false);
	}
	catch(const std::runtime_error& e){
		QUARK_TEST_VERIFY(string(e.what()) == "Cannot resolve type.");
	}
}

QUARK_UNIT_TEST("dict", "size()", "[:]", "correct size"){
	const auto vm = test__run_global(R"(
		assert(size({"one":1}) == 1)
	)");
}
QUARK_UNIT_TEST("dict", "size()", "[:]", "correct size"){
	const auto vm = test__run_global(R"(
		assert(size({"one":1, "two":2}) == 2)
	)");
}

QUARK_UNIT_TEST("dict", "update()", "add element", "valid dict, without side effect on original dict"){
	const auto vm = test__run_global(R"(
		let a = { "one": 1, "two": 2}
		let b = update(a, "three", 3)
		print(a)
		print(b)
	)");
	ut_compare_stringvects(vm->_print_output, vector<string>{
		R"({"one": 1, "two": 2})",
		R"({"one": 1, "three": 3, "two": 2})"
	});
}

QUARK_UNIT_TEST("dict", "update()", "replace element", ""){
	const auto vm = test__run_global(R"(
		let a = { "one": 1, "two": 2, "three" : 3}
		let b = update(a, "three", 333)
		print(a)
		print(b)
	)");
	ut_compare_stringvects(vm->_print_output, vector<string>{
		R"({"one": 1, "three": 3, "two": 2})",
		R"({"one": 1, "three": 333, "two": 2})"
	});
}

QUARK_UNIT_TEST("dict", "update()", "dest is empty dict", ""){
	try{
		const auto vm = test__run_global(R"(
			let a = update({}, "one", 1)
			let b = update(a, "two", 2)
			print(b)
			assert(a == {"one": 1})
			assert(b == {"one": 1, "two": 2})
		)");

		QUARK_UT_VERIFY(false);
	}
	catch(const std::runtime_error& e){
		QUARK_TEST_VERIFY(string(e.what()) == "Cannot resolve type.");
	}
}

QUARK_UNIT_TEST("dict", "exists()", "", ""){
	const auto vm = test__run_global(R"(
		let a = { "one": 1, "two": 2, "three" : 3}
		assert(exists(a, "two") == true)
		assert(exists(a, "four") == false)
	)");
}

QUARK_UNIT_TEST("dict", "erase()", "", ""){
	const auto vm = test__run_global(R"(
		let a = { "one": 1, "two": 2, "three" : 3}
		let b = erase(a, "one")
		assert(b == { "two": 2, "three" : 3})
	)");
}


//////////////////////////////////////////		STRUCT - TYPE


QUARK_UNIT_TESTQ("run_main()", "struct"){
	const auto vm = test__run_global(R"(
		struct t {}
	)");
}

QUARK_UNIT_TESTQ("run_main()", "struct"){
	const auto vm = test__run_global(R"(
		struct t { int a }
	)");
}

QUARK_UNIT_TEST("run_main()", "struct - check struct's type", "", ""){
	const auto vm = test__run_global(R"(
		struct t { int a }
		print(t)
	)");
	ut_compare_stringvects(vm->_print_output, vector<string>{
		"struct {int a;}"
	});
}

QUARK_UNIT_TESTQ("run_main()", "struct - check struct's type"){
	const auto vm = test__run_global(R"(
		struct t { int a }
		let a = t(3)
		print(a)
	)");
	ut_compare_stringvects(vm->_print_output, vector<string>{
		 R"({a=3})"
	});
}

QUARK_UNIT_TESTQ("run_main()", "struct - read back struct member"){
	const auto vm = test__run_global(R"(
		struct t { int a }
		let temp = t(4)
		print(temp.a)
	)");
	ut_compare_stringvects(vm->_print_output, vector<string>{
		"4"
	});
}

QUARK_UNIT_TEST("run_main()", "struct - instantiate nested structs", "", ""){
	const auto vm = test__run_global(R"(
		struct color { int red int green int blue }
		struct image { color back color front }

		let c = color(128, 192, 255)
		print(c)
		let i = image(color(1, 2, 3), color(200, 201, 202))
		print(i)
	)");
	ut_compare_stringvects(vm->_print_output, vector<string>{
		"{red=128, green=192, blue=255}",
		"{back={red=1, green=2, blue=3}, front={red=200, green=201, blue=202}}"
	});
}

QUARK_UNIT_TEST("run_main()", "struct - access member of nested structs", "", ""){
	const auto vm = test__run_global(R"(
		struct color { int red int green int blue }
		struct image { color back color front }
		let i = image(color(1, 2, 3), color(200, 201, 202))
		print(i.front.green)
	)");
	ut_compare_stringvects(vm->_print_output, vector<string>{
		"201"
	});
}

QUARK_UNIT_TEST("run_main()", "return struct from function", "", ""){
	const auto vm = test__run_global(R"(
		struct color { int red int green int blue }
		struct image { color back color front }
		func color make_color(){
			return color(100, 101, 102)
		}
		let z = make_color()
		print(z)
	)");
	ut_compare_stringvects(vm->_print_output, vector<string>{
		"{red=100, green=101, blue=102}",
	});
}

QUARK_UNIT_TEST("run_main()", "return struct from function", "", ""){
	const auto vm = test__run_global(R"(
		struct color { int red int green int blue }
		struct image { color back color front }
		func color make_color(){
			return color(100, 101, 102)
		}
		print(make_color())
	)");
	ut_compare_stringvects(vm->_print_output, vector<string>{
		"{red=100, green=101, blue=102}",
	});
}

QUARK_UNIT_TESTQ("run_main()", "struct - compare structs"){
	const auto vm = test__run_global(R"(
		struct color { int red int green int blue }
		print(color(1, 2, 3) == color(1, 2, 3))
	)");
	ut_compare_stringvects(vm->_print_output, vector<string>{
		"true"
	});
}

QUARK_UNIT_TESTQ("run_main()", "struct - compare structs"){
	const auto vm = test__run_global(R"(
		struct color { int red int green int blue }
		print(color(9, 2, 3) == color(1, 2, 3))
	)");
	ut_compare_stringvects(vm->_print_output, vector<string>{
		"false"
	});
}

QUARK_UNIT_TESTQ("run_main()", "struct - compare structs different types"){
	try {
		const auto vm = test__run_global(R"(
			struct color { int red int green int blue }
			struct file { int id }
			print(color(1, 2, 3) == file(404))
		)");
		QUARK_UT_VERIFY(false);
	}
	catch(const std::runtime_error& e){
		QUARK_TEST_VERIFY(string(e.what()) == "Expression type mismatch. Cannot convert 'struct {int id;}' to 'struct {int red;int green;int blue;}.");
	}
}
QUARK_UNIT_TESTQ("run_main()", "struct - compare structs with <, different types"){
	const auto vm = test__run_global(R"(
		struct color { int red int green int blue }
		print(color(1, 2, 3) < color(1, 2, 3))
	)");
	ut_compare_stringvects(vm->_print_output, vector<string>{
		"false"
	});
}

QUARK_UNIT_TESTQ("run_main()", "struct - compare structs <"){
	const auto vm = test__run_global(R"(
		struct color { int red int green int blue }
		print(color(1, 2, 3) < color(1, 4, 3))
	)");
	ut_compare_stringvects(vm->_print_output, vector<string>{
		"true"
	});
}


QUARK_UNIT_TESTQ("run_main()", "update struct manually"){
	const auto vm = test__run_global(R"(
		struct color { int red int green int blue }
		let a = color(255, 128, 128)
		let b = color(a.red, a.green, 129)
		print(a)
		print(b)
	)");
	ut_compare_stringvects(vm->_print_output, vector<string>{
		"{red=255, green=128, blue=128}",
		"{red=255, green=128, blue=129}"
	});
}


QUARK_UNIT_TEST("run_main()", "mutate struct member using = won't work", "", ""){
	try {
		const auto vm = test__run_global(R"(
			struct color { int red int green int blue }
			let a = color(255,128,128)
			let b = a.green = 3
			print(a)
			print(b)
		)");
	}
	catch(const std::runtime_error& e){
		QUARK_UT_VERIFY(string(e.what()) == "Expected constant or identifier.");
	}
}

QUARK_UNIT_TESTQ("run_main()", "mutate struct member using update()"){
	const auto vm = test__run_global(R"(
		struct color { int red int green int blue }
		let a = color(255,128,128)
		let b = update(a, "green", 3)
		print(a)
		print(b)
	)");

	ut_compare_stringvects(vm->_print_output, vector<string>{
		"{red=255, green=128, blue=128}",
		"{red=255, green=3, blue=128}",
	});
}

QUARK_UNIT_TEST("run_main()", "mutate nested member", "", ""){
	const auto vm = test__run_global(R"(
		struct color { int red int green int blue }
		struct image { color back color front }
		let a = image(color(0,100,200), color(0,0,0))
		let b = update(a, "front.green", 3)
		print(a)
		print(b)
	)");
	ut_compare_stringvects(vm->_print_output, vector<string>{
		"{back={red=0, green=100, blue=200}, front={red=0, green=0, blue=0}}",
		"{back={red=0, green=100, blue=200}, front={red=0, green=3, blue=0}}"
	});
}


/*
QUARK_UNIT_TEST("run_main()", "struct definition expression", "", ""){
	const auto vm = test__run_global(R"(
		color = struct { int red; int green; int blue;};
		a = color(255, 128, 128);
		b = color(a.red, a.green, 129);
		print(a);
		print(b);
	)");
	ut_compare_stringvects(vm->_print_output, vector<string>{
		"{red=255, green=128, blue=128}",
		"{red=255, green=128, blue=129}"
	});
}
*/


//////////////////////////////////////////		PROTOCOL - TYPE


QUARK_UNIT_TEST("run_main()", "protocol", "", ""){
	const auto vm = test__run_global(R"(
		protocol t {}
	)");
}

QUARK_UNIT_TEST("run_main()", "protocol", "", ""){
	const auto vm = test__run_global(R"(
		protocol t {
			func string f(int a, double b)
		}
	)");
}

#if 0
QUARK_UNIT_TEST("run_main()", "protocol - check protocol's type", "", ""){
	const auto vm = test__run_global(R"(
		protocol t { int a }
		print(t)
	)");
	ut_compare_stringvects(vm->_print_output, vector<string>{
		"protocol {int a;}"
	});
}
#endif


//////////////////////////////////////////


QUARK_UNIT_TEST("", "", "", ""){
	const auto vm = test__run_global(R"(
		let start = get_time_of_day()
		mutable b = 0
		mutable t = [0]
		for(i in 0...100){
			b = b + 1
			t = push_back(t, b)
		}
		end = get_time_of_day()
		print("Duration: " + to_string(end - start) + ", number = " + to_string(b))
		print(t)
	)");
	QUARK_UT_VERIFY(true);
}

//////////////////////////////////////////		Comments


QUARK_UNIT_TESTQ("comments", "// on start of line"){
	const auto vm = test__run_global(R"(
		//	XYZ
		print("Hello")
	)");
	ut_compare_stringvects(vm->_print_output, vector<string>{
		"Hello"
	});
}

QUARK_UNIT_TEST("comments", "// on start of line", "", ""){
	const auto vm = test__run_global(R"(
		print("Hello")		//	XYZ
	)");
	ut_compare_stringvects(vm->_print_output, vector<string>{
		"Hello"
	});
}

QUARK_UNIT_TEST("comments", "// on start of line", "", ""){
	const auto vm = test__run_global(R"(
		print("Hello")/* xyz */print("Bye")
	)");
	ut_compare_stringvects(vm->_print_output, vector<string>{
		"Hello",
		"Bye"
	});
}


//////////////////////////////////////////		json_value - TYPE


QUARK_UNIT_TEST("json_value-string", "deduce json_value::string", "", ""){
	const auto result = test__run_return_result(R"(
		let json_value result = "hello"
	)", {});
	ut_compare_values(result, value_t::make_json_value("hello"));
}

QUARK_UNIT_TEST("json_value-string", "string-size()", "", ""){
	const auto result = test__run_return_result(R"(
		let json_value a = "hello"
		let result = size(a);
	)", {});
	ut_compare_values(result, value_t::make_int(5));
}

QUARK_UNIT_TEST("json_value-number", "construct number", "", ""){
	const auto result = test__run_return_result(R"(
		let json_value result = 13
	)", {});
	ut_compare_values(result, value_t::make_json_value(13));
}

QUARK_UNIT_TEST("json_value-bool", "construct true", "", ""){
	const auto result = test__run_return_result(R"(
		let json_value result = true
	)", {});
	ut_compare_values(result, value_t::make_json_value(true));
}
QUARK_UNIT_TEST("json_value-bool", "construct false", "", ""){
	const auto result = test__run_return_result(R"(
		let json_value result = false
	)", {});
	ut_compare_values(result, value_t::make_json_value(false));
}

QUARK_UNIT_TEST("json_value-array", "construct array", "", ""){
	const auto result = test__run_return_result(R"(
		let json_value result = ["hello", "bye"]
	)", {});
	ut_compare_values(result, value_t::make_json_value(json_t::make_array(vector<json_t>{"hello", "bye"})));
}

QUARK_UNIT_TEST("json_value-array", "read array member", "", ""){
	const auto result = test__run_return_result(R"(
		let json_value a = ["hello", "bye"]
		let result = string(a[0]) + string(a[1])
	)", {});
	ut_compare_values(result, value_t::make_string("hellobye"));
}
QUARK_UNIT_TEST("json_value-array", "read array member", "", ""){
	const auto result = test__run_return_result(R"(
		let json_value a = ["hello", "bye"]
		let result = a[0]
	)", {});
	ut_compare_values(result, value_t::make_json_value("hello"));
}
QUARK_UNIT_TEST("json_value-array", "read array member", "", ""){
	const auto result = test__run_return_result(R"(
		let json_value a = ["hello", "bye"]
		let result = a[1]
	)", {});
	ut_compare_values(result, value_t::make_json_value("bye"));
}

QUARK_UNIT_TEST("json_value-array", "size()", "", ""){
	const auto result = test__run_return_result(R"(
		let json_value a = ["a", "b", "c", "d"]
		let result = size(a)
	)", {});
	ut_compare_values(result, value_t::make_int(4));
}

//	NOTICE: Floyd dict is stricter than JSON -- cannot have different types of values!
QUARK_UNIT_TEST("json_value-object", "def", "mix value-types in dict", ""){
	const auto vm = test__run_global(R"(
		let json_value a = { "pigcount": 3, "pigcolor": "pink" }
		print(a)
	)");
	ut_compare_stringvects(vm->_print_output, vector<string>{
		R"({ "pigcolor": "pink", "pigcount": 3 })"
	});
}

// JSON example snippets: http://json.org/example.html
QUARK_UNIT_TEST("json_value-object", "def", "read world data", ""){
	const auto vm = test__run_global(R"ABCD(
		let json_value a = {
			"menu": {
			  "id": "file",
			  "value": "File",
			  "popup": {
				"menuitem": [
				  {"value": "New", "onclick": "CreateNewDoc()"},
				  {"value": "Open", "onclick": "OpenDoc()"},
				  {"value": "Close", "onclick": "CloseDoc()"}
				]
			  }
			}
		}
		print(a)
	)ABCD");
	ut_compare_stringvects(vm->_print_output, vector<string>{
		R"ABCD({ "menu": { "id": "file", "popup": { "menuitem": [{ "onclick": "CreateNewDoc()", "value": "New" }, { "onclick": "OpenDoc()", "value": "Open" }, { "onclick": "CloseDoc()", "value": "Close" }] }, "value": "File" } })ABCD"
	});
}

QUARK_UNIT_TEST("json_value-object", "{}", "expressions inside def", ""){
	const auto vm = test__run_global(R"(
		let json_value a = { "pigcount": 1 + 2, "pigcolor": "pi" + "nk" }
		print(a)
	)");
	ut_compare_stringvects(vm->_print_output, vector<string>{
		R"({ "pigcolor": "pink", "pigcount": 3 })"
	});
}

QUARK_UNIT_TEST("json_value-object", "{}", "", ""){
	const auto vm = test__run_global(R"(
		let json_value a = { "pigcount": 3, "pigcolor": "pink" }
		print(a["pigcount"])
		print(a["pigcolor"])
	)");
	ut_compare_stringvects(vm->_print_output, vector<string>{
		"3",
		"\"pink\""
	});
}

QUARK_UNIT_TEST("json_value-object", "size()", "", ""){
	const auto vm = test__run_global(R"(
		let json_value a = { "a": 1, "b": 2, "c": 3, "d": 4, "e": 5 }
		assert(size(a) == 5)
	)");
}


QUARK_UNIT_TEST("json_value-null", "construct null", "", ""){
	const auto result = test__run_return_result(R"(
		let json_value result = null
	)", {});
	ut_compare_values(result, value_t::make_json_value(json_t()));
}


//////////////////////////////////////////		TEST TYPE DEDUCING

QUARK_UNIT_TEST("", "get_json_type()", "{}", ""){
	const auto result = test__run_return_result(R"(
		let json_value result = {}
	)", {});
	ut_compare_values(result, value_t::make_json_value(json_t::make_object()));
}



//////////////////////////////////////////		json_value features




QUARK_UNIT_TEST("", "get_json_type()", "{}", ""){
	const auto result = test__run_return_result(R"(
		let result = get_json_type(json_value({}))
	)", {});
	ut_compare_values(result, value_t::make_int(1));
}
QUARK_UNIT_TEST("", "get_json_type()", "[]", ""){
	const auto result = test__run_return_result(R"(
		let result = get_json_type(json_value([]))
	)", {});
	ut_compare_values(result, value_t::make_int(2));
}
QUARK_UNIT_TEST("", "get_json_type()", "string", ""){
	const auto result = test__run_return_result(R"(
		let result = get_json_type(json_value("hello"))
	)", {});
	ut_compare_values(result, value_t::make_int(3));
}
QUARK_UNIT_TEST("", "get_json_type()", "number", ""){
	const auto result = test__run_return_result(R"(
		let result = get_json_type(json_value(13))
	)", {});
	ut_compare_values(result, value_t::make_int(4));
}
QUARK_UNIT_TEST("", "get_json_type()", "number", ""){
	const auto result = test__run_return_result(R"(
		let result = get_json_type(json_value(true))
	)", {});
	ut_compare_values(result, value_t::make_int(5));
}
QUARK_UNIT_TEST("", "get_json_type()", "number", ""){
	const auto result = test__run_return_result(R"(
		let result = get_json_type(json_value(false))
	)", {});
	ut_compare_values(result, value_t::make_int(6));
}

QUARK_UNIT_TEST("", "get_json_type()", "null", ""){
	const auto result = test__run_return_result(R"(
		let result = get_json_type(json_value(null))
	)", {});
	ut_compare_values(result, value_t::make_int(7));
}

QUARK_UNIT_TEST("", "get_json_type()", "DOCUMENTATION SNIPPET", ""){
	const auto vm = test__run_global(R"ABCD(
		func string get_name(json_value value){
			t = get_json_type(value)
			if(t == json_object){
				return "json_object"
			}
			else if(t == json_array){
				return "json_array"
			}
			else if(t == json_string){
				return "json_string"
			}
			else if(t == json_number){
				return "json_number"
			}
			else if(t == json_true){
				return "json_true"
			}
			else if(t == json_false){
				return "json_false"
			}
			else if(t == json_null){
				return "json_null"
			}
			else {
				assert(false)
			}
		}

		assert(get_name(json_value({"a": 1, "b": 2})) == "json_object")
		assert(get_name(json_value([1,2,3])) == "json_array")
		assert(get_name(json_value("crash")) == "json_string")
		assert(get_name(json_value(0.125)) == "json_number")
		assert(get_name(json_value(true)) == "json_true")
		assert(get_name(json_value(false)) == "json_false")
	)ABCD");
}


QUARK_UNIT_TEST("", "", "", ""){
	const auto result = test__run_return_result(R"(
		struct pixel_t { double x double y }
		let a = pixel_t(100.0, 200.0)
		let result = a.x
	)", {});
	ut_compare_values(result, value_t::make_double(100.0));
}


QUARK_UNIT_TEST("", "", "", ""){
	const auto result = test__run_return_result(R"(
		struct pixel_t { double x double y }
		let c = [pixel_t(100.0, 200.0), pixel_t(101.0, 201.0)]
		let result = c[1].y
	)", {});
	ut_compare_values(result, value_t::make_double(201.0));
}

QUARK_UNIT_TEST("", "", "", ""){
	try{
		const auto result = test__run_return_result(R"(
			struct pixel_t { double x double y }

			//	c is a json_value::object
			let c = { "version": "1.0", "image": [pixel_t(100.0, 200.0), pixel_t(101.0, 201.0)] }
			let result = c["image"][1].y
		)", {});
	}
	catch(const std::runtime_error& e){
		QUARK_TEST_VERIFY(string(e.what()) == "Dict can not hold elements of different type!");
	}
}


//////////////////////////////////////////		script_to_jsonvalue()


QUARK_UNIT_TEST("", "script_to_jsonvalue()", "", ""){
	const auto result = test__run_return_result(R"(
		let result = script_to_jsonvalue("\"genelec\"")
	)", {});
	ut_compare_values(result, value_t::make_json_value(json_t("genelec")));
}

QUARK_UNIT_TEST("", "script_to_jsonvalue()", "", ""){
	const auto vm = test__run_global(R"(
		let a = script_to_jsonvalue("{ \"magic\": 1234 }")
		print(a)
	)");
	ut_compare_stringvects(vm->_print_output, vector<string>{
		R"({ "magic": 1234 })"
	});
}


//////////////////////////////////////////		jsonvalue_to_script()


QUARK_UNIT_TEST("", "jsonvalue_to_script()", "", ""){
	const auto vm = test__run_global(R"(
		let json_value a = "cheat"
		let b = jsonvalue_to_script(a)
		print(b)
	)");
	ut_compare_stringvects(vm->_print_output, vector<string>{
		"\"cheat\""
	});
}


QUARK_UNIT_TEST("", "jsonvalue_to_script()", "", ""){
	const auto vm = test__run_global(R"(
		let json_value a = { "magic": 1234 }
		let b = jsonvalue_to_script(a)
		print(b)
	)");
	ut_compare_stringvects(vm->_print_output, vector<string>{
		"{ \"magic\": 1234 }"
	});
}


//////////////////////////////////////////		value_to_jsonvalue()


QUARK_UNIT_TEST("", "value_to_jsonvalue()", "bool", ""){
	const auto result = test__run_return_result(R"(
		let result = value_to_jsonvalue(true)
	)", {});
	ut_compare_values(result, value_t::make_json_value(json_t(true)));
}
QUARK_UNIT_TEST("", "value_to_jsonvalue()", "bool", ""){
	const auto result = test__run_return_result(R"(
		let result = value_to_jsonvalue(false)
	)", {});
	ut_compare_values(result, value_t::make_json_value(json_t(false)));
}

QUARK_UNIT_TEST("", "value_to_jsonvalue()", "int", ""){
	const auto result = test__run_return_result(R"(
		let result = value_to_jsonvalue(789)
	)", {});
	ut_compare_values(result, value_t::make_json_value(json_t(789.0)));
}
QUARK_UNIT_TEST("", "value_to_jsonvalue()", "int", ""){
	const auto result = test__run_return_result(R"(
		let result = value_to_jsonvalue(-987)
	)", {});
	ut_compare_values(result, value_t::make_json_value(json_t(-987.0)));
}

QUARK_UNIT_TEST("", "value_to_jsonvalue()", "double", ""){
	const auto result = test__run_return_result(R"(
		let result = value_to_jsonvalue(-0.125)
	)", {});
	ut_compare_values(result, value_t::make_json_value(json_t(-0.125)));
}


QUARK_UNIT_TEST("", "value_to_jsonvalue()", "string", ""){
	const auto result = test__run_return_result(R"(
		let result = value_to_jsonvalue("fanta")
	)", {});
	ut_compare_values(result, value_t::make_json_value(json_t("fanta")));
}

QUARK_UNIT_TEST("", "value_to_jsonvalue()", "typeid", ""){
	const auto result = test__run_return_result(R"(
		let result = value_to_jsonvalue(typeof([2,2,3]))
	)", {});
	ut_compare_values(result, value_t::make_json_value(json_t::make_array(vector<json_t>{ "vector", "int"})));
}

QUARK_UNIT_TEST("", "value_to_jsonvalue()", "[]", ""){
	const auto result = test__run_return_result(R"(
		let result = value_to_jsonvalue([1,2,3])
	)", {});
	ut_compare_values(result, value_t::make_json_value(
		json_t::make_array(vector<json_t>{1,2,3})
	));
}

QUARK_UNIT_TEST("", "value_to_jsonvalue()", "{}", ""){
	const auto result = test__run_return_result(R"(
		let result = value_to_jsonvalue({"ten": 10, "eleven": 11})
	)", {});
	ut_compare_values(result, value_t::make_json_value(
		json_t::make_object(std::map<string,json_t>{{"ten", 10},{"eleven", 11}})
	));
}

QUARK_UNIT_TEST("", "value_to_jsonvalue()", "pixel_t", ""){
	const auto result = test__run_return_result(R"(

		struct pixel_t { double x double y }
		let c = pixel_t(100.0, 200.0)
		let a = value_to_jsonvalue(c)
		let result = jsonvalue_to_script(a)

	)", {});
	ut_compare_values(result, value_t::make_string("{ \"x\": 100, \"y\": 200 }"));
}

QUARK_UNIT_TEST("", "value_to_jsonvalue()", "[pixel_t]", ""){
	const auto result = test__run_return_result(R"(

		struct pixel_t { double x double y }
		let c = [pixel_t(100.0, 200.0), pixel_t(101.0, 201.0)]
		let a = value_to_jsonvalue(c)
		let result = jsonvalue_to_script(a)

	)", {});
	ut_compare_values(result, value_t::make_string("[{ \"x\": 100, \"y\": 200 }, { \"x\": 101, \"y\": 201 }]"));
}


//////////////////////////////////////////		value_to_jsonvalue() -> jsonvalue_to_value() roundtrip


QUARK_UNIT_TEST("", "jsonvalue_to_value()", "bool", ""){
	const auto result = test__run_return_result(R"(
		let result = jsonvalue_to_value(value_to_jsonvalue(true), bool)
	)", {});
	ut_compare_values(result, value_t::make_bool(true));
}
QUARK_UNIT_TEST("", "jsonvalue_to_value()", "bool", ""){
	const auto result = test__run_return_result(R"(
		let result = jsonvalue_to_value(value_to_jsonvalue(false), bool)
	)", {});
	ut_compare_values(result, value_t::make_bool(false));
}

QUARK_UNIT_TEST("", "jsonvalue_to_value()", "int", ""){
	const auto result = test__run_return_result(R"(
		let result = jsonvalue_to_value(value_to_jsonvalue(91), int)
	)", {});
	ut_compare_values(result, value_t::make_int(91));
}

QUARK_UNIT_TEST("", "jsonvalue_to_value()", "double", ""){
	const auto result = test__run_return_result(R"(
		let result = jsonvalue_to_value(value_to_jsonvalue(-0.125), double)
	)", {});
	ut_compare_values(result, value_t::make_double(-0.125));
}

QUARK_UNIT_TEST("", "jsonvalue_to_value()", "string", ""){
	const auto result = test__run_return_result(R"(
		let result = jsonvalue_to_value(value_to_jsonvalue(""), string)
	)", {});
	ut_compare_values(result, value_t::make_string(""));
}

QUARK_UNIT_TEST("", "jsonvalue_to_value()", "string", ""){
	const auto result = test__run_return_result(R"(
		let result = jsonvalue_to_value(value_to_jsonvalue("cola"), string)
	)", {});
	ut_compare_values(result, value_t::make_string("cola"));
}


/*
Need better way to specify typeid
QUARK_UNIT_TEST("", "jsonvalue_to_value()", "[]", ""){
	const auto result = test__run_return_result(R"(
		result = jsonvalue_to_value(value_to_jsonvalue([1,2,3]), typeof([1]));
	)", {});
	ut_compare_values(
		result,
		value_t::make_vector_value(typeid_t::make_int(), vector<value_t>{value_t::make_int(1), value_t::make_int(2), value_t::make_int(3)})
	);
}
*/


QUARK_UNIT_TEST("", "jsonvalue_to_value()", "point_t", ""){
	const auto point_t_def = std::vector<member_t>{
		member_t(typeid_t::make_double(), "x"),
		member_t(typeid_t::make_double(), "y")
	};
	const auto result = test__run_return_result(R"(
		struct point_t { double x double y }
		let result = jsonvalue_to_value(value_to_jsonvalue(point_t(1.0, 3.0)), point_t)
	)", {});
	ut_compare_values(
		result,
		value_t::make_struct_value(
			typeid_t::make_struct2(point_t_def),
			vector<value_t>{ value_t::make_double(1), value_t::make_double(3)}
		)
	);
}






///////////////////////////////////////////////////			TEST LIBRARY FEATURES


//??? It IS OK to call impure functions from a pure function:
//		1. The impure function modifies a local value only -- cannot leak out.

QUARK_UNIT_TEST("", "impure", "call pure->pure", "Compiles OK"){
	const auto result = test__run_global(R"(

		func int a(int p){ return p + 1 }
		func int b(){ return a(100) }

	)");
}
QUARK_UNIT_TEST("", "impure", "call impure->pure", "Compiles OK"){
	const auto result = test__run_global(R"(

		func int a(int p){ return p + 1 }
		func int b() impure { return a(100) }

	)");
}

QUARK_UNIT_TEST("", "impure", "call pure->impure", "Compilation error"){
	try {
		const auto result = test__run_global(R"(

			func int a(int p) impure { return p + 1 }
			func int b(){ return a(100) }

		)");
		QUARK_TEST_VERIFY(false);
	}
	catch(const std::runtime_error& e){
		QUARK_TEST_VERIFY(string(e.what()) == "Cannot call impure function pure function.");
	}
}

QUARK_UNIT_TEST("", "impure", "call impure->impure", "Compiles OK"){
	const auto result = test__run_global(R"(

		func int a(int p) impure { return p + 1 }
		func int b() impure { return a(100) }

	)");
}





///////////////////////////////////////////////////			TEST LIBRARY FEATURES






QUARK_UNIT_TEST("", "cmath_pi", "", ""){
	const auto result = test__run_return_result(R"(
		let result = cmath_pi
	)", {});

	QUARK_UT_VERIFY(result.get_double_value() >= 3.14 && result.get_double_value() < 3.15);
}

QUARK_UNIT_TEST("", "color__black", "", ""){
	const auto result = test__run_global(R"(
		assert(color__black.red == 0.0)
		assert(color__black.green == 0.0)
		assert(color__black.blue == 0.0)
		assert(color__black.alpha == 1.0)
	)");
}


QUARK_UNIT_TEST("", "color__black", "", ""){
	const auto result = test__run_global(R"(

		let r = add_colors(color_t(1.0, 2.0, 3.0, 4.0), color_t(1000.0, 2000.0, 3000.0, 4000.0))
		print(to_string(r))
		assert(r.red == 1001.0)
		assert(r.green == 2002.0)
		assert(r.blue == 3003.0)
		assert(r.alpha == 4004.0)

	)");
}





QUARK_UNIT_TEST("", "", "", ""){
	const auto a = typeid_t::make_vector(typeid_t::make_string());
	const auto b = typeid_t::make_vector(make__fsentry_t__type());
	QUARK_UT_VERIFY(a != b);
}



//////////////////////////////////////////		HOST FUNCTION - print()


QUARK_UNIT_TEST("run_global()", "Print Hello, world!", "", ""){
	const auto r = test__run_global(
		R"(
			print("Hello, World!")
		)"
	);
	QUARK_UT_VERIFY((r->_print_output == vector<string>{ "Hello, World!" }));
}


QUARK_UNIT_TEST("run_global()", "Test that VM state (print-log) escapes block!", "", ""){
	const auto r = test__run_global(
		R"(
			{
				print("Hello, World!")
			}
		)"
	);
	QUARK_UT_VERIFY((r->_print_output == vector<string>{ "Hello, World!" }));
}

QUARK_UNIT_TESTQ("run_global()", "Test that VM state (print-log) escapes IF!"){
	const auto r = test__run_global(
		R"(
			if(true){
				print("Hello, World!")
			}
		)"
	);
	QUARK_UT_VERIFY((r->_print_output == vector<string>{ "Hello, World!" }));
}



//////////////////////////////////////////		HOST FUNCTION - get_time_of_day()


QUARK_UNIT_TESTQ("run_init()", "get_time_of_day()"){
	test__run_global(
		R"(
			let int a = get_time_of_day()
			let int b = get_time_of_day()
			let int c = b - a
			print("Delta time:" + to_string(a))
		)"
	);
}



QUARK_UNIT_TEST("", "calc_string_sha1()", "", ""){
	test__run_global(
		R"(

			let a = calc_string_sha1("Violator is the seventh studio album by English electronic music band Depeche Mode.")
			print(to_string(a))
			assert(a.ascii40 == "4d5a137b3b1faf855872a312a184dd9a24594387")
		)"
	);
}
QUARK_UNIT_TEST("", "calc_binary_sha1()", "", ""){
	test__run_global(
		R"(

			let bin = binary_t("Violator is the seventh studio album by English electronic music band Depeche Mode.")
			let a = calc_binary_sha1(bin)
			print(to_string(a))
			assert(a.ascii40 == "4d5a137b3b1faf855872a312a184dd9a24594387")
		)"
	);
}



//////////////////////////////////////////		HOST FUNCTION - map()



QUARK_UNIT_TEST("", "map()", "[int] map(int f(int))", ""){
	test__run_global(
		R"(

			let a = [ 10, 11, 12 ]

			func int f(int v){
				return 1000 + v
			}

			let result = map(a, f)
			print(to_string(result))
			assert(result == [ 1010, 1011, 1012 ])

		)"
	);
}

QUARK_UNIT_TEST("", "map()", "[string] map(string f(int))", ""){
	test__run_global(
		R"(

			let a = [ 10, 11, 12 ]

			func string f(int v){
				return to_string(1000 + v)
			}

			let result = map(a, f)
			print(to_string(result))
			assert(result == [ "1010", "1011", "1012" ])

		)"
	);
}

QUARK_UNIT_TEST("", "map()", "[int] map(int f(string))", ""){
	test__run_global(
		R"(

			let a = [ "one", "two_", "three" ]

			func int f(string v){
				return size(v)
			}

			let result = map(a, f)
			print(to_string(result))
			assert(result == [ 3, 4, 5 ])

		)"
	);
}


//////////////////////////////////////////		HOST FUNCTION - map_string()



QUARK_UNIT_TEST("", "map_string()", "", ""){
	test__run_global(
		R"(

			let a = "ABC"

			func string f(string v){
				assert(size(v) == 1)

				let int ch = v[0]
				return to_string(ch)
			}

			let result = map_string(a, f)
			print(to_string(result))
			assert(result == "656667")

		)"
	);
}



//////////////////////////////////////////		HOST FUNCTION - reduce()



QUARK_UNIT_TEST("", "reduce()", "int reduce([int], int, func int(int, int))", ""){
	test__run_global(
		R"(

			func int f(int acc, int element){
				return acc + element * 2
			}

			let result = reduce([ 10, 11, 12 ], 2000, f)

			print(to_string(result))
			assert(result == 2066)

		)"
	);
}

QUARK_UNIT_TEST("", "reduce()", "string reduce([int], string, func int(string, int))", ""){
	test__run_global(
		R"xxx(

			func string f(string acc, int v){
				mutable s = acc
				for(e in 0 ..< v){
					s = "<" + s + ">"
				}
				s = "(" + s + ")"
				return s
			}

			let result = reduce([ 2, 4, 1 ], "O", f)
			print(to_string(result))
			assert(result == "(<(<<<<(<<O>>)>>>>)>)")

		)xxx"
	);
}




//////////////////////////////////////////		HOST FUNCTION - filter()



QUARK_UNIT_TEST("", "filter()", "int filter([int], int, func int(int, int))", ""){
	test__run_global(
		R"(

			func bool f(int element){
				return element % 3 == 0 ? true : false
			}

			let result = filter([ 3, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13 ], f)

			print(to_string(result))
			assert(result == [ 3, 3, 6, 9, 12 ])

		)"
	);
}

QUARK_UNIT_TEST("", "filter()", "string filter([int], string, func int(string, int))", ""){
	test__run_global(
		R"xxx(

			func bool f(string element){
				return size(element) == 3 || size(element) == 5
			}

			let result = filter([ "one", "two", "three", "four", "five", "six", "seven" ], f)

			print(to_string(result))
			assert(result == [ "one", "two", "three", "six", "seven" ])

		)xxx"
	);
}





//////////////////////////////////////////		HOST FUNCTION - supermap()



QUARK_UNIT_TEST("", "supermap()", "No dependencies", ""){
	test__run_global(
		R"(

			func string f(string v, [string] inputs){
				return "[" + v + "]"
			}

			let result = supermap([ "one", "ring", "to" ], [ -1, -1, -1 ], f)
			print(to_string(result))
			assert(result == [ "[one]", "[ring]", "[to]" ])

		)"
	);
}

QUARK_UNIT_TEST("", "supermap()", "No dependencies", ""){
	test__run_global(
		R"(

			func string f(string v, [string] inputs){
				return "[" + v + "]"
			}

			let result = supermap([ "one", "ring", "to" ], [ 1, 2, -1 ], f)
			print(to_string(result))
			assert(result == [ "[one]", "[ring]", "[to]" ])

		)"
	);
}





QUARK_UNIT_TEST("", "supermap()", "complex", ""){
	test__run_global(
		R"(

			func string f2(string acc, string element){
				if(size(acc) == 0){
					return element
				}
				else{
					return acc + ", " + element
				}
			}

			func string f(string v, [string] inputs){
				let s = reduce(inputs, "", f2)
				return v + "[" + s + "]"
			}

			let result = supermap([ "D", "B", "A", "C", "E", "F" ], [ 4, 2, -1, 4, 2, 4 ], f)
			print(to_string(result))
			assert(result == [ "D[]", "B[]", "A[B[], E[D[], C[], F[]]]", "C[]", "E[D[], C[], F[]]", "F[]" ])

		)"
	);
}





//////////////////////////////////////////		HOST FUNCTION - read_text_file()

/*
QUARK_UNIT_TEST("", "read_text_file()", "", ""){
	const auto vm = test__run_global(R"(
		path = get_env_path();
		a = read_text_file(path + "/Desktop/test1.json");
		print(a);
	)");
	ut_compare_stringvects(vm->_print_output, vector<string>{
		string() + R"({ "magic": 1234 })" + "\n"
	});
}
*/


//////////////////////////////////////////		HOST FUNCTION - write_text_file()


QUARK_UNIT_TEST("", "write_text_file()", "", ""){
	const auto vm = test__run_global(R"(
		let path = get_fs_environment().desktop_dir
		write_text_file(path + "/test_out.txt", "Floyd wrote this!")
	)");
}

//////////////////////////////////////////		HOST FUNCTION - instantiate_from_typeid()

//	instantiate_from_typeid() only works for const-symbols right now.
/*
??? Support when we can make "t = typeof(1234)" a const-symbol
QUARK_UNIT_TEST("run_global()", "", "", ""){
	const auto result = test__run_return_result(
		R"(
			t = typeof(1234);
			result = instantiate_from_typeid(t, 3);
		)", {}
	);
	ut_compare_values(result, value_t::make_int(3));
}

QUARK_UNIT_TEST("run_global()", "", "", ""){
	const auto r = test__run_global(
		R"(
			a = instantiate_from_typeid(typeof(123), 3);
			assert(to_string(typeof(a)) == "int");
			assert(a == 3);
		)"
	);
}
*/


//////////////////////////////////////////		HOST FUNCTION - get_directory_entries()


QUARK_UNIT_TEST("", "get_directory_entries()", "", ""){
	const auto result = test__run_return_result(R"(

		let result = get_fsentries_shallow("/Users/marcus/Desktop/")
		assert(size(result) > 3)
		print(to_pretty_string(result))

	)", {});

	const auto expected = typeid_t::make_vector(make__fsentry_t__type());
	QUARK_UT_VERIFY(result.get_type() == expected);

//	quark::ut_compare_strings(vm->_print_output[0].substr(0, 7), "/Users/");
}


//////////////////////////////////////////		HOST FUNCTION - get_fsentries_deep()


QUARK_UNIT_TEST("", "get_fsentries_deep()", "", ""){
	const auto result = test__run_return_result(R"(

		let result = get_fsentries_deep("/Users/marcus/Desktop/")
		assert(size(result) > 1000)
		print(to_pretty_string(result))

	)", {});

	const auto expected = typeid_t::make_vector(make__fsentry_t__type());
	QUARK_UT_VERIFY(result.get_type() == expected);
}


//////////////////////////////////////////		HOST FUNCTION - get_fsentry_info()


QUARK_UNIT_TEST("", "get_fsentry_info()", "", ""){
	const auto result = test__run_return_result(R"(

		let result = get_fsentry_info("/Users/marcus/Desktop/")
		print(to_pretty_string(result))

	)", {});

	const auto expected = make__fsentry_info_t__type();
	QUARK_UT_VERIFY(result.get_type() == expected);
}


//////////////////////////////////////////		HOST FUNCTION - get_fs_environment()


QUARK_UNIT_TEST("", "get_fs_environment()", "", ""){
	const auto result = test__run_return_result(R"(

		let result = get_fs_environment()
		print(to_pretty_string(result))

	)", {});

	const auto expected = make__fs_environment_t__type();
	QUARK_UT_VERIFY(result.get_type() == expected);
}







//////////////////////////////////////////		HOST FUNCTION - does_fsentry_exist()


QUARK_UNIT_TEST("", "does_fsentry_exist()", "", ""){
	const auto result = test__run_return_result(R"(

		let path = get_fs_environment().desktop_dir
		let result = does_fsentry_exist(path)
		print(to_pretty_string(result))

		assert(result == true)

	)", {});

	QUARK_UT_VERIFY(result.get_type() == typeid_t::make_bool());
}

//??? Also test for files.
QUARK_UNIT_TEST("", "does_fsentry_exist()", "", ""){
	const auto result = test__run_return_result(R"(

		let path = get_fs_environment().desktop_dir + "xyz"
		let result = does_fsentry_exist(path)
		print(to_pretty_string(result))

		assert(result == false)

	)", {});

	QUARK_UT_VERIFY(result.get_type() == typeid_t::make_bool());
}

void remove_test_dir(const std::string& dir_name1, const std::string& dir_name2){
	const auto path1 = GetDirectories().desktop_dir + "/" + dir_name1;
	const auto path2 = path1 + "/" + dir_name2;

	if(DoesEntryExist(path1) == true){
		DeleteDeep(path1);
		QUARK_ASSERT(DoesEntryExist(path1) == false);
		QUARK_ASSERT(DoesEntryExist(path2) == false);
	}
}



//////////////////////////////////////////		HOST FUNCTION - create_directory_branch()


QUARK_UNIT_TEST("", "create_directory_branch()", "", ""){
	remove_test_dir("unittest___create_directory_branch", "subdir");

	const auto result = test__run_return_result(R"(

		let path1 = get_fs_environment().desktop_dir + "/unittest___create_directory_branch"
		let path2 = path1 + "/subdir"

		assert(does_fsentry_exist(path1) == false)
		assert(does_fsentry_exist(path2) == false)

		//	Make test.
		create_directory_branch(path2)
		assert(does_fsentry_exist(path1) == true)
		assert(does_fsentry_exist(path2) == true)

		delete_fsentry_deep(path1)
		assert(does_fsentry_exist(path1) == false)
		assert(does_fsentry_exist(path2) == false)

		let result = true

	)", {});

	QUARK_UT_VERIFY(result.get_type() == typeid_t::make_bool());
}


//////////////////////////////////////////		HOST FUNCTION - delete_fsentry_deep()


QUARK_UNIT_TEST("", "delete_fsentry_deep()", "", ""){
	remove_test_dir("unittest___delete_fsentry_deep", "subdir");

	const auto result = test__run_return_result(R"(

		let path1 = get_fs_environment().desktop_dir + "/unittest___delete_fsentry_deep"
		let path2 = path1 + "/subdir"

		assert(does_fsentry_exist(path1) == false)
		assert(does_fsentry_exist(path2) == false)

		create_directory_branch(path2)
		assert(does_fsentry_exist(path1) == true)
		assert(does_fsentry_exist(path2) == true)

		//	Make test.
		delete_fsentry_deep(path1)
		assert(does_fsentry_exist(path1) == false)
		assert(does_fsentry_exist(path2) == false)

		let result = true

	)", {});

	QUARK_UT_VERIFY(result.get_type() == typeid_t::make_bool());
}



//////////////////////////////////////////		HOST FUNCTION - rename_fsentry()


QUARK_UNIT_TEST("", "rename_fsentry()", "", ""){
	const auto result = test__run_return_result(R"(
		let dir = get_fs_environment().desktop_dir + "/unittest___rename_fsentry"

		delete_fsentry_deep(dir)

		let file_path1 = dir + "/original.txt"
		let file_path2 = dir + "/renamed.txt"

		assert(does_fsentry_exist(file_path1) == false)
		assert(does_fsentry_exist(file_path2) == false)

		//	Will also create parent directory.
		write_text_file(file_path1, "This is the file contents\n")

		assert(does_fsentry_exist(file_path1) == true)
		assert(does_fsentry_exist(file_path2) == false)

		rename_fsentry(file_path1, "renamed.txt")

		assert(does_fsentry_exist(file_path1) == false)
		assert(does_fsentry_exist(file_path2) == true)

		delete_fsentry_deep(dir)

		let result = true

	)", {});

	QUARK_UT_VERIFY(result.get_type() == typeid_t::make_bool());
}






///////////////////////////////////////////////////			EDGE CASES



//??? test accessing array->struct->array.
//??? test structs in vectors.


QUARK_UNIT_TEST("Edge case", "", "if with non-bool expression", "exception"){
	try{
		const auto result = test__run_return_result(R"(
			if("not a bool"){
			}
			else{
				assert(false)
			}
		)", {});
		QUARK_TEST_VERIFY(false);
	}
	catch(const std::runtime_error& e){
		QUARK_TEST_VERIFY(string(e.what()) == "Boolean condition required.");
	}
}

QUARK_UNIT_TEST("Edge case", "", "assign to immutable local", "exception"){
	try{
		const auto result = test__run_return_result(R"(
			let int a = 10
			let int a = 11
		)", {});
		QUARK_TEST_VERIFY(false);
	}
	catch(const std::runtime_error& e){
		QUARK_TEST_VERIFY(string(e.what()) == "Local identifier already exists.");
	}
}
QUARK_UNIT_TEST("Edge case", "", "Define struct with colliding name", "exception"){
	try{
		const auto result = test__run_return_result(R"(
			let int a = 10
			struct a { int x }
		)", {});
		QUARK_TEST_VERIFY(false);
	}
	catch(const std::runtime_error& e){
		QUARK_TEST_VERIFY(string(e.what()) == "Name already used.");
	}
}

QUARK_UNIT_TEST("Edge case", "", "Access unknown struct member", "exception"){
	try{
		const auto result = test__run_return_result(R"(
			struct a { int x }
			let b = a(13)
			print(b.y)
		)", {});
		QUARK_TEST_VERIFY(false);
	}
	catch(const std::runtime_error& e){
		QUARK_TEST_VERIFY(string(e.what()) == "Unknown struct member \"y\".");
	}
}

QUARK_UNIT_TEST("Edge case", "", "Access unknown member in non-struct", "exception"){
	try{
		const auto result = test__run_return_result(R"(
			let a = 10
			print(a.y)
		)", {});
		QUARK_TEST_VERIFY(false);
	}
	catch(const std::runtime_error& e){
		QUARK_TEST_VERIFY(string(e.what()) == "Parent is not a struct.");
	}
}

QUARK_UNIT_TEST("Edge case", "", "Lookup in string using non-int", "exception"){
	try{
		const auto result = test__run_return_result(R"(
			let string a = "test string"
			print(a["not an integer"])
		)", {});
		QUARK_TEST_VERIFY(false);
	}
	catch(const std::runtime_error& e){
		QUARK_TEST_VERIFY(string(e.what()) == "Lookup in string by index-only.");
	}
}

QUARK_UNIT_TEST("Edge case", "", "Lookup in vector using non-int", "exception"){
	try{
		const auto result = test__run_return_result(R"(
			let [string] a = ["one", "two", "three"]
			print(a["not an integer"])
		)", {});
		QUARK_TEST_VERIFY(false);
	}
	catch(const std::runtime_error& e){
		QUARK_TEST_VERIFY(string(e.what()) == "Lookup in vector by index-only.");
	}
}

QUARK_UNIT_TEST("Edge case", "", "Lookup in dict using non-string key", "exception"){
	try{
		const auto result = test__run_return_result(R"(
			let a = { "one": 1, "two": 2 }
			print(a[3])
		)", {});
		QUARK_TEST_VERIFY(false);
	}
	catch(const std::runtime_error& e){
		QUARK_TEST_VERIFY(string(e.what()) == "Lookup in dict by string-only.");
	}
}

QUARK_UNIT_TEST("Edge case", "", "Access undefined variable", "exception"){
	try{
		const auto result = test__run_return_result(R"(
			print(a)
		)", {});
		QUARK_TEST_VERIFY(false);
	}
	catch(const std::runtime_error& e){
		QUARK_TEST_VERIFY(string(e.what()) == "Undefined variable \"a\".");
	}
}

QUARK_UNIT_TEST("Edge case", "", "Wrong number of arguments in function call", "exception"){
	try{
		const auto result = test__run_return_result(R"(
			func int f(int a){ return a + 1 }
			let a = f(1, 2)
		)", {});
		QUARK_TEST_VERIFY(false);
	}
	catch(const std::runtime_error& e){
		QUARK_TEST_VERIFY(string(e.what()) == "Wrong number of arguments in function call.");
	}
}


QUARK_UNIT_TEST("Edge case", "", "Wrong number of arguments to struct-constructor", "exception"){
	try{
		const auto result = test__run_return_result(R"(
			struct pos { double x double y }
			let a = pos(3)
		)", {});
		QUARK_TEST_VERIFY(false);
	}
	catch(const std::runtime_error& e){
		QUARK_TEST_VERIFY(string(e.what()) == "Wrong number of arguments in function call.");
	}
}

QUARK_UNIT_TEST("Edge case", "", "Wrong TYPE of arguments to struct-constructor", "exception"){
	try{
		const auto result = test__run_return_result(R"(
			struct pos { double x double y }
			let a = pos(3, "hello")
		)", {});
		QUARK_TEST_VERIFY(false);
	}
	catch(const std::runtime_error& e){
		QUARK_TEST_VERIFY(string(e.what()) == "Expression type mismatch. Cannot convert 'int' to 'double.");
	}
}

QUARK_UNIT_TEST("Edge case", "", "Wrong number of arguments to int-constructor", "exception"){
	try{
		const auto result = test__run_return_result(R"(
			let a = int()
		)", {});
		QUARK_TEST_VERIFY(false);
	}
	catch(const std::runtime_error& e){
		QUARK_TEST_VERIFY(string(e.what()) == "Wrong number of arguments in function call.");
	}
}

QUARK_UNIT_TEST("Edge case", "", "Call non-function, non-struct, non-typeid", "exception"){
	try{
		const auto result = test__run_return_result(R"(
			let a = 3()
		)", {});
		QUARK_TEST_VERIFY(false);
	}
	catch(const std::runtime_error& e){
		QUARK_TEST_VERIFY(string(e.what()) == "Cannot call non-function.");
	}
}

//??? check dict too
QUARK_UNIT_TEST("Edge case", "", "Vector can not hold elements of different types.", "exception"){
	try{
		const auto result = test__run_return_result(R"(
			let a = [3, bool]
		)", {});
		QUARK_TEST_VERIFY(false);
	}
	catch(const std::runtime_error& e){
		QUARK_TEST_VERIFY(string(e.what()) == "Vector can not hold elements of different types.");
	}
}


QUARK_UNIT_TEST("Edge case", "", ".", "exception"){
	try{
		const auto result = test__run_return_result(R"(
			let a = 3 < "hello"
		)", {});
		QUARK_TEST_VERIFY(false);
	}
	catch(const std::runtime_error& e){
		QUARK_TEST_VERIFY(string(e.what()) == "Expression type mismatch. Cannot convert 'string' to 'int.");
	}
}

QUARK_UNIT_TEST("Edge case", "", ".", "exception"){
	try{
		const auto result = test__run_return_result(R"(
			let a = 3 * 3.2
		)", {});
		QUARK_TEST_VERIFY(false);
	}
	catch(const std::runtime_error& e){
		QUARK_TEST_VERIFY(string(e.what()) == "Expression type mismatch. Cannot convert 'double' to 'int.");
	}
}
QUARK_UNIT_TEST("Edge case", "Adding bools", ".", "success"){
	const auto result = test__run_return_result(R"(
		let result = false + true
	)", {});
	QUARK_ASSERT(result.get_bool_value() == true);
}
QUARK_UNIT_TEST("Edge case", "Adding bools", ".", "success"){
	const auto result = test__run_return_result(R"(
		let result = false + false
	)", {});
	QUARK_ASSERT(result.get_bool_value() == false);
}
QUARK_UNIT_TEST("Edge case", "Adding bools", ".", "success"){
	const auto result = test__run_return_result(R"(
		let result = true + true
	)", {});
	QUARK_ASSERT(result.get_bool_value() == true);
}

QUARK_UNIT_TEST("Edge case", "", "Lookup the unlookupable", "exception"){
	try{
		const auto result = test__run_return_result(R"(
			let a = 3[0]
		)", {});
		QUARK_TEST_VERIFY(false);
	}
	catch(const std::runtime_error& e){
		QUARK_TEST_VERIFY(string(e.what()) == "Lookup using [] only works with strings, vectors, dicts and json_value.");
	}
}


QUARK_UNIT_TEST("vector-int", "size()", "3", ""){
	const auto result = test__run_return_result(R"(
		let [int] a = [1, 2, 3]
		let result = size(a)
	)", {});
	ut_compare_values(result, value_t::make_int(3));
}
QUARK_UNIT_TEST("vector-int", "size()", "3", ""){
	const auto result = test__run_return_result(R"(
		let result = push_back([1, 2], 3)
		assert(result == [1, 2, 3])
	)", {});
}





OFF_QUARK_UNIT_TEST("Analyse all test programs", "", "", ""){
	int instruction_count_total = 0;
	int symbol_count_total = 0;

	for(const auto& s: program_recording){
		try{
		const auto bc = compile_to_bytecode(s);
		int instruction_count_sum = static_cast<int>(bc._globals._instrs2.size());
		int symbol_count_sum = static_cast<int>(bc._globals._symbols.size());

		for(const auto& f: bc._function_defs){
			if(f._frame_ptr != nullptr){
				const auto instruction_count = f._frame_ptr->_instrs2.size();
				const auto symbol_count = f._frame_ptr->_symbols.size();
				instruction_count_sum += instruction_count;
				symbol_count_sum += symbol_count;
			}
		}

		instruction_count_total += instruction_count_sum;
		symbol_count_total += symbol_count_sum;
		QUARK_TRACE_SS(instruction_count_sum << "\t" <<symbol_count_sum);
		}
		catch(...){
		}
	}

	QUARK_TRACE_SS("TOTAL: " << instruction_count_total << "\t" <<symbol_count_total);
}

















///////////////////////////////////////////////////////////////////////////////////////
//	FLOYD SYSTEMS TESTS
///////////////////////////////////////////////////////////////////////////////////////




QUARK_UNIT_TEST("software-system", "parse software-system", "", ""){
	const auto test_ss = R"(

		software-system {
			"name": "My Arcade Game",
			"desc": "Space shooter for mobile devices, with connection to a server.",

			"people": {
				"Gamer": "Plays the game on one of the mobile apps",
				"Curator": "Updates achievements, competitions, make custom on-off maps",
				"Admin": "Keeps the system running"
			},
			"connections": [
				{ "source": "Game", "dest": "iphone app", "interaction": "plays", "tech": "" }
			],
			"containers": [
				"gmail mail server",
				"iphone app",
				"Android app"
			]
		}
		result = 123

	)";

	const auto result = test__run_return_result(test_ss, {});
	ut_compare_values(result, value_t::make_int(123));
}

QUARK_UNIT_TEST("software-system", "run one process", "", ""){
	const auto test_ss2 = R"(
		software-system {
			"name": "My Arcade Game",
			"desc": "Space shooter for mobile devices, with connection to a server.",
			"people": {},
			"connections": [],
			"containers": [ "iphone app" ]
		}

		container-def {
			"name": "iphone app",
			"tech": "Swift, iOS, Xcode, Open GL",
			"desc": "Mobile shooter game for iOS.",
			"clocks": {
				"main": {
					"a": "my_gui"
				}
			}
		}

		struct my_gui_state_t {
			int _count
		}

		func my_gui_state_t my_gui__init() impure {
			send("a", "dec")
			send("a", "dec")
			send("a", "dec")
			send("a", "dec")
			send("a", "stop")
			return my_gui_state_t(1000)
		}

		func my_gui_state_t my_gui(my_gui_state_t state, json_value message){
			print("received: " + to_string(message) + ", state: " + to_string(state))

			if(message == "inc"){
				return update(state, "_count", state._count + 1)
			}
			else if(message == "dec"){
				return update(state, "_count", state._count - 1)
			}
			else{
				assert(false)
			}
		}
	)";

	program_recording.push_back(test_ss2);
	const auto result = run_container2(test_ss2, {}, "iphone app");
	QUARK_UT_VERIFY(result.empty());
}

QUARK_UNIT_TEST("software-system", "run two unconnected processs", "", ""){
	const auto test_ss3 = R"(
		software-system {
			"name": "My Arcade Game",
			"desc": "Space shooter for mobile devices, with connection to a server.",
			"people": {},
			"connections": [],
			"containers": [
				"iphone app"
			]
		}

		container-def {
			"name": "iphone app",
			"tech": "Swift, iOS, Xcode, Open GL",
			"desc": "Mobile shooter game for iOS.",
			"clocks": {
				"main": {
					"a": "my_gui",
					"b": "my_audio"
				}
			}
		}


		////////////////////////////////	my_gui -- process

		struct my_gui_state_t {
			int _count
		}

		func my_gui_state_t my_gui__init() impure {
			send("a", "dec")
			send("a", "dec")
			send("a", "dec")
			send("a", "stop")
			return my_gui_state_t(1000)
		}

		func my_gui_state_t my_gui(my_gui_state_t state, json_value message) impure {
			print("my_gui --- received: " + to_string(message) + ", state: " + to_string(state))

			if(message == "inc"){
				return update(state, "_count", state._count + 1)
			}
			else if(message == "dec"){
				return update(state, "_count", state._count - 1)
			}
			else{
				assert(false)
			}
		}


		////////////////////////////////	my_audio -- process

		struct my_audio_state_t {
			int _audio
		}

		func my_audio_state_t my_audio__init() impure {
			send("b", "process")
			send("b", "process")
			send("b", "stop")
			return my_audio_state_t(0)
		}

		func my_audio_state_t my_audio(my_audio_state_t state, json_value message) impure {
			print("my_audio --- received: " + to_string(message) + ", state: " + to_string(state))

			if(message == "process"){
				return update(state, "_audio", state._audio + 1)
			}
			else{
				assert(false)
			}
		}
	)";

	program_recording.push_back(test_ss3);
	const auto result = run_container2(test_ss3, {}, "iphone app");
	QUARK_UT_VERIFY(result.empty());
}

QUARK_UNIT_TEST("software-system", "run two CONNECTED processes", "", ""){
	const auto test_ss3 = R"(
		software-system {
			"name": "My Arcade Game",
			"desc": "Space shooter for mobile devices, with connection to a server.",
			"people": {},
			"connections": [],
			"containers": [
				"iphone app"
			]
		}

		container-def {
			"name": "iphone app",
			"tech": "Swift, iOS, Xcode, Open GL",
			"desc": "Mobile shooter game for iOS.",
			"clocks": {
				"main": {
					"a": "my_gui",
					"b": "my_audio",
				}
			}
		}


		////////////////////////////////	my_gui -- process

		struct my_gui_state_t {
			int _count
		}

		func my_gui_state_t my_gui__init() impure {
			return my_gui_state_t(1000)
		}

		func my_gui_state_t my_gui(my_gui_state_t state, json_value message) impure {
			print("my_gui --- received: " + to_string(message) + ", state: " + to_string(state))

			if(message == "2"){
				send("b", "3")
				return update(state, "_count", state._count + 1)
			}
			else if(message == "4"){
				send("a", "stop")
				send("b", "stop")
				return update(state, "_count", state._count + 10)
			}
			else{
				assert(false)
			}
		}


		////////////////////////////////	my_audio -- process

		struct my_audio_state_t {
			int _audio
		}

		func my_audio_state_t my_audio__init() impure {
			send("b", "1")
			return my_audio_state_t(0)
		}

		func my_audio_state_t my_audio(my_audio_state_t state, json_value message) impure {
			print("my_audio --- received: " + to_string(message) + ", state: " + to_string(state))

			if(message == "1"){
				send("a", "2")
				return update(state, "_audio", state._audio + 1)
			}
			else if(message == "3"){
				send("a", "4")
				return update(state, "_audio", state._audio + 4)
			}
			else{
				assert(false)
			}
		}
	)";

	program_recording.push_back(test_ss3);
	const auto result = run_container2(test_ss3, {}, "iphone app");
	QUARK_UT_VERIFY(result.empty());
}






///////////////////////////////////////////////////////////////////////////////////////
//	RUN ALL EXAMPLE PROGRAMS -- VERIFY THEY WORK
///////////////////////////////////////////////////////////////////////////////////////



QUARK_UNIT_TEST("", "hello_world.floyd", "", ""){
	const auto path = get_working_dir() + "/hello_world.floyd";
	const auto program = read_text_file(path);

	const auto result = run_container2(program, {}, "");
	const std::map<std::string, value_t> expected = {{ "global", value_t::make_void() }};
	QUARK_UT_VERIFY(result == expected);
}

QUARK_UNIT_TEST("", "game_of_life.floyd", "", ""){
	const auto path = get_working_dir() + "/game_of_life.floyd";
	const auto program = read_text_file(path);

	const auto result = run_container2(program, {}, "");
	const std::map<std::string, value_t> expected = {{ "global", value_t::make_void() }};
	QUARK_UT_VERIFY(result == expected);
}

QUARK_UNIT_TEST("", "process_test1.floyd", "", ""){
	const auto path = get_working_dir() + "/process_test1.floyd";
	const auto program = read_text_file(path);

	const auto result = run_container2(program, {}, "iphone app");
	QUARK_UT_VERIFY(result.empty());
}





///////////////////////////////////////////////////////////////////////////////////////
//	QUICK REFERENCE SNIPPETS -- VERIFY THEY WORK
///////////////////////////////////////////////////////////////////////////////////////




#define QUICK_REFERENCE_TEST	QUARK_UNIT_TEST


QUICK_REFERENCE_TEST("QUICK REFERENCE SNIPPETS", "TERNARY OPERATOR", "", ""){
	run_container2(R"(

//	Snippets setup
let b = ""



let a = b == "true" ? true : false

	)", {}, "");
}

QUICK_REFERENCE_TEST("QUICK REFERENCE SNIPPETS", "COMMENTS", "", ""){
	run_container2(R"(

/* Comment can span lines. */

let a = 10; // To end of line

	)", {}, "");
}



QUICK_REFERENCE_TEST("QUICK REFERENCE SNIPPETS", "LOCALS", "", ""){
	run_container2(R"(

let a = 10
mutable b = 10
b = 11

	)", {}, "");
}


QUICK_REFERENCE_TEST("QUICK REFERENCE SNIPPETS", "WHILE", "", ""){
	//	Just make sure it compiles, don't run it!
	compile_to_bytecode(R"(

//	Snippets setup
let expression = true


while(expression){
	print("body")
}

	)");
}


QUICK_REFERENCE_TEST("QUICK REFERENCE SNIPPETS", "FOR", "", ""){
	run_container2(R"(

for (index in 1 ... 5) {
	print(index)
}
for (index in 1  ..< 5) {
	print(index)
}

	)", {}, "");
}


QUICK_REFERENCE_TEST("QUICK REFERENCE SNIPPETS", "IF ELSE", "", ""){
	run_container2(R"(

//	Snippets setup
let a = 1000



if(a > 10){
	print("bigger")
}
else if(a > 0){
	print("small positive")
}
else{
	print("zero or negative")
}

	)", {}, "");
}


QUICK_REFERENCE_TEST("QUICK REFERENCE SNIPPETS", "BOOL", "", ""){
	run_container2(R"(

let a = true
if(a){
}

	)", {}, "");
}

QUICK_REFERENCE_TEST("QUICK REFERENCE SNIPPETS", "STRING", "", ""){
	run_container2(R"(

let s1 = "Hello, world!"
let s2 = "Title: " + s1
assert(s2 == "Title: Hello, world!")
assert(s1[0] == 72) // ASCII for 'H'
assert(size(s1) == 13)
assert(subset(s1, 1, 4) == "ell")
let s4 = to_string(12003)

	)", {}, "");
}




QUICK_REFERENCE_TEST("QUICK REFERENCE SNIPPETS", "FUNCTION", "", ""){
	run_container2(R"(

func string f(double a, string s){
	return to_string(a) + ":" + s
}

let a = f(3.14, "km")

	)", {}, "");
}

QUICK_REFERENCE_TEST("QUICK REFERENCE SNIPPETS", "IMPURE FUNCTION", "", ""){
	run_container2(R"(

func int main([string] args) impure {
	return 1
}

	)", {}, "");
}



QUICK_REFERENCE_TEST("QUICK REFERENCE SNIPPETS", "STRUCT", "", ""){
	run_container2(R"(

struct rect {
	double width
	double height
}

let a = rect(0.0, 3.0)
assert(a.width == 0.0
	&& a.height == 3.0)

assert(a == a)
assert(a == rect(0.0, 3.0))
assert(a != rect(1.0, 1.0))
assert(a < rect(0.0, 4.0))

let b = update(a, "width", 100.0)
assert(a.width == 0.0)
assert(b.width == 100.0)

	)", {}, "");
}

//??? demo map(), reduce()

QUICK_REFERENCE_TEST("QUICK REFERENCE SNIPPETS", "VECTOR", "", ""){
	run_container2(R"(

let a = [ 1, 2, 3 ]
assert(size(a) == 3)
assert(a[0] == 1)

let b = update(a, 1, 6)
assert(a == [ 1, 2, 3 ])
assert(b == [ 1, 6, 3 ])

let c = a + b
let d = push_back(a, 7)

let g = subset([ 4, 5, 6, 7, 8 ], 2, 4)
assert(g == [ 6, 7 ])

let h = replace([ 1, 2, 3, 4, 5 ], 1, 4, [ 8, 9 ])
assert(h == [ 1, 8, 9, 5 ])

for(i in 0 ..< size(a)){
	print(a[i])
}

	)", {}, "");
}


QUICK_REFERENCE_TEST("QUICK REFERENCE SNIPPETS", "DICTIONARY", "", ""){
	run_container2(R"(

let a = { "one": 1, "two": 2 }
assert(a["one"] == 1)

let b = update(a, "one", 10)
assert(a == { "one": 1, "two": 2 })
assert(b == { "one": 10, "two": 2 })

	)", {}, "");
}


QUICK_REFERENCE_TEST("QUICK REFERENCE SNIPPETS", "JSON_VALUE", "", ""){
	run_container2(R"(

let json_value a = {
	"one": 1,
	"three": "three",
	"five": { "A": 10, "B": 20 }
}

	)", {}, "");
}








///////////////////////////////////////////////////////////////////////////////////////
//	MANUAL SNIPPETS -- VERIFY THEY WORK
///////////////////////////////////////////////////////////////////////////////////////

#define MANUAL_SNIPPETS_TEST	QUARK_UNIT_TEST

MANUAL_SNIPPETS_TEST("MANUAL SNIPPETS", "subset()", "", ""){
	run_container2(R"(

		assert(subset("hello", 2, 4) == "ll")
		assert(subset([ 10, 20, 30, 40 ], 1, 3 ) == [ 20, 30 ])

	)", {}, "");
}

