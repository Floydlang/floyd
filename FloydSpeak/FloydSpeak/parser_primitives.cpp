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
using std::make_shared;
using std::shared_ptr;



//////////////////////////////////////////////////		Text parsing primitives



std::string skip_whitespace(const string& s){
	return read_while(s, whitespace_chars).second;
}

QUARK_UNIT_TEST("", "skip_whitespace()", "", ""){
	QUARK_TEST_VERIFY(skip_whitespace("") == "");
	QUARK_TEST_VERIFY(skip_whitespace(" ") == "");
	QUARK_TEST_VERIFY(skip_whitespace("\t") == "");
	QUARK_TEST_VERIFY(skip_whitespace("int blob()") == "int blob()");
	QUARK_TEST_VERIFY(skip_whitespace("\t\t\t int blob()") == "int blob()");
}


bool is_whitespace(char ch){
	return whitespace_chars.find(string(1, ch)) != string::npos;
}

QUARK_UNIT_TEST("", "is_whitespace()", "", ""){
	QUARK_TEST_VERIFY(is_whitespace('x') == false);
	QUARK_TEST_VERIFY(is_whitespace(' ') == true);
	QUARK_TEST_VERIFY(is_whitespace('\t') == true);
	QUARK_TEST_VERIFY(is_whitespace('\n') == true);
}


bool is_start_char(char c){
	return c == '(' || c == '[' || c == '{' || c == '<';
}

QUARK_UNIT_TEST("", "is_start_char()", "", ""){
	QUARK_TEST_VERIFY(is_start_char('('));
	QUARK_TEST_VERIFY(is_start_char('['));
	QUARK_TEST_VERIFY(is_start_char('{'));
	QUARK_TEST_VERIFY(is_start_char('<'));

	QUARK_TEST_VERIFY(!is_start_char(')'));
	QUARK_TEST_VERIFY(!is_start_char(']'));
	QUARK_TEST_VERIFY(!is_start_char('}'));
	QUARK_TEST_VERIFY(!is_start_char('>'));

	QUARK_TEST_VERIFY(!is_start_char(' '));
}


bool is_end_char(char c){
	return c == ')' || c == ']' || c == '}' || c == '>';
}

QUARK_UNIT_TEST("", "is_end_char()", "", ""){
	QUARK_TEST_VERIFY(!is_end_char('('));
	QUARK_TEST_VERIFY(!is_end_char('['));
	QUARK_TEST_VERIFY(!is_end_char('{'));
	QUARK_TEST_VERIFY(!is_end_char('<'));

	QUARK_TEST_VERIFY(is_end_char(')'));
	QUARK_TEST_VERIFY(is_end_char(']'));
	QUARK_TEST_VERIFY(is_end_char('}'));
	QUARK_TEST_VERIFY(is_end_char('>'));

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
	else if(start_char == '<'){
		return '>';
	}
	else {
		QUARK_ASSERT(false);
	}
}

QUARK_UNIT_TEST("", "start_char_to_end_char()", "", ""){
	QUARK_TEST_VERIFY(start_char_to_end_char('(') == ')');
	QUARK_TEST_VERIFY(start_char_to_end_char('[') == ']');
	QUARK_TEST_VERIFY(start_char_to_end_char('{') == '}');
	QUARK_TEST_VERIFY(start_char_to_end_char('<') == '>');
}

/*
	First char is the start char, like '(' or '{'.
*/

seq get_balanced(const string& s){
	QUARK_ASSERT(s.size() > 0);

	const char start_char = s[0];
	QUARK_ASSERT(is_start_char(start_char));

	const char end_char = start_char_to_end_char(start_char);

	int depth = 0;
	size_t pos = 0;
	while(pos < s.size() && !(depth == 1 && s[pos] == end_char)){
		const char c = s[pos];
		if(is_start_char(c)) {
			depth++;
		}
		else if(is_end_char(c)){
			if(depth == 0){
				throw std::runtime_error("unbalanced ([{< >}])");
			}
			depth--;
		}
		pos++;
	}

	return { s.substr(0, pos + 1), s.substr(pos + 1) };
}

QUARK_UNIT_TEST("", "get_balanced()", "", ""){
//	QUARK_TEST_VERIFY(get_balanced("") == seq("", ""));
	QUARK_TEST_VERIFY(get_balanced("()") == seq("()", ""));
	QUARK_TEST_VERIFY(get_balanced("(abc)") == seq("(abc)", ""));
	QUARK_TEST_VERIFY(get_balanced("(abc)def") == seq("(abc)", "def"));
	QUARK_TEST_VERIFY(get_balanced("((abc))def") == seq("((abc))", "def"));
	QUARK_TEST_VERIFY(get_balanced("((abc)[])def") == seq("((abc)[])", "def"));
}



//////////////////////////////////////		SYMBOLS



seq read_required_single_symbol(const string& s){
	const auto a = skip_whitespace(s);
	const auto b = read_while(a, identifier_chars);

	if(b.first.empty()){
		throw std::runtime_error("missing identifier");
	}
	return b;
}

QUARK_UNIT_TESTQ("read_required_single_symbol()", ""){
	QUARK_TEST_VERIFY(read_required_single_symbol("\thello\txxx") == seq("hello", "\txxx"));
}


//////////////////////////////////////		TYPE IDENTIFIERS



seq read_type(const string& s){
	const auto a = skip_whitespace(s);
	const auto b = read_while(a, type_chars);
	return b;
}

pair<type_identifier_t, string> read_required_type_identifier(const string& s){
	const seq type_pos = read_type(s);
	if(type_pos.first.empty()){
		throw std::runtime_error("illegal character in type identifier");
	}
	const auto type = type_identifier_t::make(type_pos.first);
	return { type, skip_whitespace(type_pos.second) };
}

	bool is_valid_type_identifier(const std::string& s){
		const auto a = read_while(s, floyd_parser::type_chars);
		return a.first == s;
	}



//////////////////////////////////////		FLOYD JSON BASICS




json_value_t value_to_json(const value_t& v){
	if(v.is_null()){
		return json_value_t();
	}
	else if(v.is_bool()){
		return json_value_t(v.get_bool());
	}
	else if(v.is_int()){
		return json_value_t(static_cast<double>(v.get_int()));
	}
	else if(v.is_float()){
		return json_value_t(static_cast<double>(v.get_float()));
	}
	else if(v.is_string()){
		return json_value_t(v.get_string());
	}
	else if(v.is_struct()){
		const auto value = v.get_struct();
		std::map<string, json_value_t> result;
		for(const auto member: value->__def->_members){
			const auto member_name = member._name;
			const auto member_value = value->_member_values[member_name];
			result[member_name] = value_to_json(member_value);
		}

		return result;
	}
	else if(v.is_vector()){
		const auto value = v.get_vector();
		std::vector<json_value_t> result;
		for(int i = 0 ; i < value->_elements.size() ; i++){
			const auto element_value = value->_elements[i];
			result.push_back(value_to_json(element_value));
		}

		return result;
	}
	else{
		QUARK_ASSERT(false);
	}
}

QUARK_UNIT_TESTQ("value_to_json()", "Nested struct to nested JSON objects"){
	struct_fixture_t f;
	const value_t value = f._struct6_instance0;

	const auto result = value_to_json(value);

	QUARK_UT_VERIFY(result.is_object());
	const auto obj = result.get_object();

	QUARK_UT_VERIFY(obj.at("_bool_true").is_true());
	QUARK_UT_VERIFY(obj.at("_bool_false").is_false());
	QUARK_UT_VERIFY(obj.at("_int").get_number() == 111.0);
	QUARK_UT_VERIFY(obj.at("_string").get_string() == "test 123");
	QUARK_UT_VERIFY(obj.at("_pixel").is_object());
	QUARK_UT_VERIFY(obj.at("_pixel").get_object().at("red").get_number() == 55.0);
	QUARK_UT_VERIFY(obj.at("_pixel").get_object().at("green").get_number() == 66.0);
}

QUARK_UNIT_TESTQ("value_to_json()", "Vector"){
	const auto vector_def = make_shared<const vector_def_t>(vector_def_t::make2(type_identifier_t::make("my_vec"), type_identifier_t::make_int()));
	const auto a = make_vector_instance(vector_def, { 10, 11, 12 });
	const auto b = make_vector_instance(vector_def, { 10, 4 });

	const auto result = value_to_json(a);

	QUARK_UT_VERIFY(result.is_array());
	const auto array = result.get_array();

	QUARK_UT_VERIFY(array[0] == 10);
	QUARK_UT_VERIFY(array[1] == 11);
	QUARK_UT_VERIFY(array[2] == 12);
}

QUARK_UNIT_TESTQ("value_to_json()", ""){
	quark::ut_compare(value_to_json(value_t("hello")), json_value_t("hello"));
}

QUARK_UNIT_TESTQ("value_to_json()", ""){
	quark::ut_compare(value_to_json(value_t(123)), json_value_t(123.0));
}

QUARK_UNIT_TESTQ("value_to_json()", ""){
	quark::ut_compare(value_to_json(value_t(true)), json_value_t(true));
}

QUARK_UNIT_TESTQ("value_to_json()", ""){
	quark::ut_compare(value_to_json(value_t(false)), json_value_t(false));
}

QUARK_UNIT_TESTQ("value_to_json()", ""){
	quark::ut_compare(value_to_json(value_t()), json_value_t());
}







}	//	floyd_parser
