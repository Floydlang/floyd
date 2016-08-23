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
#include "json_support.h"

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
using floyd_parser::ast_path_t;

using std::string;
using std::vector;
using std::make_shared;
using std::shared_ptr;


void resolve_types__scope_def_mut(const ast_t& ast, const ast_path_t& path, scope_ref_t& scope_def_mut);


expression_t pass2_expression_internal(const ast_t& ast, const ast_path_t& path, const scope_ref_t& scope_def, const expression_t& e);


expression_t resolve_types__expression(const ast_t& ast, const ast_path_t& path, const scope_ref_t& scope_def, const expression_t& e){
	QUARK_ASSERT(ast.check_invariant());
	QUARK_ASSERT(path.check_invariant());
	QUARK_ASSERT(scope_def && scope_def->check_invariant());
	QUARK_ASSERT(e.check_invariant());
	const auto r = pass2_expression_internal(ast, path, scope_def, e);
	QUARK_ASSERT(!r.get_expression_type().is_null());
	return r;
}

expression_t pass2_expression_internal(const ast_t& ast, const ast_path_t& path, const scope_ref_t& scope_def, const expression_t& e){
	QUARK_ASSERT(ast.check_invariant());
	QUARK_ASSERT(path.check_invariant());
	QUARK_ASSERT(scope_def && scope_def->check_invariant());
	QUARK_ASSERT(e.check_invariant());

	if(e._constant){
		const auto& con = *e._constant;
		const auto const_type = resolve_type(ast, path, scope_def, con.get_type());
		if(!const_type){
			throw std::runtime_error("1000 - Unknown constant type \"" + con.get_type().to_string() + "\".");
		}
		return floyd_parser::expression_t::make_constant(con, floyd_parser::type_identifier_t::resolve(const_type));
	}
	else if(e._math1){
		QUARK_ASSERT(false);
		return e;
	}
	else if(e._math2){
		const auto& math2 = *e._math2;
		const auto left = resolve_types__expression(ast, path, scope_def, *math2._left);
		const auto right = resolve_types__expression(ast, path, scope_def, *math2._right);
		const auto type = left.get_expression_type();
		if(left.get_expression_type().to_string() != right.get_expression_type().to_string()){
			throw std::runtime_error("1001 - Left & right side of math2 must have same type.");
		}
		return floyd_parser::expression_t::make_math_operation2(math2._operation, left, right, type);
	}
	else if(e._call){
		const auto& call = *e._call;

		//	Resolve function name.
		const auto f = resolve_type(ast, path, scope_def, call._function);
		if(!f || f->get_type() != base_type::k_function){
			throw std::runtime_error("1002 - Undefined function \"" + call._function.to_string() + "\".");
		}
		const auto f2 = f->get_function_def();

		const auto return_type = resolve_type2(ast, path, scope_def, f2->_return_type);
		QUARK_ASSERT(return_type.is_resolved());

		if(f2->_type == scope_def_t::k_function_scope){
			//	Verify & resolve all arguments in the call vs the actual function definition.
			if(f2->_members.size() != call._inputs.size()){
				throw std::runtime_error("1003 - Wrong number of argument to function \"" + call._function.to_string() + "\".");
			}

			vector<std::shared_ptr<expression_t>> args2;
			for(int argument_index = 0 ; argument_index < call._inputs.size() ; argument_index++){
				const auto call_arg = call._inputs[argument_index];
				const auto call_arg2 = make_shared<expression_t>(resolve_types__expression(ast, path, scope_def, *call_arg));
				const auto call_arg2_type = call_arg2->get_expression_type();
				QUARK_ASSERT(call_arg2_type.is_resolved());

				const auto function_arg_type = *f2->_members[argument_index]._type;

				if(!(call_arg2_type.to_string() == function_arg_type.to_string())){
					throw std::runtime_error("1004 - Argument " + std::to_string(argument_index) + " to function \"" + call._function.to_string() + "\" mismatch.");
				}
				args2.push_back(call_arg2);
			}

			return floyd_parser::expression_t::make_function_call(type_identifier_t::resolve(f), args2, return_type);
		}
		else if(f2->_type == scope_def_t::k_subscope){
			//	Verify & resolve all arguments in the call vs the actual function definition.
			QUARK_ASSERT(call._inputs.empty());
			//??? Throw exceptions -- treat JSON-AST as user input.

			return floyd_parser::expression_t::make_function_call(type_identifier_t::resolve(f), std::vector<shared_ptr<expression_t>>{}, return_type);
		}
		else{
			QUARK_ASSERT(false);
		}
	}
	else if(e._load){
		const auto& load = *e._load;
		const auto address = resolve_types__expression(ast, path, scope_def, *load._address);
		const auto resolved_type = address.get_expression_type();
		return floyd_parser::expression_t::make_load(address, resolved_type);
	}
	else if(e._resolve_variable){
		const auto& e2 = *e._resolve_variable;
		std::pair<floyd_parser::scope_ref_t, int> x = resolve_scoped_variable(ast, path, e2._variable_name);
		if(!x.first){
			throw std::runtime_error("1005 - Undefined variable \"" + e2._variable_name + "\".");
		}
		const auto& member = x.first->_members[x.second];

		//	Error 1009
		const auto type = resolve_type_throw(ast, path, scope_def, *member._type);

		return floyd_parser::expression_t::make_resolve_variable(e2._variable_name, type);
	}
	else if(e._resolve_struct_member){
		const auto& e2 = *e._resolve_struct_member;
		const auto parent_address2 = make_shared<expression_t>(resolve_types__expression(ast, path, scope_def, *e2._parent_address));

		const auto resolved_type = parent_address2->get_expression_type();
		const auto s = resolved_type.get_resolved()->get_struct_def();
		const string member_name = e2._member_name;

		//	Error 1010
		member_t member_meta = find_struct_member_throw(s, member_name);

		//	Error 1011
		const auto value_type = resolve_type_throw(ast, path, scope_def, *member_meta._type);
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

scope_ref_t find_enclosing_function(const ast_t& ast, const ast_path_t& path, scope_ref_t scope_ref){
	QUARK_ASSERT(scope_ref && scope_ref->check_invariant());

	const auto res = resolve_path(ast, path);
	for(auto i = res._scopes.size() ; i > 0 ; i--){
		if(res._scopes[i - 1]->_type == floyd_parser::scope_def_t::k_function_scope){
			return res._scopes[i - 1];
		}
	}
	return {};
}

statement_t resolve_types__statements(const ast_t& ast, const ast_path_t& path, const scope_ref_t& scope_def, const statement_t& statement){
	QUARK_ASSERT(ast.check_invariant());
	QUARK_ASSERT(path.check_invariant());
	QUARK_ASSERT(scope_def && scope_def->check_invariant());
	QUARK_ASSERT(statement.check_invariant());

	if(statement._bind_statement){
		//??? test this exception (add tests for all exceptions!)
		//	make sure this identifier is not already defined in this scope!
		const string new_identifier = statement._bind_statement->_identifier;

		//	Make sure we have a spot for this member, or throw.
		//	Error 1012
		const auto& member = floyd_parser::find_struct_member_throw(scope_def, new_identifier);
		QUARK_ASSERT(member.check_invariant());

		const auto e2 = resolve_types__expression(ast, path, scope_def, *statement._bind_statement->_expression);

		if(!(e2.get_expression_type().to_string() == statement._bind_statement->_type.to_string())){
			throw std::runtime_error("1006 - Bind type mismatch.");
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
		const auto e2 = resolve_types__expression(ast, path, scope_def, *statement._return_statement->_expression);

		const auto function = find_enclosing_function(ast, path, scope_def);
		if(!function){
			throw std::runtime_error("1007 - Return-statement not allowed outside function definition.");
		}

		if(!(e2.get_expression_type().to_string() == function->_return_type.to_string())){
			throw std::runtime_error("1008 - return value doesn't match function return type.");
		}

		auto result = statement;
		result._return_statement->_expression = make_shared<expression_t>(e2);
		return result;
	}
	else{
		QUARK_ASSERT(false);
	}
}

void resolve_types__scope_def_mut(const ast_t& ast, const ast_path_t& path, scope_ref_t& scope_def_mut){
	QUARK_ASSERT(ast.check_invariant());
	QUARK_ASSERT(path.check_invariant());
	QUARK_ASSERT(scope_def_mut && scope_def_mut->check_invariant());

	//	Make sure all types can resolve their symbols.
	for(const auto t: scope_def_mut->_types_collector._type_definitions){
		if(t.second->is_subscope()){
			auto scope2_mut = t.second->get_subscope();
			const ast_path_t path2 { path._names + scope2_mut->_name.to_string() };
			resolve_types__scope_def_mut(ast, path2, scope2_mut);
		}
	}

	//	Make sure all members can resolve their symbols.
	for(auto member: scope_def_mut->_members){
		//	Error 1013
		//??? Tries to mutate scope_def.
		*member._type = resolve_type_throw(ast, path, scope_def_mut, *member._type);
	}

	if(scope_def_mut->_type == scope_def_t::k_function_scope){
		//	Error 1014
		const auto return_type = resolve_type_throw(ast, path, scope_def_mut, scope_def_mut->_return_type);

		scope_ref_t scope2 = scope_def_t::make2(
			scope_def_mut->_type,
			scope_def_mut->_name,
			scope_def_mut->_members,
			scope_def_mut->_executable,
			scope_def_mut->_types_collector,
			return_type
		);
		scope_def_mut.swap(scope2);
	}
	else if(scope_def_mut->_type == scope_def_t::k_struct_scope){
	}
	else if(scope_def_mut->_type == scope_def_t::k_global_scope){
	}
	else if(scope_def_mut->_type == scope_def_t::k_subscope){
		//	Error 1015
		const auto return_type = resolve_type_throw(ast, path, scope_def_mut, scope_def_mut->_return_type);

		scope_ref_t scope2 = scope_def_t::make2(
			scope_def_mut->_type,
			scope_def_mut->_name,
			scope_def_mut->_members,
			scope_def_mut->_executable,
			scope_def_mut->_types_collector,
			return_type
		);
		scope_def_mut.swap(scope2);
	}
	else{
		QUARK_ASSERT(false);
	}

	//	Make sure all statements can resolve their symbols.
	for(auto s: scope_def_mut->_executable._statements){
		 *s = resolve_types__statements(ast, path, scope_def_mut, *s);
	}
}

bool has_unresolved_types(const floyd_parser::ast_t& ast1){
	const auto a = floyd_parser::scope_def_to_json(*ast1._global_scope);
	const auto s = json_to_compact_string(a);

	const auto found = s.find("<>");
	return found != std::string::npos;
}

// !!! Mutates ast1???
floyd_parser::ast_t run_pass2(const floyd_parser::ast_t& ast1){
	//??? Copy ast shared mutable state!!
	auto ast2 = ast1;

	resolve_types__scope_def_mut(ast2, make_root(ast2), ast2._global_scope);
	trace(ast2);

	//	Make sure there are no unresolved types left in program.
	QUARK_ASSERT(!has_unresolved_types(ast2));

	return ast2;
}




///////////////////////////////////////			TESTS

/*
	These tests verify that:
	- the transform of the AST worked as expected
	- Correct errors are emitted.
*/


QUARK_UNIT_TESTQ("run_pass2()", "Minimum program"){
	const auto a = R"(
		int main(){
			return 3;
		}
		)";
	const ast_t pass1 = floyd_parser::program_to_ast(a);
	const ast_t pass2 = run_pass2(pass1);
}


//	This program uses all features of pass2.???
QUARK_UNIT_TESTQ("run_pass2()", "Maxium program"){
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



void test_error(const std::string& program, const std::string& error_string){
	const ast_t pass1 = floyd_parser::program_to_ast(program);
	try{
		const ast_t pass2 = run_pass2(pass1);
		QUARK_UT_VERIFY(false);
	}
	catch(const std::runtime_error& e){
		const auto s = std::string(e.what());
		quark::ut_compare(s, error_string);
	}
}



QUARK_UNIT_TESTQ("run_pass2()", "1001"){
	test_error(
		R"(
			string main(){
				return 10 * "hello";
			}
		)",
		"1001 - Left & right side of math2 must have same type."
	);
}

QUARK_UNIT_TESTQ("run_pass2()", "1002"){
	test_error(
		R"(
			string main(){
				int p = f();
				return p;
			}
		)",
		"1002 - Undefined function \"f\"."
	);
}

QUARK_UNIT_TESTQ("run_pass2()", "1003"){
	test_error(
		R"(
			string main(){
				int p = main(42);
				return p;
			}
		)",
		"1003 - Wrong number of argument to function \"main\"."
	);
}

QUARK_UNIT_TESTQ("run_pass2()", "1004"){
	test_error(
		R"(
			int a(string p1){
				return 31;
			}
			string main(){
				int p = a(42);
				return p;
			}
		)",
		"1004 - Argument 0 to function \"a\" mismatch."
	);
}

QUARK_UNIT_TESTQ("run_pass2()", "1005"){
	test_error(
		R"(
			string main(){
				return p;
			}
		)",
		"1005 - Undefined variable \"p\"."
	);
}

QUARK_UNIT_TESTQ("run_pass2()", "Return undefine type"){
	test_error(
		R"(
			xyz main(){
				return 10;
			}
		)",
		"Undefined type \"xyz\""
	);
}

QUARK_UNIT_TESTQ("run_pass2()", "1006"){
	test_error(
		R"(
			int main(){
				int a = "hello";
				return 0;
			}
		)",
		"1006 - Bind type mismatch."
	);
}

QUARK_UNIT_TESTQ("run_pass2()", "1007"){
	test_error(
		R"(
			return 4;
			int main(){
				return 0;
			}
		)",
		"1007 - Return-statement not allowed outside function definition."
	);
}


QUARK_UNIT_TESTQ("run_pass2()", "1008"){
	test_error(
		R"(
			int main(){
				return "goodbye";
			}
		)",
		"1008 - return value doesn't match function return type."
	);
}


QUARK_UNIT_TESTQ("run_pass2()", "1010"){
	test_error(
		R"(
			struct pixel { string s = "two"; }
			string main(){
				pixel p = pixel_constructor();
				int a = p.xyz;
				return 1;
			}
		)",
		"Unresolved member \"xyz\""
	);
}

/*
Can't get this test past the parser...
Make a json_to_ast() so we can try arbitrary asts with run_pass2().

QUARK_UNIT_TESTQ("run_pass2()", "1011"){
	test_error(
		R"(
			struct pixel { rgb s = "two"; }
			string main(){
				pixel p = pixel_constructor();
				int a = p.s;
				return 1;
			}
		)",
		"Unresolved member \"xyz\""
	);
}
*/
