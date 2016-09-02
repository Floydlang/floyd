//
//  parser_function.h
//  FloydSpeak
//
//  Created by Marcus Zetterquist on 30/07/16.
//  Copyright © 2016 Marcus Zetterquist. All rights reserved.
//

#ifndef parser_function_hpp
#define parser_function_hpp

#include "quark.h"
#include <string>

struct json_value_t;

namespace floyd_parser {
	/*
		Named function definition:

		int myfunc(string a, int b){
			...
			return b + 1;
		}

		Creates its own function sub-scope at the bottom of the ast.

		LATER:
		Lambda:

		int myfunc(string a){
			() => {
			}
		}
	*/
	std::pair<json_value_t, std::string> parse_function_definition(const std::string& pos);



	//////////////////////////////////		Testing


	const std::string test_function1 = "int test_function1(){ return 100; }";
	json_value_t make_test_function1();

	const std::string test_function2 = "string test_function2(int a, float b){ return \"sdf\"; }";
	json_value_t make_test_function2();

	json_value_t make_log_function();
	json_value_t make_log2_function();
	json_value_t make_return5();

	json_value_t make_return_hello();
}


#endif /* parser_function_hpp */
