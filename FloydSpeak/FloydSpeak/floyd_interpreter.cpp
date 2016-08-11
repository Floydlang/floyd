//
//  parser_evaluator.cpp
//  FloydSpeak
//
//  Created by Marcus Zetterquist on 26/07/16.
//  Copyright © 2016 Marcus Zetterquist. All rights reserved.
//

#include "floyd_interpreter.h"


#include "parser_expression.h"
#include "parser_statement.h"
#include "floyd_parser.h"
#include "parser_value.h"

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

	interpreter_t open_function_scope(const interpreter_t& vm, const function_def_t& f, const vector<value_t>& args){
		QUARK_ASSERT(vm.check_invariant());
		QUARK_ASSERT(f.check_invariant());
		for(const auto i: args){ QUARK_ASSERT(i.check_invariant()); };

		if(!check_args(f, args)){
			throw std::runtime_error("function arguments do not match function");
		}

		scope_instance_t new_scope;
		new_scope._def = f._function_scope;

		for(int i = 0 ; i < args.size() ; i++){
			const auto& arg_name = f._args[i]._identifier;
			const auto& arg_value = args[i];
			new_scope._values[arg_name] = arg_value;
		}

		interpreter_t result = vm;
		result._scope_instances.push_back(make_shared<scope_instance_t>(new_scope));
		return result;
	}

	value_t call_host_function(const interpreter_t& vm, const function_def_t& f, const vector<value_t>& args){
		QUARK_ASSERT(vm.check_invariant());
		QUARK_ASSERT(f.check_invariant());
		QUARK_ASSERT(f._function_scope->_executable._statements.empty());
		QUARK_ASSERT(f._function_scope->_executable._host_function);
		QUARK_ASSERT(f._function_scope->_executable._host_function_param);

		for(const auto i: args){ QUARK_ASSERT(i.check_invariant()); };

		if(!check_args(f, args)){
			throw std::runtime_error("function arguments do not match function");
		}

	//	auto local_scope = add_args(ast, f, args);
		const auto a = f._function_scope->_executable._host_function(f._function_scope->_executable._host_function_param, args);
		return a;
	}

}

value_t execute_statements(const interpreter_t& vm, const vector<shared_ptr<statement_t>>& statements){
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

namespace {

	//??? Make this operate on scope_def_t instead of function_def_t. == use scope_def:s as generic nodes.

	value_t call_interpreted_function(const interpreter_t& vm, const function_def_t& f, const vector<value_t>& args){
		QUARK_ASSERT(vm.check_invariant());
		QUARK_ASSERT(f.check_invariant());
		QUARK_ASSERT(!f._function_scope->_executable._statements.empty());
		QUARK_ASSERT(!f._function_scope->_executable._host_function);
		QUARK_ASSERT(!f._function_scope->_executable._host_function_param);
		for(const auto i: args){ QUARK_ASSERT(i.check_invariant()); };

		if(!check_args(f, args)){
			throw std::runtime_error("function arguments do not match function");
		}

		auto vm2 = open_function_scope(vm, f, args);
		const auto& statements = f._function_scope->_executable._statements;
		const auto value = execute_statements(vm2, statements);
		if(value.is_null()){
		throw std::runtime_error("function missing return statement");
		}
		else{
			return value;
		}
	}

}

value_t call_function(const interpreter_t& vm, const function_def_t& f, const vector<value_t>& args){
	QUARK_ASSERT(vm.check_invariant());
	QUARK_ASSERT(f.check_invariant());
	for(const auto i: args){ QUARK_ASSERT(i.check_invariant()); };

	if(!check_args(f, args)){
		throw std::runtime_error("function arguments do not match function");
	}

	if(f._function_scope->_executable._host_function){
		return call_host_function(vm, f, args);
	}
	else{
		return call_interpreted_function(vm, f, args);
	}
}

namespace {
	shared_ptr<const floyd_parser::function_def_t> find_global_function(const interpreter_t& vm, const string& name){
		return vm._ast._global_scope->_types_collector.resolve_function_type(name);
	}
}

QUARK_UNIT_TESTQ("call_function()", "minimal program"){
	auto ast = program_to_ast(
		"int main(string args){\n"
		"	return 3 + 4;\n"
		"}\n"
	);
	auto vm = interpreter_t(ast);
	const auto f = find_global_function(vm, "main");
	const auto result = call_function(vm, *f, vector<floyd_parser::value_t>{ floyd_parser::value_t("program_name 1 2 3") });
	QUARK_TEST_VERIFY(result == floyd_parser::value_t(7));
}


QUARK_UNIT_TESTQ("call_function()", "minimal program 2"){
	auto ast = floyd_parser::program_to_ast(
		"int main(string args){\n"
		"	return \"123\" + \"456\";\n"
		"}\n"
	);
	auto vm = interpreter_t(ast);
	const auto f = find_global_function(vm, "main");
	const auto result = call_function(vm, *f, vector<floyd_parser::value_t>{ floyd_parser::value_t("program_name 1 2 3") });
	QUARK_TEST_VERIFY(result == floyd_parser::value_t("123456"));
}

QUARK_UNIT_TESTQ("call_function()", "define additional function, call it several times"){
	auto ast = floyd_parser::program_to_ast(
		"int myfunc(){ return 5; }\n"
		"int main(string args){\n"
		"	return myfunc() + myfunc() * 2;\n"
		"}\n"
	);
	auto vm = interpreter_t(ast);
	const auto f = find_global_function(vm, "main");
	const auto result = call_function(vm, *f, vector<floyd_parser::value_t>{ floyd_parser::value_t("program_name 1 2 3") });
	QUARK_TEST_VERIFY(result == floyd_parser::value_t(15));
}



QUARK_UNIT_TESTQ("call_function()", "use function inputs"){
	auto ast = program_to_ast(
		"int main(string args){\n"
		"	return \"-\" + args + \"-\";\n"
		"}\n"
	);
	auto vm = interpreter_t(ast);
	const auto f = find_global_function(vm, "main");
	const auto result = call_function(vm, *f, vector<floyd_parser::value_t>{ floyd_parser::value_t("xyz") });
	QUARK_TEST_VERIFY(result == floyd_parser::value_t("-xyz-"));

	const auto result2 = call_function(vm, *f, vector<floyd_parser::value_t>{ floyd_parser::value_t("Hello, world!") });
	QUARK_TEST_VERIFY(result2 == floyd_parser::value_t("-Hello, world!-"));
}

//### Check return value type.

QUARK_UNIT_TESTQ("call_function()", "use local variables"){
	auto ast = program_to_ast(
		"string myfunc(string t){ return \"<\" + t + \">\"; }\n"
		"string main(string args){\n"
		"	 string a = \"--\"; string b = myfunc(args) ; return a + args + b + a;\n"
		"}\n"
	);
	auto vm = interpreter_t(ast);
	const auto f = find_global_function(vm, "main");
	const auto result = call_function(vm, *f, vector<floyd_parser::value_t>{ floyd_parser::value_t("xyz") });
	QUARK_TEST_VERIFY(result == floyd_parser::value_t("--xyz<xyz>--"));

	const auto result2 = call_function(vm, *f, vector<floyd_parser::value_t>{ floyd_parser::value_t("123") });
	QUARK_TEST_VERIFY(result2 == floyd_parser::value_t("--123<123>--"));
}

namespace {
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
	floyd_parser::value_t resolve_variable_name(const interpreter_t& vm, const std::string& s){
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

	expression_t load_deep(const interpreter_t& vm, const value_t& left_side, const expression_t& e){
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

	expression_t load(const interpreter_t& vm, const expression_t& e){
		QUARK_ASSERT(vm.check_invariant());
		QUARK_ASSERT(e.check_invariant());
		QUARK_ASSERT(e._load);

		const auto e2 = *e._load;
		QUARK_ASSERT(e2._address->_call || e2._address->_resolve_variable || e2._address->_resolve_struct_member || e2._address-> _lookup_element);

		const auto e3 = load_deep(vm, value_t(), *e2._address);
		return e3._constant;
	}

}

//### Test string + etc.


//??? tests
//??? Split into several functions.
expression_t evalute_expression(const interpreter_t& vm, const expression_t& e){
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
			if(left_value->get_type() == type_identifier_t::make_int() && right_value->get_type() == type_identifier_t::make_int()){
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
			else if(left_value->get_type() == type_identifier_t::make_float() && right_value->get_type() == type_identifier_t::make_float()){
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
			else if(left_value->get_type() == type_identifier_t::make_string() && right_value->get_type() == type_identifier_t::make_string()){
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
			if(value->get_type() == type_identifier_t::make_int()){
				if(e2._operation == math_operation1_expr_t::negate){
					return make_constant(-value->get_int());
				}
				else{
					QUARK_ASSERT(false);
				}
			}
			else if(value->get_type() == type_identifier_t::make_float()){
				if(e2._operation == math_operation1_expr_t::negate){
					return make_constant(-value->get_float());
				}
				else{
					QUARK_ASSERT(false);
				}
			}
			else if(value->get_type() == type_identifier_t::make_string()){
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

		const auto type = resolve_type(vm._scope_instances.back()->_def, call_function_expression._function_name);
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
		const value_t result = call_function(vm, function_def, constant_args);
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



//////////////////////////		interpreter_t



interpreter_t::interpreter_t(const floyd_parser::ast_t& ast) :
	_ast(ast)
{
	QUARK_ASSERT(ast.check_invariant());

	auto global_scope = scope_instance_t();
	global_scope._def = ast._global_scope;
	_scope_instances.push_back(make_shared<scope_instance_t>(global_scope));

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
	auto ast = program_to_ast(source);
	auto vm = interpreter_t(ast);
	const auto f = find_global_function(vm, "main");
	const auto r = call_function(vm, *f, args);
	return { vm, r };
}

QUARK_UNIT_TESTQ("run_main()", "minimal program 2"){
	const auto result = run_main(
		"int main(string args){\n"
		"	return \"123\" + \"456\";\n"
		"}\n",
		vector<floyd_parser::value_t>{floyd_parser::value_t("program_name 1 2 3 4")}
	);
	QUARK_TEST_VERIFY(result.second == floyd_parser::value_t("123456"));
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
	QUARK_TEST_VERIFY(a.first._ast._global_scope->_types_collector.lookup_identifier_shallow("pixel"));
	QUARK_TEST_VERIFY(a.first._ast._global_scope->_types_collector.lookup_identifier_shallow("pixel_constructor"));
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
	QUARK_TEST_VERIFY(a.first._ast._global_scope->_types_collector.lookup_identifier_shallow("pixel"));
	QUARK_TEST_VERIFY(a.first._ast._global_scope->_types_collector.lookup_identifier_shallow("pixel_constructor"));
	QUARK_TEST_VERIFY(a.second == value_t("one"));
}

QUARK_UNIT_TESTQ("struct", "Nesting structs"){
	const auto a = run_main(
		"struct pixel { string s = \"one\"; }"
		"struct image { pixel background_color; int width; int height; }"
		"string main(){\n"
		"	image i = image_constructor();"
		"	return i.background_color.s;"
		"}\n",
		{}
	);
	QUARK_TEST_VERIFY(a.first._ast._global_scope->_types_collector.lookup_identifier_shallow("pixel"));
	QUARK_TEST_VERIFY(a.first._ast._global_scope->_types_collector.lookup_identifier_shallow("pixel_constructor"));
	QUARK_TEST_VERIFY(a.second == value_t("one"));
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

/*
	Scenario ???add
*/

}	//	floyd_interpreter



namespace {

using namespace floyd_interpreter;


//??? Needs test functs?
	expression_t test_evaluate_simple(string expression_string){
		const ast_t ast;
		const auto e = parse_expression(expression_string);
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

}	//	floyd_interpreter


