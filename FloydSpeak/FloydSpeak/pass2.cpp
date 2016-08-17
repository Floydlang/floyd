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

using floyd_parser::scope_def_t;
using floyd_parser::base_type;
using floyd_parser::statement_t;
using floyd_parser::expression_t;
using floyd_parser::ast_t;
using floyd_parser::type_identifier_t;
using floyd_parser::scope_ref_t;

using std::string;

/*
	Remove all symbols, convert struct members to vectors and use index.
*/

// ??? use visitor?

void are_symbols_resolvable(const floyd_parser::scope_ref_t& scope);





void check_variable(const scope_ref_t& scope_def, const string& s){
	QUARK_ASSERT(scope_def && scope_def->check_invariant());
	QUARK_ASSERT(s.size() > 0);

	const auto a = floyd_interpreter::resolve_scoped_variable(scope_def, s);
	if(!a.first){
		throw std::runtime_error("Undefined variable \"" + s + "\"");
	}
}

void check_type(const scope_ref_t& scope_def, const string& s){
	QUARK_ASSERT(scope_def && scope_def->check_invariant());
	QUARK_ASSERT(s.size() > 0);

	const auto a = floyd_interpreter::resolve_scoped_type(scope_def, s);
	if(!a.first){
		throw std::runtime_error("Undefined type \"" + s + "\"");
	}
}


type_identifier_t get_expression_type(const scope_ref_t& scope_def, const expression_t& e){
	QUARK_ASSERT(scope_def && scope_def->check_invariant());
	QUARK_ASSERT(e.check_invariant());

#if false
	if(e._call){
		const auto a = floyd_interpreter::resolve_scoped_variable(scope_def, s);
		if(!a.first){
			throw std::runtime_error("Undefined variable \"" + s + "\"");
		}
	}
	else if(e._call){
	}
	else{
		return type_;
	}
#endif
	return type_identifier_t();
}


void are_types_compatible(const type_identifier_t& type, const expression_t& expression){
	QUARK_ASSERT(type.check_invariant());
	QUARK_ASSERT(expression.check_invariant());
}




void are_symbols_resolvable(const scope_ref_t& scope_def, const expression_t& e){
	QUARK_ASSERT(scope_def && scope_def->check_invariant());
	QUARK_ASSERT(e.check_invariant());

	if(e._call){
		check_type(scope_def, e._call->_function_name);
		for(const auto a: e._call->_inputs){
			are_symbols_resolvable(scope_def, *a);
		}
	}
}

void are_symbols_resolvable(const scope_ref_t& scope_def, const statement_t& statement){
	QUARK_ASSERT(scope_def && scope_def->check_invariant());
	QUARK_ASSERT(statement.check_invariant());

	if(statement._bind_statement){
//		_bind_statement->_identifier	??? make sure this identifier is not already defined in this scope!
		are_symbols_resolvable(scope_def, *statement._bind_statement->_expression);
		are_types_compatible(statement._bind_statement->_type, *statement._bind_statement->_expression);
	}
	else if(statement._define_struct){
		QUARK_ASSERT(false);
		are_symbols_resolvable(statement._define_struct->_struct_def);
	}
	else if(statement._define_function){
		QUARK_ASSERT(false);
		are_symbols_resolvable(statement._define_function->_function_def);
	}
	else if(statement._return_statement){
		are_symbols_resolvable(scope_def, *statement._return_statement->_expression);
	}
	else{
		QUARK_ASSERT(false);
	}
}

/*
		public: etype _type;
		public: type_identifier_t _name;
		public: std::vector<member_t> _members;
		public: std::weak_ptr<scope_def_t> _parent_scope;
		public: executable_t _executable;
		public: types_collector_t _types_collector;
		public: type_identifier_t _return_type;
*/

void are_symbols_resolvable(const scope_ref_t& scope_def){
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

floyd_parser::ast_t run_pass2(const floyd_parser::ast_t& ast1){
	auto ast2 = ast1;
	are_symbols_resolvable(ast2._global_scope);
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
void check_variable(const scope_ref_t& scope_def, const string& s){
	const auto a = floyd_interpreter::resolve_scoped_variable(scope_def, s);
	if(!a.first){
		throw std::runtime_error("Undefined variable \"" + s + "\"");
	}
}

void check_type(const scope_ref_t& scope_def, const string& s){
	const auto a = floyd_interpreter::resolve_scoped_type(scope_def, s);
	if(!a.first){
		throw std::runtime_error("Undefined type \"" + s + "\"");
	}
}

void are_symbols_resolvable(const scope_ref_t& scope_def, const expression_t& e){
	QUARK_ASSERT(scope_def && scope_def->check_invariant());
	QUARK_ASSERT(e.check_invariant());

	if(e._call){
		check_type(scope_def, e._call->_function_name);
		for(const auto a: e._call->_inputs){
			are_symbols_resolvable(scope_def, *a);
		}
	}
}

void are_symbols_resolvable(const scope_ref_t& scope_def, const statement_t& statement){
	QUARK_ASSERT(scope_def && scope_def->check_invariant());
	QUARK_ASSERT(statement.check_invariant());

	if(statement._bind_statement){
//		_bind_statement->_identifier	??? make sure this identifier is not already defined in this scope!
		are_symbols_resolvable(scope_def, *statement._bind_statement->_expression);
	}
	else if(statement._define_struct){
		QUARK_ASSERT(false);
		are_symbols_resolvable(statement._define_struct->_struct_def);
	}
	else if(statement._define_function){
		QUARK_ASSERT(false);
		are_symbols_resolvable(statement._define_function->_function_def);
	}
	else if(statement._return_statement){
		are_symbols_resolvable(scope_def, *statement._return_statement->_expression);
	}
	else{
		QUARK_ASSERT(false);
	}
}

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

