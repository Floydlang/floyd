//
//  magnus_parser_snippet.cpp
//  FloydSpeak
//
//  Created by Marcus Zetterquist on 24/08/16.
//  Copyright © 2016 Marcus Zetterquist. All rights reserved.
//

#include "quark.h"

#include <string>
#include <memory>

#include "text_parser.h"

using namespace std;


/*
	http://en.cppreference.com/w/cpp/language/operator_precedence
*/

#if 0
	//	a::b
	k_scope_resolution = 1,

	//	a()
	k_function_call = 2,

	//	a[]
	k_looup = 2,

	// a.b
	k_member_access = 2,

	//	!a
	k_logical_not = 3,

	//	~a
	k_bitwize_not = 3,
#endif


enum class eoperator_precedence {
	k_super_strong = 0,

	//	(xyz)
	k_parentesis,

	k_multiply_divider_remainder = 5,

	k_add_sub = 6,

	k_equal__not_equal = 9,

	k_logical_and = 13,
	k_logical_or = 14,

	k_comparison_operator = 15,

	k_super_weak
};

QUARK_UNIT_TESTQ("enum class()", ""){
	enum class my_enum {
		k_one = 1,
		k_four = 4
	};

	QUARK_UT_VERIFY(my_enum::k_one == my_enum::k_one);
	QUARK_UT_VERIFY(my_enum::k_one != my_enum::k_four);
	QUARK_UT_VERIFY(static_cast<int>(my_enum::k_one) == 1);
}


struct node_t {
	public: static shared_ptr<node_t> make_terminal(const std::string& op, const std::string& terminal){
		return make_shared<node_t>(node_t{ op, terminal, {} });
	};

	public: static shared_ptr<node_t> make_op(const std::string& op, const vector<shared_ptr<const node_t>>& expressions){
		return make_shared<node_t>(node_t{ op, string(""), expressions });
	};

	private: node_t(const string& op, const string& terminal, const vector<shared_ptr<const node_t>>& expressions) :
		_op(op),
		_terminal(terminal),
		_expressions(expressions)
	{
	}

	private: string _op;
	private: string _terminal;
	private: vector<shared_ptr<const node_t>> _expressions;
};




pair<int, seq_t> evaluate_expression(const seq_t& p, const eoperator_precedence precedence);

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

    const auto p2 = skip_whitespace(p);
	if(p2.empty()){
		throw std::runtime_error("Unexpected end of string");
	}

	const char ch1 = p2.first_char();

	//	"-xxx"
	if(ch1 == '-'){
		const auto a = evaluate_expression(p2.rest(), eoperator_precedence::k_super_strong);
		return { -a.first, skip_whitespace(a.second) };
	}
	//	"(yyy)xxx"
	else if(ch1 == '('){
		const auto a = evaluate_expression(p2.rest(), eoperator_precedence::k_super_weak);
		if (a.second.first(1) != ")"){
			throw std::runtime_error("Expected ')'");
		}
		return { a.first, skip_whitespace(a.second.rest()) };
	}

	//	":yyy xxx"
	else if(ch1 == ':'){
		const auto a = evaluate_expression(p2.rest(), eoperator_precedence::k_super_weak);
		return { a.first, skip_whitespace(a.second.rest()) };
	}

	//	"1234xxx" or "my_function(3)xxx"
	else {
		const auto a = evaluate_single(p2);
		return { a.first, skip_whitespace(a.second) };
	}
}




pair<int, seq_t> evaluate_operation(const seq_t& p1, const int value, const eoperator_precedence precedence){
	QUARK_ASSERT(p1.check_invariant());

	const auto p = p1;
	if(!p.empty()){
		const char op1 = p.first_char();
		const string op2 = p.first(2);

		//	Ending parantesis
		if(op1 == ')' && precedence > eoperator_precedence::k_parentesis){
			return { value, p };
		}

		//	EXPRESSION + EXPRESSION +
		//	EXPRESSION - EXPRESSION -
		else if((op1 == '+' || op1 == '-') && precedence > eoperator_precedence::k_add_sub){
			const auto a = evaluate_expression(p.rest(), eoperator_precedence::k_add_sub);
			const auto value2 = op1 == '+' ? value + a.first : value - a.first;
			const auto p2 = a.second;
			return p2.empty() ? pair<int, seq_t>{ value2, p2 } : evaluate_operation(p2, value2, precedence);
		}

		//	EXPRESSION * EXPRESSION *
		//	EXPRESSION / EXPRESSION /
		else if((op1 == '*' || op1 == '/') && precedence > eoperator_precedence::k_multiply_divider_remainder) {
			const auto a = evaluate_expression(p.rest(), eoperator_precedence::k_multiply_divider_remainder);
			const auto value2 = op1 == '*' ? value * a.first : value / a.first;
			const auto p2 = a.second;
			return p2.empty() ? pair<int, seq_t>{ value2, p2 } : evaluate_operation(p2, value2, precedence);
		}

		//	EXPRESSION ? EXPRESSION : EXPRESSION
		else if(op1 == '?' && precedence > eoperator_precedence::k_comparison_operator) {
			const auto true_expr_p = evaluate_expression(p.rest(), eoperator_precedence::k_comparison_operator);

			const auto colon = true_expr_p.second.first(1);
			if(colon != ":"){
				throw std::runtime_error("Expected ':'");
			}

			const auto false_expr_p = evaluate_expression(true_expr_p.second.rest(), precedence);
			const auto value2 = value != 0 ? true_expr_p.first : false_expr_p.first;

			//	End this precedence level.
			return { value2, false_expr_p.second.rest() };
		}

		//	EXPRESSION == EXPRESSION
		else if(op2 == "==" && precedence > eoperator_precedence::k_equal__not_equal){
			const auto right = evaluate_expression(p.rest(2), eoperator_precedence::k_equal__not_equal);
			const auto value2 = (value == right.first) ? 1 : 0;

			//	End this precedence level.
			return { value2, right.second.rest() };
		}
		//	EXPRESSION != EXPRESSION
		else if(op2 == "!=" && precedence > eoperator_precedence::k_equal__not_equal){
			const auto right = evaluate_expression(p.rest(2), eoperator_precedence::k_equal__not_equal);
			const auto value2 = (value == right.first) ? 0 : 1;

			//	End this precedence level.
			return { value2, right.second.rest() };
		}

		//	EXPRESSION || EXPRESSION
		else if(op2 == "&&" && precedence > eoperator_precedence::k_logical_and){
			const auto a = evaluate_expression(p.rest(2), eoperator_precedence::k_logical_and);
			const auto value2 = (value !=0 && a.first != 0) ? 1 : 0;
			const auto p2 = a.second;
			return p2.empty() ? pair<int, seq_t>{ value2, p2 } : evaluate_operation(p2, value2, precedence);
		}

		//	EXPRESSION || EXPRESSION
		else if(op2 == "||" && precedence > eoperator_precedence::k_logical_or){
			const auto a = evaluate_expression(p.rest(2), eoperator_precedence::k_logical_or);
			const auto value2 = (value !=0 || a.first != 0) ? 1 : 0;
			const auto p2 = a.second;
			return p2.empty() ? pair<int, seq_t>{ value2, p2 } : evaluate_operation(p2, value2, precedence);
		}

		else{
			return { value, p };
		}
	}
	else{
		return { value, p };
	}
}

pair<int, seq_t> evaluate_expression(const seq_t& p, const eoperator_precedence precedence){
	QUARK_ASSERT(p.check_invariant());

	auto a = evaluate_atom(p);
	return evaluate_operation(a.second, a.first, precedence);
}

pair<int, seq_t> evaluate_expression(const seq_t& p){
	QUARK_ASSERT(p.check_invariant());

	return evaluate_expression(p, eoperator_precedence::k_super_weak);
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

QUARK_UNIT_TESTQ("evaluate_expression()", "?:"){
	QUARK_UT_VERIFY((evaluate_expression(seq_t("1 ? 2 : 3")) == pair<int, seq_t>{ 2, seq_t("") }));
}

QUARK_UNIT_TESTQ("evaluate_expression()", "?:"){
	QUARK_UT_VERIFY((evaluate_expression(seq_t("0 ? 2 : 3")) == pair<int, seq_t>{ 3, seq_t("") }));
}


QUARK_UNIT_TESTQ("evaluate_expression()", "=="){
	QUARK_UT_VERIFY((evaluate_expression(seq_t("4 == 4")) == pair<int, seq_t>{ 1, seq_t("") }));
}
QUARK_UNIT_TESTQ("evaluate_expression()", "=="){
	QUARK_UT_VERIFY((evaluate_expression(seq_t("4 == 5")) == pair<int, seq_t>{ 0, seq_t("") }));
}

QUARK_UNIT_TESTQ("evaluate_expression()", "!="){
	QUARK_UT_VERIFY((evaluate_expression(seq_t("1 != 2")) == pair<int, seq_t>{ 1, seq_t("") }));
}
QUARK_UNIT_TESTQ("evaluate_expression()", "!="){
	QUARK_UT_VERIFY((evaluate_expression(seq_t("3 != 3")) == pair<int, seq_t>{ 0, seq_t("") }));
}


QUARK_UNIT_TESTQ("evaluate_expression()", "&&"){
	QUARK_UT_VERIFY((evaluate_expression(seq_t("0 && 0")) == pair<int, seq_t>{ 0, seq_t("") }));
}
QUARK_UNIT_TESTQ("evaluate_expression()", "&&"){
	QUARK_UT_VERIFY((evaluate_expression(seq_t("0 && 1")) == pair<int, seq_t>{ 0, seq_t("") }));
}
QUARK_UNIT_TESTQ("evaluate_expression()", "&&"){
	QUARK_UT_VERIFY((evaluate_expression(seq_t("1 && 0")) == pair<int, seq_t>{ 0, seq_t("") }));
}
QUARK_UNIT_TESTQ("evaluate_expression()", "&&"){
	QUARK_UT_VERIFY((evaluate_expression(seq_t("1 && 1")) == pair<int, seq_t>{ 1, seq_t("") }));
}

QUARK_UNIT_TESTQ("evaluate_expression()", "&&"){
	QUARK_UT_VERIFY((evaluate_expression(seq_t("0 && 0 && 0")) == pair<int, seq_t>{ 0, seq_t("") }));
}
QUARK_UNIT_TESTQ("evaluate_expression()", "&&"){
	QUARK_UT_VERIFY((evaluate_expression(seq_t("0 && 0 && 1")) == pair<int, seq_t>{ 0, seq_t("") }));
}
QUARK_UNIT_TESTQ("evaluate_expression()", "&&"){
	QUARK_UT_VERIFY((evaluate_expression(seq_t("0 && 1 && 0")) == pair<int, seq_t>{ 0, seq_t("") }));
}
QUARK_UNIT_TESTQ("evaluate_expression()", "&&"){
	QUARK_UT_VERIFY((evaluate_expression(seq_t("0 && 1 && 1")) == pair<int, seq_t>{ 0, seq_t("") }));
}
QUARK_UNIT_TESTQ("evaluate_expression()", "&&"){
	QUARK_UT_VERIFY((evaluate_expression(seq_t("1 && 0 && 0")) == pair<int, seq_t>{ 0, seq_t("") }));
}
QUARK_UNIT_TESTQ("evaluate_expression()", "&&"){
	QUARK_UT_VERIFY((evaluate_expression(seq_t("1 && 0 && 1")) == pair<int, seq_t>{ 0, seq_t("") }));
}
QUARK_UNIT_TESTQ("evaluate_expression()", "&&"){
	QUARK_UT_VERIFY((evaluate_expression(seq_t("1 && 1 && 0")) == pair<int, seq_t>{ 0, seq_t("") }));
}
QUARK_UNIT_TESTQ("evaluate_expression()", "&&"){
	QUARK_UT_VERIFY((evaluate_expression(seq_t("1 && 1 && 1")) == pair<int, seq_t>{ 1, seq_t("") }));
}

QUARK_UNIT_TESTQ("evaluate_expression()", "&&"){
	QUARK_UT_VERIFY((1 * 1 && 0 + 1) == true);
	QUARK_UT_VERIFY((evaluate_expression(seq_t("1 * 1 && 0 + 1")) == pair<int, seq_t>{ 1, seq_t("") }));
}
QUARK_UNIT_TESTQ("evaluate_expression()", "&&"){
	QUARK_UT_VERIFY((evaluate_expression(seq_t("1 * 1 && 0 * 1")) == pair<int, seq_t>{ 0, seq_t("") }));
}



QUARK_UNIT_TESTQ("evaluate_expression()", "||"){
	QUARK_UT_VERIFY((evaluate_expression(seq_t("0 || 0")) == pair<int, seq_t>{ 0, seq_t("") }));
}
QUARK_UNIT_TESTQ("evaluate_expression()", "||"){
	QUARK_UT_VERIFY((evaluate_expression(seq_t("0 || 1")) == pair<int, seq_t>{ 1, seq_t("") }));
}
QUARK_UNIT_TESTQ("evaluate_expression()", "||"){
	QUARK_UT_VERIFY((evaluate_expression(seq_t("1 || 0")) == pair<int, seq_t>{ 1, seq_t("") }));
}
QUARK_UNIT_TESTQ("evaluate_expression()", "||"){
	QUARK_UT_VERIFY((evaluate_expression(seq_t("1 || 1")) == pair<int, seq_t>{ 1, seq_t("") }));
}
QUARK_UNIT_TESTQ("evaluate_expression()", "||"){
	QUARK_UT_VERIFY((evaluate_expression(seq_t("0 || 0 || 0")) == pair<int, seq_t>{ 0, seq_t("") }));
}
QUARK_UNIT_TESTQ("evaluate_expression()", "||"){
	QUARK_UT_VERIFY((evaluate_expression(seq_t("0 || 0 || 1")) == pair<int, seq_t>{ 1, seq_t("") }));
}
QUARK_UNIT_TESTQ("evaluate_expression()", "||"){
	QUARK_UT_VERIFY((evaluate_expression(seq_t("0 || 1 || 0")) == pair<int, seq_t>{ 1, seq_t("") }));
}
QUARK_UNIT_TESTQ("evaluate_expression()", "||"){
	QUARK_UT_VERIFY((evaluate_expression(seq_t("0 || 1 || 1")) == pair<int, seq_t>{ 1, seq_t("") }));
}
QUARK_UNIT_TESTQ("evaluate_expression()", "||"){
	QUARK_UT_VERIFY((evaluate_expression(seq_t("1 || 0 || 0")) == pair<int, seq_t>{ 1, seq_t("") }));
}
QUARK_UNIT_TESTQ("evaluate_expression()", "||"){
	QUARK_UT_VERIFY((evaluate_expression(seq_t("1 || 0 || 1")) == pair<int, seq_t>{ 1, seq_t("") }));
}
QUARK_UNIT_TESTQ("evaluate_expression()", "||"){
	QUARK_UT_VERIFY((evaluate_expression(seq_t("1 || 1 || 0")) == pair<int, seq_t>{ 1, seq_t("") }));
}
QUARK_UNIT_TESTQ("evaluate_expression()", "||"){
	QUARK_UT_VERIFY((evaluate_expression(seq_t("1 || 1 || 1")) == pair<int, seq_t>{ 1, seq_t("") }));
}





