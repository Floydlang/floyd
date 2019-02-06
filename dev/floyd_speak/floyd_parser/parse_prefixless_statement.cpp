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

#include "ast_typeid.h"
#include "ast_typeid_helpers.h"
#include "ast_json.h"

#include <string>
#include <memory>
#include <map>
#include <iostream>
#include <cmath>


namespace floyd {


using namespace std;



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

implicit_statement detect_implicit_statement_lookahead(const seq_t& s){
	if(is_identifier_and_equal(s)){
		return implicit_statement::k_assign;
	}
	else{
		//	Detect "int test = 123" which is common illegal syntax, where you forgot "mutable" or "let".

		try {
			const auto maybe_type = read_type(s);
			if(maybe_type.first != nullptr){
				if(maybe_type.first->is_function()){
					throw_compiler_error(location_t(s.pos()), "Function types not supported.");
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




//	Finds identifier-string at RIGHT side of string. Stops at whitespace.
//	"1234()=<whatever> IDENTIFIER": "1234()=<whatever>", "IDENTIFIER"
std::size_t split_at_tail_identifier(const std::string& s){
	auto i = s.size();
	while(i > 0 && whitespace_chars.find(s[i - 1]) != string::npos){
		i--;
	}
	while(i > 0 && identifier_chars.find(s[i - 1]) != string::npos){
		i--;
	}
	return i;
}



//	BIND:			int x = 10
//	BIND:			x = 10
//	BIND:			int (string a) x = f(4 == 5)
//	BIND:			int x = 10
//	BIND:			x = 10
struct a_result_t {
	typeid_t type;
	std::string identifier;

	//	Points to "=" or end of sequence if no "=" was found.
	seq_t rest;
};

a_result_t parse_a(const seq_t& p, const location_t& loc){
	const auto pos = skip_whitespace(p);

	//	Notice: if there is no type, only and identifier -- then we still get a type back: with an unresolved identifier.
	const auto optional_type_pos = read_type(pos);
	const auto identifier_pos = read_identifier(optional_type_pos.second);

	if(optional_type_pos.first && identifier_pos.first != ""){
		return a_result_t{ *optional_type_pos.first, identifier_pos.first, identifier_pos.second };
	}
	else if(!optional_type_pos.first && identifier_pos.first != ""){
		QUARK_ASSERT(false);
		return a_result_t{ typeid_t::make_undefined(), optional_type_pos.first->get_unresolved_type_identifier(), identifier_pos.second };
	}
	else if(optional_type_pos.first && optional_type_pos.first->is_unresolved_type_identifier() && identifier_pos.first == ""){
		return a_result_t{ typeid_t::make_undefined(), optional_type_pos.first->get_unresolved_type_identifier(), identifier_pos.second };
	}
	else{
		throw_compiler_error(loc, "Require a value for new bind.");
	}
}

pair<ast_json_t, seq_t> parse_let(const seq_t& pos, const location_t& loc){
	const auto a_result = parse_a(pos, loc);
	if(a_result.rest.empty()){
		throw_compiler_error(loc, "Require a value for new bind.");
	}
	const auto equal_sign = read_required(skip_whitespace(a_result.rest), "=");
	const auto expression_pos = parse_expression(equal_sign);

	const auto params = std::vector<json_t>{
		typeid_to_ast_json(a_result.type, json_tags::k_tag_resolve_state)._value,
		a_result.identifier,
		expression_pos.first._value,
	};
	const auto statement = make_statement_n(loc, statement_opcode_t::k_bind, params);
	return { statement, expression_pos.second };
}

pair<ast_json_t, seq_t> parse_mutable(const seq_t& pos, const location_t& loc){
	const auto a_result = parse_a(pos, loc);
	if(a_result.rest.empty()){
		throw_compiler_error(loc, "Require a value for new bind.");
	}
	const auto equal_sign = read_required(skip_whitespace(a_result.rest), "=");
	const auto expression_pos = parse_expression(equal_sign);

	const auto meta = (json_t::make_object({pair<string,json_t>{"mutable", true}}));

	const auto params = std::vector<json_t>{
		typeid_to_ast_json(a_result.type, json_tags::k_tag_resolve_state)._value,
		a_result.identifier,
		expression_pos.first._value,
		meta
	};
	const auto statement = make_statement_n(loc, statement_opcode_t::k_bind, params);

	return { statement, expression_pos.second };
}



//////////////////////////////////////////////////		parse_bind_statement()


//	BIND:			let int x = 10
//	BIND:			let x = 10
//	BIND:			let int (string a) x = f(4 == 5)
//	BIND:			mutable int x = 10
//	BIND:			mutable x = 10

//	ERROR:			let int x
//	ERROR:			let x
//	ERROR:			mutable int x
//	ERROR:			mutable x

//	ERROR:			x = 10
//	[let]/[mutable] TYPE identifier = EXPRESSION
//					|<----------->|		call this section a.

pair<ast_json_t, seq_t> parse_bind_statement(const seq_t& s){
	const auto start = skip_whitespace(s);
	const auto loc = location_t(start.pos());

	const auto let_pos = if_first(start, keyword_t::k_let);
	if(let_pos.first){
		return parse_let(let_pos.second, loc);
	}

	const auto mutable_pos = if_first(skip_whitespace(s), keyword_t::k_mutable);
	if(mutable_pos.first){
		return parse_mutable(mutable_pos.second, loc);
	}

	quark::throw_runtime_error("Bind syntax error.");
}


QUARK_UNIT_TEST("parse_bind_statement", "", "", ""){
	const auto input = R"(

		mutable string row

	)";

	try {
		ut_verify_json_and_rest(
			QUARK_POS,
			parse_bind_statement(seq_t(input)),
			R"(
				[ 0, "bind", "^int", "test", ["k", 123, "^int"]]
			)",
			" let int a = 4 "
		);
		fail_test(QUARK_POS);
	}
	catch(const runtime_error& e){
		//	Should come here.
		ut_verify(QUARK_POS, e.what(), "Expected \'=\' character.");
	}
}


QUARK_UNIT_TEST("parse_bind_statement", "", "", ""){
	ut_verify_json_and_rest(
		QUARK_POS,
		parse_bind_statement(seq_t("let int test = 123 let int a = 4 ")),
		R"(
			[ 0, "bind", "^int", "test", ["k", 123, "^int"]]
		)",
		" let int a = 4 "
	);
}



QUARK_UNIT_TEST("parse_bind_statement", "", "", ""){
	ut_verify(QUARK_POS,
		parse_bind_statement(seq_t("let bool bb = true")).first._value,
		parse_json(seq_t(
			R"(
				[ 0, "bind", "^bool", "bb", ["k", true, "^bool"]]
			)"
		)).first
	);
}
QUARK_UNIT_TEST("parse_bind_statement", "", "", ""){
	ut_verify(QUARK_POS, 
		parse_bind_statement(seq_t("let int hello = 3")).first._value,
		parse_json(seq_t(
			R"(
				[ 0, "bind", "^int", "hello", ["k", 3, "^int"]]
			)"
		)).first
	);
}

QUARK_UNIT_TEST("parse_bind_statement", "", "", ""){
	ut_verify(QUARK_POS,
		parse_bind_statement(seq_t("mutable int a = 14")).first._value,
		parse_json(seq_t(
			R"(
				[ 0, "bind", "^int", "a", ["k", 14, "^int"], { "mutable": true }]
			)"
		)).first
	);
}

QUARK_UNIT_TEST("parse_bind_statement", "", "", ""){
	ut_verify(QUARK_POS,
		parse_bind_statement(seq_t("mutable hello = 3")).first._value,
		parse_json(seq_t(
			R"(
				[ 0, "bind", "^**undef**", "hello", ["k", 3, "^int"], { "mutable": true }]
			)"
		)).first
	);
}

QUARK_UNIT_TEST("parse_bind_statement", "", "", ""){
	ut_verify_json_and_rest(
		QUARK_POS,
		parse_bind_statement(seq_t("let int (double, [string]) test = 123 let int a = 4 ")),
		R"(
			[0, "bind", ["fun", "^int", ["^double", ["vector", "^string"]], true], "test", ["k", 123, "^int"]]
		)",
		" let int a = 4 "
	);
}

//### test float literal
//### test string literal


//////////////////////////////////////////////////		parse_assign_statement()


pair<ast_json_t, seq_t> parse_assign_statement(const seq_t& s){
	const auto start = skip_whitespace(s);
	const auto variable_pos = read_identifier(start);
	if(variable_pos.first.empty()){
		quark::throw_runtime_error("Assign syntax error");
	}
	const auto equal_pos = read_required_char(skip_whitespace(variable_pos.second), '=');
	const auto rhs_seq = skip_whitespace(equal_pos);
	const auto expression_fr = parse_expression(rhs_seq);

	const auto statement = make_statement_n(location_t(start.pos()), statement_opcode_t::k_store, { variable_pos.first, expression_fr.first._value });
	return { statement, expression_fr.second };
}

QUARK_UNIT_TEST("", "parse_assign_statement()", "", ""){
	ut_verify(QUARK_POS,
		parse_assign_statement(seq_t("x = 10;")).first._value,
		parse_json(seq_t(
			R"(
				[ 0, "store","x",["k",10,"^int"] ]
			)"
		)).first
	);
}


//////////////////////////////////////////////////		parse_expression_statement()


pair<ast_json_t, seq_t> parse_expression_statement(const seq_t& s){
	const auto start = skip_whitespace(s);
	const auto expression_fr = parse_expression(start);

	const auto statement = make_statement1(location_t(start.pos()), statement_opcode_t::k_expression_statement, expression_fr.first._value);
	return { ast_json_t{statement}, expression_fr.second };
}

QUARK_UNIT_TEST("", "parse_expression_statement()", "", ""){
	ut_verify(QUARK_POS,
		parse_expression_statement(seq_t("print(14);")).first._value,
		parse_json(seq_t(
			R"(
				[ 0, "expression-statement", [ "call", ["@", "print"], [["k", 14, "^int"]] ] ]
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
		quark::throw_runtime_error("Use 'mutable' or 'let' syntax.");
	}
}

/*
QUARK_UNIT_TEST("", "parse_prefixless_statement()", "", ""){
	ut_verify(QUARK_POS,
		parse_prefixless_statement(seq_t("x = f(3);")).first._value,
		parse_json(seq_t(R"(["bind", "^int", "x", ["call", ["@", "f"], [["k", 3, "^int"]]]])")).first
	);
}
*/

}	//	floyd
