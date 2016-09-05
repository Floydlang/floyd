//
//  floyd_main.cpp
//  FloydSpeak
//
//  Created by Marcus Zetterquist on 10/04/16.
//  Copyright © 2016 Marcus Zetterquist. All rights reserved.
//

#include <stdio.h>

#include "quark.h"
#include "game_of_life1.h"
#include "game_of_life2.h"
#include "game_of_life3.h"

//////////////////////////////////////////////////		main()



int main(int argc, const char * argv[]) {
	try {
#if QUARK_UNIT_TESTS_ON
		quark::run_tests({
			//	UNDER DEVELOPMENT - RUN FIRST!
			"pass2.cpp",

			//	Core libs
			"quark.cpp",
			"steady_vector.cpp",
			"text_parser.cpp",
			"unused_bits.cpp",
			"sha1_class.cpp",
			"sha1.cpp",
			"json_parser.cpp",
			"json_support.cpp",
			"json_writer.cpp",


			"parser_ast.cpp",
			"ast_utils.cpp",
			"experimental_runtime.cpp",
			"expressions.cpp",

			"llvm_code_gen.cpp",

			"parser2.cpp",
			"parser_value.cpp",
			"parser_types_collector.cpp",
			"parser_expression.cpp",
			"parser_function.cpp",
			"parser_primitives.cpp",
			"parser_statement.cpp",
			"parser_struct.cpp",
			"parse_statement.cpp",

			"floyd_parser.cpp",

			"runtime_core.cpp",
			"runtime_value.cpp",
			"runtime.cpp",

			"utils.cpp",


			"pass3.cpp",

			"floyd_interpreter.cpp",

			"floyd_main.cpp",
		});
#endif
	}
	catch(...){
		QUARK_TRACE("Error");
		return -1;
	}

  return 0;
}
