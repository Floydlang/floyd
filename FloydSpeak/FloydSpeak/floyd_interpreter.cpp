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



std::pair<interpreter_t, expression_t> evaluate_call_expression(const interpreter_t& vm, const expression_t& e);




//### add checking of function types when calling / returning from them. Also host functions.

typeid_t resolve_type_using_env(const interpreter_t& vm, const typeid_t& type){
	if(type.get_base_type() == base_type::k_unresolved_type_identifier){
//		QUARK_ASSERT(false);

		const auto v = resolve_env_variable(vm, type.get_unresolved_type_identifier());
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

std::pair<interpreter_t, value_t> construct_struct(const interpreter_t& vm, const typeid_t& struct_type, const vector<value_t>& values){
	QUARK_ASSERT(struct_type.get_base_type() == base_type::k_struct);
	QUARK_SCOPED_TRACE("construct_struct()");
	QUARK_TRACE("struct_type: " + typeid_to_compact_string(struct_type));

	const string struct_type_name = "unnamed";
	const auto& def = struct_type.get_struct();
	QUARK_ASSERT(values.size() == def._members.size());

#if DEBUG
	for(int i = 0 ; i < def._members.size() ; i++){
		const auto v = values[i];
		const auto a = def._members[i];
		QUARK_ASSERT(v.check_invariant());
		QUARK_ASSERT(v.get_type().get_base_type() != base_type::k_unresolved_type_identifier);
		QUARK_ASSERT(v.get_type() == a._type);
	}
#endif

	const auto instance = value_t::make_struct_value(struct_type, values);
	QUARK_TRACE(to_compact_string2(instance));

	return std::pair<interpreter_t, value_t>(vm, instance);
}


std::pair<interpreter_t, value_t> construct_value_from_typeid(const interpreter_t& vm, const typeid_t& type, const vector<value_t>& arg_values){
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
		//	Constructor.
		const auto result = construct_struct(vm, type, arg_values);
		return { result.first, result.second };
	}
	else if(type.is_vector()){
/*
		const auto element_type = type.get_vector_element_type();
		vector<value_t> elements2;
		if(element_type.is_vector()){
			for(const auto e: arg_values){
				const auto result2 = construct_value_from_typeid(vm, element_type, e.get_vector_value()->_elements);
				elements2.push_back(result2.second);
			}
		}
		else{
			elements2 = arg_values;
		}
		const auto result = value_t::make_vector_value(element_type, elements2);
		return {vm, result };
*/

	}
	else if(type.is_dict()){
	}
	else if(type.is_function()){
	}
	else if(type.is_unresolved_type_identifier()){
	}
	else{
	}

	throw std::runtime_error("Cannot call non-function.");
}


interpreter_t begin_subenv(const interpreter_t& vm){
	QUARK_ASSERT(vm.check_invariant());

	auto vm2 = vm;

	auto parent_env = vm2._call_stack.back();
	auto new_environment = environment_t::make_environment(vm2, parent_env);
	vm2._call_stack.push_back(new_environment);
	return vm2;
}

std::pair<interpreter_t, statement_result_t> execute_statements_in_env(
	const interpreter_t& vm,
	const std::vector<std::shared_ptr<statement_t>>& statements,
	const std::map<std::string, std::pair<value_t, bool>>& values
){
	QUARK_ASSERT(vm.check_invariant());

	auto vm2 = begin_subenv(vm);

	vm2._call_stack.back()->_values.insert(values.begin(), values.end());

	const auto r = execute_statements(vm2, statements);
	vm2 = r.first;
	vm2._call_stack.pop_back();
	return { vm2, r.second };
}


std::pair<interpreter_t, statement_result_t> execute_bind_local_statement(const interpreter_t& vm, const statement_t::bind_local_t& statement){
	QUARK_ASSERT(vm.check_invariant());

	auto vm_acc = vm;
	const auto name = statement._new_local_name;

	const auto rhs_expr_pair = evaluate_expression(vm_acc, statement._expression);
	vm_acc = rhs_expr_pair.first;

	QUARK_ASSERT(rhs_expr_pair.second.is_literal());

	const auto rhs_value = rhs_expr_pair.second.get_literal();

	const auto bind_statement_type = statement._bindtype;
	const auto bind_statement_mutable_mode = statement._locals_mutable_mode;
	const auto mutable_flag = bind_statement_mutable_mode == statement_t::bind_local_t::k_mutable;

	QUARK_ASSERT(vm_acc._call_stack.back()->_values.count(name) == 0);

	//	Deduced bind type -- use new value's type.
	//??? Should not be needed in interpreter. Move to pass3.
	if(bind_statement_type.is_null()){
//		QUARK_ASSERT(false);
		vm_acc._call_stack.back()->_values[name] = std::pair<value_t, bool>(rhs_value, mutable_flag);
	}

	//	Explicit bind-type -- make sure source + dest types match.
	else{
		if(bind_statement_type != rhs_value.get_type()){
			throw std::runtime_error("Types not compatible in bind.");
		}
		vm_acc._call_stack.back()->_values[name] = std::pair<value_t, bool>(rhs_value, mutable_flag);
	}
	return { vm_acc, statement_result_t::make_no_output() };
}

std::pair<interpreter_t, statement_result_t> execute_store_local_statement(const interpreter_t& vm, const statement_t::store_local_t& statement){
	QUARK_ASSERT(vm.check_invariant());

	auto vm_acc = vm;
	const auto local_name = statement._local_name;

	const auto rhs_expr_pair = evaluate_expression(vm_acc, statement._expression);
	vm_acc = rhs_expr_pair.first;

	QUARK_ASSERT(rhs_expr_pair.second.is_literal());
	const auto rhs_value = rhs_expr_pair.second.get_literal();

	const auto lhs_value_deep_ptr = resolve_env_variable(vm_acc, local_name);
	const bool lhs_value_is_mutable = lhs_value_deep_ptr && lhs_value_deep_ptr->second;

	QUARK_ASSERT(lhs_value_deep_ptr != nullptr);
	QUARK_ASSERT(lhs_value_is_mutable);

	const auto lhs_value = lhs_value_deep_ptr->first;
	QUARK_ASSERT(lhs_value.get_type() == rhs_value.get_type());

	*lhs_value_deep_ptr = std::pair<value_t, bool>(rhs_value, true);
	return { vm_acc, statement_result_t::make_no_output() };
}

std::pair<interpreter_t, statement_result_t> execute_return_statement(const interpreter_t& vm, const statement_t::return_statement_t& statement){
	QUARK_ASSERT(vm.check_invariant());

	auto vm_acc = vm;
	const auto expr = statement._expression;
	const auto result = evaluate_expression(vm_acc, expr);
	vm_acc = result.first;

	QUARK_ASSERT(result.second.is_literal());

	const auto rhs_value = result.second.get_literal();
	return {
		vm_acc,
		statement_result_t::make_return_unwind(rhs_value)
	};
}

//??? make structs unique even though they share layout and name. USer unique-ID-generator?
std::pair<interpreter_t, statement_result_t> execute_def_struct_statement(const interpreter_t& vm, const statement_t::define_struct_statement_t& statement){
	QUARK_ASSERT(vm.check_invariant());

	auto vm_acc = vm;
	const auto struct_name = statement._name;

	QUARK_ASSERT(vm_acc._call_stack.back()->_values.count(struct_name) == 0);

	//	Resolve member types in this scope.
	std::vector<member_t> members2;
	for(const auto e: statement._def._members){
		const auto name = e._name;
		const auto type = e._type;
		const auto type2 = resolve_type_using_env(vm_acc, type);
		const auto e2 = member_t(type2, name);
		members2.push_back(e2);
	}
	const auto resolved_struct_def = std::make_shared<struct_definition_t>(struct_definition_t(members2));
	const auto struct_typeid = typeid_t::make_struct(resolved_struct_def);
	vm_acc._call_stack.back()->_values[struct_name] = std::pair<value_t, bool>(value_t::make_typeid_value(struct_typeid), false);
	return { vm_acc, statement_result_t::make_no_output() };
}

std::pair<interpreter_t, statement_result_t> execute_ifelse_statement(const interpreter_t& vm, const statement_t::ifelse_statement_t& statement){
	QUARK_ASSERT(vm.check_invariant());

	auto vm_acc = vm;

	const auto condition_result = evaluate_expression(vm_acc, statement._condition);
	vm_acc = condition_result.first;
	QUARK_ASSERT(condition_result.second.is_literal());
	QUARK_ASSERT(condition_result.second.get_literal().is_bool());

	const auto condition_result_value = condition_result.second.get_literal();
	bool r = condition_result_value.get_bool_value();
	if(r){
		return execute_statements_in_env(vm_acc, statement._then_statements, {});
	}
	else{
		return execute_statements_in_env(vm_acc, statement._else_statements, {});
	}
}

std::pair<interpreter_t, statement_result_t> execute_for_statement(const interpreter_t& vm, const statement_t::for_statement_t& statement){
	QUARK_ASSERT(vm.check_invariant());

	auto vm_acc = vm;

	const auto start_value0 = evaluate_expression(vm_acc, statement._start_expression);
	vm_acc = start_value0.first;
	const auto start_value_int = start_value0.second.get_literal().get_int_value();

	const auto end_value0 = evaluate_expression(vm_acc, statement._end_expression);
	vm_acc = end_value0.first;
	const auto end_value_int = end_value0.second.get_literal().get_int_value();

	for(int x = start_value_int ; x <= end_value_int ; x++){
		const std::map<std::string, std::pair<value_t, bool>> values = { { statement._iterator_name, std::pair<value_t, bool>(value_t::make_int(x), false) } };
		const auto result = execute_statements_in_env(vm_acc, statement._body, values);
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
		const auto condition_value = condition_value_expr.second.get_literal().get_bool_value();

		if(condition_value){
			const auto result = execute_statements_in_env(vm_acc, statement._body, {});
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
	const auto rhs_value = result.second.get_literal();
	return {
		vm_acc,
		statement_result_t::make_passive_expression_output(rhs_value)
	};
}


//	Output is the RETURN VALUE of the executed statement, if any.
std::pair<interpreter_t, statement_result_t> execute_statement(const interpreter_t& vm, const statement_t& statement){
	QUARK_ASSERT(vm.check_invariant());
	QUARK_ASSERT(statement.check_invariant());

	if(statement._bind_local){
		return execute_bind_local_statement(vm, *statement._bind_local);
	}
	else if(statement._store_local){
		return execute_store_local_statement(vm, *statement._store_local);
	}
	else if(statement._block){
		return execute_statements_in_env(vm, statement._block->_statements, {});
	}
	else if(statement._return){
		return execute_return_statement(vm, *statement._return);
	}
	else if(statement._def_struct){
		return execute_def_struct_statement(vm, *statement._def_struct);
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
		throw std::exception();
	}
}



std::pair<interpreter_t, statement_result_t> execute_statements(const interpreter_t& vm, const vector<shared_ptr<statement_t>>& statements){
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

//	Warning: returns reference to the found value-entry -- this could be in any environment in the call stack.
std::pair<floyd::value_t, bool>* resolve_env_variable_deep(const interpreter_t& vm, const shared_ptr<environment_t>& env, const std::string& s){
	QUARK_ASSERT(vm.check_invariant());
	QUARK_ASSERT(env && env->check_invariant());
	QUARK_ASSERT(s.size() > 0);

	const auto it = env->_values.find(s);
	if(it != env->_values.end()){
		return &it->second;
	}
	else if(env->_parent_env){
		return resolve_env_variable_deep(vm, env->_parent_env, s);
	}
	else{
		return nullptr;
	}
}

//	Warning: returns reference to the found value-entry -- this could be in any environment in the call stack.
std::pair<floyd::value_t, bool>* resolve_env_variable(const interpreter_t& vm, const std::string& s){
	QUARK_ASSERT(vm.check_invariant());
	QUARK_ASSERT(s.size() > 0);

	return resolve_env_variable_deep(vm, vm._call_stack.back(), s);
}

floyd::value_t find_global_symbol(const interpreter_t& vm, const string& s){
	const auto t = resolve_env_variable(vm, s);
	if(t == nullptr){
		QUARK_ASSERT(false);
		throw std::runtime_error("Undefined indentifier \"" + s + "\"!");
	}
	return t->first;
}



std::pair<interpreter_t, expression_t> evaluate_resolve_member_expression(const interpreter_t& vm, const expression_t::resolve_member_expr_t& expr){
	QUARK_ASSERT(vm.check_invariant());

	auto vm_acc = vm;
	const auto parent_expr = evaluate_expression(vm_acc, *expr._parent_address);
	vm_acc = parent_expr.first;
	QUARK_ASSERT(parent_expr.second.is_literal());
	QUARK_ASSERT(parent_expr.second.get_literal().is_struct());

	const auto struct_instance = parent_expr.second.get_literal().get_struct_value();

	int index = find_struct_member_index(struct_instance->_def, expr._member_name);
	QUARK_ASSERT(index != -1);

	const value_t value = struct_instance->_member_values[index];
	return { vm_acc, expression_t::make_literal(value)};
}

std::pair<interpreter_t, expression_t> evaluate_lookup_element_expression(const interpreter_t& vm, const expression_t::lookup_expr_t& expr){
	QUARK_ASSERT(vm.check_invariant());

	auto vm_acc = vm;
	const auto parent_expr = evaluate_expression(vm_acc, *expr._parent_address);
	vm_acc = parent_expr.first;

	QUARK_ASSERT(parent_expr.second.is_literal());

	const auto key_expr = evaluate_expression(vm_acc, *expr._lookup_key);
	vm_acc = key_expr.first;

	QUARK_ASSERT(key_expr.second.is_literal());

	const auto parent_value = parent_expr.second.get_literal();
	const auto key_value = key_expr.second.get_literal();

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
			return { vm_acc, expression_t::make_literal(value2)};
		}
	}
	else if(parent_value.is_json_value()){
		const auto parent_json_value = parent_value.get_json_value();
		if(parent_json_value.is_object()){
			if(key_value.is_string() == false){
				throw std::runtime_error("Lookup in json_value object by string-key only.");
			}
			else{
				const auto lookup_key = key_value.get_string_value();

				//	get_object_element() throws if key can't be found.
				const auto value = parent_json_value.get_object_element(lookup_key);
				const auto value2 = value_t::make_json_value(value);
				return { vm_acc, expression_t::make_literal(value2)};
			}
		}
		else if(parent_json_value.is_array()){
			if(key_value.is_int() == false){
				throw std::runtime_error("Lookup in json_value array by integer index only.");
			}
			else{
				const auto lookup_index = key_value.get_int_value();

				if(lookup_index < 0 || lookup_index >= parent_json_value.get_array_size()){
					throw std::runtime_error("Lookup in json_value array: out of bounds.");
				}
				else{
					const auto value = parent_json_value.get_array_n(lookup_index);
					const auto value2 = value_t::make_json_value(value);
					return { vm_acc, expression_t::make_literal(value2)};
				}
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
			return { vm_acc, expression_t::make_literal(value)};
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
			return { vm_acc, expression_t::make_literal(value)};
		}
	}
	else {
		throw std::runtime_error("Lookup using [] only works with strings, vectors, dicts and json_value.");
	}
}

std::pair<interpreter_t, expression_t> evaluate_variable_expression(const interpreter_t& vm, const expression_t::variable_expr_t& expr){
	QUARK_ASSERT(vm.check_invariant());

	auto vm_acc = vm;
	const auto value = resolve_env_variable(vm_acc, expr._variable);
	QUARK_ASSERT(value != nullptr);
	return {vm_acc, expression_t::make_literal(value->first)};
}


std::pair<interpreter_t, expression_t> evaluate_vector_definition_expression(const interpreter_t& vm, const expression_t::vector_definition_exprt_t& expr){
	QUARK_ASSERT(vm.check_invariant());

	auto vm_acc = vm;
	const std::vector<expression_t>& elements = expr._elements;
	const auto element_type = expr._element_type;

	//	An empty vector is encoded as a constant value, not a vector-definition-expression.
	QUARK_ASSERT(elements.empty() == false);

	std::vector<value_t> elements2;
	for(const auto m: elements){
		const auto element_expr = evaluate_expression(vm_acc, m);
		vm_acc = element_expr.first;
		QUARK_ASSERT(element_expr.second.is_literal());

		const auto element = element_expr.second.get_literal();
		elements2.push_back(element);
	}

	//	If we don't have an explicit element type, deduct it from first element.
	const auto element_type2 = element_type.is_null() ? elements2[0].get_type() : element_type;
#if DEBUG
	for(const auto m: elements2){
		QUARK_ASSERT(m.get_type() == element_type2);
	}
#endif
	return {vm_acc, expression_t::make_literal(value_t::make_vector_value(element_type2, elements2))};
}



std::pair<interpreter_t, expression_t> evaluate_dict_definition_expression(const interpreter_t& vm, const expression_t::dict_definition_exprt_t& expr){
	QUARK_ASSERT(vm.check_invariant());

	auto vm_acc = vm;

	const auto& elements = expr._elements;
	typeid_t value_type = expr._value_type;

	//	An empty dict is encoded as a constant value, not a dict-definition-expression.
	QUARK_ASSERT(elements.empty() == false);

	std::map<string, value_t> elements2;
	for(const auto m: elements){
		const auto element_expr = evaluate_expression(vm_acc, m.second);
		vm_acc = element_expr.first;

		QUARK_ASSERT(element_expr.second.is_literal());

		const auto element = element_expr.second.get_literal();

		const string key_string = m.first;
		elements2[key_string] = element;
	}
	QUARK_ASSERT(value_type.is_null() == false);

	return {vm_acc, expression_t::make_literal(value_t::make_dict_value(value_type, elements2))};
}

std::pair<interpreter_t, expression_t> evaluate_arithmetic_unary_minus_expression(const interpreter_t& vm, const expression_t::unary_minus_expr_t& expr){
	QUARK_ASSERT(vm.check_invariant());

	auto vm_acc = vm;
	const auto& expr2 = evaluate_expression(vm_acc, *expr._expr);
	vm_acc = expr2.first;

	QUARK_ASSERT(expr2.second.is_literal());

	const auto& c = expr2.second.get_literal();
	vm_acc = expr2.first;

	if(c.is_int()){
		return evaluate_expression(
			vm_acc,
			expression_t::make_simple_expression__2(expression_type::k_arithmetic_subtract__2, expression_t::make_literal_int(0), expr2.second, make_shared<typeid_t>(c.get_type()))
		);
	}
	else if(c.is_float()){
		return evaluate_expression(
			vm_acc,
			expression_t::make_simple_expression__2(expression_type::k_arithmetic_subtract__2, expression_t::make_literal_float(0.0f), expr2.second, make_shared<typeid_t>(c.get_type()))
		);
	}
	else{
		throw std::runtime_error("Unary minus won't work on expressions of type \"" + json_to_compact_string(typeid_to_ast_json(c.get_type())._value) + "\".");
	}
}


std::pair<interpreter_t, expression_t> evaluate_conditional_operator_expression(const interpreter_t& vm, const expression_t::conditional_expr_t& expr){
	QUARK_ASSERT(vm.check_invariant());

	auto vm_acc = vm;

	//	Special-case since it uses 3 expressions & uses shortcut evaluation.
	const auto cond_result = evaluate_expression(vm_acc, *expr._condition);
	vm_acc = cond_result.first;

	QUARK_ASSERT(cond_result.second.is_literal());
	QUARK_ASSERT(cond_result.second.get_literal().is_bool());

	const bool cond_flag = cond_result.second.get_literal().get_bool_value();

	//	!!! Only evaluate the CHOSEN expression. Not that importan since functions are pure.
	if(cond_flag){
		return evaluate_expression(vm_acc, *expr._a);
	}
	else{
		return evaluate_expression(vm_acc, *expr._b);
	}
}


std::pair<interpreter_t, expression_t> evaluate_comparison_expression(const interpreter_t& vm, expression_type op, const expression_t::simple_expr__2_t& simple2_expr){
	QUARK_ASSERT(vm.check_invariant());

	auto vm_acc = vm;

	//	First evaluate all inputs to our operation.
	const auto left_expr = evaluate_expression(vm_acc, *simple2_expr._left);
	vm_acc = left_expr.first;

	const auto right_expr = evaluate_expression(vm_acc, *simple2_expr._right);
	vm_acc = right_expr.first;

	QUARK_ASSERT(left_expr.second.is_literal());
	QUARK_ASSERT(right_expr.second.is_literal());

	//	Perform math operation on the two constants => new constant.
	const auto left_constant = left_expr.second.get_literal();
	const auto right_constant = right_expr.second.get_literal();
	QUARK_ASSERT(left_constant.get_type() == right_constant.get_type());

	//	Do generic functionallity, independant on type.
	if(op == expression_type::k_comparison_smaller_or_equal__2){
		long diff = value_t::compare_value_true_deep(left_constant, right_constant);
		return {vm_acc, expression_t::make_literal_bool(diff <= 0)};
	}
	else if(op == expression_type::k_comparison_smaller__2){
		long diff = value_t::compare_value_true_deep(left_constant, right_constant);
		return {vm_acc, expression_t::make_literal_bool(diff < 0)};
	}
	else if(op == expression_type::k_comparison_larger_or_equal__2){
		long diff = value_t::compare_value_true_deep(left_constant, right_constant);
		return {vm_acc, expression_t::make_literal_bool(diff >= 0)};
	}
	else if(op == expression_type::k_comparison_larger__2){
		long diff = value_t::compare_value_true_deep(left_constant, right_constant);
		return {vm_acc, expression_t::make_literal_bool(diff > 0)};
	}

	else if(op == expression_type::k_logical_equal__2){
		long diff = value_t::compare_value_true_deep(left_constant, right_constant);
		return {vm_acc, expression_t::make_literal_bool(diff == 0)};
	}
	else if(op == expression_type::k_logical_nonequal__2){
		long diff = value_t::compare_value_true_deep(left_constant, right_constant);
		return {vm_acc, expression_t::make_literal_bool(diff != 0)};
	}
	else{
		QUARK_ASSERT(false);
		throw std::exception();
	}
}

std::pair<interpreter_t, expression_t> evaluate_arithmetic_expression(const interpreter_t& vm, expression_type op, const expression_t::simple_expr__2_t& simple2_expr){
	QUARK_ASSERT(vm.check_invariant());

	auto vm_acc = vm;

	//	First evaluate all inputs to our operation.
	const auto left_expr = evaluate_expression(vm_acc, *simple2_expr._left);
	vm_acc = left_expr.first;

	const auto right_expr = evaluate_expression(vm_acc, *simple2_expr._right);
	vm_acc = right_expr.first;

	//	Both left and right are constants, replace the math_operation with a constant!
	QUARK_ASSERT(left_expr.second.is_literal() != false && right_expr.second.is_literal() != false);

	//	Perform math operation on the two constants => new constant.
	const auto left_constant = left_expr.second.get_literal();
	const auto right_constant = right_expr.second.get_literal();
	QUARK_ASSERT(left_constant.get_type() == right_constant.get_type());

	const auto type_mode = left_constant.get_type();

	//	bool
	if(type_mode.is_bool()){
		const bool left = left_constant.get_bool_value();
		const bool right = right_constant.get_bool_value();

		if(false
		|| op == expression_type::k_arithmetic_add__2
		|| op == expression_type::k_arithmetic_subtract__2
		|| op == expression_type::k_arithmetic_multiply__2
		|| op == expression_type::k_arithmetic_divide__2
		|| op == expression_type::k_arithmetic_remainder__2
		){
			QUARK_ASSERT(false);
			throw std::runtime_error("Artithmetics: bool not allowed.");
		}

		else if(op == expression_type::k_logical_and__2){
			return {vm_acc, expression_t::make_literal_bool(left && right)};
		}
		else if(op == expression_type::k_logical_or__2){
			return {vm_acc, expression_t::make_literal_bool(left || right)};
		}
		else{
			QUARK_ASSERT(false);
			throw std::exception();
		}
	}

	//	int
	else if(type_mode.is_int()){
		const int left = left_constant.get_int_value();
		const int right = right_constant.get_int_value();

		if(op == expression_type::k_arithmetic_add__2){
			return {vm_acc, expression_t::make_literal_int(left + right)};
		}
		else if(op == expression_type::k_arithmetic_subtract__2){
			return {vm_acc, expression_t::make_literal_int(left - right)};
		}
		else if(op == expression_type::k_arithmetic_multiply__2){
			return {vm_acc, expression_t::make_literal_int(left * right)};
		}
		else if(op == expression_type::k_arithmetic_divide__2){
			if(right == 0){
				throw std::runtime_error("EEE_DIVIDE_BY_ZERO");
			}
			return {vm_acc, expression_t::make_literal_int(left / right)};
		}
		else if(op == expression_type::k_arithmetic_remainder__2){
			if(right == 0){
				throw std::runtime_error("EEE_DIVIDE_BY_ZERO");
			}
			return {vm_acc, expression_t::make_literal_int(left % right)};
		}

		else if(op == expression_type::k_logical_and__2){
			return {vm_acc, expression_t::make_literal_bool((left != 0) && (right != 0))};
		}
		else if(op == expression_type::k_logical_or__2){
			return {vm_acc, expression_t::make_literal_bool((left != 0) || (right != 0))};
		}
		else{
			QUARK_ASSERT(false);
			throw std::exception();
		}
	}

	//	float
	else if(type_mode.is_float()){
		const float left = left_constant.get_float_value();
		const float right = right_constant.get_float_value();

		if(op == expression_type::k_arithmetic_add__2){
			return {vm_acc, expression_t::make_literal_float(left + right)};
		}
		else if(op == expression_type::k_arithmetic_subtract__2){
			return {vm_acc, expression_t::make_literal_float(left - right)};
		}
		else if(op == expression_type::k_arithmetic_multiply__2){
			return {vm_acc, expression_t::make_literal_float(left * right)};
		}
		else if(op == expression_type::k_arithmetic_divide__2){
			if(right == 0.0f){
				throw std::runtime_error("EEE_DIVIDE_BY_ZERO");
			}
			return {vm_acc, expression_t::make_literal_float(left / right)};
		}
		else if(op == expression_type::k_arithmetic_remainder__2){
			throw std::runtime_error("Modulo operation on float not allowed.");
		}


		else if(op == expression_type::k_logical_and__2){
			return {vm_acc, expression_t::make_literal_bool((left != 0.0f) && (right != 0.0f))};
		}
		else if(op == expression_type::k_logical_or__2){
			return {vm_acc, expression_t::make_literal_bool((left != 0.0f) || (right != 0.0f))};
		}
		else{
			QUARK_ASSERT(false);
			throw std::exception();
		}
	}

	//	string
	else if(type_mode.is_string()){
		const auto left = left_constant.get_string_value();
		const auto right = right_constant.get_string_value();

		if(op == expression_type::k_arithmetic_add__2){
			return {vm_acc, expression_t::make_literal_string(left + right)};
		}

		else if(op == expression_type::k_arithmetic_subtract__2){
			QUARK_ASSERT(false);
			throw std::runtime_error("Operation not allowed on string.");
		}
		else if(op == expression_type::k_arithmetic_multiply__2){
			QUARK_ASSERT(false);
			throw std::runtime_error("Operation not allowed on string.");
		}
		else if(op == expression_type::k_arithmetic_divide__2){
			QUARK_ASSERT(false);
			throw std::runtime_error("Operation not allowed on string.");
		}
		else if(op == expression_type::k_arithmetic_remainder__2){
			QUARK_ASSERT(false);
			throw std::runtime_error("Operation not allowed on string.");
		}


		else if(op == expression_type::k_logical_and__2){
			QUARK_ASSERT(false);
			throw std::runtime_error("Operation not allowed on string.");
		}
		else if(op == expression_type::k_logical_or__2){
			QUARK_ASSERT(false);
			throw std::runtime_error("Operation not allowed on string.");
		}
		else{
			QUARK_ASSERT(false);
			throw std::exception();
		}
	}

	//	struct
	else if(type_mode.is_struct()){
		const auto left = left_constant.get_struct_value();
		const auto right = right_constant.get_struct_value();

		//	Structs mue be exactly the same type to match.
		if(left_constant.get_typeid_value() == right_constant.get_typeid_value()){
			QUARK_ASSERT(false);
			throw std::runtime_error("Struct type mismatch.");
		}

		if(op == expression_type::k_arithmetic_add__2){
			QUARK_ASSERT(false);
			throw std::runtime_error("Operation not allowed on struct.");
		}
		else if(op == expression_type::k_arithmetic_subtract__2){
			QUARK_ASSERT(false);
			throw std::runtime_error("Operation not allowed on struct.");
		}
		else if(op == expression_type::k_arithmetic_multiply__2){
			QUARK_ASSERT(false);
			throw std::runtime_error("Operation not allowed on struct.");
		}
		else if(op == expression_type::k_arithmetic_divide__2){
			QUARK_ASSERT(false);
			throw std::runtime_error("Operation not allowed on struct.");
		}
		else if(op == expression_type::k_arithmetic_remainder__2){
			QUARK_ASSERT(false);
			throw std::runtime_error("Operation not allowed on struct.");
		}

		else if(op == expression_type::k_logical_and__2){
			QUARK_ASSERT(false);
			throw std::runtime_error("Operation not allowed on struct.");
		}
		else if(op == expression_type::k_logical_or__2){
			QUARK_ASSERT(false);
			throw std::runtime_error("Operation not allowed on struct.");
		}
		else{
			QUARK_ASSERT(false);
			throw std::exception();
		}
	}

	else if(type_mode.is_vector()){
		//	Improves vectors before using them.
		const auto element_type = left_constant.get_type().get_vector_element_type();

		if(!(left_constant.get_type() == right_constant.get_type())){
			QUARK_ASSERT(false);
			throw std::runtime_error("Vector types don't match.");
		}
		else{
			if(op == expression_type::k_arithmetic_add__2){
				auto elements2 = left_constant.get_vector_value()->_elements;
				elements2.insert(elements2.end(), right_constant.get_vector_value()->_elements.begin(), right_constant.get_vector_value()->_elements.end());

				const auto value2 = value_t::make_vector_value(element_type, elements2);
				return {vm_acc, expression_t::make_literal(value2)};
			}

			else if(op == expression_type::k_arithmetic_subtract__2){
			QUARK_ASSERT(false);
				throw std::runtime_error("Operation not allowed on vectors.");
			}
			else if(op == expression_type::k_arithmetic_multiply__2){
			QUARK_ASSERT(false);
				throw std::runtime_error("Operation not allowed on vectors.");
			}
			else if(op == expression_type::k_arithmetic_divide__2){
			QUARK_ASSERT(false);
				throw std::runtime_error("Operation not allowed on vectors.");
			}
			else if(op == expression_type::k_arithmetic_remainder__2){
			QUARK_ASSERT(false);
				throw std::runtime_error("Operation not allowed on vectors.");
			}


			else if(op == expression_type::k_logical_and__2){
			QUARK_ASSERT(false);
				throw std::runtime_error("Operation not allowed on vectors.");
			}
			else if(op == expression_type::k_logical_or__2){
			QUARK_ASSERT(false);
				throw std::runtime_error("Operation not allowed on vectors.");
			}
			else{
				QUARK_ASSERT(false);
				throw std::exception();
			}
		}
	}
	else if(type_mode.is_function()){
		QUARK_ASSERT(false);
		throw std::runtime_error("Cannot perform operations on two function values.");
	}
	else{
		QUARK_ASSERT(false);
		throw std::runtime_error("Arithmetics failed.");
	}
}


std::pair<interpreter_t, expression_t> evaluate_expression(const interpreter_t& vm, const expression_t& e){
	QUARK_ASSERT(vm.check_invariant());
	QUARK_ASSERT(e.check_invariant());

	const auto op = e.get_operation();

	if(op == expression_type::k_literal){
		return { vm, e };
	}
	else if(op == expression_type::k_resolve_member){
		return evaluate_resolve_member_expression(vm, *e.get_resolve_member());
	}
	else if(op == expression_type::k_lookup_element){
		return evaluate_lookup_element_expression(vm, *e.get_lookup());
	}
	else if(op == expression_type::k_variable){
		return evaluate_variable_expression(vm, *e.get_variable());
	}

	else if(op == expression_type::k_call){
		return evaluate_call_expression(vm, e);
	}

	else if(op == expression_type::k_define_function){
		const auto expr = e.get_function_definition();
		return {vm, expression_t::make_literal(value_t::make_function_value(expr->_def))};
	}

	else if(op == expression_type::k_vector_definition){
		return evaluate_vector_definition_expression(vm, *e.get_vector_definition());
	}

	else if(op == expression_type::k_dict_definition){
		return evaluate_dict_definition_expression(vm, *e.get_dict_definition());
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
		throw std::exception();
	}
}


std::pair<interpreter_t, statement_result_t> call_function(const interpreter_t& vm, const floyd::value_t& f, const vector<value_t>& args){
	QUARK_ASSERT(vm.check_invariant());
	QUARK_ASSERT(f.check_invariant());
	for(const auto i: args){ QUARK_ASSERT(i.check_invariant()); };

	auto vm_acc = vm;

	if(f.is_function() == false){
		QUARK_ASSERT(false);
		throw std::runtime_error("Cannot call non-function.");
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
		auto parent_env = vm_acc._call_stack[0];
		auto new_environment = environment_t::make_environment(vm_acc, parent_env);

		//	Copy input arguments to the function scope.
		for(int i = 0 ; i < function_def._args.size() ; i++){
			const auto& arg_name = function_def._args[i]._name;
			const auto& arg_value = args[i];

			//	Function arguments are immutable while executing the function body.
			new_environment->_values[arg_name] = std::pair<value_t, bool>(arg_value, false);
		}
		vm_acc._call_stack.push_back(new_environment);
		const auto r = execute_statements(vm_acc, function_def._statements);
		vm_acc = r.first;
		vm_acc._call_stack.pop_back();

//??? move this check to pass3.
		if(r.second._type != statement_result_t::k_return_unwind){
			throw std::runtime_error("Function missing return statement");
		}
//??? move this check to pass3.
		else if(r.second._output.get_type().is_struct() == false && r.second._output.get_type() != return_type){
			throw std::runtime_error("Function return type wrong.");
		}
		else{
			return {vm_acc, r.second };
		}
	}
}

bool all_literals(const vector<expression_t>& e){
	for(const auto& i: e){
		if(!i.is_literal()){
			return false;
		}
	}
	return true;
}

//??? all evalute_* should return a value, not an expression!

//	May return a simplified expression instead of a value literal..
std::pair<interpreter_t, expression_t> evaluate_call_expression(const interpreter_t& vm, const expression_t& e){
	QUARK_ASSERT(vm.check_invariant());

	const auto expr = *e.get_call();

	auto vm_acc = vm;

	//	Simplify each input expression: expression[0]: which function to call, expression[1]: first argument if any.
	const auto function = evaluate_expression(vm_acc, *expr._callee);
	vm_acc = function.first;
	vector<expression_t> args2;
	for(int i = 0 ; i < expr._args.size() ; i++){
		const auto t = evaluate_expression(vm_acc, expr._args[i]);
		vm_acc = t.first;
		args2.push_back(t.second);
	}

	QUARK_ASSERT(function.second.is_literal());
	QUARK_ASSERT(all_literals(args2));

	//	Convert to values.
	const auto& function_value = function.second.get_literal();
	vector<value_t> arg_values;
	for(int i = 0 ; i < args2.size() ; i++){
		const auto& v = args2[i].get_literal();
		arg_values.push_back(v);
	}

	//	Call function-value.
	if(function_value.is_function()){
		const auto& result = call_function(vm_acc, function_value, arg_values);
		QUARK_ASSERT(result.second._type == statement_result_t::k_return_unwind);
		vm_acc = result.first;
		return { vm_acc, expression_t::make_literal(result.second._output)};
	}

	//	Attempting to call a TYPE? Then this may be a constructor call.
	else if(function_value.is_typeid()){
		const auto result = construct_value_from_typeid(vm_acc, function_value.get_typeid_value(), arg_values);
		vm_acc = result.first;
		return { vm_acc, expression_t::make_literal(result.second)};
	}
	else{
		QUARK_ASSERT(false);
		throw std::exception();
	}
}



json_t interpreter_to_json(const interpreter_t& vm){
	vector<json_t> callstack;
	for(const auto& e: vm._call_stack){
		std::map<string, json_t> values;
		for(const auto&v: e->_values){
		//??? INlcude mutable-flag?
			const auto a = value_and_type_to_ast_json(v.second.first);
			const auto b = make_array_skip_nulls({
				a._value.get_array_n(0),
				a._value.get_array_n(1),
				v.second.second ? json_t("mutable") : json_t()
			});
			values[v.first] = b;
		}

		const auto& env = json_t::make_object({
//			{ "parent_env", e->_parent_env ? e->_parent_env->_object_id : json_t() },
//			{ "object_id", json_t(double(e->_object_id)) },
			{ "values", values }
		});
		callstack.push_back(env);
	}

	return json_t::make_object({
		{ "ast", ast_to_json(*vm._ast)._value },
		{ "callstack", json_t::make_array(callstack) }
	});
}



void test__evaluate_expression(const expression_t& e, const expression_t& expected){
	const ast_t ast;
	const interpreter_t interpreter(ast);
	const auto e3 = evaluate_expression(interpreter, e);

	ut_compare_jsons(expression_to_json(e3.second)._value, expression_to_json(expected)._value);
}


QUARK_UNIT_TESTQ("evaluate_expression()", "literal 1234 == 1234") {
	test__evaluate_expression(
		expression_t::make_literal_int(1234),
		expression_t::make_literal_int(1234)
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
		expression_t::make_literal_int(3)
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
		expression_t::make_literal_int(12)
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
		expression_t::make_literal_int(60)
	);
}




//////////////////////////		environment_t



std::shared_ptr<environment_t> environment_t::make_environment(
	const interpreter_t& vm,
	std::shared_ptr<environment_t>& parent_env
)
{
	QUARK_ASSERT(vm.check_invariant());

	auto f = environment_t{ parent_env, {} };
	return make_shared<environment_t>(f);
}

bool environment_t::check_invariant() const {
	return true;
}





//	NOTICE: We do function overloading for the host functions: you can call them with any *type* of arguments and it gives any return type.
std::pair<interpreter_t, statement_result_t> call_host_function(const interpreter_t& vm, int function_id, const std::vector<floyd::value_t> args){
//	const int index = function_id - 1000;
//	QUARK_ASSERT(index >= 0 /*&& index < k_host_functions.size()*/);

	const auto& host_function = vm._host_functions.at(function_id);

/*
	//	arity
	if(args.size() != host_function._function_type.get_function_args().size()){
		throw std::runtime_error("Wrong number of arguments to host function.");
	}
*/

	const auto result = (host_function)(vm, args);
	return {
		result.first,
		statement_result_t::make_return_unwind(result.second)
	};
}

interpreter_t::interpreter_t(const ast_t& ast){
	QUARK_ASSERT(ast.check_invariant());

	_ast = make_shared<ast_t>(ast);

	//	Make the top-level environment = global scope.
	shared_ptr<environment_t> empty_env;
	auto global_env = environment_t::make_environment(*this, empty_env);

	const auto host_functions = get_host_functions();
	for(auto hf_kv: host_functions){
		const auto& function_name = hf_kv.second._name;
		const auto function_value = make_host_function_value(hf_kv.second._signature);
		const auto value_entry = std::pair<value_t, bool>{ function_value, false };
		global_env->_values.insert({ function_name, value_entry });

		const auto function_id = hf_kv.second._signature._function_id;
		const auto function_ptr = hf_kv.second._f;
		_host_functions.insert({ function_id, function_ptr });
	}

	global_env->_values[keyword_t::k_null] = std::pair<value_t, bool>{value_t::make_null(), false };
	global_env->_values[keyword_t::k_bool] = std::pair<value_t, bool>{value_t::make_typeid_value(typeid_t::make_bool()), false };
	global_env->_values[keyword_t::k_int] = std::pair<value_t, bool>{value_t::make_typeid_value(typeid_t::make_int()), false };
	global_env->_values[keyword_t::k_float] = std::pair<value_t, bool>{value_t::make_typeid_value(typeid_t::make_float()), false };
	global_env->_values[keyword_t::k_string] = std::pair<value_t, bool>{value_t::make_typeid_value(typeid_t::make_string()), false };
	global_env->_values[keyword_t::k_typeid] = std::pair<value_t, bool>{value_t::make_typeid_value(typeid_t::make_typeid()), false };
	global_env->_values[keyword_t::k_json_value] = std::pair<value_t, bool>{value_t::make_typeid_value(typeid_t::make_json_value()), false };

	global_env->_values[keyword_t::k_json_object] = std::pair<value_t, bool>{value_t::make_int(1), false };
	global_env->_values[keyword_t::k_json_array] = std::pair<value_t, bool>{value_t::make_int(2), false };
	global_env->_values[keyword_t::k_json_string] = std::pair<value_t, bool>{value_t::make_int(3), false };
	global_env->_values[keyword_t::k_json_number] = std::pair<value_t, bool>{value_t::make_int(4), false };
	global_env->_values[keyword_t::k_json_true] = std::pair<value_t, bool>{value_t::make_int(5), false };
	global_env->_values[keyword_t::k_json_false] = std::pair<value_t, bool>{value_t::make_int(6), false };
	global_env->_values[keyword_t::k_json_null] = std::pair<value_t, bool>{value_t::make_int(7), false };

	_call_stack.push_back(global_env);

	_start_time = std::chrono::high_resolution_clock::now();

	//	Run static intialization (basically run global statements before calling main()).
	const auto r = execute_statements(*this, _ast->_statements);

	_call_stack[0]->_values = r.first._call_stack[0]->_values;
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

	const auto main_function = resolve_env_variable(vm, "main");
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

	const auto main_func = resolve_env_variable(vm, "main");
	if(main_func != nullptr){
		const auto r = call_function(vm, main_func->first, args);
		return { r.first, r.second };
	}
	else{
		return { vm, statement_result_t::make_no_output() };
	}
}

value_t get_global(const interpreter_t& vm, const std::string& name){
	const auto entry = vm._call_stack[0]->_values[name];
	return entry.first;
}



}	//	floyd

