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


seq_t skipWhite(const seq_t& p1) {
	return read_while(p1, " \t\n\r").second;
}


pair<int, seq_t> toNumber(const seq_t& s) {
	char* e = nullptr;
	long value = strtol(s.c_str(), &e, 0);
	size_t used = e - s.c_str();
	if (used == 0){
		throw std::runtime_error("Invalid number");
	}
	return { (int)value, s.rest(used) };
}

QUARK_UNIT_TESTQ("toNumber()", ""){
	QUARK_UT_VERIFY((toNumber(seq_t("0")) == pair<int, seq_t>{ 0, seq_t("") }));
}

QUARK_UNIT_TESTQ("toNumber()", ""){
	QUARK_UT_VERIFY((toNumber(seq_t("0xyz")) == pair<int, seq_t>{ 0, seq_t("xyz") }));
}

QUARK_UNIT_TESTQ("toNumber()", ""){
	QUARK_UT_VERIFY((toNumber(seq_t("13xyz")) == pair<int, seq_t>{ 13, seq_t("xyz") }));
}


pair<int, seq_t> evaluate_single(const seq_t& s) {
	return toNumber(s);
}



pair<int, seq_t> evaluate(const seq_t& p1, int precedence = 0);



pair<int, seq_t> evaluate_atom(const seq_t& p1){
    auto p = skipWhite(p1);
	if(p.empty()){
		throw std::runtime_error("Unexpected end of string");
	}

	int value = 0;
	char ch1 = p.first_char();
    switch (ch1) {
		//	"-xxx"
        case '-':
			{
				const auto a = evaluate(p.rest(), 3);
				value = -a.first;
				p = a.second;
			}
			break;

		//	"(yyy)xxx"
        case '(':
			{
				const auto a = evaluate(p.rest(), 0);
				value = a.first;
				p = a.second;

				if (a.second.first(1) != ")"){
					throw std::runtime_error("Expected ')'");
				}
				p = p.rest();
			}
			break;

		//	"1234xxx" or "my_function(3)xxx"
        default:
			{
				const auto a = evaluate_single(p);
				value = a.first;
				p = a.second;
			}
			break;
    }
    p = skipWhite(p);
	return { value, p };
}

pair<int, seq_t> evaluate(const seq_t& p1, int precedence){
	auto b = evaluate_atom(p1);
	int value = b.first;
	auto p = b.second;

	if(!p.empty()){
		bool loop = false;
		do {
			loop = true;
			if(p.empty()){
				loop = false;
			}
			else{
				char ch = p.first_char();
				if((ch == '+' || ch == '-') && precedence < 1){
					const auto a = evaluate(p.rest(), 1);
					value = ch == '+' ? value + a.first : value - a.first;
					p = a.second;
				}
				else if((ch == '*' || ch == '/') && precedence < 2) {
					const auto a = evaluate(p.rest(), 2);
					value = ch == '*' ? value * a.first : value / a.first;
					p = a.second;
				}
				else if(ch == ')'){
					loop = false;
				}
				else{
					loop = false;
				}
			}
		} while (loop);
		p = skipWhite(p);
	}
	return { value, p };
}

pair<int, seq_t> evaluate_expression(const seq_t& p1){
	return evaluate(p1, 0);
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

