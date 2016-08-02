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
#include "parser_value.h"

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


//////////////////////////////////////		symbol_path



symbol_path operator+(const symbol_path& a, const symbol_path& b){
	symbol_path result = a;
	result._entries.insert(result._entries.end(), b._entries.begin(), b._entries.end());
	return result;
}

bool operator==(const symbol_path& a, const symbol_path& b){
	return a._entries == b._entries;
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



pair<symbol_path, string> read_required_symbol_path(const string& s){
	const auto a = read_required_single_symbol(s);
	if(peek_compare_char(a.second, '.')){
		const auto b = read_required_symbol_path(a.second.substr(1));
		symbol_path path = symbol_path({a.first}) + b.first;
		return pair<symbol_path, string>(path, b.second);
	}
	else{
		return pair<vector<string>, string>({a.first}, a.second);
	}
}

QUARK_UNIT_TESTQ("read_required_single_symbol_path()", "hello"){
	QUARK_TEST_VERIFY((read_required_symbol_path("hello") == pair<symbol_path, string>(symbol_path{"hello"}, "")));
}

QUARK_UNIT_TESTQ("read_required_single_symbol_path()", "hello.kitty"){
	QUARK_TEST_VERIFY((read_required_symbol_path("hello.kitty") == pair<symbol_path, string>(symbol_path(vector<string>{"hello", "kitty"}), "")));
}

QUARK_UNIT_TESTQ("read_required_single_symbol_path()", "hello.kitty.blitz\txxx"){
	QUARK_TEST_VERIFY((read_required_symbol_path("hello.kitty.blitz\txxx") == pair<symbol_path, string>(symbol_path({"hello", "kitty", "blitz"}), "\txxx")));
}




//////////////////////////////////////		TYPE IDENTIFIERS



seq read_type(const string& s){
	const auto a = skip_whitespace(s);
	const auto b = read_while(a, type_chars);
	return b;
}

pair<type_identifier_t, string> read_required_type_identifier(const string& s){
	const seq type_pos = read_type(s);
	const auto type = make_type_identifier(type_pos.first);
	return { type, skip_whitespace(type_pos.second) };
}

/*
	Validates that this is a legal string, with legal characters. Exception.
	Does NOT make sure this a known type-identifier.
	String must not be empty.
*/
type_identifier_t make_type_identifier(const std::string& s){
	QUARK_ASSERT(!s.empty());

	//	Make sure string only contains valid characters.
	const auto a = read_while(s, identifier_chars);
	if(!a.second.empty()){
		throw std::runtime_error("illegal character in type identifier");
	}

	return type_identifier_t::make_type(s);

/*
	if(s == "int"){
		return type_identifier_t::make_type("int");
	}
	else if(s == "string"){
		return type_identifier_t::make_type("string");
	}
	else if(s == "float"){
		return type_identifier_t::make_type("float");
	}
	else if(s == "function"){
		return type_identifier_t::make_type("function");
	}
	else if(s == "value_type"){
		return type_identifier_t::make_type("value_type");
	}
	else{
		return type_identifier_t("");
	}
*/
}


	//////////////////////////////////////////////////		ast_t



	bool ast_t::parser_i__is_declared_function(const std::string& s) const{
//		return _functions.find(s) != _functions.end();
		return _types_collector.resolve_function_type(s) ? true : false;
	}

	bool ast_t::parser_i__is_declared_constant_value(const std::string& s) const{
		return _constant_values.find(s) != _constant_values.end();
	}

	bool ast_t::parser_i__is_known_type(const std::string& s) const{
		return true;
	}



}	//	floyd_parser
