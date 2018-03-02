//
//  floyd_test_suite.cpp
//  FloydSpeak
//
//  Created by Marcus Zetterquist on 2018-01-21.
//  Copyright © 2018 Marcus Zetterquist. All rights reserved.
//

#include "floyd_test_suite.hpp"


#include "floyd_interpreter.h"
#include "ast_value.h"
#include "expression.h"

#include <string>
#include <vector>
#include <iostream>



using std::vector;
using std::string;

using namespace floyd;


interpreter_context_t make_test_interpreter_context(){
	const auto t = quark::trace_context_t(true, quark::get_trace());
	interpreter_context_t context{ t };
	return context;
}


///////////////////////////////////////////////////////////////////////////////////////
//	FLOYD LANGUAGE TEST SUITE
///////////////////////////////////////////////////////////////////////////////////////


value_t run_return_result(const std::string& program, const std::vector<value_t>& args){
	const auto context = make_test_interpreter_context();

	const auto r = run_main(context, program, args);
	print_vm_printlog(r.first);
	const auto result = get_global(r.first, "result");
	return result;
}

void test__run_init__check_result(const std::string& program, const value_t& expected_result){
	const auto result = run_return_result(program, {});
	ut_compare_jsons(
		expression_to_json(expression_t::make_literal(result))._value,
		expression_to_json(expression_t::make_literal(expected_result))._value
	);
}
interpreter_t test__run_global(const std::string& program){
	const auto context = make_test_interpreter_context();

	const auto result = run_global(context, program);
	return result;
}


void test__run_main(const std::string& program, const vector<floyd::value_t>& args, const value_t& expected_return){
	const auto context = make_test_interpreter_context();

	const auto result = run_main(context, program, args);
	ut_compare_jsons(
		expression_to_json(expression_t::make_literal(result.second._output))._value,
		expression_to_json(expression_t::make_literal(expected_return))._value
	);
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
	test__run_init__check_result("int result = 123;", value_t::make_int(123));
}

QUARK_UNIT_TESTQ("Floyd test suite", "bool constant expression"){
	test__run_init__check_result("bool result = true;", value_t::make_bool(true));
}
QUARK_UNIT_TESTQ("Floyd test suite", "bool constant expression"){
	test__run_init__check_result("bool result = false;", value_t::make_bool(false));
}

QUARK_UNIT_TESTQ("Floyd test suite", "int constant expression"){
	test__run_init__check_result("int result = 123;", value_t::make_int(123));
}

QUARK_UNIT_TESTQ("Floyd test suite", "float constant expression"){
	test__run_init__check_result("float result = 3.5;", value_t::make_float(float(3.5)));
}

QUARK_UNIT_TESTQ("Floyd test suite", "string constant expression"){
	test__run_init__check_result("string result = \"xyz\";", value_t::make_string("xyz"));
}


//////////////////////////////////////////		BASIC EXPRESSIONS


QUARK_UNIT_TEST("Floyd test suite", "+", "", "") {
	test__run_init__check_result("int result = 1 + 2;", value_t::make_int(3));
}
QUARK_UNIT_TESTQ("Floyd test suite", "+") {
	test__run_init__check_result("int result = 1 + 2 + 3;", value_t::make_int(6));
}
QUARK_UNIT_TESTQ("Floyd test suite", "*") {
	test__run_init__check_result("int result = 3 * 4;", value_t::make_int(12));
}

QUARK_UNIT_TESTQ("Floyd test suite", "parant") {
	test__run_init__check_result("int result = (3 * 4) * 5;", value_t::make_int(60));
}

//??? test all types, like [int] etc.

QUARK_UNIT_TEST("Floyd test suite", "Expression statement", "", "") {
	const auto r = test__run_global("print(5);");
	QUARK_UT_VERIFY((r._print_output == vector<string>{ "5" }));
}

QUARK_UNIT_TESTQ("Floyd test suite", "Deduced bind") {
	const auto r = test__run_global("a = 10;print(a);");
	QUARK_UT_VERIFY((r._print_output == vector<string>{ "10" }));
}


//////////////////////////////////////////		BASIC EXPRESSIONS - CONDITIONAL EXPRESSION


QUARK_UNIT_TESTQ("run_main()", "conditional expression"){
	test__run_init__check_result("int result = true ? 1 : 2;", value_t::make_int(1));
}
QUARK_UNIT_TESTQ("run_main()", "conditional expression"){
	test__run_init__check_result("int result = false ? 1 : 2;", value_t::make_int(2));
}

//??? Test truthness off all variable types: strings, floats

QUARK_UNIT_TESTQ("run_main()", "conditional expression"){
	test__run_init__check_result("string result = true ? \"yes\" : \"no\";", value_t::make_string("yes"));
}
QUARK_UNIT_TESTQ("run_main()", "conditional expression"){
	test__run_init__check_result("string result = false ? \"yes\" : \"no\";", value_t::make_string("no"));
}


//////////////////////////////////////////		BASIC EXPRESSIONS - PARANTHESES



QUARK_UNIT_TESTQ("evaluate_expression()", "Parentheses") {
	test__run_init__check_result("int result = 5*(4+4+1);", value_t::make_int(45));
}
QUARK_UNIT_TESTQ("evaluate_expression()", "Parentheses") {
	test__run_init__check_result("int result = 5*(2*(1+3)+1);", value_t::make_int(45));
}
QUARK_UNIT_TESTQ("evaluate_expression()", "Parentheses") {
	test__run_init__check_result("int result = 5*((1+3)*2+1);", value_t::make_int(45));
}

QUARK_UNIT_TESTQ("evaluate_expression()", "Sign before parentheses") {
	test__run_init__check_result("int result = -(2+1)*4;", value_t::make_int(-12));
}
QUARK_UNIT_TESTQ("evaluate_expression()", "Sign before parentheses") {
	test__run_init__check_result("int result = -4*(2+1);", value_t::make_int(-12));
}


//////////////////////////////////////////		BASIC EXPRESSIONS - SPACES


QUARK_UNIT_TESTQ("evaluate_expression()", "Spaces") {
	test__run_init__check_result("int result = 5 * ((1 + 3) * 2 + 1) ;", value_t::make_int(45));
}
QUARK_UNIT_TESTQ("evaluate_expression()", "Spaces") {
	test__run_init__check_result("int result = 5 - 2 * ( 3 ) ;", value_t::make_int(-1));
}
QUARK_UNIT_TESTQ("evaluate_expression()", "Spaces") {
	test__run_init__check_result("int result =  5 - 2 * ( ( 4 )  - 1 ) ;", value_t::make_int(-1));
}


//////////////////////////////////////////		BASIC EXPRESSIONS - FLOAT



QUARK_UNIT_TESTQ("evaluate_expression()", "Fractional numbers") {
	test__run_init__check_result("float result = 5.5/5.0;", value_t::make_float(1.1f));
}
QUARK_UNIT_TESTQ("evaluate_expression()", "Fractional numbers") {
//	test__run_init__check_result("int result = 1/5e10") == 2e-11);
}
QUARK_UNIT_TESTQ("evaluate_expression()", "Fractional numbers") {
	test__run_init__check_result("float result = (4.0-3.0)/(4.0*4.0);", value_t::make_float(0.0625f));
}
QUARK_UNIT_TESTQ("evaluate_expression()", "Fractional numbers") {
	test__run_init__check_result("float result = 1.0/2.0/2.0;", value_t::make_float(0.25f));
}
QUARK_UNIT_TESTQ("evaluate_expression()", "Fractional numbers") {
	test__run_init__check_result("float result = 0.25 * .5 * 0.5;", value_t::make_float(0.0625f));
}
QUARK_UNIT_TESTQ("evaluate_expression()", "Fractional numbers") {
	test__run_init__check_result("float result = .25 / 2.0 * .5;", value_t::make_float(0.0625f));
}

//////////////////////////////////////////		BASIC EXPRESSIONS - EDGE CASES


QUARK_UNIT_TESTQ("evaluate_expression()", "Repeated operators") {
	test__run_init__check_result("int result = 1+-2;", value_t::make_int(-1));
}
QUARK_UNIT_TESTQ("evaluate_expression()", "Repeated operators") {
	test__run_init__check_result("int result = --2;", value_t::make_int(2));
}
QUARK_UNIT_TESTQ("evaluate_expression()", "Repeated operators") {
	test__run_init__check_result("int result = 2---2;", value_t::make_int(0));
}
QUARK_UNIT_TESTQ("evaluate_expression()", "Repeated operators") {
	test__run_init__check_result("int result = 2-+-2;", value_t::make_int(4));
}


//////////////////////////////////////////		BASIC EXPRESSIONS - BOOL


QUARK_UNIT_TESTQ("evaluate_expression()", "Bool") {
	test__run_init__check_result("bool result = true;", value_t::make_bool(true));
}
QUARK_UNIT_TESTQ("evaluate_expression()", "Bool") {
	test__run_init__check_result("bool result = false;", value_t::make_bool(false));
}


//////////////////////////////////////////		BASIC EXPRESSIONS - CONDITIONAL EXPRESSION



QUARK_UNIT_TESTQ("evaluate_expression()", "?:") {
	test__run_init__check_result("int result = true ? 4 : 6;", value_t::make_int(4));
}
QUARK_UNIT_TESTQ("evaluate_expression()", "?:") {
	test__run_init__check_result("int result = false ? 4 : 6;", value_t::make_int(6));
}

QUARK_UNIT_TESTQ("evaluate_expression()", "?:") {
	test__run_init__check_result("int result = 1==3 ? 4 : 6;", value_t::make_int(6));
}
QUARK_UNIT_TESTQ("evaluate_expression()", "?:") {
	test__run_init__check_result("int result = 3==3 ? 4 : 6;", value_t::make_int(4));
}

QUARK_UNIT_TESTQ("evaluate_expression()", "?:") {
	test__run_init__check_result("int result = 3==3 ? 2 + 2 : 2 * 3;", value_t::make_int(4));
}

QUARK_UNIT_TESTQ("evaluate_expression()", "?:") {
	test__run_init__check_result("int result = 3==1+2 ? 2 + 2 : 2 * 3;", value_t::make_int(4));
}


//////////////////////////////////////////		BASIC EXPRESSIONS - OPERATOR ==


QUARK_UNIT_TESTQ("evaluate_expression()", "==") {
	test__run_init__check_result("bool result = 1 == 1;", value_t::make_bool(true));
}
QUARK_UNIT_TESTQ("evaluate_expression()", "==") {
	test__run_init__check_result("bool result = 1 == 2;", value_t::make_bool(false));
}
QUARK_UNIT_TESTQ("evaluate_expression()", "==") {
	test__run_init__check_result("bool result = 1.3 == 1.3;", value_t::make_bool(true));
}
QUARK_UNIT_TESTQ("evaluate_expression()", "==") {
	test__run_init__check_result("bool result = \"hello\" == \"hello\";", value_t::make_bool(true));
}
QUARK_UNIT_TESTQ("evaluate_expression()", "==") {
	test__run_init__check_result("bool result = \"hello\" == \"bye\";", value_t::make_bool(false));
}


//////////////////////////////////////////		BASIC EXPRESSIONS - OPERATOR <


QUARK_UNIT_TESTQ("evaluate_expression()", "<") {
	test__run_init__check_result("bool result = 1 < 2;", value_t::make_bool(true));
}
QUARK_UNIT_TESTQ("evaluate_expression()", "<") {
	test__run_init__check_result("bool result = 5 < 2;", value_t::make_bool(false));
}
QUARK_UNIT_TESTQ("evaluate_expression()", "<") {
	test__run_init__check_result("bool result = 0.3 < 0.4;", value_t::make_bool(true));
}
QUARK_UNIT_TESTQ("evaluate_expression()", "<") {
	test__run_init__check_result("bool result = 1.5 < 0.4;", value_t::make_bool(false));
}
QUARK_UNIT_TESTQ("evaluate_expression()", "<") {
	test__run_init__check_result("bool result = \"adwark\" < \"boat\";", value_t::make_bool(true));
}
QUARK_UNIT_TESTQ("evaluate_expression()", "<") {
	test__run_init__check_result("bool result = \"boat\" < \"adwark\";", value_t::make_bool(false));
}

//////////////////////////////////////////		BASIC EXPRESSIONS - OPERATOR &&

QUARK_UNIT_TESTQ("evaluate_expression()", "&&") {
	test__run_init__check_result("bool result = false && false;", value_t::make_bool(false));
}
QUARK_UNIT_TESTQ("evaluate_expression()", "&&") {
	test__run_init__check_result("bool result = false && true;", value_t::make_bool(false));
}
QUARK_UNIT_TESTQ("evaluate_expression()", "&&") {
	test__run_init__check_result("bool result = true && false;", value_t::make_bool(false));
}
QUARK_UNIT_TESTQ("evaluate_expression()", "&&") {
	test__run_init__check_result("bool result = true && true;", value_t::make_bool(true));
}

QUARK_UNIT_TESTQ("evaluate_expression()", "&&") {
	test__run_init__check_result("bool result = false && false && false;", value_t::make_bool(false));
}
QUARK_UNIT_TESTQ("evaluate_expression()", "&&") {
	test__run_init__check_result("bool result = false && false && true;", value_t::make_bool(false));
}
QUARK_UNIT_TESTQ("evaluate_expression()", "&&") {
	test__run_init__check_result("bool result = false && true && false;", value_t::make_bool(false));
}
QUARK_UNIT_TESTQ("evaluate_expression()", "&&") {
	test__run_init__check_result("bool result = false && true && true;", value_t::make_bool(false));
}
QUARK_UNIT_TESTQ("evaluate_expression()", "&&") {
	test__run_init__check_result("bool result = true && false && false;", value_t::make_bool(false));
}
QUARK_UNIT_TESTQ("evaluate_expression()", "&&") {
	test__run_init__check_result("bool result = true && true && false;", value_t::make_bool(false));
}
QUARK_UNIT_TESTQ("evaluate_expression()", "&&") {
	test__run_init__check_result("bool result = true && true && true;", value_t::make_bool(true));
}

//////////////////////////////////////////		BASIC EXPRESSIONS - OPERATOR ||

QUARK_UNIT_TESTQ("evaluate_expression()", "||") {
	test__run_init__check_result("bool result = false || false;", value_t::make_bool(false));
}
QUARK_UNIT_TESTQ("evaluate_expression()", "||") {
	test__run_init__check_result("bool result = false || true;", value_t::make_bool(true));
}
QUARK_UNIT_TESTQ("evaluate_expression()", "||") {
	test__run_init__check_result("bool result = true || false;", value_t::make_bool(true));
}
QUARK_UNIT_TESTQ("evaluate_expression()", "||") {
	test__run_init__check_result("bool result = true || true;", value_t::make_bool(true));
}


QUARK_UNIT_TESTQ("evaluate_expression()", "||") {
	test__run_init__check_result("bool result = false || false || false;", value_t::make_bool(false));
}
QUARK_UNIT_TESTQ("evaluate_expression()", "||") {
	test__run_init__check_result("bool result = false || false || true;", value_t::make_bool(true));
}
QUARK_UNIT_TESTQ("evaluate_expression()", "||") {
	test__run_init__check_result("bool result = false || true || false;", value_t::make_bool(true));
}
QUARK_UNIT_TESTQ("evaluate_expression()", "||") {
	test__run_init__check_result("bool result = false || true || true;", value_t::make_bool(true));
}
QUARK_UNIT_TESTQ("evaluate_expression()", "||") {
	test__run_init__check_result("bool result = true || false || false;", value_t::make_bool(true));
}
QUARK_UNIT_TESTQ("evaluate_expression()", "||") {
	test__run_init__check_result("bool result = true || false || true;", value_t::make_bool(true));
}
QUARK_UNIT_TESTQ("evaluate_expression()", "||") {
	test__run_init__check_result("bool result = true || true || false;", value_t::make_bool(true));
}
QUARK_UNIT_TESTQ("evaluate_expression()", "||") {
	test__run_init__check_result("bool result = true || true || true;", value_t::make_bool(true));
}


//////////////////////////////////////////		BASIC EXPRESSIONS - ERRORS


QUARK_UNIT_TEST("evaluate_expression()", "Type mismatch", "", "") {
	try{
		test__run_init__check_result("int result = true;", value_t::make_int(1));
		QUARK_TEST_VERIFY(false);
	}
	catch(const std::runtime_error& e){
		QUARK_TEST_VERIFY(string(e.what()) == "Expression type mismatch.");
	}
}


QUARK_UNIT_TESTQ("evaluate_expression()", "Division by zero") {
	try{
		test__run_init__check_result("int result = 2/0;", value_t::make_null());
		QUARK_TEST_VERIFY(false);
	}
	catch(const std::runtime_error& e){
		QUARK_TEST_VERIFY(string(e.what()) == "EEE_DIVIDE_BY_ZERO");
	}
}

QUARK_UNIT_TESTQ("evaluate_expression()", "Division by zero"){
	try{
		test__run_init__check_result("int result = 3+1/(5-5)+4;", value_t::make_null());
		QUARK_TEST_VERIFY(false);
	}
	catch(const std::runtime_error& e){
		QUARK_TEST_VERIFY(string(e.what()) == "EEE_DIVIDE_BY_ZERO");
	}
}

#if false

QUARK_UNIT_TESTQ("evaluate_expression()", "Errors") {
		//	Multiple errors not possible/relevant now that we use exceptions for errors.
/*
	//////////////////////////		Only one error will be detected (in this case, the last one)
	QUARK_TEST_VERIFY(test__evaluate_expression("3+1/0+4$") == EEE_WRONG_CHAR);

	QUARK_TEST_VERIFY(test__evaluate_expression("3+1/0+4") == EEE_DIVIDE_BY_ZERO);

	// ...or the first one
	QUARK_TEST_VERIFY(test__evaluate_expression("q+1/0)") == EEE_WRONG_CHAR);
	QUARK_TEST_VERIFY(test__evaluate_expression("+1/0)") == EEE_PARENTHESES);
	QUARK_TEST_VERIFY(test__evaluate_expression("+1/0") == EEE_DIVIDE_BY_ZERO);
*/
}

#endif








//////////////////////////////////////////		MINIMAL PROGRAMS



QUARK_UNIT_TESTQ("run_main", "Can make and read global int"){
	test__run_main(
		"int test = 123;"
		"int main(){\n"
		"	return test;"
		"}\n",
		{},
		value_t::make_int(123)
	);
}

QUARK_UNIT_TESTQ("run_main()", "minimal program 2"){
	test__run_main(
		"string main(string args){\n"
		"	return \"123\" + \"456\";\n"
		"}\n",
		vector<value_t>{value_t::make_string("program_name 1 2 3 4")},
		value_t::make_string("123456")
	);
}

QUARK_UNIT_TESTQ("run_main()", ""){
	test__run_main("bool main(){ return 4 < 5; }", {}, value_t::make_bool(true));
}
QUARK_UNIT_TESTQ("run_main()", ""){
	test__run_main("bool main(){ return 5 < 4; }", {}, value_t::make_bool(false));
}
QUARK_UNIT_TESTQ("run_main()", ""){
	test__run_main("bool main(){ return 4 <= 4; }", {}, value_t::make_bool(true));
}

QUARK_UNIT_TESTQ("call_function()", "minimal program"){
	const auto context = make_test_interpreter_context();
	auto ast = program_to_ast2(context,
		"int main(string args){\n"
		"	return 3 + 4;\n"
		"}\n"
	);
	auto vm = interpreter_t(ast);
	const auto f = find_global_symbol(vm, "main");
	const auto result = call_function(vm, f, vector<value_t>{ value_t::make_string("program_name 1 2 3") });
	QUARK_TEST_VERIFY(result.second == statement_result_t::make_return_unwind(value_t::make_int(7)));
}

QUARK_UNIT_TESTQ("call_function()", "minimal program 2"){
	const auto context = make_test_interpreter_context();
	auto ast = program_to_ast2(context,
		"string main(string args){\n"
		"	return \"123\" + \"456\";\n"
		"}\n"
	);
	auto vm = interpreter_t(ast);
	const auto f = find_global_symbol(vm, "main");
	const auto result = call_function(vm, f, vector<value_t>{ value_t::make_string("program_name 1 2 3") });
	QUARK_TEST_VERIFY(result.second == statement_result_t::make_return_unwind(value_t::make_string("123456")));
}



//////////////////////////////////////////		TEST CONSTRUCTOR FOR ALL TYPES


QUARK_UNIT_TEST("", "bool()", "", ""){
	test__run_init__check_result("result = bool(false);", value_t::make_bool(false));
}
QUARK_UNIT_TEST("", "bool()", "", ""){
	test__run_init__check_result("result = bool(true);", value_t::make_bool(true));
}
QUARK_UNIT_TEST("", "int()", "", ""){
	test__run_init__check_result("result = int(123);", value_t::make_int(123));
}
QUARK_UNIT_TEST("", "float()", "", ""){
	test__run_init__check_result("result = float(0.0);", value_t::make_float(0.0));
}
QUARK_UNIT_TEST("", "float()", "", ""){
	test__run_init__check_result("result = float(123.456);", value_t::make_float(123.456f));
}

QUARK_UNIT_TEST("", "string()", "", ""){
	try{
		test__run_init__check_result("result = string();", value_t::make_string(""));
		QUARK_UT_VERIFY(false);
	}
	catch(const std::runtime_error& e){
		QUARK_TEST_VERIFY(string(e.what()) == "Wrong number of arguments in function call.");
	}
}

QUARK_UNIT_TEST("", "string()", "", ""){
	test__run_init__check_result("result = string(\"ABCD\");", value_t::make_string("ABCD"));
}

QUARK_UNIT_TEST("", "json_value()", "", ""){
	test__run_init__check_result("result = json_value(123);", value_t::make_json_value(json_t(123.0)));
}
QUARK_UNIT_TEST("", "json_value()", "", ""){
	test__run_init__check_result("result = json_value(\"hello\");", value_t::make_json_value(json_t("hello")));
}
QUARK_UNIT_TEST("", "json_value()", "", ""){
	test__run_init__check_result("result = json_value([1,2,3]);", value_t::make_json_value(json_t::make_array({1,2,3})));
}
QUARK_UNIT_TEST("", "pixel_t()", "", ""){
	const auto pixel_t__def = std::make_shared<floyd::struct_definition_t>(
		std::vector<member_t>{
			member_t(typeid_t::make_int(), "red"),
			member_t(typeid_t::make_int(), "green"),
			member_t(typeid_t::make_int(), "blue")
		}
	);

	test__run_init__check_result(
		"struct pixel_t { int red; int green; int blue; } result = pixel_t(1,2,3);",
		value_t::make_struct_value(typeid_t::make_struct(pixel_t__def), vector<value_t>{value_t::make_int(1), value_t::make_int(2), value_t::make_int(3)})
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



QUARK_UNIT_TESTQ("call_function()", "define additional function, call it several times"){
	const auto context = make_test_interpreter_context();
	auto ast = program_to_ast2(context,
		"int myfunc(){ return 5; }\n"
		"int main(string args){\n"
		"	return myfunc() + myfunc() * 2;\n"
		"}\n"
	);
	auto vm = interpreter_t(ast);
	const auto f = find_global_symbol(vm, "main");
	const auto result = call_function(vm, f, vector<value_t>{ value_t::make_string("program_name 1 2 3") });
	QUARK_TEST_VERIFY(result.second == statement_result_t::make_return_unwind(value_t::make_int(15)));
}

QUARK_UNIT_TESTQ("call_function()", "use function inputs"){
	const auto context = make_test_interpreter_context();
	auto ast = program_to_ast2(context,
		"string main(string args){\n"
		"	return \"-\" + args + \"-\";\n"
		"}\n"
	);
	auto vm = interpreter_t(ast);
	const auto f = find_global_symbol(vm, "main");
	const auto result = call_function(vm, f, vector<value_t>{ value_t::make_string("xyz") });
	QUARK_TEST_VERIFY(result.second == statement_result_t::make_return_unwind(value_t::make_string("-xyz-")));

	const auto result2 = call_function(vm, f, vector<value_t>{ value_t::make_string("Hello, world!") });
	QUARK_TEST_VERIFY(result2.second == statement_result_t::make_return_unwind(value_t::make_string("-Hello, world!-")));
}


//////////////////////////////////////////		USE LOCAL VARIABLES IN FUNCTION


QUARK_UNIT_TESTQ("call_function()", "use local variables"){
	const auto context = make_test_interpreter_context();
	auto ast = program_to_ast2(context,
		"string myfunc(string t){ return \"<\" + t + \">\"; }\n"
		"string main(string args){\n"
		"	 string a = \"--\"; string b = myfunc(args) ; return a + args + b + a;\n"
		"}\n"
	);
	auto vm = interpreter_t(ast);
	const auto f = find_global_symbol(vm, "main");
	const auto result = call_function(vm, f, vector<value_t>{ value_t::make_string("xyz") });
//	QUARK_TEST_VERIFY(*result.second == value_t::make_string("--xyz<xyz>--"));
	QUARK_TEST_VERIFY(result.second == statement_result_t::make_return_unwind(value_t::make_string("--xyz<xyz>--")));

	const auto result2 = call_function(vm, f, vector<value_t>{ value_t::make_string("123") });
//	QUARK_TEST_VERIFY(*result2.second == value_t::make_string("--123<123>--"));
	QUARK_TEST_VERIFY(result2.second == statement_result_t::make_return_unwind(value_t::make_string("--123<123>--")));
}



//////////////////////////////////////////		MUTATE VARIABLES



QUARK_UNIT_TESTQ("call_function()", "mutate local"){
	auto r = test__run_global(
		R"(
			mutable a = 1;
			a = 2;
			print(a);
		)"
	);
	QUARK_UT_VERIFY((r._print_output == vector<string>{ "2" }));
}

QUARK_UNIT_TESTQ("run_init()", "increment a mutable"){
	const auto r = test__run_global(
		R"(
			mutable a = 1000;
			a = a + 1;
			print(a);
		)"
	);
	QUARK_UT_VERIFY((r._print_output == vector<string>{ "1001" }));
}

QUARK_UNIT_TESTQ("run_main()", "test locals are immutable"){
	try {
		const auto vm = test__run_global(R"(
			a = 3;
			a = 4;
		)");
		QUARK_UT_VERIFY(false);
	}
	catch(...){
	}
}

QUARK_UNIT_TESTQ("run_main()", "test function args are always immutable"){
	try {
		const auto vm = test__run_global(R"(
			int f(int x){
				x = 6;
			}
			f(5);
		)");
		QUARK_UT_VERIFY(false);
	}
	catch(...){
	}
}

		QUARK_UNIT_TEST("run_main()", "test mutating from a subscope", "", ""){
			const auto r = test__run_global(R"(
				mutable a = 7;
				a = 8;
				{
					print(a);
				}
			)");
			QUARK_UT_VERIFY((r._print_output == vector<string>{ "8" }));
		}

		QUARK_UNIT_TEST("run_main()", "test mutating from a subscope", "", ""){
			const auto r = test__run_global(R"(
				a = 7;
				print(a);
			)");
			QUARK_UT_VERIFY((r._print_output == vector<string>{ "7" }));
		}


		QUARK_UNIT_TEST("run_main()", "test mutating from a subscope", "", ""){
			const auto r = test__run_global(R"(
				a = 7;
				{
				}
				print(a);
			)");
			QUARK_UT_VERIFY((r._print_output == vector<string>{ "7" }));
		}

QUARK_UNIT_TEST("run_main()", "test mutating from a subscope", "", ""){
	const auto r = test__run_global(R"(
		mutable a = 7;
		{
			a = 8;
		}
		print(a);
	)");
	QUARK_UT_VERIFY((r._print_output == vector<string>{ "8" }));
}



//////////////////////////////////////////		RETURN STATEMENT - ADVANCED USAGE


QUARK_UNIT_TEST("call_function()", "return from middle of function", "", ""){
	auto r = test__run_global(
		"string f(){"
		"	print(\"A\");"
		"	return \"B\";"
		"	print(\"C\");"
		"	return \"D\";"
		"}"
		"string x = f();"
		"print(x);"
	);
	QUARK_UT_VERIFY((r._print_output == vector<string>{ "A", "B" }));
}

QUARK_UNIT_TESTQ("call_function()", "return from within IF block"){
	auto r = test__run_global(
		"string f(){"
		"	if(true){"
		"		print(\"A\");"
		"		return \"B\";"
		"		print(\"C\");"
		"	}"
		"	print(\"D\");"
		"	return \"E\";"
		"}"
		"string x = f();"
		"print(x);"
	);
	QUARK_UT_VERIFY((r._print_output == vector<string>{ "A", "B" }));
}

QUARK_UNIT_TESTQ("call_function()", "return from within FOR block"){
	auto r = test__run_global(
		"string f(){"
		"	for(e in 0...3){"
		"		print(\"A\");"
		"		return \"B\";"
		"		print(\"C\");"
		"	}"
		"	print(\"D\");"
		"	return \"E\";"
		"}"
		"string x = f();"
		"print(x);"
	);
	QUARK_UT_VERIFY((r._print_output == vector<string>{ "A", "B" }));
}

// ??? add test for: return from ELSE

QUARK_UNIT_TESTQ("call_function()", "return from within BLOCK"){
	auto r = test__run_global(
		"string f(){"
		"	{"
		"		print(\"A\");"
		"		return \"B\";"
		"		print(\"C\");"
		"	}"
		"	print(\"D\");"
		"	return \"E\";"
		"}"
		"string x = f();"
		"print(x);"
	);
	QUARK_UT_VERIFY((r._print_output == vector<string>{ "A", "B" }));
}






//////////////////////////////////////////		HOST FUNCTION - to_string()



QUARK_UNIT_TESTQ("run_init()", ""){
	test__run_init__check_result(
		R"(
			string result = to_string(145);
		)",
		value_t::make_string("145")
	);
}
QUARK_UNIT_TESTQ("run_init()", ""){
	test__run_init__check_result(
		R"(
			string result = to_string(3.1);
		)",
		value_t::make_string("3.1")
	);
}

QUARK_UNIT_TESTQ("run_init()", ""){
	test__run_init__check_result(
		R"(
			string result = to_string(3.0);
		)",
		value_t::make_string("3")
	);
}



//////////////////////////////////////////		HOST FUNCTION - typeof()



QUARK_UNIT_TEST("", "typeof()", "", ""){
	const auto result = run_return_result(
		R"(
			result = typeof(145);
		)", {}
	);
	ut_compare_values(result, value_t::make_typeid_value(typeid_t::make_int()));
}
QUARK_UNIT_TEST("", "typeof()", "", ""){
	const auto result = run_return_result(
		R"(
			result = to_string(typeof(145));
		)", {}
	);
	ut_compare_values(result, value_t::make_string("int"));
}

QUARK_UNIT_TEST("", "typeof()", "", ""){
	const auto result = run_return_result(
		R"(
			result = typeof("hello");
		)", {}
	);
	ut_compare_values(result, value_t::make_typeid_value(typeid_t::make_string()));
}

QUARK_UNIT_TEST("", "typeof()", "", ""){
	const auto result = run_return_result(
		R"(
			result = to_string(typeof("hello"));
		)", {}
	);
	ut_compare_values(result, value_t::make_string("string"));
}

QUARK_UNIT_TEST("", "typeof()", "", ""){
	const auto result = run_return_result(
		R"(
			result = typeof([1,2,3]);
		)", {}
	);
	ut_compare_values(result, value_t::make_typeid_value(typeid_t::make_vector(typeid_t::make_int())));
}
QUARK_UNIT_TEST("", "typeof()", "", ""){
	const auto result = run_return_result(
		R"(
			result = to_string(typeof([1,2,3]));
		)", {}
	);
	ut_compare_values(result, value_t::make_string("[int]"));
}



//////////////////////////////////////////		HOST FUNCTION - print()



QUARK_UNIT_TEST("run_global()", "Print Hello, world!", "", ""){
	const auto r = test__run_global(
		R"(
			print("Hello, World!");
		)"
	);
	QUARK_UT_VERIFY((r._print_output == vector<string>{ "Hello, World!" }));
}


QUARK_UNIT_TEST("run_global()", "Test that VM state (print-log) escapes block!", "", ""){
	const auto r = test__run_global(
		R"(
			{
				print("Hello, World!");
			}
		)"
	);
	QUARK_UT_VERIFY((r._print_output == vector<string>{ "Hello, World!" }));
}

QUARK_UNIT_TESTQ("run_global()", "Test that VM state (print-log) escapes IF!"){
	const auto r = test__run_global(
		R"(
			if(true){
				print("Hello, World!");
			}
		)"
	);
	QUARK_UT_VERIFY((r._print_output == vector<string>{ "Hello, World!" }));
}


//////////////////////////////////////////		HOST FUNCTION - assert()

QUARK_UNIT_TESTQ("run_global()", ""){
	try{
		const auto r = test__run_global(
			R"(
				assert(1 == 2);
			)"
		);
		QUARK_UT_VERIFY(false);
	}
	catch(...){
//		QUARK_UT_VERIFY((r._print_output == vector<string>{ "Assertion failed.", "A" }));
	}
}

QUARK_UNIT_TESTQ("run_global()", ""){
	const auto r = test__run_global(
		R"(
			assert(1 == 1);
			print("A");
		)"
	);
	QUARK_UT_VERIFY((r._print_output == vector<string>{ "A" }));
}


//////////////////////////////////////////		HOST FUNCTION - get_time_of_day()



QUARK_UNIT_TESTQ("run_init()", "get_time_of_day()"){
	test__run_global(
		R"(
			int a = get_time_of_day();
			int b = get_time_of_day();
			int c = b - a;
			print("Delta time:" + to_string(a));
		)"
	);
}


//////////////////////////////////////////		HOST FUNCTION - get_env_path()


QUARK_UNIT_TEST("", "get_env_path()", "", ""){
	const auto vm = test__run_global(R"(
		path = get_env_path();
		print(path);
	)");

	quark::ut_compare_strings(vm._print_output[0].substr(0, 7), "/Users/");
}

//////////////////////////////////////////		HOST FUNCTION - read_text_file()

/*
QUARK_UNIT_TEST("", "read_text_file()", "", ""){
	const auto vm = test__run_global(R"(
		path = get_env_path();
		a = read_text_file(path + "/Desktop/test1.json");
		print(a);
	)");
	ut_compare_stringvects(vm._print_output, vector<string>{
		string() + R"({ "magic": 1234 })" + "\n"
	});
}
*/


//////////////////////////////////////////		HOST FUNCTION - write_text_file()


QUARK_UNIT_TEST("", "write_text_file()", "", ""){
	const auto vm = test__run_global(R"(
		path = get_env_path();
		write_text_file(path + "/Desktop/test_out.txt", "Floyd wrote this!");
	)");
}




//////////////////////////////////////////		BLOCKS AND SCOPING



QUARK_UNIT_TESTQ("run_init()", "Empty block"){
	test__run_global(
		"{}"
	);
}

QUARK_UNIT_TESTQ("run_init()", "Block with local variable, no shadowing"){
	test__run_global(
		"{ int x = 4; }"
	);
}

QUARK_UNIT_TESTQ("run_init()", "Block with local variable, no shadowing"){
	const auto r = test__run_global(
		R"(
			int x = 3;
			print("B:" + to_string(x));
			{
				print("C:" + to_string(x));
				int x = 4;
				print("D:" + to_string(x));
			}
			print("E:" + to_string(x));
		)"
	);
	QUARK_UT_VERIFY((r._print_output == vector<string>{ "B:3", "C:3", "D:4", "E:3" }));
}




//////////////////////////////////////////		IF STATEMENT



QUARK_UNIT_TESTQ("run_init()", "if(true){}"){
	const auto r = test__run_global(
		R"(
			if(true){
				print("Hello!");
			}
			print("Goodbye!");
		)"
	);
	QUARK_UT_VERIFY((r._print_output == vector<string>{ "Hello!", "Goodbye!" }));
}

QUARK_UNIT_TESTQ("run_init()", "if(false){}"){
	const auto r = test__run_global(
		R"(
			if(false){
				print("Hello!");
			}
			print("Goodbye!");
		)"
	);
	QUARK_UT_VERIFY((r._print_output == vector<string>{ "Goodbye!" }));
}

QUARK_UNIT_TESTQ("run_init()", "if(true){}else{}"){
	const auto r = test__run_global(
		R"(
			if(true){
				print("Hello!");
			}
			else{
				print("Goodbye!");
			}
		)"
	);
	QUARK_UT_VERIFY((r._print_output == vector<string>{ "Hello!" }));
}

QUARK_UNIT_TESTQ("run_init()", "if(false){}else{}"){
	const auto r = test__run_global(
		R"(
			if(false){
				print("Hello!");
			}
			else{
				print("Goodbye!");
			}
		)"
	);
	QUARK_UT_VERIFY((r._print_output == vector<string>{ "Goodbye!" }));
}

QUARK_UNIT_TESTQ("run_init()", "if"){
	const auto r = test__run_global(
		R"(
			if(1 == 1){
				print("one");
			}
			else if(2 == 0){
				print("two");
			}
			else if(3 == 0){
				print("three");
			}
			else{
				print("four");
			}
		)"
	);
	QUARK_UT_VERIFY((r._print_output == vector<string>{ "one" }));
}

QUARK_UNIT_TESTQ("run_init()", "if"){
	const auto r = test__run_global(
		R"(
			if(1 == 0){
				print("one");
			}
			else if(2 == 2){
				print("two");
			}
			else if(3 == 0){
				print("three");
			}
			else{
				print("four");
			}
		)"
	);
	QUARK_UT_VERIFY((r._print_output == vector<string>{ "two" }));
}

QUARK_UNIT_TESTQ("run_init()", "if"){
	const auto r = test__run_global(
		R"(
			if(1 == 0){
				print("one");
			}
			else if(2 == 0){
				print("two");
			}
			else if(3 == 3){
				print("three");
			}
			else{
				print("four");
			}
		)"
	);
	QUARK_UT_VERIFY((r._print_output == vector<string>{ "three" }));
}

QUARK_UNIT_TESTQ("run_init()", "if"){
	const auto r = test__run_global(
		R"(
			if(1 == 0){
				print("one");
			}
			else if(2 == 0){
				print("two");
			}
			else if(3 == 0){
				print("three");
			}
			else{
				print("four");
			}
		)"
	);
	QUARK_UT_VERIFY((r._print_output == vector<string>{ "four" }));
}




//////////////////////////////////////////		FOR STATEMENT




QUARK_UNIT_TESTQ("run_init()", "for"){
	const auto r = test__run_global(
		R"(
			for (i in 0...2) {
				print("Iteration: " + to_string(i));
			}
		)"
	);
	QUARK_UT_VERIFY((r._print_output == vector<string>{ "Iteration: 0", "Iteration: 1", "Iteration: 2" }));
}

OFF_QUARK_UNIT_TEST("run_init()", "fibonacci", "", ""){
	const auto vm = test__run_global(
		"int fibonacci(int n) {"
		"	if (n <= 1){"
		"		return n;"
		"	}"
		"	return fibonacci(n - 2) + fibonacci(n - 1);"
		"}"

		"for (i in 0...10) {"
		"	print(fibonacci(i));"
		"}"
	);

	QUARK_UT_VERIFY((
		vm._print_output == vector<string>{
			"0", "1", "1", "2", "3", "5", "8", "13", "21", "34",
			"55" //, "89", "144", "233", "377", "610", "987", "1597", "2584", "4181"
		})
	);
}


//////////////////////////////////////////		WHILE STATEMENT


QUARK_UNIT_TESTQ("run_init()", "for"){
	const auto r = test__run_global(
		R"(
			mutable a = 100;
			while(a < 105){
				print(to_string(a));
				a = a + 1;
			}
		)"
	);
	QUARK_UT_VERIFY((r._print_output == vector<string>{ "100", "101", "102", "103", "104" }));
}


//////////////////////////////////////////		TYPEID - TYPE

//	??? Test converting different types to jsons




//////////////////////////////////////////		NULL - TYPE




QUARK_UNIT_TEST("null", "", "", "0"){
	try{
		const auto result = run_return_result(R"(
			result = null;
		)", {});
//		QUARK_UT_VERIFY(false);
	}
	catch(const std::runtime_error& e){
		QUARK_UT_VERIFY(string(e.what()) == "Types not compatible in bind.");
	}
}


//////////////////////////////////////////		STRING - TYPE



//??? add tests for equality.

QUARK_UNIT_TEST("string", "size()", "string", "0"){
	const auto vm = test__run_global(R"(
		assert(size("") == 0);
	)");
}
QUARK_UNIT_TEST("string", "size()", "string", "24"){
	const auto vm = test__run_global(R"(
		assert(size("How long is this string?") == 24);
	)");
}

QUARK_UNIT_TEST("string", "push_back()", "string", "correct final vector"){
	const auto vm = test__run_global(R"(
		a = push_back("one", "two");
		assert(a == "onetwo");
	)");
}

QUARK_UNIT_TEST("string", "update()", "string", "correct final string"){
	const auto vm = test__run_global(R"(
		a = update("hello", 1, "z");
		assert(a == "hzllo");
	)");
}


QUARK_UNIT_TEST("string", "subset()", "string", "correct final vector"){
	const auto vm = test__run_global(R"(
		assert(subset("abc", 0, 3) == "abc");
		assert(subset("abc", 1, 3) == "bc");
		assert(subset("abc", 0, 0) == "");
	)");
}

QUARK_UNIT_TEST("vector", "replace()", "combo", ""){
	const auto vm = test__run_global(R"(
		assert(replace("One ring to rule them all", 4, 8, "rabbit") == "One rabbit to rule them all");
	)");
}
// ### test pos limiting and edge cases.



//////////////////////////////////////////		VECTOR - TYPE


QUARK_UNIT_TEST("vector", "[]-constructor, inplicit type", "strings", "valid vector"){
	const auto vm = test__run_global(R"(
		a = ["one", "two"];
		print(a);
	)");
	ut_compare_stringvects(vm._print_output, vector<string>{
		R"(["one", "two"])"
	});
}

QUARK_UNIT_TEST("vector", "[]-constructor, implicit type", "cannot be deducted", "error"){
	try{
		const auto vm = test__run_global(R"(
			a = [];
			print(a);
		)");
		QUARK_UT_VERIFY(false);
	}
	catch(const std::runtime_error& e){
		QUARK_UT_VERIFY(string(e.what()) == "Cannot resolve type.");
	}
}

QUARK_UNIT_TEST("vector", "explit bind, is [] working as type?", "strings", "valid vector"){
	const auto vm = test__run_global(R"(
		[string] a = ["one", "two"];
		print(a);
	)");
	ut_compare_stringvects(vm._print_output, vector<string>{
		R"(["one", "two"])"
	});
}

QUARK_UNIT_TEST("vector", "", "empty vector", "valid vector"){
	const auto vm = test__run_global(R"(
		[string] a = [];
		print(a);
	)");
	ut_compare_stringvects(vm._print_output, vector<string>{
		R"([])"
	});
}

QUARK_UNIT_TEST("vector", "[]", "strings", ""){
	const auto vm = test__run_global(R"(
		[string] a = ["alpha", "beta"];
		assert(a[0] == "alpha");
		assert(a[1] == "beta");
	)");
}


QUARK_UNIT_TEST("vector", "computed element", "strings", ""){
	const auto vm = test__run_global(R"(
		string get_beta(){ return "beta"; }
		[string] a = ["alpha", get_beta()];
		assert(a[0] == "alpha");
		assert(a[1] == "beta");
	)");
}


QUARK_UNIT_TEST("vector", "=", "strings", "valid vector"){
	const auto vm = test__run_global(R"(
		a = ["one", "two"];
		b = a;
		assert(a == b);
		print(a);
		print(b);
	)");
	ut_compare_stringvects(vm._print_output, vector<string>{
		R"(["one", "two"])",
		R"(["one", "two"])"
	});
}
QUARK_UNIT_TEST("vector", "==", "strings", ""){
	const auto vm = test__run_global(R"(
		assert((["one", "two"] == ["one", "two"]) == true);
	)");
}
QUARK_UNIT_TEST("vector", "==", "strings", ""){
	const auto vm = test__run_global(R"(
		assert(([1,2,3,4] == [1,2,3,4]) == true);
	)");
}

//	We could support this if we had special type for empty-vector and empty-dict.

QUARK_UNIT_TEST("vector", "==", "lhs and rhs are empty-typeless", ""){
	try{
		const auto vm = test__run_global(R"(
			assert(([] == []) == true);
		)");
		QUARK_UT_VERIFY(false);
	}
	catch(const std::runtime_error& e){
		QUARK_TEST_VERIFY(string(e.what()) == "Cannot resolve type.");
	}
}

QUARK_UNIT_TEST("vector", "==", "strings", ""){
	const auto vm = test__run_global(R"(
		assert((["one", "three"] == ["one", "two"]) == false);
	)");
}

QUARK_UNIT_TEST("vector", "<", "strings", ""){
	const auto vm = test__run_global(R"(
		assert((["one", "two"] < ["one", "two"]) == false);
	)");
}

QUARK_UNIT_TEST("vector", "<", "strings", ""){
	const auto vm = test__run_global(R"(
		assert((["one", "a"] < ["one", "two"]) == true);
	)");
}
QUARK_UNIT_TEST("vector", "<", "strings", ""){
	const auto vm = test__run_global(R"(
		assert((["one", "a"] < ["one", "two"]) == true);
	)");
}

QUARK_UNIT_TEST("vector", "size()", "", "correct size"){
	try{
		const auto vm = test__run_global(R"(
			[string] a = ["one", "two"];
			assert(size(a) == 0);
		)");
		QUARK_UT_VERIFY(false);
	}
	catch(...){
	}
}

QUARK_UNIT_TEST("vector", "size()", "[]", "correct size"){
	const auto vm = test__run_global(R"(
		[string] a = [];
		assert(size(a) == 0);
	)");
}
QUARK_UNIT_TEST("vector", "size()", "[]", "correct size"){
	const auto vm = test__run_global(R"(
		[string] a = ["one", "two"];
		assert(size(a) == 2);
	)");
}

QUARK_UNIT_TEST("vector", "+", "add empty vectors", "correct final vector"){
	try{
		const auto vm = test__run_global(R"(
			[string] a = [] + [];
			assert(a == []);
		)");
		QUARK_UT_VERIFY(false);
	}
	catch(const std::runtime_error& e){
		QUARK_TEST_VERIFY(string(e.what()) == "Cannot resolve type.");
	}
}

QUARK_UNIT_TEST("vector", "+", "non-empty vectors", "correct final vector"){
	const auto vm = test__run_global(R"(
		[string] a = ["one"] + ["two"];
		assert(a == ["one", "two"]);
	)");
}

QUARK_UNIT_TEST("vector", "push_back()", "vector", "correct final vector"){
	const auto vm = test__run_global(R"(
		[string] a = push_back(["one"], "two");
		assert(a == ["one", "two"]);
	)");
}

QUARK_UNIT_TEST("vector", "find()", "vector", "correct return"){
	const auto vm = test__run_global(R"(
		assert(find([1,2,3], 4) == -1);
		assert(find([1,2,3], 1) == 0);
		assert(find([1,2,2,2,3], 2) == 1);
	)");
}

QUARK_UNIT_TEST("vector", "find()", "string", "correct return"){
	const auto vm = test__run_global(R"(
		assert(find("hello, world", "he") == 0);
		assert(find("hello, world", "e") == 1);
		assert(find("hello, world", "x") == -1);
	)");
}

QUARK_UNIT_TEST("vector", "subset()", "", ""){
	const auto vm = test__run_global(R"(
		assert(subset([10,20,30], 0, 3) == [10,20,30]);
	)");
}
QUARK_UNIT_TEST("vector", "subset()", "", ""){
	const auto vm = test__run_global(R"(
		assert(subset([10,20,30], 1, 3) == [20,30]);
	)");
}
QUARK_UNIT_TEST("vector", "subset()", "", ""){
	const auto vm = test__run_global(R"(
		result = (subset([10,20,30], 0, 0) == []);
	)");
}
QUARK_UNIT_TEST("vector", "subset()", "", ""){
	const auto vm = test__run_global(R"(
		assert(subset([10,20,30], 0, 0) == []);
	)");
}

QUARK_UNIT_TEST("vector", "replace()", "", ""){
	const auto vm = test__run_global(R"(
		assert(replace([ 1, 2, 3, 4, 5, 6 ], 2, 5, [20, 30]) == [1, 2, 20, 30, 6]);
	)");
}
// ### test pos limiting and edge cases.


QUARK_UNIT_TEST("vector", "update()", "mutate element", "valid vector, without side effect on original vector"){
	const auto vm = test__run_global(R"(
		a = [ "one", "two", "three"];
		b = update(a, 1, "zwei");
		print(a);
		print(b);
		assert(a == ["one","two","three"]);
		assert(b == ["one","zwei","three"]);
	)");
	ut_compare_stringvects(vm._print_output, vector<string>{
		R"(["one", "two", "three"])",
		R"(["one", "zwei", "three"])"
	});
}


//////////////////////////////////////////		DICT - TYPE



QUARK_UNIT_TEST("dict", "construct", "", ""){
	const auto vm = test__run_global(R"(
		[string: int] a = {"one": 1, "two": 2};
		assert(size(a) == 2);
	)");
}
QUARK_UNIT_TEST("dict", "[]", "", ""){
	const auto vm = test__run_global(R"(
		[string: int] a = {"one": 1, "two": 2};
		print(a["one"]);
		print(a["two"]);
	)");
	ut_compare_stringvects(vm._print_output, vector<string>{
		"1",
		"2"
	});
}

QUARK_UNIT_TEST("dict", "", "", ""){
	try{
		const auto vm = test__run_global(R"(
			mutable a = {};
			a = {"hello": 1};
			print(a);
		)");
		QUARK_UT_VERIFY(false);
	}
	catch(...){
	}
}

QUARK_UNIT_TEST("dict", "[:]", "", ""){
	try {
		const auto vm = test__run_global(R"(
			a = {};
			print(a);
		)");
		QUARK_UT_VERIFY(false);
	}
	catch(...){
	}
}


QUARK_UNIT_TEST("dict", "deduced type ", "", ""){
	const auto vm = test__run_global(R"(
		a = {"one": 1, "two": 2};
		print(a);
	)");
	ut_compare_stringvects(vm._print_output, vector<string>{
		R"({"one": 1, "two": 2})",
	});
}

QUARK_UNIT_TEST("dict", "{}", "", ""){
	const auto vm = test__run_global(R"(
		mutable [string:int] a = {};
		a = {};
		print(a);
	)");
	ut_compare_stringvects(vm._print_output, vector<string>{
		R"({})",
	});
}

QUARK_UNIT_TEST("dict", "==", "", ""){
	const auto vm = test__run_global(R"(
		assert(({"one": 1, "two": 2} == {"one": 1, "two": 2}) == true);
	)");
}
QUARK_UNIT_TEST("dict", "==", "", ""){
	const auto vm = test__run_global(R"(
		assert(({"one": 1, "two": 2} == {"two": 2}) == false);
	)");
}
QUARK_UNIT_TEST("dict", "==", "", ""){
	const auto vm = test__run_global(R"(
		assert(({"one": 2, "two": 2} == {"one": 1, "two": 2}) == false);
	)");
}
QUARK_UNIT_TEST("dict", "==", "", ""){
	const auto vm = test__run_global(R"(
		assert(({"one": 1, "two": 2} < {"one": 1, "two": 2}) == false);
	)");
}
QUARK_UNIT_TEST("dict", "==", "", ""){
	const auto vm = test__run_global(R"(
		assert(({"one": 1, "two": 1} < {"one": 1, "two": 2}) == true);
	)");
}

QUARK_UNIT_TEST("dict", "size()", "[:]", "correct size"){
	try{
		const auto vm = test__run_global(R"(
			assert(size({}) == 0);
		)");
	}
	catch(const std::runtime_error& e){
		QUARK_UT_VERIFY(false);
		QUARK_TEST_VERIFY(string(e.what()) == "Cannot resolve type.");
	}
}

QUARK_UNIT_TEST("dict", "size()", "[:]", "correct type"){
	try{
		const auto vm = test__run_global(R"(
			print({});
		)");
	}
	catch(const std::runtime_error& e){
		QUARK_UT_VERIFY(false);
		QUARK_TEST_VERIFY(string(e.what()) == "Cannot resolve type.");
	}
}

QUARK_UNIT_TEST("dict", "size()", "[:]", "correct size"){
	const auto vm = test__run_global(R"(
		assert(size({"one":1}) == 1);
	)");
}
QUARK_UNIT_TEST("dict", "size()", "[:]", "correct size"){
	const auto vm = test__run_global(R"(
		assert(size({"one":1, "two":2}) == 2);
	)");
}

QUARK_UNIT_TEST("dict", "update()", "add element", "valid dict, without side effect on original dict"){
	const auto vm = test__run_global(R"(
		a = { "one": 1, "two": 2};
		b = update(a, "three", 3);
		print(a);
		print(b);
	)");
	ut_compare_stringvects(vm._print_output, vector<string>{
		R"({"one": 1, "two": 2})",
		R"({"one": 1, "three": 3, "two": 2})"
	});
}

QUARK_UNIT_TEST("dict", "update()", "replace element", ""){
	const auto vm = test__run_global(R"(
		a = { "one": 1, "two": 2, "three" : 3};
		b = update(a, "three", 333);
		print(a);
		print(b);
	)");
	ut_compare_stringvects(vm._print_output, vector<string>{
		R"({"one": 1, "three": 3, "two": 2})",
		R"({"one": 1, "three": 333, "two": 2})"
	});
}

/*
QUARK_UNIT_TEST_VIP("dict", "update()", "dest is empty dict", ""){
	const auto vm = test__run_global(R"(
		a = update({}, "one", 1);
		b = update(a, "two", 2);
		print(b);
		assert(a == {"one": 1});
		assert(b == {"one": 1, "two": 2});
	)");
}
*/

QUARK_UNIT_TEST("dict", "exists()", "", ""){
	const auto vm = test__run_global(R"(
		a = { "one": 1, "two": 2, "three" : 3};
		assert(exists(a, "two") == true);
		assert(exists(a, "four") == false);
	)");
}

QUARK_UNIT_TEST("dict", "erase()", "", ""){
	const auto vm = test__run_global(R"(
		a = { "one": 1, "two": 2, "three" : 3};
		b = erase(a, "one");
		assert(b == { "two": 2, "three" : 3});
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
		struct t { int a;}
	)");
}

QUARK_UNIT_TESTQ("run_main()", "struct - make instance"){
	const auto vm = test__run_global(R"(
		struct t { int a;}
		t(3);
	)");
	QUARK_UT_VERIFY((	vm._call_stack.back()->_values["t"].first.is_typeid()	));
}

QUARK_UNIT_TEST("run_main()", "struct - check struct's type", "", ""){
	const auto vm = test__run_global(R"(
		struct t { int a;}
		print(t);
	)");
	ut_compare_stringvects(vm._print_output, vector<string>{
		"struct {int a;}"
	});
}

QUARK_UNIT_TESTQ("run_main()", "struct - check struct's type"){
	const auto vm = test__run_global(R"(
		struct t { int a;}
		a = t(3);
		print(a);
	)");
	ut_compare_stringvects(vm._print_output, vector<string>{
		 R"({a=3})"
	});
}

QUARK_UNIT_TESTQ("run_main()", "struct - read back struct member"){
	const auto vm = test__run_global(R"(
		struct t { int a;}
		temp = t(4);
		print(temp.a);
	)");
	ut_compare_stringvects(vm._print_output, vector<string>{
		"4"
	});
}

QUARK_UNIT_TEST("run_main()", "struct - instantiate nested structs", "", ""){
	const auto vm = test__run_global(R"(
		struct color { int red; int green; int blue;}
		struct image { color back; color front;}

		c = color(128, 192, 255);
		print(c);

		i = image(color(1, 2, 3), color(200, 201, 202));
		print(i);
	)");
	ut_compare_stringvects(vm._print_output, vector<string>{
		"{red=128, green=192, blue=255}",
		"{back={red=1, green=2, blue=3}, front={red=200, green=201, blue=202}}"
	});
}

QUARK_UNIT_TESTQ("run_main()", "struct - access member of nested structs"){
	const auto vm = test__run_global(R"(
		struct color { int red; int green; int blue;}
		struct image { color back; color front;}
		i = image(color(1, 2, 3), color(200, 201, 202));
		print(i.front.green);
	)");
	ut_compare_stringvects(vm._print_output, vector<string>{
		"201"
	});
}

QUARK_UNIT_TEST("run_main()", "return struct from function", "", ""){
	const auto vm = test__run_global(R"(
		struct color { int red; int green; int blue;}
		struct image { color back; color front;}
		color make_color(){
			return color(100, 101, 102);
		}
		print(make_color());
	)");
	ut_compare_stringvects(vm._print_output, vector<string>{
		"{red=100, green=101, blue=102}",
	});
}

QUARK_UNIT_TESTQ("run_main()", "struct - compare structs"){
	const auto vm = test__run_global(R"(
		struct color { int red; int green; int blue;}
		print(color(1, 2, 3) == color(1, 2, 3));
	)");
	ut_compare_stringvects(vm._print_output, vector<string>{
		"true"
	});
}

QUARK_UNIT_TESTQ("run_main()", "struct - compare structs"){
	const auto vm = test__run_global(R"(
		struct color { int red; int green; int blue;}
		print(color(9, 2, 3) == color(1, 2, 3));
	)");
	ut_compare_stringvects(vm._print_output, vector<string>{
		"false"
	});
}

QUARK_UNIT_TESTQ("run_main()", "struct - compare structs different types"){
	try {
		const auto vm = test__run_global(R"(
			struct color { int red; int green; int blue;}
			struct file { int id;}
			print(color(1, 2, 3) == file(404));
		)");
		QUARK_UT_VERIFY(false);
	}
	catch(const std::runtime_error& e){
		QUARK_TEST_VERIFY(string(e.what()) == "Expression type mismatch.");
	}
}

QUARK_UNIT_TESTQ("run_main()", "struct - compare structs with <, different types"){
	const auto vm = test__run_global(R"(
		struct color { int red; int green; int blue;}
		print(color(1, 2, 3) < color(1, 2, 3));
	)");
	ut_compare_stringvects(vm._print_output, vector<string>{
		"false"
	});
}

QUARK_UNIT_TESTQ("run_main()", "struct - compare structs <"){
	const auto vm = test__run_global(R"(
		struct color { int red; int green; int blue;}
		print(color(1, 2, 3) < color(1, 4, 3));
	)");
	ut_compare_stringvects(vm._print_output, vector<string>{
		"true"
	});
}




QUARK_UNIT_TESTQ("run_main()", "update struct manually"){
	const auto vm = test__run_global(R"(
		struct color { int red; int green; int blue;}
		a = color(255, 128, 128);
		b = color(a.red, a.green, 129);
		print(a);
		print(b);
	)");
	ut_compare_stringvects(vm._print_output, vector<string>{
		"{red=255, green=128, blue=128}",
		"{red=255, green=128, blue=129}"
	});
}


QUARK_UNIT_TESTQ("run_main()", "mutate struct member using = won't work"){
	try {
		const auto vm = test__run_global(R"(
			struct color { int red; int green; int blue;}
			a = color(255,128,128);
			b = a.green = 3;
			print(a);
			print(b);
		)");
	}
	catch(...){
	}
}

QUARK_UNIT_TESTQ("run_main()", "mutate struct member using update()"){
	const auto vm = test__run_global(R"(
		struct color { int red; int green; int blue;}
		a = color(255,128,128);
		b = update(a, "green", 3);
		print(a);
		print(b);
	)");

	ut_compare_stringvects(vm._print_output, vector<string>{
		"{red=255, green=128, blue=128}",
		"{red=255, green=3, blue=128}",
	});
}

QUARK_UNIT_TEST("run_main()", "mutate nested member", "", ""){
	const auto vm = test__run_global(R"(
		struct color { int red; int green; int blue;}
		struct image { color back; color front;}
		a = image(color(0,100,200), color(0,0,0));
		b = update(a, "front.green", 3);
		print(a);
		print(b);
	)");
	ut_compare_stringvects(vm._print_output, vector<string>{
		"{back={red=0, green=100, blue=200}, front={red=0, green=0, blue=0}}",
		"{back={red=0, green=100, blue=200}, front={red=0, green=3, blue=0}}"
	});
}



QUARK_UNIT_TEST("", "", "", ""){
	const auto vm = test__run_global(R"(
		start = get_time_of_day();
		mutable b = 0;
		mutable t = [0];
		for(i in 0...100){
			b = b + 1;
			t = push_back(t, b);
		}
		end = get_time_of_day();
		print("Duration: " + to_string(end - start) + ", number = " + to_string(b));
		print(t);
	)");
	QUARK_UT_VERIFY(true);
}



QUARK_UNIT_TESTQ("comments", "// on start of line"){
	const auto vm = test__run_global(R"(
		//	XYZ
		print("Hello");
	)");
	ut_compare_stringvects(vm._print_output, vector<string>{
		"Hello"
	});
}

QUARK_UNIT_TESTQ("comments", "// on start of line"){
	const auto vm = test__run_global(R"(
		print("Hello");		//	XYZ
	)");
	ut_compare_stringvects(vm._print_output, vector<string>{
		"Hello"
	});
}

QUARK_UNIT_TESTQ("comments", "// on start of line"){
	const auto vm = test__run_global(R"(
		print("Hello");/* xyz */print("Bye");
	)");
	ut_compare_stringvects(vm._print_output, vector<string>{
		"Hello",
		"Bye"
	});
}



//////////////////////////////////////////		json_value - TYPE




QUARK_UNIT_TEST("json_value-string", "deduce json_value::string", "", ""){
	const auto result = run_return_result(R"(
		json_value result = "hello";
	)", {});
	ut_compare_values(result, value_t::make_json_value("hello"));
}

QUARK_UNIT_TEST("json_value-string", "string-size()", "", ""){
	const auto result = run_return_result(R"(
		json_value a = "hello";
		result = size(a);
	)", {});
	ut_compare_values(result, value_t::make_int(5));
}

QUARK_UNIT_TEST("json_value-number", "construct number", "", ""){
	const auto result = run_return_result(R"(
		json_value result = 13;
	)", {});
	ut_compare_values(result, value_t::make_json_value(13));
}

QUARK_UNIT_TEST("json_value-bool", "construct true", "", ""){
	const auto result = run_return_result(R"(
		json_value result = true;
	)", {});
	ut_compare_values(result, value_t::make_json_value(true));
}
QUARK_UNIT_TEST("json_value-bool", "construct false", "", ""){
	const auto result = run_return_result(R"(
		json_value result = false;
	)", {});
	ut_compare_values(result, value_t::make_json_value(false));
}

QUARK_UNIT_TEST("json_value-array", "construct array", "", ""){
	const auto result = run_return_result(R"(
		json_value result = ["hello", "bye"];
	)", {});
	ut_compare_values(result, value_t::make_json_value(json_t::make_array(vector<json_t>{"hello", "bye"})));
}

QUARK_UNIT_TEST("json_value-array", "read array member", "", ""){
	const auto result = run_return_result(R"(
		json_value a = ["hello", "bye"];
		result = string(a[0]) + string(a[1]);
	)", {});
	ut_compare_values(result, value_t::make_string("hellobye"));
}
QUARK_UNIT_TEST("json_value-array", "read array member", "", ""){
	const auto result = run_return_result(R"(
		json_value a = ["hello", "bye"];
		result = a[0];
	)", {});
	ut_compare_values(result, value_t::make_json_value("hello"));
}
QUARK_UNIT_TEST("json_value-array", "read array member", "", ""){
	const auto result = run_return_result(R"(
		json_value a = ["hello", "bye"];
		result = a[1];
	)", {});
	ut_compare_values(result, value_t::make_json_value("bye"));
}

QUARK_UNIT_TEST("json_value-array", "size()", "", ""){
	const auto result = run_return_result(R"(
		json_value a = ["a", "b", "c", "d"];
		result = size(a)
	)", {});
	ut_compare_values(result, value_t::make_int(4));
}

//	NOTICE: Floyd dict is stricter than JSON -- cannot have different types of values!
QUARK_UNIT_TEST("json_value-object", "def", "mix value-types in dict", ""){
	const auto vm = test__run_global(R"(
		json_value a = { "pigcount": 3, "pigcolor": "pink" };
		print(a);
	)");
	ut_compare_stringvects(vm._print_output, vector<string>{
		R"({ "pigcolor": "pink", "pigcount": 3 })"
	});
}

// JSON example snippets: http://json.org/example.html
QUARK_UNIT_TEST("json_value-object", "def", "read world data", ""){
	const auto vm = test__run_global(R"ABCD(
		json_value a = {
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
		};
		print(a);
	)ABCD");
	ut_compare_stringvects(vm._print_output, vector<string>{
		R"ABCD({ "menu": { "id": "file", "popup": { "menuitem": [{ "onclick": "CreateNewDoc()", "value": "New" }, { "onclick": "OpenDoc()", "value": "Open" }, { "onclick": "CloseDoc()", "value": "Close" }] }, "value": "File" } })ABCD"
	});
}

QUARK_UNIT_TEST("json_value-object", "{}", "expressions inside def", ""){
	const auto vm = test__run_global(R"(
		json_value a = { "pigcount": 1 + 2, "pigcolor": "pi" + "nk" };
		print(a);
	)");
	ut_compare_stringvects(vm._print_output, vector<string>{
		R"({ "pigcolor": "pink", "pigcount": 3 })"
	});
}

QUARK_UNIT_TEST("json_value-object", "{}", "", ""){
	const auto vm = test__run_global(R"(
		json_value a = { "pigcount": 3, "pigcolor": "pink" };
		print(a["pigcount"]);
		print(a["pigcolor"]);
	)");
	ut_compare_stringvects(vm._print_output, vector<string>{
		"3",
		"\"pink\""
	});
}

QUARK_UNIT_TEST("json_value-object", "size()", "", ""){
	const auto vm = test__run_global(R"(
		json_value a = { "a": 1, "b": 2, "c": 3, "d": 4, "e": 5 };
		assert(size(a) == 5);
	)");
}

QUARK_UNIT_TEST("json_value-null", "construct null", "", ""){
	const auto result = run_return_result(R"(
		json_value result = null;
	)", {});
	ut_compare_values(result, value_t::make_json_value(json_t()));
}




//////////////////////////////////////////		json_value features



QUARK_UNIT_TEST("", "get_json_type()", "{}", ""){
	const auto result = run_return_result(R"(
		result = get_json_type(json_value({}))
	)", {});
	ut_compare_values(result, value_t::make_int(1));
}
QUARK_UNIT_TEST("", "get_json_type()", "[]", ""){
	const auto result = run_return_result(R"(
		result = get_json_type(json_value([]))
	)", {});
	ut_compare_values(result, value_t::make_int(2));
}
QUARK_UNIT_TEST("", "get_json_type()", "string", ""){
	const auto result = run_return_result(R"(
		result = get_json_type(json_value("hello"))
	)", {});
	ut_compare_values(result, value_t::make_int(3));
}
QUARK_UNIT_TEST("", "get_json_type()", "number", ""){
	const auto result = run_return_result(R"(
		result = get_json_type(json_value(13))
	)", {});
	ut_compare_values(result, value_t::make_int(4));
}
QUARK_UNIT_TEST("", "get_json_type()", "number", ""){
	const auto result = run_return_result(R"(
		result = get_json_type(json_value(true))
	)", {});
	ut_compare_values(result, value_t::make_int(5));
}
QUARK_UNIT_TEST("", "get_json_type()", "number", ""){
	const auto result = run_return_result(R"(
		result = get_json_type(json_value(false))
	)", {});
	ut_compare_values(result, value_t::make_int(6));
}

QUARK_UNIT_TEST("", "get_json_type()", "null", ""){
	const auto result = run_return_result(R"(
		result = get_json_type(json_value(null))
	)", {});
	ut_compare_values(result, value_t::make_int(7));
}

QUARK_UNIT_TEST("", "get_json_type()", "DOCUMENTATION SNIPPET", ""){
	const auto vm = test__run_global(R"ABCD(
		string get_name(json_value value){
			t = get_json_type(value);
			if(t == json_object){
				return "json_object";
			}
			else if(t == json_array){
				return "json_array";
			}
			else if(t == json_string){
				return "json_string";
			}
			else if(t == json_number){
				return "json_number";
			}
			else if(t == json_true){
				return "json_true";
			}
			else if(t == json_false){
				return "json_false";
			}
			else if(t == json_null){
				return "json_null";
			}
			else {
				assert(false);
			}
		}

		assert(get_name(json_value({"a": 1, "b": 2})) == "json_object");
		assert(get_name(json_value([1,2,3])) == "json_array");
		assert(get_name(json_value("crash")) == "json_string");
		assert(get_name(json_value(0.125)) == "json_number");
		assert(get_name(json_value(true)) == "json_true");
		assert(get_name(json_value(false)) == "json_false");
	)ABCD");
}


QUARK_UNIT_TEST("", "", "", ""){
	const auto result = run_return_result(R"(
		struct pixel_t { float x; float y; }
		a = pixel_t(100.0, 200.0);
		result = a.x;
	)", {});
	ut_compare_values(result, value_t::make_float(100.0));
}

QUARK_UNIT_TEST("", "", "", ""){
	const auto result = run_return_result(R"(
		struct pixel_t { float x; float y; }
		c = { "version": "1.0", "image": [pixel_t(100.0, 200.0), pixel_t(101.0, 201.0)] };
		result = c["image"][1].y
	)", {});
	ut_compare_values(result, value_t::make_float(201.0));
}


//////////////////////////////////////////		decode_json()



QUARK_UNIT_TEST("", "decode_json()", "", ""){
	const auto result = run_return_result(R"(
		result = decode_json("\"genelec\"");
	)", {});
	ut_compare_values(result, value_t::make_json_value(json_t("genelec")));
}

QUARK_UNIT_TEST("", "decode_json()", "", ""){
	const auto vm = test__run_global(R"(
		a = decode_json("{ \"magic\": 1234 }");
		print(a);
	)");
	ut_compare_stringvects(vm._print_output, vector<string>{
		R"({ "magic": 1234 })"
	});
}


//////////////////////////////////////////		encode_json()



QUARK_UNIT_TEST("", "encode_json()", "", ""){
	const auto vm = test__run_global(R"(
		json_value a = "cheat";
		b = encode_json(a);
		print(b);
	)");
	ut_compare_stringvects(vm._print_output, vector<string>{
		"\"cheat\""
	});
}


QUARK_UNIT_TEST("", "encode_json()", "", ""){
	const auto vm = test__run_global(R"(
		json_value a = { "magic": 1234 };
		b = encode_json(a);
		print(b);
	)");
	ut_compare_stringvects(vm._print_output, vector<string>{
		"{ \"magic\": 1234 }"
	});
}



//////////////////////////////////////////		flatten_to_json()



QUARK_UNIT_TEST("", "flatten_to_json()", "bool", ""){
	const auto result = run_return_result(R"(
		result = flatten_to_json(true);
	)", {});
	ut_compare_values(result, value_t::make_json_value(json_t(true)));
}
QUARK_UNIT_TEST("", "flatten_to_json()", "bool", ""){
	const auto result = run_return_result(R"(
		result = flatten_to_json(false);
	)", {});
	ut_compare_values(result, value_t::make_json_value(json_t(false)));
}

QUARK_UNIT_TEST("", "flatten_to_json()", "int", ""){
	const auto result = run_return_result(R"(
		result = flatten_to_json(789);
	)", {});
	ut_compare_values(result, value_t::make_json_value(json_t(789.0)));
}
QUARK_UNIT_TEST("", "flatten_to_json()", "int", ""){
	const auto result = run_return_result(R"(
		result = flatten_to_json(-987);
	)", {});
	ut_compare_values(result, value_t::make_json_value(json_t(-987.0)));
}

QUARK_UNIT_TEST("", "flatten_to_json()", "float", ""){
	const auto result = run_return_result(R"(
		result = flatten_to_json(-0.125);
	)", {});
	ut_compare_values(result, value_t::make_json_value(json_t(-0.125)));
}


QUARK_UNIT_TEST("", "flatten_to_json()", "string", ""){
	const auto result = run_return_result(R"(
		result = flatten_to_json("fanta");
	)", {});
	ut_compare_values(result, value_t::make_json_value(json_t("fanta")));
}

QUARK_UNIT_TEST("", "flatten_to_json()", "typeid", ""){
	const auto result = run_return_result(R"(
		result = flatten_to_json(typeof([2,2,3]));
	)", {});
	ut_compare_values(result, value_t::make_json_value(json_t::make_array(vector<json_t>{ "vector", "int"})));
}

QUARK_UNIT_TEST("", "flatten_to_json()", "[]", ""){
	const auto result = run_return_result(R"(
		result = flatten_to_json([1,2,3]);
	)", {});
	ut_compare_values(result, value_t::make_json_value(
		json_t::make_array(vector<json_t>{1,2,3})
	));
}

QUARK_UNIT_TEST("", "flatten_to_json()", "{}", ""){
	const auto result = run_return_result(R"(
		result = flatten_to_json({"ten": 10, "eleven": 11});
	)", {});
	ut_compare_values(result, value_t::make_json_value(
		json_t::make_object(std::map<string,json_t>{{"ten", 10},{"eleven", 11}})
	));
}

QUARK_UNIT_TEST("", "flatten_to_json()", "pixel_t", ""){
	const auto result = run_return_result(R"(
		struct pixel_t { float x; float y; }
		c = pixel_t(100.0, 200.0);
		a = flatten_to_json(c);
		result = encode_json(a);
	)", {});
	ut_compare_values(result, value_t::make_string("{ \"x\": 100, \"y\": 200 }"));
}

QUARK_UNIT_TEST("", "flatten_to_json()", "[pixel_t]", ""){
	const auto result = run_return_result(R"(
		struct pixel_t { float x; float y; }
		c = [pixel_t(100.0, 200.0), pixel_t(101.0, 201.0)];
		a = flatten_to_json(c);
		result = encode_json(a);
	)", {});
	ut_compare_values(result, value_t::make_string("[{ \"x\": 100, \"y\": 200 }, { \"x\": 101, \"y\": 201 }]"));
}



//////////////////////////////////////////		flatten_to_json() -> unflatten_from_json() roundtrip


QUARK_UNIT_TEST("", "unflatten_from_json()", "bool", ""){
	const auto result = run_return_result(R"(
		result = unflatten_from_json(flatten_to_json(true), bool);
	)", {});
	ut_compare_values(result, value_t::make_bool(true));
}
QUARK_UNIT_TEST("", "unflatten_from_json()", "bool", ""){
	const auto result = run_return_result(R"(
		result = unflatten_from_json(flatten_to_json(false), bool);
	)", {});
	ut_compare_values(result, value_t::make_bool(false));
}

QUARK_UNIT_TEST("", "unflatten_from_json()", "int", ""){
	const auto result = run_return_result(R"(
		result = unflatten_from_json(flatten_to_json(91), int);
	)", {});
	ut_compare_values(result, value_t::make_int(91));
}

QUARK_UNIT_TEST("", "unflatten_from_json()", "float", ""){
	const auto result = run_return_result(R"(
		result = unflatten_from_json(flatten_to_json(-0.125), float);
	)", {});
	ut_compare_values(result, value_t::make_float(-0.125));
}

QUARK_UNIT_TEST("", "unflatten_from_json()", "string", ""){
	const auto result = run_return_result(R"(
		result = unflatten_from_json(flatten_to_json(""), string);
	)", {});
	ut_compare_values(result, value_t::make_string(""));
}

QUARK_UNIT_TEST("", "unflatten_from_json()", "string", ""){
	const auto result = run_return_result(R"(
		result = unflatten_from_json(flatten_to_json("cola"), string);
	)", {});
	ut_compare_values(result, value_t::make_string("cola"));
}

/// Makes no sense to unflatten a json_value from json.

QUARK_UNIT_TEST("", "unflatten_from_json()", "typeid", ""){
	const auto result = run_return_result(R"(
		result = unflatten_from_json(flatten_to_json(typeof([3])), typeid);
	)", {});
	ut_compare_values(result, value_t::make_typeid_value(typeid_t::make_vector(typeid_t::make_int())));
}

QUARK_UNIT_TEST("", "unflatten_from_json()", "[]", ""){
	const auto result = run_return_result(R"(
		result = unflatten_from_json(flatten_to_json([1,2,3]), typeof([1]));
	)", {});
	ut_compare_values(
		result,
		value_t::make_vector_value(typeid_t::make_int(), vector<value_t>{value_t::make_int(1), value_t::make_int(2), value_t::make_int(3)})
	);
}


QUARK_UNIT_TEST("", "unflatten_from_json()", "point_t", ""){
	const auto point_t_def = std::make_shared<floyd::struct_definition_t>(
		std::vector<member_t>{
			member_t(typeid_t::make_float(), "x"),
			member_t(typeid_t::make_float(), "y")
		}
	);
	const auto result = run_return_result(R"(
		struct point_t { float x; float y; }
		result = unflatten_from_json(flatten_to_json(point_t(1.0, 3.0)), point_t);
	)", {});
	ut_compare_values(result, value_t::make_struct_value(typeid_t::make_struct(point_t_def), vector<value_t>{ value_t::make_float(1), value_t::make_float(3)}));
}




//??? test accessing array->struct->array.
//??? test structs in vectors.




QUARK_UNIT_TEST("Edge case", "", "if with non-bool expression", "exception"){
	try{
		const auto result = run_return_result(R"(
			if("not a bool"){
			}
			else{
				assert(false);
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
		const auto result = run_return_result(R"(
			int a = 10;
			int a = 11;
		)", {});
		QUARK_TEST_VERIFY(false);
	}
	catch(const std::runtime_error& e){
		QUARK_TEST_VERIFY(string(e.what()) == "Local identifier already exists.");
	}
}
QUARK_UNIT_TEST("Edge case", "", "Define struct with colliding name", "exception"){
	try{
		const auto result = run_return_result(R"(
			int a = 10;
			struct a { int x; }
		)", {});
		QUARK_TEST_VERIFY(false);
	}
	catch(const std::runtime_error& e){
		QUARK_TEST_VERIFY(string(e.what()) == "Name already used.");
	}
}

QUARK_UNIT_TEST("Edge case", "", "Access unknown struct member", "exception"){
	try{
		const auto result = run_return_result(R"(
			struct a { int x; }
			b = a(13);
			print(b.y);
		)", {});
		QUARK_TEST_VERIFY(false);
	}
	catch(const std::runtime_error& e){
		QUARK_TEST_VERIFY(string(e.what()) == "Unknown struct member \"y\".");
	}
}

QUARK_UNIT_TEST("Edge case", "", "Access unknown member in non-struct", "exception"){
	try{
		const auto result = run_return_result(R"(
			a = 10;
			print(a.y);
		)", {});
		QUARK_TEST_VERIFY(false);
	}
	catch(const std::runtime_error& e){
		QUARK_TEST_VERIFY(string(e.what()) == "Resolve struct member failed.");
	}
}

QUARK_UNIT_TEST("Edge case", "", "Lookup in string using non-int", "exception"){
	try{
		const auto result = run_return_result(R"(
			string a = "test string";
			print(a["not an integer"]);
		)", {});
		QUARK_TEST_VERIFY(false);
	}
	catch(const std::runtime_error& e){
		QUARK_TEST_VERIFY(string(e.what()) == "Lookup in string by index-only.");
	}
}

QUARK_UNIT_TEST("Edge case", "", "Lookup in vector using non-int", "exception"){
	try{
		const auto result = run_return_result(R"(
			[string] a = ["one", "two", "three"];
			print(a["not an integer"]);
		)", {});
		QUARK_TEST_VERIFY(false);
	}
	catch(const std::runtime_error& e){
		QUARK_TEST_VERIFY(string(e.what()) == "Lookup in vector by index-only.");
	}
}

QUARK_UNIT_TEST("Edge case", "", "Lookup in dict using non-string key", "exception"){
	try{
		const auto result = run_return_result(R"(
			a = { "one": 1, "two": 2 };
			print(a[3]);
		)", {});
		QUARK_TEST_VERIFY(false);
	}
	catch(const std::runtime_error& e){
		QUARK_TEST_VERIFY(string(e.what()) == "Lookup in dict by string-only.");
	}
}

QUARK_UNIT_TEST("Edge case", "", "Access undefined variable", "exception"){
	try{
		const auto result = run_return_result(R"(
			print(a);
		)", {});
		QUARK_TEST_VERIFY(false);
	}
	catch(const std::runtime_error& e){
		QUARK_TEST_VERIFY(string(e.what()) == "Undefined variable \"a\".");
	}
}

QUARK_UNIT_TEST("Edge case", "", "Wrong number of arguments in function call", "exception"){
	try{
		const auto result = run_return_result(R"(
			int f(int a){ return a + 1; }
			a = f(1, 2);
		)", {});
		QUARK_TEST_VERIFY(false);
	}
	catch(const std::runtime_error& e){
		QUARK_TEST_VERIFY(string(e.what()) == "Wrong number of arguments in function call.");
	}
}


QUARK_UNIT_TEST("Edge case", "", "Wrong number of arguments to struct-constructor", "exception"){
	try{
		const auto result = run_return_result(R"(
			struct pos { float x; float y;}
			a = pos(3);
		)", {});
		QUARK_TEST_VERIFY(false);
	}
	catch(const std::runtime_error& e){
		QUARK_TEST_VERIFY(string(e.what()) == "Wrong number of arguments in function call.");
	}
}

QUARK_UNIT_TEST("Edge case", "", "Wrong TYPE of arguments to struct-constructor", "exception"){
	try{
		const auto result = run_return_result(R"(
			struct pos { float x; float y;}
			a = pos(3, "hello");
		)", {});
		QUARK_TEST_VERIFY(false);
	}
	catch(const std::runtime_error& e){
		QUARK_TEST_VERIFY(string(e.what()) == "Expression type mismatch.");
	}
}

QUARK_UNIT_TEST("Edge case", "", "Wrong number of arguments to int-constructor", "exception"){
	try{
		const auto result = run_return_result(R"(
			a = int();
		)", {});
		QUARK_TEST_VERIFY(false);
	}
	catch(const std::runtime_error& e){
		QUARK_TEST_VERIFY(string(e.what()) == "Wrong number of arguments in function call.");
	}
}

QUARK_UNIT_TEST("Edge case", "", "Call non-function, non-struct, non-typeid", "exception"){
	try{
		const auto result = run_return_result(R"(
			a = 3();
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
		const auto result = run_return_result(R"(
			a = [3, bool];
		)", {});
		QUARK_TEST_VERIFY(false);
	}
	catch(const std::runtime_error& e){
		QUARK_TEST_VERIFY(string(e.what()) == "Vector can not hold elements of different types.");
	}
}


QUARK_UNIT_TEST("Edge case", "", ".", "exception"){
	try{
		const auto result = run_return_result(R"(
			a = 3 < "hello";
		)", {});
		QUARK_TEST_VERIFY(false);
	}
	catch(const std::runtime_error& e){
		QUARK_TEST_VERIFY(string(e.what()) == "Expression type mismatch.");
	}
}

QUARK_UNIT_TEST("Edge case", "", ".", "exception"){
	try{
		const auto result = run_return_result(R"(
			a = 3 * 3.2;
		)", {});
		QUARK_TEST_VERIFY(false);
	}
	catch(const std::runtime_error& e){
		QUARK_TEST_VERIFY(string(e.what()) == "Expression type mismatch.");
	}
}
QUARK_UNIT_TEST("Edge case", "", ".", "exception"){
	try{
		const auto result = run_return_result(R"(
			a = false + true;
		)", {});
		QUARK_TEST_VERIFY(false);
	}
	catch(const std::runtime_error& e){
		QUARK_TEST_VERIFY(string(e.what()) == "Artithmetics: bool not allowed.");
	}
}

