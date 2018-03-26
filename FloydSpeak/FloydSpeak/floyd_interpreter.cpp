//
//  parser_evaluator.cpp
//  FloydSpeak
//
//  Created by Marcus Zetterquist on 26/07/16.
//  Copyright © 2016 Marcus Zetterquist. All rights reserved.
//

#include "floyd_interpreter.h"

#include "parser_primitives.h"
#include "floyd_parser.h"
#include "ast_value.h"
#include "pass2.h"
#include "pass3.h"
#include "bytecode_gen.h"
#include "json_support.h"
#include "json_parser.h"

#include <cmath>
#include <sys/time.h>

#include <thread>
#include <chrono>
#include <algorithm>
#include <iostream>
#include <fstream>
#include "text_parser.h"
#include "host_functions.hpp"

namespace floyd {

using std::vector;
using std::string;
using std::pair;
using std::shared_ptr;
using std::make_shared;




bc_value_t execute_expression(interpreter_t& vm, const bc_expression_t& e);
statement_result_t execute_body(interpreter_t& vm, const bc_body_t& body, const std::vector<bc_value_t>& init_values);
statement_result_t execute_body(interpreter_t& vm, const bc_body_t& body);

/*
typeid_t find_type_by_name(const interpreter_t& vm, const typeid_t& type){
	if(type.get_base_type() == base_type::k_internal_unresolved_type_identifier){
		const auto v = find_symbol_by_name(vm, type.get_unresolved_type_identifier());
		if(v){
			if(v->_symbol._value_type.is_typeid()){
				return v->_value.get_typeid_value();
			}
			else{
				return typeid_t::make_undefined();
			}
		}
		else{
			return typeid_t::make_undefined();
		}
	}
	else{
		return type;
	}
}
*/


/*
//	Warning: returns reference to the found value-entry -- this could be in any environment in the call stack.
std::shared_ptr<value_entry_t> find_symbol_by_name_deep(const interpreter_t& vm, int depth, const std::string& s){
	QUARK_ASSERT(vm.check_invariant());
	QUARK_ASSERT(depth >= 0 && depth < vm._call_stack.size());
	QUARK_ASSERT(s.size() > 0);

	const auto& env = &vm._call_stack[depth];
    const auto& it = std::find_if(
    	env->_body_ptr->_symbols.begin(),
    	env->_body_ptr->_symbols.end(),
    	[&s](const std::pair<std::string, symbol_t>& e) { return e.first == s; }
	);
	if(it != env->_body_ptr->_symbols.end()){
		const auto index = it - env->_body_ptr->_symbols.begin();
		const auto pos = env->_values_offset + index;
		QUARK_ASSERT(pos >= 0 && pos < vm._value_stack.size());

		//	Assumes we are scanning from the top of the stack.
		int parent_steps = static_cast<int>(vm._call_stack.size() - 1 - depth);

		const auto value_entry = value_entry_t{
			vm._value_stack[pos],
			it->first,
			it->second,
			variable_address_t::make_variable_address(parent_steps, static_cast<int>(index))
		};
		return make_shared<value_entry_t>(value_entry);
	}
	else if(depth > 0){
		return find_symbol_by_name_deep(vm, depth - 1, s);
	}
	else{
		return nullptr;
	}
}

//	Warning: returns reference to the found value-entry -- this could be in any environment in the call stack.
std::shared_ptr<value_entry_t> find_symbol_by_name(const interpreter_t& vm, const std::string& s){
	QUARK_ASSERT(vm.check_invariant());
	QUARK_ASSERT(s.size() > 0);

	return find_symbol_by_name_deep(vm, (int)(vm._call_stack.size() - 1), s);
}
*/

std::shared_ptr<value_entry_t> find_global_symbol2(const interpreter_t& vm, const std::string& s){
	QUARK_ASSERT(vm.check_invariant());
	QUARK_ASSERT(s.size() > 0);

	const auto& symbols = vm._imm->_program._globals._symbols;
    const auto& it = std::find_if(
    	symbols.begin(),
    	symbols.end(),
    	[&s](const std::pair<std::string, symbol_t>& e) { return e.first == s; }
	);
	if(it != symbols.end()){
		const auto index = it - symbols.begin();
		const auto pos = index;
		QUARK_ASSERT(pos >= 0 && pos < vm._value_stack.size());

		const auto value_entry = value_entry_t{
			vm._value_stack[pos],
			it->first,
			it->second,
			variable_address_t::make_variable_address(-1, static_cast<int>(index))
		};
		return make_shared<value_entry_t>(value_entry);
	}
	else{
		return nullptr;
	}
}
floyd::value_t find_global_symbol(const interpreter_t& vm, const string& s){
	return get_global(vm, s);
}
value_t get_global(const interpreter_t& vm, const std::string& name){
	const auto& result = find_global_symbol2(vm, name);
	if(result == nullptr){
		throw std::runtime_error("Cannot find global.");
	}
	else{
		return bc_to_value(result->_value, result->_symbol._value_type);
	}
}

inline const bc_function_definition_t& get_function_def(const interpreter_t& vm, const floyd::bc_value_t& v){
//	QUARK_ASSERT(v.is_function());

	const auto function_id = v.get_function_value();
	QUARK_ASSERT(function_id >= 0 && function_id < vm._imm->_program._function_defs.size())

	const auto& function_def = vm._imm->_program._function_defs[function_id];
	return function_def;
}

inline environment_t* find_env_from_address(interpreter_t& vm, int parent_step){
	if(parent_step == -1){
		return &vm._call_stack[0];
	}
	else{
		return &vm._call_stack[vm._call_stack.size() - 1 - parent_step];
	}
}

inline const environment_t* find_env_from_address(const interpreter_t& vm, int parent_step){
	if(parent_step == -1){
		return &vm._call_stack[0];
	}
	else{
		return &vm._call_stack[vm._call_stack.size() - 1 - parent_step];
	}
}

//??? split into one-argument and multi-argument opcodes.
bc_value_t construct_value_from_typeid(interpreter_t& vm, const typeid_t& type, const typeid_t& arg0_type, const vector<bc_value_t>& arg_values){
	QUARK_ASSERT(vm.check_invariant());

	if(type.is_json_value()){
		QUARK_ASSERT(arg_values.size() == 1);

		const auto& arg0 = arg_values[0];
		const auto arg = bc_to_value(arg0, arg0_type);
		const auto value = value_to_ast_json(arg, json_tags::k_plain);
		return bc_value_t::make_json_value(value._value);
	}
	else if(type.is_bool() || type.is_int() || type.is_float() || type.is_string() || type.is_typeid()){
		QUARK_ASSERT(arg_values.size() == 1);

		const auto& arg = arg_values[0];
		if(type.is_string()){
			if(arg0_type.is_json_value() && arg.get_json_value().is_string()){
				return bc_value_t::make_string(arg.get_json_value().get_string());
			}
			else if(arg0_type.is_string()){
			}
		}
		else{
			if(arg0_type != type){
			}
		}
		return arg;
	}
	else if(type.is_struct()){
/*
	#if DEBUG
		const auto def = type.get_struct_ref();
		QUARK_ASSERT(arg_values.size() == def->_members.size());

		for(int i = 0 ; i < def->_members.size() ; i++){
			const auto v = arg_values[i];
			const auto a = def->_members[i];
			QUARK_ASSERT(v.check_invariant());
			QUARK_ASSERT(v.get_type().get_base_type() != base_type::k_internal_unresolved_type_identifier);
			QUARK_ASSERT(v.get_type() == a._type);
		}
	#endif
*/
		const auto instance = bc_value_t::make_struct_value(type, arg_values);
		QUARK_TRACE(to_compact_string2(instance));

		return instance;
	}
	else if(type.is_vector()){
		const auto& element_type = type.get_vector_element_type();
		QUARK_ASSERT(element_type.is_undefined() == false);

		return bc_value_t::make_vector_value(element_type, arg_values);
	}
	else if(type.is_dict()){
		const auto& element_type = type.get_dict_value_type();
		QUARK_ASSERT(element_type.is_undefined() == false);

		std::map<string, bc_value_t> m;
		for(auto i = 0 ; i < arg_values.size() / 2 ; i++){
			const auto& key = arg_values[i * 2 + 0].get_string_value();
			const auto& value = arg_values[i * 2 + 1];
			m.insert({ key, value });
		}
		return bc_value_t::make_dict_value(element_type, m);
	}
	else if(type.is_function()){
	}
	else if(type.is_unresolved_type_identifier()){
	}
	else{
	}

	QUARK_ASSERT(false);
	throw std::exception();
}

value_t call_function(interpreter_t& vm, const floyd::value_t& f, const vector<value_t>& args){
#if DEBUG
	QUARK_ASSERT(vm.check_invariant());
	QUARK_ASSERT(f.check_invariant());
	for(const auto i: args){ QUARK_ASSERT(i.check_invariant()); };
	QUARK_ASSERT(f.is_function());
#endif

	const auto& function_def = get_function_def(vm, value_to_bc(f));
	if(function_def._host_function_id != 0){
		const auto& r = call_host_function(vm, function_def._host_function_id, args);
		return r;
	}
	else{
#if DEBUG
		const auto& arg_types = f.get_type().get_function_args();

		//	arity
		QUARK_ASSERT(args.size() == arg_types.size());

		for(int i = 0 ; i < args.size() ; i++){
			if(args[i].get_type() != arg_types[i]){
				QUARK_ASSERT(false);
			}
		}
#endif

		const auto& r = execute_body(vm, function_def._body, values_to_bcs(args));
		return bc_to_value(r._output, f.get_type().get_function_return());
	}
}



//////////////////////////////////////////		STATEMENTS

QUARK_UNIT_TEST("", "", "", ""){
	const auto s = sizeof(bc_instruction_t);
	QUARK_UT_VERIFY(s == 64);
}



QUARK_UNIT_TEST("", "", "", ""){
	const auto s = sizeof(bc_expression_t);
//	QUARK_UT_VERIFY(s == 56);


	const auto opcode_offset = offsetof(bc_expression_t, _opcode);
//	QUARK_UT_VERIFY(opcode_offset == 8);

	const auto opcode_size = sizeof(bc_expression_opcode);
	QUARK_UT_VERIFY(opcode_size == 1);


	auto temp = bc_expression_t(
		bc_expression_opcode::k_expression_load,
		13,
		{},
		variable_address_t::make_variable_address(10, 11),
		bc_value_t::make_int(1234),
		16
	);
	QUARK_ASSERT(temp.check_invariant());
//	QUARK_UT_VERIFY(sizeof(temp) == 56);
}

QUARK_UNIT_TEST("", "", "", ""){
	const auto s = sizeof(std::vector<bc_expression_t>);
	QUARK_UT_VERIFY(s == 24);
}
QUARK_UNIT_TEST("", "", "", ""){
	const auto s = sizeof(variable_address_t);
	QUARK_UT_VERIFY(s == 8);
}
QUARK_UNIT_TEST("", "", "", ""){
	const auto s = sizeof(bc_value_t);
//	QUARK_UT_VERIFY(s == 16);
}





statement_result_t execute_statements(interpreter_t& vm, const std::vector<bc_instruction_t>& statements){
	QUARK_ASSERT(vm.check_invariant());

	for(const auto& statement: statements){
		QUARK_ASSERT(vm.check_invariant());
		QUARK_ASSERT(statement.check_invariant());

		const auto opcode = statement._opcode;
		if(opcode == bc_statement_opcode::k_statement_store){
			const auto& rhs_value = execute_expression(vm, statement._e[0]);
			const auto& env = find_env_from_address(vm, statement._v._parent_steps);
			const auto pos = env->_values_offset + statement._v._index;
			QUARK_ASSERT(pos >= 0 && pos < vm._value_stack.size());
			vm._value_stack[pos] = rhs_value;
		}
		else if(opcode == bc_statement_opcode::k_statement_block){
			const auto& r = execute_body(vm, statement._b[0]);
			if(r._type == statement_result_t::k_returning){
				return r;
			}
		}
		else if(opcode == bc_statement_opcode::k_statement_return){
			const auto& expr = statement._e[0];
			const auto& rhs_value = execute_expression(vm, expr);
			return statement_result_t::make_return_unwind(rhs_value);
		}
		else if(opcode == bc_statement_opcode::k_statement_if){
			const auto& condition_result_value = execute_expression(vm, statement._e[0]);

			bool flag = condition_result_value.get_bool_value_quick();
			if(flag){
				const auto& r = execute_body(vm, statement._b[0]);
				if(r._type == statement_result_t::k_returning){
					return r;
				}
			}
			else{
				const auto& r = execute_body(vm, statement._b[1]);
				if(r._type == statement_result_t::k_returning){
					return r;
				}
			}
		}
		else if(opcode == bc_statement_opcode::k_statement_for){
			const auto& start_value0 = execute_expression(vm, statement._e[0]);
			const auto start_value_int = start_value0.get_int_value_quick();

			const auto& end_value0 = execute_expression(vm, statement._e[1]);
			const auto end_value_int = end_value0.get_int_value_quick();

			const auto& body = statement._b[0];

			const auto values_offset = vm._value_stack.size();
			vm._value_stack.push_back(bc_value_t::make_undefined());

			//	These are constants (can be reused for every iteration of the for-loop, or memory slots = also reuse).
			if(body._symbols.empty() == false){
				for(vector<bc_value_t>::size_type i = 0 ; i < body._symbols.size() ; i++){
					const auto& symbol = body._symbols[i];
					vm._value_stack.push_back(value_to_bc(symbol.second._const_value));
				}
			}
			vm._call_stack.push_back(environment_t{ values_offset });

			//??? simplify code -- create a count instead.
			//	open-range
			if(statement._param_x == 0){
				for(int x = start_value_int ; x < end_value_int ; x++){
					vm._value_stack[values_offset + 0] = bc_value_t::make_int(x);

					//### If statements don't have a RETURN, then we don't need to check for it. Make two loops?
					const auto& return_value = execute_statements(vm, body._statements);

					if(return_value._type == statement_result_t::k_returning){
						vm._call_stack.pop_back();
						vm._value_stack.resize(values_offset);
						return return_value;
					}
				}
			}

			//	closed-range
			else if(statement._param_x == 1){
				for(int x = start_value_int ; x <= end_value_int ; x++){
					vm._value_stack[values_offset + 0] = bc_value_t::make_int(x);

					//### If statements don't have a RETURN, then we don't need to check for it. Make two loops?
					const auto& return_value = execute_statements(vm, body._statements);

					if(return_value._type == statement_result_t::k_returning){
						vm._call_stack.pop_back();
						vm._value_stack.resize(values_offset);
						return return_value;
					}
				}
			}
			else{
				QUARK_ASSERT(false);
			}
			vm._call_stack.pop_back();
			vm._value_stack.resize(values_offset);
		}
		else if(opcode == bc_statement_opcode::k_statement_while){
			bool again = true;
			while(again){
				const auto& condition_value_expr = execute_expression(vm, statement._e[0]);
				const auto& condition_value = condition_value_expr.get_bool_value_quick();

				if(condition_value){
					const auto& return_value = execute_body(vm, statement._b[0]);
					if(return_value._type == statement_result_t::k_returning){
						return return_value;
					}
				}
				else{
					again = false;
				}
			}
		}
		else if(opcode == bc_statement_opcode::k_statement_expression){
			/*const auto& rhs_value =*/ execute_expression(vm, statement._e[0]);
		}
		else{
			QUARK_ASSERT(false);
			throw std::exception();
		}
	}
	return statement_result_t::make__complete_without_value();
}

statement_result_t execute_body(interpreter_t& vm, const bc_body_t& body, const std::vector<bc_value_t>& init_values){
	QUARK_ASSERT(vm.check_invariant());

	const auto values_offset = vm._value_stack.size();
	if(init_values.empty() == false){
		for(const auto& e: init_values){
			vm._value_stack.push_back(e);
		}
	}
	if(body._symbols.empty() == false){
		for(vector<bc_value_t>::size_type i = init_values.size() ; i < body._symbols.size() ; i++){
			const auto& symbol = body._symbols[i];
			vm._value_stack.push_back(value_to_bc(symbol.second._const_value));
		}
	}
	vm._call_stack.push_back(environment_t{ values_offset });

	const auto& r = execute_statements(vm, body._statements);
	vm._call_stack.pop_back();
	vm._value_stack.resize(values_offset);

	return r;
}
statement_result_t execute_body(interpreter_t& vm, const bc_body_t& body){
	QUARK_ASSERT(vm.check_invariant());

	const auto values_offset = vm._value_stack.size();
	if(body._symbols.empty() == false){
		for(vector<bc_value_t>::size_type i = 0 ; i < body._symbols.size() ; i++){
			const auto& symbol = body._symbols[i];
			vm._value_stack.push_back(value_to_bc(symbol.second._const_value));
		}
	}
	vm._call_stack.push_back(environment_t{ values_offset });

	const auto& r = execute_statements(vm, body._statements);
	vm._call_stack.pop_back();
	vm._value_stack.resize(values_offset);

	return r;
}



//////////////////////////////////////////		EXPRESSIONS


inline const bc_typeid2_t& get_type(const interpreter_t& vm, const bc_typeid_t& type){
	return vm._imm->_program._types[type];
}
inline const base_type get_basetype(const interpreter_t& vm, const bc_typeid_t& type){
	return vm._imm->_program._types[type].get_base_type();
}


bc_value_t execute_resolve_member_expression(interpreter_t& vm, const bc_expression_t& expr){
	QUARK_ASSERT(vm.check_invariant());

	const auto& parent_expr = execute_expression(vm, expr._e[0]);
	QUARK_ASSERT(get_type(vm, expr._e[0]._type).get_base_type() == base_type::k_struct);

	const auto& struct_instance = parent_expr.get_struct_value();
	int index = expr._address_index;
	QUARK_ASSERT(index != -1);

	const bc_value_t value = struct_instance[index];
	return value;
}

bc_value_t execute_lookup_element_expression(interpreter_t& vm, const bc_expression_t& expr){
	QUARK_ASSERT(vm.check_invariant());

	const auto& parent_value = execute_expression(vm, expr._e[0]);
	const auto parent_type = get_basetype(vm, expr._e[0]._type);

	const auto& key_value = execute_expression(vm, expr._e[1]);
#if DEBUG
	const auto key_type = get_basetype(vm, expr._e[1]._type);
#endif


	if(parent_type == base_type::k_string){
		const auto& instance = parent_value.get_string_value();
		int lookup_index = key_value.get_int_value_quick();
		if(lookup_index < 0 || lookup_index >= instance.size()){
			throw std::runtime_error("Lookup in string: out of bounds.");
		}
		else{
			const char ch = instance[lookup_index];
			const auto value2 = bc_value_t::make_string(string(1, ch));
			return value2;
		}
	}
	else if(parent_type == base_type::k_json_value){
		//	Notice: the exact type of value in the json_value is only known at runtime = must be checked in interpreter.
		const auto& parent_json_value = parent_value.get_json_value();
		if(parent_json_value.is_object()){
			QUARK_ASSERT(key_type == base_type::k_string);
			const auto& lookup_key = key_value.get_string_value();

			//	get_object_element() throws if key can't be found.
			const auto& value = parent_json_value.get_object_element(lookup_key);
			const auto value2 = bc_value_t::make_json_value(value);
			return value2;
		}
		else if(parent_json_value.is_array()){
			const auto& lookup_index = key_value.get_int_value_quick();

			if(lookup_index < 0 || lookup_index >= parent_json_value.get_array_size()){
				throw std::runtime_error("Lookup in json_value array: out of bounds.");
			}
			else{
				const auto& value = parent_json_value.get_array_n(lookup_index);
				const auto value2 = bc_value_t::make_json_value(value);
				return value2;
			}
		}
		else{
			throw std::runtime_error("Lookup using [] on json_value only works on objects and arrays.");
		}
	}
	else if(parent_type == base_type::k_vector){
		const auto& vec = parent_value.get_vector_value();

		int lookup_index = key_value.get_int_value_quick();
		if(lookup_index < 0 || lookup_index >= vec.size()){
			throw std::runtime_error("Lookup in vector: out of bounds.");
		}
		else{
			const bc_value_t value = vec[lookup_index];
			return value;
		}
	}
	else if(parent_type == base_type::k_dict){
		QUARK_ASSERT(key_type == base_type::k_string);

		const auto& entries = parent_value.get_dict_value();
		const string key = key_value.get_string_value();

		const auto& found_it = entries.find(key);
		if(found_it == entries.end()){
			throw std::runtime_error("Lookup in dict: key not found.");
		}
		else{
			const bc_value_t value = found_it->second;
			return value;
		}
	}
	else {
		QUARK_ASSERT(false);
		throw std::exception();
	}
}

bc_value_t execute_call_expression(interpreter_t& vm, const bc_expression_t& expr){
	QUARK_ASSERT(vm.check_invariant());
	QUARK_ASSERT(expr.check_invariant());

	const auto& function_value = execute_expression(vm, expr._e[0]);
	const auto& function_def = get_function_def(vm, function_value);

	//	_e[...] contains first callee, then each argument.
	const auto arg_count = expr._e.size() - 1;


	if(function_def._host_function_id != 0){
		const auto& host_function = vm._imm->_host_functions.at(function_def._host_function_id);

		//	arity
		//	QUARK_ASSERT(args.size() == host_function._function_type.get_function_args().size());

		std::vector<value_t> arg_values;
		arg_values.reserve(arg_count);

//		const auto& arg_types = function_def._function_type.get_function_args();

		for(int i = 0 ; i < arg_count ; i++){
			const auto& arg_expr = expr._e[i + 1];
			QUARK_ASSERT(arg_expr.check_invariant());

			const auto& t = execute_expression(vm, arg_expr);
			const auto t1 = bc_to_value(t, get_type(vm, arg_expr._type));
			arg_values.push_back(t1);
		}

		const auto& result = (host_function)(vm, arg_values);
		return value_to_bc(result);
	}
	else{
		const auto values_offset = vm._value_stack.size();

		//	Notice that arguments are first in the symbol list.
		const auto symbol_count = function_def._body._symbols.size();

		for(int i = 0 ; i < arg_count ; i++){
			const auto& arg_expr = expr._e[i + 1];
			QUARK_ASSERT(arg_expr.check_invariant());

			const auto& t = execute_expression(vm, arg_expr);
			vm._value_stack.push_back(t);
		}
		if(symbol_count > arg_count){
			//	Skip args, they are already copied.
			for(vector<bc_value_t>::size_type i = arg_count ; i < symbol_count ; i++){
				const auto& symbol = function_def._body._symbols[i];
				vm._value_stack.push_back(value_to_bc(symbol.second._const_value));
			}
		}

		//??? Remove the parallell _call_stack -- encode stack frames into _value_stack.
		vm._call_stack.push_back(environment_t{ values_offset });
		const auto& result = execute_statements(vm, function_def._body._statements);
		vm._call_stack.pop_back();
		vm._value_stack.resize(values_offset);

		QUARK_ASSERT(result._type == statement_result_t::k_returning);
		return result._output;
	}
}

//	This function evaluates all input expressions, then call construct_value_from_typeid() to do the work.
//??? Make several opcodes for construct-value: construct-struct, vector, dict, basic. ALSO casting 1:1 between types.
//??? Optimize -- inline construct_value_from_typeid() to simplify a lot.
bc_value_t execute_construct_value_expression(interpreter_t& vm, const bc_expression_t& expr){
	QUARK_ASSERT(vm.check_invariant());

	const auto basetype = get_basetype(vm, expr._type);
	if(basetype == base_type::k_vector){
		const std::vector<bc_expression_t>& elements = expr._e;
		const auto& root_value_type = get_type(vm, expr._type);
		const auto& element_type = root_value_type.get_vector_element_type();
		QUARK_ASSERT(element_type.is_undefined() == false);

		//	An empty vector is encoded as a constant value by pass3, not a vector-definition-expression.
//		QUARK_ASSERT(elements.empty() == false);

		QUARK_ASSERT(root_value_type.is_undefined() == false);
		QUARK_ASSERT(element_type.is_undefined() == false);

		std::vector<bc_value_t> elements2;
		for(const auto& m: elements){
			const auto& element = execute_expression(vm, m);
			elements2.push_back(element);
		}

	#if DEBUG
		for(const auto& m: elements2){
			QUARK_ASSERT(m.get_debug_type() == element_type);
		}
	#endif
		return construct_value_from_typeid(vm, typeid_t::make_vector(element_type), element_type, elements2);
	}
	else if(basetype == base_type::k_dict){
		const auto& elements = expr._e;
		const auto& root_value_type = get_type(vm, expr._type);
		const auto& element_type = root_value_type.get_dict_value_type();

		//	An empty dict is encoded as a constant value pass3, not a dict-definition-expression.
//		QUARK_ASSERT(elements.empty() == false);

		QUARK_ASSERT(root_value_type.is_undefined() == false);
		QUARK_ASSERT(element_type.is_undefined() == false);

		std::vector<bc_value_t> elements2;
		for(auto i = 0 ; i < elements.size() / 2 ; i++){
			const auto& key_expr = elements[i * 2 + 0];
			const auto& value_expr = elements[i * 2 + 1];

			const auto& value = execute_expression(vm, value_expr);
			const string key = key_expr._value.get_string_value();
			elements2.push_back(bc_value_t::make_string(key));
			elements2.push_back(value);
		}
		return construct_value_from_typeid(vm, typeid_t::make_dict(element_type), element_type, elements2);
	}
	else if(basetype == base_type::k_struct){
		std::vector<bc_value_t> elements2;
		for(const auto& m: expr._e){
			const auto& element = execute_expression(vm, m);
			elements2.push_back(element);
		}
		return construct_value_from_typeid(vm, get_type(vm, expr._type), typeid_t::make_undefined(), elements2);
	}
	else{
		QUARK_ASSERT(expr._e.size() == 1);

		const auto& element = execute_expression(vm, expr._e[0]);
		return construct_value_from_typeid(vm, get_type(vm, expr._type), get_type(vm, expr._input_type), { element });
	}
}

bc_value_t execute_arithmetic_unary_minus_expression(interpreter_t& vm, const bc_expression_t& expr){
	QUARK_ASSERT(vm.check_invariant());

	const auto& c = execute_expression(vm, expr._e[0]);
	const auto basetype = get_basetype(vm, expr._type);
	if(basetype == base_type::k_int){
		return bc_value_t::make_int(0 - c.get_int_value_quick());
	}
	else if(basetype == base_type::k_float){
		return bc_value_t::make_float(0.0f - c.get_float_value());
	}
	else{
		QUARK_ASSERT(false);
		throw std::exception();
	}
}

bc_value_t execute_conditional_operator_expression(interpreter_t& vm, const bc_expression_t& expr){
	QUARK_ASSERT(vm.check_invariant());

	//	Special-case since it uses 3 expressions & uses shortcut evaluation.
	const auto& cond_result = execute_expression(vm, expr._e[0]);
	const bool cond_flag = cond_result.get_bool_value_quick();

	//	!!! Only execute the CHOSEN expression. Not that important since functions are pure.
	if(cond_flag){
		return execute_expression(vm, expr._e[1]);
	}
	else{
		return execute_expression(vm, expr._e[2]);
	}
}

//??? flatten to main dispatch function.
bc_value_t execute_comparison_expression(interpreter_t& vm, const bc_expression_t& expr){
	QUARK_ASSERT(vm.check_invariant());

	const auto& left_constant = execute_expression(vm, expr._e[0]);
	const auto& right_constant = execute_expression(vm, expr._e[1]);
	QUARK_ASSERT(left_constant.get_debug_type() == right_constant.get_debug_type());

	const auto opcode = expr._opcode;
	const auto& type = get_type(vm, expr._e[0]._type);

	if(opcode == bc_expression_opcode::k_expression_comparison_smaller_or_equal){
		long diff = bc_value_t::compare_value_true_deep(left_constant, right_constant, type);
		return bc_value_t::make_bool(diff <= 0);
	}
	else if(opcode == bc_expression_opcode::k_expression_comparison_smaller){
		long diff = bc_value_t::compare_value_true_deep(left_constant, right_constant, type);
		return bc_value_t::make_bool(diff < 0);
	}
	else if(opcode == bc_expression_opcode::k_expression_comparison_larger_or_equal){
		long diff = bc_value_t::compare_value_true_deep(left_constant, right_constant, type);
		return bc_value_t::make_bool(diff >= 0);
	}
	else if(opcode == bc_expression_opcode::k_expression_comparison_larger){
		long diff = bc_value_t::compare_value_true_deep(left_constant, right_constant, type);
		return bc_value_t::make_bool(diff > 0);
	}

	else if(opcode == bc_expression_opcode::k_expression_logical_equal){
		long diff = bc_value_t::compare_value_true_deep(left_constant, right_constant, type);
		return bc_value_t::make_bool(diff == 0);
	}
	else if(opcode == bc_expression_opcode::k_expression_logical_nonequal){
		long diff = bc_value_t::compare_value_true_deep(left_constant, right_constant, type);
		return bc_value_t::make_bool(diff != 0);
	}
	else{
		QUARK_ASSERT(false);
		throw std::exception();
	}
}

//??? make dedicated opcodes for each type - no need to switch.
bc_value_t execute_arithmetic_expression(interpreter_t& vm, const bc_expression_t& expr){
	QUARK_ASSERT(vm.check_invariant());

	const auto& left = execute_expression(vm, expr._e[0]);
	const auto& right = execute_expression(vm, expr._e[1]);
	QUARK_ASSERT(left.get_debug_type() == right.get_debug_type());

	const auto basetype = get_basetype(vm, expr._type);

	const auto op = expr._opcode;

	//	bool
	if(basetype == base_type::k_bool){
		const bool left2 = left.get_bool_value_quick();
		const bool right2 = right.get_bool_value_quick();

		if(op == bc_expression_opcode::k_expression_arithmetic_add){
			return bc_value_t::make_bool(left2 + right2);
		}
		else if(op == bc_expression_opcode::k_expression_logical_and){
			return bc_value_t::make_bool(left2 && right2);
		}
		else if(op == bc_expression_opcode::k_expression_logical_or){
			return bc_value_t::make_bool(left2 || right2);
		}
		else{
			QUARK_ASSERT(false);
		}
	}

	//	int
	else if(basetype == base_type::k_int){
		const int left2 = left.get_int_value_quick();
		const int right2 = right.get_int_value_quick();

		if(op == bc_expression_opcode::k_expression_arithmetic_add){
			return bc_value_t::make_int(left2 + right2);
		}
		else if(op == bc_expression_opcode::k_expression_arithmetic_subtract){
			return bc_value_t::make_int(left2 - right2);
		}
		else if(op == bc_expression_opcode::k_expression_arithmetic_multiply){
			return bc_value_t::make_int(left2 * right2);
		}
		else if(op == bc_expression_opcode::k_expression_arithmetic_divide){
			if(right2 == 0){
				throw std::runtime_error("EEE_DIVIDE_BY_ZERO");
			}
			return bc_value_t::make_int(left2 / right2);
		}
		else if(op == bc_expression_opcode::k_expression_arithmetic_remainder){
			if(right2 == 0){
				throw std::runtime_error("EEE_DIVIDE_BY_ZERO");
			}
			return bc_value_t::make_int(left2 % right2);
		}

		//### Could be replaced by feature to convert any value to bool -- they use a generic comparison for && and ||
		else if(op == bc_expression_opcode::k_expression_logical_and){
			return bc_value_t::make_bool((left2 != 0) && (right2 != 0));
		}
		else if(op == bc_expression_opcode::k_expression_logical_or){
			return bc_value_t::make_bool((left2 != 0) || (right2 != 0));
		}
		else{
			QUARK_ASSERT(false);
		}
	}

	//	float
	else if(basetype == base_type::k_float){
		const float left2 = left.get_float_value();
		const float right2 = right.get_float_value();

		if(op == bc_expression_opcode::k_expression_arithmetic_add){
			return bc_value_t::make_float(left2 + right2);
		}
		else if(op == bc_expression_opcode::k_expression_arithmetic_subtract){
			return bc_value_t::make_float(left2 - right2);
		}
		else if(op == bc_expression_opcode::k_expression_arithmetic_multiply){
			return bc_value_t::make_float(left2 * right2);
		}
		else if(op == bc_expression_opcode::k_expression_arithmetic_divide){
			if(right2 == 0.0f){
				throw std::runtime_error("EEE_DIVIDE_BY_ZERO");
			}
			return bc_value_t::make_float(left2 / right2);
		}

		else if(op == bc_expression_opcode::k_expression_logical_and){
			return bc_value_t::make_bool((left2 != 0.0f) && (right2 != 0.0f));
		}
		else if(op == bc_expression_opcode::k_expression_logical_or){
			return bc_value_t::make_bool((left2 != 0.0f) || (right2 != 0.0f));
		}
		else{
			QUARK_ASSERT(false);
		}
	}

	//	string
	else if(basetype == base_type::k_string){
		const auto& left2 = left.get_string_value();
		const auto& right2 = right.get_string_value();

		if(op == bc_expression_opcode::k_expression_arithmetic_add){
			return bc_value_t::make_string(left2 + right2);
		}
		else{
			QUARK_ASSERT(false);
		}
	}

	//	struct
	else if(basetype == base_type::k_struct){
		QUARK_ASSERT(false);
	}

	//	vector
	else if(basetype == base_type::k_vector){
		const auto& element_type = get_type(vm, expr._type).get_vector_element_type();
		if(op == bc_expression_opcode::k_expression_arithmetic_add){
			//	Copy vector into elements.
			auto elements2 = left.get_vector_value();
			const auto& rhs_elements = right.get_vector_value();
			elements2.insert(elements2.end(), rhs_elements.begin(), rhs_elements.end());

			const auto& value2 = bc_value_t::make_vector_value(element_type, elements2);
			return value2;
		}
		else{
			QUARK_ASSERT(false);
		}
	}
	else if(basetype == base_type::k_function){
		QUARK_ASSERT(false);
	}
	else{
		QUARK_ASSERT(false);
	}
	throw std::exception();
}


QUARK_UNIT_TEST("", "", "", ""){
	float a = 10.0f;
	float b = 23.3f;

	bool r = a && b;
	QUARK_UT_VERIFY(r == true);
}

bc_value_t execute_expression__switch(interpreter_t& vm, const bc_expression_t& e){
	QUARK_ASSERT(vm.check_invariant());
	QUARK_ASSERT(e.check_invariant());

	const auto& op = e._opcode;

	if(op == bc_expression_opcode::k_expression_literal){
		return e._value;
	}
	else if(op == bc_expression_opcode::k_expression_literal__int){
		return e._value;
	}
	else if(op == bc_expression_opcode::k_expression_resolve_member){
		return execute_resolve_member_expression(vm, e);
	}
	else if(op == bc_expression_opcode::k_expression_lookup_element){
		return execute_lookup_element_expression(vm, e);
	}

	//??? Optimize by inlining find_env_from_address() and making sep paths.
	else if(op == bc_expression_opcode::k_expression_load){
		environment_t* env = find_env_from_address(vm, e._address_parent_step);
		const auto pos = env->_values_offset + e._address_index;
		QUARK_ASSERT(pos >= 0 && pos < vm._value_stack.size());
		const auto& value = vm._value_stack[pos];
//		QUARK_ASSERT(value.get_type().get_base_type() == e._basetype);
		return value;
	}

	else if(op == bc_expression_opcode::k_expression_call){
		return execute_call_expression(vm, e);
	}

	else if(op == bc_expression_opcode::k_expression_construct_value){
		return execute_construct_value_expression(vm, e);
	}

	//	This can be desugared at compile time.
	else if(op == bc_expression_opcode::k_expression_arithmetic_unary_minus){
		return execute_arithmetic_unary_minus_expression(vm, e);
	}

	//	Special-case since it uses 3 expressions & uses shortcut evaluation.
	else if(op == bc_expression_opcode::k_expression_conditional_operator3){
		return execute_conditional_operator_expression(vm, e);
	}


	else if(op == bc_expression_opcode::k_expression_comparison_smaller_or_equal){
		const auto& left_constant = execute_expression(vm, e._e[0]);
		const auto& right_constant = execute_expression(vm, e._e[1]);
		QUARK_ASSERT(left_constant.get_debug_type() == right_constant.get_debug_type());
		const auto& type = get_type(vm, e._e[0]._type);
		long diff = bc_value_t::compare_value_true_deep(left_constant, right_constant, type);
		return bc_value_t::make_bool(diff <= 0);
	}
	else if(op == bc_expression_opcode::k_expression_comparison_smaller_or_equal__int){
		const auto& left_constant = execute_expression(vm, e._e[0]);
		const auto& right_constant = execute_expression(vm, e._e[1]);
		QUARK_ASSERT(left_constant.get_debug_type() == right_constant.get_debug_type());
		return bc_value_t::make_bool(left_constant.get_int_value_quick() <= right_constant.get_int_value_quick());
	}
	else if(op == bc_expression_opcode::k_expression_comparison_smaller){
		const auto& left_constant = execute_expression(vm, e._e[0]);
		const auto& right_constant = execute_expression(vm, e._e[1]);
		QUARK_ASSERT(left_constant.get_debug_type() == right_constant.get_debug_type());
		const auto& type = get_type(vm, e._e[0]._type);
		long diff = bc_value_t::compare_value_true_deep(left_constant, right_constant, type);
		return bc_value_t::make_bool(diff < 0);
	}
	else if(op == bc_expression_opcode::k_expression_comparison_larger_or_equal){
		const auto& left_constant = execute_expression(vm, e._e[0]);
		const auto& right_constant = execute_expression(vm, e._e[1]);
		QUARK_ASSERT(left_constant.get_debug_type() == right_constant.get_debug_type());
		const auto& type = get_type(vm, e._e[0]._type);
		long diff = bc_value_t::compare_value_true_deep(left_constant, right_constant, type);
		return bc_value_t::make_bool(diff >= 0);
	}
	else if(op == bc_expression_opcode::k_expression_comparison_larger){
		const auto& left_constant = execute_expression(vm, e._e[0]);
		const auto& right_constant = execute_expression(vm, e._e[1]);
		QUARK_ASSERT(left_constant.get_debug_type() == right_constant.get_debug_type());
		const auto& type = get_type(vm, e._e[0]._type);
		long diff = bc_value_t::compare_value_true_deep(left_constant, right_constant, type);
		return bc_value_t::make_bool(diff > 0);
	}


	else if(op == bc_expression_opcode::k_expression_logical_equal){
		const auto& left_constant = execute_expression(vm, e._e[0]);
		const auto& right_constant = execute_expression(vm, e._e[1]);
		QUARK_ASSERT(left_constant.get_debug_type() == right_constant.get_debug_type());
		const auto& type = get_type(vm, e._e[0]._type);
		long diff = bc_value_t::compare_value_true_deep(left_constant, right_constant, type);
		return bc_value_t::make_bool(diff == 0);
	}
	else if(op == bc_expression_opcode::k_expression_logical_nonequal){
		const auto& left_constant = execute_expression(vm, e._e[0]);
		const auto& right_constant = execute_expression(vm, e._e[1]);
		QUARK_ASSERT(left_constant.get_debug_type() == right_constant.get_debug_type());
		const auto& type = get_type(vm, e._e[0]._type);
		long diff = bc_value_t::compare_value_true_deep(left_constant, right_constant, type);
		return bc_value_t::make_bool(diff != 0);
	}

	else if(op == bc_expression_opcode::k_expression_arithmetic_add){
		const auto& left = execute_expression(vm, e._e[0]);
		const auto& right = execute_expression(vm, e._e[1]);
		QUARK_ASSERT(left.get_debug_type() == right.get_debug_type());
		const auto basetype = get_basetype(vm, e._type);

		//	bool
		if(basetype == base_type::k_bool){
			const bool left2 = left.get_bool_value_quick();
			const bool right2 = right.get_bool_value_quick();
			return bc_value_t::make_bool(left2 + right2);
		}

		//	int
		else if(basetype == base_type::k_int){
			const int left2 = left.get_int_value_quick();
			const int right2 = right.get_int_value_quick();
			return bc_value_t::make_int(left2 + right2);
		}

		//	float
		else if(basetype == base_type::k_float){
			const float left2 = left.get_float_value();
			const float right2 = right.get_float_value();
			return bc_value_t::make_float(left2 + right2);
		}

		//	string
		else if(basetype == base_type::k_string){
			const auto& left2 = left.get_string_value();
			const auto& right2 = right.get_string_value();
			return bc_value_t::make_string(left2 + right2);
		}

		//	vector
		else if(basetype == base_type::k_vector){
			const auto& element_type = get_type(vm, e._type).get_vector_element_type();

			//	Copy vector into elements.
			auto elements2 = left.get_vector_value();
			const auto& right_elements = right.get_vector_value();
			elements2.insert(elements2.end(), right_elements.begin(), right_elements.end());
			const auto& value2 = bc_value_t::make_vector_value(element_type, elements2);
			return value2;
		}
		else{
		}

		QUARK_ASSERT(false);
		throw std::exception();
	}
	else if(op == bc_expression_opcode::k_expression_arithmetic_add__int){
		const auto& left = execute_expression(vm, e._e[0]);
		const auto& right = execute_expression(vm, e._e[1]);
		QUARK_ASSERT(left.get_debug_type() == right.get_debug_type());
		const int left2 = left.get_int_value_quick();
		const int right2 = right.get_int_value_quick();
		return bc_value_t::make_int(left2 + right2);
	}

	else if(op == bc_expression_opcode::k_expression_arithmetic_subtract){
		const auto& left = execute_expression(vm, e._e[0]);
		const auto& right = execute_expression(vm, e._e[1]);
		QUARK_ASSERT(left.get_debug_type() == right.get_debug_type());
		const auto basetype = get_basetype(vm, e._type);

		//	int
		if(basetype == base_type::k_int){
			const int left2 = left.get_int_value_quick();
			const int right2 = right.get_int_value_quick();
			return bc_value_t::make_int(left2 - right2);
		}

		//	float
		else if(basetype == base_type::k_float){
			const float left2 = left.get_float_value();
			const float right2 = right.get_float_value();
			return bc_value_t::make_float(left2 - right2);
		}

		else{
		}
		QUARK_ASSERT(false);
		throw std::exception();
	}
	else if(op == bc_expression_opcode::k_expression_arithmetic_subtract__int){
		const auto& left = execute_expression(vm, e._e[0]);
		const auto& right = execute_expression(vm, e._e[1]);
		QUARK_ASSERT(left.get_debug_type() == right.get_debug_type());
		const int left2 = left.get_int_value_quick();
		const int right2 = right.get_int_value_quick();
		return bc_value_t::make_int(left2 - right2);
	}
	else if(op == bc_expression_opcode::k_expression_arithmetic_multiply){
		const auto& left = execute_expression(vm, e._e[0]);
		const auto& right = execute_expression(vm, e._e[1]);
		QUARK_ASSERT(left.get_debug_type() == right.get_debug_type());
		const auto basetype = get_basetype(vm, e._type);

		//	int
		if(basetype == base_type::k_int){
			const int left2 = left.get_int_value_quick();
			const int right2 = right.get_int_value_quick();
			return bc_value_t::make_int(left2 * right2);
		}

		//	float
		else if(basetype == base_type::k_float){
			const float left2 = left.get_float_value();
			const float right2 = right.get_float_value();
			return bc_value_t::make_float(left2 * right2);
		}

		else{
		}
		QUARK_ASSERT(false);
		throw std::exception();
	}
	else if(op == bc_expression_opcode::k_expression_arithmetic_divide){
		const auto& left = execute_expression(vm, e._e[0]);
		const auto& right = execute_expression(vm, e._e[1]);
		QUARK_ASSERT(left.get_debug_type() == right.get_debug_type());
		const auto basetype = get_basetype(vm, e._type);

		//	int
		if(basetype == base_type::k_int){
			const int left2 = left.get_int_value_quick();
			const int right2 = right.get_int_value_quick();
			if(right2 == 0){
				throw std::runtime_error("EEE_DIVIDE_BY_ZERO");
			}
			return bc_value_t::make_int(left2 / right2);
		}

		//	float
		else if(basetype == base_type::k_float){
			const float left2 = left.get_float_value();
			const float right2 = right.get_float_value();
			if(right2 == 0.0f){
				throw std::runtime_error("EEE_DIVIDE_BY_ZERO");
			}
			return bc_value_t::make_float(left2 / right2);
		}

		else{
		}
		QUARK_ASSERT(false);
		throw std::exception();
	}
	else if(op == bc_expression_opcode::k_expression_arithmetic_remainder){
		const auto& left = execute_expression(vm, e._e[0]);
		const auto& right = execute_expression(vm, e._e[1]);
		QUARK_ASSERT(left.get_debug_type() == right.get_debug_type());
		const auto basetype = get_basetype(vm, e._type);

		//	int
		if(basetype == base_type::k_int){
			const int left2 = left.get_int_value_quick();
			const int right2 = right.get_int_value_quick();
			if(right2 == 0){
				throw std::runtime_error("EEE_DIVIDE_BY_ZERO");
			}
			return bc_value_t::make_int(left2 % right2);
		}

		else{
		}
		QUARK_ASSERT(false);
		throw std::exception();
	}

	else if(op == bc_expression_opcode::k_expression_logical_and){
		const auto& left = execute_expression(vm, e._e[0]);
		const auto& right = execute_expression(vm, e._e[1]);
		QUARK_ASSERT(left.get_debug_type() == right.get_debug_type());
		const auto basetype = get_basetype(vm, e._type);

		//	bool
		if(basetype == base_type::k_bool){
			const bool left2 = left.get_bool_value_quick();
			const bool right2 = right.get_bool_value_quick();
			return bc_value_t::make_bool(left2 && right2);
		}

		//	int
		else if(basetype == base_type::k_int){
			const int left2 = left.get_int_value_quick();
			const int right2 = right.get_int_value_quick();

			//### Could be replaced by feature to convert any value to bool -- they use a generic comparison for && and ||
			return bc_value_t::make_bool((left2 != 0) && (right2 != 0));
		}

		//	float
		//??? Maybe skip support for this.
		else if(basetype == base_type::k_float){
			const float left2 = left.get_float_value();
			const float right2 = right.get_float_value();
			return bc_value_t::make_bool((left2 != 0.0f) && (right2 != 0.0f));
		}

		else{
		}
		QUARK_ASSERT(false);
		throw std::exception();
	}
	else if(op == bc_expression_opcode::k_expression_logical_or){
		const auto& left = execute_expression(vm, e._e[0]);
		const auto& right = execute_expression(vm, e._e[1]);
		QUARK_ASSERT(left.get_debug_type() == right.get_debug_type());
		const auto basetype = get_basetype(vm, e._type);

		//	bool
		if(basetype == base_type::k_bool){
			const bool left2 = left.get_bool_value_quick();
			const bool right2 = right.get_bool_value_quick();
			return bc_value_t::make_bool(left2 || right2);
		}

		//	int
		else if(basetype == base_type::k_int){
			const int left2 = left.get_int_value_quick();
			const int right2 = right.get_int_value_quick();
			return bc_value_t::make_bool((left2 != 0) || (right2 != 0));
		}

		//	float
		else if(basetype == base_type::k_float){
			const float left2 = left.get_float_value();
			const float right2 = right.get_float_value();
			return bc_value_t::make_bool((left2 != 0.0f) || (right2 != 0.0f));
		}

		else{
		}
		QUARK_ASSERT(false);
		throw std::exception();
	}

	else{
		QUARK_ASSERT(false);
	}
	throw std::exception();
}

//	Computed goto-dispatch of expressions: -- not faster than switch when running max optimizations. C++ Optimizer makes compute goto?
//	May be better bet when doing a SEQUENCE of opcode dispatches in a loop.
//??? use C++ local variables as our VM locals1?
//https://eli.thegreenplace.net/2012/07/12/computed-goto-for-efficient-dispatch-tables
bc_value_t execute_expression__computed_goto(interpreter_t& vm, const bc_expression_t& e){
	QUARK_ASSERT(vm.check_invariant());
	QUARK_ASSERT(e.check_invariant());

	const auto& op = static_cast<int>(e._opcode);


	//	Not thread safe -- avoid static locals!
	static void* dispatch_table[] = {
		&&bc_expression_opcode___k_expression_literal,
		&&bc_expression_opcode___k_expression_resolve_member,
		&&bc_expression_opcode___k_expression_lookup_element,
		&&bc_expression_opcode___k_expression_load,
		&&bc_expression_opcode___k_expression_call,
		&&bc_expression_opcode___k_expression_construct_value,

		&&bc_expression_opcode___k_expression_arithmetic_unary_minus,

		&&bc_expression_opcode___k_expression_conditional_operator3,

		&&bc_expression_opcode___k_expression_comparison_smaller_or_equal,
		&&bc_expression_opcode___k_expression_comparison_smaller,
		&&bc_expression_opcode___k_expression_comparison_larger_or_equal,
		&&bc_expression_opcode___k_expression_comparison_larger,

		&&bc_expression_opcode___k_expression_logical_equal,
		&&bc_expression_opcode___k_expression_logical_nonequal,

		&&bc_expression_opcode___k_expression_arithmetic_add,
		&&bc_expression_opcode___k_expression_arithmetic_subtract,
		&&bc_expression_opcode___k_expression_arithmetic_multiply,
		&&bc_expression_opcode___k_expression_arithmetic_divide,
		&&bc_expression_opcode___k_expression_arithmetic_remainder,

		&&bc_expression_opcode___k_expression_logical_and,
		&&bc_expression_opcode___k_expression_logical_or
	};
//	#define DISPATCH() goto *dispatch_table[op]

//	DISPATCH();
	goto *dispatch_table[op];
	while (1) {
        bc_expression_opcode___k_expression_literal:
			return e._value;

		bc_expression_opcode___k_expression_resolve_member:
			return execute_resolve_member_expression(vm, e);

		bc_expression_opcode___k_expression_lookup_element:
			return execute_lookup_element_expression(vm, e);

		//??? Optimize by inlining find_env_from_address() and making sep paths.
		bc_expression_opcode___k_expression_load:
			{
				environment_t* env = find_env_from_address(vm, e._address_parent_step);
				const auto pos = env->_values_offset + e._address_index;
				QUARK_ASSERT(pos >= 0 && pos < vm._value_stack.size());
				const auto& value = vm._value_stack[pos];
		//		QUARK_ASSERT(value.get_type().get_base_type() == e._basetype);
				return value;
			}

		bc_expression_opcode___k_expression_call:
			return execute_call_expression(vm, e);

		bc_expression_opcode___k_expression_construct_value:
			return execute_construct_value_expression(vm, e);



		bc_expression_opcode___k_expression_arithmetic_unary_minus:
			return execute_arithmetic_unary_minus_expression(vm, e);



		bc_expression_opcode___k_expression_conditional_operator3:
			return execute_conditional_operator_expression(vm, e);



		bc_expression_opcode___k_expression_comparison_smaller_or_equal:
		bc_expression_opcode___k_expression_comparison_smaller:
		bc_expression_opcode___k_expression_comparison_larger_or_equal:
		bc_expression_opcode___k_expression_comparison_larger:
		bc_expression_opcode___k_expression_logical_equal:
		bc_expression_opcode___k_expression_logical_nonequal:
			return execute_comparison_expression(vm, e);


		bc_expression_opcode___k_expression_arithmetic_add:
		bc_expression_opcode___k_expression_arithmetic_subtract:
		bc_expression_opcode___k_expression_arithmetic_multiply:
		bc_expression_opcode___k_expression_arithmetic_divide:
		bc_expression_opcode___k_expression_arithmetic_remainder:

		bc_expression_opcode___k_expression_logical_and:
		bc_expression_opcode___k_expression_logical_or:
			return execute_arithmetic_expression(vm, e);
	}
}

inline bc_value_t execute_expression(interpreter_t& vm, const bc_expression_t& e){
	return execute_expression__switch(vm, e);
//	return execute_expression__computed_goto(vm, e);
}



//////////////////////////////////////////		interpreter_t



interpreter_t::interpreter_t(const bc_program_t& program){
	QUARK_ASSERT(program.check_invariant());

	_value_stack.reserve(1024);
	_call_stack.reserve(32);

	//	Make lookup table from host-function ID to an implementation of that host function in the interpreter.
	const auto& host_functions = get_host_functions();
	std::map<int, HOST_FUNCTION_PTR> host_functions2;
	for(auto& hf_kv: host_functions){
		const auto& function_id = hf_kv.second._signature._function_id;
		const auto& function_ptr = hf_kv.second._f;
		host_functions2.insert({ function_id, function_ptr });
	}

	const auto start_time = std::chrono::high_resolution_clock::now();
	_imm = std::make_shared<interpreter_imm_t>(interpreter_imm_t{start_time, program, host_functions2});


	const auto values_offset = _value_stack.size();
	const auto& body_ptr = _imm->_program._globals;
	for(vector<bc_value_t>::size_type i = 0 ; i < body_ptr._symbols.size() ; i++){
		const auto& symbol = body_ptr._symbols[i];
		_value_stack.push_back(value_to_bc(symbol.second._const_value));
	}
	_call_stack.push_back(environment_t{ values_offset });

	//	Run static intialization (basically run global statements before calling main()).
	/*const auto& r =*/ execute_statements(*this, _imm->_program._globals._statements);
	QUARK_ASSERT(check_invariant());
}

interpreter_t::interpreter_t(const interpreter_t& other) :
	_imm(other._imm),
	_value_stack(other._value_stack),
	_call_stack(other._call_stack),
	_print_output(other._print_output)
{
	QUARK_ASSERT(other.check_invariant());
	QUARK_ASSERT(check_invariant());
}

void interpreter_t::swap(interpreter_t& other) throw(){
	other._imm.swap(this->_imm);
	other._value_stack.swap(this->_value_stack);
	_call_stack.swap(this->_call_stack);
	other._print_output.swap(this->_print_output);
}

const interpreter_t& interpreter_t::operator=(const interpreter_t& other){
	auto temp = other;
	temp.swap(*this);
	return *this;
}

#if DEBUG
bool interpreter_t::check_invariant() const {
	QUARK_ASSERT(_imm->_program.check_invariant());
	return true;
}
#endif



//////////////////////////////////////////		FUNCTIONS



json_t interpreter_to_json(const interpreter_t& vm){
	vector<json_t> callstack;
	for(int env_index = 0 ; env_index < vm._call_stack.size() ; env_index++){
		const auto& e = &vm._call_stack[vm._call_stack.size() - 1 - env_index];

		const auto local_end = (env_index == (vm._call_stack.size() - 1)) ? vm._value_stack.size() : vm._call_stack[vm._call_stack.size() - 1 - env_index + 1]._values_offset;
		const auto local_count = local_end - e->_values_offset;
		std::vector<json_t> values;
		for(int local_index = 0 ; local_index < local_count ; local_index++){
			const auto& v = vm._value_stack[e->_values_offset + local_index];
		}

		const auto& env = json_t::make_object({
			{ "values", values }
		});
		callstack.push_back(env);
	}

	return json_t::make_object({
		{ "ast", bcprogram_to_json(vm._imm->_program) },
		{ "callstack", json_t::make_array(callstack) }
	});
}


value_t call_host_function(interpreter_t& vm, int function_id, const std::vector<floyd::value_t>& args){
	QUARK_ASSERT(function_id >= 0);

	const auto& host_function = vm._imm->_host_functions.at(function_id);

	//	arity
//	QUARK_ASSERT(args.size() == host_function._function_type.get_function_args().size());

	const auto& result = (host_function)(vm, args);
	return result;
}

bc_program_t program_to_ast2(const interpreter_context_t& context, const string& program){
	parser_context_t context2{ quark::trace_context_t(context._tracer._verbose, context._tracer._tracer) };
//	parser_context_t context{ quark::make_default_tracer() };
//	QUARK_CONTEXT_TRACE(context._tracer, "Hello");

	const auto& pass1 = floyd::parse_program2(context2, program);
	const auto& pass2 = run_pass2(context2._tracer, pass1);
	const auto& pass3 = floyd::run_pass3(context2._tracer, pass2);


	const auto bc = run_bggen(context2._tracer, pass3);

	return bc;
}

void print_vm_printlog(const interpreter_t& vm){
	if(vm._print_output.empty() == false){
		std::cout << "print output:\n";
		for(const auto& line: vm._print_output){
			std::cout << line << "\n";
		}
	}
}

interpreter_t run_global(const interpreter_context_t& context, const string& source){
	auto program = program_to_ast2(context, source);
	auto vm = interpreter_t(program);
//	QUARK_TRACE(json_to_pretty_string(interpreter_to_json(vm)));
	print_vm_printlog(vm);
	return vm;
}

std::pair<interpreter_t, value_t> run_main(const interpreter_context_t& context, const string& source, const vector<floyd::value_t>& args){
	auto program = program_to_ast2(context, source);

	//	Runs global code.
	auto vm = interpreter_t(program);

	const auto& main_function = find_global_symbol2(vm, "main");
	if(main_function != nullptr){
		const auto& result = call_function(vm, bc_to_value(main_function->_value, main_function->_symbol._value_type), args);
		return { vm, result };
	}
	else{
		return {vm, value_t::make_undefined()};
	}
}

std::pair<interpreter_t, value_t> run_program(const interpreter_context_t& context, const bc_program_t& program, const vector<floyd::value_t>& args){
	auto vm = interpreter_t(program);

	const auto& main_func = find_global_symbol2(vm, "main");
	if(main_func != nullptr){
		const auto& r = call_function(vm, bc_to_value(main_func->_value, main_func->_symbol._value_type), args);
		return { vm, r };
	}
	else{
		return { vm, value_t::make_undefined() };
	}
}


}	//	floyd
