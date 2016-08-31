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
#include <vector>
#include <string>

#include "parser_ast.h"
#include "parser_value.h"


namespace floyd_parser {
	struct expression_t;
	struct statement_t;
	struct scope_def_t;


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
	std::pair<scope_ref_t, std::string> parse_function_definition(const ast_t& ast, const std::string& pos);



	//////////////////////////////////		Testing


	const std::string test_function1 = "int test_function1(){ return 100; }";
	scope_ref_t make_test_function1(scope_ref_t scope);

	const std::string test_function2 = "string test_function2(int a, float b){ return \"sdf\"; }";
	scope_ref_t make_test_function2(scope_ref_t scope);

	scope_ref_t make_log_function(scope_ref_t scope);
	scope_ref_t make_log2_function(scope_ref_t scope);
	scope_ref_t make_return5(scope_ref_t scope);

	scope_ref_t make_return_hello(scope_ref_t scope);
}


#endif /* parser_function_hpp */
