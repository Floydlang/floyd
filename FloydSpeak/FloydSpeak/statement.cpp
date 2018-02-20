//
//  parser_statement.cpp
//  FloydSpeak
//
//  Created by Marcus Zetterquist on 26/07/16.
//  Copyright © 2016 Marcus Zetterquist. All rights reserved.
//

#include "statement.h"


namespace floyd {

	using namespace std;


	QUARK_UNIT_TESTQ("statement_to_json", "return"){
		quark::ut_compare_strings(
			json_to_compact_string(
				statement_to_json(make__return_statement(expression_t::make_literal_string("abc")))._value
			)
			,
			R"(["return", ["k", "abc", "string"]])"
		);
	}


}	//	floyd
