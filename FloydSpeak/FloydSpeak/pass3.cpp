//
//  pass3.cpp
//  FloydSpeak
//
//  Created by Marcus Zetterquist on 21/08/16.
//  Copyright © 2016 Marcus Zetterquist. All rights reserved.
//

#include "pass3.h"

#include "parse_statement.h"
#include "floyd_parser.h"
#include "floyd_interpreter.h"
#include "parser_value.h"
#include "utils.h"











#if false

json_t resolve_scope_def(const parser_path_t& path);
expression_t pass2_expression_internal(const resolved_path_t& path, const expression_t& e);


scope_def_t::etype string_to_scope_type(const std::string s){
	const auto lookup = std::map<string, scope_def_t::etype>{
		{ "function", scope_def_t::k_function_scope },
		{ "struct", scope_def_t::k_struct_scope },
		{ "global", scope_def_t::k_global_scope },
		{ "subscope", scope_def_t::k_subscope }
	};
	return lookup.at(s);
}

expression_t resolve_types__expression(const resolved_path_t& path, const expression_t& e){
	QUARK_ASSERT(path.check_invariant());
	QUARK_ASSERT(e.check_invariant());

	const auto r = pass2_expression_internal(path, e);

	QUARK_ASSERT(!r.get_expression_type().is_null());
	return r;
}

bool returns_same_type(floyd_parser::math_operation2_expr_t::operation op){
	using floyd_parser::math_operation2_expr_t;
	return
		op == math_operation2_expr_t::operation::k_add
		|| op == math_operation2_expr_t::operation::k_subtract
		|| op == math_operation2_expr_t::operation::k_multiply
		|| op == math_operation2_expr_t::operation::k_divide
		|| op == math_operation2_expr_t::operation::k_remainder;
}

bool returns_bool(floyd_parser::math_operation2_expr_t::operation op){
	using floyd_parser::math_operation2_expr_t;

	return
		op == math_operation2_expr_t::operation::k_smaller_or_equal
		|| op == math_operation2_expr_t::operation::k_smaller
		|| op == math_operation2_expr_t::operation::k_larger_or_equal
		|| op == math_operation2_expr_t::operation::k_larger

		|| op == math_operation2_expr_t::operation::k_logical_equal
		|| op == math_operation2_expr_t::operation::k_logical_nonequal
		|| op == math_operation2_expr_t::operation::k_logical_and
		|| op == math_operation2_expr_t::operation::k_logical_or;
}




expression_t pass2_expression_internal(const resolved_path_t& path, const expression_t& e){
	QUARK_ASSERT(path.check_invariant());
	QUARK_ASSERT(e.check_invariant());

	const auto scope_def = path.get_leaf();
	if(e._constant){
		const auto& con = *e._constant;
		const auto const_type = resolve_type_to_def(path, con.get_type());
		if(!const_type){
			throw std::runtime_error("1000 - Unknown constant type \"" + con.get_type().to_string() + "\".");
		}
		return floyd_parser::expression_t::make_constant(con, type_identifier_t::resolve(const_type));
	}
	else if(e._math1){
		//???
		QUARK_ASSERT(false);
		return e;
	}
	else if(e._math2){
		const auto& math2 = *e._math2;
		const auto left = resolve_types__expression(path, *math2._left);
		const auto right = resolve_types__expression(path, *math2._right);

		if(left.get_expression_type().to_string() != right.get_expression_type().to_string()){
			throw std::runtime_error("1001 - Left & right side of math2 must have same type.");
		}

		if(returns_same_type(math2._operation)){
			const auto type = left.get_expression_type();
			return floyd_parser::expression_t::make_math_operation2(math2._operation, left, right, type);
		}
		else if(returns_bool(math2._operation)){
			const auto type = resolve_type_to_id(path, type_identifier_t::make_bool());
			return floyd_parser::expression_t::make_math_operation2(math2._operation, left, right, type);
		}
		else{
			QUARK_ASSERT(false);
		}

	}
	else if(e._conditional_operator){
		const auto& cond = *e._conditional_operator;
		const auto condition_expr = resolve_types__expression(path, *cond._condition);
		const auto true_expr = resolve_types__expression(path, *cond._a);
		const auto false_expr = resolve_types__expression(path, *cond._b);
		const auto type = true_expr.get_expression_type();
		if(true_expr.get_expression_type().to_string() != false_expr.get_expression_type().to_string()){
			throw std::runtime_error("1001 - Left & right side of (?:) must have same type.");
		}
		return floyd_parser::expression_t::make_conditional_operator(condition_expr, true_expr, false_expr, type);
	}
	else if(e._call){
		const auto& call = *e._call;

		//	Resolve function name.
		const auto f = resolve_type_to_def(path, call._function);
		if(!f || f->get_type() != base_type::k_function){
			throw std::runtime_error("1002 - Undefined function \"" + call._function.to_string() + "\".");
		}
		const auto f2 = f->get_function_def();

		const auto return_type = resolve_type_to_id(path, f2->_return_type);
		QUARK_ASSERT(return_type.is_resolved());

		if(f2->_type == scope_def_t::k_function_scope){
			//	Verify & resolve all arguments in the call vs the actual function definition.
			if(f2->_members.size() != call._inputs.size()){
				throw std::runtime_error("1003 - Wrong number of argument to function \"" + call._function.to_string() + "\".");
			}

			vector<expression_t> args2;
			for(int argument_index = 0 ; argument_index < call._inputs.size() ; argument_index++){
				const auto call_arg = call._inputs[argument_index];
				const auto call_arg2 = resolve_types__expression(path, call_arg);
				const auto call_arg2_type = call_arg2.get_expression_type();
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

			return floyd_parser::expression_t::make_function_call(type_identifier_t::resolve(f), vector<expression_t>{}, return_type);
		}
		else{
			QUARK_ASSERT(false);
		}
	}
	else if(e._load){
		const auto& load = *e._load;
		const auto address = resolve_types__expression(path, *load._address);
		const auto resolved_type = address.get_expression_type();
		return floyd_parser::expression_t::make_load(address, resolved_type);
	}
	else if(e._resolve_variable){
		const auto& e2 = *e._resolve_variable;
		std::pair<floyd_parser::scope_ref_t, int> x = resolve_scoped_variable(path, e2._variable_name);
		if(!x.first){
			throw std::runtime_error("1005 - Undefined variable \"" + e2._variable_name + "\".");
		}
		const auto& member = x.first->_members[x.second];
//??? Replace this with a pointer to the scope_def and the index! It's statically resolved now!
		//	Error 1009
		const auto type = resolve_type_throw(path, *member._type);

		return floyd_parser::expression_t::make_resolve_variable(e2._variable_name, type);
	}
	else if(e._resolve_member){
		const auto& e2 = *e._resolve_member;
		const auto parent_address2 = make_shared<expression_t>(resolve_types__expression(path, *e2._parent_address));

		const auto resolved_type = parent_address2->get_expression_type();
		const auto s = resolved_type.get_resolved()->get_struct_def();
		const string member_name = e2._member_name;

		//	Error 1010
		member_t member_meta = find_struct_member_throw(s, member_name);

		//	Error 1011
		const auto value_type = resolve_type_throw(path, *member_meta._type);
		return floyd_parser::expression_t::make_resolve_member(parent_address2, e2._member_name, value_type);
	}
	else if(e._lookup_element){
		QUARK_ASSERT(false);
		return e;
	}
	else{
		QUARK_ASSERT(false);
	}
}

scope_ref_t find_enclosing_function(const resolved_path_t& path){
	QUARK_ASSERT(path.check_invariant());

	for(auto i = path._scopes.size() ; i > 0 ; i--){
		if(path._scopes[i - 1]->_type == floyd_parser::scope_def_t::k_function_scope){
			return path._scopes[i - 1];
		}
	}
	return {};
}

statement_t resolve_types__statement(const resolved_path_t& path, const statement_t& statement){
	QUARK_ASSERT(path.check_invariant());
	QUARK_ASSERT(statement.check_invariant());

	const auto scope_def = path.get_leaf();
	if(statement._bind_statement){
		//??? test this exception (add tests for all exceptions!)
		//	make sure this identifier is not already defined in this scope!
		const string new_identifier = statement._bind_statement->_identifier;

		//	Make sure we have a spot for this member, or throw.
		//	Error 1012
		const auto& member = floyd_parser::find_struct_member_throw(scope_def, new_identifier);
		QUARK_ASSERT(member.check_invariant());

		const auto e2 = resolve_types__expression(path, *statement._bind_statement->_expression);

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
		const auto e2 = resolve_types__expression(path, *statement._return_statement->_expression);

		const auto function = find_enclosing_function(path);
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

#endif








#if 0
json_t resolve_type_def(const parser_path_t& path, const json_t& type_def){
	QUARK_ASSERT(path.check_invariant());
	QUARK_ASSERT(type_def.check_invariant());

	QUARK_TRACE(json_to_compact_string(type_def));

	//??? We peek into scope_def to find type. What about typedefs of ints?
	//	If this type-def holds it's own scope, resolve that scope too.
	const auto type = get_in(type_def, {"type"}).get_string();
	if(type == "function"){
		const parser_path_t path2 = go_down(path, type_def);
		const auto function_scope = resolve_scope_def(path2);
		return make_shared<type_def_t>(type_def_t::make_function_def(function_scope));
	}
	else if(type == "struct"){
		QUARK_ASSERT(false);
	}
	else{
		QUARK_ASSERT(false);
	}

/*
	if(type_def1->is_subscope()){
		auto scope3 = type_def1->get_subscope();
		const resolved_path_t path2 = go_down(path, scope3);
		scope_ref_t scope4 = resolve_type_def(parse_path, path2);
		shared_ptr<type_def_t> type_def2 = make_shared<type_def_t>(type_def1->replace_subscope(scope4));
		type_entry2._defs.push_back(type_def2);
	}
	else{
		type_entry2._defs.push_back(type_def1);
	}
*/

	return make_shared<type_def_t>(type_def_t::make_int());
}
#endif



#if 0
/*
	Returns a scope_ref_t where all types and variable names are resolved

	Creates the scope_def_t ON THE WAY UP from callstack. This means all actual resolving must be done on JSON-structures.
*/
json_t resolve_scope_def(const parser_path_t& path){
	QUARK_ASSERT(path.check_invariant());

	const auto scope1 = path._scopes.back();

	auto types_collector2 = json_t::make_object();

	//	Make sure all types can resolve their symbols.
	//	??? this makes it impossible for types to refer to eachother depending on in which order we process them!?
	{
		const auto types = get_in(scope1, { "types" }).get_object();
		QUARK_TRACE(json_to_compact_string(types));
		for(const auto type_entry_pair: types){
			const string name = type_entry_pair.first;
			const auto& type_defs = type_entry_pair.second.get_array();

			std::vector<json_t> type_defs2;
			for(const auto& type_def: type_defs){
				QUARK_TRACE(json_to_compact_string(type_def));

				json_t resolved_type_def = resolve_type_def(path, type_def);
				type_defs2.push_back(resolved_type_def);
			}

			types_collector2 = assoc(types_collector2, name, json_t(type_defs2));
		}
	}

	const auto scope3 = assoc(scope1, "types", types_collector2);


	//??? Should use a new dest_path where our half-baked scope_def is included!!

	//	Make sure all members can resolve their symbols.
	//	They will try to access their local variables / members, arguments etc.
	{
		vector<floyd_parser::member_t> members2;
		for(const auto& member1: scope3->_members){
			//	Error 1013
			const auto type2 = resolve_type_throw(dest_path, *member1._type);

			if(member1._value){
				//	Error ???
				const auto init_value_type = resolve_type_throw(dest_path, *member1._type);

				const auto init_value2 = member1._value->resolve_type(init_value_type);

				//	Error ???
				if(init_value_type.to_string() != type2.to_string()){
					throw std::runtime_error("??? \"" + init_value_type.to_string() + "\".");
				}

				const member_t member2 = floyd_parser::member_t(type2, member1._name, init_value2);
				members2.push_back(member2);
			}
			else{
				const member_t member2 = member_t(type2, member1._name);
				members2.push_back(member2);
			}
		}
		scope3 = scope_def_t::make2(
			scope3->_type,
			scope3->_name,
			members2,
			scope3->_executable,
			scope3->_types_collector,
			scope3->_return_type
		);
	}

	//??? Should use a new dest_path where our half-baked scope_def is included!!
	//	Make sure all statements can resolve their symbols. This includes access to its own members.
	scope_ref_t scope4 = scope3;
	{
		vector<shared_ptr<statement_t>> statements2;
		for(auto s: scope4->_executable._statements){
			 const auto statement2 = resolve_types__statement(dest_path, *s);
			 statements2.push_back(make_shared<statement_t>(statement2));
		}

		scope4 = scope_def_t::make2(
			scope4->_type,
			scope4->_name,
			scope4->_members,
			floyd_parser::executable_t(statements2),
			scope4->_types_collector,
			scope4->_return_type
		);
	}

	//	??? Add special handling for function-scopes that create function + body scopes.
	/*
		make_scope_def()

		{ "type", "" },
		{ "name", "" },
		{ "members", json_t::make_array() },
		{ "types", json_t::make_object() },

		//??? New in JSON, used to stored as sub-function body.
		{ "locals", json_t::make_array() },

		//	??? New in JSON version - used to be stored in _executable.
		{ "statements", json_t::make_array() },
		{ "return_type", "" }
	*/
	const auto scope_type = scope1.get_object_element("type").get_string();
	const auto scope_name = scope1.get_object_element("name").get_string();
	const auto scope_members = scope1.get_object_element("members");
	const auto scope_types_collector = scope1.get_object_element("types");
	const auto scope_locals = scope1.get_object_element("locals");
	const auto scope_statements = scope1.get_object_element("statements");
	const auto scope__return_type = scope1.get_object_element("return_type").get_string();

	scope_def_t::etype scope_type2 = string_to_scope_type(scope_type);


	const std::vector<std::shared_ptr<statement_t> > statements;

	const std::vector<member_t> members;
	const floyd_parser::executable_t executable(statements);


	types_collector_t types_collector;
	types_collector._types = result__types_collector__types;

	//	Error 1014
	const auto return_type = scope__return_type.empty() ? type_identifier_t() : resolve_type_throw(dest_path, type_identifier_t::make(scope__return_type));

	scope_ref_t scope2 = scope_def_t::make2(scope_type2, type_identifier_t::make(scope_name), members, executable, types_collector, return_type);



	QUARK_ASSERT(scope4->check_invariant());
	return scope4;
}
#endif


