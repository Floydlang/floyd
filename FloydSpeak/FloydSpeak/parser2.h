//
//  parser2.h
//  FloydSpeak
//
//  Created by Marcus Zetterquist on 25/08/16.
//  Copyright © 2016 Marcus Zetterquist. All rights reserved.
//

#ifndef parser2_h
#define parser2_h


#include "quark.h"

#include <string>
#include <memory>


/*
	C99-language constants.
*/
const std::string k_c99_number_chars = "0123456789.";
const std::string k_c99_whitespace_chars = " \n\t\r";
	const std::string k_identifier_chars = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_";

//	Precedence same as C99.
enum class eoperator_precedence {
	k_super_strong = 0,

	//	(xyz)
	k_parentesis = 0,

	//	a()
	k_function_call = 2,

	//	a.b
	k_member_access = 2,

	k_multiply_divider_remainder = 5,

	k_add_sub = 6,

	//	<   <=	For relational operators < and ≤ respectively
	//	>   >=
	k_larger_smaller = 8,


	k_equal__not_equal = 9,

	k_logical_and = 13,
	k_logical_or = 14,

	k_comparison_operator = 15,

	k_super_weak
};

enum class eoperation {
	k_0_number_constant = 100,

	//	This is string specifying a local variable, member variable, argument, global etc. Only the first entry in a chain.
	k_0_resolve,

	k_0_string_literal,

	k_2_member_access,

	k_2_add,
	k_2_subtract,
	k_2_multiply,
	k_2_divide,
	k_2_remainder,

	k_2_smaller_or_equal,
	k_2_smaller,

	k_2_larger_or_equal,
	k_2_larger,

	k_2_logical_equal,
	k_2_logical_nonequal,
	k_2_logical_and,
	k_2_logical_or,

	k_3_conditional_operator,

	k_n_call,

	k_1_logical_not,
	k_1_load
};



template<typename EXPRESSION> struct maker {
	public: virtual ~maker(){};
	public: virtual const EXPRESSION maker__on_string(const eoperation op, const std::string& s) const = 0;
	public: virtual const EXPRESSION maker__make(const eoperation op, const EXPRESSION& expr) const = 0;
	public: virtual const EXPRESSION maker__make(const eoperation op, const EXPRESSION& lhs, const EXPRESSION& rhs) const = 0;
	public: virtual const EXPRESSION maker__make(const eoperation op, const EXPRESSION& e1, const EXPRESSION& e2, const EXPRESSION& e3) const = 0;
	public: virtual const EXPRESSION maker__call(const std::string& f, const std::vector<EXPRESSION>& args) const = 0;
	public: virtual const EXPRESSION maker__member_access(const EXPRESSION& address, const std::string& member_name) const = 0;
};





template<typename EXPRESSION>
std::pair<EXPRESSION, seq_t> evaluate_operation(const maker<EXPRESSION>& helper, const seq_t& p1, const EXPRESSION& lhs, const eoperator_precedence precedence);

//??? Add support for member variable/function/lookup paths.
template<typename EXPRESSION>
std::pair<EXPRESSION, seq_t> evaluate_expression(const maker<EXPRESSION>& helper, const seq_t& p, const eoperator_precedence precedence);


seq_t skip_whitespace(const seq_t& p);

std::pair<std::string, seq_t> parse_string_literal(const seq_t& p);




/*
	Parse non-constant value. Called calcval ??? find real term.
	This is a recursive function since you can use lookups, function calls, structure members - all nested.
	This includes expressions that calculates lookup keys/index.

	Each step in the path can be one of these:
	1) *read* from a variable / constant, structure member.
	2) Function call
	3) Looking up using []

	Returns either a load_expr_t or a function_call_expr_t. The hold potentially many levels of nested lookups, function calls and member reads.

	Example input:
		"hello[4]" --->	lookup, load. Eventuallt the result-type of this expression becomes T, the type of the elements in the array.
		"f()" ---> resolve "f", function_call. Eventually the result-type of this expr becomes T, the return type of the function.
		"my_global" ---> resolve "my_global", load
		"my_local" ---> resolve "my_global", load
		"a.b" ---> resolve "a", resolve member b, load

		You can chain calcvarg:s and even nest them.

		"my_global.hello[4].f(my_local))" and so on.

		"a.b.c."
*/
template<typename EXPRESSION>
std::pair<EXPRESSION, seq_t> parse_calculated_value(const maker<EXPRESSION>& helper, const seq_t& p) {
	QUARK_ASSERT(p.check_invariant());

	const auto identifier_s = read_while(p, k_identifier_chars);
	QUARK_ASSERT(!identifier_s.first.empty());

	const auto pos2 = skip_whitespace(identifier_s.second);
	const auto next_char1 = pos2.first();

	//	Lookup expression?
	if(next_char1 == "["){
		QUARK_ASSERT(false);
	}

	//	Member access?
	else if(next_char1 == "."){
		const auto resolve_variable_expr = helper.maker__on_string(eoperation::k_0_resolve, identifier_s.first);
		const auto address_expr =  evaluate_operation(helper, pos2, resolve_variable_expr, eoperator_precedence::k_super_weak);
		const auto load_expr = helper.maker__make(eoperation::k_1_load, address_expr.first);
		return { load_expr, address_expr.second };
	}

	//	Function call?
	else if(next_char1 == "("){
		const auto pos3 = skip_whitespace(pos2.rest());
		const auto next_char = pos3.first();

		//	No arguments.
		if(next_char == ")"){
			const auto result = helper.maker__call(identifier_s.first, {});
			return { result, pos3.rest() };
		}

		//	Arguments.
		else{
			auto pos_loop = skip_whitespace(pos3);
			std::vector<EXPRESSION> arg_exprs;
			bool more = true;
			while(more){
				const auto a = evaluate_expression(helper, pos_loop, eoperator_precedence::k_super_weak);
				arg_exprs.push_back(a.first);
				const auto ch = a.second.first(1);
				if(ch == ","){
					more = true;
				}
				else if(ch == ")"){
					more = false;
				}
				else{
					throw std::runtime_error("Unexpected char");
				}
				pos_loop = a.second.rest();
			}

			const auto result = helper.maker__call(identifier_s.first, arg_exprs);
			return { result, pos_loop };
		}
	}

	//	Must be a plain variable access.
	else{
		const auto resolve = helper.maker__on_string(eoperation::k_0_resolve, identifier_s.first);
		const auto load = helper.maker__make(eoperation::k_1_load, resolve);
		return { load, identifier_s.second };
	}
}




template<typename EXPRESSION>
std::pair<EXPRESSION, seq_t> evaluate_single(const maker<EXPRESSION>& helper, const seq_t& p) {
	QUARK_ASSERT(p.check_invariant());

	if(p.first() == "\""){
		const auto s = read_while_not(p.rest(), "\"");
		const auto result = helper.maker__on_string(eoperation::k_0_string_literal, s.first);
		return { result, s.second.rest() };
	}

	{
		const auto number_s = read_while(p, k_c99_number_chars);
		if(!number_s.first.empty()){
			const auto result = helper.maker__on_string(eoperation::k_0_number_constant, number_s.first);
			return { result, number_s.second };
		}
	}

	{
		const auto identifier_s = read_while(p, k_identifier_chars);
		if(!identifier_s.first.empty()){
			return parse_calculated_value(helper, p);
		}
	}

	//??? Identifiers, string constants, true/false.

	QUARK_ASSERT(false);
}

//??? define parser white-space policy: all function input SUPPORTS whitespace. Filter at function input. No need to filter when you return for next function. Why: ony one function entry, often many function exists.

/*
	number
	string literal
	somethigne within ().
	function call
	a ? b : c
	-a
*/
template<typename EXPRESSION>
std::pair<EXPRESSION, seq_t> evaluate_atom(const maker<EXPRESSION>& helper, const seq_t& p){
	QUARK_ASSERT(p.check_invariant());

    const auto p2 = skip_whitespace(p);
	if(p2.empty()){
		throw std::runtime_error("Unexpected end of string");
	}

	const char ch1 = p2.first_char();

	//	Negate? "-xxx"
	if(ch1 == '-'){
		const auto a = evaluate_expression(helper, p2.rest(), eoperator_precedence::k_super_strong);
		const auto value2 = helper.maker__make(eoperation::k_1_logical_not, a.first);
		return { value2, skip_whitespace(a.second) };
	}
	//	Expression within paranthesis? "(yyy)xxx"
	else if(ch1 == '('){
		const auto a = evaluate_expression(helper, p2.rest(), eoperator_precedence::k_super_weak);
		if (a.second.first(1) != ")"){
			throw std::runtime_error("Expected ')'");
		}
		return { a.first, skip_whitespace(a.second.rest()) };
	}

	//	Colon for (?:) ":yyy xxx"
	else if(ch1 == ':'){
		const auto a = evaluate_expression(helper, p2.rest(), eoperator_precedence::k_super_weak);
		return { a.first, skip_whitespace(a.second.rest()) };
	}

	//	Single constant number, string literal, function call, variable access, lookup or member access. Can be a chain.
	//	"1234xxx" or "my_function(3)xxx"
	else {
		const auto a = evaluate_single(helper, p2);
		return { a.first, skip_whitespace(a.second) };
	}
}


//	Member access
//	EXPRESSION . EXPRESSION +

/*
	hello.func(x)
	lhs = ["@", "hello"]
	return = ["->", [], "kitty"]
*/
template<typename EXPRESSION>
std::pair<EXPRESSION, seq_t> on_member_access_operator(const maker<EXPRESSION>& helper, const seq_t& p1, const EXPRESSION& lhs, const eoperator_precedence precedence){
	QUARK_ASSERT(p1.check_invariant());
	QUARK_ASSERT(p1.first() == ".");
	QUARK_ASSERT(precedence > eoperator_precedence::k_member_access);

	const auto p = p1;

	const auto member_name = read_while(p.rest(), k_identifier_chars);
	const auto value2 = helper.maker__member_access(lhs, member_name.first);
	const auto address_base = evaluate_operation(helper, member_name.second, value2, precedence);
	return { value2, address_base.second.rest() };
}


template<typename EXPRESSION>
std::pair<EXPRESSION, seq_t> evaluate_operation(const maker<EXPRESSION>& helper, const seq_t& p1, const EXPRESSION& lhs, const eoperator_precedence precedence){
	QUARK_ASSERT(p1.check_invariant());

	const auto p = p1;
	if(!p.empty()){
		const char op1 = p.first_char();
		const std::string op2 = p.first(2);

		//	Ending parantesis
		if(op1 == ')' && precedence > eoperator_precedence::k_parentesis){
			return { lhs, p };
		}

		//	Member access
		//	EXPRESSION . EXPRESSION +

		else if(op1 == '.'  && precedence > eoperator_precedence::k_member_access){
			return on_member_access_operator(helper, p1, lhs, precedence);
		}

		//	EXPRESSION + EXPRESSION +
		else if(op1 == '+'  && precedence > eoperator_precedence::k_add_sub){
			const auto rhs = evaluate_expression(helper, p.rest(), eoperator_precedence::k_add_sub);
			const auto value2 = helper.maker__make(eoperation::k_2_add, lhs, rhs.first);
			return evaluate_operation(helper, rhs.second, value2, precedence);
		}

		//	EXPRESSION - EXPRESSION -
		else if(op1 == '-' && precedence > eoperator_precedence::k_add_sub){
			const auto rhs = evaluate_expression(helper, p.rest(), eoperator_precedence::k_add_sub);
			const auto value2 = helper.maker__make(eoperation::k_2_subtract, lhs, rhs.first);
			return evaluate_operation(helper, rhs.second, value2, precedence);
		}

		//	EXPRESSION * EXPRESSION *
		else if(op1 == '*' && precedence > eoperator_precedence::k_multiply_divider_remainder) {
			const auto rhs = evaluate_expression(helper, p.rest(), eoperator_precedence::k_multiply_divider_remainder);
			const auto value2 = helper.maker__make(eoperation::k_2_multiply, lhs, rhs.first);
			return evaluate_operation(helper, rhs.second, value2, precedence);
		}
		//	EXPRESSION / EXPRESSION /
		else if(op1 == '/' && precedence > eoperator_precedence::k_multiply_divider_remainder) {
			const auto rhs = evaluate_expression(helper, p.rest(), eoperator_precedence::k_multiply_divider_remainder);
			const auto value2 = helper.maker__make(eoperation::k_2_divide, lhs, rhs.first);
			return evaluate_operation(helper, rhs.second, value2, precedence);
		}

		//	EXPRESSION % EXPRESSION %
		else if(op1 == '%' && precedence > eoperator_precedence::k_multiply_divider_remainder) {
			const auto rhs = evaluate_expression(helper, p.rest(), eoperator_precedence::k_multiply_divider_remainder);
			const auto value2 = helper.maker__make(eoperation::k_2_remainder, lhs, rhs.first);
			return evaluate_operation(helper, rhs.second, value2, precedence);
		}


		//	EXPRESSION ? EXPRESSION : EXPRESSION
		else if(op1 == '?' && precedence > eoperator_precedence::k_comparison_operator) {
			const auto true_expr_p = evaluate_expression(helper, p.rest(), eoperator_precedence::k_comparison_operator);

			const auto colon = true_expr_p.second.first(1);
			if(colon != ":"){
				throw std::runtime_error("Expected ':'");
			}

			const auto false_expr_p = evaluate_expression(helper, true_expr_p.second.rest(), precedence);
			const auto value2 = helper.maker__make(eoperation::k_3_conditional_operator, lhs, true_expr_p.first, false_expr_p.first);

			//	End this precedence level.
			return { value2, false_expr_p.second.rest() };
		}


		//	EXPRESSION == EXPRESSION
		else if(op2 == "==" && precedence > eoperator_precedence::k_equal__not_equal){
			const auto rhs = evaluate_expression(helper, p.rest(2), eoperator_precedence::k_equal__not_equal);
			const auto value2 = helper.maker__make(eoperation::k_2_logical_equal, lhs, rhs.first);

			//	End this precedence level.
			return { value2, rhs.second.rest() };
		}
		//	EXPRESSION != EXPRESSION
		else if(op2 == "!=" && precedence > eoperator_precedence::k_equal__not_equal){
			const auto rhs = evaluate_expression(helper, p.rest(2), eoperator_precedence::k_equal__not_equal);
			const auto value2 = helper.maker__make(eoperation::k_2_logical_nonequal, lhs, rhs.first);

			//	End this precedence level.
			return { value2, rhs.second.rest() };
		}

		//	!!! Check for "<=" before we check for "<".
		//	EXPRESSION <= EXPRESSION
		else if(op2 == "<=" && precedence > eoperator_precedence::k_larger_smaller){
			const auto rhs = evaluate_expression(helper, p.rest(2), eoperator_precedence::k_larger_smaller);
			const auto value2 = helper.maker__make(eoperation::k_2_smaller_or_equal, lhs, rhs.first);

			//	End this precedence level.
			return { value2, rhs.second.rest() };
		}

		//	EXPRESSION < EXPRESSION
		else if(op1 == '<' && precedence > eoperator_precedence::k_larger_smaller){
			const auto rhs = evaluate_expression(helper, p.rest(2), eoperator_precedence::k_larger_smaller);
			const auto value2 = helper.maker__make(eoperation::k_2_smaller, lhs, rhs.first);

			//	End this precedence level.
			return { value2, rhs.second.rest() };
		}


		//	!!! Check for ">=" before we check for ">".
		//	EXPRESSION >= EXPRESSION
		else if(op2 == ">=" && precedence > eoperator_precedence::k_larger_smaller){
			const auto rhs = evaluate_expression(helper, p.rest(2), eoperator_precedence::k_larger_smaller);
			const auto value2 = helper.maker__make(eoperation::k_2_larger_or_equal, lhs, rhs.first);

			//	End this precedence level.
			return { value2, rhs.second.rest() };
		}

		//	EXPRESSION > EXPRESSION
		else if(op1 == '>' && precedence > eoperator_precedence::k_larger_smaller){
			const auto rhs = evaluate_expression(helper, p.rest(2), eoperator_precedence::k_larger_smaller);
			const auto value2 = helper.maker__make(eoperation::k_2_larger, lhs, rhs.first);

			//	End this precedence level.
			return { value2, rhs.second.rest() };
		}


		//	EXPRESSION && EXPRESSION
		else if(op2 == "&&" && precedence > eoperator_precedence::k_logical_and){
			const auto rhs = evaluate_expression(helper, p.rest(2), eoperator_precedence::k_logical_and);
			const auto value2 = helper.maker__make(eoperation::k_2_logical_and, lhs, rhs.first);
			return evaluate_operation(helper, rhs.second, value2, precedence);
		}

		//	EXPRESSION || EXPRESSION
		else if(op2 == "||" && precedence > eoperator_precedence::k_logical_or){
			const auto rhs = evaluate_expression(helper, p.rest(2), eoperator_precedence::k_logical_or);
			const auto value2 = helper.maker__make(eoperation::k_2_logical_or, lhs, rhs.first);
			return evaluate_operation(helper, rhs.second, value2, precedence);
		}

		else{
			return { lhs, p };
		}
	}
	else{
		return { lhs, p };
	}
}

template<typename EXPRESSION>
std::pair<EXPRESSION, seq_t> evaluate_expression(const maker<EXPRESSION>& helper, const seq_t& p, const eoperator_precedence precedence){
	QUARK_ASSERT(p.check_invariant());

	auto a = evaluate_atom(helper, p);
	return evaluate_operation<EXPRESSION>(helper, a.second, a.first, precedence);
}


template<typename EXPRESSION>
std::pair<EXPRESSION, seq_t> evaluate_expression2(const maker<EXPRESSION>& helper, const seq_t& p){
	QUARK_ASSERT(p.check_invariant());

	auto a = evaluate_atom(helper, p);
	return evaluate_operation<EXPRESSION>(helper, a.second, a.first, eoperator_precedence::k_super_weak);
}


#endif /* parser2_h */
