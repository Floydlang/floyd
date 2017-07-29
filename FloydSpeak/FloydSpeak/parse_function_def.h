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

struct json_t;
struct seq_t;

namespace floyd_parser {
	/*
		OUTPUT:
			[
				"def-func",
				{
					"name": "main",
					"args": [],
					"return_type": "<int>",
					"statements": [
						[ "return", [ "k", 3, "<int>" ]]
					]
				}
			]
	*/
	std::pair<json_t, seq_t> parse_function_definition2(const seq_t& pos);



	//////////////////////////////////		Testing


	const std::string test_function1 = "int test_function1(){ return 100; }";
	json_t make_test_function1();

	const std::string test_function2 = "string test_function2(int a, float b){ return \"sdf\"; }";
	json_t make_test_function2();

	json_t make_log_function();
	json_t make_log2_function();
	json_t make_return5();

	json_t make_return_hello();
}


#endif /* parser_function_hpp */
