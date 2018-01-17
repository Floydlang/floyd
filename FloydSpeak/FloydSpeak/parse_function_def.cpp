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

#include <cmath>

namespace floyd {
	using std::pair;
	using std::string;
	using std::vector;



std::pair<json_t, seq_t> parse_function_definition2(const seq_t& pos){
	const auto return_type_pos = read_required_type(pos);
	const auto function_name_pos = read_required_identifier(return_type_pos.second);
	const auto args_pos = read_function_arg_parantheses(function_name_pos.second);
	const auto body = parse_statement_body(args_pos.second);

	const auto args = members_to_json(args_pos.first);
	const auto function_name = function_name_pos.first;

	json_t function_def = json_t::make_array({
		"def-func",
		json_t::make_object({
			{ "name", function_name },
			{ "args", args },
			{ "statements", body.first },
			{ "return_type", typeid_to_ast_json(return_type_pos.first) }
		})
	});
	return { function_def, body.second };
}

struct test {
	std::string desc;
	std::string input;
	std::string output;
};

const std::vector<test> testsxyz = {
	{
		"Minimal function",
		"int f(){ return 3; }",

		R"(
			[
				"def-func",
				{ "args": [], "name": "f", "return_type": "int", "statements": [["return", ["k", 3, "int"]]] }
			]
		)"
	},
	{
		"3 args of different types",
		"int printf(string a, float barry, int c){ return 3; }",
		R"(
			[
				"def-func",
				{
					"args": [
						{ "name": "a", "type": "string" },
						{ "name": "barry", "type": "float" },
						{ "name": "c", "type": "int" },
					],
					"name": "printf",
					"return_type": "int",
					"statements": [["return", ["k", 3, "int"]]]
				}
			]
		)"
	},
	{
		"Max whitespace",
		" \t int \t printf( \t string \t a \t , \t float \t b \t ){ \t return \t 3 \t ; \t } \t ",
		R"(
			[
				"def-func",
				{
					"args": [
						{ "name": "a", "type": "string" },
						{ "name": "b", "type": "float" }
					],
					"name": "printf",
					"return_type": "int",
					"statements": [["return", ["k", 3, "int"]]]
				}
			]
		)"
	},
	{
		"Min whitespace",
		"int printf(string a,float b){return 3;}",
		R"(
			[
				"def-func",
				{
					"args": [
						{ "name": "a", "type": "string" },
						{ "name": "b", "type": "float" }
					],
					"name": "printf",
					"return_type": "int",
					"statements": [["return", ["k", 3, "int"]]]
				}
			]
		)"
	}
};


QUARK_UNIT_TEST("", "parse_function_definition2()", "BATCH", "Correct output JSON"){
	for(const auto e: testsxyz){
		QUARK_SCOPED_TRACE(e.desc);
		ut_compare_jsons(parse_function_definition2(seq_t(e.input)).first,parse_json(seq_t(e.output)).first);
	}
}

}	//	floyd

