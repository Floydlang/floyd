//
//  floyd_test_suite.cpp
//  FloydSpeak
//
//  Created by Marcus Zetterquist on 2018-01-21.
//  Copyright © 2018 Marcus Zetterquist. All rights reserved.
//

#include "floyd_test_suite.hpp"


#include "floyd_interpreter.h"
#include "parser_value.h"
#include "expressions.h"

#include <string>
#include <vector>



using std::vector;
using std::string;

using floyd::value_t;
using floyd::run_global;
using floyd::expression_t;
using floyd::program_to_ast2;
using floyd::interpreter_t;
using floyd::find_global_symbol;
using floyd::statement_result_t;



///////////////////////////////////////////////////////////////////////////////////////
//	FLOYD LANGUAGE TEST SUITE
///////////////////////////////////////////////////////////////////////////////////////



void test__run_init__check_result(const std::string& program, const value_t& expected_result){
	const auto result = run_global(program);
	const auto result_value = result._call_stack[0]->_values["result"];
	ut_compare_jsons(
		expression_to_json(expression_t::make_literal(result_value.first)),
		expression_to_json(expression_t::make_literal(expected_result))
	);
}
void test__run_init2(const std::string& program){
	const auto result = run_global(program);
}


void test__run_main(const std::string& program, const vector<floyd::value_t>& args, const value_t& expected_return){
	const auto result = run_main(program, args);
	ut_compare_jsons(
		expression_to_json(expression_t::make_literal(result.second._output)),
		expression_to_json(expression_t::make_literal(expected_return))
	);
}

void ut_compare_stringvects(const vector<string>& result, const vector<string>& expected){
	if(result != expected){
		if(result.size() != expected.size()){
			QUARK_TRACE("Vector are different sizes!");
		}
		for(int i = 0 ; i < result.size() ; i++){
			QUARK_SCOPED_TRACE(std::to_string(i));

			quark::ut_compare(result[i], expected[i]);
/*
			if(result[i] != expected[i]){
				QUARK_TRACE("  result: \"" + result[i] + "\"");
				QUARK_TRACE("expected: \"" + expected[i] + "\"");
			}
*/

		}
	}
}


//////////////////////////		TEST GLOBAL CONSTANTS


QUARK_UNIT_TESTQ("Floyd test suite", "Global int variable"){
	test__run_init__check_result("int result = 123;", value_t(123));
}

QUARK_UNIT_TESTQ("Floyd test suite", "bool constant expression"){
	test__run_init__check_result("bool result = true;", value_t(true));
}
QUARK_UNIT_TESTQ("Floyd test suite", "bool constant expression"){
	test__run_init__check_result("bool result = false;", value_t(false));
}

QUARK_UNIT_TESTQ("Floyd test suite", "int constant expression"){
	test__run_init__check_result("int result = 123;", value_t(123));
}

QUARK_UNIT_TESTQ("Floyd test suite", "float constant expression"){
	test__run_init__check_result("float result = 3.5;", value_t(float(3.5)));
}

QUARK_UNIT_TESTQ("Floyd test suite", "string constant expression"){
	test__run_init__check_result("string result = \"xyz\";", value_t("xyz"));
}


//////////////////////////		BASIC EXPRESSIONS


QUARK_UNIT_TESTQ("Floyd test suite", "+") {
	test__run_init__check_result("int result = 1 + 2;", value_t(3));
}
QUARK_UNIT_TESTQ("Floyd test suite", "+") {
	test__run_init__check_result("int result = 1 + 2 + 3;", value_t(6));
}
QUARK_UNIT_TESTQ("Floyd test suite", "*") {
	test__run_init__check_result("int result = 3 * 4;", value_t(12));
}

QUARK_UNIT_TESTQ("Floyd test suite", "parant") {
	test__run_init__check_result("int result = (3 * 4) * 5;", value_t(60));
}

//??? test all types, like [int] etc.

QUARK_UNIT_TESTQ("Floyd test suite", "Expression statement") {
	const auto r = run_global("print(5);");
	QUARK_UT_VERIFY((r._print_output == vector<string>{ "5" }));
}

QUARK_UNIT_TESTQ("Floyd test suite", "Deduced bind") {
	const auto r = run_global("a = 10;print(a);");
	QUARK_UT_VERIFY((r._print_output == vector<string>{ "10" }));
}


//////////////////////////		BASIC EXPRESSIONS - CONDITIONAL EXPRESSION


QUARK_UNIT_TESTQ("run_main()", "conditional expression"){
	test__run_init__check_result("int result = true ? 1 : 2;", value_t(1));
}
QUARK_UNIT_TESTQ("run_main()", "conditional expression"){
	test__run_init__check_result("int result = false ? 1 : 2;", value_t(2));
}

//??? Test truthness off all variable types: strings, floats

QUARK_UNIT_TESTQ("run_main()", "conditional expression"){
	test__run_init__check_result("string result = true ? \"yes\" : \"no\";", value_t("yes"));
}
QUARK_UNIT_TESTQ("run_main()", "conditional expression"){
	test__run_init__check_result("string result = false ? \"yes\" : \"no\";", value_t("no"));
}


//////////////////////////		BASIC EXPRESSIONS - PARANTHESES



QUARK_UNIT_TESTQ("evaluate_expression()", "Parentheses") {
	test__run_init__check_result("int result = 5*(4+4+1);", value_t(45));
}
QUARK_UNIT_TESTQ("evaluate_expression()", "Parentheses") {
	test__run_init__check_result("int result = 5*(2*(1+3)+1);", value_t(45));
}
QUARK_UNIT_TESTQ("evaluate_expression()", "Parentheses") {
	test__run_init__check_result("int result = 5*((1+3)*2+1);", value_t(45));
}

QUARK_UNIT_TESTQ("evaluate_expression()", "Sign before parentheses") {
	test__run_init__check_result("int result = -(2+1)*4;", value_t(-12));
}
QUARK_UNIT_TESTQ("evaluate_expression()", "Sign before parentheses") {
	test__run_init__check_result("int result = -4*(2+1);", value_t(-12));
}


//////////////////////////		BASIC EXPRESSIONS - SPACES


QUARK_UNIT_TESTQ("evaluate_expression()", "Spaces") {
	test__run_init__check_result("int result = 5 * ((1 + 3) * 2 + 1) ;", value_t(45));
}
QUARK_UNIT_TESTQ("evaluate_expression()", "Spaces") {
	test__run_init__check_result("int result = 5 - 2 * ( 3 ) ;", value_t(-1));
}
QUARK_UNIT_TESTQ("evaluate_expression()", "Spaces") {
	test__run_init__check_result("int result =  5 - 2 * ( ( 4 )  - 1 ) ;", value_t(-1));
}


//////////////////////////		BASIC EXPRESSIONS - FLOAT



QUARK_UNIT_TESTQ("evaluate_expression()", "Fractional numbers") {
	test__run_init__check_result("float result = 5.5/5.0;", value_t(1.1f));
}
QUARK_UNIT_TESTQ("evaluate_expression()", "Fractional numbers") {
//	test__run_init__check_result("int result = 1/5e10") == 2e-11);
}
QUARK_UNIT_TESTQ("evaluate_expression()", "Fractional numbers") {
	test__run_init__check_result("float result = (4.0-3.0)/(4.0*4.0);", value_t(0.0625f));
}
QUARK_UNIT_TESTQ("evaluate_expression()", "Fractional numbers") {
	test__run_init__check_result("float result = 1.0/2.0/2.0;", value_t(0.25f));
}
QUARK_UNIT_TESTQ("evaluate_expression()", "Fractional numbers") {
	test__run_init__check_result("float result = 0.25 * .5 * 0.5;", value_t(0.0625f));
}
QUARK_UNIT_TESTQ("evaluate_expression()", "Fractional numbers") {
	test__run_init__check_result("float result = .25 / 2.0 * .5;", value_t(0.0625f));
}

//////////////////////////		BASIC EXPRESSIONS - EDGE CASES


QUARK_UNIT_TESTQ("evaluate_expression()", "Repeated operators") {
	test__run_init__check_result("int result = 1+-2;", value_t(-1));
}
QUARK_UNIT_TESTQ("evaluate_expression()", "Repeated operators") {
	test__run_init__check_result("int result = --2;", value_t(2));
}
QUARK_UNIT_TESTQ("evaluate_expression()", "Repeated operators") {
	test__run_init__check_result("int result = 2---2;", value_t(0));
}
QUARK_UNIT_TESTQ("evaluate_expression()", "Repeated operators") {
	test__run_init__check_result("int result = 2-+-2;", value_t(4));
}


//////////////////////////		BASIC EXPRESSIONS - BOOL


QUARK_UNIT_TESTQ("evaluate_expression()", "Bool") {
	test__run_init__check_result("bool result = true;", value_t(true));
}
QUARK_UNIT_TESTQ("evaluate_expression()", "Bool") {
	test__run_init__check_result("bool result = false;", value_t(false));
}


//////////////////////////		BASIC EXPRESSIONS - CONDITIONAL EXPRESSION



QUARK_UNIT_TESTQ("evaluate_expression()", "?:") {
	test__run_init__check_result("int result = true ? 4 : 6;", value_t(4));
}
QUARK_UNIT_TESTQ("evaluate_expression()", "?:") {
	test__run_init__check_result("int result = false ? 4 : 6;", value_t(6));
}

QUARK_UNIT_TESTQ("evaluate_expression()", "?:") {
	test__run_init__check_result("int result = 1==3 ? 4 : 6;", value_t(6));
}
QUARK_UNIT_TESTQ("evaluate_expression()", "?:") {
	test__run_init__check_result("int result = 3==3 ? 4 : 6;", value_t(4));
}

QUARK_UNIT_TESTQ("evaluate_expression()", "?:") {
	test__run_init__check_result("int result = 3==3 ? 2 + 2 : 2 * 3;", value_t(4));
}

QUARK_UNIT_TESTQ("evaluate_expression()", "?:") {
	test__run_init__check_result("int result = 3==1+2 ? 2 + 2 : 2 * 3;", value_t(4));
}


//////////////////////////		BASIC EXPRESSIONS - OPERATOR ==


QUARK_UNIT_TESTQ("evaluate_expression()", "==") {
	test__run_init__check_result("bool result = 1 == 1;", value_t(true));
}
QUARK_UNIT_TESTQ("evaluate_expression()", "==") {
	test__run_init__check_result("bool result = 1 == 2;", value_t(false));
}
QUARK_UNIT_TESTQ("evaluate_expression()", "==") {
	test__run_init__check_result("bool result = 1.3 == 1.3;", value_t(true));
}
QUARK_UNIT_TESTQ("evaluate_expression()", "==") {
	test__run_init__check_result("bool result = \"hello\" == \"hello\";", value_t(true));
}
QUARK_UNIT_TESTQ("evaluate_expression()", "==") {
	test__run_init__check_result("bool result = \"hello\" == \"bye\";", value_t(false));
}


//////////////////////////		BASIC EXPRESSIONS - OPERATOR <


QUARK_UNIT_TESTQ("evaluate_expression()", "<") {
	test__run_init__check_result("bool result = 1 < 2;", value_t(true));
}
QUARK_UNIT_TESTQ("evaluate_expression()", "<") {
	test__run_init__check_result("bool result = 5 < 2;", value_t(false));
}
QUARK_UNIT_TESTQ("evaluate_expression()", "<") {
	test__run_init__check_result("bool result = 0.3 < 0.4;", value_t(true));
}
QUARK_UNIT_TESTQ("evaluate_expression()", "<") {
	test__run_init__check_result("bool result = 1.5 < 0.4;", value_t(false));
}
QUARK_UNIT_TESTQ("evaluate_expression()", "<") {
	test__run_init__check_result("bool result = \"adwark\" < \"boat\";", value_t(true));
}
QUARK_UNIT_TESTQ("evaluate_expression()", "<") {
	test__run_init__check_result("bool result = \"boat\" < \"adwark\";", value_t(false));
}

//////////////////////////		BASIC EXPRESSIONS - OPERATOR &&

QUARK_UNIT_TESTQ("evaluate_expression()", "&&") {
	test__run_init__check_result("bool result = false && false;", value_t(false));
}
QUARK_UNIT_TESTQ("evaluate_expression()", "&&") {
	test__run_init__check_result("bool result = false && true;", value_t(false));
}
QUARK_UNIT_TESTQ("evaluate_expression()", "&&") {
	test__run_init__check_result("bool result = true && false;", value_t(false));
}
QUARK_UNIT_TESTQ("evaluate_expression()", "&&") {
	test__run_init__check_result("bool result = true && true;", value_t(true));
}

QUARK_UNIT_TESTQ("evaluate_expression()", "&&") {
	test__run_init__check_result("bool result = false && false && false;", value_t(false));
}
QUARK_UNIT_TESTQ("evaluate_expression()", "&&") {
	test__run_init__check_result("bool result = false && false && true;", value_t(false));
}
QUARK_UNIT_TESTQ("evaluate_expression()", "&&") {
	test__run_init__check_result("bool result = false && true && false;", value_t(false));
}
QUARK_UNIT_TESTQ("evaluate_expression()", "&&") {
	test__run_init__check_result("bool result = false && true && true;", value_t(false));
}
QUARK_UNIT_TESTQ("evaluate_expression()", "&&") {
	test__run_init__check_result("bool result = true && false && false;", value_t(false));
}
QUARK_UNIT_TESTQ("evaluate_expression()", "&&") {
	test__run_init__check_result("bool result = true && true && false;", value_t(false));
}
QUARK_UNIT_TESTQ("evaluate_expression()", "&&") {
	test__run_init__check_result("bool result = true && true && true;", value_t(true));
}

//////////////////////////		BASIC EXPRESSIONS - OPERATOR ||

QUARK_UNIT_TESTQ("evaluate_expression()", "||") {
	test__run_init__check_result("bool result = false || false;", value_t(false));
}
QUARK_UNIT_TESTQ("evaluate_expression()", "||") {
	test__run_init__check_result("bool result = false || true;", value_t(true));
}
QUARK_UNIT_TESTQ("evaluate_expression()", "||") {
	test__run_init__check_result("bool result = true || false;", value_t(true));
}
QUARK_UNIT_TESTQ("evaluate_expression()", "||") {
	test__run_init__check_result("bool result = true || true;", value_t(true));
}


QUARK_UNIT_TESTQ("evaluate_expression()", "||") {
	test__run_init__check_result("bool result = false || false || false;", value_t(false));
}
QUARK_UNIT_TESTQ("evaluate_expression()", "||") {
	test__run_init__check_result("bool result = false || false || true;", value_t(true));
}
QUARK_UNIT_TESTQ("evaluate_expression()", "||") {
	test__run_init__check_result("bool result = false || true || false;", value_t(true));
}
QUARK_UNIT_TESTQ("evaluate_expression()", "||") {
	test__run_init__check_result("bool result = false || true || true;", value_t(true));
}
QUARK_UNIT_TESTQ("evaluate_expression()", "||") {
	test__run_init__check_result("bool result = true || false || false;", value_t(true));
}
QUARK_UNIT_TESTQ("evaluate_expression()", "||") {
	test__run_init__check_result("bool result = true || false || true;", value_t(true));
}
QUARK_UNIT_TESTQ("evaluate_expression()", "||") {
	test__run_init__check_result("bool result = true || true || false;", value_t(true));
}
QUARK_UNIT_TESTQ("evaluate_expression()", "||") {
	test__run_init__check_result("bool result = true || true || true;", value_t(true));
}


//////////////////////////		BASIC EXPRESSIONS - ERRORS


QUARK_UNIT_TESTQ("evaluate_expression()", "Type mismatch") {
	try{
		test__run_init__check_result("int result = true;", value_t(1));
		QUARK_TEST_VERIFY(false);
	}
	catch(const std::runtime_error& e){
		QUARK_TEST_VERIFY(string(e.what()) == "Types not compatible in bind.");
	}
}


QUARK_UNIT_TESTQ("evaluate_expression()", "Division by zero") {
	try{
		test__run_init__check_result("int result = 2/0;", value_t());
		QUARK_TEST_VERIFY(false);
	}
	catch(const std::runtime_error& e){
		QUARK_TEST_VERIFY(string(e.what()) == "EEE_DIVIDE_BY_ZERO");
	}
}

QUARK_UNIT_TESTQ("evaluate_expression()", "Division by zero"){
	try{
		test__run_init__check_result("int result = 3+1/(5-5)+4;", value_t());
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








//////////////////////////		MINIMAL PROGRAMS



QUARK_UNIT_TESTQ("run_main", "Can make and read global int"){
	test__run_main(
		"int test = 123;"
		"int main(){\n"
		"	return test;"
		"}\n",
		{},
		value_t(123)
	);
}

QUARK_UNIT_TESTQ("run_main()", "minimal program 2"){
	test__run_main(
		"string main(string args){\n"
		"	return \"123\" + \"456\";\n"
		"}\n",
		vector<value_t>{value_t("program_name 1 2 3 4")},
		value_t("123456")
	);
}

QUARK_UNIT_TESTQ("run_main()", ""){
	test__run_main("bool main(){ return 4 < 5; }", {}, value_t(true));
}
QUARK_UNIT_TESTQ("run_main()", ""){
	test__run_main("bool main(){ return 5 < 4; }", {}, value_t(false));
}
QUARK_UNIT_TESTQ("run_main()", ""){
	test__run_main("bool main(){ return 4 <= 4; }", {}, value_t(true));
}

QUARK_UNIT_TESTQ("call_function()", "minimal program"){
	auto ast = program_to_ast2(
		"int main(string args){\n"
		"	return 3 + 4;\n"
		"}\n"
	);
	auto vm = interpreter_t(ast);
	const auto f = find_global_symbol(vm, "main");
	const auto result = call_function(vm, f, vector<value_t>{ value_t("program_name 1 2 3") });
	QUARK_TEST_VERIFY(result.second == statement_result_t::make_return_unwind(value_t(7)));
}

QUARK_UNIT_TESTQ("call_function()", "minimal program 2"){
	auto ast = program_to_ast2(
		"string main(string args){\n"
		"	return \"123\" + \"456\";\n"
		"}\n"
	);
	auto vm = interpreter_t(ast);
	const auto f = find_global_symbol(vm, "main");
	const auto result = call_function(vm, f, vector<value_t>{ value_t("program_name 1 2 3") });
	QUARK_TEST_VERIFY(result.second == statement_result_t::make_return_unwind(value_t("123456")));
}


//////////////////////////		CALL FUNCTIONS



QUARK_UNIT_TESTQ("call_function()", "define additional function, call it several times"){
	auto ast = program_to_ast2(
		"int myfunc(){ return 5; }\n"
		"int main(string args){\n"
		"	return myfunc() + myfunc() * 2;\n"
		"}\n"
	);
	auto vm = interpreter_t(ast);
	const auto f = find_global_symbol(vm, "main");
	const auto result = call_function(vm, f, vector<value_t>{ value_t("program_name 1 2 3") });
	QUARK_TEST_VERIFY(result.second == statement_result_t::make_return_unwind(value_t(15)));
}

QUARK_UNIT_TESTQ("call_function()", "use function inputs"){
	auto ast = program_to_ast2(
		"string main(string args){\n"
		"	return \"-\" + args + \"-\";\n"
		"}\n"
	);
	auto vm = interpreter_t(ast);
	const auto f = find_global_symbol(vm, "main");
	const auto result = call_function(vm, f, vector<value_t>{ value_t("xyz") });
	QUARK_TEST_VERIFY(result.second == statement_result_t::make_return_unwind(value_t("-xyz-")));

	const auto result2 = call_function(vm, f, vector<value_t>{ value_t("Hello, world!") });
	QUARK_TEST_VERIFY(result2.second == statement_result_t::make_return_unwind(value_t("-Hello, world!-")));
}


//////////////////////////		USE LOCAL VARIABLES IN FUNCTION


QUARK_UNIT_TESTQ("call_function()", "use local variables"){
	auto ast = program_to_ast2(
		"string myfunc(string t){ return \"<\" + t + \">\"; }\n"
		"string main(string args){\n"
		"	 string a = \"--\"; string b = myfunc(args) ; return a + args + b + a;\n"
		"}\n"
	);
	auto vm = interpreter_t(ast);
	const auto f = find_global_symbol(vm, "main");
	const auto result = call_function(vm, f, vector<value_t>{ value_t("xyz") });
//	QUARK_TEST_VERIFY(*result.second == value_t("--xyz<xyz>--"));
	QUARK_TEST_VERIFY(result.second == statement_result_t::make_return_unwind(value_t("--xyz<xyz>--")));

	const auto result2 = call_function(vm, f, vector<value_t>{ value_t("123") });
//	QUARK_TEST_VERIFY(*result2.second == value_t("--123<123>--"));
	QUARK_TEST_VERIFY(result2.second == statement_result_t::make_return_unwind(value_t("--123<123>--")));
}



//////////////////////////		MUTATE VARIABLES



QUARK_UNIT_TESTQ("call_function()", "mutate local"){
	auto r = run_global(
		R"(
			mutable a = 1;
			a = 2;
			print(a);
		)"
	);
	QUARK_UT_VERIFY((r._print_output == vector<string>{ "2" }));
}

QUARK_UNIT_TESTQ("run_init()", "increment a mutable"){
	const auto r = run_global(
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
		const auto vm = run_global(R"(
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
		const auto vm = run_global(R"(
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
			const auto r = run_global(R"(
				mutable a = 7;
				a = 8;
				{
					print(a);
				}
			)");
			QUARK_UT_VERIFY((r._print_output == vector<string>{ "8" }));
		}

		QUARK_UNIT_TEST("run_main()", "test mutating from a subscope", "", ""){
			const auto r = run_global(R"(
				a = 7;
				print(a);
			)");
			QUARK_UT_VERIFY((r._print_output == vector<string>{ "7" }));
		}


		QUARK_UNIT_TEST("run_main()", "test mutating from a subscope", "", ""){
			const auto r = run_global(R"(
				a = 7;
				{
				}
				print(a);
			)");
			QUARK_UT_VERIFY((r._print_output == vector<string>{ "7" }));
		}

QUARK_UNIT_TEST("run_main()", "test mutating from a subscope", "", ""){
	const auto r = run_global(R"(
		mutable a = 7;
		{
			a = 8;
		}
		print(a);
	)");
	QUARK_UT_VERIFY((r._print_output == vector<string>{ "8" }));
}



//////////////////////////		RETURN STATEMENT - ADVANCED USAGE


QUARK_UNIT_TESTQ("call_function()", "return from middle of function"){
	auto r = run_global(
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
	auto r = run_global(
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
	auto r = run_global(
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
	auto r = run_global(
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






//////////////////////////		HOST FUNCTION - to_string()



QUARK_UNIT_TESTQ("run_init()", ""){
	test__run_init__check_result(
		R"(
			string result = to_string(145);
		)",
		value_t("145")
	);
}
QUARK_UNIT_TESTQ("run_init()", ""){
	test__run_init__check_result(
		R"(
			string result = to_string(3.1);
		)",
		value_t("3.100000")
	);
}


//////////////////////////		HOST FUNCTION - print()



QUARK_UNIT_TEST("run_global()", "Print Hello, world!", "", ""){
	const auto r = run_global(
		R"(
			print("Hello, World!");
		)"
	);
	QUARK_UT_VERIFY((r._print_output == vector<string>{ "Hello, World!" }));
}


QUARK_UNIT_TEST("run_global()", "Test that VM state (print-log) escapes block!", "", ""){
	const auto r = run_global(
		R"(
			{
				print("Hello, World!");
			}
		)"
	);
	QUARK_UT_VERIFY((r._print_output == vector<string>{ "Hello, World!" }));
}

QUARK_UNIT_TESTQ("run_global()", "Test that VM state (print-log) escapes IF!"){
	const auto r = run_global(
		R"(
			if(true){
				print("Hello, World!");
			}
		)"
	);
	QUARK_UT_VERIFY((r._print_output == vector<string>{ "Hello, World!" }));
}


//////////////////////////		HOST FUNCTION - assert()

QUARK_UNIT_TESTQ("run_global()", ""){
	try{
		const auto r = run_global(
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
	const auto r = run_global(
		R"(
			assert(1 == 1);
			print("A");
		)"
	);
	QUARK_UT_VERIFY((r._print_output == vector<string>{ "A" }));
}


//////////////////////////		HOST FUNCTION - get_time_of_day()



QUARK_UNIT_TESTQ("run_init()", "get_time_of_day()"){
	test__run_init2(
		R"(
			int a = get_time_of_day();
			int b = get_time_of_day();
			int c = b - a;
			print("Delta time:" + to_string(a));
		)"
	);
}


//////////////////////////		BLOCKS AND SCOPING



QUARK_UNIT_TESTQ("run_init()", "Empty block"){
	test__run_init2(
		"{}"
	);
}

QUARK_UNIT_TESTQ("run_init()", "Block with local variable, no shadowing"){
	test__run_init2(
		"{ int x = 4; }"
	);
}

QUARK_UNIT_TESTQ("run_init()", "Block with local variable, no shadowing"){
	const auto r = run_global(
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




//////////////////////////		IF STATEMENT



QUARK_UNIT_TESTQ("run_init()", "if(true){}"){
	const auto r = run_global(
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
	const auto r = run_global(
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
	const auto r = run_global(
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
	const auto r = run_global(
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
	const auto r = run_global(
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
	const auto r = run_global(
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
	const auto r = run_global(
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
	const auto r = run_global(
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




//////////////////////////		FOR STATEMENT




QUARK_UNIT_TESTQ("run_init()", "for"){
	const auto r = run_global(
		R"(
			for (i in 0...2) {
				print("Iteration: " + to_string(i));
			}
		)"
	);
	QUARK_UT_VERIFY((r._print_output == vector<string>{ "Iteration: 0", "Iteration: 1", "Iteration: 2" }));
}

QUARK_UNIT_TESTQ("run_init()", "fibonacci"){
	const auto vm = run_global(
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


//////////////////////////		WHILE STATEMENT


QUARK_UNIT_TESTQ("run_init()", "for"){
	const auto r = run_global(
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


//////////////////////////		TYPEID - TYPE

//	??? Test converting different types to jsons

//////////////////////////		STRING - TYPE



//??? add tests for equality.

QUARK_UNIT_TEST("string", "size()", "string", "0"){
	const auto vm = run_global(R"(
		assert(size("") == 0);
	)");
}
QUARK_UNIT_TEST("string", "size()", "string", "24"){
	const auto vm = run_global(R"(
		assert(size("How long is this string?") == 24);
	)");
}

QUARK_UNIT_TEST("string", "push_back()", "string", "correct final vector"){
	const auto vm = run_global(R"(
		a = push_back("one", "two");
		assert(a == "onetwo");
	)");
}

QUARK_UNIT_TEST("string", "update()", "string", "correct final string"){
	const auto vm = run_global(R"(
		a = update("hello", 1, "z");
		assert(a == "hzllo");
	)");
}


QUARK_UNIT_TEST("string", "subset()", "string", "correct final vector"){
	const auto vm = run_global(R"(
		assert(subset("abc", 0, 3) == "abc");
		assert(subset("abc", 1, 3) == "bc");
		assert(subset("abc", 0, 0) == "");
	)");
}

QUARK_UNIT_TEST("vector", "replace()", "combo", ""){
	const auto vm = run_global(R"(
		assert(replace("One ring to rule them all", 4, 8, "rabbit") == "One rabbit to rule them all");
	)");
}
// ### test pos limiting and edge cases.



//////////////////////////		VECTOR - TYPE


QUARK_UNIT_TEST("vector", "[]-constructor, inplicit type", "strings", "valid vector"){
	const auto vm = run_global(R"(
		a = ["one", "two"];
		print(a);
	)");
	ut_compare_stringvects(vm._print_output, vector<string>{
		R"([string]("one","two"))"
	});
}

QUARK_UNIT_TEST("vector", "[]-constructor, implicit type", "cannot be deducted", "error"){
	try{
		const auto vm = run_global(R"(
			a = [];
			print(a);
		)");
		QUARK_UT_VERIFY(false);
	}
	catch(...){
	}
}

QUARK_UNIT_TEST("vector", "explit bind, is [] working as type?", "strings", "valid vector"){
	const auto vm = run_global(R"(
		[string] a = ["one", "two"];
		print(a);
	)");
	ut_compare_stringvects(vm._print_output, vector<string>{
		R"([string]("one","two"))"
	});
}

QUARK_UNIT_TEST("vector", "", "empty vector", "valid vector"){
	const auto vm = run_global(R"(
		[string] a = [];
		print(a);
	)");
	ut_compare_stringvects(vm._print_output, vector<string>{
		R"([string]())"
	});
}

#if false
//	[string]() -- requires TYPE(EXPRESSION) conversion expressions.
QUARK_UNIT_TEST("vector", "[]-constructor, explicit type", "strings", "valid vector"){
	const auto vm = run_global(R"(
		a = [string]("one", "two");
		print(a);
	)");
	QUARK_UT_VERIFY((	vm._print_output == vector<string>{	"one", "two"	}	));
}
#endif


QUARK_UNIT_TEST("vector", "=", "strings", "valid vector"){
	const auto vm = run_global(R"(
		a = ["one", "two"];
		b = a;
		assert(a == b);
		print(a);
		print(b);
	)");
	ut_compare_stringvects(vm._print_output, vector<string>{
		R"([string]("one","two"))",
		R"([string]("one","two"))"
	});
}
QUARK_UNIT_TEST("vector", "==", "strings", ""){
	const auto vm = run_global(R"(
		assert((["one", "two"] == ["one", "two"]) == true);
	)");
}
QUARK_UNIT_TEST("vector", "==", "strings", ""){
	const auto vm = run_global(R"(
		assert(([1,2,3,4] == [1,2,3,4]) == true);
	)");
}
QUARK_UNIT_TEST("vector", "==", "strings", ""){
	const auto vm = run_global(R"(
		assert(([] == []) == true);
	)");
}
QUARK_UNIT_TEST("vector", "==", "strings", ""){
	const auto vm = run_global(R"(
		assert((["one", "three"] == ["one", "two"]) == false);
	)");
}

QUARK_UNIT_TEST("vector", "<", "strings", ""){
	const auto vm = run_global(R"(
		assert((["one", "two"] < ["one", "two"]) == false);
	)");
}

QUARK_UNIT_TEST("vector", "<", "strings", ""){
	const auto vm = run_global(R"(
		assert((["one", "a"] < ["one", "two"]) == true);
	)");
}
QUARK_UNIT_TEST("vector", "<", "strings", ""){
	const auto vm = run_global(R"(
		assert((["one", "a"] < ["one", "two"]) == true);
	)");
}

QUARK_UNIT_TEST("vector", "size()", "", "correct size"){
	try{
		const auto vm = run_global(R"(
			[string] a = ["one", "two"];
			assert(size(a) == 0);
		)");
		QUARK_UT_VERIFY(false);
	}
	catch(...){
	}
}

QUARK_UNIT_TEST("vector", "size()", "[]", "correct size"){
	const auto vm = run_global(R"(
		[string] a = [];
		assert(size(a) == 0);
	)");
}
QUARK_UNIT_TEST("vector", "size()", "[]", "correct size"){
	const auto vm = run_global(R"(
		[string] a = ["one", "two"];
		assert(size(a) == 2);
	)");
}

QUARK_UNIT_TEST("vector", "+", "add empty vectors", "correct final vector"){
	const auto vm = run_global(R"(
		[string] a = [] + [];
		assert(a == []);
	)");
}

QUARK_UNIT_TEST("vector", "+", "non-empty vectors", "correct final vector"){
	const auto vm = run_global(R"(
		[string] a = ["one"] + ["two"];
		assert(a == ["one", "two"]);
	)");
}

QUARK_UNIT_TEST("vector", "push_back()", "vector", "correct final vector"){
	const auto vm = run_global(R"(
		[string] a = push_back(["one"], "two");
		assert(a == ["one", "two"]);
	)");
}

QUARK_UNIT_TEST("vector", "find()", "vector", "correct return"){
	const auto vm = run_global(R"(
		assert(find([1,2,3], 4) == -1);
		assert(find([1,2,3], 1) == 0);
		assert(find([1,2,2,2,3], 2) == 1);
	)");
}

QUARK_UNIT_TEST("vector", "find()", "string", "correct return"){
	const auto vm = run_global(R"(
		assert(find("hello, world", "he") == 0);
		assert(find("hello, world", "e") == 1);
		assert(find("hello, world", "x") == -1);
	)");
}

QUARK_UNIT_TEST("vector", "subset()", "combo", ""){
	const auto vm = run_global(R"(
		assert(subset([10,20,30], 0, 3) == [10,20,30]);
		assert(subset([10,20,30], 1, 3) == [20,30]);
		assert(subset([10,20,30], 0, 0) == []);
	)");
}

QUARK_UNIT_TEST("vector", "replace()", "combo", ""){
	const auto vm = run_global(R"(
		assert(replace([ 1, 2, 3, 4, 5, 6 ], 2, 5, [20, 30]) == [1, 2, 20, 30, 6]);
	)");
}
// ### test pos limiting and edge cases.


QUARK_UNIT_TEST("vector", "update()", "mutate element", "valid vector, without side effect on original vector"){
	const auto vm = run_global(R"(
		a = [ "one", "two", "three"];
		b = update(a, 1, "zwei");
		print(a);
		print(b);
		assert(a == ["one","two","three"]);
		assert(b == ["one","zwei","three"]);
	)");
	ut_compare_stringvects(vm._print_output, vector<string>{
		R"([string]("one","two","three"))",
		R"([string]("one","zwei","three"))"
	});
}


//////////////////////////		DICT - TYPE



QUARK_UNIT_TEST("dict", "construct", "", ""){
	const auto vm = run_global(R"(
		[string: int] a = ["one": 1, "two": 2];
		assert(size(a) == 2);
	)");
}
QUARK_UNIT_TEST("dict", "[]", "", ""){
	const auto vm = run_global(R"(
		[string: int] a = ["one": 1, "two": 2];
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
		const auto vm = run_global(R"(
			mutable a = [:];
			a = ["hello": 1];
			print(a);
		)");
		QUARK_UT_VERIFY(false);
	}
	catch(...){
	}
}

QUARK_UNIT_TEST("dict", "[:]", "", ""){
	try {
		const auto vm = run_global(R"(
			a = [:];
			print(a);
		)");
		QUARK_UT_VERIFY(false);
	}
	catch(...){
	}
}


QUARK_UNIT_TEST("dict", "deduced type ", "", ""){
	const auto vm = run_global(R"(
		a = ["one": 1, "two": 2];
		print(a);
	)");
	ut_compare_stringvects(vm._print_output, vector<string>{
		R"([string:int]("one": 1,"two": 2))",
	});
}

QUARK_UNIT_TEST("dict", "[:]", "", ""){
	const auto vm = run_global(R"(
		mutable [string:int] a = [:];
		a = [:];
		print(a);
	)");
	ut_compare_stringvects(vm._print_output, vector<string>{
		R"([string:int]())",
	});
}

QUARK_UNIT_TEST("dict", "==", "", ""){
	const auto vm = run_global(R"(
		assert((["one": 1, "two": 2] == ["one": 1, "two": 2]) == true);
	)");
}
QUARK_UNIT_TEST("dict", "==", "", ""){
	const auto vm = run_global(R"(
		assert((["one": 1, "two": 2] == ["two": 2]) == false);
	)");
}
QUARK_UNIT_TEST("dict", "==", "", ""){
	const auto vm = run_global(R"(
		assert((["one": 2, "two": 2] == ["one": 1, "two": 2]) == false);
	)");
}
QUARK_UNIT_TEST("dict", "==", "", ""){
	const auto vm = run_global(R"(
		assert((["one": 1, "two": 2] < ["one": 1, "two": 2]) == false);
	)");
}
QUARK_UNIT_TEST("dict", "==", "", ""){
	const auto vm = run_global(R"(
		assert((["one": 1, "two": 1] < ["one": 1, "two": 2]) == true);
	)");
}

QUARK_UNIT_TEST("dict", "size()", "[:]", "correct size"){
	const auto vm = run_global(R"(
		assert(size([:]) == 0);
	)");
}
QUARK_UNIT_TEST("dict", "size()", "[:]", "correct type"){
	const auto vm = run_global(R"(
		print([:]);
	)");
	ut_compare_stringvects(vm._print_output, vector<string>{
		R"([string:null]())",
	});
}
QUARK_UNIT_TEST("dict", "size()", "[:]", "correct size"){
	const auto vm = run_global(R"(
		assert(size(["one":1]) == 1);
	)");
}
QUARK_UNIT_TEST("dict", "size()", "[:]", "correct size"){
	const auto vm = run_global(R"(
		assert(size(["one":1, "two":2]) == 2);
	)");
}

QUARK_UNIT_TEST("dict", "update()", "add element", "valid dict, without side effect on original dict"){
	const auto vm = run_global(R"(
		a = [ "one": 1, "two": 2];
		b = update(a, "three", 3);
		print(a);
		print(b);
	)");
	ut_compare_stringvects(vm._print_output, vector<string>{
		R"([string:int]("one": 1,"two": 2))",
		R"([string:int]("one": 1,"three": 3,"two": 2))"
	});
}

QUARK_UNIT_TEST("dict", "update()", "replace element", ""){
	const auto vm = run_global(R"(
		a = [ "one": 1, "two": 2, "three" : 3];
		b = update(a, "three", 333);
		print(a);
		print(b);
	)");
	ut_compare_stringvects(vm._print_output, vector<string>{
		R"([string:int]("one": 1,"three": 3,"two": 2))",
		R"([string:int]("one": 1,"three": 333,"two": 2))"
	});
}


QUARK_UNIT_TEST("dict", "update()", "dest is empty dict", ""){
	const auto vm = run_global(R"(
		a = update([:], "one", 1);
		b = update(a, "two", 2);
		print(b);
		assert(a == ["one": 1]);
		assert(b == ["one": 1, "two": 2]);
	)");
}


QUARK_UNIT_TEST("dict", "exists()", "", ""){
	const auto vm = run_global(R"(
		a = [ "one": 1, "two": 2, "three" : 3];
		assert(exists(a, "two") == true);
		assert(exists(a, "four") == false);
	)");
}

QUARK_UNIT_TEST("dict", "erase()", "", ""){
	const auto vm = run_global(R"(
		a = [ "one": 1, "two": 2, "three" : 3];
		b = erase(a, "one");
		assert(b == [ "two": 2, "three" : 3]);
	)");
}



//////////////////////////		STRUCT - TYPE



QUARK_UNIT_TESTQ("run_main()", "struct"){
	const auto vm = run_global(R"(
		struct t {}
	)");
}

QUARK_UNIT_TESTQ("run_main()", "struct"){
	const auto vm = run_global(R"(
		struct t { int a;}
	)");
}

QUARK_UNIT_TESTQ("run_main()", "struct - make instance"){
	const auto vm = run_global(R"(
		struct t { int a;}
		t(3);
	)");
	QUARK_UT_VERIFY((	vm._call_stack.back()->_values["t"].first.is_typeid()	));
}

QUARK_UNIT_TESTQ("run_main()", "struct - check struct's type"){
	const auto vm = run_global(R"(
		struct t { int a;}
		print(t);
	)");
	ut_compare_stringvects(vm._print_output, vector<string>{
		"typeid(struct {int a;})"
	});
}

QUARK_UNIT_TESTQ("run_main()", "struct - check struct's type"){
	const auto vm = run_global(R"(
		struct t { int a;}
		a = t(3);
		print(a);
	)");
	ut_compare_stringvects(vm._print_output, vector<string>{
		 R"(struct {a=3})"
	});
}

QUARK_UNIT_TESTQ("run_main()", "struct - read back struct member"){
	const auto vm = run_global(R"(
		struct t { int a;}
		temp = t(4);
		print(temp.a);
	)");
	ut_compare_stringvects(vm._print_output, vector<string>{
		"4"
	});
}

QUARK_UNIT_TESTQ("run_main()", "struct - instantiate nested structs"){
	const auto vm = run_global(R"(
		struct color { int red; int green; int blue;}
		struct image { color back; color front;}

		c = color(128, 192, 255);
		print(c);

		i = image(color(1, 2, 3), color(200, 201, 202));
		print(i);
	)");
	ut_compare_stringvects(vm._print_output, vector<string>{
		"struct {red=128,green=192,blue=255}",
		"struct {back=struct {red=1,green=2,blue=3},front=struct {red=200,green=201,blue=202}}"
	});
}

QUARK_UNIT_TESTQ("run_main()", "struct - access member of nested structs"){
	const auto vm = run_global(R"(
		struct color { int red; int green; int blue;}
		struct image { pixel back; pixel front;}
		i = image(color(1, 2, 3), color(200, 201, 202));
		print(i.front.green);
	)");
	ut_compare_stringvects(vm._print_output, vector<string>{
		"201"
	});
}

QUARK_UNIT_TEST("run_main()", "return struct from function", "", ""){
	const auto vm = run_global(R"(
		struct color { int red; int green; int blue;}
		struct image { pixel back; pixel front;}
		color make_color(){
			return color(100, 101, 102);
		}
		print(make_color());
	)");
	ut_compare_stringvects(vm._print_output, vector<string>{
		"struct {red=100,green=101,blue=102}",
	});
}

QUARK_UNIT_TESTQ("run_main()", "struct - compare structs"){
	const auto vm = run_global(R"(
		struct color { int red; int green; int blue;}
		print(color(1, 2, 3) == color(1, 2, 3));
	)");
	ut_compare_stringvects(vm._print_output, vector<string>{
		"true"
	});
}

QUARK_UNIT_TESTQ("run_main()", "struct - compare structs"){
	const auto vm = run_global(R"(
		struct color { int red; int green; int blue;}
		print(color(9, 2, 3) == color(1, 2, 3));
	)");
	ut_compare_stringvects(vm._print_output, vector<string>{
		"false"
	});
}

QUARK_UNIT_TESTQ("run_main()", "struct - compare structs different types"){
	try {
		const auto vm = run_global(R"(
			struct color { int red; int green; int blue;}
			struct file { int id;}
			print(color(1, 2, 3) == file(404));
		)");
		QUARK_UT_VERIFY(false);
	}
	catch(...){
	}
}

QUARK_UNIT_TESTQ("run_main()", "struct - compare structs with <, different types"){
	const auto vm = run_global(R"(
		struct color { int red; int green; int blue;}
		print(color(1, 2, 3) < color(1, 2, 3));
	)");
	ut_compare_stringvects(vm._print_output, vector<string>{
		"false"
	});
}

QUARK_UNIT_TESTQ("run_main()", "struct - compare structs <"){
	const auto vm = run_global(R"(
		struct color { int red; int green; int blue;}
		print(color(1, 2, 3) < color(1, 4, 3));
	)");
	ut_compare_stringvects(vm._print_output, vector<string>{
		"true"
	});
}




QUARK_UNIT_TESTQ("run_main()", "update struct manually"){
	const auto vm = run_global(R"(
		struct color { int red; int green; int blue;}
		a = color(255, 128, 128);
		b = color(a.red, a.green, 129);
		print(a);
		print(b);
	)");
	ut_compare_stringvects(vm._print_output, vector<string>{
		"struct {red=255,green=128,blue=128}",
		"struct {red=255,green=128,blue=129}"
	});
}


QUARK_UNIT_TESTQ("run_main()", "mutate struct member using = won't work"){
	try {
		const auto vm = run_global(R"(
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
	const auto vm = run_global(R"(
		struct color { int red; int green; int blue;}
		a = color(255,128,128);
		b = update(a, "green", 3);
		print(a);
		print(b);
	)");

	ut_compare_stringvects(vm._print_output, vector<string>{
		"struct {red=255,green=128,blue=128}",
		"struct {red=255,green=3,blue=128}",
	});
}

QUARK_UNIT_TEST("run_main()", "mutate nested member", "", ""){
	const auto vm = run_global(R"(
		struct color { int red; int green; int blue;}
		struct image { color back; color front;}
		a = image(color(0,100,200), color(0,0,0));
		b = update(a, "front.green", 3);
		print(a);
		print(b);
	)");
	ut_compare_stringvects(vm._print_output, vector<string>{
		"struct {back=struct {red=0,green=100,blue=200},front=struct {red=0,green=0,blue=0}}",
		"struct {back=struct {red=0,green=100,blue=200},front=struct {red=0,green=3,blue=0}}"
	});
}



QUARK_UNIT_TEST("", "", "", ""){
	const auto vm = run_global(R"(
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
	const auto vm = run_global(R"(
		//	XYZ
		print("Hello");
	)");
	ut_compare_stringvects(vm._print_output, vector<string>{
		"Hello"
	});
}

QUARK_UNIT_TESTQ("comments", "// on start of line"){
	const auto vm = run_global(R"(
		print("Hello");		//	XYZ
	)");
	ut_compare_stringvects(vm._print_output, vector<string>{
		"Hello"
	});
}

QUARK_UNIT_TESTQ("comments", "// on start of line"){
	const auto vm = run_global(R"(
		print("Hello");/* xyz */print("Bye");
	)");
	ut_compare_stringvects(vm._print_output, vector<string>{
		"Hello",
		"Bye"
	});
}



//////////////////////////		JSON - TYPE

/*
QUARK_UNIT_TEST_VIP("json", "dogs", "", ""){
	const auto vm = run_global(R"(

		test_json1 = json_value(["spotty", "stripey", "pluto" ]);
		print(test_json1);
	)");
	QUARK_UT_VERIFY((	vm._print_output == vector<string>{ "Hello", "Bye" } ))
}

QUARK_UNIT_TEST_VIP("json", "pigcount", "", ""){
	const auto vm = run_global(R"(

		test_json1 = json_value({ "pigcount": 3, "pigcolor": "pink" });
		print(test_json1);
	)");
	QUARK_UT_VERIFY((	vm._print_output == vector<string>{ "Hello", "Bye" } ))
}


QUARK_UNIT_TEST_VIP("json", "pigcount", "", ""){
	const auto vm = run_global(R"(

		a = {
		  "CustomerId": "string",
		  "PartnerOrderId": "string",
		  "Items": [
			{
			  "Sku": "string",
			  "DocumentReferenceUrl": "string",
			  "Quantity": 0,
			  "PartnerProductName": "string",
			  "PartnerItemId": "string"
			}
		  ],
		  "Metadata": "string",
		  "DeliveryOptionId": "string"
		};


		print(a);
	)");
	QUARK_UT_VERIFY((	vm._print_output == vector<string>{ "Hello", "Bye" } ))
}

*/




















//??? test accessing array->struct->array.
//??? test structs in vectors.

