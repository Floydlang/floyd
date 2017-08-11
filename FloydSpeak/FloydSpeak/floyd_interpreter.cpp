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
#include "pass3.h"
#include "json_support.h"

#include <cmath>

namespace floyd_interpreter {


using std::vector;
using std::string;
using std::pair;
using std::shared_ptr;
using std::unique_ptr;
using std::make_shared;

using namespace floyd_ast;


expression_t evaluate_call_expression(const interpreter_t& vm, const expression_t& e);
value_t make_struct_instance(const interpreter_t& vm, const typeid_t& struct_type);

namespace {


	bool compare_float_approx(float value, float expected){
		float diff = static_cast<float>(fabs(value - expected));
		return diff < 0.00001;
	}

	bool check_arg_types(const std::shared_ptr<const floyd_ast::lexical_scope_t>& f, const vector<value_t>& args){
		if(f->_args.size() != args.size()){
			return false;
		}

		for(int i = 0 ; i < args.size() ; i++){
			const auto farg = f->_args[i]._type;
			const auto call_arg = args[i].get_type();
			if(!(farg == call_arg)){
				return false;
			}
		}
		return true;
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

			if(statement->_bind){
				const auto& s = statement->_bind;
				const auto name = s->_new_variable_name;
				if(vm2._call_stack.back()->_values.count(name) != 0){
					throw std::runtime_error("local constant already exists");
				}
				const auto result = evaluate_expression(vm2, s->_expression);
				if(!result.is_constant()){
					throw std::runtime_error("unknown variables");
				}
				vm2._call_stack.back()->_values[name] = result.get_constant();
			}
			else if(statement->_return){
				const auto& s = statement->_return;
				const auto expr = s->_expression;
				const auto result = evaluate_expression(vm2, expr);

				if(!result.is_constant()){
					throw std::runtime_error("undefined");
				}

				return std::pair<interpreter_t, shared_ptr<value_t>>{ vm2, make_shared<value_t>(result.get_constant()) };
			}
			else{
				QUARK_ASSERT(false);
			}
			statement_index++;
		}
		return std::pair<interpreter_t, shared_ptr<value_t>>{ vm2, {}};
	}

}	//	unnamed


value_t make_default_value(const interpreter_t& vm, const typeid_t& t){
	QUARK_ASSERT(vm.check_invariant());
	QUARK_ASSERT(t.check_invariant());

	const auto type = t.get_base_type();
	if(type == floyd_basics::base_type::k_bool){
		return value_t(false);
	}
	else if(type == floyd_basics::base_type::k_int){
		return value_t(0);
	}
	else if(type == floyd_basics::base_type::k_float){
		return value_t(0.0f);
	}
	else if(type == floyd_basics::base_type::k_string){
		return value_t("");
	}
	else if(type == floyd_basics::base_type::k_struct){
		return make_struct_instance(vm, t);
	}
	else if(type == floyd_basics::base_type::k_vector){
		QUARK_ASSERT(false);
	}
	else if(type == floyd_basics::base_type::k_function){
		QUARK_ASSERT(false);
	}
	else{
		QUARK_ASSERT(false);
	}
}

value_t make_struct_instance(const interpreter_t& vm, const typeid_t& struct_type){
	QUARK_ASSERT(vm.check_invariant());
	QUARK_ASSERT(!struct_type.is_null() && struct_type.check_invariant());

	return value_t();
/*
	const auto k = struct_type.to_string();
	const auto& objects = vm._ast.get_objects();
	const auto struct_def_it = objects.find(k);
	if(struct_def_it == objects.end()){
		throw std::runtime_error("Undefined struct type!");
	}

	const auto struct_def = struct_def_it->second;

	std::map<std::string, value_t> member_values;
	for(int i = 0 ; i < struct_def->_members.size() ; i++){
		const auto& member_def = struct_def->_members[i];

		const auto member_type = member_def._type;
		if(!member_type.is_null()){
			throw std::runtime_error("Undefined struct type!");
		}

		//	If there is an initial value for this member, use that. Else use default value for this type.
		value_t value;
		if(member_def._value){
			value = *member_def._value;
		}
		else{
			value = make_default_value(vm, member_def._type);
		}
		member_values[member_def._name] = value;
	}
	auto instance = make_shared<struct_instance_t>(struct_instance_t(struct_type, member_values));
	return value_t(instance);
*/
}

//	const auto it = find_if(s._state.begin(), s._state.end(), [&] (const member_t& e) { return e._name == name; } );

floyd_ast::value_t resolve_env_variable_deep(const interpreter_t& vm, const shared_ptr<environment_t>& env, const std::string& s){
	QUARK_ASSERT(vm.check_invariant());
	QUARK_ASSERT(env && env->check_invariant());
	QUARK_ASSERT(s.size() > 0);

	const auto it = env->_values.find(s);
	if(it != env->_values.end()){
		return it->second;
	}
	else if(env->_parent_env){
		return resolve_env_variable_deep(vm, env->_parent_env, s);
	}
	else{
		throw std::runtime_error("Undefined variable \"" + s + "\"!");
	}
}

floyd_ast::value_t resolve_env_variable(const interpreter_t& vm, const std::string& s){
	QUARK_ASSERT(vm.check_invariant());
	QUARK_ASSERT(s.size() > 0);

	return resolve_env_variable_deep(vm, vm._call_stack.back(), s);
}

value_t find_global_symbol(const interpreter_t& vm, const string& s){
	const auto v = resolve_env_variable(vm, s);
	if(v.is_null()){
		throw std::runtime_error("Undefined function!");
	}
	return v;
}

expression_t evaluate_expression(const interpreter_t& vm, const expression_t& e){
	QUARK_ASSERT(vm.check_invariant());
	QUARK_ASSERT(e.check_invariant());

	if(e.is_constant()){
		return e;
	}

	const auto op = e.get_operation();

	if(op == floyd_basics::expression_type::k_resolve_member){
		const auto parent_expr = evaluate_expression(vm, e.get_expressions()[0]);
		if(parent_expr.is_constant() && parent_expr.get_constant().is_struct()){
			const auto struct_instance = parent_expr.get_constant().get_struct();
			const value_t value = struct_instance->_member_values[e.get_symbol()];
			return expression_t::make_constant_value(value);
		}
		else{
			throw std::runtime_error("Resolve member failed.");
		}
	}
	else if(op == floyd_basics::expression_type::k_variable){
		const auto variable_name = e.get_symbol();
		const value_t value = resolve_env_variable(vm, variable_name);
		return expression_t::make_constant_value(value);
	}

	else if(op == floyd_basics::expression_type::k_call){
		return evaluate_call_expression(vm, e);
	}

	else if(op == floyd_basics::expression_type::k_arithmetic_unary_minus__1){
		const auto& expr = evaluate_expression(vm, e.get_expressions()[0]);
		if(expr.is_constant()){
			const auto& c = expr.get_constant();
			if(c.is_int()){
				return evaluate_expression(
					vm,
					expression_t::make_simple_expression__2(floyd_basics::expression_type::k_arithmetic_subtract__2, expression_t::make_constant_int(0), expr)
				);
			}
			else if(c.is_float()){
				return evaluate_expression(
					vm,
					expression_t::make_simple_expression__2(floyd_basics::expression_type::k_arithmetic_subtract__2, expression_t::make_constant_float(0.0f), expr)
				);
			}
			else{
				throw std::runtime_error("Unary minus won't work on expressions of type \"" + json_to_compact_string(typeid_to_json(c.get_type())) + "\".");
			}
		}
		else{
			throw std::runtime_error("Could not evaluate condition in conditional expression.");
		}
	}

	//	Special-case since it uses 3 expressions & uses shortcut evaluation.
	else if(op == floyd_basics::expression_type::k_conditional_operator3){
		const auto cond_result = evaluate_expression(vm, e.get_expressions()[0]);
		if(cond_result.is_constant() && cond_result.get_constant().is_bool()){
			const bool cond_flag = cond_result.get_constant().get_bool();

			//	!!! Only evaluate the CHOSEN expression. Not that importan since functions are pure.
			if(cond_flag){
				return evaluate_expression(vm, e.get_expressions()[1]);
			}
			else{
				return evaluate_expression(vm, e.get_expressions()[2]);
			}
		}
		else{
			throw std::runtime_error("Could not evaluate condition in conditional expression.");
		}
	}

	//	First evaluate all inputs to our operation.
	const auto left_expr = evaluate_expression(vm, e.get_expressions()[0]);
	const auto right_expr = evaluate_expression(vm, e.get_expressions()[1]);

	//	Both left and right are constant, replace the math_operation with a constant!
	if(left_expr.is_constant() && right_expr.is_constant()){
		//	Perform math operation on the two constants => new constant.
		const auto left_constant = left_expr.get_constant();
		const auto right_constant = right_expr.get_constant();

		//	Is operation supported by all types?
		{
			if(op == floyd_basics::expression_type::k_comparison_smaller_or_equal__2){
				long diff = value_t::compare_value_true_deep(left_constant, right_constant);
				return expression_t::make_constant_bool(diff <= 0);
			}
			else if(op == floyd_basics::expression_type::k_comparison_smaller__2){
				long diff = value_t::compare_value_true_deep(left_constant, right_constant);
				return expression_t::make_constant_bool(diff < 0);
			}
			else if(op == floyd_basics::expression_type::k_comparison_larger_or_equal__2){
				long diff = value_t::compare_value_true_deep(left_constant, right_constant);
				return expression_t::make_constant_bool(diff >= 0);
			}
			else if(op == floyd_basics::expression_type::k_comparison_larger__2){
				long diff = value_t::compare_value_true_deep(left_constant, right_constant);
				return expression_t::make_constant_bool(diff > 0);
			}


			else if(op == floyd_basics::expression_type::k_logical_equal__2){
				long diff = value_t::compare_value_true_deep(left_constant, right_constant);
				return expression_t::make_constant_bool(diff == 0);
			}
			else if(op == floyd_basics::expression_type::k_logical_nonequal__2){
				long diff = value_t::compare_value_true_deep(left_constant, right_constant);
				return expression_t::make_constant_bool(diff != 0);
			}
		}

		//	bool
		if(left_constant.is_bool() && right_constant.is_bool()){
			const bool left = left_constant.get_bool();
			const bool right = right_constant.get_bool();

			if(op == floyd_basics::expression_type::k_arithmetic_add__2
			|| op == floyd_basics::expression_type::k_arithmetic_subtract__2
			|| op == floyd_basics::expression_type::k_arithmetic_multiply__2
			|| op == floyd_basics::expression_type::k_arithmetic_divide__2
			|| op == floyd_basics::expression_type::k_arithmetic_remainder__2
			){
				throw std::runtime_error("Arithmetics on bool not allowed.");
			}

			else if(op == floyd_basics::expression_type::k_logical_and__2){
				return expression_t::make_constant_bool(left && right);
			}
			else if(op == floyd_basics::expression_type::k_logical_or__2){
				return expression_t::make_constant_bool(left || right);
			}
			else{
				QUARK_ASSERT(false);
			}
		}

		//	int
		else if(left_constant.is_int() && right_constant.is_int()){
			const int left = left_constant.get_int();
			const int right = right_constant.get_int();

			if(op == floyd_basics::expression_type::k_arithmetic_add__2){
				return expression_t::make_constant_int(left + right);
			}
			else if(op == floyd_basics::expression_type::k_arithmetic_subtract__2){
				return expression_t::make_constant_int(left - right);
			}
			else if(op == floyd_basics::expression_type::k_arithmetic_multiply__2){
				return expression_t::make_constant_int(left * right);
			}
			else if(op == floyd_basics::expression_type::k_arithmetic_divide__2){
				if(right == 0){
					throw std::runtime_error("EEE_DIVIDE_BY_ZERO");
				}
				return expression_t::make_constant_int(left / right);
			}
			else if(op == floyd_basics::expression_type::k_arithmetic_remainder__2){
				if(right == 0){
					throw std::runtime_error("EEE_DIVIDE_BY_ZERO");
				}
				return expression_t::make_constant_int(left % right);
			}

			else if(op == floyd_basics::expression_type::k_logical_and__2){
				return expression_t::make_constant_bool((left != 0) && (right != 0));
			}
			else if(op == floyd_basics::expression_type::k_logical_or__2){
				return expression_t::make_constant_bool((left != 0) || (right != 0));
			}
			else{
				QUARK_ASSERT(false);
			}
		}

		//	float
		else if(left_constant.is_float() && right_constant.is_float()){
			const float left = left_constant.get_float();
			const float right = right_constant.get_float();

			if(op == floyd_basics::expression_type::k_arithmetic_add__2){
				return expression_t::make_constant_float(left + right);
			}
			else if(op == floyd_basics::expression_type::k_arithmetic_subtract__2){
				return expression_t::make_constant_float(left - right);
			}
			else if(op == floyd_basics::expression_type::k_arithmetic_multiply__2){
				return expression_t::make_constant_float(left * right);
			}
			else if(op == floyd_basics::expression_type::k_arithmetic_divide__2){
				if(right == 0.0f){
					throw std::runtime_error("EEE_DIVIDE_BY_ZERO");
				}
				return expression_t::make_constant_float(left / right);
			}
			else if(op == floyd_basics::expression_type::k_arithmetic_remainder__2){
				throw std::runtime_error("Modulo operation on float not allowed.");
			}


			else if(op == floyd_basics::expression_type::k_logical_and__2){
				return expression_t::make_constant_bool((left != 0.0f) && (right != 0.0f));
			}
			else if(op == floyd_basics::expression_type::k_logical_or__2){
				return expression_t::make_constant_bool((left != 0.0f) || (right != 0.0f));
			}
			else{
				QUARK_ASSERT(false);
			}
		}

		//	string
		else if(left_constant.is_string() && right_constant.is_string()){
			const string left = left_constant.get_string();
			const string right = right_constant.get_string();

			if(op == floyd_basics::expression_type::k_arithmetic_add__2){
				return expression_t::make_constant_string(left + right);
			}

			else if(op == floyd_basics::expression_type::k_arithmetic_subtract__2){
				throw std::runtime_error("Operation not allowed on string.");
			}
			else if(op == floyd_basics::expression_type::k_arithmetic_multiply__2){
				throw std::runtime_error("Operation not allowed on string.");
			}
			else if(op == floyd_basics::expression_type::k_arithmetic_divide__2){
				throw std::runtime_error("Operation not allowed on string.");
			}
			else if(op == floyd_basics::expression_type::k_arithmetic_remainder__2){
				throw std::runtime_error("Operation not allowed on string.");
			}


			else if(op == floyd_basics::expression_type::k_logical_and__2){
				throw std::runtime_error("Operation not allowed on string.");
			}
			else if(op == floyd_basics::expression_type::k_logical_or__2){
				throw std::runtime_error("Operation not allowed on string.");
			}
			else{
				QUARK_ASSERT(false);
			}
		}
		//	struct
		else if(left_constant.is_struct() && right_constant.is_struct()){
			const auto left = left_constant.get_struct();
			const auto right = right_constant.get_struct();

			if(!(left->_struct_type == right->_struct_type)){
				throw std::runtime_error("Struct type mismatch.");
			}

			if(op == floyd_basics::expression_type::k_arithmetic_add__2){
				//??? allow
				throw std::runtime_error("Operation not allowed on struct.");
			}
			else if(op == floyd_basics::expression_type::k_arithmetic_subtract__2){
				throw std::runtime_error("Operation not allowed on struct.");
			}
			else if(op == floyd_basics::expression_type::k_arithmetic_multiply__2){
				throw std::runtime_error("Operation not allowed on struct.");
			}
			else if(op == floyd_basics::expression_type::k_arithmetic_divide__2){
				throw std::runtime_error("Operation not allowed on struct.");
			}
			else if(op == floyd_basics::expression_type::k_arithmetic_remainder__2){
				throw std::runtime_error("Operation not allowed on struct.");
			}

			else if(op == floyd_basics::expression_type::k_logical_and__2){
				throw std::runtime_error("Operation not allowed on struct.");
			}
			else if(op == floyd_basics::expression_type::k_logical_or__2){
				throw std::runtime_error("Operation not allowed on struct.");
			}
			else{
				QUARK_ASSERT(false);
			}
		}

		else if(left_constant.is_vector() && right_constant.is_vector()){
			QUARK_ASSERT(false);
		}
		else{
			throw std::runtime_error("Arithmetics failed.");
		}
	}

	//	Else use a math_operation expression to perform the calculation later.
	//	We make a NEW math_operation since sub-nodes may have been evaluated.
	else{
		return expression_t::make_simple_expression__2(op, left_expr, right_expr);
	}
}

std::map<int, object_id_info_t> get_all_objects(
	const std::shared_ptr<const lexical_scope_t>& scope,
	int id,
	int parent_id
){
	std::map<int, object_id_info_t> result;

	result[id] = object_id_info_t{ scope, parent_id };

	for(const auto& e: scope->get_objects()){
		const auto t = get_all_objects(e.second, e.first, id);
		result.insert(t.begin(), t.end());
	}
	return result;
}

//	Needs to look in correct scope to find function object. It can exist in *any* scope_def, even siblings or children.
std::map<int, object_id_info_t> make_lexical_lookup(const std::shared_ptr<const lexical_scope_t>& s){
	return get_all_objects(s, 0, -1);
}

object_id_info_t lookup_object_id(const interpreter_t& vm, const floyd_ast::value_t& f){
	const auto& obj = f.get_function();
	const auto& function_id = obj->_function_id;
	const auto objectIt = vm._object_lookup.find(function_id);
	if(objectIt == vm._object_lookup.end()){
		throw std::runtime_error("Function object not found!");
	}

	return objectIt->second;
}


value_t call_function(const interpreter_t& vm, const floyd_ast::value_t& f, const vector<value_t>& args){
	QUARK_ASSERT(vm.check_invariant());
	QUARK_ASSERT(f.check_invariant());
	for(const auto i: args){ QUARK_ASSERT(i.check_invariant()); };

	///??? COMPARE arg count and types.

	if(f.is_function() == false){
		throw std::runtime_error("Cannot call non-function.");
	}

	const auto function_object_info = lookup_object_id(vm, f);
	const auto function_object = function_object_info._object;
	const auto function_object_id = f.get_function()->_function_id;

	//	Always use global scope. Future: support closures by linking to function env where function is defined.
	auto parent_env = vm._call_stack[0];

	auto new_environment = environment_t::make_environment(vm, function_object, function_object_id, parent_env);

	//	Copy input arguments to the function scope.
	for(int i = 0 ; i < function_object->_args.size() ; i++){
		const auto& arg_name = function_object->_args[i]._name;
		const auto& arg_value = args[i];
		new_environment->_values[arg_name] = arg_value;
	}

	interpreter_t vm2 = vm;
	vm2._call_stack.push_back(new_environment);

	QUARK_TRACE(json_to_pretty_string(interpreter_to_json(vm2)));

	const auto r = execute_statements(vm2, function_object->_statements);
	if(!r.second){
		throw std::runtime_error("function missing return statement");
	}
	else{
		return *r.second;
	}
}

bool all_constants(const vector<expression_t>& e){
	for(const auto& i: e){
		if(!i.is_constant()){
			return false;
		}
	}
	return true;
}

expression_t evaluate_call_expression(const interpreter_t& vm, const expression_t& e){
	QUARK_ASSERT(vm.check_invariant());
	QUARK_ASSERT(e.check_invariant());
	QUARK_ASSERT(e.get_operation() == floyd_basics::expression_type::k_call);

	//	Simplify each input expression: expression[0]: which function to call, expression[1]: first argument if any.
	const auto& expressions = e.get_expressions();
	expression_t function = evaluate_expression(vm, expressions[0]);
	vector<expression_t> args2;
	for(int i = 1 ; i < expressions.size() ; i++){
		const auto t = evaluate_expression(vm, expressions[i]);
		args2.push_back(t);
	}

	//	If not all input expressions could be evaluated, return a (maybe simplified) expression.
	if(function.is_constant() == false || all_constants(args2) == false){
		return expression_t::make_function_call(function, args2, e.get_expression_type());
	}

	//	Get function value and arg values.
	const auto& function_value = function.get_constant();
	if(function_value.is_function() == false){
		throw std::runtime_error("Cannot call non-function.");
	}

	vector<value_t> args3;
	for(const auto& i: args2){
		args3.push_back(i.get_constant());
	}

	const auto& result = call_function(vm, function_value, args3);
	return expression_t::make_constant_value(result);
}



json_t interpreter_to_json(const interpreter_t& vm){
	vector<json_t> callstack;
	for(const auto& e: vm._call_stack){
		std::map<string, json_t> values;
		for(const auto&v: e->_values){
			values[v.first] = value_to_json(v.second);
		}

		const auto& env = json_t::make_object({
			{ "parent_env", e->_parent_env ? e->_parent_env->_object_id : json_t() },
			{ "object_id", json_t(double(e->_object_id)) },
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

	ut_compare_jsons(expression_to_json(e3), expression_to_json(expected));
}


QUARK_UNIT_TESTQ("evaluate_expression()", "constant 1234 == 1234") {
	test__evaluate_expression(
		expression_t::make_constant_int(1234),
		expression_t::make_constant_int(1234)
	);
}

QUARK_UNIT_TESTQ("evaluate_expression()", "1 + 2 == 3") {
	test__evaluate_expression(
		expression_t::make_simple_expression__2(
			floyd_basics::expression_type::k_arithmetic_add__2,
			expression_t::make_constant_int(1),
			expression_t::make_constant_int(2)
		),
		expression_t::make_constant_int(3)
	);
}


QUARK_UNIT_TESTQ("evaluate_expression()", "3 * 4 == 12") {
	test__evaluate_expression(
		expression_t::make_simple_expression__2(
			floyd_basics::expression_type::k_arithmetic_multiply__2,
			expression_t::make_constant_int(3),
			expression_t::make_constant_int(4)
		),
		expression_t::make_constant_int(12)
	);
}

QUARK_UNIT_TESTQ("evaluate_expression()", "(3 * 4) * 5 == 60") {
	test__evaluate_expression(
		expression_t::make_simple_expression__2(
			floyd_basics::expression_type::k_arithmetic_multiply__2,
			expression_t::make_simple_expression__2(
				floyd_basics::expression_type::k_arithmetic_multiply__2,
				expression_t::make_constant_int(3),
				expression_t::make_constant_int(4)
			),
			expression_t::make_constant_int(5)
		),
		expression_t::make_constant_int(60)
	);
}




//////////////////////////		environment_t



std::shared_ptr<environment_t> environment_t::make_environment(
	const interpreter_t& vm,
	const std::shared_ptr<const lexical_scope_t>& object,
	int object_id,
	std::shared_ptr<floyd_interpreter::environment_t>& parent_env
)
{
	QUARK_ASSERT(vm.check_invariant());

	//	Copy global scope's functions into new env.
	std::map<std::string, floyd_ast::value_t> values;
	for(const auto& e: object->_state){
		values[e._name] = *e._value;
	}

	auto f = environment_t{ vm._ast, parent_env, object_id, values };
	return make_shared<environment_t>(f);
}

bool environment_t::check_invariant() const {
	QUARK_ASSERT(_ast.check_invariant());
	return true;
}


//////////////////////////		interpreter_t


interpreter_t::interpreter_t(const ast_t& ast) :
	_ast(ast),
	_object_lookup(make_lexical_lookup(_ast.get_global_scope()))
{
	QUARK_ASSERT(ast.check_invariant());

	shared_ptr<environment_t> parent_env;
	_call_stack.push_back(environment_t::make_environment(*this, _ast.get_global_scope(), 0, parent_env));

	//	Run static intialization (basically run global statements before calling main()).
	const auto r = execute_statements(*this, _ast.get_global_scope()->_statements);
	assert(!r.second);


	_call_stack[0]->_values = r.first._call_stack[0]->_values;
	QUARK_ASSERT(check_invariant());
}

bool interpreter_t::check_invariant() const {
	QUARK_ASSERT(_ast.check_invariant());
	return true;
}





//////////////////////////		run_main()


ast_t program_to_ast2(const string& program){
	const auto pass1 = floyd_parser::parse_program2(program);
	const auto pass2 = run_pass2(pass1);
	trace(pass2);
	return pass2;
}


interpreter_t run_global(const string& source){
	auto ast = program_to_ast2(source);
	auto vm = interpreter_t(ast);
	return vm;
}

std::pair<interpreter_t, floyd_ast::value_t> run_main(const string& source, const vector<floyd_ast::value_t>& args){
	auto ast = program_to_ast2(source);
	auto vm = interpreter_t(ast);
	const auto f = find_global_symbol(vm, "main");
	const auto r = call_function(vm, f, args);
	return { vm, r };
}





///////////////////////////////////////////////////////////////////////////////////////
//	FLOYD LANGUAGE TEST SUITE
///////////////////////////////////////////////////////////////////////////////////////



void test__run_init(const std::string& program, const value_t& expected_result){
	const auto result = run_global(program);
	const auto result_value = result._call_stack[0]->_values["result"];
	ut_compare_jsons(
		expression_to_json(expression_t::make_constant_value(result_value)),
		expression_to_json(expression_t::make_constant_value(expected_result))
	);
}


void test__run_main(const std::string& program, const vector<floyd_ast::value_t>& args, const value_t& expected_return){
	const auto result = run_main(program, args);
	ut_compare_jsons(
		expression_to_json(expression_t::make_constant_value(result.second)),
		expression_to_json(expression_t::make_constant_value(expected_return))
	);
}


//??? test all errors too!

//////////////////////////		TEST GLOBAL CONSTANTS


QUARK_UNIT_TESTQ("Floyd test suite", "Global int variable"){
	test__run_init("int result = 123;", value_t(123));
}


//////////////////////////		TEST CONSTANT EXPRESSIONS

/*
QUARK_UNIT_TESTQ("Floyd test suite", "null constant expression"){
	test__run_init("int result = null;", value_t());
}
*/

//??? why OK? result is int!
QUARK_UNIT_TESTQ("Floyd test suite", "bool constant expression"){
	test__run_init("int result = true;", value_t(true));
}
QUARK_UNIT_TESTQ("Floyd test suite", "bool constant expression"){
	test__run_init("int result = false;", value_t(false));
}

QUARK_UNIT_TESTQ("Floyd test suite", "int constant expression"){
	test__run_init("int result = 123;", value_t(123));
}

QUARK_UNIT_TESTQ("Floyd test suite", "float constant expression"){
	test__run_init("int result = 3.5;", value_t(float(3.5)));
}

QUARK_UNIT_TESTQ("Floyd test suite", "string constant expression"){
	test__run_init("int result = \"xyz\";", value_t("xyz"));
}

//??? struct
//??? vector
//??? function







//////////////////////////		TEST EXPRESSIONS


QUARK_UNIT_TESTQ("Floyd test suite", "+") {
	test__run_init("int result = 1 + 2;", value_t(3));
}
QUARK_UNIT_TESTQ("Floyd test suite", "+") {
	test__run_init("int result = 1 + 2 + 3;", value_t(6));
}
QUARK_UNIT_TESTQ("Floyd test suite", "*") {
	test__run_init("int result = 3 * 4;", value_t(12));
}

QUARK_UNIT_TESTQ("Floyd test suite", "parant") {
	test__run_init("int result = (3 * 4) * 5;", value_t(60));
}





//////////////////////////		TEST conditional expression


QUARK_UNIT_TESTQ("run_main()", "conditional expression"){
	test__run_init("int result = true ? 1 : 2;", value_t(1));
}
QUARK_UNIT_TESTQ("run_main()", "conditional expression"){
	test__run_init("int result = false ? 1 : 2;", value_t(2));
}

//??? Test truthness off all variable types: strings, floats

QUARK_UNIT_TESTQ("run_main()", "conditional expression"){
	test__run_init("string result = true ? \"yes\" : \"no\";", value_t("yes"));
}
QUARK_UNIT_TESTQ("run_main()", "conditional expression"){
	test__run_init("string result = false ? \"yes\" : \"no\";", value_t("no"));
}





QUARK_UNIT_TESTQ("evaluate_expression()", "Parenthesis") {
	test__run_init("int result = 5*(4+4+1)", value_t(45));
}
QUARK_UNIT_TESTQ("evaluate_expression()", "Parenthesis") {
	test__run_init("int result = 5*(2*(1+3)+1)", value_t(45));
}
QUARK_UNIT_TESTQ("evaluate_expression()", "Parenthesis") {
	test__run_init("int result = 5*((1+3)*2+1)", value_t(45));
}



QUARK_UNIT_TESTQ("evaluate_expression()", "Spaces") {
	test__run_init("int result = 5 * ((1 + 3) * 2 + 1) ", value_t(45));
}
QUARK_UNIT_TESTQ("evaluate_expression()", "Spaces") {
	test__run_init("int result = 5 - 2 * ( 3 ) ", value_t(-1));
}
QUARK_UNIT_TESTQ("evaluate_expression()", "Spaces") {
	test__run_init("int result =  5 - 2 * ( ( 4 )  - 1 ) ", value_t(-1));
}

QUARK_UNIT_TESTQ("evaluate_expression()", "Sign before parenthesis") {
	test__run_init("int result = -(2+1)*4", value_t(-12));
}
QUARK_UNIT_TESTQ("evaluate_expression()", "Sign before parenthesis") {
	test__run_init("int result = -4*(2+1)", value_t(-12));
}


QUARK_UNIT_TESTQ("evaluate_expression()", "Fractional numbers") {
	test__run_init("int result = 5.5/5.0", value_t(1.1f));
}
QUARK_UNIT_TESTQ("evaluate_expression()", "Fractional numbers") {
//	test__run_init("int result = 1/5e10") == 2e-11);
}
QUARK_UNIT_TESTQ("evaluate_expression()", "Fractional numbers") {
	test__run_init("int result = (4.0-3.0)/(4.0*4.0)", value_t(0.0625f));
}
QUARK_UNIT_TESTQ("evaluate_expression()", "Fractional numbers") {
	test__run_init("int result = 1.0/2.0/2.0", value_t(0.25f));
}
QUARK_UNIT_TESTQ("evaluate_expression()", "Fractional numbers") {
	test__run_init("int result = 0.25 * .5 * 0.5", value_t(0.0625f));
}
QUARK_UNIT_TESTQ("evaluate_expression()", "Fractional numbers") {
	test__run_init("int result = .25 / 2.0 * .5", value_t(0.0625f));
}

QUARK_UNIT_TESTQ("evaluate_expression()", "Repeated operators") {
	test__run_init("int result = 1+-2", value_t(-1));
}
QUARK_UNIT_TESTQ("evaluate_expression()", "Repeated operators") {
	test__run_init("int result = --2", value_t(2));
}
QUARK_UNIT_TESTQ("evaluate_expression()", "Repeated operators") {
	test__run_init("int result = 2---2", value_t(0));
}
QUARK_UNIT_TESTQ("evaluate_expression()", "Repeated operators") {
	test__run_init("int result = 2-+-2", value_t(4));
}


QUARK_UNIT_TESTQ("evaluate_expression()", "Bool") {
	test__run_init("int result = true", value_t(true));
}
QUARK_UNIT_TESTQ("evaluate_expression()", "Bool") {
	test__run_init("int result = false", value_t(false));
}


//??? add tests  for pass2()
QUARK_UNIT_TESTQ("evaluate_expression()", "?:") {
	test__run_init("int result = true ? 4 : 6", value_t(4));
}
QUARK_UNIT_TESTQ("evaluate_expression()", "?:") {
	test__run_init("int result = false ? 4 : 6", value_t(6));
}

QUARK_UNIT_TESTQ("evaluate_expression()", "?:") {
	test__run_init("int result = 1==3 ? 4 : 6", value_t(6));
}
QUARK_UNIT_TESTQ("evaluate_expression()", "?:") {
	test__run_init("int result = 3==3 ? 4 : 6", value_t(4));
}

QUARK_UNIT_TESTQ("evaluate_expression()", "?:") {
	test__run_init("int result = 3==3 ? 2 + 2 : 2 * 3", value_t(4));
}

QUARK_UNIT_TESTQ("evaluate_expression()", "?:") {
	test__run_init("int result = 3==1+2 ? 2 + 2 : 2 * 3", value_t(4));
}




QUARK_UNIT_TESTQ("evaluate_expression()", "==") {
	test__run_init("int result = 1 == 1", value_t(true));
}
QUARK_UNIT_TESTQ("evaluate_expression()", "==") {
	test__run_init("int result = 1 == 2", value_t(false));
}
QUARK_UNIT_TESTQ("evaluate_expression()", "==") {
	test__run_init("int result = 1.3 == 1.3", value_t(true));
}
QUARK_UNIT_TESTQ("evaluate_expression()", "==") {
	test__run_init("int result = \"hello\" == \"hello\"", value_t(true));
}
QUARK_UNIT_TESTQ("evaluate_expression()", "==") {
	test__run_init("int result = \"hello\" == \"bye\"", value_t(false));
}


QUARK_UNIT_TESTQ("evaluate_expression()", "<") {
	test__run_init("int result = 1 < 2", value_t(true));
}
QUARK_UNIT_TESTQ("evaluate_expression()", "<") {
	test__run_init("int result = 5 < 2", value_t(false));
}
QUARK_UNIT_TESTQ("evaluate_expression()", "<") {
	test__run_init("int result = 0.3 < 0.4", value_t(true));
}
QUARK_UNIT_TESTQ("evaluate_expression()", "<") {
	test__run_init("int result = 1.5 < 0.4", value_t(false));
}
QUARK_UNIT_TESTQ("evaluate_expression()", "<") {
	test__run_init("int result = \"adwark\" < \"boat\"", value_t(true));
}
QUARK_UNIT_TESTQ("evaluate_expression()", "<") {
	test__run_init("int result = \"boat\" < \"adwark\"", value_t(false));
}


QUARK_UNIT_TESTQ("evaluate_expression()", "&&") {
	test__run_init("int result = false && false", value_t(false));
}
QUARK_UNIT_TESTQ("evaluate_expression()", "&&") {
	test__run_init("int result = false && true", value_t(false));
}
QUARK_UNIT_TESTQ("evaluate_expression()", "&&") {
	test__run_init("int result = true && false", value_t(false));
}
QUARK_UNIT_TESTQ("evaluate_expression()", "&&") {
	test__run_init("int result = true && true", value_t(true));
}

QUARK_UNIT_TESTQ("evaluate_expression()", "&&") {
	test__run_init("int result = false && false && false", value_t(false));
}
QUARK_UNIT_TESTQ("evaluate_expression()", "&&") {
	test__run_init("int result = false && false && true", value_t(false));
}
QUARK_UNIT_TESTQ("evaluate_expression()", "&&") {
	test__run_init("int result = false && true && false", value_t(false));
}
QUARK_UNIT_TESTQ("evaluate_expression()", "&&") {
	test__run_init("int result = false && true && true", value_t(false));
}
QUARK_UNIT_TESTQ("evaluate_expression()", "&&") {
	test__run_init("int result = true && false && false", value_t(false));
}
QUARK_UNIT_TESTQ("evaluate_expression()", "&&") {
	test__run_init("int result = true && true && false", value_t(false));
}
QUARK_UNIT_TESTQ("evaluate_expression()", "&&") {
	test__run_init("int result = true && true && true", value_t(true));
}


QUARK_UNIT_TESTQ("evaluate_expression()", "||") {
	test__run_init("int result = false || false", value_t(false));
}
QUARK_UNIT_TESTQ("evaluate_expression()", "||") {
	test__run_init("int result = false || true", value_t(true));
}
QUARK_UNIT_TESTQ("evaluate_expression()", "||") {
	test__run_init("int result = true || false", value_t(true));
}
QUARK_UNIT_TESTQ("evaluate_expression()", "||") {
	test__run_init("int result = true || true", value_t(true));
}


QUARK_UNIT_TESTQ("evaluate_expression()", "||") {
	test__run_init("int result = false || false || false", value_t(false));
}
QUARK_UNIT_TESTQ("evaluate_expression()", "||") {
	test__run_init("int result = false || false || true", value_t(true));
}
QUARK_UNIT_TESTQ("evaluate_expression()", "||") {
	test__run_init("int result = false || true || false", value_t(true));
}
QUARK_UNIT_TESTQ("evaluate_expression()", "||") {
	test__run_init("int result = false || true || true", value_t(true));
}
QUARK_UNIT_TESTQ("evaluate_expression()", "||") {
	test__run_init("int result = true || false || false", value_t(true));
}
QUARK_UNIT_TESTQ("evaluate_expression()", "||") {
	test__run_init("int result = true || false || true", value_t(true));
}
QUARK_UNIT_TESTQ("evaluate_expression()", "||") {
	test__run_init("int result = true || true || false", value_t(true));
}
QUARK_UNIT_TESTQ("evaluate_expression()", "||") {
	test__run_init("int result = true || true || true", value_t(true));
}



QUARK_UNIT_TESTQ("evaluate_expression()", "Division by zero") {
	try{
		test__run_init("int result = 2/0", value_t());
		QUARK_TEST_VERIFY(false);
	}
	catch(const std::runtime_error& e){
		QUARK_TEST_VERIFY(string(e.what()) == "EEE_DIVIDE_BY_ZERO");
	}
}

QUARK_UNIT_TESTQ("evaluate_expression()", "Division by zero"){
	try{
		test__run_init("int result = 3+1/(5-5)+4", value_t());
		QUARK_TEST_VERIFY(false);
	}
	catch(const std::runtime_error& e){
		QUARK_TEST_VERIFY(string(e.what()) == "EEE_DIVIDE_BY_ZERO");
	}
}

#if false

QUARK_UNIT_TESTQ("evaluate_expression()", "Multiply errors") {
		//	Multiple errors not possible/relevant now that we use exceptions for errors.
/*
	//////////////////////////		Only one error will be detected (in this case, the last one)
	QUARK_TEST_VERIFY(test__evaluate_expression("3+1/0+4$") == EEE_WRONG_CHAR);

	QUARK_TEST_VERIFY(test__evaluate_expression("3+1/0+4") == EEE_DIVIDE_BY_ZERO);

	// ...or the first one
	QUARK_TEST_VERIFY(test__evaluate_expression("q+1/0)") == EEE_WRONG_CHAR);
	QUARK_TEST_VERIFY(test__evaluate_expression("+1/0)") == EEE_PARENTHESIS);
	QUARK_TEST_VERIFY(test__evaluate_expression("+1/0") == EEE_DIVIDE_BY_ZERO);
*/
}

#endif








//////////////////////////		TEST FUNCTIONS



QUARK_UNIT_TESTQ("run_main", "Can make and read global int"){
	test__run_main(
		"int test = 123;"
		"string main(){\n"
		"	return test;"
		"}\n",
		{},
		value_t(123)
	);
}


QUARK_UNIT_TESTQ("run_main()", "minimal program 2"){
	test__run_main(
		"string main(string args){\n"
		"	return \"123\" + \"456\";\n"
		"}\n",
		vector<floyd_ast::value_t>{floyd_ast::value_t("program_name 1 2 3 4")},
		floyd_ast::value_t("123456")
	);
}

QUARK_UNIT_TESTQ("run_main()", ""){
	test__run_main("bool main(){ return 4 < 5; }", {}, floyd_ast::value_t(true));
}
QUARK_UNIT_TESTQ("run_main()", ""){
	test__run_main("bool main(){ return 5 < 4; }", {}, floyd_ast::value_t(false));
}
QUARK_UNIT_TESTQ("run_main()", ""){
	test__run_main("bool main(){ return 4 <= 4; }", {}, floyd_ast::value_t(true));
}

QUARK_UNIT_TESTQ("call_function()", "minimal program"){
	auto ast = program_to_ast2(
		"int main(string args){\n"
		"	return 3 + 4;\n"
		"}\n"
	);
	auto vm = interpreter_t(ast);
	const auto f = find_global_symbol(vm, "main");
	const auto result = call_function(vm, f, vector<floyd_ast::value_t>{ floyd_ast::value_t("program_name 1 2 3") });
	QUARK_TEST_VERIFY(result == floyd_ast::value_t(7));
}


QUARK_UNIT_TESTQ("call_function()", "minimal program 2"){
	auto ast = program_to_ast2(
		"string main(string args){\n"
		"	return \"123\" + \"456\";\n"
		"}\n"
	);
	auto vm = interpreter_t(ast);
	const auto f = find_global_symbol(vm, "main");
	const auto result = call_function(vm, f, vector<floyd_ast::value_t>{ floyd_ast::value_t("program_name 1 2 3") });
	QUARK_TEST_VERIFY(result == floyd_ast::value_t("123456"));
}

QUARK_UNIT_TESTQ("call_function()", "define additional function, call it several times"){
	auto ast = program_to_ast2(
		"int myfunc(){ return 5; }\n"
		"int main(string args){\n"
		"	return myfunc() + myfunc() * 2;\n"
		"}\n"
	);
	auto vm = interpreter_t(ast);
	const auto f = find_global_symbol(vm, "main");
	const auto result = call_function(vm, f, vector<floyd_ast::value_t>{ floyd_ast::value_t("program_name 1 2 3") });
	QUARK_TEST_VERIFY(result == floyd_ast::value_t(15));
}

QUARK_UNIT_TESTQ("call_function()", "use function inputs"){
	auto ast = program_to_ast2(
		"string main(string args){\n"
		"	return \"-\" + args + \"-\";\n"
		"}\n"
	);
	auto vm = interpreter_t(ast);
	const auto f = find_global_symbol(vm, "main");
	const auto result = call_function(vm, f, vector<floyd_ast::value_t>{ floyd_ast::value_t("xyz") });
	QUARK_TEST_VERIFY(result == floyd_ast::value_t("-xyz-"));

	const auto result2 = call_function(vm, f, vector<floyd_ast::value_t>{ floyd_ast::value_t("Hello, world!") });
	QUARK_TEST_VERIFY(result2 == floyd_ast::value_t("-Hello, world!-"));
}


QUARK_UNIT_TESTQ("call_function()", "use local variables"){
	auto ast = program_to_ast2(
		"string myfunc(string t){ return \"<\" + t + \">\"; }\n"
		"string main(string args){\n"
		"	 string a = \"--\"; string b = myfunc(args) ; return a + args + b + a;\n"
		"}\n"
	);
	auto vm = interpreter_t(ast);
	const auto f = find_global_symbol(vm, "main");
	const auto result = call_function(vm, f, vector<floyd_ast::value_t>{ floyd_ast::value_t("xyz") });
	QUARK_TEST_VERIFY(result == floyd_ast::value_t("--xyz<xyz>--"));

	const auto result2 = call_function(vm, f, vector<floyd_ast::value_t>{ floyd_ast::value_t("123") });
	QUARK_TEST_VERIFY(result2 == floyd_ast::value_t("--123<123>--"));
}


//////////////////////////		fibonacci

/*
	fun fibonacci(n) {
	  if (n <= 1) return n;
	  return fibonacci(n - 2) + fibonacci(n - 1);
	}

	for (var i = 0; i < 20; i = i + 1) {
	  print fibonacci(i);
	}
*/

#if false
QUARK_UNIT_TESTQ("run_main", "fibonacci"){
	test__run_init(
		"int fibonacci(int n) {"
		"	if (n <= 1){"
		"		return n;"
		"	}"
		"	return fibonacci(n - 2) + fibonacci(n - 1);"
		"}"

		"for (var i = 0; i < 20; i = i + 1) {"
		"	print fibonacci(i);"
		"}",
		value_t(123)
	);
}
#endif




//////////////////////////		TEST STRUCT SUPPORT



#if false
QUARK_UNIT_TESTQ("run_main()", "struct"){
	QUARK_UT_VERIFY(test__run_main("struct t { int a;} bool main(){ t b = t_constructor(); return b == b; }", floyd_ast::value_t(true)));
}

QUARK_UNIT_1("run_main()", "", test__run_main(
	"struct t { int a;} bool main(){ t a = t_constructor(); t b = t_constructor(); return a == b; }",
	floyd_ast::value_t(true)
));


QUARK_UNIT_TESTQ("run_main()", ""){
	QUARK_TEST_VERIFY(test__run_main(
		"struct t { int a;} bool main(){ t a = t_constructor(); t b = t_constructor(); return a == b; }",
		floyd_ast::value_t(true)
	));
}

QUARK_UNIT_TESTQ("run_main()", ""){
	try {
		test__run_main(
			"struct t { int a;} bool main(){ t a = t_constructor(); int b = 1055; return a == b; }",
			floyd_ast::value_t(true)
		);
		QUARK_UT_VERIFY(false);
	}
	catch(...){
	}
}


QUARK_UNIT_TESTQ("struct", "Can define struct, instantiate it and read member data"){
	const auto a = run_main(
		"struct pixel { string s; }"
		"string main(){\n"
		"	pixel p = pixel_constructor();"
		"	return p.s;"
		"}\n",
		{}
	);
	QUARK_TEST_VERIFY(a.second == value_t(""));
}

QUARK_UNIT_TESTQ("struct", "Struct member default value"){
	const auto a = run_main(
		"struct pixel { string s = \"one\"; }"
		"string main(){\n"
		"	pixel p = pixel_constructor();"
		"	return p.s;"
		"}\n",
		{}
	);
	QUARK_TEST_VERIFY(a.second == value_t("one"));
}

QUARK_UNIT_TESTQ("struct", "Nesting structs"){
	const auto a = run_main(
		"struct pixel { string s = \"two\"; }"
		"struct image { pixel background_color; int width; int height; }"
		"string main(){\n"
		"	image i = image_constructor();"
		"	return i.background_color.s;"
		"}\n",
		{}
	);
	QUARK_TEST_VERIFY(a.second == value_t("two"));
}

QUARK_UNIT_TESTQ("struct", "Can use struct as argument"){
	const auto a = run_main(
		"string get_s(pixel p){ return p.s; }"
		"struct pixel { string s = \"two\"; }"
		"string main(){\n"
		"	pixel p = pixel_constructor();"
		"	return get_s(p);"
		"}\n",
		{}
	);
	QUARK_TEST_VERIFY(a.second == value_t("two"));
}

QUARK_UNIT_TESTQ("struct", "Can return struct"){
	const auto a = run_main(
		"struct pixel { string s = \"three\"; }"
		"pixel test(){ return pixel_constructor(); }"
		"string main(){\n"
		"	pixel p = test();"
		"	return p.s;"
		"}\n",
		{}
	);
	QUARK_TEST_VERIFY(a.second == value_t("three"));
}
#endif



}	//	floyd_interpreter

