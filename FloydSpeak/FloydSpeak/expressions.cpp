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

bool resolve_member_expr_t::operator==(const resolve_member_expr_t& other) const{
	//	Parent can by null or a proper expression.
	bool parent = (!_parent_address && !other._parent_address) || (_parent_address && other._parent_address && *_parent_address == *other._parent_address);
	return parent && _member_name == other._member_name ;
}


bool lookup_element_expr_t::operator==(const lookup_element_expr_t& other) const{
	return *_parent_address == *other._parent_address && *_lookup_key == *other._lookup_key ;
}





//////////////////////////////////////////////////		expression_t



bool expression_t::check_invariant() const{
	QUARK_ASSERT(_debug.size() > 0);

	//	Make sure exactly ONE pointer is set.
	QUARK_ASSERT((_constant ? 1 : 0) + (_math_operation1_expr ? 1 : 0) + (_math_operation2_expr ? 1 : 0) + (_call_function_expr ? 1 : 0) + (_variable_read_expr ? 1 : 0) + (_resolve_member_expr ? 1 : 0) + (_lookup_element_expr ? 1 : 0) == 1);

	return true;
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

	return std::make_shared<value_t>(value);
}

expression_t make_constant(const std::string& s){
	return std::make_shared<value_t>(value_t(s));
}

expression_t make_constant(const int i){
	return std::make_shared<value_t>(value_t(i));
}

expression_t make_constant(const float f){
	return std::make_shared<value_t>(value_t(f));
}



expression_t make_math_operation1(math_operation1_expr_t::operation op, const expression_t& input){
	QUARK_ASSERT(input.check_invariant());

	auto input2 = make_shared<expression_t>(input);
	return std::make_shared<math_operation1_expr_t>(math_operation1_expr_t{ op, input2 });
}

expression_t make_math_operation2(math_operation2_expr_t::operation op, const expression_t& left, const expression_t& right){
	QUARK_ASSERT(left.check_invariant());
	QUARK_ASSERT(right.check_invariant());

	auto left2 = make_shared<expression_t>(left);
	auto right2 = make_shared<expression_t>(right);
	return std::make_shared<math_operation2_expr_t>(math_operation2_expr_t{ op, left2, right2 });
}



expression_t make_function_call(const std::string& function_name, const std::vector<expression_t>& inputs){
	QUARK_ASSERT(function_name.size() > 0);
	for(const auto arg: inputs){
		QUARK_ASSERT(arg.check_invariant());
	}

	auto inputs2 = vector<shared_ptr<expression_t>>();
	for(auto arg: inputs){
		inputs2.push_back(make_shared<expression_t>(arg));
	}

	function_call_expr_t r = function_call_expr_t{ function_name, inputs2 };
	const auto a = std::make_shared<function_call_expr_t>(r);
	return expression_t(a);
}

expression_t make_function_call(const std::string& function_name, const std::vector<std::shared_ptr<expression_t>>& inputs){
	function_call_expr_t r = function_call_expr_t{ function_name, inputs };
	const auto a = std::make_shared<function_call_expr_t>(r);
	return expression_t(a);
}


expression_t make_variable_read(const expression_t& address_expression){
	QUARK_ASSERT(address_expression.check_invariant());

	auto address = make_shared<expression_t>(address_expression);
	return std::make_shared<variable_read_expr_t>(variable_read_expr_t{address});
}

expression_t make_variable_read_variable(const std::string& name){
	QUARK_ASSERT(name.size() > 0);

	const auto address = make_resolve_member(name);
	return make_variable_read(address);
}

expression_t make_resolve_member(const shared_ptr<expression_t>& parent_address, const std::string& member_name){
	QUARK_ASSERT(!parent_address || parent_address->check_invariant());
	QUARK_ASSERT(member_name.size() > 0);

	resolve_member_expr_t r = resolve_member_expr_t{ parent_address, member_name };
	return std::make_shared<resolve_member_expr_t>(r);
}

expression_t make_resolve_member(const std::string& member_name){
	QUARK_ASSERT(member_name.size() > 0);

	return make_resolve_member(shared_ptr<expression_t>(), member_name);
}

expression_t make_lookup(const expression_t& parent_address, const expression_t& lookup_key){
	QUARK_ASSERT(parent_address.check_invariant());
	QUARK_ASSERT(lookup_key.check_invariant());

	auto parent_address2 = make_shared<expression_t>(parent_address);
	auto lookup_key2 = make_shared<expression_t>(lookup_key);
	return std::make_shared<lookup_element_expr_t>(lookup_element_expr_t{parent_address2, lookup_key2});
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



std::string to_string(const expression_t& e){
//	QUARK_ASSERT(e.check_invariant());

	if(e._constant){
		return string("(@k ") + e._constant->value_and_type_to_string() + ")";
	}
	else if(e._math_operation2_expr){
		const auto e2 = *e._math_operation2_expr;
		const auto left = to_string(*e2._left);
		const auto right = to_string(*e2._right);
		return string("(@math2 ") + left + operation_to_string(e2._operation) + right + ")";
	}
	else if(e._math_operation1_expr){
		const auto e2 = *e._math_operation1_expr;
		const auto input = to_string(*e2._input);
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
			const auto arg_expr = to_string(*i);
			args = args + arg_expr;
		}
		return string("(@call ") + "'" + call_function._function_name + "'(" + args + "))";
	}
	else if(e._variable_read_expr){
		const auto e2 = *e._variable_read_expr;
		const auto address = to_string(*e2._address);
		return string("(@read ") + address + ")";
	}
	else if(e._resolve_member_expr){
		const auto e2 = *e._resolve_member_expr;
		return string("(@resolve ") + (e2._parent_address ? to_string(*e2._parent_address) : "nullptr") + " '" + e2._member_name + "'" + ")";
	}
	else if(e._lookup_element_expr){
		const auto e2 = *e._lookup_element_expr;
		const auto lookup_key = to_string(*e2._lookup_key);
		return string("(@lookup ") + to_string(*e2._parent_address) + " " + lookup_key + ")";
	}
	else{
		QUARK_ASSERT(false);
	}
}


QUARK_UNIT_TESTQ("to_string()", "constants"){
	quark::ut_compare(to_string(make_constant(13)), "(@k <int>13)");
	quark::ut_compare(to_string(make_constant("xyz")), "(@k <string>'xyz')");
	quark::ut_compare(to_string(make_constant(14.0f)), "(@k <float>14.000000)");
}

QUARK_UNIT_TESTQ("to_string()", "math1"){
	quark::ut_compare(
		to_string(
			make_math_operation1(math_operation1_expr_t::operation::negate, make_constant(2))),
		"(@math1 negate(@k <int>2))"
	);
}

QUARK_UNIT_TESTQ("to_string()", "math2"){
	quark::ut_compare(
		to_string(
			make_math_operation2(math_operation2_expr_t::operation::add, make_constant(2), make_constant(3))),
		"(@math2 (@k <int>2)+(@k <int>3))"
	);
}

QUARK_UNIT_TESTQ("to_string()", "call"){
	quark::ut_compare(
		to_string(
			make_function_call("my_func", { make_constant("xyz"), make_constant(123) })
		),
		"(@call 'my_func'((@k <string>'xyz')(@k <int>123)))"
	);
}

QUARK_UNIT_TESTQ("to_string()", "read & resolve"){
	quark::ut_compare(
		to_string(
			make_variable_read_variable("param1")
		),
		"(@read (@resolve nullptr 'param1'))"
	);
}

QUARK_UNIT_TESTQ("to_string()", "lookup"){
	quark::ut_compare(
		to_string(
			make_lookup(make_resolve_member("hello"), make_constant("xyz"))
		),
		"(@lookup (@resolve nullptr 'hello') (@k <string>'xyz'))"
	);
}



}
