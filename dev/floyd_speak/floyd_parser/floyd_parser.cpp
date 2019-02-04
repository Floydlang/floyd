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
#include "parse_protocol_def.h"
#include "parser_primitives.h"
#include "json_parser.h"
#include "utils.h"


namespace floyd {


using namespace std;

/*
	AST ABSTRACT SYNTAX TREE

https://en.wikipedia.org/wiki/Abstract_syntax_tree

https://en.wikipedia.org/wiki/Parsing_expression_grammar
https://en.wikipedia.org/wiki/Parsing
*/


std::pair<ast_json_t, seq_t> parse_statement(const seq_t& s){
	const auto pos = skip_whitespace(s);
	if(is_first(pos, "{")){
		return parse_block(pos);
	}
	else if(is_first(pos, keyword_t::k_return)){
		return parse_return_statement(pos);
	}
	else if(is_first(pos, keyword_t::k_struct)){
		return parse_struct_definition(pos);
	}
	else if(is_first(pos, keyword_t::k_protocol)){
		return parse_protocol_definition(pos);
	}
	else if(is_first(pos, keyword_t::k_if)){
		return  parse_if_statement(pos);
	}
	else if(is_first(pos, keyword_t::k_for)){
		return parse_for_statement(pos);
	}
	else if(is_first(pos, keyword_t::k_while)){
		return parse_while_statement(pos);
	}
	else if(is_first(pos, keyword_t::k_func)){
		return parse_function_definition2(pos);
	}
	else if(is_first(pos, keyword_t::k_let)){
		return parse_bind_statement(pos);
	}
	else if(is_first(pos, keyword_t::k_mutable)){
		return parse_bind_statement(pos);
	}
	else if(is_first(pos, keyword_t::k_software_system)){
		return parse_software_system(pos);
	}
	else if(is_first(pos, keyword_t::k_container_def)){
		return parse_container_def(pos);
	}
	else {
		return parse_prefixless_statement(pos);
	}
}

QUARK_UNIT_TEST("", "parse_statement()", "", ""){
	ut_verify(QUARK_POS,
		parse_statement(seq_t("let int x = 10;")).first._value,
		parse_json(seq_t(R"([0, "bind", "^int", "x", ["k", 10, "^int"]])")).first
	);
}
QUARK_UNIT_TEST("", "parse_statement()", "", ""){
	ut_verify(QUARK_POS,
		parse_statement(seq_t("func int f(string name){ return 13; }")).first._value,
		parse_json(seq_t(R"(
			[
				0,
				"def-func",
				{
					"args": [{ "name": "name", "type": "^string" }],
					"name": "f",
					"return_type": "^int",
					"statements": [
						[25, "return", ["k", 13, "^int"]]
					],
					"impure": false
				}
			]
		)")).first
	);
}

QUARK_UNIT_TEST("", "parse_statement()", "", ""){
	ut_verify(QUARK_POS,
		parse_statement(seq_t("let int x = f(3);")).first._value,
		parse_json(seq_t(R"([0, "bind", "^int", "x", ["call", ["@", "f"], [["k", 3, "^int"]]]])")).first
	);
}


//	"a = 1; print(a)"
parse_result_t parse_statements_no_brackets(const seq_t& s){
	vector<json_t> statements;

	auto pos = skip_whitespace(s);

	while(pos.empty() == false){
		const auto statement_pos = parse_statement(pos);
		QUARK_ASSERT(statement_pos.second.pos() >= pos.pos());

		statements.push_back(statement_pos.first._value);

		auto pos2 = skip_whitespace(statement_pos.second);

		//	Skip optional ;
		while(pos2.empty() == false && pos2.first1_char() == ';'){
			pos2 = pos2.rest1();
			pos2 = skip_whitespace(pos2);
		}

		QUARK_ASSERT(pos2.pos() >= pos.pos());
		pos = pos2;
	}
	return { ast_json_t::make(statements), pos };
}

//	"{ a = 1; print(a) }"
parse_result_t parse_statements_bracketted(const seq_t& s){
	vector<json_t> statements;

	auto pos = skip_whitespace(s);
	pos = read_required(pos, "{");
	pos = skip_whitespace(pos);

	while(pos.empty() == false && pos.first() != "}"){
		const auto statement_pos = parse_statement(pos);
		QUARK_ASSERT(statement_pos.second.pos() >= pos.pos());

		statements.push_back(statement_pos.first._value);

		auto pos2 = skip_whitespace(statement_pos.second);

		//	Skip optional ;
		while(pos2.empty() == false && pos2.first1_char() == ';'){
			pos2 = pos2.rest1();
			pos2 = skip_whitespace(pos2);
		}

		QUARK_ASSERT(pos2.pos() >= pos.pos());
		pos = pos2;
	}
	if(pos.first() == "}"){
		return { ast_json_t::make(statements), pos.rest() };
	}
	else{
		throw std::runtime_error("Missing end bracket \'}\'.");
	}
}

QUARK_UNIT_TEST("", "parse_statements_bracketted()", "", ""){
	ut_verify(QUARK_POS,
		parse_statement_body(seq_t(" { } ")).ast._value,
		parse_json(seq_t(
			R"(
				[]
			)"
		)).first
	);
}
QUARK_UNIT_TEST("", "parse_statements_bracketted()", "", ""){
	ut_verify(QUARK_POS,
		parse_statement_body(seq_t(" { let int x = 1; let int y = 2; } ")).ast._value,
		parse_json(seq_t(
			R"(
				[
					[3, "bind", "^int", "x", ["k", 1, "^int"]],
					[18, "bind", "^int", "y", ["k", 2, "^int"]]
				]
			)"
		)).first
	);
}






parse_result_t parse_program2(const string& program){
	const auto statements_pos = parse_statements_no_brackets(seq_t(program));
	return { statements_pos.ast, statements_pos.pos };
}


//////////////////////////////////////////////////		Test programs






const std::string k_test_program_0_source = "func int main(){ return 3; }";
const std::string k_test_program_0_parserout = R"(
	[
		[
			0,
			"def-func",
			{
				"args": [],
				"name": "main",
				"return_type": "^int",
				"statements": [
					[ 17, "return", [ "k", 3, "^int" ] ]
				],
				"impure": false
			}
		]
	]
)";

QUARK_UNIT_TEST("", "parse_program1()", "k_test_program_0_source", ""){
	ut_verify(QUARK_POS,
		parse_program2(k_test_program_0_source).ast._value,
		parse_json(seq_t(k_test_program_0_parserout)).first
	);
}



const std::string k_test_program_1_source =
	"func int main(string args){\n"
	"	return 3;\n"
	"}\n";
const std::string k_test_program_1_parserout = R"(
	[
		[
			0,
			"def-func",
			{
				"args": [
					{ "name": "args", "type": "^string" }
				],
				"name": "main",
				"return_type": "^int",
				"statements": [
					[ 29, "return", [ "k", 3, "^int" ] ]
				],
				"impure": false
			}
		]
	]
)";

QUARK_UNIT_TEST("", "parse_program1()", "k_test_program_1_source", ""){
	ut_verify(QUARK_POS,
		parse_program2(k_test_program_1_source).ast._value,
		parse_json(seq_t(k_test_program_1_parserout)).first
	);
}



const char k_test_program_100_parserout[] = R"(
	[
		[
			5,
			"def-struct",
			{
				"members": [
					{ "name": "red", "type": "^double" },
					{ "name": "green", "type": "^double" },
					{ "name": "blue", "type": "^double" }
				],
				"name": "pixel"
			}
		],
		[
			65,
			"def-func",
			{
				"args": [{ "name": "p", "type": "#pixel" }],
				"name": "get_grey",
				"return_type": "^double",
				"statements": [
					[
						96,
						"return",
						[
							"/",
							[
								"+",
								["+", ["->", ["@", "p"], "red"], ["->", ["@", "p"], "green"]],
								["->", ["@", "p"], "blue"]
							],
							["k", 3.0, "^double"]
						]
					]
				],
				"impure": false
			}
		],
		[
			144,
			"def-func",
			{
				"args": [],
				"name": "main",
				"return_type": "^double",
				"statements": [
					[
						169,
						"bind",
						"#pixel",
						"p",
						["call", ["@", "pixel"], [["k", 1, "^int"], ["k", 0, "^int"], ["k", 0, "^int"]]]
					],
					[204, "return", ["call", ["@", "get_grey"], [["@", "p"]]]]
				],
				"impure": false
			}
		]
	]
)";

QUARK_UNIT_TEST("", "parse_program2()", "k_test_program_100_source", ""){
	ut_verify(QUARK_POS,
		parse_program2(
			R"(
				struct pixel { double red; double green; double blue; }
				func double get_grey(pixel p){ return (p.red + p.green + p.blue) / 3.0; }

				func double main(){
					let pixel p = pixel(1, 0, 0);
					return get_grey(p);
				}
			)"
		).ast._value,
		parse_json(seq_t(k_test_program_100_parserout)).first
	);
}

}	//	namespace floyd
