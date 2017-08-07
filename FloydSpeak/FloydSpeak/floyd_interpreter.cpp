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
	value_t execute_statements(const interpreter_t& vm, const vector<shared_ptr<statement_t>>& statements){
		QUARK_ASSERT(vm.check_invariant());
		for(const auto i: statements){ QUARK_ASSERT(i->check_invariant()); };

		auto vm2 = vm;

		int statement_index = 0;
		while(statement_index < statements.size()){
			const auto statement = statements[statement_index];
/*
			if(statement->_bind_statement){
				const auto s = statement->_bind_statement;
				const auto name = s->_identifier;
				if(vm2._call_stack.back()->_values.count(name) != 0){
					throw std::runtime_error("local constant already exists");
				}
				const auto result = evalute_expression(vm2, s->_expression);
				if(!result.is_constant()){
					throw std::runtime_error("unknown variables");
				}
				vm2._call_stack.back()->_values[name] = result.get_constant();
			}
			else
*/
			if(statement->_return_statement){
				const auto expr = statement->_return_statement->_expression;
				const auto result = evalute_expression(vm2, expr);

				if(!result.is_constant()){
					throw std::runtime_error("undefined");
				}

				return result.get_constant();
			}
			else{
				QUARK_ASSERT(false);
			}
			statement_index++;
		}
		return value_t();
	}


}	//	unnamed



value_t make_struct_instance(const interpreter_t& vm, const typeid_t& struct_type);

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
		return make_struct_instance(vm, t);
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


value_t find_global_function(const interpreter_t& vm, const string& function_name){
	const auto s = vm._ast.get_global_scope();

	const auto& symbols = s->get_symbols();
	const auto it = symbols.find(function_name);
	if(it == symbols.end()){
		throw std::runtime_error("Undefined function!");
	}
	const auto& symbol = it->second;
	const auto& f = evalute_expression(vm, *symbol._constant);
	if(f.is_constant() && f.get_constant().is_function()){
		return f.get_constant();
	}
	else{
		throw std::runtime_error("Function object not found!");
	}
}


ast_t program_to_ast2(const string& program){
	const auto pass1 = parse_program2(program);
	const auto pass2 = run_pass2(pass1);
	trace(pass2);
	return pass2;
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

expression_t evaluate_call_expression(const interpreter_t& vm, const expression_t& e);


expression_t evalute_expression(const interpreter_t& vm, const expression_t& e){
	QUARK_ASSERT(vm.check_invariant());
	QUARK_ASSERT(e.check_invariant());

	if(e.is_constant()){
		return e;
	}

	const auto e2 = e;
	const auto op = e2.get_operation();

	if(op == expression_t::operation::k_resolve_member){
		const auto parent_expr = evalute_expression(vm, e.get_expressions()[0]);
		if(parent_expr.is_constant() && parent_expr.get_constant().is_struct()){
			const auto struct_instance = parent_expr.get_constant().get_struct();
			const value_t value = struct_instance->_member_values[e.get_symbol()];
			return expression_t::make_constant_value(value);
		}
		else{
			throw std::runtime_error("Resolve member failed.");
		}
	}
	else if(op == expression_t::operation::k_resolve_variable){
		const auto variable_name = e.get_symbol();
		const value_t value = resolve_variable_name(vm, variable_name);
		return expression_t::make_constant_value(value);
	}

	else if(op == expression_t::operation::k_call){
		return evaluate_call_expression(vm, e);
	}

	//	Special-case since it uses 3 expressions & uses shortcut evaluation.
	else if(op == expression_t::operation::k_conditional_operator3){
		const auto cond_result = evalute_expression(vm, e2.get_expressions()[0]);
		if(cond_result.is_constant() && cond_result.get_constant().is_bool()){
			const bool cond_flag = cond_result.get_constant().get_bool();

			//	!!! Only evaluate the CHOSEN expression. Not that importan since functions are pure.
			if(cond_flag){
				return evalute_expression(vm, e2.get_expressions()[1]);
			}
			else{
				return evalute_expression(vm, e2.get_expressions()[2]);
			}
		}
		else{
			throw std::runtime_error("Could not evaluate contion in conditional expression.");
		}
	}

	//	First evaluate all inputs to our operation.
	const auto left_expr = evalute_expression(vm, e2.get_expressions()[0]);
	const auto right_expr = evalute_expression(vm, e2.get_expressions()[1]);

	//	Both left and right are constant, replace the math_operation with a constant!
	if(left_expr.is_constant() && right_expr.is_constant()){
		//	Perform math operation on the two constants => new constant.
		const auto left_constant = left_expr.get_constant();
		const auto right_constant = right_expr.get_constant();

		//	Is operation supported by all types?
		{
			if(op == expression_t::operation::k_math2_smaller_or_equal){
				long diff = value_t::compare_value_true_deep(left_constant, right_constant);
				return expression_t::make_constant_bool(diff <= 0);
			}
			else if(op == expression_t::operation::k_math2_smaller){
				long diff = value_t::compare_value_true_deep(left_constant, right_constant);
				return expression_t::make_constant_bool(diff < 0);
			}
			else if(op == expression_t::operation::k_math2_larger_or_equal){
				long diff = value_t::compare_value_true_deep(left_constant, right_constant);
				return expression_t::make_constant_bool(diff >= 0);
			}
			else if(op == expression_t::operation::k_math2_larger){
				long diff = value_t::compare_value_true_deep(left_constant, right_constant);
				return expression_t::make_constant_bool(diff > 0);
			}


			else if(op == expression_t::operation::k_logical_equal){
				long diff = value_t::compare_value_true_deep(left_constant, right_constant);
				return expression_t::make_constant_bool(diff == 0);
			}
			else if(op == expression_t::operation::k_logical_nonequal){
				long diff = value_t::compare_value_true_deep(left_constant, right_constant);
				return expression_t::make_constant_bool(diff != 0);
			}
		}

		//	bool
		if(left_constant.is_bool() && right_constant.is_bool()){
			const bool left = left_constant.get_bool();
			const bool right = right_constant.get_bool();

			if(op == expression_t::operation::k_math2_add
			|| op == expression_t::operation::k_math2_subtract
			|| op == expression_t::operation::k_math2_multiply
			|| op == expression_t::operation::k_math2_divide
			|| op == expression_t::operation::k_math2_remainder
			){
				throw std::runtime_error("Arithmetics on bool not allowed.");
			}

			else if(op == expression_t::operation::k_logical_and){
				return expression_t::make_constant_bool(left && right);
			}
			else if(op == expression_t::operation::k_logical_or){
				return expression_t::make_constant_bool(left || right);
			}
			else if(op == expression_t::operation::k_logical_negate){
				return expression_t::make_constant_bool(!left);
			}
			else{
				QUARK_ASSERT(false);
			}
		}

		//	int
		else if(left_constant.is_int() && right_constant.is_int()){
			const int left = left_constant.get_int();
			const int right = right_constant.get_int();

			if(op == expression_t::operation::k_math2_add){
				return expression_t::make_constant_int(left + right);
			}
			else if(op == expression_t::operation::k_math2_subtract){
				return expression_t::make_constant_int(left - right);
			}
			else if(op == expression_t::operation::k_math2_multiply){
				return expression_t::make_constant_int(left * right);
			}
			else if(op == expression_t::operation::k_math2_divide){
				if(right == 0){
					throw std::runtime_error("EEE_DIVIDE_BY_ZERO");
				}
				return expression_t::make_constant_int(left / right);
			}
			else if(op == expression_t::operation::k_math2_remainder){
				if(right == 0){
					throw std::runtime_error("EEE_DIVIDE_BY_ZERO");
				}
				return expression_t::make_constant_int(left % right);
			}

			else if(op == expression_t::operation::k_logical_and){
				return expression_t::make_constant_bool((left != 0) && (right != 0));
			}
			else if(op == expression_t::operation::k_logical_or){
				return expression_t::make_constant_bool((left != 0) || (right != 0));
			}
			else if(op == expression_t::operation::k_logical_negate){
				return expression_t::make_constant_bool(left ? false : true);
			}
			else{
				QUARK_ASSERT(false);
			}
		}

		//	float
		else if(left_constant.is_float() && right_constant.is_float()){
			const float left = left_constant.get_float();
			const float right = right_constant.get_float();

			if(op == expression_t::operation::k_math2_add){
				return expression_t::make_constant_float(left + right);
			}
			else if(op == expression_t::operation::k_math2_subtract){
				return expression_t::make_constant_float(left - right);
			}
			else if(op == expression_t::operation::k_math2_multiply){
				return expression_t::make_constant_float(left * right);
			}
			else if(op == expression_t::operation::k_math2_divide){
				if(right == 0.0f){
					throw std::runtime_error("EEE_DIVIDE_BY_ZERO");
				}
				return expression_t::make_constant_float(left / right);
			}
			else if(op == expression_t::operation::k_math2_remainder){
				throw std::runtime_error("Modulo operation on float not allowed.");
			}


			else if(op == expression_t::operation::k_logical_and){
				return expression_t::make_constant_bool((left != 0.0f) && (right != 0.0f));
			}
			else if(op == expression_t::operation::k_logical_or){
				return expression_t::make_constant_bool((left != 0.0f) || (right != 0.0f));
			}
			else if(op == expression_t::operation::k_logical_negate){
				return expression_t::make_constant_bool(left ? false : true);
			}
			else{
				QUARK_ASSERT(false);
			}
		}

		//	string
		else if(left_constant.is_string() && right_constant.is_string()){
			const string left = left_constant.get_string();
			const string right = right_constant.get_string();

			if(op == expression_t::operation::k_math2_add){
				return expression_t::make_constant_string(left + right);
			}

			else if(op == expression_t::operation::k_math2_subtract){
				throw std::runtime_error("Operation not allowed on string.");
			}
			else if(op == expression_t::operation::k_math2_multiply){
				throw std::runtime_error("Operation not allowed on string.");
			}
			else if(op == expression_t::operation::k_math2_divide){
				throw std::runtime_error("Operation not allowed on string.");
			}
			else if(op == expression_t::operation::k_math2_remainder){
				throw std::runtime_error("Operation not allowed on string.");
			}


			else if(op == expression_t::operation::k_logical_and){
				throw std::runtime_error("Operation not allowed on string.");
			}
			else if(op == expression_t::operation::k_logical_or){
				throw std::runtime_error("Operation not allowed on string.");
			}
			else if(op == expression_t::operation::k_logical_negate){
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

			if(op == expression_t::operation::k_math2_add){
				//??? allow
				throw std::runtime_error("Operation not allowed on struct.");
			}
			else if(op == expression_t::operation::k_math2_subtract){
				throw std::runtime_error("Operation not allowed on struct.");
			}
			else if(op == expression_t::operation::k_math2_multiply){
				throw std::runtime_error("Operation not allowed on struct.");
			}
			else if(op == expression_t::operation::k_math2_divide){
				throw std::runtime_error("Operation not allowed on struct.");
			}
			else if(op == expression_t::operation::k_math2_remainder){
				throw std::runtime_error("Operation not allowed on struct.");
			}

			else if(op == expression_t::operation::k_logical_and){
				throw std::runtime_error("Operation not allowed on struct.");
			}
			else if(op == expression_t::operation::k_logical_or){
				throw std::runtime_error("Operation not allowed on struct.");
			}
			else if(op == expression_t::operation::k_logical_negate){
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
		return expression_t::make_math2_operation(op, left_expr, right_expr);
	}
}



value_t call_function(const interpreter_t& vm, const floyd_parser::value_t& f, const vector<value_t>& args){
	QUARK_ASSERT(vm.check_invariant());
	QUARK_ASSERT(f.check_invariant());
	for(const auto i: args){ QUARK_ASSERT(i.check_invariant()); };

	///??? COMPARE arg count and types.

	if(f.is_function() == false){
		throw std::runtime_error("Cannot call non-function.");
	}

	scope_ref_t s;
	{
		const auto& obj = f.get_function();
		const auto& function_id = obj->_function_id;

		const auto& current = vm._call_stack.back()->_def;

		//??? Needs to look in correct scope to find function object.

		const auto& objects = current->get_objects();
		const auto objectIt = objects.find(function_id);
		if(objectIt == objects.end()){
			throw std::runtime_error("Function object not found!");
		}
		s = objectIt->second;
	}

	stack_frame_t new_frame;
	new_frame._def = s;

	//	Copy only input arguments to the function scope.
	//	The function's local variables are null until written by a statement.
	for(int i = 0 ; i < s->_args.size() ; i++){
		const auto& arg_name = s->_args[i]._name;
		const auto& arg_value = args[i];
		new_frame._values[arg_name] = arg_value;
	}

	interpreter_t vm2 = vm;
	vm2._call_stack.push_back(make_shared<stack_frame_t>(new_frame));
	const auto value = execute_statements(vm2, s->_statements);
	if(value.is_null()){
		throw std::runtime_error("function missing return statement");
	}
	else{
		return value;
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
	QUARK_ASSERT(e.get_operation() == expression_t::operation::k_call);

	//	Simplify each input expression: expression[0]: which function to call, expression[1]: first argument if any.
	const auto& expressions = e.get_expressions();
	expression_t function = evalute_expression(vm, expressions[0]);
	vector<expression_t> args2;
	for(int i = 1 ; i < expressions.size() ; i++){
		const auto t = evalute_expression(vm, expressions[i]);
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
		expression_t::make_constant_int(1234),
		expression_t::make_constant_int(1234)
	);
}

QUARK_UNIT_TESTQ("evalute_expression()", "Simple expressions") {
	test_evaluate_simple(
		expression_t::make_math2_operation(
			expression_t::operation::k_math2_add,
			expression_t::make_constant_int(1),
			expression_t::make_constant_int(2)
		),
		expression_t::make_constant_int(3)
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
	const auto result = run_main(program,
		vector<floyd_parser::value_t>{}
	);
	QUARK_TRACE_SS("expect:" << expected_return.value_and_type_to_string());
	QUARK_TRACE_SS("result:" << result.second.value_and_type_to_string());
	return result.second == expected_return;
}


QUARK_UNIT_1("run_main()", "", test_prg("bool main(){ return 4 < 5; }", floyd_parser::value_t(true)));
QUARK_UNIT_1("run_main()", "", test_prg("bool main(){ return 5 < 4; }", floyd_parser::value_t(false)));
QUARK_UNIT_1("run_main()", "", test_prg("bool main(){ return 4 <= 4; }", floyd_parser::value_t(true)));

#if false
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

#endif





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

