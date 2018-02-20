//
//  main.cpp
//  FloydSpeak
//
//  Created by Marcus Zetterquist on 27/03/16.
//  Copyright © 2016 Marcus Zetterquist. All rights reserved.
//

#include "floyd_parser.h"

#include "parse_prefixless_statement.h"
#include "parse_statement.h"
#include "parse_expression.h"
#include "parse_function_def.h"
#include "parse_struct_def.h"
#include "parser_primitives.h"
#include "json_parser.h"


namespace floyd {


using namespace std;

/*
	AST ABSTRACT SYNTAX TREE

https://en.wikipedia.org/wiki/Abstract_syntax_tree

https://en.wikipedia.org/wiki/Parsing_expression_grammar
https://en.wikipedia.org/wiki/Parsing
*/

const parser_context_t test_context{ quark::trace_context_t(true, quark::get_runtime()) };


std::pair<ast_json_t, seq_t> parse_statement(const seq_t& s){
	const auto pos = skip_whitespace(s);
	if(is_first(pos, "{")){
		return parse_block(pos);
	}
	else if(is_first(pos, keyword_t::k_return)){
		return parse_return_statement(pos);
	}
	else if(is_first(pos, keyword_t::k_struct)){
		return parse_struct_definition(seq_t(pos));
	}
	else if(is_first(pos, keyword_t::k_if)){
		return  parse_if_statement(seq_t(pos));
	}
	else if(is_first(pos, keyword_t::k_for)){
		return parse_for_statement(seq_t(pos));
	}
	else if(is_first(pos, keyword_t::k_while)){
		return parse_while_statement(seq_t(pos));
	}
	else {
		return parse_prefixless_statement(pos);
	}
}

QUARK_UNIT_TEST("", "parse_statement()", "", ""){
	ut_compare_jsons(
		parse_statement(seq_t("int x = 10;")).first._value,
		parse_json(seq_t(R"(["bind", "int", "x", ["k", 10, "int"]])")).first
	);
}
QUARK_UNIT_TEST("", "parse_statement()", "", ""){
	ut_compare_jsons(
		parse_statement(seq_t("int f(string name){ return 13; }")).first._value,
		parse_json(seq_t(R"(
			[
				"def-func",
				{
					"args": [{ "name": "name", "type": "string" }],
					"name": "f",
					"return_type": "int",
					"statements": [
						["return", ["k", 13, "int"]]
					]
				}
			]
		)")).first
	);
}

QUARK_UNIT_TEST("", "parse_statement()", "", ""){
	ut_compare_jsons(
		parse_statement(seq_t("int x = f(3);")).first._value,
		parse_json(seq_t(R"(["bind", "int", "x", ["call", ["@", "f"], [["k", 3, "int"]]]])")).first
	);
}



std::pair<ast_json_t, seq_t> parse_statements(const seq_t& s){
	vector<json_t> statements;
	auto pos = skip_whitespace(s);
	while(!pos.empty()){
		const auto statement_pos = parse_statement(pos);
		statements.push_back(statement_pos.first._value);
		pos = skip_whitespace(statement_pos.second);
	}
	return { ast_json_t{json_t::make_array(statements)}, pos };
}

ast_json_t parse_program2(const parser_context_t& context, const string& program){
	const auto statements_pos = parse_statements(seq_t(program));
	QUARK_TRACE(json_to_pretty_string(statements_pos.first._value));
	return statements_pos.first;
}



//////////////////////////////////////////////////		Test programs





const std::string k_test_program_0_source = "int main(){ return 3; }";
const std::string k_test_program_0_parserout = R"(
	[
		[
			"def-func",
			{
				"args": [],
				"name": "main",
				"return_type": "int",
				"statements": [
					[ "return", [ "k", 3, "int" ] ]
				]
			}
		]
	]
)";




const std::string k_test_program_1_source =
	"int main(string args){\n"
	"	return 3;\n"
	"}\n";
const std::string k_test_program_1_parserout = R"(
	[
		[
			"def-func",
			{
				"args": [
					{ "name": "args", "type": "string" }
				],
				"name": "main",
				"return_type": "int",
				"statements": [
					[ "return", [ "k", 3, "int" ] ]
				]
			}
		]
	]
)";


const char k_test_program_100_source[] = R"(
	struct pixel { float red; float green; float blue; }
	float get_grey(pixel p){ return (p.red + p.green + p.blue) / 3.0; }

	float main(){
		pixel p = pixel(1, 0, 0);
		return get_grey(p);
	}
)";
const char k_test_program_100_parserout[] = R"(
	[
		[
			"def-struct",
			{
				"members": [
					{ "name": "red", "type": "float" },
					{ "name": "green", "type": "float" },
					{ "name": "blue", "type": "float" }
				],
				"name": "pixel"
			}
		],
		[
			"def-func",
			{
				"args": [{ "name": "p", "type": "pixel" }],
				"name": "get_grey",
				"return_type": "float",
				"statements": [
					[
						"return",
						[
							"/",
							[
								"+",
								["+", ["->", ["@", "p"], "red"], ["->", ["@", "p"], "green"]],
								["->", ["@", "p"], "blue"]
							],
							["k", 3.0, "float"]
						]
					]
				]
			}
		],
		[
			"def-func",
			{
				"args": [],
				"name": "main",
				"return_type": "float",
				"statements": [
					[
						"bind",
						"pixel",
						"p",
						["call", ["@", "pixel"], [["k", 1, "int"], ["k", 0, "int"], ["k", 0, "int"]]]
					],
					["return", ["call", ["@", "get_grey"], [["@", "p"]]]]
				]
			}
		]
	]
)";

QUARK_UNIT_TEST("", "parse_program1()", "k_test_program_0_source", ""){
	ut_compare_jsons(
		parse_program2(test_context, k_test_program_0_source)._value,
		parse_json(seq_t(k_test_program_0_parserout)).first
	);
}

QUARK_UNIT_TEST("", "parse_program1()", "k_test_program_1_source", ""){
	ut_compare_jsons(
		parse_program2(test_context, k_test_program_1_source)._value,
		parse_json(seq_t(k_test_program_1_parserout)).first
	);
}

QUARK_UNIT_TEST("", "parse_program2()", "k_test_program_100_source", ""){
	ut_compare_jsons(
		parse_program2(test_context, k_test_program_100_source)._value,
		parse_json(seq_t(k_test_program_100_parserout)).first
	);
}

}	//	namespace floyd
