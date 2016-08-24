//
//  magnus_parser_snippet.cpp
//  FloydSpeak
//
//  Created by Marcus Zetterquist on 24/08/16.
//  Copyright © 2016 Marcus Zetterquist. All rights reserved.
//


#include "quark.h"




class ConstStringException : public std::exception {
    public: ConstStringException(const char* s) : _s(s) { };
    public: virtual const char *what() const throw() { return _s; }
    protected: const char* _s;
};

void skipWhite(const char*& p) { p += strspn(p, " \t\n\r"); }
void toNumber(const char*& p, double& d) { char* e; d = strtod(p, &e); if (p == e) throw ConstStringException("Invalid number"); p = e; }
void toNumber(const char*& p, long& l) { char* e; l = strtol(p, &e, 0); if (p == e) throw ConstStringException("Invalid number"); p = e; }
void skipWhite(const wchar_t*& p) { p += wcsspn(p, L" \t\n\r"); }
void toNumber(const wchar_t*& p, double& d) { wchar_t* e; d = wcstod(p, &e); if (p == e) throw ConstStringException("Invalid number"); p = e; }
void toNumber(const wchar_t*& p, long& l) { wchar_t* e; l = wcstol(p, &e, 0); if (p == e) throw ConstStringException("Invalid number"); p = e; }
template<class C> void toNumber(C*& p, float& f) { double d; toNumber(p, d); f = static_cast<float>(d); }
template<class C, class T> void toNumber(C*& p, T& i) { long l; toNumber(p, l); i = static_cast<int>(l); }

template<class V, class C> void evaluate(const C*& p, V& v, int precedence = 0){
    skipWhite(p);
    switch (*p) {
        case '\0':
			throw ConstStringException("Unexpected end of string");

        case '-':
			evaluate(++p, v, 3);
			v = -v;
			break;

        case '(':
			evaluate(++p, v, 0);
			if (*p != ')'){
				throw ConstStringException("Expected ')'");
			}
			++p;
			break;

        default:
			toNumber(p, v);
			break;
    }
    skipWhite(p);
    bool loop;
    do {
        loop = false;
        switch (*p) {
            case '+':
				if (precedence < 1) {
					V r;
					evaluate(++p, r, 1);
					v += r;
					loop = true;
				}
				break;
            case '-':
				if (precedence < 1) {
					V r;
					evaluate(++p, r, 1);
					v -= r;
					loop = true;
				}
				break;
            case '*':
				if (precedence < 2) {
					V r;
					evaluate(++p, r, 2);
					v *= r;
					loop = true;
				}
				break;
            case '/':
				if (precedence < 2) {
					V r;
					evaluate(++p, r, 2);
					v /= r;
					loop = true;
				}
				break;
			case '\0':
				break;

			case ')':
				break;

			default:
				QUARK_ASSERT(false);
				break;
        }
    } while (loop);
    skipWhite(p);
}

int evaluate2(const std::string& s){
	const char* p = s.c_str();
	int value = 0;
	evaluate<int, char>(p, value, 0);
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


