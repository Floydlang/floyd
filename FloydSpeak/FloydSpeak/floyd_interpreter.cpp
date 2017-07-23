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
#include "json_to_ast.h"

#include <cmath>

namespace floyd_interpreter {


using std::vector;
using std::string;
using std::pair;
using std::shared_ptr;
using std::unique_ptr;
using std::make_shared;

using namespace floyd_parser;

//??? The order of stuff is confusing in this file!

namespace {

	bool compare_float_approx(float value, float expected){
		float diff = static_cast<float>(fabs(value - expected));
		return diff < 0.00001;
	}

	bool check_arg_types(const scope_ref_t& f, const vector<value_t>& args){
		if(f->_args.size() != args.size()){
			return false;
		}

		for(int i = 0 ; i < args.size() ; i++){
			const auto farg = f->_args[i]._type;
			const auto call_arg = args[i].get_type();
			if(!(*farg == *call_arg)){
				return false;
			}
		}
		return true;
	}

	interpreter_t open_function_scope(const interpreter_t& vm, const scope_ref_t& f, const vector<value_t>& args){
		QUARK_ASSERT(vm.check_invariant());
		QUARK_ASSERT(f && f->check_invariant());
		QUARK_ASSERT(f->_type == scope_def_t::etype::k_function_scope);
		for(const auto i: args){ QUARK_ASSERT(i.check_invariant()); };

		if(f->_type == scope_def_t::etype::k_function_scope && !check_arg_types(f, args)){
			throw std::runtime_error("function arguments do not match function");
		}

		stack_frame_t new_frame;
		new_frame._def = f;

		//	Copy only input arguments to the function scope. The function's local variables are null until written by a statement.
		//	??? Precalculate local variables / constants when possible!
		for(int i = 0 ; i < args.size() ; i++){
			const auto& arg_name = f->_args[i]._name;
			const auto& arg_value = args[i];
			new_frame._values[arg_name] = arg_value;
		}

		interpreter_t result = vm;
		result._call_stack.push_back(make_shared<stack_frame_t>(new_frame));
		return result;
	}

#if 0
	value_t call_host_function(const interpreter_t& vm, const scope_ref_t& f, const vector<value_t>& args){
		QUARK_ASSERT(vm.check_invariant());
		QUARK_ASSERT(f && f->check_invariant());
		QUARK_ASSERT(f->_statements.empty());
		QUARK_ASSERT(f->_host_function);
		QUARK_ASSERT(f->_host_function_param);

		for(const auto i: args){ QUARK_ASSERT(i.check_invariant()); };

		if(!check_arg_types(f, args)){
			throw std::runtime_error("function arguments do not match function");
		}

		const auto resolved_path = vm.get_resolved_path();
		const auto a = f->_host_function(vm._ast, resolved_path, f->_host_function_param, args);
		return a;
		return {};
	}
#endif

	/*
		Return value:
			null = statements were all executed through.
			value = return statement returned a value.
	*/
	value_t execute_statements(const interpreter_t& vm, const vector<shared_ptr<statement_t>>& statements){
		QUARK_ASSERT(vm.check_invariant());
		for(const auto i: statements){ QUARK_ASSERT(i->check_invariant()); };

		auto vm2 = vm;

		int statement_index = 0;
		while(statement_index < statements.size()){
			const auto statement = statements[statement_index];
			if(statement->_bind_statement){
				const auto s = statement->_bind_statement;
				const auto name = s->_identifier;
				if(vm2._call_stack.back()->_values.count(name) != 0){
					throw std::runtime_error("local constant already exists");
				}
				const auto result = evalute_expression(vm2, s->_expression);
				if(!result._constant){
					throw std::runtime_error("unknown variables");
				}
				vm2._call_stack.back()->_values[name] = *result._constant;
			}
			else if(statement->_return_statement){
				const auto expr = statement->_return_statement->_expression;
				const auto result = evalute_expression(vm2, expr);

				if(!result._constant){
					throw std::runtime_error("undefined");
				}

				return *result._constant;
			}
			else{
				QUARK_ASSERT(false);
			}
			statement_index++;
		}
		return value_t();
	}


	value_t call_interpreted_function(const interpreter_t& vm, const scope_ref_t& f, const vector<value_t>& args){
		QUARK_ASSERT(vm.check_invariant());
		QUARK_ASSERT(f && f->check_invariant());
		QUARK_ASSERT(!f->_statements.empty());
		for(const auto i: args){ QUARK_ASSERT(i.check_invariant()); };

#if false
		if(f->_type == scope_def_t::k_function_scope && !check_arg_types(f, args)){
			throw std::runtime_error("function arguments do not match function");
		}
#endif

		auto vm2 = open_function_scope(vm, f, args);
		const auto& statements = f->_statements;
		const auto value = execute_statements(vm2, statements);
		if(value.is_null()){
			throw std::runtime_error("function missing return statement");
		}
		else{
			return value;
		}
	}

}	//	unnamed



	value_t make_struct_instance(const interpreter_t& vm, const shared_ptr<const type_def_t>& struct_type);

value_t make_default_value(const interpreter_t& vm, const shared_ptr<const type_def_t>& type_def){
	QUARK_ASSERT(vm.check_invariant());
	QUARK_ASSERT(type_def && type_def->check_invariant());

	const auto type = type_def->get_base_type();
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
		return make_struct_instance(vm, type_def);
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

value_t make_struct_instance(const interpreter_t& vm, const shared_ptr<const type_def_t>& struct_type){
	QUARK_ASSERT(vm.check_invariant());
	QUARK_ASSERT(struct_type && struct_type->check_invariant());

	std::map<std::string, value_t> member_values;
	for(int i = 0 ; i < struct_type->get_struct_def()->_members.size() ; i++){
		const auto& member_def = struct_type->get_struct_def()->_members[i];

		const auto member_type = member_def._type;
		if(!member_type){
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
}


value_t call_function(const interpreter_t& vm, const scope_ref_t& f, const vector<value_t>& args){
	QUARK_ASSERT(vm.check_invariant());
	QUARK_ASSERT(f && f->check_invariant());
	for(const auto i: args){ QUARK_ASSERT(i.check_invariant()); };

#if false
	if(f->_type == scope_def_t::k_function_scope && !check_arg_types(f, args)){
		throw std::runtime_error("function arguments do not match function");
	}
#endif

	if(f->_function_variant == scope_def_t::efunc_variant::k_interpreted){
		return call_interpreted_function(vm, f, args);
	}
	else if(f->_function_variant == scope_def_t::efunc_variant::k_default_constructor){
		const auto struct_type = f->_return_type;
		const auto instance = make_struct_instance(vm, struct_type);
		return instance;
	}
	else{
		QUARK_ASSERT(false);
	}
}

scope_ref_t find_global_function(const interpreter_t& vm, const string& name){
	for(const auto p: vm._ast.get_typenames()){
		const auto type_def = p.second;
		if(type_def->get_base_type() == base_type::k_function && type_def->get_function_def()->_name.to_string() == name){
			return type_def->get_function_def();
		}
	}
	throw std::runtime_error("Unknown global function");
}


ast_t program_to_ast2(const string& program){
	const auto pass1 = parse_program(program);
	const auto pass2 = run_pass2(pass1);

	const ast_t ast = json_to_ast(pass2);
	trace(ast);
	return ast;
}

floyd_parser::value_t resolve_variable_name_deep(const std::vector<shared_ptr<stack_frame_t>>& stack_frames, const std::string& s, size_t depth){
	QUARK_ASSERT(depth < stack_frames.size());
	QUARK_ASSERT(depth >= 0);

	const auto it = stack_frames[depth]->_values.find(s);
	if(it != stack_frames[depth]->_values.end()){
		return it->second;
	}
	else if(depth > 0){
		return resolve_variable_name_deep(stack_frames, s, depth - 1);
	}
	else{
		return {};
	}
}

floyd_parser::value_t resolve_variable_name(const interpreter_t& vm, const std::string& s){
	QUARK_ASSERT(vm.check_invariant());
	QUARK_ASSERT(s.size() > 0);

	return resolve_variable_name_deep(vm._call_stack, s, vm._call_stack.size() - 1);
}



QUARK_UNIT_TESTQ("C++ bool", ""){
	quark::ut_compare(true, true);
	quark::ut_compare(true, !false);
	quark::ut_compare(false, false);
	quark::ut_compare(!false, true);

	const auto x = false + false;
	const auto y = false - false;

	QUARK_UT_VERIFY(x == false);
	QUARK_UT_VERIFY(y == false);
}





//	??? Return constant instead o expression? We use this both for interpretation but also for compile-time optimizations.
expression_t evaluate_math2(const interpreter_t& vm, const expression_t& e){
	QUARK_ASSERT(vm.check_invariant());
	QUARK_ASSERT(e.check_invariant());
	QUARK_ASSERT(e._math2);

	const auto e2 = *e._math2;
	const auto op = e2._operation;
	const auto left_expr = evalute_expression(vm, e2._left);
	const auto right_expr = evalute_expression(vm, e2._right);

	//	Both left and right are constant, replace the math_operation with a constant!
	if(left_expr._constant && right_expr._constant){

		//	Perform math operation on the two constants => new constant.
		//??? check this in pass2 at compile time!

		//	Is operation supported by all types?
		{
			if(op == expression_t::math2_operation::k_smaller_or_equal){
				long diff = value_t::compare_value_true_deep(*left_expr._constant, *right_expr._constant);
				return expression_t::make_constant(diff <= 0);
			}
			else if(op == expression_t::expression_t::math2_operation::k_smaller){
				long diff = value_t::compare_value_true_deep(*left_expr._constant, *right_expr._constant);
				return expression_t::make_constant(diff < 0);
			}
			else if(op == expression_t::math2_operation::k_larger_or_equal){
				long diff = value_t::compare_value_true_deep(*left_expr._constant, *right_expr._constant);
				return expression_t::make_constant(diff >= 0);
			}
			else if(op == expression_t::math2_operation::k_larger){
				long diff = value_t::compare_value_true_deep(*left_expr._constant, *right_expr._constant);
				return expression_t::make_constant(diff > 0);
			}


			else if(op == expression_t::math2_operation::k_logical_equal){
				long diff = value_t::compare_value_true_deep(*left_expr._constant, *right_expr._constant);
				return expression_t::make_constant(diff == 0);
			}
			else if(op == expression_t::math2_operation::k_logical_nonequal){
				long diff = value_t::compare_value_true_deep(*left_expr._constant, *right_expr._constant);
				return expression_t::make_constant(diff != 0);
			}
		}

		//	bool
		if(left_expr._constant->is_bool() && right_expr._constant->is_bool()){
			const bool left = left_expr._constant->get_bool();
			const bool right = right_expr._constant->get_bool();

			if(op == expression_t::math2_operation::k_add
			|| op == expression_t::math2_operation::k_subtract
			|| op == expression_t::math2_operation::k_multiply
			|| op == expression_t::math2_operation::k_divide
			|| op == expression_t::math2_operation::k_remainder
			){
				throw std::runtime_error("Arithmetics on bool not allowed.");
			}

			else if(op == expression_t::math2_operation::k_logical_and){
				return expression_t::make_constant(left && right);
			}
			else if(op == expression_t::math2_operation::k_logical_or){
				return expression_t::make_constant(left || right);
			}
			else{
				QUARK_ASSERT(false);
			}
		}

		//	int
		else if(left_expr._constant->is_int() && right_expr._constant->is_int()){
			const int left = left_expr._constant->get_int();
			const int right = right_expr._constant->get_int();

			if(op == expression_t::math2_operation::k_add){
				return expression_t::make_constant(left + right);
			}
			else if(op == expression_t::math2_operation::k_subtract){
				return expression_t::make_constant(left - right);
			}
			else if(op == expression_t::math2_operation::k_multiply){
				return expression_t::make_constant(left * right);
			}
			else if(op == expression_t::math2_operation::k_divide){
				if(right == 0){
					throw std::runtime_error("EEE_DIVIDE_BY_ZERO");
				}
				return expression_t::make_constant(left / right);
			}
			else if(op == expression_t::math2_operation::k_remainder){
				if(right == 0){
					throw std::runtime_error("EEE_DIVIDE_BY_ZERO");
				}
				return expression_t::make_constant(left % right);
			}

			else if(op == expression_t::math2_operation::k_logical_and){
				return expression_t::make_constant((left != 0) && (right != 0));
			}
			else if(op == expression_t::math2_operation::k_logical_or){
				return expression_t::make_constant((left != 0) || (right != 0));
			}
			else{
				QUARK_ASSERT(false);
			}
		}

		//	float
		else if(left_expr._constant->is_float() && right_expr._constant->is_float()){
			const float left = left_expr._constant->get_float();
			const float right = right_expr._constant->get_float();

			if(op == expression_t::math2_operation::k_add){
				return expression_t::make_constant(left + right);
			}
			else if(op == expression_t::math2_operation::k_subtract){
				return expression_t::make_constant(left - right);
			}
			else if(op == expression_t::math2_operation::k_multiply){
				return expression_t::make_constant(left * right);
			}
			else if(op == expression_t::math2_operation::k_divide){
				if(right == 0.0f){
					throw std::runtime_error("EEE_DIVIDE_BY_ZERO");
				}
				return expression_t::make_constant(left / right);
			}
			else if(op == expression_t::math2_operation::k_remainder){
				throw std::runtime_error("Modulo operation on float not allowed.");
			}


			else if(op == expression_t::math2_operation::k_logical_and){
				return expression_t::make_constant((left != 0.0f) && (right != 0.0f));
			}
			else if(op == expression_t::math2_operation::k_logical_or){
				return expression_t::make_constant((left != 0.0f) || (right != 0.0f));
			}
			else{
				QUARK_ASSERT(false);
			}
		}

		//	string
		else if(left_expr._constant->is_string() && right_expr._constant->is_string()){
			const string left = left_expr._constant->get_string();
			const string right = right_expr._constant->get_string();

			if(op == expression_t::math2_operation::k_add){
				return expression_t::make_constant(left + right);
			}

			else if(op == expression_t::math2_operation::k_subtract){
				throw std::runtime_error("Operation not allowed on string.");
			}
			else if(op == expression_t::math2_operation::k_multiply){
				throw std::runtime_error("Operation not allowed on string.");
			}
			else if(op == expression_t::math2_operation::k_divide){
				throw std::runtime_error("Operation not allowed on string.");
			}
			else if(op == expression_t::math2_operation::k_remainder){
				throw std::runtime_error("Operation not allowed on string.");
			}


			else if(op == expression_t::math2_operation::k_logical_and){
				throw std::runtime_error("Operation not allowed on string.");
			}
			else if(op == expression_t::math2_operation::k_logical_or){
				throw std::runtime_error("Operation not allowed on string.");
			}
			else{
				QUARK_ASSERT(false);
			}
		}
		//	struct
		else if(left_expr._constant->is_struct() && right_expr._constant->is_struct()){
			const auto left = left_expr._constant->get_struct();
			const auto right = right_expr._constant->get_struct();

			if(!(*left->_struct_type == *right->_struct_type)){
				throw std::runtime_error("Struct type mismatch.");
			}

			if(op == expression_t::math2_operation::k_add){
				//??? allow
				throw std::runtime_error("Operation not allowed on struct.");
			}
			else if(op == expression_t::math2_operation::k_subtract){
				throw std::runtime_error("Operation not allowed on struct.");
			}
			else if(op == expression_t::math2_operation::k_multiply){
				throw std::runtime_error("Operation not allowed on struct.");
			}
			else if(op == expression_t::math2_operation::k_divide){
				throw std::runtime_error("Operation not allowed on struct.");
			}
			else if(op == expression_t::math2_operation::k_remainder){
				throw std::runtime_error("Operation not allowed on struct.");
			}

			else if(op == expression_t::math2_operation::k_logical_and){
				throw std::runtime_error("Operation not allowed on struct.");
			}
			else if(op == expression_t::math2_operation::k_logical_or){
				throw std::runtime_error("Operation not allowed on struct.");
			}
			else{
				QUARK_ASSERT(false);
			}
		}

		else if(left_expr._constant->is_vector() && right_expr._constant->is_vector()){
			QUARK_ASSERT(false);
		}
		else{
			throw std::runtime_error("Arithmetics failed.");
		}
	}

	//	Else use a math_operation expression to perform the calculation later.
	//	We make a NEW math_operation since sub-nodes may have been evaluated.
	else{
		return expression_t::make_math_operation2(op, left_expr, right_expr);
	}
}

/*
	If inputs are constant, replace function call with a constant!
	??? Have different expression-classes to tell if they are resolved / unresolved. Makes it possible to execute both types of expression but not check at runtime.
*/
expression_t evaluate_conditional_operator(const interpreter_t& vm, const expression_t& e){
	QUARK_ASSERT(vm.check_invariant());
	QUARK_ASSERT(e.check_invariant());
	QUARK_ASSERT(e._conditional_operator);

	const auto& ce = *e._conditional_operator;

	const auto cond_result = evalute_expression(vm, ce._condition);
	if(cond_result._constant&& cond_result._constant->is_bool()){
		const bool cond_flag = cond_result._constant->get_bool();

		//	!!! Only evaluate the CHOSEN expression. Not that importan since functions are pure.
		if(cond_flag){
			return evalute_expression(vm, ce._a);
		}
		else{
			return evalute_expression(vm, ce._b);
		}
	}
	else{
		throw std::runtime_error("Could not evaluate contion in conditional expression.");
	}
}

expression_t evaluate_call(const interpreter_t& vm, const expression_t& e){
	QUARK_ASSERT(vm.check_invariant());
	QUARK_ASSERT(e.check_invariant());
	QUARK_ASSERT(e._call);

	const auto& call = *e._call;

	scope_ref_t scope_def = vm._call_stack.back()->_def;

//??? function prototype == the TYPE. VALUE refers to WHICH implementation of the TYPE.
#if 1
	const auto function_name = e._call->_function.to_string();

	//	find function symbol: no proper static scoping ???
	const auto found_it = find_if(
		vm._ast.get_typenames().begin(),
		vm._ast.get_typenames().end(),
		[&] (const std::pair<std::string, std::shared_ptr<const type_def_t>>& t) {
			return t.second->get_base_type() == base_type::k_function && t.second->get_function_def()->_name.to_string() == function_name;
		}
	);
	if(found_it == vm._ast.get_typenames().end()){
		throw std::runtime_error("Failed calling function - unresolved function.");
	}
	const auto type = found_it->second;

	if(!type || type->get_base_type() != base_type::k_function){
		throw std::runtime_error("Failed calling function - unresolved function.");
	}
#else
	const auto function_address = evalute_expression(vm, call._function);
	QUARK_ASSERT(function_address._constant && function_address._constant->get_base_type() == base_type::k_function);
	const auto type = function_address._constant->get_function();
#endif

	const auto& function_def = type->get_function_def();
	if(function_def->_type == scope_def_t::etype::k_function_scope){
		QUARK_ASSERT(function_def->_args.size() == call._inputs.size());
	}
	else if(function_def->_type == scope_def_t::etype::k_subscope){
	}
	else{
		QUARK_ASSERT(false);
	}

	//	Simplify each argument.
	vector<expression_t> simplified_args;
	for(const auto& i: call._inputs){
		const auto arg_expr = evalute_expression(vm, i);
		simplified_args.push_back(arg_expr);
	}

	//	All arguments to functions are constants? Else return new call_function, but with simplified arguments.
	for(const auto& i: simplified_args){
		if(!i._constant){
			//??? should use simplified_args.
			return expression_t::make_function_call(call._function, call._inputs, e.get_expression_type());
		}
	}

	//	Woha: all arguments are constants - replace this expression with the final output of the function call instead!
	vector<value_t> constant_args;
	for(const auto& i: simplified_args){
		constant_args.push_back(*i._constant);
	}

	const value_t result = call_function(vm, function_def, constant_args);
	return expression_t::make_constant(result);
}


//### Test string + etc.

expression_t evalute_expression(const interpreter_t& vm, const expression_t& e){
	QUARK_ASSERT(vm.check_invariant());
	QUARK_ASSERT(e.check_invariant());

	if(e._constant){
		return e;
	}
	else if(e._math2){
		return evaluate_math2(vm, e);
	}
	else if(e._math1){
		const auto e2 = *e._math1;
		const auto input = evalute_expression(vm, e2._input);

		//	Replace the with a constant!
		if(input._constant){
			const auto value = input._constant;
			const auto base_type = value->get_base_type();

			if(base_type == base_type::k_bool){
				throw std::runtime_error("Arithmetics failed.");
			}
			else if(base_type == base_type::k_int){
				if(e2._operation == expression_t::math1_operation::negate){
					return expression_t::make_constant(-value->get_int());
				}
				else{
					QUARK_ASSERT(false);
				}
			}
			else if(base_type == base_type::k_float){
				if(e2._operation == expression_t::math1_operation::negate){
					return expression_t::make_constant(-value->get_float());
				}
				else{
					QUARK_ASSERT(false);
				}
			}
			else if(base_type == base_type::k_string){
				throw std::runtime_error("Arithmetics failed.");
			}
			else{
				throw std::runtime_error("Arithmetics failed.");
			}
		}

		//	Else use a math_operation to make the calculation later. We make a NEW math_operation since sub-nodes may have been evaluated.
		else{
			return expression_t::make_math_operation1(e2._operation, input);
		}
	}

	else if(e._conditional_operator){
		return evaluate_conditional_operator(vm, e);
	}

	/*
		If inputs are constant, replace function call with a constant!
	*/
	else if(e._call){
		return evaluate_call(vm, e);
	}
	else if(e._resolve_variable){
		const auto variable_name = e._resolve_variable->_variable_name;
		const value_t value = resolve_variable_name(vm, variable_name);
		return expression_t::make_constant(value);
	}
	else if(e._resolve_member){
		const auto parent_expr = evalute_expression(vm, e._resolve_member->_parent_address);
		if(parent_expr._constant && parent_expr._constant->is_struct()){
			const auto struct_instance = parent_expr._constant->get_struct();
			const value_t value = struct_instance->_member_values[e._resolve_member->_member_name];
			return expression_t::make_constant(value);
		}
		else{
			throw std::runtime_error("Resolve member failed.");
		}
	}
	else if(e._lookup_element){
		QUARK_ASSERT(false);
		return e;
	}
	else{
		QUARK_ASSERT(false);
	}
}


void test_evaluate_simple(const expression_t& e, const expression_t& expected){
	const ast_t ast;
	const interpreter_t interpreter(ast);
	const auto e3 = evalute_expression(interpreter, e);
	QUARK_TEST_VERIFY(e3 == expected);
}

//??? Change so built-in types don't have to be registered to be used -- only custom types needs that.
//??? Add tests for strings and floats.

QUARK_UNIT_TESTQ("evalute_expression()", "Simple expressions") {
	test_evaluate_simple(
		expression_t::make_constant(1234),
		expression_t::make_constant(1234)
	);
}

QUARK_UNIT_TESTQ("evalute_expression()", "Simple expressions") {
	test_evaluate_simple(
		expression_t::make_math_operation2(
			expression_t::math2_operation::k_add,
			expression_t::make_constant(1),
			expression_t::make_constant(2)
		),
		expression_t::make_constant(3)
	);
}

#if false

QUARK_UNIT_TESTQ("evalute_expression()", "Simple expressions") {
	QUARK_TEST_VERIFY(test_evaluate_simple("1+2+3") == expression_t::make_constant(6));
}

QUARK_UNIT_TESTQ("evalute_expression()", "Simple expressions") {
	QUARK_TEST_VERIFY(test_evaluate_simple("3*4") == expression_t::make_constant(12));
}
QUARK_UNIT_TESTQ("evalute_expression()", "Simple expressions") {
	QUARK_TEST_VERIFY(test_evaluate_simple("3*4*5") == expression_t::make_constant(60));
}
QUARK_UNIT_TESTQ("evalute_expression()", "Simple expressions") {
	QUARK_TEST_VERIFY(test_evaluate_simple("1+2*3") == expression_t::make_constant(7));
}


QUARK_UNIT_TESTQ("evalute_expression()", "Parenthesis") {
	QUARK_TEST_VERIFY(test_evaluate_simple("5*(4+4+1)") == expression_t::make_constant(45));
}
QUARK_UNIT_TESTQ("evalute_expression()", "Parenthesis") {
	QUARK_TEST_VERIFY(test_evaluate_simple("5*(2*(1+3)+1)") == expression_t::make_constant(45));
}
QUARK_UNIT_TESTQ("evalute_expression()", "Parenthesis") {
	QUARK_TEST_VERIFY(test_evaluate_simple("5*((1+3)*2+1)") == expression_t::make_constant(45));
}


QUARK_UNIT_TESTQ("evalute_expression()", "Spaces") {
	QUARK_TEST_VERIFY(test_evaluate_simple(" 5 * ((1 + 3) * 2 + 1) ") == expression_t::make_constant(45));
}
QUARK_UNIT_TESTQ("evalute_expression()", "Spaces") {
	QUARK_TEST_VERIFY(test_evaluate_simple(" 5 - 2 * ( 3 ) ") == expression_t::make_constant(-1));
}
QUARK_UNIT_TESTQ("evalute_expression()", "Spaces") {
	QUARK_TEST_VERIFY(test_evaluate_simple(" 5 - 2 * ( ( 4 )  - 1 ) ") == expression_t::make_constant(-1));
}


QUARK_UNIT_TESTQ("evalute_expression()", "Sign before parenthesis") {
	QUARK_TEST_VERIFY(test_evaluate_simple("-(2+1)*4") == expression_t::make_constant(-12));
}
QUARK_UNIT_TESTQ("evalute_expression()", "Sign before parenthesis") {
	QUARK_TEST_VERIFY(test_evaluate_simple("-4*(2+1)") == expression_t::make_constant(-12));
}


QUARK_UNIT_TESTQ("evalute_expression()", "Fractional numbers") {
	QUARK_TEST_VERIFY(compare_float_approx(test_evaluate_simple("5.5/5.0")._constant->get_float(), 1.1f));
}
QUARK_UNIT_TESTQ("evalute_expression()", "Fractional numbers") {
//	QUARK_TEST_VERIFY(test_evaluate_simple("1/5e10") == 2e-11);
}
QUARK_UNIT_TESTQ("evalute_expression()", "Fractional numbers") {
	QUARK_TEST_VERIFY(compare_float_approx(test_evaluate_simple("(4.0-3.0)/(4.0*4.0)")._constant->get_float(), 0.0625f));
}
QUARK_UNIT_TESTQ("evalute_expression()", "Fractional numbers") {
	QUARK_TEST_VERIFY(test_evaluate_simple("1.0/2.0/2.0") == expression_t::make_constant(0.25f));
}
QUARK_UNIT_TESTQ("evalute_expression()", "Fractional numbers") {
	QUARK_TEST_VERIFY(test_evaluate_simple("0.25 * .5 * 0.5") == expression_t::make_constant(0.0625f));
}
QUARK_UNIT_TESTQ("evalute_expression()", "Fractional numbers") {
	QUARK_TEST_VERIFY(test_evaluate_simple(".25 / 2.0 * .5") == expression_t::make_constant(0.0625f));
}


QUARK_UNIT_TESTQ("evalute_expression()", "Repeated operators") {
	QUARK_TEST_VERIFY(test_evaluate_simple("1+-2") == expression_t::make_constant(-1));
}
QUARK_UNIT_TESTQ("evalute_expression()", "Repeated operators") {
	QUARK_TEST_VERIFY(test_evaluate_simple("--2") == expression_t::make_constant(2));
}
QUARK_UNIT_TESTQ("evalute_expression()", "Repeated operators") {
	QUARK_TEST_VERIFY(test_evaluate_simple("2---2") == expression_t::make_constant(0));
}
QUARK_UNIT_TESTQ("evalute_expression()", "Repeated operators") {
	QUARK_TEST_VERIFY(test_evaluate_simple("2-+-2") == expression_t::make_constant(4));
}


QUARK_UNIT_TESTQ("evalute_expression()", "Bool") {
	QUARK_TEST_VERIFY(test_evaluate_simple("true") == expression_t::make_constant(true));
}
QUARK_UNIT_TESTQ("evalute_expression()", "Bool") {
	QUARK_TEST_VERIFY(test_evaluate_simple("false") == expression_t::make_constant(false));
}


//??? add tests  for pass2()
QUARK_UNIT_TESTQ("evalute_expression()", "?:") {
	QUARK_TEST_VERIFY(test_evaluate_simple("true ? 4 : 6") == expression_t::make_constant(4));
}
QUARK_UNIT_TESTQ("evalute_expression()", "?:") {
	QUARK_TEST_VERIFY(test_evaluate_simple("false ? 4 : 6") == expression_t::make_constant(6));
}

QUARK_UNIT_TESTQ("evalute_expression()", "?:") {
	QUARK_TEST_VERIFY(test_evaluate_simple("1==3 ? 4 : 6") == expression_t::make_constant(6));
}
QUARK_UNIT_TESTQ("evalute_expression()", "?:") {
	QUARK_TEST_VERIFY(test_evaluate_simple("3==3 ? 4 : 6") == expression_t::make_constant(4));
}

QUARK_UNIT_TESTQ("evalute_expression()", "?:") {
	QUARK_TEST_VERIFY(test_evaluate_simple("3==3 ? 2 + 2 : 2 * 3") == expression_t::make_constant(4));
}

QUARK_UNIT_TESTQ("evalute_expression()", "?:") {
	QUARK_TEST_VERIFY(test_evaluate_simple("3==1+2 ? 2 + 2 : 2 * 3") == expression_t::make_constant(4));
}




QUARK_UNIT_TESTQ("evalute_expression()", "==") {
	QUARK_TEST_VERIFY(test_evaluate_simple("1 == 1") == expression_t::make_constant(true));
}
QUARK_UNIT_TESTQ("evalute_expression()", "==") {
	QUARK_TEST_VERIFY(test_evaluate_simple("1 == 2") == expression_t::make_constant(false));
}
QUARK_UNIT_TESTQ("evalute_expression()", "==") {
	QUARK_TEST_VERIFY(test_evaluate_simple("1.3 == 1.3") == expression_t::make_constant(true));
}
QUARK_UNIT_TESTQ("evalute_expression()", "==") {
	QUARK_TEST_VERIFY(test_evaluate_simple("\"hello\" == \"hello\"") == expression_t::make_constant(true));
}
QUARK_UNIT_TESTQ("evalute_expression()", "==") {
	QUARK_TEST_VERIFY(test_evaluate_simple("\"hello\" == \"bye\"") == expression_t::make_constant(false));
}


QUARK_UNIT_TESTQ("evalute_expression()", "<") {
	QUARK_TEST_VERIFY(test_evaluate_simple("1 < 2") == expression_t::make_constant(true));
}
QUARK_UNIT_TESTQ("evalute_expression()", "<") {
	QUARK_TEST_VERIFY(test_evaluate_simple("5 < 2") == expression_t::make_constant(false));
}
QUARK_UNIT_TESTQ("evalute_expression()", "<") {
	QUARK_TEST_VERIFY(test_evaluate_simple("0.3 < 0.4") == expression_t::make_constant(true));
}
QUARK_UNIT_TESTQ("evalute_expression()", "<") {
	QUARK_TEST_VERIFY(test_evaluate_simple("1.5 < 0.4") == expression_t::make_constant(false));
}
QUARK_UNIT_TESTQ("evalute_expression()", "<") {
	QUARK_TEST_VERIFY(test_evaluate_simple("\"adwark\" < \"boat\"") == expression_t::make_constant(true));
}
QUARK_UNIT_TESTQ("evalute_expression()", "<") {
	QUARK_TEST_VERIFY(test_evaluate_simple("\"boat\" < \"adwark\"") == expression_t::make_constant(false));
}


QUARK_UNIT_TESTQ("evalute_expression()", "&&") {
	QUARK_TEST_VERIFY(test_evaluate_simple("false && false") == expression_t::make_constant(false));
}
QUARK_UNIT_TESTQ("evalute_expression()", "&&") {
	QUARK_TEST_VERIFY(test_evaluate_simple("false && true") == expression_t::make_constant(false));
}
QUARK_UNIT_TESTQ("evalute_expression()", "&&") {
	QUARK_TEST_VERIFY(test_evaluate_simple("true && false") == expression_t::make_constant(false));
}
QUARK_UNIT_TESTQ("evalute_expression()", "&&") {
	QUARK_TEST_VERIFY(test_evaluate_simple("true && true") == expression_t::make_constant(true));
}

QUARK_UNIT_TESTQ("evalute_expression()", "&&") {
	QUARK_TEST_VERIFY(test_evaluate_simple("false && false && false") == expression_t::make_constant(false));
}
QUARK_UNIT_TESTQ("evalute_expression()", "&&") {
	QUARK_TEST_VERIFY(test_evaluate_simple("false && false && true") == expression_t::make_constant(false));
}
QUARK_UNIT_TESTQ("evalute_expression()", "&&") {
	QUARK_TEST_VERIFY(test_evaluate_simple("false && true && false") == expression_t::make_constant(false));
}
QUARK_UNIT_TESTQ("evalute_expression()", "&&") {
	QUARK_TEST_VERIFY(test_evaluate_simple("false && true && true") == expression_t::make_constant(false));
}
QUARK_UNIT_TESTQ("evalute_expression()", "&&") {
	QUARK_TEST_VERIFY(test_evaluate_simple("true && false && false") == expression_t::make_constant(false));
}
QUARK_UNIT_TESTQ("evalute_expression()", "&&") {
	QUARK_TEST_VERIFY(test_evaluate_simple("true && true && false") == expression_t::make_constant(false));
}
QUARK_UNIT_TESTQ("evalute_expression()", "&&") {
	QUARK_TEST_VERIFY(test_evaluate_simple("true && true && true") == expression_t::make_constant(true));
}


QUARK_UNIT_TESTQ("evalute_expression()", "||") {
	QUARK_TEST_VERIFY(test_evaluate_simple("false || false") == expression_t::make_constant(false));
}
QUARK_UNIT_TESTQ("evalute_expression()", "||") {
	QUARK_TEST_VERIFY(test_evaluate_simple("false || true") == expression_t::make_constant(true));
}
QUARK_UNIT_TESTQ("evalute_expression()", "||") {
	QUARK_TEST_VERIFY(test_evaluate_simple("true || false") == expression_t::make_constant(true));
}
QUARK_UNIT_TESTQ("evalute_expression()", "||") {
	QUARK_TEST_VERIFY(test_evaluate_simple("true || true") == expression_t::make_constant(true));
}


QUARK_UNIT_TESTQ("evalute_expression()", "||") {
	QUARK_TEST_VERIFY(test_evaluate_simple("false || false || false") == expression_t::make_constant(false));
}
QUARK_UNIT_TESTQ("evalute_expression()", "||") {
	QUARK_TEST_VERIFY(test_evaluate_simple("false || false || true") == expression_t::make_constant(true));
}
QUARK_UNIT_TESTQ("evalute_expression()", "||") {
	QUARK_TEST_VERIFY(test_evaluate_simple("false || true || false") == expression_t::make_constant(true));
}
QUARK_UNIT_TESTQ("evalute_expression()", "||") {
	QUARK_TEST_VERIFY(test_evaluate_simple("false || true || true") == expression_t::make_constant(true));
}
QUARK_UNIT_TESTQ("evalute_expression()", "||") {
	QUARK_TEST_VERIFY(test_evaluate_simple("true || false || false") == expression_t::make_constant(true));
}
QUARK_UNIT_TESTQ("evalute_expression()", "||") {
	QUARK_TEST_VERIFY(test_evaluate_simple("true || false || true") == expression_t::make_constant(true));
}
QUARK_UNIT_TESTQ("evalute_expression()", "||") {
	QUARK_TEST_VERIFY(test_evaluate_simple("true || true || false") == expression_t::make_constant(true));
}
QUARK_UNIT_TESTQ("evalute_expression()", "||") {
	QUARK_TEST_VERIFY(test_evaluate_simple("true || true || true") == expression_t::make_constant(true));
}


//??? more



QUARK_UNIT_TESTQ("evalute_expression()", "Division by zero") {
	try{
		test_evaluate_simple("2/0");
		QUARK_TEST_VERIFY(false);
	}
	catch(const std::runtime_error& e){
		QUARK_TEST_VERIFY(string(e.what()) == "EEE_DIVIDE_BY_ZERO");
	}
}

QUARK_UNIT_TESTQ("evaluate_expression()", "Division by zero"){
	try{
		test_evaluate_simple("3+1/(5-5)+4");
		QUARK_TEST_VERIFY(false);
	}
	catch(const std::runtime_error& e){
		QUARK_TEST_VERIFY(string(e.what()) == "EEE_DIVIDE_BY_ZERO");
	}
}

QUARK_UNIT_TESTQ("evalute_expression()", "Multiply errors") {
		//	Multiple errors not possible/relevant now that we use exceptions for errors.
/*
	//////////////////////////		Only one error will be detected (in this case, the last one)
	QUARK_TEST_VERIFY(test_evaluate_simple("3+1/0+4$") == EEE_WRONG_CHAR);

	QUARK_TEST_VERIFY(test_evaluate_simple("3+1/0+4") == EEE_DIVIDE_BY_ZERO);

	// ...or the first one
	QUARK_TEST_VERIFY(test_evaluate_simple("q+1/0)") == EEE_WRONG_CHAR);
	QUARK_TEST_VERIFY(test_evaluate_simple("+1/0)") == EEE_PARENTHESIS);
	QUARK_TEST_VERIFY(test_evaluate_simple("+1/0") == EEE_DIVIDE_BY_ZERO);
*/
}

#endif




//////////////////////////		interpreter_t



interpreter_t::interpreter_t(const floyd_parser::ast_t& ast) :
	_ast(ast)
{
	QUARK_ASSERT(ast.check_invariant());

	auto global_stack_frame = stack_frame_t();
	global_stack_frame._def = ast.get_global_scope();
	_call_stack.push_back(make_shared<stack_frame_t>(global_stack_frame));

	//	Run static intialization (basically run global statements before calling main()).
	{
	}

	QUARK_ASSERT(check_invariant());
}

bool interpreter_t::check_invariant() const {
	QUARK_ASSERT(_ast.check_invariant());
	return true;
}



//////////////////////////		run_main()




std::pair<interpreter_t, floyd_parser::value_t> run_main(const string& source, const vector<floyd_parser::value_t>& args){
	QUARK_ASSERT(source.size() > 0);
	auto ast = program_to_ast2(source);
	auto vm = interpreter_t(ast);
	const auto f = find_global_function(vm, "main");

	const auto r = call_function(vm, f, args);
	return { vm, r };
}

QUARK_UNIT_TESTQ("run_main()", "minimal program 2"){
	const auto result = run_main(
		"string main(string args){\n"
		"	return \"123\" + \"456\";\n"
		"}\n",
		vector<floyd_parser::value_t>{floyd_parser::value_t("program_name 1 2 3 4")}
	);
	QUARK_TEST_VERIFY(result.second == floyd_parser::value_t("123456"));
}


//////////////////////////		TEST conditional expression


QUARK_UNIT_TESTQ("run_main()", "conditional expression"){
	const auto result = run_main(
		R"(
			string main(bool input_flag){
				return input_flag ? "123" : "456";
			}
		)",
		vector<floyd_parser::value_t>{floyd_parser::value_t(true)}
	);
	QUARK_TEST_VERIFY(result.second == floyd_parser::value_t("123"));
}

QUARK_UNIT_TESTQ("run_main()", "conditional expression"){
	const auto result = run_main(
		R"(
			string main(bool input_flag){
				return input_flag ? "123" : "456";
			}
		)",
		vector<floyd_parser::value_t>{floyd_parser::value_t(false)}
	);
	QUARK_TEST_VERIFY(result.second == floyd_parser::value_t("456"));
}





bool test_prg(const std::string& program, const value_t& expected_return){
	QUARK_TRACE_SS("program:" << program);
	QUARK_TRACE_SS("expect:" << expected_return.value_and_type_to_string());
	const auto result = run_main(program,
		vector<floyd_parser::value_t>{}
	);
	QUARK_TRACE_SS("result:" << result.second.value_and_type_to_string());
	return result.second == expected_return;
}


QUARK_UNIT_1("run_main()", "", test_prg("bool main(){ return 4 < 5; }", floyd_parser::value_t(true)));
QUARK_UNIT_1("run_main()", "", test_prg("bool main(){ return 5 < 4; }", floyd_parser::value_t(false)));
QUARK_UNIT_1("run_main()", "", test_prg("bool main(){ return 4 <= 4; }", floyd_parser::value_t(true)));

QUARK_UNIT_TESTQ("run_main()", "struct"){
	QUARK_UT_VERIFY(test_prg("struct t { int a;} bool main(){ t b = t_constructor(); return b == b; }", floyd_parser::value_t(true)));
}



QUARK_UNIT_1("run_main()", "", test_prg(
	"struct t { int a;} bool main(){ t a = t_constructor(); t b = t_constructor(); return a == b; }",
	floyd_parser::value_t(true)
));



QUARK_UNIT_TESTQ("run_main()", ""){
	QUARK_TEST_VERIFY(test_prg(
		"struct t { int a;} bool main(){ t a = t_constructor(); t b = t_constructor(); return a == b; }",
		floyd_parser::value_t(true)
	));
}

QUARK_UNIT_TESTQ("run_main()", ""){
	try {
		test_prg(
			"struct t { int a;} bool main(){ t a = t_constructor(); int b = 1055; return a == b; }",
			floyd_parser::value_t(true)
		);
		QUARK_UT_VERIFY(false);
	}
	catch(...){
	}
}






QUARK_UNIT_TESTQ("call_function()", "minimal program"){
	auto ast = program_to_ast2(
		"int main(string args){\n"
		"	return 3 + 4;\n"
		"}\n"
	);
	auto vm = interpreter_t(ast);
	const auto f = find_global_function(vm, "main");
	const auto result = call_function(vm, f, vector<floyd_parser::value_t>{ floyd_parser::value_t("program_name 1 2 3") });
	QUARK_TEST_VERIFY(result == floyd_parser::value_t(7));
}


QUARK_UNIT_TESTQ("call_function()", "minimal program 2"){
	auto ast = program_to_ast2(
		"string main(string args){\n"
		"	return \"123\" + \"456\";\n"
		"}\n"
	);
	auto vm = interpreter_t(ast);
	const auto f = find_global_function(vm, "main");
	const auto result = call_function(vm, f, vector<floyd_parser::value_t>{ floyd_parser::value_t("program_name 1 2 3") });
	QUARK_TEST_VERIFY(result == floyd_parser::value_t("123456"));
}

QUARK_UNIT_TESTQ("call_function()", "define additional function, call it several times"){
	auto ast = program_to_ast2(
		"int myfunc(){ return 5; }\n"
		"int main(string args){\n"
		"	return myfunc() + myfunc() * 2;\n"
		"}\n"
	);
	auto vm = interpreter_t(ast);
	const auto f = find_global_function(vm, "main");
	const auto result = call_function(vm, f, vector<floyd_parser::value_t>{ floyd_parser::value_t("program_name 1 2 3") });
	QUARK_TEST_VERIFY(result == floyd_parser::value_t(15));
}

QUARK_UNIT_TESTQ("call_function()", "use function inputs"){
	auto ast = program_to_ast2(
		"string main(string args){\n"
		"	return \"-\" + args + \"-\";\n"
		"}\n"
	);
	auto vm = interpreter_t(ast);
	const auto f = find_global_function(vm, "main");
	const auto result = call_function(vm, f, vector<floyd_parser::value_t>{ floyd_parser::value_t("xyz") });
	QUARK_TEST_VERIFY(result == floyd_parser::value_t("-xyz-"));

	const auto result2 = call_function(vm, f, vector<floyd_parser::value_t>{ floyd_parser::value_t("Hello, world!") });
	QUARK_TEST_VERIFY(result2 == floyd_parser::value_t("-Hello, world!-"));
}


QUARK_UNIT_TESTQ("call_function()", "use local variables"){
	auto ast = program_to_ast2(
		"string myfunc(string t){ return \"<\" + t + \">\"; }\n"
		"string main(string args){\n"
		"	 string a = \"--\"; string b = myfunc(args) ; return a + args + b + a;\n"
		"}\n"
	);
	auto vm = interpreter_t(ast);
	const auto f = find_global_function(vm, "main");
	const auto result = call_function(vm, f, vector<floyd_parser::value_t>{ floyd_parser::value_t("xyz") });
	QUARK_TEST_VERIFY(result == floyd_parser::value_t("--xyz<xyz>--"));

	const auto result2 = call_function(vm, f, vector<floyd_parser::value_t>{ floyd_parser::value_t("123") });
	QUARK_TEST_VERIFY(result2 == floyd_parser::value_t("--123<123>--"));
}





//////////////////////////		TEST GLOBAL CONSTANTS




#if false
QUARK_UNIT_TESTQ("struct", "Can make and read global int"){
	const auto a = run_main(
		"int test = 123;"
		"string main(){\n"
		"	return test;"
		"}\n",
		{}
	);
	QUARK_TEST_VERIFY(a.second == value_t(123));
}
#endif


//////////////////////////		TEST STRUCT SUPPORT




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



}	//	floyd_interpreter

