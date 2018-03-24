//
//  expressions.cpp
//  FloydSpeak
//
//  Created by Marcus Zetterquist on 03/08/16.
//  Copyright © 2016 Marcus Zetterquist. All rights reserved.
//

#include "expression.h"

namespace floyd {

using namespace std;




////////////////////////////////////////////		JSON SUPPORT



QUARK_UNIT_TESTQ("expression_to_json()", "literals"){
	quark::ut_compare_strings(expression_to_json_string(expression_t::make_literal_int(13)), R"(["k", 13, "^int"])");
	quark::ut_compare_strings(expression_to_json_string(expression_t::make_literal_string("xyz")), R"(["k", "xyz", "^string"])");
	quark::ut_compare_strings(expression_to_json_string(expression_t::make_literal_float(14.0f)), R"(["k", 14, "^float"])");
	quark::ut_compare_strings(expression_to_json_string(expression_t::make_literal_bool(true)), R"(["k", true, "^bool"])");
	quark::ut_compare_strings(expression_to_json_string(expression_t::make_literal_bool(false)), R"(["k", false, "^bool"])");
}

QUARK_UNIT_TESTQ("expression_to_json()", "math2"){
	quark::ut_compare_strings(
		expression_to_json_string(
			expression_t::make_simple_expression__2(
				expression_type::k_arithmetic_add__2, expression_t::make_literal_int(2), expression_t::make_literal_int(3), nullptr)
			),
		R"(["+", ["k", 2, "^int"], ["k", 3, "^int"]])"
	);
}

QUARK_UNIT_TESTQ("expression_to_json()", "call"){
	quark::ut_compare_strings(
		expression_to_json_string(
			expression_t::make_call(
				expression_t::make_load("my_func", nullptr),
				{
					expression_t::make_literal_string("xyz"),
					expression_t::make_literal_int(123)
				},
				nullptr
			)
		),
		R"(["call", ["@", "my_func"], [["k", "xyz", "^string"], ["k", 123, "^int"]]])"
	);
}

QUARK_UNIT_TESTQ("expression_to_json()", "lookup"){
	quark::ut_compare_strings(
		expression_to_json_string(
			expression_t::make_lookup(
				expression_t::make_load("hello", nullptr),
				expression_t::make_literal_string("xyz"),
				nullptr
			)
		),
		R"(["[]", ["@", "hello"], ["k", "xyz", "^string"]])"
	);
}


}	//	floyd
