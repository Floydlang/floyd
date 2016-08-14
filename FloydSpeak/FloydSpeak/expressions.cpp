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
#include "parser_primitives.h"


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

void trace(const expression_t& e){
	QUARK_ASSERT(e.check_invariant());
	const auto json = expression_to_json(e);
	const auto s = json_to_compact_string(json);
	QUARK_TRACE(s);
}




////////////////////////////////////////////		JSON SUPPORT




/*
	En expression is a json array where entries may be other json arrays.
	[+ [+ 1 2] [k 10]]

*/
json_value_t expression_to_json(const expression_t& e){
	if(e._constant){
		return json_value_t(
			vector<json_value_t>{ json_value_t("k"), value_to_json(*e._constant) }
		);
	}
	else if(e._math2){
		const auto e2 = *e._math2;
		const auto left = expression_to_json(*e2._left);
		const auto right = expression_to_json(*e2._right);
		return json_value_t({ json_value_t(operation_to_string(e2._operation)), left, right });
	}
	else if(e._math1){
		const auto e2 = *e._math1;
		const auto input = expression_to_json(*e2._input);
		return json_value_t(vector<json_value_t>{ json_value_t(operation_to_string(e2._operation)), input });
	}
	else if(e._call){
		const auto& call_function = *e._call;
		vector<json_value_t>  args_json;
		for(const auto& i: call_function._inputs){
			const auto arg_expr = expression_to_json(*i);
			args_json.push_back(arg_expr);
		}
		return json_value_t({ json_value_t("call"), json_value_t(call_function._function_name), args_json });
	}
	else if(e._load){
		const auto e2 = *e._load;
		const auto address = expression_to_json(*e2._address);
		return json_value_t::make_array({ json_value_t("load"), address });
	}
	else if(e._resolve_variable){
		const auto e2 = *e._resolve_variable;
		return json_value_t::make_array({ json_value_t("res_var"), json_value_t(e2._variable_name) });
	}
	else if(e._resolve_struct_member){
		const auto e2 = *e._resolve_struct_member;
		return json_value_t::make_array({ json_value_t("res_member"), expression_to_json(*e2._parent_address), json_value_t(e2._member_name) });
	}
	else if(e._lookup_element){
		const auto e2 = *e._lookup_element;
		const auto lookup_key = expression_to_json(*e2._lookup_key);
		const auto parent_address = expression_to_json(*e2._parent_address);
		return json_value_t::make_array({ json_value_t("lookup"), parent_address, lookup_key });
	}
	else{
		QUARK_ASSERT(false);
	}
}

string expression_to_json_string(const expression_t& e){
	const auto json = expression_to_json(e);
	return json_to_compact_string(json);
}

QUARK_UNIT_TESTQ("expression_to_json()", "constants"){
	quark::ut_compare(expression_to_json_string(make_constant(13)), R"(["k", 13])");
	quark::ut_compare(expression_to_json_string(make_constant("xyz")), R"(["k", "xyz"])");
	quark::ut_compare(expression_to_json_string(make_constant(14.0f)), R"(["k", 14])");
}

QUARK_UNIT_TESTQ("expression_to_json()", "math1"){
	quark::ut_compare(
		expression_to_json_string(
			make_math_operation1(math_operation1_expr_t::operation::negate, make_constant(2))),
		R"(["negate", ["k", 2]])"
	);
}

QUARK_UNIT_TESTQ("expression_to_json()", "math2"){
	quark::ut_compare(
		expression_to_json_string(
			make_math_operation2(math_operation2_expr_t::operation::add, make_constant(2), make_constant(3))),
		R"(["+", ["k", 2], ["k", 3]])"
	);
}

QUARK_UNIT_TESTQ("expression_to_json()", "call"){
	quark::ut_compare(
		expression_to_json_string(
			make_function_call("my_func", { make_constant("xyz"), make_constant(123) })
		),
		R"(["call", "my_func", [["k", "xyz"], ["k", 123]]])"
	);
}

QUARK_UNIT_TESTQ("expression_to_json()", "read & resolve_variable"){
	quark::ut_compare(
		expression_to_json_string(
			make_load_variable("param1")
		),
		R"(["load", ["res_var", "param1"]])"
	);
}

QUARK_UNIT_TESTQ("expression_to_json()", "lookup"){
	quark::ut_compare(
		expression_to_json_string(
			make_lookup(make_resolve_variable("hello"), make_constant("xyz"))
		),
		R"(["lookup", ["res_var", "hello"], ["k", "xyz"]])"
	);
}


}	//	floyd_parser

