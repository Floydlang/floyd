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


namespace floyd_parser {

using std::pair;
using std::string;
using std::vector;
using std::shared_ptr;
using std::make_shared;


//??? Cleanup "neg" and "negate". Use "!" maybe?
//??? Use "f()" for functions.
//??? Use "[n]" for lookups.



static std::map<expression_t::math2_operation, string> operation_to_string_lookup = {
	{ expression_t::math2_operation::k_math2_add, "+" },
	{ expression_t::math2_operation::k_math2_subtract, "-" },
	{ expression_t::math2_operation::k_math2_multiply, "*" },
	{ expression_t::math2_operation::k_math2_divide, "/" },
	{ expression_t::math2_operation::k_math2_remainder, "%" },

	{ expression_t::math2_operation::k_math2_smaller_or_equal, "<=" },
	{ expression_t::math2_operation::k_math2_smaller, "<" },
	{ expression_t::math2_operation::k_math2_larger_or_equal, ">=" },
	{ expression_t::math2_operation::k_math2_larger, ">" },

	{ expression_t::math2_operation::k_logical_equal, "==" },
	{ expression_t::math2_operation::k_logical_nonequal, "!=" },
	{ expression_t::math2_operation::k_logical_and, "&&" },
	{ expression_t::math2_operation::k_logical_or, "||" },
	{ expression_t::math2_operation::k_logical_negate, "negate" },

	{ expression_t::math2_operation::k_constant, "k" },

	{ expression_t::math2_operation::k_conditional_operator3, "?:" },
	{ expression_t::math2_operation::k_call, "call" },

	{ expression_t::math2_operation::k_resolve_variable, "@" },
	{ expression_t::math2_operation::k_resolve_member, "->" },

	{ expression_t::math2_operation::k_lookup_element, "[-]" }
};

std::map<string, expression_t::math2_operation> make_reverse(const std::map<expression_t::math2_operation, string>& m){
	std::map<string, expression_t::math2_operation> temp;
	for(const auto e: m){
		temp[e.second] = e.first;
	}
	return temp;
}

static std::map<string, expression_t::math2_operation> string_to_operation_lookip = make_reverse(operation_to_string_lookup);





string expression_to_json_string(const expression_t& e);


QUARK_UNIT_TEST("", "math_operation2_expr_t==()", "", ""){
	const auto a = expression_t::make_math2_operation(
		expression_t::math2_operation::k_math2_add,
		expression_t::make_constant(3),
		expression_t::make_constant(4));
	const auto b = expression_t::make_math2_operation(
		expression_t::math2_operation::k_math2_add,
		expression_t::make_constant(3),
		expression_t::make_constant(4));
	QUARK_TEST_VERIFY(a == b);
}






//////////////////////////////////////////////////		expression_t



bool expression_t::check_invariant() const{
//	QUARK_ASSERT(_debug.size() > 0);

	QUARK_ASSERT(_resolved_expression_type && _resolved_expression_type->check_invariant());
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


expression_t::math2_operation expression_t::get_operation() const{
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

	
std::shared_ptr<const type_def_t> expression_t::get_resolved_expression_type() const{
	QUARK_ASSERT(check_invariant());

	return _resolved_expression_type;
}

expression_t expression_t::make_constant(const value_t& value){
	QUARK_ASSERT(value.check_invariant());

	auto result = expression_t();
	result._operation = math2_operation::k_constant;
	result._constant = make_shared<value_t>(value);
	result._resolved_expression_type = value.get_type();
	result._debug = expression_to_json_string(result);
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
	return _operation == math2_operation::k_constant;
}

const value_t& expression_t::get_constant() const{
	QUARK_ASSERT(is_constant())

	return *_constant;
}

expression_t expression_t::make_math2_operation(
	math2_operation op,
	const std::vector<expression_t>& expressions,
	const std::shared_ptr<value_t>& constant,
	const std::string& symbol
)
{
	QUARK_ASSERT(!constant || constant->check_invariant());

	auto result = expression_t();
	result._operation = op;
	result._expressions = expressions;
	result._constant = constant;
	result._symbol = symbol;
	result._resolved_expression_type = expressions[0].get_expression_type();
	result._debug = expression_to_json_string(result);
	QUARK_ASSERT(result.check_invariant());
	return result;
}


expression_t expression_t::make_conditional_operator(const expression_t& condition, const expression_t& a, const expression_t& b){
	QUARK_ASSERT(condition.check_invariant());
	QUARK_ASSERT(a.check_invariant());
	QUARK_ASSERT(b.check_invariant());

	return make_math2_operation(math2_operation::k_conditional_operator3, { condition, a, b }, {}, {});
}

expression_t expression_t::make_function_call(const type_identifier_t& function_name, const std::vector<expression_t>& inputs, const shared_ptr<const type_def_t>& resolved_expression_type){
	QUARK_ASSERT(function_name.check_invariant());
	for(const auto arg: inputs){
		QUARK_ASSERT(arg.check_invariant());
	}
	QUARK_ASSERT(resolved_expression_type && resolved_expression_type->check_invariant());

	auto result = expression_t();
	result._operation = math2_operation::k_call;
	result._expressions = inputs;
	result._symbol = function_name.to_string();
	result._resolved_expression_type = resolved_expression_type;
	result._debug = expression_to_json_string(result);
	QUARK_ASSERT(result.check_invariant());
	return result;
}


expression_t expression_t::make_resolve_variable(const std::string& variable, const shared_ptr<const type_def_t>& resolved_expression_type){
	QUARK_ASSERT(variable.size() > 0);
	QUARK_ASSERT(resolved_expression_type && resolved_expression_type->check_invariant());

	auto result = expression_t();
	result._operation = math2_operation::k_resolve_variable;
	result._symbol = variable;
	result._resolved_expression_type = resolved_expression_type;
	result._debug = expression_to_json_string(result);
	QUARK_ASSERT(result.check_invariant());
	return result;
}

expression_t expression_t::make_resolve_member(const expression_t& parent_address, const std::string& member_name, const shared_ptr<const type_def_t>& resolved_expression_type){
	QUARK_ASSERT(parent_address.check_invariant());
	QUARK_ASSERT(member_name.size() > 0);
	QUARK_ASSERT(resolved_expression_type && resolved_expression_type->check_invariant());

	auto result = expression_t();
	result._operation = math2_operation::k_resolve_member;
	result._expressions = { parent_address };
	result._symbol = member_name;
	result._resolved_expression_type = resolved_expression_type;
	result._debug = expression_to_json_string(result);
	QUARK_ASSERT(result.check_invariant());
	return result;
}

expression_t expression_t::make_lookup(const expression_t& parent_address, const expression_t& lookup_key, const shared_ptr<const type_def_t>& resolved_expression_type){
	QUARK_ASSERT(parent_address.check_invariant());
	QUARK_ASSERT(lookup_key.check_invariant());
	QUARK_ASSERT(resolved_expression_type && resolved_expression_type->check_invariant());

	auto result = expression_t();
	result._operation = math2_operation::k_lookup_element;
	result._expressions = { parent_address, lookup_key };
	result._resolved_expression_type = resolved_expression_type;
	result._debug = expression_to_json_string(result);
	QUARK_ASSERT(result.check_invariant());
	return result;
}

string operation_to_string(const expression_t::math2_operation& op){
	const auto r = operation_to_string_lookup.find(op);
	QUARK_ASSERT(r != operation_to_string_lookup.end());
	return r->second;
}

expression_t::math2_operation string_to_math2_op(const string& op){
	const auto r = string_to_operation_lookip.find(op);
	QUARK_ASSERT(r != string_to_operation_lookip.end());
	return r->second;
}

void trace(const expression_t& e){
	QUARK_ASSERT(e.check_invariant());
	const auto json = expression_to_json(e);
	const auto s = json_to_compact_string(json);
	QUARK_TRACE(s);
}




////////////////////////////////////////////		JSON SUPPORT



json_t expression_to_json(const expression_t& e){
	const auto expression_base_type = e.get_resolved_expression_type()->get_base_type();
	json_t type;
	if(expression_base_type == base_type::k_null){
		type = json_t();
	}
	else{
		const auto type_string = e.get_resolved_expression_type()->to_string();
		type = std::string("<") + type_string + ">";
	}

	const auto e2 = e;

	vector<json_t>  expressions;
	for(const auto& i: e.get_expressions()){
		const auto a = expression_to_json(i);
		expressions.push_back(a);
	}

	const auto constant = e2.is_constant() ? value_to_json(e2.get_constant()) : json_t();
	const auto function_name = e2.get_symbol().empty() == false ? e2.get_symbol() : json_t();

	auto result = json_t::make_array();
	result = push_back(result, operation_to_string(e2.get_operation()));
	if(function_name.is_null() == false){
		result = push_back(result, function_name);
	}
	if(e2.get_operation() == expression_t::math2_operation::k_call){
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
			expression_t::make_math2_operation(expression_t::math2_operation::k_math2_add, expression_t::make_constant(2), expression_t::make_constant(3))),
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


}	//	floyd_parser
