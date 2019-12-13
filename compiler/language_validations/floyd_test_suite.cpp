
//
//  floyd_test_suite.cpp
//  Floyd
//
//  Created by Marcus Zetterquist on 2018-01-21.
//  Copyright © 2018 Marcus Zetterquist. All rights reserved.
//

#include "floyd_test_suite.h"

#include "test_helpers.h"
#include "text_parser.h"
#include "floyd_corelib.h"
#include "unistd.h"

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


//??? Test truthness off all variable types: strings, doubles


#if 1
#define RUN_LANG_BASIC_TESTS1				true
#define RUN_LANG_BASIC_TESTS2				true
#define RUN_LANG_VECTOR_TESTS				true
#define RUN_LANG_DICT_TESTS					true
#define RUN_LANG_STRUCT_TESTS				true
#define RUN_LANG_JSON_TESTS					true
#define RUN_LANG_INTRINSICS_TESTS			true
#define RUN_BENCHMARK_DEF_STATEMENT__AND__BENCHMARK_EXPRESSION_TESTS true
#define RUN_CORELIB_TESTS					true
#define RUN_CONTAINER_TESTS					true
#define RUN_EXAMPLE_AND_DOCS_TESTS			true
#else
//#define RUN_LANG_BASIC_TESTS1				true
//#define RUN_LANG_BASIC_TESTS2				true
//#define RUN_LANG_VECTOR_TESTS				true
//#define RUN_LANG_DICT_TESTS					true
#define RUN_LANG_STRUCT_TESTS				true
//#define RUN_LANG_JSON_TESTS					true
//#define RUN_LANG_INTRINSICS_TESTS			true
//#define RUN_BENCHMARK_DEF_STATEMENT__AND__BENCHMARK_EXPRESSION_TESTS true
//#define RUN_CORELIB_TESTS					true
//#define RUN_CONTAINER_TESTS					true
//#define RUN_EXAMPLE_AND_DOCS_TESTS			true
#endif


#define FLOYD_LANG_PROOF(class_under_test, function_under_test, scenario, expected_result) \
	static void QUARK_UNIQUE_LABEL(quark_unit_test_)(); \
	static ::quark::unit_test_rec QUARK_UNIQUE_LABEL(rec)(__FILE__, __LINE__, class_under_test, function_under_test, scenario, expected_result, QUARK_UNIQUE_LABEL(quark_unit_test_), false); \
	static void QUARK_UNIQUE_LABEL(quark_unit_test_)()

#define FLOYD_LANG_PROOF_VIP(class_under_test, function_under_test, scenario, expected_result) \
	static void QUARK_UNIQUE_LABEL(quark_unit_test_)(); \
	static ::quark::unit_test_rec QUARK_UNIQUE_LABEL(rec)(__FILE__, __LINE__, class_under_test, function_under_test, scenario, expected_result, QUARK_UNIQUE_LABEL(quark_unit_test_), true); \
	static void QUARK_UNIQUE_LABEL(quark_unit_test_)()



static json_t make_expected_int(int64_t v){
	types_t types;
	const auto a = value_t::make_int(v);
	return value_and_type_to_json(types, a);
}
static json_t make_expected_string(const std::string& s){
	types_t types;
	const auto a = value_t::make_string(s);
	return value_and_type_to_json(types, a);
}
static json_t make_expected_double(double v){
	types_t types;
	const auto a = value_t::make_double(v);
	return value_and_type_to_json(types, a);
}
static json_t make_expected_bool(bool v){
	types_t types;
	const auto a = value_t::make_bool(v);
	return value_and_type_to_json(types, a);
}

static json_t make_expected_json(const json_t& j){
	types_t types;
	const auto a = value_t::make_json(j);
	return value_and_type_to_json(types, a);
}

static json_t make_expected_typeid(const types_t& types, const type_t& s){
	const auto a = value_t::make_typeid_value(s);
	return value_and_type_to_json(types, a);
}


static json_t make_bool_vec(const std::vector<bool>& elements){
	types_t types;
	std::vector<value_t> elements2;
	for(const auto e: elements){
		elements2.push_back(value_t::make_bool(e));
	}

	const auto v = value_t::make_vector_value(types, type_t::make_bool(), elements2);
	return value_and_type_to_json(types, v);
}

static json_t make_int_vec(const std::vector<int64_t>& elements){
	types_t types;
	std::vector<value_t> elements2;
	for(const auto& e: elements){
		elements2.push_back(value_t::make_int(e));
	}

	const auto v = value_t::make_vector_value(types, type_t::make_int(), elements2);
	return value_and_type_to_json(types, v);
}

static json_t make_double_vec(const std::vector<double>& elements){
	types_t types;
	std::vector<value_t> elements2;
	for(const auto& e: elements){
		elements2.push_back(value_t::make_double(e));
	}

	const auto v = value_t::make_vector_value(types, type_t::make_double(), elements2);
	return value_and_type_to_json(types, v);
}



#if 0
FLOYD_LANG_PROOF("NOP", "See if we leak memory", "", ""){
    usleep(11 * 1000000);
}
#endif


#if RUN_LANG_BASIC_TESTS1

//######################################################################################################################
//	DEFINE VARIABLE, SIMPLE TYPES
//######################################################################################################################


FLOYD_LANG_PROOF("Floyd test suite", "Define variable", "int", ""){
	ut_verify_global_result_nolib(QUARK_POS, "let int result = 123", make_expected_int(123));
}

FLOYD_LANG_PROOF("Floyd test suite", "Define variable", "bool", ""){
	ut_verify_global_result_nolib(QUARK_POS, "let bool result = true", make_expected_bool(true));
}
FLOYD_LANG_PROOF("Floyd test suite", "Define variable", "bool", ""){
	ut_verify_global_result_nolib(QUARK_POS, "let bool result = false", make_expected_bool(false));
}

FLOYD_LANG_PROOF("Floyd test suite", "Define variable", "int", ""){
	ut_verify_global_result_nolib(QUARK_POS, "let int result = 123", make_expected_int(123));
}

FLOYD_LANG_PROOF("Floyd test suite", "Define variable", "double", ""){
	ut_verify_global_result_nolib(QUARK_POS, "let double result = 3.5", make_expected_double(double(3.5)));
}

FLOYD_LANG_PROOF("Floyd test suite", "Define variable", "string", ""){
	ut_verify_global_result_nolib(QUARK_POS, R"xxx(let string result = "xyz")xxx", make_expected_string("xyz"));
}

//	??? Add special error message when local is not initialized.
FLOYD_LANG_PROOF("Floyd test suite", "Define variable", "Error: No initial assignment", "compiler error"){
	ut_verify_exception_nolib(
		QUARK_POS,
		R"(

			mutable string row

		)",
		R"___([Syntax] Expected '=' characters. Line: 3 "mutable string row")___"
	);
}
FLOYD_LANG_PROOF("Floyd test suite", "Define variable", "Error: assign to immutable local", "exception"){
	ut_verify_exception_nolib(
		QUARK_POS,
		R"(

			let int a = 10
			let int a = 11

		)",
		"[Semantics] Local identifier \"a\" already exists. Line: 4 \"let int a = 11\""
	);
}

FLOYD_LANG_PROOF("Floyd test suite", "Define variable", "Error: assign to unknown local", "exception"){
	ut_verify_exception_nolib(
		QUARK_POS,
		R"(

			a = 10

		)",
		"[Semantics] Unknown identifier 'a'. Line: 3 \"a = 10\""
	);
}




//######################################################################################################################
//	BASIC EXPRESSIONS
//######################################################################################################################



//////////////////////////////////////////		BASIC EXPRESSIONS, SIMPLE TYPES


FLOYD_LANG_PROOF("Floyd test suite", "+", "", "") {
	ut_verify_global_result_nolib(QUARK_POS, "let int result = 1 + 2", make_expected_int(3));
}
FLOYD_LANG_PROOF("Floyd test suite", "+", "", ""){
	ut_verify_global_result_nolib(QUARK_POS, "let int result = 1 + 2 + 3", make_expected_int(6));
}
FLOYD_LANG_PROOF("Floyd test suite", "*", "", ""){
	ut_verify_global_result_nolib(QUARK_POS, "let int result = 3 * 4", make_expected_int(12));
}

FLOYD_LANG_PROOF("Floyd test suite", "parant", "", ""){
	ut_verify_global_result_nolib(QUARK_POS, "let int result = (3 * 4) * 5", make_expected_int(60));
}



//////////////////////////////////////////		BASIC EXPRESSIONS - CONDITIONAL EXPRESSION


FLOYD_LANG_PROOF("Floyd test suite", "conditional expression", "", ""){
	ut_verify_global_result_nolib(QUARK_POS, "let int result = true ? 98 : 99", make_expected_int(98));
}
FLOYD_LANG_PROOF("Floyd test suite", "conditional expression", "", ""){
	ut_verify_global_result_nolib(QUARK_POS, "let int result = false ? 70 : 80", make_expected_int(80));
}


FLOYD_LANG_PROOF("Floyd test suite", "conditional expression", "", ""){
	ut_verify_global_result_nolib(QUARK_POS, "let string result = true ? \"yes\" : \"no\"", make_expected_string("yes"));
}
FLOYD_LANG_PROOF("Floyd test suite", "conditional expression", "", ""){
	ut_verify_global_result_nolib(QUARK_POS, "let string result = false ? \"yes\" : \"no\"", make_expected_string("no"));
}

FLOYD_LANG_PROOF("Floyd test suite", "conditional expression", "?:", ""){
	ut_verify_global_result_nolib(QUARK_POS, "let int result = true ? 4 : 6", make_expected_int(4));
}
FLOYD_LANG_PROOF("Floyd test suite", "conditional expression", "?:", ""){
	ut_verify_global_result_nolib(QUARK_POS, "let int result = false ? 4 : 6", make_expected_int(6));
}

FLOYD_LANG_PROOF("Floyd test suite", "conditional expression", "?:", ""){
	ut_verify_global_result_nolib(QUARK_POS, "let int result = 1==3 ? 4 : 6", make_expected_int(6));
}
FLOYD_LANG_PROOF("Floyd test suite", "conditional expression", "?:", ""){
	ut_verify_global_result_nolib(QUARK_POS, "let int result = 3==3 ? 4 : 6", make_expected_int(4));
}

FLOYD_LANG_PROOF("Floyd test suite", "conditional expression", "?:", ""){
	ut_verify_global_result_nolib(QUARK_POS, "let int result = 3==3 ? 2 + 2 : 2 * 3", make_expected_int(4));
}

FLOYD_LANG_PROOF("Floyd test suite", "conditional expression", "?:", ""){
	ut_verify_global_result_nolib(QUARK_POS, "let int result = 3==1+2 ? 2 + 2 : 2 * 3", make_expected_int(4));
}


//////////////////////////////////////////		BASIC EXPRESSIONS - PARANTHESES


FLOYD_LANG_PROOF("Floyd test suite", "execute_expression()", "Parentheses", ""){
	ut_verify_global_result_nolib(QUARK_POS, "let int result = 5*(4+4+1)", make_expected_int(45));
}
FLOYD_LANG_PROOF("Floyd test suite", "execute_expression()", "Parentheses", ""){
	ut_verify_global_result_nolib(QUARK_POS, "let int result = 5*(2*(1+3)+1)", make_expected_int(45));
}
FLOYD_LANG_PROOF("Floyd test suite", "execute_expression()", "Parentheses", ""){
	ut_verify_global_result_nolib(QUARK_POS, "let int result = 5*((1+3)*2+1)", make_expected_int(45));
}

FLOYD_LANG_PROOF("Floyd test suite", "execute_expression()", "Sign before parentheses", ""){
	ut_verify_global_result_nolib(QUARK_POS, "let int result = -(2+1)*4", make_expected_int(-12));
}
FLOYD_LANG_PROOF("Floyd test suite", "execute_expression()", "Sign before parentheses", ""){
	ut_verify_global_result_nolib(QUARK_POS, "let int result = -4*(2+1)", make_expected_int(-12));
}


//////////////////////////////////////////		BASIC EXPRESSIONS - SPACES


FLOYD_LANG_PROOF("Floyd test suite", "execute_expression()", "Spaces", ""){
	ut_verify_global_result_nolib(QUARK_POS, "let int result = 5 * ((1 + 3) * 2 + 1)", make_expected_int(45));
}
FLOYD_LANG_PROOF("Floyd test suite", "execute_expression()", "Spaces", ""){
	ut_verify_global_result_nolib(QUARK_POS, "let int result = 5 - 2 * ( 3 )", make_expected_int(-1));
}
FLOYD_LANG_PROOF("Floyd test suite", "execute_expression()", "Spaces", ""){
	ut_verify_global_result_nolib(QUARK_POS, "let int result =  5 - 2 * ( ( 4 )  - 1 )", make_expected_int(-1));
}


//////////////////////////////////////////		BASIC EXPRESSIONS - DOUBLE


FLOYD_LANG_PROOF("Floyd test suite", "execute_expression()", "Fractional numbers", "") {
	ut_verify_global_result_nolib(QUARK_POS, "let double result = 2.8/2.0", make_expected_double(1.4));
}
FLOYD_LANG_PROOF("Floyd test suite", "execute_expression()", "Fractional numbers", ""){
//	ut_verify_global_result_nolib(QUARK_POS, "int result = 1/5e10") == 2e-11);
}
FLOYD_LANG_PROOF("Floyd test suite", "execute_expression()", "Fractional numbers", ""){
	ut_verify_global_result_nolib(QUARK_POS, "let double result = (4.0-3.0)/(4.0*4.0)", make_expected_double(0.0625));
}
FLOYD_LANG_PROOF("Floyd test suite", "execute_expression()", "Fractional numbers", ""){
	ut_verify_global_result_nolib(QUARK_POS, "let double result = 1.0/2.0/2.0", make_expected_double(0.25));
}
FLOYD_LANG_PROOF("Floyd test suite", "execute_expression()", "Fractional numbers", ""){
	ut_verify_global_result_nolib(QUARK_POS, "let double result = 0.25 * .5 * 0.5", make_expected_double(0.0625));
}
FLOYD_LANG_PROOF("Floyd test suite", "execute_expression()", "Fractional numbers", ""){
	ut_verify_global_result_nolib(QUARK_POS, "let double result = .25 / 2.0 * .5", make_expected_double(0.0625));
}

//////////////////////////////////////////		BASIC EXPRESSIONS - EDGE CASES


FLOYD_LANG_PROOF("Floyd test suite", "execute_expression()", "Repeated operators", ""){
	ut_verify_global_result_nolib(QUARK_POS, "let int result = 1+-2", make_expected_int(-1));
}
FLOYD_LANG_PROOF("Floyd test suite", "execute_expression()", "Repeated operators", ""){
	ut_verify_global_result_nolib(QUARK_POS, "let int result = --2", make_expected_int(2));
}
FLOYD_LANG_PROOF("Floyd test suite", "execute_expression()", "Repeated operators", ""){
	ut_verify_global_result_nolib(QUARK_POS, "let int result = 2---2", make_expected_int(0));
}
FLOYD_LANG_PROOF("Floyd test suite", "execute_expression()", "Repeated operators", ""){
	ut_verify_global_result_nolib(QUARK_POS, "let int result = 2-+-2", make_expected_int(4));
}

FLOYD_LANG_PROOF("Floyd test suite", "int", "Wrong number of arguments to int-constructor", "exception"){
	ut_verify_exception_nolib(
		QUARK_POS,
		R"(

			let a = int()

		)",
		"[Semantics] Construct value of primitive type requires exactly 1 argument. Line: 3 \"let a = int()\""
	);
}

FLOYD_LANG_PROOF("Floyd test suite", "Expression", "Error: mix types", "exception"){
	ut_verify_exception_nolib(
		QUARK_POS,
		R"(

			let a = 3 < "hello"

		)",
		"[Semantics] Expression type mismatch - cannot convert 'string' to 'int. Line: 3 \"let a = 3 < \"hello\"\""
	);
}

FLOYD_LANG_PROOF("Floyd test suite", "Expression", "Error: mix types", "exception"){
	ut_verify_exception_nolib(
		QUARK_POS,
		R"(

			let a = 3 * 3.2

		)",
		"[Semantics] Expression type mismatch - cannot convert 'double' to 'int. Line: 3 \"let a = 3 * 3.2\""
	);
}


//////////////////////////////////////////		BASIC EXPRESSIONS - BOOL


FLOYD_LANG_PROOF("Floyd test suite", "bool constructor", "", ""){
	ut_verify_global_result_nolib(QUARK_POS, "let bool result = true", make_expected_bool(true));
}
FLOYD_LANG_PROOF("Floyd test suite", "bool constructor", "", ""){
	ut_verify_global_result_nolib(QUARK_POS, "let bool result = false", make_expected_bool(false));
}

FLOYD_LANG_PROOF("Floyd test suite", "bool +", "", ""){
	ut_verify_global_result_nolib(
		QUARK_POS,
		R"(

			let result = false + true

		)",
		make_expected_bool(true)
	);
}
FLOYD_LANG_PROOF("Floyd test suite", "bool +", "", ""){
	ut_verify_global_result_nolib(QUARK_POS, R"(		let result = false + false		)", make_expected_bool(false));
}
FLOYD_LANG_PROOF("Floyd test suite", "bool +", "", ""){
	ut_verify_global_result_nolib(QUARK_POS, R"(		let result = true + true		)", make_expected_bool(true));
}





//////////////////////////////////////////		BASIC EXPRESSIONS - OPERATOR ==


FLOYD_LANG_PROOF("Floyd test suite", "execute_expression()", "==", ""){
	ut_verify_global_result_nolib(QUARK_POS, "let bool result = 1 == 1", make_expected_bool(true));
}
FLOYD_LANG_PROOF("Floyd test suite", "execute_expression()", "==", ""){
	ut_verify_global_result_nolib(QUARK_POS, "let bool result = 1 == 2", make_expected_bool(false));
}
FLOYD_LANG_PROOF("Floyd test suite", "execute_expression()", "==", ""){
	ut_verify_global_result_nolib(QUARK_POS, "let bool result = 1.3 == 1.3", make_expected_bool(true));
}
FLOYD_LANG_PROOF("Floyd test suite", "execute_expression()", "==", ""){
	ut_verify_global_result_nolib(QUARK_POS, "let bool result = \"hello\" == \"hello\"", make_expected_bool(true));
}
FLOYD_LANG_PROOF("Floyd test suite", "execute_expression()", "==", ""){
	ut_verify_global_result_nolib(QUARK_POS, "let bool result = \"hello\" == \"bye\"", make_expected_bool(false));
}


//////////////////////////////////////////		BASIC EXPRESSIONS - OPERATOR <


FLOYD_LANG_PROOF("Floyd test suite", "execute_expression()", "<", "") {
	ut_verify_global_result_nolib(QUARK_POS, "let bool result = 1 < 2", make_expected_bool(true));
}
FLOYD_LANG_PROOF("Floyd test suite", "execute_expression()", "<", ""){
	ut_verify_global_result_nolib(QUARK_POS, "let bool result = 5 < 2", make_expected_bool(false));
}
FLOYD_LANG_PROOF("Floyd test suite", "execute_expression()", "<", ""){
	ut_verify_global_result_nolib(QUARK_POS, "let bool result = 0.3 < 0.4", make_expected_bool(true));
}
FLOYD_LANG_PROOF("Floyd test suite", "execute_expression()", "<", ""){
	ut_verify_global_result_nolib(QUARK_POS, "let bool result = 1.5 < 0.4", make_expected_bool(false));
}
FLOYD_LANG_PROOF("Floyd test suite", "execute_expression()", "<", ""){
	ut_verify_global_result_nolib(QUARK_POS, "let bool result = \"adwark\" < \"boat\"", make_expected_bool(true));
}
FLOYD_LANG_PROOF("Floyd test suite", "execute_expression()", "<", ""){
	ut_verify_global_result_nolib(QUARK_POS, "let bool result = \"boat\" < \"adwark\"", make_expected_bool(false));
}


//////////////////////////////////////////		BASIC EXPRESSIONS - OPERATOR &&

FLOYD_LANG_PROOF("Floyd test suite", "execute_expression()", "&&", "") {
	ut_verify_global_result_nolib(QUARK_POS, "let bool result = false && false", make_expected_bool(false));
}
FLOYD_LANG_PROOF("Floyd test suite", "execute_expression()", "&&", ""){
	ut_verify_global_result_nolib(QUARK_POS, "let bool result = false && true", make_expected_bool(false));
}
FLOYD_LANG_PROOF("Floyd test suite", "execute_expression()", "&&", ""){
	ut_verify_global_result_nolib(QUARK_POS, "let bool result = true && false", make_expected_bool(false));
}
FLOYD_LANG_PROOF("Floyd test suite", "execute_expression()", "&&", ""){
	ut_verify_global_result_nolib(QUARK_POS, "let bool result = true && true", make_expected_bool(true));
}

FLOYD_LANG_PROOF("Floyd test suite", "execute_expression()", "&&", ""){
	ut_verify_global_result_nolib(QUARK_POS, "let bool result = false && false && false", make_expected_bool(false));
}
FLOYD_LANG_PROOF("Floyd test suite", "execute_expression()", "&&", ""){
	ut_verify_global_result_nolib(QUARK_POS, "let bool result = false && false && true", make_expected_bool(false));
}
FLOYD_LANG_PROOF("Floyd test suite", "execute_expression()", "&&", ""){
	ut_verify_global_result_nolib(QUARK_POS, "let bool result = false && true && false", make_expected_bool(false));
}
FLOYD_LANG_PROOF("Floyd test suite", "execute_expression()", "&&", ""){
	ut_verify_global_result_nolib(QUARK_POS, "let bool result = false && true && true", make_expected_bool(false));
}
FLOYD_LANG_PROOF("Floyd test suite", "execute_expression()", "&&", ""){
	ut_verify_global_result_nolib(QUARK_POS, "let bool result = true && false && false", make_expected_bool(false));
}
FLOYD_LANG_PROOF("Floyd test suite", "execute_expression()", "&&", ""){
	ut_verify_global_result_nolib(QUARK_POS, "let bool result = true && true && false", make_expected_bool(false));
}
FLOYD_LANG_PROOF("Floyd test suite", "execute_expression()", "&&", ""){
	ut_verify_global_result_nolib(QUARK_POS, "let bool result = true && true && true", make_expected_bool(true));
}

//////////////////////////////////////////		BASIC EXPRESSIONS - OPERATOR ||

FLOYD_LANG_PROOF("Floyd test suite", "execute_expression()", "||", ""){
	ut_verify_global_result_nolib(QUARK_POS, "let bool result = false || false", make_expected_bool(false));
}
FLOYD_LANG_PROOF("Floyd test suite", "execute_expression()", "||", ""){
	ut_verify_global_result_nolib(QUARK_POS, "let bool result = false || true", make_expected_bool(true));
}
FLOYD_LANG_PROOF("Floyd test suite", "execute_expression()", "||", ""){
	ut_verify_global_result_nolib(QUARK_POS, "let bool result = true || false", make_expected_bool(true));
}
FLOYD_LANG_PROOF("Floyd test suite", "execute_expression()", "||", ""){
	ut_verify_global_result_nolib(QUARK_POS, "let bool result = true || true", make_expected_bool(true));
}


FLOYD_LANG_PROOF("Floyd test suite", "execute_expression()", "||", ""){
	ut_verify_global_result_nolib(QUARK_POS, "let bool result = false || false || false", make_expected_bool(false));
}
FLOYD_LANG_PROOF("Floyd test suite", "execute_expression()", "||", ""){
	ut_verify_global_result_nolib(QUARK_POS, "let bool result = false || false || true", make_expected_bool(true));
}
FLOYD_LANG_PROOF("Floyd test suite", "execute_expression()", "||", ""){
	ut_verify_global_result_nolib(QUARK_POS, "let bool result = false || true || false", make_expected_bool(true));
}
FLOYD_LANG_PROOF("Floyd test suite", "execute_expression()", "||", ""){
	ut_verify_global_result_nolib(QUARK_POS, "let bool result = false || true || true", make_expected_bool(true));
}
FLOYD_LANG_PROOF("Floyd test suite", "execute_expression()", "||", ""){
	ut_verify_global_result_nolib(QUARK_POS, "let bool result = true || false || false", make_expected_bool(true));
}
FLOYD_LANG_PROOF("Floyd test suite", "execute_expression()", "||", ""){
	ut_verify_global_result_nolib(QUARK_POS, "let bool result = true || false || true", make_expected_bool(true));
}
FLOYD_LANG_PROOF("Floyd test suite", "execute_expression()", "||", ""){
	ut_verify_global_result_nolib(QUARK_POS, "let bool result = true || true || false", make_expected_bool(true));
}
FLOYD_LANG_PROOF("Floyd test suite", "execute_expression()", "||", ""){
	ut_verify_global_result_nolib(QUARK_POS, "let bool result = true || true || true", make_expected_bool(true));
}





//////////////////////////////////////////		BASIC EXPRESSIONS - INTEGER RANGE

/*
	int64_max		9223372036854775807		(2 ^ 63 - 1)	0b01111111'11111111'11111111'11111111'11111111'11111111'11111111'11111111	0x7fffffff'ffffffff
	int64_min		-9223372036854775808	(2 ^ 63)		0b10000000'00000000'00000000'00000000'00000000'00000000'00000000'00000000	0x80000000'00000000
	int64_minus1	18446744073709551615	(2 ^ 64 - 1)	0b11111111'11111111'11111111'11111111'11111111'11111111'11111111'11111111	0xffffffff'ffffffff

	uint64_max		18446744073709551615	(2 ^ 64 - 1)	0b11111111'11111111'11111111'11111111'11111111'11111111'11111111'11111111	0xffffffff'ffffffff

const uint64_t k_floyd_int64_max =	0x7fffffff'ffffffff;
const uint64_t k_floyd_int64_min =	0x80000000'00000000;
const uint64_t k_floyd_uint64_max =	0xffffffff'ffffffff;

*/

FLOYD_LANG_PROOF("Floyd test suite", "int range", "", ""){
	ut_verify_global_result_nolib(QUARK_POS, "let int result = 9223372036854775807", make_expected_int(0b01111111'11111111'11111111'11111111'11111111'11111111'11111111'11111111));
}
FLOYD_LANG_PROOF("Floyd test suite", "int range", "", ""){
	ut_verify_global_result_nolib(QUARK_POS, "let int result = -9223372036854775808", make_expected_int(0b10000000'00000000'00000000'00000000'00000000'00000000'00000000'00000000));
}


FLOYD_LANG_PROOF("Floyd test suite", "int range", "Init with unsigned literal", ""){
	ut_verify_global_result_nolib(QUARK_POS, "let int result = 18446744073709551615", make_expected_int(0b11111111'11111111'11111111'11111111'11111111'11111111'11111111'11111111));
}

FLOYD_LANG_PROOF("Floyd test suite", "int range", "Init with unsigned literal", ""){
	ut_verify_exception_nolib(
		QUARK_POS,
		"let int result = 618446744073709551615",
		R"___([Syntax] Integer literal "618446744073709551615" larger than maxium allowed, which is 18446744073709551615 aka 0x7fffffff'ffffffff - maxium for an unsigned 64-bit integer. Line: 1 "let int result = 618446744073709551615")___"
	);
}

//////////////////////////////////////////		BASIC EXPRESSIONS - BITWISE OPERATORS



FLOYD_LANG_PROOF("Floyd test suite", "", "bw_not()", ""){
	ut_verify_global_result_nolib(QUARK_POS, "let result = bw_not(0b00000000'00000000'00000000'00000000'00000000'00000000'00000000'00000000)", make_expected_int(0b11111111'11111111'11111111'11111111'11111111'11111111'11111111'11111111));
}

FLOYD_LANG_PROOF("Floyd test suite", "", "bw_not()", ""){
	ut_verify_global_result_nolib(QUARK_POS, "let result = bw_not(0b11111111'11111111'11111111'11111111'11111111'11111111'11111111'11111111)", make_expected_int(0b00000000'00000000'00000000'00000000'00000000'00000000'00000000'00000000));
}

FLOYD_LANG_PROOF("Floyd test suite", "", "bw_not()", ""){
	ut_verify_global_result_nolib(QUARK_POS, "let result = bw_not(0b00000000'00000000'00000000'00000000'00000000'00000000'00000000'00000001)", make_expected_int(0b11111111'11111111'11111111'11111111'11111111'11111111'11111111'11111110));
}
FLOYD_LANG_PROOF("Floyd test suite", "", "bw_not()", ""){
	ut_verify_global_result_nolib(QUARK_POS, "let result = bw_not(0b11111111'11111111'11111111'11111111'11111111'11111111'11111111'11111111)", make_expected_int(0));
}




FLOYD_LANG_PROOF("Floyd test suite", "", "bw_and()", ""){
	ut_verify_global_result_nolib(QUARK_POS, "let result = bw_and(0b00000000'00000000'00000000'00000000'00000000'00000000'00000000'00000011, 0b00000000'00000000'00000000'00000000'00000000'00000000'00000000'00000001)", make_expected_int(0b00000000'00000000'00000000'00000000'00000000'00000000'00000000'00000001));
}



FLOYD_LANG_PROOF("Floyd test suite", "", "bw_or()", ""){
	ut_verify_global_result_nolib(QUARK_POS, "let result = bw_or(0b00000000'00000000'00000000'00000000'00000000'00000000'00000000'00000010, 0b00000000'00000000'00000000'00000000'00000000'00000000'00000000'00000001)", make_expected_int(0b00000000'00000000'00000000'00000000'00000000'00000000'00000000'00000011));
}



FLOYD_LANG_PROOF("Floyd test suite", "", "bw_xor()", ""){
	ut_verify_global_result_nolib(QUARK_POS, "let result = bw_xor(0b00000000'00000000'00000000'00000000'00000000'00000000'00000000'00000011, 0b00000000'00000000'00000000'00000000'00000000'00000000'00000000'00000001)", make_expected_int(0b00000000'00000000'00000000'00000000'00000000'00000000'00000000'00000010));
}

FLOYD_LANG_PROOF("Floyd test suite", "", "bw_shift_left()", ""){
	ut_verify_global_result_nolib(QUARK_POS, "let result = bw_shift_left(0b00000000'00000000'00000000'00000000'00000000'00000000'00000000'00000011, 0b00000000'00000000'00000000'00000000'00000000'00000000'00000000'00000001)", make_expected_int(0b00000000'00000000'00000000'00000000'00000000'00000000'00000000'00000110));
}

FLOYD_LANG_PROOF("Floyd test suite", "", "bw_shift_right()", ""){
	ut_verify_global_result_nolib(QUARK_POS, "let result = bw_shift_right(0b00000000'00000000'00000000'00000000'00000000'00000000'00000000'00000011, 0b00000000'00000000'00000000'00000000'00000000'00000000'00000000'00000001)", make_expected_int(0b00000000'00000000'00000000'00000000'00000000'00000000'00000000'00000001));
}



FLOYD_LANG_PROOF("Floyd test suite", "", "bw_shift_right_arithmetic()", ""){
	ut_verify_global_result_nolib(QUARK_POS, "let result = bw_shift_right_arithmetic(0b00000000'00000000'00000000'00000000'00000000'00000000'00000000'00000111, 1)", make_expected_int(0b00000000'00000000'00000000'00000000'00000000'00000000'00000000'00000011));
}
FLOYD_LANG_PROOF("Floyd test suite", "", "bw_shift_right_arithmetic()", ""){
	ut_verify_global_result_nolib(QUARK_POS, "let result = bw_shift_right_arithmetic(0b11111111'11111111'11111111'11111111'11111111'11111111'11111111'11111110, 1)", make_expected_int(0b11111111'11111111'11111111'11111111'11111111'11111111'11111111'11111111));
}


//??? more



//////////////////////////////////////////		BASIC EXPRESSIONS - ERRORS



FLOYD_LANG_PROOF("Floyd test suite", "execute_expression()", "Type mismatch", "") {
	ut_verify_exception_nolib(
		QUARK_POS,
		"let int result = true",
		"[Semantics] Expression type mismatch - cannot convert 'bool' to 'int. Line: 1 \"let int result = true\""
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
		"[Semantics] Unary minus don't work on expressions of type \"bool\", only int and double. Line: 1 \"let int result = -true\""
	);
}

FLOYD_LANG_PROOF("Floyd test suite", "Forgot let or mutable", "", "Exception"){
	ut_verify_exception_nolib(QUARK_POS, "int test = 123", "[Syntax] Use 'mutable' or 'let' syntax. Line: 1 \"int test = 123\"");
}

FLOYD_LANG_PROOF("Floyd test suite", "Access variable", "Access Undefined identifier", "exception"){
	ut_verify_exception_nolib(QUARK_POS, R"(		print(a)		)", "[Semantics] Undefined identifier \"a\". Line: 1 \"print(a)\"");
}



//////////////////////////////////////////		TEST CONSTRUCTOR - SIMPLE TYPES


FLOYD_LANG_PROOF("Floyd test suite", "Construct value", "bool()", ""){
	ut_verify_global_result_nolib(QUARK_POS, "let result = bool(false)", make_expected_bool(false));
}
FLOYD_LANG_PROOF("Floyd test suite", "Construct value", "bool()", ""){
	ut_verify_global_result_nolib(QUARK_POS, "let result = bool(true)", make_expected_bool(true));
}
FLOYD_LANG_PROOF("Floyd test suite", "Construct value", "int()", ""){
	ut_verify_global_result_nolib(QUARK_POS, "let result = int(123)", make_expected_int(123));
}
FLOYD_LANG_PROOF("Floyd test suite", "Construct value", "double()", ""){
	ut_verify_global_result_nolib(QUARK_POS, "let result = double(0.0)", make_expected_double(0.0));
}
FLOYD_LANG_PROOF("Floyd test suite", "Construct value", "double()", ""){
	ut_verify_global_result_nolib(QUARK_POS, "let result = double(123.456)", make_expected_double(123.456));
}

FLOYD_LANG_PROOF("Floyd test suite", "Construct value", "string()", ""){
	ut_verify_exception_nolib(QUARK_POS, "let result = string()", "[Semantics] Construct value of primitive type requires exactly 1 argument. Line: 1 \"let result = string()\"");
}

FLOYD_LANG_PROOF("Floyd test suite", "Construct value", "string()", ""){
	ut_verify_global_result_nolib(QUARK_POS, "let result = string(\"ABCD\")", make_expected_string("ABCD"));
}



//??? add test for irregular dividers '


//////////////////////////////////////////		TEST CHARACTER LITERALS


FLOYD_LANG_PROOF("Floyd test suite", "Character literal", "", ""){
	ut_verify_global_result_nolib(QUARK_POS, "let result = 'A'", make_expected_int(65));
}

FLOYD_LANG_PROOF("Floyd test suite", "Character literal", "Escape \0", ""){
	ut_verify_global_result_nolib(QUARK_POS, R"___(let result = '\0')___", make_expected_int('\0'));
}
FLOYD_LANG_PROOF("Floyd test suite", "Character literal", "Escape \t", ""){
	ut_verify_global_result_nolib(QUARK_POS, R"___(let result = '\t')___", make_expected_int('\t'));
}
FLOYD_LANG_PROOF("Floyd test suite", "Character literal", "Escape \\", ""){
	ut_verify_global_result_nolib(QUARK_POS, R"___(let result = '\\')___", make_expected_int('\\'));
}
FLOYD_LANG_PROOF("Floyd test suite", "Character literal", "Escape \n", ""){
	ut_verify_global_result_nolib(QUARK_POS, R"___(let result = '\n')___", make_expected_int('\n'));
}
FLOYD_LANG_PROOF("Floyd test suite", "Character literal", "Escape \r", ""){
	ut_verify_global_result_nolib(QUARK_POS, R"___(let result = '\r')___", make_expected_int('\r'));
}
FLOYD_LANG_PROOF("Floyd test suite", "Character literal", "Escape \"", ""){
	ut_verify_global_result_nolib(QUARK_POS, R"___(let result = '\"')___", make_expected_int('\"'));
}
FLOYD_LANG_PROOF("Floyd test suite", "Character literal", "Escape \'", ""){
	ut_verify_global_result_nolib(QUARK_POS, R"___(let result = '\'')___", make_expected_int('\''));
}
FLOYD_LANG_PROOF("Floyd test suite", "Character literal", "Escape \'", ""){
	ut_verify_global_result_nolib(QUARK_POS, R"___(let result = '\/')___", make_expected_int('/'));
}



//////////////////////////////////////////		TEST BINARY LITERALS


FLOYD_LANG_PROOF("Floyd test suite", "Binary literal", "", ""){
	ut_verify_global_result_nolib(QUARK_POS, "let result = 0b0", make_expected_int(0b0));
}
FLOYD_LANG_PROOF("Floyd test suite", "Binary literal", "", ""){
	ut_verify_global_result_nolib(QUARK_POS, "let result = 0b10000000", make_expected_int(0b10000000));
}
FLOYD_LANG_PROOF("Floyd test suite", "Binary literal", "", ""){
	ut_verify_global_result_nolib(QUARK_POS, "let result = 0b1000000000000000000000000000000000000000000000000000000000000001", make_expected_int(0b1000000000000000000000000000000000000000000000000000000000000001));
}
FLOYD_LANG_PROOF("Floyd test suite", "Binary literal", "", ""){
	ut_verify_global_result_nolib(QUARK_POS, "let result = 0b10000000'00000000'00000000'00000000'00000000'00000000'00000000'00000001", make_expected_int(0b10000000'00000000'00000000'00000000'00000000'00000000'00000000'00000001));
}


//////////////////////////////////////////		TEST HEXADECIMAL LITERALS


FLOYD_LANG_PROOF("Floyd test suite", "Hexadecimal literal", "", ""){
	ut_verify_global_result_nolib(QUARK_POS, "let result = 0x0", make_expected_int(0x0));
}
FLOYD_LANG_PROOF("Floyd test suite", "Hexadecimal literal", "", ""){
	ut_verify_global_result_nolib(QUARK_POS, "let result = 0xff", make_expected_int(0xff));
}
FLOYD_LANG_PROOF("Floyd test suite", "Hexadecimal literal", "", ""){
	ut_verify_global_result_nolib(QUARK_POS, "let result = 0xabcd", make_expected_int(0xabcd));
}
FLOYD_LANG_PROOF("Floyd test suite", "Hexadecimal literal", "", ""){
	ut_verify_global_result_nolib(QUARK_POS, "let result = 0xabcdef01'23456789", make_expected_int(0xabcdef01'23456789));
}
FLOYD_LANG_PROOF("Floyd test suite", "Hexadecimal literal", "", ""){
	ut_verify_global_result_nolib(QUARK_POS, "let result = 0xffffffff'ffffffff", make_expected_int(0xffffffff'ffffffff));
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
		"[Semantics] Cannot assign to immutable identifier \"a\". Line: 4 \"a = 4\""
	);
}


FLOYD_LANG_PROOF("Floyd test suite", "", "String (which requires RC)", ""){
	ut_run_closed_nolib(
		QUARK_POS,
		R"___(

			func void f(){
				let s = "A"
			}

			f()

		)___"
	);
}
FLOYD_LANG_PROOF("Floyd test suite", "Mutate", "String (which requires RC)", ""){
	ut_run_closed_nolib(
		QUARK_POS,
		R"___(

		func string f(){
			mutable s = "A"
			return s
		}

		f()

		)___"
	);
}
FLOYD_LANG_PROOF("Floyd test suite", "Mutate", "String (which requires RC)", ""){
	ut_run_closed_nolib(
		QUARK_POS,
		R"___(

			func string f(){
				mutable s = "A"
				s = "<" + s + ">"
				return s
			}

			let r = f()
			assert(r == "<A>")

		)___"
	);
}




//######################################################################################################################
//	INTRINSICS
//######################################################################################################################

//////////////////////////////////////////		INTRINSIC - print()



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


//////////////////////////////////////////		INTRINSIC - assert()


FLOYD_LANG_PROOF("Floyd test suite", "", "", ""){
	ut_verify_exception_nolib(QUARK_POS, R"(		assert(1 == 2)		)", "Assertion failed.");
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

//////////////////////////////////////////		INTRINSIC - to_string()


FLOYD_LANG_PROOF("Floyd test suite", "", "", ""){
	ut_verify_global_result_nolib(QUARK_POS, R"(		let string result = to_string(145)		)", make_expected_string("145"));
}
FLOYD_LANG_PROOF("Floyd test suite", "", "", ""){
	ut_verify_global_result_nolib(QUARK_POS, R"(		let string result = to_string(3.1)		)", make_expected_string("3.1"));
}

FLOYD_LANG_PROOF("Floyd test suite", "", "", ""){
	ut_verify_global_result_nolib(QUARK_POS, R"(		let string result = to_string(3.0)		)", make_expected_string("3.0"));
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

//??? test many arguments!

FLOYD_LANG_PROOF("Floyd test suite", "func", "Simplest func", ""){
	ut_verify_global_result_nolib(
		QUARK_POS,
		R"(

			func int f(){
				return 1000;
			}
			let result = f()

		)",
		make_expected_int(1000)
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
		make_expected_int(1001)
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
		make_expected_string("-xyz-")
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
		"[Semantics] Cannot assign to immutable identifier \"x\". Line: 4 \"x = 6\""
	);
}

FLOYD_LANG_PROOF("Floyd test suite", "func", "Recursion: function calling itself by name", ""){
	ut_run_closed_nolib(QUARK_POS, R"(		func int fx(int a){ return fx(a + 1) }		)");
}

FLOYD_LANG_PROOF("Floyd test suite", "func", "return from void function", "error"){
	ut_verify_exception_nolib(
		QUARK_POS,
		R"(

			func void f (int x) impure {
				return 3
			}

		)",
		R"xxx([Semantics] Cannot return value from function with void-return. Line: 4 "return 3")xxx"
	);
}

FLOYD_LANG_PROOF("Floyd test suite", "func", "void function, no return statement", ""){
	ut_run_closed_nolib(
		QUARK_POS,
		R"(

			func void f (int x) impure {
			}

		)"
	);
}

FLOYD_LANG_PROOF_VIP("Floyd test suite", "Function value", "", ""){
	ut_verify_printout_nolib(
		QUARK_POS,
		R"(

			func int a(string s){
				return 2
			}
			print(a)

		)",
		{
			"floydf_a"
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
		{ "f" }
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




FLOYD_LANG_PROOF("Floyd test suite", "Local function", "", ""){
	ut_run_closed_nolib(
		QUARK_POS,
		R"(

			for(i in 0 ..< 2){
				func bool f(int def, int wanted_name){
					return false
				}

			}

		)"
	);
}

FLOYD_LANG_PROOF("Floyd test suite", "Local function", "", ""){
	ut_verify_printout_nolib(
		QUARK_POS,
		R"(

			func string f(int error){
				func string f2(int error){
					return error == 1 ? "ONE" : "NOT-ONE";
				}
				return "<" + f2(error) + ">"
			}

			print(f(1))
			print(f(2))
		)",
		{ "<ONE>", "<NOT-ONE>" }
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
		"[Semantics] Wrong number of arguments in function call, got 2, expected 1. Line: 4 \"let a = f(1, 2)\""
	);
}

FLOYD_LANG_PROOF("Floyd test suite", "Call", "Call non-function, non-struct, non-typeid", "exception"){
	ut_verify_exception_nolib(QUARK_POS, R"(		let a = 3()		)", "[Semantics] Cannot call non-function, its type is int. Line: 1 \"let a = 3()\"");
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
	ut_verify_exception_nolib(
		QUARK_POS,
		R"(

			func int f(){
				return "x"
			}

		)",
		"[Semantics] Expression type mismatch - cannot convert 'string' to 'int. Line: 4 \"return \"x\"\""
	);
}


FLOYD_LANG_PROOF("Floyd test suite", "return", "You can't return from global scope", "error"){
	ut_verify_exception_nolib(
		QUARK_POS,
		R"(

			return "hello"

		)",
		R"___([Semantics] Cannot return value from function with void-return. Line: 3 "return "hello"")___"
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
	ut_run_closed_nolib(
		QUARK_POS,
		R"(

			func int a(int p){ return p + 1 }
			func int b(){ return a(100) }

		)"
	);
}
FLOYD_LANG_PROOF("Floyd test suite", "impure", "call impure->pure", "Compiles OK"){
	ut_run_closed_nolib(
		QUARK_POS,
		R"(

			func int a(int p){ return p + 1 }
			func int b() impure { return a(100) }

		)"
	);
}

FLOYD_LANG_PROOF("Floyd test suite", "impure", "call pure->impure", "Compilation error"){
	ut_verify_exception_nolib(
		QUARK_POS,
		R"(

			func int a(int p) impure { return p + 1 }
			func int b(){ return a(100) }

		)",
		"[Semantics] Cannot call impure function from a pure function. Line: 4 \"func int b(){ return a(100) }\""
	);
}

FLOYD_LANG_PROOF("Floyd test suite", "impure", "call impure->impure", "Compiles OK"){
	ut_run_closed_nolib(
		QUARK_POS,
		R"(

			func int a(int p) impure { return p + 1 }
			func int b() impure { return a(100) }

		)"
	);
}



//######################################################################################################################
//	TEST-DEF STATEMENT
//######################################################################################################################




//??? make sure only pure functions can be called inside test-def.


FLOYD_LANG_PROOF("Floyd test suite", "test-def", "Simple test-def compiles, call to test()", ""){
	ut_run_closed_nolib(
		QUARK_POS,
		R"___(

			test-def ("Test 404", "print a message"){ print("Running test 404!") }

		)___"
	);
}

FLOYD_LANG_PROOF("Floyd test suite", "test-def", "Simple test-def compiles, call to test()", ""){
	ut_run_closed_nolib(
		QUARK_POS,
		R"___(

			test-def ("Test 404", "print a message"){ print("Running test 404!") }
			test-def ("Test 1138", "thx"){ print("Watching THX!") }

			print(to_pretty_string(test_registry) + "\n")
		)___"
	);
}


FLOYD_LANG_PROOF("Floyd test suite", "test-def", "Simple test-def compiles", ""){
	ut_verify_exception_nolib(
		QUARK_POS,
		R"___(

			test-def ("subset()", "skip first character"){ assert( subset("abc", 1, 3) == "XXXbc") }

		)___",
		"All tests (1): 1 failed!\n"
		"|MODULE |FUNCTION |SCENARIO             |RESULT            |\n"
		"|-------|---------|---------------------|------------------|\n"
		"|       |subset() |skip first character |Assertion failed. |\n"
		"|-------|---------|---------------------|------------------|\n"
	);
}


#if 0
FLOYD_LANG_PROOF("Floyd test suite", "test-def", "Test running more than one simple benchmark_def", ""){
	ut_verify_printout_nolib(
		QUARK_POS,
		R"(

			test-def ("subset()", "skip first character"){ test( subset("abc", 1, 3), "bc" )}


			benchmark-def "AAA" {
				return [ benchmark_result_t(200, json("0 eleements")) ]
			}
			benchmark-def "BBB" {
				return [ benchmark_result_t(300, json("3 monkeys")) ]
			}

			for(i in 0 ..< size(benchmark_registry)){
				let e = benchmark_registry[i]
				let benchmark_result = e.f()
				for(v in 0 ..< size(benchmark_result)){
					print(e.name + ": " + to_string(v) + ": dur: " + to_string(benchmark_result[v].dur) + ", more: " + to_pretty_string(benchmark_result[v].more) )
				}
			}

		)",
		{
			R"___(AAA: 0: dur: 200, more: "0 eleements")___",
			R"___(BBB: 0: dur: 300, more: "3 monkeys")___"
		}
	);
}
#endif





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
	ut_run_closed_nolib(QUARK_POS, "{}");
}

FLOYD_LANG_PROOF("Floyd test suite", "Scopes: Block with local variable, no shadowing", "", ""){
	ut_run_closed_nolib(QUARK_POS, "{ let int x = 4 }");
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


#endif	//	RUN_LANG_BASIC_TESTS1


#if RUN_LANG_BASIC_TESTS2
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
		R"([Semantics] Boolean condition required. Line: 3 "if("not a bool"){ } else{ assert(false) }")"
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
//	QUARK_VERIFY(false);
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
	ut_run_closed_nolib(
		QUARK_POS,
		R"(

			for (i in 1 ... 0) {
			}

		)"
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
		R"abc([Semantics] For-loop requires integer iterator, start type is bool. Line: 3 "for (i in true ... false) {")abc"
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
		R"abc([Semantics] Expression type mismatch - cannot convert 'int' to 'bool. Line: 4 "while(105){")abc"
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





//######################################################################################################################
//	typeid
//######################################################################################################################




FLOYD_LANG_PROOF("Floyd test suite", "typeid", "", "exception"){
	ut_verify_exception_nolib(QUARK_POS, R"(		let int a = bool		)", "[Semantics] Expression type mismatch - cannot convert 'typeid' to 'int. Line: 1 \"let int a = bool\"");
}




//######################################################################################################################
//	STRING
//######################################################################################################################




FLOYD_LANG_PROOF("Floyd test suite", "string constructor", "", ""){
	ut_verify_global_result_nolib(QUARK_POS, "let result = string(\"ABCD\")", make_expected_string("ABCD"));
}

FLOYD_LANG_PROOF("Floyd test suite", "string constructor", "", ""){
	ut_verify_global_result_nolib(QUARK_POS, "let result = \"ABCD\"", make_expected_string("ABCD"));
}

FLOYD_LANG_PROOF("Floyd test suite", "string literal", "Escape \0", ""){
	ut_verify_global_result_nolib(QUARK_POS, R"___(let result = "\0")___", make_expected_string(std::string(1, '\0')));
}
FLOYD_LANG_PROOF("Floyd test suite", "string literal", "Escape \t", ""){
	ut_verify_global_result_nolib(QUARK_POS, R"___(let result = "\t")___", make_expected_string("\t"));
}
FLOYD_LANG_PROOF("Floyd test suite", "string literal", "Escape \\", ""){
	ut_verify_global_result_nolib(QUARK_POS, R"___(let result = "\\")___", make_expected_string("\\"));
}
FLOYD_LANG_PROOF("Floyd test suite", "string literal", "Escape \n", ""){
	ut_verify_global_result_nolib(QUARK_POS, R"___(let result = "\n")___", make_expected_string("\n"));
}
FLOYD_LANG_PROOF("Floyd test suite", "string literal", "Escape \r", ""){
	ut_verify_global_result_nolib(QUARK_POS, R"___(let result = "\r")___", make_expected_string("\r"));
}
FLOYD_LANG_PROOF("Floyd test suite", "string literal", "Escape \"", ""){
	ut_verify_global_result_nolib(QUARK_POS, R"___(let result = "\"")___", make_expected_string("\""));
}
FLOYD_LANG_PROOF("Floyd test suite", "string literal", "Escape \'", ""){
	ut_verify_global_result_nolib(QUARK_POS, R"___(let result = "\'")___", make_expected_string("\'"));
}
FLOYD_LANG_PROOF("Floyd test suite", "string literal", "Escape \'", ""){
	ut_verify_global_result_nolib(QUARK_POS, R"___(let result = "\/")___", make_expected_string("/"));
}

FLOYD_LANG_PROOF("Floyd test suite", "string =", "", ""){
	ut_run_closed_nolib(
		QUARK_POS,
		R"(

			let a = "hello"
			let b = a
			assert(b == "hello")

		)"
	);
}

FLOYD_LANG_PROOF("Floyd test suite", "string +", "", ""){
	ut_run_closed_nolib(QUARK_POS, R"(		assert("a" + "b" == "ab")		)");
}
FLOYD_LANG_PROOF("Floyd test suite", "string +", "", ""){
	ut_run_closed_nolib(QUARK_POS, R"(		assert("a" + "b" + "c" == "abc")		)");
}



FLOYD_LANG_PROOF("Floyd test suite", "string ==", "", ""){
	ut_run_closed_nolib(QUARK_POS, R"(		assert(("hello" == "hello") == true)		)");
}
FLOYD_LANG_PROOF("Floyd test suite", "string ==", "", ""){
	ut_run_closed_nolib(QUARK_POS, R"(		assert(("hello" == "Yello") == false)		)");
}
FLOYD_LANG_PROOF("Floyd test suite", "string !=", "", ""){
	ut_run_closed_nolib(QUARK_POS, R"(		assert(("hello" != "yello") == true)		)");
}
FLOYD_LANG_PROOF("Floyd test suite", "string !=", "", ""){
	ut_run_closed_nolib(QUARK_POS, R"(		assert(("hello" != "hello") == false)		)");
}


FLOYD_LANG_PROOF("Floyd test suite", "string <", "", ""){
	ut_run_closed_nolib(QUARK_POS, R"(		assert(("aaa" < "bbb") == true)		)");
}
FLOYD_LANG_PROOF("Floyd test suite", "string <", "", ""){
	ut_run_closed_nolib(QUARK_POS, R"(		assert(("aaa" < "aaa") == false)		)");
}
FLOYD_LANG_PROOF("Floyd test suite", "string <", "", ""){
	ut_run_closed_nolib(QUARK_POS, R"(		assert(("bbb" < "aaa") == false)		)");
}


FLOYD_LANG_PROOF("Floyd test suite", "string <=", "", ""){
	ut_run_closed_nolib(QUARK_POS, R"(		assert(("aaa" <= "bbb") == true)		)");
}
FLOYD_LANG_PROOF("Floyd test suite", "string <=", "", ""){
	ut_run_closed_nolib(QUARK_POS, R"(		assert(("aaa" <= "aaa") == true)		)");
}
FLOYD_LANG_PROOF("Floyd test suite", "string <=", "", ""){
	ut_run_closed_nolib(QUARK_POS, R"(		assert(("bbb" <= "aaa") == false)		)");
}


FLOYD_LANG_PROOF("Floyd test suite", "string >", "", ""){
	ut_run_closed_nolib(QUARK_POS, R"(		assert(("bbb" > "aaa") == true)		)");
}
FLOYD_LANG_PROOF("Floyd test suite", "string >", "", ""){
	ut_run_closed_nolib(QUARK_POS, R"(		assert(("aaa" > "aaa") == false)		)");
}
FLOYD_LANG_PROOF("Floyd test suite", "string >", "", ""){
	ut_run_closed_nolib(QUARK_POS, R"(		assert(("aaa" > "bbb") == false)		)");
}


FLOYD_LANG_PROOF("Floyd test suite", "string >=", "", ""){
	ut_run_closed_nolib(QUARK_POS, R"(		assert(("bbb" >= "aaa") == true)		)");
}
FLOYD_LANG_PROOF("Floyd test suite", "string >=", "", ""){
	ut_run_closed_nolib(QUARK_POS, R"(		assert(("aaa" >= "aaa") == true)		)");
}
FLOYD_LANG_PROOF("Floyd test suite", "string >=", "", ""){
	ut_run_closed_nolib(QUARK_POS, R"(		assert(("aaa" >= "bbb") == false)		)");
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
	ut_run_closed_nolib(QUARK_POS, R"(		assert("hello"[0] == 104)		)");
}

FLOYD_LANG_PROOF("Floyd test suite", "string []", "", ""){
	ut_run_closed_nolib(QUARK_POS, R"(		assert("hello"[4] == 111)		)");
}

FLOYD_LANG_PROOF("Floyd test suite", "string", "Error: Lookup in string using non-int", "exception"){
	ut_verify_exception_nolib(
		QUARK_POS,
		R"(

			let string a = "test string"
			print(a["not an integer"])

		)",
		"[Semantics] Strings can only be indexed by integers, not a \"string\". Line: 4 \"print(a[\"not an integer\"])\""
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
	ut_run_closed_nolib(QUARK_POS, R"(

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
	ut_run_closed_nolib(QUARK_POS, R"(		assert(size("") == 0)		)");
}
FLOYD_LANG_PROOF("Floyd test suite", "string size()", "", ""){
	ut_run_closed_nolib(QUARK_POS, R"(		assert(size("How long is this string?") == 24)		)");
}

FLOYD_LANG_PROOF("Floyd test suite", "string size()", "Embeded null characters - check 8 bit clean", ""){
	ut_run_closed_nolib(QUARK_POS, R"(		assert(size("hello\0world\0\0") == 13)		)");
}



//??? find() should have a start index.
FLOYD_LANG_PROOF("Floyd test suite", "string find()", "", ""){
	ut_run_closed_nolib(QUARK_POS, R"(		assert(find("hello, world", "he") == 0)		)");
}
FLOYD_LANG_PROOF("Floyd test suite", "string find()", "", ""){
	ut_run_closed_nolib(QUARK_POS, R"(		assert(find("hello, world", "e") == 1)		)");
}
FLOYD_LANG_PROOF("Floyd test suite", "string find()", "", ""){
	ut_run_closed_nolib(QUARK_POS, R"(		assert(find("hello, world", "x") == -1)		)");
}



FLOYD_LANG_PROOF("Floyd test suite", "string push_back()", "", ""){
	ut_run_closed_nolib(QUARK_POS, R"(

		let a = push_back("one", 111)
		assert(a == "oneo")

	)");
}



FLOYD_LANG_PROOF("Floyd test suite", "string subset()", "string", ""){
	ut_run_closed_nolib(QUARK_POS, R"(		assert(subset("abc", 0, 3) == "abc")		)");
}
FLOYD_LANG_PROOF("Floyd test suite", "string subset()", "string", ""){
	ut_run_closed_nolib(QUARK_POS, R"(		assert(subset("abc", 1, 3) == "bc")		)");
}
FLOYD_LANG_PROOF("Floyd test suite", "string subset()", "string", ""){
	ut_run_closed_nolib(QUARK_POS, R"(		assert(subset("abc", 0, 0) == "")		)");
}
FLOYD_LANG_PROOF("Floyd test suite", "string subset()", "string", ""){
	ut_run_closed_nolib(QUARK_POS, R"(		assert(subset("abc", 0, 10) == "abc")		)");
}
FLOYD_LANG_PROOF("Floyd test suite", "string subset()", "string", ""){
	ut_run_closed_nolib(QUARK_POS, R"(		assert(subset("abc", 2, 10) == "c")		)");
}




FLOYD_LANG_PROOF("Floyd test suite", "string replace()", "", ""){
	ut_run_closed_nolib(QUARK_POS, R"(		assert(replace("One ring to rule them all", 4, 8, "rabbit") == "One rabbit to rule them all")		)");
}
FLOYD_LANG_PROOF("Floyd test suite", "string replace()", "", ""){
	ut_run_closed_nolib(QUARK_POS, R"(		assert(replace("hello", 0, 5, "goodbye") == "goodbye")		)");
}
FLOYD_LANG_PROOF("Floyd test suite", "string replace()", "", ""){
	ut_run_closed_nolib(QUARK_POS, R"(		assert(replace("hello", 0, 6, "goodbye") == "goodbye")		)");
}
FLOYD_LANG_PROOF("Floyd test suite", "string replace()", "", ""){
	ut_run_closed_nolib(QUARK_POS, R"(		assert(replace("hello", 0, 0, "goodbye") == "goodbyehello")		)");
}
FLOYD_LANG_PROOF("Floyd test suite", "string replace()", "", ""){
	ut_run_closed_nolib(QUARK_POS, R"(		assert(replace("hello", 5, 5, "goodbye") == "hellogoodbye")		)");
}
FLOYD_LANG_PROOF("Floyd test suite", "string replace()", "", "error"){
	ut_verify_exception_nolib(QUARK_POS, R"(		assert(replace("hello", 5, 0, "goodbye") == "hellogoodbye")		)", "replace() requires start <= end.");
}
FLOYD_LANG_PROOF("Floyd test suite", "string replace()", "", "error"){
	ut_verify_exception_nolib(QUARK_POS, R"(		assert(replace("hello", 2, 3, 666) == "")		)", R"___([Semantics] replace() requires argument 4 to be same type of collection. Line: 1 "assert(replace("hello", 2, 3, 666) == "")")___");
}





#endif	//	RUN_LANG_BASIC_TESTS2


#if RUN_LANG_VECTOR_TESTS

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
		"[Semantics] Cannot infer vector element type, add explicit type. Line: 3 \"let a = []\""
	);
}



//??? Use make_string_vec() helper and ut_verify_global_result_nolib()

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
	ut_verify_exception_nolib(QUARK_POS, R"(		assert(([] == []) == true)		)", "[Semantics] Cannot infer vector element type, add explicit type. Line: 1 \"assert(([] == []) == true)\"");
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
	ut_verify_exception_nolib(QUARK_POS, R"(		let [int] a = [] + [] result = a == []		)", R"([Semantics] Cannot infer vector element type, add explicit type. Line: 1 "let [int] a = [] + [] result = a == []")");
}

#if 0
//??? This fails but should not. This code becomes a constructor call to [int] with more than 16 arguments. Byte code interpreter has 16 argument limit.
FLOYD_LANG_PROOF("Floyd test suite", "vector [] - constructor", "32 elements initialization", ""){
	ut_run_closed_nolib(QUARK_POS, R"(

		let a = [ 0,0,1,1,0,0,0,0,	0,0,1,1,0,0,0,0,	0,0,1,1,1,0,0,0,	0,0,1,1,1,1,0,0 ]
		print(a)

	)");
}
#endif

FLOYD_LANG_PROOF("Floyd test suite", "vector [string] constructor expression, computed element", "", ""){
	ut_run_closed_nolib(QUARK_POS,
		R"(

			func string get_beta(){ return "beta" }
			let [string] a = ["alpha", get_beta()]
			assert(a == ["alpha", "beta"])
		)"
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
	ut_run_closed_nolib(QUARK_POS, R"(

		let [string] a = ["one"] + ["two"]
		assert(a == ["one", "two"])

	)");
}


FLOYD_LANG_PROOF("Floyd test suite", "vector [string] ==", "same values", ""){
	ut_run_closed_nolib(QUARK_POS, R"(		assert((["one", "two"] == ["one", "two"]) == true)		)");
}

FLOYD_LANG_PROOF("Floyd test suite", "vector [string] ==", "different values", ""){
	ut_run_closed_nolib(QUARK_POS, R"(		assert((["one", "three"] == ["one", "two"]) == false)		)");
}

FLOYD_LANG_PROOF("Floyd test suite", "vector [string] <", "same values", ""){
	ut_run_closed_nolib(QUARK_POS, R"(		assert((["one", "two"] < ["one", "two"]) == false)		)");
}

FLOYD_LANG_PROOF("Floyd test suite", "vector [string] <", "different values", ""){
	ut_run_closed_nolib(QUARK_POS, R"(		assert((["one", "a"] < ["one", "two"]) == true)		)");
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

FLOYD_LANG_PROOF("Floyd test suite", "vector [string] update()", "", "no assert"){
	ut_verify_printout_nolib(
		QUARK_POS,
		R"(

			let a = [ "one", "two", "three"]
			let b = update(a, 1, "zwei")
		)",
		{
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
	ut_run_closed_nolib(QUARK_POS, R"(

		let [string] a = []
		assert(size(a) == 0)

	)");
}
FLOYD_LANG_PROOF("Floyd test suite", "vector [string] size()", "2", ""){
	ut_run_closed_nolib(QUARK_POS, R"(

		let [string] a = ["one", "two"]
		assert(size(a) == 2)

	)");
}



FLOYD_LANG_PROOF("Floyd test suite", "vector [string] find()", "", ""){
	ut_run_closed_nolib(QUARK_POS, R"(		assert(find(["one", "two", "three", "four"], "one") == 0)		)");
}
FLOYD_LANG_PROOF("Floyd test suite", "vector [string] find()", "", ""){
	ut_run_closed_nolib(QUARK_POS, R"(		assert(find(["one", "two", "three", "four"], "two") == 1)		)");
}
FLOYD_LANG_PROOF("Floyd test suite", "vector [string] find()", "", ""){
	ut_run_closed_nolib(QUARK_POS, R"(		assert(find(["one", "two", "three", "four"], "five") == -1)		)");
}



FLOYD_LANG_PROOF("Floyd test suite", "vector [string] push_back()", "", ""){
	ut_run_closed_nolib(QUARK_POS, R"(

		let [string] a = ["one"];
		let [string] b = push_back(a, "two")
		let [string] c = ["one", "two"];
		assert(b == c)

	)");
}

FLOYD_LANG_PROOF("Floyd test suite", "vector [string] push_back()", "", ""){
	ut_run_closed_nolib(QUARK_POS, R"(

		let [string] a = push_back(["one"], "two")
		assert(a == ["one", "two"])

	)");
}

FLOYD_LANG_PROOF("Floyd test suite", "vector [string] subset()", "", ""){
	ut_run_closed_nolib(QUARK_POS, R"(		assert(subset(["one", "two", "three"], 0, 3) == ["one", "two", "three"])		)");
}
FLOYD_LANG_PROOF("Floyd test suite", "vector [string] subset()", "", ""){
	ut_run_closed_nolib(QUARK_POS, R"(		assert(subset(["one", "two", "three"], 1, 3) == ["two", "three"])		)");
}
FLOYD_LANG_PROOF("Floyd test suite", "vector [string] subset()", "", ""){
	ut_run_closed_nolib(QUARK_POS, R"(		assert(subset(["one", "two", "three"], 0, 0) == [])		)");
}
FLOYD_LANG_PROOF("Floyd test suite", "vector [string] subset()", "", ""){
	ut_run_closed_nolib(QUARK_POS, R"(		assert(subset(["one", "two", "three"], 0, 10) == ["one", "two", "three"])		)");
}
FLOYD_LANG_PROOF("Floyd test suite", "vector [string] subset()", "", ""){
	ut_run_closed_nolib(QUARK_POS, R"(		assert(subset(["one", "two", "three"], 2, 10) == ["three"])		)");
}




FLOYD_LANG_PROOF("Floyd test suite", "vector [string] replace()", "", ""){
	ut_run_closed_nolib(QUARK_POS, R"(		assert(replace(["one", "two", "three", "four", "five"], 0, 5, ["goodbye"]) == ["goodbye"])		)");
}
FLOYD_LANG_PROOF("Floyd test suite", "vector [string] replace()", "", ""){
	ut_run_closed_nolib(QUARK_POS, R"(		assert(replace(["one", "two", "three", "four", "five"], 0, 6, ["goodbye"]) == ["goodbye"])		)");
}
FLOYD_LANG_PROOF("Floyd test suite", "vector [string] replace()", "", ""){
	ut_run_closed_nolib(QUARK_POS, R"(		assert(replace(["one", "two", "three", "four", "five"], 0, 0, ["goodbye"]) == ["goodbye", "one", "two", "three", "four", "five"])		)");
}
FLOYD_LANG_PROOF("Floyd test suite", "vector [string] replace()", "", ""){
	ut_run_closed_nolib(QUARK_POS, R"(		assert(replace(["one", "two", "three", "four", "five"], 5, 5, ["goodbye"]) == ["one", "two", "three", "four", "five", "goodbye"])		)");
}
FLOYD_LANG_PROOF("Floyd test suite", "vector [string] replace()", "", "error"){
	ut_verify_exception_nolib(QUARK_POS, R"(		assert(replace(["one", "two", "three", "four", "five"], 5, 0, ["goodbye"]) == ["hellogoodbye"])		)", "replace() requires start <= end.");
}
FLOYD_LANG_PROOF("Floyd test suite", "vector [string] replace()", "", "error"){
	ut_verify_exception_nolib(QUARK_POS, R"(		assert(replace(["one", "two", "three", "four", "five"], 2, 3, 666) == [])		)", R"___([Semantics] replace() requires argument 4 to be same type of collection. Line: 1 "assert(replace(["one", "two", "three", "four", "five"], 2, 3, 666) == [])")___");
}




//??? use make_bool_vec()
//////////////////////////////////////////		vector-bool


FLOYD_LANG_PROOF("Floyd test suite", "vector [bool] construct-expression", "", ""){
	ut_run_closed_nolib(QUARK_POS, R"___(		let [bool] a = [true, false, true];		assert(a == [true, false, true]) )___");
}
FLOYD_LANG_PROOF("Floyd test suite", "vector [bool] =", "copy", ""){
	ut_run_closed_nolib(QUARK_POS, R"___(		let a = [true, false, true] let b = a; assert(b == [ true, false, true ])	)___");
}
FLOYD_LANG_PROOF("Floyd test suite", "vector [bool] ==", "same values", ""){
	ut_run_closed_nolib(QUARK_POS, R"(		let a = [true, false] == [true, false]	; assert(a == true)	)");
}
FLOYD_LANG_PROOF("Floyd test suite", "vector [bool] ==", "different values", ""){
	ut_run_closed_nolib(QUARK_POS, R"(		let a = [false, false] == [true, false]	; assert(a == false)	)");
}
FLOYD_LANG_PROOF("Floyd test suite", "vector [bool] <", "", ""){
	ut_run_closed_nolib(QUARK_POS, R"(		let a = [true, true] < [true, true]	; assert(a == false)	)");
}
FLOYD_LANG_PROOF("Floyd test suite", "vector [bool] <", "different values", ""){
	ut_run_closed_nolib(QUARK_POS, R"(		let a = [true, false] < [true, true]	; assert(a == true)	)");
}
FLOYD_LANG_PROOF("Floyd test suite", "vector [bool] +", "non-empty vectors", ""){
	ut_run_closed_nolib(QUARK_POS, R"(		let [bool] a = [true, false] + [true, true]		; assert(a == [ true, false, true, true ])	)");
}



FLOYD_LANG_PROOF("Floyd test suite", "vector [bool] size()", "empty", "0"){
	ut_run_closed_nolib(QUARK_POS, R"(		let [bool] a = [] let b = size(a)	; assert(b == 0)	)");
}

FLOYD_LANG_PROOF("Floyd test suite", "vector [bool] size()", "2", ""){
	ut_run_closed_nolib(QUARK_POS, R"(		let [bool] a = [true, false, true] let b = size(a)	; assert(b == 3)	)");
}



FLOYD_LANG_PROOF("Floyd test suite", "vector [bool] push_back()", "", ""){
	ut_run_closed_nolib(QUARK_POS, R"(		let [bool] a = push_back([true, false], true)	; assert(a == [ true, false, true ])	)");
}





//////////////////////////////////////////		vector-int



FLOYD_LANG_PROOF("Floyd test suite", "vector [int] constructor expression", "", ""){
	ut_verify_global_result_nolib(QUARK_POS, R"(		let result = 404		)",		make_expected_int(404));
}

FLOYD_LANG_PROOF("Floyd test suite", "vector [int] constructor expression", "", ""){
	ut_verify_global_result_nolib(
		QUARK_POS,
		R"(		let [int] result = [10, 20, 30]		)",
		make_int_vec({ 10, 20, 30 })
	);
}
FLOYD_LANG_PROOF("Floyd test suite", "vector [int] constructor", "", "3"){
	ut_verify_global_result_nolib(
		QUARK_POS,
		R"(

			let [int] a = [1, 2, 3]
			let result = size(a)

		)",
		make_expected_int(3)
	);
}
FLOYD_LANG_PROOF("Floyd test suite", "vector [int] [] lookup", "", ""){
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
	ut_verify_global_result_nolib(QUARK_POS, R"(		let a = [10, 20, 30] let result = a;		)",		make_int_vec({ 10, 20, 30 }) );
}
FLOYD_LANG_PROOF("Floyd test suite", "vector [int] ==", "same values", ""){
	ut_verify_global_result_nolib(QUARK_POS, R"(		let result = [1, 2] == [1, 2]		)",		make_expected_bool(true));
}
FLOYD_LANG_PROOF("Floyd test suite", "vector [int] ==", "different values", ""){
	ut_verify_global_result_nolib(QUARK_POS, R"(		let result = [1, 3] == [1, 2]		)",		make_expected_bool(false));
}
FLOYD_LANG_PROOF("Floyd test suite", "vector [int] <", "", ""){
	ut_verify_global_result_nolib(QUARK_POS, R"(		let result = [1, 2] < [1, 2]		)",		make_expected_bool(false));
}
FLOYD_LANG_PROOF("Floyd test suite", "vector [int] <", "different values", ""){
	ut_verify_global_result_nolib(QUARK_POS, R"(		let result = [1, 2] < [1, 3]		)",		make_expected_bool(true));
}

FLOYD_LANG_PROOF("Floyd test suite", "vector [int] +", "non-empty vectors", ""){
	ut_verify_global_result_nolib(QUARK_POS, R"(		let [int] result = [1, 2] + [3, 4]		)",		make_int_vec({ 1, 2, 3, 4 }) );
}

FLOYD_LANG_PROOF("Floyd test suite", "vector [int] [] lookup", "", ""){
	ut_verify_printout_nolib(
		QUARK_POS,
		R"(

			let a = [ 1, 2, 3 ]
			print(a[0])
			print(a[1])
			print(a[2])

		)",
		{ "1", "2", "3" }
	);
}



FLOYD_LANG_PROOF("Floyd test suite", "vector [int] size()", "empty", "0"){
	ut_verify_global_result_nolib(QUARK_POS, R"(		let [int] a = [] let result = size(a)		)",	make_expected_int(0) );
}
FLOYD_LANG_PROOF("Floyd test suite", "vector [int] size()", "2", ""){
	ut_verify_global_result_nolib(QUARK_POS, R"(		let [int] a = [1, 2, 3] let result = size(a)		)",		make_expected_int(3) );
}

FLOYD_LANG_PROOF("Floyd test suite", "vector [int] update()", "", "valid vector, without side effect on original vector"){
	ut_verify_printout_nolib(
		QUARK_POS,
		R"(

			let a = [ 1, 2, 3 ]
			let b = update(a, 1, 2000)
			assert(a == [ 1, 2, 3 ])
			assert(b == [ 1, 2000, 3 ])
			print(a)
			print(b)

		)",
		{
			R"([1, 2, 3])",
			R"([1, 2000, 3])"
		}
	);
}


FLOYD_LANG_PROOF("Floyd test suite", "vector [int] find()", "", ""){
	ut_run_closed_nolib(QUARK_POS, R"(		assert(find([1,2,3], 4) == -1)		)");
}
FLOYD_LANG_PROOF("Floyd test suite", "vector [int] find()", "", ""){
	ut_run_closed_nolib(QUARK_POS, R"(		assert(find([1,2,3], 1) == 0)		)");
}
FLOYD_LANG_PROOF("Floyd test suite", "vector [int] find()", "", ""){
	ut_run_closed_nolib(QUARK_POS, R"(		assert(find([1,2,2,2,3], 2) == 1)		)");
}

FLOYD_LANG_PROOF("Floyd test suite", "vector [int] push_back()", "", ""){
	ut_verify_global_result_nolib(QUARK_POS, R"(		let [int] result = push_back([1, 2], 3)		)",		make_int_vec({ 1, 2, 3 }) );
}

FLOYD_LANG_PROOF("Floyd test suite", "vector [int] push_back()", "", ""){
	ut_run_closed_nolib(QUARK_POS,
		R"(

			let r = push_back([1, 2], 3)
			assert(r == [1, 2, 3])

		)"
	);
}

FLOYD_LANG_PROOF("Floyd test suite", "vector [int] subset()", "", ""){
	ut_run_closed_nolib(QUARK_POS, R"(		assert(subset([10,20,30], 0, 3) == [10,20,30])		)");
}
FLOYD_LANG_PROOF("Floyd test suite", "vector [int] subset()", "", ""){
	ut_run_closed_nolib(QUARK_POS, R"(		assert(subset([10,20,30], 1, 3) == [20,30])		)");
}
FLOYD_LANG_PROOF("Floyd test suite", "vector [int] subset()", "", ""){
	ut_run_closed_nolib(QUARK_POS, R"(		let r = (subset([10,20,30], 0, 0) == [])		)");
}
FLOYD_LANG_PROOF("Floyd test suite", "vector [int] subset()", "", ""){
	ut_run_closed_nolib(QUARK_POS, R"(		assert(subset([10,20,30], 0, 0) == [])		)");
}

FLOYD_LANG_PROOF("Floyd test suite", "vector [int] replace()", "", ""){
	ut_run_closed_nolib(QUARK_POS, R"(		assert(replace([ 1, 2, 3, 4, 5, 6 ], 2, 5, [20, 30]) == [1, 2, 20, 30, 6])		)");
}

FLOYD_LANG_PROOF("Floyd test suite", "vector [int] replace()", "", ""){
	ut_run_closed_nolib(QUARK_POS,
		R"(

			let h = replace([ 1, 2, 3, 4, 5 ], 1, 4, [ 8, 9 ])
			assert(h == [ 1, 8, 9, 5 ])

		)"
	);
}






//////////////////////////////////////////		vector-double



FLOYD_LANG_PROOF("Floyd test suite", "vector [double] constructor-expression", "", ""){
	ut_verify_global_result_nolib(QUARK_POS, R"(		let [double] result = [10.5, 20.5, 30.5]		)",	make_double_vec({ 10.5, 20.5, 30.5 }) );
}
FLOYD_LANG_PROOF("Floyd test suite", "vector", "Vector can not hold elements of different types.", "exception"){
	ut_verify_exception_nolib(QUARK_POS, R"(		let a = [3, bool]		)", "[Semantics] Vector of type [int] cannot hold an element of type typeid. Line: 1 \"let a = [3, bool]\"");
}


FLOYD_LANG_PROOF("Floyd test suite", "vector []", "Error: Lookup in vector using non-int", "exception"){
	ut_verify_exception_nolib(
		QUARK_POS,
		R"(

			let [string] a = ["one", "two", "three"]
			print(a["not an integer"])

		)",
		"[Semantics] Vector can only be indexed by integers, not a \"string\". Line: 4 \"print(a[\"not an integer\"])\""
	);
}
FLOYD_LANG_PROOF("Floyd test suite", "vector", "Error: Lookup the unlookupable", "exception"){
	ut_verify_exception_nolib(QUARK_POS, R"(		let a = 3[0]		)", "[Semantics] Lookup using [] only works with strings, vectors, dicts and json - not a \"int\". Line: 1 \"let a = 3[0]\"");
}

FLOYD_LANG_PROOF("Floyd test suite", "vector [double] =", "copy", ""){
	ut_verify_global_result_nolib(QUARK_POS, R"(		let a = [10.5, 20.5, 30.5] let result = a		)", make_double_vec({ 10.5, 20.5, 30.5 }) );
}
FLOYD_LANG_PROOF("Floyd test suite", "vector [double] ==", "same values", ""){
	ut_verify_global_result_nolib(QUARK_POS, R"(		let result = [1.5, 2.5] == [1.5, 2.5]		)",	make_expected_bool(true) );
}
FLOYD_LANG_PROOF("Floyd test suite", "vector [double] ==", "different values", ""){
	ut_verify_global_result_nolib(QUARK_POS, R"(		let result = [1.5, 3.5] == [1.5, 2.5]		)",	make_expected_bool(false) );
}
FLOYD_LANG_PROOF("Floyd test suite", "vector [double] <", "", ""){
	ut_verify_global_result_nolib(QUARK_POS, R"(		let result = [1.5, 2.5] < [1.5, 2.5]		)",	make_expected_bool(false) );
}
FLOYD_LANG_PROOF("Floyd test suite", "vector [double] <", "different values", ""){
	ut_verify_global_result_nolib(QUARK_POS, R"(		let result = [1.5, 2.5] < [1.5, 3.5]		)",	make_expected_bool(true) );
}

FLOYD_LANG_PROOF("Floyd test suite", "vector [double] +", "non-empty vectors", ""){
	ut_verify_global_result_nolib(QUARK_POS, R"(		let [double] result = [1.5, 2.5] + [3.5, 4.5]		)", make_double_vec({ 1.5, 2.5, 3.5, 4.5 }) );
}




FLOYD_LANG_PROOF("Floyd test suite", "vector [double] size()", "empty", "0"){
	ut_verify_global_result_nolib(QUARK_POS, R"(		let [double] a = [] let result = size(a)		)",	make_expected_int(0) );
}
FLOYD_LANG_PROOF("Floyd test suite", "vector [double] size()", "2", ""){
	ut_verify_global_result_nolib(QUARK_POS, R"(		let [double] a = [1.5, 2.5, 3.5] let result = size(a)		)",	make_expected_int(3) );
}



FLOYD_LANG_PROOF("Floyd test suite", "vector [double] push_back()", "", ""){
	ut_verify_global_result_nolib(QUARK_POS, R"(		let [double] result = push_back([1.5, 2.5], 3.5)		)", make_double_vec({ 1.5, 2.5, 3.5 }) );
}







//////////////////////////////////////////		TEST VECTOR CONSTRUCTOR FOR ALL TYPES




FLOYD_LANG_PROOF("Floyd test suite", "vector [bool] constructor", "", ""){
	ut_run_closed_nolib(QUARK_POS, R"(		assert(to_string([true, false, true]) == "[true, false, true]")		)");
}
FLOYD_LANG_PROOF("Floyd test suite", "vector [int] constructor", "", ""){
	ut_run_closed_nolib(QUARK_POS, R"(		assert(to_string([1, 2, 3]) == "[1, 2, 3]")		)");
}
FLOYD_LANG_PROOF("Floyd test suite", "vector [double] constructor", "", ""){
	ut_run_closed_nolib(QUARK_POS, R"(		assert(to_string([1.0, 2.0, 3.0]) == "[1.0, 2.0, 3.0]")		)");
}
FLOYD_LANG_PROOF("Floyd test suite", "vector [string] constructor", "", ""){
	ut_run_closed_nolib(QUARK_POS, R"(		assert(to_string(["one", "two", "three"]) == "[\"one\", \"two\", \"three\"]")		)");
}

FLOYD_LANG_PROOF("Floyd test suite", "vector [typeid] constructor", "", ""){
	ut_verify_printout_nolib(QUARK_POS, R"(		let a = int		)", {} );
//	ut_verify_printout_nolib(QUARK_POS, R"(		let a = [int, bool, string]		)", {} );
//	ut_verify_printout_nolib(QUARK_POS, R"(		print([int, bool, string])		)", {} );
}

FLOYD_LANG_PROOF("Floyd test suite", "vector [typeid] constructor", "", ""){
	ut_run_closed_nolib(QUARK_POS, R"(		assert(to_string([int, bool, string]) == "[int, bool, string]")		)");
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
			"[a]"
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
			"[a, b, c]"
		 }
	);
}

	FLOYD_LANG_PROOF("Floyd test suite", "vector [struct] constructor", "", ""){
		ut_verify_printout_nolib(
			QUARK_POS,
			R"(

				struct person_t { int birth_year }

				let d = person_t(1782)
				print(d)

			)",
			{
				R"___({birth_year=1782})___"
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
		R"___([Semantics] Vector of type [[string:string]] cannot hold an element of type [string:int]. Line: 3 "let d = [{"color": "red", "color2": "blue"}, {"num": 100, "num2": 200, "num3": 300}]")___"
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


#endif	//	RUN_LANG_VECTOR_TESTS



#if RUN_LANG_DICT_TESTS

//######################################################################################################################
//	DICT
//######################################################################################################################



	FLOYD_LANG_PROOF("Floyd test suite", "mini program", "", "built-ins compile"){
		ut_run_closed_nolib(
			QUARK_POS,
			R"(

				let a = 2

			)"
		);
	}

FLOYD_LANG_PROOF("Floyd test suite", "dict constructor", "No type", "error"){
	ut_verify_exception_nolib(
		QUARK_POS,
		R"(

			let a = {}
			print(a)

		)",
		"[Semantics] Cannot infer dictionary element type, add explicit type. Line: 3 \"let a = {}\""
	);
}


FLOYD_LANG_PROOF("Floyd test suite", "dict constructor", "", "Error cannot infer type"){
	ut_verify_exception_nolib(QUARK_POS, R"(		assert(size({}) == 0)		)", "[Semantics] Cannot infer dictionary element type, add explicit type. Line: 1 \"assert(size({}) == 0)\"");
}

FLOYD_LANG_PROOF("Floyd test suite", "dict constructor", "", "Error cannot infer type"){
	ut_verify_exception_nolib(QUARK_POS, R"(		print({})		)", "[Semantics] Cannot infer dictionary element type, add explicit type. Line: 1 \"print({})\"");
}

FLOYD_LANG_PROOF("Floyd test suite", "dict [int] constructor", "", "Compilation error"){
	ut_verify_exception_nolib(QUARK_POS,
		R"(

			mutable a = {}
			a = { "hello": 1 }
			print(a)

		)",
		"[Semantics] Cannot infer dictionary element type, add explicit type. Line: 3 \"mutable a = {}\""
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
		"[Semantics] Dictionary can only be looked up using string keys, not a \"int\". Line: 4 \"print(a[3])\""
	);
}

FLOYD_LANG_PROOF("Floyd test suite", "dict [int]", "Dict can not hold elements of different types.", "exception"){
	ut_verify_exception_nolib(
		QUARK_POS,
		R"(

			let a = {"one": 1, "two": bool}

		)",
		"[Semantics] Dictionary of type [string:int] cannot hold an element of type typeid. Line: 3 \"let a = {\"one\": 1, \"two\": bool}\""
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
		"[Semantics] Cannot infer dictionary element type, add explicit type. Line: 3 \"let a = update({}, \"one\", 1)\""
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
	ut_run_closed_nolib(QUARK_POS, R"(		assert(({"one": 1, "two": 2} == {"one": 1, "two": 2}) == true)		)");
}
FLOYD_LANG_PROOF("Floyd test suite", "dict [int] ==", "", ""){
	ut_run_closed_nolib(QUARK_POS, R"(		assert(({"one": 1, "two": 2} == {"two": 2}) == false)		)");
}
FLOYD_LANG_PROOF("Floyd test suite", "dict [int] ==", "", ""){
	ut_run_closed_nolib(QUARK_POS, R"(		assert(({"one": 2, "two": 2} == {"one": 1, "two": 2}) == false)		)");
}
FLOYD_LANG_PROOF("Floyd test suite", "dict [int] ==", "", ""){
	ut_run_closed_nolib(QUARK_POS, R"(		assert(({"one": 1, "two": 2} < {"one": 1, "two": 2}) == false)		)");
}
FLOYD_LANG_PROOF("Floyd test suite", "dict [int] <", "", ""){
	ut_run_closed_nolib(QUARK_POS, R"(		assert(({"one": 1, "two": 1} < {"one": 1, "two": 2}) == true)		)");
}

FLOYD_LANG_PROOF("Floyd test suite", "dict [int] size()", "", "1"){
	ut_run_closed_nolib(QUARK_POS, R"(		assert(size({"one":1}) == 1)		)");
}
FLOYD_LANG_PROOF("Floyd test suite", "dict [int] size()", "", "2"){
	ut_run_closed_nolib(QUARK_POS, R"(		assert(size({"one":1, "two":2}) == 2)		)");
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
	ut_run_closed_nolib(QUARK_POS, R"(

		let a = { "one": 1, "two": 2, "three" : 3}
		assert(exists(a, "two") == true)
		assert(exists(a, "four") == false)

	)");
}

FLOYD_LANG_PROOF("Floyd test suite", "dict [int] erase()", "", ""){
	ut_run_closed_nolib(QUARK_POS, R"(

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
	ut_run_closed_nolib(QUARK_POS, R"(		assert(({"one": "1000", "two": "2000"} == {"one": "1000", "two": "2000"}) == true)		)");
}
FLOYD_LANG_PROOF("Floyd test suite", "dict [string] ==", "", ""){
	ut_run_closed_nolib(QUARK_POS, R"(		assert(({"one": "1000", "two": "2000"} == {"two": "2000"}) == false)		)");
}
FLOYD_LANG_PROOF("Floyd test suite", "dict [string] ==", "", ""){
	ut_run_closed_nolib(QUARK_POS, R"(		assert(({"one": "2000", "two": "2000"} == {"one": "1000", "two": "2000"}) == false)		)");
}
FLOYD_LANG_PROOF("Floyd test suite", "dict [string] ==", "", ""){
	ut_run_closed_nolib(QUARK_POS, R"(		assert(({"one": "1000", "two": "2000"} < {"one": "1000", "two": "2000"}) == false)		)");
}
FLOYD_LANG_PROOF("Floyd test suite", "dict [string] <", "", ""){
	ut_run_closed_nolib(QUARK_POS, R"(		assert(({"one": "1000", "two": "1000"} < {"one": "1000", "two": "2000"}) == true)		)");
}

FLOYD_LANG_PROOF("Floyd test suite", "dict [string] size()", "", "1"){
	ut_run_closed_nolib(QUARK_POS, R"(		assert(size({"one":"1000"}) == 1)		)");
}
FLOYD_LANG_PROOF("Floyd test suite", "dict [string] size()", "", "2"){
	ut_run_closed_nolib(QUARK_POS, R"(		assert(size({"one":"1000", "two":"2000"}) == 2)		)");
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
	ut_run_closed_nolib(QUARK_POS, R"(

		let a = { "one": "1000", "two": "2000", "three" : "3000"}
		assert(exists(a, "two") == true)
		assert(exists(a, "four") == false)

	)");
}

FLOYD_LANG_PROOF("Floyd test suite", "dict [string] erase()", "", ""){
	ut_run_closed_nolib(QUARK_POS, R"(

		let a = { "a": "1000", "b": "2000", "c" : "3000" }
		let b = erase(a, "a")
		assert(b == { "b": "2000", "c" : "3000"})

	)");
}



FLOYD_LANG_PROOF("Floyd test suite", "dict [string] get_keys()", "", ""){
	ut_run_closed_nolib(QUARK_POS, R"(

		let [string: int] a = {}
		let b = get_keys(a)
		assert(b == [])

	)");
}
FLOYD_LANG_PROOF("Floyd test suite", "dict [string] get_keys()", "", ""){
	ut_run_closed_nolib(QUARK_POS, R"(

		let a = { "a": 10 }
		let b = get_keys(a)
//		assert(b == [ "a", "b", "c"])

	)");
}
FLOYD_LANG_PROOF("Floyd test suite", "dict [string] get_keys()", "", ""){
	ut_run_closed_nolib(QUARK_POS, R"(

		let a = { "a": "ten" }
		let b = get_keys(a)
//		assert(b == [ "a", "b", "c"])

	)");
}
FLOYD_LANG_PROOF("Floyd test suite", "dict [string] get_keys()", "", ""){
	ut_run_closed_nolib(QUARK_POS, R"(

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
		"[Semantics] Dictionary can only be looked up using string keys, not a \"int\". Line: 4 \"print(a[3])\""
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
			R"___({"one": a, "three": c, "two": b})___"
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


#endif //	RUN_LANG_DICT_TESTS




#if RUN_LANG_STRUCT_TESTS
//######################################################################################################################
//	STRUCT
//######################################################################################################################




//#define FLOYD_LANG_PROOF FLOYD_LANG_PROOF_VIP



FLOYD_LANG_PROOF("Floyd test suite", "struct", "", ""){
	ut_run_closed_nolib(QUARK_POS, R"(		struct t {}		)");
}
FLOYD_LANG_PROOF("Floyd test suite", "struct", "", ""){
	ut_run_closed_nolib(QUARK_POS, "struct t {\n}");
}

FLOYD_LANG_PROOF("Floyd test suite", "struct", "", ""){
	ut_run_closed_nolib(QUARK_POS, R"(		struct t { int a }		)");
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




FLOYD_LANG_PROOF("Floyd test suite", "struct", "Test struct name-equivalence", ""){
	ut_verify_exception_nolib(
		QUARK_POS,
		R"(

			struct pixel1_t { int a }
			struct pixel2_t { int a }
			let pixel2_t e = pixel1_t(100)

		)",
		R"___([Semantics] Expression type mismatch - cannot convert 'pixel1_t' to 'pixel2_t. Line: 5 "let pixel2_t e = pixel1_t(100)")___"
	);
}







FLOYD_LANG_PROOF("Floyd test suite", "struct", "Unnamed struct", ""){
	ut_verify_printout_nolib(
		QUARK_POS,
		R"(

			let a = struct { string name; int age }("Bob", 62)
			print(a)

		)",
		{
			R"___({name="Bob", age=62})___"
		}
	);
}

FLOYD_LANG_PROOF("Floyd test suite", "struct", "Unnamed struct", ""){
	ut_verify_printout_nolib(
		QUARK_POS,
		R"(

			func struct { string name; int age } f()

		)",
		{
		}
	);
}

FLOYD_LANG_PROOF("Floyd test suite", "struct", "Unnamed struct", ""){
	ut_verify_printout_nolib(
		QUARK_POS,
		R"(

			func struct { string message; int err } open(string path){
				return struct { string message; int err }("File not found", -4)
			}

			print(open("test"))

		)",
		{
			R"___({message="File not found", err=-4})___"
		}
	);
}

FLOYD_LANG_PROOF("Floyd test suite", "struct", "Unnamed struct", ""){
	ut_verify_printout_nolib(
		QUARK_POS,
		R"(

			struct node_t {
				int count
				[struct { int a int b }] elements
			}

			let a = node_t(10, [struct { int a int b}(100, 1000),struct { int a int b}(200, 2000) ])

			print(a)

		)",
		{
			R"___({count=10, elements=[{a=100, b=1000}, {a=200, b=2000}]})___"
		}
	);
}


#if 0
//??? more advanced infererence -- very concenient.
FLOYD_LANG_PROOF("Floyd test suite", "struct", "Unnamed struct", ""){
	ut_verify_printout_nolib(
		QUARK_POS,
		R"(

			let struct { string name; int age } a = ("Bob", 62)
			print(a)

		)",
		{
			R"___({name="Bob", age=62})___"
		}
	);
}

#endif



FLOYD_LANG_PROOF("Floyd test suite", "struct", "Test lexical scopes for named struct types", ""){
	ut_verify_printout_nolib(
		QUARK_POS,
		R"(

			struct pixel_t { int a }

			func void f(pixel_t p){
				print(p)
			}

			f(pixel_t(13))

		)",
		{ "{a=13}" }
	);
}


FLOYD_LANG_PROOF("Floyd test suite", "struct", "Test lexical scopes for named struct types", ""){
	ut_verify_printout_nolib(
		QUARK_POS,
		R"(

			struct pixel_t { int a }

			func void f(pixel_t p){
				struct pixel_t {
					int b;
				}

				print(p)
				print(pixel_t(14))
			}

			f(pixel_t(13))

		)",
		{ "{a=13}", "{b=14}" }
	);
}

//??? Block naming struct members 3 etc.

FLOYD_LANG_PROOF("Floyd test suite", "struct", "Use the same type in several blocks inside one function", ""){
	ut_verify_printout_nolib(
		QUARK_POS,
		R"(

			struct T { int a }


			let g = T(300)

			func void f(T p, bool first){
				struct T { int b }

				let p2 = T(14)

				print(g)
				print(p)
				print(p2)

				if(first){
					struct T { int c }
					print(g)
					print(p)
					print(p2)
					print(T(15))
				}
				else{
					print(g)
					print(p)

					//	This time, define the type in the middle of the block.
					struct T { int d }

					print(p2)
					print(T(16))
				}
			}

			f(T(100), true)
			f(T(200), false)

		)",
		{
			"{a=300}",
			"{a=100}",
			"{b=14}",
			"{a=300}",
			"{a=100}",
			"{b=14}",
			"{c=15}",

			"{a=300}",
			"{a=200}",
			"{b=14}",
			"{a=300}",
			"{a=200}",
			"{b=14}",
			"{d=16}"
		}
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

FLOYD_LANG_PROOF("Floyd test suite", "struct", "Update struct member that is a POD", ""){
	ut_verify_printout_nolib(
		QUARK_POS,
		R"(

			struct color { int red int green int blue }
			struct image { color back color front }

			let a = image(color(1, 2, 3), color(4, 5, 6))

			let b = update(a, front, color(7, 8, 9))
			print(b)

		)",
		{ "{back={red=1, green=2, blue=3}, front={red=7, green=8, blue=9}}" }
	);
}

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
		"[Semantics] Expression type mismatch - cannot convert 'file' to 'color. Line: 5 \"print(color(1, 2, 3) == file(404))\""
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
		R"___([Syntax] Expected constant or identifier. Line: 5 "let b = a.green = 3")___"
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
//	Not supported yet, should support nested indexes and lookups etc.
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
	ut_run_closed_nolib(QUARK_POS, R"(

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
		R"___([Semantics] Local identifier "a" already exists. Line: 4 "struct a { int x }")___"
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
		"[Semantics] Unknown struct member \"y\". Line: 5 \"print(b.y)\""
	);
}

FLOYD_LANG_PROOF("Floyd test suite", "struct", "Error: Access member in non-struct", "exception"){
	ut_verify_exception_nolib(
		QUARK_POS,
		R"(

			let a = 10
			print(a.y)

		)",
		"[Semantics] Left hand side is not a struct value, it's of type \"int\". Line: 4 \"print(a.y)\""
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
		make_expected_double(100.0)
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
		make_expected_double(201.0)
	);
}

FLOYD_LANG_PROOF("Floyd test suite", "Struct", "Error: Wrong number of arguments to struct-constructor", "exception"){
	ut_verify_exception_nolib(
		QUARK_POS,
		R"(

			struct pos { double x double y }
			let a = pos(3)

		)",
		"[Semantics] Wrong number of arguments in function call, got 1, expected 2. Line: 4 \"let a = pos(3)\""
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
		"[Semantics] Expression type mismatch - cannot convert 'int' to 'double. Line: 4 \"let a = pos(3, \"hello\")\""
	);
}





//	??? Add support for this!!!
FLOYD_LANG_PROOF("Floyd test suite", "struct", "recursive named types", ""){
	ut_run_closed_nolib(QUARK_POS, R"(

		struct object_t {
			object_t left
		}

	)");
}

FLOYD_LANG_PROOF("Floyd test suite", "struct", "recursive named types", ""){
	ut_run_closed_nolib(QUARK_POS, R"(

		struct object_t {
			[object_t] inside
		}
		let x = object_t( [] )
		print(x)

	)");
}


FLOYD_LANG_PROOF("Floyd test suite", "struct", "recursive named types", ""){
	ut_run_closed_nolib(QUARK_POS, R"(

		struct object_t {
			string name
			[object_t] inside
		}
		let x = object_t( "chest", [] )
		print(x)

	)");
}

FLOYD_LANG_PROOF("Floyd test suite", "struct", "recursive named types", ""){
	ut_run_closed_nolib(QUARK_POS, R"(

		struct object_t {
			string name
			[object_t] inside
		}
		let sword = object_t( "rusty sword", [] )
		let coin = object_t( "silver coin", [] )
		let chest = object_t( "chest", [ sword, coin ] )
		print(chest)

	)");
}

FLOYD_LANG_PROOF("Floyd test suite", "struct", "recursive named types", ""){
	ut_run_closed_nolib(QUARK_POS, R"(

		struct object_t {
			string name
			[object_t] inside
		}
		let sword = object_t( "rusty sword", [] )
		let coin = object_t( "silver coin", [] )
		let chest = object_t( "chest", [ sword, coin ] )
		print(chest)

		let [object_t] empty = []
		let inside2 = replace(chest.inside, 1, 2, empty)
		let chest2 = object_t( chest.name, inside2 )
		print(chest2)

	)");
}





#endif	//	RUN_LANG_STRUCT_TESTS




#if RUN_LANG_JSON_TESTS

//######################################################################################################################
//	json
//######################################################################################################################





//??? document or disable using json-value directly as lookup parent.

FLOYD_LANG_PROOF("Floyd test suite", "json::null", "", ""){
	ut_run_closed_nolib(QUARK_POS, R"(		let r = null		)");
}

FLOYD_LANG_PROOF("Floyd test suite", "json<string> Infer json::string", "", ""){
	ut_verify_global_result_nolib(QUARK_POS, R"(		let json result = "hello"		)", make_expected_json("hello"));
}

FLOYD_LANG_PROOF("Floyd test suite", "json<string> string-size()", "", ""){
	ut_verify_global_result_nolib(
		QUARK_POS,
		R"(

			let json a = "hello"
			let result = size(a);

		)",
		make_expected_int(5)
	);
}

FLOYD_LANG_PROOF("Floyd test suite", "json()", "json(123)", ""){
	ut_verify_global_result_nolib(QUARK_POS, "let result = json(123)", make_expected_json(json_t(123.0)));
}

FLOYD_LANG_PROOF("Floyd test suite", "json()", "", ""){
	ut_verify_global_result_nolib(
		QUARK_POS,
		R"(

			let result = json("hello")

		)",
		make_expected_json(json_t("hello"))
	);
}

FLOYD_LANG_PROOF("Floyd test suite", "json()", "", ""){
	ut_verify_global_result_nolib(QUARK_POS, "let result = json([1,2,3])", make_expected_json(json_t::make_array({1,2,3})));
}

FLOYD_LANG_PROOF("Floyd test suite", "json<number> construct number", "", ""){
	ut_verify_global_result_nolib(QUARK_POS, R"(		let json result = 13		)", make_expected_json(13));
}

FLOYD_LANG_PROOF("Floyd test suite", "json<true> construct true", "", ""){
	ut_verify_global_result_nolib(QUARK_POS, R"(		let json result = true		)", make_expected_json(true));
}
FLOYD_LANG_PROOF("Floyd test suite", "json<false> construct false", "", ""){
	ut_verify_global_result_nolib(QUARK_POS, R"(		let json result = false		)", make_expected_json(false));
}

FLOYD_LANG_PROOF("Floyd test suite", "json<array> construct array", "construct array", ""){
	ut_verify_global_result_nolib(QUARK_POS, R"(		let json result = ["hello", "bye"]		)", make_expected_json(json_t::make_array(std::vector<json_t>{ "hello", "bye" }))	);
}

FLOYD_LANG_PROOF("Floyd test suite", "json<string> read array member", "", ""){
	ut_verify_global_result_nolib(
		QUARK_POS,
		R"(

			let json a = ["hello", "bye"]
			let result = a[0]

		)",
		make_expected_json("hello")
	);
}
FLOYD_LANG_PROOF("Floyd test suite", "json<string> read array member", "", ""){
	ut_verify_global_result_nolib(
		QUARK_POS,
		R"(

			let json a = ["hello", "bye"]
			let result = string(a[0]) + string(a[1])

		)",
		make_expected_string("hellobye")
	);
}

FLOYD_LANG_PROOF("Floyd test suite", "json<string> read array member", "", ""){
	ut_verify_global_result_nolib(
		QUARK_POS,
		R"(

			let json a = ["hello", "bye"]
			let result = a[1]

		)",
		make_expected_json("bye")
	);
}

FLOYD_LANG_PROOF("Floyd test suite", "json<string> size()", "", ""){
	ut_verify_global_result_nolib(
		QUARK_POS,
		R"(

			let json a = ["a", "b", "c", "d"]
			let result = size(a)

		)",
		make_expected_int(4)
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
	ut_run_closed_nolib(QUARK_POS, R"(

		let json a = { "a": 1, "b": 2, "c": 3, "d": 4, "e": 5 }
		assert(size(a) == 5)

	)");
}


FLOYD_LANG_PROOF("Floyd test suite", "json<null> construct", "", ""){
	ut_verify_global_result_nolib(QUARK_POS, R"(		let json result = null		)", make_expected_json(json_t()));
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
		"[Semantics] Dictionary of type [string:string] cannot hold an element of type [pixel_t]. Line: 6 \"let c = { \"version\": \"1.0\", \"image\": [pixel_t(100.0, 200.0), pixel_t(101.0, 201.0)] }\""
	);
}




//////////////////////////////////////////		JSON TYPE INFERENCE



FLOYD_LANG_PROOF("Floyd test suite", "json<string> {}", "Infer {}", "JSON object"){
	ut_verify_global_result_nolib(QUARK_POS, R"(		let json result = {}		)", make_expected_json(json_t::make_object()));
}


FLOYD_LANG_PROOF("Floyd test suite", "get_json_type()", "", "1"){
	ut_verify_global_result_nolib(QUARK_POS, R"(		let result = get_json_type(json({}))		)", make_expected_int(1));
}

FLOYD_LANG_PROOF("Floyd test suite", "get_json_type()", "", "1"){
	ut_verify_printout_nolib(
		QUARK_POS,
		R"(

			let a = json({ "color": "black"})
			print(a)

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
		make_expected_int(1)
	);
}
FLOYD_LANG_PROOF("Floyd test suite", "get_json_type()", "array", "2"){
	ut_verify_global_result_nolib(QUARK_POS, R"(		let result = get_json_type(json([]))		)", make_expected_int(2));
}
FLOYD_LANG_PROOF("Floyd test suite", "get_json_type()", "string", "3"){
	ut_verify_global_result_nolib(QUARK_POS, R"(		let result = get_json_type(json("hello"))		)", make_expected_int(3));
}
FLOYD_LANG_PROOF("Floyd test suite", "get_json_type()", "number", "4"){
	ut_verify_global_result_nolib(QUARK_POS, R"(		let result = get_json_type(json(13))		)", make_expected_int(4));
}
FLOYD_LANG_PROOF("Floyd test suite", "get_json_type()", "true", "5"){
	ut_verify_global_result_nolib(QUARK_POS, R"(		let result = get_json_type(json(true))		)", make_expected_int(5));
}
FLOYD_LANG_PROOF("Floyd test suite", "get_json_type()", "false", "6"){
	ut_verify_global_result_nolib(QUARK_POS, R"(		let result = get_json_type(json(false))		)", make_expected_int(6));
}

FLOYD_LANG_PROOF("Floyd test suite", "get_json_type()", "null", "7"){
	ut_verify_global_result_nolib(QUARK_POS, R"(		let result = get_json_type(json(null))		)", make_expected_int(7));
}

FLOYD_LANG_PROOF("Floyd test suite", "get_json_type()", "DOCUMENTATION SNIPPET", ""){
	ut_run_closed_nolib(QUARK_POS, R"___(

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

#endif	//	RUN_LANG_JSON_TESTS




#if RUN_LANG_INTRINSICS_TESTS
//######################################################################################################################
//	INTRINSICS
//######################################################################################################################

//////////////////////////////////////////		INTRINSIC - typeof()


FLOYD_LANG_PROOF("Floyd test suite", "typeof()", "", ""){
	ut_verify_global_result_nolib(
		QUARK_POS,
		R"(

			let result = typeof(145)

		)",
		make_expected_typeid(types_t(), type_t::make_int())
	);
}

FLOYD_LANG_PROOF("Floyd test suite", "typeof()", "", ""){
	ut_verify_global_result_nolib(QUARK_POS, R"(		let result = to_string(typeof(145))		)", make_expected_string("int"));
}

FLOYD_LANG_PROOF("Floyd test suite", "typeof()", "", ""){
	ut_verify_global_result_nolib(QUARK_POS, R"(		let result = typeof("hello")		)", make_expected_typeid(types_t(), type_t::make_string()));
}

FLOYD_LANG_PROOF("Floyd test suite", "typeof()", "", ""){
	ut_verify_global_result_nolib(QUARK_POS, R"(		let result = to_string(typeof("hello"))		)",	make_expected_string("string"));
}

FLOYD_LANG_PROOF("Floyd test suite", "typeof()", "", ""){
	types_t temp;
	ut_verify_global_result_nolib(
		QUARK_POS,
		R"(		let result = typeof([1,2,3])		)",
		make_expected_typeid(temp, make_vector(temp, type_t::make_int()))
	);
}
FLOYD_LANG_PROOF("Floyd test suite", "typeof()", "", ""){
	ut_verify_global_result_nolib(QUARK_POS, R"(		let result = to_string(typeof([1,2,3]))		)",make_expected_string("[int]") );
}

FLOYD_LANG_PROOF("Floyd test suite", "typeof()", "", ""){
	ut_verify_global_result_nolib(
		QUARK_POS,
		R"(

			let result = to_string(typeof(int));

		)",
		make_expected_string("typeid")
	);
}




//////////////////////////////////////////		parse_json_script()


FLOYD_LANG_PROOF("Floyd test suite", "parse_json_script()", "", ""){
	ut_verify_global_result_nolib(QUARK_POS, R"(		let result = parse_json_script("\"genelec\"")		)", make_expected_json(json_t("genelec")));
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
	ut_verify_global_result_nolib(QUARK_POS, R"(		let result = to_json(true)		)", make_expected_json(json_t(true)));
}
FLOYD_LANG_PROOF("Floyd test suite", "to_json()", "bool", ""){
	ut_verify_global_result_nolib(QUARK_POS, R"(		let result = to_json(false)		)", make_expected_json(json_t(false)));
}

FLOYD_LANG_PROOF("Floyd test suite", "to_json()", "int", ""){
	ut_verify_global_result_nolib(QUARK_POS, R"(		let result = to_json(789)		)", make_expected_json(json_t(789.0)));
}
FLOYD_LANG_PROOF("Floyd test suite", "to_json()", "int", ""){
	ut_verify_global_result_nolib(QUARK_POS, R"(		let result = to_json(-987)		)", make_expected_json(json_t(-987.0)));
}

FLOYD_LANG_PROOF("Floyd test suite", "to_json()", "double", ""){
	ut_verify_global_result_nolib(QUARK_POS, R"(		let result = to_json(-0.125)		)", make_expected_json(json_t(-0.125)));
}


FLOYD_LANG_PROOF("Floyd test suite", "to_json()", "string", ""){
	ut_verify_global_result_nolib(QUARK_POS, R"(		let result = to_json("fanta")		)", make_expected_json(json_t("fanta")));
}

FLOYD_LANG_PROOF("Floyd test suite", "to_json()", "typeid", ""){
	ut_verify_global_result_nolib(QUARK_POS, R"(		let result = to_json(typeof([2,2,3]))		)", make_expected_json(json_t::make_array({ "vector", "int"})));
}

FLOYD_LANG_PROOF("Floyd test suite", "to_json()", "[]", ""){
	ut_verify_global_result_nolib(QUARK_POS, R"(		let result = to_json([1,2,3])		)", make_expected_json(json_t::make_array({ 1, 2, 3 })));
}

FLOYD_LANG_PROOF("Floyd test suite", "to_json()", "{}", ""){
	ut_verify_global_result_nolib(
		QUARK_POS,
		R"(

			let result = to_json({"ten": 10, "eleven": 11})

		)",
		make_expected_json(
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
		make_expected_string("{ \"x\": 100, \"y\": 200 }")
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
		make_expected_string("[{ \"x\": 100, \"y\": 200 }, { \"x\": 101, \"y\": 201 }]")
	);
}


//////////////////////////////////////////		to_json() -> from_json() roundtrip

/*
	from_json() returns different types depending on its 2nd argument.
	??? test all types!
*/
FLOYD_LANG_PROOF("Floyd test suite", "from_json()", "bool", ""){
	ut_verify_global_result_nolib(QUARK_POS, R"(		let json z = to_json(true); let result = from_json(z, bool)		)", make_expected_bool(true));
}

FLOYD_LANG_PROOF("Floyd test suite", "from_json()", "bool", ""){
	ut_verify_global_result_nolib(QUARK_POS, R"(		let result = from_json(to_json(true), bool)		)", make_expected_bool(true));
}

FLOYD_LANG_PROOF("Floyd test suite", "from_json()", "bool", ""){
	ut_verify_global_result_nolib(QUARK_POS, R"(		let result = from_json(to_json(false), bool)		)", make_expected_bool(false));
}

FLOYD_LANG_PROOF("Floyd test suite", "from_json()", "int", ""){
	ut_verify_global_result_nolib(QUARK_POS, R"(		let result = from_json(to_json(91), int)		)", make_expected_int(91));
}

FLOYD_LANG_PROOF("Floyd test suite", "from_json()", "double", ""){
	ut_verify_global_result_nolib(QUARK_POS, R"(		let result = from_json(to_json(-0.125), double)		)", make_expected_double(-0.125));
}

FLOYD_LANG_PROOF("Floyd test suite", "from_json()", "string", ""){
	ut_verify_global_result_nolib(QUARK_POS, R"(		let result = from_json(to_json(""), string)		)", make_expected_string(""));
}

FLOYD_LANG_PROOF("Floyd test suite", "from_json()", "string", ""){
	ut_verify_global_result_nolib(QUARK_POS, R"(		let result = from_json(to_json("cola"), string)		)", make_expected_string("cola"));
}

#if 0
//??? requires unnamed structs
FLOYD_LANG_PROOF("Floyd test suite", "from_json()", "point_t", ""){
	types_t temp;
	const auto point_t_def = std::vector<member_t>{
		member_t(type_t::make_double(), "x"),
		member_t(type_t::make_double(), "y")
	};
	const auto expected = value_t::make_struct_value(
		temp,
		make_struct(temp, struct_type_desc_t(point_t_def)),
		{ value_t::make_double(1), value_t::make_double(3) }
	);

	ut_verify_global_result_nolib(
		QUARK_POS,
		R"(

			struct point_t { double x double y }
			let result = from_json(to_json(point_t(1.0, 3.0)), point_t)

		)",
		value_and_type_to_json(temp, expected)
	);
}
#endif








//////////////////////////////////////////		HIGHER-ORDER INTRINSICS - map()


FLOYD_LANG_PROOF("Floyd test suite", "map()", "map over [int]", ""){
	ut_run_closed_nolib(QUARK_POS, R"(

		let a = [ 10, 11, 12 ]

		func int f(int v, string context){
			assert(context == "some context")
			return 1000 + v
		}

		let r = map(a, f, "some context")
//		print(to_string(result))
		assert(r == [ 1010, 1011, 1012 ])

	)");
}

FLOYD_LANG_PROOF("Floyd test suite", "map()", "map over [int]", ""){
	ut_run_closed_nolib(QUARK_POS, R"(

		let a = [ 10, 11, 12 ]

		func string f(int v, string context){
			return to_string(1000 + v)
		}

		let r = map(a, f, "some context")
//		print(to_string(result))
		assert(r == [ "1010", "1011", "1012" ])

	)");
}

FLOYD_LANG_PROOF("Floyd test suite", "map()", "map over [string]", ""){
	ut_run_closed_nolib(QUARK_POS, R"(

		let a = [ "one", "two_", "three" ]

		func int f(string v, string context){
			return size(v)
		}

		let r = map(a, f, "some context")
//		print(to_string(result))
		assert(r == [ 3, 4, 5 ])

	)");
}

FLOYD_LANG_PROOF("Floyd test suite", "map()", "context struct", ""){
	ut_run_closed_nolib(QUARK_POS, R"(

		struct context_t { int a string b }

		func int f(int v, context_t context){
			assert(context.a == 2000)
			assert(context.b == "twenty")
			return context.a + v
		}

		let r = map([ 10, 11, 12 ], f, context_t( 2000, "twenty"))
		assert(r == [ 2010, 2011, 2012 ])

	)");
}
//??? make sure f() can't be impure!

/*
//////////////////////////////////////////		HIGHER-ORDER INTRINSICS - map_string()



FLOYD_LANG_PROOF("Floyd test suite", "map_string()", "", ""){
	ut_run_closed_nolib(QUARK_POS, R"(

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



//////////////////////////////////////////		HIGHER-ORDER INTRINSICS - map_dag()



FLOYD_LANG_PROOF("Floyd test suite", "map_dag()", "No dependencies", ""){
	ut_run_closed_nolib(QUARK_POS, R"(

		func string f(string v, [string] inputs, string context){
			assert(context == "iop")
			return "[" + v + "]"
		}

		let r = map_dag([ "one", "ring", "to" ], [ -1, -1, -1 ], f, "iop")
//		print(to_string(result))
		assert(r == [ "[one]", "[ring]", "[to]" ])

	)");
}

FLOYD_LANG_PROOF("Floyd test suite", "map_dag()", "No dependencies", ""){
	ut_run_closed_nolib(QUARK_POS, R"(

		func string f(string v, [string] inputs, string context){
			assert(context == "qwerty")
			return "[" + v + "]"
		}

		let r = map_dag([ "one", "ring", "to" ], [ 1, 2, -1 ], f, "qwerty")
//		print(to_string(result))
		assert(r == [ "[one]", "[ring]", "[to]" ])

	)");
}

FLOYD_LANG_PROOF("Floyd test suite", "map_dag()", "complex", ""){
	ut_run_closed_nolib(QUARK_POS, R"(

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

		let r = map_dag([ "D", "B", "A", "C", "E", "F" ], [ 4, 2, -1, 4, 2, 4 ], f, "1234")
//		print(to_string(result))
		assert(r == [ "D[]", "B[]", "A[B[], E[D[], C[], F[]]]", "C[]", "E[D[], C[], F[]]", "F[]" ])

	)");
}



//////////////////////////////////////////		HIGHER-ORDER INTRINSICS - reduce()



FLOYD_LANG_PROOF("Floyd test suite", "reduce()", "int reduce([int], int, func int(int, int))", ""){
	ut_run_closed_nolib(QUARK_POS, R"(

		func int f(int acc, int element, string context){
			assert(context == "con123")
			return acc + element * 2
		}

		let r = reduce([ 10, 11, 12 ], 2000, f, "con123")

//		print(to_string(result))
		assert(r == 2066)

	)");
}

FLOYD_LANG_PROOF("Floyd test suite", "reduce()", "string reduce([int], string, func int(string, int))", ""){
	ut_run_closed_nolib(QUARK_POS, R"___(

		func string f(string acc, int v, string context){
			assert(context == "1234")

			mutable s = acc
			for(e in 0 ..< v){
				s = "<" + s + ">"
			}
			s = "(" + s + ")"
			return s
		}

		let r = reduce([ 2, 4, 1 ], "O", f, "1234")
//		print(to_string(result))
		assert(r == "(<(<<<<(<<O>>)>>>>)>)")

	)___");
}



//////////////////////////////////////////		HIGHER-ORDER INTRINSICS - filter()



FLOYD_LANG_PROOF("Floyd test suite", "filter()", "int filter([int], int, func int(int, int))", ""){
	ut_run_closed_nolib(QUARK_POS, R"(

		func bool f(int element, string context){
			assert(context == "abcd")
			return element % 3 == 0 ? true : false
		}

		let r = filter([ 3, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13 ], f, "abcd")

//		print(to_string(result))
		assert(r == [ 3, 3, 6, 9, 12 ])

	)");
}

FLOYD_LANG_PROOF("Floyd test suite", "filter()", "string filter([int], string, func int(string, int))", ""){
	ut_run_closed_nolib(QUARK_POS, R"___(

		func bool f(string element, string context){
			assert(context == "xyz")
			return size(element) == 3 || size(element) == 5
		}

		let r = filter([ "one", "two", "three", "four", "five", "six", "seven" ], f, "xyz")

//		print(to_string(result))
		assert(r == [ "one", "two", "three", "six", "seven" ])

	)___");
}



//////////////////////////////////////////		HIGHER-ORDER INTRINSICS - stable_sort()



FLOYD_LANG_PROOF("Floyd test suite", "stable_sort()", "[int]", ""){
	ut_verify_printout_nolib(
		QUARK_POS,
		R"(

			func bool less_f(int left, int right, string s){
				return left < right
			}

			let a = stable_sort([ 1, 2, 8, 4 ], less_f, "hello")
			print(a)

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

			let a = stable_sort([ 1, 2, 8, 4 ], less_f, "hello")
			print(a)

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

			let a = stable_sort([ "1", "2", "8", "4" ], less_f, "hello")
			print(a)

		)",
		{ R"___(["1", "2", "4", "8"])___" }
	);
}


#endif	//	RUN_LANG_INTRINSICS_TESTS





#if RUN_BENCHMARK_DEF_STATEMENT__AND__BENCHMARK_EXPRESSION_TESTS


//######################################################################################################################
//	BENCHMARK-DEF STATEMENT, BENCHMARK EXPRESSION
//######################################################################################################################


FLOYD_LANG_PROOF("Floyd test suite", "benchmark", "empty body", "overflows max samples"){
	ut_run_closed_nolib(
		QUARK_POS,
		R"(

			let dur = benchmark {
			}
			print(dur)

		)"
	);
}

FLOYD_LANG_PROOF("Floyd test suite", "benchmark", "", ""){
	ut_verify_printout_nolib(
		QUARK_POS,
		R"(

			let dur = benchmark {
				let a = [ 10, 20, 30 ]
			}

		)",
		{ }
	);
}

FLOYD_LANG_PROOF("Floyd test suite", "benchmark", "", ""){
	ut_run_closed_nolib(
		QUARK_POS,
		R"___(

			for(run in 0 ... 50){
				let dur = benchmark {
				}
				print(dur)
			}

		)___"
	);
}

FLOYD_LANG_PROOF("Floyd test suite", "benchmark", "", ""){
	ut_run_closed_nolib(
		QUARK_POS,
		R"___(

			for(instance in 0 ... 3){
				let dur = benchmark {
					for(e in 0 ... 2){
						let a = "hello, world!"
					}
				}
				print(dur)
			}

		)___"
	);
}



/*
//	??? Semantic analyser doesn't yet catch this problem. LLVM codegen does. Function doesnt return in all paths
FLOYD_LANG_PROOF("Floyd test suite", "benchmark-def", "Must return", ""){
	ut_verify_exception_nolib(
		QUARK_POS,
		R"(

			benchmark-def "ABC" {
				print("Running benchmark ABC")
			}

		)",
		""
	);
}
*/

FLOYD_LANG_PROOF("Floyd test suite", "benchmark-def", "Make sure def compiles", ""){
	ut_run_closed_nolib(
		QUARK_POS,
		R"(

			benchmark-def "ABC" {
				return [ benchmark_result_t(200, json("0 eleements")) ]
			}

		)"
	);
}

FLOYD_LANG_PROOF("Floyd test suite", "benchmark-def", "Access benchmark registry", ""){
	ut_verify_printout_nolib(
		QUARK_POS,
		R"(

			benchmark-def "ABC" {
				return [ benchmark_result_t(200, json("0 eleements")) ]
			}

			print(benchmark_registry)

		)",
		{
			R"___([{name="ABC", f=benchmark__ABC}])___"
		}
	);
}


	FLOYD_LANG_PROOF("Floyd test suite", "for", "Make sure loop variable is hidden outside of for-body", ""){
		ut_verify_exception_nolib(
			QUARK_POS,
			R"(

				for(i in 0 ..< 1){
					let result = 3
				}
				assert(result)
			)",
			R"___([Semantics] Undefined identifier "result". Line: 6 "assert(result)")___"
		);
	}

	FLOYD_LANG_PROOF("Floyd test suite", "benchmark-def", "Test running more than one simple benchmark_def", ""){
		ut_verify_printout_nolib(
			QUARK_POS,
			R"(

				struct test_t { json more }
				let b = test_t(json("3 monkeys"))
				print(b.more)

			)",
			{
				R"___("3 monkeys")___"
			}
		);
	}

	FLOYD_LANG_PROOF("Floyd test suite", "benchmark-def", "Test running more than one simple benchmark_def", ""){
		ut_verify_printout_nolib(
			QUARK_POS,
			R"(

				let vec = [ benchmark_result_t(200, json("0 eleements")), benchmark_result_t(300, json("3 monkeys")) ]

				for(v in 0 ..< size(vec)){
					print(to_string(v) + ": dur: " + to_string(vec[v].dur) + ", more: " + to_pretty_string(vec[v].more) + "\n")
				}

			)",
			{
				R"___(0: dur: 200, more: "0 eleements")___",
				R"___(1: dur: 300, more: "3 monkeys")___"
			}
		);
	}

	FLOYD_LANG_PROOF("Floyd test suite", "benchmark-def", "Test running more than one simple benchmark_def", ""){
		ut_verify_printout_nolib(
			QUARK_POS,
			R"(

				benchmark-def "AAA" {
					return [ benchmark_result_t(200, json("0 eleements")) ]
				}
				benchmark-def "BBB" {
					return [ benchmark_result_t(300, json("3 monkeys")) ]
				}

				for(i in 0 ..< size(benchmark_registry)){
					print(benchmark_registry[i])
				}

			)",
			{
				R"___({name="AAA", f=benchmark__AAA})___",
				R"___({name="BBB", f=benchmark__BBB})___"
			}
		);
	}

	FLOYD_LANG_PROOF("Floyd test suite", "benchmark-def", "Test running more than one simple benchmark_def", ""){
		ut_verify_printout_nolib(
			QUARK_POS,
			R"(

				struct test_t { string s }
				func test_t f(){ return test_t("cats")) }
				let a = f()
				print(a.s)

			)",
			{
				R"___(cats)___"
			}
		);
	}

	FLOYD_LANG_PROOF("Floyd test suite", "benchmark-def", "Test running more than one simple benchmark_def", ""){
		ut_verify_printout_nolib(
			QUARK_POS,
			R"(

				func json f(){ return json("12 monkeys") }
				let a = f()
				print(a)

			)",
			{
				R"___("12 monkeys")___"
			}
		);
	}

	FLOYD_LANG_PROOF("Floyd test suite", "benchmark-def", "Test running more than one simple benchmark_def", ""){
		ut_verify_printout_nolib(
			QUARK_POS,
			R"(

				struct test_t { string more }
				func test_t f(){ return test_t(string("4 monkeys")) }
				let a = f()
				print(a.more)

			)",
			{
				R"___(4 monkeys)___"
			}
		);
	}

	FLOYD_LANG_PROOF("Floyd test suite", "benchmark-def", "Test running more than one simple benchmark_def", ""){
		ut_verify_printout_nolib(
			QUARK_POS,
			R"(

				func json f(){ return json("12 monkeys") }
				let a = f()
				print(a)

			)",
			{
				R"___("12 monkeys")___"
			}
		);
	}

	FLOYD_LANG_PROOF("Floyd test suite", "benchmark-def", "Test running more than one simple benchmark_def", ""){
		ut_verify_printout_nolib(
			QUARK_POS,
			R"(

				struct test_t { json more }
				func test_t f(){ return test_t(json("12 monkeys")) }
				let a = f()
				print(a.more)

			)",
			{
				R"___("12 monkeys")___"
			}
		);
	}

FLOYD_LANG_PROOF("Floyd test suite", "benchmark-def", "Test running more than one simple benchmark_def", ""){
	ut_verify_printout_nolib(
		QUARK_POS,
		R"(

			benchmark-def "AAA" {
				return [ benchmark_result_t(200, json("0 eleements")) ]
			}
			benchmark-def "BBB" {
				return [ benchmark_result_t(300, json("3 monkeys")) ]
			}

			for(i in 0 ..< size(benchmark_registry)){
				let e = benchmark_registry[i]
				let benchmark_result = e.f()
				for(v in 0 ..< size(benchmark_result)){
					print(e.name + ": " + to_string(v) + ": dur: " + to_string(benchmark_result[v].dur) + ", more: " + to_pretty_string(benchmark_result[v].more) )
				}
			}

		)",
		{
			R"___(AAA: 0: dur: 200, more: "0 eleements")___",
			R"___(BBB: 0: dur: 300, more: "3 monkeys")___"
		}
	);
}

FLOYD_LANG_PROOF("Floyd test suite", "benchmark-def", "Test running benchmark_def with variants", ""){
	ut_verify_printout_nolib(
		QUARK_POS,
		R"(

			benchmark-def "AAA" {
				return [
					benchmark_result_t(1111, json("ONE")),
					benchmark_result_t(2222, json("TWO")),
					benchmark_result_t(3333, json("THREE"))
				]
			}

			for(i in 0 ..< size(benchmark_registry)){
				let e = benchmark_registry[i]
				let benchmark_result = e.f()
				for(v in 0 ..< size(benchmark_result)){
					print(e.name + ": " + to_string(v) + ": dur: " + to_string(benchmark_result[v].dur) + ", more: " + to_pretty_string(benchmark_result[v].more) )
				}
			}

		)",
		{
			R"___(AAA: 0: dur: 1111, more: "ONE")___",
			R"___(AAA: 1: dur: 2222, more: "TWO")___",
			R"___(AAA: 2: dur: 3333, more: "THREE")___"
		}
	);
}

FLOYD_LANG_PROOF("Floyd test suite", "benchmark-def", "Test running benchmark_def with complex 'more'", ""){
	ut_verify_printout_nolib(
		QUARK_POS,
		R"(

			benchmark-def "AAA" {
				return [
					benchmark_result_t(1111, json("ONE")),
					benchmark_result_t(2222, json( { "elements": 10, "copies": 20 } ))
				]
			}

			for(i in 0 ..< size(benchmark_registry)){
				let e = benchmark_registry[i]
				let benchmark_result = e.f()
				for(v in 0 ..< size(benchmark_result)){
					print(e.name + ": " + to_string(v) + ": dur: " + to_string(benchmark_result[v].dur) + ", more: " + to_pretty_string(benchmark_result[v].more) )
				}
			}

		)",
		{
			R"___(AAA: 0: dur: 1111, more: "ONE")___",
			R"___(AAA: 1: dur: 2222, more: { "copies": 20, "elements": 10 })___",
		}
	);
}

FLOYD_LANG_PROOF("Floyd test suite", "benchmark-def", "Example", ""){
	ut_run_closed_nolib(
		QUARK_POS,
		R"(

			benchmark-def "Linear veq" {
				mutable [benchmark_result_t] results = []
				let instances = [ 0, 1, 2, 3, 4, 10, 20, 100, 1000, 10000, 100000 ]
				for(i in 0 ..< size(instances)){
					let x = instances[i]
					mutable acc = 0

					let r = benchmark {
						//	The work to measure
						for(a in 0 ..< x){
							acc = acc + 1
						}
					}
					results = push_back(results, benchmark_result_t(r, json( { "Count": x } )))
				}
				return results
			}

			func void run_benchmarks() impure {
				for(i in 0 ..< size(benchmark_registry)){
					let e = benchmark_registry[i]
					let benchmark_result = e.f()
					for(v in 0 ..< size(benchmark_result)){
						print(e.name + ": " + to_string(v) + ": dur: " + to_string(benchmark_result[v].dur) + ", more: " + to_pretty_string(benchmark_result[v].more) )
					}
				}
			}

			run_benchmarks()
		)"
	);
}

FLOYD_LANG_PROOF("Floyd test suite", "get_benchmarks()", "", ""){
	ut_verify_printout_lib(
		QUARK_POS,
		R"(

			benchmark-def "AAA" {
				return [ benchmark_result_t(200, json("0 eleements")) ]
			}
			benchmark-def "BBB" {
				return [ benchmark_result_t(300, json("3 monkeys")) ]
			}


			print(get_benchmarks())
		)",
		{
			R"___([{module="", test="AAA"}, {module="", test="BBB"}])___",
		}
	);
}

FLOYD_LANG_PROOF("Floyd test suite", "run_benchmarks()", "", ""){
	ut_verify_printout_lib(
		QUARK_POS,
		R"(

			benchmark-def "AAA" {
				return [ benchmark_result_t(200, json("0 eleements")) ]
			}
			benchmark-def "BBB" {
				return [ benchmark_result_t(300, json("3 monkeys")) ]
			}

			print(run_benchmarks(get_benchmarks()))
		)",
		{
			R"___([{test_id={module="", test="AAA"}, result={dur=200, more="0 eleements"}}, {test_id={module="", test="BBB"}, result={dur=300, more="3 monkeys"}}])___",
		}
	);
}

FLOYD_LANG_PROOF("Floyd test suite", "run_benchmarks()", "", ""){
	ut_verify_printout_lib(
		QUARK_POS,
		R"(

			benchmark-def "AAA" {
				return [ benchmark_result_t(200, json("0 eleements")) ]
			}
			benchmark-def "BBB" {
				return [ benchmark_result_t(300, json("3 monkeys")) ]
			}

			print(make_benchmark_report(run_benchmarks(get_benchmarks())))
		)",
		{
			"| MODULE  | TEST  |    DUR|              |",
			"|---------|-------|-------|--------------|",
			"|         | AAA   | 200 ns| 0 eleements  |",
			"|         | BBB   | 300 ns| 3 monkeys    |"
		}
	);
}

FLOYD_LANG_PROOF("Floyd test suite", "Call corelib function", "", ""){
	ut_run_closed_lib(
		QUARK_POS,
		R"(

			make_benchmark_report([])

		)"
	);
}


FLOYD_LANG_PROOF("Floyd test suite", "benchmark-def", "Example", ""){
	ut_verify_printout_lib(
		QUARK_POS,
		R"(

			let test_results = [
				benchmark_result2_t(benchmark_id_t("mod1", "my"), benchmark_result_t(240, "")),
				benchmark_result2_t(benchmark_id_t("mod1", "my"), benchmark_result_t(3000, "")),
				benchmark_result2_t(benchmark_id_t("mod1", "my"), benchmark_result_t(100000, "kb/s")),
				benchmark_result2_t(benchmark_id_t("mod1", "my"), benchmark_result_t(1200000, "mb/s")),
				benchmark_result2_t(benchmark_id_t("mod1", "baggins"), benchmark_result_t(5000, "mb/s")),
				benchmark_result2_t(benchmark_id_t("mod1", "baggins"), benchmark_result_t(7000, "mb/s")),
				benchmark_result2_t(benchmark_id_t("mod1", "baggins"), benchmark_result_t(8000, "mb/s")),
				benchmark_result2_t(benchmark_id_t("mod1", "baggins"), benchmark_result_t(80000, "mb/s"))
			]

			print(make_benchmark_report(test_results))

		)",
		{
"| MODULE  | TEST     |        DUR|       |",
"|---------|----------|-----------|-------|",
"| mod1    | my       |     240 ns|       |",
"| mod1    | my       |    3000 ns|       |",
"| mod1    | my       |  100000 ns| kb/s  |",
"| mod1    | my       | 1200000 ns| mb/s  |",
"| mod1    | baggins  |    5000 ns| mb/s  |",
"| mod1    | baggins  |    7000 ns| mb/s  |",
"| mod1    | baggins  |    8000 ns| mb/s  |",
"| mod1    | baggins  |   80000 ns| mb/s  |"
		}
	);
}

#endif	//	RUN_BENCHMARK_DEF_STATEMENT__AND__BENCHMARK_EXPRESSION_TESTS







#if RUN_CORELIB_TESTS

//######################################################################################################################
//	CORE LIBRARY
//######################################################################################################################


FLOYD_LANG_PROOF("Floyd test suite", "Include library", "", ""){
	ut_run_closed_lib(
		QUARK_POS,
		R"(

			let a = 3

		)"
	);
}




FLOYD_LANG_PROOF("Floyd test suite", "detect_hardware_caps()", "", ""){
	ut_run_closed_lib(
		QUARK_POS,
		R"(

			let caps = detect_hardware_caps()
			print(to_pretty_string(caps))

		)"
	);
}



FLOYD_LANG_PROOF("Floyd test suite", "make_hardware_caps_report()", "", ""){
	ut_run_closed_lib(
		QUARK_POS,
		R"(

		)"
	);
}

FLOYD_LANG_PROOF("Floyd test suite", "make_hardware_caps_report_brief()", "", ""){
	ut_verify_printout_lib(
		QUARK_POS,
		R"(

			let caps = {
				"machdep_cpu_brand_string": json("Intel(R) Core(TM) i7-4790K CPU @ 4.00GHz"),
				"logical_processor_count": json(8),
				"mem_size": json(17179869184)
			};

			let r = make_hardware_caps_report_brief(caps);
			print(r)

		)",
		{ "Intel(R) Core(TM) i7-4790K CPU @ 4.00GHz  16 GB DRAM  8 cores" }
	);
}




FLOYD_LANG_PROOF("Floyd test suite", "make_hardware_caps_report()", "", ""){
	ut_run_closed_lib(
		QUARK_POS,
		R"(

			let caps = detect_hardware_caps()
			let r = make_hardware_caps_report(caps);
			print(r)

		)"
	);
}

FLOYD_LANG_PROOF("Floyd test suite", "make_hardware_caps_report()", "", ""){
	ut_run_closed_lib(
		QUARK_POS,
		R"(

			let s = get_current_date_and_time_string()
			print(s)

		)"
	);
}





FLOYD_LANG_PROOF("Floyd test suite", "cmath_pi", "", ""){
	ut_verify_global_result_lib(
		QUARK_POS,
		R"(

			let x = cmath_pi
			let result = x >= 3.14 && x < 3.15

		)",
		make_expected_bool(true)
	);
}



FLOYD_LANG_PROOF("Floyd test suite", "", "", ""){
	types_t temp;
	const auto a = make_vector(temp, type_t::make_string());
	const auto b = make_vector(temp, make__fsentry_t__type(temp));
	ut_verify_auto(QUARK_POS, a != b, true);
}





//////////////////////////////////////////		CORE LIBRARY - calc_string_sha1()


FLOYD_LANG_PROOF("Floyd test suite", "calc_string_sha1()", "", ""){
	ut_run_closed_lib(QUARK_POS, R"(

		let a = calc_string_sha1("Violator is the seventh studio album by English electronic music band Depeche Mode.")
//		print(to_string(a))
		assert(a.ascii40 == "4d5a137b3b1faf855872a312a184dd9a24594387")

	)");
}


//////////////////////////////////////////		CORE LIBRARY - calc_binary_sha1()


FLOYD_LANG_PROOF("Floyd test suite", "calc_binary_sha1()", "", ""){
	ut_run_closed_lib(QUARK_POS, R"(

		let bin = binary_t("Violator is the seventh studio album by English electronic music band Depeche Mode.")
		let a = calc_binary_sha1(bin)
//		print(to_string(a))
		assert(a.ascii40 == "4d5a137b3b1faf855872a312a184dd9a24594387")

	)");
}



//////////////////////////////////////////		CORE LIBRARY - get_time_of_day()

FLOYD_LANG_PROOF("Floyd test suite", "get_time_of_day()", "", ""){
	ut_run_closed_lib(QUARK_POS, R"(

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
	ut_run_closed_lib(QUARK_POS, R"(

		let int a = get_time_of_day()
		let int b = get_time_of_day()
		let int c = b - a
//		print("Delta time:" + to_string(a))

	)");
}




//////////////////////////////////////////		CORE LIBRARY - read_text_file()

/*
FLOYD_LANG_PROOF("Floyd test suite", "read_text_file()", "", ""){
	ut_run_closed_nolib(QUARK_POS, R"(

		path = get_env_path();
		a = read_text_file(path + "/Desktop/test1.json");
		print(a);

	)");
	ut_verify(QUARK_POS, vm->_print_output, { string() + R"({ "magic": 1234 })" + "\n" });
}
*/


//////////////////////////////////////////		CORE LIBRARY - write_text_file()


FLOYD_LANG_PROOF("Floyd test suite", "write_text_file()", "", ""){
	ut_run_closed_lib(QUARK_POS, R"(

		let path = get_fs_environment().desktop_dir
		write_text_file(path + "/test_out.txt", "Floyd wrote this!")

	)");
}



//////////////////////////////////////////		CORE LIBRARY - get_directory_entries()

#if 0
//??? needs clean prep/take down code.
FLOYD_LANG_PROOF("Floyd test suite", "get_fsentries_shallow()", "", ""){
	ut_verify_global_result_lib(
		QUARK_POS,
		R"(

			let result0 = get_fsentries_shallow("/Users/marcus/Desktop/")
			assert(size(result0) > 3)
//			print(to_pretty_string(result0))

			let result = typeof(result0)

		)",
		value_t::make_typeid_value(
			make_vector(make__fsentry_t__type())
		)
	);
}
#endif


//////////////////////////////////////////		CORE LIBRARY - get_fsentries_deep()

#if 0
//??? needs clean prep/take down code.
FLOYD_LANG_PROOF("Floyd test suite", "get_fsentries_deep()", "", ""){
	ut_run_closed_lib(QUARK_POS,
		R"(

			let r = get_fsentries_deep("/Users/marcus/Desktop/")
			assert(size(r) > 3)
//			print(to_pretty_string(result))

		)"
	);
}
#endif

//////////////////////////////////////////		CORE LIBRARY - get_fsentry_info()

#if 0
//??? needs clean prep/take down code.
FLOYD_LANG_PROOF("Floyd test suite", "get_fsentry_info()", "", ""){
	ut_verify_global_result_lib(
		QUARK_POS,
		R"(

			let x = get_fsentry_info("/Users/marcus/Desktop/")
//			print(to_pretty_string(x))
			let result = typeof(x)

		)",
		value_t::make_typeid_value(
			make__fsentry_info_t__type()
		)
	);
}
#endif

//////////////////////////////////////////		CORE LIBRARY - get_fs_environment()

#if 0
FLOYD_LANG_PROOF("Floyd test suite", "get_fs_environment()", "", ""){
	types_t temp;

	ut_verify_global_result_lib(
		QUARK_POS,
		R"(

			let x = get_fs_environment()
//			print(to_pretty_string(x))
			let result = typeof(x)

		)",
		make_expected_typeid(temp, make__fs_environment_t__type(temp))
	);
}
#endif



//////////////////////////////////////////		CORE LIBRARY - does_fsentry_exist()


FLOYD_LANG_PROOF("Floyd test suite", "does_fsentry_exist()", "", ""){
	ut_verify_global_result_lib(
		QUARK_POS,
		R"(

			let path = get_fs_environment().desktop_dir
			let x = does_fsentry_exist(path)
//			print(to_pretty_string(x))

			assert(x == true)
			let result = typeof(x)

		)",
		make_expected_typeid(types_t(), type_t::make_bool())
	);
}

FLOYD_LANG_PROOF("Floyd test suite", "does_fsentry_exist()", "", ""){
	ut_verify_global_result_lib(
		QUARK_POS,
		R"(

			let path = get_fs_environment().desktop_dir + "xyz"
			let result = does_fsentry_exist(path)
//			print(to_pretty_string(result))

			assert(result == false)

		)",
		make_expected_bool(false)
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

	ut_run_closed_lib(QUARK_POS,
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

		)"
	);
}


//////////////////////////////////////////		CORE LIBRARY - delete_fsentry_deep()


FLOYD_LANG_PROOF("Floyd test suite", "delete_fsentry_deep()", "", ""){
	remove_test_dir("unittest___delete_fsentry_deep", "subdir");

	ut_run_closed_lib(QUARK_POS,
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

		)"
	);
}



//////////////////////////////////////////		CORE LIBRARYCORE LIBRARY - rename_fsentry()


FLOYD_LANG_PROOF("Floyd test suite", "rename_fsentry()", "", ""){
	ut_run_closed_lib(QUARK_POS,
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

		)"
	);
}

#endif	//	RUN_CORELIB_TESTS



#if RUN_LANG_BASIC_TESTS

//######################################################################################################################
//	PARSER ERRORS
//######################################################################################################################




FLOYD_LANG_PROOF("Parser error", "", "", ""){
	ut_verify_exception_nolib(QUARK_POS, R"(		£		)", R"___([Syntax] Illegal characters. Line: 1 "£")___");
}


FLOYD_LANG_PROOF("Parser error", "", "", ""){
	ut_verify_exception_nolib(QUARK_POS, R"(		{ let a = 10		)", R"___([Syntax] Block is missing end bracket '}'. Line: 1 "{ let a = 10")___");
}

FLOYD_LANG_PROOF("Parser error", "", "", ""){
	ut_verify_exception_nolib(
		QUARK_POS,
		R"(

			[ 100, 200 }

		)",
		R"___([Syntax] Unexpected char "}" in bounded list [ ]! Line: 3 "[ 100, 200 }")___"
	);
}

FLOYD_LANG_PROOF("Parser error", "", "", ""){
	ut_verify_exception_nolib(
		QUARK_POS,
		R"(

			x = { "a": 100 ]

		)",
		R"___([Syntax] Unexpected char "]" in bounded list { }! Line: 3 "x = { "a": 100 ]")___"
	);
}


FLOYD_LANG_PROOF("Parser error", "", "", ""){
	ut_verify_exception_nolib(
		QUARK_POS,
		R"("abc\)",
		R"___([Syntax] Incomplete escape sequence in string literal: "abc"! Line: 1 ""abc\")___"
	);
}

#endif	//	RUN_LANG_BASIC_TESTS




#if RUN_CONTAINER_TESTS

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

	ut_run_closed_nolib(QUARK_POS, test_ss);
}





//######################################################################################################################
//	CONTAINER-DEF AND PROCESSES
//######################################################################################################################


//#define FLOYD_LANG_PROOF FLOYD_LANG_PROOF_VIP


FLOYD_LANG_PROOF("container-def", "Minimal floyd process demo", "", ""){
	const auto program = R"(

		container-def {
			"name": "", "tech": "", "desc": "",
			"clocks": {
				"main_clock": {
					"a": "hello"
				}
			}
		}

		func int hello__init() impure {
			send("a", "dec")
			send("a", "stop")
			return 3
		}

		func int hello__msg(int state, string message) impure {
			if(message == "dec"){
				return state - 1
			}
			else if(message == "stop"){
				exit()
				return state
			}
			return state
		}

	)";

	ut_run_closed_nolib(QUARK_POS, program);
}

#if 0
FLOYD_LANG_PROOF_VIP("container-def", "Test named type for message", "", ""){
	const auto program = R"(

		container-def {
			"name": "", "tech": "", "desc": "",
			"clocks": {
				"main_clock": {
					"a": "hello"
				}
			}
		}

		struct message_t { string t }

		func int hello__init() impure {
			send("a", message_t("stop"))
			return 3
		}

		func int hello__msg(int state, message_t message) impure {
			if(message.t == "stop"){
				exit()
			}
			return state
		}

	)";

	ut_run_closed_nolib(QUARK_POS, program);
}
#endif


FLOYD_LANG_PROOF("container-def", "Mismatch of message type: send() vs __msg()", "", ""){
	const auto program = R"(

		container-def {
			"name": "", "tech": "", "desc": "",
			"clocks": {
				"main_clock": {
					"a": "hello"
				}
			}
		}

		func int hello__init() impure {
			send("a", 1000)
			return 0
		}

		func int hello__msg(int state, string message) impure {
			return state
		}

	)";

	ut_verify_exception_nolib(
		QUARK_POS,
		program,
		"[Floyd runtime] Message type to send() is <int> but ___msg() requires message type <string>."
	);
}


FLOYD_LANG_PROOF("container-def", "run one process", "", ""){
	const auto program = R"(

		container-def {
			"name": "iphone app",
			"tech": "Swift, iOS, Xcode, Open GL",
			"desc": "Mobile shooter game for iOS.",
			"clocks": {
				"main_clock": {
					"a": "my_gui"
				}
			}
		}

		struct my_gui_state_t { int _count }

		func my_gui_state_t my_gui__init() impure {
			send("a", "dec")
			send("a", "dec")
			send("a", "dec")
			send("a", "dec")
			send("a", "stop")
			return my_gui_state_t(1000)
		}

		func my_gui_state_t my_gui__msg(my_gui_state_t state, string message) impure{
			print("received: " + to_string(message) + ", state: " + to_string(state))

			if(message == "inc"){
				return update(state, _count, state._count + 1)
			}
			else if(message == "dec"){
				return update(state, _count, state._count - 1)
			}
			else if(message == "stop"){
				exit()
				return state
			}
			else{
				assert(false)
				return state
			}
		}

	)";

	ut_run_closed_nolib(QUARK_POS, program);
}

#if 0
FLOYD_LANG_PROOF_VIP("container-def", "Test use struct as message", "", ""){
	const auto program = R"(

		container-def {
			"name": "",
			"tech": "",
			"desc": "",
			"clocks": {
				"main_clock": {
					"a": "my_gui"
				}
			}
		}

		struct my_gui_state_t { int _count }
		struct my_message_t { string data }

		func my_gui_state_t my_gui__init() impure {
			send("a", my_message_t("dec"))
			send("a", my_message_t("dec"))
			send("a", my_message_t("dec"))
			send("a", my_message_t("dec"))
			send("a", my_message_t("stop"))
			return my_gui_state_t(1000)
		}

		func my_gui_state_t my_gui__msg(my_gui_state_t state, my_message_t message) impure{
			print("received: " + to_string(message) + ", state: " + to_string(state))

			if(message.data == "inc"){
				return update(state, _count, state._count + 1)
			}
			else if(message.data == "dec"){
				return update(state, _count, state._count - 1)
			}
			else if(message.data == "stop"){
				exit()
				return state
			}
			else{
				assert(false)
				return state
			}
		}

	)";

	ut_run_closed_nolib(QUARK_POS, program);
}
#endif

FLOYD_LANG_PROOF("container-def", "run two unconnected processs", "", ""){
	const auto program = R"(

		container-def {
			"name": "",
			"tech": "",
			"desc": "",
			"clocks": {
				"main_clock": {
					"a": "my_gui"
				},
				"audio_clock": {
					"b": "my_audio"
				}
			}
		}



		struct my_gui_state_t { int _count }

		func my_gui_state_t my_gui__init() impure {
			send("a", "dec")
			send("a", "dec")
			send("a", "dec")
			send("a", "stop")
			return my_gui_state_t(1000)
		}

		func my_gui_state_t my_gui__msg(my_gui_state_t state, string message) impure {
			print("my_gui --- received: " + to_string(message) + ", state: " + to_string(state))

			if(message == "inc"){
				return update(state, _count, state._count + 1)
			}
			else if(message == "dec"){
				return update(state, _count, state._count - 1)
			}
			else if(message == "stop"){
				exit()
				return state
			}
			else{
				assert(false)
				return state
			}
		}



		struct my_audio_state_t { int _audio }

		func my_audio_state_t my_audio__init() impure {
			send("b", "process")
			send("b", "process")
			send("b", "stop")
			return my_audio_state_t(0)
		}

		func my_audio_state_t my_audio__msg(my_audio_state_t state, string message) impure {
			print("my_audio --- received: " + to_string(message) + ", state: " + to_string(state))

			if(message == "process"){
				return update(state, _audio, state._audio + 1)
			}
			else if(message == "stop"){
				exit()
				return state
			}
			else{
				assert(false)
				return state
			}
		}

	)";

	ut_run_closed_nolib(QUARK_POS, program);
}

FLOYD_LANG_PROOF("container-def", "run two CONNECTED processes", "", ""){
	const auto program = R"(

		container-def {
			"name": "",
			"tech": "",
			"desc": "",
			"clocks": {
				"main_clock": {
					"gui": "gui"
				},
				"audio_clock": {
					"audio": "audio"
				}
			}
		}



		struct gui_state_t { int _count }

		func gui_state_t gui__init() impure {
			return gui_state_t(1000)
		}

		func gui_state_t gui__msg(gui_state_t state, string message) impure {
			print("gui --- received: " + to_string(message) + ", state: " + to_string(state))

			if(message == "2"){
				send("audio", "3")
				return update(state, _count, state._count + 1)
			}
			else if(message == "4"){
				send("gui", "stop")
				send("audio", "stop")
				return update(state, _count, state._count + 10)
			}
			else if(message == "stop"){
				exit()
				return state
			}
			else{
				assert(false)
				return state
			}
		}



		struct audio_state_t { int _audio }

		func audio_state_t audio__init() impure {
			send("audio", "1")
			return audio_state_t(0)
		}

		func audio_state_t audio__msg(audio_state_t state, string message) impure {
			print("audio --- received: " + to_string(message) + ", state: " + to_string(state))

			if(message == "1"){
				send("gui", "2")
				return update(state, _audio, state._audio + 1)
			}
			else if(message == "3"){
				send("gui", "4")
				return update(state, _audio, state._audio + 4)
			}
			else if(message == "stop"){
				exit()
				return state
			}
			else{
				assert(false)
				return state
			}
		}

	)";

	ut_run_closed_nolib(QUARK_POS, program);
}

#endif	//	RUN_CONTAINER_TESTS





#if RUN_EXAMPLE_AND_DOCS_TESTS

//######################################################################################################################
//	RUN ALL EXAMPLE PROGRAMS -- VERIFY THEY WORK
//######################################################################################################################




FLOYD_LANG_PROOF("Floyd test suite", "hello_world.floyd", "", ""){
	const auto path = get_working_dir() + "/examples/hello_world.floyd";
	const auto program = read_text_file(path);

	ut_run_closed_lib(QUARK_POS, program);
}

FLOYD_LANG_PROOF("Floyd test suite", "game_of_life.floyd", "", ""){
	const auto path = get_working_dir() + "/examples/game_of_life.floyd";
	const auto program = read_text_file(path);

	ut_run_closed_lib(QUARK_POS, program);
}





//######################################################################################################################
//	QUICK REFERENCE SNIPPETS -- VERIFY THEY WORK
//######################################################################################################################






FLOYD_LANG_PROOF("QUICK REFERENCE SNIPPETS", "TERNARY OPERATOR", "", ""){
	ut_run_closed_nolib(QUARK_POS, R"(

//	Snippets setup
let b = ""



let a = b == "true" ? true : false

	)");
}

FLOYD_LANG_PROOF("QUICK REFERENCE SNIPPETS", "COMMENTS", "", ""){
	ut_run_closed_nolib(QUARK_POS, R"(

/* Comment can span lines. */

let a = 10; // To end of line

	)");
}



FLOYD_LANG_PROOF("QUICK REFERENCE SNIPPETS", "LOCALS", "", ""){
	ut_run_closed_nolib(QUARK_POS, R"(

let a = 10
mutable b = 10
b = 11

	)");
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
	ut_run_closed_nolib(QUARK_POS, R"(

for (index in 1 ... 5) {
	print(index)
}
for (index in 1  ..< 5) {
	print(index)
}

	)");
}


FLOYD_LANG_PROOF("QUICK REFERENCE SNIPPETS", "IF ELSE", "", ""){
	ut_run_closed_nolib(QUARK_POS, R"(

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

	)");
}


FLOYD_LANG_PROOF("QUICK REFERENCE SNIPPETS", "BOOL", "", ""){
	ut_run_closed_nolib(QUARK_POS, R"(

let a = true
if(a){
}

	)");
}

FLOYD_LANG_PROOF("QUICK REFERENCE SNIPPETS", "STRING", "", ""){
	ut_run_closed_nolib(QUARK_POS, R"(

let s1 = "Hello, world!"
let s2 = "Title: " + s1
assert(s2 == "Title: Hello, world!")
assert(s1[0] == 72) // ASCII for 'H'
assert(size(s1) == 13)
assert(subset(s1, 1, 4) == "ell")
let s4 = to_string(12003)

	)");
}




FLOYD_LANG_PROOF("QUICK REFERENCE SNIPPETS", "FUNCTION", "", ""){
	ut_run_closed_nolib(QUARK_POS, R"(

func string f(double a, string s){
	return to_string(a) + ":" + s
}

let a = f(3.14, "km")

	)");
}

FLOYD_LANG_PROOF("QUICK REFERENCE SNIPPETS", "IMPURE FUNCTION", "", ""){
	ut_verify_mainfunc_return_nolib(
	QUARK_POS,
	R"(

func int main([string] args) impure {
	return 1
}

	)", {}, 1);
}



FLOYD_LANG_PROOF("QUICK REFERENCE SNIPPETS", "STRUCT", "", ""){
	ut_run_closed_nolib(QUARK_POS, R"(

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

	)");
}


FLOYD_LANG_PROOF("QUICK REFERENCE SNIPPETS", "VECTOR", "", ""){
	ut_run_closed_nolib(QUARK_POS, R"(

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

	)");
}


FLOYD_LANG_PROOF("QUICK REFERENCE SNIPPETS", "DICTIONARY", "", ""){
	ut_run_closed_nolib(QUARK_POS, R"(

let a = { "one": 1, "two": 2 }
assert(a["one"] == 1)

let b = update(a, "one", 10)
assert(a == { "one": 1, "two": 2 })
assert(b == { "one": 10, "two": 2 })

	)");
}


FLOYD_LANG_PROOF("QUICK REFERENCE SNIPPETS", "json", "", ""){
	ut_run_closed_nolib(QUARK_POS, R"(

let json a = {
	"one": 1,
	"three": "three",
	"five": { "A": 10, "B": 20 }
}

	)");
}




FLOYD_LANG_PROOF("QUICK REFERENCE SNIPPETS", "MAP", "", ""){
	ut_run_closed_nolib(QUARK_POS, R"(

		func int f(int v, int c){
			return 1000 + v
		}
		let r = map([ 10, 11, 12 ], f, 0)
		assert(r == [ 1010, 1011, 1012 ])

	)");
}




//######################################################################################################################
//	MANUAL SNIPPETS -- VERIFY THEY WORK
//######################################################################################################################




FLOYD_LANG_PROOF("MANUAL SNIPPETS", "subset()", "", ""){
	ut_run_closed_nolib(QUARK_POS, R"(

		assert(subset("hello", 2, 4) == "ll")
		assert(subset([ 10, 20, 30, 40 ], 1, 3 ) == [ 20, 30 ])

	)");
}





#if 0
Crashes at runtime when k_max_samples_count = 100000
FLOYD_LANG_PROOF("Floyd test suite", "", "", ""){
	ut_run_closed_lib(
		QUARK_POS,
		R"___(

				mutable [benchmark_result_t] results = []
//				let instances = [ 0, 1, 2, 3, 4, 10, 20, 100, 1000 ]
				let instances = [ 0, 1, 2, 3, 4, 10, 20 ]
//				let instances = [ 20 ]
//				let instances = [ 10, 20 ]
				for(instance in 0 ..< size(instances)){
					let count = instances[instance]
					let [int] start = []

					let r = benchmark {
						//	The work to measure
						for(a in 0 ..< count){
							let b = push_back(start, 0)
						}
					}
					results = push_back(results, benchmark_result_t(r, json( { "Count": count, "Per Element": count > 0 ? r / count : -1 } )))
				}

		)___"
	);
}
#endif

#if 0
FLOYD_LANG_PROOF("Floyd test suite", "dict hamt performance()", "", ""){
	ut_run_closed_lib(
		QUARK_POS,
		R"___(

			benchmark-def "dict hamt push_back()" {
				mutable [benchmark_result_t] results = []
				let instances = [ 0, 1, 2, 3, 4, 10, 20, 100, 1000 ]
				for(instance in 0 ..< size(instances)){
					let count = instances[instance]
					let [int] start = []

					let r = benchmark {
						//	The work to measure
						for(a in 0 ..< count){
							let b = push_back(start, 0)
						}
					}
					results = push_back(results, benchmark_result_t(r, json( { "Count": count, "Per Element": count > 0 ? r / count : -1 } )))
				}
				return results
			}

			print(make_benchmark_report(run_benchmarks(get_benchmarks())))

		)___"
	);
}
#endif


#endif	//	RUN_EXAMPLE_AND_DOCS_TESTS









/*
FLOYD_LANG_PROOF("Floyd test suite", "OPTIMZATION SETTINGS" "Fibonacci 10", "", ""){
	ut_run_closed_nolib(
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

		)"
	);
}
*/




//######################################################################################################################
//	REGRESSION TEST
//######################################################################################################################

FLOYD_LANG_PROOF("REGRESSION TEST", "a inited after ifelsejoin, don't delete it inside *then*", "", ""){
	ut_verify_printout_nolib(
		QUARK_POS,
		R"(

			func int f(){
				let a = "aaa"
				if(true){
					let b = "bbb"
					return 100
					let c = "ccc"
				}
				else {
					let d = "ddd"
					return 200
					let e = "eee"
				}
				let f = "fff"
				return 300
			}

			f()

		)",
		{}
	);
}

FLOYD_LANG_PROOF("REGRESSION TEST", "a inited after ifelsejoin, don't delete it inside *then*", "", ""){
	ut_verify_printout_nolib(
		QUARK_POS,
		R"(

			func int f(int command){
				//	Here we need to destruct locals before returning, but a has not been inited yet = can't be destructed!
				return 100

				let a = "look"
				return 20
			}

			f(1)

		)",
		{}
	);
}




//######################################################################################################################
//	DEMOS
//######################################################################################################################






FLOYD_LANG_PROOF("", "Demo DEEP BY VALUE", "", ""){
	ut_run_closed_nolib(
		QUARK_POS,
		R"___(


			struct vec2d_t { double x; double y }
			func vec2d_t make_zero_vec2d(){ return vec2d_t(0.0, 0.0) }

			struct game_object_t {
				vec2d_t pos
				vec2d_t velocity
				string key
				[game_object_t] children
			}

			let a = game_object_t(make_zero_vec2d(), vec2d_t(0.0, 1.0), "player", [])
			let b = game_object_t(make_zero_vec2d(), make_zero_vec2d(), "world", [ a ])

			assert(a != b)
			assert(a > b)

			let [game_object_t] c = [ a, b ]
			let [game_object_t] d = push_back(c, a)
			assert(c != d)


		)___"
	);
}

#if 0
FLOYD_LANG_PROOF("", "Demo DEEP BY VALUE", "", ""){
	ut_run_closed_nolib(
		QUARK_POS,
		R"___(


			container-def {
				"name": "iphone app",
				"tech": "Swift, iOS, Xcode, Open GL",
				"desc": "Mobile shooter game for iOS.",
				"clocks": {
					"main_clock": {
						"gui": "my_gui"
					},
					"audio_clock": {
						"audio": "my_audio"
					}
				}
			}

			struct doc_t { [double] audio }

			struct gui_message_t { string type }

			struct audio_engine_t { int audio ; doc_t doc }
			struct audio_message_t { string type ; doc_t doc }

			func audio_engine_t my_audio__init() impure {
				return audio_engine_t(0, doc_t([]))
			}

			func audio_engine_t my_audio__msg(audio_engine_t state, audio_message_t m) impure {
				if(m.type == "start_note"){
					return update(state, audio, state.audio + 1)
				}
				else if(m.type == "buffer_switch"){
					return update(state, audio, state.audio + 1)
				}
				else if(m.type == "exit"){
					exit()
				}
				else{
					assert(false)
				}
				return state
			}


			struct gui_t { int count ; doc_t doc }

			func gui_t my_gui__init() impure {
				send("gui", gui_message_t("key_press"))
				send("gui", gui_message_t("key_press"))
				send("gui", gui_message_t("quit"))
				return gui_t(1000, doc_t([ 0.0, 1.0, 2.0 ]))
			}

			func gui_t my_gui__msg(gui_t state, gui_message_t m) impure {
				if(m.type == "key_press"){
					send("audio", audio_message_t("start_note", state.doc))
					return update(state, count, state.count + 1)
				}
				else if(m.type == "quit"){
					send("audio", audio_message_t("exit", state.doc))
					exit()
				}
				else{
					assert(false)
				}
				return state
			}


		)___"
	);
}
#endif


#if 0
FLOYD_LANG_PROOF("generics", "", "", ""){
	ut_run_closed_nolib(
		QUARK_POS,
		R"(

			func string f(any value){
				return to_string(value)
			}

			print(f(10))
			print(f("hello"))

		)"
	);
}
#endif


//######################################################################################################################
//	NETWORK COMPONENT
//######################################################################################################################



#if 0
FLOYD_LANG_PROOF_VIP("network component", "", "", ""){
	ut_run_closed_lib(
		QUARK_POS,
		R"(
			let c = network_component_t(666)

			let request_line = http_request_line_t ( "GET", "/index.html", "HTTP/1.0" )
			let a = http_request_t ( request_line, [], "" )
			let ip = lookup_host_from_name("example.com").addresses_IPv4[0]
			let dest = ip_address_and_port_t(ip, 80)
			let r = execute_http_request(c, dest, pack_http_request(a))
			print(r)

		)"
	);
}
#endif



