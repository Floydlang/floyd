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

QUARK_UNIT_TESTQ("parse_long()", ""){
	QUARK_UT_VERIFY((parse_long(seq_t("0")) == pair<int, seq_t>{ 0, seq_t("") }));
}

QUARK_UNIT_TESTQ("parse_long()", ""){
	QUARK_UT_VERIFY((parse_long(seq_t("0xyz")) == pair<int, seq_t>{ 0, seq_t("xyz") }));
}

QUARK_UNIT_TESTQ("parse_long()", ""){
	QUARK_UT_VERIFY((parse_long(seq_t("13xyz")) == pair<int, seq_t>{ 13, seq_t("xyz") }));
}



	template<typename EXPRESSION>
	struct my_helper : public on_node_i<EXPRESSION> {

		public: virtual const EXPRESSION on_node_i__on_number_constant(const std::string& terminal) const{
			return stoi(terminal);
		}
		public: virtual const EXPRESSION on_node_i__on_identifier(const std::string& terminal) const{
			return 0;
		}
		public: virtual const EXPRESSION on_node_i__on_string_constant(const std::string& terminal) const{
			return 0;
		}

		public: virtual const EXPRESSION on_node_i__on_plus(const EXPRESSION& lhs, const EXPRESSION& rhs) const{
			return lhs + rhs;
		};
		public: virtual const EXPRESSION on_node_i__on_minus(const EXPRESSION& lhs, const EXPRESSION& rhs) const{
			return lhs - rhs;
		}
		public: virtual const EXPRESSION on_node_i__on_multiply(const EXPRESSION& lhs, const EXPRESSION& rhs) const{
			return lhs * rhs;
		};
		public: virtual const EXPRESSION on_node_i__on_divide(const EXPRESSION& lhs, const EXPRESSION& rhs) const{
			return lhs / rhs;
		};
		public: virtual const EXPRESSION on_node_i__on_remainder(const EXPRESSION& lhs, const EXPRESSION& rhs) const{
			return lhs % rhs;
		};

		public: virtual const EXPRESSION on_node_i__on_logical_equal(const EXPRESSION& lhs, const EXPRESSION& rhs) const{
			return lhs == rhs ? 1 : 0;
		};
		public: virtual const EXPRESSION on_node_i__on_logical_nonequal(const EXPRESSION& lhs, const EXPRESSION& rhs) const{
			return lhs != rhs ? 1 : 0;
		};
		public: virtual const EXPRESSION on_node_i__on_logical_and(const EXPRESSION& lhs, const EXPRESSION& rhs) const{
			return lhs != 0 && rhs != 0 ? 1 : 0;
		};
		public: virtual const EXPRESSION on_node_i__on_logical_or(const EXPRESSION& lhs, const EXPRESSION& rhs) const{
			return lhs != 0 || rhs != 0 ? 1 : 0;
		};

		public: virtual const EXPRESSION on_node_i__on_conditional_operator(const EXPRESSION& condition, const EXPRESSION& true_expr, const EXPRESSION& false_expr) const{
			return condition != 0 ? true_expr : false_expr;
		}
		public: virtual const EXPRESSION on_node_i__on_arithm_negate(const EXPRESSION& value) const{
			return -value;
		}
	};

pair<int, seq_t> test_evaluate_int_expr(const seq_t& p){
	QUARK_ASSERT(p.check_invariant());

	my_helper<int64_t> helper;
	return evaluate_expression2<int64_t>(helper, p);
}



QUARK_UNIT_TESTQ("test_evaluate_int_expr()", ""){
	try{
		test_evaluate_int_expr(seq_t(""));
		QUARK_UT_VERIFY(false);
	}
	catch(...){
	}
}

QUARK_UNIT_TESTQ("test_evaluate_int_expr()", ""){
	QUARK_UT_VERIFY((test_evaluate_int_expr(seq_t("0")) == pair<int, seq_t>{ 0, seq_t("") }));
}

QUARK_UNIT_TESTQ("test_evaluate_int_expr()", ""){
	QUARK_UT_VERIFY((test_evaluate_int_expr(seq_t("1234567890")) == pair<int, seq_t>{ 1234567890, seq_t("") }));
}



QUARK_UNIT_TESTQ("test_evaluate_int_expr()", ""){
	QUARK_UT_VERIFY((test_evaluate_int_expr(seq_t("10 + 4")) == pair<int, seq_t>{ 14, seq_t("") }));
}

QUARK_UNIT_TESTQ("test_evaluate_int_expr()", ""){
	QUARK_UT_VERIFY((test_evaluate_int_expr(seq_t("1 + 2 + 3 + 4")) == pair<int, seq_t>{ 10, seq_t("") }));
}

QUARK_UNIT_TESTQ("test_evaluate_int_expr()", ""){
	QUARK_UT_VERIFY((test_evaluate_int_expr(seq_t("1 + 8 + 7 + 2 * 3 + 4 * 5 + 6")) == pair<int, seq_t>{ 48, seq_t("") }));
}



QUARK_UNIT_TESTQ("test_evaluate_int_expr()", ""){
	QUARK_UT_VERIFY((test_evaluate_int_expr(seq_t("10 * 4")) == pair<int, seq_t>{ 40, seq_t("") }));
}

QUARK_UNIT_TESTQ("test_evaluate_int_expr()", ""){
	QUARK_UT_VERIFY((test_evaluate_int_expr(seq_t("10 * 4 * 3")) == pair<int, seq_t>{ 120, seq_t("") }));
}

QUARK_UNIT_TESTQ("test_evaluate_int_expr()", ""){
	QUARK_UT_VERIFY((test_evaluate_int_expr(seq_t("40 / 4")) == pair<int, seq_t>{ 10, seq_t("") }));
}

QUARK_UNIT_TESTQ("test_evaluate_int_expr()", ""){
	QUARK_ASSERT((40 / 5 / 2) == 4);
	QUARK_UT_VERIFY((test_evaluate_int_expr(seq_t("40 / 5 / 2")) == pair<int, seq_t>{ 4, seq_t("") }));
}


QUARK_UNIT_TESTQ("test_evaluate_int_expr()", ""){
	QUARK_UT_VERIFY((test_evaluate_int_expr(seq_t("41 % 5")) == pair<int, seq_t>{ 1, seq_t("") }));
}

QUARK_UNIT_TESTQ("test_evaluate_int_expr()", ""){
	QUARK_ASSERT((413 % 50 % 10) == 3);
	QUARK_UT_VERIFY((test_evaluate_int_expr(seq_t("413 % 50 % 10")) == pair<int, seq_t>{ 3, seq_t("") }));
}



QUARK_UNIT_TESTQ("test_evaluate_int_expr()", ""){
	QUARK_UT_VERIFY((test_evaluate_int_expr(seq_t("(3)")) == pair<int, seq_t>{ 3, seq_t("") }));
}

QUARK_UNIT_TESTQ("test_evaluate_int_expr()", ""){
	QUARK_UT_VERIFY((test_evaluate_int_expr(seq_t("(3 * 8)")) == pair<int, seq_t>{ 24, seq_t("") }));
}

QUARK_UNIT_TESTQ("test_evaluate_int_expr()", ""){
	QUARK_UT_VERIFY((test_evaluate_int_expr(seq_t("1 + 3 * 2 + 100")) == pair<int, seq_t>{ 107, seq_t("") }));
}

QUARK_UNIT_TESTQ("test_evaluate_int_expr()", ""){
	QUARK_UT_VERIFY((test_evaluate_int_expr(seq_t("-(3 * 2 + (8 * 2)) - (((1))) * 2")) == pair<int, seq_t>{ -(3 * 2 + (8 * 2)) - (((1))) * 2, seq_t("") }));
}

QUARK_UNIT_TESTQ("test_evaluate_int_expr()", "?:"){
	QUARK_UT_VERIFY((test_evaluate_int_expr(seq_t("1 ? 2 : 3")) == pair<int, seq_t>{ 2, seq_t("") }));
}

QUARK_UNIT_TESTQ("test_evaluate_int_expr()", "?:"){
	QUARK_UT_VERIFY((test_evaluate_int_expr(seq_t("0 ? 2 : 3")) == pair<int, seq_t>{ 3, seq_t("") }));
}


QUARK_UNIT_TESTQ("test_evaluate_int_expr()", "=="){
	QUARK_UT_VERIFY((test_evaluate_int_expr(seq_t("4 == 4")) == pair<int, seq_t>{ 1, seq_t("") }));
}
QUARK_UNIT_TESTQ("test_evaluate_int_expr()", "=="){
	QUARK_UT_VERIFY((test_evaluate_int_expr(seq_t("4 == 5")) == pair<int, seq_t>{ 0, seq_t("") }));
}

QUARK_UNIT_TESTQ("test_evaluate_int_expr()", "!="){
	QUARK_UT_VERIFY((test_evaluate_int_expr(seq_t("1 != 2")) == pair<int, seq_t>{ 1, seq_t("") }));
}
QUARK_UNIT_TESTQ("test_evaluate_int_expr()", "!="){
	QUARK_UT_VERIFY((test_evaluate_int_expr(seq_t("3 != 3")) == pair<int, seq_t>{ 0, seq_t("") }));
}


QUARK_UNIT_TESTQ("test_evaluate_int_expr()", "&&"){
	QUARK_UT_VERIFY((test_evaluate_int_expr(seq_t("0 && 0")) == pair<int, seq_t>{ 0, seq_t("") }));
}
QUARK_UNIT_TESTQ("test_evaluate_int_expr()", "&&"){
	QUARK_UT_VERIFY((test_evaluate_int_expr(seq_t("0 && 1")) == pair<int, seq_t>{ 0, seq_t("") }));
}
QUARK_UNIT_TESTQ("test_evaluate_int_expr()", "&&"){
	QUARK_UT_VERIFY((test_evaluate_int_expr(seq_t("1 && 0")) == pair<int, seq_t>{ 0, seq_t("") }));
}
QUARK_UNIT_TESTQ("test_evaluate_int_expr()", "&&"){
	QUARK_UT_VERIFY((test_evaluate_int_expr(seq_t("1 && 1")) == pair<int, seq_t>{ 1, seq_t("") }));
}

QUARK_UNIT_TESTQ("test_evaluate_int_expr()", "&&"){
	QUARK_UT_VERIFY((test_evaluate_int_expr(seq_t("0 && 0 && 0")) == pair<int, seq_t>{ 0, seq_t("") }));
}
QUARK_UNIT_TESTQ("test_evaluate_int_expr()", "&&"){
	QUARK_UT_VERIFY((test_evaluate_int_expr(seq_t("0 && 0 && 1")) == pair<int, seq_t>{ 0, seq_t("") }));
}
QUARK_UNIT_TESTQ("test_evaluate_int_expr()", "&&"){
	QUARK_UT_VERIFY((test_evaluate_int_expr(seq_t("0 && 1 && 0")) == pair<int, seq_t>{ 0, seq_t("") }));
}
QUARK_UNIT_TESTQ("test_evaluate_int_expr()", "&&"){
	QUARK_UT_VERIFY((test_evaluate_int_expr(seq_t("0 && 1 && 1")) == pair<int, seq_t>{ 0, seq_t("") }));
}
QUARK_UNIT_TESTQ("test_evaluate_int_expr()", "&&"){
	QUARK_UT_VERIFY((test_evaluate_int_expr(seq_t("1 && 0 && 0")) == pair<int, seq_t>{ 0, seq_t("") }));
}
QUARK_UNIT_TESTQ("test_evaluate_int_expr()", "&&"){
	QUARK_UT_VERIFY((test_evaluate_int_expr(seq_t("1 && 0 && 1")) == pair<int, seq_t>{ 0, seq_t("") }));
}
QUARK_UNIT_TESTQ("test_evaluate_int_expr()", "&&"){
	QUARK_UT_VERIFY((test_evaluate_int_expr(seq_t("1 && 1 && 0")) == pair<int, seq_t>{ 0, seq_t("") }));
}
QUARK_UNIT_TESTQ("test_evaluate_int_expr()", "&&"){
	QUARK_UT_VERIFY((test_evaluate_int_expr(seq_t("1 && 1 && 1")) == pair<int, seq_t>{ 1, seq_t("") }));
}

QUARK_UNIT_TESTQ("test_evaluate_int_expr()", "&&"){
	QUARK_UT_VERIFY((1 * 1 && 0 + 1) == true);
	QUARK_UT_VERIFY((test_evaluate_int_expr(seq_t("1 * 1 && 0 + 1")) == pair<int, seq_t>{ 1, seq_t("") }));
}
QUARK_UNIT_TESTQ("test_evaluate_int_expr()", "&&"){
	QUARK_UT_VERIFY((test_evaluate_int_expr(seq_t("1 * 1 && 0 * 1")) == pair<int, seq_t>{ 0, seq_t("") }));
}



QUARK_UNIT_TESTQ("test_evaluate_int_expr()", "||"){
	QUARK_UT_VERIFY((test_evaluate_int_expr(seq_t("0 || 0")) == pair<int, seq_t>{ 0, seq_t("") }));
}
QUARK_UNIT_TESTQ("test_evaluate_int_expr()", "||"){
	QUARK_UT_VERIFY((test_evaluate_int_expr(seq_t("0 || 1")) == pair<int, seq_t>{ 1, seq_t("") }));
}
QUARK_UNIT_TESTQ("test_evaluate_int_expr()", "||"){
	QUARK_UT_VERIFY((test_evaluate_int_expr(seq_t("1 || 0")) == pair<int, seq_t>{ 1, seq_t("") }));
}
QUARK_UNIT_TESTQ("test_evaluate_int_expr()", "||"){
	QUARK_UT_VERIFY((test_evaluate_int_expr(seq_t("1 || 1")) == pair<int, seq_t>{ 1, seq_t("") }));
}
QUARK_UNIT_TESTQ("test_evaluate_int_expr()", "||"){
	QUARK_UT_VERIFY((test_evaluate_int_expr(seq_t("0 || 0 || 0")) == pair<int, seq_t>{ 0, seq_t("") }));
}
QUARK_UNIT_TESTQ("test_evaluate_int_expr()", "||"){
	QUARK_UT_VERIFY((test_evaluate_int_expr(seq_t("0 || 0 || 1")) == pair<int, seq_t>{ 1, seq_t("") }));
}
QUARK_UNIT_TESTQ("test_evaluate_int_expr()", "||"){
	QUARK_UT_VERIFY((test_evaluate_int_expr(seq_t("0 || 1 || 0")) == pair<int, seq_t>{ 1, seq_t("") }));
}
QUARK_UNIT_TESTQ("test_evaluate_int_expr()", "||"){
	QUARK_UT_VERIFY((test_evaluate_int_expr(seq_t("0 || 1 || 1")) == pair<int, seq_t>{ 1, seq_t("") }));
}
QUARK_UNIT_TESTQ("test_evaluate_int_expr()", "||"){
	QUARK_UT_VERIFY((test_evaluate_int_expr(seq_t("1 || 0 || 0")) == pair<int, seq_t>{ 1, seq_t("") }));
}
QUARK_UNIT_TESTQ("test_evaluate_int_expr()", "||"){
	QUARK_UT_VERIFY((test_evaluate_int_expr(seq_t("1 || 0 || 1")) == pair<int, seq_t>{ 1, seq_t("") }));
}
QUARK_UNIT_TESTQ("test_evaluate_int_expr()", "||"){
	QUARK_UT_VERIFY((test_evaluate_int_expr(seq_t("1 || 1 || 0")) == pair<int, seq_t>{ 1, seq_t("") }));
}
QUARK_UNIT_TESTQ("test_evaluate_int_expr()", "||"){
	QUARK_UT_VERIFY((test_evaluate_int_expr(seq_t("1 || 1 || 1")) == pair<int, seq_t>{ 1, seq_t("") }));
}


QUARK_UNIT_TESTQ("test_evaluate_int_expr()", ""){
	QUARK_UT_VERIFY((test_evaluate_int_expr(seq_t("10 + my_variable")) == pair<int, seq_t>{ 10, seq_t("") }));
}

QUARK_UNIT_TESTQ("test_evaluate_int_expr()", ""){
	QUARK_UT_VERIFY((test_evaluate_int_expr(seq_t("10 + \"my string\"")) == pair<int, seq_t>{ 10, seq_t("") }));
}





