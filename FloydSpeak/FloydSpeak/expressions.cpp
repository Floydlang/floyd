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
	const auto a = expression_t::make_math_operation2(
		math_operation2_expr_t::add,
		expression_t::make_constant(3),
		expression_t::make_constant(4));
	const auto b = expression_t::make_math_operation2(
		math_operation2_expr_t::add,
		expression_t::make_constant(3),
		expression_t::make_constant(4));
	QUARK_TEST_VERIFY(a == b);
}



bool math_operation2_expr_t::operator==(const math_operation2_expr_t& other) const {
	return _operation == other._operation && *_left == *other._left && *_right == *other._right;
}

bool math_operation1_expr_t::operator==(const math_operation1_expr_t& other) const {
	return _operation == other._operation && *_input == *other._input;
}



bool operator_question_colon_expr_t::operator==(const operator_question_colon_expr_t& other) const {
	return *_condition == *other._condition && *_a == *other._a && *_b == *other._b;
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
	QUARK_ASSERT((_constant ? 1 : 0) + (_math1 ? 1 : 0) + (_math2 ? 1 : 0) + (_operator_question_colon ? 1 : 0) + (_call ? 1 : 0) + (_load ? 1 : 0) + (_resolve_variable ? 1 : 0) + (_resolve_struct_member ? 1 : 0) + (_lookup_element ? 1 : 0) == 1);

	return true;
}

bool expression_t::operator==(const expression_t& other) const {
	QUARK_ASSERT(check_invariant());
	QUARK_ASSERT(other.check_invariant());

	if(_constant){
		return compare_shared_values(_constant, other._constant);
	}
	else if(_math1){
		return compare_shared_values(_math1, other._math1);
	}
	else if(_math2){
		return compare_shared_values(_math2, other._math2);
	}
	else if(_operator_question_colon){
		return compare_shared_values(_operator_question_colon, other._operator_question_colon);
	}
	else if(_call){
		return compare_shared_values(_call, other._call);
	}
	else if(_load){
		return compare_shared_values(_load, other._load);
	}
	else if(_resolve_variable){
		return compare_shared_values(_resolve_variable, other._resolve_variable);
	}
	else if(_resolve_struct_member){
		return compare_shared_values(_resolve_struct_member, other._resolve_struct_member);
	}
	else if(_lookup_element){
		return compare_shared_values(_lookup_element, other._lookup_element);
	}
	else{
		QUARK_ASSERT(false);
		return false;
	}
}




expression_t expression_t::make_constant(const value_t& value, const type_identifier_t& resolved_expression_type){
	QUARK_ASSERT(value.check_invariant());
	QUARK_ASSERT(resolved_expression_type.check_invariant());

	auto result = expression_t();
	result._constant = std::make_shared<value_t>(value);
	result._resolved_expression_type = resolved_expression_type.is_null() ? value.get_type() : resolved_expression_type;
	result._debug_aaaaaaaaaaaaaaaaaaaaaaa = expression_to_json_string(result);
	QUARK_ASSERT(result.check_invariant());
	return result;
}

expression_t expression_t::make_constant(const std::string& s){
	return make_constant(value_t(s));
}

expression_t expression_t::make_constant(const int i){
	return make_constant(value_t(i));
}

expression_t expression_t::make_constant(const float f){
	return make_constant(value_t(f));
}



expression_t expression_t::make_math_operation1(math_operation1_expr_t::operation op, const expression_t& input, const type_identifier_t& resolved_expression_type){
	QUARK_ASSERT(input.check_invariant());
	QUARK_ASSERT(resolved_expression_type.check_invariant());

	auto input2 = make_shared<expression_t>(input);

	auto result = expression_t();
	result._math1 = std::make_shared<math_operation1_expr_t>(math_operation1_expr_t{ op, input2 });
	result._resolved_expression_type = resolved_expression_type;
	result._debug_aaaaaaaaaaaaaaaaaaaaaaa = expression_to_json_string(result);
	QUARK_ASSERT(result.check_invariant());
	return result;
}

expression_t expression_t::make_math_operation2(math_operation2_expr_t::operation op, const expression_t& left, const expression_t& right, const type_identifier_t& resolved_expression_type){
	QUARK_ASSERT(left.check_invariant());
	QUARK_ASSERT(right.check_invariant());
	QUARK_ASSERT(resolved_expression_type.check_invariant());

	auto left2 = make_shared<expression_t>(left);
	auto right2 = make_shared<expression_t>(right);

	auto result = expression_t();
	result._math2 = std::make_shared<math_operation2_expr_t>(math_operation2_expr_t{ op, left2, right2 });
	result._resolved_expression_type = resolved_expression_type;
	result._debug_aaaaaaaaaaaaaaaaaaaaaaa = expression_to_json_string(result);
	QUARK_ASSERT(result.check_invariant());
	return result;
}

expression_t expression_t::make_operator_question_colon(const expression_t& condition, const expression_t& a, const expression_t& b, const type_identifier_t& resolved_expression_type){
	QUARK_ASSERT(condition.check_invariant());
	QUARK_ASSERT(a.check_invariant());
	QUARK_ASSERT(b.check_invariant());
	QUARK_ASSERT(resolved_expression_type.check_invariant());

	auto condition2 = make_shared<expression_t>(condition);
	auto a2 = make_shared<expression_t>(a);
	auto b2 = make_shared<expression_t>(b);

	auto result = expression_t();
	result._operator_question_colon = std::make_shared<operator_question_colon_expr_t>(operator_question_colon_expr_t{ condition2, a2,b2 });
	result._resolved_expression_type = resolved_expression_type;
	result._debug_aaaaaaaaaaaaaaaaaaaaaaa = expression_to_json_string(result);
	QUARK_ASSERT(result.check_invariant());
	return result;
}

expression_t expression_t::make_function_call(const type_identifier_t& function, const std::vector<std::shared_ptr<expression_t>>& inputs, const type_identifier_t& resolved_expression_type){
	QUARK_ASSERT(function.check_invariant());
	for(const auto arg: inputs){
		QUARK_ASSERT(arg && arg->check_invariant());
	}
	QUARK_ASSERT(resolved_expression_type.check_invariant());

	auto result = expression_t();
	result._call = std::make_shared<function_call_expr_t>(function_call_expr_t{ function, inputs });
	result._resolved_expression_type = resolved_expression_type;
	result._debug_aaaaaaaaaaaaaaaaaaaaaaa = expression_to_json_string(result);
	QUARK_ASSERT(result.check_invariant());
	return result;
}



expression_t expression_t::make_load(const expression_t& address_expression, const type_identifier_t& resolved_expression_type){
	QUARK_ASSERT(address_expression.check_invariant());
	QUARK_ASSERT(resolved_expression_type.check_invariant());

	auto result = expression_t();
	auto address = make_shared<expression_t>(address_expression);
	result._load = std::make_shared<load_expr_t>(load_expr_t{ address });
	result._resolved_expression_type = resolved_expression_type;
	result._debug_aaaaaaaaaaaaaaaaaaaaaaa = expression_to_json_string(result);
	QUARK_ASSERT(result.check_invariant());
	return result;
}

expression_t expression_t::make_load_variable(const std::string& name){
	QUARK_ASSERT(name.size() > 0);

	const auto address = make_resolve_variable(name, type_identifier_t());
	return make_load(address, type_identifier_t());
}

expression_t expression_t::make_resolve_variable(const std::string& variable, const type_identifier_t& resolved_expression_type){
	QUARK_ASSERT(variable.size() > 0);
	QUARK_ASSERT(resolved_expression_type.check_invariant());

	auto result = expression_t();
	result._resolve_variable = std::make_shared<resolve_variable_expr_t>(resolve_variable_expr_t{ variable });
	result._resolved_expression_type = resolved_expression_type;
	result._debug_aaaaaaaaaaaaaaaaaaaaaaa = expression_to_json_string(result);
	QUARK_ASSERT(result.check_invariant());
	return result;
}


expression_t expression_t::make_resolve_struct_member(const shared_ptr<expression_t>& parent_address, const std::string& member_name, const type_identifier_t& resolved_expression_type){
	QUARK_ASSERT(parent_address && parent_address->check_invariant());
	QUARK_ASSERT(member_name.size() > 0);
	QUARK_ASSERT(resolved_expression_type.check_invariant());

	auto result = expression_t();
	result._resolve_struct_member = std::make_shared<resolve_struct_member_expr_t>(resolve_struct_member_expr_t{ parent_address, member_name });
	result._resolved_expression_type = resolved_expression_type;
	result._debug_aaaaaaaaaaaaaaaaaaaaaaa = expression_to_json_string(result);
	QUARK_ASSERT(result.check_invariant());
	return result;
}

expression_t expression_t::make_lookup(const expression_t& parent_address, const expression_t& lookup_key, const type_identifier_t& resolved_expression_type){
	QUARK_ASSERT(parent_address.check_invariant());
	QUARK_ASSERT(lookup_key.check_invariant());
	QUARK_ASSERT(resolved_expression_type.check_invariant());

	auto parent_address2 = make_shared<expression_t>(parent_address);
	auto lookup_key2 = make_shared<expression_t>(lookup_key);

	auto result = expression_t();
	result._lookup_element = std::make_shared<lookup_element_expr_t>(lookup_element_expr_t{ parent_address2, lookup_key2 });
	result._resolved_expression_type = resolved_expression_type;
	result._debug_aaaaaaaaaaaaaaaaaaaaaaa = expression_to_json_string(result);
	QUARK_ASSERT(result.check_invariant());
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
	const auto t = e._resolved_expression_type.to_string();
	json_value_t type;
	if(t == "null"){
		type = json_value_t("<>");
	}
	else{
		type = json_value_t(std::string("<") + t + ">");
	}

	if(e._constant){
		return json_value_t(
			vector<json_value_t>{ json_value_t("k"), type, value_to_json(*e._constant) }
		);
	}
	else if(e._math2){
		const auto e2 = *e._math2;
		const auto left = expression_to_json(*e2._left);
		const auto right = expression_to_json(*e2._right);
		return json_value_t({ json_value_t(operation_to_string(e2._operation)), type, left, right });
	}
	else if(e._math1){
		const auto e2 = *e._math1;
		const auto input = expression_to_json(*e2._input);
		return json_value_t(vector<json_value_t>{ json_value_t(operation_to_string(e2._operation)), type, input });
	}
	else if(e._operator_question_colon){
		const auto e2 = *e._operator_question_colon;
		const auto condition = expression_to_json(*e2._condition);
		const auto a = expression_to_json(*e2._a);
		const auto b = expression_to_json(*e2._b);
		return json_value_t({ json_value_t("?:"), condition, a, b });
	}
	else if(e._call){
		const auto& call_function = *e._call;
		vector<json_value_t>  args_json;
		for(const auto& i: call_function._inputs){
			const auto arg_expr = expression_to_json(*i);
			args_json.push_back(arg_expr);
		}
		return json_value_t({ json_value_t("call"), json_value_t(call_function._function.to_string()), type, args_json });
	}
	else if(e._load){
		const auto e2 = *e._load;
		const auto address = expression_to_json(*e2._address);
		return json_value_t::make_array({ json_value_t("load"), type, address });
	}
	else if(e._resolve_variable){
		const auto e2 = *e._resolve_variable;
		return json_value_t::make_array({ json_value_t("res_var"), type, json_value_t(e2._variable_name) });
	}
	else if(e._resolve_struct_member){
		const auto e2 = *e._resolve_struct_member;
		return json_value_t::make_array({ json_value_t("res_member"), type, expression_to_json(*e2._parent_address), json_value_t(e2._member_name) });
	}
	else if(e._lookup_element){
		const auto e2 = *e._lookup_element;
		const auto lookup_key = expression_to_json(*e2._lookup_key);
		const auto parent_address = expression_to_json(*e2._parent_address);
		return json_value_t::make_array({ json_value_t("lookup"), type, parent_address, lookup_key });
	}
	else{
		QUARK_ASSERT(false);
	}
}

/*
json_value_t expression_to_json(const expression_t& e){
	//	Add resolved type, if any.
	auto a = expression_to_json_internal(e);
	const auto t = e._resolved_expression_type.to_string();
	QUARK_ASSERT(a.is_array());
	auto a1 = a.get_array();


	const string type = e._resolved_expression_type != type_identifier_t() ? t : "?";
	const string type_string = std::string("<") + type + ">";

	a1.insert(a1.begin(), json_value_t(type_string));
	return json_value_t(a1);
}
*/

string expression_to_json_string(const expression_t& e){
	const auto json = expression_to_json(e);
	return json_to_compact_string(json);
}

QUARK_UNIT_TESTQ("expression_to_json()", "constants"){
	quark::ut_compare(expression_to_json_string(expression_t::make_constant(13)), R"(["k", "<int>", 13])");
	quark::ut_compare(expression_to_json_string(expression_t::make_constant("xyz")), R"(["k", "<string>", "xyz"])");
	quark::ut_compare(expression_to_json_string(expression_t::make_constant(14.0f)), R"(["k", "<float>", 14])");
}

QUARK_UNIT_TESTQ("expression_to_json()", "math1"){
	quark::ut_compare(
		expression_to_json_string(
			expression_t::make_math_operation1(math_operation1_expr_t::operation::negate, expression_t::make_constant(2))),
		R"(["negate", "<>", ["k", "<int>", 2]])"
	);
}

QUARK_UNIT_TESTQ("expression_to_json()", "math2"){
	quark::ut_compare(
		expression_to_json_string(
			expression_t::make_math_operation2(math_operation2_expr_t::operation::add, expression_t::make_constant(2), expression_t::make_constant(3))),
		R"(["+", "<>", ["k", "<int>", 2], ["k", "<int>", 3]])"
	);
}

QUARK_UNIT_TESTQ("expression_to_json()", "call"){
	quark::ut_compare(
		expression_to_json_string(
			expression_t::make_function_call(
				type_identifier_t::make("my_func"),
				{
					make_shared<expression_t>(expression_t::make_constant("xyz")),
					make_shared<expression_t>(expression_t::make_constant(123))
				},
				type_identifier_t()
			)
		),
		R"(["call", "my_func", "<>", [["k", "<string>", "xyz"], ["k", "<int>", 123]]])"
	);
}

QUARK_UNIT_TESTQ("expression_to_json()", "read & resolve_variable"){
	quark::ut_compare(
		expression_to_json_string(
			expression_t::make_load_variable("param1")
		),
		R"(["load", "<>", ["res_var", "<>", "param1"]])"
	);
}

QUARK_UNIT_TESTQ("expression_to_json()", "lookup"){
	quark::ut_compare(
		expression_to_json_string(
			expression_t::make_lookup(expression_t::make_resolve_variable("hello", type_identifier_t()), expression_t::make_constant("xyz"))
		),
		R"(["lookup", "<>", ["res_var", "<>", "hello"], ["k", "<string>", "xyz"]])"
	);
}



	//////////////////////////////////////////////////		visit()


/*
expression_t visit(const visit_expression_i& v, const expression_t& e){
	if(e._constant){
		return v.visit_expression_i__on_constant(*e._constant);
	}
	else if(e._math1){
		return v.visit_expression_i__on_math1(*e._math1);
	}
	else if(e._math2){
		return v.visit_expression_i__on_math2(*e._math2);
	}
	else if(e._call){
		return v.visit_expression_i__on_call(*e._call);
	}
	else if(e._load){
		return v.visit_expression_i__on_load(*e._load);
	}

	else if(e._resolve_variable){
		return v.visit_expression_i__on_resolve_variable(*e._resolve_variable);
	}
	else if(e._resolve_struct_member){
		return v.visit_expression_i__on_resolve_struct_member(*e._resolve_struct_member);
	}
	else if(e._lookup_element){
		return v.visit_expression_i__on_lookup_element(*e._lookup_element);
	}
	else{
		QUARK_ASSERT(false);
	}
}
*/




}	//	floyd_parser

