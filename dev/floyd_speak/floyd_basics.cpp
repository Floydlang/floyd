//
//  floyd_basics.cpp
//  FloydSpeak
//
//  Created by Marcus Zetterquist on 2017-08-09.
//  Copyright © 2017 Marcus Zetterquist. All rights reserved.
//

#include "floyd_basics.h"

using std::string;
using std::vector;

#include "ast_value.h"
#include "text_parser.h"
#include "json_support.h"

namespace floyd {

const location_t k_no_location(std::numeric_limits<std::size_t>::max());






void ut_verify_json_and_rest(const quark::call_context_t& context, const std::pair<ast_json_t, seq_t>& result_pair, const std::string& expected_json, const std::string& expected_rest){
	ut_verify(
		context,
		result_pair.first._value,
		parse_json(seq_t(expected_json)).first
	);

	ut_verify(context, result_pair.second.str(), expected_rest);
}


void ut_verify_values(const quark::call_context_t& context, const value_t& result, const value_t& expected){
	if(result != expected){
		QUARK_TRACE("result:");
		QUARK_TRACE(json_to_pretty_string(value_and_type_to_ast_json(result)._value));
		QUARK_TRACE("expected:");
		QUARK_TRACE(json_to_pretty_string(value_and_type_to_ast_json(expected)._value));

		fail_test(context);
	}
}

void ut_verify(const quark::call_context_t& context, const std::pair<std::string, seq_t>& result, const std::pair<std::string, seq_t>& expected){
	if(result == expected){
	}
	else{
		ut_verify(context, result.first, expected.first);
		ut_verify(context, result.second.str(), expected.second.str());
	}
}


}
