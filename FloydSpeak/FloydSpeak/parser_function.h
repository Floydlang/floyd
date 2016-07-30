//
//  parser_function.hpp
//  FloydSpeak
//
//  Created by Marcus Zetterquist on 30/07/16.
//  Copyright © 2016 Marcus Zetterquist. All rights reserved.
//

#ifndef parser_function_hpp
#define parser_function_hpp

#include "quark.h"
#include <vector>
#include <string>

#include "parser_types.h"
#include "parser_value.hpp"


namespace floyd_parser {
	struct expression_t;
	struct statement_t;
	struct ast_t;

	/*
		()
		(int a)
		(int x, int y)
	*/
	std::vector<arg_t> parse_functiondef_arguments(const std::string& s2);

	/*
		Never simplifes - the parser is non-lossy.

		Must not have whitespace before / after {}.
		{}

		{
			return 3;
		}

		{
			return 3 + 4;
		}
		{
			return f(3, 4) + 2;
		}


		//	Example: binding constants to constants, result of function calls and math operations.
		{
			int a = 10;
			int b = f(a);
			int c = a + b;
			return c;
		}

		//	Local scope.
		{
			{
				int a = 10;
			}
		}
		{
			struct point2d {
				int _x;
				int _y;
			}
		}

		{
			int my_func(string a, string b){
				int c = a + b;
				return c;
			}
		}

		FUTURE
		- Include comments
		- Split-out parse_statement().
		- Add struct {}
		- Add variables
		- Add local functions
	*/
	std::vector<std::shared_ptr<statement_t>> parse_function_body(const ast_t& ast, const std::string& s);

	/*
		Named function:

		int myfunc(string a, int b){
			...
			return b + 1;
		}


		LATER:
		Lambda:

		int myfunc(string a){
			() => {
			}
		}
	*/
	std::pair<std::pair<std::string, function_def_t>, std::string> parse_function_definition_statement(const ast_t& ast, const std::string& pos);




const std::string test_function1 = "int test_function1(){ return 100; }";

function_def_t make_test_function1();

const std::string test_function2 = "string test_function2(int a, float b){ return \"sdf\"; }";

function_def_t make_test_function2();


function_def_t make_log_function();
function_def_t make_log2_function();
function_def_t make_return5();




}


#endif /* parser_function_hpp */
