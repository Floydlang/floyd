//
//  expressions.cpp
//  FloydSpeak
//
//  Created by Marcus Zetterquist on 03/08/16.
//  Copyright © 2016 Marcus Zetterquist. All rights reserved.
//

#include "expressions.h"

#include "text_parser.h"
#include "parser_statement.h"
#include "parser_value.h"

#include <cmath>


namespace floyd_parser {


using std::pair;
using std::string;
using std::vector;
using std::shared_ptr;
using std::make_shared;






QUARK_UNIT_TEST("", "math_operation2_expr_t==()", "", ""){
	const auto a = make_math_operation2(math_operation2_expr_t::add, make_constant(3), make_constant(4));
	const auto b = make_math_operation2(math_operation2_expr_t::add, make_constant(3), make_constant(4));
	QUARK_TEST_VERIFY(a == b);
}



bool math_operation2_expr_t::operator==(const math_operation2_expr_t& other) const {
	return _operation == other._operation && *_left == *other._left && *_right == *other._right;
}

bool math_operation1_expr_t::operator==(const math_operation1_expr_t& other) const {
	return _operation == other._operation && *_input == *other._input;
}

bool variable_read_expr_t::operator==(const variable_read_expr_t& other) const{
	return *_address == *other._address ;
}

bool lookup_element_expr_t::operator==(const lookup_element_expr_t& other) const{
	return *_lookup_key == *other._lookup_key ;
}





//////////////////////////////////////////////////		expression_t



bool expression_t::check_invariant() const{
	return true;
}

expression_t::expression_t(){
	QUARK_ASSERT(check_invariant());
}

bool expression_t::operator==(const expression_t& other) const {
	QUARK_ASSERT(check_invariant());
	QUARK_ASSERT(other.check_invariant());

	if(_constant){
		return other._constant && *_constant == *other._constant;
	}
	else if(_math_operation1_expr){
		return other._math_operation1_expr && *_math_operation1_expr == *other._math_operation1_expr;
	}
	else if(_math_operation2_expr){
		return other._math_operation2_expr && *_math_operation2_expr == *other._math_operation2_expr;
	}
	else if(_call_function_expr){
		return other._call_function_expr && *_call_function_expr == *other._call_function_expr;
	}
	else if(_variable_read_expr){
		return other._variable_read_expr && *_variable_read_expr == *other._variable_read_expr;
	}
	else if(_resolve_member_expr){
		return other._resolve_member_expr && *_resolve_member_expr == *other._resolve_member_expr;
	}
	else if(_lookup_element_expr){
		return other._lookup_element_expr && *_lookup_element_expr == *other._lookup_element_expr;
	}
	else{
		QUARK_ASSERT(false);
		return false;
	}
}




expression_t make_constant(const value_t& value){
	QUARK_ASSERT(value.check_invariant());

	expression_t result;
	result._constant = std::make_shared<value_t>(value);
	return result;
}

expression_t make_constant(const std::string& s){
	expression_t result;
	result._constant = std::make_shared<value_t>(value_t(s));
	return result;
}

expression_t make_constant(const int i){
	expression_t result;
	result._constant = std::make_shared<value_t>(value_t(i));
	return result;
}

expression_t make_constant(const float f){
	expression_t result;
	result._constant = std::make_shared<value_t>(value_t(f));
	return result;
}



expression_t make_math_operation1(math_operation1_expr_t::operation op, const expression_t& input){
	QUARK_ASSERT(input.check_invariant());

	expression_t result;
	auto input2 = make_shared<expression_t>(input);

	math_operation1_expr_t r = math_operation1_expr_t{ op, input2 };
	result._math_operation1_expr = std::make_shared<math_operation1_expr_t>(r);
	return result;
}

expression_t make_math_operation2(math_operation2_expr_t::operation op, const expression_t& left, const expression_t& right){
	QUARK_ASSERT(left.check_invariant());
	QUARK_ASSERT(right.check_invariant());

	expression_t result;
	auto left2 = make_shared<expression_t>(left);
	auto right2 = make_shared<expression_t>(right);

	math_operation2_expr_t r = math_operation2_expr_t{ op, left2, right2 };
	result._math_operation2_expr = std::make_shared<math_operation2_expr_t>(r);
	return result;
}



expression_t make_function_call(const std::string& function_name, const std::vector<expression_t>& inputs){
	expression_t result;
	auto inputs2 = vector<shared_ptr<expression_t>>();
	for(auto arg: inputs){
		inputs2.push_back(make_shared<expression_t>(arg));
	}

	function_call_expr_t r = function_call_expr_t{ function_name, inputs2 };
	result._call_function_expr = std::make_shared<function_call_expr_t>(r);
	return result;
}

expression_t make_function_call(const std::string& function_name, const std::vector<std::shared_ptr<expression_t>>& inputs){
	expression_t result;
	function_call_expr_t r = function_call_expr_t{ function_name, inputs };
	result._call_function_expr = std::make_shared<function_call_expr_t>(r);
	return result;
}


expression_t make_variable_read(const expression_t& address_expression){
	QUARK_ASSERT(address_expression.check_invariant());

	expression_t result;
	auto address = make_shared<expression_t>(address_expression);
	variable_read_expr_t r = variable_read_expr_t{address};
	result._variable_read_expr = std::make_shared<variable_read_expr_t>(r);
	return result;
}

expression_t make_variable_read_variable(const std::string& name){
	const auto resolve = make_resolve_member(name);
	return make_variable_read(resolve);
}


expression_t make_resolve_member(const std::string& member_name){
	expression_t result;
	resolve_member_expr_t r = resolve_member_expr_t{ member_name };
	result._resolve_member_expr = std::make_shared<resolve_member_expr_t>(r);
	return result;
}

expression_t make_lookup(const expression_t& lookup_key){
	QUARK_ASSERT(lookup_key.check_invariant());

	expression_t result;
	auto address = make_shared<expression_t>(lookup_key);
	lookup_element_expr_t r = lookup_element_expr_t{address};
	result._lookup_element_expr = std::make_shared<lookup_element_expr_t>(r);
	return result;
}



string operation_to_string(const math_operation2_expr_t::operation& op){
	if(op == math_operation2_expr_t::add){
		return "+";
	}
	else if(op == math_operation2_expr_t::subtract){
		return "-";
	}
	else if(op == math_operation2_expr_t::multiply){
		return "*";
	}
	else if(op == math_operation2_expr_t::divide){
		return "/";
	}
	else{
		QUARK_ASSERT(false);
	}
}

string operation_to_string(const math_operation1_expr_t::operation& op){
	if(op == math_operation1_expr_t::negate){
		return "negate";
	}
	else{
		QUARK_ASSERT(false);
	}
}





void trace(const math_operation2_expr_t& e){
	string s = "math_operation2_expr_t: " + operation_to_string(e._operation);
	QUARK_SCOPED_TRACE(s);
	trace(*e._left);
	trace(*e._right);
}

void trace(const math_operation1_expr_t& e){
	string s = "math_operation1_expr_t: " + operation_to_string(e._operation);
	QUARK_SCOPED_TRACE(s);
	trace(*e._input);
}

void trace(const function_call_expr_t& e){
	string s = "function_call_expr_t: " + e._function_name;
	QUARK_SCOPED_TRACE(s);
	for(const auto i: e._inputs){
		trace(*i);
	}
}

void trace(const variable_read_expr_t& e){
	string s = "variable_read_expr_t: address:";
	QUARK_SCOPED_TRACE(s);
	trace(*e._address);
}

void trace(const resolve_member_expr_t& e){
	QUARK_TRACE_SS("resolve_member_expr_t: " << e._member_name);
}

void trace(const lookup_element_expr_t& e){
	string s = "lookup_element_expr_t: ";
	QUARK_SCOPED_TRACE(s);
	trace(*e._lookup_key);
}



void trace(const expression_t& e){
	QUARK_ASSERT(e.check_invariant());

	if(e._constant){
		trace(*e._constant);
	}
	else if(e._math_operation2_expr){
		trace(*e._math_operation2_expr);
	}
	else if(e._math_operation1_expr){
		trace(*e._math_operation1_expr);
	}
	else if(e._call_function_expr){
		trace(*e._call_function_expr);
	}
	else if(e._variable_read_expr){
		trace(*e._variable_read_expr);
	}
	else if(e._resolve_member_expr){
		trace(*e._resolve_member_expr);
	}
	else if(e._lookup_element_expr){
		trace(*e._lookup_element_expr);
	}
	else{
		QUARK_ASSERT(false);
	}
}



std::string expression_to_string(const expression_t& e){
	QUARK_ASSERT(e.check_invariant());

	if(e._constant){
		return string("(@k ") + e._constant->value_and_type_to_string() + ")";
	}
	else if(e._math_operation2_expr){
		const auto e2 = *e._math_operation2_expr;
		const auto left = expression_to_string(*e2._left);
		const auto right = expression_to_string(*e2._right);
		return string("(@math2 ") + left + operation_to_string(e2._operation) + right + ")";
	}
	else if(e._math_operation1_expr){
		const auto e2 = *e._math_operation1_expr;
		const auto input = expression_to_string(*e2._input);
		return string("(@math1 ") + operation_to_string(e2._operation) + input + ")";
	}

	/*
		If inputs are constant, replace function call with a constant!
	*/
	else if(e._call_function_expr){
		const auto& call_function = *e._call_function_expr;

//		const auto& function_def = ast._types_collector.resolve_function_type(call_function_expression._function_name);

		string  args;
		for(const auto& i: call_function._inputs){
			const auto arg_expr = expression_to_string(*i);
			args = args + arg_expr;
		}
		return string("(@call ") + "'" + call_function._function_name + "'(" + args + "))";
	}
	else if(e._variable_read_expr){
		const auto e2 = *e._variable_read_expr;
		const auto address = expression_to_string(*e2._address);
		return string("(@read ") + address + ")";
	}
	else if(e._resolve_member_expr){
		const auto e2 = *e._resolve_member_expr;
		return string("(@resolve ") + "'" + e2._member_name + "'" + ")";
	}
	else if(e._lookup_element_expr){
		const auto e2 = *e._lookup_element_expr;
		const auto lookup_key = expression_to_string(*e2._lookup_key);
		return string("(@lookup ") + lookup_key + ")";
	}
	else{
		QUARK_ASSERT(false);
	}
}


QUARK_UNIT_TESTQ("expression_to_string()", "constants"){
	quark::ut_compare(expression_to_string(make_constant(13)), "(@k <int>13)");
	quark::ut_compare(expression_to_string(make_constant("xyz")), "(@k <string>'xyz')");
	quark::ut_compare(expression_to_string(make_constant(14.0f)), "(@k <float>14.000000)");
}

QUARK_UNIT_TESTQ("expression_to_string()", "math1"){
	quark::ut_compare(
		expression_to_string(
			make_math_operation1(math_operation1_expr_t::operation::negate, make_constant(2))),
		"(@math1 negate(@k <int>2))"
	);
}

QUARK_UNIT_TESTQ("expression_to_string()", "math2"){
	quark::ut_compare(
		expression_to_string(
			make_math_operation2(math_operation2_expr_t::operation::add, make_constant(2), make_constant(3))),
		"(@math2 (@k <int>2)+(@k <int>3))"
	);
}

QUARK_UNIT_TESTQ("expression_to_string()", "call"){
	quark::ut_compare(
		expression_to_string(
			make_function_call("my_func", { make_constant("xyz"), make_constant(123) })
		),
		"(@call 'my_func'((@k <string>'xyz')(@k <int>123)))"
	);
}

QUARK_UNIT_TESTQ("expression_to_string()", "read & resolve"){
	quark::ut_compare(
		expression_to_string(
			make_variable_read_variable("param1")
		),
		"(@read (@resolve 'param1'))"
	);
}

QUARK_UNIT_TESTQ("expression_to_string()", "lookup"){
	quark::ut_compare(
		expression_to_string(
			make_lookup(make_constant("xyz"))
		),
		"(@lookup (@k <string>'xyz'))"
	);
}



}
