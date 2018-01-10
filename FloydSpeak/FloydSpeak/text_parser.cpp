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
#include <cstring>
#include <memory>
#include <map>
#include <iostream>
#include <cmath>

using std::vector;
using std::string;
using std::pair;
using std::shared_ptr;
using std::unique_ptr;
using std::make_shared;



///////////////////////////////		String utils



string trim_ends(const string& s){
	QUARK_ASSERT(s.size() >= 2);

	return s.substr(1, s.size() - 2);
}

std::string quote(const std::string& s){
	return std::string("\"") + s + "\"";
}

QUARK_UNIT_TESTQ("quote()", ""){
	QUARK_UT_VERIFY(quote("") == "\"\"");
}

QUARK_UNIT_TESTQ("quote()", ""){
	QUARK_UT_VERIFY(quote("abc") == "\"abc\"");
}



float parse_float(const std::string& pos){
	size_t end = -1;
	auto res = std::stof(pos, &end);
	if(isnan(res) || end == 0){
		throw std::runtime_error("EEE_WRONG_CHAR");
	}
	return res;
}

std::string float_to_string(float value){
	std::stringstream s;
	s << value;
	const auto result = s.str();
	return result;
}

QUARK_UNIT_TESTQ("float_to_string()", ""){
	quark::ut_compare(float_to_string(0.0f), "0");
}
QUARK_UNIT_TESTQ("float_to_string()", ""){
	quark::ut_compare(float_to_string(13.0f), "13");
}
QUARK_UNIT_TESTQ("float_to_string()", ""){
	quark::ut_compare(float_to_string(13.5f), "13.5");
}



std::string double_to_string(double value){
	std::stringstream s;
	s << value;
	const auto result = s.str();
	return result;
}

QUARK_UNIT_TESTQ("double_to_string()", ""){
	quark::ut_compare(float_to_string(0.0), "0");
}
QUARK_UNIT_TESTQ("double_to_string()", ""){
	quark::ut_compare(float_to_string(13.0), "13");
}
QUARK_UNIT_TESTQ("double_to_string()", ""){
	quark::ut_compare(float_to_string(13.5), "13.5");
}





///////////////////////////////		seq_t




seq_t::seq_t(const std::string& s) :
	_str(make_shared<string>(s)),
	_pos(0)
{
	FIRST_debug = _str->c_str() + _pos + 0;

	QUARK_ASSERT(check_invariant());
}

seq_t::seq_t(const std::shared_ptr<const std::string>& str, std::size_t pos) :
	_str(str),
	_pos(pos)
{
	QUARK_ASSERT(str);
	QUARK_ASSERT(pos <= str->size());

	FIRST_debug = _str->c_str() + _pos + 0;

	QUARK_ASSERT(check_invariant());
}

bool seq_t::operator==(const seq_t& other) const {
	QUARK_ASSERT(check_invariant());
	QUARK_ASSERT(other.check_invariant());

	return first(1) == other.first(1) && get_s() == other.get_s();
}

bool seq_t::check_invariant() const {
	QUARK_ASSERT(_str);
	QUARK_ASSERT(_pos <= _str->size());
	return true;
}


char seq_t::first1_char() const{
	QUARK_ASSERT(check_invariant());

	if(_pos >= _str->size()){
		throw std::runtime_error("");
	}

	return (*_str)[_pos];
}

std::string seq_t::first1() const{
	return first(1);
}

std::string seq_t::first(size_t chars) const{
	QUARK_ASSERT(check_invariant());

	return _pos < _str->size() ? _str->substr(_pos, chars) : string();
}

seq_t seq_t::rest1() const{
	QUARK_ASSERT(check_invariant());

	return rest(1);
}

seq_t seq_t::rest(size_t skip) const{
	QUARK_ASSERT(check_invariant());

	const auto p = std::min(_str->size(), _pos + skip);
	return seq_t(_str, p);
}

std::string seq_t::get_s() const{
	QUARK_ASSERT(check_invariant());

	return _str->substr(_pos);
}

std::size_t seq_t::size() const{
	QUARK_ASSERT(check_invariant());

	return empty() ? 0 : _str->size() - _pos;
}


bool seq_t::empty() const{
	QUARK_ASSERT(check_invariant());

	return _str->size() == _pos;
}

const char* seq_t::c_str() const{
	QUARK_ASSERT(check_invariant());

	return empty() ? nullptr : _str->c_str() + _pos;
}



QUARK_UNIT_TESTQ("seq_t()", ""){
	seq_t("");
}
QUARK_UNIT_TESTQ("seq_t()", ""){
	seq_t("hello, world!");
}


QUARK_UNIT_TESTQ("first_char()", ""){
	QUARK_TEST_VERIFY(seq_t("a").first1_char() == 'a');
}
QUARK_UNIT_TESTQ("first_char()", ""){
	QUARK_TEST_VERIFY(seq_t("abcd").first1_char() == 'a');
}


QUARK_UNIT_TESTQ("first()", ""){
	QUARK_TEST_VERIFY(seq_t("").first1() == "");
}
QUARK_UNIT_TESTQ("first()", ""){
	QUARK_TEST_VERIFY(seq_t("a").first1() == "a");
}
QUARK_UNIT_TESTQ("first()", ""){
	QUARK_TEST_VERIFY(seq_t("abc").first1() == "a");
}


QUARK_UNIT_TESTQ("first(n)", ""){
	QUARK_TEST_VERIFY(seq_t("abc").first(0) == "");
}
QUARK_UNIT_TESTQ("first(n)", ""){
	QUARK_TEST_VERIFY(seq_t("").first(0) == "");
}
QUARK_UNIT_TESTQ("first(n)", ""){
	QUARK_TEST_VERIFY(seq_t("").first(3) == "");
}
QUARK_UNIT_TESTQ("first(n)", ""){
	QUARK_TEST_VERIFY(seq_t("abc").first(1) == "a");
}
QUARK_UNIT_TESTQ("first(n)", ""){
	QUARK_TEST_VERIFY(seq_t("abc").first(3) == "abc");
}


QUARK_UNIT_TESTQ("rest()", ""){
	QUARK_TEST_VERIFY(seq_t("abc").rest1().first1() == "b");
}
QUARK_UNIT_TESTQ("rest()", ""){
	QUARK_TEST_VERIFY(seq_t("").rest1().first1() == "");
}


QUARK_UNIT_TESTQ("rest(n)", ""){
	QUARK_TEST_VERIFY(seq_t("abc").rest(2).first1() == "c");
}
QUARK_UNIT_TESTQ("rest(n)", ""){
	QUARK_TEST_VERIFY(seq_t("").rest1().first1() == "");
}
QUARK_UNIT_TESTQ("rest(n)", ""){
	QUARK_TEST_VERIFY(seq_t("abc").rest(100).first(100) == "");
}







pair<string, seq_t> read_while(const seq_t& p1, const string& chars){
	string a;
	seq_t p2 = p1;

	while(!p2.empty() && chars.find(p2.first1_char()) != string::npos){
		a = a + p2.first1_char();
		p2 = p2.rest1();
	}

	return { a, p2 };
}

QUARK_UNIT_TEST("", "read_while()", "", ""){
	QUARK_TEST_VERIFY((read_while(seq_t(""), test_whitespace_chars) == pair<string, seq_t>{ "", seq_t("") }));
}

QUARK_UNIT_TEST("", "read_while()", "", ""){
	QUARK_TEST_VERIFY((read_while(seq_t("\t"), test_whitespace_chars) == pair<string, seq_t>{ "\t", seq_t("") }));
}

QUARK_UNIT_TEST("", "read_while()", "", ""){
	QUARK_TEST_VERIFY((read_while(seq_t("end\t"), test_whitespace_chars) == pair<string, seq_t>{ "", seq_t("end\t") }));
}

QUARK_UNIT_TEST("", "read_while()", "", ""){
	QUARK_TEST_VERIFY((read_while(seq_t("\nend"), test_whitespace_chars) == pair<string, seq_t>{ "\n", seq_t("end") }));
}

QUARK_UNIT_TEST("", "read_while()", "", ""){
	QUARK_TEST_VERIFY((read_while(seq_t("\n\t\rend"), test_whitespace_chars) == pair<string, seq_t>{ "\n\t\r", seq_t("end") }));
}





pair<string, seq_t> read_until(const seq_t& p1, const string& chars){
	string a;
	seq_t p2 = p1;

	while(!p2.empty() && chars.find(p2.first1_char()) == string::npos){
		a = a + p2.first1_char();
		p2 = p2.rest1();
	}

	return { a, p2 };
}

pair<string, seq_t> split_at(const seq_t& p1, const string& str){
	const auto p = std::strstr(p1.c_str(), str.c_str());
	if(p == nullptr){
		return { "", p1 };
	}
	else{
		const auto pos = p - p1.c_str();
		return { p1.first(pos), p1.rest(pos + str.size())};
	}
}

QUARK_UNIT_TEST("", "split_at()", "", ""){
	QUARK_TEST_VERIFY((split_at(seq_t("hello123world"), "123") == pair<string, seq_t>{ "hello", seq_t("world") }));
}
QUARK_UNIT_TEST("", "split_at()", "", ""){
	QUARK_TEST_VERIFY((split_at(seq_t("hello123world"), "456") == pair<string, seq_t>{ "", seq_t("hello123world") }));
}


std::pair<bool, seq_t> if_first(const seq_t& p, const std::string& wanted_string){
	const auto size = wanted_string.size();
	if(p.first(size) == wanted_string){
		return { true, p.rest(size) };
	}
	else{
		return { false, p };
	}
}

QUARK_UNIT_TESTQ("if_first()", ""){
	const auto result = if_first(seq_t("hello, world!"), "hell");
	const auto expected = std::pair<bool, seq_t>(true, seq_t("o, world!"));

	QUARK_TEST_VERIFY(result == expected);
}

bool is_first(const seq_t& p, const std::string& wanted_string){
	return if_first(p, wanted_string).first;
}




pair<char, seq_t> read_char(const seq_t& s){
	if(!s.empty()){
		return { s.first1_char(), s.rest1() };
	}
	else{
		throw std::runtime_error("expected character.");
	}
}

seq_t read_required_char(const seq_t& s, char ch){
	return read_required(s, string(1, ch));
}

seq_t read_required(const seq_t& s, const std::string& req){
	const auto count = req.size();
	const auto peek = s.first(count);
	if(peek != req){
		throw std::runtime_error("expected '" + req  + "'.");
	}
	return s.rest(count);
}

pair<bool, seq_t> read_optional_char(const seq_t& s, char ch){
	if(s.size() > 0 && s.first1_char() == ch){
		return { true, s.rest1() };
	}
	else{
		return { false, s };
	}
}

