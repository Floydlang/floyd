//
//  parser2.h
//  FloydSpeak
//
//  Created by Marcus Zetterquist on 24/08/16.
//  Copyright © 2016 Marcus Zetterquist. All rights reserved.
//

//	Based on Magnus snippet.

#include "quark.h"

#include <string>
#include <memory>

#include "text_parser.h"


#include "parser2.h"

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




QUARK_UNIT_TESTQ("enum class()", ""){
	enum class my_enum {
		k_one = 1,
		k_four = 4
	};

	QUARK_UT_VERIFY(my_enum::k_one == my_enum::k_one);
	QUARK_UT_VERIFY(my_enum::k_one != my_enum::k_four);
	QUARK_UT_VERIFY(static_cast<int>(my_enum::k_one) == 1);
}



/*
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
*/




pair<int, seq_t> parse_long(const seq_t& s) {
	QUARK_ASSERT(s.check_invariant());

	char* e = nullptr;
	long value = strtol(s.c_str(), &e, 0);
	size_t used = e - s.c_str();
	if (used == 0){
		throw std::runtime_error("Invalid number");
	}
	return { (int)value, s.rest(used) };
}

QUARK_UNIT_1("parse_long()", "", (parse_long(seq_t("0")) == pair<int, seq_t>{ 0, seq_t("") }));
QUARK_UNIT_1("parse_long()", "", (parse_long(seq_t("0xyz")) == pair<int, seq_t>{ 0, seq_t("xyz") }));
QUARK_UNIT_1("parse_long()", "", (parse_long(seq_t("13xyz")) == pair<int, seq_t>{ 13, seq_t("xyz") }));



	template<typename EXPRESSION>
	struct my_helper : public maker<EXPRESSION> {


		public: virtual const EXPRESSION maker__on_string(const eoperation op, const std::string& s) const{
			if(op == eoperation::k_0_number_constant){
				return stoi(s);
			}
			else if(op == eoperation::k_0_identifer){
				return 0;
			}
			else if(op == eoperation::k_0_string_literal){
				return 0;
			}
			else{
				QUARK_ASSERT(false);
			}
		}
		public: virtual const EXPRESSION maker__make(const eoperation op, const EXPRESSION& expr) const{
			if(op == eoperation::k_1_logical_not){
				return -expr;
			}
			else{
				QUARK_ASSERT(false);
			}
		}
		public: virtual const EXPRESSION maker__make(const eoperation op, const EXPRESSION& lhs, const EXPRESSION& rhs) const{
			if(op == eoperation::k_2_add){
				return lhs + rhs;
			}
			else if(op == eoperation::k_2_subtract){
				return lhs - rhs;
			}
			else if(op == eoperation::k_2_multiply){
				return lhs * rhs;
			}
			else if(op == eoperation::k_2_divide){
				return lhs / rhs;
			}
			else if(op == eoperation::k_2_remainder){
				return lhs % rhs;
			}

			else if(op == eoperation::k_2_smaller_or_equal){
				return lhs <= rhs;
			}
			else if(op == eoperation::k_2_smaller){
				return lhs < rhs;
			}
			else if(op == eoperation::k_2_larger_or_equal){
				return lhs >= rhs;
			}
			else if(op == eoperation::k_2_larger){
				return lhs > rhs;
			}


			else if(op == eoperation::k_2_logical_equal){
				return lhs == rhs;
			}
			else if(op == eoperation::k_2_logical_nonequal){
				return lhs != rhs;
			}
			else if(op == eoperation::k_2_logical_and){
				return lhs != 0 && rhs != 0;
			}
			else if(op == eoperation::k_2_logical_or){
				return lhs != 0 || rhs != 0;
			}
			else{
				QUARK_ASSERT(false);
			}
		}
		public: virtual const EXPRESSION maker__make(const eoperation op, const EXPRESSION& e1, const EXPRESSION& e2, const EXPRESSION& e3) const{
			if(op == eoperation::k_3_conditional_operator){
				return e1 != 0 ? e2 : e3;
			}
			else{
				QUARK_ASSERT(false);
			}
		}
	};

pair<int, seq_t> test_evaluate_int_expr(const seq_t& p){
	QUARK_ASSERT(p.check_invariant());

	my_helper<int64_t> helper;
	return evaluate_expression2<int64_t>(helper, p);
}

bool test(const std::string& expression, int expected_int, string expected_rest){
	const auto result = test_evaluate_int_expr(seq_t(expression));
	if(result.first != expected_int){
		return false;
	}
	else if(result.second.get_all() != expected_rest){
		return false;
	}
	return true;
}

QUARK_UNIT_TESTQ("test_evaluate_int_expr()", ""){
	try{
		test_evaluate_int_expr(seq_t(""));
		QUARK_UT_VERIFY(false);
	}
	catch(...){
	}
}

QUARK_UNIT_1("test_evaluate_int_expr()", "", test("0", 0, ""));
QUARK_UNIT_1("test_evaluate_int_expr()", "", test("1234567890", 1234567890, ""));

QUARK_UNIT_1("test_evaluate_int_expr()", "", test("10 + 4", 14, ""));
QUARK_UNIT_1("test_evaluate_int_expr()", "", test("1 + 2 + 3 + 4", 10, ""));

QUARK_UNIT_1("test_evaluate_int_expr()", "", test("1 + 8 + 7 + 2 * 3 + 4 * 5 + 6", 48, ""));
QUARK_UNIT_1("test_evaluate_int_expr()", "", test("10 * 4", 40, ""));
QUARK_UNIT_1("test_evaluate_int_expr()", "", test("10 * 4 * 3", 120, ""));
QUARK_UNIT_1("test_evaluate_int_expr()", "", test("40 / 4", 10, ""));

QUARK_UNIT_TESTQ("test_evaluate_int_expr()", ""){ 
	QUARK_ASSERT((40 / 5 / 2) == 4);
	QUARK_UT_VERIFY(test("40 / 5 / 2", 4, ""));
}

QUARK_UNIT_1("test_evaluate_int_expr()", "", test("41 % 5", 1, ""));
QUARK_UNIT_TESTQ("test_evaluate_int_expr()", ""){
	QUARK_ASSERT((413 % 50 % 10) == 3);
	QUARK_UT_VERIFY(test("413 % 50 % 10", 3, ""));
}

QUARK_UNIT_1("test_evaluate_int_expr()", "", test("(3)", 3, ""));
QUARK_UNIT_1("test_evaluate_int_expr()", "", test("(3 * 8)", 24, ""));
QUARK_UNIT_1("test_evaluate_int_expr()", "", test("1 + 3 * 2 + 100", 107, ""));
QUARK_UNIT_1("test_evaluate_int_expr()", "", test("-(3 * 2 + (8 * 2)) - (((1))) * 2", -(3 * 2 + (8 * 2)) - (((1))) * 2, ""));

QUARK_UNIT_1("test_evaluate_int_expr()", "?:", test("1 ? 2 : 3", 2, ""));
QUARK_UNIT_1("test_evaluate_int_expr()", "?:", test("0 ? 2 : 3", 3, ""));

QUARK_UNIT_1("test_evaluate_int_expr()", "<=", test("4 <= 4", 1, ""));
QUARK_UNIT_1("test_evaluate_int_expr()", "<=", test("3 <= 4", 1, ""));
QUARK_UNIT_1("test_evaluate_int_expr()", "<=", test("5 <= 4", 0, ""));

QUARK_UNIT_1("test_evaluate_int_expr()", "<", test("2 < 3", 1, ""));
QUARK_UNIT_1("test_evaluate_int_expr()", "<", test("3 < 3", 0, ""));

QUARK_UNIT_1("test_evaluate_int_expr()", ">=", test("4 >= 4", 1, ""));
QUARK_UNIT_1("test_evaluate_int_expr()", ">=", test("4 >= 3", 1, ""));
QUARK_UNIT_1("test_evaluate_int_expr()", ">=", test("4 >= 5", 0, ""));

QUARK_UNIT_1("test_evaluate_int_expr()", ">", test("3 > 2", 1, ""));
QUARK_UNIT_1("test_evaluate_int_expr()", ">", test("3 > 3", 0, ""));

QUARK_UNIT_1("test_evaluate_int_expr()", "==", test("4 == 4", 1, ""));
QUARK_UNIT_1("test_evaluate_int_expr()", "==", test("4 == 5", 0, ""));

QUARK_UNIT_1("test_evaluate_int_expr()", "!=", test("1 != 2", 1, ""));
QUARK_UNIT_1("test_evaluate_int_expr()", "!=", test("3 != 3", 0, ""));

QUARK_UNIT_1("test_evaluate_int_expr()", "&&", test("0 && 0", 0, ""));
QUARK_UNIT_1("test_evaluate_int_expr()", "&&", test("0 && 1", 0, ""));
QUARK_UNIT_1("test_evaluate_int_expr()", "&&", test("1 && 0", 0, ""));
QUARK_UNIT_1("test_evaluate_int_expr()", "&&", test("1 && 1", 1, ""));
QUARK_UNIT_1("test_evaluate_int_expr()", "&&", test("0 && 0 && 0", 0, ""));
QUARK_UNIT_1("test_evaluate_int_expr()", "&&", test("0 && 0 && 1", 0, ""));
QUARK_UNIT_1("test_evaluate_int_expr()", "&&", test("0 && 1 && 0", 0, ""));
QUARK_UNIT_1("test_evaluate_int_expr()", "&&", test("0 && 1 && 1", 0, ""));
QUARK_UNIT_1("test_evaluate_int_expr()", "&&", test("1 && 0 && 0", 0, ""));
QUARK_UNIT_1("test_evaluate_int_expr()", "&&", test("1 && 0 && 1", 0, ""));
QUARK_UNIT_1("test_evaluate_int_expr()", "&&", test("1 && 1 && 0", 0, ""));
QUARK_UNIT_1("test_evaluate_int_expr()", "&&", test("1 && 1 && 1", 1, ""));
QUARK_UNIT_TESTQ("test_evaluate_int_expr()", "&&"){
	QUARK_UT_VERIFY((1 * 1 && 0 + 1) == true);
	QUARK_UT_VERIFY(test("1 * 1 && 0 + 1", 1, ""));
}
QUARK_UNIT_1("test_evaluate_int_expr()", "&&", test("1 * 1 && 0 * 1", 0, ""));

QUARK_UNIT_1("test_evaluate_int_expr()", "||", test("0 || 0", 0, ""));
QUARK_UNIT_1("test_evaluate_int_expr()", "||", test("0 || 1", 1, ""));
QUARK_UNIT_1("test_evaluate_int_expr()", "||", test("1 || 0", 1, ""));
QUARK_UNIT_1("test_evaluate_int_expr()", "||", test("1 || 1", 1, ""));
QUARK_UNIT_1("test_evaluate_int_expr()", "||", test("0 || 0 || 0", 0, ""));
QUARK_UNIT_1("test_evaluate_int_expr()", "||", test("0 || 0 || 1", 1, ""));
QUARK_UNIT_1("test_evaluate_int_expr()", "||", test("0 || 1 || 0", 1, ""));
QUARK_UNIT_1("test_evaluate_int_expr()", "||", test("0 || 1 || 1", 1, ""));
QUARK_UNIT_1("test_evaluate_int_expr()", "||", test("1 || 0 || 0", 1, ""));
QUARK_UNIT_1("test_evaluate_int_expr()", "||", test("1 || 0 || 1", 1, ""));
QUARK_UNIT_1("test_evaluate_int_expr()", "||", test("1 || 1 || 0", 1, ""));
QUARK_UNIT_1("test_evaluate_int_expr()", "||", test("1 || 1 || 1", 1, ""));

QUARK_UNIT_1("test_evaluate_int_expr()", "", test("10 + my_variable", 10, ""));
QUARK_UNIT_1("test_evaluate_int_expr()", "", test("10 + \"my string\"", 10, ""));

