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
using floyd_parser::value_t;
using floyd_parser::member_t;

using std::string;
using std::vector;
using std::make_shared;
using std::shared_ptr;

/*
	Remove all symbols, convert struct members to vectors and use index.
*/

// ??? use visitor?

void resolve_types__scope_def(scope_ref_t scope);





member_t read_struct_member(const scope_ref_t& struct_ref, const std::string& member_name){
	QUARK_ASSERT(struct_ref && struct_ref->check_invariant());
	QUARK_ASSERT(member_name.size() > 0);

	const auto found_it = find_if(
		struct_ref->_members.begin(),
		struct_ref->_members.end(),
		[&] (const member_t& member) { return member._name == member_name; }
	);
	if(found_it == struct_ref->_members.end()){
		throw std::runtime_error("Unresolved member \"" + member_name + "\"");
	}

	return *found_it;
}



type_identifier_t resolve_type_err(const scope_ref_t& scope_def, const floyd_parser::type_identifier_t& s){
	QUARK_ASSERT(scope_def && scope_def->check_invariant());
	QUARK_ASSERT(s.check_invariant());

	if(s.is_resolved()){
		return s;
	}
	else{
		const auto a = floyd_parser::resolve_type2(scope_def, s);
		if(!a.is_resolved()){
			throw std::runtime_error("Undefined type \"" + s.to_string() + "\"");
		}
		return a;
	}
}


expression_t pass2_expression_internal(const scope_ref_t& scope_def, const expression_t& e);


/*
	Returns new expression were
	- the types and symbols are explicit, deeply.
	- all types in the expressions match.
	- all symbols could be found
	- all types could be found and are correct
*/
expression_t resolve_types__expression(const scope_ref_t& scope_def, const expression_t& e){
	QUARK_ASSERT(scope_def && scope_def->check_invariant());
	QUARK_ASSERT(e.check_invariant());
	const auto r = pass2_expression_internal(scope_def, e);
	QUARK_ASSERT(!r.get_expression_type().is_null());
	return r;
}

expression_t pass2_expression_internal(const scope_ref_t& scope_def, const expression_t& e){
	QUARK_ASSERT(scope_def && scope_def->check_invariant());
	QUARK_ASSERT(e.check_invariant());

	if(e._constant){
		const auto& con = *e._constant;
		const auto const_type = resolve_type(scope_def, con.get_type());
		if(!const_type){
			throw std::runtime_error("Unresolved constant type.");
		}
		return floyd_parser::expression_t::make_constant(con, floyd_parser::type_identifier_t::resolve(const_type));
	}
	else if(e._math1){
		QUARK_ASSERT(false);
		return e;
	}
	else if(e._math2){
		const auto& math2 = *e._math2;
		const auto left = resolve_types__expression(scope_def, *math2._left);
		const auto right = resolve_types__expression(scope_def, *math2._right);
		const auto type = left.get_expression_type();
		if(left.get_expression_type().to_string() != right.get_expression_type().to_string()){
			throw std::runtime_error("Left & right side of math2 must have same type.");
		}
		return floyd_parser::expression_t::make_math_operation2(math2._operation, left, right, type);
	}
	else if(e._call){
		const auto& call = *e._call;

		//	Resolve function name.
		const auto f = resolve_type(scope_def, call._function);
		if(!f || f->get_type() != base_type::k_function){
			throw std::runtime_error("Unresolved function.");
		}
		const auto f2 = f->get_function_def();

		const auto return_type = resolve_type2(scope_def, f2->_return_type);
		QUARK_ASSERT(return_type.is_resolved());

		if(f2->_type == scope_def_t::k_function_scope){
			//	Verify & resolve all arguments in the call vs the actual function definition.
			if(f2->_members.size() != call._inputs.size()){
				throw std::runtime_error("Wrong number of argument to function call.");
			}

			vector<std::shared_ptr<expression_t>> args2;
			for(int argument_index = 0 ; argument_index < call._inputs.size() ; argument_index++){
				const auto call_arg = call._inputs[argument_index];
				const auto call_arg2 = make_shared<expression_t>(resolve_types__expression(scope_def, *call_arg));
				const auto call_arg2_type = call_arg2->get_expression_type();
				QUARK_ASSERT(call_arg2_type.is_resolved());

				const auto function_arg_type = *f2->_members[argument_index]._type;

				if(!(call_arg2_type.to_string() == function_arg_type.to_string())){
					throw std::runtime_error("Argument type mismatch.");
				}

				args2.push_back(call_arg2);
			}

			return floyd_parser::expression_t::make_function_call(type_identifier_t::resolve(f), args2, return_type);
		}
		else if(f2->_type == scope_def_t::k_subscope){
			//	Verify & resolve all arguments in the call vs the actual function definition.
			QUARK_ASSERT(call._inputs.empty());
			//??? Throw exceptions -- treat JSON-AST as user input.

			return floyd_parser::expression_t::make_function_call(type_identifier_t::resolve(f), std::vector<expression_t>{}, return_type);
		}
		else{
			QUARK_ASSERT(false);
		}
	}
	else if(e._load){
		const auto& load = *e._load;
		const auto address = resolve_types__expression(scope_def, *load._address);
		const auto resolved_type = address.get_expression_type();
		return floyd_parser::expression_t::make_load(address, resolved_type);
	}
	else if(e._resolve_variable){
		const auto& e2 = *e._resolve_variable;
		std::pair<floyd_parser::scope_ref_t, int> x = resolve_scoped_variable(scope_def, e2._variable_name);
		if(!x.first){
			throw std::runtime_error("Undefined variable \"" + e2._variable_name + "\".");
		}
		const auto& member = x.first->_members[x.second];
		const auto type = resolve_type_err(scope_def, *member._type);
		return floyd_parser::expression_t::make_resolve_variable(e2._variable_name, type);
	}
	else if(e._resolve_struct_member){
		const auto& e2 = *e._resolve_struct_member;
		const auto parent_address2 = make_shared<expression_t>(resolve_types__expression(scope_def, *e2._parent_address));

		const auto resolved_type = parent_address2->get_expression_type();
		const auto s = resolved_type.get_resolved()->get_struct_def();
		const string member_name = e2._member_name;
		member_t member_meta = read_struct_member(s, member_name);
		const auto value_type = resolve_type_err(scope_def, *member_meta._type);
		return floyd_parser::expression_t::make_resolve_struct_member(parent_address2, e2._member_name, value_type);
	}
	else if(e._lookup_element){
		QUARK_ASSERT(false);
		return e;
	}
	else{
		QUARK_ASSERT(false);
	}
}

scope_ref_t find_enclosing_function(scope_ref_t scope_ref){
	QUARK_ASSERT(scope_ref && scope_ref->check_invariant());

	if(scope_ref->_type == scope_def_t::k_function_scope){
		return scope_ref;
	}
	auto parent = scope_ref->_parent_scope.lock();
	if(parent){
		return find_enclosing_function(parent);
	}
	else{
		return {};
	}
}

statement_t resolve_types__statements(const scope_ref_t& scope_def, const statement_t& statement){
	QUARK_ASSERT(scope_def && scope_def->check_invariant());
	QUARK_ASSERT(statement.check_invariant());

	if(statement._bind_statement){
		//??? test this exception (add tests for all exceptions!)
		//	make sure this identifier is not already defined in this scope!
		const string new_identifier = statement._bind_statement->_identifier;
		const auto found_it = find_if(
			scope_def->_members.begin(),
			scope_def->_members.end(),
			[&] (const member_t& member) { return member._name == new_identifier; }
		);
		if(found_it == scope_def->_members.end()){
			throw std::runtime_error("Unknown identifier \"" + new_identifier + "\".");
		}

		const auto e2 = resolve_types__expression(scope_def, *statement._bind_statement->_expression);

		if(!(e2.get_expression_type().to_string() == statement._bind_statement->_type.to_string())){
			throw std::runtime_error("Argument type mismatch.");
		}

		auto result = statement;
		result._bind_statement->_expression = make_shared<expression_t>(e2);
		return result;
	}
	else if(statement._define_struct){
		QUARK_ASSERT(false);
	}
	else if(statement._define_function){
		QUARK_ASSERT(false);
	}
	else if(statement._return_statement){
		const auto e2 = resolve_types__expression(scope_def, *statement._return_statement->_expression);

		const auto function = find_enclosing_function(scope_def);
		if(!function){
			throw std::runtime_error("Return-statement not allowed outside function definition.");
		}

		if(!(e2.get_expression_type().to_string() == function->_return_type.to_string())){
			throw std::runtime_error("Argument type mismatch.");
		}

		auto result = statement;
		result._return_statement->_expression = make_shared<expression_t>(e2);
		return result;
	}
	else{
		QUARK_ASSERT(false);
	}
}


// Mutates the scope_def in-place.
void resolve_types__scope_def(scope_ref_t scope_def){
	QUARK_ASSERT(scope_def && scope_def->check_invariant());

	//	Make sure all types can resolve their symbols.
	for(const auto t: scope_def->_types_collector._type_definitions){
		const auto type_def = t.second;

		if(type_def->get_type() == base_type::k_struct){
			resolve_types__scope_def(type_def->get_struct_def());
		}
		else if(type_def->get_type() == base_type::k_vector){
//			type_def->_struct_def
		}
		else if(type_def->get_type() == base_type::k_function){
			resolve_types__scope_def(type_def->get_function_def());
		}
	}

	//	Make sure all members can resolve their symbols.
	for(auto member: scope_def->_members){
		*member._type = resolve_type_err(scope_def, *member._type);
	}

	if(scope_def->_type == scope_def_t::k_function_scope){
		scope_def->_return_type = resolve_type_err(scope_def->_parent_scope.lock(), scope_def->_return_type);
	}
	else if(scope_def->_type == scope_def_t::k_struct_scope){
	}
	else if(scope_def->_type == scope_def_t::k_global_scope){
	}
	else if(scope_def->_type == scope_def_t::k_subscope){
		scope_def->_return_type = resolve_type_err(scope_def->_parent_scope.lock(), scope_def->_return_type);
	}
	else{
		QUARK_ASSERT(false);
	}

	//	Make sure all statements can resolve their symbols.
	for(auto s: scope_def->_executable._statements){
		 *s = resolve_types__statements(scope_def, *s);
	}
}


floyd_parser::ast_t run_pass2(const floyd_parser::ast_t& ast1){
//??? Copy ast shared mutable state!!
	auto ast2 = ast1;

	resolve_types__scope_def(ast2._global_scope);
	trace(ast2);
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

//??? cannot find variable "p" inside main() because local variables are not stored as function-members at compile time.

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
		quark::ut_compare(string(e.what()), "Argument type mismatch.");
	}
}

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
		quark::ut_compare(string(e.what()), "Argument type mismatch.");
	}
}







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

