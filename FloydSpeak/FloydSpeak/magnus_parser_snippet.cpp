//
//  magnus_parser_snippet.cpp
//  FloydSpeak
//
//  Created by Marcus Zetterquist on 24/08/16.
//  Copyright © 2016 Marcus Zetterquist. All rights reserved.
//

#include "quark.h"

#include <string>

using namespace std;



string skipWhite(const string& p) {
	const auto count = strspn(p.c_str(), " \t\n\r");
	return p.substr(count);
}


pair<int, string> toNumber(const string& s) {
	char* e = nullptr;
	long value = strtol(s.c_str(), &e, 0);
	size_t used = e - s.c_str();
	if (used == 0){
		throw std::runtime_error("Invalid number");
	}
	return { (int)value, s.substr(used) };
}

QUARK_UNIT_TESTQ("toNumber()", ""){
	QUARK_UT_VERIFY((toNumber("0") == pair<int, string>{ 0, "" }));
}

QUARK_UNIT_TESTQ("toNumber()", ""){
	QUARK_UT_VERIFY((toNumber("0xyz") == pair<int, string>{ 0, "xyz" }));
}

QUARK_UNIT_TESTQ("toNumber()", ""){
	QUARK_UT_VERIFY((toNumber("13xyz") == pair<int, string>{ 13, "xyz" }));
}



pair<int, string> evaluate(const string& p1, int precedence = 0){
    auto p = skipWhite(p1);

	int value = 0;
    switch (p[0]) {
        case '\0':
			throw std::runtime_error("Unexpected end of string");

		//	"-xxx"
        case '-':
			{
				p = p.substr(1);
				const auto a = evaluate(p, 3);
				value = -a.first;
				p = a.second;
			}
			break;

		//	"(yyy)xxx"
        case '(':
			{
				p = p.substr(1);
				const auto a = evaluate(p, 0);
				value = a.first;
				p = a.second;

				if (a.second[0] != ')'){
					throw std::runtime_error("Expected ')'");
				}
				p = p.substr(1);
			}
			break;

		//	"1234xxx"
        default:
			{
				const auto a = toNumber(p);
				value = a.first;
				p = a.second;
			}
			break;
    }
    p = skipWhite(p);

	if(!p.empty()){
		bool loop = false;
		do {
			loop = true;
			char ch = p[0];
			if((ch == '+' || ch == '-') && precedence < 1){
				const auto a = evaluate(p.substr(1), 1);
				value = ch == '+' ? value + a.first : value - a.first;
				p = a.second;
			}
			else if((ch == '*' || ch == '/') && precedence < 2) {
				const auto a = evaluate(p.substr(1), 2);
				value = ch == '*' ? value * a.first : value / a.first;
				p = a.second;
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
		p = skipWhite(p);
	}
	return { value, p };
}

QUARK_UNIT_TESTQ("evaluate()", ""){
	try{
		evaluate("");
		QUARK_UT_VERIFY(false);
	}
	catch(...){
	}
}

QUARK_UNIT_TESTQ("evaluate()", ""){
	QUARK_UT_VERIFY((evaluate("0") == pair<int, string>{ 0, "" }));
}

QUARK_UNIT_TESTQ("evaluate()", ""){
	QUARK_UT_VERIFY((evaluate("1234567890") == pair<int, string>{ 1234567890, "" }));
}

QUARK_UNIT_TESTQ("evaluate()", ""){
	QUARK_UT_VERIFY((evaluate("10 + 4") == pair<int, string>{ 14, ""}));
}

QUARK_UNIT_TESTQ("evaluate()", ""){
	QUARK_UT_VERIFY((evaluate("10 * 4") == pair<int, string>{ 40, ""}));
}

QUARK_UNIT_TESTQ("evaluate()", ""){
	QUARK_UT_VERIFY((evaluate("(3)") == pair<int, string>{ 3, ""}));
}

QUARK_UNIT_TESTQ("evaluate()", ""){
	QUARK_UT_VERIFY((evaluate("(3 * 8)") == pair<int, string>{ 24, ""}));
}

QUARK_UNIT_TESTQ("evaluate()", ""){
	QUARK_UT_VERIFY((evaluate("1 + 3 * 2 + 100") == pair<int, string>{ 107, ""}));
}

QUARK_UNIT_TESTQ("evaluate()", ""){
	QUARK_UT_VERIFY((evaluate("-(3 * 2 + (8 * 2)) - (((1))) * 2") == pair<int, string>{ -(3 * 2 + (8 * 2)) - (((1))) * 2, ""}));
}

