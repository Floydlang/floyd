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

namespace floyd {

using std::vector;
using std::string;
using std::pair;
using std::shared_ptr;
using std::unique_ptr;
using std::make_shared;



std::pair<interpreter_t, expression_t> evaluate_call_expression(const interpreter_t& vm, const expression_t::function_call_expr_t& e);
std::pair<floyd::value_t, bool>* resolve_env_variable(const interpreter_t& vm, const std::string& s);


	//??? add conversions.
	//??? This is builing block for promoting values / casting.
	//	We know which type we need. If the value has not type, retype it.
	value_t improve_value_type(const value_t& value0, const typeid_t& expected_type){
		QUARK_ASSERT(value0.check_invariant());
		QUARK_ASSERT(expected_type.check_invariant());

		if(expected_type.is_null()){
			return value0;
		}
		else if(expected_type.is_bool()){
			if(value0.is_json_value() && value0.get_json_value().is_true()){
				return value_t::make_bool(true);
			}
			if(value0.is_json_value() && value0.get_json_value().is_false()){
				return value_t::make_bool(false);
			}
			else{
				return value0;
			}
		}
		else if(expected_type.is_float()){
			if(value0.is_json_value() && value0.get_json_value().is_number()){
				return value_t::make_float((float)value0.get_json_value().get_number());
			}
			else{
				return value0;
			}
		}
		else if(expected_type.is_string()){
			if(value0.is_json_value() && value0.get_json_value().is_string()){
				return value_t::make_string(value0.get_json_value().get_string());
			}
			else{
				return value0;
			}
		}
		else if(expected_type.is_json_value()){
			const auto v2 = value_to_ast_json(value0);
			return value_t::make_json_value(v2._value);
		}
		else{
			const auto value = value0;

			if(value.is_vector()){
				const auto v = value.get_vector_value();

				//	When [] appears in an expression we know it's an empty vector but not which type. It can be used as any vector type.
				if(v->_element_type.is_null() && v->_elements.empty()){
					QUARK_ASSERT(expected_type.is_vector());
					return value_t::make_vector_value(expected_type.get_vector_element_type(), value.get_vector_value()->_elements);
				}
				else{
					return value;
				}
			}
			else if(value.is_dict()){
				const auto v = value.get_dict_value();

				//	When [:] appears in an expression we know it's an empty dict but not which type. It can be used as any dict type.
				if(v->_value_type.is_null() && v->_elements.empty()){
					QUARK_ASSERT(expected_type.is_dict());
					return value_t::make_dict_value(expected_type.get_dict_value_type(), {});
				}
				else{
					return value;
				}
			}
			else{
				return value;
			}
		}
	}


std::pair<interpreter_t, value_t> construct_struct(const interpreter_t& vm, const typeid_t& struct_type, const vector<value_t>& values){
	QUARK_ASSERT(struct_type.get_base_type() == base_type::k_struct);
	QUARK_SCOPED_TRACE("construct_struct()");
	QUARK_TRACE("struct_type: " + typeid_to_compact_string(struct_type));

	const string struct_type_name = "unnamed";
	const auto& def = struct_type.get_struct();
	if(values.size() != def._members.size()){
		throw std::runtime_error(
			 string() + "Calling constructor for \"" + struct_type_name + "\" with " + std::to_string(values.size()) + " arguments, " + std::to_string(def._members.size()) + " + required."
		);
	}
	for(int i = 0 ; i < def._members.size() ; i++){
		const auto v = values[i];
		const auto a = def._members[i];

		QUARK_ASSERT(v.check_invariant());
		QUARK_ASSERT(v.get_type().get_base_type() != base_type::k_unresolved_type_identifier);

		if(v.get_type() != a._type){
//??? wont work until we sorted out struct-types.
			throw std::runtime_error("Constructor needs an arguement exactly matching type and order of struct members");
		}
	}

	const auto instance = value_t::make_struct_value(struct_type, values);
	QUARK_TRACE(to_compact_string2(instance));

	return std::pair<interpreter_t, value_t>(vm, instance);
}


std::pair<interpreter_t, value_t> construct_value_from_typeid(const interpreter_t& vm, const typeid_t& type, const vector<value_t>& arg_values){

	if(type.is_bool() || type.is_int() || type.is_float() || type.is_string() || type.is_json_value() || type.is_typeid()){
		if(arg_values.size() != 1){
			throw std::runtime_error("Constructor requires 1 argument");
		}
		const auto value = improve_value_type(arg_values[0], type);
		if(value.get_type() != type){
			throw std::runtime_error("Constructor requires 1 argument");
		}
		return {vm, value };
	}
	else if(type.is_struct()){
		//	Constructor.
		const auto result = construct_struct(vm, type, arg_values);
		return { result.first, result.second };
	}
	else if(type.is_vector()){
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

typeid_t cleanup_vector_constructor_type(const typeid_t& type){
	if(type.is_vector()){
		const auto element_type = type.get_vector_element_type();
		if(element_type.is_vector()){
			const auto c = cleanup_vector_constructor_type(element_type);
			return typeid_t::make_vector(c);
		}
		else{
			assert(element_type.is_typeid());
			return type;
//???			return typeid_t::make_vector(element_type.get_typeid_typeid());
		}
	}
	else if(type.is_dict()){
		return type;
	}
	else{
		return type;
	}
}



//??? encode type.
//??? encode version number?
value_t flatten_to_json(const value_t& value){
	const auto j = value_to_ast_json(value);
	value_t json_value = value_t::make_json_value(j._value);
	return json_value;
}

//??? encode structs using key-value instead of array?
//??? Extend to support all Floyd types, including structs!
value_t unflatten_json_to_specific_type(const json_t& v, const typeid_t& target_type){
	QUARK_ASSERT(v.check_invariant());

	if(target_type.is_null()){
		throw std::runtime_error("Invalid json schema, found null - unsupported by Floyd.");
	}
	else if(target_type.is_bool()){
		if(v.is_true()){
			return value_t::make_bool(true);
		}
		else if(v.is_false()){
			return value_t::make_bool(false);
		}
		else{
			throw std::runtime_error("Invalid json schema, expected true or false.");
		}
	}
	else if(target_type.is_int()){
		if(v.is_number()){
			return value_t::make_int((int)v.get_number());
		}
		else{
			throw std::runtime_error("Invalid json schema, expected number.");
		}
	}
	else if(target_type.is_float()){
		if(v.is_number()){
			return value_t::make_float((float)v.get_number());
		}
		else{
			throw std::runtime_error("Invalid json schema, expected number.");
		}
	}
	else if(target_type.is_string()){
		if(v.is_string()){
			return value_t::make_string(v.get_string());
		}
		else{
			throw std::runtime_error("Invalid json schema, expected string.");
		}
	}
	else if(target_type.is_json_value()){
		return value_t::make_json_value(v);
	}
	else if(target_type.is_typeid()){
		const auto typeid_value = typeid_from_ast_json(ast_json_t({v}));
		return value_t::make_typeid_value(typeid_value);
	}
	else if(target_type.is_struct()){
		if(v.is_object()){
			const auto struct_def = target_type.get_struct();
			vector<value_t> members2;
			for(const auto member: struct_def._members){
				const auto member_value0 = v.get_object_element(member._name);
				const auto member_value1 = unflatten_json_to_specific_type(member_value0, member._type);
				members2.push_back(member_value1);
			}
			const auto result = value_t::make_struct_value(target_type, members2);
			return result;
		}
		else{
			throw std::runtime_error("Invalid json schema for Floyd struct, expected JSON object.");
		}
	}
	else if(target_type.is_vector()){
		if(v.is_array()){
			const auto target_element_type = target_type.get_vector_element_type();
			vector<value_t> elements2;
			for(int i = 0 ; i < v.get_array_size() ; i++){
				const auto member_value0 = v.get_array_n(i);
				const auto member_value1 = unflatten_json_to_specific_type(member_value0, target_element_type);
				elements2.push_back(member_value1);
			}
			const auto result = value_t::make_vector_value(target_element_type, elements2);
			return result;
		}
		else{
			throw std::runtime_error("Invalid json schema for Floyd vector, expected JSON array.");
		}
	}
	else if(target_type.is_dict()){
		if(v.is_object()){
			const auto value_type = target_type.get_dict_value_type();
			const auto source_obj = v.get_object();
			std::map<std::string, value_t> obj2;
			for(const auto member: source_obj){
				const auto member_name = member.first;
				const auto member_value0 = member.second;
				const auto member_value1 = unflatten_json_to_specific_type(member_value0, value_type);
				obj2[member_name] = member_value1;
			}
			const auto result = value_t::make_dict_value(value_type, obj2);
			return result;
		}
		else{
			throw std::runtime_error("Invalid json schema, expected JSON object.");
		}
	}
	else if(target_type.is_function()){
		throw std::runtime_error("Invalid json schema, cannot unflatten functions.");
	}
	else if(target_type.is_unresolved_type_identifier()){
		QUARK_ASSERT(false);
		throw std::exception();
//		throw std::runtime_error("Invalid json schema, cannot unflatten functions.");
	}
	else{
		QUARK_ASSERT(false);
		throw std::exception();
	}

/*
	if(v.is_object()){
		const auto obj = v.get_object();
		std::map<string, value_t> obj2;
		for(const auto e: obj){
			const auto key = e.first;
			const auto value = e.second;
			const auto value2 = value_t::make_json_value(value);	//??? value_from_ast_json() but Floyd vector are homogenous, json arrays are not.
			obj2[key] = value2;
		}
		return value_t::make_dict_value(typeid_t::make_json_value(), obj2);
	}
	else if(v.is_array()){
		const auto elements = v.get_array();
		std::vector<value_t> elements2;
		for(const auto e: elements){
			const auto e2 = value_t::make_json_value(e);
			elements2.push_back(e2);
		}
		return value_t::make_vector_value(typeid_t::make_json_value(), elements2);	//??? value_from_ast_json() but Floyd vector are homogenous, json arrays are not.
	}
	else if(v.is_string()){
		return value_t::make_string(v.get_string());
	}
	else if(v.is_number()){
		return value_t::make_float(static_cast<float>(v.get_number()));
	}
	else if(v.is_true()){
		return value_t::make_bool(true);
	}
	else if(v.is_false()){
		return value_t::make_bool(false);
	}
	else if(v.is_null()){
		return value_t::make_null();
	}
	else{
		QUARK_ASSERT(false);
	}
*/

}


namespace {

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


std::pair<interpreter_t, statement_result_t> execute_bind_statement(const interpreter_t& vm, const bind_or_assign_statement_t& statement){
	QUARK_ASSERT(vm.check_invariant());

	auto vm_acc = vm;
	const auto name = statement._new_variable_name;

	const auto result = evaluate_expression(vm_acc, statement._expression);
	vm_acc = result.first;

	const auto result_value = result.second;
	if(result_value.is_literal() == false){
		throw std::runtime_error("Cannot evaluate expression.");
	}
	else{
		const auto bind_statement_type = statement._bindtype;
		const auto bind_statement_mutable_tag_flag = statement._bind_as_mutable_tag;

		//	If we have a type or we have the mutable-flag, then this statement is a bind.
		bool is_bind = bind_statement_type.is_null() == false || bind_statement_mutable_tag_flag;

		//	Bind.
		//	int a = 10
		//	mutable a = 10
		//	mutable = 10
		if(is_bind){
			const auto retyped_value = improve_value_type(result_value.get_literal(), bind_statement_type);
			const auto value_exists_in_env = vm_acc._call_stack.back()->_values.count(name) > 0;

			if(value_exists_in_env){
				throw std::runtime_error("Local identifier already exists.");
			}

			//	Deduced bind type -- use new value's type.
			if(bind_statement_type.is_null()){
				if(retyped_value.get_type() == typeid_t::make_vector(typeid_t::make_null())){
					throw std::runtime_error("Cannot deduce vector type.");
				}
				else if(retyped_value.get_type() == typeid_t::make_dict(typeid_t::make_null())){
					throw std::runtime_error("Cannot deduce dictionary type.");
				}
				else{
					vm_acc._call_stack.back()->_values[name] = std::pair<value_t, bool>(retyped_value, bind_statement_mutable_tag_flag);
				}
			}

			//	Explicit bind-type -- make sure source + dest types match.
			else{
				if(bind_statement_type != retyped_value.get_type()){
					throw std::runtime_error("Types not compatible in bind.");
				}
				vm_acc._call_stack.back()->_values[name] = std::pair<value_t, bool>(retyped_value, bind_statement_mutable_tag_flag);
			}
		}

		//	Assign
		//	a = 10
		else{
			const auto existing_value_deep_ptr = resolve_env_variable(vm_acc, name);
			const bool existing_variable_is_mutable = existing_value_deep_ptr && existing_value_deep_ptr->second;

			//	Mutate!
			if(existing_value_deep_ptr){
				if(existing_variable_is_mutable){
					const auto existing_value = existing_value_deep_ptr->first;
					const auto new_value = result_value.get_literal();

					//	Let both existing & new values influence eachother to the exact type of the new variable.
					//	Existing could be a [null]=[], or could new_value
					const auto new_value2 = improve_value_type(new_value, existing_value.get_type());
					const auto existing_value2 = improve_value_type(existing_value, new_value2.get_type());

					if(existing_value2.get_type() != new_value2.get_type()){
						throw std::runtime_error("Types not compatible in bind.");
					}
					*existing_value_deep_ptr = std::pair<value_t, bool>(new_value2, existing_variable_is_mutable);
				}
				else{
					throw std::runtime_error("Cannot assign to immutable identifier.");
				}
			}

			//	Deduce type and bind it -- to local env.
			else{
				const auto new_value = result_value.get_literal();

				//	Can we deduce type from the rhs value?
				if(new_value.get_type() == typeid_t::make_vector(typeid_t::make_null())){
					throw std::runtime_error("Cannot deduce vector type.");
				}
				else if(new_value.get_type() == typeid_t::make_dict(typeid_t::make_null())){
					throw std::runtime_error("Cannot deduce dictionary type.");
				}
				else{
					vm_acc._call_stack.back()->_values[name] = std::pair<value_t, bool>(new_value, false);
				}
			}
		}
	}
	return { vm_acc, statement_result_t::make_no_output() };
}

std::pair<interpreter_t, statement_result_t> execute_return_statement(const interpreter_t& vm, const return_statement_t& statement){
	QUARK_ASSERT(vm.check_invariant());

	auto vm_acc = vm;
	const auto expr = statement._expression;
	const auto result = evaluate_expression(vm_acc, expr);
	vm_acc = result.first;

	const auto result_value = result.second;

	if(!result_value.is_literal()){
		throw std::runtime_error("undefined");
	}

	//	Check that return value's type matches function's return type. Cannot be done here since we don't know who called us.
	//	Instead calling code must check.
	return {
		vm_acc,
		statement_result_t::make_return_unwind(result_value.get_literal())
	};
}

//??? make structs unique even though they share layout and name. USer unique-ID-generator?
std::pair<interpreter_t, statement_result_t> execute_def_struct_statement(const interpreter_t& vm, const define_struct_statement_t& statement){
	QUARK_ASSERT(vm.check_invariant());

	auto vm_acc = vm;
	const auto struct_name = statement._name;
	if(vm_acc._call_stack.back()->_values.count(struct_name) > 0){
		throw std::runtime_error("Name already used.");
	}

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

std::pair<interpreter_t, statement_result_t> execute_ifelse_statement(const interpreter_t& vm, const ifelse_statement_t& statement){
	QUARK_ASSERT(vm.check_invariant());

	auto vm_acc = vm;

	const auto condition_result = evaluate_expression(vm_acc, statement._condition);
	vm_acc = condition_result.first;
	const auto condition_result_value = condition_result.second;
	if(!condition_result_value.is_literal() || !condition_result_value.get_literal().is_bool()){
		throw std::runtime_error("Boolean condition required.");
	}

	bool r = condition_result_value.get_literal().get_bool_value();
	if(r){
		return execute_statements_in_env(vm_acc, statement._then_statements, {});
	}
	else{
		return execute_statements_in_env(vm_acc, statement._else_statements, {});
	}
}

std::pair<interpreter_t, statement_result_t> execute_for_statement(const interpreter_t& vm, const for_statement_t& statement){
	QUARK_ASSERT(vm.check_invariant());

	auto vm_acc = vm;

	const auto start_value0 = evaluate_expression(vm_acc, statement._start_expression);
	vm_acc = start_value0.first;
	const auto start_value = start_value0.second.get_literal().get_int_value();

	const auto end_value0 = evaluate_expression(vm_acc, statement._end_expression);
	vm_acc = end_value0.first;
	const auto end_value = end_value0.second.get_literal().get_int_value();

	for(int x = start_value ; x <= end_value ; x++){
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

std::pair<interpreter_t, statement_result_t> execute_while_statement(const interpreter_t& vm, const while_statement_t& statement){
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

std::pair<interpreter_t, statement_result_t> execute_expression_statement(const interpreter_t& vm, const expression_statement_t& statement){
	QUARK_ASSERT(vm.check_invariant());

	auto vm_acc = vm;
	const auto result = evaluate_expression(vm_acc, statement._expression);
	vm_acc = result.first;
	const auto result_value = result.second.get_literal();
	return {
		vm_acc,
		statement_result_t::make_passive_expression_output(result_value)
	};
}


//	Output is the RETURN VALUE of the executed statement, if any.
std::pair<interpreter_t, statement_result_t> execute_statement(const interpreter_t& vm, const statement_t& statement){
	QUARK_ASSERT(vm.check_invariant());
	QUARK_ASSERT(statement.check_invariant());

	if(statement._bind_or_assign){
		return execute_bind_statement(vm, *statement._bind_or_assign);
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



}	//	unnamed


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
		throw std::runtime_error("Undefined indentifier \"" + s + "\"!");
	}
	return t->first;
}





std::pair<interpreter_t, expression_t> evaluate_resolve_member_expression(const interpreter_t& vm, const expression_t::resolve_member_expr_t& expr){
	QUARK_ASSERT(vm.check_invariant());

	auto vm_acc = vm;
	const auto parent_expr = evaluate_expression(vm_acc, *expr._parent_address);
	vm_acc = parent_expr.first;
	if(parent_expr.second.is_literal() && parent_expr.second.get_literal().is_struct()){
		const auto struct_instance = parent_expr.second.get_literal().get_struct_value();

		int index = find_struct_member_index(struct_instance->_def, expr._member_name);
		if(index == -1){
			throw std::runtime_error("Unknown struct member \"" + expr._member_name + "\".");
		}
		const value_t value = struct_instance->_member_values[index];
		return { vm_acc, expression_t::make_literal(value)};
	}
	else{
		throw std::runtime_error("Resolve struct member failed.");
	}
}

std::pair<interpreter_t, expression_t> evaluate_lookup_element_expression(const interpreter_t& vm, const expression_t::lookup_expr_t& expr){
	QUARK_ASSERT(vm.check_invariant());

	auto vm_acc = vm;
	const auto parent_expr = evaluate_expression(vm_acc, *expr._parent_address);
	vm_acc = parent_expr.first;

	if(parent_expr.second.is_literal() == false){
		throw std::runtime_error("Cannot compute lookup parent.");
	}
	else{
		const auto key_expr = evaluate_expression(vm_acc, *expr._lookup_key);
		vm_acc = key_expr.first;

		if(key_expr.second.is_literal() == false){
			throw std::runtime_error("Cannot compute lookup key.");
		}//??? add debug code to validate all collection values, struct members - are the correct type.
		else{
			const auto parent_value = parent_expr.second.get_literal();
			const auto key_value = key_expr.second.get_literal();

			if(parent_value.is_string()){
				if(key_value.is_int() == false){
					throw std::runtime_error("Lookup in string by index-only.");
				}
				else{
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
				if(key_value.is_int() == false){
					throw std::runtime_error("Lookup in vector by index-only.");
				}
				else{
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
			}
			else if(parent_value.is_dict()){
				if(key_value.is_string() == false){
					throw std::runtime_error("Lookup in dict by string-only.");
				}
				else{
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
			}
			else {
				throw std::runtime_error("Lookup using [] only works with strings, vectors, dicts and json_value.");
			}
		}
	}
}

std::pair<interpreter_t, expression_t> evaluate_variabele_expression(const interpreter_t& vm, const expression_t::variable_expr_t& expr){
	QUARK_ASSERT(vm.check_invariant());

	auto vm_acc = vm;
	const auto value = resolve_env_variable(vm_acc, expr._variable);
	if(value != nullptr){
		return {vm_acc, expression_t::make_literal(value->first)};
	}
	else{
		throw std::runtime_error("Undefined variable \"" + expr._variable + "\"!");
	}
}


std::pair<interpreter_t, expression_t> evaluate_vector_definition_expression(const interpreter_t& vm, const expression_t::vector_definition_exprt_t& expr){
	QUARK_ASSERT(vm.check_invariant());

	auto vm_acc = vm;
	const std::vector<expression_t>& elements = expr._elements;
	const auto element_type = expr._element_type;

	if(elements.empty()){
		return {vm_acc, expression_t::make_literal(value_t::make_vector_value(element_type, {}))};
	}
	else{
		std::vector<value_t> elements2;
		for(const auto m: elements){
			const auto element_expr = evaluate_expression(vm_acc, m);
			vm_acc = element_expr.first;
			if(element_expr.second.is_literal() == false){
				throw std::runtime_error("Cannot evaluate element of vector definition!");
			}

			const auto element = element_expr.second.get_literal();
			elements2.push_back(element);
		}

		//	If we don't have an explicit element type, deduct it from first element.
		const auto element_type2 = element_type.is_null() ? elements2[0].get_type() : element_type;
		for(const auto m: elements2){
			if((m.get_type() == element_type2) == false){
				throw std::runtime_error("Vector can not hold elements of different type!");
			}
		}
		return {vm_acc, expression_t::make_literal(value_t::make_vector_value(element_type2, elements2))};
	}
}



std::pair<interpreter_t, expression_t> evaluate_dict_definition_expression(const interpreter_t& vm, const expression_t::dict_definition_exprt_t& expr){
	QUARK_ASSERT(vm.check_invariant());

	auto vm_acc = vm;

	const auto& elements = expr._elements;
	typeid_t value_type = expr._value_type;

	if(elements.empty()){
		return {vm_acc, expression_t::make_literal(value_t::make_dict_value(value_type, {}))};
	}
	else{
		std::map<string, value_t> elements2;
		for(const auto m: elements){
			const auto element_expr = evaluate_expression(vm_acc, m.second);
			vm_acc = element_expr.first;

			if(element_expr.second.is_literal() == false){
				throw std::runtime_error("Cannot evaluate element of vector definition!");
			}

			const auto element = element_expr.second.get_literal();

			const string key_string = m.first;
			elements2[key_string] = element;
		}

		//	If we have no value-type, deduct it from first element.
		const auto value_type2 = value_type.is_null() ? elements2.begin()->second.get_type() : value_type;

/*
		for(const auto m: elements2){
			if((m.second.get_type() == value_type2) == false){
				throw std::runtime_error("Dict can not hold elements of different type!");
			}
		}
*/
		return {vm_acc, expression_t::make_literal(value_t::make_dict_value(value_type2, elements2))};
	}
}
	
std::pair<interpreter_t, expression_t> evaluate_arithmetic_unary_minus_expression(const interpreter_t& vm, const expression_t::unary_minus_expr_t& expr){
	QUARK_ASSERT(vm.check_invariant());

	auto vm_acc = vm;
	const auto& expr2 = evaluate_expression(vm_acc, *expr._expr);
	vm_acc = expr2.first;

	if(expr2.second.is_literal()){
		const auto& c = expr2.second.get_literal();
		vm_acc = expr2.first;

		if(c.is_int()){
			return evaluate_expression(
				vm_acc,
				expression_t::make_simple_expression__2(expression_type::k_arithmetic_subtract__2, expression_t::make_literal_int(0), expr2.second)
			);
		}
		else if(c.is_float()){
			return evaluate_expression(
				vm_acc,
				expression_t::make_simple_expression__2(expression_type::k_arithmetic_subtract__2, expression_t::make_literal_float(0.0f), expr2.second)
			);
		}
		else{
			throw std::runtime_error("Unary minus won't work on expressions of type \"" + json_to_compact_string(typeid_to_ast_json(c.get_type())._value) + "\".");
		}
	}
	else{
		throw std::runtime_error("Could not evaluate condition in conditional expression.");
	}
}


std::pair<interpreter_t, expression_t> evaluate_conditional_operator_expression(const interpreter_t& vm, const expression_t::conditional_expr_t& expr){
	QUARK_ASSERT(vm.check_invariant());

	auto vm_acc = vm;

	//	Special-case since it uses 3 expressions & uses shortcut evaluation.
	const auto cond_result = evaluate_expression(vm_acc, *expr._condition);
	vm_acc = cond_result.first;

	if(cond_result.second.is_literal() && cond_result.second.get_literal().is_bool()){
		const bool cond_flag = cond_result.second.get_literal().get_bool_value();

		//	!!! Only evaluate the CHOSEN expression. Not that importan since functions are pure.
		if(cond_flag){
			return evaluate_expression(vm_acc, *expr._a);
		}
		else{
			return evaluate_expression(vm_acc, *expr._b);
		}
	}
	else{
		throw std::runtime_error("Could not evaluate condition in conditional expression.");
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

	//	Both left and right are constants, replace the math_operation with a constant!
	if(left_expr.second.is_literal() == false || right_expr.second.is_literal() == false) {
		throw std::runtime_error("Left and right expressions must be same type!");
	}
	else{

		//	Perform math operation on the two constants => new constant.
		const auto left_constant = left_expr.second.get_literal();

		//	Make rhs match left if needed/possible.
		const auto right_constant = improve_value_type(right_expr.second.get_literal(), left_constant.get_type());

		if(!(left_constant.get_type()== right_constant.get_type())){
			throw std::runtime_error("Left and right expressions must be same type!");
		}
		else{
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
	if(left_expr.second.is_literal() == false || right_expr.second.is_literal() == false) {
		throw std::runtime_error("Cannot compute arguments to operation.");
	}
	else{
		//	Perform math operation on the two constants => new constant.
		const auto left_constant = left_expr.second.get_literal();

		//	Make rhs match left if needed/possible.
		const auto right_constant = improve_value_type(right_expr.second.get_literal(), left_constant.get_type());

		if(left_constant.get_type() != right_constant.get_type()){
			throw std::runtime_error("Left and right expressions must be same type!");
		}

		else{
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
					throw std::runtime_error("Arithmetics on bool not allowed.");
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
					throw std::runtime_error("Operation not allowed on string.");
				}
				else if(op == expression_type::k_arithmetic_multiply__2){
					throw std::runtime_error("Operation not allowed on string.");
				}
				else if(op == expression_type::k_arithmetic_divide__2){
					throw std::runtime_error("Operation not allowed on string.");
				}
				else if(op == expression_type::k_arithmetic_remainder__2){
					throw std::runtime_error("Operation not allowed on string.");
				}


				else if(op == expression_type::k_logical_and__2){
					throw std::runtime_error("Operation not allowed on string.");
				}
				else if(op == expression_type::k_logical_or__2){
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
					throw std::runtime_error("Struct type mismatch.");
				}

				if(op == expression_type::k_arithmetic_add__2){
					throw std::runtime_error("Operation not allowed on struct.");
				}
				else if(op == expression_type::k_arithmetic_subtract__2){
					throw std::runtime_error("Operation not allowed on struct.");
				}
				else if(op == expression_type::k_arithmetic_multiply__2){
					throw std::runtime_error("Operation not allowed on struct.");
				}
				else if(op == expression_type::k_arithmetic_divide__2){
					throw std::runtime_error("Operation not allowed on struct.");
				}
				else if(op == expression_type::k_arithmetic_remainder__2){
					throw std::runtime_error("Operation not allowed on struct.");
				}

				else if(op == expression_type::k_logical_and__2){
					throw std::runtime_error("Operation not allowed on struct.");
				}
				else if(op == expression_type::k_logical_or__2){
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
						throw std::runtime_error("Operation not allowed on vectors.");
					}
					else if(op == expression_type::k_arithmetic_multiply__2){
						throw std::runtime_error("Operation not allowed on vectors.");
					}
					else if(op == expression_type::k_arithmetic_divide__2){
						throw std::runtime_error("Operation not allowed on vectors.");
					}
					else if(op == expression_type::k_arithmetic_remainder__2){
						throw std::runtime_error("Operation not allowed on vectors.");
					}


					else if(op == expression_type::k_logical_and__2){
						throw std::runtime_error("Operation not allowed on vectors.");
					}
					else if(op == expression_type::k_logical_or__2){
						throw std::runtime_error("Operation not allowed on vectors.");
					}
					else{
						QUARK_ASSERT(false);
						throw std::exception();
					}
				}
			}
			else if(type_mode.is_function()){
				throw std::runtime_error("Cannot perform operations on two function values.");
			}
			else{
				throw std::runtime_error("Arithmetics failed.");
			}
		}
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
		return evaluate_variabele_expression(vm, *e.get_variable());
	}

	else if(op == expression_type::k_call){
		return evaluate_call_expression(vm, *e.get_function_call());
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
		if(args.size() != arg_types.size()){
			throw std::runtime_error("Wrong number of arguments to function.");
		}
		for(int i = 0 ; i < args.size() ; i++){
			if(args[i].get_type() != arg_types[i]){
				throw std::runtime_error("Function argument type do not match.");
			}
		}

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

//		QUARK_TRACE(json_to_pretty_string(interpreter_to_json(vm_acc)));

		const auto r = execute_statements(vm_acc, function_def._statements);

		vm_acc = r.first;
		vm_acc._call_stack.pop_back();

		if(r.second._type != statement_result_t::k_return_unwind){
			throw std::runtime_error("Function missing return statement");
		}
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



//	May return a simplified expression instead of a value literal..
std::pair<interpreter_t, expression_t> evaluate_call_expression(const interpreter_t& vm, const expression_t::function_call_expr_t& expr){
	QUARK_ASSERT(vm.check_invariant());

	auto vm_acc = vm;

	//	Simplify each input expression: expression[0]: which function to call, expression[1]: first argument if any.
	const auto function = evaluate_expression(vm_acc, *expr._function);
	vm_acc = function.first;
	vector<expression_t> args2;
	for(int i = 0 ; i < expr._args.size() ; i++){
		const auto t = evaluate_expression(vm_acc, expr._args[i]);
		vm_acc = t.first;
		args2.push_back(t.second);
	}

	//	If not all input expressions could be evaluated, return a (maybe simplified) expression.
	if(function.second.is_literal() == false || all_literals(args2) == false){
		return {vm_acc, expression_t::make_function_call(function.second, args2)};
	}

	//	Convert to values.
	const auto& function_value = function.second.get_literal();
	vector<value_t> arg_values;
	for(int i = 0 ; i < args2.size() ; i++){
		const auto& v = args2[i].get_literal();
		arg_values.push_back(v);
	}

	//	Get function value and arg values.
	if(function_value.is_function() == false){
		//	Attempting to call a TYPE? Then this may be a constructor call.
		if(function_value.is_typeid()){
			const auto result = construct_value_from_typeid(vm_acc, function_value.get_typeid_value(), arg_values);
			vm_acc = result.first;
			return { vm_acc, expression_t::make_literal(result.second)};
		}
/*
		else if(function_value.is_vector()){
			const auto result = construct_value_from_typeid(vm_acc, cleanup_vector_constructor_type(function_value.get_type()), arg_values);
			vm_acc = result.first;
			return { vm_acc, expression_t::make_literal(result.second)};
		}
*/
		else{
			throw std::runtime_error("Cannot call non-function.");
		}
	}

	//	Call function-value.
	else{
		const auto& result = call_function(vm_acc, function_value, arg_values);
		QUARK_ASSERT(result.second._type == statement_result_t::k_return_unwind);
		vm_acc = result.first;
		return { vm_acc, expression_t::make_literal(result.second._output)};
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
			expression_t::make_literal_int(2)
		),
		expression_t::make_literal_int(3)
	);
}


QUARK_UNIT_TESTQ("evaluate_expression()", "3 * 4 == 12") {
	test__evaluate_expression(
		expression_t::make_simple_expression__2(
			expression_type::k_arithmetic_multiply__2,
			expression_t::make_literal_int(3),
			expression_t::make_literal_int(4)
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
				expression_t::make_literal_int(4)
			),
			expression_t::make_literal_int(5)
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



//	Records all output to interpreter
std::pair<interpreter_t, value_t> host__print(const interpreter_t& vm, const std::vector<value_t>& args){
	QUARK_ASSERT(vm.check_invariant());

	if(args.size() != 1){
		throw std::runtime_error("assert() requires 1 argument!");
	}

	auto vm2 = vm;
	const auto& value = args[0];

#if 0
	if(value.is_struct() && value.get_struct_value()->_def == *json_value___struct_def){
		const auto s = json_value__to_compact_string(value);
		vm2._print_output.push_back(s);
	}
	else
#endif
	{
		const auto s = to_compact_string2(value);
		printf("%s\n", s.c_str());
		vm2._print_output.push_back(s);
	}

	return {vm2, value_t::make_null() };
}

std::pair<interpreter_t, value_t> host__assert(const interpreter_t& vm, const std::vector<value_t>& args){
	QUARK_ASSERT(vm.check_invariant());

	if(args.size() != 1){
		throw std::runtime_error("assert() requires 1 argument!");
	}

	auto vm2 = vm;
	const auto& value = args[0];
	if(value.is_bool() == false){
		throw std::runtime_error("First argument to assert() must be of type bool.");
	}
	bool ok = value.get_bool_value();
	if(!ok){
		vm2._print_output.push_back("Assertion failed.");
		throw std::runtime_error("Floyd assertion failed.");
	}
	return {vm2, value_t::make_null() };
}

//	string to_string(value_t)
std::pair<interpreter_t, value_t> host__to_string(const interpreter_t& vm, const std::vector<value_t>& args){
	QUARK_ASSERT(vm.check_invariant());

	if(args.size() != 1){
		throw std::runtime_error("to_string() requires 1 argument!");
	}

	const auto& value = args[0];
	const auto a = to_compact_string2(value);
	return {vm, value_t::make_string(a) };
}
std::pair<interpreter_t, value_t> host__to_pretty_string(const interpreter_t& vm, const std::vector<value_t>& args){
	QUARK_ASSERT(vm.check_invariant());

	if(args.size() != 1){
		throw std::runtime_error("to_pretty_string() requires 1 argument!");
	}

	const auto& value = args[0];
	const auto json = value_to_ast_json(value);
	const auto s = json_to_pretty_string(json._value, 0, pretty_t{80, 4});
	return {vm, value_t::make_string(s) };
}

std::pair<interpreter_t, value_t> host__typeof(const interpreter_t& vm, const std::vector<value_t>& args){
	QUARK_ASSERT(vm.check_invariant());

	if(args.size() != 1){
		throw std::runtime_error("typeof() requires 1 argument!");
	}

	const auto& value = args[0];
	const auto type = value.get_type();
	const auto result = value_t::make_typeid_value(type);
	return {vm, result };
}

std::pair<interpreter_t, value_t> host__get_time_of_day(const interpreter_t& vm, const std::vector<value_t>& args){
	QUARK_ASSERT(vm.check_invariant());

	if(args.size() != 0){
		throw std::runtime_error("get_time_of_day() requires 0 arguments!");
	}

	std::chrono::time_point<std::chrono::high_resolution_clock> t = std::chrono::high_resolution_clock::now();
	std::chrono::duration<double> elapsed_seconds = t - vm._start_time;
	const auto ms = elapsed_seconds.count() * 1000.0;
	return {vm, value_t::make_int(int(ms)) };
}

QUARK_UNIT_TESTQ("sizeof(int)", ""){
	QUARK_TRACE(std::to_string(sizeof(int)));
	QUARK_TRACE(std::to_string(sizeof(int64_t)));
}

QUARK_UNIT_TESTQ("get_time_of_day_ms()", ""){
	const auto a = std::chrono::high_resolution_clock::now();
    std::this_thread::sleep_for(std::chrono::milliseconds(7));
	const auto b = std::chrono::high_resolution_clock::now();

	std::chrono::duration<double> elapsed_seconds = b - a;
	const int ms = static_cast<int>((static_cast<double>(elapsed_seconds.count()) * 1000.0));

	QUARK_UT_VERIFY(ms >= 7)
}





//### add checking of function types when calling / returning from them. Also host functions.

typeid_t resolve_type_using_env(const interpreter_t& vm, const typeid_t& type){
	if(type.get_base_type() == base_type::k_unresolved_type_identifier){
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


	value_t update_struct_member_shallow(const interpreter_t& vm, const value_t& obj, const std::string& member_name, const value_t& new_value){
		QUARK_ASSERT(obj.check_invariant());
		QUARK_ASSERT(member_name.empty() == false);
		QUARK_ASSERT(new_value.check_invariant());

		const auto s = obj.get_struct_value();
		const auto def = s->_def;

		int member_index = find_struct_member_index(def, member_name);
		if(member_index == -1){
			throw std::runtime_error("Unknown member.");
		}

		const auto struct_typeid = obj.get_type();
		const auto values = s->_member_values;


		QUARK_TRACE(typeid_to_compact_string(new_value.get_type()));
		QUARK_TRACE(typeid_to_compact_string(def._members[member_index]._type));

		const auto dest_member_entry = def._members[member_index];
		auto dest_member_resolved_type = dest_member_entry._type;
		dest_member_resolved_type = resolve_type_using_env(vm, dest_member_entry._type);

		if(!(new_value.get_type() == dest_member_resolved_type)){
			throw std::runtime_error("Value type not matching struct member type.");
		}

		auto values2 = values;
		values2[member_index] = new_value;

		auto s2 = value_t::make_struct_value(struct_typeid, values2);
		return s2;
	}

	value_t update_struct_member_deep(const interpreter_t& vm, const value_t& obj, const std::vector<std::string>& path, const value_t& new_value){
		QUARK_ASSERT(obj.check_invariant());
		QUARK_ASSERT(path.empty() == false);
		QUARK_ASSERT(new_value.check_invariant());

		if(path.size() == 1){
			return update_struct_member_shallow(vm, obj, path[0], new_value);
		}
		else{
			vector<string> subpath = path;
			subpath.erase(subpath.begin());

			const auto s = obj.get_struct_value();
			const auto def = s->_def;
			int member_index = find_struct_member_index(def, path[0]);
			if(member_index == -1){
				throw std::runtime_error("Unknown member.");
			}

			const auto child_value = s->_member_values[member_index];
			if(child_value.is_struct() == false){
				throw std::runtime_error("Value type not matching struct member type.");
			}

			const auto child2 = update_struct_member_deep(vm, child_value, subpath, new_value);
			const auto obj2 = update_struct_member_shallow(vm, obj, path[0], child2);
			return obj2;
		}
	}

std::pair<interpreter_t, value_t> host__update(const interpreter_t& vm, const std::vector<value_t>& args){
	QUARK_ASSERT(vm.check_invariant());

	QUARK_TRACE(json_to_pretty_string(interpreter_to_json(vm)));

	const auto& obj1 = args[0];
	const auto& lookup_key = args[1];
	const auto& new_value = args[2];

	if(args.size() != 3){
		throw std::runtime_error("update() needs 3 arguments.");
	}
	else{
		if(obj1.is_string()){
			if(lookup_key.is_int() == false){
				throw std::runtime_error("String lookup using integer index only.");
			}
			else{
				const auto obj = obj1;
				const auto v = obj.get_string_value();

				if((new_value.get_type().is_string() && new_value.get_string_value().size() == 1) == false){
					throw std::runtime_error("Update element must be a 1-character string.");
				}
				else{
					const int lookup_index = lookup_key.get_int_value();
					if(lookup_index < 0 || lookup_index >= v.size()){
						throw std::runtime_error("String lookup out of bounds.");
					}
					else{
						string v2 = v;
						v2[lookup_index] = new_value.get_string_value()[0];
						const auto s2 = value_t::make_string(v2);
						return {vm, s2 };
					}
				}
			}
		}
		else if(obj1.is_json_value()){
			const auto json_value0 = obj1.get_json_value();
			if(json_value0.is_array()){
				assert(false);
			}
			else if(json_value0.is_object()){
				assert(false);
			}
			else{
				throw std::runtime_error("Can only update string, vector, dict or struct.");
			}
		}
		else if(obj1.is_vector()){
			if(lookup_key.is_int() == false){
				throw std::runtime_error("Vector lookup using integer index only.");
			}
			else{
				const auto obj = improve_value_type(obj1, typeid_t::make_vector(new_value.get_type()));
				const auto v = obj.get_vector_value();
				auto v2 = v->_elements;

				if((v->_element_type == new_value.get_type()) == false){
					throw std::runtime_error("Update element must match vector type.");
				}
				else{
					const int lookup_index = lookup_key.get_int_value();
					if(lookup_index < 0 || lookup_index >= v->_elements.size()){
						throw std::runtime_error("Vector lookup out of bounds.");
					}
					else{
						v2[lookup_index] = new_value;
						const auto s2 = value_t::make_vector_value(v->_element_type, v2);
						return {vm, s2 };
					}
				}
			}
		}
		else if(obj1.is_dict()){
			if(lookup_key.is_string() == false){
				throw std::runtime_error("Dict lookup using string key only.");
			}
			else{
				const auto obj = improve_value_type(obj1, typeid_t::make_dict(new_value.get_type()));
				const auto v = obj.get_dict_value();
				auto v2 = v->_elements;

				if((v->_value_type == new_value.get_type()) == false){
					throw std::runtime_error("Update element must match dict value type.");
				}
				else{
					const string key = lookup_key.get_string_value();
					auto elements2 = v->_elements;
					elements2[key] = new_value;
					const auto value2 = value_t::make_dict_value(obj.get_dict_value()->_value_type, elements2);
					return { vm, value2 };
				}
			}
		}
		else if(obj1.is_struct()){
			if(lookup_key.is_string() == false){
				throw std::runtime_error("You must specify structure member using string.");
			}
			else{
				const auto obj = obj1;

				//### Does simple check for "." -- we should use vector of strings instead.
				const auto nodes = split_on_chars(seq_t(lookup_key.get_string_value()), ".");
				if(nodes.empty()){
					throw std::runtime_error("You must specify structure member using string.");
				}

				const auto s2 = update_struct_member_deep(vm, obj, nodes, new_value);
				return {vm, s2 };
			}
		}
		else {
			throw std::runtime_error("Can only update string, vector, dict or struct.");
		}
	}
}

std::pair<interpreter_t, value_t> host__size(const interpreter_t& vm, const std::vector<value_t>& args){
	QUARK_ASSERT(vm.check_invariant());

	if(args.size() != 1){
		throw std::runtime_error("find requires 2 arguments");
	}

	const auto obj = args[0];
	if(obj.is_string()){
		const auto size = obj.get_string_value().size();
		return {vm, value_t::make_int(static_cast<int>(size))};
	}
	else if(obj.is_json_value()){
		const auto value = obj.get_json_value();
		if(value.is_object()){
			const auto size = value.get_object_size();
			return {vm, value_t::make_int(static_cast<int>(size))};
		}
		else if(value.is_array()){
			const auto size = value.get_array_size();
			return {vm, value_t::make_int(static_cast<int>(size))};
		}
		else if(value.is_string()){
			const auto size = value.get_string().size();
			return {vm, value_t::make_int(static_cast<int>(size))};
		}
		else{
			throw std::runtime_error("Calling size() on unsupported type of value.");
		}
	}
	else if(obj.is_vector()){
		const auto size = obj.get_vector_value()->_elements.size();
		return {vm, value_t::make_int(static_cast<int>(size))};
	}
	else if(obj.is_dict()){
		const auto size = obj.get_dict_value()->_elements.size();
		return {vm, value_t::make_int(static_cast<int>(size))};
	}
	else{
		throw std::runtime_error("Calling size() on unsupported type of value.");
	}
}

std::pair<interpreter_t, value_t> host__find(const interpreter_t& vm, const std::vector<value_t>& args){
	QUARK_ASSERT(vm.check_invariant());

	if(args.size() != 2){
		throw std::runtime_error("find requires 2 arguments");
	}

	const auto obj = args[0];
	const auto wanted = args[1];

	if(obj.is_string()){
		const auto str = obj.get_string_value();
		const auto wanted2 = wanted.get_string_value();

		const auto r = str.find(wanted2);
		if(r == std::string::npos){
			return {vm, value_t::make_int(static_cast<int>(-1))};
		}
		else{
			return {vm, value_t::make_int(static_cast<int>(r))};
		}
	}
	else if(obj.is_vector()){
		const auto vec = obj.get_vector_value();
		if(wanted.get_type() != vec->_element_type){
			throw std::runtime_error("Type mismatch.");
		}
		const auto size = vec->_elements.size();
		int index = 0;
		while(index < size && vec->_elements[index] != wanted){
			index++;
		}
		if(index == size){
			return {vm, value_t::make_int(static_cast<int>(-1))};
		}
		else{
			return {vm, value_t::make_int(static_cast<int>(index))};
		}
	}
	else{
		throw std::runtime_error("Calling find() on unsupported type of value.");
	}
}

std::pair<interpreter_t, value_t> host__exists(const interpreter_t& vm, const std::vector<value_t>& args){
	QUARK_ASSERT(vm.check_invariant());

	if(args.size() != 2){
		throw std::runtime_error("find requires 2 arguments");
	}

	const auto obj = args[0];
	const auto key = args[1];

	if(obj.is_dict()){
		if(key.get_type().is_string() == false){
			throw std::runtime_error("Key must be string.");
		}
		const auto key_string = key.get_string_value();
		const auto v = obj.get_dict_value();

		const auto found_it = v->_elements.find(key_string);
		const bool exists = found_it != v->_elements.end();
		return {vm, value_t::make_bool(exists)};
	}
	else{
		throw std::runtime_error("Calling exist() on unsupported type of value.");
	}
}

std::pair<interpreter_t, value_t> host__erase(const interpreter_t& vm, const std::vector<value_t>& args){
	QUARK_ASSERT(vm.check_invariant());

	if(args.size() != 2){
		throw std::runtime_error("find requires 2 arguments");
	}

	const auto obj = args[0];
	const auto key = args[1];

	if(obj.is_dict()){
		if(key.get_type().is_string() == false){
			throw std::runtime_error("Key must be string.");
		}
		const auto key_string = key.get_string_value();
		const auto v = obj.get_dict_value();

		std::map<string, value_t> elements2 = v->_elements;
		elements2.erase(key_string);
		const auto value2 = value_t::make_dict_value(obj.get_dict_value()->_value_type, elements2);
		return { vm, value2 };
	}
	else{
		throw std::runtime_error("Calling exist() on unsupported type of value.");
	}
}




//	assert(push_back(["one","two"], "three") == ["one","two","three"])
std::pair<interpreter_t, value_t> host__push_back(const interpreter_t& vm, const std::vector<value_t>& args){
	QUARK_ASSERT(vm.check_invariant());

	if(args.size() != 2){
		throw std::runtime_error("find requires 2 arguments");
	}

	const auto obj = args[0];
	const auto element = args[1];
	if(obj.is_string()){
		const auto str = obj.get_string_value();
		const auto ch = element.get_string_value();

		auto str2 = str + ch;
		const auto v = value_t::make_string(str2);
		return {vm, v};
	}
	else if(obj.is_vector()){
		const auto vec = obj.get_vector_value();
		if(element.get_type() != vec->_element_type){
			throw std::runtime_error("Type mismatch.");
		}
		auto elements2 = vec->_elements;
		elements2.push_back(element);
		const auto v = value_t::make_vector_value(vec->_element_type, elements2);
		return {vm, v};
	}
	else{
		throw std::runtime_error("Calling push_back() on unsupported type of value.");
	}
}

//	assert(subset("abc", 1, 3) == "bc");
std::pair<interpreter_t, value_t> host__subset(const interpreter_t& vm, const std::vector<value_t>& args){
	QUARK_ASSERT(vm.check_invariant());

	if(args.size() != 3){
		throw std::runtime_error("subset() requires 2 arguments");
	}

	const auto obj = args[0];

	if(args[1].is_int() == false || args[2].is_int() == false){
		throw std::runtime_error("subset() requires start and end to be integers.");
	}

	const auto start = args[1].get_int_value();
	const auto end = args[2].get_int_value();
	if(start < 0 || end < 0){
		throw std::runtime_error("subset() requires start and end to be non-negative.");
	}

	if(obj.is_string()){
		const auto str = obj.get_string_value();
		const auto start2 = std::min(start, static_cast<int>(str.size()));
		const auto end2 = std::min(end, static_cast<int>(str.size()));

		string str2;
		for(int i = start2 ; i < end2 ; i++){
			str2.push_back(str[i]);
		}
		const auto v = value_t::make_string(str2);
		return {vm, v};
	}
	else if(obj.is_vector()){
		const auto vec = obj.get_vector_value();
		const auto start2 = std::min(start, static_cast<int>(vec->_elements.size()));
		const auto end2 = std::min(end, static_cast<int>(vec->_elements.size()));
		vector<value_t> elements2;
		for(int i = start2 ; i < end2 ; i++){
			elements2.push_back(vec->_elements[i]);
		}
		const auto v = value_t::make_vector_value(vec->_element_type, elements2);
		return {vm, v};
	}
	else{
		throw std::runtime_error("Calling push_back() on unsupported type of value.");
	}
}



//	assert(replace("One ring to rule them all", 4, 7, "rabbit") == "One rabbit to rule them all");
std::pair<interpreter_t, value_t> host__replace(const interpreter_t& vm, const std::vector<value_t>& args){
	QUARK_ASSERT(vm.check_invariant());

	if(args.size() != 4){
		throw std::runtime_error("replace() requires 4 arguments");
	}

	const auto obj = args[0];

	if(args[1].is_int() == false || args[2].is_int() == false){
		throw std::runtime_error("replace() requires start and end to be integers.");
	}

	const auto start = args[1].get_int_value();
	const auto end = args[2].get_int_value();
	if(start < 0 || end < 0){
		throw std::runtime_error("replace() requires start and end to be non-negative.");
	}
	if(args[3].get_type() != args[0].get_type()){
		throw std::runtime_error("replace() requires 4th arg to be same as argument 0.");
	}

	if(obj.is_string()){
		const auto str = obj.get_string_value();
		const auto start2 = std::min(start, static_cast<int>(str.size()));
		const auto end2 = std::min(end, static_cast<int>(str.size()));
		const auto new_bits = args[3].get_string_value();

		string str2 = str.substr(0, start2) + new_bits + str.substr(end2);
		const auto v = value_t::make_string(str2);
		return {vm, v};
	}
	else if(obj.is_vector()){
		const auto vec = obj.get_vector_value();
		const auto start2 = std::min(start, static_cast<int>(vec->_elements.size()));
		const auto end2 = std::min(end, static_cast<int>(vec->_elements.size()));
		const auto new_bits = args[3].get_vector_value();


		const auto string_left = vector<value_t>(vec->_elements.begin(), vec->_elements.begin() + start2);
		const auto string_right = vector<value_t>(vec->_elements.begin() + end2, vec->_elements.end());

		auto result = string_left;
		result.insert(result.end(), new_bits->_elements.begin(), new_bits->_elements.end());
		result.insert(result.end(), string_right.begin(), string_right.end());

		const auto v = value_t::make_vector_value(vec->_element_type, result);
		return {vm, v};
	}
	else{
		throw std::runtime_error("Calling replace() on unsupported type of value.");
	}
}

std::pair<interpreter_t, value_t> host__get_env_path(const interpreter_t& vm, const std::vector<value_t>& args){
	QUARK_ASSERT(vm.check_invariant());

	if(args.size() != 0){
		throw std::runtime_error("get_env_path() requires 0 arguments!");
	}

    const char *homeDir = getenv("HOME");
    const std::string env_path(homeDir);
//	const std::string env_path = "~/Desktop/";

	return {vm, value_t::make_string(env_path) };
}

std::pair<interpreter_t, value_t> host__read_text_file(const interpreter_t& vm, const std::vector<value_t>& args){
	QUARK_ASSERT(vm.check_invariant());

	if(args.size() != 1){
		throw std::runtime_error("read_text_file() requires 1 arguments!");
	}
	if(args[0].is_string() == false){
		throw std::runtime_error("read_text_file() requires a file path as a string.");
	}

	const string source_path = args[0].get_string_value();
	std::string file_contents;
	{
		std::ifstream f (source_path);
		if (f.is_open() == false){
			throw std::runtime_error("Cannot read source file.");
		}
		std::string line;
		while ( getline(f, line) ) {
			file_contents.append(line + "\n");
		}
		f.close();
	}
	return {vm, value_t::make_string(file_contents) };
}

std::pair<interpreter_t, value_t> host__write_text_file(const interpreter_t& vm, const std::vector<value_t>& args){
	QUARK_ASSERT(vm.check_invariant());

	if(args.size() != 2){
		throw std::runtime_error("write_text_file() requires 2 arguments!");
	}
	else if(args[0].is_string() == false){
		throw std::runtime_error("write_text_file() requires a file path as a string.");
	}
	else if(args[1].is_string() == false){
		throw std::runtime_error("write_text_file() requires a file path as a string.");
	}
	else{
		const string path = args[0].get_string_value();
		const string file_contents = args[1].get_string_value();

		std::ofstream outputFile;
		outputFile.open(path);
		outputFile << file_contents;
		outputFile.close();
		return {vm, value_t() };
	}
}

/*
	Reads json from a text string, returning an unpacked json_value.
*/
std::pair<interpreter_t, value_t> host__decode_json(const interpreter_t& vm, const std::vector<value_t>& args){
	QUARK_ASSERT(vm.check_invariant());

	if(args.size() != 1){
		throw std::runtime_error("decode_json_value() requires 1 argument!");
	}
	else if(args[0].is_string() == false){
		throw std::runtime_error("decode_json_value() requires string argument.");
	}
	else{
		const string s = args[0].get_string_value();
		std::pair<json_t, seq_t> result = parse_json(seq_t(s));
		value_t json_value = value_t::make_json_value(result.first);
		return {vm, json_value };
	}
}

std::pair<interpreter_t, value_t> host__encode_json(const interpreter_t& vm, const std::vector<value_t>& args){
	QUARK_ASSERT(vm.check_invariant());

	if(args.size() != 1){
		throw std::runtime_error("encode_json() requires 1 argument!");
	}
	else if(args[0].is_json_value()== false){
		throw std::runtime_error("encode_json() requires argument to be json_value.");
	}
	else{
		const auto value0 = args[0].get_json_value();
		const string s = json_to_compact_string(value0);

		return {vm, value_t::make_string(s) };
	}
}



std::pair<interpreter_t, value_t> host__flatten_to_json(const interpreter_t& vm, const std::vector<value_t>& args){
	QUARK_ASSERT(vm.check_invariant());

	if(args.size() != 1){
		throw std::runtime_error("flatten_to_json() requires 1 argument!");
	}
	else{
		const auto value = args[0];
		const auto result = flatten_to_json(value);
		return {vm, result };
	}
}

std::pair<interpreter_t, value_t> host__unflatten_from_json(const interpreter_t& vm, const std::vector<value_t>& args){
	QUARK_ASSERT(vm.check_invariant());

	if(args.size() != 2){
		throw std::runtime_error("unflatten_from_json() requires 2 argument!");
	}
	else if(args[0].is_json_value() == false){
		throw std::runtime_error("unflatten_from_json() requires string argument.");
	}
	else if(args[1].is_typeid()== false){
		throw std::runtime_error("unflatten_from_json() requires string argument.");
	}
	else{
		const auto json_value = args[0].get_json_value();
		const auto target_type = args[1].get_typeid_value();
		const auto value = unflatten_json_to_specific_type(json_value, target_type);
		return {vm, value };
	}
}


std::pair<interpreter_t, value_t> host__get_json_type(const interpreter_t& vm, const std::vector<value_t>& args){
	QUARK_ASSERT(vm.check_invariant());

	if(args.size() != 1){
		throw std::runtime_error("get_json_type() requires 1 argument!");
	}
	else if(args[0].is_json_value() == false){
		throw std::runtime_error("get_json_type() requires json_value argument.");
	}
	else{
		const auto json_value = args[0].get_json_value();

		if(json_value.is_object()){
			return { vm, value_t::make_int(1) };
//			return { vm, value_t::make_typeid_value(typeid_t::make_dict(typeid_t::make_json_value())) };
		}
		else if(json_value.is_array()){
			return { vm, value_t::make_int(2) };
//			return { vm, value_t::make_typeid_value(typeid_t::make_vector(typeid_t::make_json_value())) };
		}
		else if(json_value.is_string()){
			return { vm, value_t::make_int(3) };
//			return { vm, value_t::make_typeid_value(typeid_t::make_string()) };
		}
		else if(json_value.is_number()){
			return { vm, value_t::make_int(4) };
//			return { vm, value_t::make_typeid_value(typeid_t::make_float()) };
		}
		else if(json_value.is_true()){
			return { vm, value_t::make_int(5) };
//			return { vm, value_t::make_typeid_value(typeid_t::make_bool()) };
		}
		else if(json_value.is_false()){
			return { vm, value_t::make_int(6) };
//			return { vm, value_t::make_typeid_value(typeid_t::make_bool()) };
		}
		else if(json_value.is_null()){
			return { vm, value_t::make_int(7) };
//			return { vm, value_t::make_typeid_value(typeid_t::make_null()) };
		}
		else{
			QUARK_ASSERT(false);
			throw std::exception();
		}
	}
}


typedef std::pair<interpreter_t, value_t> (*HOST_FUNCTION_PTR)(const interpreter_t& vm, const std::vector<value_t>& args);

struct host_function_t {
	std::string _name;
	HOST_FUNCTION_PTR _function_ptr;
	typeid_t _function_type;
};

const vector<host_function_t> k_host_functions {
	host_function_t{ "print", host__print, typeid_t::make_function(typeid_t::make_null(), {typeid_t::make_null()}) },
	host_function_t{ "assert", host__assert, typeid_t::make_function(typeid_t::make_null(), {typeid_t::make_null()}) },
	host_function_t{ "to_string", host__to_string, typeid_t::make_function(typeid_t::make_string(), {typeid_t::make_null()}) },
	host_function_t{ "to_pretty_string", host__to_pretty_string, typeid_t::make_function(typeid_t::make_string(), {typeid_t::make_null()}) },
	host_function_t{ "typeof", host__typeof, typeid_t::make_function(typeid_t::make_typeid(), {typeid_t::make_null()}) },

	host_function_t{ "get_time_of_day", host__get_time_of_day, typeid_t::make_function(typeid_t::make_int(), {}) },
	host_function_t{ "update", host__update, typeid_t::make_function(typeid_t::make_null(), {typeid_t::make_null(),typeid_t::make_null(),typeid_t::make_null()}) },
	host_function_t{ "size", host__size, typeid_t::make_function(typeid_t::make_null(), {typeid_t::make_null()}) },
	host_function_t{ "find", host__find, typeid_t::make_function(typeid_t::make_int(), {typeid_t::make_null(), typeid_t::make_null()}) },
	host_function_t{ "exists", host__exists, typeid_t::make_function(typeid_t::make_bool(), {typeid_t::make_null(),typeid_t::make_null()}) },
	host_function_t{ "erase", host__erase, typeid_t::make_function(typeid_t::make_null(), {typeid_t::make_null(),typeid_t::make_null()}) },
	host_function_t{ "push_back", host__push_back, typeid_t::make_function(typeid_t::make_null(), {typeid_t::make_null(),typeid_t::make_null()}) },
	host_function_t{ "subset", host__subset, typeid_t::make_function(typeid_t::make_null(), {typeid_t::make_null(),typeid_t::make_null(),typeid_t::make_null()}) },
	host_function_t{ "replace", host__replace, typeid_t::make_function(typeid_t::make_null(), {typeid_t::make_null(),typeid_t::make_null(),typeid_t::make_null(),typeid_t::make_null()}) },

	host_function_t{ "get_env_path", host__get_env_path, typeid_t::make_function(typeid_t::make_string(), {}) },
	host_function_t{ "read_text_file", host__read_text_file, typeid_t::make_function(typeid_t::make_string(), {typeid_t::make_null()}) },
	host_function_t{ "write_text_file", host__write_text_file, typeid_t::make_function(typeid_t::make_string(), {typeid_t::make_null(), typeid_t::make_null()}) },

	host_function_t{ "decode_json", host__decode_json, typeid_t::make_function(typeid_t::make_string(), {typeid_t::make_null()}) },
	host_function_t{ "encode_json", host__encode_json, typeid_t::make_function(typeid_t::make_string(), {typeid_t::make_null()}) },

	host_function_t{ "flatten_to_json", host__flatten_to_json, typeid_t::make_function(typeid_t::make_json_value(), {typeid_t::make_null()}) },
	host_function_t{ "unflatten_from_json", host__unflatten_from_json, typeid_t::make_function(typeid_t::make_null(), {typeid_t::make_null(), typeid_t::make_null()}) },

	host_function_t{ "get_json_type", host__get_json_type, typeid_t::make_function(typeid_t::make_typeid(), {typeid_t::make_json_value()}) }
};




//	NOTICE: We do function overloading for the host functions: you can call them with any *type* of arguments and it gives any return type.
std::pair<interpreter_t, statement_result_t> call_host_function(const interpreter_t& vm, int function_id, const std::vector<floyd::value_t> args){
	const int index = function_id - 1000;
	QUARK_ASSERT(index >= 0 && index < k_host_functions.size())

	const auto& host_function = k_host_functions[index];

	//	arity
	if(args.size() != host_function._function_type.get_function_args().size()){
		throw std::runtime_error("Wrong number of arguments to host function.");
	}

	const auto result = (host_function._function_ptr)(vm, args);
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


	//	Insert built-in functions into AST.
	for(auto i = 0 ; i < k_host_functions.size() ; i++){
		const auto& hf = k_host_functions[i];
		const auto def = function_definition_t(
			{},
			i + 1000,
			hf._function_type.get_function_return()
		);

		const auto function_value = value_t::make_function_value(def);
		global_env->_values[hf._name] = std::pair<value_t, bool>{function_value, false };
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
	return pass2;
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
	parser_context_t context2{ quark::trace_context_t(context._tracer._verbose, context._tracer._tracer) };

	auto ast = program_to_ast2(context, source);
	auto vm = interpreter_t(ast);
//	QUARK_TRACE(json_to_pretty_string(interpreter_to_json(vm)));
	print_vm_printlog(vm);
	return vm;
}

std::pair<interpreter_t, statement_result_t> run_main(const interpreter_context_t& context, const string& source, const vector<floyd::value_t>& args){
	parser_context_t context2{ quark::trace_context_t(context._tracer._verbose, context._tracer._tracer) };

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

