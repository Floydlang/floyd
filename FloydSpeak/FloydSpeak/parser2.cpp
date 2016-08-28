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



QUARK_UNIT_TESTQ("enum class()", ""){
	enum class my_enum {
		k_one = 1,
		k_four = 4
	};

	QUARK_UT_VERIFY(my_enum::k_one == my_enum::k_one);
	QUARK_UT_VERIFY(my_enum::k_one != my_enum::k_four);
	QUARK_UT_VERIFY(static_cast<int>(my_enum::k_one) == 1);
}



seq_t skip_whitespace(const seq_t& p) {
	QUARK_ASSERT(p.check_invariant());

	return read_while(p, k_c99_whitespace_chars).second;
}



pair<std::string, seq_t> parse_string_literal(const seq_t& p){
	QUARK_ASSERT(!p.empty());
	QUARK_ASSERT(p.first_char() == '\"');

	const auto pos = p.rest();
	const auto s = read_while_not(pos, "\"");
	return { s.first, s.second.rest() };
}

QUARK_UNIT_TESTQ("parse_string_literal()", ""){
	quark::ut_compare(parse_string_literal(seq_t("\"\" xxx")), pair<std::string, seq_t>("", seq_t(" xxx")));
}

QUARK_UNIT_TESTQ("parse_string_literal()", ""){
	quark::ut_compare(parse_string_literal(seq_t("\"hello\" xxx")), pair<std::string, seq_t>("hello", seq_t(" xxx")));
}

QUARK_UNIT_TESTQ("parse_string_literal()", ""){
	quark::ut_compare(parse_string_literal(seq_t("\".5\" xxx")), pair<std::string, seq_t>(".5", seq_t(" xxx")));
}




std::pair<constant_value_t, seq_t> parse_numeric_constant(const seq_t& p) {
	QUARK_ASSERT(p.check_invariant());
	QUARK_ASSERT(k_c99_number_chars.find(p.first()) != std::string::npos);

	const auto number_pos = read_while(p, k_c99_number_chars);
	if(number_pos.first.empty()){
		throw std::runtime_error("EEE_WRONG_CHAR");
	}

	//	If it contains a "." its a float, else an int.
	if(number_pos.first.find('.') != std::string::npos){
		const auto number = parse_float(number_pos.first);
		return { constant_value_t(number), number_pos.second };
	}
	else{
		int number = atoi(number_pos.first.c_str());
		return { constant_value_t(number), number_pos.second };
	}
}

QUARK_UNIT_TESTQ("parse_numeric_constant()", ""){
	const auto a = parse_numeric_constant(seq_t("0 xxx"));
	QUARK_UT_VERIFY(a.first._type == constant_value_t::etype::k_int && a.first._int == 0);
	QUARK_UT_VERIFY(a.second.get_all() == " xxx");
}

QUARK_UNIT_TESTQ("parse_numeric_constant()", ""){
	const auto a = parse_numeric_constant(seq_t("1234 xxx"));
	QUARK_UT_VERIFY(a.first._type == constant_value_t::etype::k_int && a.first._int == 1234);
	QUARK_UT_VERIFY(a.second.get_all() == " xxx");
}

QUARK_UNIT_TESTQ("parse_numeric_constant()", ""){
	const auto a = parse_numeric_constant(seq_t("0.5 xxx"));
	QUARK_UT_VERIFY(a.first._type == constant_value_t::etype::k_float && a.first._float == 0.5f);
	QUARK_UT_VERIFY(a.second.get_all() == " xxx");
}



template<typename EXPRESSION>
struct my_helper : public maker<EXPRESSION> {


	public: virtual const EXPRESSION maker__make_identifier(const std::string& s) const{
		return 0;
	}
	public: virtual const EXPRESSION maker__make1(const eoperation op, const EXPRESSION& expr) const{
		if(op == eoperation::k_1_logical_not){
			return -expr;
		}
		else if(op == eoperation::k_1_load){
			return 0;
		}
		else{
			QUARK_ASSERT(false);
		}
	}
	public: virtual const EXPRESSION maker__make2(const eoperation op, const EXPRESSION& lhs, const EXPRESSION& rhs) const{
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
	public: virtual const EXPRESSION maker__make3(const eoperation op, const EXPRESSION& e1, const EXPRESSION& e2, const EXPRESSION& e3) const{
		if(op == eoperation::k_3_conditional_operator){
			return e1 != 0 ? e2 : e3;
		}
		else{
			QUARK_ASSERT(false);
		}
	}

	public: virtual const EXPRESSION maker__call(const EXPRESSION& f, const std::vector<EXPRESSION>& args) const{
		return 0;
	}

	public: virtual const EXPRESSION maker__member_access(const EXPRESSION& address, const std::string& member_name) const{
		return 0;
	}

	public: virtual const EXPRESSION maker__make_constant(const constant_value_t& value) const{
		if(value._type == constant_value_t::etype::k_bool){
			return 0;
		}
		else if(value._type == constant_value_t::etype::k_int){
			return value._int;
		}
		else if(value._type == constant_value_t::etype::k_float){
			return 0;
		}
		else if(value._type == constant_value_t::etype::k_string){
			return 0;
		}
		else{
			QUARK_ASSERT(false);
		}
	}

};


bool test_parse_single(const std::string& expression, int expected_value, const std::string& expected_seq){
	my_helper<int64_t> helper;
	const auto result = parse_single<int64_t>(helper, seq_t(expression));
	if(result.first != expected_value){
		return false;
	}
	else if(result.second.get_all() != expected_seq){
		return false;
	}
	return true;
}



#if false
QUARK_UNIT_TESTQ("parse_single", ""){
	QUARK_TEST_VERIFY((parse_single("truexyz") == pair<expression_t, string>(expression_t::make_constant(true), "xyz")));
}
QUARK_UNIT_TESTQ("parse_single", ""){
	QUARK_TEST_VERIFY((parse_single("falsexyz") == pair<expression_t, string>(expression_t::make_constant(false), "xyz")));
}

QUARK_UNIT_TESTQ("parse_single", ""){
	QUARK_TEST_VERIFY((parse_single("\"\"") == pair<expression_t, string>(expression_t::make_constant(""), "")));
}
QUARK_UNIT_TESTQ("parse_single", ""){
	QUARK_TEST_VERIFY((parse_single("\"abcd\"") == pair<expression_t, string>(expression_t::make_constant("abcd"), "")));
}

QUARK_UNIT_TESTQ("parse_single", "number"){
	QUARK_TEST_VERIFY((parse_single("9.0") == pair<expression_t, string>(expression_t::make_constant(9.0f), "")));
}

QUARK_UNIT_TESTQ("parse_single", "function call"){
	const auto a = parse_single("log(34.5)");
	QUARK_TEST_VERIFY(a.first._call->_function.to_string() == "log");
	QUARK_TEST_VERIFY(a.first._call->_inputs.size() == 1);
	QUARK_TEST_VERIFY(*a.first._call->_inputs[0]->_constant == value_t(34.5f));
	QUARK_TEST_VERIFY(a.second == "");
}

QUARK_UNIT_TESTQ("parse_single", "function call with two args"){
	const auto a = parse_single("log2(\"123\" + \"xyz\", 1000 * 3)");
	QUARK_TEST_VERIFY(a.first._call->_function.to_string() == "log2");
	QUARK_TEST_VERIFY(a.first._call->_inputs.size() == 2);
	QUARK_TEST_VERIFY(a.first._call->_inputs[0]->_math2);
	QUARK_TEST_VERIFY(a.first._call->_inputs[1]->_math2);
	QUARK_TEST_VERIFY(a.second == "");
}

QUARK_UNIT_TESTQ("parse_single", "nested function calls"){
	const auto a = parse_single("log2(2.1, f(3.14))");
	QUARK_TEST_VERIFY(a.first._call->_function.to_string() == "log2");
	QUARK_TEST_VERIFY(a.first._call->_inputs.size() == 2);
	QUARK_TEST_VERIFY(a.first._call->_inputs[0]->_constant);
	QUARK_TEST_VERIFY(a.first._call->_inputs[1]->_call->_function.to_string() == "f");
	QUARK_TEST_VERIFY(a.first._call->_inputs[1]->_call->_inputs.size() == 1);
	QUARK_TEST_VERIFY(*a.first._call->_inputs[1]->_call->_inputs[0] == expression_t::make_constant(3.14f));
	QUARK_TEST_VERIFY(a.second == "");
}

QUARK_UNIT_TESTQ("parse_single", "variable read"){
	const auto a = pair<expression_t, string>(expression_t::make_load_variable("k_my_global"), "");
	const auto b = parse_single("k_my_global");
	QUARK_TEST_VERIFY(a == b);
}

QUARK_UNIT_TESTQ("parse_single", "read struct member"){
	quark::ut_compare(to_seq(parse_single("k_my_global.member")),  seq(R"(["load", ["->", ["@", "k_my_global"], "member"]])", ""));
}
#endif




#if false
//### more tests here!
QUARK_UNIT_TESTQ("parse_atom", ""){
	QUARK_TEST_VERIFY((parse_atom("0.0", 0) == pair<expression_t, string>(expression_t::make_constant(0.0f), "")));
}

QUARK_UNIT_TESTQ("parse_atom", ""){
	QUARK_TEST_VERIFY((parse_atom("9.0", 0) == pair<expression_t, string>(expression_t::make_constant(9.0f), "")));
}

QUARK_UNIT_TESTQ("parse_atom", ""){
	QUARK_TEST_VERIFY((parse_atom("12345.0", 0) == pair<expression_t, string>(expression_t::make_constant(12345.0f), "")));
}

QUARK_UNIT_TESTQ("parse_atom", ""){
	QUARK_TEST_VERIFY((parse_atom("10.0", 0) == pair<expression_t, string>(expression_t::make_constant(10.0f), "")));
}

QUARK_UNIT_TESTQ("parse_atom", ""){
	QUARK_TEST_VERIFY((parse_atom("-10.0", 0) == pair<expression_t, string>(expression_t::make_constant(-10.0f), "")));
}

QUARK_UNIT_TESTQ("parse_atom", ""){
	QUARK_TEST_VERIFY((parse_atom("+10.0", 0) == pair<expression_t, string>(expression_t::make_constant( 10.0f), "")));
}

QUARK_UNIT_TESTQ("parse_atom", ""){
	QUARK_TEST_VERIFY((parse_atom("4.0+", 0) == pair<expression_t, string>(expression_t::make_constant(4.0f), "+")));
}

QUARK_UNIT_TESTQ("parse_atom", ""){
	QUARK_TEST_VERIFY((parse_atom("\"hello\"", 0) == pair<expression_t, string>(expression_t::make_constant("hello"), "")));
}
//??? check function calls with paths.
#endif




pair<int, seq_t> test_evaluate_int_expr(const seq_t& p){
	QUARK_ASSERT(p.check_invariant());

	my_helper<int64_t> helper;
	return parse_expression<int64_t>(helper, p);
}

bool test(const std::string& expression, int expected_int, string expected_seq){
	const auto result = test_evaluate_int_expr(seq_t(expression));
	if(result.first != expected_int){
		return false;
	}
	else if(result.second.get_all() != expected_seq){
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

QUARK_UNIT_1("test_evaluate_int_expr()", "?:", test("1 ? 2 : 3 xxx", 2, " xxx"));
QUARK_UNIT_1("test_evaluate_int_expr()", "?:", test("0 ? 2 : 3 xxx", 3, " xxx"));

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


QUARK_UNIT_1("test_evaluate_int_expr()", "f()", test("f()", 0, ""));
QUARK_UNIT_1("test_evaluate_int_expr()", "f(3)", test("f(3)", 0, ""));
QUARK_UNIT_1("test_evaluate_int_expr()", "f(3)", test("f(3, 4, 5)", 0, ""));





/////////////////////////////////		TO JSON

#include "json_support.h"

template<typename EXPRESSION>
struct json_helper : public maker<EXPRESSION> {


	public: virtual const EXPRESSION maker__make_identifier(const std::string& s) const{
		return json_value_t::make_array_skip_nulls({ json_value_t("@"), json_value_t(), json_value_t(s) });
	}
	public: virtual const EXPRESSION maker__make1(const eoperation op, const EXPRESSION& expr) const{
		if(op == eoperation::k_1_logical_not){
			return json_value_t::make_array_skip_nulls({ json_value_t("neg"), json_value_t(), expr });
		}
		else if(op == eoperation::k_1_load){
			return json_value_t::make_array_skip_nulls({ json_value_t("load"), json_value_t(), expr });
		}
		else{
			QUARK_ASSERT(false);
		}
	}

	private: static const std::map<eoperation, string> _2_operator_to_string;

	public: virtual const EXPRESSION maker__make2(const eoperation op, const EXPRESSION& lhs, const EXPRESSION& rhs) const{
		const auto op_str = _2_operator_to_string.at(op);
		return json_value_t::make_array2({ json_value_t(op_str), lhs, rhs });
	}
	public: virtual const EXPRESSION maker__make3(const eoperation op, const EXPRESSION& e1, const EXPRESSION& e2, const EXPRESSION& e3) const{
		if(op == eoperation::k_3_conditional_operator){
			return json_value_t::make_array2({ json_value_t("?:"), e1, e2, e3 });
		}
		else{
			QUARK_ASSERT(false);
		}
	}

	public: virtual const EXPRESSION maker__call(const EXPRESSION& f, const std::vector<EXPRESSION>& args) const{
		return json_value_t::make_array_skip_nulls({ json_value_t("call"), json_value_t(f), json_value_t(), args });
	}

	public: virtual const EXPRESSION maker__member_access(const EXPRESSION& address, const std::string& member_name) const{
		return json_value_t::make_array_skip_nulls({ json_value_t("->"), json_value_t(), address, json_value_t(member_name) });
	}

	public: virtual const EXPRESSION maker__make_constant(const constant_value_t& value) const{
		if(value._type == constant_value_t::etype::k_bool){
			return json_value_t::make_array_skip_nulls({ json_value_t("k"), json_value_t("<bool>"), json_value_t(value._bool) });
		}
		else if(value._type == constant_value_t::etype::k_int){
			return json_value_t::make_array_skip_nulls({ json_value_t("k"), json_value_t("<int>"), json_value_t((double)value._int) });
		}
		else if(value._type == constant_value_t::etype::k_float){
			return json_value_t::make_array_skip_nulls({ json_value_t("k"), json_value_t("<float>"), json_value_t(value._float) });
		}
		else if(value._type == constant_value_t::etype::k_string){
			return json_value_t::make_array_skip_nulls({ json_value_t("k"), json_value_t("<string>"), json_value_t(value._string) });
		}
		else{
			QUARK_ASSERT(false);
		}
	}
};

template<typename EXPRESSION>
const std::map<eoperation, string> json_helper<EXPRESSION>::_2_operator_to_string{
//	{ eoperation::k_2_member_access, "->" },

	{ eoperation::k_2_looup, "[-]" },

	{ eoperation::k_2_add, "+" },
	{ eoperation::k_2_subtract, "-" },
	{ eoperation::k_2_multiply, "*" },
	{ eoperation::k_2_divide, "/" },
	{ eoperation::k_2_remainder, "%" },

	{ eoperation::k_2_smaller_or_equal, "<=" },
	{ eoperation::k_2_smaller, "<" },
	{ eoperation::k_2_larger_or_equal, ">=" },
	{ eoperation::k_2_larger, ">" },

	{ eoperation::k_2_logical_equal, "==" },
	{ eoperation::k_2_logical_nonequal, "!=" },
	{ eoperation::k_2_logical_and, "&&" },
	{ eoperation::k_2_logical_or, "||" },
};



bool test__parse_single(const std::string& expression, const std::string& expected_value, const std::string& expected_seq){
	QUARK_TRACE_SS("input:" << expression);
	QUARK_TRACE_SS("expect:" << expected_value);

	json_helper<json_value_t> helper;
	const auto result = parse_single<json_value_t>(helper, seq_t(expression));
	const string json_s = json_to_compact_string(result.first);
	QUARK_TRACE_SS("result:" << json_s);
	if(json_s != expected_value){
		return false;
	}
	else if(result.second.get_all() != expected_seq){
		return false;
	}
	return true;
}

QUARK_UNIT_1("parse_single()", "identifier", test__parse_single(
	"123 xxx",
	R"(["k", "<int>", 123])",
	" xxx"
));

QUARK_UNIT_1("parse_single()", "identifier", test__parse_single(
	"123.4 xxx",
	R"(["k", "<float>", 123.4])",
	" xxx"
));

QUARK_UNIT_1("parse_single()", "identifier", test__parse_single(
	"hello xxx",
	R"(["@", "hello"])",
	" xxx"
));

QUARK_UNIT_1("parse_single()", "identifier", test__parse_single(
	"\"world!\" xxx",
	R"(["k", "<string>", "world!"])",
	" xxx"
));




bool test__parse_expression(const std::string& expression, const std::string& expected_value, const std::string& expected_seq){
	QUARK_TRACE_SS("input:" << expression);
	QUARK_TRACE_SS("expect:" << expected_value);

	json_helper<json_value_t> helper;
	const auto result = parse_expression<json_value_t>(helper, seq_t(expression));
	const string json_s = json_to_compact_string(result.first);
	QUARK_TRACE_SS("result:" << json_s);
	if(json_s != expected_value){
		return false;
	}
	else if(result.second.get_all() != expected_seq){
		return false;
	}
	return true;
}

QUARK_UNIT_1("parse_expression()", "||", test__parse_expression(
	"1 || 0 || 1",
	R"(["||", ["||", ["k", "<int>", 1], ["k", "<int>", 0]], ["k", "<int>", 1]])",
	""
));

//??? Change all int-tests to json tests.



QUARK_UNIT_1("parse_expression()", "identifier", test__parse_expression(
	"hello xxx",
	R"(["@", "hello"])",
	" xxx"
));

QUARK_UNIT_1("parse_expression()", "struct member access", test__parse_expression(
	"hello.kitty xxx",
	R"(["->", ["@", "hello"], "kitty"])",
	" xxx"
));

QUARK_UNIT_1("parse_expression()", "struct member access", test__parse_expression(
	"hello.kitty.cat xxx",
	R"(["->", ["->", ["@", "hello"], "kitty"], "cat"])",
	" xxx"
));

QUARK_UNIT_1("parse_expression()", "struct member access -- whitespace", test__parse_expression(
	"hello . kitty . cat xxx",
	R"(["->", ["->", ["@", "hello"], "kitty"], "cat"])",
	" xxx"
));




QUARK_UNIT_1("parse_expression()", "function call", test__parse_expression(
	"f() xxx",
	R"(["call", ["@", "f"], []])", " xxx"
));

QUARK_UNIT_1("parse_expression()", "function call, one simple arg", test__parse_expression(
	"f(3)",
	R"(["call", ["@", "f"], [["k", "<int>", 3]]])",
	""
));

QUARK_UNIT_1("parse_expression()", "call with expression-arg", test__parse_expression(
	"f(x+10) xxx",
	R"(["call", ["@", "f"], [["+", ["@", "x"], ["k", "<int>", 10]]]])",
	" xxx"
));
QUARK_UNIT_1("parse_expression()", "call with expression-arg", test__parse_expression(
	"f(1,2) xxx",
	R"(["call", ["@", "f"], [["k", "<int>", 1], ["k", "<int>", 2]]])",
	" xxx"
));
QUARK_UNIT_1("parse_expression()", "call with expression-arg -- whitespace", test__parse_expression(
	"f ( 1 , 2 ) xxx",
	R"(["call", ["@", "f"], [["k", "<int>", 1], ["k", "<int>", 2]]])",
	" xxx"
));

QUARK_UNIT_1("parse_expression()", "function call", test__parse_expression(
	"poke.mon.f() xxx",
	R"(["call", ["->", ["->", ["@", "poke"], "mon"], "f"], []])", " xxx"
));

QUARK_UNIT_1("parse_expression()", "function call", test__parse_expression(
	"f().g() xxx",
	R"(["call", ["->", ["call", ["@", "f"], []], "g"], []])", " xxx"
));



QUARK_UNIT_1("parse_expression()", "complex chain", test__parse_expression(
	"hello[\"troll\"].kitty[10].cat xxx",
	R"(["->", ["[-]", ["->", ["[-]", ["@", "hello"], ["k", "<string>", "troll"]], "kitty"], ["k", "<int>", 10]], "cat"])",
	" xxx"
));


QUARK_UNIT_1("parse_expression()", "chain", test__parse_expression(
	"poke.mon.v[10].a.b.c[\"three\"] xxx",
	R"(["[-]", ["->", ["->", ["->", ["[-]", ["->", ["->", ["@", "poke"], "mon"], "v"], ["k", "<int>", 10]], "a"], "b"], "c"], ["k", "<string>", "three"]])",
	" xxx"
));


QUARK_UNIT_1("parse_expression()", "function call with expression-args", test__parse_expression(
	"f(3 + 4, 4 * g(1000 + 2345), \"hello\", 5)",
	R"(["call", ["@", "f"], [["+", ["k", "<int>", 3], ["k", "<int>", 4]], ["*", ["k", "<int>", 4], ["call", ["@", "g"], [["+", ["k", "<int>", 1000], ["k", "<int>", 2345]]]]], ["k", "<string>", "hello"], ["k", "<int>", 5]]])",
	""
));




QUARK_UNIT_1("parse_expression()", "lookup with int", test__parse_expression(
	"hello[10] xxx",
	R"(["[-]", ["@", "hello"], ["k", "<int>", 10]])", " xxx"
));

QUARK_UNIT_1("parse_expression()", "lookup with string", test__parse_expression(
	"hello[\"troll\"] xxx",
	R"(["[-]", ["@", "hello"], ["k", "<string>", "troll"]])", " xxx"
));

QUARK_UNIT_1("parse_expression()", "lookup with string -- whitespace", test__parse_expression(
	"hello [ \"troll\" ] xxx",
	R"(["[-]", ["@", "hello"], ["k", "<string>", "troll"]])", " xxx"
));




QUARK_UNIT_1("parse_expression()", "?:", test__parse_expression(
	"1 ? 2 : 3 xxx",
	R"(["?:", ["k", "<int>", 1], ["k", "<int>", 2], ["k", "<int>", 3]])", " xxx"
));

QUARK_UNIT_1("parse_expression()", "?:", test__parse_expression(
	"1 ? \"true!!!\" : \"false!!!\" xxx",
	R"(["?:", ["k", "<int>", 1], ["k", "<string>", "true!!!"], ["k", "<string>", "false!!!"]])", " xxx"
));

QUARK_UNIT_1("parse_expression()", "?:", test__parse_expression(
	"1 + 2 ? 3 + 4 : 5 + 6 xxx",
	R"(["?:", ["+", ["k", "<int>", 1], ["k", "<int>", 2]], ["+", ["k", "<int>", 3], ["k", "<int>", 4]], ["+", ["k", "<int>", 5], ["k", "<int>", 6]]])", " xxx"
));

//??? Add more test to see precedence works as it should!


#if false
QUARK_UNIT_TESTQ("parse_expression()", ""){
	quark::ut_compare(expression_to_json_string(parse_expression("input_flag ? 100 + 10 * 2 : 1000 - 3 * 4")), R"(["load", ["->", ["@", "pixel"], "red"]])");
}

QUARK_UNIT_TESTQ("parse_expression()", ""){
	quark::ut_compare(expression_to_json_string(parse_expression("input_flag ? \"123\" : \"456\"")), R"(["load", ["->", ["@", "pixel"], "red"]])");
}
#endif




