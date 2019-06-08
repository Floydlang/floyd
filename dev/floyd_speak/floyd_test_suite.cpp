//
//  floyd_test_suite.cpp
//  FloydSpeak
//
//  Created by Marcus Zetterquist on 2018-01-21.
//  Copyright © 2018 Marcus Zetterquist. All rights reserved.
//

#include "floyd_test_suite.h"

#include "test_helpers.h"

#include "floyd_interpreter.h"
#include "ast_value.h"
#include "expression.h"
#include "json_support.h"
#include "text_parser.h"
#include "host_functions.h"
#include "file_handling.h"
#include "compiler_helpers.h"

#include <string>
#include <vector>
#include <iostream>

using namespace floyd;




/*
unsupported syntax
	"result = [int](1,2,3);",

unsupported syntax
	"result = [[int]]([1,2,3], [4,5,6]);",

unsupported syntax
	"struct pixel_t { int red; int green; int blue; } result = [pixel_t](pixel_t(1,2,3),pixel_t(4,5,6));",

	R"(result = [{string: int}]({"a":1,"b":2,"c":3}, {"d":4,"e":5,"f":6});)",
}
*/

//??? Run all container2 tests for LLVM too!
//??? Add tests for arithmetic bounds, limiting, wrapping.
//??? Test truthness off all variable types: strings, doubles



void run_closed(const std::string& program){
	ut_run_closed(program, compilation_unit_mode::k_no_core_lib);
}
void run_closed_lib(const std::string& program){
	ut_run_closed(program, compilation_unit_mode::k_include_core_lib);
}


void ut_verify_printout_lib(const quark::call_context_t& context, const std::string& program, const std::vector<std::string>& printout){
	ut_verify_printout(context, program, compilation_unit_mode::k_include_core_lib, printout);
}
void ut_verify_printout_nolib(const quark::call_context_t& context, const std::string& program, const std::vector<std::string>& printout){
	ut_verify_printout(context, program, compilation_unit_mode::k_no_core_lib, printout);
}


void ut_verify_global_result_lib(const quark::call_context_t& context, const std::string& program, const value_t& expected_result){
	ut_verify_global_result(context, program, compilation_unit_mode::k_include_core_lib, expected_result);
}
void ut_verify_global_result_nolib(const quark::call_context_t& context, const std::string& program, const value_t& expected_result){
	ut_verify_global_result(context, program, compilation_unit_mode::k_no_core_lib, expected_result);
}

void ut_verify_exception_nolib(const quark::call_context_t& context, const std::string& program, const std::string& expected_what){
	ut_verify_exception(context, program, compilation_unit_mode::k_no_core_lib, expected_what);
}



//////////////////////////////////////////		DEFINE VARIABLE, SIMPLE TYPES



QUARK_UNIT_TEST("Floyd test suite", "Define variable", "int", ""){
	ut_verify_global_result_nolib(QUARK_POS, "let int result = 123", value_t::make_int(123));
}

QUARK_UNIT_TEST("Floyd test suite", "Define variable", "bool", ""){
	ut_verify_global_result_nolib(QUARK_POS, "let bool result = true", value_t::make_bool(true));
}
QUARK_UNIT_TEST("Floyd test suite", "Define variable", "bool", ""){
	ut_verify_global_result_nolib(QUARK_POS, "let bool result = false", value_t::make_bool(false));
}

QUARK_UNIT_TEST("Floyd test suite", "Define variable", "int", ""){
	ut_verify_global_result_nolib(QUARK_POS, "let int result = 123", value_t::make_int(123));
}

QUARK_UNIT_TEST("Floyd test suite", "Define variable", "double", ""){
	ut_verify_global_result_nolib(QUARK_POS, "let double result = 3.5", value_t::make_double(double(3.5)));
}

QUARK_UNIT_TEST("Floyd test suite", "Define variable", "string", ""){
	ut_verify_global_result_nolib(QUARK_POS, R"xxx(let string result = "xyz")xxx", value_t::make_string("xyz"));
}

//	??? Add special error message when local is not initialized.
QUARK_UNIT_TEST("Floyd test suite", "Define variable", "Error: No initial assignment", "compiler error"){
	ut_verify_exception_nolib(
		QUARK_POS,
		R"(

			mutable string row

		)",
		R"___(Expected '=' character. Line: 3 "mutable string row")___"
	);
}
QUARK_UNIT_TEST("Floyd test suite", "Define variable", "Error: assign to immutable local", "exception"){
	ut_verify_exception_nolib(
		QUARK_POS,
		R"(

			let int a = 10
			let int a = 11

		)",
		"Local identifier \"a\" already exists. Line: 4 \"let int a = 11\""
	);
}

QUARK_UNIT_TEST("Floyd test suite", "Define variable", "Error: assign to unknown local", "exception"){
	ut_verify_exception_nolib(
		QUARK_POS,
		R"(

			a = 10

		)",
		"Unknown identifier 'a'. Line: 3 \"a = 10\""
	);
}


//////////////////////////////////////////		BASIC EXPRESSIONS, SIMPLE TYPES


QUARK_UNIT_TEST("Floyd test suite", "+", "", "") {
	ut_verify_global_result_nolib(QUARK_POS, "let int result = 1 + 2", value_t::make_int(3));
}
QUARK_UNIT_TEST("Floyd test suite", "+", "", ""){
	ut_verify_global_result_nolib(QUARK_POS, "let int result = 1 + 2 + 3", value_t::make_int(6));
}
QUARK_UNIT_TEST("Floyd test suite", "*", "", ""){
	ut_verify_global_result_nolib(QUARK_POS, "let int result = 3 * 4", value_t::make_int(12));
}

QUARK_UNIT_TEST("Floyd test suite", "parant", "", ""){
	ut_verify_global_result_nolib(QUARK_POS, "let int result = (3 * 4) * 5", value_t::make_int(60));
}



//////////////////////////////////////////		BASIC EXPRESSIONS - CONDITIONAL EXPRESSION


QUARK_UNIT_TEST("Floyd test suite", "conditional expression", "", ""){
	ut_verify_global_result_nolib(QUARK_POS, "let int result = true ? 98 : 99", value_t::make_int(98));
}
QUARK_UNIT_TEST("Floyd test suite", "conditional expression", "", ""){
	ut_verify_global_result_nolib(QUARK_POS, "let int result = false ? 70 : 80", value_t::make_int(80));
}


QUARK_UNIT_TEST("Floyd test suite", "conditional expression", "", ""){
	ut_verify_global_result_nolib(QUARK_POS, "let string result = true ? \"yes\" : \"no\"", value_t::make_string("yes"));
}
QUARK_UNIT_TEST("Floyd test suite", "conditional expression", "", ""){
	ut_verify_global_result_nolib(QUARK_POS, "let string result = false ? \"yes\" : \"no\"", value_t::make_string("no"));
}

QUARK_UNIT_TEST("Floyd test suite", "conditional expression", "?:", ""){
	ut_verify_global_result_nolib(QUARK_POS, "let int result = true ? 4 : 6", value_t::make_int(4));
}
QUARK_UNIT_TEST("Floyd test suite", "conditional expression", "?:", ""){
	ut_verify_global_result_nolib(QUARK_POS, "let int result = false ? 4 : 6", value_t::make_int(6));
}

QUARK_UNIT_TEST("Floyd test suite", "conditional expression", "?:", ""){
	ut_verify_global_result_nolib(QUARK_POS, "let int result = 1==3 ? 4 : 6", value_t::make_int(6));
}
QUARK_UNIT_TEST("Floyd test suite", "conditional expression", "?:", ""){
	ut_verify_global_result_nolib(QUARK_POS, "let int result = 3==3 ? 4 : 6", value_t::make_int(4));
}

QUARK_UNIT_TEST("Floyd test suite", "conditional expression", "?:", ""){
	ut_verify_global_result_nolib(QUARK_POS, "let int result = 3==3 ? 2 + 2 : 2 * 3", value_t::make_int(4));
}

QUARK_UNIT_TEST("Floyd test suite", "conditional expression", "?:", ""){
	ut_verify_global_result_nolib(QUARK_POS, "let int result = 3==1+2 ? 2 + 2 : 2 * 3", value_t::make_int(4));
}


//////////////////////////////////////////		BASIC EXPRESSIONS - PARANTHESES


QUARK_UNIT_TEST("Floyd test suite", "execute_expression()", "Parentheses", ""){
	ut_verify_global_result_nolib(QUARK_POS, "let int result = 5*(4+4+1)", value_t::make_int(45));
}
QUARK_UNIT_TEST("Floyd test suite", "execute_expression()", "Parentheses", ""){
	ut_verify_global_result_nolib(QUARK_POS, "let int result = 5*(2*(1+3)+1)", value_t::make_int(45));
}
QUARK_UNIT_TEST("Floyd test suite", "execute_expression()", "Parentheses", ""){
	ut_verify_global_result_nolib(QUARK_POS, "let int result = 5*((1+3)*2+1)", value_t::make_int(45));
}

QUARK_UNIT_TEST("Floyd test suite", "execute_expression()", "Sign before parentheses", ""){
	ut_verify_global_result_nolib(QUARK_POS, "let int result = -(2+1)*4", value_t::make_int(-12));
}
QUARK_UNIT_TEST("Floyd test suite", "execute_expression()", "Sign before parentheses", ""){
	ut_verify_global_result_nolib(QUARK_POS, "let int result = -4*(2+1)", value_t::make_int(-12));
}


//////////////////////////////////////////		BASIC EXPRESSIONS - SPACES


QUARK_UNIT_TEST("Floyd test suite", "execute_expression()", "Spaces", ""){
	ut_verify_global_result_nolib(QUARK_POS, "let int result = 5 * ((1 + 3) * 2 + 1)", value_t::make_int(45));
}
QUARK_UNIT_TEST("Floyd test suite", "execute_expression()", "Spaces", ""){
	ut_verify_global_result_nolib(QUARK_POS, "let int result = 5 - 2 * ( 3 )", value_t::make_int(-1));
}
QUARK_UNIT_TEST("Floyd test suite", "execute_expression()", "Spaces", ""){
	ut_verify_global_result_nolib(QUARK_POS, "let int result =  5 - 2 * ( ( 4 )  - 1 )", value_t::make_int(-1));
}


//////////////////////////////////////////		BASIC EXPRESSIONS - DOUBLE


QUARK_UNIT_TEST("Floyd test suite", "execute_expression()", "Fractional numbers", "") {
	ut_verify_global_result_nolib(QUARK_POS, "let double result = 2.8/2.0", value_t::make_double(1.4));
}
QUARK_UNIT_TEST("Floyd test suite", "execute_expression()", "Fractional numbers", ""){
//	ut_verify_global_result_nolib(QUARK_POS, "int result = 1/5e10") == 2e-11);
}
QUARK_UNIT_TEST("Floyd test suite", "execute_expression()", "Fractional numbers", ""){
	ut_verify_global_result_nolib(QUARK_POS, "let double result = (4.0-3.0)/(4.0*4.0)", value_t::make_double(0.0625));
}
QUARK_UNIT_TEST("Floyd test suite", "execute_expression()", "Fractional numbers", ""){
	ut_verify_global_result_nolib(QUARK_POS, "let double result = 1.0/2.0/2.0", value_t::make_double(0.25));
}
QUARK_UNIT_TEST("Floyd test suite", "execute_expression()", "Fractional numbers", ""){
	ut_verify_global_result_nolib(QUARK_POS, "let double result = 0.25 * .5 * 0.5", value_t::make_double(0.0625));
}
QUARK_UNIT_TEST("Floyd test suite", "execute_expression()", "Fractional numbers", ""){
	ut_verify_global_result_nolib(QUARK_POS, "let double result = .25 / 2.0 * .5", value_t::make_double(0.0625));
}

//////////////////////////////////////////		BASIC EXPRESSIONS - EDGE CASES


QUARK_UNIT_TEST("Floyd test suite", "execute_expression()", "Repeated operators", ""){
	ut_verify_global_result_nolib(QUARK_POS, "let int result = 1+-2", value_t::make_int(-1));
}
QUARK_UNIT_TEST("Floyd test suite", "execute_expression()", "Repeated operators", ""){
	ut_verify_global_result_nolib(QUARK_POS, "let int result = --2", value_t::make_int(2));
}
QUARK_UNIT_TEST("Floyd test suite", "execute_expression()", "Repeated operators", ""){
	ut_verify_global_result_nolib(QUARK_POS, "let int result = 2---2", value_t::make_int(0));
}
QUARK_UNIT_TEST("Floyd test suite", "execute_expression()", "Repeated operators", ""){
	ut_verify_global_result_nolib(QUARK_POS, "let int result = 2-+-2", value_t::make_int(4));
}

QUARK_UNIT_TEST("Floyd test suite", "int", "Wrong number of arguments to int-constructor", "exception"){
	ut_verify_exception_nolib(
		QUARK_POS,
		R"(

			let a = int()

		)",
		"Wrong number of arguments in function call, got 0, expected 1. Line: 3 \"let a = int()\""
	);
}

QUARK_UNIT_TEST("Floyd test suite", "Expression", "Error: mix types", "exception"){
	ut_verify_exception_nolib(
		QUARK_POS,
		R"(

			let a = 3 < "hello"

		)",
		"Expression type mismatch - cannot convert 'string' to 'int. Line: 3 \"let a = 3 < \"hello\"\""
	);
}

QUARK_UNIT_TEST("Floyd test suite", "Expression", "Error: mix types", "exception"){
	ut_verify_exception_nolib(
		QUARK_POS,
		R"(

			let a = 3 * 3.2

		)",
		"Expression type mismatch - cannot convert 'double' to 'int. Line: 3 \"let a = 3 * 3.2\""
	);
}


//////////////////////////////////////////		BASIC EXPRESSIONS - BOOL


QUARK_UNIT_TEST("Floyd test suite", "bool constructor", "", ""){
	ut_verify_global_result_nolib(QUARK_POS, "let bool result = true", value_t::make_bool(true));
}
QUARK_UNIT_TEST("Floyd test suite", "bool constructor", "", ""){
	ut_verify_global_result_nolib(QUARK_POS, "let bool result = false", value_t::make_bool(false));
}

QUARK_UNIT_TEST("Floyd test suite", "bool +", "", ""){
	ut_verify_global_result_nolib(
		QUARK_POS,
		R"(

			let result = false + true

		)",
		value_t::make_bool(true)
	);
}
QUARK_UNIT_TEST("Floyd test suite", "bool +", "", ""){
	ut_verify_global_result_nolib(QUARK_POS, R"(		let result = false + false		)", value_t::make_bool(false));
}
QUARK_UNIT_TEST("Floyd test suite", "bool +", "", ""){
	ut_verify_global_result_nolib(QUARK_POS, R"(		let result = true + true		)", value_t::make_bool(true));
}





//////////////////////////////////////////		BASIC EXPRESSIONS - OPERATOR ==


QUARK_UNIT_TEST("Floyd test suite", "execute_expression()", "==", ""){
	ut_verify_global_result_nolib(QUARK_POS, "let bool result = 1 == 1", value_t::make_bool(true));
}
QUARK_UNIT_TEST("Floyd test suite", "execute_expression()", "==", ""){
	ut_verify_global_result_nolib(QUARK_POS, "let bool result = 1 == 2", value_t::make_bool(false));
}
QUARK_UNIT_TEST("Floyd test suite", "execute_expression()", "==", ""){
	ut_verify_global_result_nolib(QUARK_POS, "let bool result = 1.3 == 1.3", value_t::make_bool(true));
}
QUARK_UNIT_TEST("Floyd test suite", "execute_expression()", "==", ""){
	ut_verify_global_result_nolib(QUARK_POS, "let bool result = \"hello\" == \"hello\"", value_t::make_bool(true));
}
QUARK_UNIT_TEST("Floyd test suite", "execute_expression()", "==", ""){
	ut_verify_global_result_nolib(QUARK_POS, "let bool result = \"hello\" == \"bye\"", value_t::make_bool(false));
}


//////////////////////////////////////////		BASIC EXPRESSIONS - OPERATOR <


QUARK_UNIT_TEST("Floyd test suite", "execute_expression()", "<", "") {
	ut_verify_global_result_nolib(QUARK_POS, "let bool result = 1 < 2", value_t::make_bool(true));
}
QUARK_UNIT_TEST("Floyd test suite", "execute_expression()", "<", ""){
	ut_verify_global_result_nolib(QUARK_POS, "let bool result = 5 < 2", value_t::make_bool(false));
}
QUARK_UNIT_TEST("Floyd test suite", "execute_expression()", "<", ""){
	ut_verify_global_result_nolib(QUARK_POS, "let bool result = 0.3 < 0.4", value_t::make_bool(true));
}
QUARK_UNIT_TEST("Floyd test suite", "execute_expression()", "<", ""){
	ut_verify_global_result_nolib(QUARK_POS, "let bool result = 1.5 < 0.4", value_t::make_bool(false));
}
QUARK_UNIT_TEST("Floyd test suite", "execute_expression()", "<", ""){
	ut_verify_global_result_nolib(QUARK_POS, "let bool result = \"adwark\" < \"boat\"", value_t::make_bool(true));
}
QUARK_UNIT_TEST("Floyd test suite", "execute_expression()", "<", ""){
	ut_verify_global_result_nolib(QUARK_POS, "let bool result = \"boat\" < \"adwark\"", value_t::make_bool(false));
}


//////////////////////////////////////////		BASIC EXPRESSIONS - OPERATOR &&

QUARK_UNIT_TEST("Floyd test suite", "execute_expression()", "&&", "") {
	ut_verify_global_result_nolib(QUARK_POS, "let bool result = false && false", value_t::make_bool(false));
}
QUARK_UNIT_TEST("Floyd test suite", "execute_expression()", "&&", ""){
	ut_verify_global_result_nolib(QUARK_POS, "let bool result = false && true", value_t::make_bool(false));
}
QUARK_UNIT_TEST("Floyd test suite", "execute_expression()", "&&", ""){
	ut_verify_global_result_nolib(QUARK_POS, "let bool result = true && false", value_t::make_bool(false));
}
QUARK_UNIT_TEST("Floyd test suite", "execute_expression()", "&&", ""){
	ut_verify_global_result_nolib(QUARK_POS, "let bool result = true && true", value_t::make_bool(true));
}

QUARK_UNIT_TEST("Floyd test suite", "execute_expression()", "&&", ""){
	ut_verify_global_result_nolib(QUARK_POS, "let bool result = false && false && false", value_t::make_bool(false));
}
QUARK_UNIT_TEST("Floyd test suite", "execute_expression()", "&&", ""){
	ut_verify_global_result_nolib(QUARK_POS, "let bool result = false && false && true", value_t::make_bool(false));
}
QUARK_UNIT_TEST("Floyd test suite", "execute_expression()", "&&", ""){
	ut_verify_global_result_nolib(QUARK_POS, "let bool result = false && true && false", value_t::make_bool(false));
}
QUARK_UNIT_TEST("Floyd test suite", "execute_expression()", "&&", ""){
	ut_verify_global_result_nolib(QUARK_POS, "let bool result = false && true && true", value_t::make_bool(false));
}
QUARK_UNIT_TEST("Floyd test suite", "execute_expression()", "&&", ""){
	ut_verify_global_result_nolib(QUARK_POS, "let bool result = true && false && false", value_t::make_bool(false));
}
QUARK_UNIT_TEST("Floyd test suite", "execute_expression()", "&&", ""){
	ut_verify_global_result_nolib(QUARK_POS, "let bool result = true && true && false", value_t::make_bool(false));
}
QUARK_UNIT_TEST("Floyd test suite", "execute_expression()", "&&", ""){
	ut_verify_global_result_nolib(QUARK_POS, "let bool result = true && true && true", value_t::make_bool(true));
}

//////////////////////////////////////////		BASIC EXPRESSIONS - OPERATOR ||

QUARK_UNIT_TEST("Floyd test suite", "execute_expression()", "||", ""){
	ut_verify_global_result_nolib(QUARK_POS, "let bool result = false || false", value_t::make_bool(false));
}
QUARK_UNIT_TEST("Floyd test suite", "execute_expression()", "||", ""){
	ut_verify_global_result_nolib(QUARK_POS, "let bool result = false || true", value_t::make_bool(true));
}
QUARK_UNIT_TEST("Floyd test suite", "execute_expression()", "||", ""){
	ut_verify_global_result_nolib(QUARK_POS, "let bool result = true || false", value_t::make_bool(true));
}
QUARK_UNIT_TEST("Floyd test suite", "execute_expression()", "||", ""){
	ut_verify_global_result_nolib(QUARK_POS, "let bool result = true || true", value_t::make_bool(true));
}


QUARK_UNIT_TEST("Floyd test suite", "execute_expression()", "||", ""){
	ut_verify_global_result_nolib(QUARK_POS, "let bool result = false || false || false", value_t::make_bool(false));
}
QUARK_UNIT_TEST("Floyd test suite", "execute_expression()", "||", ""){
	ut_verify_global_result_nolib(QUARK_POS, "let bool result = false || false || true", value_t::make_bool(true));
}
QUARK_UNIT_TEST("Floyd test suite", "execute_expression()", "||", ""){
	ut_verify_global_result_nolib(QUARK_POS, "let bool result = false || true || false", value_t::make_bool(true));
}
QUARK_UNIT_TEST("Floyd test suite", "execute_expression()", "||", ""){
	ut_verify_global_result_nolib(QUARK_POS, "let bool result = false || true || true", value_t::make_bool(true));
}
QUARK_UNIT_TEST("Floyd test suite", "execute_expression()", "||", ""){
	ut_verify_global_result_nolib(QUARK_POS, "let bool result = true || false || false", value_t::make_bool(true));
}
QUARK_UNIT_TEST("Floyd test suite", "execute_expression()", "||", ""){
	ut_verify_global_result_nolib(QUARK_POS, "let bool result = true || false || true", value_t::make_bool(true));
}
QUARK_UNIT_TEST("Floyd test suite", "execute_expression()", "||", ""){
	ut_verify_global_result_nolib(QUARK_POS, "let bool result = true || true || false", value_t::make_bool(true));
}
QUARK_UNIT_TEST("Floyd test suite", "execute_expression()", "||", ""){
	ut_verify_global_result_nolib(QUARK_POS, "let bool result = true || true || true", value_t::make_bool(true));
}


//////////////////////////////////////////		BASIC EXPRESSIONS - ERRORS



QUARK_UNIT_TEST("Floyd test suite", "execute_expression()", "Type mismatch", "") {
	ut_verify_exception_nolib(
		QUARK_POS,
		"let int result = true",
		"Expression type mismatch - cannot convert 'bool' to 'int. Line: 1 \"let int result = true\""
	);
}

#if 0
QUARK_UNIT_TEST("Floyd test suite", "execute_expression()", "Division by zero", "") {
	ut_verify_exception_nolib(QUARK_POS, "let int result = 2/0", "EEE_DIVIDE_BY_ZERO");
}

QUARK_UNIT_TEST("Floyd test suite", "execute_expression()", "Division by zero", ""){
	ut_verify_exception_nolib(QUARK_POS, "let int result = 3+1/(5-5)+4", "EEE_DIVIDE_BY_ZERO");
}
QUARK_UNIT_TEST("Floyd test suite", "execute_expression()", "Division by zero", ""){
	ut_verify_exception_nolib(QUARK_POS, "let double result = 2.0 / 1.0", "EEE_DIVIDE_BY_ZERO");
}
#endif

QUARK_UNIT_TEST("Floyd test suite", "execute_expression()", "-true", "") {
	ut_verify_exception_nolib(
		QUARK_POS,
		"let int result = -true",
		"Unary minus don't work on expressions of type \"bool\", only int and double. Line: 1 \"let int result = -true\""
	);
}

QUARK_UNIT_TEST("Floyd test suite", "Forgot let or mutable", "", "Exception"){
	ut_verify_exception_nolib(QUARK_POS, "int test = 123", "Use 'mutable' or 'let' syntax. Line: 1 \"int test = 123\"");
}

QUARK_UNIT_TEST("Floyd test suite", "Access variable", "Access undefined variable", "exception"){
	ut_verify_exception_nolib(QUARK_POS, R"(		print(a)		)", "Undefined variable \"a\". Line: 1 \"print(a)\"");
}



//////////////////////////////////////////		TEST CONSTRUCTOR - SIMPLE TYPES


QUARK_UNIT_TEST("Floyd test suite", "Construct value", "bool()", ""){
	ut_verify_global_result_nolib(QUARK_POS, "let result = bool(false)", value_t::make_bool(false));
}
QUARK_UNIT_TEST("Floyd test suite", "Construct value", "bool()", ""){
	ut_verify_global_result_nolib(QUARK_POS, "let result = bool(true)", value_t::make_bool(true));
}
QUARK_UNIT_TEST("Floyd test suite", "Construct value", "int()", ""){
	ut_verify_global_result_nolib(QUARK_POS, "let result = int(123)", value_t::make_int(123));
}
QUARK_UNIT_TEST("Floyd test suite", "Construct value", "double()", ""){
	ut_verify_global_result_nolib(QUARK_POS, "let result = double(0.0)", value_t::make_double(0.0));
}
QUARK_UNIT_TEST("Floyd test suite", "Construct value", "double()", ""){
	ut_verify_global_result_nolib(QUARK_POS, "let result = double(123.456)", value_t::make_double(123.456));
}

QUARK_UNIT_TEST("Floyd test suite", "Construct value", "string()", ""){
	ut_verify_exception_nolib(QUARK_POS, "let result = string()", "Wrong number of arguments in function call, got 0, expected 1. Line: 1 \"let result = string()\"");
}

QUARK_UNIT_TEST("Floyd test suite", "Construct value", "string()", ""){
	ut_verify_global_result_nolib(QUARK_POS, "let result = string(\"ABCD\")", value_t::make_string("ABCD"));
}




//////////////////////////////////////////		HOST FUNCTION - print()



QUARK_UNIT_TEST("Floyd test suite", "Expression statement", "", ""){
	ut_verify_printout_nolib(QUARK_POS, "print(5)", { "5" });
}

QUARK_UNIT_TEST("Floyd test suite", "print() supports ints and strings", "", ""){
	ut_verify_printout_nolib(
		QUARK_POS,
		R"(

			print("Hello, world!")
			print(123)

		)",
		{ "Hello, world!", "123" }
	);
}





//////////////////////////////////////////		MUTATE VARIABLES




QUARK_UNIT_TEST("Floyd test suite", "Mutate", "mutate local", ""){
	ut_verify_printout_nolib(
		QUARK_POS,
		R"(

			mutable a = 1
			a = 2
			print(a)

		)",
		{ "2" }
	);
}

QUARK_UNIT_TEST("Floyd test suite", "Mutate", "increment a mutable", "OK"){
	ut_verify_printout_nolib(
		QUARK_POS,
		R"(

			mutable a = 1000
			a = a + 1
			print(a)

		)",
		{ "1001" }
	);
}

QUARK_UNIT_TEST("Floyd test suite", "Mutate", "Store to immutable", "compiler error"){
	ut_verify_exception_nolib(
		QUARK_POS,
		R"(

			let a = 3
			a = 4

		)",
		"Cannot assign to immutable identifier \"a\". Line: 4 \"a = 4\""
	);
}


QUARK_UNIT_TEST("Floyd test suite", "", "String (which requires RC)", ""){
	run_closed(R"___(

		func void f(){
			let s = "A"
		}

		f()

	)___");
}
QUARK_UNIT_TEST("Floyd test suite", "Mutate", "String (which requires RC)", ""){
	run_closed(R"___(

		func string f(){
			mutable s = "A"
			return s
		}

		f()

	)___");
}
QUARK_UNIT_TEST("Floyd test suite", "Mutate", "String (which requires RC)", ""){
	run_closed(R"___(

		func string f(){
			mutable s = "A"
			s = "<" + s + ">"
			return s
		}

		let result = f()
		assert(result == "<A>")

	)___");
}



//////////////////////////////////////////		HOST FUNCTION - assert()

//??? add file + line to Floyd's asserts
QUARK_UNIT_TEST("Floyd test suite", "", "", ""){
	ut_verify_exception_nolib(QUARK_POS, R"(		assert(1 == 2)		)", "Floyd assertion failed.");
}

QUARK_UNIT_TEST("Floyd test suite", "", "", ""){
	ut_verify_printout_nolib(
		QUARK_POS,
		R"(

			assert(1 == 1)
			print("A")

		)",
		{ "A" }
	);
}

//////////////////////////////////////////		HOST FUNCTION - to_string()


QUARK_UNIT_TEST("Floyd test suite", "", "", ""){
	ut_verify_global_result_nolib(QUARK_POS, R"(		let string result = to_string(145)		)", value_t::make_string("145"));
}
QUARK_UNIT_TEST("Floyd test suite", "", "", ""){
	ut_verify_global_result_nolib(QUARK_POS, R"(		let string result = to_string(3.1)		)", value_t::make_string("3.1"));
}

QUARK_UNIT_TEST("Floyd test suite", "", "", ""){
	ut_verify_global_result_nolib(QUARK_POS, R"(		let string result = to_string(3.0)		)", value_t::make_string("3.0"));
}



//////////////////////////////////////////		IMPLEMENT MAIN()


QUARK_UNIT_TEST("Floyd test suite", "main() - Can make and read global int", "", ""){
	ut_verify_mainfunc_return_nolib(
		QUARK_POS,
		R"(

			let int test = 123
			func int main(){
				return test
			}

		)",
		{},
		123
	);
}

QUARK_UNIT_TEST("Floyd test suite", "main()", "int main()", ""){
	ut_verify_mainfunc_return_nolib(
		QUARK_POS,
		R"(

			func int main(){
				return 3 + 4
			}

		)",
		{ "a" },
		7
	);
}




//////////////////////////////////////////		func


QUARK_UNIT_TEST("Floyd test suite", "func", "Simplest func", ""){
	ut_verify_global_result_nolib(
		QUARK_POS,
		R"(

			func int f(){
				return 1000;
			}
			let result = f()

		)",
		value_t::make_int(1000)
	);
}

QUARK_UNIT_TEST("Floyd test suite", "func", "Function with int argument", ""){
	ut_verify_global_result_nolib(
		QUARK_POS,
		R"(

			func int f(int a){
				return a + 1;
			}
			let result = f(1000)

		)",
		value_t::make_int(1001)
	);
}

QUARK_UNIT_TEST("Floyd test suite", "func", "define additional function, call it several times", ""){
	ut_verify_mainfunc_return_nolib(
		QUARK_POS,
		R"(

			func int myfunc(){ return 5 }
			func int main(){
				return myfunc() + myfunc() * 2
			}

		)",
		{ "dummy-arg" },
		15
	);
}

QUARK_UNIT_TEST("Floyd test suite", "func", "use function inputs", ""){
	ut_verify_global_result_nolib(
		QUARK_POS,
		R"(

			func string test(string args){
				return "-" + args + "-"
			}

			let result = test("xyz");
		)",
		value_t::make_string("-xyz-")
	);
}


QUARK_UNIT_TEST("Floyd test suite", "func", "test function args are always immutable", ""){
	ut_verify_exception_nolib(
		QUARK_POS,
		R"(

			func int f(int x){
				x = 6
			}
			f(5)

		)",
		"Cannot assign to immutable identifier \"x\". Line: 4 \"x = 6\""
	);
}

QUARK_UNIT_TEST("Floyd test suite", "func", "Recursion: function calling itself by name", ""){
	run_closed(R"(		func int fx(int a){ return fx(a + 1) }		)");
}

QUARK_UNIT_TEST("Floyd test suite", "func", "return from void function", "error"){
	ut_verify_exception_nolib(
		QUARK_POS,
		R"(

			func void f (int x) impure {
				return 3
			}

		)",
		R"xxx(Cannot return value from function with void-return. Line: 4 "return 3")xxx"
	);
}

QUARK_UNIT_TEST("Floyd test suite", "func", "void function, no return statement", ""){
	run_closed(
		R"(

			func void f (int x) impure {
			}

		)"
	);
}


QUARK_UNIT_TEST("Floyd test suite", "Call", "Error: Wrong number of arguments in function call", "exception"){
	ut_verify_exception_nolib(
		QUARK_POS,
		R"(

			func int f(int a){ return a + 1 }
			let a = f(1, 2)

		)",
		"Wrong number of arguments in function call, got 2, expected 1. Line: 4 \"let a = f(1, 2)\""
	);
}

QUARK_UNIT_TEST("Floyd test suite", "Call", "Call non-function, non-struct, non-typeid", "exception"){
	ut_verify_exception_nolib(QUARK_POS, R"(		let a = 3()		)", "Cannot call non-function, its type is int. Line: 1 \"let a = 3()\"");
}




//////////////////////////////////////////		RETURN STATEMENT



QUARK_UNIT_TEST("Floyd test suite", "return", "return from middle of function", ""){
	ut_verify_printout_nolib(
		QUARK_POS,
		R"(

			func string f(){
				print("A")
				return "B"
				print("C")
				return "D"
			}
			let string x = f()
			print(x)

		)",
		{ "A", "B" }
	);
}

QUARK_UNIT_TEST("Floyd test suite", "return", "return from within BLOCK", ""){
	ut_verify_printout_nolib(
		QUARK_POS,
		R"(

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

		)",
		{ "A", "B" }
	);
}

QUARK_UNIT_TEST("Floyd test suite", "return", "Make sure returning wrong type => error", ""){
	try {
		test_run_container2(R"(

			func int f(){
				return "x"
			}

		)", {}, "", "");
	}
	catch(const std::runtime_error& e){
		ut_verify(QUARK_POS, e.what(), "Expression type mismatch - cannot convert 'string' to 'int. Line: 4 \"return \"x\"\"");
	}
}


QUARK_UNIT_TEST("Floyd test suite", "return", "You can't return from global scope", "error"){
	ut_verify_exception_nolib(
		QUARK_POS,
		R"(

			return "hello"

		)",
		R"___(Cannot return value from function with void-return. Line: 3 "return "hello"")___"
	);
}





///////////////////////////////////////////////////			PURE & IMPURE FUNCTIONS


//??? It IS OK to call impure functions from a pure function:
//		1. The impure function modifies a local value only -- cannot leak out.

QUARK_UNIT_TEST("Floyd test suite", "impure", "call pure->pure", "Compiles OK"){
	run_closed(R"(

		func int a(int p){ return p + 1 }
		func int b(){ return a(100) }

	)");
}
QUARK_UNIT_TEST("Floyd test suite", "impure", "call impure->pure", "Compiles OK"){
	run_closed(R"(

		func int a(int p){ return p + 1 }
		func int b() impure { return a(100) }

	)");
}

QUARK_UNIT_TEST("Floyd test suite", "impure", "call pure->impure", "Compilation error"){
	ut_verify_exception_nolib(
		QUARK_POS,
		R"(

			func int a(int p) impure { return p + 1 }
			func int b(){ return a(100) }

		)",
		"Cannot call impure function from a pure function. Line: 4 \"func int b(){ return a(100) }\""
	);
}

QUARK_UNIT_TEST("Floyd test suite", "impure", "call impure->impure", "Compiles OK"){
	run_closed(R"(

		func int a(int p) impure { return p + 1 }
		func int b() impure { return a(100) }

	)");
}





//////////////////////////////////////////		Comments


QUARK_UNIT_TEST("comments", "// on start of line", "", ""){
	ut_verify_printout_nolib(
		QUARK_POS,
		R"(

			//	XYZ
			print("Hello")

		)",
		{ "Hello" }
	);
}

QUARK_UNIT_TEST("comments", "// on start of line", "", ""){
	ut_verify_printout_nolib(
		QUARK_POS,
		R"(

			print("Hello")		//	XYZ

		)",
		{ "Hello" }
	);
}

QUARK_UNIT_TEST("comments", "// on start of line", "", ""){
	ut_verify_printout_nolib(
		QUARK_POS,
		R"(

			print("Hello")/* xyz */print("Bye")

		)",
		{ "Hello", "Bye" }
	);
}



//////////////////////////////////////////		BLOCK & SCOPES & LOCALS


QUARK_UNIT_TEST("Floyd test suite", "Scopes: Empty block", "", ""){
	run_closed("{}");
}

QUARK_UNIT_TEST("Floyd test suite", "Scopes: Block with local variable, no shadowing", "", ""){
	run_closed("{ let int x = 4 }");
}


QUARK_UNIT_TEST("Floyd test suite", "Scopes: test reading from subscope", "", ""){
	ut_verify_printout_nolib(
		QUARK_POS,
		R"(

			mutable a = 7
			a = 8
			{
				print(a)
			}

		)",
		{ "8" }
	);
}

QUARK_UNIT_TEST("Floyd test suite", "Scopes: test mutating from a subscope", "", ""){
	ut_verify_printout_nolib(
		QUARK_POS,
		R"(

			let a = 7
			print(a)

		)",
		{ "7" }
	);
}


QUARK_UNIT_TEST("Floyd test suite", "Scopes: test scope ends", "", ""){
	ut_verify_printout_nolib(
		QUARK_POS,
		R"(

			let a = 7
			{
			}
			print(a)

		)",
		{ "7" }
	);
}

QUARK_UNIT_TEST("Floyd test suite", "Scopes: test mutating from a subscope", "", ""){
	ut_verify_printout_nolib(
		QUARK_POS,
		R"(

			mutable a = 7
			{
				a = 8
			}
			print(a)

		)",
		{ "8" }
	);
}


QUARK_UNIT_TEST("Floyd test suite", "Scopes: global", "", ""){
	ut_verify_printout_nolib(
		QUARK_POS,
		R"(

			//	GLOBAL SCOPE
			let a = 7
			let b = 1007

			assert(a == 7)	//	global-a
			print(b)		//	global-b
		)",
		{"1007"}
	);
}


QUARK_UNIT_TEST("Floyd test suite", "Scopes: Global block scopes, shadowing or not", "", ""){
	ut_verify_printout_nolib(
		QUARK_POS,
		R"(

			//	GLOBAL SCOPE
			let a = 7
			let b = 1007

			assert(a == 7)	//	global-a
			print(b)		//	global-b


			//	BLOCK IN GLOBAL SCOPE, SHADOW GLOBAL
			{
				assert(a == 7)		//	global-a
				assert(b == 1007)	//	global-b

				let a = 10
				assert(a == 10)		//	block local-a, shadowing global-a
				assert(b == 1007)	//	global-b
			}
		)",
		{"1007"}
	);
}

QUARK_UNIT_TEST("Floyd test suite", "Scopes: Three levels", "", ""){
	ut_verify_printout_nolib(
		QUARK_POS,
		R"(

			//	GLOBAL SCOPE
			let a = 1000
			assert(a == 1000)

			{
				assert(a == 1000)
				let a = 1001
				let b = 2001
				assert(a == 1001)
				assert(b == 2001)

				{
					assert(a == 1001)
					assert(b == 2001)
					let a = 1002
					let c = 3002
					assert(a == 1002)
					assert(b == 2001)
					assert(c == 3002)
				}
				assert(a == 1001)
				assert(b == 2001)
			}
			assert(a == 1000)

		)",
		{}
	);
}



QUARK_UNIT_TEST("Floyd test suite", "Scopes: Global block scopes, shadowing or not", "", ""){
	ut_verify_printout_nolib(
		QUARK_POS,
		R"(

			//	GLOBAL SCOPE
			let a = 7
			let b = 1007

			assert(a == 7)	//	global-a
			print(b)		//	global-b


			//	BLOCK IN GLOBAL SCOPE, SHADOW GLOBAL
			{
				assert(a == 7)		//	global-a
				assert(b == 1007)	//	global-b

				let a = 10
				assert(a == 10)		//	block local-a, shadowing global-a
				assert(b == 1007)	//	global-b
			}
		)",
		{"1007"}
	);
}

QUARK_UNIT_TEST("Floyd test suite", "Scopes: Global block scopes, shadowing or not", "", ""){
	ut_verify_printout_nolib(
		QUARK_POS,
		R"(

			//	GLOBAL SCOPE
			let a = 7
			let b = 1007

			assert(a == 7)	//	global-a
			print(b)		//	global-b


			//	BLOCK IN GLOBAL SCOPE, SHADOW GLOBAL
			{
				assert(a == 7)		//	global-a
				assert(b == 1007)	//	global-b

				let a = 10
				assert(a == 10)		//	block local-a, shadowing global-a
				assert(b == 1007)	//	global-b
			}

			//	Make we access the globals again.
			assert(a == 7)	//	global-a
			assert(b == 1007)	//	global-b
		)",
		{"1007"}
	);
}

QUARK_UNIT_TEST("Floyd test suite", "Scopes: Function arguments & locals & blocks vs globals, shadowing or not", "", ""){
	ut_verify_printout_nolib(
		QUARK_POS,
		R"(

			//	GLOBAL SCOPE
			let a = 7
			let b = 1007

			assert(a == 7)	//	global-a
			print(b)		//	global-b

			//	FUNCTION SCOPING
			//	Argument-a will shadow the global-a
			func int test(int a){
				assert(a == 8)		//	argument-a
				assert(b == 1007)	//	global-b

				//	Make local
				let c = 2007

				assert(c == 2007)	//	local-c



				//	Shadow global-b
				{
					assert(a == 8)		//	argument-a
					assert(b == 1007)	//	global-b
					assert(c == 2007)	//	local-

					let int b = 3

					assert(a == 8)		//	argument-a
					assert(b == 3)		//	shadow global-b
					assert(c == 2007)	//	local-c
				}

				assert(a == 8)		//	argument-a
				assert(b == 1007)	//	global-b
				assert(c == 2007)	//	local-c

				//	Shadow argument-a
				{
					assert(a == 8)		//	argument-a
					assert(b == 1007)	//	global-b
					assert(c == 2007)	//	local-c

					let int a = 4

					assert(a == 4)		//	block-local, shadowing argument-a
					assert(b == 1007)	//	global-b
					assert(c == 2007)	//	local-c
				}

				assert(a == 8)		//	argument-a
				assert(b == 1007)	//	global-b
				assert(c == 2007)	//	local-c


				return 0
			}

			test(8)
		)",
		{"1007"}
	);
}

QUARK_UNIT_TEST("Floyd test suite", "Scopes: All block scopes, shadowing or not", "", ""){
	ut_verify_printout_nolib(
		QUARK_POS,
		R"(

			//	GLOBAL SCOPE
			let a = 7
			let b = 1007

			assert(a == 7)	//	global-a
			print(b)	//	global-b


			//	BLOCK IN GLOBAL SCOPE, SHADOW GLOBAL
			{
				assert(a == 7)		//	global-a
				assert(b == 1007)	//	global-b

				let a = 10
				assert(a == 10)		//	block local-a, shadowing global-a
				assert(b == 1007)	//	global-b
			}

			//	Make we access the globals again.
			assert(a == 7)	//	global-ar
			assert(b == 1007)	//	global-b


			//	FUNCTION SCOPING
			//	Argument-a will shadow the global-a
			func int test(int a){
				assert(a == 8)		//	argument-a
				assert(b == 1007)	//	global-b

				//	Make local
				let c = 2007

				assert(c == 2007)	//	local-c

				//	Shadow global-b
				{
					assert(a == 8)		//	argument-a
					assert(b == 1007)	//	global-b
					assert(c == 2007)	//	local-

					let b = 3

					assert(a == 8)		//	argument-a
					assert(b == 3)		//	shadow global-b
					assert(c == 2007)	//	local-c
				}

				assert(a == 8)		//	argument-a
				assert(b == 1007)	//	global-b
				assert(c == 2007)	//	local-c

				//	Shadow argument-a
				{
					assert(a == 8)		//	argument-a
					assert(b == 1007)	//	global-b
					assert(c == 2007)	//	local-c

					let a = 4

					assert(a == 4)		//	block-local, shadowing argument-a
					assert(b == 1007)	//	global-b
					assert(c == 2007)	//	local-c
				}

				assert(a == 8)		//	argument-a
				assert(b == 1007)	//	global-b
				assert(c == 2007)	//	local-c

				return 0
			}

			test(8)
		)",
		{"1007"}
	);
}



//////////////////////////////////////////		IF STATEMENT


QUARK_UNIT_TEST("Floyd test suite", "if", "if(true){}", ""){
	ut_verify_printout_nolib(
		QUARK_POS,
		R"(

			if(true){
				print("Hello!")
			}
			print("Goodbye!")

		)",
		{ "Hello!", "Goodbye!" }
	);
}

QUARK_UNIT_TEST("Floyd test suite", "if", "if(false){}", ""){
	ut_verify_printout_nolib(
		QUARK_POS,
		R"(

			if(false){
				print("Hello!")
			}
			print("Goodbye!")

		)",
		{ "Goodbye!" }
	);
}

QUARK_UNIT_TEST("Floyd test suite", "if", "if(true){}else{}", ""){
	ut_verify_printout_nolib(
		QUARK_POS,
		R"(

			if(true){
				print("Hello!")
			}
			else{
				print("Goodbye!")
			}

		)",
		{ "Hello!" }
	);
}

QUARK_UNIT_TEST("Floyd test suite", "if", "if(false){}else{}", ""){
	ut_verify_printout_nolib(
		QUARK_POS,
		R"(

			if(false){
				print("Hello!")
			}
			else{
				print("Goodbye!")
			}

		)",
		{ "Goodbye!" }
	);
}

QUARK_UNIT_TEST("Floyd test suite", "if", "if-elseif-elseif-else TRUE-FALSE-FALSE", ""){
	ut_verify_printout_nolib(
		QUARK_POS,
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

		)",
		{ "one" }
	);
}

QUARK_UNIT_TEST("Floyd test suite", "if", "if-elseif-elseif-else FALSE-TRUE-FALSE", ""){
	ut_verify_printout_nolib(
		QUARK_POS,
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

		)",
		{ "two" }
	);
}

QUARK_UNIT_TEST("Floyd test suite", "if", "if-elseif-elseif-else FALSE-FALSE-TRUE", ""){
	ut_verify_printout_nolib(
		QUARK_POS,
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

		)",
		{ "three" }
	);
}

QUARK_UNIT_TEST("Floyd test suite", "if", "if-elseif-elseif-else FALSE-FALSE-FALSE", ""){
	ut_verify_printout_nolib(
		QUARK_POS,
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

		)",
		{ "four" }
	);
}

QUARK_UNIT_TEST("Floyd test suite", "if", "Error: if with non-bool expression", "exception"){
	ut_verify_exception_nolib(
		QUARK_POS,
		R"(

			if("not a bool"){ } else{ assert(false) }

		)",
		R"(Boolean condition required. Line: 3 "if("not a bool"){ } else{ assert(false) }")"
	);
}

QUARK_UNIT_TEST("Floyd test suite", "if+scope: Make sure a function can access global independent on how it's called in callstack", "", ""){
	ut_verify_printout_nolib(
		QUARK_POS,
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

		)",
		{ "13", "14", "1014" }
	);
}

QUARK_UNIT_TEST("Floyd test suite", "if", "return from within IF block", ""){
	ut_verify_printout_nolib(
		QUARK_POS,
		R"(

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

		)",
		{ "A", "B" }
	);
}

QUARK_UNIT_TEST("Floyd test suite", "if", "Make sure return from ELSE works", ""){
	ut_verify_printout_nolib(
		QUARK_POS,
		R"(

			func string f(){
				if(false){
					print("A")
					return "B"
					print("C")
				}
				else{
					print("F")
					return "G"
					print("H")
				}
				print("D")
				return "E"
			}
			let string x = f()
			print(x)

		)",
		{ "F", "G" }
	);
}

QUARK_UNIT_TEST("Floyd test suite", "if", "Compiler error: not all paths returns", ""){
//	QUARK_UT_VERIFY(false);
}


//////////////////////////////////////////		FOR STATEMENT



QUARK_UNIT_TEST("Floyd test suite", "for", "0 ... 0", ""){
	ut_verify_printout_nolib(
		QUARK_POS,
		R"(

			for (i in 0 ... 0) {
				print(i)
			}

		)",
		{ "0" }
	);
}

QUARK_UNIT_TEST("Floyd test suite", "for", "0 ... 1", ""){
	ut_verify_printout_nolib(
		QUARK_POS,
		R"(

			for (i in 0 ... 1) {
				print(i)
			}

		)",
		{ "0", "1" }
	);
}

QUARK_UNIT_TEST("Floyd test suite", "for", "0 ... 2", ""){
	ut_verify_printout_nolib(
		QUARK_POS,
		R"(

			for (i in 0 ... 2) {
				print(i)
			}

		)",
		{ "0", "1", "2" }
	);
}

QUARK_UNIT_TEST("Floyd test suite", "for", "0 ... 3", ""){
	ut_verify_printout_nolib(
		QUARK_POS,
		R"(

			for (i in 0 ... 3) {
				print(i)
			}

		)",
		{ "0", "1", "2", "3" }
	);
}

QUARK_UNIT_TEST("Floyd test suite", "for", "1 ... 0", ""){
	ut_verify_printout_nolib(
		QUARK_POS,
		R"(

			for (i in 1 ... 0) {
				print(i)
			}

		)",
		{ }
	);
}

QUARK_UNIT_TEST("Floyd test suite", "for", "1000 ... 1002", ""){
	ut_verify_printout_nolib(
		QUARK_POS,
		R"(

			for (i in 1000 ... 1002) {
				print(i)
			}

		)",
		{ "1000", "1001", "1002"
		}
	);
}


QUARK_UNIT_TEST("Floyd test suite", "for", "0 ..< 0", ""){
	ut_verify_printout_nolib(
		QUARK_POS,
		R"(

			for (i in 0 ..< 0) {
				print(i)
			}

		)",
		{ }
	);
}

QUARK_UNIT_TEST("Floyd test suite", "for", "0 ..< 1", ""){
	ut_verify_printout_nolib(
		QUARK_POS,
		R"(

			for (i in 0 ..< 1) {
				print(i)
			}

		)",
		{ "0" }
	);
}

QUARK_UNIT_TEST("Floyd test suite", "for", "0 ..< 2", ""){
	ut_verify_printout_nolib(
		QUARK_POS,
		R"(

			for (i in 0 ..< 2) {
				print(i)
			}

		)",
		{ "0", "1" }
	);
}

QUARK_UNIT_TEST("Floyd test suite", "for", "0 ..< 3", ""){
	ut_verify_printout_nolib(
		QUARK_POS,
		R"(

			for (i in 0 ..< 3) {
				print(i)
			}

		)",
		{ "0", "1", "2" }
	);
}

QUARK_UNIT_TEST("Floyd test suite", "for" "recusion test using fibonacci 10", "", ""){
	ut_verify_printout_nolib(
		QUARK_POS,
		R"(

			func int fibonacci(int n) {
				if (n <= 1){
					return n
				}
				return fibonacci(n - 2) + fibonacci(n - 1)
			}

			for (i in 0...10) {
				print(fibonacci(i))
			}

		)",
		{
			"0", "1", "1", "2", "3", "5", "8", "13", "21", "34",
			"55" //, "89", "144", "233", "377", "610", "987", "1597", "2584", "4181"
		}
	);
}

QUARK_UNIT_TEST("Floyd test suite", "for", "return from within FOR block", ""){
	ut_verify_printout_nolib(
		QUARK_POS,
		R"(

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

		)",
		{ "A", "B" }
	);
}



QUARK_UNIT_TEST("Floyd test suite", "for", "bool ... bool", "Error"){
	ut_verify_exception_nolib(
		QUARK_POS,
		R"(

			for (i in true ... false) {
				print(i)
			}

		)",
		R"abc(For-loop requires integer iterator, start type is bool. Line: 3 "for (i in true ... false) {")abc"
	);
}

QUARK_UNIT_TEST("Floyd test suite", "for", "EXPR ... EXPR", ""){
	ut_verify_printout_nolib(
		QUARK_POS,
		R"(

			func int magic(int x){
				return x * x;
			}

			for (i in magic(2) ... magic(3)) {
				print(i)
			}

		)",
		{ "4", "5", "6", "7", "8", "9" }
	);
}



QUARK_UNIT_TEST("Floyd test suite", "for", "nested for loops", ""){
	ut_verify_printout_nolib(
		QUARK_POS,
		R"(

			for (a in 0 ... 2) {
				print(a)
				for (b in 100 ... 102) {
					print(a * 1000 + b)
				}
			}

		)",
		{
			"0", "100", "101", "102",
			"1", "1100", "1101", "1102",
			"2", "2100", "2101", "2102"
		}
	);
}


QUARK_UNIT_TEST("Floyd test suite", "for", "print_board()", ""){
	ut_verify_printout_nolib(
		QUARK_POS,
		R"(

			let BOARD_WIDTH = 79
			let BOARD_HEIGHT = 24

			func int print_board ([[int]] board) impure {
				for (j in 0 ..< BOARD_HEIGHT) {
					mutable string row = ""
					for (i in 0 ..< BOARD_WIDTH) {
						row = push_back(row, board[i][j] != 0 ? 120 : 45)
					}
					print(row)
				}
				return 3
			}

		)",
		{
		}
	);
}



//////////////////////////////////////////		WHILE STATEMENT




QUARK_UNIT_TEST("Floyd test suite", "while", "global while", ""){
	ut_verify_printout_nolib(
		QUARK_POS,
		R"(

			mutable a = 100
			while(a < 105){
				print(a)
				a = a + 2
			}

		)",
		{ "100", "102", "104" }
	);
}




QUARK_UNIT_TEST("Floyd test suite", "while", "while inside function", ""){
	ut_verify_printout_nolib(
		QUARK_POS,
		R"(

			func int magic(int x){
				mutable i = x
				while(i != 9){
					print(i)
					i = i + 1
				}
				return i
			}

			let a = magic(5)

		)",
		{ "5", "6", "7", "8" }
	);
}

QUARK_UNIT_TEST("Floyd test suite", "while", "Error: while with non-bool condition", ""){
	ut_verify_exception_nolib(
		QUARK_POS,
		R"(

			mutable a = 100
			while(105){
				print(a)
				a = a + 2
			}

		)",
		R"abc(Expression type mismatch - cannot convert 'int' to 'bool. Line: 4 "while(105){")abc"
	);
}

QUARK_UNIT_TEST("Floyd test suite", "while", "return from within while", ""){
	ut_verify_printout_nolib(
		QUARK_POS,
		R"(

			func int magic(int x){
				mutable i = x
				while(i != 9){
					print(i)
					if(i == 7){
						return 3
					}
					i = i + 1
				}
				return i
			}

			let a = magic(5)
			print(a)

		)",
		{ "5", "6", "7", "3" }
	);
}




//////////////////////////////////////////		TYPEID - TYPE







//////////////////////////////////////////		STRING - TYPE




QUARK_UNIT_TEST("Floyd test suite", "string constructor", "", ""){
	ut_verify_global_result_nolib(QUARK_POS, "let result = string(\"ABCD\")", value_t::make_string("ABCD"));
}

QUARK_UNIT_TEST("Floyd test suite", "string constructor", "", ""){
	ut_verify_global_result_nolib(QUARK_POS, "let result = \"ABCD\"", value_t::make_string("ABCD"));
}

QUARK_UNIT_TEST("Floyd test suite", "string literal", "Escape \0", ""){
	ut_verify_global_result_nolib(QUARK_POS, R"___(let result = "\0")___", value_t::make_string(std::string(1, '\0')));
}
QUARK_UNIT_TEST("Floyd test suite", "string literal", "Escape \t", ""){
	ut_verify_global_result_nolib(QUARK_POS, R"___(let result = "\t")___", value_t::make_string("\t"));
}
QUARK_UNIT_TEST("Floyd test suite", "string literal", "Escape \\", ""){
	ut_verify_global_result_nolib(QUARK_POS, R"___(let result = "\\")___", value_t::make_string("\\"));
}
QUARK_UNIT_TEST("Floyd test suite", "string literal", "Escape \n", ""){
	ut_verify_global_result_nolib(QUARK_POS, R"___(let result = "\n")___", value_t::make_string("\n"));
}
QUARK_UNIT_TEST("Floyd test suite", "string literal", "Escape \r", ""){
	ut_verify_global_result_nolib(QUARK_POS, R"___(let result = "\r")___", value_t::make_string("\r"));
}
QUARK_UNIT_TEST("Floyd test suite", "string literal", "Escape \"", ""){
	ut_verify_global_result_nolib(QUARK_POS, R"___(let result = "\"")___", value_t::make_string("\""));
}
QUARK_UNIT_TEST("Floyd test suite", "string literal", "Escape \'", ""){
	ut_verify_global_result_nolib(QUARK_POS, R"___(let result = "\'")___", value_t::make_string("\'"));
}

QUARK_UNIT_TEST("Floyd test suite", "string =", "", ""){
	run_closed(R"(

		let a = "hello"
		let b = a
		assert(b == "hello")

	)");
}

QUARK_UNIT_TEST("Floyd test suite", "string +", "", ""){
	run_closed(R"(		assert("a" + "b" == "ab")		)");
}
QUARK_UNIT_TEST("Floyd test suite", "string +", "", ""){
	run_closed(R"(		assert("a" + "b" + "c" == "abc")		)");
}



QUARK_UNIT_TEST("Floyd test suite", "string ==", "", ""){
	run_closed(R"(		assert(("hello" == "hello") == true)		)");
}
QUARK_UNIT_TEST("Floyd test suite", "string ==", "", ""){
	run_closed(R"(		assert(("hello" == "Yello") == false)		)");
}
QUARK_UNIT_TEST("Floyd test suite", "string !=", "", ""){
	run_closed(R"(		assert(("hello" != "yello") == true)		)");
}
QUARK_UNIT_TEST("Floyd test suite", "string !=", "", ""){
	run_closed(R"(		assert(("hello" != "hello") == false)		)");
}


QUARK_UNIT_TEST("Floyd test suite", "string <", "", ""){
	run_closed(R"(		assert(("aaa" < "bbb") == true)		)");
}
QUARK_UNIT_TEST("Floyd test suite", "string <", "", ""){
	run_closed(R"(		assert(("aaa" < "aaa") == false)		)");
}
QUARK_UNIT_TEST("Floyd test suite", "string <", "", ""){
	run_closed(R"(		assert(("bbb" < "aaa") == false)		)");
}


QUARK_UNIT_TEST("Floyd test suite", "string <=", "", ""){
	run_closed(R"(		assert(("aaa" <= "bbb") == true)		)");
}
QUARK_UNIT_TEST("Floyd test suite", "string <=", "", ""){
	run_closed(R"(		assert(("aaa" <= "aaa") == true)		)");
}
QUARK_UNIT_TEST("Floyd test suite", "string <=", "", ""){
	run_closed(R"(		assert(("bbb" <= "aaa") == false)		)");
}


QUARK_UNIT_TEST("Floyd test suite", "string >", "", ""){
	run_closed(R"(		assert(("bbb" > "aaa") == true)		)");
}
QUARK_UNIT_TEST("Floyd test suite", "string >", "", ""){
	run_closed(R"(		assert(("aaa" > "aaa") == false)		)");
}
QUARK_UNIT_TEST("Floyd test suite", "string >", "", ""){
	run_closed(R"(		assert(("aaa" > "bbb") == false)		)");
}


QUARK_UNIT_TEST("Floyd test suite", "string >=", "", ""){
	run_closed(R"(		assert(("bbb" >= "aaa") == true)		)");
}
QUARK_UNIT_TEST("Floyd test suite", "string >=", "", ""){
	run_closed(R"(		assert(("aaa" >= "aaa") == true)		)");
}
QUARK_UNIT_TEST("Floyd test suite", "string >=", "", ""){
	run_closed(R"(		assert(("aaa" >= "bbb") == false)		)");
}



QUARK_UNIT_TEST("Floyd test suite", "string []", "", ""){
	ut_verify_printout_nolib(
		QUARK_POS,
		R"(

			let message = "hello"
			print(message[0])
			print(message[1])
			print(message[2])
			print(message[3])
			print(message[4])

		)",
		{ "104", "101", "108", "108", "111" }
	);
}

QUARK_UNIT_TEST("Floyd test suite", "string []", "", ""){
	run_closed(R"(		assert("hello"[0] == 104)		)");
}

QUARK_UNIT_TEST("Floyd test suite", "string []", "", ""){
	run_closed(R"(		assert("hello"[4] == 111)		)");
}

QUARK_UNIT_TEST("Floyd test suite", "string", "Error: Lookup in string using non-int", "exception"){
	ut_verify_exception_nolib(
		QUARK_POS,
		R"(

			let string a = "test string"
			print(a["not an integer"])

		)",
		"Strings can only be indexed by integers, not a \"string\". Line: 4 \"print(a[\"not an integer\"])\""
	);
}


/*
(print())
update()
size()
find()
push_back()
subset()
replace()
*/

QUARK_UNIT_TEST("Floyd test suite", "string update()", "", ""){
	run_closed(R"(

		let a = update("hello", 1, 98)
		assert(a == "hbllo")

	)");
}
QUARK_UNIT_TEST("Floyd test suite", "string update()", "error: pos > len", "exception"){
	ut_verify_exception_nolib(
		QUARK_POS,
		R"(

			let a = update("hello", 5, 98)

		)",
		"Position argument to update() is outside collection span."
	);
}



QUARK_UNIT_TEST("Floyd test suite", "string size()", "", ""){
	run_closed(R"(		assert(size("") == 0)		)");
}
QUARK_UNIT_TEST("Floyd test suite", "string size()", "", ""){
	run_closed(R"(		assert(size("How long is this string?") == 24)		)");
}

QUARK_UNIT_TEST("Floyd test suite", "string size()", "Embeded null characters - check 8 bit clean", ""){
	run_closed(R"(		assert(size("hello\0world\0\0") == 13)		)");
}



//??? find() should have a start index.
QUARK_UNIT_TEST("Floyd test suite", "string find()", "", ""){
	run_closed(R"(		assert(find("hello, world", "he") == 0)		)");
}
QUARK_UNIT_TEST("Floyd test suite", "string find()", "", ""){
	run_closed(R"(		assert(find("hello, world", "e") == 1)		)");
}
QUARK_UNIT_TEST("Floyd test suite", "string find()", "", ""){
	run_closed(R"(		assert(find("hello, world", "x") == -1)		)");
}


//??? Add character-literal / type.
QUARK_UNIT_TEST("Floyd test suite", "string push_back()", "", ""){
	run_closed(R"(

		let a = push_back("one", 111)
		assert(a == "oneo")

	)");
}



QUARK_UNIT_TEST("Floyd test suite", "string subset()", "string", ""){
	run_closed(R"(		assert(subset("abc", 0, 3) == "abc")		)");
}
QUARK_UNIT_TEST("Floyd test suite", "string subset()", "string", ""){
	run_closed(R"(		assert(subset("abc", 1, 3) == "bc")		)");
}
QUARK_UNIT_TEST("Floyd test suite", "string subset()", "string", ""){
	run_closed(R"(		assert(subset("abc", 0, 0) == "")		)");
}
QUARK_UNIT_TEST("Floyd test suite", "string subset()", "string", ""){
	run_closed(R"(		assert(subset("abc", 0, 10) == "abc")		)");
}
QUARK_UNIT_TEST("Floyd test suite", "string subset()", "string", ""){
	run_closed(R"(		assert(subset("abc", 2, 10) == "c")		)");
}




QUARK_UNIT_TEST("Floyd test suite", "string replace()", "", ""){
	run_closed(R"(		assert(replace("One ring to rule them all", 4, 8, "rabbit") == "One rabbit to rule them all")		)");
}
QUARK_UNIT_TEST("Floyd test suite", "string replace()", "", ""){
	run_closed(R"(		assert(replace("hello", 0, 5, "goodbye") == "goodbye")		)");
}
QUARK_UNIT_TEST("Floyd test suite", "string replace()", "", ""){
	run_closed(R"(		assert(replace("hello", 0, 6, "goodbye") == "goodbye")		)");
}
QUARK_UNIT_TEST("Floyd test suite", "string replace()", "", ""){
	run_closed(R"(		assert(replace("hello", 0, 0, "goodbye") == "goodbyehello")		)");
}
QUARK_UNIT_TEST("Floyd test suite", "string replace()", "", ""){
	run_closed(R"(		assert(replace("hello", 5, 5, "goodbye") == "hellogoodbye")		)");
}
QUARK_UNIT_TEST("Floyd test suite", "string replace()", "", "error"){
	ut_verify_exception_nolib(QUARK_POS, R"(		assert(replace("hello", 5, 0, "goodbye") == "hellogoodbye")		)", "replace() requires start <= end.");
}
QUARK_UNIT_TEST("Floyd test suite", "string replace()", "", "error"){
	ut_verify_exception_nolib(QUARK_POS, R"(		assert(replace("hello", 2, 3, 666) == "")		)", "replace() requires argument 4 to be same type of collection.");
}









//////////////////////////////////////////		VECTOR


/*
Constructor expression
Lookup
Assign
Concatunation operator +
Comparisons x 6

print()
update()		-- string, vector, dict
size()			-- string, vector, dict
find()			-- string/vector. ??? dict?
exists()		-- dict. ??? vector?
erase()			--	dict. ??? also vector?
push_back()		-- string, vector
subset()		-- string vector
replace()		-- string, vector
typeof()
*/


QUARK_UNIT_TEST("Floyd test suite", "vector [] - empty constructor", "cannot be infered", "error"){
	ut_verify_exception_nolib(
		QUARK_POS,
		R"(

			let a = []
			print(a)

		)",
		"Cannot infer vector element type, add explicit type. Line: 3 \"let a = []\""
	);
}



//////////////////////////////////////////		VECTOR STRING

//	Vector-string is the first vector-type we test. It does extensive testing of type inferrence etc. Other vector-types don't need to test all that.


QUARK_UNIT_TEST("Floyd test suite", "vector [string] - constructor", "Infer type", "valid vector"){
	ut_verify_printout_nolib(
		QUARK_POS,
		R"(

			let [string] a = ["one", "two"]
			print(a)

		)",
		{ R"(["one", "two"])" }
	);
}

QUARK_UNIT_TEST("Floyd test suite", "vector [string] - constructor", "Infer type", "valid vector"){
	ut_verify_printout_nolib(
		QUARK_POS,
		R"(

			let a = ["one", "two"]
			print(a)

		)",
		{ R"(["one", "two"])" }
	);
}

QUARK_UNIT_TEST("Floyd test suite", "vector [string] - constructor", "empty vector", "valid vector"){
	ut_verify_printout_nolib(
		QUARK_POS,
		R"(

			let [string] a = []
			print(a)

		)",
		{ R"([])" }
	);
}

//	We could support this if we had special type for empty-vector and empty-dict.
QUARK_UNIT_TEST("Floyd test suite", "vector ==", "lhs and rhs are empty-typeless", ""){
	ut_verify_exception_nolib(QUARK_POS, R"(		assert(([] == []) == true)		)", "Cannot infer vector element type, add explicit type. Line: 1 \"assert(([] == []) == true)\"");
}

QUARK_UNIT_TEST("Floyd test suite", "vector [string] =", "copy", ""){
	ut_verify_printout_nolib(
		QUARK_POS,
		R"(

			let a = ["one", "two"]
			let b = a
			assert(a == b)
			print(a)
			print(b)

		)",
		{ R"(["one", "two"])", R"(["one", "two"])" }
	);
}

QUARK_UNIT_TEST("Floyd test suite", "vector +", "add empty vectors", ""){
	ut_verify_exception_nolib(QUARK_POS, R"(		let [int] a = [] + [] result = a == []		)", R"(Cannot infer vector element type, add explicit type. Line: 1 "let [int] a = [] + [] result = a == []")");
}

#if 0
//??? This fails but should not. This code becomes a constructor call to [int] with more than 16 arguments. Byte code interpreter has 16 argument limit.
QUARK_UNIT_TEST("Floyd test suite", "vector [] - constructor", "32 elements initialization", ""){
	run_closed(R"(

		let a = [ 0,0,1,1,0,0,0,0,	0,0,1,1,0,0,0,0,	0,0,1,1,1,0,0,0,	0,0,1,1,1,1,0,0 ]
		print(a)

	)");
}
#endif

QUARK_UNIT_TEST("Floyd test suite", "vector [string] constructor expression, computed element", "", ""){
	ut_verify_global_result_as_json_nolib(
		QUARK_POS,
		R"(

			func string get_beta(){ return "beta" }
			let [string] result = ["alpha", get_beta()]

		)",
		R"(		[[ "vector", "^string" ], ["alpha","beta"]]		)"
	);
}




QUARK_UNIT_TEST("Floyd test suite", "vector [string] =", "", ""){
	ut_verify_printout_nolib(
		QUARK_POS,
		R"(

			let a = ["one", "two"]
			let b = a
			assert(a == b)
			print(a)
			print(b)

		)",
		{ R"(["one", "two"])", R"(["one", "two"])" }
	);
}


QUARK_UNIT_TEST("Floyd test suite", "vector [string] +", "non-empty vectors", ""){
	run_closed(R"(

		let [string] a = ["one"] + ["two"]
		assert(a == ["one", "two"])

	)");
}


QUARK_UNIT_TEST("Floyd test suite", "vector [string] ==", "same values", ""){
	run_closed(R"(		assert((["one", "two"] == ["one", "two"]) == true)		)");
}

QUARK_UNIT_TEST("Floyd test suite", "vector [string] ==", "different values", ""){
	run_closed(R"(		assert((["one", "three"] == ["one", "two"]) == false)		)");
}

QUARK_UNIT_TEST("Floyd test suite", "vector [string] <", "same values", ""){
	run_closed(R"(		assert((["one", "two"] < ["one", "two"]) == false)		)");
}

QUARK_UNIT_TEST("Floyd test suite", "vector [string] <", "different values", ""){
	run_closed(R"(		assert((["one", "a"] < ["one", "two"]) == true)		)");
}



QUARK_UNIT_TEST("Floyd test suite", "vector [string] [] lookup", "", ""){
	ut_verify_printout_nolib(
		QUARK_POS,
		R"(

			let a = ["one", "two"]
			print(a[0])
			print(a[1])

		)",
		{ "one", "two" }
	);
}



QUARK_UNIT_TEST("Floyd test suite", "vector [string] print()", "empty", "[]"){
	ut_verify_printout_nolib(
		QUARK_POS,
		R"(

			let [string] a = []
			print(a)

		)",
		{
			"[]"
		}
	);
}
QUARK_UNIT_TEST("Floyd test suite", "vector [string] print()", "2 elements", ""){
	ut_verify_printout_nolib(
		QUARK_POS,
		R"(

			let a = ["one", "two", "three"]
			print(a)

		)",
		{
			R"(["one", "two", "three"])"
		}
	);
}

//??? test bounds error
QUARK_UNIT_TEST("Floyd test suite", "vector [string] update()", "", "valid vector, without side effect on original vector"){
	ut_verify_printout_nolib(
		QUARK_POS,
		R"(

			let a = [ "one", "two", "three"]
			let b = update(a, 1, "zwei")
			print(a)
			print(b)
			assert(a == ["one","two","three"])
			assert(b == ["one","zwei","three"])

		)",
		{
			R"(["one", "two", "three"])",
			R"(["one", "zwei", "three"])"
		}
	);
}

QUARK_UNIT_TEST("Floyd test suite", "vector [string] size()", "empty", "0"){
	run_closed(R"(

		let [string] a = []
		assert(size(a) == 0)

	)");
}
QUARK_UNIT_TEST("Floyd test suite", "vector [string] size()", "2", ""){
	run_closed(R"(

		let [string] a = ["one", "two"]
		assert(size(a) == 2)

	)");
}



QUARK_UNIT_TEST("Floyd test suite", "vector [string] find()", "", ""){
	run_closed(R"(		assert(find(["one", "two", "three", "four"], "one") == 0)		)");
}
QUARK_UNIT_TEST("Floyd test suite", "vector [string] find()", "", ""){
	run_closed(R"(		assert(find(["one", "two", "three", "four"], "two") == 1)		)");
}
QUARK_UNIT_TEST("Floyd test suite", "vector [string] find()", "", ""){
	run_closed(R"(		assert(find(["one", "two", "three", "four"], "five") == -1)		)");
}



QUARK_UNIT_TEST("Floyd test suite", "vector [string] push_back()", "", ""){
	run_closed(R"(

		let [string] a = ["one"];
		let [string] b = push_back(a, "two")
		let [string] c = ["one", "two"];
		assert(b == c)

	)");
}

QUARK_UNIT_TEST("Floyd test suite", "vector [string] push_back()", "", ""){
	run_closed(R"(

		let [string] a = push_back(["one"], "two")
		assert(a == ["one", "two"])

	)");
}

QUARK_UNIT_TEST("Floyd test suite", "vector [string] subset()", "", ""){
	run_closed(R"(		assert(subset(["one", "two", "three"], 0, 3) == ["one", "two", "three"])		)");
}
QUARK_UNIT_TEST("Floyd test suite", "vector [string] subset()", "", ""){
	run_closed(R"(		assert(subset(["one", "two", "three"], 1, 3) == ["two", "three"])		)");
}
QUARK_UNIT_TEST("Floyd test suite", "vector [string] subset()", "", ""){
	run_closed(R"(		assert(subset(["one", "two", "three"], 0, 0) == [])		)");
}
QUARK_UNIT_TEST("Floyd test suite", "vector [string] subset()", "", ""){
	run_closed(R"(		assert(subset(["one", "two", "three"], 0, 10) == ["one", "two", "three"])		)");
}
QUARK_UNIT_TEST("Floyd test suite", "vector [string] subset()", "", ""){
	run_closed(R"(		assert(subset(["one", "two", "three"], 2, 10) == ["three"])		)");
}




QUARK_UNIT_TEST("Floyd test suite", "vector [string] replace()", "", ""){
	run_closed(R"(		assert(replace(["one", "two", "three", "four", "five"], 0, 5, ["goodbye"]) == ["goodbye"])		)");
}
QUARK_UNIT_TEST("Floyd test suite", "vector [string] replace()", "", ""){
	run_closed(R"(		assert(replace(["one", "two", "three", "four", "five"], 0, 6, ["goodbye"]) == ["goodbye"])		)");
}
QUARK_UNIT_TEST("Floyd test suite", "vector [string] replace()", "", ""){
	run_closed(R"(		assert(replace(["one", "two", "three", "four", "five"], 0, 0, ["goodbye"]) == ["goodbye", "one", "two", "three", "four", "five"])		)");
}
QUARK_UNIT_TEST("Floyd test suite", "vector [string] replace()", "", ""){
	run_closed(R"(		assert(replace(["one", "two", "three", "four", "five"], 5, 5, ["goodbye"]) == ["one", "two", "three", "four", "five", "goodbye"])		)");
}
QUARK_UNIT_TEST("Floyd test suite", "vector [string] replace()", "", "error"){
	ut_verify_exception_nolib(QUARK_POS, R"(		assert(replace(["one", "two", "three", "four", "five"], 5, 0, ["goodbye"]) == ["hellogoodbye"])		)", "replace() requires start <= end.");
}
QUARK_UNIT_TEST("Floyd test suite", "vector [string] replace()", "", "error"){
	ut_verify_exception_nolib(QUARK_POS, R"(		assert(replace(["one", "two", "three", "four", "five"], 2, 3, 666) == [])		)", "replace() requires argument 4 to be same type of collection.");
}


//??? test vector<struct>, vector<json_value>


//////////////////////////////////////////		vector-bool


QUARK_UNIT_TEST("Floyd test suite", "vector [bool] construct-expression", "", ""){
	ut_verify_global_result_as_json_nolib(QUARK_POS, R"(		let [bool] result = [true, false, true]		)",				R"(		[[ "vector", "^bool" ], [true, false, true]]		)");
}
QUARK_UNIT_TEST("Floyd test suite", "vector [bool] =", "copy", ""){
	ut_verify_global_result_as_json_nolib(QUARK_POS, R"(		let a = [true, false, true] let result = a		)",				R"(		[[ "vector", "^bool" ], [true, false, true]]		)");
}
QUARK_UNIT_TEST("Floyd test suite", "vector [bool] ==", "same values", ""){
	ut_verify_global_result_as_json_nolib(QUARK_POS, R"(		let result = [true, false] == [true, false]		)",			R"(		[ "^bool", true]		)" );
}
QUARK_UNIT_TEST("Floyd test suite", "vector [bool] ==", "different values", ""){
	ut_verify_global_result_as_json_nolib(QUARK_POS, R"(		let result = [false, false] == [true, false]		)", 	R"(		[ "^bool", false]		)" );
}
QUARK_UNIT_TEST("Floyd test suite", "vector [bool] <", "", ""){
	ut_verify_global_result_as_json_nolib(QUARK_POS, R"(		let result = [true, true] < [true, true]		)",			R"(		[ "^bool", false]	)");
}
QUARK_UNIT_TEST("Floyd test suite", "vector [bool] <", "different values", ""){
	ut_verify_global_result_as_json_nolib(QUARK_POS, R"(		let result = [true, false] < [true, true]		)", 		R"(		[ "^bool", true]		)");
}
QUARK_UNIT_TEST("Floyd test suite", "vector [bool] +", "non-empty vectors", ""){
	ut_verify_global_result_as_json_nolib(QUARK_POS, R"(		let [bool] result = [true, false] + [true, true]		)", R"(		[[ "vector", "^bool" ], [true, false, true, true]]		)");
}

//???	update()		-- string, vector, dict

QUARK_UNIT_TEST("Floyd test suite", "vector [bool] size()", "empty", "0"){
	ut_verify_global_result_as_json_nolib(QUARK_POS, R"(		let [bool] a = [] let result = size(a)		)",					R"(		[ "^int", 0]		)");
}

QUARK_UNIT_TEST("Floyd test suite", "vector [bool] size()", "2", ""){
	ut_verify_global_result_as_json_nolib(QUARK_POS, R"(		let [bool] a = [true, false, true] let result = size(a)		)", R"(		[ "^int", 3]		)");
}

//??? find()

QUARK_UNIT_TEST("Floyd test suite", "vector [bool] push_back()", "", ""){
	ut_verify_global_result_as_json_nolib(QUARK_POS, R"(		let [bool] result = push_back([true, false], true)		)", R"(		[[ "vector", "^bool" ], [true, false, true]]		)");
}

//???	subset()		-- string vector
//???	replace()		-- string, vector


//////////////////////////////////////////		vector-int


QUARK_UNIT_TEST("Floyd test suite", "vector [int] constructor expression", "", ""){
	ut_verify_global_result_as_json_nolib(QUARK_POS, R"(		let [int] result = [10, 20, 30]		)",		R"(		[[ "vector", "^int" ], [10, 20, 30]]		)" );
}
QUARK_UNIT_TEST("Floyd test suite", "vector [int] constructor", "", "3"){
	ut_verify_global_result_nolib(
		QUARK_POS,
		R"(

			let [int] a = [1, 2, 3]
			let result = size(a)

		)",
		value_t::make_int(3)
	);
}
QUARK_UNIT_TEST("Floyd test suite", "vector [string] [] lookup", "", ""){
	ut_verify_printout_nolib(
		QUARK_POS,
		R"(

			let a = [5, 6, 7]
			print(a[0])
			print(a[1])

		)",
		{ "5", "6" }
	);
}

QUARK_UNIT_TEST("Floyd test suite", "vector [int] =", "copy", ""){
	ut_verify_global_result_as_json_nolib(QUARK_POS, R"(		let a = [10, 20, 30] let result = a;		)",	R"(		[[ "vector", "^int" ], [10, 20, 30]]		)"	);
}
QUARK_UNIT_TEST("Floyd test suite", "vector [int] ==", "same values", ""){
	ut_verify_global_result_as_json_nolib(QUARK_POS, R"(		let result = [1, 2] == [1, 2]		)",		R"(		[ "^bool", true]		)");
	ut_verify_global_result_as_json_nolib(QUARK_POS, R"(		let result = [1, 2] == [1, 2]		)",		R"(		[ "^bool", true]		)");
}
QUARK_UNIT_TEST("Floyd test suite", "vector [int] ==", "different values", ""){
	ut_verify_global_result_as_json_nolib(QUARK_POS, R"(		let result = [1, 3] == [1, 2]		)",		R"(		[ "^bool", false]		)");
}
QUARK_UNIT_TEST("Floyd test suite", "vector [int] <", "", ""){
	ut_verify_global_result_as_json_nolib(QUARK_POS, R"(		let result = [1, 2] < [1, 2]		)",		R"(		[ "^bool", false]	)");
}
QUARK_UNIT_TEST("Floyd test suite", "vector [int] <", "different values", ""){
	ut_verify_global_result_as_json_nolib(QUARK_POS, R"(		let result = [1, 2] < [1, 3]		)",		R"(		[ "^bool", true]		)");
}

QUARK_UNIT_TEST("Floyd test suite", "vector [int] +", "non-empty vectors", ""){
	ut_verify_global_result_as_json_nolib(QUARK_POS, R"(		let [int] result = [1, 2] + [3, 4]		)",	R"(		[[ "vector", "^int" ], [1, 2, 3, 4]]		)");
}

//???	print()
//???	update()		-- string, vector, dict

QUARK_UNIT_TEST("Floyd test suite", "vector [int] size()", "empty", "0"){
	ut_verify_global_result_as_json_nolib(QUARK_POS, R"(		let [int] a = [] let result = size(a)		)",	R"(		[ "^int", 0]		)");
}
QUARK_UNIT_TEST("Floyd test suite", "vector [int] size()", "2", ""){
	ut_verify_global_result_as_json_nolib(QUARK_POS, R"(		let [int] a = [1, 2, 3] let result = size(a)		)",		R"(		[ "^int", 3]		)");
}


QUARK_UNIT_TEST("Floyd test suite", "vector [int] find()", "", ""){
	run_closed(R"(		assert(find([1,2,3], 4) == -1)		)");
}
QUARK_UNIT_TEST("Floyd test suite", "vector [int] find()", "", ""){
	run_closed(R"(		assert(find([1,2,3], 1) == 0)		)");
}
QUARK_UNIT_TEST("Floyd test suite", "vector [int] find()", "", ""){
	run_closed(R"(		assert(find([1,2,2,2,3], 2) == 1)		)");
}

QUARK_UNIT_TEST("Floyd test suite", "vector [int] push_back()", "", ""){
	ut_verify_global_result_as_json_nolib(QUARK_POS, R"(		let [int] result = push_back([1, 2], 3)		)",		R"(		[[ "vector", "^int" ], [1, 2, 3]]		)");
}

QUARK_UNIT_TEST("Floyd test suite", "vector [int] push_back()", "", ""){
	run_closed(
		R"(

			let result = push_back([1, 2], 3)
			assert(result == [1, 2, 3])

		)"
	);
}

QUARK_UNIT_TEST("Floyd test suite", "vector [int] subset()", "", ""){
	run_closed(R"(		assert(subset([10,20,30], 0, 3) == [10,20,30])		)");
}
QUARK_UNIT_TEST("Floyd test suite", "vector [int] subset()", "", ""){
	run_closed(R"(		assert(subset([10,20,30], 1, 3) == [20,30])		)");
}
QUARK_UNIT_TEST("Floyd test suite", "vector [int] subset()", "", ""){
	run_closed(R"(		let result = (subset([10,20,30], 0, 0) == [])		)");
}
QUARK_UNIT_TEST("Floyd test suite", "vector [int] subset()", "", ""){
	run_closed(R"(		assert(subset([10,20,30], 0, 0) == [])		)");
}

QUARK_UNIT_TEST("Floyd test suite", "vector [int] replace()", "", ""){
	run_closed(R"(		assert(replace([ 1, 2, 3, 4, 5, 6 ], 2, 5, [20, 30]) == [1, 2, 20, 30, 6])		)");
}

QUARK_UNIT_TEST("Floyd test suite", "vector [int] replace()", "", ""){
	run_closed(
		R"(

			let h = replace([ 1, 2, 3, 4, 5 ], 1, 4, [ 8, 9 ])
			assert(h == [ 1, 8, 9, 5 ])

		)"
	);
}

// ### test pos limiting and edge cases.




//////////////////////////////////////////		vector-double



QUARK_UNIT_TEST("Floyd test suite", "vector [double] constructor-expression", "", ""){
	ut_verify_global_result_as_json_nolib(QUARK_POS, R"(		let [double] result = [10.5, 20.5, 30.5]		)",	R"(		[[ "vector", "^double" ], [10.5, 20.5, 30.5]]		)");
}
QUARK_UNIT_TEST("Floyd test suite", "vector", "Vector can not hold elements of different types.", "exception"){
	ut_verify_exception_nolib(QUARK_POS, R"(		let a = [3, bool]		)", "Vector of type [int] cannot hold an element of type typeid. Line: 1 \"let a = [3, bool]\"");
}


QUARK_UNIT_TEST("Floyd test suite", "vector []", "Error: Lookup in vector using non-int", "exception"){
	ut_verify_exception_nolib(
		QUARK_POS,
		R"(

			let [string] a = ["one", "two", "three"]
			print(a["not an integer"])

		)",
		"Vector can only be indexed by integers, not a \"string\". Line: 4 \"print(a[\"not an integer\"])\""
	);
}
QUARK_UNIT_TEST("Floyd test suite", "vector", "Error: Lookup the unlookupable", "exception"){
	ut_verify_exception_nolib(QUARK_POS, R"(		let a = 3[0]		)", "Lookup using [] only works with strings, vectors, dicts and json_value - not a \"int\". Line: 1 \"let a = 3[0]\"");
}

QUARK_UNIT_TEST("Floyd test suite", "vector [double] =", "copy", ""){
	ut_verify_global_result_as_json_nolib(QUARK_POS, R"(		let a = [10.5, 20.5, 30.5] let result = a		)",	R"(		[[ "vector", "^double" ], [10.5, 20.5, 30.5]]		)");
}
QUARK_UNIT_TEST("Floyd test suite", "vector [double] ==", "same values", ""){
	ut_verify_global_result_as_json_nolib(QUARK_POS, R"(		let result = [1.5, 2.5] == [1.5, 2.5]		)",	R"(		[ "^bool", true]		)");
}
QUARK_UNIT_TEST("Floyd test suite", "vector [double] ==", "different values", ""){
	ut_verify_global_result_as_json_nolib(QUARK_POS, R"(		let result = [1.5, 3.5] == [1.5, 2.5]		)",	R"(		[ "^bool", false]		)");
}
QUARK_UNIT_TEST("Floyd test suite", "vector [double] <", "", ""){
	ut_verify_global_result_as_json_nolib(QUARK_POS, R"(		let result = [1.5, 2.5] < [1.5, 2.5]		)",	R"(		[ "^bool", false]	)");
}
QUARK_UNIT_TEST("Floyd test suite", "vector [double] <", "different values", ""){
	ut_verify_global_result_as_json_nolib(QUARK_POS, R"(		let result = [1.5, 2.5] < [1.5, 3.5]		)",	R"(		[ "^bool", true]		)");
}

QUARK_UNIT_TEST("Floyd test suite", "vector [double] +", "non-empty vectors", ""){
	ut_verify_global_result_as_json_nolib(QUARK_POS, R"(		let [double] result = [1.5, 2.5] + [3.5, 4.5]		)",		R"(		[[ "vector", "^double" ], [1.5, 2.5, 3.5, 4.5]]		)");
}

//???	print()
//???	update()		-- string, vector, dict

QUARK_UNIT_TEST("Floyd test suite", "vector [double] size()", "empty", "0"){
	ut_verify_global_result_as_json_nolib(QUARK_POS, R"(		let [double] a = [] let result = size(a)		)",	R"(		[ "^int", 0]		)");
}
QUARK_UNIT_TEST("Floyd test suite", "vector [double] size()", "2", ""){
	ut_verify_global_result_as_json_nolib(QUARK_POS, R"(		let [double] a = [1.5, 2.5, 3.5] let result = size(a)		)",	R"(		[ "^int", 3]		)");
}

//???	find()			-- string/vector. ??? dict?

QUARK_UNIT_TEST("Floyd test suite", "vector [double] push_back()", "", ""){
	ut_verify_global_result_as_json_nolib(QUARK_POS, R"(		let [double] result = push_back([1.5, 2.5], 3.5)		)",	R"(		[[ "vector", "^double" ], [1.5, 2.5, 3.5]]		)");
}

//???	subset()		-- string vector
//???	replace()		-- string, vector



//??? test json_value element type
//??? test struct element type
//??? test typeid element type
//??? test function element type
//??? test vector element type
//??? test dict element type



QUARK_UNIT_TEST("Floyd test suite", "vector [bool] constructor", "", ""){
	run_closed(R"(		assert(to_string([true, false, true]) == "[true, false, true]")		)");
}
QUARK_UNIT_TEST("Floyd test suite", "vector [int] constructor", "", ""){
	run_closed(R"(		assert(to_string([1, 2, 3]) == "[1, 2, 3]")		)");
}
QUARK_UNIT_TEST("Floyd test suite", "vector [double] constructor", "", ""){
	run_closed(R"(		assert(to_string([1.0, 2.0, 3.0]) == "[1.0, 2.0, 3.0]")		)");
}
QUARK_UNIT_TEST("Floyd test suite", "vector [double] constructor", "", ""){
	run_closed(R"(		assert(to_string(["one", "two", "three"]) == "[\"one\", \"two\", \"three\"]")		)");
}

QUARK_UNIT_TEST("Floyd test suite", "vector [double] constructor", "", ""){
	run_closed(R"(		assert(to_string([int, bool, string]) == "[int, bool, string]")		)");
}

QUARK_UNIT_TEST("Floyd test suite", "vector [typeid] constructor", "", ""){
	ut_verify_printout_nolib(
		QUARK_POS,
		R"(
			let a = typeof(3)
			let b = typeof(true)
			let c = typeof("str")
			print(a)
			print(b)
			print(c)

			let d = [a, b, c]
			print(d)
		)",
		{
			"int",
			"bool",
			"string",
			"[int, bool, string]"
		 }
	);
}

#if 0
QUARK_UNIT_TEST("Floyd test suite", "vector [func] constructor", "", ""){
	ut_verify_printout_nolib(
		QUARK_POS,
		R"(
			func int a(string s){
				return 2
			}

			func int b(string s){
				return 3
			}

			func int c(string s){
				return 4
			}

			let d = [a, b, c]
			print(d)
		)",
		{
			"int",
			"bool",
			"string",
			"[int, bool, string]"
		 }
	);
}
#endif


//////////////////////////////////////////		DICT - TYPE


QUARK_UNIT_TEST("Floyd test suite", "dict construct", "", ""){
	run_closed(R"(

		let [string: int] a = {"one": 1, "two": 2}

	)");
}
QUARK_UNIT_TEST("Floyd test suite", "dict construct", "", ""){
	run_closed(R"(

		let [string: int] a = {"one": 1, "two": 2}
		assert(size(a) == 2)

	)");
}
QUARK_UNIT_TEST("Floyd test suite", "dict<string:int> []", "", ""){
	ut_verify_printout_nolib(
		QUARK_POS,
		R"(

			let [string: int] a = {"one": 1, "two": 2}
			print(a["one"])
			print(a["two"])

		)",
		{ "1", "2" }
	);
}

QUARK_UNIT_TEST("Floyd test suite", "dict<string:int> ", "", ""){
	ut_verify_exception_nolib(QUARK_POS,
		R"(

			mutable a = {}
			a = {"hello": 1}
			print(a)

		)",
		"Cannot infer type in construct-value-expression. Line: 3 \"mutable a = {}\""
	);
}

QUARK_UNIT_TEST("Floyd test suite", "dict constructor", "No type", "error"){
	ut_verify_exception_nolib(
		QUARK_POS,
		R"(

			let a = {}
			print(a)

		)",
		"Cannot infer type in construct-value-expression. Line: 3 \"let a = {}\""
	);
}


QUARK_UNIT_TEST("Floyd test suite", "dict<string:int> {}", "{}", "Infered type"){
	ut_verify_printout_nolib(
		QUARK_POS,
		R"(

			let a = {"one": 1, "two": 2}
			print(a)

		)",
		{ R"({"one": 1, "two": 2})" }
	);
}

QUARK_UNIT_TEST("Floyd test suite", "dict<string:int> {}", "{}", "Infered type"){
	ut_verify_printout_nolib(
		QUARK_POS,
		R"(

			mutable [string:int] a = {}
			a = {}
			print(a)

		)",
		{ R"({})" }
	);
}

QUARK_UNIT_TEST("Floyd test suite", "dict<string:int> ==", "", ""){
	run_closed(R"(		assert(({"one": 1, "two": 2} == {"one": 1, "two": 2}) == true)		)");
}
QUARK_UNIT_TEST("Floyd test suite", "dict<string:int> ==", "", ""){
	run_closed(R"(		assert(({"one": 1, "two": 2} == {"two": 2}) == false)		)");
}
QUARK_UNIT_TEST("Floyd test suite", "dict<string:int> ==", "", ""){
	run_closed(R"(		assert(({"one": 2, "two": 2} == {"one": 1, "two": 2}) == false)		)");
}
QUARK_UNIT_TEST("Floyd test suite", "dict<string:int> ==", "", ""){
	run_closed(R"(		assert(({"one": 1, "two": 2} < {"one": 1, "two": 2}) == false)		)");
}
QUARK_UNIT_TEST("Floyd test suite", "dict<string:int> <", "", ""){
	run_closed(R"(		assert(({"one": 1, "two": 1} < {"one": 1, "two": 2}) == true)		)");
}

QUARK_UNIT_TEST("Floyd test suite", "dict<> {}", "", "Error cannot infer type"){
	ut_verify_exception_nolib(QUARK_POS, R"(		assert(size({}) == 0)		)", "Cannot infer type in construct-value-expression. Line: 1 \"assert(size({}) == 0)\"");
}

QUARK_UNIT_TEST("Floyd test suite", "dict<> {}", "", "Error cannot infer type"){
	ut_verify_exception_nolib(QUARK_POS, R"(		print({})		)", "Cannot infer type in construct-value-expression. Line: 1 \"print({})\"");
}

QUARK_UNIT_TEST("Floyd test suite", "dict<string:int> size()", "", "1"){
	run_closed(R"(		assert(size({"one":1}) == 1)		)");
}
QUARK_UNIT_TEST("Floyd test suite", "dict<string:int> size()", "", "2"){
	run_closed(R"(		assert(size({"one":1, "two":2}) == 2)		)");
}

QUARK_UNIT_TEST("Floyd test suite", "dict<string:int> update()", "add", "correct dict, without side effect on original dict"){
	ut_verify_printout_nolib(
		QUARK_POS,
		R"(

			let a = { "one": 1, "two": 2}
			let b = update(a, "three", 3)
			print(a)
			print(b)

		)",
		{
			R"({"one": 1, "two": 2})",
			R"({"one": 1, "three": 3, "two": 2})"
		}
	);
}

QUARK_UNIT_TEST("Floyd test suite", "dict<string:int> update()", "replace element", "correct dict, without side effect on original dict"){
	ut_verify_printout_nolib(
		QUARK_POS,
		R"(

			let a = { "one": 1, "two": 2, "three" : 3}
			let b = update(a, "three", 333)
			print(a)
			print(b)

		)",
		{
			R"({"one": 1, "three": 3, "two": 2})",
			R"({"one": 1, "three": 333, "two": 2})"
		}
	);
}

QUARK_UNIT_TEST("Floyd test suite", "dict<string:int> update()", "dest is empty dict", "error"){
	ut_verify_exception_nolib(
		QUARK_POS,
		R"(

			let a = update({}, "one", 1)
			let b = update(a, "two", 2)
			print(b)
			assert(a == {"one": 1})
			assert(b == {"one": 1, "two": 2})

		)",
		"Cannot infer type in construct-value-expression. Line: 3 \"let a = update({}, \"one\", 1)\""
	);
}

QUARK_UNIT_TEST("Floyd test suite", "dict<string:int> exists()", "", ""){
	run_closed(R"(

		let a = { "one": 1, "two": 2, "three" : 3}
		assert(exists(a, "two") == true)
		assert(exists(a, "four") == false)

	)");
}

QUARK_UNIT_TEST("Floyd test suite", "dict<string:int> erase()", "", ""){
	run_closed(R"(

		let a = { "one": 1, "two": 2, "three" : 3}
		let b = erase(a, "one")
		assert(b == { "two": 2, "three" : 3})

	)");
}

QUARK_UNIT_TEST("Floyd test suite", "dict", "Error: Lookup in dict using non-string key", "exception"){
	ut_verify_exception_nolib(
		QUARK_POS,
		R"(

			let a = { "one": 1, "two": 2 }
			print(a[3])

		)",
		"Dictionary can only be looked up using string keys, not a \"int\". Line: 4 \"print(a[3])\""
	);
}

QUARK_UNIT_TEST("Floyd test suite", "dict", "Dict can not hold elements of different types.", "exception"){
	ut_verify_exception_nolib(
		QUARK_POS,
		R"(

			let a = {"one": 1, "two": bool}

		)",
		"Dictionary of type [string:int] cannot hold an element of type typeid. Line: 3 \"let a = {\"one\": 1, \"two\": bool}\""
	);
}


//////////////////////////////////////////		STRUCT - TYPE



QUARK_UNIT_TEST("Floyd test suite", "struct", "", ""){
	run_closed(R"(		struct t {}		)");
}

QUARK_UNIT_TEST("Floyd test suite", "struct", "", ""){
	run_closed(R"(		struct t { int a }		)");
}

QUARK_UNIT_TEST("Floyd test suite", "struct", "check struct's type", ""){
	ut_verify_printout_nolib(
		QUARK_POS,
		R"(

			struct t { int a }
			print(t)

		)",
		{ "struct {int a;}" }
	);
}

QUARK_UNIT_TEST("Floyd test suite", "struct", "check struct's type", ""){
	ut_verify_printout_nolib(
		QUARK_POS,
		R"(

			struct t { int a }
			let a = t(3)
			print(a)

		)",
		{ R"({a=3})" }
	);
}

QUARK_UNIT_TEST("Floyd test suite", "struct", "read back struct member", ""){
	ut_verify_printout_nolib(
		QUARK_POS,
		R"(

			struct t { int a }
			let temp = t(42)
			print(temp.a)

		)",
		{ "42" }
	);
}

QUARK_UNIT_TEST("Floyd test suite", "struct", "instantiate nested structs", ""){
	ut_verify_printout_nolib(
		QUARK_POS,
		R"(

			struct color { int red int green int blue }

			let c = color(128, 192, 255)
			print(c)

		)",
		{
			"{red=128, green=192, blue=255}",
		}
	);
}

//??? Test all types of members in structs.
QUARK_UNIT_TEST("Floyd test suite", "struct", "instantiate nested structs 2", ""){
	ut_verify_printout_nolib(
		QUARK_POS,
		R"(

			struct color { int red int green int blue }
			struct image { color back color front }

			let i = image(color(1, 2, 3), color(200, 201, 202))
			print(i)

		)",
		{
			"{back={red=1, green=2, blue=3}, front={red=200, green=201, blue=202}}"
		}
	);
}

QUARK_UNIT_TEST("Floyd test suite", "struct", "access member of nested structs", ""){
	ut_verify_printout_nolib(
		QUARK_POS,
		R"(

			struct color { int red int green int blue }
			struct image { color back color front }
			let i = image(color(1, 2, 3), color(200, 201, 202))
			print(i.front.green)

		)",
		{ "201" }
	);
}

QUARK_UNIT_TEST("Floyd test suite", "struct", "return struct from function", ""){
	ut_verify_printout_nolib(
		QUARK_POS,
		R"(

			struct color { int red int green int blue }
			struct image { color back color front }
			func color make_color(){
				return color(100, 101, 102)
			}
			let z = make_color()
			print(z)

		)",
		{ "{red=100, green=101, blue=102}" }
	);
}

QUARK_UNIT_TEST("Floyd test suite", "struct", "return struct from function", ""){
	ut_verify_printout_nolib(
		QUARK_POS,
		R"(

			struct color { int red int green int blue }
			struct image { color back color front }
			func color make_color(){
				return color(100, 101, 102)
			}
			print(make_color())

		)",
		{ "{red=100, green=101, blue=102}" }
	);
}

QUARK_UNIT_TEST("Floyd test suite", "struct", "compare structs", ""){
	ut_verify_printout_nolib(
		QUARK_POS,
		R"(

			struct color { int red int green int blue }
			print(color(1, 2, 3) == color(1, 2, 3))

		)",
		{ "true" }
	);
}

QUARK_UNIT_TEST("Floyd test suite", "struct", "compare structs", ""){
	ut_verify_printout_nolib(
		QUARK_POS,
		R"(

			struct color { int red int green int blue }
			print(color(9, 2, 3) == color(1, 2, 3))

		)",
		{ "false" }
	);
}

QUARK_UNIT_TEST("Floyd test suite", "struct", "compare structs different types", ""){
	ut_verify_exception_nolib(
		QUARK_POS,
		R"(

			struct color { int red int green int blue }
			struct file { int id }
			print(color(1, 2, 3) == file(404))

		)",
		"Expression type mismatch - cannot convert 'struct {int id;}' to 'struct {int red;int green;int blue;}. Line: 5 \"print(color(1, 2, 3) == file(404))\""
	);
}
QUARK_UNIT_TEST("Floyd test suite", "struct", "compare structs with <, different types", ""){
	ut_verify_printout_nolib(
		QUARK_POS,
		R"(

			struct color { int red int green int blue }
			print(color(1, 2, 3) < color(1, 2, 3))

		)",
		{ "false" }
	);
}

QUARK_UNIT_TEST("Floyd test suite", "struct", "compare structs <", ""){
	ut_verify_printout_nolib(
		QUARK_POS,
		R"(

			struct color { int red int green int blue }
			print(color(1, 2, 3) < color(1, 4, 3))

		)",
		{ "true" }
	);
}

QUARK_UNIT_TEST("Floyd test suite", "struct", "update struct manually", ""){
	ut_verify_printout_nolib(
		QUARK_POS,
		R"(

			struct color { int red int green int blue }
			let a = color(255, 128, 128)
			let b = color(a.red, a.green, 129)
			print(a)
			print(b)

		)",
		{ "{red=255, green=128, blue=128}", "{red=255, green=128, blue=129}" }
	);
}

QUARK_UNIT_TEST("Floyd test suite", "struct", "mutate struct member using = won't work", ""){
	ut_verify_exception_nolib(
		QUARK_POS,
		R"(

			struct color { int red int green int blue }
			let a = color(255,128,128)
			let b = a.green = 3
			print(a)
			print(b)

		)",
		R"___(Expected constant or identifier. Line: 5 "let b = a.green = 3")___"
	);
}

QUARK_UNIT_TEST("Floyd test suite", "struct", "mutate struct member using update()", ""){
	ut_verify_printout_nolib(
		QUARK_POS,
		R"(

			struct color { int red int green int blue }
			let a = color(255,128,128)
			let b = update(a, "green", 3)
			print(a)
			print(b)

		)",
		{ "{red=255, green=128, blue=128}", "{red=255, green=3, blue=128}" }
	);
}

OFF_QUARK_UNIT_TEST("Floyd test suite", "struct", "mutate nested member", ""){
	ut_verify_printout_nolib(
		QUARK_POS,
		R"(

			struct color { int red int green int blue }
			struct image { color back color front }
			let a = image(color(0,100,200), color(0,0,0))
			let b = update(a, "front.green", 3)
			print(a)
			print(b)

		)",
		{
			"{back={red=0, green=100, blue=200}, front={red=0, green=0, blue=0}}",
			"{back={red=0, green=100, blue=200}, front={red=0, green=3, blue=0}}"
		}
	);
}

QUARK_UNIT_TEST("Floyd test suite", "struct", "Error: Define struct with colliding name", "exception"){
	ut_verify_exception_nolib(
		QUARK_POS,
		R"(

			let int a = 10
			struct a { int x }

		)",
		"Name \"a\" already used in current lexical scope. Line: 4 \"struct a { int x }\""
	);
}

QUARK_UNIT_TEST("Floyd test suite", "struct", "Error: Access unknown struct member", "exception"){
	ut_verify_exception_nolib(
		QUARK_POS,
		R"(

			struct a { int x }
			let b = a(13)
			print(b.y)

		)",
		"Unknown struct member \"y\". Line: 5 \"print(b.y)\""
	);
}

QUARK_UNIT_TEST("Floyd test suite", "struct", "Error: Access member in non-struct", "exception"){
	ut_verify_exception_nolib(
		QUARK_POS,
		R"(

			let a = 10
			print(a.y)

		)",
		"Left hand side is not a struct value, it's of type \"int\". Line: 4 \"print(a.y)\""
	);
}

QUARK_UNIT_TEST("Floyd test suite", "struct", "", ""){
	ut_verify_global_result_nolib(
		QUARK_POS,
		R"(

			struct pixel_t { double x double y }
			let a = pixel_t(100.0, 200.0)
			let result = a.x

		)",
		value_t::make_double(100.0)
	);
}


QUARK_UNIT_TEST("Floyd test suite", "struct", "Read member of struct sitting in a vector", ""){
	ut_verify_global_result_nolib(
		QUARK_POS,
		R"(

			struct pixel_t { double x double y }
			let c = [pixel_t(100.0, 200.0), pixel_t(101.0, 201.0)]
			let result = c[1].y

		)",
		value_t::make_double(201.0)
	);
}

QUARK_UNIT_TEST("Floyd test suite", "Struct", "Error: Wrong number of arguments to struct-constructor", "exception"){
	ut_verify_exception_nolib(
		QUARK_POS,
		R"(

			struct pos { double x double y }
			let a = pos(3)

		)",
		"Wrong number of arguments in function call, got 1, expected 2. Line: 4 \"let a = pos(3)\""
	);
}

//??? Also tell *which* argument is wrong type -- its name and index.
QUARK_UNIT_TEST("Floyd test suite", "struct", "Error: Wrong TYPE of arguments to struct-constructor", "exception"){
	ut_verify_exception_nolib(
		QUARK_POS,
		R"(

			struct pos { double x double y }
			let a = pos(3, "hello")

		)",
		"Expression type mismatch - cannot convert 'int' to 'double. Line: 4 \"let a = pos(3, \"hello\")\""
	);
}







//////////////////////////////////////////		json_value - TYPE


//??? document or disable using json-value directly as lookup parent.

QUARK_UNIT_TEST("Floyd test suite", "json_value::null", "", ""){
	run_closed(R"(		let result = null		)");
}

QUARK_UNIT_TEST("Floyd test suite", "json_value<string> Infer json_value::string", "", ""){
	ut_verify_global_result_nolib(QUARK_POS, R"(		let json_value result = "hello"		)", value_t::make_json_value("hello"));
}

QUARK_UNIT_TEST("Floyd test suite", "json_value<string> string-size()", "", ""){
	ut_verify_global_result_nolib(
		QUARK_POS,
		R"(

			let json_value a = "hello"
			let result = size(a);

		)",
		value_t::make_int(5)
	);
}

QUARK_UNIT_TEST("Floyd test suite", "json_value()", "json_value(123)", ""){
	ut_verify_global_result_nolib(QUARK_POS, "let result = json_value(123)", value_t::make_json_value(json_t(123.0)));
}

QUARK_UNIT_TEST("Floyd test suite", "json_value()", "", ""){
	ut_verify_global_result_nolib(
		QUARK_POS,
		R"(

			let result = json_value("hello")

		)",
		value_t::make_json_value(json_t("hello"))
	);
}

QUARK_UNIT_TEST("Floyd test suite", "json_value()", "", ""){
	ut_verify_global_result_nolib(QUARK_POS, "let result = json_value([1,2,3])", value_t::make_json_value(json_t::make_array({1,2,3})));
}

QUARK_UNIT_TEST("Floyd test suite", "json_value<number> construct number", "", ""){
	ut_verify_global_result_nolib(QUARK_POS, R"(		let json_value result = 13		)", value_t::make_json_value(13));
}

QUARK_UNIT_TEST("Floyd test suite", "json_value<true> construct true", "", ""){
	ut_verify_global_result_nolib(QUARK_POS, R"(		let json_value result = true		)", value_t::make_json_value(true));
}
QUARK_UNIT_TEST("Floyd test suite", "json_value<false> construct false", "", ""){
	ut_verify_global_result_nolib(QUARK_POS, R"(		let json_value result = false		)", value_t::make_json_value(false));
}

QUARK_UNIT_TEST("Floyd test suite", "json_value<array> construct array", "construct array", ""){
	ut_verify_global_result_nolib(QUARK_POS, R"(		let json_value result = ["hello", "bye"]		)", value_t::make_json_value(json_t::make_array(std::vector<json_t>{ "hello", "bye" }))	);
}

QUARK_UNIT_TEST("Floyd test suite", "json_value<string> read array member", "", ""){
	ut_verify_global_result_nolib(
		QUARK_POS,
		R"(

			let json_value a = ["hello", "bye"]
			let result = a[0]

		)",
		value_t::make_json_value("hello")
	);
}
QUARK_UNIT_TEST("Floyd test suite", "json_value<string> read array member", "", ""){
	ut_verify_global_result_nolib(
		QUARK_POS,
		R"(

			let json_value a = ["hello", "bye"]
			let result = string(a[0]) + string(a[1])

		)",
		value_t::make_string("hellobye")
	);
}

QUARK_UNIT_TEST("Floyd test suite", "json_value<string> read array member", "", ""){
	ut_verify_global_result_nolib(
		QUARK_POS,
		R"(

			let json_value a = ["hello", "bye"]
			let result = a[1]

		)",
		value_t::make_json_value("bye")
	);
}

QUARK_UNIT_TEST("Floyd test suite", "json_value<string> size()", "", ""){
	ut_verify_global_result_nolib(
		QUARK_POS,
		R"(

			let json_value a = ["a", "b", "c", "d"]
			let result = size(a)

		)",
		value_t::make_int(4)
	);
}

//	NOTICE: Floyd dict is stricter than JSON -- cannot have different types of values!
QUARK_UNIT_TEST("Floyd test suite", "json_value<string> construct", "mix value-types in dict", ""){
	ut_verify_printout_nolib(
		QUARK_POS,
		R"(

			let json_value a = { "pigcount": 3, "pigcolor": "pink" }
			print(a)

		)",
		{ R"({ "pigcolor": "pink", "pigcount": 3 })" }
	);
}

// JSON example snippets: http://json.org/example.html
QUARK_UNIT_TEST("Floyd test suite", "json_value<string> construct", "read world data", ""){
	ut_verify_printout_nolib(
		QUARK_POS,
		R"___(

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

		)___",
		{
			R"___({ "menu": { "id": "file", "popup": { "menuitem": [{ "onclick": "CreateNewDoc()", "value": "New" }, { "onclick": "OpenDoc()", "value": "Open" }, { "onclick": "CloseDoc()", "value": "Close" }] }, "value": "File" } })___"
		}
	);
}

QUARK_UNIT_TEST("Floyd test suite", "json_value<string> construct", "expressions inside def", ""){
	ut_verify_printout_nolib(
		QUARK_POS,
		R"___(

			let json_value a = { "pigcount": 1 + 2, "pigcolor": "pi" + "nk" }
			print(a)

		)___",
		{
			R"({ "pigcolor": "pink", "pigcount": 3 })"
		}
	);
}

QUARK_UNIT_TEST("Floyd test suite", "json_value<string> construct", "", ""){
	ut_verify_printout_nolib(
		QUARK_POS,
		R"___(

			let json_value a = { "pigcount": 3, "pigcolor": "pink" }
			print(a["pigcount"])
			print(a["pigcolor"])

		)___",
		{ "3", "\"pink\"" }
	);
}

QUARK_UNIT_TEST("Floyd test suite", "json_value<string> size()", "", ""){
	run_closed(R"(

		let json_value a = { "a": 1, "b": 2, "c": 3, "d": 4, "e": 5 }
		assert(size(a) == 5)

	)");
}


QUARK_UNIT_TEST("Floyd test suite", "json_value<null> construct", "", ""){
	ut_verify_global_result_nolib(QUARK_POS, R"(		let json_value result = null		)", value_t::make_json_value(json_t()));
}



QUARK_UNIT_TEST("Floyd test suite", "json_value", "", "error"){
	ut_verify_exception_nolib(
		QUARK_POS,
		R"(

			struct pixel_t { double x double y }

			//	c is a json_value::object
			let c = { "version": "1.0", "image": [pixel_t(100.0, 200.0), pixel_t(101.0, 201.0)] }
			let result = c["image"][1].y

		)",
		"Dictionary of type [string:string] cannot hold an element of type [struct {double x;double y;}]. Line: 6 \"let c = { \"version\": \"1.0\", \"image\": [pixel_t(100.0, 200.0), pixel_t(101.0, 201.0)] }\""
	);
}





//////////////////////////////////////////		TEST TYPE INFERING

QUARK_UNIT_TEST("Floyd test suite", "json_value<string> {}", "Infer {}", "JSON object"){
	ut_verify_global_result_nolib(QUARK_POS, R"(		let json_value result = {}		)", value_t::make_json_value(json_t::make_object()));
}


QUARK_UNIT_TEST("Floyd test suite", "get_json_type()", "", "1"){
	ut_verify_global_result_nolib(QUARK_POS, R"(		let result = get_json_type(json_value({}))		)", value_t::make_int(1));
}

QUARK_UNIT_TEST("Floyd test suite", "get_json_type()", "", "1"){
	ut_verify_printout_nolib(
		QUARK_POS,
		R"(

			let result = json_value({ "color": "black"})
			print(result)

		)",
		{ R"xyz({ "color": "black" })xyz" }
	);
}
QUARK_UNIT_TEST("Floyd test suite", "get_json_type()", "", "1"){
	ut_verify_global_result_nolib(
		QUARK_POS,
		R"(

			let result = get_json_type(json_value({ "color": "black"}))

		)",
		value_t::make_int(1)
	);
}
QUARK_UNIT_TEST("Floyd test suite", "get_json_type()", "array", "2"){
	ut_verify_global_result_nolib(QUARK_POS, R"(		let result = get_json_type(json_value([]))		)", value_t::make_int(2));
}
QUARK_UNIT_TEST("Floyd test suite", "get_json_type()", "string", "3"){
	ut_verify_global_result_nolib(QUARK_POS, R"(		let result = get_json_type(json_value("hello"))		)", value_t::make_int(3));
}
QUARK_UNIT_TEST("Floyd test suite", "get_json_type()", "number", "4"){
	ut_verify_global_result_nolib(QUARK_POS, R"(		let result = get_json_type(json_value(13))		)", value_t::make_int(4));
}
QUARK_UNIT_TEST("Floyd test suite", "get_json_type()", "true", "5"){
	ut_verify_global_result_nolib(QUARK_POS, R"(		let result = get_json_type(json_value(true))		)", value_t::make_int(5));
}
QUARK_UNIT_TEST("Floyd test suite", "get_json_type()", "false", "6"){
	ut_verify_global_result_nolib(QUARK_POS, R"(		let result = get_json_type(json_value(false))		)", value_t::make_int(6));
}

QUARK_UNIT_TEST("Floyd test suite", "get_json_type()", "null", "7"){
	ut_verify_global_result_nolib(QUARK_POS, R"(		let result = get_json_type(json_value(null))		)", value_t::make_int(7));
}

QUARK_UNIT_TEST("Floyd test suite", "get_json_type()", "DOCUMENTATION SNIPPET", ""){
	run_closed(R"___(

		func string get_name(json_value value){
			let t = get_json_type(value)
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
				return "error"
			}
		}

		assert(get_name(json_value({"a": 1, "b": 2})) == "json_object")
		assert(get_name(json_value([1,2,3])) == "json_array")
		assert(get_name(json_value("crash")) == "json_string")
		assert(get_name(json_value(0.125)) == "json_number")
		assert(get_name(json_value(true)) == "json_true")
		assert(get_name(json_value(false)) == "json_false")

	)___");
}





//////////////////////////////////////////		HOST FUNCTION - typeof()


QUARK_UNIT_TEST("Floyd test suite", "typeof()", "", ""){
	ut_verify_global_result_nolib(
		QUARK_POS,
		R"(

			let result = typeof(145)

		)",
		value_t::make_typeid_value(typeid_t::make_int())
	);
}

QUARK_UNIT_TEST("Floyd test suite", "typeof()", "", ""){
	ut_verify_global_result_nolib(QUARK_POS, R"(		let result = to_string(typeof(145))		)", value_t::make_string("int"));
}

QUARK_UNIT_TEST("Floyd test suite", "typeof()", "", ""){
	ut_verify_global_result_nolib(QUARK_POS, R"(		let result = typeof("hello")		)", value_t::make_typeid_value(typeid_t::make_string()));
}

QUARK_UNIT_TEST("Floyd test suite", "typeof()", "", ""){
	ut_verify_global_result_nolib(QUARK_POS, R"(		let result = to_string(typeof("hello"))		)",	value_t::make_string("string"));
}

QUARK_UNIT_TEST("Floyd test suite", "typeof()", "", ""){
	ut_verify_global_result_nolib(QUARK_POS, R"(		let result = typeof([1,2,3])		)", value_t::make_typeid_value(typeid_t::make_vector(typeid_t::make_int()))	);
}
QUARK_UNIT_TEST("Floyd test suite", "typeof()", "", ""){
	ut_verify_global_result_nolib(QUARK_POS, R"(		let result = to_string(typeof([1,2,3]))		)",value_t::make_string("[int]") );
}

QUARK_UNIT_TEST("Floyd test suite", "typeof()", "", ""){
	ut_verify_global_result_nolib(
		QUARK_POS,
		R"(

			let result = to_string(typeof(int));

		)",
		value_t::make_string("typeid")
	);
}




//////////////////////////////////////////		script_to_jsonvalue()


QUARK_UNIT_TEST("Floyd test suite", "script_to_jsonvalue()", "", ""){
	ut_verify_global_result_nolib(QUARK_POS, R"(		let result = script_to_jsonvalue("\"genelec\"")		)", value_t::make_json_value(json_t("genelec")));
}

QUARK_UNIT_TEST("Floyd test suite", "script_to_jsonvalue()", "", ""){
	ut_verify_printout_nolib(
		QUARK_POS,
		R"___(

			let a = script_to_jsonvalue("{ \"magic\": 1234 }")
			print(a)

		)___",
		{ R"({ "magic": 1234 })" }
	);
}


//////////////////////////////////////////		jsonvalue_to_script()


QUARK_UNIT_TEST("Floyd test suite", "jsonvalue_to_script()", "", ""){
	ut_verify_printout_nolib(
		QUARK_POS,
		R"___(

			let json_value a = "cheat"
			let b = jsonvalue_to_script(a)
			print(b)

		)___",
		{ "\"cheat\"" }
	);
}


QUARK_UNIT_TEST("Floyd test suite", "jsonvalue_to_script()", "", ""){
	ut_verify_printout_nolib(
		QUARK_POS,
		R"___(

			let json_value a = { "magic": 1234 }
			let b = jsonvalue_to_script(a)
			print(b)

		)___",
		{ "{ \"magic\": 1234 }" }
	);
}


//////////////////////////////////////////		value_to_jsonvalue()


QUARK_UNIT_TEST("Floyd test suite", "value_to_jsonvalue()", "bool", ""){
	ut_verify_global_result_nolib(QUARK_POS, R"(		let result = value_to_jsonvalue(true)		)", value_t::make_json_value(json_t(true)));
}
QUARK_UNIT_TEST("Floyd test suite", "value_to_jsonvalue()", "bool", ""){
	ut_verify_global_result_nolib(QUARK_POS, R"(		let result = value_to_jsonvalue(false)		)", value_t::make_json_value(json_t(false)));
}

QUARK_UNIT_TEST("Floyd test suite", "value_to_jsonvalue()", "int", ""){
	ut_verify_global_result_nolib(QUARK_POS, R"(		let result = value_to_jsonvalue(789)		)", value_t::make_json_value(json_t(789.0)));
}
QUARK_UNIT_TEST("Floyd test suite", "value_to_jsonvalue()", "int", ""){
	ut_verify_global_result_nolib(QUARK_POS, R"(		let result = value_to_jsonvalue(-987)		)", value_t::make_json_value(json_t(-987.0)));
}

QUARK_UNIT_TEST("Floyd test suite", "value_to_jsonvalue()", "double", ""){
	ut_verify_global_result_nolib(QUARK_POS, R"(		let result = value_to_jsonvalue(-0.125)		)", value_t::make_json_value(json_t(-0.125)));
}


QUARK_UNIT_TEST("Floyd test suite", "value_to_jsonvalue()", "string", ""){
	ut_verify_global_result_nolib(QUARK_POS, R"(		let result = value_to_jsonvalue("fanta")		)", value_t::make_json_value(json_t("fanta")));
}

QUARK_UNIT_TEST("Floyd test suite", "value_to_jsonvalue()", "typeid", ""){
	ut_verify_global_result_nolib(QUARK_POS, R"(		let result = value_to_jsonvalue(typeof([2,2,3]))		)", value_t::make_json_value(json_t::make_array({ "vector", "int"})));
}

QUARK_UNIT_TEST("Floyd test suite", "value_to_jsonvalue()", "[]", ""){
	ut_verify_global_result_nolib(QUARK_POS, R"(		let result = value_to_jsonvalue([1,2,3])		)", value_t::make_json_value(json_t::make_array({ 1, 2, 3 })));
}

QUARK_UNIT_TEST("Floyd test suite", "value_to_jsonvalue()", "{}", ""){
	ut_verify_global_result_nolib(
		QUARK_POS,
		R"(

			let result = value_to_jsonvalue({"ten": 10, "eleven": 11})

		)",
		value_t::make_json_value(
			json_t::make_object({{ "ten", 10 }, { "eleven", 11 }})
		)
	);
}

QUARK_UNIT_TEST("Floyd test suite", "jsonvalue_to_script()", "pixel_t", ""){
	ut_verify_global_result_nolib(
		QUARK_POS,
		R"(
	
			struct pixel_t { double x double y }
			let c = pixel_t(100.0, 200.0)
			let a = value_to_jsonvalue(c)
			let result = jsonvalue_to_script(a)

		)",
		value_t::make_string("{ \"x\": 100, \"y\": 200 }")
	);
}

QUARK_UNIT_TEST("Floyd test suite", "jsonvalue_to_script()", "[pixel_t]", ""){
	ut_verify_global_result_nolib(
		QUARK_POS,
		R"(

			struct pixel_t { double x double y }
			let c = [pixel_t(100.0, 200.0), pixel_t(101.0, 201.0)]
			let a = value_to_jsonvalue(c)
			let result = jsonvalue_to_script(a)

		)",
		value_t::make_string("[{ \"x\": 100, \"y\": 200 }, { \"x\": 101, \"y\": 201 }]")
	);
}


//////////////////////////////////////////		value_to_jsonvalue() -> jsonvalue_to_value() roundtrip

/*
	jsonvalue_to_value() returns different types depending on its 2nd argument.
	??? test all types!
*/
QUARK_UNIT_TEST("Floyd test suite", "jsonvalue_to_value()", "bool", ""){
	ut_verify_global_result_nolib(QUARK_POS, R"(		let result = jsonvalue_to_value(value_to_jsonvalue(true), bool)		)", value_t::make_bool(true));
}

QUARK_UNIT_TEST("Floyd test suite", "jsonvalue_to_value()", "bool", ""){
	ut_verify_global_result_nolib(QUARK_POS, R"(		let result = jsonvalue_to_value(value_to_jsonvalue(false), bool)		)", value_t::make_bool(false));
}

QUARK_UNIT_TEST("Floyd test suite", "jsonvalue_to_value()", "int", ""){
	ut_verify_global_result_nolib(QUARK_POS, R"(		let result = jsonvalue_to_value(value_to_jsonvalue(91), int)		)", value_t::make_int(91));
}

QUARK_UNIT_TEST("Floyd test suite", "jsonvalue_to_value()", "double", ""){
	ut_verify_global_result_nolib(QUARK_POS, R"(		let result = jsonvalue_to_value(value_to_jsonvalue(-0.125), double)		)", value_t::make_double(-0.125));
}

QUARK_UNIT_TEST("Floyd test suite", "jsonvalue_to_value()", "string", ""){
	ut_verify_global_result_nolib(QUARK_POS, R"(		let result = jsonvalue_to_value(value_to_jsonvalue(""), string)		)", value_t::make_string(""));
}

QUARK_UNIT_TEST("Floyd test suite", "jsonvalue_to_value()", "string", ""){
	ut_verify_global_result_nolib(QUARK_POS, R"(		let result = jsonvalue_to_value(value_to_jsonvalue("cola"), string)		)", value_t::make_string("cola"));
}

QUARK_UNIT_TEST("Floyd test suite", "jsonvalue_to_value()", "point_t", ""){
	const auto point_t_def = std::vector<member_t>{
		member_t(typeid_t::make_double(), "x"),
		member_t(typeid_t::make_double(), "y")
	};
	const auto expected = value_t::make_struct_value(
		typeid_t::make_struct2(point_t_def),
		{ value_t::make_double(1), value_t::make_double(3) }
	);

	ut_verify_global_result_nolib(
		QUARK_POS,
		R"(

			struct point_t { double x double y }
			let result = jsonvalue_to_value(value_to_jsonvalue(point_t(1.0, 3.0)), point_t)

		)",
		expected
	);
}






///////////////////////////////////////////////////			TEST LIBRARY FEATURES





QUARK_UNIT_TEST("Floyd test suite", "cmath_pi", "", ""){
	ut_verify_global_result_lib(
		QUARK_POS,
		R"(

			let x = cmath_pi
			let result = x >= 3.14 && x < 3.15

		)",
		value_t::make_bool(true)
	);
}

QUARK_UNIT_TEST("Floyd test suite", "color__black", "", ""){
	run_closed_lib(R"(

		assert(color__black.red == 0.0)
		assert(color__black.green == 0.0)
		assert(color__black.blue == 0.0)
		assert(color__black.alpha == 1.0)

	)");
}


QUARK_UNIT_TEST("Floyd test suite", "color__black", "", ""){
	run_closed_lib(R"(

		let r = add_colors(color_t(1.0, 2.0, 3.0, 4.0), color_t(1000.0, 2000.0, 3000.0, 4000.0))
		assert(r.red == 1001.0)
		assert(r.green == 2002.0)
		assert(r.blue == 3003.0)
		assert(r.alpha == 4004.0)

	)");
}

QUARK_UNIT_TEST("Floyd test suite", "", "pixel_t()", ""){
	const auto pixel_t__def = std::vector<member_t>{
		member_t(typeid_t::make_int(), "red"),
		member_t(typeid_t::make_int(), "green"),
		member_t(typeid_t::make_int(), "blue")
	};

	ut_verify_global_result_nolib(
		QUARK_POS,

		"struct pixel_t { int red int green int blue } let result = pixel_t(1,2,3)",

		value_t::make_struct_value(
			typeid_t::make_struct2(pixel_t__def),
			std::vector<value_t>{ value_t::make_int(1), value_t::make_int(2), value_t::make_int(3) }
		)
	);
}




QUARK_UNIT_TEST("Floyd test suite", "", "", ""){
	const auto a = typeid_t::make_vector(typeid_t::make_string());
	const auto b = typeid_t::make_vector(make__fsentry_t__type());
	ut_verify_auto(QUARK_POS, a != b, true);
}




//////////////////////////////////////////		HOST FUNCTION - get_time_of_day()

QUARK_UNIT_TEST("Floyd test suite", "get_time_of_day()", "", ""){
	run_closed(R"(

		let start = get_time_of_day()
		mutable b = 0
		mutable t = [0]
		for(i in 0...100){
			b = b + 1
			t = push_back(t, b)
		}
		let end = get_time_of_day()
//		print("Duration: " + to_string(end - start) + ", number = " + to_string(b))
//		print(t)

	)");
}

QUARK_UNIT_TEST("Floyd test suite", "get_time_of_day()", "", ""){
	run_closed(R"(

		let int a = get_time_of_day()
		let int b = get_time_of_day()
		let int c = b - a
//		print("Delta time:" + to_string(a))

	)");
}

//////////////////////////////////////////		HOST FUNCTION - calc_string_sha1()


QUARK_UNIT_TEST("Floyd test suite", "calc_string_sha1()", "", ""){
	run_closed(R"(

		let a = calc_string_sha1("Violator is the seventh studio album by English electronic music band Depeche Mode.")
//		print(to_string(a))
		assert(a.ascii40 == "4d5a137b3b1faf855872a312a184dd9a24594387")

	)");
}


//////////////////////////////////////////		HOST FUNCTION - calc_binary_sha1()


QUARK_UNIT_TEST("Floyd test suite", "calc_binary_sha1()", "", ""){
	run_closed_lib(R"(

		let bin = binary_t("Violator is the seventh studio album by English electronic music band Depeche Mode.")
		let a = calc_binary_sha1(bin)
//		print(to_string(a))
		assert(a.ascii40 == "4d5a137b3b1faf855872a312a184dd9a24594387")

	)");
}



//////////////////////////////////////////		HOST FUNCTION - map()



QUARK_UNIT_TEST("Floyd test suite", "map()", "[int] map(int f(int))", ""){
	run_closed(R"(

		let a = [ 10, 11, 12 ]

		func int f(int v){
			return 1000 + v
		}

		let result = map(a, f)
//		print(to_string(result))
		assert(result == [ 1010, 1011, 1012 ])

	)");
}

QUARK_UNIT_TEST("Floyd test suite", "map()", "[string] map(string f(int))", ""){
	run_closed(R"(

		let a = [ 10, 11, 12 ]

		func string f(int v){
			return to_string(1000 + v)
		}

		let result = map(a, f)
//		print(to_string(result))
		assert(result == [ "1010", "1011", "1012" ])

	)");
}

QUARK_UNIT_TEST("Floyd test suite", "map()", "[int] map(int f(string))", ""){
	run_closed(R"(

		let a = [ "one", "two_", "three" ]

		func int f(string v){
			return size(v)
		}

		let result = map(a, f)
//		print(to_string(result))
		assert(result == [ 3, 4, 5 ])

	)");
}


//////////////////////////////////////////		HOST FUNCTION - map_string()



QUARK_UNIT_TEST("Floyd test suite", "map_string()", "", ""){
	run_closed(R"(

		let a = "ABC"

		func string f(string v){
			assert(size(v) == 1)

			let int ch = v[0]
			return to_string(ch)
		}

		let result = map_string(a, f)
//		print(to_string(result))
		assert(result == "656667")

	)");
}



//////////////////////////////////////////		HOST FUNCTION - reduce()



QUARK_UNIT_TEST("Floyd test suite", "reduce()", "int reduce([int], int, func int(int, int))", ""){
	run_closed(R"(

		func int f(int acc, int element){
			return acc + element * 2
		}

		let result = reduce([ 10, 11, 12 ], 2000, f)

//		print(to_string(result))
		assert(result == 2066)

	)");
}

QUARK_UNIT_TEST("Floyd test suite", "reduce()", "string reduce([int], string, func int(string, int))", ""){
	run_closed(R"___(

		func string f(string acc, int v){
			mutable s = acc
			for(e in 0 ..< v){
				s = "<" + s + ">"
			}
			s = "(" + s + ")"
			return s
		}

		let result = reduce([ 2, 4, 1 ], "O", f)
//		print(to_string(result))
		assert(result == "(<(<<<<(<<O>>)>>>>)>)")

	)___");
}




//////////////////////////////////////////		HOST FUNCTION - filter()



QUARK_UNIT_TEST("Floyd test suite", "filter()", "int filter([int], int, func int(int, int))", ""){
	run_closed(R"(

		func bool f(int element){
			return element % 3 == 0 ? true : false
		}

		let result = filter([ 3, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13 ], f)

//		print(to_string(result))
		assert(result == [ 3, 3, 6, 9, 12 ])

	)");
}

QUARK_UNIT_TEST("Floyd test suite", "filter()", "string filter([int], string, func int(string, int))", ""){
	run_closed(	R"___(

		func bool f(string element){
			return size(element) == 3 || size(element) == 5
		}

		let result = filter([ "one", "two", "three", "four", "five", "six", "seven" ], f)

//		print(to_string(result))
		assert(result == [ "one", "two", "three", "six", "seven" ])

	)___");
}





//////////////////////////////////////////		HOST FUNCTION - supermap()



QUARK_UNIT_TEST("Floyd test suite", "supermap()", "No dependencies", ""){
	run_closed(R"(

		func string f(string v, [string] inputs){
			return "[" + v + "]"
		}

		let result = supermap([ "one", "ring", "to" ], [ -1, -1, -1 ], f)
//		print(to_string(result))
		assert(result == [ "[one]", "[ring]", "[to]" ])

	)");
}

QUARK_UNIT_TEST("Floyd test suite", "supermap()", "No dependencies", ""){
	run_closed(R"(

		func string f(string v, [string] inputs){
			return "[" + v + "]"
		}

		let result = supermap([ "one", "ring", "to" ], [ 1, 2, -1 ], f)
//		print(to_string(result))
		assert(result == [ "[one]", "[ring]", "[to]" ])

	)");
}

QUARK_UNIT_TEST("Floyd test suite", "supermap()", "complex", ""){
	run_closed(R"(

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
//		print(to_string(result))
		assert(result == [ "D[]", "B[]", "A[B[], E[D[], C[], F[]]]", "C[]", "E[D[], C[], F[]]", "F[]" ])

	)");
}



//////////////////////////////////////////		HOST FUNCTION - read_text_file()

/*
QUARK_UNIT_TEST("Floyd test suite", "read_text_file()", "", ""){
	run_closed(R"(

		path = get_env_path();
		a = read_text_file(path + "/Desktop/test1.json");
		print(a);

	)");
	ut_verify(QUARK_POS, vm->_print_output, { string() + R"({ "magic": 1234 })" + "\n" });
}
*/


//////////////////////////////////////////		HOST FUNCTION - write_text_file()


QUARK_UNIT_TEST("Floyd test suite", "write_text_file()", "", ""){
	run_closed(R"(

		let path = get_fs_environment().desktop_dir
		write_text_file(path + "/test_out.txt", "Floyd wrote this!")

	)");
}

//////////////////////////////////////////		HOST FUNCTION - instantiate_from_typeid()

//	instantiate_from_typeid() only works for const-symbols right now.
/*
??? Support when we can make "t = typeof(1234)" a const-symbol
QUARK_UNIT_TEST("Floyd test suite", "", "", ""){
	const auto result = test__run_return_result(
		R"(
			t = typeof(1234);
			result = instantiate_from_typeid(t, 3);
		)", {}
	);
	ut_verify_values(QUARK_POS, result, value_t::make_int(3));
}

QUARK_UNIT_TEST("Floyd test suite", "", "", ""){
	run_closed(R"(

		a = instantiate_from_typeid(typeof(123), 3);
		assert(to_string(typeof(a)) == "int");
		assert(a == 3);

	)");
}
*/


//////////////////////////////////////////		HOST FUNCTION - get_directory_entries()


QUARK_UNIT_TEST("Floyd test suite", "get_fsentries_shallow()", "", ""){
	ut_verify_global_result_nolib(
		QUARK_POS,
		R"(

			let result0 = get_fsentries_shallow("/Users/marcus/Desktop/")
			assert(size(result0) > 3)
			print(to_pretty_string(result0))

			let result = typeof(result0)

		)",
		value_t::make_typeid_value(
			typeid_t::make_vector(make__fsentry_t__type())
		)
	);
}


//////////////////////////////////////////		HOST FUNCTION - get_fsentries_deep()


QUARK_UNIT_TEST("Floyd test suite", "get_fsentries_deep()", "", ""){
	run_closed(
		R"(

			let result = get_fsentries_deep("/Users/marcus/Desktop/")
			assert(size(result) > 3)
//			print(to_pretty_string(result))

		)"
	);
}


//////////////////////////////////////////		HOST FUNCTION - get_fsentry_info()


QUARK_UNIT_TEST("Floyd test suite", "get_fsentry_info()", "", ""){
	ut_verify_global_result_nolib(
		QUARK_POS,
		R"(

			let x = get_fsentry_info("/Users/marcus/Desktop/")
			print(to_pretty_string(x))
			let result = typeof(x)

		)",
		value_t::make_typeid_value(
			make__fsentry_info_t__type()
		)
	);
}


//////////////////////////////////////////		HOST FUNCTION - get_fs_environment()


QUARK_UNIT_TEST("Floyd test suite", "get_fs_environment()", "", ""){
	ut_verify_global_result_nolib(
		QUARK_POS,
		R"(

			let x = get_fs_environment()
			print(to_pretty_string(x))
			let result = typeof(x)

		)",
		value_t::make_typeid_value(
			make__fs_environment_t__type()
		)
	);
}




//////////////////////////////////////////		HOST FUNCTION - does_fsentry_exist()


QUARK_UNIT_TEST("Floyd test suite", "does_fsentry_exist()", "", ""){
	ut_verify_global_result_nolib(
		QUARK_POS,
		R"(

			let path = get_fs_environment().desktop_dir
			let x = does_fsentry_exist(path)
			print(to_pretty_string(x))

			assert(x == true)
			let result = typeof(x)

		)",
		value_t::make_typeid_value(typeid_t::make_bool())
	);
}

QUARK_UNIT_TEST("Floyd test suite", "does_fsentry_exist()", "", ""){
	ut_verify_global_result_nolib(
		QUARK_POS,
		R"(

			let path = get_fs_environment().desktop_dir + "xyz"
			let result = does_fsentry_exist(path)
			print(to_pretty_string(result))

			assert(result == false)

		)",
		value_t::make_bool(false)
	);
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


QUARK_UNIT_TEST("Floyd test suite", "create_directory_branch()", "", ""){
	remove_test_dir("unittest___create_directory_branch", "subdir");

	run_closed(
		R"(

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

		)"
	);
}


//////////////////////////////////////////		HOST FUNCTION - delete_fsentry_deep()


QUARK_UNIT_TEST("Floyd test suite", "delete_fsentry_deep()", "", ""){
	remove_test_dir("unittest___delete_fsentry_deep", "subdir");

	run_closed(
		R"(

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

		)"
	);
}



//////////////////////////////////////////		HOST FUNCTION - rename_fsentry()


QUARK_UNIT_TEST("Floyd test suite", "rename_fsentry()", "", ""){
	run_closed(
		R"(

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

		)"
	);
}





///////////////////////////////////////////////////			PARSER ERRORS

QUARK_UNIT_TEST("Parser error", "", "", ""){
	ut_verify_exception_nolib(QUARK_POS, R"(		£		)", R"___(Illegal characters. Line: 1 "£")___");
}


QUARK_UNIT_TEST("Parser error", "", "", ""){
	ut_verify_exception_nolib(QUARK_POS, R"(		{ let a = 10		)", R"___(Block is missing end bracket '}'. Line: 1 "{ let a = 10")___");
}

QUARK_UNIT_TEST("Parser error", "", "", ""){
	ut_verify_exception_nolib(
		QUARK_POS,
		R"(

			[ 100, 200 }

		)",
		R"___(Unexpected char "}" in bounded list [ ]! Line: 3 "[ 100, 200 }")___"
	);
}

QUARK_UNIT_TEST("Parser error", "", "", ""){
	ut_verify_exception_nolib(
		QUARK_POS,
		R"(

			x = { "a": 100 ]

		)",
		R"___(Unexpected char "]" in bounded list { }! Line: 3 "x = { "a": 100 ]")___"
	);
}


QUARK_UNIT_TEST("Parser error", "", "", ""){
	ut_verify_exception_nolib(
		QUARK_POS,
		R"("abc\)",
		R"___(Incomplete escape sequence in string literal: "abc"! Line: 1 ""abc\")___"
	);
}






#if 0
QUARK_UNIT_TEST("Analyse all test programs", "", "", ""){
	int instruction_count_total = 0;
	int symbol_count_total = 0;

	for(const auto& s: program_recording){
		try{
		const auto bc = test_compile_to_bytecode(s, "");
		int instruction_count_sum = static_cast<int>(bc._globals._instructions.size());
		int symbol_count_sum = static_cast<int>(bc._globals._symbols.size());

		for(const auto& f: bc._function_defs){
			if(f._frame_ptr != nullptr){
				const auto instruction_count = f._frame_ptr->_instructions.size();
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
#endif









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

	)";

	run_closed(test_ss);
}

#if 0
QUARK_UNIT_TEST("", "try calling LLVM function", "", ""){
	const auto p = R"(

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
					"a": "my_gui"
				}
			}
		}

		func double my_gui__init() impure {
			return 3.14
		}

		func double my_gui(double state, json_value message) impure{
			print("received: " + to_string(message) + ", state: " + to_string(state))
			return state
		}

	)";

	const auto result = test_run_container2(p, {}, "iphone app", "");
	QUARK_UT_VERIFY(result.empty());
}
#endif

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

		func my_gui_state_t my_gui(my_gui_state_t state, json_value message) impure{
			print("received: " + to_string(message) + ", state: " + to_string(state))

			if(message == "inc"){
				return update(state, "_count", state._count + 1)
			}
			else if(message == "dec"){
				return update(state, "_count", state._count - 1)
			}
			else{
				assert(false)
				return state
			}
		}

	)";

	const auto result = test_run_container2(test_ss2, {}, "iphone app", "");
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
				return state
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
				return state
			}
		}

	)";

	const auto result = test_run_container2(test_ss3, {}, "iphone app", "");
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
				return state
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
				return state
			}
		}

	)";

	const auto result = test_run_container2(test_ss3, {}, "iphone app", "");
	QUARK_UT_VERIFY(result.empty());
}





///////////////////////////////////////////////////////////////////////////////////////
//	RUN ALL EXAMPLE PROGRAMS -- VERIFY THEY WORK
///////////////////////////////////////////////////////////////////////////////////////



QUARK_UNIT_TEST("Floyd test suite", "hello_world.floyd", "", ""){
	const auto path = get_working_dir() + "/examples/hello_world.floyd";
	const auto program = read_text_file(path);

	const auto result = test_run_container2(program, {}, "", "");
	const std::map<std::string, value_t> expected = {{ "global", value_t::make_void() }};
	QUARK_UT_VERIFY(result == expected);
}

QUARK_UNIT_TEST("Floyd test suite", "game_of_life.floyd", "", ""){
	const auto path = get_working_dir() + "/examples/game_of_life.floyd";
	const auto program = read_text_file(path);

	const auto result = test_run_container2(program, {}, "", "");
	const std::map<std::string, value_t> expected = {{ "global", value_t::make_void() }};
	QUARK_UT_VERIFY(result == expected);
}

#if 0
QUARK_UNIT_TEST("Floyd test suite", "process_test1.floyd", "", ""){
	const auto path = get_working_dir() + "/examples/process_test1.floyd";
	const auto program = read_text_file(path);

	const auto result = test_run_container2(program, {}, "iphone app", "");
	QUARK_UT_VERIFY(result.empty());
}
#endif




///////////////////////////////////////////////////////////////////////////////////////
//	QUICK REFERENCE SNIPPETS -- VERIFY THEY WORK
///////////////////////////////////////////////////////////////////////////////////////




#define QUICK_REFERENCE_TEST	QUARK_UNIT_TEST


QUICK_REFERENCE_TEST("QUICK REFERENCE SNIPPETS", "TERNARY OPERATOR", "", ""){
	test_run_container2(R"(

//	Snippets setup
let b = ""



let a = b == "true" ? true : false

	)", {}, "", "");
}

QUICK_REFERENCE_TEST("QUICK REFERENCE SNIPPETS", "COMMENTS", "", ""){
	test_run_container2(R"(

/* Comment can span lines. */

let a = 10; // To end of line

	)", {}, "", "");
}



QUICK_REFERENCE_TEST("QUICK REFERENCE SNIPPETS", "LOCALS", "", ""){
	test_run_container2(R"(

let a = 10
mutable b = 10
b = 11

	)", {}, "", "");
}


#if 0
QUICK_REFERENCE_TEST("QUICK REFERENCE SNIPPETS", "WHILE", "", ""){
	//	Just make sure it compiles, don't run it!
	ut_run_closed(R"(

//	Snippets setup
let expression = true


while(expression){
	print("body")
}

	)",
	compilation_unit_mode::k_no_core_lib
	);
}
#endif


QUICK_REFERENCE_TEST("QUICK REFERENCE SNIPPETS", "FOR", "", ""){
	test_run_container2(R"(

for (index in 1 ... 5) {
	print(index)
}
for (index in 1  ..< 5) {
	print(index)
}

	)", {}, "", "");
}


QUICK_REFERENCE_TEST("QUICK REFERENCE SNIPPETS", "IF ELSE", "", ""){
	test_run_container2(R"(

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

	)", {}, "", "");
}


QUICK_REFERENCE_TEST("QUICK REFERENCE SNIPPETS", "BOOL", "", ""){
	test_run_container2(R"(

let a = true
if(a){
}

	)", {}, "", "");
}

QUICK_REFERENCE_TEST("QUICK REFERENCE SNIPPETS", "STRING", "", ""){
	test_run_container2(R"(

let s1 = "Hello, world!"
let s2 = "Title: " + s1
assert(s2 == "Title: Hello, world!")
assert(s1[0] == 72) // ASCII for 'H'
assert(size(s1) == 13)
assert(subset(s1, 1, 4) == "ell")
let s4 = to_string(12003)

	)", {}, "", "");
}




QUICK_REFERENCE_TEST("QUICK REFERENCE SNIPPETS", "FUNCTION", "", ""){
	test_run_container2(R"(

func string f(double a, string s){
	return to_string(a) + ":" + s
}

let a = f(3.14, "km")

	)", {}, "", "");
}

QUICK_REFERENCE_TEST("QUICK REFERENCE SNIPPETS", "IMPURE FUNCTION", "", ""){
	test_run_container2(R"(

func int main([string] args) impure {
	return 1
}

	)", {}, "", "");
}



QUICK_REFERENCE_TEST("QUICK REFERENCE SNIPPETS", "STRUCT", "", ""){
	test_run_container2(R"(

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

	)", {}, "", "");
}


QUICK_REFERENCE_TEST("QUICK REFERENCE SNIPPETS", "VECTOR", "", ""){
	test_run_container2(R"(

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

	)", {}, "", "");
}


QUICK_REFERENCE_TEST("QUICK REFERENCE SNIPPETS", "DICTIONARY", "", ""){
	test_run_container2(R"(

let a = { "one": 1, "two": 2 }
assert(a["one"] == 1)

let b = update(a, "one", 10)
assert(a == { "one": 1, "two": 2 })
assert(b == { "one": 10, "two": 2 })

	)", {}, "", "");
}


QUICK_REFERENCE_TEST("QUICK REFERENCE SNIPPETS", "JSON_VALUE", "", ""){
	test_run_container2(R"(

let json_value a = {
	"one": 1,
	"three": "three",
	"five": { "A": 10, "B": 20 }
}

	)", {}, "", "");
}





QUICK_REFERENCE_TEST("QUICK REFERENCE SNIPPETS", "MAP", "", ""){
	test_run_container2(R"(

		func int f(int v){
			return 1000 + v
		}
		let result = map([ 10, 11, 12 ], f)
		assert(result == [ 1010, 1011, 1012 ])

	)", {}, "", "");
}





///////////////////////////////////////////////////////////////////////////////////////
//	MANUAL SNIPPETS -- VERIFY THEY WORK
///////////////////////////////////////////////////////////////////////////////////////

#define MANUAL_SNIPPETS_TEST	QUARK_UNIT_TEST

MANUAL_SNIPPETS_TEST("MANUAL SNIPPETS", "subset()", "", ""){
	test_run_container2(R"(

		assert(subset("hello", 2, 4) == "ll")
		assert(subset([ 10, 20, 30, 40 ], 1, 3 ) == [ 20, 30 ])

	)", {}, "", "");
}

