//
//  examples_test.cpp
//  floyd
//
//  Created by Marcus Zetterquist on 2020-01-12.
//  Copyright © 2020 Marcus Zetterquist. All rights reserved.
//

#include "test_helpers.h"
#include "floyd_corelib.h"




using namespace floyd;



//######################################################################################################################
//	RUN ALL EXAMPLE PROGRAMS IN EXAMPLES-DIRECTORY-- VERIFY THEY WORK
//######################################################################################################################




QUARK_TEST("Floyd test suite", "hello_world.floyd", "", ""){
	const auto path = get_working_dir() + "/examples/hello_world.floyd";
	const auto program = read_text_file(path);

	ut_run_closed_lib(QUARK_POS, program);
}

#if 0
// Very slow when running in deep debug mode: creats lots of big vectors.
QUARK_TEST("Floyd test suite", "game_of_life.floyd", "", ""){
	const auto path = get_working_dir() + "/examples/game_of_life.floyd";
	const auto program = read_text_file(path);

	ut_run_closed_lib(QUARK_POS, program);
}
#endif



#if 0
//	WARNING: This test never completes + is impure.
//	WARNING: Since it doesn't stop running, only the BC will run the program, not LLVM.
// Very slow when running in deep debug mode: creats lots of big vectors.
QUARK_TEST("Floyd test suite", "game_of_life.floyd", "", ""){
	const auto path = get_working_dir() + "/examples/http_test.floyd";
	const auto program = read_text_file(path);

	ut_run_closed_lib(QUARK_POS, program);
}
#endif


