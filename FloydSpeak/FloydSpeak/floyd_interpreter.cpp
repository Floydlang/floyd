
//
//  parser_evaluator.cpp
//  FloydSpeak
//
//  Created by Marcus Zetterquist on 26/07/16.
//  Copyright © 2016 Marcus Zetterquist. All rights reserved.
//

#include "floyd_interpreter.h"

#include "parse_expression.h"
#include "parse_statement.h"
#include "statements.h"
#include "floyd_parser.h"
#include "parser_value.h"
#include "pass2.h"
#include "json_support.h"
#include "json_parser.h"

#include <cmath>
#include <sys/time.h>

#include <thread>
#include <chrono>
#include <algorithm>


namespace floyd {


using std::vector;
using std::string;
using std::pair;
using std::shared_ptr;
using std::unique_ptr;
using std::make_shared;

using namespace floyd;



std::pair<interpreter_t, expression_t> evaluate_call_expression(const interpreter_t& vm, const expression_t& e);
std::pair<floyd::value_t, bool>* resolve_env_variable(const interpreter_t& vm, const std::string& s);

namespace {

	//	WHen [] appears in an expression we know it's an empty vector but not which type. It can be used as any vector type.
	bool is_vector_john_doe(const value_t& value){
		QUARK_ASSERT(value.check_invariant());
		QUARK_ASSERT(value.is_vector());

		const auto p = value.get_vector_value();
		if(p->_element_type.is_null() && p->_elements.empty()){
			return true;
		}
		else{
			return false;
		}
	}

	// ### Make this built in into evaluate_expression()?
	value_t improve_vector(const value_t& value){
		QUARK_ASSERT(value.check_invariant());
		QUARK_ASSERT(value.is_vector());

		const auto p = value.get_vector_value();

		const auto element_type = p->_element_type;//	.get_typeid_typeid().is_null()){

		//	Type == vector[null]?
		if(element_type.is_null()){
			if(p->_elements.empty()){
				return value;
			}
			else{
				//	Figure out the element type.
				const auto element_type2 = p->_elements[0].get_type();
				return improve_vector(make_vector_value(element_type2, p->_elements));
			}
		}
		else{
			//	Check that element types match vector type.
			for(const auto e: p->_elements){
				if(e.get_type() != element_type){
					throw std::runtime_error("Vector element of wrong type.");
				}
			}
			return value;
		}
	}

	value_t improve_value(const value_t& value){
		if(value.is_vector()){
			return improve_vector(value);
		}
		else{
			return value;
		}
	}

	//	We know which type we need. If the value has not type, retype it.
	value_t improve_value_type(const value_t& value, const typeid_t& expected_type){
		QUARK_ASSERT(value.check_invariant());
		QUARK_ASSERT(expected_type.check_invariant());

		if(value.is_vector() && is_vector_john_doe(value) && expected_type.is_vector()){
			return make_vector_value(expected_type.get_vector_element_type(), value.get_vector_value()->_elements);
		}
		else{
			return value;
		}
	}


	std::pair<interpreter_t, shared_ptr<value_t>> execute_statements(const interpreter_t& vm, const vector<shared_ptr<statement_t>>& statements);

/*
	bool compare_float_approx(float value, float expected){
		float diff = static_cast<float>(fabs(value - expected));
		return diff < 0.00001;
	}
*/

	interpreter_t begin_subenv(const interpreter_t& vm){
		QUARK_ASSERT(vm.check_invariant());

		auto vm2 = vm;

		auto parent_env = vm2._call_stack.back();
		auto new_environment = environment_t::make_environment(vm2, parent_env);
		vm2._call_stack.push_back(new_environment);

//		QUARK_TRACE(json_to_pretty_string(interpreter_to_json(vm2)));
		return vm2;
	}



	std::pair<interpreter_t, shared_ptr<value_t>> execute_statements_in_env(
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

	//	Output is the RETURN VALUE of the executed statement, if any.
	std::pair<interpreter_t, shared_ptr<value_t>> execute_statement(const interpreter_t& vm0, const statement_t& statement){
		QUARK_ASSERT(vm0.check_invariant());
		QUARK_ASSERT(statement.check_invariant());

		auto vm2 = vm0;

		if(statement._bind_or_assign){
			const auto& s = statement._bind_or_assign;
			const auto name = s->_new_variable_name;

			const auto result = evaluate_expression(vm2, s->_expression);
			vm2 = result.first;
			const auto result_value = result.second;

			if(result_value.is_literal() == false){
				throw std::runtime_error("Cannot evaluate expression.");
			}
			else{
				const auto bind_statement_type = s->_bindtype;
				const auto bind_statement_mutable_tag_flag = s->_bind_as_mutable_tag;

				const auto new_value = result_value.get_literal();

				//	If we have a type or we have the mutable-flag, then this statement is a bind.
				bool is_bind = bind_statement_type.is_null() == false || bind_statement_mutable_tag_flag;

				//	Bind.
				//	int a = 10
				//	mutable a = 10
				//	mutable = 10
				if(is_bind){
					const auto value_exists_in_env = vm2._call_stack.back()->_values.count(name) > 0;

					if(value_exists_in_env){
						throw std::runtime_error("Local identifier already exists.");
					}

					//	Deduced bind type -- use new value's type.
					if(bind_statement_type.is_null()){
						vm2._call_stack.back()->_values[name] = std::pair<value_t, bool>(new_value, bind_statement_mutable_tag_flag);
					}

					//	Explicit bind-type -- make sure source + dest types match.
					else{
						const auto retyped_value = improve_value_type(new_value, bind_statement_type);

						if(bind_statement_type != retyped_value.get_type()){
							throw std::runtime_error("Types not compatible in bind.");
						}
						vm2._call_stack.back()->_values[name] = std::pair<value_t, bool>(new_value, bind_statement_mutable_tag_flag);
					}
				}

				//	Assign
				//	a = 10
				else{
					const auto existing_value_deep_ptr = resolve_env_variable(vm2, name);
					const bool existing_variable_is_mutable = existing_value_deep_ptr && existing_value_deep_ptr->second;

					//	Mutate!
					if(existing_value_deep_ptr){
						if(existing_variable_is_mutable){
							const auto existing_value_type = existing_value_deep_ptr->first.get_type();
							if(existing_value_type != new_value.get_type()){
								throw std::runtime_error("Types not compatible in bind.");
							}
							*existing_value_deep_ptr = std::pair<value_t, bool>(new_value, existing_variable_is_mutable);
						}
						else{
							throw std::runtime_error("Cannot assign to immutable identifier.");
						}
					}

					//	Deduce type and bind it -- to local env.
					else{
						vm2._call_stack.back()->_values[name] = std::pair<value_t, bool>(new_value, false);
					}
				}
			}
			return { vm2, {}};
		}
		else if(statement._block){
			const auto& s = statement._block;
			return execute_statements_in_env(vm2, s->_statements, {});
		}
		else if(statement._return){
			const auto& s = statement._return;
			const auto expr = s->_expression;
			const auto result = evaluate_expression(vm2, expr);
			vm2 = result.first;
			const auto result_value = result.second;

			if(!result_value.is_literal()){
				throw std::runtime_error("undefined");
			}

			//??? Check that return value's type matches function's return type.
			return { vm2, make_shared<value_t>(result_value.get_literal()) };
		}


		//??? make structs unique even though they share layout and name. USer unique-ID-generator?
		else if(statement._def_struct){
			const auto& s = statement._def_struct;

			const auto name = s->_def._name;
			if(vm2._call_stack.back()->_values.count(name) > 0){
				throw std::runtime_error("Name already used.");
			}
			const auto struct_typeid = typeid_t::make_struct(std::make_shared<struct_definition_t>(s->_def));
			vm2._call_stack.back()->_values[name] = std::pair<value_t, bool>(make_typeid_value(struct_typeid), false);
			return { vm2, {}};
		}

		else if(statement._if){
			const auto& s = statement._if;
			const auto condition_result = evaluate_expression(vm2, s->_condition);
			vm2 = condition_result.first;
			const auto condition_result_value = condition_result.second;
			if(!condition_result_value.is_literal() || !condition_result_value.get_literal().is_bool()){
				throw std::runtime_error("Boolean condition required.");
			}

			bool r = condition_result_value.get_literal().get_bool_value();
			if(r){
				return execute_statements_in_env(vm2, s->_then_statements, {});
			}
			else{
				return execute_statements_in_env(vm2, s->_else_statements, {});
			}
		}
		else if(statement._for){
			const auto& s = statement._for;

			const auto start_value0 = evaluate_expression(vm2, s->_start_expression);
			vm2 = start_value0.first;
			const auto start_value = start_value0.second.get_literal().get_int_value();

			const auto end_value0 = evaluate_expression(vm2, s->_end_expression);
			vm2 = end_value0.first;
			const auto end_value = end_value0.second.get_literal().get_int_value();

			for(int x = start_value ; x <= end_value ; x++){
				const std::map<std::string, std::pair<value_t, bool>> values = { { s->_iterator_name, std::pair<value_t, bool>(value_t(x), false) } };
				const auto result = execute_statements_in_env(vm2, s->_body, values);
				vm2 = result.first;
				const auto return_value = result.second;
				if(return_value != nullptr){
					return { vm2, return_value };
				}
			}
			return { vm2, {} };
		}
		else if(statement._while){
			const auto& s = statement._while;

			bool again = true;
			while(again){
				const auto condition_value_expr = evaluate_expression(vm2, s->_condition);
				vm2 = condition_value_expr.first;
				const auto condition_value = condition_value_expr.second.get_literal().get_bool_value();

				if(condition_value){
					const auto result = execute_statements_in_env(vm2, s->_body, {});
					vm2 = result.first;
					const auto return_value = result.second;
					if(return_value != nullptr){
						return { vm2, return_value };
					}
				}
				else{
					again = false;
				}
			}
			return { vm2, {} };
		}
		else if(statement._expression){
			const auto& s = statement._expression;

			const auto result = evaluate_expression(vm2, s->_expression);
			vm2 = result.first;
			const auto result_value = result.second;
			return { vm2, {}};
		}
		else{
			QUARK_ASSERT(false);
		}
	}

	/*
		Return value:
			null = statements were all executed through.
			value = return statement returned a value.
	*/
	std::pair<interpreter_t, shared_ptr<value_t>> execute_statements(const interpreter_t& vm, const vector<shared_ptr<statement_t>>& statements){
		QUARK_ASSERT(vm.check_invariant());
		for(const auto i: statements){ QUARK_ASSERT(i->check_invariant()); };

		auto vm2 = vm;

		int statement_index = 0;
		while(statement_index < statements.size()){
			const auto statement = statements[statement_index];
			const auto& r = execute_statement(vm2, *statement);
			vm2 = r.first;
			if(r.second){
				return { vm2, r.second };
			}
			statement_index++;
		}
		return { vm2, {}};
	}


}	//	unnamed


value_t make_default_value(const interpreter_t& vm, const typeid_t& t){
	QUARK_ASSERT(vm.check_invariant());
	QUARK_ASSERT(t.check_invariant());

	const auto type = t.get_base_type();
	if(type == base_type::k_bool){
		return value_t(false);
	}
	else if(type == base_type::k_int){
		return value_t(0);
	}
	else if(type == base_type::k_float){
		return value_t(0.0f);
	}
	else if(type == base_type::k_string){
		return value_t("");
	}
	else if(type == base_type::k_struct){
		QUARK_ASSERT(false);
//		return make_struct_instance(vm, t);
	}
	else if(type == base_type::k_vector){
		QUARK_ASSERT(false);
	}
	else if(type == base_type::k_function){
		QUARK_ASSERT(false);
	}
	else{
		QUARK_ASSERT(false);
	}
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
		throw std::runtime_error("Undefined variable \"" + s + "\"!");
	}
	return t->first;
}

std::pair<interpreter_t, expression_t> evaluate_expression_internal(const interpreter_t& vm, const expression_t& e);

std::pair<interpreter_t, expression_t> evaluate_expression(const interpreter_t& vm, const expression_t& e){
	const auto result = evaluate_expression_internal(vm, e);
	if(result.second.is_literal()){
		const auto value2 = improve_value(result.second.get_literal());
		return { result.first, expression_t::make_literal(value2)};
	}
	else{
		return result;
	}
}
std::pair<interpreter_t, expression_t> evaluate_expression_internal(const interpreter_t& vm, const expression_t& e){
	QUARK_ASSERT(vm.check_invariant());
	QUARK_ASSERT(e.check_invariant());

	auto vm2 = vm;

	if(e.is_literal()){
		return {vm2, e};
	}

	const auto op = e.get_operation();

	if(op == expression_type::k_resolve_member){
		const auto expr = e.get_resolve_member();
		const auto parent_expr = evaluate_expression(vm2, *expr->_parent_address);
		vm2 = parent_expr.first;
		if(parent_expr.second.is_literal() && parent_expr.second.get_literal().is_struct()){
			const auto struct_instance = parent_expr.second.get_literal().get_struct_value();

			int index = find_struct_member_index(struct_instance->_def, expr->_member_name);
			if(index == -1){
				throw std::runtime_error("Unknown struct member \"" + expr->_member_name + "\".");
			}
			const value_t value = struct_instance->_member_values[index];
			return { vm2, expression_t::make_literal(value)};
		}
		else{
			throw std::runtime_error("Resolve member failed.");
		}
	}
	else if(op == expression_type::k_lookup_element){
		const auto expr = e.get_lookup();
		const auto parent_expr = evaluate_expression(vm2, *expr->_parent_address);
		vm2 = parent_expr.first;

		if(parent_expr.second.is_literal() == false){
			throw std::runtime_error("Cannot compute lookup parent.");
		}
		else{
			const auto key_expr = evaluate_expression(vm2, *expr->_lookup_key);
			vm2 = key_expr.first;

			if(key_expr.second.is_literal() == false){
				throw std::runtime_error("Cannot compute lookup key.");
			}
			else{
				if(parent_expr.second.get_literal().is_vector()){
					if(key_expr.second.get_literal().is_int() == false){
						throw std::runtime_error("Lookup in vector by index-only.");
					}
					else{
						const auto instance = parent_expr.second.get_literal().get_vector_value();

						int lookup_index = key_expr.second.get_literal().get_int_value();
						if(lookup_index < 0 || lookup_index >= instance->_elements.size()){
							throw std::runtime_error("Lookup in vector: out of bounds.");
						}
						else{
							const value_t value = instance->_elements[lookup_index];
							return { vm2, expression_t::make_literal(value)};
						}
					}
				}
				else if(parent_expr.second.get_literal().is_dict()){
					if(key_expr.second.get_literal().is_string() == false){
						throw std::runtime_error("Lookup in dict by string-only.");
					}
					else{
						const auto instance = parent_expr.second.get_literal().get_dict_value();
						const string key = key_expr.second.get_literal().get_string_value();

						const auto found_it = instance->_elements.find(key);
						if(found_it == instance->_elements.end()){
							throw std::runtime_error("Lookup in dict: key not found.");
						}
						else{
							const value_t value = found_it->second;
							return { vm2, expression_t::make_literal(value)};
						}
					}
				}
				else {
					throw std::runtime_error("Can only lookup inside vectors and dicts using [].");
				}
			}
		}
	}
	else if(op == expression_type::k_variable){
		const auto expr = e.get_variable();

		const auto value = resolve_env_variable(vm2, expr->_variable);
		if(value != nullptr){
			return {vm2, expression_t::make_literal(value->first)};
		}
		else{
			//??? Hack to make "vector(int)" work.
			if(expr->_variable == keyword_t::k_bool){
				return {vm2, expression_t::make_literal(make_typeid_value(typeid_t::make_bool()))};
			}
			else if(expr->_variable == keyword_t::k_int){
				return {vm2, expression_t::make_literal(make_typeid_value(typeid_t::make_int()))};
			}
			else if(expr->_variable == keyword_t::k_float){
				return {vm2, expression_t::make_literal(make_typeid_value(typeid_t::make_float()))};
			}
			else if(expr->_variable == keyword_t::k_string){
				return {vm2, expression_t::make_literal(make_typeid_value(typeid_t::make_string()))};
			}

			else {
				throw std::runtime_error("Undefined variable \"" + expr->_variable + "\"!");
			}
		}
	}

	else if(op == expression_type::k_call){
		return evaluate_call_expression(vm2, e);
	}

	else if(op == expression_type::k_define_function){
		const auto expr = e.get_function_definition();
		return {vm2, expression_t::make_literal(make_function_value(expr->_def))};
	}

	else if(op == expression_type::k_vector_definition){
		const auto expr = e.get_vector_definition();

		const std::vector<expression_t>& elements = expr->_elements;
		std::vector<value_t> elements2;

		typeid_t element_type = expr->_element_type;

		for(const auto m: elements){
			const auto element_expr = evaluate_expression(vm2, m);
			vm2 = element_expr.first;
			if(element_expr.second.is_literal() == false){
				throw std::runtime_error("Cannot evaluate element of vector definition!");
			}

			const auto element = element_expr.second.get_literal();

			//??? Solved already,using improve_vector()?
			//	Grab vector type from element(s). This is a little fuzzy. Should probably make sure all elements are the same type.
			if(element_type.is_null()){
				element_type = element.get_type();
			}

			elements2.push_back(element);
		}

		for(const auto m: elements2){
			if((m.get_type() == element_type) == false){
				throw std::runtime_error("Vector can not hold elements of different type!");
			}
		}
		return {vm2, expression_t::make_literal(make_vector_value(element_type, elements2))};
	}

	else if(op == expression_type::k_dict_definition){
		const auto expr = e.get_dict_definition();

		const auto& elements = expr->_elements;
		std::map<string, value_t> elements2;

		typeid_t value_type = expr->_value_type;

		for(const auto m: elements){
			const auto element_expr = evaluate_expression(vm2, m.second);
			vm2 = element_expr.first;

			if(element_expr.second.is_literal() == false){
				throw std::runtime_error("Cannot evaluate element of vector definition!");
			}

			const auto element = element_expr.second.get_literal();

			//??? Solved already,using improve_vector()?
			//	Grab vector type from element(s). This is a little fuzzy. Should probably make sure all elements are the same type.
			if(value_type.is_null()){
				value_type = element.get_type();
			}

			const string key_string = m.first;
			elements2[key_string] = element;
		}

		for(const auto m: elements2){
			if((m.second.get_type() == value_type) == false){
				throw std::runtime_error("Vector can not hold elements of different type!");
			}
		}
		return {vm2, expression_t::make_literal(make_dict_value(value_type, elements2))};
	}

	//	This can be desugared at compile time.
	else if(op == expression_type::k_arithmetic_unary_minus__1){
		const auto expr = e.get_unary_minus();
		const auto& expr2 = evaluate_expression(vm2, *expr->_expr);
		vm2 = expr2.first;

		if(expr2.second.is_literal()){
			const auto& c = expr2.second.get_literal();
			vm2 = expr2.first;

			if(c.is_int()){
				return evaluate_expression(
					vm2,
					expression_t::make_simple_expression__2(expression_type::k_arithmetic_subtract__2, expression_t::make_literal_int(0), expr2.second)
				);
			}
			else if(c.is_float()){
				return evaluate_expression(
					vm2,
					expression_t::make_simple_expression__2(expression_type::k_arithmetic_subtract__2, expression_t::make_literal_float(0.0f), expr2.second)
				);
			}
			else{
				throw std::runtime_error("Unary minus won't work on expressions of type \"" + json_to_compact_string(typeid_to_normalized_json(c.get_type())) + "\".");
			}
		}
		else{
			throw std::runtime_error("Could not evaluate condition in conditional expression.");
		}
	}

	//	Special-case since it uses 3 expressions & uses shortcut evaluation.
	else if(op == expression_type::k_conditional_operator3){
		const auto expr = e.get_conditional();
		const auto cond_result = evaluate_expression(vm2, *expr->_condition);
		vm2 = cond_result.first;

		if(cond_result.second.is_literal() && cond_result.second.get_literal().is_bool()){
			const bool cond_flag = cond_result.second.get_literal().get_bool_value();

			//	!!! Only evaluate the CHOSEN expression. Not that importan since functions are pure.
			if(cond_flag){
				return evaluate_expression(vm2, *expr->_a);
			}
			else{
				return evaluate_expression(vm2, *expr->_b);
			}
		}
		else{
			throw std::runtime_error("Could not evaluate condition in conditional expression.");
		}
	}

	//	First evaluate all inputs to our operation.
	const auto simple2_expr = e.get_simple__2();
	const auto left_expr = evaluate_expression(vm2, *simple2_expr->_left);
	vm2 = left_expr.first;
	const auto right_expr = evaluate_expression(vm2, *simple2_expr->_right);
	vm2 = right_expr.first;


	//	Both left and right are constants, replace the math_operation with a constant!
	if(left_expr.second.is_literal() && right_expr.second.is_literal()) {

		//	Perform math operation on the two constants => new constant.
		const auto left_constant = left_expr.second.get_literal();

		//	Make rhs match left if needed/possible.
		const auto right_constant = improve_value_type(right_expr.second.get_literal(), left_constant.get_type());

		if(!(left_constant.get_type()== right_constant.get_type())){
			throw std::runtime_error("Left and right expressions must be same type!");
		}

		//	Do generic functionalliy, independant on types?
		{
			if(op == expression_type::k_comparison_smaller_or_equal__2){
				long diff = value_t::compare_value_true_deep(left_constant, right_constant);
				return {vm2, expression_t::make_literal_bool(diff <= 0)};
			}
			else if(op == expression_type::k_comparison_smaller__2){
				long diff = value_t::compare_value_true_deep(left_constant, right_constant);
				return {vm2, expression_t::make_literal_bool(diff < 0)};
			}
			else if(op == expression_type::k_comparison_larger_or_equal__2){
				long diff = value_t::compare_value_true_deep(left_constant, right_constant);
				return {vm2, expression_t::make_literal_bool(diff >= 0)};
			}
			else if(op == expression_type::k_comparison_larger__2){
				long diff = value_t::compare_value_true_deep(left_constant, right_constant);
				return {vm2, expression_t::make_literal_bool(diff > 0)};
			}


			else if(op == expression_type::k_logical_equal__2){
				long diff = value_t::compare_value_true_deep(left_constant, right_constant);
				return {vm2, expression_t::make_literal_bool(diff == 0)};
			}
			else if(op == expression_type::k_logical_nonequal__2){
				long diff = value_t::compare_value_true_deep(left_constant, right_constant);
				return {vm2, expression_t::make_literal_bool(diff != 0)};
			}
		}

		//	bool
		if(left_constant.is_bool() && right_constant.is_bool()){
			const bool left = left_constant.get_bool_value();
			const bool right = right_constant.get_bool_value();

			if(op == expression_type::k_arithmetic_add__2
			|| op == expression_type::k_arithmetic_subtract__2
			|| op == expression_type::k_arithmetic_multiply__2
			|| op == expression_type::k_arithmetic_divide__2
			|| op == expression_type::k_arithmetic_remainder__2
			){
				throw std::runtime_error("Arithmetics on bool not allowed.");
			}

			else if(op == expression_type::k_logical_and__2){
				return {vm2, expression_t::make_literal_bool(left && right)};
			}
			else if(op == expression_type::k_logical_or__2){
				return {vm2, expression_t::make_literal_bool(left || right)};
			}
			else{
				QUARK_ASSERT(false);
			}
		}

		//	int
		else if(left_constant.is_int() && right_constant.is_int()){
			const int left = left_constant.get_int_value();
			const int right = right_constant.get_int_value();

			if(op == expression_type::k_arithmetic_add__2){
				return {vm2, expression_t::make_literal_int(left + right)};
			}
			else if(op == expression_type::k_arithmetic_subtract__2){
				return {vm2, expression_t::make_literal_int(left - right)};
			}
			else if(op == expression_type::k_arithmetic_multiply__2){
				return {vm2, expression_t::make_literal_int(left * right)};
			}
			else if(op == expression_type::k_arithmetic_divide__2){
				if(right == 0){
					throw std::runtime_error("EEE_DIVIDE_BY_ZERO");
				}
				return {vm2, expression_t::make_literal_int(left / right)};
			}
			else if(op == expression_type::k_arithmetic_remainder__2){
				if(right == 0){
					throw std::runtime_error("EEE_DIVIDE_BY_ZERO");
				}
				return {vm2, expression_t::make_literal_int(left % right)};
			}

			else if(op == expression_type::k_logical_and__2){
				return {vm2, expression_t::make_literal_bool((left != 0) && (right != 0))};
			}
			else if(op == expression_type::k_logical_or__2){
				return {vm2, expression_t::make_literal_bool((left != 0) || (right != 0))};
			}
			else{
				QUARK_ASSERT(false);
			}
		}

		//	float
		else if(left_constant.is_float() && right_constant.is_float()){
			const float left = left_constant.get_float_value();
			const float right = right_constant.get_float_value();

			if(op == expression_type::k_arithmetic_add__2){
				return {vm2, expression_t::make_literal_float(left + right)};
			}
			else if(op == expression_type::k_arithmetic_subtract__2){
				return {vm2, expression_t::make_literal_float(left - right)};
			}
			else if(op == expression_type::k_arithmetic_multiply__2){
				return {vm2, expression_t::make_literal_float(left * right)};
			}
			else if(op == expression_type::k_arithmetic_divide__2){
				if(right == 0.0f){
					throw std::runtime_error("EEE_DIVIDE_BY_ZERO");
				}
				return {vm2, expression_t::make_literal_float(left / right)};
			}
			else if(op == expression_type::k_arithmetic_remainder__2){
				throw std::runtime_error("Modulo operation on float not allowed.");
			}


			else if(op == expression_type::k_logical_and__2){
				return {vm2, expression_t::make_literal_bool((left != 0.0f) && (right != 0.0f))};
			}
			else if(op == expression_type::k_logical_or__2){
				return {vm2, expression_t::make_literal_bool((left != 0.0f) || (right != 0.0f))};
			}
			else{
				QUARK_ASSERT(false);
			}
		}

		//	string
		else if(left_constant.is_string() && right_constant.is_string()){
			const auto left = left_constant.get_string_value();
			const auto right = right_constant.get_string_value();

			if(op == expression_type::k_arithmetic_add__2){
				return {vm2, expression_t::make_literal_string(left + right)};
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
			}
		}

		//	struct
		else if(left_constant.is_struct() && right_constant.is_struct()){
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
			}
		}

		else if(left_constant.is_vector() && right_constant.is_vector()){
			//	Improves vectors before using them.
			const auto element_type = left_constant.get_type().get_vector_element_type();

			if(!(left_constant.get_type() == right_constant.get_type())){
				throw std::runtime_error("Vector types don't match.");
			}
			else{
				if(op == expression_type::k_arithmetic_add__2){
					auto elements2 = left_constant.get_vector_value()->_elements;
					elements2.insert(elements2.end(), right_constant.get_vector_value()->_elements.begin(), right_constant.get_vector_value()->_elements.end());

					const auto value2 = make_vector_value(element_type, elements2);
					return {vm2, expression_t::make_literal(value2)};
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
				}
			}
		}
		else if(left_constant.is_function() && right_constant.is_function()){
			throw std::runtime_error("Cannot perform operations on two function values.");
		}
		else{
			throw std::runtime_error("Arithmetics failed.");
		}
	}

	//	Else use a math_operation expression to perform the calculation later.
	//	We make a NEW math_operation since sub-nodes may have been evaluated.
	else{
		return {vm2, expression_t::make_simple_expression__2(op, left_expr.second, right_expr.second)};
	}
}


std::pair<interpreter_t, std::shared_ptr<value_t>> call_function(const interpreter_t& vm, const floyd::value_t& f, const vector<value_t>& args){
	QUARK_ASSERT(vm.check_invariant());
	QUARK_ASSERT(f.check_invariant());
	for(const auto i: args){ QUARK_ASSERT(i.check_invariant()); };

	auto vm2 = vm;

	///??? COMPARE arg count and types.

	if(f.is_function() == false){
		throw std::runtime_error("Cannot call non-function.");
	}

	const auto& function_def = f.get_function_value()->_def;
	if(function_def._host_function != 0){
		const auto r = vm2.call_host_function(function_def._host_function, args);
		return { r.first, make_shared<value_t>(r.second) };
	}
	else{

		//	Always use global scope.
		//	Future: support closures by linking to function env where function is defined.
		auto parent_env = vm2._call_stack[0];
		auto new_environment = environment_t::make_environment(vm2, parent_env);

		//	Copy input arguments to the function scope.
		for(int i = 0 ; i < function_def._args.size() ; i++){
			const auto& arg_name = function_def._args[i]._name;
			const auto& arg_value = args[i];

			//	Function arguments are immutable while executing the function body.
			new_environment->_values[arg_name] = std::pair<value_t, bool>(arg_value, false);
		}
		vm2._call_stack.push_back(new_environment);

//		QUARK_TRACE(json_to_pretty_string(interpreter_to_json(vm2)));

		const auto r = execute_statements(vm2, function_def._statements);

		vm2 = r.first;
		vm2._call_stack.pop_back();

		if(!r.second){
			throw std::runtime_error("function missing return statement");
		}
		else{
			return {vm2, r.second };
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

std::pair<interpreter_t, value_t> construct_struct(const interpreter_t& vm, const typeid_t& struct_type, const vector<value_t>& values){
	QUARK_ASSERT(struct_type.get_base_type() == base_type::k_struct);
	QUARK_SCOPED_TRACE("construct_struct()");
	QUARK_TRACE("struct_type: " + typeid_to_compact_string(struct_type));

	const auto& def = struct_type.get_struct();
	if(values.size() != def._members.size()){
		throw std::runtime_error(
			 string() + "Calling constructor for \"" + def._name + "\" with " + std::to_string(values.size()) + " arguments, " + std::to_string(def._members.size()) + " + required."
		);
	}

	//??? check types of members.

	for(const auto e: values){
		QUARK_ASSERT(e.check_invariant());
		QUARK_ASSERT(e.get_type().get_base_type() != base_type::k_unknown_identifier);
	}

	const auto instance = make_struct_value(struct_type, def, values);
	QUARK_TRACE(instance.to_compact_string());

	return std::pair<interpreter_t, value_t>(vm, instance);
}

std::pair<interpreter_t, expression_t> evaluate_call_expression(const interpreter_t& vm, const expression_t& e){
	QUARK_ASSERT(vm.check_invariant());
	QUARK_ASSERT(e.check_invariant());
	QUARK_ASSERT(e.get_operation() == expression_type::k_call);

	auto vm2 = vm;
	const auto call = e.get_function_call();
	QUARK_ASSERT(call);

	//	Simplify each input expression: expression[0]: which function to call, expression[1]: first argument if any.
	const auto function = evaluate_expression(vm2, *call->_function);
	vm2 = function.first;
	vector<expression_t> args2;
	for(int i = 0 ; i < call->_args.size() ; i++){
		const auto t = evaluate_expression(vm2, call->_args[i]);
		vm2 = t.first;
		args2.push_back(t.second);
	}

	//	If not all input expressions could be evaluated, return a (maybe simplified) expression.
	if(function.second.is_literal() == false || all_literals(args2) == false){
		return {vm2, expression_t::make_function_call(function.second, args2)};
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
			const auto typeid_contained_type = function_value.get_typeid_value();
			if(typeid_contained_type.get_base_type() == base_type::k_struct){
				//	Constructor.
				const auto result = construct_struct(vm2, typeid_contained_type, arg_values);
				vm2 = result.first;
				return { vm2, expression_t::make_literal(result.second)};
			}
			else{
				throw std::runtime_error("Cannot call non-function.");
			}
		}
		else{
			throw std::runtime_error("Cannot call non-function.");
		}
	}

	//	Call function-value.
	else{
		vector<value_t> args3;
		for(const auto& i: args2){
			args3.push_back(i.get_literal());
		}

		const auto& result = call_function(vm2, function_value, args3);
		vm2 = result.first;
		return { vm2, expression_t::make_literal(*result.second)};
	}
}



json_t interpreter_to_json(const interpreter_t& vm){
	vector<json_t> callstack;
	for(const auto& e: vm._call_stack){
		std::map<string, json_t> values;
		for(const auto&v: e->_values){
		//??? INlcude mutable-flag?
			values[v.first] = value_to_json(v.second.first);
		}

		const auto& env = json_t::make_object({
//			{ "parent_env", e->_parent_env ? e->_parent_env->_object_id : json_t() },
//			{ "object_id", json_t(double(e->_object_id)) },
			{ "values", values }
		});
		callstack.push_back(env);
	}

	return json_t::make_object({
		{ "ast", ast_to_json(vm._ast) },
		{ "callstack", json_t::make_array(callstack) }
	});
}



void test__evaluate_expression(const expression_t& e, const expression_t& expected){
	const ast_t ast;
	const interpreter_t interpreter(ast);
	const auto e3 = evaluate_expression(interpreter, e);

	ut_compare_jsons(expression_to_json(e3.second), expression_to_json(expected));
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



//////////////////////////		interpreter_t




//	Records all output to interpreter
std::pair<interpreter_t, value_t> host__print(const interpreter_t& vm, const std::vector<value_t>& args){
	QUARK_ASSERT(vm.check_invariant());

	if(args.size() != 1){
		throw std::runtime_error("assert() requires 1 argument!");
	}

	auto vm2 = vm;
	const auto& value = args[0];
	const auto s = value.to_compact_string();
	printf("%s\n", s.c_str());

	vm2._print_output.push_back(s);
	return {vm2, value_t() };
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
	return {vm2, value_t() };
}

//	string to_string(value_t)
std::pair<interpreter_t, value_t> host__to_string(const interpreter_t& vm, const std::vector<value_t>& args){
	QUARK_ASSERT(vm.check_invariant());

	if(args.size() != 1){
		throw std::runtime_error("to_string() requires 1 argument!");
	}

	const auto& value = args[0];
	const auto a = value.to_compact_string();
	return {vm, value_t(a) };
}



/*
    std::chrono::time_point<std::chrono::system_clock> start, end;
    start = std::chrono::system_clock::now();
    std::cout << "f(42) = " << fibonacci(42) << '\n';
    end = std::chrono::system_clock::now();
 
    std::chrono::duration<double> elapsed_seconds = end-start;
    std::time_t end_time = std::chrono::system_clock::to_time_t(end);
 
    std::cout << "finished computation at " << std::ctime(&end_time)
              << "elapsed time: " << elapsed_seconds.count() << "s\n";
*/



#if 0
	uint64_t get_time_of_day_ms(){
		std::chrono::time_point<std::chrono::high_resolution_clock> t = std::chrono::high_resolution_clock::now();
		std::chrono::duration<double> elapsed_seconds = t - start;
		std::time_t sec = std::chrono::high_resolution_clock::to_time_t(t);
		return sec * 1000.0;
/*
		timeval time;
		gettimeofday(&time, NULL);


	//	QUARK_ASSERT(sizeof(int) == sizeof(int64_t));
		int64_t ms = (time.tv_sec * 1000) + (time.tv_usec / 1000);
		return ms;
*/
	}
#endif

std::pair<interpreter_t, value_t> host__get_time_of_day(const interpreter_t& vm, const std::vector<value_t>& args){
	QUARK_ASSERT(vm.check_invariant());

	if(args.size() != 0){
		throw std::runtime_error("get_time_of_day() requires 0 arguments!");
	}

	std::chrono::time_point<std::chrono::high_resolution_clock> t = std::chrono::high_resolution_clock::now();
	std::chrono::duration<double> elapsed_seconds = t - vm._start_time;
	const auto ms = elapsed_seconds.count() * 1000.0;
	return {vm, value_t(int(ms)) };
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
	if(type.get_base_type() == base_type::k_unknown_identifier){
		const auto v = resolve_env_variable(vm, type.get_unknown_identifier());
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

		auto s2 = make_struct_value(struct_typeid, def, values2);
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

	const auto& obj = args[0];
	const auto& lookup_key = args[1];
	const auto& new_value = args[2];

	if(args.size() != 3){
		throw std::runtime_error("update() needs 3 arguments.");
	}
	else{
		if(obj.is_struct()){
			if(lookup_key.is_string() == false){
				throw std::runtime_error("You must specify structure member using string.");
			}
			else{
				//### Does simple check for "." -- we should use vector of strings instead.
				const auto nodes = split_on_chars(seq_t(lookup_key.get_string_value()), ".");
				if(nodes.empty()){
					throw std::runtime_error("You must specify structure member using string.");
				}

				const auto s2 = update_struct_member_deep(vm, obj, nodes, new_value);
				return {vm, s2 };
			}
		}
		else if(obj.is_vector()){
			if(lookup_key.is_int() == false){
				throw std::runtime_error("Vector lookup using integer index only.");
			}
			else{
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
						const auto s2 = make_vector_value(v->_element_type, v2);
						return {vm, s2 };
					}
				}
			}
		}
		else if(obj.is_string()){
			if(lookup_key.is_int() == false){
				throw std::runtime_error("String lookup using integer index only.");
			}
			else{
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
						const auto s2 = value_t(v2);
						return {vm, s2 };
					}
				}
			}
		}
		else {
			throw std::runtime_error("Can only update struct, vector or string.");
		}
	}
}



//		vector(int)
//		vector(int, 1, 2, 3)
std::pair<interpreter_t, value_t> host__vector(const interpreter_t& vm, const std::vector<value_t>& args){
	QUARK_ASSERT(vm.check_invariant());

	QUARK_TRACE(json_to_pretty_string(interpreter_to_json(vm)));

	if(args.size() == 0){
		throw std::runtime_error("vector() requires minimum one argument.");
	}

	if(args[0].is_typeid() == false){
		throw std::runtime_error("vector() first argument must be the type of elements in the vector.");
	}
	const auto element_type = args[0].get_typeid_value();

	for(int i = 1 ; i < args.size() ; i++){
		if((args[i].get_type() == element_type) == false){
			throw std::runtime_error("vector() element wrong type.");
		}
	}

	const auto elements = vector<value_t>(args.begin() + 1, args.end());
	const auto v = make_vector_value(element_type, elements);
	return {vm, v};
}

std::pair<interpreter_t, value_t> host__size(const interpreter_t& vm, const std::vector<value_t>& args){
	QUARK_ASSERT(vm.check_invariant());

	if(args.size() != 1){
		throw std::runtime_error("find requires 2 arguments");
	}

	const auto obj = args[0];
	if(obj.is_vector()){
		const auto size = obj.get_vector_value()->_elements.size();
		return {vm, value_t(static_cast<int>(size))};
	}
	else if(obj.is_dict()){
		const auto size = obj.get_dict_value()->_elements.size();
		return {vm, value_t(static_cast<int>(size))};
	}
	else if(obj.is_string()){
		const auto size = obj.get_string_value().size();
		return {vm, value_t(static_cast<int>(size))};
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
	if(obj.is_vector()){
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
			return {vm, value_t(static_cast<int>(-1))};
		}
		else{
			return {vm, value_t(static_cast<int>(index))};
		}
	}
	else if(obj.is_string()){
		const auto str = obj.get_string_value();
		const auto wanted2 = wanted.get_string_value();

		const auto r = str.find(wanted2);
		if(r == std::string::npos){
			return {vm, value_t(static_cast<int>(-1))};
		}
		else{
			return {vm, value_t(static_cast<int>(r))};
		}
	}
	else{
		throw std::runtime_error("Calling find() on unsupported type of value.");
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
	if(obj.is_vector()){
		const auto vec = obj.get_vector_value();
		if(element.get_type() != vec->_element_type){
			throw std::runtime_error("Type mismatch.");
		}
		auto elements2 = vec->_elements;
		elements2.push_back(element);
		const auto v = make_vector_value(vec->_element_type, elements2);
		return {vm, v};
	}
	else if(obj.is_string()){
		const auto str = obj.get_string_value();
		const auto ch = element.get_string_value();

		auto str2 = str + ch;
		const auto v = value_t(str2);
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

	if(obj.is_vector()){
		const auto vec = obj.get_vector_value();
		const auto start2 = std::min(start, static_cast<int>(vec->_elements.size()));
		const auto end2 = std::min(end, static_cast<int>(vec->_elements.size()));
		vector<value_t> elements2;
		for(int i = start2 ; i < end2 ; i++){
			elements2.push_back(vec->_elements[i]);
		}
		const auto v = make_vector_value(vec->_element_type, elements2);
		return {vm, v};
	}
	else if(obj.is_string()){
		const auto str = obj.get_string_value();
		const auto start2 = std::min(start, static_cast<int>(str.size()));
		const auto end2 = std::min(end, static_cast<int>(str.size()));

		string str2;
		for(int i = start2 ; i < end2 ; i++){
			str2.push_back(str[i]);
		}
		const auto v = value_t(str2);
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

	if(obj.is_vector()){
		const auto vec = obj.get_vector_value();
		const auto start2 = std::min(start, static_cast<int>(vec->_elements.size()));
		const auto end2 = std::min(end, static_cast<int>(vec->_elements.size()));
		const auto new_bits = args[3].get_vector_value();


		const auto string_left = vector<value_t>(vec->_elements.begin(), vec->_elements.begin() + start2);
		const auto string_right = vector<value_t>(vec->_elements.begin() + end2, vec->_elements.end());

		auto result = string_left;
		result.insert(result.end(), new_bits->_elements.begin(), new_bits->_elements.end());
		result.insert(result.end(), string_right.begin(), string_right.end());

		const auto v = make_vector_value(vec->_element_type, result);
		return {vm, v};
	}
	else if(obj.is_string()){
		const auto str = obj.get_string_value();
		const auto start2 = std::min(start, static_cast<int>(str.size()));
		const auto end2 = std::min(end, static_cast<int>(str.size()));
		const auto new_bits = args[3].get_string_value();

		string str2 = str.substr(0, start2) + new_bits + str.substr(end2);
		const auto v = value_t(str2);
		return {vm, v};
	}
	else{
		throw std::runtime_error("Calling replace() on unsupported type of value.");
	}
}





typedef std::pair<interpreter_t, value_t> (*HOST_FUNCTION_PTR)(const interpreter_t& vm, const std::vector<value_t>& args);

struct host_function_t {
	std::string _name;
	HOST_FUNCTION_PTR _function_ptr;
	typeid_t _function_type;
};

const vector<host_function_t> k_host_functions {
	host_function_t{ "print", host__print, typeid_t::make_function(typeid_t::make_null(), {}) },
	host_function_t{ "assert", host__assert, typeid_t::make_function(typeid_t::make_null(), {}) },
	host_function_t{ "to_string", host__to_string, typeid_t::make_function(typeid_t::make_string(), {}) },
	host_function_t{ "get_time_of_day", host__get_time_of_day, typeid_t::make_function(typeid_t::make_int(), {}) },
	host_function_t{ "update", host__update, typeid_t::make_function(typeid_t::make_null(), {}) },
	host_function_t{ "vector", host__vector, typeid_t::make_function(typeid_t::make_null(), {}) },
	host_function_t{ "size", host__size, typeid_t::make_function(typeid_t::make_null(), {}) },
	host_function_t{ "find", host__find, typeid_t::make_function(typeid_t::make_int(), {}) },
	host_function_t{ "push_back", host__push_back, typeid_t::make_function(typeid_t::make_null(), {}) },
	host_function_t{ "subset", host__subset, typeid_t::make_function(typeid_t::make_null(), {}) },
	host_function_t{ "replace", host__replace, typeid_t::make_function(typeid_t::make_null(), {}) }
};





std::pair<interpreter_t, floyd::value_t> interpreter_t::call_host_function(int function_id, const std::vector<floyd::value_t> args) const{
	const int index = function_id - 1000;
	QUARK_ASSERT(index >= 0 && index < k_host_functions.size())

	const auto& host_function = k_host_functions[index];
	return (host_function._function_ptr)(*this, args);
}


interpreter_t::interpreter_t(const ast_t& ast){
	QUARK_ASSERT(ast.check_invariant());

	std::vector<std::shared_ptr<statement_t>> init_statements;

	//	Insert built-in functions into AST.
	for(auto i = 0 ; i < k_host_functions.size() ; i++){
		const auto& hf = k_host_functions[i];
		const auto def = function_definition_t(
			{},
			i + 1000,
			hf._function_type.get_function_return()
		);

		init_statements.push_back(
			make_shared<statement_t>(make_function_statement(hf._name, def))
		);
	}

	_ast = ast_t(init_statements + ast._statements);


	//	Make the top-level environment = global scope.
	shared_ptr<environment_t> empty_env;
	auto global_env = environment_t::make_environment(*this, empty_env);

	_call_stack.push_back(global_env);

	_start_time = std::chrono::high_resolution_clock::now();

	//	Run static intialization (basically run global statements before calling main()).
	const auto r = execute_statements(*this, _ast._statements);
	if(r.second){
		throw std::runtime_error("Return statement illegal in global scope.");
	}

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


bool interpreter_t::check_invariant() const {
	QUARK_ASSERT(_ast.check_invariant());
	return true;
}






ast_t program_to_ast2(const string& program){
	const auto pass1 = floyd::parse_program2(program);
	const auto pass2 = run_pass2(pass1);
	trace(pass2);
	return pass2;
}


interpreter_t run_global(const string& source){
	auto ast = program_to_ast2(source);
	auto vm = interpreter_t(ast);
//	QUARK_TRACE(json_to_pretty_string(interpreter_to_json(vm)));
	return vm;
}

std::pair<interpreter_t, floyd::value_t> run_main(const string& source, const vector<floyd::value_t>& args){
	auto ast = program_to_ast2(source);
	auto vm = interpreter_t(ast);
	const auto f = find_global_symbol(vm, "main");
	const auto r = call_function(vm, f, args);
	return { r.first, *r.second };
}




}	//	floyd

