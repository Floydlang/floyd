//
//  main.cpp
//  FloydSpeak
//
//  Created by Marcus Zetterquist on 27/03/16.
//  Copyright © 2016 Marcus Zetterquist. All rights reserved.
//

/*
	floydrt -- floyd runtime.
	floydgen -- generated code from user program.
	floydc -- floyd compiler
*/

#include "floyd_parser.h"

/*
#define QUARK_ASSERT_ON true
#define QUARK_TRACE_ON true
#define QUARK_UNIT_TESTS_ON true
*/

#include "text_parser.h"
#include "steady_vector.h"
#include "parser_expression.hpp"
#include "parser_statement.hpp"

#include <string>
#include <memory>
#include <map>
#include <iostream>
#include <cmath>

namespace floyd_parser {


using std::vector;
using std::string;
using std::pair;
using std::shared_ptr;
using std::unique_ptr;
using std::make_shared;

/*
	AST ABSTRACT SYNTAX TREE

https://en.wikipedia.org/wiki/Abstract_syntax_tree

https://en.wikipedia.org/wiki/Parsing_expression_grammar
https://en.wikipedia.org/wiki/Parsing
*/



struct test_value_class_a {
	int _a = 10;
	int _b = 10;

	bool operator==(const test_value_class_a& other){
		return _a == other._a && _b == other._b;
	}
};

QUARK_UNIT_TESTQ("test_value_class_a", "what is needed for basic operations"){
	test_value_class_a a;
	test_value_class_a b = a;

	QUARK_TEST_VERIFY(b._a == 10);
	QUARK_TEST_VERIFY(a == b);
}




void IncreaseIndent(){
	auto r = quark::get_runtime();
	r->runtime_i__add_log_indent(1);
}

void DecreateIndent(){
	auto r = quark::get_runtime();
	r->runtime_i__add_log_indent(-1);
}










//////////////////////////////////////////////////		VISITOR




struct visitor_i {
	virtual ~visitor_i(){};

	virtual void visitor_interface__on_math_operation2(const math_operation2_expr_t& e) = 0;
	virtual void visitor_interface__on_function_def_expr(const function_def_expr_t& e) = 0;

	virtual void visitor_interface__on_bind_statement_statement(const bind_statement_t& s) = 0;
	virtual void visitor_interface__on_return_statement(const return_statement_t& s) = 0;
};

void visit_statement(const statement_t& s, visitor_i& visitor){
	if(s._bind_statement){
		visitor.visitor_interface__on_bind_statement_statement(*s._bind_statement);
	}
	else if(s._return_statement){
		visitor.visitor_interface__on_return_statement(*s._return_statement);
	}
	else{
		QUARK_ASSERT(false);
	}
}


void visit_program(const ast_t& program, visitor_i& visitor){
	for(const auto i: program._top_level_statements){
		visit_statement(i, visitor);
	}
}



//////////////////////////////////////////////////		TRACE







void trace(const statement_t& s){
	if(s._bind_statement){
		const auto s2 = s._bind_statement;
		string t = "bind_statement_t: \"" + s2->_identifier + "\"";
		QUARK_SCOPED_TRACE(t);
		trace(*s2->_expression);
	}
	else if(s._return_statement){
		const auto s2 = s._return_statement;
		QUARK_SCOPED_TRACE("return_statement_t");
		trace(*s2->_expression);
	}
	else{
		QUARK_ASSERT(false);
	}
}

void trace(const type_identifier_t& v){
	QUARK_TRACE("type_identifier_t <" + v.to_string() + ">");
}



void trace(const ast_t& program){
	QUARK_SCOPED_TRACE("program");

	for(const auto i: program._top_level_statements){
		trace(i);
	}
}




//////////////////////////////////////////////////		Syntax-specific reading


/*
	()
	(int a)
	(int x, int y)
*/
vector<arg_t> parse_functiondef_arguments(const string& s2){
	const auto s(s2.substr(1, s2.length() - 2));
	vector<arg_t> args;
	auto str = s;
	while(!str.empty()){
		const auto arg_type = get_type(str);
		const auto arg_name = get_identifier(arg_type.second);
		const auto optional_comma = read_optional_char(arg_name.second, ',');

		const auto a = arg_t{ make_type_identifier(arg_type.first), arg_name.first };
		args.push_back(a);
		str = skip_whitespace(optional_comma.second);
	}

	trace_vec("parsed arguments:", args);
	return args;
}

QUARK_UNIT_TEST("", "", "", ""){
	QUARK_TEST_VERIFY((parse_functiondef_arguments("()") == vector<arg_t>{}));
}

QUARK_UNIT_TEST("", "", "", ""){
	const auto r = parse_functiondef_arguments("(int x, string y, float z)");
	QUARK_TEST_VERIFY((r == vector<arg_t>{
		{ make_type_identifier("int"), "x" },
		{ make_type_identifier("string"), "y" },
		{ make_type_identifier("float"), "z" }
	}
	));
}







//////////////////////////////////////////////////		EVALUATE EXPRESSIONS



bool check_args(const function_def_expr_t& f, const vector<value_t>& args){
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
identifiers_t add_args(const identifiers_t& identifiers, const function_def_expr_t& f, const vector<value_t>& args){
	QUARK_ASSERT(identifiers.check_invariant());
	QUARK_ASSERT(f.check_invariant());
	for(const auto i: args){ QUARK_ASSERT(i.check_invariant()); };

	if(!check_args(f, args)){
		throw std::runtime_error("function arguments do not match function");
	}

	auto local_scope = identifiers;
	for(int i = 0 ; i < args.size() ; i++){
		const auto& arg_name = f._args[i]._identifier;
		const auto& arg_value = args[i];
		local_scope._constant_values[arg_name] = make_shared<const value_t>(arg_value);
	}
	return local_scope;
}



value_t run_function(const identifiers_t& identifiers, const function_def_expr_t& f, const vector<value_t>& args){
	QUARK_ASSERT(identifiers.check_invariant());
	QUARK_ASSERT(f.check_invariant());
	for(const auto i: args){ QUARK_ASSERT(i.check_invariant()); };

	if(!check_args(f, args)){
		throw std::runtime_error("function arguments do not match function");
	}
	auto local_scope = add_args(identifiers, f, args);

	//	??? Should respect {} for local variable scopes!
	const auto body = f._body;
	int statement_index = 0;
	while(statement_index < body._statements.size()){
		const auto statement = body._statements[statement_index];
		if(statement._bind_statement){
			const auto s = statement._bind_statement;
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
		else if(statement._return_statement){
			const auto expr = statement._return_statement->_expression;
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


bool compare_float_approx(float value, float expected){
	float diff = static_cast<float>(fabs(value - expected));
	return diff < 0.00001;
}

//### Test string + etc.


expression_t evaluate3(const identifiers_t& identifiers, const expression_t& e){
	QUARK_ASSERT(identifiers.check_invariant());
	QUARK_ASSERT(e.check_invariant());

	if(e._constant){
		return e;
	}
	else if(e._math_operation2_expr){
		const auto e2 = *e._math_operation2_expr;
		const auto left = evaluate3(identifiers, *e2._left);
		const auto right = evaluate3(identifiers, *e2._right);

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
		const auto input = evaluate3(identifiers, *e2._input);

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

		const auto it = identifiers._functions.find(call_function_expression._function_name);
		QUARK_ASSERT(it != identifiers._functions.end());

		const auto& make_function_expression = *it->second;

		QUARK_ASSERT(make_function_expression._args.size() == call_function_expression._inputs.size());

		//	Simplify each argument.
		vector<expression_t> simplified_args;
		for(const auto& i: call_function_expression._inputs){
			const auto arg_expr = evaluate3(identifiers, *i);
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
		const value_t result = run_function(identifiers, make_function_expression, constant_args);
		return make_constant(result);
	}
	else if(e._variable_read_expr){
		const string& identifier = e._variable_read_expr->_variable_name;
		const auto it = identifiers._constant_values.find(identifier);
		QUARK_ASSERT(it != identifiers._constant_values.end());

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


namespace {
	expression_t test_evaluate_simple(string expression_string){
		identifiers_t identifiers;
		const auto e = parse_expression(identifiers, expression_string);
		const auto e2 = evaluate3(identifiers, e);
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
			QUARK_TEST_VERIFY(string(e.what()) == "EEE_WRONG_CHAR");
		}

		//////////////////////////		Wrong position of an operator
		try{
			test_evaluate_simple("*2");
			QUARK_TEST_VERIFY(false);
		}
		catch(const std::runtime_error& e){
			QUARK_TEST_VERIFY(string(e.what()) == "EEE_WRONG_CHAR");
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
			QUARK_TEST_VERIFY(string(e.what()) == "EEE_WRONG_CHAR");
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




//////////////////////////////////////////////////		PARSE FUNCTION DEFINTION EXPRESSION

pair<statement_t, string> parse_assignment_statement(const identifiers_t& identifiers, const string& s);



bool is_known_data_type(const std::string& s){
	return true;
//???token_pos.first) != type_identifier_t("")){
}


/*
	Pre-evaluates when possible! NO DO NOT! THAT MAKES PARSER LOSSY!


	Must not have whitespace before / after {}.
	{}

	{
		return 3;
	}

	{
		return 3 + 4;
	}
	{
		return f(3, 4) + 2;
	}


	//	Example: binding constants to constants, result of function calls and math operations.
	{
		int a = 10;
		int b = f(a);
		int c = a + b;
		return c;
	}

	//	Local scope.
	{
		{
			int a = 10;
		}
	}
	{
		struct point2d {
			int _x;
			int _y;
		}
	}

	{
		int my_func(string a, string b){
			int c = a + b;
			return c;
		}
	}

	FUTURE
	- Include comments
	- Split-out parse_statement().
	- Add struct {}
	- Add variables
	- Add local functions

*/
function_body_t parse_function_body(const identifiers_t& ident, const string& s){
	QUARK_SCOPED_TRACE("parse_function_body()");
	QUARK_ASSERT(s.size() >= 2);
	QUARK_ASSERT(s[0] == '{' && s[s.size() - 1] == '}');

	const string body_str = skip_whitespace(s.substr(1, s.size() - 2));

	vector<statement_t> statements;
	identifiers_t local_scope = ident;

	string pos = body_str;
	while(!pos.empty()){
		const auto token_pos = read_until(pos, whitespace_chars);
		if(token_pos.first == "return"){
			const auto expression_pos = read_until(skip_whitespace(token_pos.second), ";");
			const auto expression1 = parse_expression(local_scope, expression_pos.first);
			const auto expression2 = evaluate3(local_scope, expression1);
			const auto statement = statement_t(return_statement_t{ make_shared<expression_t>(expression2) });
			statements.push_back(statement);

			//	Skip trailing ";".
			pos = skip_whitespace(expression_pos.second.substr(1));
		}

		/*
			"int a = 10;"
			"string hello = f(a) + \"_suffix\";";
		*/
		else if(is_known_data_type(token_pos.first)){
			pair<statement_t, string> assignment_statement = parse_assignment_statement(local_scope, pos);
			const string& identifier = assignment_statement.first._bind_statement->_identifier;

			const auto it = local_scope._constant_values.find(identifier);
			if(it != local_scope._constant_values.end()){
				throw std::runtime_error("Variable name already in use!");
			}

			shared_ptr<const value_t> blank;
			local_scope._constant_values[identifier] = blank;

			statements.push_back(assignment_statement.first);

			//	Skips trailing ";".
			pos = skip_whitespace(assignment_statement.second);
		}
		else{
			throw std::runtime_error("syntax error");
		}
	}
	const auto result = function_body_t{ statements };
	trace(result);
	return result;
}

QUARK_UNIT_TEST("", "", "", ""){
	QUARK_TEST_VERIFY((parse_function_body({}, "{}")._statements.empty()));
}

QUARK_UNIT_TEST("", "", "", ""){
	QUARK_TEST_VERIFY(parse_function_body({}, "{return 3;}")._statements.size() == 1);
}

QUARK_UNIT_TEST("", "", "", ""){
	QUARK_TEST_VERIFY(parse_function_body({}, "{\n\treturn 3;\n}")._statements.size() == 1);
}

QUARK_UNIT_TEST("", "", "", ""){
	const auto identifiers = make_test_functions();
	const auto a = parse_function_body(identifiers,
		"{	float test = log(10.11);\n"
		"	return 3;\n}"
	);
	QUARK_TEST_VERIFY(a._statements.size() == 2);
	QUARK_TEST_VERIFY(a._statements[0]._bind_statement->_identifier == "test");
	QUARK_TEST_VERIFY(a._statements[0]._bind_statement->_expression->_call_function_expr->_function_name == "log");
	QUARK_TEST_VERIFY(a._statements[0]._bind_statement->_expression->_call_function_expr->_inputs.size() == 1);
	QUARK_TEST_VERIFY(*a._statements[0]._bind_statement->_expression->_call_function_expr->_inputs[0]->_constant == value_t(10.11f));

	QUARK_TEST_VERIFY(*a._statements[1]._return_statement->_expression->_constant == value_t(3));
}

QUARK_UNIT_TEST("", "", "", ""){
	const auto identifiers = make_test_functions();
	const auto a = parse_function_body(identifiers,
		"{ return return5() + return5() * 2;\n}"
	);
	QUARK_TEST_VERIFY(a._statements.size() == 1);
//	QUARK_TEST_VERIFY(a._statements[0]._return_statement->_expression._math_operation2);

	const auto b = evaluate3(identifiers, *a._statements[0]._return_statement->_expression);
	QUARK_TEST_VERIFY(b._constant && *b._constant == value_t(15));

//	QUARK_TEST_VERIFY(a._statements[0]._bind_statement->_identifier == "test");
//	QUARK_TEST_VERIFY(a._statements[0]._bind_statement->_expression._call_function->_function_name == "log");
//	QUARK_TEST_VERIFY(a._statements[0]._bind_statement->_expression._call_function->_inputs.size() == 1);
//	QUARK_TEST_VERIFY(*a._statements[0]._bind_statement->_expression._call_function->_inputs[0]->_constant == value_t(10.11f));
}





identifiers_t add_arg_identifiers(const identifiers_t& identifiers, const vector<arg_t> arg_types){
	QUARK_ASSERT(identifiers.check_invariant());
	for(const auto i: arg_types){ QUARK_ASSERT(i.check_invariant()); };

	auto local_scope = identifiers;
	for(const auto arg: arg_types){
		const auto& arg_name = arg._identifier;
		shared_ptr<value_t> blank_arg_value;
		local_scope._constant_values[arg_name] = blank_arg_value;
	}
	return local_scope;
}

/*
	Named function:

	int myfunc(string a, int b){
		...
		return b + 1;
	}


	LATER:
	Lambda:

	int myfunc(string a){
		() => {
		}
	}
*/
pair<pair<string, function_def_expr_t>, string> parse_function_definition(const identifiers_t& identifiers, const string& pos){
	QUARK_ASSERT(identifiers.check_invariant());

	const auto type_pos = read_required_type_identifier(pos);
	const auto identifier_pos = read_required_identifier(type_pos.second);

	//	Skip whitespace.
	const auto rest = skip_whitespace(identifier_pos.second);

	if(!peek_compare_char(rest, '(')){
		throw std::runtime_error("expected function argument list enclosed by (),");
	}

	const auto arg_list_pos = get_balanced(rest);
	const auto args = parse_functiondef_arguments(arg_list_pos.first);
	const auto body_rest_pos = skip_whitespace(arg_list_pos.second);

	if(!peek_compare_char(body_rest_pos, '{')){
		throw std::runtime_error("expected function body enclosed by {}.");
	}
	const auto body_pos = get_balanced(body_rest_pos);
	const auto local_scope = add_arg_identifiers(identifiers, args);

	const auto body = parse_function_body(local_scope, body_pos.first);
	const auto a = function_def_expr_t{ type_pos.first, args, body };
//	trace_node(a);

	return { { identifier_pos.first, a }, body_pos.second };
}

QUARK_UNIT_TEST("", "parse_function_definition()", "", ""){
	try{
		const auto result = parse_function_definition({}, "int f()");
		QUARK_TEST_VERIFY(false);
	}
	catch(...){
	}
}

QUARK_UNIT_TEST("", "parse_function_definition()", "", ""){
	const auto result = parse_function_definition({}, "int f(){}");
	QUARK_TEST_VERIFY(result.first.first == "f");
	QUARK_TEST_VERIFY(result.first.second._return_type == type_identifier_t::make_type("int"));
	QUARK_TEST_VERIFY(result.first.second._args.empty());
	QUARK_TEST_VERIFY(result.first.second._body._statements.empty());
	QUARK_TEST_VERIFY(result.second == "");
}

QUARK_UNIT_TEST("", "parse_function_definition()", "Test many arguments of different types", ""){
	const auto result = parse_function_definition({}, "int printf(string a, float barry, int c){}");
	QUARK_TEST_VERIFY(result.first.first == "printf");
	QUARK_TEST_VERIFY(result.first.second._return_type == type_identifier_t::make_type("int"));
	QUARK_TEST_VERIFY((result.first.second._args == vector<arg_t>{
		{ make_type_identifier("string"), "a" },
		{ make_type_identifier("float"), "barry" },
		{ make_type_identifier("int"), "c" },
	}));
	QUARK_TEST_VERIFY(result.first.second._body._statements.empty());
	QUARK_TEST_VERIFY(result.second == "");
}

/*
QUARK_UNIT_TEST("", "parse_function_definition()", "Test exteme whitespaces", ""){
	const auto result = parse_function_definition("    int    printf   (   string    a   ,   float   barry  ,   int   c  )  {  }  ");
	QUARK_TEST_VERIFY(result.first.first == "printf");
	QUARK_TEST_VERIFY(result.first.second._return_type == type_identifier_t::make_type("int"));
	QUARK_TEST_VERIFY((result.first.second._args == vector<arg_t>{
		{ make_type_identifier("string"), "a" },
		{ make_type_identifier("float"), "barry" },
		{ make_type_identifier("int"), "c" },
	}));
	QUARK_TEST_VERIFY(result.first.second._body._statements.empty());
	QUARK_TEST_VERIFY(result.second == "");
}
*/





//////////////////////////////////////////////////		PARSE STATEMENTS



/*
	"int a = 10;"
	"float b = 0.3;"
	"int c = a + b;"
	"int b = f(a);"
	"string hello = f(a) + \"_suffix\";";

	...can contain trailing whitespace.
*/
pair<statement_t, string> parse_assignment_statement(const identifiers_t& identifiers, const string& s){
	QUARK_SCOPED_TRACE("parse_assignment_statement()");
	QUARK_ASSERT(identifiers.check_invariant());

	const auto token_pos = read_until(s, whitespace_chars);
	const auto type = make_type_identifier(token_pos.first);
	QUARK_ASSERT(is_known_data_type(read_until(s, whitespace_chars).first));

	const auto variable_pos = read_until(skip_whitespace(token_pos.second), whitespace_chars + "=");
	const auto equal_rest = read_required_char(skip_whitespace(variable_pos.second), '=');
	const auto expression_pos = read_until(skip_whitespace(equal_rest), ";");

	const auto expression = parse_expression(identifiers, expression_pos.first);

	const auto statement = make__bind_statement(variable_pos.first, expression);
	trace(statement);

	//	Skip trailing ";".
	return { statement, expression_pos.second.substr(1) };
}

QUARK_UNIT_TESTQ("parse_assignment_statement", "int"){
	const auto a = parse_assignment_statement(identifiers_t(), "int a = 10; \n");
	QUARK_TEST_VERIFY(a.first._bind_statement->_identifier == "a");
	QUARK_TEST_VERIFY(*a.first._bind_statement->_expression->_constant == value_t(10));
	QUARK_TEST_VERIFY(a.second == " \n");
}

QUARK_UNIT_TESTQ("parse_assignment_statement", "float"){
	const auto a = parse_assignment_statement(identifiers_t(), "float b = 0.3; \n");
	QUARK_TEST_VERIFY(a.first._bind_statement->_identifier == "b");
	QUARK_TEST_VERIFY(*a.first._bind_statement->_expression->_constant == value_t(0.3f));
	QUARK_TEST_VERIFY(a.second == " \n");
}

QUARK_UNIT_TESTQ("parse_assignment_statement", "function call"){
	const auto identifiers = make_test_functions();
	const auto a = parse_assignment_statement(identifiers, "float test = log(\"hello\");\n");
	QUARK_TEST_VERIFY(a.first._bind_statement->_identifier == "test");
	QUARK_TEST_VERIFY(a.first._bind_statement->_expression->_call_function_expr->_function_name == "log");
	QUARK_TEST_VERIFY(a.first._bind_statement->_expression->_call_function_expr->_inputs.size() == 1);
	QUARK_TEST_VERIFY(*a.first._bind_statement->_expression->_call_function_expr->_inputs[0]->_constant ==value_t("hello"));
	QUARK_TEST_VERIFY(a.second == "\n");
}




//////////////////////////////////////////////////		read_toplevel_statement()


/*
	Function definitions
		int my_func(){ return 100; };
		string test3(int a, float b){ return "sdf" };


	FUTURE
	- Define data structures (also in local scopes).
	- Add support for global constants.
	- Assign global constants
		int my_global = 3;
*/

pair<statement_t, string> read_toplevel_statement(const identifiers_t& identifiers, const string& pos){
	QUARK_ASSERT(identifiers.check_invariant());

	const auto type_pos = read_required_type_identifier(pos);
	const auto identifier_pos = read_required_identifier(type_pos.second);

	pair<pair<string, function_def_expr_t>, string> function = parse_function_definition(identifiers, pos);
	const auto bind = bind_statement_t{ function.first.first, make_shared<expression_t>(function.first.second) };
	return { bind, function.second };
}

QUARK_UNIT_TEST("", "read_toplevel_statement()", "", ""){
	try{
		const auto result = read_toplevel_statement({}, "int f()");
		QUARK_TEST_VERIFY(false);
	}
	catch(...){
	}
}

QUARK_UNIT_TEST("", "read_toplevel_statement()", "", ""){
	const auto result = read_toplevel_statement({}, "int f(){}");
	QUARK_TEST_VERIFY(result.first._bind_statement);
	QUARK_TEST_VERIFY(result.first._bind_statement->_identifier == "f");
	QUARK_TEST_VERIFY(result.first._bind_statement->_expression->_function_def_expr);

	const auto make_function_expression = result.first._bind_statement->_expression->_function_def_expr;
	QUARK_TEST_VERIFY(make_function_expression->_return_type == type_identifier_t::make_type("int"));
	QUARK_TEST_VERIFY(make_function_expression->_args.empty());
	QUARK_TEST_VERIFY(make_function_expression->_body._statements.empty());

	QUARK_TEST_VERIFY(result.second == "");
}




//////////////////////////////////////////////////		program_to_ast()



ast_t program_to_ast(const identifiers_t& builtins, const string& program){
	vector<statement_t> top_level_statements;
	identifiers_t identifiers = builtins;
	auto pos = program;
	pos = skip_whitespace(pos);
	while(!pos.empty()){
		const auto statement_pos = read_toplevel_statement(identifiers, pos);
		top_level_statements.push_back(statement_pos.first);

		if(statement_pos.first._bind_statement){
			string identifier = statement_pos.first._bind_statement->_identifier;
			const auto e = statement_pos.first._bind_statement->_expression;

			if(e->_function_def_expr){
				const auto foundIt = identifiers._functions.find(identifier);
				if(foundIt != identifiers._functions.end()){
					throw std::runtime_error("Function \"" + identifier + "\" already defined.");
				}

				//	shared_ptr
				identifiers._functions[identifier] = e->_function_def_expr;
			}
		}
		else{
			QUARK_ASSERT(false);
		}
		pos = skip_whitespace(statement_pos.second);
	}

	const ast_t result{ {}, identifiers, top_level_statements };
	trace(result);

	return result;
}

QUARK_UNIT_TEST("", "program_to_ast()", "kProgram1", ""){
	const string kProgram1 =
	"int main(string args){\n"
	"	return 3;\n"
	"}\n";

	const auto result = program_to_ast(identifiers_t(), kProgram1);
	QUARK_TEST_VERIFY(result._top_level_statements.size() == 1);
	QUARK_TEST_VERIFY(result._top_level_statements[0]._bind_statement);
	QUARK_TEST_VERIFY(result._top_level_statements[0]._bind_statement->_identifier == "main");
	QUARK_TEST_VERIFY(result._top_level_statements[0]._bind_statement->_expression->_function_def_expr);

	const auto make_function_expression = result._top_level_statements[0]._bind_statement->_expression->_function_def_expr;
	QUARK_TEST_VERIFY(make_function_expression->_return_type == type_identifier_t::make_type("int"));
	QUARK_TEST_VERIFY((make_function_expression->_args == vector<arg_t>{ arg_t{ type_identifier_t::make_type("string"), "args" }}));
}


QUARK_UNIT_TEST("", "program_to_ast()", "three arguments", ""){
	const string kProgram =
	"int f(int x, int y, string z){\n"
	"	return 3;\n"
	"}\n"
	;

	const auto result = program_to_ast(identifiers_t(), kProgram);
	QUARK_TEST_VERIFY(result._top_level_statements.size() == 1);
	QUARK_TEST_VERIFY(result._top_level_statements[0]._bind_statement);
	QUARK_TEST_VERIFY(result._top_level_statements[0]._bind_statement->_identifier == "f");
	QUARK_TEST_VERIFY(result._top_level_statements[0]._bind_statement->_expression->_function_def_expr);

	const auto make_function_expression = result._top_level_statements[0]._bind_statement->_expression->_function_def_expr;
	QUARK_TEST_VERIFY(make_function_expression->_return_type == type_identifier_t::make_type("int"));
	QUARK_TEST_VERIFY((make_function_expression->_args == vector<arg_t>{
		arg_t{ type_identifier_t::make_type("int"), "x" },
		arg_t{ type_identifier_t::make_type("int"), "y" },
		arg_t{ type_identifier_t::make_type("string"), "z" }
	}));
}


QUARK_UNIT_TEST("", "program_to_ast()", "two functions", ""){
	const string kProgram =
	"string hello(int x, int y, string z){\n"
	"	return \"test abc\";\n"
	"}\n"
	"int main(string args){\n"
	"	return 3;\n"
	"}\n"
	;
	QUARK_TRACE(kProgram);

	const auto result = program_to_ast(identifiers_t(), kProgram);
	QUARK_TEST_VERIFY(result._top_level_statements.size() == 2);

	QUARK_TEST_VERIFY(result._top_level_statements[0]._bind_statement);
	QUARK_TEST_VERIFY(result._top_level_statements[0]._bind_statement->_identifier == "hello");
	QUARK_TEST_VERIFY(result._top_level_statements[0]._bind_statement->_expression->_function_def_expr);

	const auto hello = result._top_level_statements[0]._bind_statement->_expression->_function_def_expr;
	QUARK_TEST_VERIFY(hello->_return_type == type_identifier_t::make_type("string"));
	QUARK_TEST_VERIFY((hello->_args == vector<arg_t>{
		arg_t{ type_identifier_t::make_type("int"), "x" },
		arg_t{ type_identifier_t::make_type("int"), "y" },
		arg_t{ type_identifier_t::make_type("string"), "z" }
	}));


	QUARK_TEST_VERIFY(result._top_level_statements[1]._bind_statement);
	QUARK_TEST_VERIFY(result._top_level_statements[1]._bind_statement->_identifier == "main");
	QUARK_TEST_VERIFY(result._top_level_statements[1]._bind_statement->_expression->_function_def_expr);

	const auto main = result._top_level_statements[1]._bind_statement->_expression->_function_def_expr;
	QUARK_TEST_VERIFY(main->_return_type == type_identifier_t::make_type("int"));
	QUARK_TEST_VERIFY((main->_args == vector<arg_t>{
		arg_t{ type_identifier_t::make_type("string"), "args" }
	}));

}


QUARK_UNIT_TESTQ("program_to_ast()", ""){
	const string kProgram2 =
	"float testx(float v){\n"
	"	return 13.4;\n"
	"}\n"
	"int main(string args){\n"
	"	float test = testx(1234);\n"
	"	return 3;\n"
	"}\n";
	auto r = program_to_ast(identifiers_t(), kProgram2);
	QUARK_TEST_VERIFY(r._top_level_statements.size() == 2);
	QUARK_TEST_VERIFY(r._top_level_statements[0]._bind_statement);
	QUARK_TEST_VERIFY(r._top_level_statements[0]._bind_statement->_identifier == "testx");
	QUARK_TEST_VERIFY(r._top_level_statements[0]._bind_statement->_expression->_function_def_expr->_return_type == make_type_identifier("float"));
	QUARK_TEST_VERIFY(r._top_level_statements[0]._bind_statement->_expression->_function_def_expr->_args.size() == 1);
	QUARK_TEST_VERIFY(r._top_level_statements[0]._bind_statement->_expression->_function_def_expr->_args[0]._type == make_type_identifier("float"));
	QUARK_TEST_VERIFY(r._top_level_statements[0]._bind_statement->_expression->_function_def_expr->_args[0]._identifier == "v");
	QUARK_TEST_VERIFY(r._top_level_statements[0]._bind_statement->_expression->_function_def_expr->_body._statements.size() == 1);

	QUARK_TEST_VERIFY(r._top_level_statements[1]._bind_statement);
	QUARK_TEST_VERIFY(r._top_level_statements[1]._bind_statement->_identifier == "main");
	QUARK_TEST_VERIFY(r._top_level_statements[1]._bind_statement->_expression->_function_def_expr->_return_type == make_type_identifier("int"));
	QUARK_TEST_VERIFY(r._top_level_statements[1]._bind_statement->_expression->_function_def_expr->_args.size() == 1);
	QUARK_TEST_VERIFY(r._top_level_statements[1]._bind_statement->_expression->_function_def_expr->_args[0]._type == make_type_identifier("string"));
	QUARK_TEST_VERIFY(r._top_level_statements[1]._bind_statement->_expression->_function_def_expr->_args[0]._identifier == "args");
	QUARK_TEST_VERIFY(r._top_level_statements[1]._bind_statement->_expression->_function_def_expr->_body._statements.size() == 2);
	//### Test body?
}


}	//	floyd_parser



#if false
QUARK_UNIT_TEST("", "run()", "", ""){
	auto ast = program_to_ast(
		"int main(string args){\n"
		"	return \"hello, world!\";\n"
		"}\n"
	);

	value_t result = run(ast);
	QUARK_TEST_VERIFY(result == value_t("hello, world!"));
}
#endif

