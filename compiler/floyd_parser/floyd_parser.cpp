//
//  main.cpp
//  Floyd
//
//  Created by Marcus Zetterquist on 27/03/16.
//  Copyright © 2016 Marcus Zetterquist. All rights reserved.
//

#include "floyd_parser.h"

#include "parse_statement.h"
#include "parse_expression.h"
#include "parser_primitives.h"
#include "json_support.h"
#include "utils.h"
#include "floyd_syntax.h"
#include "compiler_basics.h"
#include "typeid.h"

static const bool k_trace_parse_tree_flag = false;

namespace floyd {



namespace parser {

std::pair<json_t, seq_t> parse_prefixless_statement(const seq_t& s);


std::pair<json_t, seq_t> parse_statement(const seq_t& s){
	const auto pos = skip_whitespace(s);
	try {
		if(is_first(pos, "{")){
			return parse_block(pos);
		}
		else if(is_first(pos, keyword_t::k_return)){
			return parse_return_statement(pos);
		}
		else if(is_first(pos, keyword_t::k_struct)){
			return parse_struct_definition_statement(pos);
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
			return parse_function_definition_statement(pos);
		}
		else if(is_first(pos, keyword_t::k_let)){
			return parse_bind_statement(pos);
		}
		else if(is_first(pos, keyword_t::k_mutable)){
			return parse_bind_statement(pos);
		}
		else if(is_first(pos, keyword_t::k_software_system)){
			return parse_software_system_def_statement(pos);
		}
		else if(is_first(pos, keyword_t::k_container_def)){
			return parse_container_def_statement(pos);
		}
		else if(is_first(pos, keyword_t::k_benchmark_def)){
			return parse_benchmark_def_statement(pos);
		}
		else {
			//	k_assign and k_expression_statement has no prefix, we need to figure out if it's one of those.
			return parse_prefixless_statement(pos);
		}
	}

	//	If an exception other than compiler_error is thrown, make a compiler error with location info.
	catch(const compiler_error& e){
		if(e.location == k_no_location){
			QUARK_ASSERT(e.location2.loc == k_no_location);
			throw_compiler_error(location_t(pos.pos()), e.what());
		}
		else{
			throw;
		}
	}
	catch(const std::runtime_error& e){
		throw_compiler_error(location_t(pos.pos()), e.what());
	}
	catch(const std::exception& e){
		throw_compiler_error(location_t(pos.pos()), "Failed to parse statement.");
	}
}

QUARK_TEST("", "parse_statement()", "", ""){
	ut_verify(QUARK_POS,
		parse_statement(seq_t("let int x = 10;")).first,
		parse_json(seq_t(R"([0, "init-local", "^int", "x", ["k", 10, "^int"]])")).first
	);
}

QUARK_TEST("", "parse_statement()", "", ""){
	ut_verify(QUARK_POS,
		parse_statement(seq_t("func int f(string name){ return 13; }")).first,
		parse_json(seq_t(R"(
			[
				0,
				"init-local",
				["func", "^int", ["^string"], true],
				"f",
				[
					"function-def",
					["func", "^int", ["^string"], true],
					"f",
					[{ "name": "name", "type": "^string" }],
					{ "statements": [[25, "return", ["k", 13, "^int"]]], "symbols": null }
				]
			]
		)")).first
	);
}

QUARK_TEST("", "parse_statement()", "", ""){
	ut_verify(QUARK_POS,
		parse_statement(seq_t("let int x = f(3);")).first,
		parse_json(seq_t(R"([0, "init-local", "^int", "x", ["call", ["@", "f"], [["k", 3, "^int"]]]])")).first
	);
}


parse_result_t parse_statements_no_brackets(const seq_t& s){
	std::vector<json_t> statements;

	auto pos = skip_whitespace(s);

	while(pos.empty() == false){
		const auto statement_pos = parse_statement(pos);
		QUARK_ASSERT(statement_pos.second.pos() >= pos.pos());

		statements.push_back(statement_pos.first);

		auto pos2 = skip_whitespace(statement_pos.second);

		//	Skip optional ;
		while(pos2.empty() == false && pos2.first1_char() == ';'){
			pos2 = pos2.rest1();
			pos2 = skip_whitespace(pos2);
		}

		QUARK_ASSERT(pos2.pos() >= pos.pos());
		pos = pos2;
	}
	return { statements, pos };
}

//	"{ a = 1; print(a) }"
parse_result_t parse_statements_bracketted(const seq_t& s){
	std::vector<json_t> statements;

	auto pos = skip_whitespace(s);
	pos = read_required(pos, "{");
	pos = skip_whitespace(pos);

	while(pos.empty() == false && pos.first() != "}"){
		const auto statement_pos = parse_statement(pos);
		QUARK_ASSERT(statement_pos.second.pos() >= pos.pos());

		statements.push_back(statement_pos.first);

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
		return { statements, pos.rest() };
	}
	else{
		throw_compiler_error_nopos("Block is missing end bracket \'}\'.");
	}
}

QUARK_TEST("", "parse_statements_bracketted()", "", ""){
	ut_verify(QUARK_POS,
		parse_statement_body(seq_t(" { } ")).parse_tree,
		parse_json(seq_t(
			R"(
				[]
			)"
		)).first
	);
}
QUARK_TEST("", "parse_statements_bracketted()", "", ""){
	ut_verify(QUARK_POS,
		parse_statement_body(seq_t(" { let int x = 1; let int y = 2; } ")).parse_tree,
		parse_json(seq_t(
			R"(
				[
					[3, "init-local", "^int", "x", ["k", 1, "^int"]],
					[18, "init-local", "^int", "y", ["k", 2, "^int"]]
				]
			)"
		)).first
	);
}

void check_illegal_chars(const seq_t& p){
	const auto illegal_char = read_while(p, k_valid_expression_chars);
	const auto pos = illegal_char.first.size();
	if(pos < p.size()){
		throw_compiler_error(location_t(pos), "Illegal characters.");
	}
}

parse_tree_t parse_program2(const std::string& program){
	try {
		const auto pos = seq_t(program);
		check_illegal_chars(pos);

		const auto statements_pos = parse_statements_no_brackets(pos);
		const auto p = parse_tree_t{ statements_pos.parse_tree };

		if(k_trace_parse_tree_flag){
			QUARK_SCOPED_TRACE("Parser tree output");
			QUARK_TRACE(json_to_pretty_string(p._value));
		}

		return p;
	}
	catch(const compiler_error& e){
		const auto what = e.what();
		const auto what2 = std::string("[Syntax] ") + what;
		throw compiler_error(e.location, e.location2, what2);
	}
}

const std::string k_test_program_0_source = "func int main(){ return 3; }";
const std::string k_test_program_0_parserout = R"(
	[
		[
			0,
			"init-local",
			["func", "^int", [], true],
			"main",
			["function-def", ["func", "^int", [], true], "main", [], { "statements": [[17, "return", ["k", 3, "^int"]]], "symbols": null }]
		]
	]
)";

QUARK_TEST("", "parse_program2()", "k_test_program_0_source", ""){
	ut_verify(QUARK_POS,
		parse_program2(k_test_program_0_source)._value,
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
			"init-local",
			["func", "^int", ["^string"], true],
			"main",
			[
				"function-def",
				["func", "^int", ["^string"], true],
				"main",
				[{ "name": "args", "type": "^string" }],
				{ "statements": [[29, "return", ["k", 3, "^int"]]], "symbols": null }
			]
		]
	]
)";

QUARK_TEST("", "parse_program2()", "k_test_program_1_source", ""){
	ut_verify(QUARK_POS,
		parse_program2(k_test_program_1_source)._value,
		parse_json(seq_t(k_test_program_1_parserout)).first
	);
}

//	???named-types
#if 0
const char k_test_program_100_parserout[] = R"(
	[
		[
			18,
			"expression-statement",
			["struct-def", "pixel", [{ "name": "red", "type": "^double" }, { "name": "green", "type": "^double" }, { "name": "blue", "type": "^double" }]]
		],
		[
			65,
			"init-local",
			["func", "^double", ["#pixel"], true],
			"get_grey",
			[
				"function-def",
				["func", "^double", ["#pixel"], true],
				"get_grey",
				[{ "name": "p", "type": "#pixel" }],
				{
					"statements": [
						[96, "return", ["/", ["+", ["+", ["->", ["@", "p"], "red"], ["->", ["@", "p"], "green"]], ["->", ["@", "p"], "blue"]], ["k", 3, "^double"]]]
					],
					"symbols": null
				}
			]
		],
		[
			144,
			"init-local",
			["func", "^double", [], true],
			"main",
			[
				"function-def",
				["func", "^double", [], true],
				"main",
				[],
				{
					"statements": [
						[169, "init-local", "#pixel", "p", ["call", ["@", "pixel"], [["k", 1, "^int"], ["k", 0, "^int"], ["k", 0, "^int"]]]],
						[204, "return", ["call", ["@", "get_grey"], [["@", "p"]]]]
					],
					"symbols": null
				}
			]
		]
	]
)";
#else
const char k_test_program_100_parserout[] = R"(
	[
		[
			18,
			"init-local",
			"^typeid",
			"pixel",
			["struct-def", "pixel", [{ "name": "red", "type": "^double" }, { "name": "green", "type": "^double" }, { "name": "blue", "type": "^double" }]]
		],
		[
			65,
			"init-local",
			["func", "^double", ["#pixel"], true],
			"get_grey",
			[
				"function-def",
				["func", "^double", ["#pixel"], true],
				"get_grey",
				[{ "name": "p", "type": "#pixel" }],
				{
					"statements": [
						[96, "return", ["/", ["+", ["+", ["->", ["@", "p"], "red"], ["->", ["@", "p"], "green"]], ["->", ["@", "p"], "blue"]], ["k", 3, "^double"]]]
					],
					"symbols": null
				}
			]
		],
		[
			144,
			"init-local",
			["func", "^double", [], true],
			"main",
			[
				"function-def",
				["func", "^double", [], true],
				"main",
				[],
				{
					"statements": [
						[169, "init-local", "#pixel", "p", ["call", ["@", "pixel"], [["k", 1, "^int"], ["k", 0, "^int"], ["k", 0, "^int"]]]],
						[204, "return", ["call", ["@", "get_grey"], [["@", "p"]]]]
					],
					"symbols": null
				}
			]
		]
	]
)";
#endif

QUARK_TEST("", "parse_program2()", "k_test_program_100_source", ""){
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
		)._value,
		parse_json(seq_t(k_test_program_100_parserout)).first
	);
}


//////////////////////////////////////////////////		detect_implicit_statement_lookahead()


enum class implicit_statement {
	k_error,
	k_expression_statement,
	k_assign
};

bool is_identifier_and_equal(const seq_t& s){
	const auto identifier_fr = read_identifier(s);
	const auto next_seq = skip_whitespace(identifier_fr.second);
	if(identifier_fr.first.empty() == false && next_seq.first1() == "="){
		return true;
	}
	else{
		return false;
	}
}

static implicit_statement detect_implicit_statement_lookahead(const seq_t& s){
	if(is_identifier_and_equal(s)){
		return implicit_statement::k_assign;
	}
	else{
		//	Detect "int test = 123" which is common illegal syntax, where you forgot "mutable" or "let".

		try {
			const auto maybe_type = read_type(s);
			if(maybe_type.first != nullptr){
				if(maybe_type.first->is_function()){
					throw_compiler_error_nopos("Function types not supported.");
				}
				if(is_identifier_and_equal(maybe_type.second)){
					return implicit_statement::k_error;
				}
			}
		}
		catch(...){
		}
		return implicit_statement::k_expression_statement;
	}
}

//#define DETECT_TEST QUARK_TEST_VIP
#define DETECT_TEST QUARK_TEST

DETECT_TEST("", "detect_implicit_statement_lookahead()", "", "ERROR"){
	QUARK_ASSERT(detect_implicit_statement_lookahead(seq_t(R"(	int test = 123 xyz	)")) == implicit_statement::k_error);
}

DETECT_TEST("", "detect_implicit_statement_lookahead()", "", "EXPRESSION-STATEMENT"){
	QUARK_UT_VERIFY(detect_implicit_statement_lookahead(seq_t(R"(	print("B:" + to_string(x))	{ print(3) int x = 4 } xyz	)")) == implicit_statement::k_expression_statement);
}
DETECT_TEST("", "detect_implicit_statement_lookahead()", "", "EXPRESSION-STATEMENT"){
	QUARK_UT_VERIFY(detect_implicit_statement_lookahead(seq_t(R"(	print(3) int x = 4	xyz	)")) == implicit_statement::k_expression_statement);
}


DETECT_TEST("", "detect_implicit_statement_lookahead()", "", "EXPRESSION-STATEMENT"){
	QUARK_UT_VERIFY(detect_implicit_statement_lookahead(seq_t(" print ( \"Hello, World!\" )		xyz")) == implicit_statement::k_expression_statement);
	QUARK_UT_VERIFY(detect_implicit_statement_lookahead(seq_t(R"( print ( "Hello, World!" )		xyz)")) == implicit_statement::k_expression_statement);
}
DETECT_TEST("", "detect_implicit_statement_lookahead()", "", "EXPRESSION-STATEMENT"){
	QUARK_UT_VERIFY(detect_implicit_statement_lookahead(seq_t("print(\"Hello, World!\")		xyz")) == implicit_statement::k_expression_statement);
	QUARK_UT_VERIFY(detect_implicit_statement_lookahead(seq_t(R"(print("Hello, World!")		xyz)")) == implicit_statement::k_expression_statement);
}
DETECT_TEST("", "detect_implicit_statement_lookahead()", "", "EXPRESSION-STATEMENT"){
	QUARK_UT_VERIFY(detect_implicit_statement_lookahead(seq_t(R"(     print("/Desktop/test_out.txt")		xyz)")) == implicit_statement::k_expression_statement);
}
DETECT_TEST("", "detect_implicit_statement_lookahead()", "", "EXPRESSION-STATEMENT"){
	QUARK_UT_VERIFY(detect_implicit_statement_lookahead(seq_t("print(3)		xyz")) == implicit_statement::k_expression_statement);
}
DETECT_TEST("", "detect_implicit_statement_lookahead()", "", "EXPRESSION-STATEMENT"){
	QUARK_UT_VERIFY(detect_implicit_statement_lookahead(seq_t("3		xyz")) == implicit_statement::k_expression_statement);
}
DETECT_TEST("", "detect_implicit_statement_lookahead()", "", "EXPRESSION-STATEMENT"){
	QUARK_UT_VERIFY(detect_implicit_statement_lookahead(seq_t("3 + 4		xyz")) == implicit_statement::k_expression_statement);
}
DETECT_TEST("", "detect_implicit_statement_lookahead()", "", "EXPRESSION-STATEMENT"){
	QUARK_UT_VERIFY(detect_implicit_statement_lookahead(seq_t("3 + f(1) + f(2)		xyz")) == implicit_statement::k_expression_statement);
}


DETECT_TEST("", "detect_implicit_statement_lookahead()", "", "assign"){
	QUARK_UT_VERIFY(detect_implicit_statement_lookahead(seq_t(" x = 10		xyz")) == implicit_statement::k_assign);
}
DETECT_TEST("", "detect_implicit_statement_lookahead()", "", "assign"){
	QUARK_UT_VERIFY(detect_implicit_statement_lookahead(seq_t(" x = \"hello\"		xyz")) == implicit_statement::k_assign);
}
DETECT_TEST("", "detect_implicit_statement_lookahead()", "", "assign"){
	QUARK_UT_VERIFY(detect_implicit_statement_lookahead(seq_t(" x = f ( 3 ) == 2		xyz")) == implicit_statement::k_assign);
}
DETECT_TEST("", "detect_implicit_statement_lookahead()", "vector", "assign"){
	QUARK_UT_VERIFY(detect_implicit_statement_lookahead(seq_t("a = [1,2,3]		xyz")) == implicit_statement::k_assign);
}
DETECT_TEST("", "detect_implicit_statement_lookahead()", "dict", "assign"){
	QUARK_UT_VERIFY(detect_implicit_statement_lookahead(seq_t(R"(a = {"uno": 1, "duo": 2}		xyz)")) == implicit_statement::k_assign);
}

/*
	Detects each of the other implicit statements and parses them.
	a = EXPRESSIONm like "a = sin(1.3)"
	or
	EXPRESSION, like "print(3)"
*/
std::pair<json_t, seq_t> parse_prefixless_statement(const seq_t& s){
	const auto pos = skip_whitespace(s);
	const auto implicit_type = detect_implicit_statement_lookahead(pos);
	if(implicit_type == implicit_statement::k_expression_statement){
		return parse_expression_statement(pos);
	}
	else if(implicit_type == implicit_statement::k_assign){
		return parse_assign_statement(pos);
	}
	else{
		throw_compiler_error_nopos("Use 'mutable' or 'let' syntax.");
	}
}

/*
QUARK_TEST("", "parse_prefixless_statement()", "", ""){
	ut_verify(QUARK_POS,
		parse_prefixless_statement(seq_t("x = f(3);")).first._value,
		parse_json(seq_t(R"(["init-local", "^int", "x", ["call", ["@", "f"], [["k", 3, "^int"]]]])")).first
	);
}
*/

}	// parser
}	// floyd
