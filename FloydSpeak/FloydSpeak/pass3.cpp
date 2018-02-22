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



std::pair<analyser_t, expression_t> analyse_function_definition_expression(const analyser_t& vm, const expression_t& e, const std::string& function_name);

std::pair<analyser_t, expression_t> analyse_call_expression(const analyser_t& vm, const expression_t& e);
symbol_t* resolve_env_symbol2(const analyser_t& vm, const std::string& s);



	//??? add conversions. When we say we support a conversion, we should also create AST to make the conversion. AST output should be type-matched.
	//??? This is builing block for promoting values / casting.
	//	We know which type we need. If the value has not type, retype it.
	floyd::typeid_t improve_value_type(const floyd::typeid_t& value0, const floyd::typeid_t& expected_type){
		QUARK_ASSERT(value0.check_invariant());
		QUARK_ASSERT(expected_type.check_invariant());

		return value0;
/*		if(expected_type.is_null()){
			return value0;
		}
		else if(expected_type.is_bool()){
			if(value0.is_json_value() && value0.get_json_value().is_true()){
				return value_t::make_bool(true);
			}
			if(value0.is_json_value() && value0.get_json_value().is_false()){
				return value_t::make_bool(false);
			}
			else{
				return value0;
			}
		}
		else if(expected_type.is_float()){
			if(value0.is_json_value() && value0.get_json_value().is_number()){
				return value_t::make_float((float)value0.get_json_value().get_number());
			}
			else{
				return value0;
			}
		}
		else if(expected_type.is_string()){
			if(value0.is_json_value() && value0.get_json_value().is_string()){
				return value_t::make_string(value0.get_json_value().get_string());
			}
			else{
				return value0;
			}
		}
		else if(expected_type.is_json_value()){
			const auto v2 = value_to_ast_json(value0);
			return value_t::make_json_value(v2._value);
		}
		else{
			const auto value = value0;

			if(value.is_vector()){
				const auto v = value.get_vector_value();

				//	When [] appears in an expression we know it's an empty vector but not which type. It can be used as any vector type.
				if(v->_element_type.is_null() && v->_elements.empty()){
					QUARK_ASSERT(expected_type.is_vector());
					return value_t::make_vector_value(expected_type.get_vector_element_type(), value.get_vector_value()->_elements);
				}
				else{
					return value;
				}
			}
			else if(value.is_dict()){
				const auto v = value.get_dict_value();

				//	When [:] appears in an expression we know it's an empty dict but not which type. It can be used as any dict type.
				if(v->_value_type.is_null() && v->_elements.empty()){
					QUARK_ASSERT(expected_type.is_dict());
					return value_t::make_dict_value(expected_type.get_dict_value_type(), {});
				}
				else{
					return value;
				}
			}
			else{
				return value;
			}
		}
*/

	}


#if 0
std::pair<analyser_t, value_t> construct_struct(const analyser_t& vm, const typeid_t& struct_type, const vector<value_t>& values){
	QUARK_ASSERT(struct_type.get_base_type() == floyd::base_type::k_struct);
	QUARK_SCOPED_TRACE("construct_struct()");
	QUARK_TRACE("struct_type: " + typeid_to_compact_string(struct_type));

	const string struct_type_name = "unnamed";
	const auto& def = struct_type.get_struct();
	if(values.size() != def._members.size()){
		throw std::runtime_error(
			 string() + "Calling constructor for \"" + struct_type_name + "\" with " + std::to_string(values.size()) + " arguments, " + std::to_string(def._members.size()) + " + required."
		);
	}
	for(int i = 0 ; i < def._members.size() ; i++){
		const auto v = values[i];
		const auto a = def._members[i];

		QUARK_ASSERT(v.check_invariant());
		QUARK_ASSERT(v.get_type().get_base_type() != floyd::base_type::k_unresolved_type_identifier);

		if(v.get_type() != a._type){
			throw std::runtime_error("Constructor needs an arguement exactly matching type and order of struct members");
		}
	}

	const auto instance = value_t::make_struct_value(struct_type, values);
	QUARK_TRACE(to_compact_string2(instance));

	return std::pair<analyser_t, value_t>(vm, instance);
}


std::pair<analyser_t, value_t> analyze_construct_value_from_typeid(const analyser_t& vm, const typeid_t& type, const vector<value_t>& arg_values){
	if(type.is_bool() || type.is_int() || type.is_float() || type.is_string() || type.is_json_value() || type.is_typeid()){
		if(arg_values.size() != 1){
			throw std::runtime_error("Constructor requires 1 argument");
		}
		const auto value = improve_value_type(arg_values[0], type);
		if(value.get_type() != type){
			throw std::runtime_error("Constructor requires 1 argument");
		}
		return {vm, value };
	}
	else if(type.is_struct()){
		//	Constructor.
		const auto result = construct_struct(vm, type, arg_values);
		return { result.first, result.second };
	}
	else if(type.is_vector()){
		const auto element_type = type.get_vector_element_type();
		vector<value_t> elements2;
		if(element_type.is_vector()){
			for(const auto e: arg_values){
				const auto result2 = analyze_construct_value_from_typeid(vm, element_type, e.get_vector_value()->_elements);
				elements2.push_back(result2.second);
			}
		}
		else{
			elements2 = arg_values;
		}
		const auto result = value_t::make_vector_value(element_type, elements2);
		return {vm, result };
	}
	else if(type.is_dict()){
	}
	else if(type.is_function()){
	}
	else if(type.is_unresolved_type_identifier()){
	}
	else{
	}

	throw std::runtime_error("Cannot call non-function.");
}

typeid_t cleanup_vector_constructor_type(const typeid_t& type){
	if(type.is_vector()){
		const auto element_type = type.get_vector_element_type();
		if(element_type.is_vector()){
			const auto c = cleanup_vector_constructor_type(element_type);
			return typeid_t::make_vector(c);
		}
		else{
			assert(element_type.is_typeid());
			return type;
//???			return typeid_t::make_vector(element_type.get_typeid_typeid());
		}
	}
	else if(type.is_dict()){
		return type;
	}
	else{
		return type;
	}
}
#endif


analyser_t begin_subenv(const analyser_t& vm){
	QUARK_ASSERT(vm.check_invariant());

	auto vm2 = vm;
	auto parent_env = vm2._call_stack.back();
	auto new_environment = lexical_scope_t::make_environment(vm2, parent_env);
	vm2._call_stack.push_back(new_environment);
	return vm2;
}

std::pair<analyser_t, std::vector<std::shared_ptr<floyd::statement_t>> > analyse_statements_in_env(
	const analyser_t& vm,
	const std::vector<std::shared_ptr<statement_t>>& statements,
	const std::map<std::string, symbol_t>& symbols
){
	QUARK_ASSERT(vm.check_invariant());

	auto vm2 = begin_subenv(vm);
	vm2._call_stack.back()->_symbols.insert(symbols.begin(), symbols.end());
	const auto result = analyse_statements(vm2, statements);
	vm2 = result.first;
	vm2._call_stack.pop_back();
	return { vm2, result.second };
}


/*
Parser today:

	ASSIGN						IDENTIFIER = EXPRESSION;
		x = 10;
		x = "hello";
		x = f(3) == 2;
		mutable x = 10;

	BIND						TYPE IDENTIFIER = EXPRESSION;
		int x = {"a": 1, "b": 2};
		int x = 10;
		int (string a) x = f(4 == 5);
		mutable int x = 10;

BETTER:
	SIMPLE_ASSIGN						IDENTIFIER = EXPRESSION;
		x = 10;
		x = "hello";
		x = f(3) == 2;

	BIND_NEW						TYPE IDENTIFIER = EXPRESSION;
		int x = {"a": 1, "b": 2};
		int x = 10;
		int (string a) x = f(4 == 5);
		mutable int x = 10;
		mutable x = 10;
*/
// ???rename this equal_statement, the make specific statements for mutate vs bind.

/*
	Simple assign

	- Can update an existing local (if local is mutable).
	- Can implicitly create a new local
*/
std::pair<analyser_t, statement_t> analyse_statement_as_simple_assign(const analyser_t& vm, const statement_t& s){
	QUARK_ASSERT(vm.check_invariant());

	const auto statement = *s._store_local;

	auto vm_acc = vm;

	const auto bind_name = statement._new_variable_name;//??? rename to .variable_name.

	const auto expr2 = analyse_expression(vm_acc, statement._expression);
	vm_acc = expr2.first;

	const auto rhs_expr3 = expr2.second;
	const auto rhs_type = rhs_expr3.get_annotated_type();

	const auto existing_value_deep_ptr = resolve_env_symbol2(vm_acc, bind_name);

	//	Attempt to mutate existing value!
	if(existing_value_deep_ptr != nullptr){
		if(existing_value_deep_ptr->_symbol_type != symbol_t::mutable_local){
			throw std::runtime_error("Cannot assign to immutable identifier.");
		}
		else{
			const auto lhs_type = existing_value_deep_ptr->get_type();

			//	Let both existing & new values influence eachother to the exact type of the new variable.
			//	Existing could be a [null]=[], or could rhs_type
			const auto rhs_type2 = improve_value_type(rhs_type, lhs_type);
			const auto lhs_type2 = improve_value_type(lhs_type, rhs_type2);

			//??? we don't have type when calling push_back().
			if(rhs_type2 != lhs_type2 && rhs_type.is_null() == false){
				throw std::runtime_error("Types not compatible in bind.");
			}
			else{
				return { vm_acc, statement_t::make__store_local(bind_name, rhs_expr3) };
			}
		}
	}

	//	Deduce type and bind it -- to local env.
	else{
		//	Can we deduce type from the rhs value?
		if(rhs_type == typeid_t::make_vector(typeid_t::make_null())){
			throw std::runtime_error("Cannot deduce vector type.");
		}
		else if(rhs_type == typeid_t::make_dict(typeid_t::make_null())){
			throw std::runtime_error("Cannot deduce dictionary type.");
		}
		else{
			vm_acc._call_stack.back()->_symbols[bind_name] = symbol_t::make_immutable_local(rhs_type);
			return { vm_acc, statement_t::make__bind_local(bind_name, rhs_type, rhs_expr3, statement_t::bind_local_t::k_immutable) };
		}
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
std::pair<analyser_t, statement_t> analyse_statement_as_bindnew(const analyser_t& vm, const statement_t& s){
	QUARK_ASSERT(vm.check_invariant());

	const auto statement = *s._bind_local;
	auto vm_acc = vm;

	const auto bind_name = statement._new_variable_name;//??? rename to .variable_name.
	const auto bind_statement_type = statement._bindtype;
	const auto bind_statement_mutable_tag_flag = statement._locals_mutable_mode == statement_t::bind_local_t::k_mutable;

	const auto value_exists_in_env = vm_acc._call_stack.back()->_symbols.count(bind_name) > 0;
	if(value_exists_in_env){
		throw std::runtime_error("Local identifier already exists.");
	}

	//	Important: the expression may reference the symbol we are binding (this happens when recursive function definitions, like fibonacci).
	//	We need to insert symbol before analysing the expression.

	const auto rhs_expr2_pair = analyse_expression(vm_acc, statement._expression);
	vm_acc = rhs_expr2_pair.first;

	const auto rhs_expr3 = rhs_expr2_pair.second;
	const auto rhs_type = rhs_expr3.get_annotated_type();

	const auto lhs_type = bind_statement_type;
	const auto rhs_type2 = improve_value_type(rhs_type, lhs_type);

	//	Deduced bind type -- use new value's type.
	if(lhs_type.is_null()){
		if(rhs_type2 == typeid_t::make_vector(typeid_t::make_null())){
			throw std::runtime_error("Cannot deduce vector type.");
		}
		else if(rhs_type2 == typeid_t::make_dict(typeid_t::make_null())){
			throw std::runtime_error("Cannot deduce dictionary type.");
		}
		else{
			vm_acc._call_stack.back()->_symbols[bind_name] = bind_statement_mutable_tag_flag ? symbol_t::make_mutable_local(rhs_type2) : symbol_t::make_immutable_local(rhs_type2);
			return {
				vm_acc,
				statement_t::make__bind_local(bind_name, rhs_type2, rhs_expr3, bind_statement_mutable_tag_flag ? statement_t::bind_local_t::k_mutable : statement_t::bind_local_t::k_immutable)
			};
		}
	}

	//	Explicit bind-type -- make sure source + dest types match.
	else{
		if(lhs_type != rhs_type2){
//???			throw std::runtime_error("Types not compatible in bind.");
		}
		vm_acc._call_stack.back()->_symbols[bind_name] = bind_statement_mutable_tag_flag ? symbol_t::make_mutable_local(lhs_type) : symbol_t::make_immutable_local(lhs_type);
		return {
			vm_acc,
			statement_t::make__bind_local(bind_name, lhs_type, rhs_expr3, bind_statement_mutable_tag_flag ? statement_t::bind_local_t::k_mutable : statement_t::bind_local_t::k_immutable)
		};
	}
}


std::pair<analyser_t, statement_t> analyse_block_statement(const analyser_t& vm, const statement_t::block_statement_t& statement){
	QUARK_ASSERT(vm.check_invariant());

	const auto e = analyse_statements_in_env(vm, statement._statements, {});
	return {e.first, statement_t::make__block_statement(e.second)};
}

std::pair<analyser_t, statement_t> analyse_return_statement(const analyser_t& vm, const statement_t::return_statement_t& statement){
	QUARK_ASSERT(vm.check_invariant());

	auto vm_acc = vm;
	const auto expr = statement._expression;
	const auto result = analyse_expression(vm_acc, expr);
	vm_acc = result.first;

	const auto result_value = result.second;

	//	Check that return value's type matches function's return type. Cannot be done here since we don't know who called us.
	//	Instead calling code must check.
	return { vm_acc, statement_t::make__return_statement(result.second) };
}

//	TODO: When symbol tables are kept with pass3, this function returns a NOP-statement.
std::pair<analyser_t, statement_t> analyse_def_struct_statement(const analyser_t& vm, const statement_t::define_struct_statement_t& statement){
	QUARK_ASSERT(vm.check_invariant());

	auto vm_acc = vm;
	const auto struct_name = statement._name;
	if(vm_acc._call_stack.back()->_symbols.count(struct_name) > 0){
		throw std::runtime_error("Name already used.");
	}

	//	Resolve member types in this scope.
	std::vector<member_t> members2;
	for(const auto e: statement._def._members){
		const auto name = e._name;
		const auto type = e._type;
		const auto type2 = resolve_type_using_env(vm_acc, type);
		const auto e2 = member_t(type2, name);
		members2.push_back(e2);
	}
	const auto resolved_struct_def = std::make_shared<struct_definition_t>(struct_definition_t(members2));
	const auto struct_typeid = typeid_t::make_struct(resolved_struct_def);
	const auto struct_typeid_value = value_t::make_typeid_value(struct_typeid);
	vm_acc._call_stack.back()->_symbols[struct_name] = symbol_t::make_constant(struct_typeid_value);
	return { vm_acc, statement };
}

std::pair<analyser_t, statement_t> analyse_ifelse_statement(const analyser_t& vm, const statement_t::ifelse_statement_t& statement){
	QUARK_ASSERT(vm.check_invariant());

	auto vm_acc = vm;

	const auto condition2 = analyse_expression(vm_acc, statement._condition);
	vm_acc = condition2.first;

	const auto condition_type = condition2.second.get_annotated_type();
	if(condition_type.is_bool() == false){
		throw std::runtime_error("Boolean condition required.");
	}

	const auto then2 = analyse_statements_in_env(vm_acc, statement._then_statements, {});
	const auto else2 = analyse_statements_in_env(vm_acc, statement._else_statements, {});
	return { vm_acc, statement_t::make__ifelse_statement(condition2.second, then2.second, else2.second) };
}

std::pair<analyser_t, statement_t> analyse_for_statement(const analyser_t& vm, const statement_t::for_statement_t& statement){
	QUARK_ASSERT(vm.check_invariant());

	auto vm_acc = vm;

	const auto start_expr2 = analyse_expression(vm_acc, statement._start_expression);
	vm_acc = start_expr2.first;

	if(start_expr2.second.get_annotated_type().is_int() == false){
		throw std::runtime_error("For-loop requires integer iterator.");
	}

	const auto end_expr2 = analyse_expression(vm_acc, statement._end_expression);
	vm_acc = end_expr2.first;

	if(end_expr2.second.get_annotated_type().is_int() == false){
		throw std::runtime_error("For-loop requires integer iterator.");
	}

	const auto iterator_symbol = symbol_t::make_immutable_local(typeid_t::make_int());
	const std::map<std::string, symbol_t> symbols = { { statement._iterator_name, iterator_symbol} };

	const auto result = analyse_statements_in_env(vm_acc, statement._body, symbols);
	vm_acc = result.first;

	return { vm_acc, statement_t::make__for_statement(statement._iterator_name, start_expr2.second, end_expr2.second, result.second) };
}

std::pair<analyser_t, statement_t> analyse_while_statement(const analyser_t& vm, const statement_t::while_statement_t& statement){
	QUARK_ASSERT(vm.check_invariant());

	auto vm_acc = vm;

	const auto condition2_expr = analyse_expression(vm_acc, statement._condition);
	vm_acc = condition2_expr.first;

	const auto result = analyse_statements_in_env(vm_acc, statement._body, {});
	vm_acc = result.first;

	return { vm_acc, statement_t::make__while_statement(condition2_expr.second, result.second) };
}

std::pair<analyser_t, statement_t> analyse_expression_statement(const analyser_t& vm, const statement_t::expression_statement_t& statement){
	QUARK_ASSERT(vm.check_invariant());

	auto vm_acc = vm;
	const auto expr2 = analyse_expression(vm_acc, statement._expression);
	vm_acc = expr2.first;

	return { vm_acc, statement_t::make__expression_statement(expr2.second) };
}

/*
std::pair<analyser_t, statement_t> analyse_bind_statement(const analyser_t& vm, const statement_t::bind_or_assign_statement_t& statement){
	QUARK_ASSERT(vm.check_invariant());

	const auto bind_statement_type = statement._bindtype;
	const auto bind_statement_mutable_mode = statement._bind_as_mutable_tag;

	//	If we have a type or we have the mutable-flag, then this statement is a bindnew, else it's a simple-assign.
	bool bindnew_mode = bind_statement_type.is_null() == false || bind_statement_mutable_mode == statement_t::bind_or_assign_statement_t::k_has_mutable_tag;
	if(bindnew_mode){
		return analyse_statement_as_bindnew(vm, statement);
	}
	else{
		return analyse_statement_as_simple_assign(vm, statement);
	}
}
*/

//	Output is the RETURN VALUE of the analysed statement, if any.
std::pair<analyser_t, statement_t> analyse_statement(const analyser_t& vm, const statement_t& statement){
	QUARK_ASSERT(vm.check_invariant());
	QUARK_ASSERT(statement.check_invariant());

	if(statement._bind_local){
		const auto e = analyse_statement_as_bindnew(vm, statement);
		QUARK_ASSERT(e.second.is_annotated_deep());
		return e;
	}
	else if(statement._store_local){
		const auto e = analyse_statement_as_simple_assign(vm, statement);
		QUARK_ASSERT(e.second.is_annotated_deep());
		return e;
	}
	else if(statement._block){
		const auto e = analyse_block_statement(vm, *statement._block);
		QUARK_ASSERT(e.second.is_annotated_deep());
		return e;
	}
	else if(statement._return){
		const auto e = analyse_return_statement(vm, *statement._return);
		QUARK_ASSERT(e.second.is_annotated_deep());
		return e;
	}
	else if(statement._def_struct){
		const auto e = analyse_def_struct_statement(vm, *statement._def_struct);
		QUARK_ASSERT(e.second.is_annotated_deep());
		return e;
	}
	else if(statement._if){
		const auto e = analyse_ifelse_statement(vm, *statement._if);
		QUARK_ASSERT(e.second.is_annotated_deep());
		return e;
	}
	else if(statement._for){
		const auto e = analyse_for_statement(vm, *statement._for);
		QUARK_ASSERT(e.second.is_annotated_deep());
		return e;
	}
	else if(statement._while){
		const auto e = analyse_while_statement(vm, *statement._while);
		QUARK_ASSERT(e.second.is_annotated_deep());
		return e;
	}
	else if(statement._expression){
		const auto e = analyse_expression_statement(vm, *statement._expression);
		QUARK_ASSERT(e.second.is_annotated_deep());
		return e;
	}
	else{
		QUARK_ASSERT(false);
		throw std::exception();
	}
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
		QUARK_ASSERT(r.second.is_annotated_deep());

		vm_acc = r.first;
		statements2.push_back(make_shared<statement_t>(r.second));
		statement_index++;
	}
	return { vm_acc, statements2 };
}

//	Warning: returns reference to the found value-entry -- this could be in any environment in the call stack.
symbol_t* resolve_env_variable_deep(const analyser_t& vm, const shared_ptr<lexical_scope_t>& env, const std::string& s){
	QUARK_ASSERT(vm.check_invariant());
	QUARK_ASSERT(env && env->check_invariant());
	QUARK_ASSERT(s.size() > 0);

	const auto it = env->_symbols.find(s);
	if(it != env->_symbols.end()){
		return &it->second;
	}
	else if(env->_parent_env){
		return resolve_env_variable_deep(vm, env->_parent_env, s);
	}
	else{
		return nullptr;
	}
}

//	Warning: returns reference to the found value-entry -- this could be in any environment in the call stack.
symbol_t* resolve_env_symbol2(const analyser_t& vm, const std::string& s){
	QUARK_ASSERT(vm.check_invariant());
	QUARK_ASSERT(s.size() > 0);

	return resolve_env_variable_deep(vm, vm._call_stack.back(), s);
}

symbol_t find_global_symbol(const analyser_t& vm, const string& s){
	const auto t = resolve_env_symbol2(vm, s);
	if(t == nullptr){
		throw std::runtime_error("Undefined indentifier \"" + s + "\"!");
	}
	return *t;
}



std::pair<analyser_t, expression_t> analyse_resolve_member_expression(const analyser_t& vm, const expression_t::resolve_member_expr_t& expr){
	QUARK_ASSERT(vm.check_invariant());

	auto vm_acc = vm;
	const auto parent_expr = analyse_expression(vm_acc, *expr._parent_address);
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

//??? Convert all these mechamisms into a built-lookup-function.
std::pair<analyser_t, expression_t> analyse_lookup_element_expression(const analyser_t& vm, const expression_t::lookup_expr_t& expr){
	QUARK_ASSERT(vm.check_invariant());

	auto vm_acc = vm;
	const auto parent_expr = analyse_expression(vm_acc, *expr._parent_address);
	vm_acc = parent_expr.first;

	const auto key_expr = analyse_expression(vm_acc, *expr._lookup_key);
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

std::pair<analyser_t, expression_t> analyse_variable_expression(const analyser_t& vm, const expression_t& e){
	QUARK_ASSERT(vm.check_invariant());

	const auto expr = *e.get_variable();

	auto vm_acc = vm;
	const auto value = resolve_env_symbol2(vm_acc, expr._variable);
	if(value != nullptr){
		return {vm_acc, expression_t::make_variable_expression(expr._variable, make_shared<typeid_t>(value->_value_type)) };
	}
	else{
		throw std::runtime_error("Undefined variable \"" + expr._variable + "\".");
	}
}

std::pair<analyser_t, expression_t> analyse_vector_definition_expression(const analyser_t& vm, const expression_t::vector_definition_exprt_t& expr){
	QUARK_ASSERT(vm.check_invariant());

	auto vm_acc = vm;
	const std::vector<expression_t>& elements = expr._elements;
	const auto element_type = expr._element_type;

	if(elements.empty()){
		return {vm_acc, expression_t::make_literal(value_t::make_vector_value(element_type, {}))};
	}
	else{
		std::vector<expression_t> elements2;
		for(const auto m: elements){
			const auto element_expr = analyse_expression(vm_acc, m);
			vm_acc = element_expr.first;
			elements2.push_back(element_expr.second);
		}

		//	If we don't have an explicit element type, deduct it from first element.
		const auto element_type2 = element_type.is_null() ? elements2[0].get_annotated_type(): element_type;
		for(const auto m: elements2){
			if(m.get_annotated_type() != element_type2){
				throw std::runtime_error("Vector can not hold elements of different type!");
			}
		}
		return { vm_acc, expression_t::make_vector_definition(element_type2, elements2) };
	}
}


std::pair<analyser_t, expression_t> analyse_dict_definition_expression(const analyser_t& vm, const expression_t::dict_definition_exprt_t& expr){
	QUARK_ASSERT(vm.check_invariant());

	auto vm_acc = vm;

	const auto& elements = expr._elements;
	const typeid_t value_type = expr._value_type;

	if(elements.empty()){
		return {vm_acc, expression_t::make_literal(value_t::make_dict_value(value_type, {}))};
	}
	else{
		map<string, expression_t> elements2;
		for(const auto m_kv: elements){
			const auto element_expr = analyse_expression(vm_acc, m_kv.second);
			vm_acc = element_expr.first;

			elements2.insert(make_pair(m_kv.first, element_expr.second));
		}

		//	Deduce type of dictionary.
		const auto element_type2 = value_type.is_null() == false ? value_type : elements2.begin()->second.get_annotated_type();

		//	Make sure all elements have the correct type.
		for(const auto m_kv: elements2){
			const auto element_type = m_kv.second.get_annotated_type();
			if(element_type != element_type2){
//??? dict-def needs to hold mixed values to allow json_value to type object to work.
//???				throw std::runtime_error("Dict can not hold elements of different type!");
			}
		}

		return {vm_acc, expression_t::make_dict_definition(element_type2, elements2)};
	}
}

std::pair<analyser_t, expression_t> analyse_arithmetic_unary_minus_expression(const analyser_t& vm, const expression_t::unary_minus_expr_t& expr){
	QUARK_ASSERT(vm.check_invariant());

	auto vm_acc = vm;
	const auto& expr2 = analyse_expression(vm_acc, *expr._expr);
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
	const auto cond_result = analyse_expression(vm_acc, *expr._condition);
	vm_acc = cond_result.first;

	const auto a = analyse_expression(vm_acc, *expr._a);
	vm_acc = a.first;

	const auto b = analyse_expression(vm_acc, *expr._b);
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
	const auto left_expr = analyse_expression(vm_acc, *simple2_expr._left);
	vm_acc = left_expr.first;

	const auto right_expr = analyse_expression(vm_acc, *simple2_expr._right);
	vm_acc = right_expr.first;

	//	Perform math operation on the two constants => new constant.
	const auto left_type = left_expr.second.get_annotated_type();

	//	Make rhs match left if needed/possible.
	const auto right_type = improve_value_type(right_expr.second.get_annotated_type(), left_type);

	// ??? we don't have all types yet.
	if(left_type != right_type && left_type.is_null() == false && right_type.is_null() == false){
		throw std::runtime_error("Left and right expressions must be same type!");
	}
	else{
		const auto final_type = left_type;

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
	const auto left_expr = analyse_expression(vm_acc, *simple2_expr._left);
	vm_acc = left_expr.first;

	const auto right_expr = analyse_expression(vm_acc, *simple2_expr._right);
	vm_acc = right_expr.first;


	const auto left_type = left_expr.second.get_annotated_type();

	//	Make rhs match lhs if needed/possible.
	const auto right_type = improve_value_type(right_expr.second.get_annotated_type(), left_type);

	if(left_type != right_type){
		throw std::runtime_error("Left and right expressions must be same type!");
	}
	else{
		const auto shared_type = left_type;

		//	bool
		if(shared_type.is_bool()){
			if(false
			|| op == expression_type::k_arithmetic_add__2
			|| op == expression_type::k_arithmetic_subtract__2
			|| op == expression_type::k_arithmetic_multiply__2
			|| op == expression_type::k_arithmetic_divide__2
			|| op == expression_type::k_arithmetic_remainder__2
			){
				throw std::runtime_error("Arithmetics on bool not allowed.");
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
			const auto element_type = left_type.get_vector_element_type();
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

std::pair<analyser_t, expression_t> analyse_expression(const analyser_t& vm, const expression_t& e){
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
	else if(op == expression_type::k_variable){
		return analyse_variable_expression(vm, e);
	}

	else if(op == expression_type::k_call){
		return analyse_call_expression(vm, e);
	}

	else if(op == expression_type::k_define_function){
		return analyse_function_definition_expression(vm, e, "");
	}

	else if(op == expression_type::k_vector_definition){
		return analyse_vector_definition_expression(vm, *e.get_vector_definition());
	}

	else if(op == expression_type::k_dict_definition){
		return analyse_dict_definition_expression(vm, *e.get_dict_definition());
	}

	//	This can be desugared at compile time.
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

/*
//	NOTICE: We do function overloading for the host functions: you can call them with any *type* of arguments and it gives any return type.
std::pair<analyser_t, statement_result_t> call_host_function(const analyser_t& vm, int function_id, const std::vector<floyd::value_t> args){
	const int index = function_id - 1000;
	QUARK_ASSERT(index >= 0 && index < k_host_functions.size())

	const auto& host_function = k_host_functions[index];

	//	arity
	if(args.size() != host_function._function_type.get_function_args().size()){
		throw std::runtime_error("Wrong number of arguments to host function.");
	}

	const auto result = (host_function._function_ptr)(vm, args);
	return {
		result.first,
		statement_result_t::make_return_unwind(result.second)
	};
}
*/
/*
	a = my_func(x, 13, "cat");
	a = json_value(1 + 3);
*/
std::pair<analyser_t, expression_t> analyse_call_expression(const analyser_t& vm, const expression_t& e){
	QUARK_ASSERT(vm.check_invariant());

	const auto expr = *e.get_call();
	auto vm_acc = vm;

	const auto callee_expr0 = analyse_expression(vm_acc, *expr._callee);
	vm_acc = callee_expr0.first;
	const auto callee_expr = callee_expr0.second;

	vector<expression_t> call_expr_args;
	for(int i = 0 ; i < expr._args.size() ; i++){
		const auto t = analyse_expression(vm_acc, expr._args[i]);
		vm_acc = t.first;
		call_expr_args.push_back(t.second);
	}

	//	This is a call to a function-value.
	const auto callee_type = callee_expr.get_annotated_type();
	if(callee_type.is_function()){
		const auto callee_args = callee_type.get_function_args();
		const auto callee_return_value = callee_type.get_function_return();

		//	arity
		if(call_expr_args.size() != callee_args.size()){
			throw std::runtime_error("Wrong number of arguments in function call.");
		}
		for(int i = 0 ; i < callee_args.size() ; i++){
			const auto arg_lhs = callee_args[i];
			const auto arg_rhs = call_expr_args[i].get_annotated_type();

			//	null means "any" right now...###
			if(arg_lhs.is_null() || arg_lhs == arg_rhs){
			}
			else{
				throw std::runtime_error("Argument type mismatch.");
			}
		}
		return { vm_acc, expression_t::make_call(callee_expr, call_expr_args, make_shared<typeid_t>(callee_return_value)) };
	}

	//	Attempting to call a TYPE? Then this may be a constructor call.
	else if(callee_type.is_typeid() && callee_expr.get_operation() == expression_type::k_variable){
		const auto variable_expr = *callee_expr.get_variable();
		const auto variable_name = variable_expr._variable;
		const symbol_t* symbol = resolve_env_symbol2(vm_acc, variable_name);
		if(symbol == nullptr){
			throw std::runtime_error("Cannot resolve callee.");
		}
		else if(symbol->_symbol_type != symbol_t::type::immutable_local && symbol->_symbol_type != symbol_t::type::mutable_local){
			throw std::runtime_error("Cannot resolve callee.");
		}
		else{
			const auto type = symbol->_default_value.get_typeid_value();
			return { vm_acc, expression_t::make_call(callee_expr, call_expr_args, make_shared<typeid_t>(type)) };
		}
	}
	else{
		throw std::runtime_error("Cannot call non-function.");
	}
}





std::pair<analyser_t, expression_t> analyse_function_definition_expression(const analyser_t& vm, const expression_t& e, const std::string& function_name){
	QUARK_ASSERT(vm.check_invariant());

	const auto function_def_expr = *e.get_function_definition();
	const auto def = function_def_expr._def;
	const auto function_type = get_function_type(def);

	const auto iterator_symbol = symbol_t::make_immutable_local(typeid_t::make_int());
	const std::map<std::string, symbol_t> symbols =
		[&](){
			std::map<std::string, symbol_t> result;
			for(const auto e: def._args){
				result[e._name] = symbol_t::make_immutable_local(e._type);
			}
			return result;
		}();

	auto symbols2 = symbols;
	if(function_name.empty() == false){
		symbols2[function_name] = symbol_t::make_immutable_local(function_type);
	}

	const auto function_body_pair = analyse_statements_in_env(vm, def._statements, symbols);
	auto vm_acc = function_body_pair.first;

	const auto def2 = function_definition_t(def._args, function_body_pair.second, def._return_type);
	return {vm_acc, expression_t::make_function_definition(def2) };
}


json_t analyser_to_json(const analyser_t& vm){
	vector<json_t> callstack;
	for(const auto& e: vm._call_stack){
		std::map<string, json_t> values;
		for(const auto& symbol_kv: e->_symbols){
			const auto b = json_t::make_array({
				json_t((int)symbol_kv.second._symbol_type),
				typeid_to_ast_json(symbol_kv.second._value_type)._value,
				value_to_ast_json(symbol_kv.second._default_value)._value
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
		{ "ast", ast_to_json(*vm._ast)._value },
		{ "callstack", json_t::make_array(callstack) }
	});
}



void test__analyse_expression(const expression_t& e, const expression_t& expected){
	const ast_t ast;
	const analyser_t interpreter(ast);
	const auto e3 = analyse_expression(interpreter, e);

	ut_compare_jsons(expression_to_json(e3.second)._value, expression_to_json(expected)._value);
}


QUARK_UNIT_TEST("analyse_expression()", "literal 1234 == 1234", "", "") {
	test__analyse_expression(
		expression_t::make_literal_int(1234),
		expression_t::make_literal_int(1234)
	);
}

QUARK_UNIT_TESTQ("analyse_expression()", "1 + 2 == 3") {


	const ast_t ast;
	const analyser_t interpreter(ast);
	const auto e3 = analyse_expression(interpreter,

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







//////////////////////////		lexical_scope_t



std::shared_ptr<lexical_scope_t> lexical_scope_t::make_environment(
	const analyser_t& vm,
	std::shared_ptr<lexical_scope_t>& parent_env
)
{
	QUARK_ASSERT(vm.check_invariant());

	auto f = lexical_scope_t{ parent_env, {} };
	return make_shared<lexical_scope_t>(f);
}

bool lexical_scope_t::check_invariant() const {
	return true;
}





//### add checking of function types when calling / returning from them. Also host functions.

typeid_t resolve_type_using_env(const analyser_t& vm, const typeid_t& type){
	if(type.get_base_type() == base_type::k_unresolved_type_identifier){
		const auto v = resolve_env_symbol2(vm, type.get_unresolved_type_identifier());
		if(v){
			if(v->_value_type.is_typeid()){
				return v->_default_value.get_typeid_value();
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


struct host_function_t {
	std::string _name;
	int _function_id;
	typeid_t _function_type;
};

analyser_t::analyser_t(const ast_t& ast){
	QUARK_ASSERT(ast.check_invariant());

	_ast = make_shared<ast_t>(ast);
}

ast_t analyse(const analyser_t& a0){
	QUARK_ASSERT(a0.check_invariant());

	auto a = a0;

	//	Make the top-level environment = global scope.
	shared_ptr<lexical_scope_t> empty_env;
	auto global_env = lexical_scope_t::make_environment(a, empty_env);


	//	??? Had problem having this a global constant: keyword_t::k_null had not be statically initialized.
	const vector<host_function_t> k_host_functions {
		host_function_t{ "print", 1000, typeid_t::make_function(typeid_t::make_null(), {typeid_t::make_null()}) },
		host_function_t{ "assert", 1001, typeid_t::make_function(typeid_t::make_null(), {typeid_t::make_null()}) },
		host_function_t{ "to_string", 1002, typeid_t::make_function(typeid_t::make_string(), {typeid_t::make_null()}) },
		host_function_t{ "to_pretty_string", 1003, typeid_t::make_function(typeid_t::make_string(), {typeid_t::make_null()}) },
		host_function_t{ "typeof", 1004, typeid_t::make_function(typeid_t::make_typeid(), {typeid_t::make_null()}) },

		host_function_t{ "get_time_of_day", 1005, typeid_t::make_function(typeid_t::make_int(), {}) },
		host_function_t{ "update", 1006, typeid_t::make_function(typeid_t::make_null(), {typeid_t::make_null(),typeid_t::make_null(),typeid_t::make_null()}) },
		host_function_t{ "size", 1007, typeid_t::make_function(typeid_t::make_int(), {typeid_t::make_null()}) },
		host_function_t{ "find", 1008, typeid_t::make_function(typeid_t::make_int(), {typeid_t::make_null(), typeid_t::make_null()}) },
		host_function_t{ "exists", 1009, typeid_t::make_function(typeid_t::make_bool(), {typeid_t::make_null(),typeid_t::make_null()}) },
		host_function_t{ "erase", 1010, typeid_t::make_function(typeid_t::make_null(), {typeid_t::make_null(),typeid_t::make_null()}) },
		host_function_t{ "push_back", 1011, typeid_t::make_function(typeid_t::make_null(), {typeid_t::make_null(),typeid_t::make_null()}) },
		host_function_t{ "subset", 1012, typeid_t::make_function(typeid_t::make_null(), {typeid_t::make_null(),typeid_t::make_null(),typeid_t::make_null()}) },
		host_function_t{ "replace", 1013, typeid_t::make_function(typeid_t::make_null(), {typeid_t::make_null(),typeid_t::make_null(),typeid_t::make_null(),typeid_t::make_null()}) },

		host_function_t{ "get_env_path", 1014, typeid_t::make_function(typeid_t::make_string(), {}) },
		host_function_t{ "read_text_file", 1015, typeid_t::make_function(typeid_t::make_string(), {typeid_t::make_null()}) },
		host_function_t{ "write_text_file", 1016, typeid_t::make_function(typeid_t::make_null(), {typeid_t::make_null(), typeid_t::make_null()}) },

		host_function_t{ "decode_json", 1017, typeid_t::make_function(typeid_t::make_json_value(), {typeid_t::make_string()}) },
		host_function_t{ "encode_json", 1018, typeid_t::make_function(typeid_t::make_string(), {typeid_t::make_json_value()}) },

		host_function_t{ "flatten_to_json", 1019, typeid_t::make_function(typeid_t::make_json_value(), {typeid_t::make_null()}) },
		host_function_t{ "unflatten_from_json", 1020, typeid_t::make_function(typeid_t::make_null(), {typeid_t::make_null(), typeid_t::make_null()}) },

		host_function_t{ "get_json_type", 10121, typeid_t::make_function(typeid_t::make_int(), {typeid_t::make_json_value()}) }
	};

	//	Insert built-in functions into AST.
	for(auto i = 0 ; i < k_host_functions.size() ; i++){
		const auto& hf = k_host_functions[i];

		const auto aaaaa = [&](){
			vector<member_t> result;
			for(const auto e: hf._function_type.get_function_args()){
				result.push_back(member_t(e, "dummy"));
			}
			return result;
		}();

		const auto def = function_definition_t(
			aaaaa,
			i + 1000,
			hf._function_type.get_function_return()
		);

		const auto function_value = value_t::make_function_value(def);
		global_env->_symbols[hf._name] = symbol_t::make_constant(function_value);
	}
	global_env->_symbols[keyword_t::k_null] = symbol_t::make_type(typeid_t::make_null());
	global_env->_symbols[keyword_t::k_bool] = symbol_t::make_type(typeid_t::make_bool());
	global_env->_symbols[keyword_t::k_int] = symbol_t::make_type(typeid_t::make_int());
	global_env->_symbols[keyword_t::k_float] = symbol_t::make_type(typeid_t::make_float());
	global_env->_symbols[keyword_t::k_string] = symbol_t::make_type(typeid_t::make_string());
	global_env->_symbols[keyword_t::k_typeid] = symbol_t::make_type(typeid_t::make_typeid());
	global_env->_symbols[keyword_t::k_json_value] = symbol_t::make_type(typeid_t::make_json_value());

	global_env->_symbols[keyword_t::k_json_object] = symbol_t::make_constant(value_t::make_int(1));
	global_env->_symbols[keyword_t::k_json_array] = symbol_t::make_constant(value_t::make_int(2));
	global_env->_symbols[keyword_t::k_json_string] = symbol_t::make_constant(value_t::make_int(3));
	global_env->_symbols[keyword_t::k_json_number] = symbol_t::make_constant(value_t::make_int(4));
	global_env->_symbols[keyword_t::k_json_true] = symbol_t::make_constant(value_t::make_int(5));
	global_env->_symbols[keyword_t::k_json_false] = symbol_t::make_constant(value_t::make_int(6));
	global_env->_symbols[keyword_t::k_json_null] = symbol_t::make_constant(value_t::make_int(7));

	a._call_stack.push_back(global_env);

	//	Run static intialization (basically run global statements before calling main()).
	const auto result = analyse_statements(a, a._ast->_statements);

	a._call_stack[0]->_symbols = result.first._call_stack[0]->_symbols;

	const auto result_ast = ast_t(result.second);

	QUARK_ASSERT(result_ast.check_invariant());
	return result_ast;
}

analyser_t::analyser_t(const analyser_t& other) :
	_ast(other._ast),
	_call_stack(other._call_stack)
{
	QUARK_ASSERT(other.check_invariant());
	QUARK_ASSERT(check_invariant());
}


	//??? make proper operator=(). Exception safety etc.
const analyser_t& analyser_t::operator=(const analyser_t& other){
	_ast = other._ast;
	_call_stack = other._call_stack;
	return *this;
}

#if DEBUG
bool analyser_t::check_invariant() const {
	QUARK_ASSERT(_ast->check_invariant());
	return true;
}
#endif


#if 1

floyd::ast_t run_pass3(const quark::trace_context_t& tracer, const floyd::ast_t& ast_pass2){
	QUARK_ASSERT(ast_pass2.check_invariant());

	QUARK_CONTEXT_SCOPED_TRACE(tracer, "pass3");

	QUARK_CONTEXT_TRACE_SS(tracer, "INPUT:  " << json_to_pretty_string(ast_to_json(ast_pass2)._value));
	analyser_t a(ast_pass2);
	const auto pass3_result = analyse(a);

	QUARK_CONTEXT_TRACE_SS(tracer, "OUTPUT: " << json_to_pretty_string(ast_to_json(pass3_result)._value));

	QUARK_ASSERT(statement_t::is_annotated_deep(pass3_result._statements));
	return pass3_result;
}

#else

floyd::ast_t run_pass3(const quark::trace_context_t& tracer, const floyd::ast_t& ast_pass2){
	return ast_pass2;
}

#endif



}	//	floyd

