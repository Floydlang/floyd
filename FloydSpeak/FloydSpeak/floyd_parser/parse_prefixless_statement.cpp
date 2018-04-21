//
//  parse_implicit_statement.cpp
//  FloydSpeak
//
//  Created by Marcus Zetterquist on 2018-01-15.
//  Copyright © 2018 Marcus Zetterquist. All rights reserved.
//

#include "parse_prefixless_statement.h"


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
#include "ast_basics.h"


namespace quark {
	template<> std::string to_debug_str<std::pair<std::vector<std::string>, seq_t>>(const std::pair<std::vector<std::string>, seq_t>& s){
		return json_to_pretty_string(
			json_t::make_array({
				from_string_vec(s.first),
				s.second.str().substr(0, 24)
			})
		);
	}
}


namespace floyd {


using namespace std;


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

//////////////////////////////////////////////////		detect_implicit_statement_lookahead()


pair<string, string> split_at_tail_identifier(const std::string& s){
	auto i = s.size();
	while(i > 0 && whitespace_chars.find(s[i - 1]) != string::npos){
		i--;
	}
	while(i > 0 && identifier_chars.find(s[i - 1]) != string::npos){
		i--;
	}
	const auto pre_identifier = skip_whitespace(s.substr(0, i));
	const auto identifier = s.substr(i);
	return { pre_identifier, identifier };
}


enum class implicit_statement {
	k_function_definition = 10,
	k_expression_statement,
	k_bind,
	k_assign
};


/*
	FUNCTION-DEFINITION			TYPE IDENTIFIER "(" ARGUMENTS ")" "{" ... "}"
		int print(float a) { ... }
		int print (float a) { ... }
		int f(string name)
		int (string a) f(string name)

	EXPRESSION-STATEMENT		EXPRESSION;
		print(3);
		print (3);
		3 + 4;
		hello + 3;
		print ("Hello, World!");
		print("Hello, World!" + f(3) == 2);

	ASSIGN						IDENTIFIER = EXPRESSION;
		x = 10;
		x = "hello";
		x = f(3) == 2;
		mutable x = 10;

	BIND						TYPE IDENTIFIER = EXPRESSION;
		int x = {"a": 1, "b": 2};
		int x = 10;
		int (string a) x = f(4 == 5);
		mutable int x = 10;
*/

//	NOTICE: This function is very complex -- let's keep it FOCUSED just on figuring out the type of statement.
implicit_statement detect_implicit_statement_lookahead(const seq_t& s){
	auto pos = read_until_toplevel_match(skip_whitespace(s), ";{");
	if(pos.second.first1() == ";"){
		pos.first.push_back(';');
		pos.second = pos.second.rest1();
	}
	const auto equal_sign_pos = read_until_toplevel_match(seq_t(pos.first), "=");
	if(equal_sign_pos.second.empty()){
		const auto posx = read_until_toplevel_match(skip_whitespace(s), ";{");
		if(posx.second.first() == "{"){
			return implicit_statement::k_function_definition;
		}
		else{
			return implicit_statement::k_expression_statement;
		}
	}
	else{
		const auto statement_pos = read_until_toplevel_match(skip_whitespace(s), ";");
		const auto equal_sign_pos2 = read_until_toplevel_match(seq_t(statement_pos.first), "=");
		const auto rhs_expression1 = skip_whitespace(equal_sign_pos2.second.rest1()).str();
		const auto pre_identifier__identifier = split_at_tail_identifier(equal_sign_pos2.first);
		const auto pre_identifier = skip_whitespace_ends(pre_identifier__identifier.first);

		if(pre_identifier == ""){
			return implicit_statement::k_assign;
		}
		else{
			return implicit_statement::k_bind;
		}
	}
}

QUARK_UNIT_TEST("", "detect_implicit_statement_lookahead()", "", "BIND"){
	QUARK_UT_VERIFY(detect_implicit_statement_lookahead(seq_t(" int x = 10 ; xyz")) == implicit_statement::k_bind);
}
QUARK_UNIT_TEST("", "detect_implicit_statement_lookahead()", "function defs", "BIND"){
	QUARK_UT_VERIFY(detect_implicit_statement_lookahead(seq_t(" int ( string a ) x = f ( 4 == 5 ) ; xyz")) == implicit_statement::k_bind);
}
QUARK_UNIT_TEST("", "detect_implicit_statement_lookahead()", "", "BIND"){
	QUARK_UT_VERIFY(detect_implicit_statement_lookahead(seq_t(" mutable int x = 10 ; xyz")) == implicit_statement::k_bind);
}
QUARK_UNIT_TEST("", "detect_implicit_statement_lookahead()", "mutable", "BIND"){
	QUARK_UT_VERIFY(detect_implicit_statement_lookahead(seq_t(" mutable x = 10 ;xyz")) == implicit_statement::k_bind);
}
QUARK_UNIT_TEST("", "detect_implicit_statement_lookahead()", "vector", "BIND"){
	QUARK_UT_VERIFY(detect_implicit_statement_lookahead(seq_t("[int] a = [1,2,3];xyz")) == implicit_statement::k_bind);
}
QUARK_UNIT_TEST("", "detect_implicit_statement_lookahead()", "dict", "BIND"){
	QUARK_UT_VERIFY(detect_implicit_statement_lookahead(seq_t(R"([string:int] a = {"uno": 1, "duo": 2};xyz)")) == implicit_statement::k_bind);
}


QUARK_UNIT_TEST("", "detect_implicit_statement_lookahead()", "", "FUNCTION-DEFINITION"){
	QUARK_UT_VERIFY(detect_implicit_statement_lookahead(seq_t(" int f ( ) { return 0 ; } xyz")) == implicit_statement::k_function_definition);
	QUARK_UT_VERIFY(detect_implicit_statement_lookahead(seq_t(" int f ( string name ) { return 13 ; }xyz")) == implicit_statement::k_function_definition);
	QUARK_UT_VERIFY(detect_implicit_statement_lookahead(seq_t(" int ( string a ) f ( string name ) { return 100 == 101 ; }xyz")) == implicit_statement::k_function_definition);
}
QUARK_UNIT_TEST("", "detect_implicit_statement_lookahead()", "", "FUNCTION-DEFINITION"){
	QUARK_UT_VERIFY(detect_implicit_statement_lookahead(seq_t(" int f ( ) { return 0 ; } xyz")) == implicit_statement::k_function_definition);
}
QUARK_UNIT_TEST("", "detect_implicit_statement_lookahead()", "", "FUNCTION-DEFINITION"){
	QUARK_UT_VERIFY(detect_implicit_statement_lookahead(seq_t(" int f ( string name ) { return 13 ; }xyz")) == implicit_statement::k_function_definition);
}
QUARK_UNIT_TEST("", "detect_implicit_statement_lookahead()", "", "FUNCTION-DEFINITION"){
	QUARK_UT_VERIFY(detect_implicit_statement_lookahead(seq_t(" int ( string a ) f ( string name ) { return 100 == 101 ; }xyz")) == implicit_statement::k_function_definition);
}


QUARK_UNIT_TEST("", "detect_implicit_statement_lookahead()", "", "EXPRESSION-STATEMENT"){
	QUARK_UT_VERIFY(detect_implicit_statement_lookahead(seq_t("print(3);")) != implicit_statement::k_function_definition);
	QUARK_UT_VERIFY(detect_implicit_statement_lookahead(seq_t("print ( \"Hello, World!\" ) ;xyz")) != implicit_statement::k_function_definition);
	QUARK_UT_VERIFY(detect_implicit_statement_lookahead(seq_t("print(3);xyz")) != implicit_statement::k_function_definition);
	QUARK_UT_VERIFY(detect_implicit_statement_lookahead(seq_t("3;xyz")) != implicit_statement::k_function_definition);
	QUARK_UT_VERIFY(detect_implicit_statement_lookahead(seq_t("3 + 4;xyz")) != implicit_statement::k_function_definition);
	QUARK_UT_VERIFY(detect_implicit_statement_lookahead(seq_t("3 + f(1) + f(2);xyz")) != implicit_statement::k_function_definition);
	QUARK_UT_VERIFY(detect_implicit_statement_lookahead(seq_t("3")) != implicit_statement::k_function_definition);
}
QUARK_UNIT_TEST("", "detect_implicit_statement_lookahead()", "", "EXPRESSION-STATEMENT"){
	QUARK_UT_VERIFY(detect_implicit_statement_lookahead(seq_t(" print ( \"Hello, World!\" ) ;xyz")) == implicit_statement::k_expression_statement);
}
QUARK_UNIT_TEST("", "detect_implicit_statement_lookahead()", "", "EXPRESSION-STATEMENT"){
	QUARK_UT_VERIFY(detect_implicit_statement_lookahead(seq_t(" print ( \"Hello, World!\" ) ;xyz")) == implicit_statement::k_expression_statement);
}
QUARK_UNIT_TEST("", "detect_implicit_statement_lookahead()", "", "EXPRESSION-STATEMENT"){
	QUARK_UT_VERIFY(detect_implicit_statement_lookahead(seq_t("print(3);xyz")) == implicit_statement::k_expression_statement);
}
QUARK_UNIT_TEST("", "detect_implicit_statement_lookahead()", "", "EXPRESSION-STATEMENT"){
	QUARK_UT_VERIFY(detect_implicit_statement_lookahead(seq_t("3;xyz")) == implicit_statement::k_expression_statement);
}
QUARK_UNIT_TEST("", "detect_implicit_statement_lookahead()", "", "EXPRESSION-STATEMENT"){
	QUARK_UT_VERIFY(detect_implicit_statement_lookahead(seq_t("3 + 4;xyz")) == implicit_statement::k_expression_statement);
}
QUARK_UNIT_TEST("", "detect_implicit_statement_lookahead()", "", "EXPRESSION-STATEMENT"){
	QUARK_UT_VERIFY(detect_implicit_statement_lookahead(seq_t("3 + f(1) + f(2);xyz")) == implicit_statement::k_expression_statement);
}


QUARK_UNIT_TEST("", "detect_implicit_statement_lookahead()", "", "store"){
	QUARK_UT_VERIFY(detect_implicit_statement_lookahead(seq_t(" x = 10 ;xyz")) == implicit_statement::k_assign);
}
QUARK_UNIT_TEST("", "detect_implicit_statement_lookahead()", "", "store"){
	QUARK_UT_VERIFY(detect_implicit_statement_lookahead(seq_t(" x = \"hello\" ;xyz")) == implicit_statement::k_assign);
}
QUARK_UNIT_TEST("", "detect_implicit_statement_lookahead()", "", "store"){
	QUARK_UT_VERIFY(detect_implicit_statement_lookahead(seq_t(" x = f ( 3 ) == 2 ;xyz")) == implicit_statement::k_assign);
}
QUARK_UNIT_TEST("", "detect_implicit_statement_lookahead()", "vector", "store"){
	QUARK_UT_VERIFY(detect_implicit_statement_lookahead(seq_t("a = [1,2,3];xyz")) == implicit_statement::k_assign);
}
QUARK_UNIT_TEST("", "detect_implicit_statement_lookahead()", "dict", "store"){
	QUARK_UT_VERIFY(detect_implicit_statement_lookahead(seq_t(R"(a = {"uno": 1, "duo": 2};xyz)")) == implicit_statement::k_assign);
}


//////////////////////////////////////////////////		parse_bind_statement()


//	BIND:			int x = 10;
//	BIND:			int (string a) x = f(4 == 5);
//	BIND:			mutable int x = 10;
pair<ast_json_t, seq_t> parse_bind_statement(const seq_t& s){
	auto pos = read_until_toplevel_match(skip_whitespace(s), ";");
	if(pos.second.first1() == ";"){
		pos.first.push_back(';');
		pos.second = pos.second.rest1();
	}
	const auto r = seq_t(pos.first);

	const auto equal_sign_pos = read_until_toplevel_match(r, "=");


	auto rhs_expression1 = skip_whitespace(equal_sign_pos.second.rest1().str());
	if(rhs_expression1.back() != ';'){
		throw std::runtime_error("syntax error");
	}

	if(rhs_expression1.back() == ';'){
		rhs_expression1.pop_back();
	}
	const auto rhs_expression = skip_whitespace_ends(rhs_expression1);

	const auto pre_identifier__identifier = split_at_tail_identifier(equal_sign_pos.first);
	const auto pre_identifier = skip_whitespace_ends(pre_identifier__identifier.first);
	const auto identifier0 = skip_whitespace_ends(pre_identifier__identifier.second);

	vector<string> parsed_bits = { "[BIND]", pre_identifier, identifier0, rhs_expression };


	QUARK_ASSERT(parsed_bits[1].empty() == false);
	QUARK_ASSERT(parsed_bits[2].empty() == false);
	QUARK_ASSERT(parsed_bits[3].empty() == false);

	const auto type_seq = seq_t(parsed_bits[1]);
	const auto identifier = parsed_bits[2];
	const auto expression_str = parsed_bits[3];

	const auto mutable_pos = if_first(skip_whitespace(type_seq), keyword_t::k_mutable);
	const bool mutable_flag = mutable_pos.first;
	const auto type_pos = skip_whitespace(mutable_pos.second);

	const auto type = type_pos.empty() ? typeid_t::make_undefined() : read_required_type(type_pos).first;
	const auto expression = parse_expression_all(seq_t(expression_str));

	const auto meta = mutable_flag ? (json_t::make_object({pair<string,json_t>{"mutable", true}})) : json_t();
	const auto statement = make_array_skip_nulls({ "bind", typeid_to_ast_json(type, json_tags::k_tag_resolve_state)._value, identifier, expression._value, meta });

	const auto x = read_until(s, ";");
	return { ast_json_t{statement}, x.second.rest1() };
}

QUARK_UNIT_TESTQ("parse_bind_statement", ""){
	ut_compare_jsons(
		parse_bind_statement(seq_t("bool bb = true;")).first._value,
		parse_json(seq_t(
			R"(
				[ "bind", "^bool", "bb", ["k", true, "^bool"]]
			)"
		)).first
	);
}
QUARK_UNIT_TESTQ("parse_bind_statement", ""){
	ut_compare_jsons(
		parse_bind_statement(seq_t("int hello = 3;")).first._value,
		parse_json(seq_t(
			R"(
				[ "bind", "^int", "hello", ["k", 3, "^int"]]
			)"
		)).first
	);
}

QUARK_UNIT_TESTQ("parse_bind_statement", ""){
	ut_compare_jsons(
		parse_bind_statement(seq_t("mutable int a = 14;")).first._value,
		parse_json(seq_t(
			R"(
				[ "bind", "^int", "a", ["k", 14, "^int"], { "mutable": true }]
			)"
		)).first
	);
}

QUARK_UNIT_TESTQ("parse_bind_statement", ""){
	ut_compare_jsons(
		parse_bind_statement(seq_t("mutable hello = 3;")).first._value,
		parse_json(seq_t(
			R"(
				[ "bind", "^**undef**", "hello", ["k", 3, "^int"], { "mutable": true }]
			)"
		)).first
	);
}

//### test float literal
//### test string literal


//////////////////////////////////////////////////		parse_assign_statement()


pair<ast_json_t, seq_t> parse_assign_statement(const seq_t& s){
	const auto variable_pos = read_identifier(s);
	const auto equal_pos = read_required_char(skip_whitespace(variable_pos.second), '=');
	const auto expression_pos = read_until_toplevel_match(skip_whitespace(equal_pos), ";");
	const auto expression = parse_expression_all(seq_t(expression_pos.first));

	const auto statement = json_t::make_array({ "store", variable_pos.first, expression._value });
	return { ast_json_t{statement}, expression_pos.second.rest1() };
}

QUARK_UNIT_TEST("", "parse_assign_statement()", "", ""){
	ut_compare_jsons(
		parse_assign_statement(seq_t("x = 10;")).first._value,
		parse_json(seq_t(
			R"(
				["store","x",["k",10,"^int"]]
			)"
		)).first
	);
}


//////////////////////////////////////////////////		parse_expression_statement()


pair<ast_json_t, seq_t> parse_expression_statement(const seq_t& s){
	const auto expression_pos = read_until(skip_whitespace(s), ";");
	const auto expression = parse_expression_all(seq_t(expression_pos.first));

	const auto statement = json_t::make_array({ "expression-statement", expression._value });
	return { ast_json_t{statement}, expression_pos.second.rest1() };
}

QUARK_UNIT_TEST("", "parse_expression_statement()", "", ""){
	ut_compare_jsons(
		parse_expression_statement(seq_t("print(14);")).first._value,
		parse_json(seq_t(
			R"(
				[ "expression-statement", [ "call", ["@", "print"], [["k", 14, "^int"]] ] ]
			)"
		)).first
	);
}


std::pair<ast_json_t, seq_t> parse_prefixless_statement(const seq_t& s){
	const auto pos = skip_whitespace(s);
	const auto implicit_type = detect_implicit_statement_lookahead(pos);
	if(implicit_type == implicit_statement::k_bind){
		return parse_bind_statement(pos);
	}
	else if(implicit_type == implicit_statement::k_function_definition){
		return parse_function_definition2(pos);
	}
	else if(implicit_type == implicit_statement::k_expression_statement){
		return parse_expression_statement(pos);
	}
	else if(implicit_type == implicit_statement::k_assign){
		return parse_assign_statement(pos);
	}
	else{
		QUARK_ASSERT(false);
		throw std::exception();
	}
}

QUARK_UNIT_TEST("", "parse_prefixless_statement()", "", ""){
	ut_compare_jsons(
		parse_prefixless_statement(seq_t("int x = f(3);")).first._value,
		parse_json(seq_t(R"(["bind", "^int", "x", ["call", ["@", "f"], [["k", 3, "^int"]]]])")).first
	);
}

}	//	floyd
