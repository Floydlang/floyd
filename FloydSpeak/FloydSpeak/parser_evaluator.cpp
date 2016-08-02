//
//  parser_evaluator.cpp
//  FloydSpeak
//
//  Created by Marcus Zetterquist on 26/07/16.
//  Copyright © 2016 Marcus Zetterquist. All rights reserved.
//

#include "parser_evaluator.hpp"


#include "parser_expression.hpp"
#include "parser_statement.hpp"
#include "floyd_parser.h"

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

	/*
		- Use callstack instead of duplicating all identifiers.
	*/
	ast_t add_args(const ast_t& ast, const function_def_t& f, const vector<value_t>& args){
		QUARK_ASSERT(ast.check_invariant());
		QUARK_ASSERT(f.check_invariant());
		for(const auto i: args){ QUARK_ASSERT(i.check_invariant()); };

		if(!check_args(f, args)){
			throw std::runtime_error("function arguments do not match function");
		}

		auto local_scope = ast;
		for(int i = 0 ; i < args.size() ; i++){
			const auto& arg_name = f._args[i]._identifier;
			const auto& arg_value = args[i];
			local_scope._constant_values[arg_name] = make_shared<const value_t>(arg_value);
		}
		return local_scope;
	}

}


value_t run_function(const ast_t& ast, const function_def_t& f, const vector<value_t>& args){
	QUARK_ASSERT(ast.check_invariant());
	QUARK_ASSERT(f.check_invariant());
	for(const auto i: args){ QUARK_ASSERT(i.check_invariant()); };

	if(!check_args(f, args)){
		throw std::runtime_error("function arguments do not match function");
	}

	auto local_scope = add_args(ast, f, args);

	//	??? Should respect {} for local variable scopes!
	const auto& statements = f._statements;
	int statement_index = 0;
	while(statement_index < statements.size()){
		const auto statement = statements[statement_index];
		if(statement->_bind_statement){
			const auto s = statement->_bind_statement;
			const auto name = s->_identifier;
			if(local_scope._constant_values.count(name) != 0){
				throw std::runtime_error("local constant already exists");
			}
			const auto result = evaluate3(local_scope, *s->_expression);
			if(!result._constant){
				throw std::runtime_error("unknown variables");
			}
			local_scope._constant_values[name] = make_shared<value_t>(*result._constant);
		}
		else if(statement->_return_statement){
			const auto expr = statement->_return_statement->_expression;
			const auto result = evaluate3(local_scope, *expr);

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
	throw std::runtime_error("function missing return statement");
}



//### Test string + etc.


//??? tests
//??? Split into several functions.
expression_t evaluate3(const ast_t& ast, const expression_t& e){
	QUARK_ASSERT(ast.check_invariant());
	QUARK_ASSERT(e.check_invariant());

	if(e._constant){
		return e;
	}
	else if(e._math_operation2_expr){
		const auto e2 = *e._math_operation2_expr;
		const auto left = evaluate3(ast, *e2._left);
		const auto right = evaluate3(ast, *e2._right);

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
					return make_constant(value_t(left_value->get_string() + right_value->get_string()));
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
			return make_math_operation2_expr(e2._operation, left, right);
		}
	}
	else if(e._math_operation1_expr){
		const auto e2 = *e._math_operation1_expr;
		const auto input = evaluate3(ast, *e2._input);

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
	else if(e._call_function_expr){
		const auto& call_function_expression = *e._call_function_expr;

		const auto& function_def = ast._types_collector.resolve_function_type(call_function_expression._function_name);

		QUARK_ASSERT(function_def->_args.size() == call_function_expression._inputs.size());

		//	Simplify each argument.
		vector<expression_t> simplified_args;
		for(const auto& i: call_function_expression._inputs){
			const auto arg_expr = evaluate3(ast, *i);
			simplified_args.push_back(arg_expr);
		}

		//	All arguments to functions are constants? Else return new call_function, but with simplified arguments.
		for(const auto& i: simplified_args){
			if(!i._constant){
				return { function_call_expr_t{ call_function_expression._function_name, call_function_expression._inputs } };
			}
		}

		//	WOha: all arguments were constants - replace this expression with result of function call instead!
		vector<value_t> constant_args;
		for(const auto& i: simplified_args){
			constant_args.push_back(*i._constant);
			if(!i._constant){
				return { function_call_expr_t{ call_function_expression._function_name, call_function_expression._inputs } };
			}
		}
		const value_t result = run_function(ast, *function_def, constant_args);
		return make_constant(result);
	}
	else if(e._variable_read_expr){
		const auto address = e._variable_read_expr->_address;

		std::string function_name;
		//??? Very limited addressing!
		if(address->_resolve_member_expr){
			function_name = address->_resolve_member_expr->_member_name;
		}

		const auto it = ast._constant_values.find(function_name);
		QUARK_ASSERT(it != ast._constant_values.end());

		const auto value_ref = it->second;
		if(value_ref){
			return make_constant(*value_ref);
		}
		else{
			return e;
		}
	}
	else if(e._nop_expr){
		return e;
	}
	else{
		QUARK_ASSERT(false);
	}
}


}	//	floyd_parser



namespace {

using namespace floyd_parser;


struct test_parser : public parser_i {
	public: test_parser(){
	}

	public: virtual bool parser_i_is_declared_function(const std::string& s) const{
		return s == "log" || s == "log2" || s == "f" || s == "return5";
	}
	public: virtual bool parser_i_is_declared_constant_value(const std::string& s) const{
		return false;
	}
};

//??? Needs test functs?
	expression_t test_evaluate_simple(string expression_string){
		const ast_t ast;
		const auto e = parse_expression(ast, expression_string);
		const auto e2 = evaluate3(ast, e);
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


