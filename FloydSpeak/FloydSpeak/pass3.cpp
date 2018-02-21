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




std::pair<analyser_t, expression_t> analyse_call_expression(const analyser_t& vm, const expression_t& e);
symbol_t* resolve_env_symbol2(const analyser_t& vm, const std::string& s);



	//??? add conversions.
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



namespace {

analyser_t begin_subenv(const analyser_t& vm){
	QUARK_ASSERT(vm.check_invariant());

	auto vm2 = vm;
	auto parent_env = vm2._call_stack.back();
	auto new_environment = environment_t::make_environment(vm2, parent_env);
	vm2._call_stack.push_back(new_environment);
	return vm2;
}

std::pair<analyser_t, statement_result_t> analyse_statements_in_env(
	const analyser_t& vm,
	const std::vector<std::shared_ptr<statement_t>>& statements,
	const std::map<std::string, symbol_t>& symbols
){
	QUARK_ASSERT(vm.check_invariant());

	auto vm2 = begin_subenv(vm);

	vm2._call_stack.back()->_symbols.insert(symbols.begin(), symbols.end());

	const auto r = analyse_statements(vm2, statements);
	vm2 = r.first;
	vm2._call_stack.pop_back();
	return { vm2, r.second };
}


std::pair<analyser_t, statement_result_t> analyse_bind_statement(const analyser_t& vm, const statement_t::bind_or_assign_statement_t& statement){
	QUARK_ASSERT(vm.check_invariant());

	auto vm_acc = vm;
	const auto name = statement._new_variable_name;//??? rename to .variable_name.

	const auto result = analyse_expression(vm_acc, statement._expression);
	vm_acc = result.first;

	const auto result_value = result.second;
//	if(result_value.is_literal() == false){
//		throw std::runtime_error("Cannot analyse expression.");
//	}
	/*else*/{
		const auto bind_statement_type = statement._bindtype;
		const auto bind_statement_mutable_tag_flag = statement._bind_as_mutable_tag;

		//	If we have a type or we have the mutable-flag, then this statement is a bind.
		bool is_bind = bind_statement_type.is_null() == false || bind_statement_mutable_tag_flag;

		//	Bind.
		//	int a = 10
		//	mutable a = 10
		//	mutable = 10
		if(is_bind){
			const auto retyped_value = improve_value_type(result_value.get_annotated_type(), bind_statement_type);
			const auto value_exists_in_env = vm_acc._call_stack.back()->_symbols.count(name) > 0;

			if(value_exists_in_env){
				throw std::runtime_error("Local identifier already exists.");
			}

			//	Deduced bind type -- use new value's type.
			if(bind_statement_type.is_null()){
				if(retyped_value == typeid_t::make_vector(typeid_t::make_null())){
					throw std::runtime_error("Cannot deduce vector type.");
				}
				else if(retyped_value == typeid_t::make_dict(typeid_t::make_null())){
					throw std::runtime_error("Cannot deduce dictionary type.");
				}
				else{
					vm_acc._call_stack.back()->_symbols[name] = bind_statement_mutable_tag_flag ? symbol_t::make_mutable_local(retyped_value) : symbol_t::make_immutable_local(retyped_value);
				}
			}

			//	Explicit bind-type -- make sure source + dest types match.
			else{
				if(bind_statement_type != retyped_value){
					throw std::runtime_error("Types not compatible in bind.");
				}
//				vm_acc._call_stack.back()->_symbols[name] = std::pair<value_t, bool>(retyped_value, bind_statement_mutable_tag_flag);
				vm_acc._call_stack.back()->_symbols[name] = bind_statement_mutable_tag_flag ? symbol_t::make_mutable_local(retyped_value) : symbol_t::make_immutable_local(retyped_value);
			}
		}

		//	Assign
		//	a = 10
		else{
			const auto existing_value_deep_ptr = resolve_env_symbol2(vm_acc, name);
			if(existing_value_deep_ptr->_symbol_type != symbol_t::immutable_local && existing_value_deep_ptr->_symbol_type != symbol_t::immutable_local){
				throw std::runtime_error("xxxx.");
			}
			const bool existing_variable_is_mutable = existing_value_deep_ptr && (existing_value_deep_ptr->_symbol_type != symbol_t::mutable_local);

			//	Mutate!
			if(existing_value_deep_ptr){
				if(existing_variable_is_mutable){
					const auto existing_value = *existing_value_deep_ptr;
					const auto new_value = result_value.get_annotated_type();

					//	Let both existing & new values influence eachother to the exact type of the new variable.
					//	Existing could be a [null]=[], or could new_value
					const auto new_value2 = improve_value_type(new_value, existing_value.get_type());
					const auto existing_value2 = improve_value_type(existing_value.get_type(), new_value2);

					if(existing_value2 != new_value2){
						throw std::runtime_error("Types not compatible in bind.");
					}
//					*existing_value_deep_ptr = std::pair<value_t, bool>(new_value2, existing_variable_is_mutable);
				}
				else{
					throw std::runtime_error("Cannot assign to immutable identifier.");
				}
			}

			//	Deduce type and bind it -- to local env.
			else{
				const auto new_value = result_value.get_literal();

				//	Can we deduce type from the rhs value?
				if(new_value.get_type() == typeid_t::make_vector(typeid_t::make_null())){
					throw std::runtime_error("Cannot deduce vector type.");
				}
				else if(new_value.get_type() == typeid_t::make_dict(typeid_t::make_null())){
					throw std::runtime_error("Cannot deduce dictionary type.");
				}
				else{
//					vm_acc._call_stack.back()->_symbols[name] = std::pair<value_t, bool>(new_value, false);
					vm_acc._call_stack.back()->_symbols[name] = symbol_t{ symbol_t::type::immutable_local, new_value.get_type(), {} };
				}
			}
		}
	}
	return { vm_acc, statement_result_t::make_no_output() };
}

std::pair<analyser_t, statement_result_t> analyse_return_statement(const analyser_t& vm, const statement_t::return_statement_t& statement){
	QUARK_ASSERT(vm.check_invariant());

	auto vm_acc = vm;
	const auto expr = statement._expression;
	const auto result = analyse_expression(vm_acc, expr);
	vm_acc = result.first;

	const auto result_value = result.second;

	if(!result_value.is_literal()){
		throw std::runtime_error("undefined");
	}

	//	Check that return value's type matches function's return type. Cannot be done here since we don't know who called us.
	//	Instead calling code must check.
	return {
		vm_acc,
		statement_result_t::make_return_unwind(result_value.get_literal())
	};
}

//??? make structs unique even though they share layout and name. USer unique-ID-generator?
std::pair<analyser_t, statement_result_t> analyse_def_struct_statement(const analyser_t& vm, const statement_t::define_struct_statement_t& statement){
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
//	vm_acc._call_stack.back()->_symbols[struct_name] = std::pair<value_t, bool>(struct_typeid_value, false);
	vm_acc._call_stack.back()->_symbols[struct_name] = symbol_t::make_constant(struct_typeid_value);
	return { vm_acc, statement_result_t::make_no_output() };
}

std::pair<analyser_t, statement_result_t> analyse_ifelse_statement(const analyser_t& vm, const statement_t::ifelse_statement_t& statement){
	QUARK_ASSERT(vm.check_invariant());

	auto vm_acc = vm;

	const auto condition_result = analyse_expression(vm_acc, statement._condition);
	vm_acc = condition_result.first;
	const auto condition_result_value = condition_result.second;
	if(!condition_result_value.is_literal() || !condition_result_value.get_literal().is_bool()){
		throw std::runtime_error("Boolean condition required.");
	}

	bool r = condition_result_value.get_literal().get_bool_value();
	if(r){
		return analyse_statements_in_env(vm_acc, statement._then_statements, {});
	}
	else{
		return analyse_statements_in_env(vm_acc, statement._else_statements, {});
	}
}

std::pair<analyser_t, statement_result_t> analyse_for_statement(const analyser_t& vm, const statement_t::for_statement_t& statement){
	QUARK_ASSERT(vm.check_invariant());

	auto vm_acc = vm;

	const auto start_value0 = analyse_expression(vm_acc, statement._start_expression);
	vm_acc = start_value0.first;
	const auto start_value = start_value0.second.get_literal().get_int_value();

	const auto end_value0 = analyse_expression(vm_acc, statement._end_expression);
	vm_acc = end_value0.first;
	const auto end_value = end_value0.second.get_literal().get_int_value();

	for(int x = start_value ; x <= end_value ; x++){
		const auto iterator_symbol = symbol_t::make_immutable_local(typeid_t::make_int());
		const std::map<std::string, symbol_t> symbols = { { statement._iterator_name, iterator_symbol} };

		const auto result = analyse_statements_in_env(vm_acc, statement._body, symbols);
		vm_acc = result.first;
		const auto return_value = result.second;
		if(return_value._type == statement_result_t::k_return_unwind){
			return { vm_acc, return_value };
		}
	}
	return { vm_acc, statement_result_t::make_no_output() };
}

std::pair<analyser_t, statement_result_t> analyse_while_statement(const analyser_t& vm, const statement_t::while_statement_t& statement){
	QUARK_ASSERT(vm.check_invariant());

	auto vm_acc = vm;

	bool again = true;
	while(again){
		const auto condition_value_expr = analyse_expression(vm_acc, statement._condition);
		vm_acc = condition_value_expr.first;
		const auto condition_value = condition_value_expr.second.get_literal().get_bool_value();

		if(condition_value){
			const auto result = analyse_statements_in_env(vm_acc, statement._body, {});
			vm_acc = result.first;
			const auto return_value = result.second;
			if(return_value._type == statement_result_t::k_return_unwind){
				return { vm_acc, return_value };
			}
		}
		else{
			again = false;
		}
	}
	return { vm_acc, statement_result_t::make_no_output() };
}

std::pair<analyser_t, statement_result_t> analyse_expression_statement(const analyser_t& vm, const statement_t::expression_statement_t& statement){
	QUARK_ASSERT(vm.check_invariant());

	auto vm_acc = vm;
	const auto result = analyse_expression(vm_acc, statement._expression);
	vm_acc = result.first;
	const auto result_value = result.second.get_literal();
	return {
		vm_acc,
		statement_result_t::make_passive_expression_output(result_value)
	};
}


//	Output is the RETURN VALUE of the analysed statement, if any.
std::pair<analyser_t, statement_result_t> analyse_statement(const analyser_t& vm, const statement_t& statement){
	QUARK_ASSERT(vm.check_invariant());
	QUARK_ASSERT(statement.check_invariant());

	if(statement._bind_or_assign){
		return analyse_bind_statement(vm, *statement._bind_or_assign);
	}
	else if(statement._block){
		return analyse_statements_in_env(vm, statement._block->_statements, {});
	}
	else if(statement._return){
		return analyse_return_statement(vm, *statement._return);
	}
	else if(statement._def_struct){
		return analyse_def_struct_statement(vm, *statement._def_struct);
	}
	else if(statement._if){
		return analyse_ifelse_statement(vm, *statement._if);
	}
	else if(statement._for){
		return analyse_for_statement(vm, *statement._for);
	}
	else if(statement._while){
		return analyse_while_statement(vm, *statement._while);
	}
	else if(statement._expression){
		return analyse_expression_statement(vm, *statement._expression);
	}
	else{
		QUARK_ASSERT(false);
		throw std::exception();
	}
}



}	//	unnamed


std::pair<analyser_t, statement_result_t> analyse_statements(const analyser_t& vm, const vector<shared_ptr<statement_t>>& statements){
	QUARK_ASSERT(vm.check_invariant());
	for(const auto i: statements){ QUARK_ASSERT(i->check_invariant()); };

	auto vm_acc = vm;

	int statement_index = 0;
	while(statement_index < statements.size()){
		const auto statement = statements[statement_index];
		const auto& r = analyse_statement(vm_acc, *statement);
		vm_acc = r.first;
		if(r.second._type == statement_result_t::k_return_unwind){
			return { vm_acc, r.second };
		}
		else{

			//	Last statement outputs its value, if any. This is passive output, not a return-unwind.
			if(statement_index == (statements.size() - 1)){
				if(r.second._type == statement_result_t::k_passive_expression_output){
					return { vm_acc, r.second };
				}
			}
			else{
			}

			statement_index++;
		}
	}
	return { vm_acc, statement_result_t::make_no_output() };
}

//	Warning: returns reference to the found value-entry -- this could be in any environment in the call stack.
symbol_t* resolve_env_variable_deep(const analyser_t& vm, const shared_ptr<environment_t>& env, const std::string& s){
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
	if(parent_expr.second.is_literal() && parent_expr.second.get_literal().is_struct()){
		const auto struct_instance = parent_expr.second.get_literal().get_struct_value();

		int index = find_struct_member_index(struct_instance->_def, expr._member_name);
		if(index == -1){
			throw std::runtime_error("Unknown struct member \"" + expr._member_name + "\".");
		}
		const value_t value = struct_instance->_member_values[index];
		return { vm_acc, expression_t::make_literal(value)};
	}
	else{
		throw std::runtime_error("Resolve struct member failed.");
	}
}

std::pair<analyser_t, expression_t> analyse_lookup_element_expression(const analyser_t& vm, const expression_t::lookup_expr_t& expr){
	QUARK_ASSERT(vm.check_invariant());

	auto vm_acc = vm;
	const auto parent_expr = analyse_expression(vm_acc, *expr._parent_address);
	vm_acc = parent_expr.first;

	if(parent_expr.second.is_literal() == false){
		throw std::runtime_error("Cannot compute lookup parent.");
	}
	else{
		const auto key_expr = analyse_expression(vm_acc, *expr._lookup_key);
		vm_acc = key_expr.first;

		if(key_expr.second.is_literal() == false){
			throw std::runtime_error("Cannot compute lookup key.");
		}//??? add debug code to validate all collection values, struct members - are the correct type.
		else{
			const auto parent_value = parent_expr.second.get_literal();
			const auto key_value = key_expr.second.get_literal();

			if(parent_value.is_string()){
				if(key_value.is_int() == false){
					throw std::runtime_error("Lookup in string by index-only.");
				}
				else{
					const auto instance = parent_value.get_string_value();
					int lookup_index = key_value.get_int_value();
					if(lookup_index < 0 || lookup_index >= instance.size()){
						throw std::runtime_error("Lookup in string: out of bounds.");
					}
					else{
						const char ch = instance[lookup_index];
						const auto value2 = value_t::make_string(string(1, ch));
						return { vm_acc, expression_t::make_literal(value2)};
					}
				}
			}
			else if(parent_value.is_json_value()){
				const auto parent_json_value = parent_value.get_json_value();
				if(parent_json_value.is_object()){
					if(key_value.is_string() == false){
						throw std::runtime_error("Lookup in json_value object by string-key only.");
					}
					else{
						const auto lookup_key = key_value.get_string_value();

						//	get_object_element() throws if key can't be found.
						const auto value = parent_json_value.get_object_element(lookup_key);
						const auto value2 = value_t::make_json_value(value);
						return { vm_acc, expression_t::make_literal(value2)};
					}
				}
				else if(parent_json_value.is_array()){
					if(key_value.is_int() == false){
						throw std::runtime_error("Lookup in json_value array by integer index only.");
					}
					else{
						const auto lookup_index = key_value.get_int_value();

						if(lookup_index < 0 || lookup_index >= parent_json_value.get_array_size()){
							throw std::runtime_error("Lookup in json_value array: out of bounds.");
						}
						else{
							const auto value = parent_json_value.get_array_n(lookup_index);
							const auto value2 = value_t::make_json_value(value);
							return { vm_acc, expression_t::make_literal(value2)};
						}
					}
				}
				else{
					throw std::runtime_error("Lookup using [] on json_value only works on objects and arrays.");
				}
			}
			else if(parent_value.is_vector()){
				if(key_value.is_int() == false){
					throw std::runtime_error("Lookup in vector by index-only.");
				}
				else{
					const auto instance = parent_value.get_vector_value();

					int lookup_index = key_value.get_int_value();
					if(lookup_index < 0 || lookup_index >= instance->_elements.size()){
						throw std::runtime_error("Lookup in vector: out of bounds.");
					}
					else{
						const value_t value = instance->_elements[lookup_index];
						return { vm_acc, expression_t::make_literal(value)};
					}
				}
			}
			else if(parent_value.is_dict()){
				if(key_value.is_string() == false){
					throw std::runtime_error("Lookup in dict by string-only.");
				}
				else{
					const auto instance = parent_value.get_dict_value();
					const string key = key_value.get_string_value();

					const auto found_it = instance->_elements.find(key);
					if(found_it == instance->_elements.end()){
						throw std::runtime_error("Lookup in dict: key not found.");
					}
					else{
						const value_t value = found_it->second;
						return { vm_acc, expression_t::make_literal(value)};
					}
				}
			}
			else {
				throw std::runtime_error("Lookup using [] only works with strings, vectors, dicts and json_value.");
			}
		}
	}
}
std::pair<analyser_t, expression_t> analyse_variabele_expression(const analyser_t& vm, const expression_t& e){
	QUARK_ASSERT(vm.check_invariant());

	const auto expr = *e.get_variable();

	auto vm_acc = vm;
	const auto value = resolve_env_symbol2(vm_acc, expr._variable);
	if(value != nullptr){
		return {vm_acc, expression_t::make_variable_expression(expr._variable, make_shared<typeid_t>(value->_value_type)) };
	}
	else{
		throw std::runtime_error("Undefined variable \"" + expr._variable + "\"!");
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
		std::vector<value_t> elements2;
		for(const auto m: elements){
			const auto element_expr = analyse_expression(vm_acc, m);
			vm_acc = element_expr.first;
			if(element_expr.second.is_literal() == false){
				throw std::runtime_error("Cannot analyse element of vector definition!");
			}

			const auto element = element_expr.second.get_literal();
			elements2.push_back(element);
		}

		//	If we don't have an explicit element type, deduct it from first element.
		const auto element_type2 = element_type.is_null() ? elements2[0].get_type() : element_type;
		for(const auto m: elements2){
			if((m.get_type() == element_type2) == false){
				throw std::runtime_error("Vector can not hold elements of different type!");
			}
		}
		return {vm_acc, expression_t::make_literal(value_t::make_vector_value(element_type2, elements2))};
	}
}



std::pair<analyser_t, expression_t> analyse_dict_definition_expression(const analyser_t& vm, const expression_t::dict_definition_exprt_t& expr){
	QUARK_ASSERT(vm.check_invariant());

	auto vm_acc = vm;

	const auto& elements = expr._elements;
	typeid_t value_type = expr._value_type;

	if(elements.empty()){
		return {vm_acc, expression_t::make_literal(value_t::make_dict_value(value_type, {}))};
	}
	else{
		std::map<string, value_t> elements2;
		for(const auto m: elements){
			const auto element_expr = analyse_expression(vm_acc, m.second);
			vm_acc = element_expr.first;

			if(element_expr.second.is_literal() == false){
				throw std::runtime_error("Cannot analyse element of vector definition!");
			}

			const auto element = element_expr.second.get_literal();

			const string key_string = m.first;
			elements2[key_string] = element;
		}

		//	If we have no value-type, deduct it from first element.
		const auto value_type2 = value_type.is_null() ? elements2.begin()->second.get_type() : value_type;

/*
		for(const auto m: elements2){
			if((m.second.get_type() == value_type2) == false){
				throw std::runtime_error("Dict can not hold elements of different type!");
			}
		}
*/
		return {vm_acc, expression_t::make_literal(value_t::make_dict_value(value_type2, elements2))};
	}
}

std::pair<analyser_t, expression_t> analyse_arithmetic_unary_minus_expression(const analyser_t& vm, const expression_t::unary_minus_expr_t& expr){
	QUARK_ASSERT(vm.check_invariant());

	auto vm_acc = vm;
	const auto& expr2 = analyse_expression(vm_acc, *expr._expr);
	vm_acc = expr2.first;

	if(expr2.second.is_literal()){
		const auto& c = expr2.second.get_literal();
		vm_acc = expr2.first;

		if(c.is_int()){
			return analyse_expression(
				vm_acc,
				expression_t::make_simple_expression__2(expression_type::k_arithmetic_subtract__2, expression_t::make_literal_int(0), expr2.second)
			);
		}
		else if(c.is_float()){
			return analyse_expression(
				vm_acc,
				expression_t::make_simple_expression__2(expression_type::k_arithmetic_subtract__2, expression_t::make_literal_float(0.0f), expr2.second)
			);
		}
		else{
			throw std::runtime_error("Unary minus won't work on expressions of type \"" + json_to_compact_string(typeid_to_ast_json(c.get_type())._value) + "\".");
		}
	}
	else{
		throw std::runtime_error("Could not analyse condition in conditional expression.");
	}
}


std::pair<analyser_t, expression_t> analyse_conditional_operator_expression(const analyser_t& vm, const expression_t::conditional_expr_t& expr){
	QUARK_ASSERT(vm.check_invariant());

	auto vm_acc = vm;

	//	Special-case since it uses 3 expressions & uses shortcut evaluation.
	const auto cond_result = analyse_expression(vm_acc, *expr._condition);
	vm_acc = cond_result.first;

	if(cond_result.second.is_literal() && cond_result.second.get_literal().is_bool()){
		const bool cond_flag = cond_result.second.get_literal().get_bool_value();

		//	!!! Only analyse the CHOSEN expression. Not that importan since functions are pure.
		if(cond_flag){
			return analyse_expression(vm_acc, *expr._a);
		}
		else{
			return analyse_expression(vm_acc, *expr._b);
		}
	}
	else{
		throw std::runtime_error("Could not analyse condition in conditional expression.");
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

	//	Both left and right are constants, replace the math_operation with a constant!
	if(left_expr.second.is_literal() == false || right_expr.second.is_literal() == false) {
		throw std::runtime_error("Left and right expressions must be same type!");
	}
	else{
		//	Perform math operation on the two constants => new constant.
		const auto left_type = left_expr.second.get_annotated_type();

		//	Make rhs match left if needed/possible.
		const auto right_type = improve_value_type(right_expr.second.get_annotated_type(), left_type);

		if(!(left_type == right_type)){
			throw std::runtime_error("Left and right expressions must be same type!");
		}
		else{
			//	Do generic functionallity, independant on type.
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
				QUARK_ASSERT(false);
				throw std::exception();
			}
			//??? always a bool!
			return {vm_acc, expression_t::make_simple_expression__2(e.get_operation(), left_expr.second, right_expr.second, make_shared<typeid_t>(typeid_t::make_bool())) };
		}
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
		return analyse_variabele_expression(vm, e);
	}

	else if(op == expression_type::k_call){
		return analyse_call_expression(vm, e);
	}

	else if(op == expression_type::k_define_function){
		const auto expr = e.get_function_definition();
		return {vm, expression_t::make_literal(value_t::make_function_value(expr->_def))};
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
std::pair<analyser_t, statement_result_t> call_function(const analyser_t& vm, const floyd::value_t& f, const vector<value_t>& args){
	QUARK_ASSERT(vm.check_invariant());
	QUARK_ASSERT(f.check_invariant());
	for(const auto i: args){ QUARK_ASSERT(i.check_invariant()); };

	auto vm_acc = vm;

	if(f.is_function() == false){
		throw std::runtime_error("Cannot call non-function.");
	}

	const auto& function_def = f.get_function_value()->_def;
	if(function_def._host_function != 0){
		const auto r = call_host_function(vm_acc, function_def._host_function, args);
		return { r.first, r.second };
	}
	else{
		const auto return_type = f.get_type().get_function_return();
		const auto arg_types = f.get_type().get_function_args();

		//	arity
		if(args.size() != arg_types.size()){
			throw std::runtime_error("Wrong number of arguments to function.");
		}
		for(int i = 0 ; i < args.size() ; i++){
			if(args[i].get_type() != arg_types[i]){
				throw std::runtime_error("Function argument type do not match.");
			}
		}

		//	Always use global scope.
		//	Future: support closures by linking to function env where function is defined.
		auto parent_env = vm_acc._call_stack[0];
		auto new_environment = environment_t::make_environment(vm_acc, parent_env);

		//	Copy input arguments to the function scope.
		for(int i = 0 ; i < function_def._args.size() ; i++){
			const auto& arg_name = function_def._args[i]._name;
			const auto& arg_value = args[i];

			//	Function arguments are immutable while executing the function body.
			new_environment->_symbols[arg_name] = std::pair<value_t, bool>(arg_value, false);
		}
		vm_acc._call_stack.push_back(new_environment);

//		QUARK_TRACE(json_to_pretty_string(analyser_to_json(vm_acc)));

		const auto r = analyse_statements(vm_acc, function_def._statements);

		vm_acc = r.first;
		vm_acc._call_stack.pop_back();

		if(r.second._type != statement_result_t::k_return_unwind){
			throw std::runtime_error("Function missing return statement");
		}
		else if(r.second._output.get_type().is_struct() == false && r.second._output.get_type() != return_type){
			throw std::runtime_error("Function return type wrong.");
		}
		else{
			return {vm_acc, r.second };
		}
	}
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

		if(call_expr_args.size() != callee_args.size()){
			throw std::runtime_error("Wrong number of arguments in function call.");
		}
		for(int i = 0 ; i < callee_args.size() ; i++){
			const auto arg_lhs = callee_args[i];
			const auto arg_rhs = call_expr_args[i].get_annotated_type();

			if(arg_lhs != arg_rhs){
				throw std::runtime_error("Argument type mismatch.");
			}
		}
		return { vm_acc, expression_t::make_call(callee_expr, call_expr_args) };
	}

	//	Attempting to call a TYPE? Then this may be a constructor call.
	else if(callee_type.is_typeid()){
//		const auto result = analyze_construct_value_from_typeid(vm_acc, callee_type.get_typeid_value(), arg_values);
//		vm_acc = result.first;
//???
		QUARK_ASSERT(false);
		return { vm_acc, expression_t::make_call(callee_expr, call_expr_args) };
	}
	else{
		throw std::runtime_error("Cannot call non-function.");
	}
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
	test__analyse_expression(
		expression_t::make_simple_expression__2(
			expression_type::k_arithmetic_add__2,
			expression_t::make_literal_int(1),
			expression_t::make_literal_int(2)
		),
		expression_t::make_simple_expression__2(
			expression_type::k_arithmetic_add__2,
			expression_t::make_literal_int(1),
			expression_t::make_literal_int(2)
		)
	);
}





//////////////////////////		environment_t



std::shared_ptr<environment_t> environment_t::make_environment(
	const analyser_t& vm,
	std::shared_ptr<environment_t>& parent_env
)
{
	QUARK_ASSERT(vm.check_invariant());

	auto f = environment_t{ parent_env, {} };
	return make_shared<environment_t>(f);
}

bool environment_t::check_invariant() const {
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


analyser_t::analyser_t(const ast_t& ast){
	QUARK_ASSERT(ast.check_invariant());

	_ast = make_shared<ast_t>(ast);

	//	Make the top-level environment = global scope.
	shared_ptr<environment_t> empty_env;
	auto global_env = environment_t::make_environment(*this, empty_env);



	//	??? Had problem having this a global constant: keyword_t::k_null had not be statically initialized.
	const vector<host_function_t> k_host_functions {
		host_function_t{ "print", 1000, typeid_t::make_function(typeid_t::make_null(), {typeid_t::make_null()}) },
		host_function_t{ "assert", 1001, typeid_t::make_function(typeid_t::make_null(), {typeid_t::make_null()}) },
		host_function_t{ "to_string", 1002, typeid_t::make_function(typeid_t::make_string(), {typeid_t::make_null()}) },
		host_function_t{ "to_pretty_string", 1003, typeid_t::make_function(typeid_t::make_string(), {typeid_t::make_null()}) },
		host_function_t{ "typeof", 1004, typeid_t::make_function(typeid_t::make_typeid(), {typeid_t::make_null()}) },

		host_function_t{ "get_time_of_day", 1005, typeid_t::make_function(typeid_t::make_int(), {}) },
		host_function_t{ "update", 1006, typeid_t::make_function(typeid_t::make_null(), {typeid_t::make_null(),typeid_t::make_null(),typeid_t::make_null()}) },
		host_function_t{ "size", 1007, typeid_t::make_function(typeid_t::make_null(), {typeid_t::make_null()}) },
		host_function_t{ "find", 1008, typeid_t::make_function(typeid_t::make_int(), {typeid_t::make_null(), typeid_t::make_null()}) },
		host_function_t{ "exists", 1009, typeid_t::make_function(typeid_t::make_bool(), {typeid_t::make_null(),typeid_t::make_null()}) },
		host_function_t{ "erase", 1010, typeid_t::make_function(typeid_t::make_null(), {typeid_t::make_null(),typeid_t::make_null()}) },
		host_function_t{ "push_back", 1011, typeid_t::make_function(typeid_t::make_null(), {typeid_t::make_null(),typeid_t::make_null()}) },
		host_function_t{ "subset", 1012, typeid_t::make_function(typeid_t::make_null(), {typeid_t::make_null(),typeid_t::make_null(),typeid_t::make_null()}) },
		host_function_t{ "replace", 1013, typeid_t::make_function(typeid_t::make_null(), {typeid_t::make_null(),typeid_t::make_null(),typeid_t::make_null(),typeid_t::make_null()}) },

		host_function_t{ "get_env_path", 1014, typeid_t::make_function(typeid_t::make_string(), {}) },
		host_function_t{ "read_text_file", 1015, typeid_t::make_function(typeid_t::make_string(), {typeid_t::make_null()}) },
		host_function_t{ "write_text_file", 1016, typeid_t::make_function(typeid_t::make_string(), {typeid_t::make_null(), typeid_t::make_null()}) },

		host_function_t{ "decode_json", 1017, typeid_t::make_function(typeid_t::make_string(), {typeid_t::make_null()}) },
		host_function_t{ "encode_json", 1018, typeid_t::make_function(typeid_t::make_string(), {typeid_t::make_null()}) },

		host_function_t{ "flatten_to_json", 1019, typeid_t::make_function(typeid_t::make_json_value(), {typeid_t::make_null()}) },
		host_function_t{ "unflatten_from_json", 1020, typeid_t::make_function(typeid_t::make_null(), {typeid_t::make_null(), typeid_t::make_null()}) },

		host_function_t{ "get_json_type", 10121, typeid_t::make_function(typeid_t::make_typeid(), {typeid_t::make_json_value()}) }
	};



	//	Insert built-in functions into AST.
	for(auto i = 0 ; i < k_host_functions.size() ; i++){
		const auto& hf = k_host_functions[i];
		const auto def = function_definition_t(
			{},
			i + 1000,
			hf._function_type.get_function_return()
		);

		const auto function_value = value_t::make_function_value(def);
		global_env->_symbols[hf._name] = symbol_t::make_constant(function_value);
	}
	global_env->_symbols[keyword_t::k_null] = symbol_t::make_constant(value_t::make_null());
	global_env->_symbols[keyword_t::k_bool] = symbol_t::make_constant(value_t::make_typeid_value(typeid_t::make_bool()));
	global_env->_symbols[keyword_t::k_int] = symbol_t::make_constant(value_t::make_typeid_value(typeid_t::make_int()));
	global_env->_symbols[keyword_t::k_float] = symbol_t::make_constant(value_t::make_typeid_value(typeid_t::make_float()));
	global_env->_symbols[keyword_t::k_string] = symbol_t::make_constant(value_t::make_typeid_value(typeid_t::make_string()));
	global_env->_symbols[keyword_t::k_typeid] = symbol_t::make_constant(value_t::make_typeid_value(typeid_t::make_typeid()));
	global_env->_symbols[keyword_t::k_json_value] = symbol_t::make_constant(value_t::make_typeid_value(typeid_t::make_json_value()));

	global_env->_symbols[keyword_t::k_json_object] = symbol_t::make_constant(value_t::make_int(1));
	global_env->_symbols[keyword_t::k_json_array] = symbol_t::make_constant(value_t::make_int(2));
	global_env->_symbols[keyword_t::k_json_string] = symbol_t::make_constant(value_t::make_int(3));
	global_env->_symbols[keyword_t::k_json_number] = symbol_t::make_constant(value_t::make_int(4));
	global_env->_symbols[keyword_t::k_json_true] = symbol_t::make_constant(value_t::make_int(5));
	global_env->_symbols[keyword_t::k_json_false] = symbol_t::make_constant(value_t::make_int(6));
	global_env->_symbols[keyword_t::k_json_null] = symbol_t::make_constant(value_t::make_int(7));

	_call_stack.push_back(global_env);

	_start_time = std::chrono::high_resolution_clock::now();

	//	Run static intialization (basically run global statements before calling main()).
	const auto r = analyse_statements(*this, _ast->_statements);

	_call_stack[0]->_symbols = r.first._call_stack[0]->_symbols;
	_print_output = r.first._print_output;
	QUARK_ASSERT(check_invariant());
}

analyser_t::analyser_t(const analyser_t& other) :
	_start_time(other._start_time),
	_ast(other._ast),
	_call_stack(other._call_stack),
	_print_output(other._print_output)
{
	QUARK_ASSERT(other.check_invariant());
	QUARK_ASSERT(check_invariant());
}


	//??? make proper operator=(). Exception safety etc.
const analyser_t& analyser_t::operator=(const analyser_t& other){
	_start_time = other._start_time;
	_ast = other._ast;
	_call_stack = other._call_stack;
	_print_output = other._print_output;
	return *this;
}

#if DEBUG
bool analyser_t::check_invariant() const {
	QUARK_ASSERT(_ast->check_invariant());
	return true;
}
#endif



/*
std::pair<analyser_t, statement_result_t> run_main(const interpreter_context_t& context, const string& source, const vector<floyd::value_t>& args){

	auto ast = program_to_ast2(context, source);

	//	Runs global code.
	auto vm = analyser_t(ast);

	const auto main_function = resolve_env_symbol2(vm, "main");
	if(main_function != nullptr){
		const auto result = call_function(vm, main_function->first, args);
		return { result.first, result.second };
	}
	else{
		return {vm, statement_result_t::make_no_output()};
	}
}

std::pair<analyser_t, statement_result_t> run_program(const interpreter_context_t& context, const ast_t& ast, const vector<floyd::value_t>& args){
	auto vm = analyser_t(ast);

	const auto main_func = resolve_env_symbol2(vm, "main");
	if(main_func != nullptr){
		const auto r = call_function(vm, main_func->first, args);
		return { r.first, r.second };
	}
	else{
		return { vm, statement_result_t::make_no_output() };
	}
}
*/



#if 0
floyd::ast_t run_pass3(const quark::trace_context_t& tracer, const floyd::ast_t& ast_pass2){
	QUARK_ASSERT(ast_pass2.check_invariant());

	QUARK_CONTEXT_SCOPED_TRACE(tracer, "pass3");

	QUARK_CONTEXT_TRACE(tracer, json_to_pretty_string(ast_to_json(ast_pass2)._value));
	analyser_t a(ast_pass2);

//		public: std::shared_ptr<const floyd::ast_t> _ast;

	const auto result = *a._ast;

	QUARK_ASSERT(statement_t::is_annotated_deep(result._statements));
	return result;
}
#else
floyd::ast_t run_pass3(const quark::trace_context_t& tracer, const floyd::ast_t& ast_pass2){
	return ast_pass2;
}
#endif



}	//	floyd

