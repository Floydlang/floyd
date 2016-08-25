//
//  magnus_parser_snippet.cpp
//  FloydSpeak
//
//  Created by Marcus Zetterquist on 24/08/16.
//  Copyright © 2016 Marcus Zetterquist. All rights reserved.
//

#include "quark.h"

#include <string>

#include "text_parser.h"

using namespace std;


pair<int, seq_t> evaluate_expression(const seq_t& p, int precedence);

seq_t skip_whitespace(const seq_t& p) {
	QUARK_ASSERT(p.check_invariant());

	return read_while(p, " \t\n\r").second;
}


pair<int, seq_t> parse_number(const seq_t& s) {
	QUARK_ASSERT(s.check_invariant());

	char* e = nullptr;
	long value = strtol(s.c_str(), &e, 0);
	size_t used = e - s.c_str();
	if (used == 0){
		throw std::runtime_error("Invalid number");
	}
	return { (int)value, s.rest(used) };
}

QUARK_UNIT_TESTQ("parse_number()", ""){
	QUARK_UT_VERIFY((parse_number(seq_t("0")) == pair<int, seq_t>{ 0, seq_t("") }));
}

QUARK_UNIT_TESTQ("parse_number()", ""){
	QUARK_UT_VERIFY((parse_number(seq_t("0xyz")) == pair<int, seq_t>{ 0, seq_t("xyz") }));
}

QUARK_UNIT_TESTQ("parse_number()", ""){
	QUARK_UT_VERIFY((parse_number(seq_t("13xyz")) == pair<int, seq_t>{ 13, seq_t("xyz") }));
}


pair<int, seq_t> evaluate_single(const seq_t& p) {
	QUARK_ASSERT(p.check_invariant());

	return parse_number(p);
}

pair<int, seq_t> evaluate_atom(const seq_t& p){
	QUARK_ASSERT(p.check_invariant());

    auto p2 = skip_whitespace(p);
	if(p2.empty()){
		throw std::runtime_error("Unexpected end of string");
	}

	char ch1 = p2.first_char();

	//	"-xxx"
	if(ch1 == '-'){
		const auto a = evaluate_expression(p2.rest(), 3);
		return { -a.first, skip_whitespace(a.second) };
	}
	//	"(yyy)xxx"
	else if(ch1 == '('){
		const auto a = evaluate_expression(p2.rest(), 0);
		if (a.second.first(1) != ")"){
			throw std::runtime_error("Expected ')'");
		}
		return { a.first, skip_whitespace(a.second.rest()) };
	}

	//	"1234xxx" or "my_function(3)xxx"
	else {
		const auto a = evaluate_single(p2);
		return { a.first, skip_whitespace(a.second) };
	}
}

pair<int, seq_t> evaluate_operation(const seq_t& p1, const int value, const int precedence){
	QUARK_ASSERT(p1.check_invariant());
	QUARK_ASSERT(precedence >= 0);

	auto p = p1;
	if(!p.empty()){
		char ch = p.first_char();
		if((ch == '+' || ch == '-') && precedence < 1){
			const auto a = evaluate_expression(p.rest(), 1);
			const auto value2 = ch == '+' ? value + a.first : value - a.first;
			const auto p2 = a.second;
			return p2.empty() ? pair<int, seq_t>{ value2, p2 } : evaluate_operation(p2, value2, precedence);
		}
		else if((ch == '*' || ch == '/') && precedence < 2) {
			const auto a = evaluate_expression(p.rest(), 2);
			const auto value2 = ch == '*' ? value * a.first : value / a.first;
			const auto p2 = a.second;
			return p2.empty() ? pair<int, seq_t>{ value2, p2 } : evaluate_operation(p2, value2, precedence);
		}
		else if(ch == ')'){
			return { value, p };
		}
		else{
			return { value, p };
		}
	}
	else{
		return { value, p };
	}
}

pair<int, seq_t> evaluate_expression(const seq_t& p, int precedence){
	QUARK_ASSERT(p.check_invariant());
	QUARK_ASSERT(precedence >= 0);

	auto a = evaluate_atom(p);
	return evaluate_operation(a.second, a.first, precedence);
}

pair<int, seq_t> evaluate_expression(const seq_t& p){
	QUARK_ASSERT(p.check_invariant());

	return evaluate_expression(p, 0);
}



QUARK_UNIT_TESTQ("evaluate_expression()", ""){
	try{
		evaluate_expression(seq_t(""));
		QUARK_UT_VERIFY(false);
	}
	catch(...){
	}
}

QUARK_UNIT_TESTQ("evaluate_expression()", ""){
	QUARK_UT_VERIFY((evaluate_expression(seq_t("0")) == pair<int, seq_t>{ 0, seq_t("") }));
}

QUARK_UNIT_TESTQ("evaluate_expression()", ""){
	QUARK_UT_VERIFY((evaluate_expression(seq_t("1234567890")) == pair<int, seq_t>{ 1234567890, seq_t("") }));
}

QUARK_UNIT_TESTQ("evaluate_expression()", ""){
	QUARK_UT_VERIFY((evaluate_expression(seq_t("10 + 4")) == pair<int, seq_t>{ 14, seq_t("") }));
}

QUARK_UNIT_TESTQ("evaluate_expression()", ""){
	QUARK_UT_VERIFY((evaluate_expression(seq_t("1 + 2 + 3 + 4")) == pair<int, seq_t>{ 10, seq_t("") }));
}

QUARK_UNIT_TESTQ("evaluate_expression()", ""){
	QUARK_UT_VERIFY((evaluate_expression(seq_t("1 + 8 + 7 + 2 * 3 + 4 * 5 + 6")) == pair<int, seq_t>{ 48, seq_t("") }));
}

QUARK_UNIT_TESTQ("evaluate_expression()", ""){
	QUARK_UT_VERIFY((evaluate_expression(seq_t("10 * 4")) == pair<int, seq_t>{ 40, seq_t("") }));
}

QUARK_UNIT_TESTQ("evaluate_expression()", ""){
	QUARK_UT_VERIFY((evaluate_expression(seq_t("(3)")) == pair<int, seq_t>{ 3, seq_t("") }));
}

QUARK_UNIT_TESTQ("evaluate_expression()", ""){
	QUARK_UT_VERIFY((evaluate_expression(seq_t("(3 * 8)")) == pair<int, seq_t>{ 24, seq_t("") }));
}

QUARK_UNIT_TESTQ("evaluate_expression()", ""){
	QUARK_UT_VERIFY((evaluate_expression(seq_t("1 + 3 * 2 + 100")) == pair<int, seq_t>{ 107, seq_t("") }));
}

QUARK_UNIT_TESTQ("evaluate_expression()", ""){
	QUARK_UT_VERIFY((evaluate_expression(seq_t("-(3 * 2 + (8 * 2)) - (((1))) * 2")) == pair<int, seq_t>{ -(3 * 2 + (8 * 2)) - (((1))) * 2, seq_t("") }));
}

