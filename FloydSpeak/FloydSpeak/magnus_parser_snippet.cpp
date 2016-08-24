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

template<class V, class C> void evaluate(const C*& p, V& v, int precedence = 0)
{
    skipWhite(p);
    switch (*p) {
        case 0: throw ConstStringException("Unexpected end of string");
        case '-': evaluate(++p, v, 3); v = -v; break;
        case '(': evaluate(++p, v, 0); if (*p != ')') throw ConstStringException("Expected ')'"); ++p; break;
        default: toNumber(p, v); break;
    }
    skipWhite(p);
    bool loop;
    do {
        loop = false;
        switch (*p) {
            case '+': if (precedence < 1) { V r; evaluate(++p, r, 1); v += r; loop = true; } break;
            case '-': if (precedence < 1) { V r; evaluate(++p, r, 1); v -= r; loop = true; } break;
            case '*': if (precedence < 2) { V r; evaluate(++p, r, 2); v *= r; loop = true; } break;
            case '/': if (precedence < 2) { V r; evaluate(++p, r, 2); v /= r; loop = true; } break;
        }
    } while (loop);
    skipWhite(p);
}



QUARK_UNIT_TESTQ("evaluate()", ""){
	char a[] = "10 + 4";

	const char* p = a;
	int value = 0;
	evaluate<int, char>(p, value, 0);

	QUARK_UT_VERIFY(value == 14);
//	quark::ut_compare(expression_to_json_string(parse_expression("pixel.red")), R"(["load", "<>", ["res_member", "<>", ["res_var", "<>", "pixel"], "red"]])");
}



