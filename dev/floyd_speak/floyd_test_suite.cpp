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
#include "file_handling.h"
#include "compiler_helpers.h"
#include "floyd_filelib.h"

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

//??? Add tests for arithmetic bounds, limiting, wrapping.
//??? Test truthness off all variable types: strings, doubles



//#define FLOYD_LANG_PROOF QUARK_UNIT_TEST



#define FLOYD_LANG_PROOF(class_under_test, function_under_test, scenario, expected_result) \
	static void QUARK_UNIQUE_LABEL(cppext_unit_test_)(); \
	static ::quark::unit_test_rec QUARK_UNIQUE_LABEL(rec)(__FILE__, __LINE__, class_under_test, function_under_test, scenario, expected_result, QUARK_UNIQUE_LABEL(cppext_unit_test_), false); \
	static void QUARK_UNIQUE_LABEL(cppext_unit_test_)()

#define FLOYD_LANG_PROOF_VIP(class_under_test, function_under_test, scenario, expected_result) \
	static void QUARK_UNIQUE_LABEL(cppext_unit_test_)(); \
	static ::quark::unit_test_rec QUARK_UNIQUE_LABEL(rec)(__FILE__, __LINE__, class_under_test, function_under_test, scenario, expected_result, QUARK_UNIQUE_LABEL(cppext_unit_test_), true); \
	static void QUARK_UNIQUE_LABEL(cppext_unit_test_)()




FLOYD_LANG_PROOF("Floyd test suite", "", "", ""){
	ut_verify_global_result_nolib(QUARK_POS, "let int result = 123", value_t::make_int(123));
}
FLOYD_LANG_PROOF("Floyd test suite", "", "", ""){
	test_floyd(
		QUARK_POS,
		make_compilation_unit("let int result = 123", "", compilation_unit_mode::k_no_core_lib),
		{},
		check_result(value_t::make_int(123))
	);
}
FLOYD_LANG_PROOF("Floyd test suite", "", "", ""){
	test_floyd(
		QUARK_POS,
		make_compilation_unit("let int result = 123", "", compilation_unit_mode::k_no_core_lib),
		{},
		check_result(value_t::make_int(123))
	);
}





//######################################################################################################################
//	DEFINE VARIABLE, SIMPLE TYPES
//######################################################################################################################


FLOYD_LANG_PROOF("Floyd test suite", "Define variable", "int", ""){
	ut_verify_global_result_nolib(QUARK_POS, "let int result = 123", value_t::make_int(123));
}

FLOYD_LANG_PROOF("Floyd test suite", "Define variable", "bool", ""){
	ut_verify_global_result_nolib(QUARK_POS, "let bool result = true", value_t::make_bool(true));
}
FLOYD_LANG_PROOF("Floyd test suite", "Define variable", "bool", ""){
	ut_verify_global_result_nolib(QUARK_POS, "let bool result = false", value_t::make_bool(false));
}

FLOYD_LANG_PROOF("Floyd test suite", "Define variable", "int", ""){
	ut_verify_global_result_nolib(QUARK_POS, "let int result = 123", value_t::make_int(123));
}

FLOYD_LANG_PROOF("Floyd test suite", "Define variable", "double", ""){
	ut_verify_global_result_nolib(QUARK_POS, "let double result = 3.5", value_t::make_double(double(3.5)));
}

FLOYD_LANG_PROOF("Floyd test suite", "Define variable", "string", ""){
	ut_verify_global_result_nolib(QUARK_POS, R"xxx(let string result = "xyz")xxx", value_t::make_string("xyz"));
}

//	??? Add special error message when local is not initialized.
FLOYD_LANG_PROOF("Floyd test suite", "Define variable", "Error: No initial assignment", "compiler error"){
	ut_verify_exception_nolib(
		QUARK_POS,
		R"(

			mutable string row

		)",
		R"___(Expected '=' character. Line: 3 "mutable string row")___"
	);
}
FLOYD_LANG_PROOF("Floyd test suite", "Define variable", "Error: assign to immutable local", "exception"){
	ut_verify_exception_nolib(
		QUARK_POS,
		R"(

			let int a = 10
			let int a = 11

		)",
		"Local identifier \"a\" already exists. Line: 4 \"let int a = 11\""
	);
}

FLOYD_LANG_PROOF("Floyd test suite", "Define variable", "Error: assign to unknown local", "exception"){
	ut_verify_exception_nolib(
		QUARK_POS,
		R"(

			a = 10

		)",
		"Unknown identifier 'a'. Line: 3 \"a = 10\""
	);
}




//######################################################################################################################
//	BASIC EXPRESSIONS
//######################################################################################################################



//////////////////////////////////////////		BASIC EXPRESSIONS, SIMPLE TYPES


FLOYD_LANG_PROOF("Floyd test suite", "+", "", "") {
	ut_verify_global_result_nolib(QUARK_POS, "let int result = 1 + 2", value_t::make_int(3));
}
FLOYD_LANG_PROOF("Floyd test suite", "+", "", ""){
	ut_verify_global_result_nolib(QUARK_POS, "let int result = 1 + 2 + 3", value_t::make_int(6));
}
FLOYD_LANG_PROOF("Floyd test suite", "*", "", ""){
	ut_verify_global_result_nolib(QUARK_POS, "let int result = 3 * 4", value_t::make_int(12));
}

FLOYD_LANG_PROOF("Floyd test suite", "parant", "", ""){
	ut_verify_global_result_nolib(QUARK_POS, "let int result = (3 * 4) * 5", value_t::make_int(60));
}



//////////////////////////////////////////		BASIC EXPRESSIONS - CONDITIONAL EXPRESSION


FLOYD_LANG_PROOF("Floyd test suite", "conditional expression", "", ""){
	ut_verify_global_result_nolib(QUARK_POS, "let int result = true ? 98 : 99", value_t::make_int(98));
}
FLOYD_LANG_PROOF("Floyd test suite", "conditional expression", "", ""){
	ut_verify_global_result_nolib(QUARK_POS, "let int result = false ? 70 : 80", value_t::make_int(80));
}


FLOYD_LANG_PROOF("Floyd test suite", "conditional expression", "", ""){
	ut_verify_global_result_nolib(QUARK_POS, "let string result = true ? \"yes\" : \"no\"", value_t::make_string("yes"));
}
FLOYD_LANG_PROOF("Floyd test suite", "conditional expression", "", ""){
	ut_verify_global_result_nolib(QUARK_POS, "let string result = false ? \"yes\" : \"no\"", value_t::make_string("no"));
}

FLOYD_LANG_PROOF("Floyd test suite", "conditional expression", "?:", ""){
	ut_verify_global_result_nolib(QUARK_POS, "let int result = true ? 4 : 6", value_t::make_int(4));
}
FLOYD_LANG_PROOF("Floyd test suite", "conditional expression", "?:", ""){
	ut_verify_global_result_nolib(QUARK_POS, "let int result = false ? 4 : 6", value_t::make_int(6));
}

FLOYD_LANG_PROOF("Floyd test suite", "conditional expression", "?:", ""){
	ut_verify_global_result_nolib(QUARK_POS, "let int result = 1==3 ? 4 : 6", value_t::make_int(6));
}
FLOYD_LANG_PROOF("Floyd test suite", "conditional expression", "?:", ""){
	ut_verify_global_result_nolib(QUARK_POS, "let int result = 3==3 ? 4 : 6", value_t::make_int(4));
}

FLOYD_LANG_PROOF("Floyd test suite", "conditional expression", "?:", ""){
	ut_verify_global_result_nolib(QUARK_POS, "let int result = 3==3 ? 2 + 2 : 2 * 3", value_t::make_int(4));
}

FLOYD_LANG_PROOF("Floyd test suite", "conditional expression", "?:", ""){
	ut_verify_global_result_nolib(QUARK_POS, "let int result = 3==1+2 ? 2 + 2 : 2 * 3", value_t::make_int(4));
}


//////////////////////////////////////////		BASIC EXPRESSIONS - PARANTHESES


FLOYD_LANG_PROOF("Floyd test suite", "execute_expression()", "Parentheses", ""){
	ut_verify_global_result_nolib(QUARK_POS, "let int result = 5*(4+4+1)", value_t::make_int(45));
}
FLOYD_LANG_PROOF("Floyd test suite", "execute_expression()", "Parentheses", ""){
	ut_verify_global_result_nolib(QUARK_POS, "let int result = 5*(2*(1+3)+1)", value_t::make_int(45));
}
FLOYD_LANG_PROOF("Floyd test suite", "execute_expression()", "Parentheses", ""){
	ut_verify_global_result_nolib(QUARK_POS, "let int result = 5*((1+3)*2+1)", value_t::make_int(45));
}

FLOYD_LANG_PROOF("Floyd test suite", "execute_expression()", "Sign before parentheses", ""){
	ut_verify_global_result_nolib(QUARK_POS, "let int result = -(2+1)*4", value_t::make_int(-12));
}
FLOYD_LANG_PROOF("Floyd test suite", "execute_expression()", "Sign before parentheses", ""){
	ut_verify_global_result_nolib(QUARK_POS, "let int result = -4*(2+1)", value_t::make_int(-12));
}


//////////////////////////////////////////		BASIC EXPRESSIONS - SPACES


FLOYD_LANG_PROOF("Floyd test suite", "execute_expression()", "Spaces", ""){
	ut_verify_global_result_nolib(QUARK_POS, "let int result = 5 * ((1 + 3) * 2 + 1)", value_t::make_int(45));
}
FLOYD_LANG_PROOF("Floyd test suite", "execute_expression()", "Spaces", ""){
	ut_verify_global_result_nolib(QUARK_POS, "let int result = 5 - 2 * ( 3 )", value_t::make_int(-1));
}
FLOYD_LANG_PROOF("Floyd test suite", "execute_expression()", "Spaces", ""){
	ut_verify_global_result_nolib(QUARK_POS, "let int result =  5 - 2 * ( ( 4 )  - 1 )", value_t::make_int(-1));
}


//////////////////////////////////////////		BASIC EXPRESSIONS - DOUBLE


FLOYD_LANG_PROOF("Floyd test suite", "execute_expression()", "Fractional numbers", "") {
	ut_verify_global_result_nolib(QUARK_POS, "let double result = 2.8/2.0", value_t::make_double(1.4));
}
FLOYD_LANG_PROOF("Floyd test suite", "execute_expression()", "Fractional numbers", ""){
//	ut_verify_global_result_nolib(QUARK_POS, "int result = 1/5e10") == 2e-11);
}
FLOYD_LANG_PROOF("Floyd test suite", "execute_expression()", "Fractional numbers", ""){
	ut_verify_global_result_nolib(QUARK_POS, "let double result = (4.0-3.0)/(4.0*4.0)", value_t::make_double(0.0625));
}
FLOYD_LANG_PROOF("Floyd test suite", "execute_expression()", "Fractional numbers", ""){
	ut_verify_global_result_nolib(QUARK_POS, "let double result = 1.0/2.0/2.0", value_t::make_double(0.25));
}
FLOYD_LANG_PROOF("Floyd test suite", "execute_expression()", "Fractional numbers", ""){
	ut_verify_global_result_nolib(QUARK_POS, "let double result = 0.25 * .5 * 0.5", value_t::make_double(0.0625));
}
FLOYD_LANG_PROOF("Floyd test suite", "execute_expression()", "Fractional numbers", ""){
	ut_verify_global_result_nolib(QUARK_POS, "let double result = .25 / 2.0 * .5", value_t::make_double(0.0625));
}

//////////////////////////////////////////		BASIC EXPRESSIONS - EDGE CASES


FLOYD_LANG_PROOF("Floyd test suite", "execute_expression()", "Repeated operators", ""){
	ut_verify_global_result_nolib(QUARK_POS, "let int result = 1+-2", value_t::make_int(-1));
}
FLOYD_LANG_PROOF("Floyd test suite", "execute_expression()", "Repeated operators", ""){
	ut_verify_global_result_nolib(QUARK_POS, "let int result = --2", value_t::make_int(2));
}
FLOYD_LANG_PROOF("Floyd test suite", "execute_expression()", "Repeated operators", ""){
	ut_verify_global_result_nolib(QUARK_POS, "let int result = 2---2", value_t::make_int(0));
}
FLOYD_LANG_PROOF("Floyd test suite", "execute_expression()", "Repeated operators", ""){
	ut_verify_global_result_nolib(QUARK_POS, "let int result = 2-+-2", value_t::make_int(4));
}

FLOYD_LANG_PROOF("Floyd test suite", "int", "Wrong number of arguments to int-constructor", "exception"){
	ut_verify_exception_nolib(
		QUARK_POS,
		R"(

			let a = int()

		)",
		"Wrong number of arguments in function call, got 0, expected 1. Line: 3 \"let a = int()\""
	);
}

FLOYD_LANG_PROOF("Floyd test suite", "Expression", "Error: mix types", "exception"){
	ut_verify_exception_nolib(
		QUARK_POS,
		R"(

			let a = 3 < "hello"

		)",
		"Expression type mismatch - cannot convert 'string' to 'int. Line: 3 \"let a = 3 < \"hello\"\""
	);
}

FLOYD_LANG_PROOF("Floyd test suite", "Expression", "Error: mix types", "exception"){
	ut_verify_exception_nolib(
		QUARK_POS,
		R"(

			let a = 3 * 3.2

		)",
		"Expression type mismatch - cannot convert 'double' to 'int. Line: 3 \"let a = 3 * 3.2\""
	);
}


//////////////////////////////////////////		BASIC EXPRESSIONS - BOOL


FLOYD_LANG_PROOF("Floyd test suite", "bool constructor", "", ""){
	ut_verify_global_result_nolib(QUARK_POS, "let bool result = true", value_t::make_bool(true));
}
FLOYD_LANG_PROOF("Floyd test suite", "bool constructor", "", ""){
	ut_verify_global_result_nolib(QUARK_POS, "let bool result = false", value_t::make_bool(false));
}

FLOYD_LANG_PROOF("Floyd test suite", "bool +", "", ""){
	ut_verify_global_result_nolib(
		QUARK_POS,
		R"(

			let result = false + true

		)",
		value_t::make_bool(true)
	);
}
FLOYD_LANG_PROOF("Floyd test suite", "bool +", "", ""){
	ut_verify_global_result_nolib(QUARK_POS, R"(		let result = false + false		)", value_t::make_bool(false));
}
FLOYD_LANG_PROOF("Floyd test suite", "bool +", "", ""){
	ut_verify_global_result_nolib(QUARK_POS, R"(		let result = true + true		)", value_t::make_bool(true));
}





//////////////////////////////////////////		BASIC EXPRESSIONS - OPERATOR ==


FLOYD_LANG_PROOF("Floyd test suite", "execute_expression()", "==", ""){
	ut_verify_global_result_nolib(QUARK_POS, "let bool result = 1 == 1", value_t::make_bool(true));
}
FLOYD_LANG_PROOF("Floyd test suite", "execute_expression()", "==", ""){
	ut_verify_global_result_nolib(QUARK_POS, "let bool result = 1 == 2", value_t::make_bool(false));
}
FLOYD_LANG_PROOF("Floyd test suite", "execute_expression()", "==", ""){
	ut_verify_global_result_nolib(QUARK_POS, "let bool result = 1.3 == 1.3", value_t::make_bool(true));
}
FLOYD_LANG_PROOF("Floyd test suite", "execute_expression()", "==", ""){
	ut_verify_global_result_nolib(QUARK_POS, "let bool result = \"hello\" == \"hello\"", value_t::make_bool(true));
}
FLOYD_LANG_PROOF("Floyd test suite", "execute_expression()", "==", ""){
	ut_verify_global_result_nolib(QUARK_POS, "let bool result = \"hello\" == \"bye\"", value_t::make_bool(false));
}


//////////////////////////////////////////		BASIC EXPRESSIONS - OPERATOR <


FLOYD_LANG_PROOF("Floyd test suite", "execute_expression()", "<", "") {
	ut_verify_global_result_nolib(QUARK_POS, "let bool result = 1 < 2", value_t::make_bool(true));
}
FLOYD_LANG_PROOF("Floyd test suite", "execute_expression()", "<", ""){
	ut_verify_global_result_nolib(QUARK_POS, "let bool result = 5 < 2", value_t::make_bool(false));
}
FLOYD_LANG_PROOF("Floyd test suite", "execute_expression()", "<", ""){
	ut_verify_global_result_nolib(QUARK_POS, "let bool result = 0.3 < 0.4", value_t::make_bool(true));
}
FLOYD_LANG_PROOF("Floyd test suite", "execute_expression()", "<", ""){
	ut_verify_global_result_nolib(QUARK_POS, "let bool result = 1.5 < 0.4", value_t::make_bool(false));
}
FLOYD_LANG_PROOF("Floyd test suite", "execute_expression()", "<", ""){
	ut_verify_global_result_nolib(QUARK_POS, "let bool result = \"adwark\" < \"boat\"", value_t::make_bool(true));
}
FLOYD_LANG_PROOF("Floyd test suite", "execute_expression()", "<", ""){
	ut_verify_global_result_nolib(QUARK_POS, "let bool result = \"boat\" < \"adwark\"", value_t::make_bool(false));
}


//////////////////////////////////////////		BASIC EXPRESSIONS - OPERATOR &&

FLOYD_LANG_PROOF("Floyd test suite", "execute_expression()", "&&", "") {
	ut_verify_global_result_nolib(QUARK_POS, "let bool result = false && false", value_t::make_bool(false));
}
FLOYD_LANG_PROOF("Floyd test suite", "execute_expression()", "&&", ""){
	ut_verify_global_result_nolib(QUARK_POS, "let bool result = false && true", value_t::make_bool(false));
}
FLOYD_LANG_PROOF("Floyd test suite", "execute_expression()", "&&", ""){
	ut_verify_global_result_nolib(QUARK_POS, "let bool result = true && false", value_t::make_bool(false));
}
FLOYD_LANG_PROOF("Floyd test suite", "execute_expression()", "&&", ""){
	ut_verify_global_result_nolib(QUARK_POS, "let bool result = true && true", value_t::make_bool(true));
}

FLOYD_LANG_PROOF("Floyd test suite", "execute_expression()", "&&", ""){
	ut_verify_global_result_nolib(QUARK_POS, "let bool result = false && false && false", value_t::make_bool(false));
}
FLOYD_LANG_PROOF("Floyd test suite", "execute_expression()", "&&", ""){
	ut_verify_global_result_nolib(QUARK_POS, "let bool result = false && false && true", value_t::make_bool(false));
}
FLOYD_LANG_PROOF("Floyd test suite", "execute_expression()", "&&", ""){
	ut_verify_global_result_nolib(QUARK_POS, "let bool result = false && true && false", value_t::make_bool(false));
}
FLOYD_LANG_PROOF("Floyd test suite", "execute_expression()", "&&", ""){
	ut_verify_global_result_nolib(QUARK_POS, "let bool result = false && true && true", value_t::make_bool(false));
}
FLOYD_LANG_PROOF("Floyd test suite", "execute_expression()", "&&", ""){
	ut_verify_global_result_nolib(QUARK_POS, "let bool result = true && false && false", value_t::make_bool(false));
}
FLOYD_LANG_PROOF("Floyd test suite", "execute_expression()", "&&", ""){
	ut_verify_global_result_nolib(QUARK_POS, "let bool result = true && true && false", value_t::make_bool(false));
}
FLOYD_LANG_PROOF("Floyd test suite", "execute_expression()", "&&", ""){
	ut_verify_global_result_nolib(QUARK_POS, "let bool result = true && true && true", value_t::make_bool(true));
}

//////////////////////////////////////////		BASIC EXPRESSIONS - OPERATOR ||

FLOYD_LANG_PROOF("Floyd test suite", "execute_expression()", "||", ""){
	ut_verify_global_result_nolib(QUARK_POS, "let bool result = false || false", value_t::make_bool(false));
}
FLOYD_LANG_PROOF("Floyd test suite", "execute_expression()", "||", ""){
	ut_verify_global_result_nolib(QUARK_POS, "let bool result = false || true", value_t::make_bool(true));
}
FLOYD_LANG_PROOF("Floyd test suite", "execute_expression()", "||", ""){
	ut_verify_global_result_nolib(QUARK_POS, "let bool result = true || false", value_t::make_bool(true));
}
FLOYD_LANG_PROOF("Floyd test suite", "execute_expression()", "||", ""){
	ut_verify_global_result_nolib(QUARK_POS, "let bool result = true || true", value_t::make_bool(true));
}


FLOYD_LANG_PROOF("Floyd test suite", "execute_expression()", "||", ""){
	ut_verify_global_result_nolib(QUARK_POS, "let bool result = false || false || false", value_t::make_bool(false));
}
FLOYD_LANG_PROOF("Floyd test suite", "execute_expression()", "||", ""){
	ut_verify_global_result_nolib(QUARK_POS, "let bool result = false || false || true", value_t::make_bool(true));
}
FLOYD_LANG_PROOF("Floyd test suite", "execute_expression()", "||", ""){
	ut_verify_global_result_nolib(QUARK_POS, "let bool result = false || true || false", value_t::make_bool(true));
}
FLOYD_LANG_PROOF("Floyd test suite", "execute_expression()", "||", ""){
	ut_verify_global_result_nolib(QUARK_POS, "let bool result = false || true || true", value_t::make_bool(true));
}
FLOYD_LANG_PROOF("Floyd test suite", "execute_expression()", "||", ""){
	ut_verify_global_result_nolib(QUARK_POS, "let bool result = true || false || false", value_t::make_bool(true));
}
FLOYD_LANG_PROOF("Floyd test suite", "execute_expression()", "||", ""){
	ut_verify_global_result_nolib(QUARK_POS, "let bool result = true || false || true", value_t::make_bool(true));
}
FLOYD_LANG_PROOF("Floyd test suite", "execute_expression()", "||", ""){
	ut_verify_global_result_nolib(QUARK_POS, "let bool result = true || true || false", value_t::make_bool(true));
}
FLOYD_LANG_PROOF("Floyd test suite", "execute_expression()", "||", ""){
	ut_verify_global_result_nolib(QUARK_POS, "let bool result = true || true || true", value_t::make_bool(true));
}




//////////////////////////////////////////		BASIC EXPRESSIONS - BITWISE OPERATORS


//	2^63: 9223372036854775808
//	2^64: 18446744073709551616

FLOYD_LANG_PROOF("Floyd test suite", "", "bw_not()", ""){
	ut_verify_global_result_nolib(QUARK_POS, "let result = bw_not(0)", value_t::make_int(0b11111111'11111111'11111111'11111111'11111111'11111111'11111111'11111111));
}
FLOYD_LANG_PROOF("Floyd test suite", "", "bw_not()", ""){
	ut_verify_global_result_nolib(QUARK_POS, "let result = bw_not(18446744073709551615)", value_t::make_int(0b00000000'00000000'00000000'00000000'00000000'00000000'00000000'00000000));
}
FLOYD_LANG_PROOF("Floyd test suite", "", "bw_not()", ""){
	ut_verify_global_result_nolib(QUARK_POS, "let result = bw_not(1)", value_t::make_int(0b11111111'11111111'11111111'11111111'11111111'11111111'11111111'11111110));
}
FLOYD_LANG_PROOF("Floyd test suite", "", "bw_not()", ""){
	ut_verify_global_result_nolib(QUARK_POS, "let result = bw_not(9223372036854775808)", value_t::make_int(0));
}




FLOYD_LANG_PROOF("Floyd test suite", "", "bw_and()", ""){
	ut_verify_global_result_nolib(QUARK_POS, "let result = bw_and(3, 1)", value_t::make_int(0b00000000'00000000'00000000'00000000'00000000'00000000'00000000'00000001));
}



FLOYD_LANG_PROOF("Floyd test suite", "", "bw_or()", ""){
	ut_verify_global_result_nolib(QUARK_POS, "let result = bw_or(2, 1)", value_t::make_int(0b00000000'00000000'00000000'00000000'00000000'00000000'00000000'00000011));
}



FLOYD_LANG_PROOF("Floyd test suite", "", "bw_xor()", ""){
	ut_verify_global_result_nolib(QUARK_POS, "let result = bw_xor(3, 1)", value_t::make_int(0b00000000'00000000'00000000'00000000'00000000'00000000'00000000'00000010));
}

FLOYD_LANG_PROOF("Floyd test suite", "", "bw_shift_left()", ""){
	ut_verify_global_result_nolib(QUARK_POS, "let result = bw_shift_left(3, 1)", value_t::make_int(0b00000000'00000000'00000000'00000000'00000000'00000000'00000000'00000110));
}

FLOYD_LANG_PROOF("Floyd test suite", "", "bw_shift_right()", ""){
	ut_verify_global_result_nolib(QUARK_POS, "let result = bw_shift_right(3, 1)", value_t::make_int(0b00000000'00000000'00000000'00000000'00000000'00000000'00000000'00000001));
}



FLOYD_LANG_PROOF("Floyd test suite", "", "floyd_funcdef__bw_shift_right_arithmetic()", ""){
	ut_verify_global_result_nolib(QUARK_POS, "let result = bw_shift_right_arithmetic(7, 1)", value_t::make_int(0b00000000'00000000'00000000'00000000'00000000'00000000'00000000'00000011));
}
FLOYD_LANG_PROOF("Floyd test suite", "", "floyd_funcdef__bw_shift_right_arithmetic()", ""){
	ut_verify_global_result_nolib(QUARK_POS, "let result = bw_shift_right_arithmetic(-2, 1)", value_t::make_int(-1));
}


//??? more



//////////////////////////////////////////		BASIC EXPRESSIONS - ERRORS



FLOYD_LANG_PROOF("Floyd test suite", "execute_expression()", "Type mismatch", "") {
	ut_verify_exception_nolib(
		QUARK_POS,
		"let int result = true",
		"Expression type mismatch - cannot convert 'bool' to 'int. Line: 1 \"let int result = true\""
	);
}

#if 0
FLOYD_LANG_PROOF("Floyd test suite", "execute_expression()", "Division by zero", "") {
	ut_verify_exception_nolib(QUARK_POS, "let int result = 2/0", "EEE_DIVIDE_BY_ZERO");
}

FLOYD_LANG_PROOF("Floyd test suite", "execute_expression()", "Division by zero", ""){
	ut_verify_exception_nolib(QUARK_POS, "let int result = 3+1/(5-5)+4", "EEE_DIVIDE_BY_ZERO");
}
FLOYD_LANG_PROOF("Floyd test suite", "execute_expression()", "Division by zero", ""){
	ut_verify_exception_nolib(QUARK_POS, "let double result = 2.0 / 1.0", "EEE_DIVIDE_BY_ZERO");
}
#endif

FLOYD_LANG_PROOF("Floyd test suite", "execute_expression()", "-true", "") {
	ut_verify_exception_nolib(
		QUARK_POS,
		"let int result = -true",
		"Unary minus don't work on expressions of type \"bool\", only int and double. Line: 1 \"let int result = -true\""
	);
}

FLOYD_LANG_PROOF("Floyd test suite", "Forgot let or mutable", "", "Exception"){
	ut_verify_exception_nolib(QUARK_POS, "int test = 123", "Use 'mutable' or 'let' syntax. Line: 1 \"int test = 123\"");
}

FLOYD_LANG_PROOF("Floyd test suite", "Access variable", "Access undefined variable", "exception"){
	ut_verify_exception_nolib(QUARK_POS, R"(		print(a)		)", "Undefined variable \"a\". Line: 1 \"print(a)\"");
}



//////////////////////////////////////////		TEST CONSTRUCTOR - SIMPLE TYPES


FLOYD_LANG_PROOF("Floyd test suite", "Construct value", "bool()", ""){
	ut_verify_global_result_nolib(QUARK_POS, "let result = bool(false)", value_t::make_bool(false));
}
FLOYD_LANG_PROOF("Floyd test suite", "Construct value", "bool()", ""){
	ut_verify_global_result_nolib(QUARK_POS, "let result = bool(true)", value_t::make_bool(true));
}
FLOYD_LANG_PROOF("Floyd test suite", "Construct value", "int()", ""){
	ut_verify_global_result_nolib(QUARK_POS, "let result = int(123)", value_t::make_int(123));
}
FLOYD_LANG_PROOF("Floyd test suite", "Construct value", "double()", ""){
	ut_verify_global_result_nolib(QUARK_POS, "let result = double(0.0)", value_t::make_double(0.0));
}
FLOYD_LANG_PROOF("Floyd test suite", "Construct value", "double()", ""){
	ut_verify_global_result_nolib(QUARK_POS, "let result = double(123.456)", value_t::make_double(123.456));
}

FLOYD_LANG_PROOF("Floyd test suite", "Construct value", "string()", ""){
	ut_verify_exception_nolib(QUARK_POS, "let result = string()", "Wrong number of arguments in function call, got 0, expected 1. Line: 1 \"let result = string()\"");
}

FLOYD_LANG_PROOF("Floyd test suite", "Construct value", "string()", ""){
	ut_verify_global_result_nolib(QUARK_POS, "let result = string(\"ABCD\")", value_t::make_string("ABCD"));
}



//??? add test for irregular dividers '


//////////////////////////////////////////		TEST CHARACTER LITERALS


FLOYD_LANG_PROOF("Floyd test suite", "Character literal", "", ""){
	ut_verify_global_result_nolib(QUARK_POS, "let result = 'A'", value_t::make_int(65));
}

FLOYD_LANG_PROOF("Floyd test suite", "Character literal", "Escape \0", ""){
	ut_verify_global_result_nolib(QUARK_POS, R"___(let result = '\0')___", value_t::make_int('\0'));
}
FLOYD_LANG_PROOF("Floyd test suite", "Character literal", "Escape \t", ""){
	ut_verify_global_result_nolib(QUARK_POS, R"___(let result = '\t')___", value_t::make_int('\t'));
}
FLOYD_LANG_PROOF("Floyd test suite", "Character literal", "Escape \\", ""){
	ut_verify_global_result_nolib(QUARK_POS, R"___(let result = '\\')___", value_t::make_int('\\'));
}
FLOYD_LANG_PROOF("Floyd test suite", "Character literal", "Escape \n", ""){
	ut_verify_global_result_nolib(QUARK_POS, R"___(let result = '\n')___", value_t::make_int('\n'));
}
FLOYD_LANG_PROOF("Floyd test suite", "Character literal", "Escape \r", ""){
	ut_verify_global_result_nolib(QUARK_POS, R"___(let result = '\r')___", value_t::make_int('\r'));
}
FLOYD_LANG_PROOF("Floyd test suite", "Character literal", "Escape \"", ""){
	ut_verify_global_result_nolib(QUARK_POS, R"___(let result = '\"')___", value_t::make_int('\"'));
}
FLOYD_LANG_PROOF("Floyd test suite", "Character literal", "Escape \'", ""){
	ut_verify_global_result_nolib(QUARK_POS, R"___(let result = '\'')___", value_t::make_int('\''));
}
FLOYD_LANG_PROOF("Floyd test suite", "Character literal", "Escape \'", ""){
	ut_verify_global_result_nolib(QUARK_POS, R"___(let result = '\/')___", value_t::make_int('/'));
}



//////////////////////////////////////////		TEST BINARY LITERALS


FLOYD_LANG_PROOF("Floyd test suite", "Binary literal", "", ""){
	ut_verify_global_result_nolib(QUARK_POS, "let result = 0b0", value_t::make_int(0b0));
}
FLOYD_LANG_PROOF("Floyd test suite", "Binary literal", "", ""){
	ut_verify_global_result_nolib(QUARK_POS, "let result = 0b10000000", value_t::make_int(0b10000000));
}
FLOYD_LANG_PROOF("Floyd test suite", "Binary literal", "", ""){
	ut_verify_global_result_nolib(QUARK_POS, "let result = 0b1000000000000000000000000000000000000000000000000000000000000001", value_t::make_int(0b1000000000000000000000000000000000000000000000000000000000000001));
}
FLOYD_LANG_PROOF("Floyd test suite", "Binary literal", "", ""){
	ut_verify_global_result_nolib(QUARK_POS, "let result = 0b10000000'00000000'00000000'00000000'00000000'00000000'00000000'00000001", value_t::make_int(0b10000000'00000000'00000000'00000000'00000000'00000000'00000000'00000001));
}


//////////////////////////////////////////		TEST HEXADECIMAL LITERALS


FLOYD_LANG_PROOF("Floyd test suite", "Hexadecimal literal", "", ""){
	ut_verify_global_result_nolib(QUARK_POS, "let result = 0x0", value_t::make_int(0x0));
}
FLOYD_LANG_PROOF("Floyd test suite", "Hexadecimal literal", "", ""){
	ut_verify_global_result_nolib(QUARK_POS, "let result = 0xff", value_t::make_int(0xff));
}
FLOYD_LANG_PROOF("Floyd test suite", "Hexadecimal literal", "", ""){
	ut_verify_global_result_nolib(QUARK_POS, "let result = 0xabcd", value_t::make_int(0xabcd));
}
FLOYD_LANG_PROOF("Floyd test suite", "Hexadecimal literal", "", ""){
	ut_verify_global_result_nolib(QUARK_POS, "let result = 0xabcdef01'23456789", value_t::make_int(0xabcdef01'23456789));
}
FLOYD_LANG_PROOF("Floyd test suite", "Hexadecimal literal", "", ""){
	ut_verify_global_result_nolib(QUARK_POS, "let result = 0xffffffff'ffffffff", value_t::make_int(0xffffffff'ffffffff));
}






//######################################################################################################################
//	MUTATE VARIABLES
//######################################################################################################################




FLOYD_LANG_PROOF("Floyd test suite", "Mutate", "mutate local", ""){
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

FLOYD_LANG_PROOF("Floyd test suite", "Mutate", "increment a mutable", "OK"){
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

FLOYD_LANG_PROOF("Floyd test suite", "Mutate", "Store to immutable", "compiler error"){
	ut_verify_exception_nolib(
		QUARK_POS,
		R"(

			let a = 3
			a = 4

		)",
		"Cannot assign to immutable identifier \"a\". Line: 4 \"a = 4\""
	);
}


FLOYD_LANG_PROOF("Floyd test suite", "", "String (which requires RC)", ""){
	ut_run_closed_nolib(R"___(

		func void f(){
			let s = "A"
		}

		f()

	)___");
}
FLOYD_LANG_PROOF("Floyd test suite", "Mutate", "String (which requires RC)", ""){
	ut_run_closed_nolib(R"___(

		func string f(){
			mutable s = "A"
			return s
		}

		f()

	)___");
}
FLOYD_LANG_PROOF("Floyd test suite", "Mutate", "String (which requires RC)", ""){
	ut_run_closed_nolib(R"___(

		func string f(){
			mutable s = "A"
			s = "<" + s + ">"
			return s
		}

		let result = f()
		assert(result == "<A>")

	)___");
}




//######################################################################################################################
//	CORE CALLS
//######################################################################################################################

//////////////////////////////////////////		CORE CALL - print()



FLOYD_LANG_PROOF("Floyd test suite", "Expression statement", "", ""){
	ut_verify_printout_nolib(QUARK_POS, "print(5)", { "5" });
}

FLOYD_LANG_PROOF("Floyd test suite", "print() supports ints and strings", "", ""){
	ut_verify_printout_nolib(
		QUARK_POS,
		R"(

			print("Hello, world!")
			print(123)

		)",
		{ "Hello, world!", "123" }
	);
}


//////////////////////////////////////////		CORE CALL - assert()

//??? add file + line to Floyd's asserts
FLOYD_LANG_PROOF("Floyd test suite", "", "", ""){
	ut_verify_exception_nolib(QUARK_POS, R"(		assert(1 == 2)		)", "Floyd assertion failed.");
}

FLOYD_LANG_PROOF("Floyd test suite", "", "", ""){
	ut_verify_printout_nolib(
		QUARK_POS,
		R"(

			assert(1 == 1)
			print("A")

		)",
		{ "A" }
	);
}

//////////////////////////////////////////		CORE CALL - to_string()


FLOYD_LANG_PROOF("Floyd test suite", "", "", ""){
	ut_verify_global_result_nolib(QUARK_POS, R"(		let string result = to_string(145)		)", value_t::make_string("145"));
}
FLOYD_LANG_PROOF("Floyd test suite", "", "", ""){
	ut_verify_global_result_nolib(QUARK_POS, R"(		let string result = to_string(3.1)		)", value_t::make_string("3.1"));
}

FLOYD_LANG_PROOF("Floyd test suite", "", "", ""){
	ut_verify_global_result_nolib(QUARK_POS, R"(		let string result = to_string(3.0)		)", value_t::make_string("3.0"));
}



//######################################################################################################################
//	MAIN()
//######################################################################################################################


FLOYD_LANG_PROOF("Floyd test suite", "main() - Can make and read global int", "", ""){
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

FLOYD_LANG_PROOF("Floyd test suite", "main()", "int main()", ""){
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




//######################################################################################################################
//	FUNCTION DEFINITION STATEMENT
//######################################################################################################################


FLOYD_LANG_PROOF("Floyd test suite", "func", "Simplest func", ""){
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

FLOYD_LANG_PROOF("Floyd test suite", "func", "Function with int argument", ""){
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

FLOYD_LANG_PROOF("Floyd test suite", "func", "define additional function, call it several times", ""){
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

FLOYD_LANG_PROOF("Floyd test suite", "func", "use function inputs", ""){
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


FLOYD_LANG_PROOF("Floyd test suite", "func", "test function args are always immutable", ""){
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

FLOYD_LANG_PROOF("Floyd test suite", "func", "Recursion: function calling itself by name", ""){
	ut_run_closed_nolib(R"(		func int fx(int a){ return fx(a + 1) }		)");
}

FLOYD_LANG_PROOF("Floyd test suite", "func", "return from void function", "error"){
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

FLOYD_LANG_PROOF("Floyd test suite", "func", "void function, no return statement", ""){
	ut_run_closed_nolib(
		R"(

			func void f (int x) impure {
			}

		)"
	);
}

FLOYD_LANG_PROOF("Floyd test suite", "Function value", "", ""){
	ut_verify_printout_nolib(
		QUARK_POS,
		R"(

			func int a(string s){
				return 2
			}
			print(a)

		)",
		{
			"function int(string) pure"
		 }
	);
}


FLOYD_LANG_PROOF("Floyd test suite", "func", "Declaration only", ""){
	ut_verify_printout_nolib(
		QUARK_POS,
		R"(

			func int f()

		)",
		{}
	);
}

FLOYD_LANG_PROOF("Floyd test suite", "func", "Declaration only, get its type", ""){
	ut_verify_printout_nolib(
		QUARK_POS,
		R"(

			func int f()
			print(f)

		)",
		{ "function int() pure" }
	);
}

FLOYD_LANG_PROOF("Floyd test suite", "No statements", "", ""){
	ut_verify_printout_nolib(
		QUARK_POS,
		R"(


		)",
		{}
	);
}

FLOYD_LANG_PROOF("Floyd test suite", "func", "Calling function without implementation", ""){
	ut_verify_exception_nolib(
		QUARK_POS,
		R"(

			func int f()
			f()

		)",
		"Attempting to calling unimplemented function."
	);
}




//######################################################################################################################
//	CALL EXPRESSION
//######################################################################################################################



FLOYD_LANG_PROOF("Floyd test suite", "Call", "Error: Wrong number of arguments in function call", "exception"){
	ut_verify_exception_nolib(
		QUARK_POS,
		R"(

			func int f(int a){ return a + 1 }
			let a = f(1, 2)

		)",
		"Wrong number of arguments in function call, got 2, expected 1. Line: 4 \"let a = f(1, 2)\""
	);
}

FLOYD_LANG_PROOF("Floyd test suite", "Call", "Call non-function, non-struct, non-typeid", "exception"){
	ut_verify_exception_nolib(QUARK_POS, R"(		let a = 3()		)", "Cannot call non-function, its type is int. Line: 1 \"let a = 3()\"");
}




//######################################################################################################################
//	RETURN STATEMENT
//######################################################################################################################



FLOYD_LANG_PROOF("Floyd test suite", "return", "return from middle of function", ""){
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

FLOYD_LANG_PROOF("Floyd test suite", "return", "return from within BLOCK", ""){
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

FLOYD_LANG_PROOF("Floyd test suite", "return", "Make sure returning wrong type => error", ""){
	try {
		test_run_container3(R"(

			func int f(){
				return "x"
			}

		)", {}, "");
	}
	catch(const std::runtime_error& e){
		ut_verify(QUARK_POS, e.what(), "Expression type mismatch - cannot convert 'string' to 'int. Line: 4 \"return \"x\"\"");
	}
}


FLOYD_LANG_PROOF("Floyd test suite", "return", "You can't return from global scope", "error"){
	ut_verify_exception_nolib(
		QUARK_POS,
		R"(

			return "hello"

		)",
		R"___(Cannot return value from function with void-return. Line: 3 "return "hello"")___"
	);
}


FLOYD_LANG_PROOF("Floyd test suite", "return", "return RC-value from nested block", ""){
	ut_verify_printout_nolib(
		QUARK_POS,
		R"(

			func string f(){
				let a = "test"
				if(true) {
					return "B"
				}
				else{
					return "C"
				}
			}
			print(f())

		)",
		{ "B" }
	);
}

FLOYD_LANG_PROOF("Floyd test suite", "return", "return RC-value from nested block", ""){
	ut_verify_printout_nolib(
		QUARK_POS,
		R"(
			let q = "q"

			func string f(){
				let a = "a"
				for(e in 0...3){
					let b = "b"
					if(true) {
						let c = "c"
						return "1"
					}
					else{
						let d = "d"
						return "2"
					}
				}
				return "3"
			}
			print(f())

		)",
		{ "1" }
	);
}




//######################################################################################################################
//	PURE & IMPURE FUNCTIONS
//######################################################################################################################


//??? It IS OK to call impure functions from a pure function:
//		1. The impure function modifies a local value only -- cannot leak out.

FLOYD_LANG_PROOF("Floyd test suite", "impure", "call pure->pure", "Compiles OK"){
	ut_run_closed_nolib(R"(

		func int a(int p){ return p + 1 }
		func int b(){ return a(100) }

	)");
}
FLOYD_LANG_PROOF("Floyd test suite", "impure", "call impure->pure", "Compiles OK"){
	ut_run_closed_nolib(R"(

		func int a(int p){ return p + 1 }
		func int b() impure { return a(100) }

	)");
}

FLOYD_LANG_PROOF("Floyd test suite", "impure", "call pure->impure", "Compilation error"){
	ut_verify_exception_nolib(
		QUARK_POS,
		R"(

			func int a(int p) impure { return p + 1 }
			func int b(){ return a(100) }

		)",
		"Cannot call impure function from a pure function. Line: 4 \"func int b(){ return a(100) }\""
	);
}

FLOYD_LANG_PROOF("Floyd test suite", "impure", "call impure->impure", "Compiles OK"){
	ut_run_closed_nolib(R"(

		func int a(int p) impure { return p + 1 }
		func int b() impure { return a(100) }

	)");
}





//######################################################################################################################
//	COMMENTS
//######################################################################################################################


FLOYD_LANG_PROOF("comments", "// on start of line", "", ""){
	ut_verify_printout_nolib(
		QUARK_POS,
		R"(

			//	XYZ
			print("Hello")

		)",
		{ "Hello" }
	);
}

FLOYD_LANG_PROOF("comments", "// on start of line", "", ""){
	ut_verify_printout_nolib(
		QUARK_POS,
		R"(

			print("Hello")		//	XYZ

		)",
		{ "Hello" }
	);
}

FLOYD_LANG_PROOF("comments", "// on start of line", "", ""){
	ut_verify_printout_nolib(
		QUARK_POS,
		R"(

			print("Hello")/* xyz */print("Bye")

		)",
		{ "Hello", "Bye" }
	);
}



//######################################################################################################################
//	BLOCK & SCOPES & LOCALS
//######################################################################################################################


FLOYD_LANG_PROOF("Floyd test suite", "Scopes: Empty block", "", ""){
	ut_run_closed_nolib("{}");
}

FLOYD_LANG_PROOF("Floyd test suite", "Scopes: Block with local variable, no shadowing", "", ""){
	ut_run_closed_nolib("{ let int x = 4 }");
}


FLOYD_LANG_PROOF("Floyd test suite", "Scopes: test reading from subscope", "", ""){
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

FLOYD_LANG_PROOF("Floyd test suite", "Scopes: test mutating from a subscope", "", ""){
	ut_verify_printout_nolib(
		QUARK_POS,
		R"(

			let a = 7
			print(a)

		)",
		{ "7" }
	);
}


FLOYD_LANG_PROOF("Floyd test suite", "Scopes: test scope ends", "", ""){
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

FLOYD_LANG_PROOF("Floyd test suite", "Scopes: test mutating from a subscope", "", ""){
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


FLOYD_LANG_PROOF("Floyd test suite", "Scopes: global", "", ""){
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


FLOYD_LANG_PROOF("Floyd test suite", "Scopes: Global block scopes, shadowing or not", "", ""){
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

FLOYD_LANG_PROOF("Floyd test suite", "Scopes: Three levels", "", ""){
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



FLOYD_LANG_PROOF("Floyd test suite", "Scopes: Global block scopes, shadowing or not", "", ""){
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

FLOYD_LANG_PROOF("Floyd test suite", "Scopes: Global block scopes, shadowing or not", "", ""){
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

FLOYD_LANG_PROOF("Floyd test suite", "Scopes: Function arguments & locals & blocks vs globals, shadowing or not", "", ""){
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

FLOYD_LANG_PROOF("Floyd test suite", "Scopes: All block scopes, shadowing or not", "", ""){
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



//######################################################################################################################
//	IF STATEMENT
//######################################################################################################################


FLOYD_LANG_PROOF("Floyd test suite", "if", "if(true){}", ""){
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

FLOYD_LANG_PROOF("Floyd test suite", "if", "if(false){}", ""){
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

FLOYD_LANG_PROOF("Floyd test suite", "if", "if(true){}else{}", ""){
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

FLOYD_LANG_PROOF("Floyd test suite", "if", "if(false){}else{}", ""){
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

FLOYD_LANG_PROOF("Floyd test suite", "if", "if-elseif-elseif-else TRUE-FALSE-FALSE", ""){
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

FLOYD_LANG_PROOF("Floyd test suite", "if", "if-elseif-elseif-else FALSE-TRUE-FALSE", ""){
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

FLOYD_LANG_PROOF("Floyd test suite", "if", "if-elseif-elseif-else FALSE-FALSE-TRUE", ""){
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

FLOYD_LANG_PROOF("Floyd test suite", "if", "if-elseif-elseif-else FALSE-FALSE-FALSE", ""){
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

FLOYD_LANG_PROOF("Floyd test suite", "if", "Error: if with non-bool expression", "exception"){
	ut_verify_exception_nolib(
		QUARK_POS,
		R"(

			if("not a bool"){ } else{ assert(false) }

		)",
		R"(Boolean condition required. Line: 3 "if("not a bool"){ } else{ assert(false) }")"
	);
}

FLOYD_LANG_PROOF("Floyd test suite", "if+scope: Make sure a function can access global independent on how it's called in callstack", "", ""){
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

FLOYD_LANG_PROOF("Floyd test suite", "if", "return from within IF block", ""){
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

FLOYD_LANG_PROOF("Floyd test suite", "if", "Make sure return from ELSE works", ""){
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

FLOYD_LANG_PROOF("Floyd test suite", "if", "Compiler error: not all paths returns", ""){
//	QUARK_UT_VERIFY(false);
}




//######################################################################################################################
//	FOR STATEMENT
//######################################################################################################################



FLOYD_LANG_PROOF("Floyd test suite", "for", "0 ... 0", ""){
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

FLOYD_LANG_PROOF("Floyd test suite", "for", "0 ... 1", ""){
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

FLOYD_LANG_PROOF("Floyd test suite", "for", "0 ... 2", ""){
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

FLOYD_LANG_PROOF("Floyd test suite", "for", "0 ... 3", ""){
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

FLOYD_LANG_PROOF("Floyd test suite", "for", "1 ... 0", ""){
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

FLOYD_LANG_PROOF("Floyd test suite", "for", "1000 ... 1002", ""){
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


FLOYD_LANG_PROOF("Floyd test suite", "for", "0 ..< 0", ""){
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

FLOYD_LANG_PROOF("Floyd test suite", "for", "0 ..< 1", ""){
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

FLOYD_LANG_PROOF("Floyd test suite", "for", "0 ..< 2", ""){
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

FLOYD_LANG_PROOF("Floyd test suite", "for", "0 ..< 3", ""){
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

FLOYD_LANG_PROOF("Floyd test suite", "for" "recusion test using fibonacci 10", "", ""){
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

FLOYD_LANG_PROOF("Floyd test suite", "for", "return from within FOR block", ""){
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



FLOYD_LANG_PROOF("Floyd test suite", "for", "bool ... bool", "Error"){
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

FLOYD_LANG_PROOF("Floyd test suite", "for", "EXPR ... EXPR", ""){
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



FLOYD_LANG_PROOF("Floyd test suite", "for", "nested for loops", ""){
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


FLOYD_LANG_PROOF("Floyd test suite", "for", "print_board()", ""){
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



//######################################################################################################################
//	WHILE STATEMENT
//######################################################################################################################




FLOYD_LANG_PROOF("Floyd test suite", "while", "global while", ""){
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




FLOYD_LANG_PROOF("Floyd test suite", "while", "while inside function", ""){
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

FLOYD_LANG_PROOF("Floyd test suite", "while", "Error: while with non-bool condition", ""){
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

FLOYD_LANG_PROOF("Floyd test suite", "while", "return from within while", ""){
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







//######################################################################################################################
//	STRING
//######################################################################################################################




FLOYD_LANG_PROOF("Floyd test suite", "string constructor", "", ""){
	ut_verify_global_result_nolib(QUARK_POS, "let result = string(\"ABCD\")", value_t::make_string("ABCD"));
}

FLOYD_LANG_PROOF("Floyd test suite", "string constructor", "", ""){
	ut_verify_global_result_nolib(QUARK_POS, "let result = \"ABCD\"", value_t::make_string("ABCD"));
}

FLOYD_LANG_PROOF("Floyd test suite", "string literal", "Escape \0", ""){
	ut_verify_global_result_nolib(QUARK_POS, R"___(let result = "\0")___", value_t::make_string(std::string(1, '\0')));
}
FLOYD_LANG_PROOF("Floyd test suite", "string literal", "Escape \t", ""){
	ut_verify_global_result_nolib(QUARK_POS, R"___(let result = "\t")___", value_t::make_string("\t"));
}
FLOYD_LANG_PROOF("Floyd test suite", "string literal", "Escape \\", ""){
	ut_verify_global_result_nolib(QUARK_POS, R"___(let result = "\\")___", value_t::make_string("\\"));
}
FLOYD_LANG_PROOF("Floyd test suite", "string literal", "Escape \n", ""){
	ut_verify_global_result_nolib(QUARK_POS, R"___(let result = "\n")___", value_t::make_string("\n"));
}
FLOYD_LANG_PROOF("Floyd test suite", "string literal", "Escape \r", ""){
	ut_verify_global_result_nolib(QUARK_POS, R"___(let result = "\r")___", value_t::make_string("\r"));
}
FLOYD_LANG_PROOF("Floyd test suite", "string literal", "Escape \"", ""){
	ut_verify_global_result_nolib(QUARK_POS, R"___(let result = "\"")___", value_t::make_string("\""));
}
FLOYD_LANG_PROOF("Floyd test suite", "string literal", "Escape \'", ""){
	ut_verify_global_result_nolib(QUARK_POS, R"___(let result = "\'")___", value_t::make_string("\'"));
}
FLOYD_LANG_PROOF("Floyd test suite", "string literal", "Escape \'", ""){
	ut_verify_global_result_nolib(QUARK_POS, R"___(let result = "\/")___", value_t::make_string("/"));
}

FLOYD_LANG_PROOF("Floyd test suite", "string =", "", ""){
	ut_run_closed_nolib(R"(

		let a = "hello"
		let b = a
		assert(b == "hello")

	)");
}

FLOYD_LANG_PROOF("Floyd test suite", "string +", "", ""){
	ut_run_closed_nolib(R"(		assert("a" + "b" == "ab")		)");
}
FLOYD_LANG_PROOF("Floyd test suite", "string +", "", ""){
	ut_run_closed_nolib(R"(		assert("a" + "b" + "c" == "abc")		)");
}



FLOYD_LANG_PROOF("Floyd test suite", "string ==", "", ""){
	ut_run_closed_nolib(R"(		assert(("hello" == "hello") == true)		)");
}
FLOYD_LANG_PROOF("Floyd test suite", "string ==", "", ""){
	ut_run_closed_nolib(R"(		assert(("hello" == "Yello") == false)		)");
}
FLOYD_LANG_PROOF("Floyd test suite", "string !=", "", ""){
	ut_run_closed_nolib(R"(		assert(("hello" != "yello") == true)		)");
}
FLOYD_LANG_PROOF("Floyd test suite", "string !=", "", ""){
	ut_run_closed_nolib(R"(		assert(("hello" != "hello") == false)		)");
}


FLOYD_LANG_PROOF("Floyd test suite", "string <", "", ""){
	ut_run_closed_nolib(R"(		assert(("aaa" < "bbb") == true)		)");
}
FLOYD_LANG_PROOF("Floyd test suite", "string <", "", ""){
	ut_run_closed_nolib(R"(		assert(("aaa" < "aaa") == false)		)");
}
FLOYD_LANG_PROOF("Floyd test suite", "string <", "", ""){
	ut_run_closed_nolib(R"(		assert(("bbb" < "aaa") == false)		)");
}


FLOYD_LANG_PROOF("Floyd test suite", "string <=", "", ""){
	ut_run_closed_nolib(R"(		assert(("aaa" <= "bbb") == true)		)");
}
FLOYD_LANG_PROOF("Floyd test suite", "string <=", "", ""){
	ut_run_closed_nolib(R"(		assert(("aaa" <= "aaa") == true)		)");
}
FLOYD_LANG_PROOF("Floyd test suite", "string <=", "", ""){
	ut_run_closed_nolib(R"(		assert(("bbb" <= "aaa") == false)		)");
}


FLOYD_LANG_PROOF("Floyd test suite", "string >", "", ""){
	ut_run_closed_nolib(R"(		assert(("bbb" > "aaa") == true)		)");
}
FLOYD_LANG_PROOF("Floyd test suite", "string >", "", ""){
	ut_run_closed_nolib(R"(		assert(("aaa" > "aaa") == false)		)");
}
FLOYD_LANG_PROOF("Floyd test suite", "string >", "", ""){
	ut_run_closed_nolib(R"(		assert(("aaa" > "bbb") == false)		)");
}


FLOYD_LANG_PROOF("Floyd test suite", "string >=", "", ""){
	ut_run_closed_nolib(R"(		assert(("bbb" >= "aaa") == true)		)");
}
FLOYD_LANG_PROOF("Floyd test suite", "string >=", "", ""){
	ut_run_closed_nolib(R"(		assert(("aaa" >= "aaa") == true)		)");
}
FLOYD_LANG_PROOF("Floyd test suite", "string >=", "", ""){
	ut_run_closed_nolib(R"(		assert(("aaa" >= "bbb") == false)		)");
}


FLOYD_LANG_PROOF("Floyd test suite", "string []", "", ""){
	ut_verify_printout_nolib(
		QUARK_POS,
		R"(

			let message = "helloxyz12345678"
			print(message[0])
			print(message[1])
			print(message[2])
			print(message[3])
			print(message[4])
			print(message[5])
			print(message[6])
			print(message[7])

			print(message[8])
			print(message[9])
			print(message[10])
			print(message[11])
			print(message[12])
			print(message[13])
			print(message[14])
			print(message[15])

		)",
		{
			"104",
			"101",
			"108",
			"108",
			"111",
			"120",
			"121",
			"122",

			"49",
			"50",
			"51",
			"52",
			"53",
			"54",
			"55",
			"56"
		}
	);
}

FLOYD_LANG_PROOF("Floyd test suite", "string []", "", ""){
	ut_run_closed_nolib(R"(		assert("hello"[0] == 104)		)");
}

FLOYD_LANG_PROOF("Floyd test suite", "string []", "", ""){
	ut_run_closed_nolib(R"(		assert("hello"[4] == 111)		)");
}

FLOYD_LANG_PROOF("Floyd test suite", "string", "Error: Lookup in string using non-int", "exception"){
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

FLOYD_LANG_PROOF("Floyd test suite", "string update()", "", ""){
	ut_run_closed_nolib(R"(

		let a = update("hello", 1, 98)
		assert(a == "hbllo")

	)");
}
FLOYD_LANG_PROOF("Floyd test suite", "string update()", "error: pos > len", "exception"){
	ut_verify_exception_nolib(
		QUARK_POS,
		R"(

			let a = update("hello", 5, 98)

		)",
		"Position argument to update() is outside collection span."
	);
}



FLOYD_LANG_PROOF("Floyd test suite", "string size()", "", ""){
	ut_run_closed_nolib(R"(		assert(size("") == 0)		)");
}
FLOYD_LANG_PROOF("Floyd test suite", "string size()", "", ""){
	ut_run_closed_nolib(R"(		assert(size("How long is this string?") == 24)		)");
}

FLOYD_LANG_PROOF("Floyd test suite", "string size()", "Embeded null characters - check 8 bit clean", ""){
	ut_run_closed_nolib(R"(		assert(size("hello\0world\0\0") == 13)		)");
}



//??? find() should have a start index.
FLOYD_LANG_PROOF("Floyd test suite", "string find()", "", ""){
	ut_run_closed_nolib(R"(		assert(find("hello, world", "he") == 0)		)");
}
FLOYD_LANG_PROOF("Floyd test suite", "string find()", "", ""){
	ut_run_closed_nolib(R"(		assert(find("hello, world", "e") == 1)		)");
}
FLOYD_LANG_PROOF("Floyd test suite", "string find()", "", ""){
	ut_run_closed_nolib(R"(		assert(find("hello, world", "x") == -1)		)");
}


//??? Add character-literal / type.
FLOYD_LANG_PROOF("Floyd test suite", "string push_back()", "", ""){
	ut_run_closed_nolib(R"(

		let a = push_back("one", 111)
		assert(a == "oneo")

	)");
}



FLOYD_LANG_PROOF("Floyd test suite", "string subset()", "string", ""){
	ut_run_closed_nolib(R"(		assert(subset("abc", 0, 3) == "abc")		)");
}
FLOYD_LANG_PROOF("Floyd test suite", "string subset()", "string", ""){
	ut_run_closed_nolib(R"(		assert(subset("abc", 1, 3) == "bc")		)");
}
FLOYD_LANG_PROOF("Floyd test suite", "string subset()", "string", ""){
	ut_run_closed_nolib(R"(		assert(subset("abc", 0, 0) == "")		)");
}
FLOYD_LANG_PROOF("Floyd test suite", "string subset()", "string", ""){
	ut_run_closed_nolib(R"(		assert(subset("abc", 0, 10) == "abc")		)");
}
FLOYD_LANG_PROOF("Floyd test suite", "string subset()", "string", ""){
	ut_run_closed_nolib(R"(		assert(subset("abc", 2, 10) == "c")		)");
}




FLOYD_LANG_PROOF("Floyd test suite", "string replace()", "", ""){
	ut_run_closed_nolib(R"(		assert(replace("One ring to rule them all", 4, 8, "rabbit") == "One rabbit to rule them all")		)");
}
FLOYD_LANG_PROOF("Floyd test suite", "string replace()", "", ""){
	ut_run_closed_nolib(R"(		assert(replace("hello", 0, 5, "goodbye") == "goodbye")		)");
}
FLOYD_LANG_PROOF("Floyd test suite", "string replace()", "", ""){
	ut_run_closed_nolib(R"(		assert(replace("hello", 0, 6, "goodbye") == "goodbye")		)");
}
FLOYD_LANG_PROOF("Floyd test suite", "string replace()", "", ""){
	ut_run_closed_nolib(R"(		assert(replace("hello", 0, 0, "goodbye") == "goodbyehello")		)");
}
FLOYD_LANG_PROOF("Floyd test suite", "string replace()", "", ""){
	ut_run_closed_nolib(R"(		assert(replace("hello", 5, 5, "goodbye") == "hellogoodbye")		)");
}
FLOYD_LANG_PROOF("Floyd test suite", "string replace()", "", "error"){
	ut_verify_exception_nolib(QUARK_POS, R"(		assert(replace("hello", 5, 0, "goodbye") == "hellogoodbye")		)", "replace() requires start <= end.");
}
FLOYD_LANG_PROOF("Floyd test suite", "string replace()", "", "error"){
	ut_verify_exception_nolib(QUARK_POS, R"(		assert(replace("hello", 2, 3, 666) == "")		)", "replace() requires argument 4 to be same type of collection.");
}









//######################################################################################################################
//	VECTOR
//######################################################################################################################
//### test pos limiting and edge cases.
//### test all bounds errors
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


FLOYD_LANG_PROOF("Floyd test suite", "vector [] - empty constructor", "cannot be infered", "error"){
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


FLOYD_LANG_PROOF("Floyd test suite", "vector [string] - constructor", "Infer type", "valid vector"){
	ut_verify_printout_nolib(
		QUARK_POS,
		R"(

			let [string] a = ["one", "two"]
			print(a)

		)",
		{ R"(["one", "two"])" }
	);
}

FLOYD_LANG_PROOF("Floyd test suite", "vector [string] - constructor", "Infer type", "valid vector"){
	ut_verify_printout_nolib(
		QUARK_POS,
		R"(

			let a = ["one", "two"]
			print(a)

		)",
		{ R"(["one", "two"])" }
	);
}

FLOYD_LANG_PROOF("Floyd test suite", "vector [string] - constructor", "empty vector", "valid vector"){
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
FLOYD_LANG_PROOF("Floyd test suite", "vector ==", "lhs and rhs are empty-typeless", ""){
	ut_verify_exception_nolib(QUARK_POS, R"(		assert(([] == []) == true)		)", "Cannot infer vector element type, add explicit type. Line: 1 \"assert(([] == []) == true)\"");
}

FLOYD_LANG_PROOF("Floyd test suite", "vector [string] =", "copy", ""){
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

FLOYD_LANG_PROOF("Floyd test suite", "vector +", "add empty vectors", ""){
	ut_verify_exception_nolib(QUARK_POS, R"(		let [int] a = [] + [] result = a == []		)", R"(Cannot infer vector element type, add explicit type. Line: 1 "let [int] a = [] + [] result = a == []")");
}

#if 0
//??? This fails but should not. This code becomes a constructor call to [int] with more than 16 arguments. Byte code interpreter has 16 argument limit.
FLOYD_LANG_PROOF("Floyd test suite", "vector [] - constructor", "32 elements initialization", ""){
	ut_run_closed_nolib(R"(

		let a = [ 0,0,1,1,0,0,0,0,	0,0,1,1,0,0,0,0,	0,0,1,1,1,0,0,0,	0,0,1,1,1,1,0,0 ]
		print(a)

	)");
}
#endif

FLOYD_LANG_PROOF("Floyd test suite", "vector [string] constructor expression, computed element", "", ""){
	ut_verify_global_result_as_json_nolib(
		QUARK_POS,
		R"(

			func string get_beta(){ return "beta" }
			let [string] result = ["alpha", get_beta()]

		)",
		R"(		[[ "vector", "^string" ], ["alpha","beta"]]		)"
	);
}




FLOYD_LANG_PROOF("Floyd test suite", "vector [string] =", "", ""){
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


FLOYD_LANG_PROOF("Floyd test suite", "vector [string] +", "non-empty vectors", ""){
	ut_run_closed_nolib(R"(

		let [string] a = ["one"] + ["two"]
		assert(a == ["one", "two"])

	)");
}


FLOYD_LANG_PROOF("Floyd test suite", "vector [string] ==", "same values", ""){
	ut_run_closed_nolib(R"(		assert((["one", "two"] == ["one", "two"]) == true)		)");
}

FLOYD_LANG_PROOF("Floyd test suite", "vector [string] ==", "different values", ""){
	ut_run_closed_nolib(R"(		assert((["one", "three"] == ["one", "two"]) == false)		)");
}

FLOYD_LANG_PROOF("Floyd test suite", "vector [string] <", "same values", ""){
	ut_run_closed_nolib(R"(		assert((["one", "two"] < ["one", "two"]) == false)		)");
}

FLOYD_LANG_PROOF("Floyd test suite", "vector [string] <", "different values", ""){
	ut_run_closed_nolib(R"(		assert((["one", "a"] < ["one", "two"]) == true)		)");
}



FLOYD_LANG_PROOF("Floyd test suite", "vector [string] [] lookup", "", ""){
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



FLOYD_LANG_PROOF("Floyd test suite", "vector [string] print()", "empty", "[]"){
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
FLOYD_LANG_PROOF("Floyd test suite", "vector [string] print()", "2 elements", ""){
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


FLOYD_LANG_PROOF("Floyd test suite", "vector [string] update()", "", "valid vector, without side effect on original vector"){
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

FLOYD_LANG_PROOF("Floyd test suite", "vector [string] size()", "empty", "0"){
	ut_run_closed_nolib(R"(

		let [string] a = []
		assert(size(a) == 0)

	)");
}
FLOYD_LANG_PROOF("Floyd test suite", "vector [string] size()", "2", ""){
	ut_run_closed_nolib(R"(

		let [string] a = ["one", "two"]
		assert(size(a) == 2)

	)");
}



FLOYD_LANG_PROOF("Floyd test suite", "vector [string] find()", "", ""){
	ut_run_closed_nolib(R"(		assert(find(["one", "two", "three", "four"], "one") == 0)		)");
}
FLOYD_LANG_PROOF("Floyd test suite", "vector [string] find()", "", ""){
	ut_run_closed_nolib(R"(		assert(find(["one", "two", "three", "four"], "two") == 1)		)");
}
FLOYD_LANG_PROOF("Floyd test suite", "vector [string] find()", "", ""){
	ut_run_closed_nolib(R"(		assert(find(["one", "two", "three", "four"], "five") == -1)		)");
}



FLOYD_LANG_PROOF("Floyd test suite", "vector [string] push_back()", "", ""){
	ut_run_closed_nolib(R"(

		let [string] a = ["one"];
		let [string] b = push_back(a, "two")
		let [string] c = ["one", "two"];
		assert(b == c)

	)");
}

FLOYD_LANG_PROOF("Floyd test suite", "vector [string] push_back()", "", ""){
	ut_run_closed_nolib(R"(

		let [string] a = push_back(["one"], "two")
		assert(a == ["one", "two"])

	)");
}

FLOYD_LANG_PROOF("Floyd test suite", "vector [string] subset()", "", ""){
	ut_run_closed_nolib(R"(		assert(subset(["one", "two", "three"], 0, 3) == ["one", "two", "three"])		)");
}
FLOYD_LANG_PROOF("Floyd test suite", "vector [string] subset()", "", ""){
	ut_run_closed_nolib(R"(		assert(subset(["one", "two", "three"], 1, 3) == ["two", "three"])		)");
}
FLOYD_LANG_PROOF("Floyd test suite", "vector [string] subset()", "", ""){
	ut_run_closed_nolib(R"(		assert(subset(["one", "two", "three"], 0, 0) == [])		)");
}
FLOYD_LANG_PROOF("Floyd test suite", "vector [string] subset()", "", ""){
	ut_run_closed_nolib(R"(		assert(subset(["one", "two", "three"], 0, 10) == ["one", "two", "three"])		)");
}
FLOYD_LANG_PROOF("Floyd test suite", "vector [string] subset()", "", ""){
	ut_run_closed_nolib(R"(		assert(subset(["one", "two", "three"], 2, 10) == ["three"])		)");
}




FLOYD_LANG_PROOF("Floyd test suite", "vector [string] replace()", "", ""){
	ut_run_closed_nolib(R"(		assert(replace(["one", "two", "three", "four", "five"], 0, 5, ["goodbye"]) == ["goodbye"])		)");
}
FLOYD_LANG_PROOF("Floyd test suite", "vector [string] replace()", "", ""){
	ut_run_closed_nolib(R"(		assert(replace(["one", "two", "three", "four", "five"], 0, 6, ["goodbye"]) == ["goodbye"])		)");
}
FLOYD_LANG_PROOF("Floyd test suite", "vector [string] replace()", "", ""){
	ut_run_closed_nolib(R"(		assert(replace(["one", "two", "three", "four", "five"], 0, 0, ["goodbye"]) == ["goodbye", "one", "two", "three", "four", "five"])		)");
}
FLOYD_LANG_PROOF("Floyd test suite", "vector [string] replace()", "", ""){
	ut_run_closed_nolib(R"(		assert(replace(["one", "two", "three", "four", "five"], 5, 5, ["goodbye"]) == ["one", "two", "three", "four", "five", "goodbye"])		)");
}
FLOYD_LANG_PROOF("Floyd test suite", "vector [string] replace()", "", "error"){
	ut_verify_exception_nolib(QUARK_POS, R"(		assert(replace(["one", "two", "three", "four", "five"], 5, 0, ["goodbye"]) == ["hellogoodbye"])		)", "replace() requires start <= end.");
}
FLOYD_LANG_PROOF("Floyd test suite", "vector [string] replace()", "", "error"){
	ut_verify_exception_nolib(QUARK_POS, R"(		assert(replace(["one", "two", "three", "four", "five"], 2, 3, 666) == [])		)", "replace() requires argument 4 to be same type of collection.");
}





//////////////////////////////////////////		vector-bool


FLOYD_LANG_PROOF("Floyd test suite", "vector [bool] construct-expression", "", ""){
	ut_verify_global_result_as_json_nolib(QUARK_POS, R"(		let [bool] result = [true, false, true]		)",				R"(		[[ "vector", "^bool" ], [true, false, true]]		)");
}
FLOYD_LANG_PROOF("Floyd test suite", "vector [bool] =", "copy", ""){
	ut_verify_global_result_as_json_nolib(QUARK_POS, R"(		let a = [true, false, true] let result = a		)",				R"(		[[ "vector", "^bool" ], [true, false, true]]		)");
}
FLOYD_LANG_PROOF("Floyd test suite", "vector [bool] ==", "same values", ""){
	ut_verify_global_result_as_json_nolib(QUARK_POS, R"(		let result = [true, false] == [true, false]		)",			R"(		[ "^bool", true]		)" );
}
FLOYD_LANG_PROOF("Floyd test suite", "vector [bool] ==", "different values", ""){
	ut_verify_global_result_as_json_nolib(QUARK_POS, R"(		let result = [false, false] == [true, false]		)", 	R"(		[ "^bool", false]		)" );
}
FLOYD_LANG_PROOF("Floyd test suite", "vector [bool] <", "", ""){
	ut_verify_global_result_as_json_nolib(QUARK_POS, R"(		let result = [true, true] < [true, true]		)",			R"(		[ "^bool", false]	)");
}
FLOYD_LANG_PROOF("Floyd test suite", "vector [bool] <", "different values", ""){
	ut_verify_global_result_as_json_nolib(QUARK_POS, R"(		let result = [true, false] < [true, true]		)", 		R"(		[ "^bool", true]		)");
}
FLOYD_LANG_PROOF("Floyd test suite", "vector [bool] +", "non-empty vectors", ""){
	ut_verify_global_result_as_json_nolib(QUARK_POS, R"(		let [bool] result = [true, false] + [true, true]		)", R"(		[[ "vector", "^bool" ], [true, false, true, true]]		)");
}



FLOYD_LANG_PROOF("Floyd test suite", "vector [bool] size()", "empty", "0"){
	ut_verify_global_result_as_json_nolib(QUARK_POS, R"(		let [bool] a = [] let result = size(a)		)",					R"(		[ "^int", 0]		)");
}

FLOYD_LANG_PROOF("Floyd test suite", "vector [bool] size()", "2", ""){
	ut_verify_global_result_as_json_nolib(QUARK_POS, R"(		let [bool] a = [true, false, true] let result = size(a)		)", R"(		[ "^int", 3]		)");
}



FLOYD_LANG_PROOF("Floyd test suite", "vector [bool] push_back()", "", ""){
	ut_verify_global_result_as_json_nolib(QUARK_POS, R"(		let [bool] result = push_back([true, false], true)		)", R"(		[[ "vector", "^bool" ], [true, false, true]]		)");
}





//////////////////////////////////////////		vector-int


FLOYD_LANG_PROOF("Floyd test suite", "vector [int] constructor expression", "", ""){
	ut_verify_global_result_as_json_nolib(QUARK_POS, R"(		let [int] result = [10, 20, 30]		)",		R"(		[[ "vector", "^int" ], [10, 20, 30]]		)" );
}
FLOYD_LANG_PROOF("Floyd test suite", "vector [int] constructor", "", "3"){
	ut_verify_global_result_nolib(
		QUARK_POS,
		R"(

			let [int] a = [1, 2, 3]
			let result = size(a)

		)",
		value_t::make_int(3)
	);
}
FLOYD_LANG_PROOF("Floyd test suite", "vector [string] [] lookup", "", ""){
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

FLOYD_LANG_PROOF("Floyd test suite", "vector [int] =", "copy", ""){
	ut_verify_global_result_as_json_nolib(QUARK_POS, R"(		let a = [10, 20, 30] let result = a;		)",	R"(		[[ "vector", "^int" ], [10, 20, 30]]		)"	);
}
FLOYD_LANG_PROOF("Floyd test suite", "vector [int] ==", "same values", ""){
	ut_verify_global_result_as_json_nolib(QUARK_POS, R"(		let result = [1, 2] == [1, 2]		)",		R"(		[ "^bool", true]		)");
	ut_verify_global_result_as_json_nolib(QUARK_POS, R"(		let result = [1, 2] == [1, 2]		)",		R"(		[ "^bool", true]		)");
}
FLOYD_LANG_PROOF("Floyd test suite", "vector [int] ==", "different values", ""){
	ut_verify_global_result_as_json_nolib(QUARK_POS, R"(		let result = [1, 3] == [1, 2]		)",		R"(		[ "^bool", false]		)");
}
FLOYD_LANG_PROOF("Floyd test suite", "vector [int] <", "", ""){
	ut_verify_global_result_as_json_nolib(QUARK_POS, R"(		let result = [1, 2] < [1, 2]		)",		R"(		[ "^bool", false]	)");
}
FLOYD_LANG_PROOF("Floyd test suite", "vector [int] <", "different values", ""){
	ut_verify_global_result_as_json_nolib(QUARK_POS, R"(		let result = [1, 2] < [1, 3]		)",		R"(		[ "^bool", true]		)");
}

FLOYD_LANG_PROOF("Floyd test suite", "vector [int] +", "non-empty vectors", ""){
	ut_verify_global_result_as_json_nolib(QUARK_POS, R"(		let [int] result = [1, 2] + [3, 4]		)",	R"(		[[ "vector", "^int" ], [1, 2, 3, 4]]		)");
}




FLOYD_LANG_PROOF("Floyd test suite", "vector [int] size()", "empty", "0"){
	ut_verify_global_result_as_json_nolib(QUARK_POS, R"(		let [int] a = [] let result = size(a)		)",	R"(		[ "^int", 0]		)");
}
FLOYD_LANG_PROOF("Floyd test suite", "vector [int] size()", "2", ""){
	ut_verify_global_result_as_json_nolib(QUARK_POS, R"(		let [int] a = [1, 2, 3] let result = size(a)		)",		R"(		[ "^int", 3]		)");
}


FLOYD_LANG_PROOF("Floyd test suite", "vector [int] find()", "", ""){
	ut_run_closed_nolib(R"(		assert(find([1,2,3], 4) == -1)		)");
}
FLOYD_LANG_PROOF("Floyd test suite", "vector [int] find()", "", ""){
	ut_run_closed_nolib(R"(		assert(find([1,2,3], 1) == 0)		)");
}
FLOYD_LANG_PROOF("Floyd test suite", "vector [int] find()", "", ""){
	ut_run_closed_nolib(R"(		assert(find([1,2,2,2,3], 2) == 1)		)");
}

FLOYD_LANG_PROOF("Floyd test suite", "vector [int] push_back()", "", ""){
	ut_verify_global_result_as_json_nolib(QUARK_POS, R"(		let [int] result = push_back([1, 2], 3)		)",		R"(		[[ "vector", "^int" ], [1, 2, 3]]		)");
}

FLOYD_LANG_PROOF("Floyd test suite", "vector [int] push_back()", "", ""){
	ut_run_closed_nolib(
		R"(

			let result = push_back([1, 2], 3)
			assert(result == [1, 2, 3])

		)"
	);
}

FLOYD_LANG_PROOF("Floyd test suite", "vector [int] subset()", "", ""){
	ut_run_closed_nolib(R"(		assert(subset([10,20,30], 0, 3) == [10,20,30])		)");
}
FLOYD_LANG_PROOF("Floyd test suite", "vector [int] subset()", "", ""){
	ut_run_closed_nolib(R"(		assert(subset([10,20,30], 1, 3) == [20,30])		)");
}
FLOYD_LANG_PROOF("Floyd test suite", "vector [int] subset()", "", ""){
	ut_run_closed_nolib(R"(		let result = (subset([10,20,30], 0, 0) == [])		)");
}
FLOYD_LANG_PROOF("Floyd test suite", "vector [int] subset()", "", ""){
	ut_run_closed_nolib(R"(		assert(subset([10,20,30], 0, 0) == [])		)");
}

FLOYD_LANG_PROOF("Floyd test suite", "vector [int] replace()", "", ""){
	ut_run_closed_nolib(R"(		assert(replace([ 1, 2, 3, 4, 5, 6 ], 2, 5, [20, 30]) == [1, 2, 20, 30, 6])		)");
}

FLOYD_LANG_PROOF("Floyd test suite", "vector [int] replace()", "", ""){
	ut_run_closed_nolib(
		R"(

			let h = replace([ 1, 2, 3, 4, 5 ], 1, 4, [ 8, 9 ])
			assert(h == [ 1, 8, 9, 5 ])

		)"
	);
}






//////////////////////////////////////////		vector-double



FLOYD_LANG_PROOF("Floyd test suite", "vector [double] constructor-expression", "", ""){
	ut_verify_global_result_as_json_nolib(QUARK_POS, R"(		let [double] result = [10.5, 20.5, 30.5]		)",	R"(		[[ "vector", "^double" ], [10.5, 20.5, 30.5]]		)");
}
FLOYD_LANG_PROOF("Floyd test suite", "vector", "Vector can not hold elements of different types.", "exception"){
	ut_verify_exception_nolib(QUARK_POS, R"(		let a = [3, bool]		)", "Vector of type [int] cannot hold an element of type typeid. Line: 1 \"let a = [3, bool]\"");
}


FLOYD_LANG_PROOF("Floyd test suite", "vector []", "Error: Lookup in vector using non-int", "exception"){
	ut_verify_exception_nolib(
		QUARK_POS,
		R"(

			let [string] a = ["one", "two", "three"]
			print(a["not an integer"])

		)",
		"Vector can only be indexed by integers, not a \"string\". Line: 4 \"print(a[\"not an integer\"])\""
	);
}
FLOYD_LANG_PROOF("Floyd test suite", "vector", "Error: Lookup the unlookupable", "exception"){
	ut_verify_exception_nolib(QUARK_POS, R"(		let a = 3[0]		)", "Lookup using [] only works with strings, vectors, dicts and json - not a \"int\". Line: 1 \"let a = 3[0]\"");
}

FLOYD_LANG_PROOF("Floyd test suite", "vector [double] =", "copy", ""){
	ut_verify_global_result_as_json_nolib(QUARK_POS, R"(		let a = [10.5, 20.5, 30.5] let result = a		)",	R"(		[[ "vector", "^double" ], [10.5, 20.5, 30.5]]		)");
}
FLOYD_LANG_PROOF("Floyd test suite", "vector [double] ==", "same values", ""){
	ut_verify_global_result_as_json_nolib(QUARK_POS, R"(		let result = [1.5, 2.5] == [1.5, 2.5]		)",	R"(		[ "^bool", true]		)");
}
FLOYD_LANG_PROOF("Floyd test suite", "vector [double] ==", "different values", ""){
	ut_verify_global_result_as_json_nolib(QUARK_POS, R"(		let result = [1.5, 3.5] == [1.5, 2.5]		)",	R"(		[ "^bool", false]		)");
}
FLOYD_LANG_PROOF("Floyd test suite", "vector [double] <", "", ""){
	ut_verify_global_result_as_json_nolib(QUARK_POS, R"(		let result = [1.5, 2.5] < [1.5, 2.5]		)",	R"(		[ "^bool", false]	)");
}
FLOYD_LANG_PROOF("Floyd test suite", "vector [double] <", "different values", ""){
	ut_verify_global_result_as_json_nolib(QUARK_POS, R"(		let result = [1.5, 2.5] < [1.5, 3.5]		)",	R"(		[ "^bool", true]		)");
}

FLOYD_LANG_PROOF("Floyd test suite", "vector [double] +", "non-empty vectors", ""){
	ut_verify_global_result_as_json_nolib(QUARK_POS, R"(		let [double] result = [1.5, 2.5] + [3.5, 4.5]		)",		R"(		[[ "vector", "^double" ], [1.5, 2.5, 3.5, 4.5]]		)");
}




FLOYD_LANG_PROOF("Floyd test suite", "vector [double] size()", "empty", "0"){
	ut_verify_global_result_as_json_nolib(QUARK_POS, R"(		let [double] a = [] let result = size(a)		)",	R"(		[ "^int", 0]		)");
}
FLOYD_LANG_PROOF("Floyd test suite", "vector [double] size()", "2", ""){
	ut_verify_global_result_as_json_nolib(QUARK_POS, R"(		let [double] a = [1.5, 2.5, 3.5] let result = size(a)		)",	R"(		[ "^int", 3]		)");
}



FLOYD_LANG_PROOF("Floyd test suite", "vector [double] push_back()", "", ""){
	ut_verify_global_result_as_json_nolib(QUARK_POS, R"(		let [double] result = push_back([1.5, 2.5], 3.5)		)",	R"(		[[ "vector", "^double" ], [1.5, 2.5, 3.5]]		)");
}







//////////////////////////////////////////		TEST VECTOR CONSTRUCTOR FOR ALL TYPES




FLOYD_LANG_PROOF("Floyd test suite", "vector [bool] constructor", "", ""){
	ut_run_closed_nolib(R"(		assert(to_string([true, false, true]) == "[true, false, true]")		)");
}
FLOYD_LANG_PROOF("Floyd test suite", "vector [int] constructor", "", ""){
	ut_run_closed_nolib(R"(		assert(to_string([1, 2, 3]) == "[1, 2, 3]")		)");
}
FLOYD_LANG_PROOF("Floyd test suite", "vector [double] constructor", "", ""){
	ut_run_closed_nolib(R"(		assert(to_string([1.0, 2.0, 3.0]) == "[1.0, 2.0, 3.0]")		)");
}
FLOYD_LANG_PROOF("Floyd test suite", "vector [string] constructor", "", ""){
	ut_run_closed_nolib(R"(		assert(to_string(["one", "two", "three"]) == "[\"one\", \"two\", \"three\"]")		)");
}

FLOYD_LANG_PROOF("Floyd test suite", "vector [typeid] constructor", "", ""){
	ut_run_closed_nolib(R"(		assert(to_string([int, bool, string]) == "[int, bool, string]")		)");
}

FLOYD_LANG_PROOF("Floyd test suite", "vector [typeid] constructor", "", ""){
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

FLOYD_LANG_PROOF("Floyd test suite", "vector [func] constructor", "", ""){
	ut_verify_printout_nolib(
		QUARK_POS,
		R"(

			func int a(string s){
				return 2
			}
			let d = [a]
			print(d)

		)",
		{
			"[function int(string) pure]"
		 }
	);
}

FLOYD_LANG_PROOF("Floyd test suite", "vector [func] constructor", "", ""){
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
			"[function int(string) pure, function int(string) pure, function int(string) pure]"
		 }
	);
}


FLOYD_LANG_PROOF("Floyd test suite", "vector [struct] constructor", "", ""){
	ut_verify_printout_nolib(
		QUARK_POS,
		R"(

			struct person_t { string name int birth_year }

			let d = [person_t("Mozart", 1782), person_t("Bono", 1955)]
			print(d)

		)",
		{
			R"___([{name="Mozart", birth_year=1782}, {name="Bono", birth_year=1955}])___"
		 }
	);
}

FLOYD_LANG_PROOF("Floyd test suite", "vector [vector[string]] constructor", "", ""){
	ut_verify_printout_nolib(
		QUARK_POS,
		R"(

			let d = [["red", "blue"], ["one", "two", "three"]]
			print(d)

		)",
		{
			R"___([["red", "blue"], ["one", "two", "three"]])___"
		 }
	);
}

FLOYD_LANG_PROOF("Floyd test suite", "vector [dict[string]] constructor", "", ""){
	ut_verify_printout_nolib(
		QUARK_POS,
		R"(

			let d = [{"color": "red", "color2": "blue"}, {"num": "one", "num2": "two", "num3": "three"}]
			print(d)

		)",
		{
			R"___([{"color": "red", "color2": "blue"}, {"num": "one", "num2": "two", "num3": "three"}])___"
		 }
	);
}
FLOYD_LANG_PROOF("Floyd test suite", "vector [dict[string]] constructor", "Mixing dict types in a vector", "Compilation error"){
	ut_verify_exception_nolib(
		QUARK_POS,
		R"(

			let d = [{"color": "red", "color2": "blue"}, {"num": 100, "num2": 200, "num3": 300}]
			print(d)

		)",
		R"___(Vector of type [[string:string]] cannot hold an element of type [string:int]. Line: 3 "let d = [{"color": "red", "color2": "blue"}, {"num": 100, "num2": 200, "num3": 300}]")___"
	);
}

FLOYD_LANG_PROOF("Floyd test suite", "vector [json] constructor", "", ""){
	ut_verify_printout_nolib(
		QUARK_POS,
		R"(

			let d = [json("red"), json("blue")]
			print(d)

		)",
		{
			R"___(["red", "blue"])___"
		 }
	);
}




//######################################################################################################################
//	DICT
//######################################################################################################################



FLOYD_LANG_PROOF("Floyd test suite", "dict constructor", "No type", "error"){
	ut_verify_exception_nolib(
		QUARK_POS,
		R"(

			let a = {}
			print(a)

		)",
		"Cannot infer type in construct-value-expression. Line: 3 \"let a = {}\""
	);
}

FLOYD_LANG_PROOF("Floyd test suite", "dict constructor", "", "Error cannot infer type"){
	ut_verify_exception_nolib(QUARK_POS, R"(		assert(size({}) == 0)		)", "Cannot infer type in construct-value-expression. Line: 1 \"assert(size({}) == 0)\"");
}

FLOYD_LANG_PROOF("Floyd test suite", "dict constructor", "", "Error cannot infer type"){
	ut_verify_exception_nolib(QUARK_POS, R"(		print({})		)", "Cannot infer type in construct-value-expression. Line: 1 \"print({})\"");
}

FLOYD_LANG_PROOF("Floyd test suite", "dict [int] constructor", "", "Compilation error"){
	ut_verify_exception_nolib(QUARK_POS,
		R"(

			mutable a = {}
			a = { "hello": 1 }
			print(a)

		)",
		"Cannot infer type in construct-value-expression. Line: 3 \"mutable a = {}\""
	);
}

FLOYD_LANG_PROOF("Floyd test suite", "dict [] constructor", "{}", ""){
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

FLOYD_LANG_PROOF("Floyd test suite", "dict [int]", "Error: Lookup in dict using non-string key", "exception"){
	ut_verify_exception_nolib(
		QUARK_POS,
		R"(

			let a = { "one": 1, "two": 2 }
			print(a[3])

		)",
		"Dictionary can only be looked up using string keys, not a \"int\". Line: 4 \"print(a[3])\""
	);
}

FLOYD_LANG_PROOF("Floyd test suite", "dict [int]", "Dict can not hold elements of different types.", "exception"){
	ut_verify_exception_nolib(
		QUARK_POS,
		R"(

			let a = {"one": 1, "two": bool}

		)",
		"Dictionary of type [string:int] cannot hold an element of type typeid. Line: 3 \"let a = {\"one\": 1, \"two\": bool}\""
	);
}

FLOYD_LANG_PROOF("Floyd test suite", "dict [int] update()", "dest is empty dict", "error"){
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






//////////////////////////////////////////		DICT [INT]



FLOYD_LANG_PROOF("Floyd test suite", "dict [int] construct", "", ""){
	ut_verify_printout_nolib(
		QUARK_POS,
		R"(

			let [string: int] a = { "one": 1, "two": 2 }
			print(a)

		)",
		{ R"___({"one": 1, "two": 2})___" }
	);
}
FLOYD_LANG_PROOF("Floyd test suite", "dict [int] construct", "", ""){
	ut_verify_printout_nolib(
		QUARK_POS,
		R"(

			print( { "one": 1, "two": 2 } )

		)",
		{ R"___({"one": 1, "two": 2})___" }
	);
}

FLOYD_LANG_PROOF("Floyd test suite", "dict [int] constructor", "{}", "Infered type"){
	ut_verify_printout_nolib(
		QUARK_POS,
		R"(

			print( {"one": 1, "two": 2} )

		)",
		{ R"({"one": 1, "two": 2})" }
	);
}




FLOYD_LANG_PROOF("Floyd test suite", "dict [int] []", "", ""){
	ut_verify_printout_nolib(
		QUARK_POS,
		R"(

			let [string: int] a = { "one": 1, "two": 2 }
			print(a["one"])
			print(a["two"])

		)",
		{ "1", "2" }
	);
}

FLOYD_LANG_PROOF("Floyd test suite", "dict [int] ==", "", ""){
	ut_run_closed_nolib(R"(		assert(({"one": 1, "two": 2} == {"one": 1, "two": 2}) == true)		)");
}
FLOYD_LANG_PROOF("Floyd test suite", "dict [int] ==", "", ""){
	ut_run_closed_nolib(R"(		assert(({"one": 1, "two": 2} == {"two": 2}) == false)		)");
}
FLOYD_LANG_PROOF("Floyd test suite", "dict [int] ==", "", ""){
	ut_run_closed_nolib(R"(		assert(({"one": 2, "two": 2} == {"one": 1, "two": 2}) == false)		)");
}
FLOYD_LANG_PROOF("Floyd test suite", "dict [int] ==", "", ""){
	ut_run_closed_nolib(R"(		assert(({"one": 1, "two": 2} < {"one": 1, "two": 2}) == false)		)");
}
FLOYD_LANG_PROOF("Floyd test suite", "dict [int] <", "", ""){
	ut_run_closed_nolib(R"(		assert(({"one": 1, "two": 1} < {"one": 1, "two": 2}) == true)		)");
}

FLOYD_LANG_PROOF("Floyd test suite", "dict [int] size()", "", "1"){
	ut_run_closed_nolib(R"(		assert(size({"one":1}) == 1)		)");
}
FLOYD_LANG_PROOF("Floyd test suite", "dict [int] size()", "", "2"){
	ut_run_closed_nolib(R"(		assert(size({"one":1, "two":2}) == 2)		)");
}

FLOYD_LANG_PROOF("Floyd test suite", "dict [int] update()", "add", "correct dict, without side effect on original dict"){
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

FLOYD_LANG_PROOF("Floyd test suite", "dict [int] update()", "replace element", "correct dict, without side effect on original dict"){
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

FLOYD_LANG_PROOF("Floyd test suite", "dict [int] exists()", "", ""){
	ut_run_closed_nolib(R"(

		let a = { "one": 1, "two": 2, "three" : 3}
		assert(exists(a, "two") == true)
		assert(exists(a, "four") == false)

	)");
}

FLOYD_LANG_PROOF("Floyd test suite", "dict [int] erase()", "", ""){
	ut_run_closed_nolib(R"(

		let a = { "one": 1, "two": 2, "three" : 3}
		let b = erase(a, "one")
		assert(b == { "two": 2, "three" : 3})

	)");
}




//////////////////////////////////////////		DICT [STRING]

// Test using strings as value in dictionary. It is RC-based, not inline like int.


FLOYD_LANG_PROOF("Floyd test suite", "dict [string] construct", "", ""){
	ut_verify_printout_nolib(
		QUARK_POS,
		R"(

			print( { "one": "1000", "two": "2000" } )

		)",
		{ R"({"one": "1000", "two": "2000"})" }
	);
}


FLOYD_LANG_PROOF("Floyd test suite", "dict [string] []", "", ""){
	ut_verify_printout_nolib(
		QUARK_POS,
		R"(

			let a = { "one": "1000", "two": "2000" }
			print(a["one"])
			print(a["two"])

		)",
		{ "1000", "2000" }
	);
}

FLOYD_LANG_PROOF("Floyd test suite", "dict [string] ==", "", ""){
	ut_run_closed_nolib(R"(		assert(({"one": "1000", "two": "2000"} == {"one": "1000", "two": "2000"}) == true)		)");
}
FLOYD_LANG_PROOF("Floyd test suite", "dict [string] ==", "", ""){
	ut_run_closed_nolib(R"(		assert(({"one": "1000", "two": "2000"} == {"two": "2000"}) == false)		)");
}
FLOYD_LANG_PROOF("Floyd test suite", "dict [string] ==", "", ""){
	ut_run_closed_nolib(R"(		assert(({"one": "2000", "two": "2000"} == {"one": "1000", "two": "2000"}) == false)		)");
}
FLOYD_LANG_PROOF("Floyd test suite", "dict [string] ==", "", ""){
	ut_run_closed_nolib(R"(		assert(({"one": "1000", "two": "2000"} < {"one": "1000", "two": "2000"}) == false)		)");
}
FLOYD_LANG_PROOF("Floyd test suite", "dict [string] <", "", ""){
	ut_run_closed_nolib(R"(		assert(({"one": "1000", "two": "1000"} < {"one": "1000", "two": "2000"}) == true)		)");
}

FLOYD_LANG_PROOF("Floyd test suite", "dict [string] size()", "", "1"){
	ut_run_closed_nolib(R"(		assert(size({"one":"1000"}) == 1)		)");
}
FLOYD_LANG_PROOF("Floyd test suite", "dict [string] size()", "", "2"){
	ut_run_closed_nolib(R"(		assert(size({"one":"1000", "two":"2000"}) == 2)		)");
}

FLOYD_LANG_PROOF("Floyd test suite", "dict [string] update()", "add", "correct dict, without side effect on original dict"){
	ut_verify_printout_nolib(
		QUARK_POS,
		R"(

			let a = { "one": "1000", "two": "2000"}
			let b = update(a, "three", "3000")
			print(a)
			print(b)

		)",
		{
			R"({"one": "1000", "two": "2000"})",
			R"({"one": "1000", "three": "3000", "two": "2000"})"
		}
	);
}

FLOYD_LANG_PROOF("Floyd test suite", "dict [string] update()", "replace element", "correct dict, without side effect on original dict"){
	ut_verify_printout_nolib(
		QUARK_POS,
		R"(

			let a = { "a": "1000", "b": "2000", "c" : "3000"}
			let b = update(a, "c", "333")
			print(a)
			print(b)

		)",
		{
			R"({"a": "1000", "b": "2000", "c": "3000"})",
			R"({"a": "1000", "b": "2000", "c": "333"})"
		}
	);
}

FLOYD_LANG_PROOF("Floyd test suite", "dict [string] exists()", "", ""){
	ut_run_closed_nolib(R"(

		let a = { "one": "1000", "two": "2000", "three" : "3000"}
		assert(exists(a, "two") == true)
		assert(exists(a, "four") == false)

	)");
}

FLOYD_LANG_PROOF("Floyd test suite", "dict [string] erase()", "", ""){
	ut_run_closed_nolib(R"(

		let a = { "a": "1000", "b": "2000", "c" : "3000" }
		let b = erase(a, "a")
		assert(b == { "b": "2000", "c" : "3000"})

	)");
}



FLOYD_LANG_PROOF("Floyd test suite", "dict [string] get_keys()", "", ""){
	ut_run_closed_nolib(R"(

		let [string: int] a = {}
		let b = get_keys(a)
		assert(b == [])

	)");
}
FLOYD_LANG_PROOF("Floyd test suite", "dict [string] get_keys()", "", ""){
	ut_run_closed_nolib(R"(

		let a = { "a": 10 }
		let b = get_keys(a)
//		assert(b == [ "a", "b", "c"])

	)");
}
FLOYD_LANG_PROOF("Floyd test suite", "dict [string] get_keys()", "", ""){
	ut_run_closed_nolib(R"(

		let a = { "a": "ten" }
		let b = get_keys(a)
//		assert(b == [ "a", "b", "c"])

	)");
}
FLOYD_LANG_PROOF("Floyd test suite", "dict [string] get_keys()", "", ""){
	ut_run_closed_nolib(R"(

		let a = { "a": 1, "b": 2, "c" : 3 }
		let b = get_keys(a)
		assert(b == [ "a", "b", "c" ] || b == [ "c", "b", "a" ])

	)");
}





FLOYD_LANG_PROOF("Floyd test suite", "dict [string]", "Error: Lookup in dict using non-string key", "exception"){
	ut_verify_exception_nolib(
		QUARK_POS,
		R"(

			let a = { "one": "1000", "two": "2000" }
			print(a[3])

		)",
		"Dictionary can only be looked up using string keys, not a \"int\". Line: 4 \"print(a[3])\""
	);
}








//////////////////////////////////////////		TEST DICT CONSTRUCTOR FOR ALL TYPES





//	Notice: the order of dict-values is implementation specific: to_string() can give different results.
FLOYD_LANG_PROOF("Floyd test suite", "dict [bool] constructor", "", ""){
	ut_verify_printout_nolib(
		QUARK_POS,
		R"(

			let a = to_string({ "one": true, "two": false, "three": true })
			print(a)

		)",
		{ "{\"one\": true, \"three\": true, \"two\": false}" }
	);
}

//	Notice: the order of dict-values is implementation specific: to_string() can give different results.
FLOYD_LANG_PROOF("Floyd test suite", "dict [int] constructor", "", ""){
	ut_verify_printout_nolib(
		QUARK_POS,
		R"(

			let a = to_string({ "one": 1000, "two": 2000, "three": 3000 })
			print(a)

		)",
		{ "{\"one\": 1000, \"three\": 3000, \"two\": 2000}" }
	);
}
FLOYD_LANG_PROOF("Floyd test suite", "dict [double] constructor", "", ""){
	ut_verify_printout_nolib(
		QUARK_POS,
		R"(

			let a = to_string({ "one": 1.0, "two": 2.0, "three": 3.0 })
			print(a)

		)",
		{ "{\"one\": 1.0, \"three\": 3.0, \"two\": 2.0}" }
	);
}
FLOYD_LANG_PROOF("Floyd test suite", "dict [string] constructor", "", ""){
	ut_verify_printout_nolib(
		QUARK_POS,
		R"(

			let a = to_string({ "one": "1000", "two": "2000", "three": "3000" })
			print(a)

		)",
		{ R"___({"one": "1000", "three": "3000", "two": "2000"})___" }
	);
}

FLOYD_LANG_PROOF("Floyd test suite", "dict [typeid] constructor", "", ""){
	ut_verify_printout_nolib(
		QUARK_POS,
		R"(

			let a = to_string({ "one": int, "two": bool, "three": string })
			print(a)

		)",
		{ R"___({"one": int, "three": string, "two": bool})___" }
	);
}

FLOYD_LANG_PROOF("Floyd test suite", "dict [func] constructor", "", ""){
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

			let d = {"one": a, "two": b, "three": c}
			print(d)

		)",
		{
			R"___({"one": function int(string) pure, "three": function int(string) pure, "two": function int(string) pure})___"
		 }
	);
}


FLOYD_LANG_PROOF("Floyd test suite", "dict [struct] constructor", "", ""){
	ut_verify_printout_nolib(
		QUARK_POS,
		R"(

			struct person_t { string name int birth_year }

			let d = { "one": person_t("Mozart", 1782), "two": person_t("Bono", 1955) }
			print(d)

		)",
		{
			R"___({"one": {name="Mozart", birth_year=1782}, "two": {name="Bono", birth_year=1955}})___"
		 }
	);
}

FLOYD_LANG_PROOF("Floyd test suite", "dict [vector[string]] constructor", "", ""){
	ut_verify_printout_nolib(
		QUARK_POS,
		R"(

			let d = { "a": ["red", "blue"], "b": ["one", "two", "three"] }
			print(d)

		)",
		{
			R"___({"a": ["red", "blue"], "b": ["one", "two", "three"]})___"
		 }
	);
}

FLOYD_LANG_PROOF("Floyd test suite", "dict [dict[string]] constructor", "", ""){
	ut_verify_printout_nolib(
		QUARK_POS,
		R"(

			let d = { "a": {"color": "red", "color2": "blue"}, "b": {"num": "one", "num2": "two", "num3": "three"} }
			print(d)

		)",
		{
			R"___({"a": {"color": "red", "color2": "blue"}, "b": {"num": "one", "num2": "two", "num3": "three"}})___"
		 }
	);
}

FLOYD_LANG_PROOF("Floyd test suite", "dict [json] constructor", "", ""){
	ut_verify_printout_nolib(
		QUARK_POS,
		R"(

			let d = { "a": json("red"), "b": json("blue") }
			print(d)

		)",
		{
			R"___({"a": "red", "b": "blue"})___"
		 }
	);
}








//######################################################################################################################
//	STRUCT
//######################################################################################################################





FLOYD_LANG_PROOF("Floyd test suite", "struct", "update without quoting member name", ""){
	ut_verify_printout_nolib(
		QUARK_POS,
		R"(

			struct color { int red int green int blue }
			let a = color(0, 1, 2)
			let b = update(a, green, 100)
			print(b)

		)",
		{ "{red=0, green=100, blue=2}" }
	);
}




FLOYD_LANG_PROOF("Floyd test suite", "struct", "", ""){
	ut_run_closed_nolib(R"(		struct t {}		)");
}

FLOYD_LANG_PROOF("Floyd test suite", "struct", "", ""){
	ut_run_closed_nolib(R"(		struct t { int a }		)");
}

FLOYD_LANG_PROOF("Floyd test suite", "struct", "check struct's type", ""){
	ut_verify_printout_nolib(
		QUARK_POS,
		R"(

			struct t { int a }
			print(t)

		)",
		{ "struct {int a;}" }
	);
}

FLOYD_LANG_PROOF("Floyd test suite", "struct", "check struct's type", ""){
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

FLOYD_LANG_PROOF("Floyd test suite", "struct", "read back struct member", ""){
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

FLOYD_LANG_PROOF("Floyd test suite", "struct", "instantiate nested structs", ""){
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
FLOYD_LANG_PROOF("Floyd test suite", "struct", "instantiate nested structs 2", ""){
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

FLOYD_LANG_PROOF("Floyd test suite", "struct", "access member of nested structs", ""){
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

FLOYD_LANG_PROOF("Floyd test suite", "struct", "return struct from function", ""){
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

FLOYD_LANG_PROOF("Floyd test suite", "struct", "return struct from function", ""){
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

FLOYD_LANG_PROOF("Floyd test suite", "struct", "compare structs", ""){
	ut_verify_printout_nolib(
		QUARK_POS,
		R"(

			struct color { int red int green int blue }
			print(color(1, 2, 3) == color(1, 2, 3))

		)",
		{ "true" }
	);
}

FLOYD_LANG_PROOF("Floyd test suite", "struct", "compare structs", ""){
	ut_verify_printout_nolib(
		QUARK_POS,
		R"(

			struct color { int red int green int blue }
			print(color(9, 2, 3) == color(1, 2, 3))

		)",
		{ "false" }
	);
}

FLOYD_LANG_PROOF("Floyd test suite", "struct", "compare structs different types", ""){
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
FLOYD_LANG_PROOF("Floyd test suite", "struct", "compare structs with <, different types", ""){
	ut_verify_printout_nolib(
		QUARK_POS,
		R"(

			struct color { int red int green int blue }
			print(color(1, 2, 3) < color(1, 2, 3))

		)",
		{ "false" }
	);
}

FLOYD_LANG_PROOF("Floyd test suite", "struct", "compare structs <", ""){
	ut_verify_printout_nolib(
		QUARK_POS,
		R"(

			struct color { int red int green int blue }
			print(color(1, 2, 3) < color(1, 4, 3))

		)",
		{ "true" }
	);
}

FLOYD_LANG_PROOF("Floyd test suite", "struct", "", ""){
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

FLOYD_LANG_PROOF("Floyd test suite", "struct", "mutate struct member using = won't work", ""){
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

FLOYD_LANG_PROOF("Floyd test suite", "struct", "mutate struct member using update()", ""){
	ut_verify_printout_nolib(
		QUARK_POS,
		R"(

			struct color { int red int green int blue }
			let a = color(255,128,128)
			let b = update(a, green, 3)
			print(a)
			print(b)

		)",
		{ "{red=255, green=128, blue=128}", "{red=255, green=3, blue=128}" }
	);
}

#if 0
FLOYD_LANG_PROOF("Floyd test suite", "struct", "mutate nested member", ""){
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
#endif

//??? add more tests for struct with non-simple members
FLOYD_LANG_PROOF("Floyd test suite", "struct", "string member", ""){
	ut_run_closed_nolib(R"(

		struct context_t {
			double a
			string b
		}
		let x = context_t( 2000.0, "twenty")

	)");
}


FLOYD_LANG_PROOF("Floyd test suite", "struct", "Error: Define struct with colliding name", "exception"){
	ut_verify_exception_nolib(
		QUARK_POS,
		R"(

			let int a = 10
			struct a { int x }

		)",
		"Name \"a\" already used in current lexical scope. Line: 4 \"struct a { int x }\""
	);
}

FLOYD_LANG_PROOF("Floyd test suite", "struct", "Error: Access unknown struct member", "exception"){
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

FLOYD_LANG_PROOF("Floyd test suite", "struct", "Error: Access member in non-struct", "exception"){
	ut_verify_exception_nolib(
		QUARK_POS,
		R"(

			let a = 10
			print(a.y)

		)",
		"Left hand side is not a struct value, it's of type \"int\". Line: 4 \"print(a.y)\""
	);
}

FLOYD_LANG_PROOF("Floyd test suite", "struct", "", ""){
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


FLOYD_LANG_PROOF("Floyd test suite", "struct", "Read member of struct sitting in a vector", ""){
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

FLOYD_LANG_PROOF("Floyd test suite", "Struct", "Error: Wrong number of arguments to struct-constructor", "exception"){
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
FLOYD_LANG_PROOF("Floyd test suite", "struct", "Error: Wrong TYPE of arguments to struct-constructor", "exception"){
	ut_verify_exception_nolib(
		QUARK_POS,
		R"(

			struct pos { double x double y }
			let a = pos(3, "hello")

		)",
		"Expression type mismatch - cannot convert 'int' to 'double. Line: 4 \"let a = pos(3, \"hello\")\""
	);
}





//######################################################################################################################
//	json
//######################################################################################################################





//??? document or disable using json-value directly as lookup parent.

FLOYD_LANG_PROOF("Floyd test suite", "json::null", "", ""){
	ut_run_closed_nolib(R"(		let result = null		)");
}

FLOYD_LANG_PROOF("Floyd test suite", "json<string> Infer json::string", "", ""){
	ut_verify_global_result_nolib(QUARK_POS, R"(		let json result = "hello"		)", value_t::make_json("hello"));
}

FLOYD_LANG_PROOF("Floyd test suite", "json<string> string-size()", "", ""){
	ut_verify_global_result_nolib(
		QUARK_POS,
		R"(

			let json a = "hello"
			let result = size(a);

		)",
		value_t::make_int(5)
	);
}

FLOYD_LANG_PROOF("Floyd test suite", "json()", "json(123)", ""){
	ut_verify_global_result_nolib(QUARK_POS, "let result = json(123)", value_t::make_json(json_t(123.0)));
}

FLOYD_LANG_PROOF("Floyd test suite", "json()", "", ""){
	ut_verify_global_result_nolib(
		QUARK_POS,
		R"(

			let result = json("hello")

		)",
		value_t::make_json(json_t("hello"))
	);
}

FLOYD_LANG_PROOF("Floyd test suite", "json()", "", ""){
	ut_verify_global_result_nolib(QUARK_POS, "let result = json([1,2,3])", value_t::make_json(json_t::make_array({1,2,3})));
}

FLOYD_LANG_PROOF("Floyd test suite", "json<number> construct number", "", ""){
	ut_verify_global_result_nolib(QUARK_POS, R"(		let json result = 13		)", value_t::make_json(13));
}

FLOYD_LANG_PROOF("Floyd test suite", "json<true> construct true", "", ""){
	ut_verify_global_result_nolib(QUARK_POS, R"(		let json result = true		)", value_t::make_json(true));
}
FLOYD_LANG_PROOF("Floyd test suite", "json<false> construct false", "", ""){
	ut_verify_global_result_nolib(QUARK_POS, R"(		let json result = false		)", value_t::make_json(false));
}

FLOYD_LANG_PROOF("Floyd test suite", "json<array> construct array", "construct array", ""){
	ut_verify_global_result_nolib(QUARK_POS, R"(		let json result = ["hello", "bye"]		)", value_t::make_json(json_t::make_array(std::vector<json_t>{ "hello", "bye" }))	);
}

FLOYD_LANG_PROOF("Floyd test suite", "json<string> read array member", "", ""){
	ut_verify_global_result_nolib(
		QUARK_POS,
		R"(

			let json a = ["hello", "bye"]
			let result = a[0]

		)",
		value_t::make_json("hello")
	);
}
FLOYD_LANG_PROOF("Floyd test suite", "json<string> read array member", "", ""){
	ut_verify_global_result_nolib(
		QUARK_POS,
		R"(

			let json a = ["hello", "bye"]
			let result = string(a[0]) + string(a[1])

		)",
		value_t::make_string("hellobye")
	);
}

FLOYD_LANG_PROOF("Floyd test suite", "json<string> read array member", "", ""){
	ut_verify_global_result_nolib(
		QUARK_POS,
		R"(

			let json a = ["hello", "bye"]
			let result = a[1]

		)",
		value_t::make_json("bye")
	);
}

FLOYD_LANG_PROOF("Floyd test suite", "json<string> size()", "", ""){
	ut_verify_global_result_nolib(
		QUARK_POS,
		R"(

			let json a = ["a", "b", "c", "d"]
			let result = size(a)

		)",
		value_t::make_int(4)
	);
}

//	NOTICE: Floyd dict is stricter than JSON -- cannot have different types of values!
FLOYD_LANG_PROOF("Floyd test suite", "json<string> construct", "mix value-types in dict", ""){
	ut_verify_printout_nolib(
		QUARK_POS,
		R"(

			let json a = { "pigcount": 3, "pigcolor": "pink" }
			print(a)

		)",
		{ R"({ "pigcolor": "pink", "pigcount": 3 })" }
	);
}

// JSON example snippets: http://json.org/example.html
FLOYD_LANG_PROOF("Floyd test suite", "json<string> construct", "read world data", ""){
	ut_verify_printout_nolib(
		QUARK_POS,
		R"___(

			let json a = {
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

FLOYD_LANG_PROOF("Floyd test suite", "json<string> construct", "expressions inside def", ""){
	ut_verify_printout_nolib(
		QUARK_POS,
		R"___(

			let json a = { "pigcount": 1 + 2, "pigcolor": "pi" + "nk" }
			print(a)

		)___",
		{
			R"({ "pigcolor": "pink", "pigcount": 3 })"
		}
	);
}

FLOYD_LANG_PROOF("Floyd test suite", "json<string> construct", "", ""){
	ut_verify_printout_nolib(
		QUARK_POS,
		R"___(

			let json a = { "pigcount": 3, "pigcolor": "pink" }
			print(a["pigcount"])
			print(a["pigcolor"])

		)___",
		{ "3", "\"pink\"" }
	);
}

FLOYD_LANG_PROOF("Floyd test suite", "json<string> size()", "", ""){
	ut_run_closed_nolib(R"(

		let json a = { "a": 1, "b": 2, "c": 3, "d": 4, "e": 5 }
		assert(size(a) == 5)

	)");
}


FLOYD_LANG_PROOF("Floyd test suite", "json<null> construct", "", ""){
	ut_verify_global_result_nolib(QUARK_POS, R"(		let json result = null		)", value_t::make_json(json_t()));
}



FLOYD_LANG_PROOF("Floyd test suite", "json", "", "error"){
	ut_verify_exception_nolib(
		QUARK_POS,
		R"(

			struct pixel_t { double x double y }

			//	c is a json::object
			let c = { "version": "1.0", "image": [pixel_t(100.0, 200.0), pixel_t(101.0, 201.0)] }
			let result = c["image"][1].y

		)",
		"Dictionary of type [string:string] cannot hold an element of type [struct {double x;double y;}]. Line: 6 \"let c = { \"version\": \"1.0\", \"image\": [pixel_t(100.0, 200.0), pixel_t(101.0, 201.0)] }\""
	);
}



//######################################################################################################################
//	TYPE INFERENCE
//######################################################################################################################




FLOYD_LANG_PROOF("Floyd test suite", "json<string> {}", "Infer {}", "JSON object"){
	ut_verify_global_result_nolib(QUARK_POS, R"(		let json result = {}		)", value_t::make_json(json_t::make_object()));
}


FLOYD_LANG_PROOF("Floyd test suite", "get_json_type()", "", "1"){
	ut_verify_global_result_nolib(QUARK_POS, R"(		let result = get_json_type(json({}))		)", value_t::make_int(1));
}

FLOYD_LANG_PROOF("Floyd test suite", "get_json_type()", "", "1"){
	ut_verify_printout_nolib(
		QUARK_POS,
		R"(

			let result = json({ "color": "black"})
			print(result)

		)",
		{ R"xyz({ "color": "black" })xyz" }
	);
}
FLOYD_LANG_PROOF("Floyd test suite", "get_json_type()", "", "1"){
	ut_verify_global_result_nolib(
		QUARK_POS,
		R"(

			let result = get_json_type(json({ "color": "black"}))

		)",
		value_t::make_int(1)
	);
}
FLOYD_LANG_PROOF("Floyd test suite", "get_json_type()", "array", "2"){
	ut_verify_global_result_nolib(QUARK_POS, R"(		let result = get_json_type(json([]))		)", value_t::make_int(2));
}
FLOYD_LANG_PROOF("Floyd test suite", "get_json_type()", "string", "3"){
	ut_verify_global_result_nolib(QUARK_POS, R"(		let result = get_json_type(json("hello"))		)", value_t::make_int(3));
}
FLOYD_LANG_PROOF("Floyd test suite", "get_json_type()", "number", "4"){
	ut_verify_global_result_nolib(QUARK_POS, R"(		let result = get_json_type(json(13))		)", value_t::make_int(4));
}
FLOYD_LANG_PROOF("Floyd test suite", "get_json_type()", "true", "5"){
	ut_verify_global_result_nolib(QUARK_POS, R"(		let result = get_json_type(json(true))		)", value_t::make_int(5));
}
FLOYD_LANG_PROOF("Floyd test suite", "get_json_type()", "false", "6"){
	ut_verify_global_result_nolib(QUARK_POS, R"(		let result = get_json_type(json(false))		)", value_t::make_int(6));
}

FLOYD_LANG_PROOF("Floyd test suite", "get_json_type()", "null", "7"){
	ut_verify_global_result_nolib(QUARK_POS, R"(		let result = get_json_type(json(null))		)", value_t::make_int(7));
}

FLOYD_LANG_PROOF("Floyd test suite", "get_json_type()", "DOCUMENTATION SNIPPET", ""){
	ut_run_closed_nolib(R"___(

		func string get_name(json value){
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

		assert(get_name(json({"a": 1, "b": 2})) == "json_object")
		assert(get_name(json([1,2,3])) == "json_array")
		assert(get_name(json("crash")) == "json_string")
		assert(get_name(json(0.125)) == "json_number")
		assert(get_name(json(true)) == "json_true")
		assert(get_name(json(false)) == "json_false")

	)___");
}




//######################################################################################################################
//	CORE CALLS
//######################################################################################################################

//////////////////////////////////////////		CORE CALL - typeof()


FLOYD_LANG_PROOF("Floyd test suite", "typeof()", "", ""){
	ut_verify_global_result_nolib(
		QUARK_POS,
		R"(

			let result = typeof(145)

		)",
		value_t::make_typeid_value(typeid_t::make_int())
	);
}

FLOYD_LANG_PROOF("Floyd test suite", "typeof()", "", ""){
	ut_verify_global_result_nolib(QUARK_POS, R"(		let result = to_string(typeof(145))		)", value_t::make_string("int"));
}

FLOYD_LANG_PROOF("Floyd test suite", "typeof()", "", ""){
	ut_verify_global_result_nolib(QUARK_POS, R"(		let result = typeof("hello")		)", value_t::make_typeid_value(typeid_t::make_string()));
}

FLOYD_LANG_PROOF("Floyd test suite", "typeof()", "", ""){
	ut_verify_global_result_nolib(QUARK_POS, R"(		let result = to_string(typeof("hello"))		)",	value_t::make_string("string"));
}

FLOYD_LANG_PROOF("Floyd test suite", "typeof()", "", ""){
	ut_verify_global_result_nolib(QUARK_POS, R"(		let result = typeof([1,2,3])		)", value_t::make_typeid_value(typeid_t::make_vector(typeid_t::make_int()))	);
}
FLOYD_LANG_PROOF("Floyd test suite", "typeof()", "", ""){
	ut_verify_global_result_nolib(QUARK_POS, R"(		let result = to_string(typeof([1,2,3]))		)",value_t::make_string("[int]") );
}

FLOYD_LANG_PROOF("Floyd test suite", "typeof()", "", ""){
	ut_verify_global_result_nolib(
		QUARK_POS,
		R"(

			let result = to_string(typeof(int));

		)",
		value_t::make_string("typeid")
	);
}




//////////////////////////////////////////		parse_json_script()


FLOYD_LANG_PROOF("Floyd test suite", "parse_json_script()", "", ""){
	ut_verify_global_result_nolib(QUARK_POS, R"(		let result = parse_json_script("\"genelec\"")		)", value_t::make_json(json_t("genelec")));
}

FLOYD_LANG_PROOF("Floyd test suite", "parse_json_script()", "", ""){
	ut_verify_printout_nolib(
		QUARK_POS,
		R"___(

			let a = parse_json_script("{ \"magic\": 1234 }")
			print(a)

		)___",
		{ R"({ "magic": 1234 })" }
	);
}


//////////////////////////////////////////		generate_json_script()


FLOYD_LANG_PROOF("Floyd test suite", "generate_json_script()", "", ""){
	ut_verify_printout_nolib(
		QUARK_POS,
		R"___(

			let json a = "cheat"
			let b = generate_json_script(a)
			print(b)

		)___",
		{ "\"cheat\"" }
	);
}


FLOYD_LANG_PROOF("Floyd test suite", "generate_json_script()", "", ""){
	ut_verify_printout_nolib(
		QUARK_POS,
		R"___(

			let json a = { "magic": 1234 }
			let b = generate_json_script(a)
			print(b)

		)___",
		{ "{ \"magic\": 1234 }" }
	);
}


//////////////////////////////////////////		to_json()


FLOYD_LANG_PROOF("Floyd test suite", "to_json()", "bool", ""){
	ut_verify_global_result_nolib(QUARK_POS, R"(		let result = to_json(true)		)", value_t::make_json(json_t(true)));
}
FLOYD_LANG_PROOF("Floyd test suite", "to_json()", "bool", ""){
	ut_verify_global_result_nolib(QUARK_POS, R"(		let result = to_json(false)		)", value_t::make_json(json_t(false)));
}

FLOYD_LANG_PROOF("Floyd test suite", "to_json()", "int", ""){
	ut_verify_global_result_nolib(QUARK_POS, R"(		let result = to_json(789)		)", value_t::make_json(json_t(789.0)));
}
FLOYD_LANG_PROOF("Floyd test suite", "to_json()", "int", ""){
	ut_verify_global_result_nolib(QUARK_POS, R"(		let result = to_json(-987)		)", value_t::make_json(json_t(-987.0)));
}

FLOYD_LANG_PROOF("Floyd test suite", "to_json()", "double", ""){
	ut_verify_global_result_nolib(QUARK_POS, R"(		let result = to_json(-0.125)		)", value_t::make_json(json_t(-0.125)));
}


FLOYD_LANG_PROOF("Floyd test suite", "to_json()", "string", ""){
	ut_verify_global_result_nolib(QUARK_POS, R"(		let result = to_json("fanta")		)", value_t::make_json(json_t("fanta")));
}

FLOYD_LANG_PROOF("Floyd test suite", "to_json()", "typeid", ""){
	ut_verify_global_result_nolib(QUARK_POS, R"(		let result = to_json(typeof([2,2,3]))		)", value_t::make_json(json_t::make_array({ "vector", "int"})));
}

FLOYD_LANG_PROOF("Floyd test suite", "to_json()", "[]", ""){
	ut_verify_global_result_nolib(QUARK_POS, R"(		let result = to_json([1,2,3])		)", value_t::make_json(json_t::make_array({ 1, 2, 3 })));
}

FLOYD_LANG_PROOF("Floyd test suite", "to_json()", "{}", ""){
	ut_verify_global_result_nolib(
		QUARK_POS,
		R"(

			let result = to_json({"ten": 10, "eleven": 11})

		)",
		value_t::make_json(
			json_t::make_object({{ "ten", 10 }, { "eleven", 11 }})
		)
	);
}

FLOYD_LANG_PROOF("Floyd test suite", "generate_json_script()", "pixel_t", ""){
	ut_verify_global_result_nolib(
		QUARK_POS,
		R"(
	
			struct pixel_t { double x double y }
			let c = pixel_t(100.0, 200.0)
			let a = to_json(c)
			let result = generate_json_script(a)

		)",
		value_t::make_string("{ \"x\": 100, \"y\": 200 }")
	);
}

FLOYD_LANG_PROOF("Floyd test suite", "generate_json_script()", "[pixel_t]", ""){
	ut_verify_global_result_nolib(
		QUARK_POS,
		R"(

			struct pixel_t { double x double y }
			let c = [pixel_t(100.0, 200.0), pixel_t(101.0, 201.0)]
			let a = to_json(c)
			let result = generate_json_script(a)

		)",
		value_t::make_string("[{ \"x\": 100, \"y\": 200 }, { \"x\": 101, \"y\": 201 }]")
	);
}


//////////////////////////////////////////		to_json() -> from_json() roundtrip

/*
	from_json() returns different types depending on its 2nd argument.
	??? test all types!
*/
FLOYD_LANG_PROOF("Floyd test suite", "from_json()", "bool", ""){
	ut_verify_global_result_nolib(QUARK_POS, R"(		let result = from_json(to_json(true), bool)		)", value_t::make_bool(true));
}

FLOYD_LANG_PROOF("Floyd test suite", "from_json()", "bool", ""){
	ut_verify_global_result_nolib(QUARK_POS, R"(		let result = from_json(to_json(false), bool)		)", value_t::make_bool(false));
}

FLOYD_LANG_PROOF("Floyd test suite", "from_json()", "int", ""){
	ut_verify_global_result_nolib(QUARK_POS, R"(		let result = from_json(to_json(91), int)		)", value_t::make_int(91));
}

FLOYD_LANG_PROOF("Floyd test suite", "from_json()", "double", ""){
	ut_verify_global_result_nolib(QUARK_POS, R"(		let result = from_json(to_json(-0.125), double)		)", value_t::make_double(-0.125));
}

FLOYD_LANG_PROOF("Floyd test suite", "from_json()", "string", ""){
	ut_verify_global_result_nolib(QUARK_POS, R"(		let result = from_json(to_json(""), string)		)", value_t::make_string(""));
}

FLOYD_LANG_PROOF("Floyd test suite", "from_json()", "string", ""){
	ut_verify_global_result_nolib(QUARK_POS, R"(		let result = from_json(to_json("cola"), string)		)", value_t::make_string("cola"));
}

FLOYD_LANG_PROOF("Floyd test suite", "from_json()", "point_t", ""){
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
			let result = from_json(to_json(point_t(1.0, 3.0)), point_t)

		)",
		expected
	);
}








//////////////////////////////////////////		HIGHER-ORDER CORECALLS - map()



FLOYD_LANG_PROOF("Floyd test suite", "map()", "map over [int]", ""){
	ut_run_closed_nolib(R"(

		let a = [ 10, 11, 12 ]

		func int f(int v, string context){
			assert(context == "some context")
			return 1000 + v
		}

		let result = map(a, f, "some context")
//		print(to_string(result))
		assert(result == [ 1010, 1011, 1012 ])

	)");
}

FLOYD_LANG_PROOF("Floyd test suite", "map()", "map over [int]", ""){
	ut_run_closed_nolib(R"(

		let a = [ 10, 11, 12 ]

		func string f(int v, string context){
			return to_string(1000 + v)
		}

		let result = map(a, f, "some context")
//		print(to_string(result))
		assert(result == [ "1010", "1011", "1012" ])

	)");
}

FLOYD_LANG_PROOF("Floyd test suite", "map()", "map over [string]", ""){
	ut_run_closed_nolib(R"(

		let a = [ "one", "two_", "three" ]

		func int f(string v, string context){
			return size(v)
		}

		let result = map(a, f, "some context")
//		print(to_string(result))
		assert(result == [ 3, 4, 5 ])

	)");
}

FLOYD_LANG_PROOF	("Floyd test suite", "map()", "context struct", ""){
	ut_run_closed_nolib(R"(

		struct context_t { int a string b }

		func int f(int v, context_t context){
			assert(context.a == 2000)
			assert(context.b == "twenty")
			return context.a + v
		}

		let result = map([ 10, 11, 12 ], f, context_t( 2000, "twenty"))
		assert(result == [ 2010, 2011, 2012 ])

	)");
}
//??? make sure f() can't be impure!

/*
//////////////////////////////////////////		HIGHER-ORDER CORECALLS - map_string()



FLOYD_LANG_PROOF("Floyd test suite", "map_string()", "", ""){
	ut_run_closed_nolib(R"(

		let a = "ABC"

		func int f(int ch, string context){
			assert(context == "con")

			return ch)
		}

		let result = map_string(a, f, "con")
//		print(to_string(result))
		assert(result == "656667")

	)");
}
*/


//////////////////////////////////////////		HIGHER-ORDER CORECALLS - map_dag()



FLOYD_LANG_PROOF("Floyd test suite", "map_dag()", "No dependencies", ""){
	ut_run_closed_nolib(R"(

		func string f(string v, [string] inputs, string context){
			assert(context == "iop")
			return "[" + v + "]"
		}

		let result = map_dag([ "one", "ring", "to" ], [ -1, -1, -1 ], f, "iop")
//		print(to_string(result))
		assert(result == [ "[one]", "[ring]", "[to]" ])

	)");
}

FLOYD_LANG_PROOF("Floyd test suite", "map_dag()", "No dependencies", ""){
	ut_run_closed_nolib(R"(

		func string f(string v, [string] inputs, string context){
			assert(context == "qwerty")
			return "[" + v + "]"
		}

		let result = map_dag([ "one", "ring", "to" ], [ 1, 2, -1 ], f, "qwerty")
//		print(to_string(result))
		assert(result == [ "[one]", "[ring]", "[to]" ])

	)");
}

FLOYD_LANG_PROOF("Floyd test suite", "map_dag()", "complex", ""){
	ut_run_closed_nolib(R"(

		func string f2(string acc, string element, string context){
			assert(context == "12345678")
			if(size(acc) == 0){
				return element
			}
			else{
				return acc + ", " + element
			}
		}

		func string f(string v, [string] inputs, string context){
			assert(context == "1234")
			let s = reduce(inputs, "", f2, context + "5678")
			return v + "[" + s + "]"
		}

		let result = map_dag([ "D", "B", "A", "C", "E", "F" ], [ 4, 2, -1, 4, 2, 4 ], f, "1234")
//		print(to_string(result))
		assert(result == [ "D[]", "B[]", "A[B[], E[D[], C[], F[]]]", "C[]", "E[D[], C[], F[]]", "F[]" ])

	)");
}



//////////////////////////////////////////		HIGHER-ORDER CORECALLS - reduce()



FLOYD_LANG_PROOF("Floyd test suite", "reduce()", "int reduce([int], int, func int(int, int))", ""){
	ut_run_closed_nolib(R"(

		func int f(int acc, int element, string context){
			assert(context == "con123")
			return acc + element * 2
		}

		let result = reduce([ 10, 11, 12 ], 2000, f, "con123")

//		print(to_string(result))
		assert(result == 2066)

	)");
}

FLOYD_LANG_PROOF("Floyd test suite", "reduce()", "string reduce([int], string, func int(string, int))", ""){
	ut_run_closed_nolib(R"___(

		func string f(string acc, int v, string context){
			assert(context == "1234")

			mutable s = acc
			for(e in 0 ..< v){
				s = "<" + s + ">"
			}
			s = "(" + s + ")"
			return s
		}

		let result = reduce([ 2, 4, 1 ], "O", f, "1234")
//		print(to_string(result))
		assert(result == "(<(<<<<(<<O>>)>>>>)>)")

	)___");
}



//////////////////////////////////////////		HIGHER-ORDER CORECALLS - filter()



FLOYD_LANG_PROOF("Floyd test suite", "filter()", "int filter([int], int, func int(int, int))", ""){
	ut_run_closed_nolib(R"(

		func bool f(int element, string context){
			assert(context == "abcd")
			return element % 3 == 0 ? true : false
		}

		let result = filter([ 3, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13 ], f, "abcd")

//		print(to_string(result))
		assert(result == [ 3, 3, 6, 9, 12 ])

	)");
}

FLOYD_LANG_PROOF("Floyd test suite", "filter()", "string filter([int], string, func int(string, int))", ""){
	ut_run_closed_nolib(	R"___(

		func bool f(string element, string context){
			assert(context == "xyz")
			return size(element) == 3 || size(element) == 5
		}

		let result = filter([ "one", "two", "three", "four", "five", "six", "seven" ], f, "xyz")

//		print(to_string(result))
		assert(result == [ "one", "two", "three", "six", "seven" ])

	)___");
}



//////////////////////////////////////////		HIGHER-ORDER CORECALLS - stable_sort()



FLOYD_LANG_PROOF("Floyd test suite", "stable_sort()", "[int]", ""){
	ut_verify_printout_nolib(
		QUARK_POS,
		R"(

			func bool less_f(int left, int right, string s){
				return left < right
			}

			let result = stable_sort([ 1, 2, 8, 4 ], less_f, "hello")
			print(result)

		)",
		{ "[1, 2, 4, 8]" }
	);
}

FLOYD_LANG_PROOF("Floyd test suite", "stable_sort()", "[int] reverse", ""){
	ut_verify_printout_nolib(
		QUARK_POS,
		R"(

			func bool less_f(int left, int right, string s){
				return left > right
			}

			let result = stable_sort([ 1, 2, 8, 4 ], less_f, "hello")
			print(result)

		)",
		{ "[8, 4, 2, 1]" }
	);
}

FLOYD_LANG_PROOF("Floyd test suite", "stable_sort()", "Test context argument", ""){
	ut_verify_printout_nolib(
		QUARK_POS,
		R"(

			struct state_t {
				string s
				bool less_flag
			}

			func bool less_f(int left, int right, state_t s){
				assert(s.s == "xyz")
				return s.less_flag ? left < right : right < left
			}

			let result1 = stable_sort([ 1, 2, 8, 4 ], less_f, state_t("xyz", true))
			print(result1)
			let result2 = stable_sort([ 1, 2, 8, 4 ], less_f, state_t("xyz", false))
			print(result2)

		)",
		{ "[1, 2, 4, 8]", "[8, 4, 2, 1]" }
	);
}

FLOYD_LANG_PROOF("Floyd test suite", "stable_sort()", "[string]", ""){
	ut_verify_printout_nolib(
		QUARK_POS,
		R"(

			func bool less_f(string left, string right, string s){
				return left < right
			}

			let result = stable_sort([ "1", "2", "8", "4" ], less_f, "hello")
			print(result)

		)",
		{ R"___(["1", "2", "4", "8"])___" }
	);
}








//######################################################################################################################
//	CORE LIBRARY
//######################################################################################################################





FLOYD_LANG_PROOF("Floyd test suite", "cmath_pi", "", ""){
	ut_verify_global_result_lib(
		QUARK_POS,
		R"(

			let x = cmath_pi
			let result = x >= 3.14 && x < 3.15

		)",
		value_t::make_bool(true)
	);
}

FLOYD_LANG_PROOF("Floyd test suite", "color__black", "", ""){
	ut_run_closed_lib(R"(

		assert(color__black.red == 0.0)
		assert(color__black.green == 0.0)
		assert(color__black.blue == 0.0)
		assert(color__black.alpha == 1.0)

	)");
}


FLOYD_LANG_PROOF("Floyd test suite", "color__black", "", ""){
	ut_run_closed_lib(R"(

		let r = add_colors(color_t(1.0, 2.0, 3.0, 4.0), color_t(1000.0, 2000.0, 3000.0, 4000.0))
		assert(r.red == 1001.0)
		assert(r.green == 2002.0)
		assert(r.blue == 3003.0)
		assert(r.alpha == 4004.0)

	)");
}

FLOYD_LANG_PROOF("Floyd test suite", "", "pixel_t()", ""){
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




FLOYD_LANG_PROOF("Floyd test suite", "", "", ""){
	const auto a = typeid_t::make_vector(typeid_t::make_string());
	const auto b = typeid_t::make_vector(make__fsentry_t__type());
	ut_verify_auto(QUARK_POS, a != b, true);
}





//////////////////////////////////////////		CORE LIBRARY - get_time_of_day()

FLOYD_LANG_PROOF("Floyd test suite", "get_time_of_day()", "", ""){
	ut_run_closed_lib(R"(

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

FLOYD_LANG_PROOF("Floyd test suite", "get_time_of_day()", "", ""){
	ut_run_closed_lib(R"(

		let int a = get_time_of_day()
		let int b = get_time_of_day()
		let int c = b - a
//		print("Delta time:" + to_string(a))

	)");
}

//////////////////////////////////////////		CORE LIBRARY - calc_string_sha1()


FLOYD_LANG_PROOF("Floyd test suite", "calc_string_sha1()", "", ""){
	ut_run_closed_lib(R"(

		let a = calc_string_sha1("Violator is the seventh studio album by English electronic music band Depeche Mode.")
//		print(to_string(a))
		assert(a.ascii40 == "4d5a137b3b1faf855872a312a184dd9a24594387")

	)");
}


//////////////////////////////////////////		CORE LIBRARY - calc_binary_sha1()


FLOYD_LANG_PROOF("Floyd test suite", "calc_binary_sha1()", "", ""){
	ut_run_closed_lib(R"(

		let bin = binary_t("Violator is the seventh studio album by English electronic music band Depeche Mode.")
		let a = calc_binary_sha1(bin)
//		print(to_string(a))
		assert(a.ascii40 == "4d5a137b3b1faf855872a312a184dd9a24594387")

	)");
}






//////////////////////////////////////////		CORE LIBRARY - read_text_file()

/*
FLOYD_LANG_PROOF("Floyd test suite", "read_text_file()", "", ""){
	ut_run_closed_nolib(R"(

		path = get_env_path();
		a = read_text_file(path + "/Desktop/test1.json");
		print(a);

	)");
	ut_verify(QUARK_POS, vm->_print_output, { string() + R"({ "magic": 1234 })" + "\n" });
}
*/


//////////////////////////////////////////		CORE LIBRARY - write_text_file()


FLOYD_LANG_PROOF("Floyd test suite", "write_text_file()", "", ""){
	ut_run_closed_lib(R"(

		let path = get_fs_environment().desktop_dir
		write_text_file(path + "/test_out.txt", "Floyd wrote this!")

	)");
}

//////////////////////////////////////////		CORE CALL - instantiate_from_typeid()

//	instantiate_from_typeid() only works for const-symbols right now.
/*
??? Support when we can make "t = typeof(1234)" a const-symbol
FLOYD_LANG_PROOF("Floyd test suite", "", "", ""){
	const auto result = test__run_return_result(
		R"(
			t = typeof(1234);
			result = instantiate_from_typeid(t, 3);
		)", {}
	);
	ut_verify_values(QUARK_POS, result, value_t::make_int(3));
}

FLOYD_LANG_PROOF("Floyd test suite", "", "", ""){
	ut_run_closed_nolib(R"(

		a = instantiate_from_typeid(typeof(123), 3);
		assert(to_string(typeof(a)) == "int");
		assert(a == 3);

	)");
}
*/


//////////////////////////////////////////		CORE LIBRARY - get_directory_entries()


FLOYD_LANG_PROOF("Floyd test suite", "get_fsentries_shallow()", "", ""){
	ut_verify_global_result_lib(
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


//////////////////////////////////////////		CORE LIBRARY - get_fsentries_deep()


FLOYD_LANG_PROOF("Floyd test suite", "get_fsentries_deep()", "", ""){
	ut_run_closed_lib(
		R"(

			let result = get_fsentries_deep("/Users/marcus/Desktop/")
			assert(size(result) > 3)
//			print(to_pretty_string(result))

		)"
	);
}


//////////////////////////////////////////		CORE LIBRARY - get_fsentry_info()


FLOYD_LANG_PROOF("Floyd test suite", "get_fsentry_info()", "", ""){
	ut_verify_global_result_lib(
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


//////////////////////////////////////////		CORE LIBRARY - get_fs_environment()


FLOYD_LANG_PROOF("Floyd test suite", "get_fs_environment()", "", ""){
	ut_verify_global_result_lib(
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




//////////////////////////////////////////		CORE LIBRARY - does_fsentry_exist()


FLOYD_LANG_PROOF("Floyd test suite", "does_fsentry_exist()", "", ""){
	ut_verify_global_result_lib(
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

FLOYD_LANG_PROOF("Floyd test suite", "does_fsentry_exist()", "", ""){
	ut_verify_global_result_lib(
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



//////////////////////////////////////////		CORE LIBRARY - create_directory_branch()


FLOYD_LANG_PROOF("Floyd test suite", "create_directory_branch()", "", ""){
	remove_test_dir("unittest___create_directory_branch", "subdir");

	ut_run_closed_lib(
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


//////////////////////////////////////////		CORE LIBRARY - delete_fsentry_deep()


FLOYD_LANG_PROOF("Floyd test suite", "delete_fsentry_deep()", "", ""){
	remove_test_dir("unittest___delete_fsentry_deep", "subdir");

	ut_run_closed_lib(
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



//////////////////////////////////////////		CORE LIBRARYCORE LIBRARY - rename_fsentry()


FLOYD_LANG_PROOF("Floyd test suite", "rename_fsentry()", "", ""){
	ut_run_closed_lib(
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




//######################################################################################################################
//	PARSER ERRORS
//######################################################################################################################




FLOYD_LANG_PROOF("Parser error", "", "", ""){
	ut_verify_exception_nolib(QUARK_POS, R"(		£		)", R"___(Illegal characters. Line: 1 "£")___");
}


FLOYD_LANG_PROOF("Parser error", "", "", ""){
	ut_verify_exception_nolib(QUARK_POS, R"(		{ let a = 10		)", R"___(Block is missing end bracket '}'. Line: 1 "{ let a = 10")___");
}

FLOYD_LANG_PROOF("Parser error", "", "", ""){
	ut_verify_exception_nolib(
		QUARK_POS,
		R"(

			[ 100, 200 }

		)",
		R"___(Unexpected char "}" in bounded list [ ]! Line: 3 "[ 100, 200 }")___"
	);
}

FLOYD_LANG_PROOF("Parser error", "", "", ""){
	ut_verify_exception_nolib(
		QUARK_POS,
		R"(

			x = { "a": 100 ]

		)",
		R"___(Unexpected char "]" in bounded list { }! Line: 3 "x = { "a": 100 ]")___"
	);
}


FLOYD_LANG_PROOF("Parser error", "", "", ""){
	ut_verify_exception_nolib(
		QUARK_POS,
		R"("abc\)",
		R"___(Incomplete escape sequence in string literal: "abc"! Line: 1 ""abc\")___"
	);
}






#if 0
FLOYD_LANG_PROOF("Analyse all test programs", "", "", ""){
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









//######################################################################################################################
//	SOFTWARE-SYSTEM-DEF
//######################################################################################################################




FLOYD_LANG_PROOF("software-system-def", "parse software-system-def", "", ""){
	const auto test_ss = R"(

		software-system-def {
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

	ut_run_closed_nolib(test_ss);
}

#if 0
FLOYD_LANG_PROOF("", "try calling LLVM function", "", ""){
	const auto p = R"(

		software-system-def {
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

		func double my_gui(double state, json message) impure{
			print("received: " + to_string(message) + ", state: " + to_string(state))
			return state
		}

	)";

	const auto result = test_run_container3(p, {}, "");
	QUARK_UT_VERIFY(result.empty());
}
#endif




//######################################################################################################################
//	CONTAINER-DEF AND PROCESSES
//######################################################################################################################



FLOYD_LANG_PROOF("software-system-def", "run one process", "", ""){
	const auto test_ss2 = R"(

		software-system-def {
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

		func my_gui_state_t my_gui(my_gui_state_t state, json message) impure{
			print("received: " + to_string(message) + ", state: " + to_string(state))

			if(message == "inc"){
				return update(state, _count, state._count + 1)
			}
			else if(message == "dec"){
				return update(state, _count, state._count - 1)
			}
			else{
				assert(false)
				return state
			}
		}

	)";

	const auto result = test_run_container3(test_ss2, {}, "");
	QUARK_UT_VERIFY(result == run_output_t());
}

FLOYD_LANG_PROOF("software-system-def", "run two unconnected processs", "", ""){
	const auto test_ss3 = R"(

		software-system-def {
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

		func my_gui_state_t my_gui(my_gui_state_t state, json message) impure {
			print("my_gui --- received: " + to_string(message) + ", state: " + to_string(state))

			if(message == "inc"){
				return update(state, _count, state._count + 1)
			}
			else if(message == "dec"){
				return update(state, _count, state._count - 1)
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

		func my_audio_state_t my_audio(my_audio_state_t state, json message) impure {
			print("my_audio --- received: " + to_string(message) + ", state: " + to_string(state))

			if(message == "process"){
				return update(state, _audio, state._audio + 1)
			}
			else{
				assert(false)
				return state
			}
		}

	)";

	const auto result = test_run_container3(test_ss3, {}, "");
	QUARK_UT_VERIFY(result == run_output_t());
}

FLOYD_LANG_PROOF("software-system-def", "run two CONNECTED processes", "", ""){
	const auto test_ss3 = R"(

		software-system-def {
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

		func my_gui_state_t my_gui(my_gui_state_t state, json message) impure {
			print("my_gui --- received: " + to_string(message) + ", state: " + to_string(state))

			if(message == "2"){
				send("b", "3")
				return update(state, _count, state._count + 1)
			}
			else if(message == "4"){
				send("a", "stop")
				send("b", "stop")
				return update(state, _count, state._count + 10)
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

		func my_audio_state_t my_audio(my_audio_state_t state, json message) impure {
			print("my_audio --- received: " + to_string(message) + ", state: " + to_string(state))

			if(message == "1"){
				send("a", "2")
				return update(state, _audio, state._audio + 1)
			}
			else if(message == "3"){
				send("a", "4")
				return update(state, _audio, state._audio + 4)
			}
			else{
				assert(false)
				return state
			}
		}

	)";

	const auto result = test_run_container3(test_ss3, {}, "");
	QUARK_UT_VERIFY(result == run_output_t() );
}





//######################################################################################################################
//	RUN ALL EXAMPLE PROGRAMS -- VERIFY THEY WORK
//######################################################################################################################




FLOYD_LANG_PROOF("Floyd test suite", "hello_world.floyd", "", ""){
	const auto path = get_working_dir() + "/examples/hello_world.floyd";
	const auto program = read_text_file(path);

	const auto result = test_run_container3(program, {}, "");
	const run_output_t expected = {};
	QUARK_UT_VERIFY(result == expected);
}

FLOYD_LANG_PROOF("Floyd test suite", "game_of_life.floyd", "", ""){
	const auto path = get_working_dir() + "/examples/game_of_life.floyd";
	const auto program = read_text_file(path);

	const auto result = test_run_container3(program, {}, "");
	const run_output_t expected = {};
	QUARK_UT_VERIFY(result == expected);
}





//######################################################################################################################
//	QUICK REFERENCE SNIPPETS -- VERIFY THEY WORK
//######################################################################################################################






FLOYD_LANG_PROOF("QUICK REFERENCE SNIPPETS", "TERNARY OPERATOR", "", ""){
	test_run_container3(R"(

//	Snippets setup
let b = ""



let a = b == "true" ? true : false

	)", {}, "");
}

FLOYD_LANG_PROOF("QUICK REFERENCE SNIPPETS", "COMMENTS", "", ""){
	test_run_container3(R"(

/* Comment can span lines. */

let a = 10; // To end of line

	)", {}, "");
}



FLOYD_LANG_PROOF("QUICK REFERENCE SNIPPETS", "LOCALS", "", ""){
	test_run_container3(R"(

let a = 10
mutable b = 10
b = 11

	)", {}, "");
}


#if 0
FLOYD_LANG_PROOF("QUICK REFERENCE SNIPPETS", "WHILE", "", ""){
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


FLOYD_LANG_PROOF("QUICK REFERENCE SNIPPETS", "FOR", "", ""){
	test_run_container3(R"(

for (index in 1 ... 5) {
	print(index)
}
for (index in 1  ..< 5) {
	print(index)
}

	)", {}, "");
}


FLOYD_LANG_PROOF("QUICK REFERENCE SNIPPETS", "IF ELSE", "", ""){
	test_run_container3(R"(

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


FLOYD_LANG_PROOF("QUICK REFERENCE SNIPPETS", "BOOL", "", ""){
	test_run_container3(R"(

let a = true
if(a){
}

	)", {}, "");
}

FLOYD_LANG_PROOF("QUICK REFERENCE SNIPPETS", "STRING", "", ""){
	test_run_container3(R"(

let s1 = "Hello, world!"
let s2 = "Title: " + s1
assert(s2 == "Title: Hello, world!")
assert(s1[0] == 72) // ASCII for 'H'
assert(size(s1) == 13)
assert(subset(s1, 1, 4) == "ell")
let s4 = to_string(12003)

	)", {}, "");
}




FLOYD_LANG_PROOF("QUICK REFERENCE SNIPPETS", "FUNCTION", "", ""){
	test_run_container3(R"(

func string f(double a, string s){
	return to_string(a) + ":" + s
}

let a = f(3.14, "km")

	)", {}, "");
}

FLOYD_LANG_PROOF("QUICK REFERENCE SNIPPETS", "IMPURE FUNCTION", "", ""){
	test_run_container3(R"(

func int main([string] args) impure {
	return 1
}

	)", {}, "");
}



FLOYD_LANG_PROOF("QUICK REFERENCE SNIPPETS", "STRUCT", "", ""){
	test_run_container3(R"(

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

let b = update(a, width, 100.0)
assert(a.width == 0.0)
assert(b.width == 100.0)

	)", {}, "");
}


FLOYD_LANG_PROOF("QUICK REFERENCE SNIPPETS", "VECTOR", "", ""){
	test_run_container3(R"(

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


FLOYD_LANG_PROOF("QUICK REFERENCE SNIPPETS", "DICTIONARY", "", ""){
	test_run_container3(R"(

let a = { "one": 1, "two": 2 }
assert(a["one"] == 1)

let b = update(a, "one", 10)
assert(a == { "one": 1, "two": 2 })
assert(b == { "one": 10, "two": 2 })

	)", {}, "");
}


FLOYD_LANG_PROOF("QUICK REFERENCE SNIPPETS", "json", "", ""){
	test_run_container3(R"(

let json a = {
	"one": 1,
	"three": "three",
	"five": { "A": 10, "B": 20 }
}

	)", {}, "");
}





FLOYD_LANG_PROOF("QUICK REFERENCE SNIPPETS", "MAP", "", ""){
	test_run_container3(R"(

		func int f(int v){
			return 1000 + v
		}
		let result = map([ 10, 11, 12 ], f)
		assert(result == [ 1010, 1011, 1012 ])

	)", {}, "");
}





//######################################################################################################################
//	MANUAL SNIPPETS -- VERIFY THEY WORK
//######################################################################################################################




FLOYD_LANG_PROOF("MANUAL SNIPPETS", "subset()", "", ""){
	test_run_container3(R"(

		assert(subset("hello", 2, 4) == "ll")
		assert(subset([ 10, 20, 30, 40 ], 1, 3 ) == [ 20, 30 ])

	)", {}, "");
}

