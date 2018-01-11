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

namespace floyd_parser {

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



std::string reverse(const std::string& s){
	return std::string(s.rbegin(), s.rend());
}




//////////////////////////////////////		SYMBOLS


//	Returns "" if no symbol is found.
std::pair<std::string, seq_t> read_single_symbol(const seq_t& s){
	const auto a = skip_whitespace(s);
	const auto b = read_while(a, identifier_chars);
	return b;
}
std::pair<std::string, seq_t> read_required_single_symbol(const seq_t& s){
	const auto b = read_single_symbol(s);
	if(b.first.empty()){
		throw std::runtime_error("missing identifier");
	}
	return b;
}

QUARK_UNIT_TESTQ("read_required_single_symbol()", ""){
	QUARK_TEST_VERIFY(read_required_single_symbol(seq_t("\thello\txxx")) == (std::pair<std::string, seq_t>("hello", seq_t("\txxx"))));
}



std::pair<std::string, seq_t> read_type_identifier(const seq_t& s){
	const auto a = skip_whitespace(s);
	const auto b = read_while(a, type_chars);
	return b;
}

QUARK_UNIT_TEST("", "read_type_identifier()", "", ""){
	QUARK_TEST_VERIFY((read_type_identifier(seq_t("-3")) == std::pair<std::string, seq_t>{ "", "-3" }));
}
QUARK_UNIT_TEST("", "read_type_identifier()", "", ""){	QUARK_TEST_VERIFY(read_type_identifier(seq_t("null")).first == "null");
	QUARK_TEST_VERIFY((read_type_identifier(seq_t("null")) == std::pair<std::string, seq_t>{ "null", "" }));
}
QUARK_UNIT_TEST("", "read_type_identifier()", "", ""){
	QUARK_TEST_VERIFY((read_type_identifier(seq_t("bool")) == std::pair<std::string, seq_t>{ "bool", "" }));
}
QUARK_UNIT_TEST("", "read_type_identifier()", "", ""){
	QUARK_TEST_VERIFY((read_type_identifier(seq_t("int")) == std::pair<std::string, seq_t>{ "int", "" }));
}
QUARK_UNIT_TEST("", "read_type_identifier()", "", ""){
	QUARK_TEST_VERIFY((read_type_identifier(seq_t("float")) == std::pair<std::string, seq_t>{ "float", "" }));
}
QUARK_UNIT_TEST("", "read_type_identifier()", "", ""){
	QUARK_TEST_VERIFY((read_type_identifier(seq_t("string")) == std::pair<std::string, seq_t>{ "string", "" }));
}


//??? Fix resolve_base_type_name(). Also make to/from JSON.

std::pair<floyd_basics::typeid_t, seq_t> read_type_identifier2(const seq_t& s){
	const auto pos0 = skip_whitespace(s);
	const auto pos1 = read_while(pos0, identifier_chars);
	if(pos1.first.empty()){
		return { floyd_basics::typeid_t::make_null(), pos1.second };
	}
	else if(pos1.first == "null"){
		return { floyd_basics::typeid_t::make_null(), pos1.second };
	}
	else if(pos1.first == "bool"){
		return { floyd_basics::typeid_t::make_bool(), pos1.second };
	}
	else if(pos1.first == "int"){
		return { floyd_basics::typeid_t::make_int(), pos1.second };
	}
	else if(pos1.first == "float"){
		return { floyd_basics::typeid_t::make_float(), pos1.second };
	}
	else if(pos1.first == "string"){
		return { floyd_basics::typeid_t::make_string(), pos1.second };
	}
	else{
		return { floyd_basics::typeid_t::make_null(), pos1.second };
	}
}

QUARK_UNIT_TEST("", "read_type_identifier2()", "", ""){
	QUARK_TEST_VERIFY(read_type_identifier2(seq_t("-3")).first == floyd_basics::typeid_t::make_null());
}
QUARK_UNIT_TEST("", "read_type_identifier2()", "", ""){
	QUARK_TEST_VERIFY(read_type_identifier2(seq_t("null")).first == floyd_basics::typeid_t::make_null());
}
QUARK_UNIT_TEST("", "read_type_identifier2()", "", ""){
	QUARK_TEST_VERIFY(read_type_identifier2(seq_t("bool")).first == floyd_basics::typeid_t::make_bool());
}
QUARK_UNIT_TEST("", "read_type_identifier2()", "", ""){
	QUARK_TEST_VERIFY(read_type_identifier2(seq_t("int")).first == floyd_basics::typeid_t::make_int());
}
QUARK_UNIT_TEST("", "read_type_identifier2()", "", ""){
	QUARK_TEST_VERIFY(read_type_identifier2(seq_t("float")).first == floyd_basics::typeid_t::make_float());
}
QUARK_UNIT_TEST("", "read_type_identifier2()", "", ""){
	QUARK_TEST_VERIFY(read_type_identifier2(seq_t("string")).first == floyd_basics::typeid_t::make_string());
}

/*
QUARK_UNIT_TEST("", "read_type_identifier()", "", ""){
	QUARK_TEST_VERIFY(read_type_identifier(seq_t("int f()")).first == "int f()");
}
QUARK_UNIT_TEST("", "read_type_identifier()", "", ""){
	QUARK_TEST_VERIFY(read_type_identifier(seq_t("int f(float a, float b)")).first == "int f(float a, float b)");
}
QUARK_UNIT_TEST("", "read_type_identifier()", "", ""){
	QUARK_TEST_VERIFY(read_type_identifier(seq_t("int (float a) g(int(float b))")).first == "int (float a) g(int(float b))");
}
QUARK_UNIT_TEST("", "read_type_identifier()", "", ""){
	QUARK_TEST_VERIFY(read_type_identifier(seq_t("my_symbol")).first == "my_symbol)");
}
QUARK_UNIT_TEST("", "read_type_identifier()", "", ""){
	QUARK_TEST_VERIFY(read_type_identifier(seq_t("my_symbol ()")).first == "my_symbol)");
}
*/

/*
-	null
-	bool
-	int
-	float
-	string
-	TYPE-IDENTIFIER (TYPE-IDENTIFIER a, TYPE-IDENTIFIER b, ...)

#### Custom types
-	struct_type_x
-	struct_type_y
*/



pair<std::string, seq_t> read_required_type_identifier(const seq_t& s){
	const auto type_pos = read_type_identifier(s);
	if(type_pos.first.empty()){
		throw std::runtime_error("illegal character in type identifier");
	}
	return type_pos;
}

	bool is_valid_type_identifier(const std::string& s){
		const auto a = read_while(seq_t(s), floyd_parser::type_chars);
		return a.first == s;
	}





//////////////////////////////////////		FLOYD JSON BASICS





json_t make_member_def(const std::string& type, const std::string& name, const json_t& expression){
	QUARK_ASSERT(type.empty() || (type.size() > 2 && type.front() == '<' && type.back() == '>'));
	QUARK_ASSERT(expression.check_invariant());

	if(expression.is_null()){
		return json_t::make_object({
			{ "type", type },
			{ "name", name }
		});
	}
	else{
		return json_t::make_object({
			{ "type", type },
			{ "name", name },
			{ "expr", expression }
		});
	}
}


}	//	floyd_parser
