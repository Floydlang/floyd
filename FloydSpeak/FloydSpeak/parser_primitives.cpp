//
//  parser_primitives.cpp
//  FloydSpeak
//
//  Created by Marcus Zetterquist on 26/07/16.
//  Copyright © 2016 Marcus Zetterquist. All rights reserved.
//


#include "parser_primitives.h"


#include "text_parser.h"

#include "steady_vector.h"
#include "statements.h"
#include "parser_value.h"
#include "json_support.h"
#include "json_parser.h"
#include "json_writer.h"

#include <string>
#include <memory>
#include <map>
#include <iostream>
#include <cmath>

namespace floyd {

using std::make_shared;
using std::shared_ptr;
using std::vector;
using std::string;
using std::pair;


//////////////////////////////////////////////////		Text parsing primitives



std::string skip_whitespace(const string& s){
	return read_while(seq_t(s), whitespace_chars).second.get_s();
}

QUARK_UNIT_TEST("", "skip_whitespace()", "", ""){
	QUARK_TEST_VERIFY(skip_whitespace("") == "");
}
QUARK_UNIT_TEST("", "skip_whitespace()", "", ""){
	QUARK_TEST_VERIFY(skip_whitespace(" ") == "");
}
QUARK_UNIT_TEST("", "skip_whitespace()", "", ""){
	QUARK_TEST_VERIFY(skip_whitespace("\t") == "");
}
QUARK_UNIT_TEST("", "skip_whitespace()", "", ""){
	QUARK_TEST_VERIFY(skip_whitespace("int blob()") == "int blob()");
}
QUARK_UNIT_TEST("", "skip_whitespace()", "", ""){
	QUARK_TEST_VERIFY(skip_whitespace("\t\t\t int blob()") == "int blob()");
}


seq_t skip_whitespace(const seq_t& s){
	return read_while(s, whitespace_chars).second;
}


bool is_whitespace(char ch){
	return whitespace_chars.find(string(1, ch)) != string::npos;
}

QUARK_UNIT_TEST("", "is_whitespace()", "", ""){
	QUARK_TEST_VERIFY(is_whitespace('x') == false);
}
QUARK_UNIT_TEST("", "is_whitespace()", "", ""){
	QUARK_TEST_VERIFY(is_whitespace(' ') == true);
}
QUARK_UNIT_TEST("", "is_whitespace()", "", ""){
	QUARK_TEST_VERIFY(is_whitespace('\t') == true);
}
QUARK_UNIT_TEST("", "is_whitespace()", "", ""){
	QUARK_TEST_VERIFY(is_whitespace('\n') == true);
}



std::string skip_whitespace_ends(const std::string& s){
	const auto a = skip_whitespace(s);

	string::size_type i = a.size();
	while(i > 0 && whitespace_chars.find(a[i - 1]) != string::npos){
		i--;
	}
	return a.substr(0, i);
}

QUARK_UNIT_TEST("", "skip_whitespace_ends()", "", ""){
	QUARK_TEST_VERIFY(skip_whitespace_ends("") == "");
}
QUARK_UNIT_TEST("", "skip_whitespace_ends()", "", ""){
	QUARK_TEST_VERIFY(skip_whitespace_ends("  a") == "a");
}
QUARK_UNIT_TEST("", "skip_whitespace_ends()", "", ""){
	QUARK_TEST_VERIFY(skip_whitespace_ends("b  ") == "b");
}
QUARK_UNIT_TEST("", "skip_whitespace_ends()", "", ""){
	QUARK_TEST_VERIFY(skip_whitespace_ends("  c  ") == "c");
}



//////////////////////////////////////////////////		BALANCING PARANTHESES, BRACKETS



bool is_start_char(char c){
	return c == '(' || c == '[' || c == '{';
}

QUARK_UNIT_TEST("", "is_start_char()", "", ""){
	QUARK_TEST_VERIFY(is_start_char('('));
}
QUARK_UNIT_TEST("", "is_start_char()", "", ""){
	QUARK_TEST_VERIFY(is_start_char('['));
}
QUARK_UNIT_TEST("", "is_start_char()", "", ""){
	QUARK_TEST_VERIFY(is_start_char('{'));
}
QUARK_UNIT_TEST("", "is_start_char()", "", ""){
	QUARK_TEST_VERIFY(!is_start_char('<'));
}

QUARK_UNIT_TEST("", "is_start_char()", "", ""){
	QUARK_TEST_VERIFY(!is_start_char(')'));
}
QUARK_UNIT_TEST("", "is_start_char()", "", ""){
	QUARK_TEST_VERIFY(!is_start_char(']'));
}
QUARK_UNIT_TEST("", "is_start_char()", "", ""){
	QUARK_TEST_VERIFY(!is_start_char('}'));
}
QUARK_UNIT_TEST("", "is_start_char()", "", ""){
	QUARK_TEST_VERIFY(!is_start_char('>'));
}
QUARK_UNIT_TEST("", "is_start_char()", "", ""){
	QUARK_TEST_VERIFY(!is_start_char(' '));
}


bool is_end_char(char c){
	return c == ')' || c == ']' || c == '}';
}

QUARK_UNIT_TEST("", "is_end_char()", "", ""){
	QUARK_TEST_VERIFY(!is_end_char('('));
}
QUARK_UNIT_TEST("", "is_end_char()", "", ""){
	QUARK_TEST_VERIFY(!is_end_char('['));
}
QUARK_UNIT_TEST("", "is_end_char()", "", ""){
	QUARK_TEST_VERIFY(!is_end_char('{'));
}
QUARK_UNIT_TEST("", "is_end_char()", "", ""){
	QUARK_TEST_VERIFY(!is_end_char('<'));
}
QUARK_UNIT_TEST("", "is_end_char()", "", ""){
	QUARK_TEST_VERIFY(is_end_char(')'));
}
QUARK_UNIT_TEST("", "is_end_char()", "", ""){
	QUARK_TEST_VERIFY(is_end_char(']'));
}
QUARK_UNIT_TEST("", "is_end_char()", "", ""){
	QUARK_TEST_VERIFY(is_end_char('}'));
}
QUARK_UNIT_TEST("", "is_end_char()", "", ""){
	QUARK_TEST_VERIFY(!is_end_char('>'));
}
QUARK_UNIT_TEST("", "is_end_char()", "", ""){
	QUARK_TEST_VERIFY(!is_end_char(' '));
}



char start_char_to_end_char(char start_char){
	QUARK_ASSERT(is_start_char(start_char));

	if(start_char == '('){
		return ')';
	}
	else if(start_char == '['){
		return ']';
	}
	else if(start_char == '{'){
		return '}';
	}
	else {
		QUARK_ASSERT(false);
	}
}

QUARK_UNIT_TEST("", "start_char_to_end_char()", "", ""){
	QUARK_TEST_VERIFY(start_char_to_end_char('(') == ')');
}
QUARK_UNIT_TEST("", "start_char_to_end_char()", "", ""){
	QUARK_TEST_VERIFY(start_char_to_end_char('[') == ']');
}
QUARK_UNIT_TEST("", "start_char_to_end_char()", "", ""){
	QUARK_TEST_VERIFY(start_char_to_end_char('{') == '}');
}

std::pair<std::string, seq_t> get_balanced(const seq_t& s){
	QUARK_ASSERT(s.size() > 0);

	const auto r = read_balanced2(s, brackets);
	if(r.first == ""){
		throw std::runtime_error("unbalanced ([{< >}])");
	}

	return r;
}

QUARK_UNIT_TEST("", "get_balanced()", "", ""){
//	QUARK_TEST_VERIFY(get_balanced("") == seq("", ""));
	QUARK_TEST_VERIFY(get_balanced(seq_t("()")) == (std::pair<std::string, seq_t>("()", seq_t(""))));
}
QUARK_UNIT_TEST("", "get_balanced()", "", ""){
	QUARK_TEST_VERIFY(get_balanced(seq_t("(abc)")) == (std::pair<std::string, seq_t>("(abc)", seq_t(""))));
}
QUARK_UNIT_TEST("", "get_balanced()", "", ""){
	QUARK_TEST_VERIFY(get_balanced(seq_t("(abc)def")) == (std::pair<std::string, seq_t>("(abc)", seq_t("def"))));
}
QUARK_UNIT_TEST("", "get_balanced()", "", ""){
	QUARK_TEST_VERIFY(get_balanced(seq_t("((abc))def")) == (std::pair<std::string, seq_t>("((abc))", seq_t("def"))));
}
QUARK_UNIT_TEST("", "get_balanced()", "", ""){
	QUARK_TEST_VERIFY(get_balanced(seq_t("((abc)[])def")) == (std::pair<std::string, seq_t>("((abc)[])", seq_t("def"))));
}
QUARK_UNIT_TEST("", "get_balanced()", "", ""){
	QUARK_TEST_VERIFY(get_balanced(seq_t("(return 4 < 5;)xxx")) == (std::pair<std::string, seq_t>("(return 4 < 5;)", seq_t("xxx"))));
}
QUARK_UNIT_TEST("", "get_balanced()", "", ""){
	QUARK_TEST_VERIFY(get_balanced(seq_t("{}")) == (std::pair<std::string, seq_t>("{}", seq_t(""))));
}
QUARK_UNIT_TEST("", "get_balanced()", "", ""){
	QUARK_TEST_VERIFY(get_balanced(seq_t("{aaa}bbb")) == (std::pair<std::string, seq_t>("{aaa}", seq_t("bbb"))));
}
QUARK_UNIT_TEST("", "get_balanced()", "", ""){
	QUARK_TEST_VERIFY(get_balanced(seq_t("{return 4 < 5;}xxx")) == (std::pair<std::string, seq_t>("{return 4 < 5;}", seq_t("xxx"))));
}
//	QUARK_TEST_VERIFY(get_balanced("{\n\t\t\t\treturn 4 < 5;\n\t\t\t}\n\t\t") == seq("((abc)[])", "def"));



std::pair<std::string, seq_t> read_enclosed_in_parantheses(const seq_t& pos){
	const auto pos2 = skip_whitespace(pos);
	read_required(pos2, "(");
	const auto range = get_balanced(pos2);
	const auto range2 = seq_t(trim_ends(range.first));
	return { range2.str(), range.second };
}

QUARK_UNIT_TEST("", "read_enclosed_in_parantheses()", "", ""){
	QUARK_UT_VERIFY((	read_enclosed_in_parantheses(seq_t("()xyz")) == std::pair<std::string, seq_t>{"", seq_t("xyz") } 	));
}
QUARK_UNIT_TEST("", "read_enclosed_in_parantheses()", "", ""){
	QUARK_UT_VERIFY((	read_enclosed_in_parantheses(seq_t(" ( abc )xyz")) == std::pair<std::string, seq_t>{" abc ", seq_t("xyz") } 	));
}





std::pair<string, seq_t> read_until_toplevel_match(const seq_t& s, const std::string& match_chars){
	auto pos = s;
	while(pos.empty() == false && match_chars.find(pos.first1()) == string::npos){
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

///### TESTS



//////////////////////////////////////		BASIC STRING


std::string reverse(const std::string& s){
	return std::string(s.rbegin(), s.rend());
}




//////////////////////////////////////		IDENTIFIER


//	Returns "" if no symbol is found.
std::pair<std::string, seq_t> read_identifier(const seq_t& s){
	const auto a = skip_whitespace(s);
	const auto b = read_while(a, identifier_chars);
	return b;
}
std::pair<std::string, seq_t> read_required_identifier(const seq_t& s){
	const auto b = read_identifier(s);
	if(b.first.empty()){
		throw std::runtime_error("missing identifier");
	}
	return b;
}

QUARK_UNIT_TESTQ("read_required_identifier()", ""){
	QUARK_TEST_VERIFY(read_required_identifier(seq_t("\thello\txxx")) == (std::pair<std::string, seq_t>("hello", seq_t("\txxx"))));
}




//////////////////////////////////////		TYPES



std::pair<shared_ptr<typeid_t>, seq_t> read_basic_type(const seq_t& s){
	const auto pos0 = skip_whitespace(s);

	const auto pos1 = read_while(pos0, identifier_chars);
	if(pos1.first.empty()){
		return { nullptr, pos1.second };
	}
	else if(pos1.first == "null"){
		return { make_shared<typeid_t>(typeid_t::make_null()), pos1.second };
	}
	else if(pos1.first == "bool"){
		return { make_shared<typeid_t>(typeid_t::make_bool()), pos1.second };
	}
	else if(pos1.first == "int"){
		return { make_shared<typeid_t>(typeid_t::make_int()), pos1.second };
	}
	else if(pos1.first == "float"){
		return { make_shared<typeid_t>(typeid_t::make_float()), pos1.second };
	}
	else if(pos1.first == "string"){
		return { make_shared<typeid_t>(typeid_t::make_string()), pos1.second };
	}
	else{
		return { make_shared<typeid_t>(typeid_t::make_unknown_identifier(pos1.first)), pos1.second };
	}
}


vector<pair<typeid_t, string>> parse_functiondef_arguments2(const string& s){
	QUARK_ASSERT(s.size() >= 2);
	QUARK_ASSERT(s[0] == '(');
	QUARK_ASSERT(s.back() == ')');

	const auto s2 = seq_t(trim_ends(s));
	vector<pair<typeid_t, string>> args;
	auto pos = skip_whitespace(s2);
	while(!pos.empty()){
		const auto arg_type = read_required_type(pos);
		const auto arg_name = read_identifier(arg_type.second);
		const auto optional_comma = read_optional_char(skip_whitespace(arg_name.second), ',');
		args.push_back({ arg_type.first, arg_name.first });
		pos = skip_whitespace(optional_comma.second);
	}
	return args;
}

QUARK_UNIT_TEST("", "parse_functiondef_arguments2()", "", ""){
	QUARK_TEST_VERIFY((		parse_functiondef_arguments2("()") == vector<pair<typeid_t, string>>{}		));
}
QUARK_UNIT_TEST("", "parse_functiondef_arguments2()", "", ""){
	QUARK_TEST_VERIFY((		parse_functiondef_arguments2("(int a)") == vector<pair<typeid_t, string>>{ { typeid_t::make_int(), "a" }}		));
}
QUARK_UNIT_TEST("", "parse_functiondef_arguments2()", "", ""){
	QUARK_TEST_VERIFY((		parse_functiondef_arguments2("(int x, string y, float z)") == vector<pair<typeid_t, string>>{
		{ typeid_t::make_int(), "x" },
		{ typeid_t::make_string(), "y" },
		{ typeid_t::make_float(), "z" }

	}		));
}

//??? use member_t
std::pair<vector<pair<typeid_t, string>>, seq_t> read_function_arg_parantheses(const seq_t& s){
	QUARK_ASSERT(s.first1() == "(");

	std::pair<std::string, seq_t> args_pos = read_balanced2(s, brackets);
	if(args_pos.first.empty()){
		throw std::runtime_error("unbalanced ()");
	}
	const auto r = parse_functiondef_arguments2(args_pos.first);
	return { r, args_pos.second };
}

std::pair<shared_ptr<typeid_t>, seq_t> read_basic_or_vector(const seq_t& s){
	const auto pos0 = skip_whitespace(s);
	if(pos0.first1() == "["){
		const auto pos2 = pos0.rest1();
		const auto element_type_pos = read_required_type(pos2);
		const auto pos3 = skip_whitespace(element_type_pos.second);
		if(pos3.first1() != "]"){
			throw std::runtime_error("unbalanced []");
		}
		return { make_shared<typeid_t>(typeid_t::make_vector(element_type_pos.first)), pos3.rest1() };
	}
	else {
		//	Read basic type.
		const auto basic = read_basic_type(pos0);
		return basic;
	}
}

std::pair<shared_ptr<typeid_t>, seq_t> read_optional_trailing_function_args(const typeid_t& type, const seq_t& s){
	//	See if there is a () afterward type_pos -- that would be that type_pos is the return value of a function-type.
	const auto more_pos = skip_whitespace(s);
	if(more_pos.first1() == "("){
		const auto function_args_pos = read_function_arg_parantheses(more_pos);
		vector<typeid_t> nameless_args;
		for(const auto e: function_args_pos.first){
			nameless_args.push_back(e.first);
		}
		const auto pos = function_args_pos.second;
		const auto function_type = typeid_t::make_function(type, nameless_args);
		const auto result = read_optional_trailing_function_args(function_type, pos);
		return result;
	}
	else{
		return { make_shared<typeid_t>(type), s };
	}
}
std::pair<std::shared_ptr<typeid_t>, seq_t> read_type(const seq_t& s){
	const auto type_pos = read_basic_or_vector(s);
	if(type_pos.first == nullptr){
		return type_pos;
	}
	else {
		return read_optional_trailing_function_args(*type_pos.first, type_pos.second);
	}
}

QUARK_UNIT_TEST("", "read_type()", "", ""){
	QUARK_TEST_VERIFY(read_type(seq_t("-3")).first == nullptr);
}
QUARK_UNIT_TEST("", "read_type()", "", ""){
	QUARK_TEST_VERIFY(*read_type(seq_t("null")).first == typeid_t::make_null());
}
QUARK_UNIT_TEST("", "read_type()", "", ""){
	QUARK_TEST_VERIFY(*read_type(seq_t("bool")).first == typeid_t::make_bool());
}
QUARK_UNIT_TEST("", "read_type()", "", ""){
	QUARK_TEST_VERIFY(*read_type(seq_t("int")).first == typeid_t::make_int());
}
QUARK_UNIT_TEST("", "read_type()", "", ""){
	QUARK_TEST_VERIFY(*read_type(seq_t("float")).first == typeid_t::make_float());
}
QUARK_UNIT_TEST("", "read_type()", "", ""){
	QUARK_TEST_VERIFY(*read_type(seq_t("string")).first == typeid_t::make_string());
}
QUARK_UNIT_TEST("", "read_type()", "", ""){
	const auto r = read_type(seq_t("temp"));
	QUARK_TEST_VERIFY(*r.first ==  typeid_t::make_unknown_identifier("temp"));
	QUARK_TEST_VERIFY(r.second == seq_t(""));
}
QUARK_UNIT_TEST("", "read_type()", "", ""){
	const auto r = read_type(seq_t("[int]"));
	QUARK_TEST_VERIFY(	*r.first ==  typeid_t::make_vector(typeid_t::make_int())		);
	QUARK_TEST_VERIFY(r.second == seq_t(""));
}
QUARK_UNIT_TEST("", "read_type()", "", ""){
	const auto r = read_type(seq_t("[[int]]"));
	QUARK_TEST_VERIFY(	*r.first ==  typeid_t::make_vector(typeid_t::make_vector(typeid_t::make_int()))		);
	QUARK_TEST_VERIFY(r.second == seq_t(""));
}


QUARK_UNIT_TEST("", "read_type()", "", ""){
	const auto r = read_type(seq_t("int ()"));
	QUARK_TEST_VERIFY(*r.first ==  typeid_t::make_function(typeid_t::make_int(), {}));
	QUARK_TEST_VERIFY(r.second == seq_t(""));
}

QUARK_UNIT_TEST("", "read_type_identifier()", "", ""){
	const auto r = read_type(seq_t("string (float a, float b)"));
	QUARK_TEST_VERIFY(	*r.first ==  typeid_t::make_function(typeid_t::make_string(), { typeid_t::make_float(), typeid_t::make_float() })	);
	QUARK_TEST_VERIFY(r.second == seq_t(""));
}
QUARK_UNIT_TEST("", "read_type_identifier()", "", ""){
	const auto r = read_type(seq_t("int (float a) ()"));

	QUARK_TEST_VERIFY( *r.first == typeid_t::make_function(
			typeid_t::make_function(typeid_t::make_int(), { typeid_t::make_float() }),
			{}
		)
	);
	QUARK_TEST_VERIFY(	r.second == seq_t("") );
}
QUARK_UNIT_TEST("", "read_type_identifier()", "", ""){
	const auto r = read_type(seq_t("bool (int (float a) b)"));

	QUARK_TEST_VERIFY(
		*r.first
		==
		typeid_t::make_function(
			typeid_t::make_bool(),
			{
				typeid_t::make_function(typeid_t::make_int(), { typeid_t::make_float() })
			}
		)
	);

	QUARK_TEST_VERIFY(	r.second == seq_t("") );
}
/*
QUARK_UNIT_TEST("", "read_type_identifier()", "", ""){
	QUARK_TEST_VERIFY(read_type_identifier(seq_t("int (float a) g(int(float b))")).first == "int (float a) g(int(float b))");
}

-	TYPE-IDENTIFIER (TYPE-IDENTIFIER a, TYPE-IDENTIFIER b, ...)

[int]([int] audio)
*/



pair<typeid_t, seq_t> read_required_type(const seq_t& s){
	const auto type_pos = read_type(s);
	if(type_pos.first == nullptr){
		throw std::runtime_error("illegal character in type identifier");
	}
	return { *type_pos.first, type_pos.second };
}









//////////////////////////////////////		FLOYD JSON BASICS





json_t make_member_def(const typeid_t& type, const std::string& name){
	QUARK_ASSERT(type.check_invariant());
	QUARK_ASSERT(name.size() > 0);

	return json_t::make_object({
		{ "type", typeid_to_ast_json(type) },
		{ "name", name }
	});
}



}	//	floyd
