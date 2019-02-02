//
//  parser_function.cpp
//  FloydSpeak
//
//  Created by Marcus Zetterquist on 30/07/16.
//  Copyright © 2016 Marcus Zetterquist. All rights reserved.
//

#include "parse_function_def.h"

#include "text_parser.h"
#include "floyd_parser.h"
#include "parser_primitives.h"
#include "parse_statement.h"
#include "json_support.h"
#include "json_parser.h"
#include "ast_typeid.h"
#include "ast_typeid_helpers.h"

#include <cmath>

namespace floyd {
	using std::pair;
	using std::string;
	using std::vector;


std::pair<ast_json_t, seq_t> parse_function_definition2(const seq_t& pos){
	const auto func_pos = read_required(skip_whitespace(pos), keyword_t::k_func);
	const auto return_type_pos = read_required_type(func_pos);
	const auto function_name_pos = read_required_identifier(return_type_pos.second);
	const auto args_pos = read_function_arg_parantheses(skip_whitespace(function_name_pos.second));

	const auto impure_pos = if_first(skip_whitespace(args_pos.second), keyword_t::k_impure);
	const auto body = parse_statement_body(impure_pos.second);

	const auto args = members_to_json(args_pos.first);
	const auto function_name = function_name_pos.first;

	const auto function_def = make_statement(
		0,
		"def-func",
		json_t::make_object({
			{ "name", function_name },
			{ "args", args },
			{ "statements", body.ast._value },
			{ "return_type", typeid_to_ast_json(return_type_pos.first, json_tags::k_tag_resolve_state)._value },
			{ "impure", impure_pos.first }
		})
	);
	return { function_def, body.pos };
}


struct test {
	std::string desc;
	std::string input;
	std::string output;
};


QUARK_UNIT_TEST("", "parse_function_definition2()", "Minimal function IMPURE", ""){
	const std::string input = "func int f() impure{ return 3; }";
	const std::string expected = R"(
		[
			"def-func",
			{ "args": [], "name": "f", "return_type": "^int", "statements": [["return", ["k", 3, "^int"]]], "impure": true }
		]
	)";
	ut_compare_jsons(parse_function_definition2(seq_t(input)).first._value, parse_json(seq_t(expected)).first);
}

const std::vector<test> testsxyz = {
	{
		"Minimal function",
		"func int f(){ return 3; }",

		R"(
			[
				"def-func",
				{ "args": [], "name": "f", "return_type": "^int", "statements": [["return", ["k", 3, "^int"]]], "impure": false }
			]
		)"
	},
	{
		"3 args of different types",
		"func int printf(string a, double barry, int c){ return 3; }",
		R"(
			[
				"def-func",
				{
					"args": [
						{ "name": "a", "type": "^string" },
						{ "name": "barry", "type": "^double" },
						{ "name": "c", "type": "^int" },
					],
					"name": "printf",
					"return_type": "^int",
					"statements": [["return", ["k", 3, "^int"]]],
					"impure": false
				}
			]
		)"
	},
	{
		"Max whitespace",
		" func  \t int \t printf( \t string \t a \t , \t double \t b \t ){ \t return \t 3 \t ; \t } \t ",
		R"(
			[
				"def-func",
				{
					"args": [
						{ "name": "a", "type": "^string" },
						{ "name": "b", "type": "^double" }
					],
					"name": "printf",
					"return_type": "^int",
					"statements": [["return", ["k", 3, "^int"]]],
					"impure": false
				}
			]
		)"
	},
	{
		"Min whitespace",
		"func int printf(string a,double b){return 3;}",
		R"(
			[
				"def-func",
				{
					"args": [
						{ "name": "a", "type": "^string" },
						{ "name": "b", "type": "^double" }
					],
					"name": "printf",
					"return_type": "^int",
					"statements": [["return", ["k", 3, "^int"]]],
					"impure": false
				}
			]
		)"
	}
};


QUARK_UNIT_TEST("", "parse_function_definition2()", "BATCH", "Correct output JSON"){
	for(const auto& e: testsxyz){
		QUARK_SCOPED_TRACE(e.desc);
		ut_compare_jsons(parse_function_definition2(seq_t(e.input)).first._value, parse_json(seq_t(e.output)).first);
	}
}

}	//	floyd

