//
//  main.cpp
//  FloydSpeak
//
//  Created by Marcus Zetterquist on 27/03/16.
//  Copyright © 2016 Marcus Zetterquist. All rights reserved.
//

#include "floyd_parser.h"

#include "parser_primitives.h"
#include "text_parser.h"
#include "parse_statement.h"
#include "parse_expression.h"
#include "parse_function_def.h"
#include "parse_struct_def.h"
#include "utils.h"
#include "json_support.h"
#include "json_parser.h"

#include <string>
#include <memory>
#include <map>
#include <iostream>
#include <cmath>

namespace floyd_parser {


using namespace std;

static std::pair<json_t, seq_t> read_statement2(const seq_t& pos);

/*
	AST ABSTRACT SYNTAX TREE

https://en.wikipedia.org/wiki/Abstract_syntax_tree

https://en.wikipedia.org/wiki/Parsing_expression_grammar
https://en.wikipedia.org/wiki/Parsing
*/


/*
	A:
		if (2 > 1){
			...
		}


	B:
		if (2 > 1){
			...
		}
		else{
			...
		}

	C:
		if (2 > 1){
			...
		}
		else if(2 > 3){
			...
		}
		else{
			...
		}


	OUTPUT
		??? introduces a block.
		["if", EXPRESSION, [STATEMENT1, STATEMENT2] ]

		??? ELSE, ELSE IF?
		["if", EXPRESSION, [STATEMENT1, STATEMENT2] ]
*/

std::pair<json_t, seq_t> parse_if_statement(const seq_t& pos){
	std::pair<bool, seq_t> a = if_first(pos, "if");
	QUARK_ASSERT(a.first);

	const auto pos2 = skip_whitespace(a.second);

	if(pos2.first1_char() != '('){
		throw std::runtime_error("syntax error");
	}

	const auto header = get_balanced(pos2);
	const auto pos3 = skip_whitespace(header.second);
	const auto body = get_balanced(pos3);

	const auto condition = parse_expression_all(seq_t(trim_ends(header.first)));
	const auto statements = read_statement2(seq_t(trim_ends(body.first)));

	return { json_t{}, body.second };
}


QUARK_UNIT_TEST("", "parse_if_statement()", "if(){}", ""){
	ut_compare_jsons(
		parse_if_statement(seq_t("if (1 > 2) { return 3; }")).first,
		parse_json(seq_t("null")).first
	);
}


/*
	for ( int x = 0 ; x < 10 ; x++ ){
		...
	}
*/
std::pair<json_t, seq_t> parse_for_statement(const seq_t& pos){
	std::pair<bool, seq_t> a = if_first(pos, "for");
	QUARK_ASSERT(a.first);

	const auto pos2 = skip_whitespace(a.second);

	if(pos2.first1_char() != '('){
		throw std::runtime_error("syntax error");
	}

	const auto header = get_balanced(pos2);
	const auto pos3 = skip_whitespace(header.second);
	const auto body = get_balanced(pos3);

	const auto init_statement = read_until(seq_t(trim_ends(header.first)), ";");
	const auto condition_expression = read_until(init_statement.second.rest1(), ";");
	const auto post_statement = condition_expression.second.rest1();

	const auto init_statement2 = read_statement2(seq_t(init_statement.first));
	const auto condition_expression2 = parse_expression_all(seq_t(condition_expression.first));
	const auto statements = read_statement2(post_statement);

	return { json_t{}, body.second };
}

QUARK_UNIT_TEST("", "parse_for_statement()", "for(){}", ""){
	ut_compare_jsons(
		parse_for_statement(seq_t("for ( int x = 0 ; x < 10 ; int x = 10 ) { a = 42; }")).first,
		parse_json(seq_t("null")).first
	);
}


//////////////////////////////////////////////////		read_statement()


/*
	Read one statement, including any expressions it uses.
	Supports all statments:
		- return statement
		- struct-definition
		- function-definition
		- define constant, with initializating.

	Never simplifes expressions- the parser is non-lossy.

	OUTPUT

	["return", EXPRESSION ]
	["bind", "<string>", "local_name", EXPRESSION ]
	["def_struct", STRUCT_DEF ]
	["define_function", FUNCTION_DEF ]
*/

static std::pair<json_t, seq_t> read_statement2(const seq_t& pos0){
	const auto pos = skip_whitespace(pos0);
	const auto token_pos = read_until(pos, whitespace_chars);

	//	return statement?
	if(token_pos.first == "return"){
		const auto return_statement_pos = parse_return_statement(pos);
		return { return_statement_pos.first, skip_whitespace(return_statement_pos.second) };
	}

	//	struct definition?
	else if(token_pos.first == "struct"){
		const auto a = parse_struct_definition(seq_t(pos));
		return { a.first, skip_whitespace(a.second) };
	}

	else if(token_pos.first == "if"){
		const auto a = parse_if_statement(seq_t(pos));
		return { a.first, skip_whitespace(a.second) };
	}
	else if(token_pos.first == "for"){
		const auto a = parse_for_statement(seq_t(pos));
		return { a.first, skip_whitespace(a.second) };
	}
	else {
		const auto type_pos = read_required_type_identifier(seq_t(pos));
		const auto identifier_pos = read_required_single_symbol(type_pos.second);

		/*
			Function definition?
			"int xyz(string a, string b){ ... }
		*/
		if(if_first(skip_whitespace(identifier_pos.second), "(").first){
			const auto function = parse_function_definition2(pos);
            return { function.first, skip_whitespace(seq_t(function.second)) };
		}

		/*
			Define variable?

			"int a = 10;"
			"string hello = f(a) + \"_suffix\";";
		*/
		else if(if_first(skip_whitespace(identifier_pos.second), "=").first){
			const auto assignment_statement = parse_assignment_statement(pos);
			return { assignment_statement.first, skip_whitespace(assignment_statement.second) };
		}

		else{
			throw std::runtime_error("syntax error");
		}
	}
}

std::pair<json_t, seq_t> read_statements2(const seq_t& s){
	QUARK_ASSERT(s.size() > 0);

	vector<json_t> statements;
	auto pos = skip_whitespace(s);
	while(!pos.empty()){
		const auto statement_pos = read_statement2(pos);
		const auto statement = statement_pos.first;
		statements.push_back(statement);
		pos = skip_whitespace(statement_pos.second);
	}

	return { json_t::make_array(statements), pos };
}

json_t parse_program2(const string& program){
	const auto statements_pos = read_statements2(seq_t(program));
	QUARK_TRACE(json_to_pretty_string(statements_pos.first));
	return statements_pos.first;
}



//////////////////////////////////////////////////		Test programs




const string kProgram2 =
	"int f(int x, int y, string z){\n"
	"	return 3;\n"
	"}\n";

const string kProgram3 =
	"int main(string args){\n"
	"	int a = 4;\n"
	"	return 3;\n"
	"}\n";

const string kProgram4 =
	"string hello(int x, int y, string z){\n"
	"	return \"test abc\";\n"
	"}\n"
	"int main(string args){\n"
	"	return 3;\n"
	"}\n";

const string kProgram5 =
	"float testx(float v){\n"
	"	return 13.4;\n"
	"}\n"
	"int main(string args){\n"
	"	float test = testx(1234);\n"
	"	return 3;\n"
	"}\n";

const auto kProgram6 =
	"struct pixel { string s; }"
	"string main(){\n"
	"	return \"\";"
	"}\n";

const auto kProgram7 =
	"string main(){\n"
	"	return p.s + a;"
	"}\n";






QUARK_UNIT_TEST("", "parse_program1()", "k_test_program_0_source", ""){
	ut_compare_jsons(
		parse_program2(k_test_program_0_source),
		parse_json(seq_t(k_test_program_0_parserout)).first
	);
}

QUARK_UNIT_TEST("", "parse_program1()", "k_test_program_1_source", ""){
	ut_compare_jsons(
		parse_program2(k_test_program_1_source),
		parse_json(seq_t(k_test_program_1_parserout)).first
	);
}

QUARK_UNIT_TEST("", "parse_program2()", "k_test_program_100_source", ""){
	ut_compare_jsons(
		parse_program2(k_test_program_100_source),
		parse_json(seq_t(k_test_program_100_parserout)).first
	);
}



#if false
QUARK_UNIT_TEST("", "parse_program1()", "three arguments", ""){

	const auto result = parse_program1(kProgram2);
	QUARK_UT_VERIFY(result);
#if false
	const auto f = make_function_object(
		type_identifier_t::make("f"),
		type_identifier_t::make_int(),
		{
			member_t{ type_identifier_t::make_int(), "x" },
			member_t{ type_identifier_t::make_int(), "y" },
			member_t{ type_identifier_t::make_string(), "z" }
		},
		executable_t({
			make_shared<statement_t>(make__return_statement(expression_t::make_constant(3)))
		}),
		{},
		{}
	);

//	QUARK_TEST_VERIFY((*resolve_function_type(result._global_scope->_types_collector, "f") == *f));
	QUARK_TEST_VERIFY(resolve_function_type(result._global_scope->_types_collector, "f"));

	const auto f2 = resolve_function_type(result._global_scope->_types_collector, "f");
	QUARK_UT_VERIFY(f2->_type == lexical_scope_t::k_function_scope);

	const auto body = resolve_function_type(f2->_types_collector, "___body");
	QUARK_UT_VERIFY(body->_type == lexical_scope_t::k_subscope);
	QUARK_UT_VERIFY(body->_executable._statements.size() == 1);
#endif
}

QUARK_UNIT_TEST("", "parse_program1()", "Local variables", ""){
	auto result = parse_program1(kProgram3);
	QUARK_UT_VERIFY(result);
#if false
	const auto f2 = resolve_function_type(result._global_scope->_types_collector, "main");
	QUARK_UT_VERIFY(f2);
	QUARK_UT_VERIFY(f2->_type == lexical_scope_t::k_function_scope);

	const auto body = resolve_function_type(f2->_types_collector, "___body");
	QUARK_UT_VERIFY(body);
	QUARK_UT_VERIFY(body->_type == lexical_scope_t::k_subscope);
	QUARK_UT_VERIFY(body->_members.size() == 1);
	QUARK_UT_VERIFY(body->_members[0]._name == "a");
	QUARK_UT_VERIFY(body->_executable._statements.size() == 2);
#endif
}

QUARK_UNIT_TEST("", "parse_program1()", "two functions", ""){
	const auto result = parse_program1(kProgram4);
	QUARK_UT_VERIFY(result);

#if false
	QUARK_TEST_VERIFY(result._global_scope->_executable._statements.size() == 0);

	const auto f = make_function_object(
		type_identifier_t::make("hello"),
		type_identifier_t::make_string(),
		{
			member_t{ type_identifier_t::make_int(), "x" },
			member_t{ type_identifier_t::make_int(), "y" },
			member_t{ type_identifier_t::make_string(), "z" }
		},
		executable_t({
			make_shared<statement_t>(make__return_statement(expression_t::make_constant("test abc")))
		}),
		{}
	);
//	QUARK_TEST_VERIFY((*resolve_function_type(result._global_scope->_types_collector, "hello") == *f));
	QUARK_TEST_VERIFY(resolve_function_type(result._global_scope->_types_collector, "hello"));

	const auto f2 = make_function_object(
		type_identifier_t::make("main"),
		type_identifier_t::make_int(),
		{
			member_t{ type_identifier_t::make_string(), "args" }
		},
		executable_t({
			make_shared<statement_t>(make__return_statement(expression_t::make_constant(3)))
		}),
		{}
	);
//	QUARK_TEST_VERIFY((*resolve_function_type(result._global_scope->_types_collector, "main") == *f2));
	QUARK_TEST_VERIFY(resolve_function_type(result._global_scope->_types_collector, "main"));
#endif
}


QUARK_UNIT_TESTQ("parse_program1()", "Call function a from function b"){
	auto result = parse_program1(kProgram5);
	QUARK_UT_VERIFY(result);
#if false
	QUARK_TEST_VERIFY(result._global_scope->_executable._statements.size() == 0);

	const auto f = make_function_object(
		type_identifier_t::make("testx"),
		type_identifier_t::make_float(),
		{
			member_t{ type_identifier_t::make_float(), "v" }
		},
		executable_t({
			make_shared<statement_t>(make__return_statement(expression_t::make_constant(13.4f)))
		}),
		{}
	);
//	QUARK_TEST_VERIFY((*resolve_function_type(result._global_scope->_types_collector, "testx") == *f));
	QUARK_TEST_VERIFY(resolve_function_type(result._global_scope->_types_collector, "testx"));
#endif
}
#endif


}	//	floyd_parser

