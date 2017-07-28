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
		const auto arg_type = read_type(str);
		const auto arg_name = read_required_single_symbol(arg_type.second);
		const auto optional_comma = read_optional_char(skip_whitespace(arg_name.second), ',');

		const auto a = make_member_def("<" + arg_type.first + ">", arg_name.first, json_value_t());
		args.push_back(a);
		str = skip_whitespace(optional_comma.second);
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

//??? Replace this with template function or something. These functions are just duplication.
QUARK_UNIT_TEST("", "parse_functiondef_arguments()", "Function definition 1 -- three arguments", "Correct output JSON"){
	ut_compare_jsons(
		json_value_t::make_array2(parse_functiondef_arguments(kTestFunctionArguments1)),
		parse_json(seq_t(kTestFunctionArguments1JSON)).first
	);
}


std::pair<json_value_t, std::string> parse_function_definition2(const string& pos){
	const auto return_type_pos = read_required_type_identifier(pos);
	const auto function_name_pos = read_required_single_symbol(return_type_pos.second);

	//	Skip whitespace.
	const auto rest = skip_whitespace(function_name_pos.second);

	if(!peek_compare_char(rest, '(')){
		throw std::runtime_error("expected function argument list enclosed by (),");
	}

	const auto arg_list_pos = get_balanced(rest);
	const auto args = parse_functiondef_arguments(arg_list_pos.first);
	const auto body_rest_pos = skip_whitespace(arg_list_pos.second);

	if(!peek_compare_char(body_rest_pos, '{')){
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
	return { function_def, body_pos.second };
}

const auto kTestFunctionDefinition0 = "int f(){ return 3; }";
const string kTestFunctionDefinition0JSON = R"(
	[
		"def-func",
		{ "args": [], "name": "f", "return_type": "<int>", "statements": [["return", ["k", 3, "<int>"]]] }
	]
)";

const auto kTestFunctionDefinition1 = "int printf(string a, float barry, int c){ return 3; }";
const string kTestFunctionDefinition1JSON = R"(
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
)";

const auto kTestFunctionDefinition2 = " \t int \t printf( \t string \t a \t , \t float \t b \t ){ \t return \t 3 \t ; \t } \t ";
const string kTestFunctionDefinition2JSON = R"(
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
)";


QUARK_UNIT_TEST("", "parse_function_definition2()", "Function definition 0 -- mininal function", "Correct output JSON"){
	ut_compare_jsons(
		parse_function_definition2(kTestFunctionDefinition0).first,
		parse_json(seq_t(kTestFunctionDefinition0JSON)).first
	);
}

QUARK_UNIT_TEST("", "parse_function_definition2()", "Function definition 1 -- 3 args of different types", "Correct output JSON"){
	ut_compare_jsons(
		parse_function_definition2(kTestFunctionDefinition1).first,
		parse_json(seq_t(kTestFunctionDefinition1JSON)).first
	);
}

QUARK_UNIT_TEST("", "parse_function_definition2()", "Function definition 2 -- Max whitespace", "Correct output JSON"){
	ut_compare_jsons(
		parse_function_definition2(kTestFunctionDefinition2).first,
		parse_json(seq_t(kTestFunctionDefinition2JSON)).first
	);
}

}	//	floyd_parser

