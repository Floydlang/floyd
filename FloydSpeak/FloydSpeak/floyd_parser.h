//
//  floyd_parser.h
//  FloydSpeak
//
//  Created by Marcus Zetterquist on 10/04/16.
//  Copyright © 2016 Marcus Zetterquist. All rights reserved.
//

#ifndef floyd_parser_h
#define floyd_parser_h

#include "quark.h"

#include "parser_primitives.h"

namespace floyd_parser {
	struct expression_t;
	struct statement_t;
	struct function_def_expr_t;
	struct value_t;




	/*
		Global function definitions:

			#1
			int test_func1(){ return 100; };

			#2
			string test_func2(int a, float b){ return "sdf" };

		Global struct definitions:

			struct my_image {
				int width;
				int height;
			};

			struct my_sprite {
				string name;
				my_image image;
			};

		FUTURE
		- Define data structures (also in local scopes).
		- Add support for global constants.
		- Assign global constants


		Global constants:

			float my_global1 = 3.1415f + ;
			my_sprite my_test_sprite =
	*/
	std::pair<statement_t, std::string> read_statement(const ast_t& ast, const std::string& pos);




	ast_t program_to_ast(const ast_t& init, const std::string& program);

}	//	floyd_parser



#endif /* floyd_parser_h */
