	//
//  pass2.cpp
//  FloydSpeak
//
//  Created by Marcus Zetterquist on 09/08/16.
//  Copyright © 2016 Marcus Zetterquist. All rights reserved.
//

#include "pass3.h"

#include "statement.h"
#include "floyd_parser.h"
#include "ast_value.h"
#include "ast_basics.h"
#include "utils.h"
#include "json_support.h"
#include "json_parser.h"
#include "parser_primitives.h"
#include "host_functions.hpp"
#include <fstream>

namespace floyd_pass3 {

	using namespace std;
	using floyd::value_t;
	using floyd::typeid_t;
	using floyd::base_type;
	using floyd::expression_t;
	using floyd::statement_t;
	using floyd::ast_json_t;
	using floyd::member_t;
	using floyd::struct_definition_t;
	using floyd::expression_type;
	using floyd::ast_t;
	using floyd::function_definition_t;
	using floyd::keyword_t;
	using floyd::parser_context_t;
	using floyd::symbol_t;
	using floyd::body_t;

std::pair<analyser_t, std::shared_ptr<statement_t>> analyse_statement(const analyser_t& vm, const statement_t& statement);



std::vector<std::pair<std::string, symbol_t>> symbol_map_to_vec(const std::map<std::string, symbol_t>& symbols){
	std::vector<std::pair<std::string, symbol_t>> result;
	for(const auto e: symbols){
		result.push_back({e.first, e.second});
	}
	return result;
}


std::pair<analyser_t, vector<shared_ptr<statement_t>>> analyse_statements(const analyser_t& vm, const vector<shared_ptr<statement_t>>& statements){
	QUARK_ASSERT(vm.check_invariant());
	for(const auto i: statements){ QUARK_ASSERT(i->check_invariant()); };

	auto vm_acc = vm;

	vector<shared_ptr<statement_t>> statements2;
	int statement_index = 0;
	while(statement_index < statements.size()){
		const auto statement = statements[statement_index];
		const auto& r = analyse_statement(vm_acc, *statement);
		vm_acc = r.first;

		if(r.second){
			QUARK_ASSERT(r.second->is_annotated_deep());
			statements2.push_back(r.second);
		}
		statement_index++;
	}
	return { vm_acc, statements2 };
}

std::pair<analyser_t, body_t > analyse_body(const analyser_t& vm, const floyd::body_t& body){
	QUARK_ASSERT(vm.check_invariant());

	auto vm_acc = vm;

	auto new_environment = lexical_scope_t{ body._symbols };
	vm_acc._call_stack.push_back(make_shared<lexical_scope_t>(new_environment));
	const auto result = analyse_statements(vm_acc, body._statements);
	vm_acc = result.first;

	const auto body2 = body_t(result.second, result.first._call_stack.back()->_symbols);

	vm_acc._call_stack.pop_back();
	return { vm_acc, body2 };
}


//	Warning: returns reference to the found value-entry -- this could be in any environment in the call stack.
std::pair<symbol_t*, floyd::variable_address_t> resolve_env_variable_deep(const analyser_t& vm, int depth, const std::string& s){
	QUARK_ASSERT(vm.check_invariant());
	QUARK_ASSERT(depth >= 0 && depth < vm._call_stack.size());
	QUARK_ASSERT(s.size() > 0);

    const auto it = std::find_if(vm._call_stack[depth]->_symbols.begin(), vm._call_stack[depth]->_symbols.end(),  [&s](const std::pair<std::string, floyd::symbol_t>& e) { return e.first == s; });
	if(it != vm._call_stack[depth]->_symbols.end()){
		const auto parent_index = depth == 0 ? -1 : (int)(vm._call_stack.size() - depth - 1);
		const auto variable_index = (int)(it - vm._call_stack[depth]->_symbols.begin());
		return { &it->second, floyd::variable_address_t::make_variable_address(parent_index, variable_index) };
	}
	else if(depth > 0){
		return resolve_env_variable_deep(vm, depth - 1, s);
	}
	else{
		return { nullptr, floyd::variable_address_t::make_variable_address(0, 0) };
	}
}
//	Warning: returns reference to the found value-entry -- this could be in any environment in the call stack.
std::pair<symbol_t*, floyd::variable_address_t> find_symbol_by_name(const analyser_t& vm, const std::string& s){
	QUARK_ASSERT(vm.check_invariant());
	QUARK_ASSERT(s.size() > 0);

	return resolve_env_variable_deep(vm, static_cast<int>(vm._call_stack.size() - 1), s);
}

bool does_symbol_exist_shallow(const analyser_t& vm, const std::string& s){
    const auto it = std::find_if(vm._call_stack.back()->_symbols.begin(), vm._call_stack.back()->_symbols.end(),  [&s](const std::pair<std::string, floyd::symbol_t>& e) { return e.first == s; });
	return it != vm._call_stack.back()->_symbols.end();
}

//	Warning: returns reference to the found value-entry -- this could be in any environment in the call stack.
symbol_t* resolve_symbol_by_address(const analyser_t& vm, const floyd::variable_address_t& s){
	QUARK_ASSERT(vm.check_invariant());

	const auto env_index = s._parent_steps == -1 ? 0 : vm._call_stack.size() - s._parent_steps - 1;
	auto& env = vm._call_stack[env_index];
	return &env->_symbols[s._index].second;
}

symbol_t find_global_symbol(const analyser_t& vm, const string& s){
	const auto t = find_symbol_by_name(vm, s);
	if(t.first == nullptr){
		throw std::runtime_error("Undefined indentifier \"" + s + "\"!");
	}
	return *t.first;
}


//	Make sure there is no null or [null] or [string: null] etc. inside type.
bool check_type_fully_defined(const typeid_t& type){
	if(type.is_null()){
		return true;
	}
	else if(type.is_function()){
/*
		if(check_type_fully_defined(type.get_function_return()) == false){
			return false;
		}
*/
		for(const auto e: type.get_function_args()){
			if(check_type_fully_defined(e) == false){
				return false;
			}
		}
		return true;
	}
	else if(type.is_dict()){
		const auto value_type = type.get_dict_value_type();
		if(value_type.is_null()){
			return false;
		}
		else{
			return check_type_fully_defined(value_type);
		}
	}
	else if(type.is_vector()){
		const auto element_type = type.get_vector_element_type();
		if(element_type.is_null()){
			return false;
		}
		else{
			return check_type_fully_defined(element_type);
		}
	}
	else if(type.is_json_value()){
		return true;
	}
	else if(type.is_struct()){
		const auto def = type.get_struct();
		for(const auto e: def._members){
			if(check_type_fully_defined(e._type) == false){
				return false;
			}
		}
		return true;
	}
	else{
		return true;
	}
}



/////////////////////////////////////////			STATEMENTS



/*
	Simple assign

	- Can update an existing local (if local is mutable).
	- Can implicitly create a new local
*/
std::pair<analyser_t, statement_t> analyse_store_statement(const analyser_t& vm, const statement_t& s){
	QUARK_ASSERT(vm.check_invariant());

	auto vm_acc = vm;
	const auto statement = *s._store;
	const auto local_name = statement._local_name;
	const auto existing_value_deep_ptr = find_symbol_by_name(vm_acc, local_name);

	//	Attempt to mutate existing value!
	if(existing_value_deep_ptr.first != nullptr){
		if(existing_value_deep_ptr.first->_symbol_type != symbol_t::mutable_local){
			throw std::runtime_error("Cannot assign to immutable identifier.");
		}
		else{
			const auto lhs_type = existing_value_deep_ptr.first->get_type();
			QUARK_ASSERT(check_type_fully_defined(lhs_type));

			const auto rhs_expr2 = analyse_expression_to_target(vm_acc, statement._expression, lhs_type);
			vm_acc = rhs_expr2.first;
			const auto rhs_expr3 = rhs_expr2.second;

			//??? we don't have type when calling push_back().
			if(lhs_type != rhs_expr3.get_annotated_type()){
				throw std::runtime_error("Types not compatible in bind.");
			}
			else{
				return { vm_acc, statement_t::make__store2(existing_value_deep_ptr.second, rhs_expr3) };
			}
		}
	}

	//	Bind new value -- deduce type.
	else{
		const auto rhs_expr2 = analyse_expression_no_target(vm_acc, statement._expression);
		vm_acc = rhs_expr2.first;
		const auto rhs_expr2_type = rhs_expr2.second.get_annotated_type();

		vm_acc._call_stack.back()->_symbols.push_back({local_name, symbol_t::make_immutable_local(rhs_expr2_type)});
		int variable_index = (int)(vm_acc._call_stack.back()->_symbols.size() - 1);
		return { vm_acc, statement_t::make__store2(floyd::variable_address_t::make_variable_address(0, variable_index), rhs_expr2.second) };
	}
}

/*
Assign with target type info.
- Always creates a new local.

Ex:
	int a = 10
	mutable a = 10
	mutable = 10
*/
std::pair<analyser_t, statement_t> analyse_bind_local_statement(const analyser_t& vm, const statement_t& s){
	QUARK_ASSERT(vm.check_invariant());

	const auto statement = *s._bind_local;
	auto vm_acc = vm;

	const auto new_local_name = statement._new_local_name;
	const auto lhs_type = statement._bindtype;
	const auto bind_statement_mutable_tag_flag = statement._locals_mutable_mode == statement_t::bind_local_t::k_mutable;

	const auto value_exists_in_env = does_symbol_exist_shallow(vm_acc, new_local_name);
	if(value_exists_in_env){
		throw std::runtime_error("Local identifier already exists.");
	}

	//	Setup temporary simply so function definition can find itself = recursive.
	//	Notice: the final type may not be correct yet, but for function defintions it is.
	//	This logicl should be available for deduced binds too, in analyse_store_statement().
	vm_acc._call_stack.back()->_symbols.push_back({new_local_name, bind_statement_mutable_tag_flag ? symbol_t::make_mutable_local(lhs_type) : symbol_t::make_immutable_local(lhs_type)});
	const auto local_name_index = vm_acc._call_stack.back()->_symbols.size() - 1;

	try {
		const auto rhs_expr_pair = lhs_type.is_null()? analyse_expression_no_target(vm_acc, statement._expression) : analyse_expression_to_target(vm_acc, statement._expression, lhs_type);
		vm_acc = rhs_expr_pair.first;

//??? if expression is a k_define_struct, k_define_function -- make it a constant in symbol table and emit no store-statement!

		const auto rhs_type = rhs_expr_pair.second.get_annotated_type();
		const auto lhs_type2 = lhs_type.is_null() ? rhs_type : lhs_type;

		if(lhs_type2 != lhs_type2){
			throw std::runtime_error("Types not compatible in bind.");
		}
		else{
			//	Updated the symbol with the real function defintion.
			vm_acc._call_stack.back()->_symbols[local_name_index] = {new_local_name, bind_statement_mutable_tag_flag ? symbol_t::make_mutable_local(lhs_type2) : symbol_t::make_immutable_local(lhs_type2)};
			return {
				vm_acc,
				statement_t::make__store2(floyd::variable_address_t::make_variable_address(0, (int)local_name_index), rhs_expr_pair.second)
			};
		}
	}
	catch(...){

		//	Erase temporary symbol.
		vm_acc._call_stack.back()->_symbols.pop_back();

		throw;
	}
}

std::pair<analyser_t, statement_t> analyse_block_statement(const analyser_t& vm, const statement_t& s){
	QUARK_ASSERT(vm.check_invariant());

	const auto statement = *s._block;
	const auto e = analyse_body(vm, statement._body);
	return {e.first, statement_t::make__block_statement(e.second)};
}

std::pair<analyser_t, statement_t> analyse_return_statement(const analyser_t& vm, const statement_t& s){
	QUARK_ASSERT(vm.check_invariant());

	const auto statement = *s._return;
	auto vm_acc = vm;
	const auto expr = statement._expression;
	const auto result = analyse_expression_no_target(vm_acc, expr);
	vm_acc = result.first;

	const auto result_value = result.second;

	//	Check that return value's type matches function's return type. Cannot be done here since we don't know who called us.
	//	Instead calling code must check.
	return { vm_acc, statement_t::make__return_statement(result.second) };
}

typeid_t find_type_by_name(const analyser_t& vm, const typeid_t& type){
	if(type.get_base_type() == base_type::k_unresolved_type_identifier){
		const auto found = find_symbol_by_name(vm, type.get_unresolved_type_identifier());
		if(found.first != nullptr){
			if(found.first->_value_type.is_typeid()){
				return found.first->_const_value.get_typeid_value();
			}
			else{
				return typeid_t::make_null();
			}
		}
		else{
			return typeid_t::make_null();
		}
	}
	else{
		return type;
	}
}

analyser_t analyse_def_struct_statement(const analyser_t& vm, const statement_t::define_struct_statement_t& statement){
	QUARK_ASSERT(vm.check_invariant());

	auto vm_acc = vm;
	const auto struct_name = statement._name;
	if(does_symbol_exist_shallow(vm_acc, struct_name)){
		throw std::runtime_error("Name already used.");
	}

	//	Resolve member types in this scope.
	std::vector<member_t> members2;
	for(const auto e: statement._def->_members){
		const auto name = e._name;
		const auto type = e._type;
		const auto type2 = find_type_by_name(vm_acc, type);

		QUARK_ASSERT(type2.get_base_type() != base_type::k_unresolved_type_identifier);

		const auto e2 = member_t(type2, name);
		members2.push_back(e2);
	}

	const auto resolved_struct_def = std::make_shared<struct_definition_t>(struct_definition_t(members2));
	const auto struct_typeid = typeid_t::make_struct(resolved_struct_def);
	const auto struct_typeid_value = value_t::make_typeid_value(struct_typeid);
	vm_acc._call_stack.back()->_symbols.push_back({struct_name, symbol_t::make_constant(struct_typeid_value)});

	return vm_acc;
}

std::pair<analyser_t, statement_t> analyse_ifelse_statement(const analyser_t& vm, const statement_t::ifelse_statement_t& statement){
	QUARK_ASSERT(vm.check_invariant());

	auto vm_acc = vm;

	const auto condition2 = analyse_expression_no_target(vm_acc, statement._condition);
	vm_acc = condition2.first;

	const auto condition_type = condition2.second.get_annotated_type();
	if(condition_type.is_bool() == false){
		throw std::runtime_error("Boolean condition required.");
	}

	const auto then2 = analyse_body(vm_acc, statement._then_body);
	const auto else2 = analyse_body(vm_acc, statement._else_body);
	return { vm_acc, statement_t::make__ifelse_statement(condition2.second, then2.second, else2.second) };
}

std::pair<analyser_t, statement_t> analyse_for_statement(const analyser_t& vm, const statement_t::for_statement_t& statement){
	QUARK_ASSERT(vm.check_invariant());

	auto vm_acc = vm;

	const auto start_expr2 = analyse_expression_no_target(vm_acc, statement._start_expression);
	vm_acc = start_expr2.first;

	if(start_expr2.second.get_annotated_type().is_int() == false){
		throw std::runtime_error("For-loop requires integer iterator.");
	}

	const auto end_expr2 = analyse_expression_no_target(vm_acc, statement._end_expression);
	vm_acc = end_expr2.first;

	if(end_expr2.second.get_annotated_type().is_int() == false){
		throw std::runtime_error("For-loop requires integer iterator.");
	}

	const auto iterator_symbol = symbol_t::make_immutable_local(typeid_t::make_int());

	//	Add the iterator as a symbol to the body of the for-loop.
	auto symbols = statement._body._symbols;
	symbols.push_back({ statement._iterator_name, iterator_symbol});
	const auto body_injected = body_t(statement._body._statements, symbols);
	const auto result = analyse_body(vm_acc, body_injected);
	vm_acc = result.first;

	return { vm_acc, statement_t::make__for_statement(statement._iterator_name, start_expr2.second, end_expr2.second, result.second) };
}

std::pair<analyser_t, statement_t> analyse_while_statement(const analyser_t& vm, const statement_t::while_statement_t& statement){
	QUARK_ASSERT(vm.check_invariant());

	auto vm_acc = vm;

	const auto condition2_expr = analyse_expression_no_target(vm_acc, statement._condition);
	vm_acc = condition2_expr.first;

	const auto result = analyse_body(vm_acc, statement._body);
	vm_acc = result.first;

	return { vm_acc, statement_t::make__while_statement(condition2_expr.second, result.second) };
}

std::pair<analyser_t, statement_t> analyse_expression_statement(const analyser_t& vm, const statement_t::expression_statement_t& statement){
	QUARK_ASSERT(vm.check_invariant());

	auto vm_acc = vm;
	const auto expr2 = analyse_expression_no_target(vm_acc, statement._expression);
	vm_acc = expr2.first;

	return { vm_acc, statement_t::make__expression_statement(expr2.second) };
}

//	Output is the RETURN VALUE of the analysed statement, if any.
std::pair<analyser_t, std::shared_ptr<statement_t>> analyse_statement(const analyser_t& vm, const statement_t& statement){
	QUARK_ASSERT(vm.check_invariant());
	QUARK_ASSERT(statement.check_invariant());

	if(statement._bind_local){
		const auto e = analyse_bind_local_statement(vm, statement);
		QUARK_ASSERT(e.second.is_annotated_deep());
		return { e.first, std::make_shared<statement_t>(e.second) };
	}
	else if(statement._store){
		const auto e = analyse_store_statement(vm, statement);
		QUARK_ASSERT(e.second.is_annotated_deep());
		return { e.first, std::make_shared<statement_t>(e.second) };
	}
	else if(statement._block){
		const auto e = analyse_block_statement(vm, statement);
		QUARK_ASSERT(e.second.is_annotated_deep());
		return { e.first, std::make_shared<statement_t>(e.second) };
	}
	else if(statement._return){
		const auto e = analyse_return_statement(vm, statement);
		QUARK_ASSERT(e.second.is_annotated_deep());
		return { e.first, std::make_shared<statement_t>(e.second) };
	}
	else if(statement._def_struct){
		const auto e = analyse_def_struct_statement(vm, *statement._def_struct);
		return { e, {} };
	}
	else if(statement._if){
		const auto e = analyse_ifelse_statement(vm, *statement._if);
		QUARK_ASSERT(e.second.is_annotated_deep());
		return { e.first, std::make_shared<statement_t>(e.second) };
	}
	else if(statement._for){
		const auto e = analyse_for_statement(vm, *statement._for);
		QUARK_ASSERT(e.second.is_annotated_deep());
		return { e.first, std::make_shared<statement_t>(e.second) };
	}
	else if(statement._while){
		const auto e = analyse_while_statement(vm, *statement._while);
		QUARK_ASSERT(e.second.is_annotated_deep());
		return { e.first, std::make_shared<statement_t>(e.second) };
	}
	else if(statement._expression){
		const auto e = analyse_expression_statement(vm, *statement._expression);
		QUARK_ASSERT(e.second.is_annotated_deep());
		return { e.first, std::make_shared<statement_t>(e.second) };
	}
	else{
		QUARK_ASSERT(false);
		throw std::exception();
	}
}




/////////////////////////////////////////			EXPRESSIONS




//?? check all types of expressions. Time for visitor-pattern?
bool check_type_fully_defined(const expression_t& e){
	if(check_type_fully_defined(e.get_annotated_type()) == false){
		return false;
	}
	else{
		const auto op = e.get_operation();
		return true;
	}
}


/*
	If e doesn't have a fully-formed type, attempt to give it one.
*/
expression_t improve_expression_type(const expression_t& e, const floyd::typeid_t& wanted_type){
	QUARK_ASSERT(e.check_invariant());
	QUARK_ASSERT(wanted_type.check_invariant());

	const auto e_type = e.get_annotated_type();

	if(wanted_type.is_null()){
		return e;
	}
	else if(wanted_type.is_string()){
		if(e_type.is_json_value()){
			return expression_t::make_construct_value_expr(wanted_type, { e });
		}
		else{
			return e;
		}
	}
	else if(wanted_type.is_json_value()){
		if(e_type.is_null() || e_type.is_int() || e_type.is_float() || e_type.is_string() || e_type.is_bool() || e_type.is_dict() || e_type.is_vector()){
			return expression_t::make_construct_value_expr(wanted_type, { e });
		}
		else{
			return e;
		}
	}
	else if(wanted_type.is_vector()){
		if(e.get_operation() == expression_type::k_literal && e.get_literal() == value_t::make_vector_value(typeid_t::make_null(), {})){
			return expression_t::make_literal(value_t::make_vector_value(wanted_type.get_vector_element_type(), {}));
		}
		else if(e_type.is_vector() && e_type.get_vector_element_type().is_null()  && e.get_operation() == expression_type::k_construct_value){
			QUARK_ASSERT(false);
			return expression_t::make_construct_value_expr(typeid_t::make_vector(wanted_type), e.get_construct_value()->_args);
		}
		else{
			return e;
		}
	}
	else if(wanted_type.is_dict()){
		if(e.get_operation() == expression_type::k_literal && e.get_literal() == value_t::make_dict_value(typeid_t::make_null(), {})){
			return expression_t::make_literal(value_t::make_dict_value(wanted_type.get_dict_value_type(), {}));
		}
		else{
			return e;
		}
	}
	else{
		return e;
	}
}




std::pair<analyser_t, expression_t> analyse_resolve_member_expression(const analyser_t& vm, const expression_t::resolve_member_expr_t& expr){
	QUARK_ASSERT(vm.check_invariant());

	auto vm_acc = vm;
	const auto parent_expr = analyse_expression_no_target(vm_acc, *expr._parent_address);
	vm_acc = parent_expr.first;

	const auto parent_type = parent_expr.second.get_annotated_type();

	if(parent_type.is_struct()){
		const auto struct_def = parent_type.get_struct();

		int index = find_struct_member_index(struct_def, expr._member_name);
		if(index == -1){
			throw std::runtime_error("Unknown struct member \"" + expr._member_name + "\".");
		}
		const auto member_type = struct_def._members[index]._type;
		return { vm_acc, expression_t::make_resolve_member(parent_expr.second, expr._member_name, make_shared<typeid_t>(member_type))};
	}
	else{
		throw std::runtime_error("Resolve struct member failed.");
	}
}

std::pair<analyser_t, expression_t> analyse_lookup_element_expression(const analyser_t& vm, const expression_t::lookup_expr_t& expr){
	QUARK_ASSERT(vm.check_invariant());

	auto vm_acc = vm;
	const auto parent_expr = analyse_expression_no_target(vm_acc, *expr._parent_address);
	vm_acc = parent_expr.first;

	const auto key_expr = analyse_expression_no_target(vm_acc, *expr._lookup_key);
	vm_acc = key_expr.first;

	const auto parent_type = parent_expr.second.get_annotated_type();
	const auto key_type = key_expr.second.get_annotated_type();

	if(parent_type.is_string()){
		if(key_type.is_int() == false){
			throw std::runtime_error("Lookup in string by index-only.");
		}
		else{
		}
		return { vm_acc, expression_t::make_lookup(parent_expr.second, key_expr.second, make_shared<typeid_t>(typeid_t::make_string())) };
	}
	else if(parent_type.is_json_value()){
		return { vm_acc, expression_t::make_lookup(parent_expr.second, key_expr.second, make_shared<typeid_t>(typeid_t::make_json_value())) };
	}
	else if(parent_type.is_vector()){
		if(key_type.is_int() == false){
			throw std::runtime_error("Lookup in vector by index-only.");
		}
		else{
		}
		//??? maybe use annotated type here!??
		return { vm_acc, expression_t::make_lookup(parent_expr.second, key_expr.second, make_shared<typeid_t>(parent_type.get_vector_element_type())) };
	}
	else if(parent_type.is_dict()){
		if(key_type.is_string() == false){
			throw std::runtime_error("Lookup in dict by string-only.");
		}
		else{
		}
		//??? maybe use annotated type here!??
		return { vm_acc, expression_t::make_lookup(parent_expr.second, key_expr.second, make_shared<typeid_t>(parent_type.get_dict_value_type())) };
	}
	else {
		throw std::runtime_error("Lookup using [] only works with strings, vectors, dicts and json_value.");
	}
}

std::pair<analyser_t, expression_t> analyse_load(const analyser_t& vm, const expression_t& e){
	QUARK_ASSERT(vm.check_invariant());

	const auto expr = *e.get_load();

	auto vm_acc = vm;
	const auto found = find_symbol_by_name(vm_acc, expr._variable);
	if(found.first != nullptr){
//		return {vm_acc, expression_t::make_variable_expression(expr._variable, make_shared<typeid_t>(found.first->_value_type)) };
		return {vm_acc, expression_t::make_load2(found.second, make_shared<typeid_t>(found.first->_value_type)) };
	}
	else{
		throw std::runtime_error("Undefined variable \"" + expr._variable + "\".");
	}
}

std::pair<analyser_t, expression_t> analyse_load2(const analyser_t& vm, const expression_t& e){
	QUARK_ASSERT(vm.check_invariant());

	const auto expr = *e.get_load();

	auto vm_acc = vm;
	const auto found = find_symbol_by_name(vm_acc, expr._variable);
	if(found.first != nullptr){
		return {vm_acc, expression_t::make_variable_expression(expr._variable, make_shared<typeid_t>(found.first->_value_type)) };
	}
	else{
		throw std::runtime_error("Undefined variable \"" + expr._variable + "\".");
	}
}

std::pair<analyser_t, expression_t> analyse_construct_value_expression(const analyser_t& vm, const expression_t& e){
	QUARK_ASSERT(vm.check_invariant());

	auto vm_acc = vm;
	const auto expr = *e.get_construct_value();

	if(expr._value_type2.is_vector()){
		const std::vector<expression_t>& elements = expr._args;
		QUARK_ASSERT(expr._value_type2.is_vector());
		const auto element_type = expr._value_type2.get_vector_element_type();

		if(elements.empty()){
			return {vm_acc, expression_t::make_literal(value_t::make_vector_value(element_type, {}))};
		}
		else{
			std::vector<expression_t> elements2;
			for(const auto m: elements){
				const auto element_expr = analyse_expression_no_target(vm_acc, m);
				vm_acc = element_expr.first;
				elements2.push_back(element_expr.second);
			}

			//	If we don't have an explicit element type, deduct it from first element.
			const auto element_type2 = element_type.is_null() ? elements2[0].get_annotated_type(): element_type;
			for(const auto m: elements2){
				if(m.get_annotated_type() != element_type2){
					throw std::runtime_error("Vector can not hold elements of different types.");
				}
			}
			return { vm_acc, expression_t::make_construct_value_expr(typeid_t::make_vector(element_type2), elements2) };
		}
	}

	//	Dicts uses pairs of (string,value). This is stored in _args as interleaved expression: string0, value0, string1, value1.
	else if(expr._value_type2.is_dict()){
		const auto& elements = expr._args;
		QUARK_ASSERT(elements.size() % 2 == 0);

		const auto element_type = expr._value_type2.get_dict_value_type();

		if(elements.empty()){
			return {vm_acc, expression_t::make_literal(value_t::make_dict_value(element_type, {}))};
		}
		else{
			std::vector<expression_t> elements2;
			for(int i = 0 ; i < elements.size() / 2 ; i++){
				const auto& key = elements[i * 2 + 0].get_literal().get_string_value();
				const auto& value = elements[i * 2 + 1];
				const auto element_expr = analyse_expression_no_target(vm_acc, value);
				vm_acc = element_expr.first;
				elements2.push_back(expression_t::make_literal_string(key));
				elements2.push_back(element_expr.second);
			}

			//	Deduce type of dictionary based on first value.
			const auto element_type2 = element_type.is_null() == false ? element_type : elements2[0 * 2 + 1].get_annotated_type();

			//	Make sure all elements have the correct type.
			for(int i = 0 ; i < elements2.size() / 2 ; i++){
				const auto element_type0 = elements2[i * 2 + 1].get_annotated_type();
				if(element_type0 != element_type2){

				//??? Make we make a json_value::object if we find mixed values in dict?
				//??? dict-def needs to hold mixed values to allow json_value to type object to work.
				//???				throw std::runtime_error("Dict can not hold elements of different type!");
				}
			}
			return {vm_acc, expression_t::make_construct_value_expr(typeid_t::make_dict(element_type2), elements2)};
		}
	}
	else{
		QUARK_ASSERT(false);
	}
}


std::pair<analyser_t, expression_t> analyse_arithmetic_unary_minus_expression(const analyser_t& vm, const expression_t::unary_minus_expr_t& expr){
	QUARK_ASSERT(vm.check_invariant());

	auto vm_acc = vm;
	const auto& expr2 = analyse_expression_no_target(vm_acc, *expr._expr);
	vm_acc = expr2.first;

	//??? We could simplify here and return [ "-", 0, expr]
	const auto type = expr2.second.get_annotated_type();
	if(type.is_int() || type.is_float()){
		return {vm_acc, expression_t::make_unary_minus(expr2.second, make_shared<typeid_t>(type))  };
	}
	else{
		throw std::runtime_error("Unary minus won't work on expressions of type \"" + json_to_compact_string(typeid_to_ast_json(type)._value) + "\".");
	}
}

std::pair<analyser_t, expression_t> analyse_conditional_operator_expression(const analyser_t& vm, const expression_t::conditional_expr_t& expr){
	QUARK_ASSERT(vm.check_invariant());

	auto vm_acc = vm;

	//	Special-case since it uses 3 expressions & uses shortcut evaluation.
	const auto cond_result = analyse_expression_no_target(vm_acc, *expr._condition);
	vm_acc = cond_result.first;

	const auto a = analyse_expression_no_target(vm_acc, *expr._a);
	vm_acc = a.first;

	const auto b = analyse_expression_no_target(vm_acc, *expr._b);
	vm_acc = b.first;

	const auto type = cond_result.second.get_annotated_type();
	if(type.is_bool() == false){
		throw std::runtime_error("Could not analyse condition in conditional expression.");
	}
	else if(a.second.get_annotated_type() != b.second.get_annotated_type()){
		throw std::runtime_error("Conditional expression requires both true/false conditions to have the same type.");
	}
	else{
		const auto final_expression_type = a.second.get_annotated_type();
		return {vm_acc, expression_t::make_conditional_operator(cond_result.second, a.second, b.second, make_shared<typeid_t>(final_expression_type))  };
	}
}

std::pair<analyser_t, expression_t> analyse_comparison_expression(const analyser_t& vm, expression_type op, const expression_t& e){
	QUARK_ASSERT(vm.check_invariant());

	const auto simple2_expr = *e.get_simple__2();
	auto vm_acc = vm;

	//	First analyse all inputs to our operation.
	const auto left_expr = analyse_expression_no_target(vm_acc, *simple2_expr._left);
	vm_acc = left_expr.first;

	const auto lhs_type = left_expr.second.get_annotated_type();

	//	Make rhs match left if needed/possible.
	const auto right_expr = analyse_expression_to_target(vm_acc, *simple2_expr._right, lhs_type);
	vm_acc = right_expr.first;
	const auto rhs_type = right_expr.second.get_annotated_type();

	// ??? we don't have all types yet.
	if(lhs_type != rhs_type && lhs_type.is_null() == false && rhs_type.is_null() == false){
		throw std::runtime_error("Comparison: Left and right expressions must be same type!");
	}
	else{
		const auto shared_type = lhs_type;

		if(op == expression_type::k_comparison_smaller_or_equal__2){
		}
		else if(op == expression_type::k_comparison_smaller__2){
		}
		else if(op == expression_type::k_comparison_larger_or_equal__2){
		}
		else if(op == expression_type::k_comparison_larger__2){
		}

		else if(op == expression_type::k_logical_equal__2){
		}
		else if(op == expression_type::k_logical_nonequal__2){
		}
		else{
			throw std::exception();
		}
		return {vm_acc, expression_t::make_simple_expression__2(e.get_operation(), left_expr.second, right_expr.second, make_shared<typeid_t>(typeid_t::make_bool())) };
	}
}

std::pair<analyser_t, expression_t> analyse_arithmetic_expression(const analyser_t& vm, expression_type op, const expression_t& e){
	QUARK_ASSERT(vm.check_invariant());

	const auto simple2_expr = *e.get_simple__2();
	auto vm_acc = vm;

	//	First analyse both inputs to our operation.
	const auto left_expr = analyse_expression_no_target(vm_acc, *simple2_expr._left);
	vm_acc = left_expr.first;

	const auto lhs_type = left_expr.second.get_annotated_type();

	//	Make rhs match lhs if needed/possible.
	const auto right_expr = analyse_expression_to_target(vm_acc, *simple2_expr._right, lhs_type);
	vm_acc = right_expr.first;

	const auto rhs_type = right_expr.second.get_annotated_type();


	if(lhs_type != rhs_type){
		throw std::runtime_error("Artithmetics: Left and right expressions must be same type!");
	}
	else{
		const auto shared_type = lhs_type;

		//	bool
		if(shared_type.is_bool()){
			if(op == expression_type::k_arithmetic_add__2){
			}
			else if(op == expression_type::k_arithmetic_subtract__2){
				throw std::runtime_error("Operation not allowed on bool.");
			}
			else if(op == expression_type::k_arithmetic_multiply__2){
				throw std::runtime_error("Operation not allowed on bool.");
			}
			else if(op == expression_type::k_arithmetic_divide__2){
				throw std::runtime_error("Operation not allowed on bool.");
			}
			else if(op == expression_type::k_arithmetic_remainder__2){
				throw std::runtime_error("Operation not allowed on bool.");
			}

			else if(op == expression_type::k_logical_and__2){
			}
			else if(op == expression_type::k_logical_or__2){
			}
			else{
				QUARK_ASSERT(false);
				throw std::exception();
			}

			return {vm_acc, expression_t::make_simple_expression__2(e.get_operation(), left_expr.second, right_expr.second, make_shared<typeid_t>(shared_type)) };
		}

		//	int
		else if(shared_type.is_int()){
			if(op == expression_type::k_arithmetic_add__2){
			}
			else if(op == expression_type::k_arithmetic_subtract__2){
			}
			else if(op == expression_type::k_arithmetic_multiply__2){
			}
			else if(op == expression_type::k_arithmetic_divide__2){
			}
			else if(op == expression_type::k_arithmetic_remainder__2){
			}

			else if(op == expression_type::k_logical_and__2){
			}
			else if(op == expression_type::k_logical_or__2){
			}
			else{
				QUARK_ASSERT(false);
				throw std::exception();
			}

			return {vm_acc, expression_t::make_simple_expression__2(e.get_operation(), left_expr.second, right_expr.second, make_shared<typeid_t>(shared_type)) };
		}

		//	float
		else if(shared_type.is_float()){
			if(op == expression_type::k_arithmetic_add__2){
			}
			else if(op == expression_type::k_arithmetic_subtract__2){
			}
			else if(op == expression_type::k_arithmetic_multiply__2){
			}
			else if(op == expression_type::k_arithmetic_divide__2){
			}
			else if(op == expression_type::k_arithmetic_remainder__2){
				throw std::runtime_error("Modulo operation on float not allowed.");
			}

			else if(op == expression_type::k_logical_and__2){
			}
			else if(op == expression_type::k_logical_or__2){
			}
			else{
				QUARK_ASSERT(false);
				throw std::exception();
			}

			return {vm_acc, expression_t::make_simple_expression__2(e.get_operation(), left_expr.second, right_expr.second, make_shared<typeid_t>(shared_type)) };
		}

		//	string
		else if(shared_type.is_string()){
			if(op == expression_type::k_arithmetic_add__2){
			}

			else if(op == expression_type::k_arithmetic_subtract__2){
				throw std::runtime_error("Operation not allowed on string.");
			}
			else if(op == expression_type::k_arithmetic_multiply__2){
				throw std::runtime_error("Operation not allowed on string.");
			}
			else if(op == expression_type::k_arithmetic_divide__2){
				throw std::runtime_error("Operation not allowed on string.");
			}
			else if(op == expression_type::k_arithmetic_remainder__2){
				throw std::runtime_error("Operation not allowed on string.");
			}

			else if(op == expression_type::k_logical_and__2){
				throw std::runtime_error("Operation not allowed on string.");
			}
			else if(op == expression_type::k_logical_or__2){
				throw std::runtime_error("Operation not allowed on string.");
			}
			else{
				QUARK_ASSERT(false);
				throw std::exception();
			}

			return {vm_acc, expression_t::make_simple_expression__2(e.get_operation(), left_expr.second, right_expr.second, make_shared<typeid_t>(shared_type)) };
		}

		//	struct
		else if(shared_type.is_struct()){
			//	Structs must be exactly the same type to match.

			if(op == expression_type::k_arithmetic_add__2){
				throw std::runtime_error("Operation not allowed on struct.");
			}
			else if(op == expression_type::k_arithmetic_subtract__2){
				throw std::runtime_error("Operation not allowed on struct.");
			}
			else if(op == expression_type::k_arithmetic_multiply__2){
				throw std::runtime_error("Operation not allowed on struct.");
			}
			else if(op == expression_type::k_arithmetic_divide__2){
				throw std::runtime_error("Operation not allowed on struct.");
			}
			else if(op == expression_type::k_arithmetic_remainder__2){
				throw std::runtime_error("Operation not allowed on struct.");
			}

			else if(op == expression_type::k_logical_and__2){
				throw std::runtime_error("Operation not allowed on struct.");
			}
			else if(op == expression_type::k_logical_or__2){
				throw std::runtime_error("Operation not allowed on struct.");
			}
			else{
				QUARK_ASSERT(false);
				throw std::exception();
			}

			return {vm_acc, expression_t::make_simple_expression__2(e.get_operation(), left_expr.second, right_expr.second, make_shared<typeid_t>(shared_type)) };
		}

		else if(shared_type.is_vector()){
			const auto element_type = shared_type.get_vector_element_type();
			if(op == expression_type::k_arithmetic_add__2){
			}

			else if(op == expression_type::k_arithmetic_subtract__2){
				throw std::runtime_error("Operation not allowed on vectors.");
			}
			else if(op == expression_type::k_arithmetic_multiply__2){
				throw std::runtime_error("Operation not allowed on vectors.");
			}
			else if(op == expression_type::k_arithmetic_divide__2){
				throw std::runtime_error("Operation not allowed on vectors.");
			}
			else if(op == expression_type::k_arithmetic_remainder__2){
				throw std::runtime_error("Operation not allowed on vectors.");
			}


			else if(op == expression_type::k_logical_and__2){
				throw std::runtime_error("Operation not allowed on vectors.");
			}
			else if(op == expression_type::k_logical_or__2){
				throw std::runtime_error("Operation not allowed on vectors.");
			}
			else{
				QUARK_ASSERT(false);
				throw std::exception();
			}
			return {vm_acc, expression_t::make_simple_expression__2(e.get_operation(), left_expr.second, right_expr.second, make_shared<typeid_t>(shared_type)) };
		}
		else if(shared_type.is_function()){
			throw std::runtime_error("Cannot perform operations on two function values.");
		}
		else{
			throw std::runtime_error("Arithmetics failed.");
		}
	}
}

/*
	Magic: a function taking ONE argument of type NULL: variable arguments count of any type. Like c-lang (...).
*/
std::pair<analyser_t, vector<expression_t>> analyze_call_args(const analyser_t& vm, const vector<expression_t>& call_args, const std::vector<typeid_t>& callee_args){

	if(callee_args.size() == 1 && callee_args[0].is_null()){
		auto vm_acc = vm;
		vector<expression_t> call_args2;
		for(int i = 0 ; i < call_args.size() ; i++){
			const auto call_arg_pair = analyse_expression_no_target(vm_acc, call_args[i]);
			vm_acc = call_arg_pair.first;
			call_args2.push_back(call_arg_pair.second);
		}
		return { vm_acc, call_args2 };
	}
	else{
		//	arity
		if(call_args.size() != callee_args.size()){
			throw std::runtime_error("Wrong number of arguments in function call.");
		}

		auto vm_acc = vm;
		vector<expression_t> call_args2;
		for(int i = 0 ; i < callee_args.size() ; i++){
			const auto callee_arg = callee_args[i];

			const auto call_arg_pair = analyse_expression_to_target(vm_acc, call_args[i], callee_arg);
			vm_acc = call_arg_pair.first;
			call_args2.push_back(call_arg_pair.second);
		}
		return { vm_acc, call_args2 };
	}
}

bool is_host_function_call(const analyser_t& vm, const expression_t& callee_expr){
	if(callee_expr.get_operation() == expression_type::k_load){
		const auto function_name = callee_expr.get_load()->_variable;

		const auto find_it = vm._imm->_host_functions.find(function_name);
		return find_it != vm._imm->_host_functions.end();
	}
	else if(callee_expr.get_operation() == expression_type::k_load2){
		const auto callee = resolve_symbol_by_address(vm, callee_expr.get_load2()->_address);
		QUARK_ASSERT(callee != nullptr);

		if(callee->_const_value.is_function()){
			return callee->_const_value.get_function_value()->_def._host_function != 0;
		}
		else{
			return false;
		}
	}
	else{
		return false;
	}
}

typeid_t get_host_function_return_type(const analyser_t& vm, const expression_t& callee_expr, const vector<expression_t>& args){
	QUARK_ASSERT(is_host_function_call(vm, callee_expr));
	QUARK_ASSERT(callee_expr.get_operation() == expression_type::k_load2);

	const auto callee = resolve_symbol_by_address(vm, callee_expr.get_load2()->_address);
	QUARK_ASSERT(callee != nullptr);
	const auto host_function_id = callee->_const_value.get_function_value()->_def._host_function;

	const auto& host_functions = vm._imm->_host_functions;
	const auto found_it = find_if(host_functions.begin(), host_functions.end(), [&](const std::pair<std::string, floyd::host_function_signature_t>& kv){ return kv.second._function_id == host_function_id; });
	QUARK_ASSERT(found_it != host_functions.end());

	const auto function_name = found_it->first;

	if(function_name == "update"){
		return args[0].get_annotated_type();
	}
	else if(function_name == "erase"){
		return args[0].get_annotated_type();
	}
	else if(function_name == "push_back"){
		return args[0].get_annotated_type();
	}
	else if(function_name == "subset"){
		return args[0].get_annotated_type();
	}
	else if(function_name == "replace"){
		return args[0].get_annotated_type();
	}
	else if(function_name == "instantiate_from_typeid"){
		if(args[0].get_operation() == expression_type::k_load2){
			const auto symbol = resolve_symbol_by_address(vm, args[0].get_load2()->_address);
			if(symbol != nullptr && symbol->_const_value.is_null() == false){
				return symbol->_const_value.get_typeid_value();
			}
			else{
				throw std::runtime_error("Cannot resolve type for instantiate_from_typeid().");
			}
		}
		else{
			throw std::runtime_error("Cannot resolve type for instantiate_from_typeid().");
		}
	}
/*
		else if(function_name == "unflatten_from_json"){
			return args[0].get_annotated_type();
		}
*/
	else{
		return callee_expr.get_annotated_type().get_function_return();
	}
}

/*
	callee(callee_args)		== function def: int(
	a = my_func(x, 13, "cat");
	a = json_value(1 + 3);
*/
std::pair<analyser_t, expression_t> analyse_call_expression(const analyser_t& vm, const expression_t& e){
	QUARK_ASSERT(vm.check_invariant());

	const auto expr = *e.get_call();
	auto vm_acc = vm;

	const auto callee_expr0 = analyse_expression_no_target(vm_acc, *expr._callee);
	vm_acc = callee_expr0.first;
	const auto callee_expr = callee_expr0.second;

	//	This is a call to a function-value. Callee is a function-type.
	const auto callee_type = callee_expr.get_annotated_type();
	if(callee_type.is_function()){
		const auto callee_args = callee_type.get_function_args();
		const auto callee_return_value = callee_type.get_function_return();
		const auto call_args_pair = analyze_call_args(vm_acc, expr._args, callee_args);
		vm_acc = call_args_pair.first;
		if(is_host_function_call(vm, callee_expr)){
			const auto return_type = get_host_function_return_type(vm, callee_expr, call_args_pair.second);
			return { vm_acc, expression_t::make_call(callee_expr, call_args_pair.second, make_shared<typeid_t>(return_type)) };
		}
		else{
			return { vm_acc, expression_t::make_call(callee_expr, call_args_pair.second, make_shared<typeid_t>(callee_return_value)) };
		}
	}

	//	Attempting to call a TYPE? Then this may be a constructor call.
	//	Converts these calls to construct-value-expressions.
	else if(callee_type.is_typeid() && callee_expr.get_operation() == expression_type::k_load2){
		const auto found_symbol_ptr = resolve_symbol_by_address(vm_acc, callee_expr.get_load2()->_address);
		QUARK_ASSERT(found_symbol_ptr != nullptr);

		if(found_symbol_ptr->_const_value.is_null()){
			throw std::runtime_error("Cannot resolve callee.");
		}
		else{
			const auto callee_type2 = found_symbol_ptr->_const_value.get_typeid_value();

			//	Convert calls to struct-type into construct-value expression.
			if(callee_type2.is_struct()){
				const auto& def = callee_type2.get_struct();
				const auto callee_args = get_member_types(def._members);
				const auto call_args_pair = analyze_call_args(vm_acc, expr._args, callee_args);
				vm_acc = call_args_pair.first;

				return { vm_acc, expression_t::make_construct_value_expr(callee_type2, call_args_pair.second) };
			}

			//	One argument for primitive types.
			else{
				const auto callee_args = vector<typeid_t>{ callee_type2 };
				const auto call_args_pair = analyze_call_args(vm_acc, expr._args, callee_args);
				vm_acc = call_args_pair.first;
				return { vm_acc, expression_t::make_construct_value_expr(callee_type2, call_args_pair.second) };
			}
		}
	}
	else{
		throw std::runtime_error("Cannot call non-function.");
	}
}

std::pair<analyser_t, expression_t> analyse_struct_definition_expression(const analyser_t& vm, const expression_t& e0){
	QUARK_ASSERT(vm.check_invariant());

	auto vm_acc = vm;
	const auto expr = *e0.get_struct_def();

	//	Resolve member types in this scope.
	std::vector<member_t> members2;
	for(const auto e: expr._def->_members){
		const auto name = e._name;
		const auto type = e._type;
		const auto type2 = find_type_by_name(vm_acc, type);

		QUARK_ASSERT(type2.get_base_type() != base_type::k_unresolved_type_identifier);

		const auto e2 = member_t(type2, name);
		members2.push_back(e2);
	}
	const auto resolved_struct_def = std::make_shared<struct_definition_t>(struct_definition_t(members2));
	return {vm_acc, expression_t::make_struct_definition(resolved_struct_def) };
}

std::pair<analyser_t, expression_t> analyse_function_definition_expression(const analyser_t& vm, const expression_t& e){
	QUARK_ASSERT(vm.check_invariant());

	const auto function_def_expr = *e.get_function_definition();
	const auto def = function_def_expr._def;
	const auto function_type = get_function_type(def);

	//	Make function body with arguments injected as local symbols.
	auto symbol_vec = def._body->_symbols;
	for(const auto x: def._args){
		symbol_vec.push_back({x._name , symbol_t::make_immutable_local(x._type)});
	}
	const auto injected_body = body_t(def._body->_statements, symbol_vec);

	const auto function_body_pair = analyse_body(vm, injected_body);
	auto vm_acc = function_body_pair.first;

	const auto body = function_body_pair.second;
	const auto def2 = function_definition_t(def._args, make_shared<body_t>(body), def._return_type);
	return {vm_acc, expression_t::make_function_definition(def2) };
}



std::pair<analyser_t, expression_t> analyse_expression__op_specific(const analyser_t& vm, const expression_t& e){
	QUARK_ASSERT(vm.check_invariant());
	QUARK_ASSERT(e.check_invariant());

	const auto op = e.get_operation();

	if(op == expression_type::k_literal){
		return { vm, e };
	}
	else if(op == expression_type::k_resolve_member){
		return analyse_resolve_member_expression(vm, *e.get_resolve_member());
	}
	else if(op == expression_type::k_lookup_element){
		return analyse_lookup_element_expression(vm, *e.get_lookup());
	}
	else if(op == expression_type::k_load){
		return analyse_load(vm, e);
	}
	else if(op == expression_type::k_load2){
		return analyse_load2(vm, e);
	}

	else if(op == expression_type::k_call){
		return analyse_call_expression(vm, e);
	}

	else if(op == expression_type::k_define_struct){
		return analyse_struct_definition_expression(vm, e);
	}

	else if(op == expression_type::k_define_function){
		return analyse_function_definition_expression(vm, e);
	}

	else if(op == expression_type::k_construct_value){
		return analyse_construct_value_expression(vm, e);
	}

	//	This can be desugared at compile time. ???
	else if(op == expression_type::k_arithmetic_unary_minus__1){
		return analyse_arithmetic_unary_minus_expression(vm, *e.get_unary_minus());
	}

	//	Special-case since it uses 3 expressions & uses shortcut evaluation.
	else if(op == expression_type::k_conditional_operator3){
		return analyse_conditional_operator_expression(vm, *e.get_conditional());
	}
	else if (false
		|| op == expression_type::k_comparison_smaller_or_equal__2
		|| op == expression_type::k_comparison_smaller__2
		|| op == expression_type::k_comparison_larger_or_equal__2
		|| op == expression_type::k_comparison_larger__2

		|| op == expression_type::k_logical_equal__2
		|| op == expression_type::k_logical_nonequal__2
	){
		return analyse_comparison_expression(vm, op, e);
	}
	else if (false
		|| op == expression_type::k_arithmetic_add__2
		|| op == expression_type::k_arithmetic_subtract__2
		|| op == expression_type::k_arithmetic_multiply__2
		|| op == expression_type::k_arithmetic_divide__2
		|| op == expression_type::k_arithmetic_remainder__2

		|| op == expression_type::k_logical_and__2
		|| op == expression_type::k_logical_or__2
	){
		return analyse_arithmetic_expression(vm, op, e);
	}
	else{
		QUARK_ASSERT(false);
		throw std::exception();
	}
}

std::pair<analyser_t, expression_t> analyse_expression_to_target(const analyser_t& vm, const expression_t& e, const typeid_t& target_type){
	QUARK_ASSERT(vm.check_invariant());
	QUARK_ASSERT(e.check_invariant());

	auto vm_acc = vm;
	const auto e1_pair = analyse_expression__op_specific(vm_acc, e);
	vm_acc = e1_pair.first;

	const auto e3 = improve_expression_type(e1_pair.second, target_type);

	if(e3.get_annotated_type() == target_type){
		if(check_type_fully_defined(e3) == false){
			throw std::runtime_error("Cannot resolve type.");
		}
	}
	else if(target_type.is_null()){
		if(check_type_fully_defined(e3) == false){
			throw std::runtime_error("Cannot resolve type.");
		}
	}
	else if(e3.get_annotated_type().is_null()){
		throw std::runtime_error("Expression type mismatch.");
	}
	else{
		throw std::runtime_error("Expression type mismatch.");
	}

	return { vm_acc, e3 };
}

std::pair<analyser_t, expression_t> analyse_expression_no_target(const analyser_t& vm, const expression_t& e){
	QUARK_ASSERT(vm.check_invariant());
	QUARK_ASSERT(e.check_invariant());

	QUARK_ASSERT(vm.check_invariant());
	QUARK_ASSERT(e.check_invariant());

	auto vm_acc = vm;
	const auto e1_pair = analyse_expression__op_specific(vm_acc, e);
	vm_acc = e1_pair.first;

	QUARK_ASSERT(e1_pair.second.get_annotated_type2() != nullptr);
	const auto e2 = e1_pair.second;

	if(check_type_fully_defined(e2) == false){
		throw std::runtime_error("Cannot resolve type.");
	}
	return { vm_acc, e2 };
}


json_t analyser_to_json(const analyser_t& vm){
	vector<json_t> callstack;
	for(const auto& e: vm._call_stack){
		std::map<string, json_t> values;
		for(const auto& symbol_kv: e->_symbols){
			const auto b = json_t::make_array({
				json_t((int)symbol_kv.second._symbol_type),
				typeid_to_ast_json(symbol_kv.second._value_type)._value,
				value_to_ast_json(symbol_kv.second._const_value)._value
			});
			values[symbol_kv.first] = b;
		}

		const auto& env = json_t::make_object({
//			{ "parent_env", e->_parent_env ? e->_parent_env->_object_id : json_t() },
//			{ "object_id", json_t(double(e->_object_id)) },
			{ "values", values }
		});
		callstack.push_back(env);
	}

	return json_t::make_object({
		{ "ast", ast_to_json(vm._imm->_ast)._value },
		{ "host_functions", json_t("dummy1234") },
		{ "callstack", json_t::make_array(callstack) }
	});
}



void test__analyse_expression(const expression_t& e, const expression_t& expected){
	const ast_t ast;
	const analyser_t interpreter(ast);
	const auto e3 = analyse_expression_no_target(interpreter, e);

	ut_compare_jsons(expression_to_json(e3.second)._value, expression_to_json(expected)._value);
}


QUARK_UNIT_TEST("analyse_expression_no_target()", "literal 1234 == 1234", "", "") {
	test__analyse_expression(
		expression_t::make_literal_int(1234),
		expression_t::make_literal_int(1234)
	);
}

QUARK_UNIT_TESTQ("analyse_expression_no_target()", "1 + 2 == 3") {
	const ast_t ast;
	const analyser_t interpreter(ast);
	const auto e3 = analyse_expression_no_target(interpreter,
		expression_t::make_simple_expression__2(
			expression_type::k_arithmetic_add__2,
			expression_t::make_literal_int(1),
			expression_t::make_literal_int(2),
			nullptr
		)
	);

	ut_compare_jsons(
		expression_to_json(e3.second)._value,
		parse_json(seq_t(R"(   ["+", ["k", 1, "int"], ["k", 2, "int"], "int"]   )")).first
	);
}

analyser_t::analyser_t(const ast_t& ast){
	QUARK_ASSERT(ast.check_invariant());

	const auto host_functions = floyd::get_host_function_signatures();
	_imm = make_shared<analyzer_imm_t>(analyzer_imm_t{ast, host_functions});
}

ast_t analyse(const analyser_t& a){
	QUARK_ASSERT(a.check_invariant());

	/*
		Create built-in globla symbol map: built in data types, built-in functions (host functions).
	*/
	std::map<std::string, symbol_t> symbol_map;

	//	Insert built-in functions into AST.
	for(auto hf_kv: a._imm->_host_functions){
		const auto& function_name = hf_kv.first;
		const auto function_value = make_host_function_value(hf_kv.second);
		symbol_map[function_name] = symbol_t::make_constant(function_value);
	}

	symbol_map[keyword_t::k_null] = symbol_t::make_constant(value_t::make_null());
	symbol_map[keyword_t::k_bool] = symbol_t::make_type(typeid_t::make_bool());
	symbol_map[keyword_t::k_int] = symbol_t::make_type(typeid_t::make_int());
	symbol_map[keyword_t::k_float] = symbol_t::make_type(typeid_t::make_float());
	symbol_map[keyword_t::k_string] = symbol_t::make_type(typeid_t::make_string());
	symbol_map[keyword_t::k_typeid] = symbol_t::make_type(typeid_t::make_typeid());
	symbol_map[keyword_t::k_json_value] = symbol_t::make_type(typeid_t::make_json_value());

	symbol_map[keyword_t::k_json_object] = symbol_t::make_constant(value_t::make_int(1));
	symbol_map[keyword_t::k_json_array] = symbol_t::make_constant(value_t::make_int(2));
	symbol_map[keyword_t::k_json_string] = symbol_t::make_constant(value_t::make_int(3));
	symbol_map[keyword_t::k_json_number] = symbol_t::make_constant(value_t::make_int(4));
	symbol_map[keyword_t::k_json_true] = symbol_t::make_constant(value_t::make_int(5));
	symbol_map[keyword_t::k_json_false] = symbol_t::make_constant(value_t::make_int(6));
	symbol_map[keyword_t::k_json_null] = symbol_t::make_constant(value_t::make_int(7));

	const auto body = body_t(a._imm->_ast._globals._statements, symbol_map_to_vec(symbol_map));
	const auto result = analyse_body(a, body);
	const auto result_ast = ast_t(result.second);

	QUARK_ASSERT(result_ast.check_invariant());
	return result_ast;
}

analyser_t::analyser_t(const analyser_t& other) :
	_imm(other._imm),
	_call_stack(other._call_stack)
{
	QUARK_ASSERT(other.check_invariant());
	QUARK_ASSERT(check_invariant());
}


//??? make proper operator=(). Exception safety etc.
const analyser_t& analyser_t::operator=(const analyser_t& other){
	_imm = other._imm;
	_call_stack = other._call_stack;
	return *this;
}

#if DEBUG
bool analyser_t::check_invariant() const {
	QUARK_ASSERT(_imm->_ast.check_invariant());
	return true;
}
#endif


floyd::ast_t run_pass3(const quark::trace_context_t& tracer, const floyd::ast_t& ast_pass2){
	QUARK_ASSERT(ast_pass2.check_invariant());

	QUARK_CONTEXT_SCOPED_TRACE(tracer, "pass3");

	QUARK_CONTEXT_TRACE_SS(tracer, "INPUT:  " << json_to_pretty_string(ast_to_json(ast_pass2)._value));
	analyser_t a(ast_pass2);
	const auto pass3_result = analyse(a);

	QUARK_CONTEXT_TRACE_SS(tracer, "OUTPUT: " << json_to_pretty_string(ast_to_json(pass3_result)._value));

	QUARK_ASSERT(statement_t::is_annotated_deep(pass3_result._globals._statements));
	return pass3_result;
}


}	//	floyd
