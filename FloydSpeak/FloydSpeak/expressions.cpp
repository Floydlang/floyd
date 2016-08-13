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
#include "json_writer.h"
#include "utils.h"
#include "json_support.h"
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

bool load_expr_t::operator==(const load_expr_t& other) const{
	return *_address == *other._address ;
}

bool resolve_variable_expr_t::operator==(const resolve_variable_expr_t& other) const{
	return _variable_name == other._variable_name;
}



bool resolve_struct_member_expr_t::operator==(const resolve_struct_member_expr_t& other) const{
	return *_parent_address == *other._parent_address && _member_name == other._member_name;
}


bool lookup_element_expr_t::operator==(const lookup_element_expr_t& other) const{
	return *_parent_address == *other._parent_address && *_lookup_key == *other._lookup_key ;
}





//////////////////////////////////////////////////		expression_t



bool expression_t::check_invariant() const{
	QUARK_ASSERT(_debug_aaaaaaaaaaaaaaaaaaaaaaa.size() > 0);

	//	Make sure exactly ONE pointer is set.
	QUARK_ASSERT((_constant ? 1 : 0) + (_math1 ? 1 : 0) + (_math2 ? 1 : 0) + (_call ? 1 : 0) + (_load ? 1 : 0) + (_resolve_variable ? 1 : 0) + (_resolve_struct_member ? 1 : 0) + (_lookup_element ? 1 : 0) == 1);

	return true;
}

bool expression_t::operator==(const expression_t& other) const {
	QUARK_ASSERT(check_invariant());
	QUARK_ASSERT(other.check_invariant());

	if(_constant){
		return other._constant && *_constant == *other._constant;
	}
	else if(_math1){
		return other._math1 && *_math1 == *other._math1;
	}
	else if(_math2){
		return other._math2 && *_math2 == *other._math2;
	}
	else if(_call){
		return other._call && *_call == *other._call;
	}
	else if(_load){
		return other._load && *_load == *other._load;
	}
	else if(_resolve_variable){
		return other._resolve_variable && *_resolve_variable == *other._resolve_variable;
	}
	else if(_resolve_struct_member){
		return other._resolve_struct_member && *_resolve_struct_member == *other._resolve_struct_member;
	}
	else if(_lookup_element){
		return other._lookup_element && *_lookup_element == *other._lookup_element;
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


expression_t make_load(const expression_t& address_expression){
	QUARK_ASSERT(address_expression.check_invariant());

	auto address = make_shared<expression_t>(address_expression);
	return std::make_shared<load_expr_t>(load_expr_t{address});
}

expression_t make_load_variable(const std::string& name){
	QUARK_ASSERT(name.size() > 0);

	const auto address = make_resolve_variable(name);
	return make_load(address);
}


//??? Better handling of input strings to the expression. Validate etc. Use identifier_t etc.

expression_t make_resolve_variable(const std::string& variable){
	QUARK_ASSERT(variable.size() > 0);

	return std::make_shared<resolve_variable_expr_t>(resolve_variable_expr_t{ variable });
}


expression_t make_resolve_struct_member(const shared_ptr<expression_t>& parent_address, const std::string& member_name){
	QUARK_ASSERT(parent_address && parent_address->check_invariant());
	QUARK_ASSERT(member_name.size() > 0);

	resolve_struct_member_expr_t r = resolve_struct_member_expr_t{ parent_address, member_name };
	return std::make_shared<resolve_struct_member_expr_t>(r);
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

void trace(const load_expr_t& e){
	string s = "load_expr_t: address:";
	QUARK_SCOPED_TRACE(s);
	trace(*e._address);
}

void trace(const resolve_variable_expr_t& e){
	QUARK_TRACE_SS("resolve_variable_expr_t: " << e._variable_name);
}

void trace(const resolve_struct_member_expr_t& e){
	QUARK_TRACE_SS("resolve_struct_member_expr_t: " << e._member_name);
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
	else if(e._math2){
		trace(*e._math2);
	}
	else if(e._math1){
		trace(*e._math1);
	}
	else if(e._call){
		trace(*e._call);
	}
	else if(e._load){
		trace(*e._load);
	}
	else if(e._resolve_variable){
		trace(*e._resolve_variable);
	}
	else if(e._resolve_struct_member){
		trace(*e._resolve_struct_member);
	}
	else if(e._lookup_element){
		trace(*e._lookup_element);
	}
	else{
		QUARK_ASSERT(false);
	}
}

json_value_t to_json(const value_t& v){
	if(v.is_null()){
		return json_value_t();
	}
	else if(v.is_bool()){
		return json_value_t(v.get_bool());
	}
	else if(v.is_int()){
		return json_value_t(static_cast<double>(v.get_int()));
	}
	else if(v.is_float()){
		return json_value_t(static_cast<double>(v.get_float()));
	}
	else if(v.is_string()){
		return json_value_t(v.get_string());
	}
	else if(v.is_struct_instance()){
#if false
		const auto struct_instance = v.get_struct_instance();
		const auto struct_def = struct_instance->__def;
		std::map<string, json_value_t> m;


		public: type_identifier_t _name;
		public: std::vector<member_t> _members;
		public: scope_ref_t _struct_scope;


		//??? scope_def should have names
		for(const auto member: struct_def->_members){
			const auto member_name = member._name;
			struct_instance->_member_values[member_name];
		}
		//??? Return
#endif
		return json_value_t();
	}
	else{
		QUARK_ASSERT(false);
	}
}

/*
	En expression is a json array where entries may be other json arrays.
	[+ [+ 1 2] [k 10]]

*/
json_value_t to_json_value(const expression_t& e){
	if(e._constant){
		return json_value_t(
			vector<json_value_t>{ json_value_t("\"k\""), to_json(*e._constant) }
		);
	}
	return json_value_t();
#if false
	else if(e._math2){
		const auto e2 = *e._math2;
		const auto left = to_json(*e2._left);
		const auto right = to_json(*e2._right);
		return to_json2({ quote(operation_to_string(e2._operation)), left, right });
	}
	else if(e._math1){
		const auto e2 = *e._math1;
		const auto input = to_json(*e2._input);
		return to_json2({ quote(operation_to_string(e2._operation)), input });
	}
	else if(e._call){
		const auto& call_function = *e._call;
		vector<string>  args_json;
		for(const auto& i: call_function._inputs){
			const auto arg_expr = to_json(*i);
			args_json.push_back(arg_expr);
		}
		return to_json2({ "\"call\"", quote(call_function._function_name), to_json2(args_json) });
	}
	else if(e._load){
		const auto e2 = *e._load;
		const auto address = to_json(*e2._address);
		return to_json2({ "\"load\"", address });
	}
	else if(e._resolve_variable){
		const auto e2 = *e._resolve_variable;
		return to_json2({ "\"res_var\"", quote(e2._variable_name) });
	}
	else if(e._resolve_struct_member){
		const auto e2 = *e._resolve_struct_member;
		return to_json2({ "\"res_member\"", to_json(*e2._parent_address), quote(e2._member_name) });
	}
	else if(e._lookup_element){
		const auto e2 = *e._lookup_element;
		const auto lookup_key = to_json(*e2._lookup_key);
		return to_json2({ "\"lookup\"", to_json(*e2._parent_address), lookup_key });
	}
	else{
		QUARK_ASSERT(false);
	}
#endif
}


std::string to_json(const expression_t& e){
	if(e._constant){
		return to_json2({ "\"k\"", e._constant->to_json() });
	}
	else if(e._math2){
		const auto e2 = *e._math2;
		const auto left = to_json(*e2._left);
		const auto right = to_json(*e2._right);
		return to_json2({ quote(operation_to_string(e2._operation)), left, right });
	}
	else if(e._math1){
		const auto e2 = *e._math1;
		const auto input = to_json(*e2._input);
		return to_json2({ quote(operation_to_string(e2._operation)), input });
	}
	else if(e._call){
		const auto& call_function = *e._call;
		vector<string>  args_json;
		for(const auto& i: call_function._inputs){
			const auto arg_expr = to_json(*i);
			args_json.push_back(arg_expr);
		}
		return to_json2({ "\"call\"", quote(call_function._function_name), to_json2(args_json) });
	}
	else if(e._load){
		const auto e2 = *e._load;
		const auto address = to_json(*e2._address);
		return to_json2({ "\"load\"", address });
	}
	else if(e._resolve_variable){
		const auto e2 = *e._resolve_variable;
		return to_json2({ "\"res_var\"", quote(e2._variable_name) });
	}
	else if(e._resolve_struct_member){
		const auto e2 = *e._resolve_struct_member;
		return to_json2({ "\"res_member\"", to_json(*e2._parent_address), quote(e2._member_name) });
	}
	else if(e._lookup_element){
		const auto e2 = *e._lookup_element;
		const auto lookup_key = to_json(*e2._lookup_key);
		return to_json2({ "\"lookup\"", to_json(*e2._parent_address), lookup_key });
	}
	else{
		QUARK_ASSERT(false);
	}
}

/*
[
  "+",
  [ "load", [ "res_member", [ "res_var", "p" ],"s" ] ],
  [ "load", [ "res_var", "a" ] ]
]
*/
QUARK_UNIT_TESTQ("to_json()", "call"){
	quark::ut_compare(
		to_json(
			make_function_call("my_func", { make_constant("xyz"), make_constant(123) })
		),
		"[ \"call\", \"my_func\", [ [ \"k\", \"xyz\" ], [ \"k\", 123 ] ] ]"
	);
}



std::string to_string(const expression_t& e){
//	QUARK_ASSERT(e.check_invariant());

	QUARK_TRACE(to_json(e));

	if(e._constant){
		return string("(@k ") + e._constant->value_and_type_to_string() + ")";
	}
	else if(e._math2){
		const auto e2 = *e._math2;
		const auto left = to_string(*e2._left);
		const auto right = to_string(*e2._right);
		return string("(@") + operation_to_string(e2._operation) + " " + left + " " + right + ")";
	}
	else if(e._math1){
		const auto e2 = *e._math1;
		const auto input = to_string(*e2._input);
		return string("(@") + operation_to_string(e2._operation) + " " + input + ")";
	}
	else if(e._call){
		const auto& call_function = *e._call;
		string  args;
		for(const auto& i: call_function._inputs){
			const auto arg_expr = to_string(*i);
			args = args + arg_expr;
		}
		return string("(@call ") + "'" + call_function._function_name + "'(" + args + "))";
	}
	else if(e._load){
		const auto e2 = *e._load;
		const auto address = to_string(*e2._address);
		return string("(@load ") + address + ")";
	}
	else if(e._resolve_variable){
		const auto e2 = *e._resolve_variable;
		return string("(@res_var '") + e2._variable_name + "'" + ")";
	}
	else if(e._resolve_struct_member){
		const auto e2 = *e._resolve_struct_member;
		return string("(@res_member ") + to_string(*e2._parent_address) + " '" + e2._member_name + "'" + ")";
	}
	else if(e._lookup_element){
		const auto e2 = *e._lookup_element;
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
		"(@negate (@k <int>2))"
	);
}

QUARK_UNIT_TESTQ("to_string()", "math2"){
	quark::ut_compare(
		to_string(
			make_math_operation2(math_operation2_expr_t::operation::add, make_constant(2), make_constant(3))),
		"(@+ (@k <int>2) (@k <int>3))"
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

QUARK_UNIT_TESTQ("to_string()", "read & resolve_variable"){
	quark::ut_compare(
		to_string(
			make_load_variable("param1")
		),
		"(@load (@res_var 'param1'))"
	);
}
//??? test all addressing.
QUARK_UNIT_TESTQ("to_string()", "lookup"){
	quark::ut_compare(
		to_string(
			make_lookup(make_resolve_variable("hello"), make_constant("xyz"))
		),
		"(@lookup (@res_var 'hello') (@k <string>'xyz'))"
	);
}





}
