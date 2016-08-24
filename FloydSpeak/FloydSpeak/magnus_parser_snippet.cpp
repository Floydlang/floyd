//
//  magnus_parser_snippet.cpp
//  FloydSpeak
//
//  Created by Marcus Zetterquist on 24/08/16.
//  Copyright © 2016 Marcus Zetterquist. All rights reserved.
//

#include "quark.h"


void skipWhite(const char*& p) {
	p += strspn(p, " \t\n\r");
}

void toNumber(const char*& p, double& d) {
	char* e;
	d = strtod(p, &e);
	if (p == e){
		throw std::runtime_error("Invalid number");
	}
	p = e;
}

void toNumber(const char*& p, long& l) {
	char* e;
	l = strtol(p, &e, 0);
	if (p == e){
		throw std::runtime_error("Invalid number");
	}
	p = e;
}

void skipWhite(const wchar_t*& p) {
	p += wcsspn(p, L" \t\n\r");
}

void toNumber(const wchar_t*& p, double& d) {
	wchar_t* e;
	d = wcstod(p, &e);
	if (p == e){
		throw std::runtime_error("Invalid number");
	}
	p = e;
}

void toNumber(const wchar_t*& p, long& l) {
	wchar_t* e;
	l = wcstol(p, &e, 0);
	if (p == e){
		throw std::runtime_error("Invalid number");
	}
	p = e;
}

template<class C> void toNumber(C*& p, float& f) {
	double d;
	toNumber(p, d);
	f = static_cast<float>(d);
}

template<class C, class T> void toNumber(C*& p, T& i) {
	long l;
	toNumber(p, l);
	i = static_cast<int>(l);
}



void evaluate(const char*& p, int& v, int precedence = 0){
    skipWhite(p);
    switch (*p) {
        case '\0':
			throw std::runtime_error("Unexpected end of string");

		//	"-xxx"
        case '-':
			evaluate(++p, v, 3);
			v = -v;
			break;

		//	"(yyy)xxx"
        case '(':
			evaluate(++p, v, 0);
			if (*p != ')'){
				throw std::runtime_error("Expected ')'");
			}
			++p;
			break;

		//	"1234xxx"
        default:
			toNumber(p, v);
			break;
    }
    skipWhite(p);

    bool loop = false;
    do {
        loop = true;
		char ch = *p;

		if((ch == '+' || ch == '-') && precedence < 1){
			int r = 0;
			evaluate(++p, r, 1);
			v = ch == '+' ? v + r : v - r;
		}
		else if((ch == '*' || ch == '/') && precedence < 2) {
			int r = 0;
			evaluate(++p, r, 2);
			v = ch == '*' ? v * r : v / r;
		}

		else if(ch == '\0'){
			loop = false;
		}
		else if(ch == ')'){
			loop = false;
		}
		else{
			loop = false;
		}
    } while (loop);
    skipWhite(p);
}

int evaluate2(const std::string& s){
	const char* p = s.c_str();
	int value = 0;
	evaluate(p, value, 0);
	return value;
}

QUARK_UNIT_TESTQ("evaluate()", ""){
	try{
		evaluate2("");
		QUARK_UT_VERIFY(false);
	}
	catch(...){
	}
}

QUARK_UNIT_TESTQ("evaluate()", ""){
	QUARK_UT_VERIFY(evaluate2("0") == 0);
}

QUARK_UNIT_TESTQ("evaluate()", ""){
	QUARK_UT_VERIFY(evaluate2("1234567890") == 1234567890);
}

QUARK_UNIT_TESTQ("evaluate()", ""){
	QUARK_UT_VERIFY(evaluate2("10 + 4") == 14);
}

QUARK_UNIT_TESTQ("evaluate()", ""){
	QUARK_UT_VERIFY(evaluate2("10 * 4") == 40);
}

QUARK_UNIT_TESTQ("evaluate()", ""){
	QUARK_UT_VERIFY(evaluate2("(3)") == 3);
}

QUARK_UNIT_TESTQ("evaluate()", ""){
	QUARK_UT_VERIFY(evaluate2("(3 * 8)") == 24);
}

QUARK_UNIT_TESTQ("evaluate()", ""){
	QUARK_UT_VERIFY(evaluate2("1 + 3 * 2 + 100") == 107);
}

QUARK_UNIT_TESTQ("evaluate()", ""){
	QUARK_UT_VERIFY(evaluate2("-(3 * 2 + (8 * 2)) - (((1))) * 2") == -(3 * 2 + (8 * 2)) - (((1))) * 2);
}

