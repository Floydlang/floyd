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
#include <typeinfo>

namespace floyd {

using std::pair;
using std::string;
using std::vector;
using std::shared_ptr;
using std::make_shared;


string expression_to_json_string(const expression_t& e);

/*
QUARK_UNIT_TEST("", "math_operation2_expr_t==()", "", ""){
	const auto a = expression_t::make_simple_expression__2(
		expression_type::k_arithmetic_add__2,
		expression_t::make_literal_int(3),
		expression_t::make_literal_int(4));
	const auto b = expression_t::make_simple_expression__2(
		expression_type::k_arithmetic_add__2,
		expression_t::make_literal_int(3),
		expression_t::make_literal_int(4));
	QUARK_TEST_VERIFY(a == b);
}
*/

//////////////////////////////////////////////////		expression_t


expression_t::expression_t(
	const expression_type operation,
	const std::shared_ptr<const expr_base_t>& expr
)
:
	_debug(""),
	_operation(operation),
	_expr(expr)
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

	if(_expr && other._expr){
//		const auto& ta = typeid(_expr.get()).hash_code();
//		const auto& tb = typeid(other._expr.get()).hash_code();

//		if(ta != tb){
//			return false;
//		}

/*
		const auto lhs = dynamic_cast<const function_definition_expr_t*>(_expr.get());
		const auto rhs = dynamic_cast<const function_definition_expr_t*>(other._expr.get());
		if(lhs && rhs && *lhs == *rhs){
			return true;
		}
*/
		return false;
	}
	else if((_expr && !other._expr) || (!_expr && other._expr)){
		return false;
	}
	else{
		return
			(_operation == other._operation);
	}
}

expression_type expression_t::get_operation() const{
	QUARK_ASSERT(check_invariant());

	return _operation;
}



expression_t expression_t::make_literal_null(){
	return make_literal(value_t());
}
expression_t expression_t::make_literal_int(const int i){
	return make_literal(value_t(i));
}
expression_t expression_t::make_literal_bool(const bool i){
	return make_literal(value_t(i));
}
expression_t expression_t::make_literal_float(const float i){
	return make_literal(value_t(i));
}
expression_t expression_t::make_literal_string(const std::string& s){
	return make_literal(value_t(s));
}



bool expression_t::is_literal() const{
	return dynamic_cast<const literal_expr_t*>(_expr.get()) != nullptr;
}

const value_t& expression_t::get_literal() const{
	QUARK_ASSERT(is_literal())

	return dynamic_cast<const literal_expr_t*>(_expr.get())->_value;
}

bool is_simple_expression__2(const std::string& op){
	return
		op == "+" || op == "-" || op == "*" || op == "/" || op == "%"
		|| op == "<=" || op == "<" || op == ">=" || op == ">"
		|| op == "==" || op == "!=" || op == "&&" || op == "||";
}




////////////////////////////////////////////		JSON SUPPORT



ast_json_t expression_to_json(const expression_t& e){
	return e.get_expr()->expr_base__to_json();
}

ast_json_t expressions_to_json(const std::vector<expression_t> v){
	vector<json_t> r;
	for(const auto e: v){
		r.push_back(expression_to_json(e)._value);
	}
	return ast_json_t{json_t::make_array(r)};
}

string expression_to_json_string(const expression_t& e){
	const auto json = expression_to_json(e);
	return json_to_compact_string(json._value);
}

QUARK_UNIT_TESTQ("expression_to_json()", "literals"){
	quark::ut_compare_strings(expression_to_json_string(expression_t::make_literal_int(13)), R"(["k", 13, "int"])");
	quark::ut_compare_strings(expression_to_json_string(expression_t::make_literal_string("xyz")), R"(["k", "xyz", "string"])");
	quark::ut_compare_strings(expression_to_json_string(expression_t::make_literal_float(14.0f)), R"(["k", 14, "float"])");
	quark::ut_compare_strings(expression_to_json_string(expression_t::make_literal_bool(true)), R"(["k", true, "bool"])");
	quark::ut_compare_strings(expression_to_json_string(expression_t::make_literal_bool(false)), R"(["k", false, "bool"])");
}

QUARK_UNIT_TESTQ("expression_to_json()", "math2"){
	quark::ut_compare_strings(
		expression_to_json_string(
			expression_t::make_simple_expression__2(
				expression_type::k_arithmetic_add__2, expression_t::make_literal_int(2), expression_t::make_literal_int(3))
			),
		R"(["+", ["k", 2, "int"], ["k", 3, "int"]])"
	);
}

QUARK_UNIT_TESTQ("expression_to_json()", "call"){
	quark::ut_compare_strings(
		expression_to_json_string(
			expression_t::make_function_call(
				expression_t::make_variable_expression("my_func"),
				{
					expression_t::make_literal_string("xyz"),
					expression_t::make_literal_int(123)
				}
			)
		),
		R"(["call", ["@", "my_func"], [["k", "xyz", "string"], ["k", 123, "int"]]])"
	);
}

QUARK_UNIT_TESTQ("expression_to_json()", "lookup"){
	quark::ut_compare_strings(
		expression_to_json_string(
			expression_t::make_lookup(
				expression_t::make_variable_expression("hello"),
				expression_t::make_literal_string("xyz")
			)
		),
		R"(["[]", ["@", "hello"], ["k", "xyz", "string"]])"
	);
}


}	//	floyd
