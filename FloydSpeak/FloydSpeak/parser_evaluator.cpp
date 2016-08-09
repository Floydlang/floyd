//
//  parser_evaluator.cpp
//  FloydSpeak
//
//  Created by Marcus Zetterquist on 26/07/16.
//  Copyright © 2016 Marcus Zetterquist. All rights reserved.
//

#include "parser_evaluator.h"


#include "parser_expression.h"
#include "parser_statement.h"
#include "floyd_parser.h"
#include "floyd_vm.h"
#include "parser_value.h"

#include <cmath>

namespace floyd_parser {


using std::vector;
using std::string;
using std::pair;
using std::shared_ptr;
using std::unique_ptr;
using std::make_shared;



namespace {

	bool compare_float_approx(float value, float expected){
		float diff = static_cast<float>(fabs(value - expected));
		return diff < 0.00001;
	}

	bool check_args(const function_def_t& f, const vector<value_t>& args){
		if(f._args.size() != args.size()){
			return false;
		}
		for(int i = 0 ; i < args.size() ; i++){
			if(f._args[i]._type != args[i].get_type()){
				return false;
			}
		}
		return true;
	}

	vm_t open_function_scope(const vm_t& vm, const function_def_t& f, const vector<value_t>& args){
		QUARK_ASSERT(vm.check_invariant());
		QUARK_ASSERT(f.check_invariant());
		for(const auto i: args){ QUARK_ASSERT(i.check_invariant()); };

		if(!check_args(f, args)){
			throw std::runtime_error("function arguments do not match function");
		}

		scope_instance_t new_scope;
		new_scope._def = f._function_scope.get();

		for(int i = 0 ; i < args.size() ; i++){
			const auto& arg_name = f._args[i]._identifier;
			const auto& arg_value = args[i];
			new_scope._values[arg_name] = arg_value;
		}

		vm_t result = vm;
		result._scope_instances.push_back(make_shared<scope_instance_t>(new_scope));
		return result;
	}

}


value_t call_host_function(const vm_t& vm, const function_def_t& f, const vector<value_t>& args){
	QUARK_ASSERT(vm.check_invariant());
	QUARK_ASSERT(f.check_invariant());
	QUARK_ASSERT(f._function_scope->_statements.empty());
	QUARK_ASSERT(f._function_scope->_host_function);
	QUARK_ASSERT(f._function_scope->_host_function_param);

	for(const auto i: args){ QUARK_ASSERT(i.check_invariant()); };

	if(!check_args(f, args)){
		throw std::runtime_error("function arguments do not match function");
	}

//	auto local_scope = add_args(ast, f, args);
	const auto a = f._function_scope->_host_function(f._function_scope->_host_function_param, args);
	return a;
}

value_t execute_statements(const vm_t& vm, const vector<shared_ptr<statement_t>>& statements){
	QUARK_ASSERT(vm.check_invariant());
	for(const auto i: statements){ QUARK_ASSERT(i->check_invariant()); };

	auto vm2 = vm;

	//	??? Should respect {} for local variable scopes!
	int statement_index = 0;
	while(statement_index < statements.size()){
		const auto statement = statements[statement_index];
		if(statement->_bind_statement){
			const auto s = statement->_bind_statement;
			const auto name = s->_identifier;
			if(vm2._scope_instances.back()->_values.count(name) != 0){
				throw std::runtime_error("local constant already exists");
			}
			const auto result = evalute_expression(vm2, *s->_expression);
			if(!result._constant){
				throw std::runtime_error("unknown variables");
			}
			vm2._scope_instances.back()->_values[name] = *result._constant;
		}
		else if(statement->_return_statement){
			const auto expr = statement->_return_statement->_expression;
			const auto result = evalute_expression(vm2, *expr);

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


//??? Make this operate on scope_def_t instead of function_def_t.
value_t call_interpreted_function(const vm_t& vm, const function_def_t& f, const vector<value_t>& args){
	QUARK_ASSERT(vm.check_invariant());
	QUARK_ASSERT(f.check_invariant());
	QUARK_ASSERT(!f._function_scope->_statements.empty());
	QUARK_ASSERT(!f._function_scope->_host_function);
	QUARK_ASSERT(!f._function_scope->_host_function_param);
	for(const auto i: args){ QUARK_ASSERT(i.check_invariant()); };

	if(!check_args(f, args)){
		throw std::runtime_error("function arguments do not match function");
	}

	auto vm2 = open_function_scope(vm, f, args);
	const auto& statements = f._function_scope->_statements;
	const auto value = execute_statements(vm2, statements);
	if(value.is_null()){
	throw std::runtime_error("function missing return statement");
	}
	else{
		return value;
	}
}


value_t run_function(const vm_t& vm, const function_def_t& f, const vector<value_t>& args){
	QUARK_ASSERT(vm.check_invariant());
	QUARK_ASSERT(f.check_invariant());
	for(const auto i: args){ QUARK_ASSERT(i.check_invariant()); };

	if(!check_args(f, args)){
		throw std::runtime_error("function arguments do not match function");
	}

	if(f._function_scope->_host_function){
		return call_host_function(vm, f, args);
	}
	else{
		return call_interpreted_function(vm, f, args);
	}
}






floyd_parser::value_t resolve_variable_name_deep(const std::vector<shared_ptr<scope_instance_t>>& scopes, const std::string& s, size_t depth){
	QUARK_ASSERT(depth < scopes.size());
	QUARK_ASSERT(depth >= 0);

	const auto it = scopes[depth]->_values.find(s);
	if(it != scopes[depth]->_values.end()){
		return it->second;
	}
	else if(depth > 0){
		return resolve_variable_name_deep(scopes, s, depth - 1);
	}
	else{
		return {};
	}
}

/*
	Uses runtime callstack to find a variable by its name.
*/
floyd_parser::value_t resolve_variable_name(const vm_t& vm, const std::string& s){
	QUARK_ASSERT(vm.check_invariant());
	QUARK_ASSERT(s.size() > 0);

	return resolve_variable_name_deep(vm._scope_instances, s, vm._scope_instances.size() - 1);
}

		/*
			PROBLEM: How to resolve a complex address expression tree into something you can read a value from (or store a value to or call as a function etc.
			We don't have any value we can return from each expression in tree.
			Alternatives:
			A) Have dedicated expression types:
				struct_member_address_t { expression_t _parent_address, struct_def* _def, shared_ptr<struct_instance_t> _instance, string _member_name; }
				collection_lookup { vector_def* _def, shared_ptr<vector_instance_t> _instance, value_t _key };

			B)	Have value_t of type struct_member_spec_t { string member_name, value_t} so a value_t can point to a specific member variable.
			C)	parse address in special function that resolves the expression and keeps the actual address on the side. Address can be raw C++ pointer.
		*/

/*
	a = my_global_int;
	b = my_global_obj.next;
	c = my_global_obj.all[3].f(10).prev;
*/

expression_t load_deep(const vm_t& vm, const value_t& left_side, const expression_t& e){
	QUARK_ASSERT(vm.check_invariant());
	QUARK_ASSERT(e.check_invariant());
	QUARK_ASSERT(e._resolve_variable || e._resolve_struct_member || e._lookup_element);

	if(e._call){
		//	???
		QUARK_ASSERT(false);
	}
	else if(e._resolve_variable){
		QUARK_ASSERT(left_side.is_null());
		const auto variable_name = e._resolve_variable->_variable_name;
		const value_t value = resolve_variable_name(vm, variable_name);
		return make_constant(value);
	}
	else if(e._resolve_struct_member){
		const auto parent = load_deep(vm, left_side, *e._resolve_struct_member->_parent_address);
		QUARK_ASSERT(parent._constant && parent._constant->is_struct_instance());

		const auto member_name = e._resolve_struct_member->_member_name;
		const auto struct_instance = parent._constant->get_struct_instance();
		const value_t value = struct_instance->_member_values[member_name];
		return make_constant(value);
	}
	else if(e._lookup_element){
		//	???
		QUARK_ASSERT(false);
		return make_constant(value_t());
	}
	else{
		QUARK_ASSERT(false);
	}
}

expression_t load(const vm_t& vm, const expression_t& e){
	QUARK_ASSERT(vm.check_invariant());
	QUARK_ASSERT(e.check_invariant());
	QUARK_ASSERT(e._load);

	const auto e2 = *e._load;
	QUARK_ASSERT(e2._address->_call || e2._address->_resolve_variable || e2._address->_resolve_struct_member || e2._address-> _lookup_element);

	const auto e3 = load_deep(vm, value_t(), *e2._address);
	return e3._constant;
}


//### Test string + etc.


//??? tests
//??? Split into several functions.
expression_t evalute_expression(const vm_t& vm, const expression_t& e){
	QUARK_ASSERT(vm.check_invariant());
	QUARK_ASSERT(e.check_invariant());

	if(e._constant){
		return e;
	}
	else if(e._math2){
		const auto e2 = *e._math2;
		const auto left = evalute_expression(vm, *e2._left);
		const auto right = evalute_expression(vm, *e2._right);

		//	Both left and right are constant, replace the math_operation with a constant!
		if(left._constant && right._constant){
			const auto left_value = left._constant;
			const auto right_value = right._constant;
			if(left_value->get_type() == make_type_identifier("int") && right_value->get_type() == make_type_identifier("int")){
				if(e2._operation == math_operation2_expr_t::add){
					return make_constant(left_value->get_int() + right_value->get_int());
				}
				else if(e2._operation == math_operation2_expr_t::subtract){
					return make_constant(left_value->get_int() - right_value->get_int());
				}
				else if(e2._operation == math_operation2_expr_t::multiply){
					return make_constant(left_value->get_int() * right_value->get_int());
				}
				else if(e2._operation == math_operation2_expr_t::divide){
					if(right_value->get_int() == 0){
						throw std::runtime_error("EEE_DIVIDE_BY_ZERO");
					}
					return make_constant(left_value->get_int() / right_value->get_int());
				}
				else{
					QUARK_ASSERT(false);
				}
			}
			else if(left_value->get_type() == make_type_identifier("float") && right_value->get_type() == make_type_identifier("float")){
				if(e2._operation == math_operation2_expr_t::add){
					return make_constant(left_value->get_float() + right_value->get_float());
				}
				else if(e2._operation == math_operation2_expr_t::subtract){
					return make_constant(left_value->get_float() - right_value->get_float());
				}
				else if(e2._operation == math_operation2_expr_t::multiply){
					return make_constant(left_value->get_float() * right_value->get_float());
				}
				else if(e2._operation == math_operation2_expr_t::divide){
					if(right_value->get_float() == 0.0f){
						throw std::runtime_error("EEE_DIVIDE_BY_ZERO");
					}
					return make_constant(left_value->get_float() / right_value->get_float());
				}
				else{
					QUARK_ASSERT(false);
				}
			}
			else if(left_value->get_type() == make_type_identifier("string") && right_value->get_type() == make_type_identifier("string")){
				if(e2._operation == math_operation2_expr_t::add){
					return make_constant(left_value->get_string() + right_value->get_string());
				}
				else{
					throw std::runtime_error("Arithmetics failed.");
				}
			}
			else{
				throw std::runtime_error("Arithmetics failed.");
			}
		}

		//	Else use a math_operation to make the calculation later. We make a NEW math_operation since sub-nodes may have been evaluated.
		else{
			return make_math_operation2(e2._operation, left, right);
		}
	}
	else if(e._math1){
		const auto e2 = *e._math1;
		const auto input = evalute_expression(vm, *e2._input);

		//	Replace the with a constant!
		if(input._constant){
			const auto value = input._constant;
			if(value->get_type() == make_type_identifier("int")){
				if(e2._operation == math_operation1_expr_t::negate){
					return make_constant(-value->get_int());
				}
				else{
					QUARK_ASSERT(false);
				}
			}
			else if(value->get_type() == make_type_identifier("float")){
				if(e2._operation == math_operation1_expr_t::negate){
					return make_constant(-value->get_float());
				}
				else{
					QUARK_ASSERT(false);
				}
			}
			else if(value->get_type() == make_type_identifier("string")){
				throw std::runtime_error("Arithmetics failed.");
			}
			else{
				throw std::runtime_error("Arithmetics failed.");
			}
		}

		//	Else use a math_operation to make the calculation later. We make a NEW math_operation since sub-nodes may have been evaluated.
		else{
			return make_math_operation1(e2._operation, input);
		}
	}

	/*
		If inputs are constant, replace function call with a constant!
	*/
	else if(e._call){
		const auto& call_function_expression = *e._call;

		//	??? Function calls should also use resolve_address_expression() to find function.

		const auto type = resolve_type(*vm._scope_instances.back()->_def, call_function_expression._function_name);
		if(!type || !type->_function_def){
			throw std::runtime_error("Failed calling function - unresolved function.");
		}

		const auto& function_def = *type->_function_def;
		QUARK_ASSERT(function_def._args.size() == call_function_expression._inputs.size());

		//	Simplify each argument.
		vector<expression_t> simplified_args;
		for(const auto& i: call_function_expression._inputs){
			const auto arg_expr = evalute_expression(vm, *i);
			simplified_args.push_back(arg_expr);
		}

		//	All arguments to functions are constants? Else return new call_function, but with simplified arguments.
		for(const auto& i: simplified_args){
			if(!i._constant){
				return make_function_call(call_function_expression._function_name, call_function_expression._inputs);
			}
		}

		//	Woha: all arguments are constants - replace this expression with the final output of the function call instead!
		vector<value_t> constant_args;
		for(const auto& i: simplified_args){
			constant_args.push_back(*i._constant);
			if(!i._constant){
				return make_function_call(call_function_expression._function_name, call_function_expression._inputs);
			}
		}
		const value_t result = run_function(vm, function_def, constant_args);
		return make_constant(result);
	}
	else if(e._load){
		return load(vm, e);
	}
	else if(e._resolve_variable){
		QUARK_ASSERT(false);
		return e;
	}
	else if(e._resolve_struct_member){
		QUARK_ASSERT(false);
		return e;
	}
	else if(e._lookup_element){
		QUARK_ASSERT(false);
		return e;
	}
	else{
		QUARK_ASSERT(false);
	}
}


}	//	floyd_parser



namespace {

using namespace floyd_parser;


//??? Needs test functs?
	expression_t test_evaluate_simple(string expression_string){
		const ast_t ast;
		const auto e = parse_expression(ast, expression_string);
		const auto e2 = evalute_expression(ast, e);
		return e2;
	}
}


//??? Add tests for strings and floats.
QUARK_UNIT_TEST("", "evaluate()", "", "") {
	// Some simple expressions
	QUARK_TEST_VERIFY(test_evaluate_simple("1234") == make_constant(1234));
	QUARK_TEST_VERIFY(test_evaluate_simple("1+2") == make_constant(3));
	QUARK_TEST_VERIFY(test_evaluate_simple("1+2+3") == make_constant(6));
	QUARK_TEST_VERIFY(test_evaluate_simple("3*4") == make_constant(12));
	QUARK_TEST_VERIFY(test_evaluate_simple("3*4*5") == make_constant(60));
	QUARK_TEST_VERIFY(test_evaluate_simple("1+2*3") == make_constant(7));

	// Parenthesis
	QUARK_TEST_VERIFY(test_evaluate_simple("5*(4+4+1)") == make_constant(45));
	QUARK_TEST_VERIFY(test_evaluate_simple("5*(2*(1+3)+1)") == make_constant(45));
	QUARK_TEST_VERIFY(test_evaluate_simple("5*((1+3)*2+1)") == make_constant(45));

	// Spaces
	QUARK_TEST_VERIFY(test_evaluate_simple(" 5 * ((1 + 3) * 2 + 1) ") == make_constant(45));
	QUARK_TEST_VERIFY(test_evaluate_simple(" 5 - 2 * ( 3 ) ") == make_constant(-1));
	QUARK_TEST_VERIFY(test_evaluate_simple(" 5 - 2 * ( ( 4 )  - 1 ) ") == make_constant(-1));

	// Sign before parenthesis
	QUARK_TEST_VERIFY(test_evaluate_simple("-(2+1)*4") == make_constant(-12));
	QUARK_TEST_VERIFY(test_evaluate_simple("-4*(2+1)") == make_constant(-12));
	
	// Fractional numbers
	QUARK_TEST_VERIFY(compare_float_approx(test_evaluate_simple("5.5/5.0")._constant->get_float(), 1.1f));
//	QUARK_TEST_VERIFY(test_evaluate_simple("1/5e10") == 2e-11);
	QUARK_TEST_VERIFY(compare_float_approx(test_evaluate_simple("(4.0-3.0)/(4.0*4.0)")._constant->get_float(), 0.0625f));
	QUARK_TEST_VERIFY(test_evaluate_simple("1.0/2.0/2.0") == make_constant(0.25f));
	QUARK_TEST_VERIFY(test_evaluate_simple("0.25 * .5 * 0.5") == make_constant(0.0625f));
	QUARK_TEST_VERIFY(test_evaluate_simple(".25 / 2.0 * .5") == make_constant(0.0625f));
	
	// Repeated operators
	QUARK_TEST_VERIFY(test_evaluate_simple("1+-2") == make_constant(-1));
	QUARK_TEST_VERIFY(test_evaluate_simple("--2") == make_constant(2));
	QUARK_TEST_VERIFY(test_evaluate_simple("2---2") == make_constant(0));
	QUARK_TEST_VERIFY(test_evaluate_simple("2-+-2") == make_constant(4));


	// === Errors ===

	if(true){
		//////////////////////////		Parenthesis error
		try{
			test_evaluate_simple("5*((1+3)*2+1");
			QUARK_TEST_VERIFY(false);
		}
		catch(const std::runtime_error& e){
			QUARK_TEST_VERIFY(string(e.what()) == "EEE_PARENTHESIS");
		}

		try{
			test_evaluate_simple("5*((1+3)*2)+1)");
			QUARK_TEST_VERIFY(false);
		}
		catch(const std::runtime_error& e){
			QUARK_TEST_VERIFY(string(e.what()) == "EEE_WRONG_CHAR");
		}


		//////////////////////////		Repeated operators (wrong)
		try{
			test_evaluate_simple("5*/2");
			QUARK_TEST_VERIFY(false);
		}
		catch(const std::runtime_error& e){
//			QUARK_TEST_VERIFY(string(e.what()) == "EEE_WRONG_CHAR");
		}

		//////////////////////////		Wrong position of an operator
		try{
			test_evaluate_simple("*2");
			QUARK_TEST_VERIFY(false);
		}
		catch(const std::runtime_error& e){
//			QUARK_TEST_VERIFY(string(e.what()) == "EEE_WRONG_CHAR");
		}

		try{
			test_evaluate_simple("2+");
			QUARK_TEST_VERIFY(false);
		}
		catch(const std::runtime_error& e){
			QUARK_TEST_VERIFY(string(e.what()) == "Expected number");
		}

		try{
			test_evaluate_simple("2*");
			QUARK_TEST_VERIFY(false);
		}
		catch(const std::runtime_error& e){
			QUARK_TEST_VERIFY(string(e.what()) == "Expected number");
		}

		//////////////////////////		Division by zero

		try{
			test_evaluate_simple("2/0");
			QUARK_TEST_VERIFY(false);
		}
		catch(const std::runtime_error& e){
			QUARK_TEST_VERIFY(string(e.what()) == "EEE_DIVIDE_BY_ZERO");
		}

		try{
			test_evaluate_simple("3+1/(5-5)+4");
			QUARK_TEST_VERIFY(false);
		}
		catch(const std::runtime_error& e){
			QUARK_TEST_VERIFY(string(e.what()) == "EEE_DIVIDE_BY_ZERO");
		}

		try{
			test_evaluate_simple("2/");
			QUARK_TEST_VERIFY(false);
		}
		catch(const std::runtime_error& e){
			QUARK_TEST_VERIFY(string(e.what()) == "Expected number");
		}

		//////////////////////////		Invalid characters
		try{
			test_evaluate_simple("~5");
			QUARK_TEST_VERIFY(false);
		}
		catch(const std::runtime_error& e){
//			QUARK_TEST_VERIFY(string(e.what()) == "EEE_WRONG_CHAR");
		}
		try{
			test_evaluate_simple("5x");
			QUARK_TEST_VERIFY(false);
		}
		catch(const std::runtime_error& e){
			QUARK_TEST_VERIFY(string(e.what()) == "EEE_WRONG_CHAR");
		}


		//////////////////////////		Multiply errors
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

		// An emtpy string
		try{
			test_evaluate_simple("");
			QUARK_TEST_VERIFY(false);
		}
		catch(...){
//			QUARK_TEST_VERIFY(error == "EEE_WRONG_CHAR");
		}
	}

}


