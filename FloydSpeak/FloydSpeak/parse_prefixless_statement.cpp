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

//////////////////////////////////////////////////		parse_implicit_statement()



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


//	Scans from right side and finds position where paranthesis starts.
//	EXPRESSION-STATEMENT:	IDENTIFIER(EXPRESSION, EXPRESSION*);
size_t find_parantheses_left__from_back(const seq_t& s){
	const auto s2 = s.str();

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
	return split_pos;
}


/*
	Function definition or an expression?
	int print(float a) { ... }					==	function definition
	int print (float a) { ... }					==	function definition
	print(3);									==	expression
	print (3);									==	expression
	3 + 4;										==	expression
	hello + 3									==	expression

	TYPE<whitespace>IDENTIFIER "(" ARGUMENTS ")" "{" ... "}"
*/
bool is_function_def(const seq_t& pos){
	const auto posx = read_until_toplevel_match(skip_whitespace(pos), ";{");

	if(posx.second.first() == "{"){
		return true;
	}
	else{
		return false;
	}

/*

//	const auto expression_pos = parse_expression_seq(pos);
	const auto type_pos = read_type(pos);
	if(type_pos.first){
		const auto whitespace = skip_whitespace2(type_pos.second);
		if(whitespace.first.size() > 0){
			const auto function_name = read_identifier(whitespace.second);
			if(function_name.first.size() > 0){
				const auto pos2 = skip_whitespace2(function_name.second);
				if(pos2.second.first1() == "("){
					return true;
				}
				else{
					return false;
				}
			}
			else{
				return false;
			}
		}
		else{
			return false;
		}
	}
	else{
		return false;
	}
*/

}

QUARK_UNIT_TEST("", "is_function_def()", "", ""){
	QUARK_UT_VERIFY(is_function_def(seq_t("print(3);")) == false);
	QUARK_UT_VERIFY(is_function_def(seq_t("print ( \"Hello, World!\" ) ;xyz")) == false);
	QUARK_UT_VERIFY(is_function_def(seq_t("print(3);xyz")) == false);
	QUARK_UT_VERIFY(is_function_def(seq_t("3;xyz")) == false);
	QUARK_UT_VERIFY(is_function_def(seq_t("3 + 4;xyz")) == false);
	QUARK_UT_VERIFY(is_function_def(seq_t("3 + f(1) + f(2);xyz")) == false);
	QUARK_UT_VERIFY(is_function_def(seq_t("3")) == false);


	QUARK_UT_VERIFY(is_function_def(seq_t(" int f ( ) { return 0 ; } xyz")) == true);
	QUARK_UT_VERIFY(is_function_def(seq_t(" int f ( string name ) { return 13 ; }xyz")) == true);
	QUARK_UT_VERIFY(is_function_def(seq_t(" int ( string a ) f ( string name ) { return 100 == 101 ; }xyz")) == true);
}







//	NOTICE: This function is very complex -- let's keep it FOCUSED just on figuring out the type of statement.
//	Don't give it more work.
pair<vector<string>, seq_t> parse_implicit_statement(const seq_t& s){
	auto pos = read_until_toplevel_match(skip_whitespace(s), ";{");
	if(pos.second.first1() == ";"){
		pos.first.push_back(';');
		pos.second = pos.second.rest1();
	}
	const auto r = seq_t(pos.first);

	const auto equal_sign_pos = read_until_toplevel_match(r, "=");
	if(equal_sign_pos.second.empty()){
		//	FUNCTION-DEFINITION:	TYPE<spaces or tabs>IDENTIFIER(TYPE IDENTIFIER, TYPE IDENTIFIER.*)
		//	FUNCTION-DEFINITION:	int f(string name)
		//	FUNCTION-DEFINITION:	int (string a) f(string name)

		//	EXPRESSION-STATEMENT:	EXPRESSION;
		//	EXPRESSION-STATEMENT:	IDENTIFIER(EXPRESSION, EXPRESSION*);
		//	EXPRESSION-STATEMENT:	print ("Hello, World!");
		//	EXPRESSION-STATEMENT:	print("Hello, World!" + f(3) == 2);
		//	EXPRESSION-STATEMENT:	print(3);

		//	EXPRESSION-STATEMENT:	3 + 4;

		const auto pos2 = equal_sign_pos.second;
		const auto s2 = pos2.str();
if(false){
		const auto split_pos = find_parantheses_left__from_back(pos2);

		const auto pre_parantheses = s2.substr(0, split_pos);
		const auto parantheses = s2.substr(split_pos);

		const auto pre_identifier__identifier = split_at_tail_identifier(pre_parantheses);
		const auto pre_identifier = skip_whitespace_ends(pre_identifier__identifier.first);
		const auto identifier = skip_whitespace_ends(pre_identifier__identifier.second);

		if(pre_identifier == ""){
			auto s3 = skip_whitespace_ends(s2);
			if(s3.back() != ';'){
				throw std::runtime_error("syntax error");
			}
			s3.pop_back();
			s3 = skip_whitespace_ends(s3);
			return { { "[EXPRESSION-STATEMENT]", s3 }, s };
		}
		else{
			return { { "[FUNCTION-DEFINITION]", skip_whitespace_ends(s2) }, s };
		}
}
else{
	const auto r = skip_whitespace_ends(pos.first);
	if(is_function_def(s)){
		return { { "[FUNCTION-DEFINITION]", r }, s };
	}
	else{
		const auto r2 = skip_whitespace_ends(string(r.begin(), r.end() - 1));
		return { { "[EXPRESSION-STATEMENT]", r2 }, s };
	}
}

	}
	else{
		//	BIND:			int x = 10;
		//	BIND:			int (string a) x = f(4 == 5);
		//	BIND:			mutable int x = 10;
		//	ASSIGN			x = 10;
		//	ASSIGN			x = "hello";
		//	ASSIGN			x = f(3) == 2;
		//	ASSIGN			mutable x = 10;
		//	MUTATE_LOCAL	x <=== 11;
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
		const auto identifier = skip_whitespace_ends(pre_identifier__identifier.second);

		if(pre_identifier == ""){
			return { { "[ASSIGN]", identifier, rhs_expression }, s };
		}
		else{
			return { { "[BIND]", pre_identifier, identifier, rhs_expression }, s };
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
	else if(split.first[0] == "[ASSIGN]"){
		const auto identifier = split.first[1];
		const auto rhs_expression = split.first[2];
		analysis = string() + "[ASSIGN] IDENTIFIER: " + "\"" + identifier + "\"" + " = EXPRESSION: " + "\"" + rhs_expression + "\"";
	}
	else if(split.first[0] == "[BIND]"){
		const auto pre_identifier = split.first[1];
		const auto identifier = split.first[2];
		const auto rhs_expression = split.first[3];
		analysis = string() + "[BIND] TYPE: " + "\"" + pre_identifier + "\"" + " IDENTIFIER: " + "\"" + identifier + "\"" + " = EXPRESSION: " + "\"" + rhs_expression + "\"";
	}
	temp = temp + analysis;

	const auto out = concat_strings(temp);
	return out;
}

QUARK_UNIT_TEST("", "parse_implicit_statement()", "", ""){
	//	BIND	TYPE	IDENTIFIER	=	EXPRESSION;
	QUARK_TRACE((test_split_line("BIND", seq_t("int x = 10;xyz"))));
	QUARK_TRACE((test_split_line("BIND", seq_t("int (string a) x = f(4 == 5);xyz"))));
	QUARK_TRACE((test_split_line("BIND", seq_t("mutable int x = 10;xyz"))));
	QUARK_TRACE((test_split_line("BIND", seq_t("mutable x = 10;xyz"))));

	//	FUNCTION-DEFINITION	TYPE	IDENTIFIER	( EXPRESSION-LIST )	{ STATEMENTS }
	QUARK_TRACE((test_split_line("FUNCTION-DEFINITION", seq_t("int f(){ return 0; }xyz"))));
	QUARK_TRACE((test_split_line("FUNCTION-DEFINITION", seq_t("int f(string name){ return 13; }xyz"))));
	QUARK_TRACE((test_split_line("FUNCTION-DEFINITION", seq_t("int (string a) f(string name){ return 100 == 101; }xyz"))));

	//	EXPRESSION-STATEMENT	EXPRESSION;
	QUARK_TRACE((test_split_line("EXPRESSION-STATEMENT", seq_t("print (\"Hello, World!\");xyz"))));
	QUARK_TRACE((test_split_line("EXPRESSION-STATEMENT", seq_t("print(\"Hello, World!\" + f(3) == 2);xyz"))));
	QUARK_TRACE((test_split_line("EXPRESSION-STATEMENT", seq_t("print(3);xyz"))));

	//	ASSIGN			IDENTIFIER	=	EXPRESSION;
	QUARK_TRACE((test_split_line("ASSIGN", seq_t("x = 10;xyz"))));
	QUARK_TRACE((test_split_line("ASSIGN", seq_t("x = \"hello\";xyz"))));
	QUARK_TRACE((test_split_line("ASSIGN", seq_t("x = f(3) == 2;xyz"))));

	//	MUTATE-LOCAL	IDENTIFIER	<===	EXPRESSION;
	//	QUARK_TRACE((test_split_line("MUTATE-LOCAL", seq_t("x <=== 11;xyz"))));
}


QUARK_UNIT_TEST("", "parse_implicit_statement()", "BIND", ""){
	QUARK_UT_VERIFY2(
		parse_implicit_statement(seq_t(" int x = 10 ; xyz")),
		(pair<vector<string>, seq_t>{
			vector<string>{ "[BIND]", "int", "x", "10" },
			seq_t(" int x = 10 ; xyz")
		})
	);
}


QUARK_UNIT_TEST("", "parse_implicit_statement()", "BIND", ""){
	QUARK_UT_VERIFY((	parse_implicit_statement(seq_t(" int x = 10 ; xyz")) == pair<vector<string>, seq_t>{vector<string>{ "[BIND]", "int", "x", "10" }, seq_t(" int x = 10 ; xyz") }	));
}
QUARK_UNIT_TEST("", "parse_implicit_statement()", "BIND", ""){
	QUARK_UT_VERIFY((	parse_implicit_statement(seq_t(" int ( string a ) x = f ( 4 == 5 ) ; xyz")) == pair<vector<string>, seq_t>{vector<string>{ "[BIND]", "int ( string a )", "x", "f ( 4 == 5 )" }, seq_t(" int ( string a ) x = f ( 4 == 5 ) ; xyz") }	));
}
QUARK_UNIT_TEST("", "parse_implicit_statement()", "BIND", ""){
	QUARK_UT_VERIFY((	parse_implicit_statement(seq_t(" mutable int x = 10 ; xyz")) == pair<vector<string>, seq_t>{vector<string>{ "[BIND]", "mutable int", "x", "10" }, seq_t(" mutable int x = 10 ; xyz") }	));
}
QUARK_UNIT_TEST("", "parse_implicit_statement()", "BIND", ""){
	QUARK_UT_VERIFY((	parse_implicit_statement(seq_t(" mutable x = 10 ;xyz")) == pair<vector<string>, seq_t>{vector<string>{ "[BIND]", "mutable", "x", "10" }, seq_t(" mutable x = 10 ;xyz") }	));
}



QUARK_UNIT_TEST("", "parse_implicit_statement()", "FUNCTION-DEFINITION", ""){
	QUARK_UT_VERIFY((	parse_implicit_statement(seq_t(" int f ( ) { return 0 ; } xyz")) == pair<vector<string>, seq_t>{vector<string>{ "[FUNCTION-DEFINITION]", "int f ( )" }, seq_t(" int f ( ) { return 0 ; } xyz") }	));
}
QUARK_UNIT_TEST("", "parse_implicit_statement()", "FUNCTION-DEFINITION", ""){
	QUARK_UT_VERIFY((	parse_implicit_statement(seq_t(" int f ( string name ) { return 13 ; }xyz")) == pair<vector<string>, seq_t>{vector<string>{ "[FUNCTION-DEFINITION]", "int f ( string name )" }, seq_t(" int f ( string name ) { return 13 ; }xyz") }	));
}
QUARK_UNIT_TEST("", "parse_implicit_statement()", "FUNCTION-DEFINITION", ""){
	QUARK_UT_VERIFY((	parse_implicit_statement(seq_t(" int ( string a ) f ( string name ) { return 100 == 101 ; }xyz")) == pair<vector<string>, seq_t>{vector<string>{ "[FUNCTION-DEFINITION]", "int ( string a ) f ( string name )" }, seq_t(" int ( string a ) f ( string name ) { return 100 == 101 ; }xyz") }	));
}






QUARK_UNIT_TEST("", "parse_implicit_statement()", "EXPRESSION-STATEMENT", ""){
	QUARK_UT_VERIFY((	parse_implicit_statement(seq_t(" print ( \"Hello, World!\" ) ;xyz")) == pair<vector<string>, seq_t>{vector<string>{ "[EXPRESSION-STATEMENT]", "print ( \"Hello, World!\" )" }, seq_t(" print ( \"Hello, World!\" ) ;xyz") }	));
}
QUARK_UNIT_TEST("", "parse_implicit_statement()", "EXPRESSION-STATEMENT", ""){
	QUARK_UT_VERIFY((	parse_implicit_statement(seq_t(" print ( \"Hello, World!\" ) ;xyz")) == pair<vector<string>, seq_t>{vector<string>{ "[EXPRESSION-STATEMENT]", "print ( \"Hello, World!\" )" }, seq_t(" print ( \"Hello, World!\" ) ;xyz") }	));
}
QUARK_UNIT_TEST("", "parse_implicit_statement()", "EXPRESSION-STATEMENT", ""){
	QUARK_UT_VERIFY((	parse_implicit_statement(seq_t("print(3);xyz")) == pair<vector<string>, seq_t>{vector<string>{ "[EXPRESSION-STATEMENT]", "print(3)" }, seq_t("print(3);xyz") }	));
}

QUARK_UNIT_TEST("", "parse_implicit_statement()", "EXPRESSION-STATEMENT", ""){
	QUARK_UT_VERIFY((	parse_implicit_statement(seq_t("print(3);xyz")) == pair<vector<string>, seq_t>{vector<string>{ "[EXPRESSION-STATEMENT]", "print(3)" }, seq_t("print(3);xyz") }	));
}

QUARK_UNIT_TEST("", "parse_implicit_statement()", "EXPRESSION-STATEMENT", ""){
	QUARK_UT_VERIFY2(
		parse_implicit_statement(seq_t("3;xyz")),
		(pair<vector<string>, seq_t>{vector<string>{ "[EXPRESSION-STATEMENT]", "3" }, seq_t("3;xyz") })
	);
}
QUARK_UNIT_TEST("", "parse_implicit_statement()", "EXPRESSION-STATEMENT", ""){
	QUARK_UT_VERIFY((	parse_implicit_statement(seq_t("3 + 4;xyz")) == pair<vector<string>, seq_t>{vector<string>{ "[EXPRESSION-STATEMENT]", "3 + 4" }, seq_t("3 + 4;xyz") }	));
}
QUARK_UNIT_TEST("", "parse_implicit_statement()", "EXPRESSION-STATEMENT", ""){
	QUARK_UT_VERIFY((	parse_implicit_statement(seq_t("3 + f(1) + f(2);xyz")) == pair<vector<string>, seq_t>{vector<string>{ "[EXPRESSION-STATEMENT]", "3 + f(1) + f(2)" }, seq_t("3 + f(1) + f(2);xyz") }	));
}


QUARK_UNIT_TEST("", "parse_implicit_statement()", "ASSIGN", ""){
	QUARK_UT_VERIFY((	parse_implicit_statement(seq_t(" x = 10 ;xyz")) == pair<vector<string>, seq_t>{vector<string>{ "[ASSIGN]", "x", "10" }, seq_t(" x = 10 ;xyz") }	));
}
QUARK_UNIT_TEST("", "parse_implicit_statement()", "ASSIGN", ""){
	QUARK_UT_VERIFY((	parse_implicit_statement(seq_t(" x = \"hello\" ;xyz")) == pair<vector<string>, seq_t>{vector<string>{ "[ASSIGN]", "x", "\"hello\"" }, seq_t(" x = \"hello\" ;xyz") }	));
}
QUARK_UNIT_TEST("", "parse_implicit_statement()", "ASSIGN", ""){
	QUARK_UT_VERIFY((	parse_implicit_statement(seq_t(" x = f ( 3 ) == 2 ;xyz")) == pair<vector<string>, seq_t>{vector<string>{ "[ASSIGN]", "x", "f ( 3 ) == 2" }, seq_t(" x = f ( 3 ) == 2 ;xyz") }	));
}




//////////////////////////////////////////////////		parse_bind_statement()



pair<json_t, seq_t> parse_bind_statement(const vector<string>& parsed_bits, const seq_t& full_statement_pos){
	QUARK_ASSERT(parsed_bits.size() == 4);
	QUARK_ASSERT(parsed_bits[0] == "[BIND]");
	QUARK_ASSERT(parsed_bits[1].empty() == false);
	QUARK_ASSERT(parsed_bits[2].empty() == false);
	QUARK_ASSERT(parsed_bits[3].empty() == false);

	const auto type_seq = seq_t(parsed_bits[1]);
	const auto identifier = parsed_bits[2];
	const auto expression_str = parsed_bits[3];

	const auto mutable_pos = if_first(skip_whitespace(type_seq), keyword_t::k_mutable);
	const bool mutable_flag = mutable_pos.first;
	const auto type_pos = skip_whitespace(mutable_pos.second);

	const auto type = type_pos.empty() ? typeid_t::make_null() : read_required_type(type_pos).first;
	const auto expression = parse_expression_all(seq_t(expression_str));

	const auto meta = mutable_flag ? (json_t::make_object({pair<string,json_t>{"mutable", true}})) : json_t();
	const auto statement = make_array_skip_nulls({ "bind", typeid_to_normalized_json(type), identifier, expression, meta });

	const auto x = read_until(full_statement_pos, ";");
	return { statement, x.second.rest1() };
}

QUARK_UNIT_TESTQ("parse_bind_statement", ""){
	ut_compare_jsons(
		parse_bind_statement({ "[BIND]", keyword_t::k_bool, "bb", keyword_t::k_true }, seq_t("bool bb = true;")).first,
		parse_json(seq_t(
			R"(
				[ "bind", "bool", "bb", ["k", true, "bool"]]
			)"
		)).first
	);
}
QUARK_UNIT_TESTQ("parse_bind_statement", ""){
	ut_compare_jsons(
		parse_bind_statement({ "[BIND]", "int", "hello", "3" }, seq_t("int hello = 3;")).first,
		parse_json(seq_t(
			R"(
				[ "bind", "int", "hello", ["k", 3, "int"]]
			)"
		)).first
	);
}

QUARK_UNIT_TESTQ("parse_bind_statement", ""){
	ut_compare_jsons(
		parse_bind_statement({ "[BIND]", "mutable int", "a", "14" }, seq_t("mutable int hello = 3;")).first,
		parse_json(seq_t(
			R"(
				[ "bind", "int", "a", ["k", 14, "int"], { "mutable": true }]
			)"
		)).first
	);
}

QUARK_UNIT_TESTQ("parse_bind_statement", ""){
	ut_compare_jsons(
		parse_bind_statement({ "[BIND]", "mutable", "a", "14" }, seq_t("mutable hello = 3;")).first,
		parse_json(seq_t(
			R"(
				[ "bind", "null", "a", ["k", 14, "int"], { "mutable": true }]
			)"
		)).first
	);
}

//### test float literal
//### test string literal




//////////////////////////////////////////////////		parse_assign_statement()


pair<json_t, seq_t> parse_assign_statement(const seq_t& s){
	const auto variable_pos = read_identifier(s);
	const auto equal_pos = read_required_char(skip_whitespace(variable_pos.second), '=');
	const auto expression_pos = read_until(skip_whitespace(equal_pos), ";");
	const auto expression = parse_expression_all(seq_t(expression_pos.first));

	const auto statement = json_t::make_array({ "assign", variable_pos.first, expression });
	return { statement, expression_pos.second.rest1() };
}

QUARK_UNIT_TEST("", "parse_assign_statement()", "", ""){
	ut_compare_jsons(
		parse_assign_statement(seq_t("x = 10;")).first,
		parse_json(seq_t(
			R"(
				["assign","x",["k",10,"int"]]
			)"
		)).first
	);
}


//////////////////////////////////////////////////		parse_expression_statement()


pair<json_t, seq_t> parse_expression_statement(const seq_t& s){
	const auto expression_pos = read_until(skip_whitespace(s), ";");
	const auto expression = parse_expression_all(seq_t(expression_pos.first));

	const auto statement = json_t::make_array({ "expression-statement", expression });
	return { statement, expression_pos.second.rest1() };
}

QUARK_UNIT_TEST("", "parse_expression_statement()", "", ""){
	ut_compare_jsons(
		parse_expression_statement(seq_t("print(14);")).first,
		parse_json(seq_t(
			R"(
				[ "expression-statement", [ "call", ["@", "print"], [["k", 14, "int"]] ] ]
			)"
		)).first
	);
}




std::pair<json_t, seq_t> parse_prefixless_statement(const seq_t& s){
	const auto pos = skip_whitespace(s);
	const auto implicit = parse_implicit_statement(pos);
	QUARK_ASSERT(implicit.first.size() > 0);

	const auto statement_type = implicit.first[0];
	if(statement_type == "[BIND]"){
		return parse_bind_statement(implicit.first, implicit.second);
	}
	else if(statement_type == "[FUNCTION-DEFINITION]"){
		return parse_function_definition2(pos);
	}
	else if(statement_type == "[EXPRESSION-STATEMENT]"){
		return parse_expression_statement(pos);
	}
	else if(statement_type == "[ASSIGN]"){
		return parse_assign_statement(pos);
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

}	//	floyd
