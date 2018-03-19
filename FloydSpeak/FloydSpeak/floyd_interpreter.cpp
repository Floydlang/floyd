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


value_t execute_call_expression(interpreter_t& vm, const expression_t& e);


environment_t* find_env_from_address(interpreter_t& vm, const variable_address_t& a){
	if(a._parent_steps == -1){
		return &vm._call_stack[0];
	}
	else{
		return &vm._call_stack[vm._call_stack.size() - 1 - a._parent_steps];
	}
}
const environment_t* find_env_from_address(const interpreter_t& vm, const variable_address_t& a){
	if(a._parent_steps == -1){
		return &vm._call_stack[0];
	}
	else{
		return &vm._call_stack[vm._call_stack.size() - 1 - a._parent_steps];
	}
}


value_t construct_value_from_typeid(interpreter_t& vm, const typeid_t& type, const vector<value_t>& arg_values){
	QUARK_ASSERT(vm.check_invariant());

	if(type.is_json_value()){
		QUARK_ASSERT(arg_values.size() == 1);

		const auto& arg = arg_values[0];
		const auto value = value_to_ast_json(arg);
		return value_t::make_json_value(value._value);
	}
	else if(type.is_bool() || type.is_int() || type.is_float() || type.is_string() || type.is_typeid()){
		QUARK_ASSERT(arg_values.size() == 1);

		const auto& arg = arg_values[0];
		if(type.is_string()){
			if(arg.is_json_value() && arg.get_json_value().is_string()){
				return value_t::make_string(arg.get_json_value().get_string());
			}
			else if(arg.is_string()){
			}
		}
		else{
			if(arg.get_type() != type){
			}
		}
		return arg;
	}
	else if(type.is_struct()){
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

		return instance;
	}
	else if(type.is_vector()){
		const auto& element_type = type.get_vector_element_type();
		QUARK_ASSERT(element_type.is_null() == false);

		return value_t::make_vector_value(element_type, arg_values);
	}
	else if(type.is_dict()){
		const auto& element_type = type.get_dict_value_type();
		QUARK_ASSERT(element_type.is_null() == false);

		std::map<string, value_t> m;
		for(auto i = 0 ; i < arg_values.size() / 2 ; i++){
			const auto& key = arg_values[i * 2 + 0].get_string_value();
			const auto& value = arg_values[i * 2 + 1];
			m.insert({ key, value });
		}
		return value_t::make_dict_value(element_type, m);
	}
	else if(type.is_function()){
	}
	else if(type.is_unresolved_type_identifier()){
	}
	else{
	}

	QUARK_ASSERT(false);
	throw std::exception();
}

statement_result_t execute_statements2(interpreter_t& vm, const std::vector<std::shared_ptr<statement_t>>& statements){
	QUARK_ASSERT(vm.check_invariant());
	for(const auto i: statements){ QUARK_ASSERT(i->check_invariant()); };

	int statement_index = 0;
	while(statement_index < statements.size()){
		const auto& statement = statements[statement_index];
		const auto& r = execute_statement(vm, *statement);
		if(r._type == statement_result_t::k_return_unwind){
			return r;
		}
		else{

			//	Last statement outputs its value, if any. This is passive output, not a return-unwind.
			if(statement_index == (statements.size() - 1)){
				if(r._type == statement_result_t::k_passive_expression_output){
					return r;
				}
			}
			else{
			}

			statement_index++;
		}
	}
	return statement_result_t::make_no_output();
}
statement_result_t execute_statements(interpreter_t& vm, const std::vector<std::shared_ptr<statement_t>>& statements){
	return execute_statements2(vm, statements);
}

statement_result_t execute_body(
	interpreter_t& vm,
	const body_t& body,
	const std::vector<value_t>& init_values
)
{
	QUARK_ASSERT(vm.check_invariant());

	const auto values_offset = vm._value_stack.size();
	for(const auto& e: init_values){
		vm._value_stack.push_back(e);
	}
	for(vector<value_t>::size_type i = init_values.size() ; i < body._symbols.size() ; i++){
		const auto& symbol = body._symbols[i];
		vm._value_stack.push_back(symbol.second._const_value);
	}
	vm._call_stack.push_back(environment_t{ &body, values_offset });

	const auto& r = execute_statements2(vm, body._statements);
	vm._call_stack.pop_back();
	vm._value_stack.resize(values_offset);

	return r;
}

statement_result_t execute_store2_statement(interpreter_t& vm, const statement_t::store2_t& statement){
	QUARK_ASSERT(vm.check_invariant());

	const auto& address = statement._dest_variable;
	const auto& rhs_value = execute_expression(vm, statement._expression);
	auto env = find_env_from_address(vm, address);
	const auto pos = env->_values_offset + address._index;
	QUARK_ASSERT(pos >= 0 && pos < vm._value_stack.size());
	vm._value_stack[pos] = rhs_value;

	return statement_result_t::make_no_output();
}

statement_result_t execute_return_statement(interpreter_t& vm, const statement_t::return_statement_t& statement){
	QUARK_ASSERT(vm.check_invariant());

	const auto& expr = statement._expression;
	const auto& rhs_value = execute_expression(vm, expr);
	return statement_result_t::make_return_unwind(rhs_value);
}

typeid_t find_type_by_name(const interpreter_t& vm, const typeid_t& type){
	if(type.get_base_type() == base_type::k_unresolved_type_identifier){
		const auto v = find_symbol_by_name(vm, type.get_unresolved_type_identifier());
		if(v){
			if(v->is_typeid()){
				return v->get_typeid_value();
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

statement_result_t execute_ifelse_statement(interpreter_t& vm, const statement_t::ifelse_statement_t& statement){
	QUARK_ASSERT(vm.check_invariant());

	const auto& condition_result_value = execute_expression(vm, statement._condition);
	QUARK_ASSERT(condition_result_value.is_bool());
	bool r = condition_result_value.get_bool_value();
	if(r){
		return execute_body(vm, statement._then_body, {});
	}
	else{
		return execute_body(vm, statement._else_body, {});
	}
}

statement_result_t execute_for_statement(interpreter_t& vm, const statement_t::for_statement_t& statement){
	QUARK_ASSERT(vm.check_invariant());

	const auto& start_value0 = execute_expression(vm, statement._start_expression);
	const auto start_value_int = start_value0.get_int_value();

	const auto& end_value0 = execute_expression(vm, statement._end_expression);
	const auto end_value_int = end_value0.get_int_value();

	vector<value_t> space_for_iterator = {value_t::make_null()};
	for(int x = start_value_int ; x <= end_value_int ; x++){
		space_for_iterator[0] = value_t::make_int(x);
		const auto& return_value = execute_body(vm, statement._body, space_for_iterator);
		if(return_value._type == statement_result_t::k_return_unwind){
			return return_value;
		}
	}
	return statement_result_t::make_no_output();
}

statement_result_t execute_while_statement(interpreter_t& vm, const statement_t::while_statement_t& statement){
	QUARK_ASSERT(vm.check_invariant());

	bool again = true;
	while(again){
		const auto& condition_value_expr = execute_expression(vm, statement._condition);
		const auto& condition_value = condition_value_expr.get_bool_value();

		if(condition_value){
			const auto& return_value = execute_body(vm, statement._body, {});
			if(return_value._type == statement_result_t::k_return_unwind){
				return return_value;
			}
		}
		else{
			again = false;
		}
	}
	return statement_result_t::make_no_output();
}

statement_result_t execute_expression_statement(interpreter_t& vm, const statement_t::expression_statement_t& statement){
	QUARK_ASSERT(vm.check_invariant());

	const auto& rhs_value = execute_expression(vm, statement._expression);
	return statement_result_t::make_passive_expression_output(rhs_value);
}

statement_result_t execute_statement(interpreter_t& vm, const statement_t& statement){
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
	throw std::exception();
}



//	Warning: returns reference to the found value-entry -- this could be in any environment in the call stack.
const floyd::value_t* find_symbol_by_name_deep(const interpreter_t& vm, int depth, const std::string& s){
	QUARK_ASSERT(vm.check_invariant());
	QUARK_ASSERT(depth >= 0 && depth < vm._call_stack.size());
	QUARK_ASSERT(s.size() > 0);

	const auto env = &vm._call_stack[depth];
    const auto& it = std::find_if(
    	env->_body_ptr->_symbols.begin(),
    	env->_body_ptr->_symbols.end(),
    	[&s](const std::pair<std::string, symbol_t>& e) { return e.first == s; }
	);
	if(it != env->_body_ptr->_symbols.end()){
		const auto index = it - env->_body_ptr->_symbols.begin();
		const auto pos = env->_values_offset + index;
		QUARK_ASSERT(pos >= 0 && pos < vm._value_stack.size());
		return &vm._value_stack[pos];
	}
	else if(depth > 0){
		return find_symbol_by_name_deep(vm, depth - 1, s);
	}
	else{
		return nullptr;
	}
}

//	Warning: returns reference to the found value-entry -- this could be in any environment in the call stack.
const floyd::value_t* find_symbol_by_name(const interpreter_t& vm, const std::string& s){
	QUARK_ASSERT(vm.check_invariant());
	QUARK_ASSERT(s.size() > 0);

	return find_symbol_by_name_deep(vm, (int)(vm._call_stack.size() - 1), s);
}

floyd::value_t find_global_symbol(const interpreter_t& vm, const string& s){
	return get_global(vm, s);
}
value_t get_global(const interpreter_t& vm, const std::string& name){
	const auto result = find_symbol_by_name_deep(vm, 0, name);
	if(result == nullptr){
		throw std::runtime_error("Cannot find global.");
	}
	else{
		return *result;
	}
}



value_t execute_resolve_member_expression(interpreter_t& vm, const expression_t::resolve_member_expr_t& expr){
	QUARK_ASSERT(vm.check_invariant());

	const auto& parent_expr = execute_expression(vm, *expr._parent_address);
	QUARK_ASSERT(parent_expr.is_struct());

	const auto& struct_instance = parent_expr.get_struct_value();

	int index = find_struct_member_index(*struct_instance->_def, expr._member_name);
	QUARK_ASSERT(index != -1);

	const value_t value = struct_instance->_member_values[index];
	return value;
}

value_t execute_lookup_element_expression(interpreter_t& vm, const expression_t::lookup_expr_t& expr){
	QUARK_ASSERT(vm.check_invariant());

	const auto& parent_value = execute_expression(vm, *expr._parent_address);
	const auto& key_value = execute_expression(vm, *expr._lookup_key);
	if(parent_value.is_string()){
		QUARK_ASSERT(key_value.is_int());

		const auto& instance = parent_value.get_string_value();
		int lookup_index = key_value.get_int_value();
		if(lookup_index < 0 || lookup_index >= instance.size()){
			throw std::runtime_error("Lookup in string: out of bounds.");
		}
		else{
			const char ch = instance[lookup_index];
			const auto value2 = value_t::make_string(string(1, ch));
			return value2;
		}
	}
	else if(parent_value.is_json_value()){
		//	Notice: the exact type of value in the json_value is only known at runtime = must be checked in interpreter.
		const auto& parent_json_value = parent_value.get_json_value();
		if(parent_json_value.is_object()){
			QUARK_ASSERT(key_value.is_string());
			const auto& lookup_key = key_value.get_string_value();

			//	get_object_element() throws if key can't be found.
			const auto& value = parent_json_value.get_object_element(lookup_key);
			const auto value2 = value_t::make_json_value(value);
			return value2;
		}
		else if(parent_json_value.is_array()){
			QUARK_ASSERT(key_value.is_int());
			const auto& lookup_index = key_value.get_int_value();

			if(lookup_index < 0 || lookup_index >= parent_json_value.get_array_size()){
				throw std::runtime_error("Lookup in json_value array: out of bounds.");
			}
			else{
				const auto& value = parent_json_value.get_array_n(lookup_index);
				const auto value2 = value_t::make_json_value(value);
				return value2;
			}
		}
		else{
			throw std::runtime_error("Lookup using [] on json_value only works on objects and arrays.");
		}
	}
	else if(parent_value.is_vector()){
		QUARK_ASSERT(key_value.is_int());

		const auto& vec = parent_value.get_vector_value();

		int lookup_index = key_value.get_int_value();
		if(lookup_index < 0 || lookup_index >= vec.size()){
			throw std::runtime_error("Lookup in vector: out of bounds.");
		}
		else{
			const value_t value = vec[lookup_index];
			return value;
		}
	}
	else if(parent_value.is_dict()){
		QUARK_ASSERT(key_value.is_string());

		const auto& entries = parent_value.get_dict_value();
		const string key = key_value.get_string_value();

		const auto& found_it = entries.find(key);
		if(found_it == entries.end()){
			throw std::runtime_error("Lookup in dict: key not found.");
		}
		else{
			const value_t value = found_it->second;
			return value;
		}
	}
	else {
		QUARK_ASSERT(false);
		throw std::exception();
	}
}

value_t execute_load2_expression(interpreter_t& vm, const expression_t& e){
	QUARK_ASSERT(vm.check_invariant());

	const auto& expr = *e.get_load2();

	environment_t* env = find_env_from_address(vm, expr._address);
	const auto pos = env->_values_offset + expr._address._index;
	QUARK_ASSERT(pos >= 0 && pos < vm._value_stack.size());
	const auto& value = vm._value_stack[pos];
	QUARK_ASSERT(value.get_type() == e.get_annotated_type() /*|| e.get_annotated_type().is_null()*/);
	return value;
}

value_t execute_construct_value_expression(interpreter_t& vm, const expression_t& e){
	QUARK_ASSERT(vm.check_invariant());

	const auto& expr = *e.get_construct_value();

	if(expr._value_type2.is_vector()){
		const std::vector<expression_t>& elements = expr._args;
		const auto& root_value_type = expr._value_type2;
		const auto& element_type = root_value_type.get_vector_element_type();
		QUARK_ASSERT(element_type.is_null() == false);

		//	An empty vector is encoded as a constant value by pass3, not a vector-definition-expression.
		QUARK_ASSERT(elements.empty() == false);

		QUARK_ASSERT(root_value_type.is_null() == false);
		QUARK_ASSERT(element_type.is_null() == false);

		std::vector<value_t> elements2;
		for(const auto& m: elements){
			const auto& element = execute_expression(vm, m);
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
		const auto& root_value_type = expr._value_type2;
		const auto& element_type = root_value_type.get_dict_value_type();

		//	An empty dict is encoded as a constant value pass3, not a dict-definition-expression.
		QUARK_ASSERT(elements.empty() == false);

		QUARK_ASSERT(root_value_type.is_null() == false);
		QUARK_ASSERT(element_type.is_null() == false);

		std::vector<value_t> elements2;
		for(auto i = 0 ; i < elements.size() / 2 ; i++){
			const auto& key_expr = elements[i * 2 + 0];
			const auto& value_expr = elements[i * 2 + 1];

			const auto& value = execute_expression(vm, value_expr);
			const string key = key_expr.get_literal().get_string_value();
			elements2.push_back(value_t::make_string(key));
			elements2.push_back(value);
		}
		return construct_value_from_typeid(vm, typeid_t::make_dict(element_type), elements2);
	}
	else if(expr._value_type2.is_struct()){
		std::vector<value_t> elements2;
		for(const auto m: expr._args){
			const auto& element = execute_expression(vm, m);
			elements2.push_back(element);
		}
		return construct_value_from_typeid(vm, expr._value_type2, elements2);
	}
	else{
		QUARK_ASSERT(expr._args.size() == 1);

		const auto& element = execute_expression(vm, expr._args[0]);
		return construct_value_from_typeid(vm, expr._value_type2, { element });
	}
}

value_t execute_arithmetic_unary_minus_expression(interpreter_t& vm, const expression_t::unary_minus_expr_t& expr){
	QUARK_ASSERT(vm.check_invariant());

	const auto& c = execute_expression(vm, *expr._expr);
	if(c.is_int()){
		return execute_expression(
			vm,
			expression_t::make_simple_expression__2(
				expression_type::k_arithmetic_subtract__2,
				expression_t::make_literal_int(0),
				expression_t::make_literal(c),
				make_shared<typeid_t>(c.get_type())
			)
		);
	}
	else if(c.is_float()){
		return execute_expression(
			vm,
			expression_t::make_simple_expression__2(
				expression_type::k_arithmetic_subtract__2,
				expression_t::make_literal_float(0.0f),
				expression_t::make_literal(c),
				make_shared<typeid_t>(c.get_type())
			)
		);
	}
	else{
		QUARK_ASSERT(false);
		throw std::exception();
	}
}

value_t execute_conditional_operator_expression(interpreter_t& vm, const expression_t::conditional_expr_t& expr){
	QUARK_ASSERT(vm.check_invariant());

	//	Special-case since it uses 3 expressions & uses shortcut evaluation.
	const auto& cond_result = execute_expression(vm, *expr._condition);
	QUARK_ASSERT(cond_result.is_bool());
	const bool cond_flag = cond_result.get_bool_value();

	//	!!! Only execute the CHOSEN expression. Not that important since functions are pure.
	if(cond_flag){
		return execute_expression(vm, *expr._a);
	}
	else{
		return execute_expression(vm, *expr._b);
	}
}

value_t execute_comparison_expression(interpreter_t& vm, expression_type op, const expression_t::simple_expr__2_t& simple2_expr){
	QUARK_ASSERT(vm.check_invariant());

	const auto& left_constant = execute_expression(vm, *simple2_expr._left);
	const auto& right_constant = execute_expression(vm, *simple2_expr._right);
	QUARK_ASSERT(left_constant.get_type() == right_constant.get_type());

	//	Do generic functionallity, independant on type.
	if(op == expression_type::k_comparison_smaller_or_equal__2){
		long diff = value_t::compare_value_true_deep(left_constant, right_constant);
		return value_t::make_bool(diff <= 0);
	}
	else if(op == expression_type::k_comparison_smaller__2){
		long diff = value_t::compare_value_true_deep(left_constant, right_constant);
		return value_t::make_bool(diff < 0);
	}
	else if(op == expression_type::k_comparison_larger_or_equal__2){
		long diff = value_t::compare_value_true_deep(left_constant, right_constant);
		return value_t::make_bool(diff >= 0);
	}
	else if(op == expression_type::k_comparison_larger__2){
		long diff = value_t::compare_value_true_deep(left_constant, right_constant);
		return value_t::make_bool(diff > 0);
	}

	else if(op == expression_type::k_logical_equal__2){
		long diff = value_t::compare_value_true_deep(left_constant, right_constant);
		return value_t::make_bool(diff == 0);
	}
	else if(op == expression_type::k_logical_nonequal__2){
		long diff = value_t::compare_value_true_deep(left_constant, right_constant);
		return value_t::make_bool(diff != 0);
	}
	else{
		QUARK_ASSERT(false);
		throw std::exception();
	}
}

value_t execute_arithmetic_expression(interpreter_t& vm, expression_type op, const expression_t::simple_expr__2_t& simple2_expr){
	QUARK_ASSERT(vm.check_invariant());

	const auto& left_constant = execute_expression(vm, *simple2_expr._left);
	const auto& right_constant = execute_expression(vm, *simple2_expr._right);
	QUARK_ASSERT(left_constant.get_type() == right_constant.get_type());

	const auto& type_mode = left_constant.get_type();

	//	bool
	if(type_mode.is_bool()){
		const bool left = left_constant.get_bool_value();
		const bool right = right_constant.get_bool_value();

		if(op == expression_type::k_arithmetic_add__2){
			return value_t::make_bool(left + right);
		}
		else if(op == expression_type::k_logical_and__2){
			return value_t::make_bool(left && right);
		}
		else if(op == expression_type::k_logical_or__2){
			return value_t::make_bool(left || right);
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
			return value_t::make_int(left + right);
		}
		else if(op == expression_type::k_arithmetic_subtract__2){
			return value_t::make_int(left - right);
		}
		else if(op == expression_type::k_arithmetic_multiply__2){
			return value_t::make_int(left * right);
		}
		else if(op == expression_type::k_arithmetic_divide__2){
			if(right == 0){
				throw std::runtime_error("EEE_DIVIDE_BY_ZERO");
			}
			return value_t::make_int(left / right);
		}
		else if(op == expression_type::k_arithmetic_remainder__2){
			if(right == 0){
				throw std::runtime_error("EEE_DIVIDE_BY_ZERO");
			}
			return value_t::make_int(left % right);
		}

		//??? Could be replaced by feature to convert any value to bool -- they use a generic comparison for && and ||
		else if(op == expression_type::k_logical_and__2){
			return value_t::make_bool((left != 0) && (right != 0));
		}
		else if(op == expression_type::k_logical_or__2){
			return value_t::make_bool((left != 0) || (right != 0));
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
			return value_t::make_float(left + right);
		}
		else if(op == expression_type::k_arithmetic_subtract__2){
			return value_t::make_float(left - right);
		}
		else if(op == expression_type::k_arithmetic_multiply__2){
			return value_t::make_float(left * right);
		}
		else if(op == expression_type::k_arithmetic_divide__2){
			if(right == 0.0f){
				throw std::runtime_error("EEE_DIVIDE_BY_ZERO");
			}
			return value_t::make_float(left / right);
		}

		else if(op == expression_type::k_logical_and__2){
			return value_t::make_bool((left != 0.0f) && (right != 0.0f));
		}
		else if(op == expression_type::k_logical_or__2){
			return value_t::make_bool((left != 0.0f) || (right != 0.0f));
		}
		else{
			QUARK_ASSERT(false);
		}
	}

	//	string
	else if(type_mode.is_string()){
		const auto& left = left_constant.get_string_value();
		const auto& right = right_constant.get_string_value();

		if(op == expression_type::k_arithmetic_add__2){
			return value_t::make_string(left + right);
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
		const auto& element_type = left_constant.get_type().get_vector_element_type();
		if(op == expression_type::k_arithmetic_add__2){
			auto elements2 = left_constant.get_vector_value();
			elements2.insert(elements2.end(), right_constant.get_vector_value().begin(), right_constant.get_vector_value().end());

			const auto& value2 = value_t::make_vector_value(element_type, elements2);
			return value2;
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
	throw std::exception();
}

value_t execute_expression(interpreter_t& vm, const expression_t& e){
	QUARK_ASSERT(vm.check_invariant());
	QUARK_ASSERT(e.check_invariant());

	const auto& op = e.get_operation();

	if(op == expression_type::k_literal){
		return e.get_literal();
	}
	else if(op == expression_type::k_resolve_member){
		return execute_resolve_member_expression(vm, *e.get_resolve_member());
	}
	else if(op == expression_type::k_lookup_element){
		return execute_lookup_element_expression(vm, *e.get_lookup());
	}
	else if(op == expression_type::k_load){
		QUARK_ASSERT(false);
	}
	else if(op == expression_type::k_load2){
		return execute_load2_expression(vm, e);
	}

	else if(op == expression_type::k_call){
		return execute_call_expression(vm, e);
	}

	else if(op == expression_type::k_define_struct){
		// Moved entire function to symbol table -- no need for k_define_function-expression in interpreter!
		QUARK_ASSERT(false);
		return value_t::make_null();
	}

	//??? Move entire function to symbol table -- no need for k_define_function-expression in interpreter!
	else if(op == expression_type::k_define_function){
		const auto& expr = e.get_function_definition();
		return value_t::make_function_value(expr->_def);
	}

	else if(op == expression_type::k_construct_value){
		return execute_construct_value_expression(vm, e);
	}

	//	This can be desugared at compile time.
	else if(op == expression_type::k_arithmetic_unary_minus__1){
		return execute_arithmetic_unary_minus_expression(vm, *e.get_unary_minus());
	}

	//	Special-case since it uses 3 expressions & uses shortcut evaluation.
	else if(op == expression_type::k_conditional_operator3){
		return execute_conditional_operator_expression(vm, *e.get_conditional());
	}
	else if (false
		|| op == expression_type::k_comparison_smaller_or_equal__2
		|| op == expression_type::k_comparison_smaller__2
		|| op == expression_type::k_comparison_larger_or_equal__2
		|| op == expression_type::k_comparison_larger__2

		|| op == expression_type::k_logical_equal__2
		|| op == expression_type::k_logical_nonequal__2
	){
		return execute_comparison_expression(vm, op, *e.get_simple__2());
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
		return execute_arithmetic_expression(vm, op, *e.get_simple__2());
	}
	else{
		QUARK_ASSERT(false);
	}
	throw std::exception();
}

statement_result_t call_function(interpreter_t& vm, const floyd::value_t& f, const vector<value_t>& args){
#if DEBUG
	QUARK_ASSERT(vm.check_invariant());
	QUARK_ASSERT(f.check_invariant());
	for(const auto i: args){ QUARK_ASSERT(i.check_invariant()); };
	QUARK_ASSERT(f.is_function());
#endif

	const auto& function_def = f.get_function_value();
	if(function_def->_host_function != 0){
		const auto& r = call_host_function(vm, function_def->_host_function, args);
		return r;
	}
	else{
#if DEBUG
		const auto arg_types = f.get_type().get_function_args();

		//	arity
		QUARK_ASSERT(args.size() == arg_types.size());

		for(int i = 0 ; i < args.size() ; i++){
			if(args[i].get_type() != arg_types[i]){
				QUARK_ASSERT(false);
			}
		}
#endif

		const auto& r = execute_body(vm, *function_def->_body, args);

/*
		const auto& return_type = f.get_type().get_function_return();
		// ??? move this check to pass3.
		if(r._type != statement_result_t::k_return_unwind){
			throw std::runtime_error("Function missing return statement");
		}

		// ??? move this check to pass3.
		else if(r._output.get_type().is_struct() == false && r._output.get_type() != return_type){
			throw std::runtime_error("Function return type wrong.");
		}
		else{
		}
*/

		return r;
	}
}

value_t execute_call_expression(interpreter_t& vm, const expression_t& e){
	QUARK_ASSERT(vm.check_invariant());

	const auto& expr = *e.get_call();

	const auto& function_value = execute_expression(vm, *expr._callee);
	QUARK_ASSERT(function_value.is_function());

	vector<value_t> arg_values;
	for(int i = 0 ; i < expr._args.size() ; i++){
		const auto& t = execute_expression(vm, expr._args[i]);
		arg_values.push_back(t);
	}

	const auto& result = call_function(vm, function_value, arg_values);
	QUARK_ASSERT(result._type == statement_result_t::k_return_unwind);
	return result._output;
}

json_t interpreter_to_json(const interpreter_t& vm){
	vector<json_t> callstack;
	for(int env_index = 0 ; env_index < vm._call_stack.size() ; env_index++){
		const auto e = &vm._call_stack[vm._call_stack.size() - 1 - env_index];

		const auto local_end = (env_index == (vm._call_stack.size() - 1)) ? vm._value_stack.size() : vm._call_stack[vm._call_stack.size() - 1 - env_index + 1]._values_offset;
		const auto local_count = local_end - e->_values_offset;
		std::vector<json_t> values;
		for(int local_index = 0 ; local_index < local_count ; local_index++){
			const auto& v = vm._value_stack[e->_values_offset + local_index];
			const auto& a = value_and_type_to_ast_json(v);
			values.push_back(a._value);
		}

		const auto& env = json_t::make_object({
			{ "values", values }
		});
		callstack.push_back(env);
	}

	return json_t::make_object({
		{ "ast", ast_to_json(vm._imm->_ast)._value },
		{ "callstack", json_t::make_array(callstack) }
	});
}

void test__execute_expression(const expression_t& e, const value_t& expected_value){
	const ast_t ast;
	interpreter_t interpreter(ast);
	const auto& e3 = execute_expression(interpreter, e);

	ut_compare_jsons(value_to_ast_json(e3)._value, value_to_ast_json(expected_value)._value);
}


QUARK_UNIT_TESTQ("execute_expression()", "literal 1234 == 1234") {
	test__execute_expression(
		expression_t::make_literal_int(1234),
		value_t::make_int(1234)
	);
}

QUARK_UNIT_TESTQ("execute_expression()", "1 + 2 == 3") {
	test__execute_expression(
		expression_t::make_simple_expression__2(
			expression_type::k_arithmetic_add__2,
			expression_t::make_literal_int(1),
			expression_t::make_literal_int(2),
			make_shared<typeid_t>(typeid_t::make_int())
		),
		value_t::make_int(3)
	);
}


QUARK_UNIT_TESTQ("execute_expression()", "3 * 4 == 12") {
	test__execute_expression(
		expression_t::make_simple_expression__2(
			expression_type::k_arithmetic_multiply__2,
			expression_t::make_literal_int(3),
			expression_t::make_literal_int(4),
			make_shared<typeid_t>(typeid_t::make_int())
		),
		value_t::make_int(12)
	);
}

QUARK_UNIT_TESTQ("execute_expression()", "(3 * 4) * 5 == 60") {
	test__execute_expression(
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




statement_result_t call_host_function(interpreter_t& vm, int function_id, const std::vector<floyd::value_t> args){
	QUARK_ASSERT(function_id >= 0);

	const auto& host_function = vm._imm->_host_functions.at(function_id);

	//	arity
//	QUARK_ASSERT(args.size() == host_function._function_type.get_function_args().size());

	const auto& result = (host_function)(vm, args);
	return statement_result_t::make_return_unwind(result);
}

interpreter_t::interpreter_t(const ast_t& ast){
	QUARK_ASSERT(ast.check_invariant());

	_value_stack.reserve(1024);
	_call_stack.reserve(32);

	//	Make lookup table from host-function ID to an implementation of that host function in the interpreter.
	const auto& host_functions = get_host_functions();
	std::map<int, HOST_FUNCTION_PTR> host_functions2;
	for(auto& hf_kv: host_functions){
		const auto& function_id = hf_kv.second._signature._function_id;
		const auto& function_ptr = hf_kv.second._f;
		host_functions2.insert({ function_id, function_ptr });
	}

	const auto start_time = std::chrono::high_resolution_clock::now();
	_imm = std::make_shared<interpreter_imm_t>(interpreter_imm_t{start_time, ast, host_functions2});


	const auto values_offset = _value_stack.size();
	const auto& body_ptr = _imm->_ast._globals;
	for(vector<value_t>::size_type i = 0 ; i < body_ptr._symbols.size() ; i++){
		const auto& symbol = body_ptr._symbols[i];
		_value_stack.push_back(symbol.second._const_value);
	}
	_call_stack.push_back(environment_t{ &body_ptr, values_offset });

	//	Run static intialization (basically run global statements before calling main()).
	const auto& r = execute_statements2(*this, _imm->_ast._globals._statements);
	QUARK_ASSERT(check_invariant());
}

interpreter_t::interpreter_t(const interpreter_t& other) :
	_imm(other._imm),
	_value_stack(other._value_stack),
	_call_stack(other._call_stack),
	_print_output(other._print_output)
{
	QUARK_ASSERT(other.check_invariant());
	QUARK_ASSERT(check_invariant());
}

void interpreter_t::swap(interpreter_t& other) throw(){
	other._imm.swap(this->_imm);
	other._value_stack.swap(this->_value_stack);
	_call_stack.swap(this->_call_stack);
	other._print_output.swap(this->_print_output);
}

const interpreter_t& interpreter_t::operator=(const interpreter_t& other){
	auto temp = other;
	temp.swap(*this);
	return *this;
}

#if DEBUG
bool interpreter_t::check_invariant() const {
	QUARK_ASSERT(_imm->_ast.check_invariant());
	return true;
}
#endif


ast_t program_to_ast2(const interpreter_context_t& context, const string& program){
	parser_context_t context2{ quark::trace_context_t(context._tracer._verbose, context._tracer._tracer) };
//	parser_context_t context{ quark::make_default_tracer() };
//	QUARK_CONTEXT_TRACE(context._tracer, "Hello");

	const auto& pass1 = floyd::parse_program2(context2, program);
	const auto& pass2 = run_pass2(context2._tracer, pass1);
	const auto& pass3 = floyd_pass3::run_pass3(context2._tracer, pass2);
	return pass3;
}

void print_vm_printlog(const interpreter_t& vm){
	if(vm._print_output.empty() == false){
		std::cout << "print output:\n";
		for(const auto& line: vm._print_output){
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

	const auto& main_function = find_symbol_by_name(vm, "main");
	if(main_function != nullptr){
		const auto& result = call_function(vm, *main_function, args);
		return { vm, result };
	}
	else{
		return {vm, statement_result_t::make_no_output()};
	}
}

std::pair<interpreter_t, statement_result_t> run_program(const interpreter_context_t& context, const ast_t& ast, const vector<floyd::value_t>& args){
	auto vm = interpreter_t(ast);

	const auto& main_func = find_symbol_by_name(vm, "main");
	if(main_func != nullptr){
		const auto& r = call_function(vm, *main_func, args);
		return { vm, r };
	}
	else{
		return { vm, statement_result_t::make_no_output() };
	}
}


}	//	floyd
