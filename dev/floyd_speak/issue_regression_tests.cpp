//
//  issue_regression_tests.cpp
//  floyd_speak
//
//  Created by Marcus Zetterquist on 2019-02-25.
//  Copyright © 2019 Marcus Zetterquist. All rights reserved.
//

#include "test_helpers.h"


using namespace floyd;


OFF_QUARK_UNIT_TEST("https://github.com/marcusz/floyd/issues/8", "", "", "") {
	ut_verify_exception(
		QUARK_POS,
		
R"(for (i in 1..5) {
  print(i)
}
)",
		"Expression type mismatch - cannot convert 'bool' to 'int. Line: 1 \"let int result = true\""
	);
}




