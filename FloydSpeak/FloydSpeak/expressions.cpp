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

namespace floyd_ast {

using std::pair;
using std::string;
using std::vector;
using std::shared_ptr;
using std::make_shared;


string expression_to_json_string(const expression_t& e);

/*
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
*/

//////////////////////////////////////////////////		expression_t


expression_t::expression_t(
	const floyd_basics::expression_type operation,
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

floyd_basics::expression_type expression_t::get_operation() const{
	QUARK_ASSERT(check_invariant());

	return _operation;
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
	return dynamic_cast<const literal_expr_t*>(_expr.get()) != nullptr;
}

const value_t& expression_t::get_constant() const{
	QUARK_ASSERT(is_constant())

	return dynamic_cast<const literal_expr_t*>(_expr.get())->_value;
}

bool is_simple_expression__2(const std::string& op){
	return
		op == "+" || op == "-" || op == "*" || op == "/" || op == "%"
		|| op == "<=" || op == "<" || op == ">=" || op == ">"
		|| op == "==" || op == "!=" || op == "&&" || op == "||";
}







void trace(const expression_t& e){
	QUARK_ASSERT(e.check_invariant());
	const auto json = expression_to_json(e);
	const auto s = json_to_compact_string(json);
	QUARK_TRACE(s);
}



////////////////////////////////////////////		JSON SUPPORT



json_t expression_to_json(const expression_t& e){
	return e.get_expr()->expr_base__to_json();
}

json_t expressions_to_json(const std::vector<expression_t> v){
	vector<json_t> r;
	for(const auto e: v){
		r.push_back(expression_to_json(e));
	}
	return json_t::make_array(r);
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
		R"(["call", ["@", "my_func", "null"], [["k", "xyz", "string"], ["k", 123, "int"]], "string"])"
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
