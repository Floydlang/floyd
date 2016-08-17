//
//  pass2.cpp
//  FloydSpeak
//
//  Created by Marcus Zetterquist on 09/08/16.
//  Copyright © 2016 Marcus Zetterquist. All rights reserved.
//

#include "pass2.h"

#include "parser_statement.h"
#include "floyd_parser.h"
#include "floyd_interpreter.h"
#include "parser_value.h"
#include "ast_utils.h"
#include "utils.h"

using floyd_parser::scope_def_t;
using floyd_parser::base_type;
using floyd_parser::statement_t;
using floyd_parser::expression_t;
using floyd_parser::ast_t;
using floyd_parser::type_identifier_t;
using floyd_parser::scope_ref_t;
using floyd_parser::type_def_t;

using std::string;
using std::vector;
using std::make_shared;
using std::shared_ptr;

/*
	Remove all symbols, convert struct members to vectors and use index.
*/

// ??? use visitor?

scope_ref_t pass2_scope_def(const scope_ref_t& scope);





void check_variable(const scope_ref_t& scope_def, const string& s){
	QUARK_ASSERT(scope_def && scope_def->check_invariant());
	QUARK_ASSERT(s.size() > 0);

	const auto a = floyd_parser::resolve_scoped_variable(scope_def, s);
	if(!a.first){
		throw std::runtime_error("Undefined variable \"" + s + "\"");
	}
}

void check_type(const scope_ref_t& scope_def, const floyd_parser::type_identifier_t& s){
	QUARK_ASSERT(scope_def && scope_def->check_invariant());
	QUARK_ASSERT(s.check_invariant());

	const auto a = floyd_parser::resolve_type(scope_def, s);
	if(!a){
		throw std::runtime_error("Undefined type \"" + s.to_string() + "\"");
	}
}

//	Returns pre-computed expression type. Must have been pre-computed or defect.
type_identifier_t get_expression_type(const scope_ref_t& scope_def, const expression_t& e){
	if(e._call){
		QUARK_ASSERT(e._call->_function.is_resolved());
		return e._call->_function;
	}
	else if(e._math2){
	}
	else{
	}
	return {};
}

//??? Merge types_collector_t with scope_def_t::members_t ===> one local symbol table that holds types, variables etc.
/*
	Returns new expression were the types and symbols are explicit, deeply.
*/
expression_t resolve_expression_type(const scope_ref_t& scope_def, const expression_t& e){
	QUARK_ASSERT(scope_def && scope_def->check_invariant());
	QUARK_ASSERT(e.check_invariant());

	if(e._call){
		const auto& call_function_expression = *e._call;

		const auto function_def_type = resolve_type(scope_def, call_function_expression._function);
		if(!function_def_type || function_def_type->get_type() != base_type::k_function){
			throw std::runtime_error("Unresolved function.");
		}

		const auto function_def = function_def_type->get_function_def();
		if(function_def->_members.size() != call_function_expression._inputs.size()){
			throw std::runtime_error("Wrong number of argument to function call.");
		}

		//	Resolves types in the function definition itself, if needed. Argument types, types used in statements, return value etc.

		//	Resolve types for all function argument expressions.
		vector<std::shared_ptr<expression_t>> args2;
		for(int argument_index = 0 ; argument_index < call_function_expression._inputs.size() ; argument_index++){
			const auto i = call_function_expression._inputs[argument_index];
			const auto arg2 = make_shared<expression_t>(resolve_expression_type(scope_def, *i));
			args2.push_back(arg2);

			const auto arg2_type = get_expression_type(scope_def, *arg2);
			const auto function_arg_type = *function_def->_members[argument_index]._type;

//			if(!(compare_shared_values(arg2_type, function_arg_type))){
			if(!(arg2_type == function_arg_type)){
				throw std::runtime_error("Argument type missmatch.");
			}
		}

		return floyd_parser::make_function_call(function_def, args2);
	}
	else if(e._math2){
	}
	else{
	}

	return e;
}


void are_types_compatible(const type_identifier_t& type, const expression_t& expression){
	QUARK_ASSERT(type.check_invariant());
	QUARK_ASSERT(expression.check_invariant());
}



void are_symbols_resolvable(const scope_ref_t& scope_def, const expression_t& e){
	QUARK_ASSERT(scope_def && scope_def->check_invariant());
	QUARK_ASSERT(e.check_invariant());

	if(e._call){
		check_type(scope_def, e._call->_function);
		for(const auto a: e._call->_inputs){
			are_symbols_resolvable(scope_def, *a);
		}
	}
}

scope_ref_t pass2_statements(const scope_ref_t& scope_def, const statement_t& statement){
	QUARK_ASSERT(scope_def && scope_def->check_invariant());
	QUARK_ASSERT(statement.check_invariant());

	auto result = scope_def;

	if(statement._bind_statement){
//		_bind_statement->_identifier	??? make sure this identifier is not already defined in this scope!
		are_symbols_resolvable(scope_def, *statement._bind_statement->_expression);
		are_types_compatible(statement._bind_statement->_type, *statement._bind_statement->_expression);
	}
	else if(statement._define_struct){
		QUARK_ASSERT(false);
	}
	else if(statement._define_function){
		QUARK_ASSERT(false);
	}
	else if(statement._return_statement){
		are_symbols_resolvable(scope_def, *statement._return_statement->_expression);
	}
	else{
		QUARK_ASSERT(false);
	}
	return result;
}

scope_ref_t pass2_scope_def(const scope_ref_t& scope_def){
	QUARK_ASSERT(scope_def && scope_def->check_invariant());

	scope_ref_t result = scope_def;

	if(result->_type == scope_def_t::k_function){
		check_type(result->_parent_scope.lock(), result->_return_type);
	}
	else if(result->_type == scope_def_t::k_struct){
	}
	else if(result->_type == scope_def_t::k_global){
	}
	else if(result->_type == scope_def_t::k_subscope){
	}
	else{
		QUARK_ASSERT(false);
	}

	//	Make sure all types can resolve their symbols.
	for(const auto t: result->_types_collector._type_definitions){
		const auto type_def = t.second;

		if(type_def->get_type() == base_type::k_struct){
			pass2_scope_def(type_def->get_struct_def());
		}
		else if(type_def->get_type() == base_type::k_vector){
//			type_def->_struct_def
		}
		else if(type_def->get_type() == base_type::k_function){
			pass2_scope_def(type_def->get_function_def());
		}
	}

	//	Make sure all statements can resolve their symbols.
	for(const auto t: result->_executable._statements){
		result = pass2_statements(result, *t);
	}
	return result;
}



floyd_parser::ast_t run_pass2(const floyd_parser::ast_t& ast1){
	auto ast2 = ast1;

	ast2._global_scope = pass2_scope_def(ast2._global_scope);
	return ast2;
}


floyd_parser::ast_t run_pass3(const floyd_parser::ast_t& ast1){
	auto ast2 = ast1;
	return ast2;
}



///////////////////////////////////////			TESTS



QUARK_UNIT_TESTQ("struct", "Call undefined function"){
	const auto a = R"(
		string main(){
			int p = f();
			return p;
		}
		)";

	const ast_t pass1 = floyd_parser::program_to_ast(a);
	try{
		const ast_t pass2 = run_pass2(pass1);
		QUARK_UT_VERIFY(false);
	}
	catch(...){
	}
}

QUARK_UNIT_TESTQ("struct", "Return undefine type"){
	const auto a = R"(
		xyz main(){
			return 10;
		}
		)";

	const ast_t pass1 = floyd_parser::program_to_ast(a);
	try{
		const ast_t pass2 = run_pass2(pass1);
		QUARK_UT_VERIFY(false);
	}
	catch(const std::runtime_error& e){
		quark::ut_compare(string(e.what()), "Undefined type \"xyz\"");
	}
}

QUARK_UNIT_TESTQ("struct", ""){
	const auto a = R"(
		string get_s(pixel p){ return p.s; }
		struct pixel { string s = "two"; }
		string main(){
			pixel p = pixel_constructor();
			return get_s(p);
		}
		)";

	const ast_t pass1 = floyd_parser::program_to_ast(a);
	const ast_t pass2 = run_pass2(pass1);
}

#if false
QUARK_UNIT_TESTQ("struct", "Bind type mismatch"){
	const auto a = R"(
		int main(){
			int a = "hello";
			return 0;
		}
		)";

	const ast_t pass1 = floyd_parser::program_to_ast(a);
	try{
		const ast_t pass2 = run_pass2(pass1);
		QUARK_UT_VERIFY(false);
	}
	catch(const std::runtime_error& e){
		quark::ut_compare(string(e.what()), "Undefined type \"xyz\"");
	}
}
#endif

#if false
QUARK_UNIT_TESTQ("struct", "Return type mismatch"){
	const auto a = R"(
		int main(){
			return "goodbye";
		}
		)";

	const ast_t pass1 = floyd_parser::program_to_ast(a);
	try{
		const ast_t pass2 = run_pass2(pass1);
		QUARK_UT_VERIFY(false);
	}
	catch(const std::runtime_error& e){
		quark::ut_compare(string(e.what()), "Undefined type \"xyz\"");
	}
}
#endif









#if false

class visitor_i {
	public: virtual ~visitor_i(){};
	public: virtual void visitor_i_on_scope(const scope_ref_t& scope_def){};
	public: virtual void visitor_i_on_function_def(const scope_ref_t& scope_def){};
	public: virtual void visitor_i_on_struct_def(const scope_ref_t& scope_def){};
	public: virtual void visitor_i_on_global_scope(const scope_ref_t& scope_def){};

	public: virtual void visitor_i_on_expression(const scope_ref_t& scope_def){};
};

scope_ref_t visit_scope(const scope_ref_t& scope_def){
	QUARK_ASSERT(scope_def && scope_def->check_invariant());

	if(scope_def->_type == scope_def_t::k_function){
		check_type(scope_def->_parent_scope.lock(), scope_def->_return_type.to_string());
	}

	//	Make sure all statements can resolve their symbols.
	for(const auto t: scope_def->_executable._statements){
		are_symbols_resolvable(scope_def, *t);
	}

	//	Make sure all types can resolve their symbols.
	for(const auto t: scope_def->_types_collector._type_definitions){
		const auto type_def = t.second;

		if(type_def->get_type() == base_type::k_struct){
			are_symbols_resolvable(type_def->get_struct_def());
		}
		else if(type_def->get_type() == base_type::k_vector){
//			type_def->_struct_def
		}
		else if(type_def->get_type() == base_type::k_function){
			are_symbols_resolvable(type_def->get_function_def());
		}
	}
}
#endif

