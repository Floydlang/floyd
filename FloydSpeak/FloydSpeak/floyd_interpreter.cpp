//
//  parser_evaluator.cpp
//  FloydSpeak
//
//  Created by Marcus Zetterquist on 26/07/16.
//  Copyright © 2016 Marcus Zetterquist. All rights reserved.
//

#include "floyd_interpreter.h"

#include "parser_primitives.h"
#include "parse_expression.h"
#include "parse_statement.h"
#include "statement.h"
#include "floyd_parser.h"
#include "ast_value.h"
#include "pass2.h"
#include "pass3.h"
#include "json_support.h"
#include "json_parser.h"

#include <cmath>
#include <sys/time.h>

#include <thread>
#include <chrono>
#include <algorithm>
#include <iostream>
#include <fstream>
#include "text_parser.h"
#include "host_functions.hpp"

namespace floyd {

using std::vector;
using std::string;
using std::pair;
using std::shared_ptr;
using std::unique_ptr;
using std::make_shared;


std::pair<interpreter_t, value_t> evaluate_call_expression(const interpreter_t& vm, const expression_t& e);


std::pair<interpreter_t, value_t> construct_value_from_typeid(const interpreter_t& vm, const typeid_t& type, const vector<value_t>& arg_values){
	QUARK_ASSERT(vm.check_invariant());

	if(type.is_json_value()){
		QUARK_ASSERT(arg_values.size() == 1);

		const auto arg = arg_values[0];
		const auto value = value_to_ast_json(arg);
		return {vm, value_t::make_json_value(value._value)};
	}
	else if(type.is_bool() || type.is_int() || type.is_float() || type.is_string() || type.is_typeid()){
		QUARK_ASSERT(arg_values.size() == 1);

		const auto arg = arg_values[0];
		if(type.is_string()){
			if(arg.is_json_value() && arg.get_json_value().is_string()){
				return {vm, value_t::make_string(arg.get_json_value().get_string())};
			}
			else if(arg.is_string()){
			}
		}
		else{
			if(arg.get_type() != type){
			}
		}
		return {vm, arg };
	}
	else if(type.is_struct()){
//		QUARK_SCOPED_TRACE("construct_struct()");
//		QUARK_TRACE("type: " + typeid_to_compact_string(struct_type));

	#if DEBUG
		const auto def = type.get_struct_ref();
		QUARK_ASSERT(arg_values.size() == def->_members.size());

		for(int i = 0 ; i < def->_members.size() ; i++){
			const auto v = arg_values[i];
			const auto a = def->_members[i];
			QUARK_ASSERT(v.check_invariant());
			QUARK_ASSERT(v.get_type().get_base_type() != base_type::k_unresolved_type_identifier);
			QUARK_ASSERT(v.get_type() == a._type);
		}
	#endif
		const auto instance = value_t::make_struct_value(type, arg_values);
		QUARK_TRACE(to_compact_string2(instance));

		return { vm, instance };
	}

	else if(type.is_vector()){
		const auto element_type = type.get_vector_element_type();
		QUARK_ASSERT(element_type.is_null() == false);

		return { vm, value_t::make_vector_value(element_type, arg_values) };
	}
	else if(type.is_dict()){
		const auto element_type = type.get_dict_value_type();
		QUARK_ASSERT(element_type.is_null() == false);

		std::map<string, value_t> m;
		for(auto i = 0 ; i < arg_values.size() / 2 ; i++){
			const auto key = arg_values[i * 2 + 0].get_string_value();
			const auto value = arg_values[i * 2 + 1];
			m.insert({ key, value });
		}
		return { vm, value_t::make_dict_value(element_type, m) };
	}
	else if(type.is_function()){
	}
	else if(type.is_unresolved_type_identifier()){
	}
	else{
	}

	QUARK_ASSERT(false);
}

std::pair<interpreter_t, statement_result_t> execute_statements(const interpreter_t& vm, const std::vector<std::shared_ptr<statement_t>>& statements){
	QUARK_ASSERT(vm.check_invariant());
	for(const auto i: statements){ QUARK_ASSERT(i->check_invariant()); };

	auto vm_acc = vm;

	int statement_index = 0;
	while(statement_index < statements.size()){
		const auto statement = statements[statement_index];
		const auto& r = execute_statement(vm_acc, *statement);
		vm_acc = r.first;
		if(r.second._type == statement_result_t::k_return_unwind){
			return { vm_acc, r.second };
		}
		else{

			//	Last statement outputs its value, if any. This is passive output, not a return-unwind.
			if(statement_index == (statements.size() - 1)){
				if(r.second._type == statement_result_t::k_passive_expression_output){
					return { vm_acc, r.second };
				}
			}
			else{
			}

			statement_index++;
		}
	}
	return { vm_acc, statement_result_t::make_no_output() };
}

std::pair<interpreter_t, statement_result_t> execute_body(
	const interpreter_t& vm,
	const body_t& body,
	const std::map<std::string, std::pair<value_t, bool>>& init_values
)
{
	QUARK_ASSERT(vm.check_invariant());

	auto vm_acc = vm;

	auto new_environment = environment_t::make_environment(&body, init_values);
	vm_acc._call_stack.push_back(new_environment);

	const auto r = execute_statements(vm_acc, body._statements);
	vm_acc = r.first;
	vm_acc._call_stack.pop_back();

	return { vm_acc, r.second };
}

/*
bool does_symbol_exist_shallow(const interpreter_t& vm, const std::string& symbol){
    const auto it = std::find_if(
    	vm._call_stack.back()->_values.begin(),
    	vm._call_stack.back()->_values.end(),
    	[&symbol](const std::pair<std::string, std::pair<value_t, bool>>& e) { return e.first == symbol; }
	);
	return it != vm._call_stack.back()->_values.end();
}
*/

std::pair<interpreter_t, statement_result_t> execute_store2_statement(const interpreter_t& vm, const statement_t::store2_t& statement){
	QUARK_ASSERT(vm.check_invariant());

	auto vm_acc = vm;
	const auto address = statement._dest_variable;

	const auto rhs_expr_pair = evaluate_expression(vm_acc, statement._expression);
	vm_acc = rhs_expr_pair.first;
	const auto rhs_value = rhs_expr_pair.second;

	const auto env_index = address._parent_steps == -1 ? 0: vm._call_stack.size() - address._parent_steps - 1;
	auto& env = vm._call_stack[env_index];
	const auto lhs_value_deep_ptr = &env->_values[address._index].second;
	QUARK_ASSERT(lhs_value_deep_ptr != nullptr);

	const auto lhs_value = lhs_value_deep_ptr->first;

	*lhs_value_deep_ptr = std::pair<value_t, bool>(rhs_value, true);
	return { vm_acc, statement_result_t::make_no_output() };
}

std::pair<interpreter_t, statement_result_t> execute_return_statement(const interpreter_t& vm, const statement_t::return_statement_t& statement){
	QUARK_ASSERT(vm.check_invariant());

	auto vm_acc = vm;
	const auto expr = statement._expression;
	const auto result = evaluate_expression(vm_acc, expr);
	vm_acc = result.first;

	const auto rhs_value = result.second;
	return {
		vm_acc,
		statement_result_t::make_return_unwind(rhs_value)
	};
}

typeid_t find_type_by_name(const interpreter_t& vm, const typeid_t& type){
	if(type.get_base_type() == base_type::k_unresolved_type_identifier){
		const auto v = find_symbol_by_name(vm, type.get_unresolved_type_identifier());
		if(v){
			if(v->first.is_typeid()){
				return v->first.get_typeid_value();
			}
			else{
				return typeid_t::make_null();
			}
		}
		else{
			return typeid_t::make_null();
		}
	}
	else{
		return type;
	}
}

std::pair<interpreter_t, statement_result_t> execute_ifelse_statement(const interpreter_t& vm, const statement_t::ifelse_statement_t& statement){
	QUARK_ASSERT(vm.check_invariant());

	auto vm_acc = vm;

	const auto condition_result = evaluate_expression(vm_acc, statement._condition);
	vm_acc = condition_result.first;
	QUARK_ASSERT(condition_result.second.is_bool());

	const auto condition_result_value = condition_result.second;
	bool r = condition_result_value.get_bool_value();
	if(r){
		return execute_body(vm_acc, statement._then_body, {});
	}
	else{
		return execute_body(vm_acc, statement._else_body, {});
	}
}

std::pair<interpreter_t, statement_result_t> execute_for_statement(const interpreter_t& vm, const statement_t::for_statement_t& statement){
	QUARK_ASSERT(vm.check_invariant());

	auto vm_acc = vm;

	const auto start_value0 = evaluate_expression(vm_acc, statement._start_expression);
	vm_acc = start_value0.first;
	const auto start_value_int = start_value0.second.get_int_value();

	const auto end_value0 = evaluate_expression(vm_acc, statement._end_expression);
	vm_acc = end_value0.first;
	const auto end_value_int = end_value0.second.get_int_value();

	for(int x = start_value_int ; x <= end_value_int ; x++){
		const std::map<std::string, std::pair<value_t, bool>> values = { { statement._iterator_name, std::pair<value_t, bool>(value_t::make_int(x), false) } };
		const auto result = execute_body(vm_acc, statement._body, values);
		vm_acc = result.first;

		const auto return_value = result.second;
		if(return_value._type == statement_result_t::k_return_unwind){
			return { vm_acc, return_value };
		}
	}
	return { vm_acc, statement_result_t::make_no_output() };
}

std::pair<interpreter_t, statement_result_t> execute_while_statement(const interpreter_t& vm, const statement_t::while_statement_t& statement){
	QUARK_ASSERT(vm.check_invariant());

	auto vm_acc = vm;

	bool again = true;
	while(again){
		const auto condition_value_expr = evaluate_expression(vm_acc, statement._condition);
		vm_acc = condition_value_expr.first;
		const auto condition_value = condition_value_expr.second.get_bool_value();

		if(condition_value){
			const auto result = execute_body(vm_acc, statement._body, {});
			vm_acc = result.first;
			const auto return_value = result.second;
			if(return_value._type == statement_result_t::k_return_unwind){
				return { vm_acc, return_value };
			}
		}
		else{
			again = false;
		}
	}
	return { vm_acc, statement_result_t::make_no_output() };
}

std::pair<interpreter_t, statement_result_t> execute_expression_statement(const interpreter_t& vm, const statement_t::expression_statement_t& statement){
	QUARK_ASSERT(vm.check_invariant());

	auto vm_acc = vm;
	const auto result = evaluate_expression(vm_acc, statement._expression);
	vm_acc = result.first;
	const auto rhs_value = result.second;
	return {
		vm_acc,
		statement_result_t::make_passive_expression_output(rhs_value)
	};
}

std::pair<interpreter_t, statement_result_t> execute_statement(const interpreter_t& vm, const statement_t& statement){
	QUARK_ASSERT(vm.check_invariant());
	QUARK_ASSERT(statement.check_invariant());

	if(statement._bind_local){
		//	Bind is converted to symbol table + store-local in pass3.
		QUARK_ASSERT(false);
	}
	else if(statement._store){
		QUARK_ASSERT(false);
	}
	else if(statement._store2){
		return execute_store2_statement(vm, *statement._store2);
	}
	else if(statement._block){
		return execute_body(vm, statement._block->_body, {});
	}
	else if(statement._return){
		return execute_return_statement(vm, *statement._return);
	}
	else if(statement._def_struct){
		QUARK_ASSERT(false);
//		return execute_def_struct_statement(vm, *statement._def_struct);
//		return { vm, statement_result_t::make_no_output() };
	}
	else if(statement._if){
		return execute_ifelse_statement(vm, *statement._if);
	}
	else if(statement._for){
		return execute_for_statement(vm, *statement._for);
	}
	else if(statement._while){
		return execute_while_statement(vm, *statement._while);
	}
	else if(statement._expression){
		return execute_expression_statement(vm, *statement._expression);
	}
	else{
		QUARK_ASSERT(false);
	}
}

//	Warning: returns reference to the found value-entry -- this could be in any environment in the call stack.
std::pair<floyd::value_t, bool>* resolve_env_variable_deep(const interpreter_t& vm, int depth, const std::string& s){
	QUARK_ASSERT(vm.check_invariant());
	QUARK_ASSERT(depth >= 0 && depth < vm._call_stack.size());
	QUARK_ASSERT(s.size() > 0);

    const auto it = std::find_if(
    	vm._call_stack[depth]->_values.begin(),
    	vm._call_stack[depth]->_values.end(),
    	[&s](const std::pair<std::string, std::pair<value_t, bool>>& e) { return e.first == s; }
	);
	if(it != vm._call_stack[depth]->_values.end()){
		return &it->second;
	}
	else if(depth > 0){
		return resolve_env_variable_deep(vm, depth - 1, s);
	}
	else{
		return nullptr;
	}
}

//	Warning: returns reference to the found value-entry -- this could be in any environment in the call stack.
std::pair<floyd::value_t, bool>* find_symbol_by_name(const interpreter_t& vm, const std::string& s){
	QUARK_ASSERT(vm.check_invariant());
	QUARK_ASSERT(s.size() > 0);

	return resolve_env_variable_deep(vm, static_cast<int>(vm._call_stack.size() - 1), s);
}

floyd::value_t find_global_symbol(const interpreter_t& vm, const string& s){
	const auto t = find_symbol_by_name(vm, s);
	if(t == nullptr){
		QUARK_ASSERT(false);
	}
	return t->first;
}

std::pair<interpreter_t, value_t> evaluate_resolve_member_expression(const interpreter_t& vm, const expression_t::resolve_member_expr_t& expr){
	QUARK_ASSERT(vm.check_invariant());

	auto vm_acc = vm;
	const auto parent_expr = evaluate_expression(vm_acc, *expr._parent_address);
	vm_acc = parent_expr.first;
	QUARK_ASSERT(parent_expr.second.is_struct());

	const auto struct_instance = parent_expr.second.get_struct_value();

	int index = find_struct_member_index(*struct_instance->_def, expr._member_name);
	QUARK_ASSERT(index != -1);

	const value_t value = struct_instance->_member_values[index];
	return { vm_acc, value};
}

std::pair<interpreter_t, value_t> evaluate_lookup_element_expression(const interpreter_t& vm, const expression_t::lookup_expr_t& expr){
	QUARK_ASSERT(vm.check_invariant());

	auto vm_acc = vm;
	const auto parent_expr = evaluate_expression(vm_acc, *expr._parent_address);
	vm_acc = parent_expr.first;

	const auto key_expr = evaluate_expression(vm_acc, *expr._lookup_key);
	vm_acc = key_expr.first;

	const auto parent_value = parent_expr.second;
	const auto key_value = key_expr.second;

	if(parent_value.is_string()){
		QUARK_ASSERT(key_value.is_int());

		const auto instance = parent_value.get_string_value();
		int lookup_index = key_value.get_int_value();
		if(lookup_index < 0 || lookup_index >= instance.size()){
			throw std::runtime_error("Lookup in string: out of bounds.");
		}
		else{
			const char ch = instance[lookup_index];
			const auto value2 = value_t::make_string(string(1, ch));
			return { vm_acc, value2};
		}
	}
	else if(parent_value.is_json_value()){
		//	Notice: the exact type of value in the json_value is only known at runtime = must be checked in interpreter.
		const auto parent_json_value = parent_value.get_json_value();
		if(parent_json_value.is_object()){
			QUARK_ASSERT(key_value.is_string());
			const auto lookup_key = key_value.get_string_value();

			//	get_object_element() throws if key can't be found.
			const auto value = parent_json_value.get_object_element(lookup_key);
			const auto value2 = value_t::make_json_value(value);
			return { vm_acc, value2};
		}
		else if(parent_json_value.is_array()){
			QUARK_ASSERT(key_value.is_int());
			const auto lookup_index = key_value.get_int_value();

			if(lookup_index < 0 || lookup_index >= parent_json_value.get_array_size()){
				throw std::runtime_error("Lookup in json_value array: out of bounds.");
			}
			else{
				const auto value = parent_json_value.get_array_n(lookup_index);
				const auto value2 = value_t::make_json_value(value);
				return { vm_acc, value2};
			}
		}
		else{
			throw std::runtime_error("Lookup using [] on json_value only works on objects and arrays.");
		}
	}
	else if(parent_value.is_vector()){
		QUARK_ASSERT(key_value.is_int());

		const auto instance = parent_value.get_vector_value();

		int lookup_index = key_value.get_int_value();
		if(lookup_index < 0 || lookup_index >= instance->_elements.size()){
			throw std::runtime_error("Lookup in vector: out of bounds.");
		}
		else{
			const value_t value = instance->_elements[lookup_index];
			return { vm_acc, value};
		}
	}
	else if(parent_value.is_dict()){
		QUARK_ASSERT(key_value.is_string());

		const auto instance = parent_value.get_dict_value();
		const string key = key_value.get_string_value();

		const auto found_it = instance->_elements.find(key);
		if(found_it == instance->_elements.end()){
			throw std::runtime_error("Lookup in dict: key not found.");
		}
		else{
			const value_t value = found_it->second;
			return { vm_acc, value};
		}
	}
	else {
		QUARK_ASSERT(false);
	}
}

std::pair<interpreter_t, value_t> evaluate_load2_expression(const interpreter_t& vm, const expression_t& e){
	QUARK_ASSERT(vm.check_invariant());

	const auto expr = *e.get_load2();

	auto vm_acc = vm;

	const auto env_index = expr._address._parent_steps == -1 ? 0 : vm._call_stack.size() - expr._address._parent_steps - 1;
	QUARK_ASSERT(env_index >= 0 && env_index < vm._call_stack.size());

	const auto env = vm._call_stack[env_index];

	QUARK_ASSERT(expr._address._index >= 0 && expr._address._index < env->_values.size());
	const auto value = env->_values[expr._address._index].second.first;

	QUARK_ASSERT(value.get_type() == e.get_annotated_type() /*|| e.get_annotated_type().is_null()*/);

	return {vm_acc, value};
}

//??? Should support any type including structs and int.
std::pair<interpreter_t, value_t> evaluate_construct_value_expression(const interpreter_t& vm, const expression_t& e){
	QUARK_ASSERT(vm.check_invariant());

	const auto expr = *e.get_construct_value();
	auto vm_acc = vm;

	if(expr._value_type2.is_vector()){
		const std::vector<expression_t>& elements = expr._args;
		const auto root_value_type = expr._value_type2;
		const auto element_type = root_value_type.get_vector_element_type();
		QUARK_ASSERT(element_type.is_null() == false);

		//	An empty vector is encoded as a constant value by pass3, not a vector-definition-expression.
		QUARK_ASSERT(elements.empty() == false);

		QUARK_ASSERT(root_value_type.is_null() == false);
		QUARK_ASSERT(element_type.is_null() == false);

		std::vector<value_t> elements2;
		for(const auto m: elements){
			const auto element_expr = evaluate_expression(vm_acc, m);
			vm_acc = element_expr.first;

			const auto element = element_expr.second;
			elements2.push_back(element);
		}

	#if DEBUG
		for(const auto m: elements2){
			QUARK_ASSERT(m.get_type() == element_type);
		}
	#endif
		return construct_value_from_typeid(vm, typeid_t::make_vector(element_type), elements2);
	}
	else if(expr._value_type2.is_dict()){
		const auto& elements = expr._args;
		const auto root_value_type = expr._value_type2;
		const auto element_type = root_value_type.get_dict_value_type();

		//	An empty dict is encoded as a constant value pass3, not a dict-definition-expression.
		QUARK_ASSERT(elements.empty() == false);

		QUARK_ASSERT(root_value_type.is_null() == false);
		QUARK_ASSERT(element_type.is_null() == false);

		std::vector<value_t> elements2;
		for(auto i = 0 ; i < elements.size() / 2 ; i++){
			const auto key_expr = elements[i * 2 + 0];
			const auto value_expr = elements[i * 2 + 1];

			const auto element_expr = evaluate_expression(vm_acc, value_expr);
			vm_acc = element_expr.first;

			const auto value = element_expr.second;
			const string key = key_expr.get_literal().get_string_value();
			elements2.push_back(value_t::make_string(key));
			elements2.push_back(value);
		}
		return construct_value_from_typeid(vm, typeid_t::make_dict(element_type), elements2);
	}
	else if(expr._value_type2.is_struct()){
		std::vector<value_t> elements2;
		for(const auto m: expr._args){
			const auto element_expr = evaluate_expression(vm_acc, m);
			vm_acc = element_expr.first;

			const auto element = element_expr.second;
			elements2.push_back(element);
		}
		return construct_value_from_typeid(vm, expr._value_type2, elements2);
	}
	else{
		QUARK_ASSERT(false);
	}
}

std::pair<interpreter_t, value_t> evaluate_arithmetic_unary_minus_expression(const interpreter_t& vm, const expression_t::unary_minus_expr_t& expr){
	QUARK_ASSERT(vm.check_invariant());

	auto vm_acc = vm;
	const auto& expr2 = evaluate_expression(vm_acc, *expr._expr);
	vm_acc = expr2.first;

	const auto& c = expr2.second;
	vm_acc = expr2.first;

	if(c.is_int()){
		return evaluate_expression(
			vm_acc,
			expression_t::make_simple_expression__2(
				expression_type::k_arithmetic_subtract__2,
				expression_t::make_literal_int(0),
				expression_t::make_literal(expr2.second),
				make_shared<typeid_t>(c.get_type())
			)
		);
	}
	else if(c.is_float()){
		return evaluate_expression(
			vm_acc,
			expression_t::make_simple_expression__2(
				expression_type::k_arithmetic_subtract__2,
				expression_t::make_literal_float(0.0f),
				expression_t::make_literal(expr2.second),
				make_shared<typeid_t>(c.get_type())
			)
		);
	}
	else{
		QUARK_ASSERT(false);
	}
}

std::pair<interpreter_t, value_t> evaluate_conditional_operator_expression(const interpreter_t& vm, const expression_t::conditional_expr_t& expr){
	QUARK_ASSERT(vm.check_invariant());

	auto vm_acc = vm;

	//	Special-case since it uses 3 expressions & uses shortcut evaluation.
	const auto cond_result = evaluate_expression(vm_acc, *expr._condition);
	vm_acc = cond_result.first;

	QUARK_ASSERT(cond_result.second.is_bool());

	const bool cond_flag = cond_result.second.get_bool_value();

	//	!!! Only evaluate the CHOSEN expression. Not that important since functions are pure.
	if(cond_flag){
		return evaluate_expression(vm_acc, *expr._a);
	}
	else{
		return evaluate_expression(vm_acc, *expr._b);
	}
}

std::pair<interpreter_t, value_t> evaluate_comparison_expression(const interpreter_t& vm, expression_type op, const expression_t::simple_expr__2_t& simple2_expr){
	QUARK_ASSERT(vm.check_invariant());

	auto vm_acc = vm;

	//	First evaluate all inputs to our operation.
	const auto left_expr = evaluate_expression(vm_acc, *simple2_expr._left);
	vm_acc = left_expr.first;

	const auto right_expr = evaluate_expression(vm_acc, *simple2_expr._right);
	vm_acc = right_expr.first;

	//	Perform math operation on the two constants => new constant.
	const auto left_constant = left_expr.second;
	const auto right_constant = right_expr.second;
	QUARK_ASSERT(left_constant.get_type() == right_constant.get_type());

	//	Do generic functionallity, independant on type.
	if(op == expression_type::k_comparison_smaller_or_equal__2){
		long diff = value_t::compare_value_true_deep(left_constant, right_constant);
		return {vm_acc, value_t::make_bool(diff <= 0)};
	}
	else if(op == expression_type::k_comparison_smaller__2){
		long diff = value_t::compare_value_true_deep(left_constant, right_constant);
		return {vm_acc, value_t::make_bool(diff < 0)};
	}
	else if(op == expression_type::k_comparison_larger_or_equal__2){
		long diff = value_t::compare_value_true_deep(left_constant, right_constant);
		return {vm_acc, value_t::make_bool(diff >= 0)};
	}
	else if(op == expression_type::k_comparison_larger__2){
		long diff = value_t::compare_value_true_deep(left_constant, right_constant);
		return {vm_acc, value_t::make_bool(diff > 0)};
	}

	else if(op == expression_type::k_logical_equal__2){
		long diff = value_t::compare_value_true_deep(left_constant, right_constant);
		return {vm_acc, value_t::make_bool(diff == 0)};
	}
	else if(op == expression_type::k_logical_nonequal__2){
		long diff = value_t::compare_value_true_deep(left_constant, right_constant);
		return {vm_acc, value_t::make_bool(diff != 0)};
	}
	else{
		QUARK_ASSERT(false);
	}
}

std::pair<interpreter_t, value_t> evaluate_arithmetic_expression(const interpreter_t& vm, expression_type op, const expression_t::simple_expr__2_t& simple2_expr){
	QUARK_ASSERT(vm.check_invariant());

	auto vm_acc = vm;

	//	First evaluate all inputs to our operation.
	const auto left_expr = evaluate_expression(vm_acc, *simple2_expr._left);
	vm_acc = left_expr.first;

	const auto right_expr = evaluate_expression(vm_acc, *simple2_expr._right);
	vm_acc = right_expr.first;

	//	Perform math operation on the two constants => new constant.
	const auto left_constant = left_expr.second;
	const auto right_constant = right_expr.second;
	QUARK_ASSERT(left_constant.get_type() == right_constant.get_type());

	const auto type_mode = left_constant.get_type();

	//	bool
	if(type_mode.is_bool()){
		const bool left = left_constant.get_bool_value();
		const bool right = right_constant.get_bool_value();

		if(op == expression_type::k_arithmetic_add__2){
			return {vm_acc, value_t::make_bool(left + right)};
		}
		else if(op == expression_type::k_logical_and__2){
			return {vm_acc, value_t::make_bool(left && right)};
		}
		else if(op == expression_type::k_logical_or__2){
			return {vm_acc, value_t::make_bool(left || right)};
		}
		else{
			QUARK_ASSERT(false);
		}
	}

	//	int
	else if(type_mode.is_int()){
		const int left = left_constant.get_int_value();
		const int right = right_constant.get_int_value();

		if(op == expression_type::k_arithmetic_add__2){
			return {vm_acc, value_t::make_int(left + right)};
		}
		else if(op == expression_type::k_arithmetic_subtract__2){
			return {vm_acc, value_t::make_int(left - right)};
		}
		else if(op == expression_type::k_arithmetic_multiply__2){
			return {vm_acc, value_t::make_int(left * right)};
		}
		else if(op == expression_type::k_arithmetic_divide__2){
			if(right == 0){
				throw std::runtime_error("EEE_DIVIDE_BY_ZERO");
			}
			return {vm_acc, value_t::make_int(left / right)};
		}
		else if(op == expression_type::k_arithmetic_remainder__2){
			if(right == 0){
				throw std::runtime_error("EEE_DIVIDE_BY_ZERO");
			}
			return {vm_acc, value_t::make_int(left % right)};
		}

		//??? Could be replaced by feture to convert any value to bool -- they use a generic comparison for && and ||
		else if(op == expression_type::k_logical_and__2){
			return {vm_acc, value_t::make_bool((left != 0) && (right != 0))};
		}
		else if(op == expression_type::k_logical_or__2){
			return {vm_acc, value_t::make_bool((left != 0) || (right != 0))};
		}
		else{
			QUARK_ASSERT(false);
		}
	}

	//	float
	else if(type_mode.is_float()){
		const float left = left_constant.get_float_value();
		const float right = right_constant.get_float_value();

		if(op == expression_type::k_arithmetic_add__2){
			return {vm_acc, value_t::make_float(left + right)};
		}
		else if(op == expression_type::k_arithmetic_subtract__2){
			return {vm_acc, value_t::make_float(left - right)};
		}
		else if(op == expression_type::k_arithmetic_multiply__2){
			return {vm_acc, value_t::make_float(left * right)};
		}
		else if(op == expression_type::k_arithmetic_divide__2){
			if(right == 0.0f){
				throw std::runtime_error("EEE_DIVIDE_BY_ZERO");
			}
			return {vm_acc, value_t::make_float(left / right)};
		}

		else if(op == expression_type::k_logical_and__2){
			return {vm_acc, value_t::make_bool((left != 0.0f) && (right != 0.0f))};
		}
		else if(op == expression_type::k_logical_or__2){
			return {vm_acc, value_t::make_bool((left != 0.0f) || (right != 0.0f))};
		}
		else{
			QUARK_ASSERT(false);
		}
	}

	//	string
	else if(type_mode.is_string()){
		const auto left = left_constant.get_string_value();
		const auto right = right_constant.get_string_value();

		if(op == expression_type::k_arithmetic_add__2){
			return {vm_acc, value_t::make_string(left + right)};
		}
		else{
			QUARK_ASSERT(false);
		}
	}

	//	struct
	else if(type_mode.is_struct()){
		QUARK_ASSERT(false);
	}

	//	vector
	else if(type_mode.is_vector()){
		//	Improves vectors before using them.
		const auto element_type = left_constant.get_type().get_vector_element_type();

		if(op == expression_type::k_arithmetic_add__2){
			auto elements2 = left_constant.get_vector_value()->_elements;
			elements2.insert(elements2.end(), right_constant.get_vector_value()->_elements.begin(), right_constant.get_vector_value()->_elements.end());

			const auto value2 = value_t::make_vector_value(element_type, elements2);
			return {vm_acc, value2};
		}
		else{
			QUARK_ASSERT(false);
		}
	}
	else if(type_mode.is_function()){
		QUARK_ASSERT(false);
	}
	else{
		QUARK_ASSERT(false);
	}
}

std::pair<interpreter_t, value_t> evaluate_expression(const interpreter_t& vm, const expression_t& e){
	QUARK_ASSERT(vm.check_invariant());
	QUARK_ASSERT(e.check_invariant());

	const auto op = e.get_operation();

	if(op == expression_type::k_literal){
		return { vm, e.get_literal() };
	}
	else if(op == expression_type::k_resolve_member){
		return evaluate_resolve_member_expression(vm, *e.get_resolve_member());
	}
	else if(op == expression_type::k_lookup_element){
		return evaluate_lookup_element_expression(vm, *e.get_lookup());
	}
	else if(op == expression_type::k_load){
		QUARK_ASSERT(false);
	}
	else if(op == expression_type::k_load2){
		return evaluate_load2_expression(vm, e);
	}

	else if(op == expression_type::k_call){
		return evaluate_call_expression(vm, e);
	}

	//??? Move entire function to symbol table -- no need for k_define_function-expression in interpreter!
	else if(op == expression_type::k_define_struct){
		QUARK_ASSERT(false);
		return {vm, value_t::make_null()};
	}
	//??? Move entire function to symbol table -- no need for k_define_function-expression in interpreter!
	else if(op == expression_type::k_define_function){
		const auto expr = e.get_function_definition();
		return {vm, value_t::make_function_value(expr->_def)};
	}

	else if(op == expression_type::k_construct_value){
		return evaluate_construct_value_expression(vm, e);
	}

	//	This can be desugared at compile time.
	else if(op == expression_type::k_arithmetic_unary_minus__1){
		return evaluate_arithmetic_unary_minus_expression(vm, *e.get_unary_minus());
	}

	//	Special-case since it uses 3 expressions & uses shortcut evaluation.
	else if(op == expression_type::k_conditional_operator3){
		return evaluate_conditional_operator_expression(vm, *e.get_conditional());
	}
	else if (false
		|| op == expression_type::k_comparison_smaller_or_equal__2
		|| op == expression_type::k_comparison_smaller__2
		|| op == expression_type::k_comparison_larger_or_equal__2
		|| op == expression_type::k_comparison_larger__2

		|| op == expression_type::k_logical_equal__2
		|| op == expression_type::k_logical_nonequal__2
	){
		return evaluate_comparison_expression(vm, op, *e.get_simple__2());
	}
	else if (false
		|| op == expression_type::k_arithmetic_add__2
		|| op == expression_type::k_arithmetic_subtract__2
		|| op == expression_type::k_arithmetic_multiply__2
		|| op == expression_type::k_arithmetic_divide__2
		|| op == expression_type::k_arithmetic_remainder__2

		|| op == expression_type::k_logical_and__2
		|| op == expression_type::k_logical_or__2
	){
		return evaluate_arithmetic_expression(vm, op, *e.get_simple__2());
	}
	else{
		QUARK_ASSERT(false);
	}
}

std::pair<interpreter_t, statement_result_t> call_function(const interpreter_t& vm, const floyd::value_t& f, const vector<value_t>& args){
	QUARK_ASSERT(vm.check_invariant());
	QUARK_ASSERT(f.check_invariant());
	for(const auto i: args){ QUARK_ASSERT(i.check_invariant()); };

	auto vm_acc = vm;

	if(f.is_function() == false){
		QUARK_ASSERT(false);
	}

	const auto& function_def = f.get_function_value()->_def;
	if(function_def._host_function != 0){
		const auto r = call_host_function(vm_acc, function_def._host_function, args);
		return { r.first, r.second };
	}
	else{
		const auto return_type = f.get_type().get_function_return();
		const auto arg_types = f.get_type().get_function_args();

		//	arity
		QUARK_ASSERT(args.size() == arg_types.size());

#if DEBUG
		for(int i = 0 ; i < args.size() ; i++){
			if(args[i].get_type() != arg_types[i]){
				QUARK_ASSERT(false);
			}
		}
#endif

		//	Always use global scope.
		//	Future: support closures by linking to function env where function is defined.

		//	Copy input arguments to the function scope.
		std::map<string, std::pair<value_t, bool>> args2;
		for(int i = 0 ; i < function_def._args.size() ; i++){
			const auto& arg_name = function_def._args[i]._name;
			const auto& arg_value = args[i];

			//	Function arguments are immutable while executing the function body.
			args2[arg_name] = std::pair<value_t, bool>(arg_value, false);
		}
		const auto r = execute_body(vm_acc, *function_def._body, args2);
		vm_acc = r.first;

		// ??? move this check to pass3.
		if(r.second._type != statement_result_t::k_return_unwind){
			throw std::runtime_error("Function missing return statement");
		}

		// ??? move this check to pass3.
		else if(r.second._output.get_type().is_struct() == false && r.second._output.get_type() != return_type){
			throw std::runtime_error("Function return type wrong.");
		}
		else{
			return {vm_acc, r.second };
		}
	}
}

//	May return a simplified expression instead of a value literal..
std::pair<interpreter_t, value_t> evaluate_call_expression(const interpreter_t& vm, const expression_t& e){
	QUARK_ASSERT(vm.check_invariant());

	const auto expr = *e.get_call();

	auto vm_acc = vm;

	//	Simplify each input expression: expression[0]: which function to call, expression[1]: first argument if any.
	const auto function = evaluate_expression(vm_acc, *expr._callee);
	vm_acc = function.first;

	const auto& function_value = function.second;

	vector<value_t> arg_values;
	for(int i = 0 ; i < expr._args.size() ; i++){
		const auto t = evaluate_expression(vm_acc, expr._args[i]);
		vm_acc = t.first;
		arg_values.push_back(t.second);
	}

	//	Call function-value.
	if(function_value.is_function()){
		const auto& result = call_function(vm_acc, function_value, arg_values);
		QUARK_ASSERT(result.second._type == statement_result_t::k_return_unwind);
		vm_acc = result.first;
		return { vm_acc, result.second._output};
	}

	//	Attempting to call a TYPE? Then this may be a constructor call.
	else if(function_value.is_typeid()){
		const auto result = construct_value_from_typeid(vm_acc, function_value.get_typeid_value(), arg_values);
		vm_acc = result.first;
		return { vm_acc, result.second};
	}
	else{
		QUARK_ASSERT(false);
	}
}

json_t interpreter_to_json(const interpreter_t& vm){
	vector<json_t> callstack;
	for(const auto& e: vm._call_stack){
		std::map<string, json_t> values;
		for(const auto&v: e->_values){

			const auto a = value_and_type_to_ast_json(v.second.first);
			const auto b = make_array_skip_nulls({
				a._value.get_array_n(0),
				a._value.get_array_n(1),
				v.second.second ? json_t("mutable") : json_t()
			});
			values[v.first] = b;
		}

		const auto& env = json_t::make_object({
			{ "values", values }
		});
		callstack.push_back(env);
	}

	return json_t::make_object({
		{ "ast", ast_to_json(*vm._ast)._value },
		{ "callstack", json_t::make_array(callstack) }
	});
}

void test__evaluate_expression(const expression_t& e, const value_t& expected_value){
	const ast_t ast;
	const interpreter_t interpreter(ast);
	const auto e3 = evaluate_expression(interpreter, e);

	ut_compare_jsons(value_to_ast_json(e3.second)._value, value_to_ast_json(expected_value)._value);
}


QUARK_UNIT_TESTQ("evaluate_expression()", "literal 1234 == 1234") {
	test__evaluate_expression(
		expression_t::make_literal_int(1234),
		value_t::make_int(1234)
	);
}

QUARK_UNIT_TESTQ("evaluate_expression()", "1 + 2 == 3") {
	test__evaluate_expression(
		expression_t::make_simple_expression__2(
			expression_type::k_arithmetic_add__2,
			expression_t::make_literal_int(1),
			expression_t::make_literal_int(2),
			make_shared<typeid_t>(typeid_t::make_int())
		),
		value_t::make_int(3)
	);
}


QUARK_UNIT_TESTQ("evaluate_expression()", "3 * 4 == 12") {
	test__evaluate_expression(
		expression_t::make_simple_expression__2(
			expression_type::k_arithmetic_multiply__2,
			expression_t::make_literal_int(3),
			expression_t::make_literal_int(4),
			make_shared<typeid_t>(typeid_t::make_int())
		),
		value_t::make_int(12)
	);
}

QUARK_UNIT_TESTQ("evaluate_expression()", "(3 * 4) * 5 == 60") {
	test__evaluate_expression(
		expression_t::make_simple_expression__2(
			expression_type::k_arithmetic_multiply__2,
			expression_t::make_simple_expression__2(
				expression_type::k_arithmetic_multiply__2,
				expression_t::make_literal_int(3),
				expression_t::make_literal_int(4),
				make_shared<typeid_t>(typeid_t::make_int())
			),
			expression_t::make_literal_int(5),
			make_shared<typeid_t>(typeid_t::make_int())
		),
		value_t::make_int(60)
	);
}




//////////////////////////		environment_t



//??? For each init-value, keep the index where to store it in environment values vector.
std::shared_ptr<environment_t> environment_t::make_environment(const body_t* body_ptr, const std::map<std::string, std::pair<value_t, bool>>& init_values){
	QUARK_ASSERT(body_ptr != nullptr);

	vector<std::pair<std::string, std::pair<value_t, bool>>> values;
	for(const auto e: body_ptr->_symbols){
		if(e.second._symbol_type == symbol_t::immutable_local){
			const std::pair<std::string, std::pair<value_t, bool>> val = {e.first, { e.second._const_value, false } };
			values.push_back(val);
		}
		else if(e.second._symbol_type == symbol_t::mutable_local){
			const std::pair<std::string, std::pair<value_t, bool>> val = {e.first, { e.second._const_value, true } };
			values.push_back(val);
		}
		else{
			QUARK_ASSERT(false);
		}
	}

	for(const auto e: init_values){
		auto found_it = std::find_if(values.begin(), values.end(), [&](const std::pair<std::string, std::pair<value_t, bool>>& v){ return v.first == e.first; });
		QUARK_ASSERT(found_it != values.end());

		found_it->second.first = e.second.first;
	}

	const auto temp = environment_t{ body_ptr, values };
	return make_shared<environment_t>(temp);
}

bool environment_t::check_invariant() const {
	return true;
}

std::pair<interpreter_t, statement_result_t> call_host_function(const interpreter_t& vm, int function_id, const std::vector<floyd::value_t> args){
	QUARK_ASSERT(function_id >= 0);

	const auto& host_function = vm._host_functions.at(function_id);

	//	arity
//	QUARK_ASSERT(args.size() == host_function._function_type.get_function_args().size());

	const auto result = (host_function)(vm, args);
	return {
		result.first,
		statement_result_t::make_return_unwind(result.second)
	};
}

interpreter_t::interpreter_t(const ast_t& ast){
	QUARK_ASSERT(ast.check_invariant());

	_ast = make_shared<ast_t>(ast);

	//	Make lookup table from host-function ID to an implementation of that host function in the interpreter.
	const auto host_functions = get_host_functions();
	for(auto hf_kv: host_functions){
		const auto function_id = hf_kv.second._signature._function_id;
		const auto function_ptr = hf_kv.second._f;
		_host_functions.insert({ function_id, function_ptr });
	}

	//	Make the top-level environment = global scope.
	auto global_env = environment_t::make_environment(&_ast->_globals, {});
	_call_stack.push_back(global_env);

	_start_time = std::chrono::high_resolution_clock::now();

	//	Run static intialization (basically run global statements before calling main()).
	const auto r = execute_statements(*this, _ast->_globals._statements);

	_print_output = r.first._print_output;
	QUARK_ASSERT(check_invariant());
}

interpreter_t::interpreter_t(const interpreter_t& other) :
	_start_time(other._start_time),
	_ast(other._ast),
	_host_functions(other._host_functions),
	_call_stack(other._call_stack),
	_print_output(other._print_output)
{
	QUARK_ASSERT(other.check_invariant());
	QUARK_ASSERT(check_invariant());
}

//??? make proper operator=(). Exception safety etc.
const interpreter_t& interpreter_t::operator=(const interpreter_t& other){
	_start_time = other._start_time;
	_ast = other._ast;
	_host_functions = other._host_functions;
	_call_stack = other._call_stack;
	_print_output = other._print_output;
	return *this;
}

#if DEBUG
bool interpreter_t::check_invariant() const {
	QUARK_ASSERT(_ast->check_invariant());
	return true;
}
#endif


ast_t program_to_ast2(const interpreter_context_t& context, const string& program){
	parser_context_t context2{ quark::trace_context_t(context._tracer._verbose, context._tracer._tracer) };
//	parser_context_t context{ quark::make_default_tracer() };
//	QUARK_CONTEXT_TRACE(context._tracer, "Hello");

	const auto pass1 = floyd::parse_program2(context2, program);
	const auto pass2 = run_pass2(context2._tracer, pass1);
	const auto pass3 = floyd_pass3::run_pass3(context2._tracer, pass2);
	return pass3;
}

void print_vm_printlog(const interpreter_t& vm){
	if(vm._print_output.empty() == false){
		std::cout << "print output:\n";
		for(const auto line: vm._print_output){
			std::cout << line << "\n";
		}
	}
}

interpreter_t run_global(const interpreter_context_t& context, const string& source){
	auto ast = program_to_ast2(context, source);
	auto vm = interpreter_t(ast);
//	QUARK_TRACE(json_to_pretty_string(interpreter_to_json(vm)));
	print_vm_printlog(vm);
	return vm;
}

std::pair<interpreter_t, statement_result_t> run_main(const interpreter_context_t& context, const string& source, const vector<floyd::value_t>& args){
	auto ast = program_to_ast2(context, source);

	//	Runs global code.
	auto vm = interpreter_t(ast);

	const auto main_function = find_symbol_by_name(vm, "main");
	if(main_function != nullptr){
		const auto result = call_function(vm, main_function->first, args);
		return { result.first, result.second };
	}
	else{
		return {vm, statement_result_t::make_no_output()};
	}
}

std::pair<interpreter_t, statement_result_t> run_program(const interpreter_context_t& context, const ast_t& ast, const vector<floyd::value_t>& args){
	auto vm = interpreter_t(ast);

	const auto main_func = find_symbol_by_name(vm, "main");
	if(main_func != nullptr){
		const auto r = call_function(vm, main_func->first, args);
		return { r.first, r.second };
	}
	else{
		return { vm, statement_result_t::make_no_output() };
	}
}

value_t get_global(const interpreter_t& vm, const std::string& name){
    const auto it = std::find_if(
    	vm._call_stack.front()->_values.begin(),
    	vm._call_stack.front()->_values.end(),
    	[&name](const std::pair<std::string, std::pair<value_t, bool>>& e) { return e.first == name; }
	);
	if(it != vm._call_stack.front()->_values.end()){
		return it->second.first;
	}
	throw std::runtime_error("Cannot find global.");
}


}	//	floyd
