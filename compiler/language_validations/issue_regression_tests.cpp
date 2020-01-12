//
//  issue_regression_tests.cpp
//  Floyd
//
//  Created by Marcus Zetterquist on 2019-02-25.
//  Copyright © 2019 Marcus Zetterquist. All rights reserved.
//

#include "test_helpers.h"


using namespace floyd;


QUARK_TEST("https://github.com/marcusz/floyd/issues/8", "", "", "") {
	ut_verify_exception_nolib(
		QUARK_POS,
		
R"(for (i in 1..5) {
  print(i)
}
)",
		R"___([Syntax] For loop has illegal range syntax. Line: 1 "for (i in 1..5) {")___"
	);
}





FLOYD_LANG_PROOF("Regression tests", "a inited after ifelsejoin, don't delete it inside *then*", "", ""){
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

FLOYD_LANG_PROOF("Regression tests", "a inited after ifelsejoin, don't delete it inside *then*", "", ""){
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

