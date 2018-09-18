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

#include "ast_typeid.h"

#include <string>
#include <memory>
#include <map>
#include <iostream>
#include <cmath>


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
		for(const auto& e: v){
			result += "\t|\t" + e;
		}
		return result;
	}
}

//////////////////////////////////////////////////		detect_implicit_statement_lookahead()

//	Finds identifier-string at RIGHT side of string. Stops at whitespace.
//	"1234()=<whatever> IDENTIFIER": "1234()=<whatever>", "IDENTIFIER"
//	Removed leading/trailing whitespace of both returned strings.
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
	return { skip_whitespace_ends(pre_identifier), skip_whitespace_ends(identifier) };
}


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

implicit_statement detect_implicit_statement_lookahead(const seq_t& s){
	if(is_identifier_and_equal(s)){
		return implicit_statement::k_assign;
	}
	else{
		//	Detect "int test = 123" which is common illegal syntax, where you forgot "mutable" or "let".

		try {
			const auto maybe_type = read_type_verify(s);
			if(maybe_type.first){
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

//#define DETECT_TEST QUARK_UNIT_TEST_VIP
#define DETECT_TEST QUARK_UNIT_TEST

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
}
DETECT_TEST("", "detect_implicit_statement_lookahead()", "", "EXPRESSION-STATEMENT"){
	QUARK_UT_VERIFY(detect_implicit_statement_lookahead(seq_t(" print ( \"Hello, World!\" )		xyz")) == implicit_statement::k_expression_statement);
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


//////////////////////////////////////////////////		parse_bind_statement()


//	BIND:			let int x = 10
//	BIND:			let x = 10
//	BIND:			let int (string a) x = f(4 == 5)
//	BIND:			mutable int x = 10
//	BIND:			mutable x = 10
pair<ast_json_t, seq_t> parse_bind_statement(const seq_t& s){
	const auto let_pos = if_first(skip_whitespace(s), keyword_t::k_let);
	const auto mutable_pos = if_first(skip_whitespace(s), keyword_t::k_mutable);
	if(let_pos.first == false && mutable_pos.first == false){
		throw std::runtime_error("Bind syntax error");
	}
	const auto pos = let_pos.first ? let_pos.second : mutable_pos.second;
	const auto equal_sign_fr = read_until_toplevel_match(pos, "=");
	const auto rhs = skip_whitespace(equal_sign_fr.second.rest1());
	const auto lhs_pair = split_at_tail_identifier(equal_sign_fr.first);
	const auto identifier = lhs_pair.second;

	const bool mutable_flag = mutable_pos.first;
	const auto type_pos = seq_t(lhs_pair.first);;

	const auto type = type_pos.empty() ? typeid_t::make_undefined() : read_required_type(type_pos).first;

	const auto expression_fr = parse_expression_seq(rhs);

	const auto meta = mutable_flag ? (json_t::make_object({pair<string,json_t>{"mutable", true}})) : json_t();
	const auto statement = make_array_skip_nulls({ "bind", typeid_to_ast_json(type, json_tags::k_tag_resolve_state)._value, identifier, expression_fr.first._value, meta });

	return { ast_json_t{statement}, expression_fr.second };
}


void ut_compare_json_and_rest(const pair<ast_json_t, seq_t>& result_pair, const std::string& expected_json, const std::string& expected_rest){
	ut_compare_jsons(
		result_pair.first._value,
		parse_json(seq_t(expected_json)).first
	);

	quark::ut_compare_strings(result_pair.second.str(), expected_rest);
}

QUARK_UNIT_TEST("parse_bind_statement", "", "", ""){
	ut_compare_json_and_rest(
		parse_bind_statement(seq_t("let int test = 123 let int a = 4 ")),
		R"(
			[ "bind", "^int", "test", ["k", 123, "^int"]]
		)",
		" let int a = 4 "
	);
}



QUARK_UNIT_TEST("parse_bind_statement", "", "", ""){
	ut_compare_jsons(
		parse_bind_statement(seq_t("let bool bb = true")).first._value,
		parse_json(seq_t(
			R"(
				[ "bind", "^bool", "bb", ["k", true, "^bool"]]
			)"
		)).first
	);
}
QUARK_UNIT_TEST("parse_bind_statement", "", "", ""){
	ut_compare_jsons(
		parse_bind_statement(seq_t("let int hello = 3")).first._value,
		parse_json(seq_t(
			R"(
				[ "bind", "^int", "hello", ["k", 3, "^int"]]
			)"
		)).first
	);
}

QUARK_UNIT_TEST("parse_bind_statement", "", "", ""){
	ut_compare_jsons(
		parse_bind_statement(seq_t("mutable int a = 14")).first._value,
		parse_json(seq_t(
			R"(
				[ "bind", "^int", "a", ["k", 14, "^int"], { "mutable": true }]
			)"
		)).first
	);
}

QUARK_UNIT_TEST("parse_bind_statement", "", "", ""){
	ut_compare_jsons(
		parse_bind_statement(seq_t("mutable hello = 3")).first._value,
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
	if(variable_pos.first.empty()){
		throw std::runtime_error("Assign syntax error");
	}
	const auto equal_pos = read_required_char(skip_whitespace(variable_pos.second), '=');
	const auto rhs_seq = skip_whitespace(equal_pos);
	const auto expression_fr = parse_expression_seq(rhs_seq);

	const auto statement = json_t::make_array({ "store", variable_pos.first, expression_fr.first._value });
	return { ast_json_t{statement}, expression_fr.second };
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
	const auto expression_fr = parse_expression_seq(s);

	const auto statement = json_t::make_array({ "expression-statement", expression_fr.first._value });
	return { ast_json_t{statement}, expression_fr.second };
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
	if(implicit_type == implicit_statement::k_expression_statement){
		return parse_expression_statement(pos);
	}
	else if(implicit_type == implicit_statement::k_assign){
		return parse_assign_statement(pos);
	}
	else{
		throw std::runtime_error("Use 'mutable' or 'let' syntax.");
	}
}

/*
QUARK_UNIT_TEST("", "parse_prefixless_statement()", "", ""){
	ut_compare_jsons(
		parse_prefixless_statement(seq_t("x = f(3);")).first._value,
		parse_json(seq_t(R"(["bind", "^int", "x", ["call", ["@", "f"], [["k", 3, "^int"]]]])")).first
	);
}
*/

}	//	floyd
