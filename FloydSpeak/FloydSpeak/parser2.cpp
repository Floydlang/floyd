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
#include <map>
#include "text_parser.h"
#include "floyd_basics.h"


#include "parser2.h"

using namespace std;
using floyd::keyword_t;



namespace parser2 {




QUARK_UNIT_TESTQ("C++ operators", ""){
	const int a = 2-+-2;
	QUARK_TEST_VERIFY(a == 4);
}



QUARK_UNIT_TESTQ("C++ enum class()", ""){
	enum class my_enum {
		k_one = 1,
		k_four = 4
	};

	QUARK_UT_VERIFY(my_enum::k_one == my_enum::k_one);
	QUARK_UT_VERIFY(my_enum::k_one != my_enum::k_four);
	QUARK_UT_VERIFY(static_cast<int>(my_enum::k_one) == 1);
}


bool is_valid_expr_chars(const std::string& s){
	const auto allowed = k_c99_identifier_chars + k_c99_number_chars + k_c99_whitespace_chars + "+-*/%" + "\"[]().?:=!<>&,|#$";
	for(auto i = 0 ; i < s.size() ; i++){
		const char ch = s[i];
		if(allowed.find(ch) == string::npos){
			return false;
		}
	}
	return true;
}



seq_t skip_expr_whitespace(const seq_t& p) {
	QUARK_ASSERT(p.check_invariant());

	return read_while(p, k_c99_whitespace_chars).second;
}

/*
	()
	(100)
	(100, 200)
	(get_first() * 3, 4)
*/
std::pair<std::vector<expr_t>, seq_t> parse_bounded_list(const seq_t& p1, const std::string& start_char, const std::string& end_char){
	QUARK_ASSERT(p1.check_invariant());
	QUARK_ASSERT(p1.first() == start_char);

	const auto p = p1;
	const auto pos3 = skip_expr_whitespace(p.rest());

	//	No arguments.
	if(pos3.first() == end_char){
		return {{}, pos3.rest() };
	}
	//	1-many arguments.
	else{
		auto pos_loop = skip_expr_whitespace(pos3);
		std::vector<expr_t> arg_exprs;
		bool more = true;
		while(more){
			const auto a = parse_expression_int(pos_loop, eoperator_precedence::k_super_weak);
			arg_exprs.push_back(a.first);

			const auto pos5 = skip_expr_whitespace(a.second);
			const auto ch = pos5.first();
			if(ch == ","){
				more = true;
			}
			else if(ch == end_char){
				more = false;
			}
			else{
				throw std::runtime_error("Unexpected char");
			}
			pos_loop = pos5.rest();
		}
		return { arg_exprs, pos_loop };
	}
}


pair<std::string, seq_t> parse_string_literal(const seq_t& p){
	QUARK_ASSERT(!p.empty());
	QUARK_ASSERT(p.first1_char() == '\"');

	const auto pos = p.rest();
	const auto s = read_until(pos, "\"");
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
	QUARK_UT_VERIFY(a.second.get_s() == " xxx");
}

QUARK_UNIT_TESTQ("parse_numeric_constant()", ""){
	const auto a = parse_numeric_constant(seq_t("1234 xxx"));
	QUARK_UT_VERIFY(a.first._type == constant_value_t::etype::k_int && a.first._int == 1234);
	QUARK_UT_VERIFY(a.second.get_s() == " xxx");
}

QUARK_UNIT_TESTQ("parse_numeric_constant()", ""){
	const auto a = parse_numeric_constant(seq_t("0.5 xxx"));
	QUARK_UT_VERIFY(a.first._type == constant_value_t::etype::k_float && a.first._float == 0.5f);
	QUARK_UT_VERIFY(a.second.get_s() == " xxx");
}




/*
	Generates expressions encode as JSON in std::string values. Use for testing.
*/

const expr_t maker__make_identifier(const std::string& s){
	return expr_t{ eoperation::k_0_resolve, {}, {}, s };
}

const expr_t maker__make1(const eoperation op, const expr_t& expr){
	return expr_t{ op, { expr }, {}, "" };
}

const expr_t maker__make2(const eoperation op, const expr_t& lhs, const expr_t& rhs){
	return expr_t{ op, { lhs, rhs }, {}, "" };
}

const expr_t maker__make3(const eoperation op, const expr_t& e1, const expr_t& e2, const expr_t& e3){
	return expr_t{ op, { e1, e2, e3 }, {}, "" };
}

const expr_t maker__call(const expr_t& f, const std::vector<expr_t>& args){
	std::vector<expr_t> exprs;
	exprs.push_back(f);
	exprs.insert(exprs.end(), args.begin(), args.end());
	return expr_t{ eoperation::k_n_call, exprs, {}, "" };
}

const expr_t maker_vector_definition(const std::string& element_type, const std::vector<expr_t>& elements){
	return expr_t{ eoperation::k_1_vector_definition, elements, {}, element_type };
}

const expr_t maker__member_access(const expr_t& address, const std::string& member_name){
	return expr_t{ eoperation::k_x_member_access, { address }, {}, member_name };
}

const expr_t maker__make_constant(const constant_value_t& value){
	if(value._type == constant_value_t::etype::k_string){
		return expr_t{ eoperation::k_0_string_literal, {}, std::make_shared<constant_value_t>(value), "" };
	}
	else{
		return expr_t{ eoperation::k_0_number_constant, {}, std::make_shared<constant_value_t>(value), "" };
	}
}


std::pair<expr_t, seq_t> parse_optional_operation_rightward(const seq_t& p0, const expr_t& lhs, const eoperator_precedence precedence){
	QUARK_ASSERT(p0.check_invariant());

	const auto p = skip_expr_whitespace(p0);
	if(p.empty()){
		return { lhs, p0 };
	}
	else {
		const auto op1 = p.first();
		const auto op2 = p.first(2);

		//	Detect end of chain. Notice that we leave the ")" or "]".
		if(op1 == ")" && precedence > eoperator_precedence::k_parentesis){
			return { lhs, p0 };
		}

		else if(op1 == "]" && precedence > eoperator_precedence::k_parentesis){
			return { lhs, p0 };
		}

		//	lhs OPERATOR rhs
		else {
			//	Function call
			//	EXPRESSION (EXPRESSION +, EXPRESSION)
			if(op1 == "(" && precedence > eoperator_precedence::k_function_call){
				const auto a_pos = parse_bounded_list(p, "(", ")");
				const auto call = maker__call(lhs, a_pos.first);
				return parse_optional_operation_rightward(a_pos.second, call, precedence);
			}

			//	Member access
			//	EXPRESSION "." EXPRESSION +
			else if(op1 == "."  && precedence > eoperator_precedence::k_member_access){
				const auto identifier_s = read_while(skip_expr_whitespace(p.rest()), k_c99_identifier_chars);
				if(identifier_s.first.empty()){
					throw std::runtime_error("Expected ')'");
				}
				const auto value2 = maker__member_access(lhs, identifier_s.first);

				return parse_optional_operation_rightward(identifier_s.second, value2, precedence);
			}

			//	Lookup / subscription
			//	EXPRESSION "[" EXPRESSION "]" +
			else if(op1 == "["  && precedence > eoperator_precedence::k_looup){
				const auto p2 = skip_expr_whitespace(p.rest());
				const auto key = parse_expression_int(p2, eoperator_precedence::k_super_weak);
				const auto result = maker__make2(eoperation::k_2_looup, lhs, key.first);
				const auto p3 = skip_expr_whitespace(key.second);

				// Closing "]".
				if(p3.first() != "]"){
					throw std::runtime_error("Expected closing \"]\"");
				}
				return parse_optional_operation_rightward(p3.rest(), result, precedence);
			}

			//	EXPRESSION "+" EXPRESSION
			else if(op1 == "+"  && precedence > eoperator_precedence::k_add_sub){
				const auto rhs = parse_expression_int(p.rest(), eoperator_precedence::k_add_sub);
				const auto value2 = maker__make2(eoperation::k_2_add, lhs, rhs.first);
				return parse_optional_operation_rightward(rhs.second, value2, precedence);
			}

			//	EXPRESSION "-" EXPRESSION
			else if(op1 == "-" && precedence > eoperator_precedence::k_add_sub){
				const auto rhs = parse_expression_int(p.rest(), eoperator_precedence::k_add_sub);
				const auto value2 = maker__make2(eoperation::k_2_subtract, lhs, rhs.first);
				return parse_optional_operation_rightward(rhs.second, value2, precedence);
			}

			//	EXPRESSION "*" EXPRESSION
			else if(op1 == "*" && precedence > eoperator_precedence::k_multiply_divider_remainder) {
				const auto rhs = parse_expression_int(p.rest(), eoperator_precedence::k_multiply_divider_remainder);
				const auto value2 = maker__make2(eoperation::k_2_multiply, lhs, rhs.first);
				return parse_optional_operation_rightward(rhs.second, value2, precedence);
			}
			//	EXPRESSION "/" EXPRESSION
			else if(op1 == "/" && precedence > eoperator_precedence::k_multiply_divider_remainder) {
				const auto rhs = parse_expression_int(p.rest(), eoperator_precedence::k_multiply_divider_remainder);
				const auto value2 = maker__make2(eoperation::k_2_divide, lhs, rhs.first);
				return parse_optional_operation_rightward(rhs.second, value2, precedence);
			}

			//	EXPRESSION "%" EXPRESSION
			else if(op1 == "%" && precedence > eoperator_precedence::k_multiply_divider_remainder) {
				const auto rhs = parse_expression_int(p.rest(), eoperator_precedence::k_multiply_divider_remainder);
				const auto value2 = maker__make2(eoperation::k_2_remainder, lhs, rhs.first);
				return parse_optional_operation_rightward(rhs.second, value2, precedence);
			}


			//	EXPRESSION "?" EXPRESSION ":" EXPRESSION
			else if(op1 == "?" && precedence > eoperator_precedence::k_comparison_operator) {
				const auto true_expr_p = parse_expression_int(p.rest(), eoperator_precedence::k_comparison_operator);

				const auto pos2 = skip_expr_whitespace(true_expr_p.second);
				const auto colon = pos2.first();
				if(colon != ":"){
					throw std::runtime_error("Expected \":\"");
				}

				const auto false_expr_p = parse_expression_int(pos2.rest(), precedence);
				const auto value2 = maker__make3(eoperation::k_3_conditional_operator, lhs, true_expr_p.first, false_expr_p.first);
				return parse_optional_operation_rightward(false_expr_p.second, value2, precedence);
			}


			//	EXPRESSION "==" EXPRESSION
			else if(op2 == "==" && precedence > eoperator_precedence::k_equal__not_equal){
				const auto rhs = parse_expression_int(p.rest(2), eoperator_precedence::k_equal__not_equal);
				const auto value2 = maker__make2(eoperation::k_2_logical_equal, lhs, rhs.first);
				return parse_optional_operation_rightward(rhs.second, value2, precedence);
			}
			//	EXPRESSION "!=" EXPRESSION
			else if(op2 == "!=" && precedence > eoperator_precedence::k_equal__not_equal){
				const auto rhs = parse_expression_int(p.rest(2), eoperator_precedence::k_equal__not_equal);
				const auto value2 = maker__make2(eoperation::k_2_logical_nonequal, lhs, rhs.first);
				return parse_optional_operation_rightward(rhs.second, value2, precedence);
			}

			//	!!! Check for "<=" before we check for "<".
			//	EXPRESSION "<=" EXPRESSION
			else if(op2 == "<=" && precedence > eoperator_precedence::k_larger_smaller){
				const auto rhs = parse_expression_int(p.rest(2), eoperator_precedence::k_larger_smaller);
				const auto value2 = maker__make2(eoperation::k_2_smaller_or_equal, lhs, rhs.first);
				return parse_optional_operation_rightward(rhs.second, value2, precedence);
			}

			//	EXPRESSION "<" EXPRESSION
			else if(op1 == "<" && precedence > eoperator_precedence::k_larger_smaller){
				const auto rhs = parse_expression_int(p.rest(2), eoperator_precedence::k_larger_smaller);
				const auto value2 = maker__make2(eoperation::k_2_smaller, lhs, rhs.first);
				return parse_optional_operation_rightward(rhs.second, value2, precedence);
			}


			//	!!! Check for ">=" before we check for ">".
			//	EXPRESSION ">=" EXPRESSION
			else if(op2 == ">=" && precedence > eoperator_precedence::k_larger_smaller){
				const auto rhs = parse_expression_int(p.rest(2), eoperator_precedence::k_larger_smaller);
				const auto value2 = maker__make2(eoperation::k_2_larger_or_equal, lhs, rhs.first);
				return parse_optional_operation_rightward(rhs.second, value2, precedence);
			}

			//	EXPRESSION ">" EXPRESSION
			else if(op1 == ">" && precedence > eoperator_precedence::k_larger_smaller){
				const auto rhs = parse_expression_int(p.rest(2), eoperator_precedence::k_larger_smaller);
				const auto value2 = maker__make2(eoperation::k_2_larger, lhs, rhs.first);
				return parse_optional_operation_rightward(rhs.second, value2, precedence);
			}


			//	EXPRESSION "&&" EXPRESSION
			else if(op2 == "&&" && precedence > eoperator_precedence::k_logical_and){
				const auto rhs = parse_expression_int(p.rest(2), eoperator_precedence::k_logical_and);
				const auto value2 = maker__make2(eoperation::k_2_logical_and, lhs, rhs.first);
				return parse_optional_operation_rightward(rhs.second, value2, precedence);
			}

			//	EXPRESSION "||" EXPRESSION
			else if(op2 == "||" && precedence > eoperator_precedence::k_logical_or){
				const auto rhs = parse_expression_int(p.rest(2), eoperator_precedence::k_logical_or);
				const auto value2 = maker__make2(eoperation::k_2_logical_or, lhs, rhs.first);
				return parse_optional_operation_rightward(rhs.second, value2, precedence);
			}

			//	EXPRESSION
			//	Nope, no operation -- just use the lhs expression on its own.
			else{
				return { lhs, p0 };
			}
		}
	}
}



std::pair<expr_t, seq_t> parse_terminal(const seq_t& p0) {
	QUARK_ASSERT(p0.check_invariant());

	const auto p = skip_expr_whitespace(p0);

	//	String literal?
	if(p.first1() == "\""){
		const auto s = read_until(p.rest1(), "\"");
		const auto result = maker__make_constant(constant_value_t(s.first));
		return { result, s.second.rest1() };
	}

	//	Number constant?
	// [0-9] and "."  => numeric constant.
	else if(k_c99_number_chars.find(p.first1()) != std::string::npos){
		const auto value_p = parse_numeric_constant(p);
		const auto result = maker__make_constant(value_p.first);
		return { result, value_p.second };
	}

	else if(if_first(p, keyword_t::k_true).first){
		const auto result = maker__make_constant(constant_value_t(true));
		return { result, if_first(p, keyword_t::k_true).second };
	}

	else if(if_first(p, keyword_t::k_false).first){
		const auto result = maker__make_constant(constant_value_t(false));
		return { result, if_first(p, keyword_t::k_false).second };
	}

	//	Identifier?
	{
		const auto identifier_s = read_while(p, k_c99_identifier_chars);
		if(!identifier_s.first.empty()){
			const auto resolve = maker__make_identifier(identifier_s.first);
			return { resolve, identifier_s.second };
		}
	}

	throw std::runtime_error("Expected constant or identifier.");
}

std::pair<expr_t, seq_t> parse_lhs_atom(const seq_t& p){
	QUARK_ASSERT(p.check_invariant());

    const auto p2 = skip_expr_whitespace(p);
	if(p2.empty()){
		throw std::runtime_error("Unexpected end of string");
	}

	const char ch1 = p2.first1_char();

	//	Negate? "-xxx"
	if(ch1 == '-'){
		const auto a = parse_expression_int(p2.rest1(), eoperator_precedence::k_super_strong);
		const auto value2 = maker__make1(eoperation::k_1_unary_minus, a.first);
		return { value2, a.second };
	}
	else if(ch1 == '+'){
		const auto a = parse_expression_int(p2.rest1(), eoperator_precedence::k_super_strong);
		return a;
	}
	//	Expression within parantheses?
	//	(EXPRESSION)xxx"
	else if(ch1 == '('){
		const auto a = parse_expression_int(p2.rest1(), eoperator_precedence::k_super_weak);
		const auto p3 = skip_expr_whitespace(a.second);
		if (p3.first() != ")"){
			throw std::runtime_error("Expected ')'");
		}
		return { a.first, p3.rest() };
	}

	//	Vector definition "[ 1, 2, 3 ]", "[ calc_pi(), 2.8, calc_pi * 2.0]"
	//	"[ EXPRESSION, EXPRESSION... ]"
	else if(ch1 == '['){
		const auto a = parse_bounded_list(p2, "[", "]");
		const auto result = maker_vector_definition("", a.first);
		return {result, a.second };
	}

	//	Single constant number, string literal, function call, variable access, lookup or member access. Can be a chain.
	//	"1234xxx" or "my_function(3)xxx"
	else {
		const auto a = parse_terminal(p2);
		return a;
	}
}

QUARK_UNIT_TESTQ("parse_lhs_atom()", ""){
	const auto a = parse_lhs_atom(seq_t("3"));
	QUARK_UT_VERIFY(a.first == maker__make_constant(constant_value_t(3)));
}
#if false
QUARK_UNIT_TESTQ("parse_lhs_atom()", ""){
	const auto a = parse_lhs_atom(seq_t("[3]"));
	QUARK_UT_VERIFY(a.first == maker__make_constant(constant_value_t(3)));
}
#endif




std::pair<expr_t, seq_t> parse_expression_int(const seq_t& p, const eoperator_precedence precedence){
	QUARK_ASSERT(p.check_invariant());

	auto lhs = parse_lhs_atom(p);
	return parse_optional_operation_rightward(lhs.second, lhs.first, precedence);
}


std::pair<expr_t, seq_t> parse_expression(const seq_t& p){
	if(!is_valid_expr_chars(p.get_s())){
		throw std::runtime_error("Illegal characters.");
	}
	return parse_expression_int(p, eoperator_precedence::k_super_weak);
}



//////////////////////			expr_to_string


/*
	Convert expr_t to a json-formatted string, for easy testing.
*/

static const std::map<eoperation, string> k_2_operator_to_string{
//	{ eoperation::k_x_member_access, "->" },

	{ eoperation::k_2_looup, "[]" },

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


	static string make2(string s0, string s1){
		std::ostringstream ss;
		ss << "[" << s0 << ", " << s1 << "]";
		return ss.str();
	}

	static string make3(string s0, string s1, string s2){
		std::ostringstream ss;
		ss << "[" << s0 << ", " << s1 << ", " << s2 << "]";
		return ss.str();
	}

	std::string expr_to_string(const expr_t& e);

	static string op_to_string(eoperation op, const expr_t& expr0, const expr_t& expr1){
		const auto op_string = quote(k_2_operator_to_string.at(op));
		const auto expr0_str = expr_to_string(expr0);
		const auto expr1_str = expr_to_string(expr1);

		std::ostringstream ss;
		ss << "[" << op_string << ", " << expr0_str << ", " << expr1_str << "]";
		return ss.str();
	}

std::string expr_to_string(const expr_t& e){
	if(e._op == eoperation::k_0_number_constant){
		const auto value = *e._constant;
		if(value._type == constant_value_t::etype::k_bool){
			return make3("\"k\"", "\"bool\"", value._bool ? keyword_t::k_true : keyword_t::k_false);
		}
		else if(value._type == constant_value_t::etype::k_int){
			return make3("\"k\"", "\"int\"", std::to_string(value._int));
		}
		else if(value._type == constant_value_t::etype::k_float){
			return make3("\"k\"", "\"float\"", float_to_string(value._float));
		}
		else if(value._type == constant_value_t::etype::k_string){
			//	 Use k_0_string_literal!
			QUARK_ASSERT(false);
		}
		else{
			QUARK_ASSERT(false);
		}
	}
	else if(e._op == eoperation::k_0_resolve){
		return make2(quote("@"), quote(e._identifier));
	}
	else if(e._op == eoperation::k_0_string_literal){
		const auto value = *e._constant;
		QUARK_ASSERT(value._type == constant_value_t::etype::k_string);
		return make3("\"k\"", "\"string\"", quote(value._string));
	}
	else if(e._op == eoperation::k_x_member_access){
		return make3(quote("->"), expr_to_string(e._exprs[0]), quote(e._identifier));
	}
	else if(e._op == eoperation::k_2_looup){
		return op_to_string(e._op, e._exprs[0], e._exprs[1]);
	}
	else if(e._op == eoperation::k_2_add){
		return op_to_string(e._op, e._exprs[0], e._exprs[1]);
	}
	else if(e._op == eoperation::k_2_subtract){
		return op_to_string(e._op, e._exprs[0], e._exprs[1]);
	}
	else if(e._op == eoperation::k_2_multiply){
		return op_to_string(e._op, e._exprs[0], e._exprs[1]);
	}
	else if(e._op == eoperation::k_2_divide){
		return op_to_string(e._op, e._exprs[0], e._exprs[1]);
	}
	else if(e._op == eoperation::k_2_remainder){
		return op_to_string(e._op, e._exprs[0], e._exprs[1]);
	}
	else if(e._op == eoperation::k_2_smaller_or_equal){
		return op_to_string(e._op, e._exprs[0], e._exprs[1]);
	}
	else if(e._op == eoperation::k_2_smaller){
		return op_to_string(e._op, e._exprs[0], e._exprs[1]);
	}
	else if(e._op == eoperation::k_2_larger_or_equal){
		return op_to_string(e._op, e._exprs[0], e._exprs[1]);
	}
	else if(e._op == eoperation::k_2_larger){
		return op_to_string(e._op, e._exprs[0], e._exprs[1]);
	}
	else if(e._op == eoperation::k_2_logical_equal){
		return op_to_string(e._op, e._exprs[0], e._exprs[1]);
	}
	else if(e._op == eoperation::k_2_logical_nonequal){
		return op_to_string(e._op, e._exprs[0], e._exprs[1]);
	}
	else if(e._op == eoperation::k_2_logical_and){
		return op_to_string(e._op, e._exprs[0], e._exprs[1]);
	}
	else if(e._op == eoperation::k_2_logical_or){
		return op_to_string(e._op, e._exprs[0], e._exprs[1]);
	}
	else if(e._op == eoperation::k_3_conditional_operator){
		std::ostringstream ss;
		ss << "[\"?:\", " << expr_to_string(e._exprs[0]) << ", " << expr_to_string(e._exprs[1]) << ", " << expr_to_string(e._exprs[2]) + "]";
		return ss.str();
	}
	else if(e._op == eoperation::k_n_call){
		std::ostringstream ss;
		ss << "[\"call\", " + expr_to_string(e._exprs[0]) + ", [";
		for(auto i = 0 ; i < e._exprs.size() - 1 ; i++){
			const auto& arg = expr_to_string(e._exprs[i + 1]);
			ss << arg;
			if(i != (e._exprs.size() - 2)){
				ss << ", ";
			}
		}
		ss << "]]";
		return ss.str();
	}
	else if(e._op == eoperation::k_1_unary_minus){
		return "[\"unary_minus\", " + expr_to_string(e._exprs[0]) + "]";
	}
	//	["vector-def", ELEMENT-TYPE, [ ELEMENT, ELEMENT, ...]]
	else if(e._op == eoperation::k_1_vector_definition){
		std::ostringstream ss;
		ss << "[\"vector-def\", " << "\"" << e._identifier + "\"" << ", [";
		for(auto i = 0 ; i < e._exprs.size() ; i++){
			const auto& arg = expr_to_string(e._exprs[i]);
			ss << arg;
			if(i != (e._exprs.size() - 1)){
				ss << ", ";
			}
		}
		ss << "]]";
		return ss.str();
	}
	else{
		QUARK_ASSERT(false)
		return "";
	}
}



bool test__parse_terminal(const std::string& expression, const std::string& expected_value, const std::string& expected_seq){
	QUARK_TRACE_SS("input:" << expression);
	QUARK_TRACE_SS("expect:" << expected_value);

	const auto result = parse_terminal(seq_t(expression));
	const string json_s = expr_to_string(result.first);
	QUARK_TRACE_SS("result:" << json_s);
	if(json_s != expected_value){
		return false;
	}
	else if(result.second.get_s() != expected_seq){
		return false;
	}
	return true;
}


QUARK_UNIT_1("parse_terminal()", "identifier", test__parse_terminal(
	"123 xxx",
	R"(["k", "int", 123])",
	" xxx"
));

QUARK_UNIT_1("parse_terminal()", "identifier", test__parse_terminal(
	"123.5 xxx",
	R"(["k", "float", 123.5])",
	" xxx"
));

QUARK_UNIT_1("parse_terminal()", "identifier", test__parse_terminal(
	"0.0 xxx",
	R"(["k", "float", 0])",
	" xxx"
));

QUARK_UNIT_1("parse_terminal()", "identifier", test__parse_terminal(
	"hello xxx",
	R"(["@", "hello"])",
	" xxx"
));

QUARK_UNIT_1("parse_terminal()", "identifier", test__parse_terminal(
	"\"world!\" xxx",
	R"(["k", "string", "world!"])",
	" xxx"
));

QUARK_UNIT_1("parse_terminal()", "identifier", test__parse_terminal(
	"\"\" xxx",
	R"(["k", "string", ""])",
	" xxx"
));


QUARK_UNIT_1("parse_terminal()", "identifier", test__parse_terminal(
	"true xxx",
	R"(["k", "bool", true])",
	" xxx"
));
QUARK_UNIT_1("parse_terminal()", "identifier", test__parse_terminal(
	"false xxx",
	R"(["k", "bool", false])",
	" xxx"
));




std::pair<std::string, seq_t> test_parse_expression2(const seq_t& expression){
	const auto result = parse_expression(seq_t(expression));
	return { expr_to_string(result.first), result.second };
}


bool test__parse_expression(const std::string& expression, string expected_value, string expected_seq){
	QUARK_TRACE_SS("input:" << expression);
	QUARK_TRACE_SS("expect:" << expected_value);

	const auto result = test_parse_expression2(seq_t(expression));
	QUARK_TRACE_SS("result:" << result.first);
	if(result.first != expected_value){
		return false;
	}
	else if(result.second.get_s() != expected_seq){
		return false;
	}
	return true;
}



//////////////////////////////////			EMPTY

QUARK_UNIT_TESTQ("parse_expression()", ""){
	try{
		test__parse_expression("", "", "");
		QUARK_UT_VERIFY(false);
	}
	catch(...){
	}
}

//////////////////////////////////			CONSTANTS

QUARK_UNIT_1("parse_expression()", "", test__parse_expression("0", "[\"k\", \"int\", 0]", ""));
QUARK_UNIT_1("parse_expression()", "", test__parse_expression("0 xxx", "[\"k\", \"int\", 0]", " xxx"));
QUARK_UNIT_1("parse_expression()", "", test__parse_expression("1234567890", "[\"k\", \"int\", 1234567890]", ""));
QUARK_UNIT_1("parse_expression()", "", test__parse_expression("\"hello, world!\"", "[\"k\", \"string\", \"hello, world!\"]", ""));


//////////////////////////////////			ARITHMETICS

QUARK_UNIT_1("parse_expression()", "arithmetics", test__parse_expression(
	"10 + 4",
	R"(["+", ["k", "int", 10], ["k", "int", 4]])",
	""
));
QUARK_UNIT_1("parse_expression()", "arithmetics", test__parse_expression(
	"1 + 2 + 3 + 4",
	R"(["+", ["+", ["+", ["k", "int", 1], ["k", "int", 2]], ["k", "int", 3]], ["k", "int", 4]])",
	""
));

QUARK_UNIT_1("parse_expression()", "arithmetics", test__parse_expression(
	"10 * 4",
	R"(["*", ["k", "int", 10], ["k", "int", 4]])",
	""
));
QUARK_UNIT_1("parse_expression()", "arithmetics", test__parse_expression(
	"10 * 4 * 3",
	R"(["*", ["*", ["k", "int", 10], ["k", "int", 4]], ["k", "int", 3]])",
	""
));
QUARK_UNIT_1("parse_expression()", "arithmetics", test__parse_expression(
	"40 / 4",
	R"(["/", ["k", "int", 40], ["k", "int", 4]])",
	""
));

QUARK_UNIT_1("parse_expression()", "arithmetics", test__parse_expression(
	"40 / 5 / 2",
	R"(["/", ["/", ["k", "int", 40], ["k", "int", 5]], ["k", "int", 2]])",
	""
));

QUARK_UNIT_1("parse_expression()", "arithmetics", test__parse_expression(
	"41 % 5",
	R"(["%", ["k", "int", 41], ["k", "int", 5]])",
	""
));
QUARK_UNIT_1("parse_expression()", "arithmetics", test__parse_expression(
	"413 % 50 % 10",
	R"(["%", ["%", ["k", "int", 413], ["k", "int", 50]], ["k", "int", 10]])",
	""
));

QUARK_UNIT_1("parse_expression()", "arithmetics", test__parse_expression(
	"1 + 3 * 2 + 100",
	R"(["+", ["+", ["k", "int", 1], ["*", ["k", "int", 3], ["k", "int", 2]]], ["k", "int", 100]])",
	""
));

QUARK_UNIT_1("parse_expression()", "arithmetics", test__parse_expression(
	"1 + 8 + 7 + 2 * 3 + 4 * 5 + 6",
	R"(["+", ["+", ["+", ["+", ["+", ["k", "int", 1], ["k", "int", 8]], ["k", "int", 7]], ["*", ["k", "int", 2], ["k", "int", 3]]], ["*", ["k", "int", 4], ["k", "int", 5]]], ["k", "int", 6]])",
""
));



//////////////////////////////////			parantheses

QUARK_UNIT_1("parse_expression()", "parantheses", test__parse_expression("(3)", "[\"k\", \"int\", 3]", ""));
QUARK_UNIT_1("parse_expression()", "parantheses", test__parse_expression("(3 * 8)", R"(["*", ["k", "int", 3], ["k", "int", 8]])", ""));

/*
	["-",
		["+",
			["*", ["k", "int", 3], 	["k", "int", 2]],
			["*", ["k", "int", 8], ["k", "int", 2]]
		],
		["*",["k", "int", 1], ["k", "int", 2]]
	]
*/

QUARK_UNIT_1("parse_expression()", "parantheses", test__parse_expression(
	"(3 * 2 + (8 * 2)) - (((1))) * 2",
	R"(["-", ["+", ["*", ["k", "int", 3], ["k", "int", 2]], ["*", ["k", "int", 8], ["k", "int", 2]]], ["*", ["k", "int", 1], ["k", "int", 2]]])",
	""
));




//////////////////////////////////			vector definition

//	"[\"vector-def\", \"\", [[\"k\", \"int\", 1], [\"k\", \"int\", 2], [\"k\", \"int\", 3]]]"

QUARK_UNIT_TESTQ("run_main()", "vector"){
	QUARK_UT_VERIFY(test__parse_expression("[]", R"(["vector-def", "", []])", ""));
}

QUARK_UNIT_TESTQ("run_main()", "vector"){
	QUARK_UT_VERIFY(test__parse_expression("[1,2,3]", R"(["vector-def", "", [["k", "int", 1], ["k", "int", 2], ["k", "int", 3]]])", ""));
}
/*
QUARK_UNIT_TESTQ("run_main()", "vector"){
	QUARK_UT_VERIFY(test__parse_expression("[1,2] + [3,4]", R"(["vector-def", "", [["k", "int", 1], ["k", "int", 2], ["k", "int", 3]]])", ""));
}
*/

//////////////////////////////////			NEG

QUARK_UNIT_1("parse_expression()", "", test__parse_expression("-2 xxx", "[\"unary_minus\", [\"k\", \"int\", 2]]", " xxx"));

QUARK_UNIT_1("parse_expression()", "arithmetics", test__parse_expression(
	"-(3)",
	R"(["unary_minus", ["k", "int", 3]])",
	""
));

QUARK_UNIT_1("parse_expression()", "combo arithmetics", test__parse_expression(
	"2---2 xxx",
	R"(["-", ["k", "int", 2], ["unary_minus", ["unary_minus", ["k", "int", 2]]]])",
	" xxx"
));

QUARK_UNIT_1("parse_expression()", "combo arithmetics", test__parse_expression(
	"2-+-2 xxx",
	R"(["-", ["k", "int", 2], ["unary_minus", ["k", "int", 2]]])",
	" xxx"
));




//////////////////////////////////			LOGICAL EQUALITY


QUARK_UNIT_1("parse_expression()", "<=", test__parse_expression(
	"3 <= 4",
	R"(["<=", ["k", "int", 3], ["k", "int", 4]])",
	""
));
QUARK_UNIT_1("parse_expression()", "<", test__parse_expression(
	"3 < 4",
	R"(["<", ["k", "int", 3], ["k", "int", 4]])",
	""
));
QUARK_UNIT_1("parse_expression()", ">=", test__parse_expression(
	"3 >= 4",
	R"([">=", ["k", "int", 3], ["k", "int", 4]])",
	""
));
QUARK_UNIT_1("parse_expression()", ">", test__parse_expression(
	"3 > 4",
	R"([">", ["k", "int", 3], ["k", "int", 4]])",
	""
));
QUARK_UNIT_1("parse_expression()", "==", test__parse_expression(
	"3 == 4",
	R"(["==", ["k", "int", 3], ["k", "int", 4]])",
	""
));
QUARK_UNIT_1("parse_expression()", "==", test__parse_expression(
	"1==3",
	R"(["==", ["k", "int", 1], ["k", "int", 3]])",
	""
));
QUARK_UNIT_1("parse_expression()", "!=", test__parse_expression(
	"3 != 4",
	R"(["!=", ["k", "int", 3], ["k", "int", 4]])",
	""
));

QUARK_UNIT_1("parse_expression()", "&&", test__parse_expression(
	"3 && 4",
	R"(["&&", ["k", "int", 3], ["k", "int", 4]])",
	""
));
QUARK_UNIT_1("parse_expression()", "&&", test__parse_expression(
	"3 && 4 && 5",
	R"(["&&", ["&&", ["k", "int", 3], ["k", "int", 4]], ["k", "int", 5]])",
	""
));
QUARK_UNIT_1("parse_expression()", "&&", test__parse_expression(
	"1 * 1 && 0 * 1",
	R"(["&&", ["*", ["k", "int", 1], ["k", "int", 1]], ["*", ["k", "int", 0], ["k", "int", 1]]])",
	""
));

QUARK_UNIT_1("parse_expression()", "||", test__parse_expression(
	"3 || 4",
	R"(["||", ["k", "int", 3], ["k", "int", 4]])",
	""
));
QUARK_UNIT_1("parse_expression()", "||", test__parse_expression(
	"3 || 4 || 5",
	R"(["||", ["||", ["k", "int", 3], ["k", "int", 4]], ["k", "int", 5]])",
	""
));
QUARK_UNIT_1("parse_expression()", "||", test__parse_expression(
	"1 * 1 || 0 * 1",
	R"(["||", ["*", ["k", "int", 1], ["k", "int", 1]], ["*", ["k", "int", 0], ["k", "int", 1]]])",
	""
));

//??? Check combos of || and &&



//////////////////////////////////			IDENTIFIERS


QUARK_UNIT_1("parse_expression()", "identifier", test__parse_expression(
	"hello xxx",
	R"(["@", "hello"])",
	" xxx"
));


//////////////////////////////////			COMPARISON OPERATOR

QUARK_UNIT_1("parse_expression()", "?:", test__parse_expression(
	"1 ? 2 : 3 xxx",
	R"(["?:", ["k", "int", 1], ["k", "int", 2], ["k", "int", 3]])",
	" xxx"
));

QUARK_UNIT_1("parse_expression()", "?:", test__parse_expression(
	"1==3 ? 4 : 6 xxx",
	R"(["?:", ["==", ["k", "int", 1], ["k", "int", 3]], ["k", "int", 4], ["k", "int", 6]])",
	" xxx"
));

QUARK_UNIT_1("parse_expression()", "?:", test__parse_expression(
	"1 ? \"true!!!\" : \"false!!!\" xxx",
	R"(["?:", ["k", "int", 1], ["k", "string", "true!!!"], ["k", "string", "false!!!"]])",
	" xxx"
));

QUARK_UNIT_1("parse_expression()", "?:", test__parse_expression(
	"1 + 2 ? 3 + 4 : 5 + 6 xxx",
	R"(["?:", ["+", ["k", "int", 1], ["k", "int", 2]], ["+", ["k", "int", 3], ["k", "int", 4]], ["+", ["k", "int", 5], ["k", "int", 6]]])",
	" xxx"
));

//??? Add more test to see precedence works as it should!


QUARK_UNIT_1("parse_expression()", "?:", test__parse_expression(
	"input_flag ? \"123\" : \"456\"",
	R"(["?:", ["@", "input_flag"], ["k", "string", "123"], ["k", "string", "456"]])",
	""
));


QUARK_UNIT_1("parse_expression()", "?:", test__parse_expression(
	"input_flag ? 100 + 10 * 2 : 1000 - 3 * 4",
	R"(["?:", ["@", "input_flag"], ["+", ["k", "int", 100], ["*", ["k", "int", 10], ["k", "int", 2]]], ["-", ["k", "int", 1000], ["*", ["k", "int", 3], ["k", "int", 4]]]])",
	""
));




//////////////////////////////////			FUNCTION CALLS

QUARK_UNIT_1("parse_expression()", "function call", test__parse_expression(
	"f() xxx",
	R"(["call", ["@", "f"], []])",
	" xxx"
));

QUARK_UNIT_1("parse_expression()", "function call, one simple arg", test__parse_expression(
	"f(3)",
	R"(["call", ["@", "f"], [["k", "int", 3]]])",
	""
));

QUARK_UNIT_1("parse_expression()", "call with expression-arg", test__parse_expression(
	"f(x+10) xxx",
	R"(["call", ["@", "f"], [["+", ["@", "x"], ["k", "int", 10]]]])",
	" xxx"
));
QUARK_UNIT_1("parse_expression()", "call with expression-arg", test__parse_expression(
	"f(1,2) xxx",
	R"(["call", ["@", "f"], [["k", "int", 1], ["k", "int", 2]]])",
	" xxx"
));
QUARK_UNIT_1("parse_expression()", "call with expression-arg -- whitespace", test__parse_expression(
	"f ( 1 , 2 ) xxx",
	R"(["call", ["@", "f"], [["k", "int", 1], ["k", "int", 2]]])",
	" xxx"
));

QUARK_UNIT_1("parse_expression()", "function call with expression-args", test__parse_expression(
	"f(3 + 4, 4 * g(1000 + 2345), \"hello\", 5)",
	R"(["call", ["@", "f"], [["+", ["k", "int", 3], ["k", "int", 4]], ["*", ["k", "int", 4], ["call", ["@", "g"], [["+", ["k", "int", 1000], ["k", "int", 2345]]]]], ["k", "string", "hello"], ["k", "int", 5]]])",
	""
));

QUARK_UNIT_TESTQ("parse_expression()", "function call, expression argument"){
	const auto result = test_parse_expression2(seq_t("1 == 2)"));
	QUARK_UT_VERIFY((		result == pair<string, seq_t>( "[\"==\", [\"k\", \"int\", 1], [\"k\", \"int\", 2]]", ")" )		));
}



QUARK_UNIT_TESTQ("parse_expression()", "function call, expression argument"){
	const auto result = test_parse_expression2(seq_t("f(1 == 2)"));
	QUARK_UT_VERIFY((		result == pair<string, seq_t>( R"(["call", ["@", "f"], [["==", ["k", "int", 1], ["k", "int", 2]]]])", "" )		));
}


QUARK_UNIT_TESTQ("parse_expression()", "function call"){
	const auto result = test_parse_expression2(seq_t("((3))))"));
}
QUARK_UNIT_TESTQ("parse_expression()", "function call"){
	const auto result = test_parse_expression2(seq_t("print((3))))"));
}

QUARK_UNIT_TESTQ("parse_expression()", "function call"){
	const auto result = test_parse_expression2(seq_t("print(1 < 2)"));
}

QUARK_UNIT_TESTQ("parse_expression()", "function call"){
	const auto result = test_parse_expression2(seq_t("print(1 < color(1, 2, 3))"));
}


QUARK_UNIT_TESTQ("parse_expression()", "function call"){
	const auto result = test_parse_expression2(seq_t("print(color(1, 2, 3) < color(1, 2, 3))"));
}

QUARK_UNIT_TESTQ("parse_expression()", "function call"){
	QUARK_UT_VERIFY(
		test__parse_expression(
			"print(color(1, 2, 3) == file(404)) xxx",
			"[\"call\", [\"@\", \"print\"], [[\"==\", [\"call\", [\"@\", \"color\"], [[\"k\", \"int\", 1], [\"k\", \"int\", 2], [\"k\", \"int\", 3]]], [\"call\", [\"@\", \"file\"], [[\"k\", \"int\", 404]]]]]]",
			" xxx"
		)
	)
}


//////////////////////////////////			MEMBER ACCESS

QUARK_UNIT_TESTQ("parse_expression()", "struct member access"){
	QUARK_UT_VERIFY(test__parse_expression("hello.kitty xxx", R"(["->", ["@", "hello"], "kitty"])", " xxx"));
}

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


//////////////////////////////////			LOOKUP

QUARK_UNIT_1("parse_expression()", "lookup with int", test__parse_expression(
	"hello[10] xxx",
	R"(["[]", ["@", "hello"], ["k", "int", 10]])",
	" xxx"
));

QUARK_UNIT_1("parse_expression()", "lookup with string", test__parse_expression(
	"hello[\"troll\"] xxx",
	R"(["[]", ["@", "hello"], ["k", "string", "troll"]])",
	" xxx"
));

QUARK_UNIT_1("parse_expression()", "lookup with string -- whitespace", test__parse_expression(
	"hello [ \"troll\" ] xxx",
	R"(["[]", ["@", "hello"], ["k", "string", "troll"]])",
	" xxx"
));



//////////////////////////////////			COMBOS


QUARK_UNIT_1("parse_expression()", "combo - function call", test__parse_expression(
	"poke.mon.f() xxx",
	R"(["call", ["->", ["->", ["@", "poke"], "mon"], "f"], []])",
	" xxx"
));

QUARK_UNIT_1("parse_expression()", "combo - function call", test__parse_expression(
	"f().g() xxx",
	R"(["call", ["->", ["call", ["@", "f"], []], "g"], []])",
	" xxx"
));



QUARK_UNIT_1("parse_expression()", "complex chain", test__parse_expression(
	"hello[\"troll\"].kitty[10].cat xxx",
	R"(["->", ["[]", ["->", ["[]", ["@", "hello"], ["k", "string", "troll"]], "kitty"], ["k", "int", 10]], "cat"])",
	" xxx"
));


QUARK_UNIT_1("parse_expression()", "chain", test__parse_expression(
	"poke.mon.v[10].a.b.c[\"three\"] xxx",
	R"(["[]", ["->", ["->", ["->", ["[]", ["->", ["->", ["@", "poke"], "mon"], "v"], ["k", "int", 10]], "a"], "b"], "c"], ["k", "string", "three"]])",
	" xxx"
));

QUARK_UNIT_1("parse_expression()", "combo arithmetics", test__parse_expression(
	" 5 - 2 * ( 3 ) xxx",
	R"(["-", ["k", "int", 5], ["*", ["k", "int", 2], ["k", "int", 3]]])",
	" xxx"
));




//////////////////////////////////			EXPRESSION ERRORS



void test__parse_expression__throw(const std::string& expression, const std::string& exception_message){
	try{
		const auto result = parse_expression(seq_t(expression));
		QUARK_TEST_VERIFY(false);
	}
	catch(const std::runtime_error& e){
		const std::string es(e.what());
		if(!exception_message.empty()){
			QUARK_TEST_VERIFY(es == exception_message);
		}
	}
}


QUARK_UNIT_TESTQ("evaluate_expression()", "Parentheses error") {
	test__parse_expression__throw("5*((1+3)*2+1", "");
}

QUARK_UNIT_TESTQ("evaluate_expression()", "Parentheses error") {
//	test__parse_expression__throw("5*((1+3)*2)+1)", "");
}

QUARK_UNIT_TESTQ("evaluate_expression()", "Repeated operators (wrong)") {
	test__parse_expression__throw("5*/2", "");
}

QUARK_UNIT_TESTQ("evaluate_expression()", "Wrong position of an operator") {
	test__parse_expression__throw("*2", "");
}

QUARK_UNIT_TESTQ("evaluate_expression()", "Wrong position of an operator") {
	test__parse_expression__throw("2+", "Unexpected end of string");
}
QUARK_UNIT_TESTQ("evaluate_expression()", "Wrong position of an operator") {
	test__parse_expression__throw("2*", "Unexpected end of string");
}


QUARK_UNIT_TESTQ("evaluate_expression()", "Invalid characters") {
	test__parse_expression__throw("~5", "Illegal characters.");
}

QUARK_UNIT_TESTQ("evaluate_expression()", "Invalid characters") {
	test__parse_expression__throw("~5", "");
}

QUARK_UNIT_TESTQ("evaluate_expression()", "Invalid characters") {
//	test__parse_expression__throw("5x", "EEE_WRONG_CHAR");
}




QUARK_UNIT_TESTQ("evaluate_expression()", "Invalid characters") {
	test__parse_expression__throw("2/", "Unexpected end of string");
}


}	//	parser2
