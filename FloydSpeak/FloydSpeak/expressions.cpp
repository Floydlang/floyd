//
//  expressions.cpp
//  FloydSpeak
//
//  Created by Marcus Zetterquist on 03/08/16.
//  Copyright © 2016 Marcus Zetterquist. All rights reserved.
//

#include "expressions.h"

#include "statements.h"
#include "parser_value.h"
#include "json_writer.h"
#include "utils.h"
#include "json_support.h"
#include <cmath>


namespace floyd_ast {

using std::pair;
using std::string;
using std::vector;
using std::shared_ptr;
using std::make_shared;


string expression_to_json_string(const expression_t& e);


QUARK_UNIT_TEST("", "math_operation2_expr_t==()", "", ""){
	const auto a = expression_t::make_simple_expression__2(
		floyd_basics::expression_type::k_arithmetic_add__2,
		expression_t::make_constant_int(3),
		expression_t::make_constant_int(4));
	const auto b = expression_t::make_simple_expression__2(
		floyd_basics::expression_type::k_arithmetic_add__2,
		expression_t::make_constant_int(3),
		expression_t::make_constant_int(4));
	QUARK_TEST_VERIFY(a == b);
}


//////////////////////////////////////////////////		expression_t


expression_t::expression_t(
	floyd_basics::expression_type operation,
	const std::vector<expression_t>& expressions,
	const std::shared_ptr<value_t>& constant,
	const std::string& symbol,
	const typeid_t& result_type
)
:
	_debug(""),
	_operation(operation),
	_expressions(expressions),
	_constant(constant),
	_symbol(symbol),
	_result_type(result_type)
{
	_debug = expression_to_json_string(*this);

	QUARK_ASSERT(check_invariant());
}

bool expression_t::check_invariant() const{
//	QUARK_ASSERT(_debug.size() > 0);

//	QUARK_ASSERT(_result_type._base_type != base_type::k_null && _result_type.check_invariant());
	return true;
}

bool expression_t::operator==(const expression_t& other) const {
	QUARK_ASSERT(check_invariant());
	QUARK_ASSERT(other.check_invariant());

	return
		(_operation == other._operation)
		&& (_expressions == other._expressions)
		&& (compare_shared_values(_constant, other._constant))
		&& (_symbol == other._symbol);
}

floyd_basics::expression_type expression_t::get_operation() const{
	QUARK_ASSERT(check_invariant());

	return _operation;
}

const std::vector<expression_t>& expression_t::get_expressions() const{
	QUARK_ASSERT(check_invariant());

	return _expressions;
}

const std::string& expression_t::get_symbol() const{
	QUARK_ASSERT(check_invariant());

	return _symbol;
}

	
typeid_t expression_t::get_result_type() const{
	QUARK_ASSERT(check_invariant());

	return _result_type;
}

expression_t expression_t::make_constant_value(const value_t& value){
	QUARK_ASSERT(value.check_invariant());

	auto result = expression_t(
		floyd_basics::expression_type::k_constant,
		{},
		make_shared<value_t>(value),
		{},
		value.get_type()
	);
	QUARK_ASSERT(result.check_invariant());
	return result;
}


expression_t expression_t::make_constant_null(){
	return make_constant_value(value_t());
}
expression_t expression_t::make_constant_int(const int i){
	return make_constant_value(value_t(i));
}
expression_t expression_t::make_constant_bool(const bool i){
	return make_constant_value(value_t(i));
}
expression_t expression_t::make_constant_float(const float i){
	return make_constant_value(value_t(i));
}
expression_t expression_t::make_constant_string(const std::string& s){
	return make_constant_value(value_t(s));
}



bool expression_t::is_constant() const{
	return _operation == floyd_basics::expression_type::k_constant;
}

const value_t& expression_t::get_constant() const{
	QUARK_ASSERT(is_constant())

	return *_constant;
}

bool is_simple_expression__2(const std::string& op){
	return
		op == "+" || op == "-" || op == "*" || op == "/" || op == "%"
		|| op == "<=" || op == "<" || op == ">=" || op == ">"
		|| op == "==" || op == "!=" || op == "&&" || op == "||";
}

expression_t expression_t::make_simple_expression__2(floyd_basics::expression_type op, const expression_t& left, const expression_t& right){
	if(
		op == floyd_basics::expression_type::k_arithmetic_add__2
		|| op == floyd_basics::expression_type::k_arithmetic_subtract__2
		|| op == floyd_basics::expression_type::k_arithmetic_multiply__2
		|| op == floyd_basics::expression_type::k_arithmetic_divide__2
		|| op == floyd_basics::expression_type::k_arithmetic_remainder__2
	)
	{
		return expression_t(op, { left, right }, {}, {}, left.get_expression_type());
	}
	else if(
		op == floyd_basics::expression_type::k_comparison_smaller_or_equal__2
		|| op == floyd_basics::expression_type::k_comparison_smaller__2
		|| op == floyd_basics::expression_type::k_comparison_larger_or_equal__2
		|| op == floyd_basics::expression_type::k_comparison_larger__2

		|| op == floyd_basics::expression_type::k_logical_equal__2
		|| op == floyd_basics::expression_type::k_logical_nonequal__2
		|| op == floyd_basics::expression_type::k_logical_and__2
		|| op == floyd_basics::expression_type::k_logical_or__2
//		|| op == floyd_basics::expression_type::k_logical_negate
	)
	{
		return expression_t(op, { left, right }, {}, {}, typeid_t::make_bool());
	}
	else if(
		op == floyd_basics::expression_type::k_constant
		|| op == floyd_basics::expression_type::k_arithmetic_unary_minus__1
		|| op == floyd_basics::expression_type::k_conditional_operator3
		|| op == floyd_basics::expression_type::k_call
		|| op == floyd_basics::expression_type::k_variable
		|| op == floyd_basics::expression_type::k_resolve_member
		|| op == floyd_basics::expression_type::k_lookup_element)
	{
		QUARK_ASSERT(false);
	}
	else{
		QUARK_ASSERT(false);
	}
}


expression_t expression_t::make_unary_minus(const expression_t& expr){
	QUARK_ASSERT(expr.check_invariant());

	auto result = expression_t(
		floyd_basics::expression_type::k_arithmetic_unary_minus__1,
		{ expr },
		{},
		{},
		expr.get_expression_type()
	);
	QUARK_ASSERT(result.check_invariant());
	return result;
}

expression_t expression_t::make_conditional_operator(const expression_t& condition, const expression_t& a, const expression_t& b){
	QUARK_ASSERT(condition.check_invariant());
	QUARK_ASSERT(a.check_invariant());
	QUARK_ASSERT(b.check_invariant());

	auto result = expression_t(
		floyd_basics::expression_type::k_conditional_operator3,
		{ condition, a, b },
		{},
		{},
		a.get_expression_type()
	);
	QUARK_ASSERT(result.check_invariant());
	return result;
}

expression_t expression_t::make_function_call(const expression_t& function, const std::vector<expression_t>& args, const typeid_t& result_type){
	QUARK_ASSERT(function.check_invariant());
	for(const auto arg: args){
		QUARK_ASSERT(arg.check_invariant());
	}
//	QUARK_ASSERT(result_type._base_type != base_type::k_null && result_type.check_invariant());

	vector<expression_t> expressions = { function };
	expressions.insert(expressions.end(), args.begin(), args.end());
	auto result = expression_t(
		floyd_basics::expression_type::k_call,
		expressions,
		{},
		{},
		result_type
	);
	QUARK_ASSERT(result.check_invariant());
	return result;
}


expression_t expression_t::make_variable_expression(const std::string& variable, const typeid_t& result_type){
	QUARK_ASSERT(variable.size() > 0);

	auto result = expression_t(
		floyd_basics::expression_type::k_variable,
		{},
		{},
		variable,
		result_type
	);
	QUARK_ASSERT(result.check_invariant());
	return result;
}

expression_t expression_t::make_resolve_member(const expression_t& parent_address, const std::string& member_name, const typeid_t& result_type){
	QUARK_ASSERT(parent_address.check_invariant());
	QUARK_ASSERT(member_name.size() > 0);

	auto result = expression_t(
		floyd_basics::expression_type::k_resolve_member,
		{ parent_address },
		{},
		member_name,
		result_type
	);
	QUARK_ASSERT(result.check_invariant());
	return result;
}

expression_t expression_t::make_lookup(const expression_t& parent_address, const expression_t& lookup_key, const typeid_t& result_type){
	QUARK_ASSERT(parent_address.check_invariant());
	QUARK_ASSERT(lookup_key.check_invariant());
	QUARK_ASSERT(result_type._base_type != floyd_basics::base_type::k_null && result_type.check_invariant());

	auto result = expression_t(
		floyd_basics::expression_type::k_lookup_element,
		{ parent_address, lookup_key },
		{},
		{},
		result_type
	);
	QUARK_ASSERT(result.check_invariant());
	return result;
}


void trace(const expression_t& e){
	QUARK_ASSERT(e.check_invariant());
	const auto json = expression_to_json(e);
	const auto s = json_to_compact_string(json);
	QUARK_TRACE(s);
}



////////////////////////////////////////////		JSON SUPPORT



json_t expression_to_json(const expression_t& e){
	vector<json_t>  expressions;
	for(const auto& i: e.get_expressions()){
		const auto a = expression_to_json(i);
		expressions.push_back(a);
	}

	const auto constant = e.is_constant() ? value_to_json(e.get_constant()) : json_t();
	const auto symbol = e.get_symbol().empty() == false ? e.get_symbol() : json_t();

	auto result = json_t::make_array();
	result = push_back(result, floyd_basics::expression_type_to_token(e.get_operation()));
	if(symbol.is_null() == false){
		result = push_back(result, symbol);
	}

	if(e.get_operation() == floyd_basics::expression_type::k_call){
		//	Add ONE element that is an array of expressions.
		result = push_back(result, expressions);
	}
	else{
		for(const auto f: expressions){
			result = push_back(result, f);
		}
	}

	if(constant.is_null() == false){
		result = push_back(result, constant);
	}

	result = push_back(result, typeid_to_json(e.get_result_type()));

	return result;
}

string expression_to_json_string(const expression_t& e){
	const auto json = expression_to_json(e);
	return json_to_compact_string(json);
}

QUARK_UNIT_TESTQ("expression_to_json()", "constants"){
	quark::ut_compare(expression_to_json_string(expression_t::make_constant_int(13)), R"(["k", 13, "int"])");
	quark::ut_compare(expression_to_json_string(expression_t::make_constant_string("xyz")), R"(["k", "xyz", "string"])");
	quark::ut_compare(expression_to_json_string(expression_t::make_constant_float(14.0f)), R"(["k", 14, "float"])");
	quark::ut_compare(expression_to_json_string(expression_t::make_constant_bool(true)), R"(["k", true, "bool"])");
	quark::ut_compare(expression_to_json_string(expression_t::make_constant_bool(false)), R"(["k", false, "bool"])");
}

QUARK_UNIT_TESTQ("expression_to_json()", "math2"){
	quark::ut_compare(
		expression_to_json_string(
			expression_t::make_simple_expression__2(
				floyd_basics::expression_type::k_arithmetic_add__2, expression_t::make_constant_int(2), expression_t::make_constant_int(3))
			),
		R"(["+", ["k", 2, "int"], ["k", 3, "int"], "int"])"
	);
}

QUARK_UNIT_TESTQ("expression_to_json()", "call"){
	quark::ut_compare(
		expression_to_json_string(
			expression_t::make_function_call(
				expression_t::make_variable_expression("my_func", typeid_t::make_null()),
				{
					expression_t::make_constant_string("xyz"),
					expression_t::make_constant_int(123)
				},
				typeid_t::make_string()
			)
		),
		R"(["call", [["@", "my_func", "null"], ["k", "xyz", "string"], ["k", 123, "int"]], "string"])"
	);
}

QUARK_UNIT_TESTQ("expression_to_json()", "lookup"){
	quark::ut_compare(
		expression_to_json_string(
			expression_t::make_lookup(
				expression_t::make_variable_expression("hello", typeid_t::make_string()),
				expression_t::make_constant_string("xyz"),
				typeid_t::make_string()
			)
		),
		R"(["[-]", ["@", "hello", "string"], ["k", "xyz", "string"], "string"])"
	);
}


}	//	floyd_ast
