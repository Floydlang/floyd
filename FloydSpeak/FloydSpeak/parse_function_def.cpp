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
#include "json_support.h"
#include "json_parser.h"

#include <cmath>

namespace floyd_parser {
	using std::pair;
	using std::string;
	using std::vector;

/*
	()
	(int a)
	(int x, int y)
*/
static vector<json_value_t> parse_functiondef_arguments(const string& s2){
	const auto s = trim_ends(s2);
	vector<json_value_t> args;
	auto str = skip_whitespace(s);
	while(!str.empty()){
		const auto arg_type = read_type(seq_t(str));
		const auto arg_name = read_required_single_symbol(arg_type.second);
		const auto optional_comma = read_optional_char(skip_whitespace(arg_name.second), ',');

		const auto a = make_member_def("<" + arg_type.first + ">", arg_name.first, json_value_t());
		args.push_back(a);
		str = skip_whitespace(optional_comma.second).get_s();
	}
	return args;
}


const auto kTestFunctionArguments0 = "()";
const string kTestFunctionArguments0JSON = R"(
	[]
)";

QUARK_UNIT_TEST("", "parse_functiondef_arguments()", "Function definition 0 -- zero arguments", "Correct output JSON"){
	ut_compare_jsons(
		json_value_t::make_array2(parse_functiondef_arguments(kTestFunctionArguments0)),
		parse_json(seq_t(kTestFunctionArguments0JSON)).first
	);
}


const auto kTestFunctionArguments1 = "(int x, string y, float z)";
const string kTestFunctionArguments1JSON = R"(
	[
		{ "name": "x", "type": "<int>" },
		{ "name": "y", "type": "<string>" },
		{ "name": "z", "type": "<float>" },
	]
)";

QUARK_UNIT_TEST("", "parse_functiondef_arguments()", "Function definition 1 -- three arguments", "Correct output JSON"){
	ut_compare_jsons(
		json_value_t::make_array2(parse_functiondef_arguments(kTestFunctionArguments1)),
		parse_json(seq_t(kTestFunctionArguments1JSON)).first
	);
}


std::pair<json_value_t, std::string> parse_function_definition2(const string& pos){
	const auto return_type_pos = read_required_type_identifier(seq_t(pos));
	const auto function_name_pos = read_required_single_symbol(return_type_pos.second);

	//	Skip whitespace.
	const auto rest = skip_whitespace(function_name_pos.second);

	if(!if_first(rest, "(").first){
		throw std::runtime_error("expected function argument list enclosed by (),");
	}

	const auto arg_list_pos = get_balanced(rest);
	const auto args = parse_functiondef_arguments(arg_list_pos.first);
	const auto body_rest_pos = skip_whitespace(arg_list_pos.second);

	if(!if_first(body_rest_pos, "{").first){
		throw std::runtime_error("expected function body enclosed by {}.");
	}
	const auto body_pos = get_balanced(body_rest_pos);
	const auto function_name = function_name_pos.first;

	const auto statements = read_statements2(trim_ends(body_pos.first));

	json_value_t function_def = json_value_t::make_array2({
		"def-func",
		json_value_t::make_object({
			{ "name", function_name },
			{ "args", json_value_t::make_array2(args) },
			{ "statements", statements.first },
			{ "return_type", "<" + return_type_pos.first.to_string() + ">" }
		})
	});
	return { function_def, body_pos.second.get_s() };
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
				{ "args": [], "name": "f", "return_type": "<int>", "statements": [["return", ["k", 3, "<int>"]]] }
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
						{ "name": "a", "type": "<string>" },
						{ "name": "barry", "type": "<float>" },
						{ "name": "c", "type": "<int>" },
					],
					"name": "printf",
					"return_type": "<int>",
					"statements": [["return", ["k", 3, "<int>"]]]
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
						{ "name": "a", "type": "<string>" },
						{ "name": "b", "type": "<float>" }
					],
					"name": "printf",
					"return_type": "<int>",
					"statements": [["return", ["k", 3, "<int>"]]]
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
						{ "name": "a", "type": "<string>" },
						{ "name": "b", "type": "<float>" }
					],
					"name": "printf",
					"return_type": "<int>",
					"statements": [["return", ["k", 3, "<int>"]]]
				}
			]
		)"
	}
};


QUARK_UNIT_TEST("", "parse_function_definition2()", "BATCH", "Correct output JSON"){
	for(const auto e: testsxyz){
		QUARK_SCOPED_TRACE(e.desc);
		ut_compare_jsons(parse_function_definition2(e.input).first,parse_json(seq_t(e.output)).first);
	}
}

}	//	floyd_parser

