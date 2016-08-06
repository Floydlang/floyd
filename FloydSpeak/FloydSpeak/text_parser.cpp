//
//  text_parser.cpp
//  FloydSpeak
//
//  Created by Marcus Zetterquist on 26/07/16.
//  Copyright © 2016 Marcus Zetterquist. All rights reserved.
//

#include "text_parser.h"

#include "quark.h"
#include <string>
#include <memory>
#include <map>
#include <iostream>



using std::vector;
using std::string;
using std::pair;
using std::shared_ptr;
using std::unique_ptr;
using std::make_shared;



const std::string test_whitespace_chars = " \n\t";





///////////////////////////////		seq2




seq2::seq2(const std::string& s) :
	_str(make_shared<string>(s)),
	_rest_pos(0)
{
	QUARK_ASSERT(check_invariant());
}

seq2::seq2(const std::shared_ptr<const std::string>& str, std::size_t rest_pos) :
	_str(str),
	_rest_pos(rest_pos)
{
	QUARK_ASSERT(check_invariant());
}

bool seq2::check_invariant() const {
	QUARK_ASSERT(_str);
	QUARK_ASSERT(_rest_pos <= _str->size());
	return true;
}


char seq2::first() const{
	QUARK_ASSERT(check_invariant());
	QUARK_ASSERT(_rest_pos <= _str->size());

	return (*_str)[_rest_pos];
}

const seq2 seq2::rest() const{
	QUARK_ASSERT(check_invariant());

	return seq2(_str, _rest_pos + 1);
}

std::size_t seq2::rest_size() const{
	QUARK_ASSERT(check_invariant());

	return _str->size() - _rest_pos;
}



QUARK_UNIT_TESTQ("seq2::seq2()", ""){
	QUARK_TEST_VERIFY(seq2("").rest_size() == 0);
}

QUARK_UNIT_TESTQ("seq2::seq2()", ""){
	QUARK_TEST_VERIFY(seq2("xyz").rest_size() == 3);
}

QUARK_UNIT_TESTQ("seq2::first()", ""){
	QUARK_TEST_VERIFY(seq2("xyz").first() == 'x');
}

QUARK_UNIT_TESTQ("seq2::first()", ""){
	QUARK_TEST_VERIFY(seq2("xyz").rest().first() == 'y');
}




	std::string remove_trailing_comma(const std::string& a){
		auto s = a;
		if(s.size() > 1 && s.back() == ','){
			s.pop_back();
		}
		return s;
	}



seq read_while(const string& s, const string& match){
	size_t pos = 0;
	while(pos < s.size() && match.find(s[pos]) != string::npos){
		pos++;
	}

	return seq(
		s.substr(0, pos),
		s.substr(pos)
	);
}

QUARK_UNIT_TEST("", "read_while()", "", ""){
	QUARK_TEST_VERIFY(read_while("", test_whitespace_chars) == seq("", ""));
	QUARK_TEST_VERIFY(read_while(" ", test_whitespace_chars) == seq(" ", ""));
	QUARK_TEST_VERIFY(read_while("    ", test_whitespace_chars) == seq("    ", ""));

	QUARK_TEST_VERIFY(read_while("while", test_whitespace_chars) == seq("", "while"));
	QUARK_TEST_VERIFY(read_while(" while", test_whitespace_chars) == seq(" ", "while"));
	QUARK_TEST_VERIFY(read_while("    while", test_whitespace_chars) == seq("    ", "while"));
}

seq read_until(const string& s, const string& match){
	size_t pos = 0;
	while(pos < s.size() && match.find(s[pos]) == string::npos){
		pos++;
	}

	return { s.substr(0, pos), s.substr(pos) };
}

QUARK_UNIT_TEST("", "read_until()", "", ""){
	QUARK_TEST_VERIFY(read_until("", ",.") == seq("", ""));
	QUARK_TEST_VERIFY(read_until("ab", ",.") == seq("ab", ""));
	QUARK_TEST_VERIFY(read_until("ab,cd", ",.") == seq("ab", ",cd"));
	QUARK_TEST_VERIFY(read_until("ab.cd", ",.") == seq("ab", ".cd"));
}





pair<char, string> read_char(const std::string& s){
	if(!s.empty()){
		return { s[0], s.substr(1) };
	}
	else{
		throw std::runtime_error("expected character.");
	}
}

std::string read_required_char(const std::string& s, char ch){
	if(s.size() > 0 && s[0] == ch){
		return s.substr(1);
	}
	else{
		throw std::runtime_error("expected character '" + string(1, ch)  + "'.");
	}
}

pair<bool, std::string> read_optional_char(const std::string& s, char ch){
	if(s.size() > 0 && s[0] == ch){
		return { true, s.substr(1) };
	}
	else{
		return { false, s };
	}
}


bool peek_compare_char(const std::string& s, char ch){
	return s.size() > 0 && s[0] == ch;
}

bool peek_string(const std::string& s, const std::string& peek){
	return s.size() >= peek.size() && s.substr(0, peek.size()) == peek;
}

QUARK_UNIT_TEST("", "peek_string()", "", ""){
	QUARK_TEST_VERIFY(peek_string("", "") == true);
	QUARK_TEST_VERIFY(peek_string("a", "a") == true);
	QUARK_TEST_VERIFY(peek_string("a", "b") == false);
	QUARK_TEST_VERIFY(peek_string("", "b") == false);
	QUARK_TEST_VERIFY(peek_string("abc", "abc") == true);
	QUARK_TEST_VERIFY(peek_string("abc", "abx") == false);
	QUARK_TEST_VERIFY(peek_string("abc", "ab") == true);
}



string trim_ends(const string& s){
	QUARK_ASSERT(s.size() >= 2);

	return s.substr(1, s.size() - 2);
}
