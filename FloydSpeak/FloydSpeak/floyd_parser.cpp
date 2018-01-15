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

namespace floyd {


using namespace std;


/*
	AST ABSTRACT SYNTAX TREE

https://en.wikipedia.org/wiki/Abstract_syntax_tree

https://en.wikipedia.org/wiki/Parsing_expression_grammar
https://en.wikipedia.org/wiki/Parsing
*/

const std::string k_backward_brackets = ")(}{][";



std::string concat_strings(const vector<string>& v){
	if(v.empty()){
		return "";
	}
	else{
		string result;
		for(const auto e: v){
			result += "\t|\t" + e;
		}
		return result;
	}
}

//////////////////////////////////////////////////		parse_statement()

/*
	let TYPE SYMBOL = EXPRESSION;
	FUNC TYPE SYMBOL ( EXPRESSION-LIST ){ STATEMENTS }
	let SYMBOL = EXPRESSION;

	BIND					TYPE				SYMBOL		=		EXPRESSION;
	BIND					int					x			=		10;
	BIND					int	(string a)		x			=		f(4 == 5);

	FUNCTION-DEFINITION		TYPE				SYMBOL				( EXPRESSION-LIST )	{ STATEMENTS }
	FUNCTION-DEFINITION		int					f					(string name)		{ return 13; }
	FUNCTION-DEFINITION		int (string a)		f					(string name)		{ return 100 == 101; }

	EXPRESSION-STATEMENT						EXPRESSION;
	EXPRESSION-STATEMENT						print				("Hello, World!");
	EXPRESSION-STATEMENT						print				("Hello, World!" + f(3) == 2);

	DEDUCED-BIND								SYMBOL		=		EXPRESSION;
	DEDUCED-BIND								x			=		10;
	DEDUCED-BIND								x			=		"hello";
	DEDUCED-BIND								x			=		f(3) == 2;

	MUTATE-LOCAL								SYMBOL		<===	EXPRESSION;
	MUTATE-LOCAL								x			<===	11;

//	read entire statement
find thislevel "}" or thislevel ";"

find thislevel "="
	BIND or DEDUCED_BIND
else
	FUNCTION-DEFINITION or EXPRESSION-STATEMENT
*/


// Includes trailing ";". Does not include body of a function definition.
std::pair<string, seq_t> read_until_semicolor_or_seagull(const seq_t& pos0){
	const auto pos1 = skip_whitespace(pos0);
	auto pos = pos1;
	while(pos.first() != ";" && pos.first() != "{" && pos.empty() == false){
		if(is_start_char(pos.first()[0])){
			const auto end = get_balanced(pos).second;
			pos = end;
		}
		else{
			pos = pos.rest1();
		}
	}
	if(pos.first1() == ";"){
		pos = pos.rest1();
	}
	const auto r = get_range(pos1, pos);
	return { r, pos };
}

//	If none are found, returns { "", s }
std::pair<string, seq_t> read_until_toplevel_char(const seq_t& s, const char ch){
	auto pos = s;
	while(pos.empty() == false && pos.first1_char() != ch){
		if(is_start_char(pos.first()[0])){
			const auto end = get_balanced(pos).second;
			pos = end;
		}
		else{
			pos = pos.rest1();
		}
	}
	if (pos.empty()){
		return { "", s };
	}
	else{
		const auto r = get_range(s, pos);
		return { r, pos };
	}
}


pair<string, string> split_at_tail_symbol(const std::string& s){
	auto i = s.size();
	while(i > 0 && whitespace_chars.find(s[i - 1]) != string::npos){
		i--;
	}
	while(i > 0 && identifier_chars.find(s[i - 1]) != string::npos){
		i--;
	}
	const auto pre_symbol = skip_whitespace(s.substr(0, i));
	const auto symbol = s.substr(i);
	return { pre_symbol, symbol };
}



pair<vector<string>, seq_t> parse_implicit_statement(const seq_t& s1){
	const auto r = seq_t(read_until_semicolor_or_seagull(s1).first);
	const auto equal_sign_pos = read_until_toplevel_char(r, '=');
	if(equal_sign_pos.first.empty()){
		//	FUNCTION-DEFINITION:	int f(string name)
		//	FUNCTION-DEFINITION:	int (string a) f(string name)

		//	EXPRESSION-STATEMENT:	print ("Hello, World!");
		//	EXPRESSION-STATEMENT:	print("Hello, World!" + f(3) == 2);
		//	EXPRESSION-STATEMENT:	print(3);
		const auto s2 = equal_sign_pos.second.str();

		const seq_t rev(reverse(s2));
		auto rev2 = skip_whitespace(rev);
		if(rev2.first1() == ";"){
			rev2 = rev2.rest1();
		}
		rev2 = skip_whitespace(rev2);
		if(rev2.first1() != ")"){
			throw std::runtime_error("syntax error");
		}
		const auto parantheses_rev = read_balanced2(rev2, k_backward_brackets);
		const auto parantheses_char_count = parantheses_rev.first.size();

		const auto split_pos = rev2.size() - parantheses_char_count;
		const auto pre_parantheses = s2.substr(0, split_pos);
		const auto parantheses = s2.substr(split_pos);

		const auto pre_symbol__symbol = split_at_tail_symbol(pre_parantheses);
		const auto pre_symbol = skip_whitespace_ends(pre_symbol__symbol.first);
		const auto symbol = skip_whitespace_ends(pre_symbol__symbol.second);

		if(pre_symbol == ""){
			auto s3 = skip_whitespace_ends(s2);
			if(s3.back() != ';'){
				throw std::runtime_error("syntax error");
			}
			s3.pop_back();
			s3 = skip_whitespace_ends(s3);
			return { { "[EXPRESSION-STATEMENT]", s3 }, s1 };
		}
		else{
			return { { "[FUNCTION-DEFINITION]", skip_whitespace_ends(s2) }, s1 };
		}
	}
	else{
		//	BIND:			int x = 10;
		//	BIND:			int (string a) x = f(4 == 5);
		//	BIND:			mutable int x = 10;
		//	DEDUCED-BIND	x = 10;
		//	DEDUCED-BIND	x = "hello";
		//	DEDUCED-BIND	x = f(3) == 2;
		//	DEDUCED-BIND	mutable x = 10;
		//	MUTATE_LOCAL	x <=== 11;
		auto rhs_expression1 = skip_whitespace(equal_sign_pos.second.rest1().str());
		if(rhs_expression1.back() != ';'){
			throw std::runtime_error("syntax error");
		}

		if(rhs_expression1.back() == ';'){
			rhs_expression1.pop_back();
		}
		const auto rhs_expression = skip_whitespace_ends(rhs_expression1);

		const auto pre_symbol__symbol = split_at_tail_symbol(equal_sign_pos.first);
		const auto pre_symbol = skip_whitespace_ends(pre_symbol__symbol.first);
		const auto symbol = skip_whitespace_ends(pre_symbol__symbol.second);

		if(pre_symbol == ""){
			return { { "[DEDUCED-BIND]", symbol, rhs_expression }, s1 };
		}
		else{
			return { { "[BIND]", pre_symbol, symbol, rhs_expression }, s1 };
		}
	}
}

std::string test_split_line(const string& title, const seq_t& in){
	vector<string> temp;
	temp.push_back(title);
	temp.push_back(in.str());
	temp = temp + in.first(30);

	const auto split = parse_implicit_statement(in);

	string analysis;
	if(split.first[0] == "[EXPRESSION-STATEMENT]"){
		const auto expression = split.first[1];
		analysis = string() + "[EXPRESSION-STATEMENT] EXPRESSION: " + "\"" + expression + "\"";
	}
	else if(split.first[0] == "[FUNCTION-DEFINITION]"){
		const auto function_def = split.first[1];
		analysis = string() + "[FUNCTION-DEFINITION] DEF: " + "\"" + function_def + "\"";
	}
	else if(split.first[0] == "[DEDUCED-BIND]"){
		const auto symbol = split.first[1];
		const auto rhs_expression = split.first[2];
		analysis = string() + "[DEDUCED-BIND] SYMBOL: " + "\"" + symbol + "\"" + " = EXPRESSION: " + "\"" + rhs_expression + "\"";
	}
	else if(split.first[0] == "[BIND]"){
		const auto pre_symbol = split.first[1];
		const auto symbol = split.first[2];
		const auto rhs_expression = split.first[3];
		analysis = string() + "[BIND] TYPE: " + "\"" + pre_symbol + "\"" + " SYMBOL: " + "\"" + symbol + "\"" + " = EXPRESSION: " + "\"" + rhs_expression + "\"";
	}
	temp = temp + analysis;

	const auto out = concat_strings(temp);
	return out;
}

QUARK_UNIT_TEST("", "parse_implicit_statement()", "", ""){
	//	BIND	TYPE	SYMBOL	=	EXPRESSION;
	QUARK_TRACE((test_split_line("BIND", seq_t("int x = 10;xyz"))));
	QUARK_TRACE((test_split_line("BIND", seq_t("int (string a) x = f(4 == 5);xyz"))));
	QUARK_TRACE((test_split_line("BIND", seq_t("mutable int x = 10;xyz"))));

	//	FUNCTION-DEFINITION	TYPE	SYMBOL	( EXPRESSION-LIST )	{ STATEMENTS }
	QUARK_TRACE((test_split_line("FUNCTION-DEFINITION", seq_t("int f(){ return 0; }xyz"))));
	QUARK_TRACE((test_split_line("FUNCTION-DEFINITION", seq_t("int f(string name){ return 13; }xyz"))));
	QUARK_TRACE((test_split_line("FUNCTION-DEFINITION", seq_t("int (string a) f(string name){ return 100 == 101; }xyz"))));

	//	EXPRESSION-STATEMENT	EXPRESSION;
	QUARK_TRACE((test_split_line("EXPRESSION-STATEMENT", seq_t("print (\"Hello, World!\");xyz"))));
	QUARK_TRACE((test_split_line("EXPRESSION-STATEMENT", seq_t("print(\"Hello, World!\" + f(3) == 2);xyz"))));
	QUARK_TRACE((test_split_line("EXPRESSION-STATEMENT", seq_t("print(3);xyz"))));

	//	DEDUCED-BIND	SYMBOL	=	EXPRESSION;
	QUARK_TRACE((test_split_line("DEDUCED-BIND", seq_t("x = 10;xyz"))));
	QUARK_TRACE((test_split_line("DEDUCED-BIND", seq_t("x = \"hello\";xyz"))));
	QUARK_TRACE((test_split_line("DEDUCED-BIND", seq_t("x = f(3) == 2;xyz"))));
	QUARK_TRACE((test_split_line("DEDUCED-BIND", seq_t("mutable x = 10;xyz"))));

	//	MUTATE-LOCAL	SYMBOL	<===	EXPRESSION;
	//	QUARK_TRACE((test_split_line("MUTATE-LOCAL", seq_t("x <=== 11;xyz"))));
}

QUARK_UNIT_TEST("", "parse_implicit_statement()", "", ""){
	QUARK_UT_VERIFY((	parse_implicit_statement(seq_t(" int x = 10 ; xyz")) == pair<vector<string>, seq_t>{vector<string>{ "[BIND]", "int", "x", "10" }, seq_t(" int x = 10 ; xyz") }	));
}
QUARK_UNIT_TEST("", "parse_implicit_statement()", "", ""){
	QUARK_UT_VERIFY((	parse_implicit_statement(seq_t(" int ( string a ) x = f ( 4 == 5 ) ; xyz")) == pair<vector<string>, seq_t>{vector<string>{ "[BIND]", "int ( string a )", "x", "f ( 4 == 5 )" }, seq_t(" int ( string a ) x = f ( 4 == 5 ) ; xyz") }	));
}
QUARK_UNIT_TEST("", "parse_implicit_statement()", "", ""){
	QUARK_UT_VERIFY((	parse_implicit_statement(seq_t(" mutable int x = 10 ; xyz")) == pair<vector<string>, seq_t>{vector<string>{ "[BIND]", "mutable int", "x", "10" }, seq_t(" mutable int x = 10 ; xyz") }	));
}

QUARK_UNIT_TEST("", "parse_implicit_statement()", "", ""){
	QUARK_UT_VERIFY((	parse_implicit_statement(seq_t(" int f ( ) { return 0 ; } xyz")) == pair<vector<string>, seq_t>{vector<string>{ "[FUNCTION-DEFINITION]", "int f ( )" }, seq_t(" int f ( ) { return 0 ; } xyz") }	));
}
QUARK_UNIT_TEST("", "parse_implicit_statement()", "", ""){
	QUARK_UT_VERIFY((	parse_implicit_statement(seq_t(" int f ( string name ) { return 13 ; }xyz")) == pair<vector<string>, seq_t>{vector<string>{ "[FUNCTION-DEFINITION]", "int f ( string name )" }, seq_t(" int f ( string name ) { return 13 ; }xyz") }	));
}
QUARK_UNIT_TEST("", "parse_implicit_statement()", "", ""){
	QUARK_UT_VERIFY((	parse_implicit_statement(seq_t(" int ( string a ) f ( string name ) { return 100 == 101 ; }xyz")) == pair<vector<string>, seq_t>{vector<string>{ "[FUNCTION-DEFINITION]", "int ( string a ) f ( string name )" }, seq_t(" int ( string a ) f ( string name ) { return 100 == 101 ; }xyz") }	));
}

QUARK_UNIT_TEST("", "parse_implicit_statement()", "", ""){
	QUARK_UT_VERIFY((	parse_implicit_statement(seq_t(" print ( \"Hello, World!\" ) ;xyz")) == pair<vector<string>, seq_t>{vector<string>{ "[EXPRESSION-STATEMENT]", "print ( \"Hello, World!\" )" }, seq_t(" print ( \"Hello, World!\" ) ;xyz") }	));
}
QUARK_UNIT_TEST("", "parse_implicit_statement()", "", ""){
	QUARK_UT_VERIFY((	parse_implicit_statement(seq_t(" print ( \"Hello, World!\" ) ;xyz")) == pair<vector<string>, seq_t>{vector<string>{ "[EXPRESSION-STATEMENT]", "print ( \"Hello, World!\" )" }, seq_t(" print ( \"Hello, World!\" ) ;xyz") }	));
}
QUARK_UNIT_TEST("", "parse_implicit_statement()", "", ""){
	QUARK_UT_VERIFY((	parse_implicit_statement(seq_t("print(3);xyz")) == pair<vector<string>, seq_t>{vector<string>{ "[EXPRESSION-STATEMENT]", "print(3)" }, seq_t("print(3);xyz") }	));
}

QUARK_UNIT_TEST("", "parse_implicit_statement()", "", ""){
	QUARK_UT_VERIFY((	parse_implicit_statement(seq_t(" x = 10 ;xyz")) == pair<vector<string>, seq_t>{vector<string>{ "[DEDUCED-BIND]", "x", "10" }, seq_t(" x = 10 ;xyz") }	));
}
QUARK_UNIT_TEST("", "parse_implicit_statement()", "", ""){
	QUARK_UT_VERIFY((	parse_implicit_statement(seq_t(" x = \"hello\" ;xyz")) == pair<vector<string>, seq_t>{vector<string>{ "[DEDUCED-BIND]", "x", "\"hello\"" }, seq_t(" x = \"hello\" ;xyz") }	));
}
QUARK_UNIT_TEST("", "parse_implicit_statement()", "", ""){
	QUARK_UT_VERIFY((	parse_implicit_statement(seq_t(" x = f ( 3 ) == 2 ;xyz")) == pair<vector<string>, seq_t>{vector<string>{ "[DEDUCED-BIND]", "x", "f ( 3 ) == 2" }, seq_t(" x = f ( 3 ) == 2 ;xyz") }	));
}

/*
QUARK_UNIT_TEST("", "parse_implicit_statement()", "", ""){
	QUARK_UT_VERIFY((	parse_implicit_statement(seq_t(" mutable x = 10 ;xyz")) == pair<vector<string>, seq_t>{vector<string>{ "[DEDUCED-BIND]", "x", "10" }, seq_t(" mutable x = 10 ;xyz") }	));
}
*/


std::pair<json_t, seq_t> parse_prefixless_statement(const seq_t& s){
	const auto pos = skip_whitespace(s);
	const auto implicit = parse_implicit_statement(pos);
	const auto statement_type = implicit.first[0];
	if(statement_type == "[BIND]"){
		return parse_assignment_statement(pos);
	}
	else if(statement_type == "[FUNCTION-DEFINITION]"){
		return parse_function_definition2(pos);
	}
	else if(statement_type == "[EXPRESSION-STATEMENT]"){
		return parse_expression_statement(pos);
	}
	else if(statement_type == "[DEDUCED-BIND]"){
		return parse_deduced_bind_statement(pos);
	}
	else{
		QUARK_ASSERT(false);
	}
}

QUARK_UNIT_TEST("", "parse_prefixless_statement()", "", ""){
	ut_compare_jsons(
		parse_prefixless_statement(seq_t("int x = f(3);")).first,
		parse_json(seq_t(R"(["bind", "int", "x", ["call", ["@", "f"], [["k", 3, "int"]]]])")).first
	);
}

#if false
QUARK_UNIT_TEST("", "parse_statement()", "", ""){
	ut_compare_jsons(
		parse_prefixless_statement(seq_t("f(3);")).first,
		parse_json(seq_t(R"(["call", ["@", "f"], [["k", 3, "int"]]])")).first
	);
}
#endif


std::pair<json_t, seq_t> parse_statement(const seq_t& pos0){
	const auto pos = skip_whitespace(pos0);
	if(is_first(pos, "{")){
		return parse_block(pos);
	}
	else if(is_first(pos, "return")){
		return parse_return_statement(pos);
	}
	else if(is_first(pos, "struct")){
		return parse_struct_definition(seq_t(pos));
	}
	else if(is_first(pos, "if")){
		const auto temp = parse_if_statement(seq_t(pos));
		QUARK_TRACE(json_to_pretty_string(temp.first));
		return temp;
	}
	else if(is_first(pos, "for")){
		return parse_for_statement(seq_t(pos));
	}
	else {
		return parse_prefixless_statement(pos);
	}
}

QUARK_UNIT_TEST("", "parse_statement()", "", ""){
	ut_compare_jsons(
		parse_statement(seq_t("int x = 10;")).first,
		parse_json(seq_t(R"(["bind", "int", "x", ["k", 10, "int"] ])")).first
	);
}
QUARK_UNIT_TEST("", "parse_statement()", "", ""){
	ut_compare_jsons(
		parse_statement(seq_t("int f(string name){ return 13; }")).first,
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
		parse_statement(seq_t("int x = f(3);")).first,
		parse_json(seq_t(R"(["bind", "int", "x", ["call", ["@", "f"], [["k", 3, "int"]]]])")).first
	);
}



std::pair<json_t, seq_t> parse_statements(const seq_t& s0){
	vector<json_t> statements;
	auto pos = skip_whitespace(s0);
	while(!pos.empty()){
		const auto statement_pos = parse_statement(pos);
		const auto statement = statement_pos.first;
		statements.push_back(statement);
		pos = skip_whitespace(statement_pos.second);
	}
	return { json_t::make_array(statements), pos };
}

json_t parse_program2(const string& program){
	const auto statements_pos = parse_statements(seq_t(program));
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


}	//	namespace floyd

