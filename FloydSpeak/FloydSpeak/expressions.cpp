//
//  expressions.cpp
//  FloydSpeak
//
//  Created by Marcus Zetterquist on 03/08/16.
//  Copyright © 2016 Marcus Zetterquist. All rights reserved.
//

#include "expressions.h"

#include "text_parser.h"
#include "statements.h"
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



string expression_to_json_string(const expression_t& e);



QUARK_UNIT_TEST("", "math_operation2_expr_t==()", "", ""){
	const auto a = expression_t::make_math_operation2(
		expression_t::math2_operation::k_math2_add,
		expression_t::make_constant(3),
		expression_t::make_constant(4));
	const auto b = expression_t::make_math_operation2(
		expression_t::math2_operation::k_math2_add,
		expression_t::make_constant(3),
		expression_t::make_constant(4));
	QUARK_TEST_VERIFY(a == b);
}

bool math_operation2_expr_t::operator==(const math_operation2_expr_t& other) const {
	return
		(_operation == other._operation)
		&& (_expressions == other._expressions)
		&& (compare_shared_values(_constant, other._constant))
		&& (_symbol == other._symbol);
}

bool lookup_element_expr_t::operator==(const lookup_element_expr_t& other) const{
	return _parent_address == other._parent_address && _lookup_key == other._lookup_key ;
}



//////////////////////////////////////////////////		expression_t



bool expression_t::check_invariant() const{
	QUARK_ASSERT(_debug_aaaaaaaaaaaaaaaaaaaaaaa.size() > 0);

	//	Make sure exactly ONE or ZERO pointers are set.
	const auto count =
		+ (_math2 ? 1 : 0)
		+ (_lookup_element ? 1 : 0);

	QUARK_ASSERT(count == 0 || count == 1);

	QUARK_ASSERT(count == 0 || (_resolved_expression_type && _resolved_expression_type->check_invariant()));
	return true;
}

bool expression_t::operator==(const expression_t& other) const {
	QUARK_ASSERT(check_invariant());
	QUARK_ASSERT(other.check_invariant());

	if(is_nop() && other.is_nop()){
		return true;
	}

	if(_math2){
		return compare_shared_values(_math2, other._math2);
	}
	else if(_lookup_element){
		return compare_shared_values(_lookup_element, other._lookup_element);
	}
	else{
		QUARK_ASSERT(false);
		return false;
	}
}



expression_t expression_t::make_nop(){
	auto result = expression_t();
	result._debug_aaaaaaaaaaaaaaaaaaaaaaa = "NOP";
	QUARK_ASSERT(result.check_invariant());
	return result;
}

bool expression_t::is_nop() const{
//	QUARK_ASSERT(check_invariant());

	const auto count =
		(_math2 ? 1 : 0)
		+ (_lookup_element ? 1 : 0);

	return count == 0;
}




expression_t expression_t::make_constant(const value_t& value){
	QUARK_ASSERT(value.check_invariant());

	const auto resolved_expression_type = value.get_type();
	auto result = expression_t();
	result._math2 = std::make_shared<math_operation2_expr_t>(
		math_operation2_expr_t{ math2_operation::k_constant, {}, make_shared<value_t>(value), {} }
	);
	result._resolved_expression_type = value.get_type();
	result._debug_aaaaaaaaaaaaaaaaaaaaaaa = expression_to_json_string(result);
	QUARK_ASSERT(result.check_invariant());
	return result;
}


expression_t expression_t::make_constant(const char s[]){
	return make_constant(value_t(s));
}
expression_t expression_t::make_constant(const std::string& s){
	return make_constant(value_t(s));
}

expression_t expression_t::make_constant(const int i){
	return make_constant(value_t(i));
}

expression_t expression_t::make_constant(const bool i){
	return make_constant(value_t(i));
}

expression_t expression_t::make_constant(const float f){
	return make_constant(value_t(f));
}

bool expression_t::is_constant() const{
	return _math2 && _math2->_operation == math2_operation::k_constant;
}

value_t expression_t::get_constant() const{
	QUARK_ASSERT(is_constant())

	return *_math2->_constant;
}

expression_t expression_t::make_math_operation2(
	math2_operation op,
	const std::vector<expression_t>& expressions,
	const std::shared_ptr<value_t>& constant,
	const std::string& symbol
)
{
	QUARK_ASSERT(!constant || constant->check_invariant());

	auto result = expression_t();
	result._math2 = std::make_shared<math_operation2_expr_t>(math_operation2_expr_t{ op, expressions, constant, symbol });
	result._resolved_expression_type = expressions[0].get_expression_type();
	result._debug_aaaaaaaaaaaaaaaaaaaaaaa = expression_to_json_string(result);
	QUARK_ASSERT(result.check_invariant());
	return result;
}


expression_t expression_t::make_conditional_operator(const expression_t& condition, const expression_t& a, const expression_t& b){
	QUARK_ASSERT(condition.check_invariant());
	QUARK_ASSERT(a.check_invariant());
	QUARK_ASSERT(b.check_invariant());

	return make_math_operation2(math2_operation::k_conditional_operator3, { condition, a, b }, {}, {});
}

expression_t expression_t::make_function_call(const type_identifier_t& function_name, const std::vector<expression_t>& inputs, const shared_ptr<const type_def_t>& resolved_expression_type){
	QUARK_ASSERT(function_name.check_invariant());
	for(const auto arg: inputs){
		QUARK_ASSERT(arg.check_invariant());
	}
	QUARK_ASSERT(resolved_expression_type && resolved_expression_type->check_invariant());

	auto result = expression_t();
	result._math2 = std::make_shared<math_operation2_expr_t>(math_operation2_expr_t{ math2_operation::k_call, inputs, {}, function_name.to_string() });
	result._resolved_expression_type = resolved_expression_type;
	result._debug_aaaaaaaaaaaaaaaaaaaaaaa = expression_to_json_string(result);
	QUARK_ASSERT(result.check_invariant());
	return result;
}


expression_t expression_t::make_resolve_variable(const std::string& variable, const shared_ptr<const type_def_t>& resolved_expression_type){
	QUARK_ASSERT(variable.size() > 0);
	QUARK_ASSERT(resolved_expression_type && resolved_expression_type->check_invariant());

	auto result = expression_t();
	result._math2 = std::make_shared<math_operation2_expr_t>(math_operation2_expr_t{ math2_operation::k_resolve_variable, {}, {}, variable });
	result._resolved_expression_type = resolved_expression_type;
	result._debug_aaaaaaaaaaaaaaaaaaaaaaa = expression_to_json_string(result);
	QUARK_ASSERT(result.check_invariant());
	return result;
}

expression_t expression_t::make_resolve_member(const expression_t& parent_address, const std::string& member_name, const shared_ptr<const type_def_t>& resolved_expression_type){
	QUARK_ASSERT(parent_address.check_invariant());
	QUARK_ASSERT(member_name.size() > 0);
	QUARK_ASSERT(resolved_expression_type && resolved_expression_type->check_invariant());

	auto result = expression_t();
	result._math2 = std::make_shared<math_operation2_expr_t>(math_operation2_expr_t{ math2_operation::k_resolve_member, { parent_address }, {}, member_name });
	result._resolved_expression_type = resolved_expression_type;
	result._debug_aaaaaaaaaaaaaaaaaaaaaaa = expression_to_json_string(result);
	QUARK_ASSERT(result.check_invariant());
	return result;
}

expression_t expression_t::make_lookup(const expression_t& parent_address, const expression_t& lookup_key, const shared_ptr<const type_def_t>& resolved_expression_type){
	QUARK_ASSERT(parent_address.check_invariant());
	QUARK_ASSERT(lookup_key.check_invariant());
	QUARK_ASSERT(resolved_expression_type && resolved_expression_type->check_invariant());

	auto result = expression_t();
	result._lookup_element = std::make_shared<lookup_element_expr_t>(lookup_element_expr_t{ parent_address, lookup_key });
	result._resolved_expression_type = resolved_expression_type;
	result._debug_aaaaaaaaaaaaaaaaaaaaaaa = expression_to_json_string(result);
	QUARK_ASSERT(result.check_invariant());
	return result;
}


//??? replace with map<string, expression_t::math2_operation> and map <expression_t::math2_operation, string>
string operation_to_string(const expression_t::math2_operation& op){

	if(op == expression_t::math2_operation::k_math2_add){
		return "+";
	}
	else if(op == expression_t::math2_operation::k_math2_subtract){
		return "-";
	}
	else if(op == expression_t::math2_operation::k_math2_multiply){
		return "*";
	}
	else if(op == expression_t::math2_operation::k_math2_divide){
		return "/";
	}
	else if(op == expression_t::math2_operation::k_math2_remainder){
		return "%";
	}

	else if(op == expression_t::math2_operation::k_math2_smaller_or_equal){
		return "<=";
	}
	else if(op == expression_t::math2_operation::k_math2_smaller){
		return "<";
	}
	else if(op == expression_t::math2_operation::k_math2_larger_or_equal){
		return ">=";
	}
	else if(op == expression_t::math2_operation::k_math2_larger){
		return ">";
	}

	else if(op == expression_t::math2_operation::k_logical_equal){
		return "==";
	}
	else if(op == expression_t::math2_operation::k_logical_nonequal){
		return "!=";
	}
	else if(op == expression_t::math2_operation::k_logical_and){
		return "&&";
	}
	else if(op == expression_t::math2_operation::k_logical_or){
		return "||";
	}
	else if(op == expression_t::math2_operation::k_logical_negate){
		return "negate";
	}


	else if(op == expression_t::math2_operation::k_constant){
		return "k";
	}
	else if(op == expression_t::math2_operation::k_conditional_operator3){
		return "?:";
	}
	else if(op == expression_t::math2_operation::k_call){
		return "call";
	}
	else if(op == expression_t::math2_operation::k_resolve_variable){
		return "@";
	}
	else if(op == expression_t::math2_operation::k_resolve_member){
		return "->";
	}

	else{
		QUARK_ASSERT(false);
	}
}

QUARK_UNIT_TESTQ("operation_to_string()", ""){
	quark::ut_compare(operation_to_string(expression_t::math2_operation::k_math2_add), "+");
}
QUARK_UNIT_TESTQ("operation_to_string()", ""){
	quark::ut_compare(operation_to_string(expression_t::math2_operation::k_math2_subtract), "-");
}
QUARK_UNIT_TESTQ("operation_to_string()", ""){
	quark::ut_compare(operation_to_string(expression_t::math2_operation::k_math2_multiply), "*");
}
QUARK_UNIT_TESTQ("operation_to_string()", ""){
	quark::ut_compare(operation_to_string(expression_t::math2_operation::k_math2_divide), "/");
}
QUARK_UNIT_TESTQ("operation_to_string()", ""){
	quark::ut_compare(operation_to_string(expression_t::math2_operation::k_math2_remainder), "%");
}


QUARK_UNIT_TESTQ("operation_to_string()", ""){
	quark::ut_compare(operation_to_string(expression_t::math2_operation::k_math2_smaller_or_equal), "<=");
}
QUARK_UNIT_TESTQ("operation_to_string()", ""){
	quark::ut_compare(operation_to_string(expression_t::math2_operation::k_math2_smaller), "<");
}
QUARK_UNIT_TESTQ("operation_to_string()", ""){
	quark::ut_compare(operation_to_string(expression_t::math2_operation::k_math2_larger_or_equal), ">=");
}
QUARK_UNIT_TESTQ("operation_to_string()", ""){
	quark::ut_compare(operation_to_string(expression_t::math2_operation::k_math2_larger), ">");
}

QUARK_UNIT_TESTQ("operation_to_string()", ""){
	quark::ut_compare(operation_to_string(expression_t::math2_operation::k_logical_equal), "==");
}
QUARK_UNIT_TESTQ("operation_to_string()", ""){
	quark::ut_compare(operation_to_string(expression_t::math2_operation::k_logical_nonequal), "!=");
}
QUARK_UNIT_TESTQ("operation_to_string()", ""){
	quark::ut_compare(operation_to_string(expression_t::math2_operation::k_logical_and), "&&");
}
QUARK_UNIT_TESTQ("operation_to_string()", ""){
	quark::ut_compare(operation_to_string(expression_t::math2_operation::k_logical_or), "||");
}
QUARK_UNIT_TESTQ("operation_to_string()", ""){
	quark::ut_compare(operation_to_string(expression_t::math2_operation::k_logical_negate), "negate");
}


QUARK_UNIT_TESTQ("operation_to_string()", ""){
	quark::ut_compare(operation_to_string(expression_t::math2_operation::k_constant), "k");
}
QUARK_UNIT_TESTQ("operation_to_string()", ""){
	quark::ut_compare(operation_to_string(expression_t::math2_operation::k_conditional_operator3), "?:");
}
QUARK_UNIT_TESTQ("operation_to_string()", ""){
	quark::ut_compare(operation_to_string(expression_t::math2_operation::k_call), "call");
}

QUARK_UNIT_TESTQ("operation_to_string()", ""){
	quark::ut_compare(operation_to_string(expression_t::math2_operation::k_resolve_variable), "@");
}
QUARK_UNIT_TESTQ("operation_to_string()", ""){
	quark::ut_compare(operation_to_string(expression_t::math2_operation::k_resolve_member), "->");
}

void trace(const expression_t& e){
	QUARK_ASSERT(e.check_invariant());
	const auto json = expression_to_json(e);
	const auto s = json_to_compact_string(json);
	QUARK_TRACE(s);
}




////////////////////////////////////////////		JSON SUPPORT



/*
	An expression is a json array where entries may be other json arrays.
	["+", ["+", 1, 2], ["k", 10]]
*/
json_t expression_to_json(const expression_t& e){
	if(e.is_nop()){
		return json_t();
	}
	else{
		const auto expression_base_type = e._resolved_expression_type->get_base_type();
		json_t type;
		if(expression_base_type == base_type::k_null){
			type = json_t();
		}
		else{
			const auto type_string = e._resolved_expression_type->to_string();
			type = std::string("<") + type_string + ">";
		}

		if(e.is_constant()){
			return json_t::make_array({ "k", value_to_json(e.get_constant()), type });
		}
		else if(e._math2){
			const auto e2 = *e._math2;

			vector<json_t>  expressions;
			for(const auto& i: e._math2->_expressions){
				const auto a = expression_to_json(i);
				expressions.push_back(a);
			}

			const auto constant = e2._constant ? value_to_json(*e2._constant) : json_t();
			const auto function_name = e2._symbol.empty() == false ? e2._symbol : json_t();

			auto result = json_t::make_array();
			result = push_back(result, operation_to_string(e2._operation));
			if(function_name.is_null() == false){
				result = push_back(result, function_name);
			}
			if(e2._operation == expression_t::math2_operation::k_call){
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
			result = push_back(result, type);
			return result;
		}
		else if(e._lookup_element){
			const auto e2 = *e._lookup_element;
			const auto lookup_key = expression_to_json(e2._lookup_key);
			const auto parent_address = expression_to_json(e2._parent_address);
			return json_t::make_array({ "[-]", parent_address, lookup_key, type });
		}
		else{
			QUARK_ASSERT(false);
		}
	}
}

string expression_to_json_string(const expression_t& e){
	const auto json = expression_to_json(e);
	return json_to_compact_string(json);
}

QUARK_UNIT_TESTQ("expression_to_json()", "constants"){
	quark::ut_compare(expression_to_json_string(expression_t::make_constant(13)), R"(["k", 13, "<int>"])");
	quark::ut_compare(expression_to_json_string(expression_t::make_constant("xyz")), R"(["k", "xyz", "<string>"])");
	quark::ut_compare(expression_to_json_string(expression_t::make_constant(14.0f)), R"(["k", 14, "<float>"])");
	quark::ut_compare(expression_to_json_string(expression_t::make_constant(true)), R"(["k", true, "<bool>"])");
	quark::ut_compare(expression_to_json_string(expression_t::make_constant(false)), R"(["k", false, "<bool>"])");
}

QUARK_UNIT_TESTQ("expression_to_json()", "math2"){
	quark::ut_compare(
		expression_to_json_string(
			expression_t::make_math_operation2(expression_t::math2_operation::k_math2_add, expression_t::make_constant(2), expression_t::make_constant(3))),
		R"(["+", ["k", 2, "<int>"], ["k", 3, "<int>"], "<int>"])"
	);
}

QUARK_UNIT_TESTQ("expression_to_json()", "call"){
	quark::ut_compare(
		expression_to_json_string(
			expression_t::make_function_call(
				type_identifier_t::make("my_func"),
//???				expression_t::make_constant(make_test_func()),
				{
					expression_t::make_constant("xyz"),
					expression_t::make_constant(123)
				},
				type_def_t::make_string_typedef()
			)
		),
		R"(["call", "my_func", [["k", "xyz", "<string>"], ["k", 123, "<int>"]], "<string>"])"
	);
}

QUARK_UNIT_TESTQ("expression_to_json()", "lookup"){
	quark::ut_compare(
		expression_to_json_string(
			expression_t::make_lookup(
				expression_t::make_resolve_variable("hello", type_def_t::make_string_typedef()),
				expression_t::make_constant("xyz"),
				type_def_t::make_string_typedef()
			)
		),
		R"(["[-]", ["@", "hello", "<string>"], ["k", "xyz", "<string>"], "<string>"])"
	);
}


expression_t::math2_operation string_to_math2_op(const string& op){
	if(op == "+"){
		return expression_t::math2_operation::k_math2_add;
	}
	else if(op == "-"){
		return expression_t::math2_operation::k_math2_subtract;
	}
	else if(op == "*"){
		return expression_t::math2_operation::k_math2_multiply;
	}
	else if(op == "/"){
		return expression_t::math2_operation::k_math2_divide;
	}
	else if(op == "%"){
		return expression_t::math2_operation::k_math2_remainder;
	}

	else if(op == "<="){
		return expression_t::math2_operation::k_math2_smaller_or_equal;
	}
	else if(op == "<"){
		return expression_t::math2_operation::k_math2_smaller;
	}
	else if(op == ">="){
		return expression_t::math2_operation::k_math2_larger_or_equal;
	}
	else if(op == ">"){
		return expression_t::math2_operation::k_math2_larger;
	}

	else if(op == "=="){
		return expression_t::math2_operation::k_logical_equal;
	}
	else if(op == "!="){
		return expression_t::math2_operation::k_logical_nonequal;
	}
	else if(op == "&&"){
		return expression_t::math2_operation::k_logical_and;
	}
	else if(op == "||"){
		return expression_t::math2_operation::k_logical_or;
	}
	else if(op == "negate"){
		return expression_t::math2_operation::k_logical_negate;
	}

	else if(op == "k"){
		return expression_t::math2_operation::k_constant;
	}
	else if(op == "?:"){
		return expression_t::math2_operation::k_conditional_operator3;
	}
	else if(op == "call"){
		return expression_t::math2_operation::k_call;
	}
	else if(op == "@"){
		return expression_t::math2_operation::k_resolve_variable;
	}
	else if(op == "->"){
		return expression_t::math2_operation::k_resolve_member;
	}

	else{
		QUARK_ASSERT(false);
	}
}


bool is_math2_op(const string& op){
	//???	HOw to handle lookup?
	QUARK_ASSERT(op != "[-]");

	return
		op == "+" || op == "-" || op == "*" || op == "/" || op == "%"
		|| op == "<=" || op == "<" || op == ">=" || op == ">"
		|| op == "==" || op == "!=" || op == "&&" || op == "||"
		|| op == "neg";
}


}	//	floyd_parser
