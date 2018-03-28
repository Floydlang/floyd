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

#include <array>
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



bc_value_t execute_expression__switch(interpreter_t& vm, const bc_expression_t& e);
bc_value_t execute_expression__computed_goto(interpreter_t& vm, const bc_expression_t& e);

bc_value_t execute_expression(interpreter_t& vm, const bc_expression_t& e);
/*
inline bc_value_t execute_expression(interpreter_t& vm, const bc_expression_t& e){
	return execute_expression__switch(vm, e);
//	return execute_expression__computed_goto(vm, e);
}
*/
#define execute_expression execute_expression__switch

inline const bc_typeid2_t& get_type(const interpreter_t& vm, const bc_typeid_t& type){
	return vm._imm->_program._types[type];
}
inline const base_type get_basetype(const interpreter_t& vm, const bc_typeid_t& type){
	return vm._imm->_program._types[type].get_base_type();
}


inline interpret_stack_element_t make_object_element(const bc_value_t& value, const typeid_t& type){
	interpret_stack_element_t temp;
	value._value_internals._ext->_rc++;
	temp._internals._ext = value._value_internals._ext;
	return temp;
}

inline interpret_stack_element_t make_element(const bc_value_t& value, const typeid_t& type){
	const auto basettype = type.get_base_type();
	if(basettype == base_type::k_internal_undefined){
		interpret_stack_element_t temp;
		temp._internals._ext = nullptr;
		return temp;
	}
	else if(basettype == base_type::k_internal_dynamic){
		interpret_stack_element_t temp;
		temp._internals._ext = nullptr;
		return temp;
	}
	else if(basettype == base_type::k_void){
		interpret_stack_element_t temp;
		temp._internals._ext = nullptr;
		return temp;
	}
	else if(basettype == base_type::k_bool){
		interpret_stack_element_t temp;
		temp._internals._bool = value.get_bool_value();
		return temp;
	}
	else if(basettype == base_type::k_int){
		interpret_stack_element_t temp;
		temp._internals._int = value.get_int_value();
		return temp;
	}
	else if(basettype == base_type::k_float){
		interpret_stack_element_t temp;
		temp._internals._float = value.get_float_value();
		return temp;
	}
	else if(basettype == base_type::k_function){
		interpret_stack_element_t temp;
		temp._internals._function_id = value.get_function_value();
		return temp;
	}

	else if(bc_value_t::is_bc_ext(basettype)){
		return make_object_element(value, type);
	}
	else{
		QUARK_ASSERT(false);
		throw std::exception();
	}
}
inline bc_value_t element_to_value(const interpret_stack_element_t& e, const typeid_t& type){
	if(type.is_undefined()){
		return bc_value_t::make_undefined();
	}
	else if(type.is_internal_dynamic()){
		return bc_value_t::make_internal_dynamic();
	}
	else if(type.is_void()){
		return bc_value_t::make_void();
	}
	else if(type.is_bool()){
		return bc_value_t::make_bool(e._internals._bool);
	}
	else if(type.is_int()){
		return bc_value_t::make_int(e._internals._int);
	}
	else if(type.is_float()){
		return bc_value_t::make_float(e._internals._float);
	}
	else if(type.is_string()){
		return bc_value_t::make_object(e._internals._ext);
	}
	else if(type.is_json_value()){
		return bc_value_t::make_object(e._internals._ext);
	}
	else if(type.is_typeid()){
		return bc_value_t::make_object(e._internals._ext);
	}
	else if(type.is_struct()){
		return bc_value_t::make_object(e._internals._ext);
	}
	else if(type.is_vector()){
		return bc_value_t::make_object(e._internals._ext);
	}
	else if(type.is_dict()){
		return bc_value_t::make_object(e._internals._ext);
	}
	else if(type.is_function()){
		return bc_value_t::make_function_value(type, e._internals._function_id);
	}
	else{
		QUARK_ASSERT(false);
		throw std::exception();
	}
}

inline void push_value(interpreter_t& vm, const bc_value_t& value, const typeid_t& type){
	const auto e = make_element(value, type);
	vm._value_stack.push_back(e);
}

inline bc_value_t load_value(const interpreter_t& vm, int pos, const typeid_t& type){
	const auto& e = vm._value_stack[pos];
	const auto result = element_to_value(e, type);
	return result;
}

inline bc_value_t load_int(const interpreter_t& vm, int pos){
//??? make faster! Return int, don't go via element_to_value(). Don't instantiate typeid_t!
	return load_value(vm, pos, typeid_t::make_int());
}

inline void replace_value_same_type(interpreter_t& vm, int pos, const bc_value_t& value, const typeid_t& type){
	//	Unpack existing value so it's RC:ed OK.
	const auto prev_value = element_to_value(vm._value_stack[pos], type);

	const auto e = make_element(value, type);
	vm._value_stack[pos] = e;
}

inline void pop_value(interpreter_t& vm, const typeid_t& type){
	//	Unpack existing value so it's RC:ed OK.
	const auto prev_value = element_to_value(vm._value_stack.back(), type);
	vm._value_stack.pop_back();
}


statement_result_t execute_body(interpreter_t& vm, const bc_body_t& body, const std::vector<bc_value_t>& init_values);
statement_result_t execute_body(interpreter_t& vm, const bc_body_t& body);

//	Returns new frame-pos, same as vm._current_stack_frame.
//	Will NOT add local variables etc.
//	Only pushes previous stack frame pos.
int open_stack_frame(interpreter_t& vm){
	const auto new_frame_start = static_cast<int>(vm._value_stack.size());
	const auto prev_frame_pos = vm._current_stack_frame;
	push_value(vm, bc_value_t::make_int(prev_frame_pos), typeid_t::make_int());
	const auto new_frame_pos = new_frame_start + 1;
	vm._current_stack_frame = new_frame_pos;
	return new_frame_pos;
}

//	Pops entire stack frame -- all locals etc.
//	Restores previous stack frame pos.
//	Returns resulting stack frame pos.
//??? Make this decrement all RC too!
int close_stack_frame(interpreter_t& vm){
	const auto current_pos = vm._current_stack_frame;
	const auto prev_frame_pos = load_int(vm, current_pos - 1).get_int_value();
	const auto prev_frame_end_pos = current_pos - 1;
	vm._value_stack.resize(prev_frame_end_pos);
	vm._current_stack_frame = prev_frame_pos;
	return prev_frame_pos;
}

int open_stack_frame2(interpreter_t& vm, const bc_body_t& body, const bc_value_t* init_values, int init_value_count){
	int frame_pos = open_stack_frame(vm);

	if(init_value_count > 0){
		for(int i = 0 ; i < init_value_count ; i++){
			push_value(vm, init_values[i], body._symbols[i].second._value_type);
		}
	}

	for(vector<bc_value_t>::size_type i = init_value_count ; i < body._symbols.size() ; i++){
		const auto& symbol = body._symbols[i];

		//	Variable slot.
		if(symbol.second._const_value.get_basetype() == base_type::k_internal_undefined){
			const auto basetype = symbol.second._value_type.get_base_type();

			//	This is just a variable slot without constant. We need to put something there, but that don't confuse RC.
			//	Problem is that IF this is an RC_object, it WILL be decremented when written to using replace_value_same_type().
			//	Use a placeholder object. Type won't match symbol but that's OK.
			if(bc_value_t::is_bc_ext(basetype)){
				push_value(vm, vm._internal_placeholder_object, typeid_t::make_string());
			}
			else{
				push_value(vm, bc_value_t::make_undefined(), typeid_t::make_undefined());
			}
		}

		//	Constant.
		else{
			push_value(vm, value_to_bc(symbol.second._const_value), symbol.second._const_value.get_type());
		}
	}
	return frame_pos;
}



//	#0 is top of stack, last elementis bottom.
//	first: frame_pos, second: framesize-1. Does not include the first slot, which is the prev_frame_pos.
vector<std::pair<int, int>> get_stack_frames(const interpreter_t& vm){
	int frame_pos = vm._current_stack_frame;

	//	We use the entire current stack to calc top frame's size. This can be wrong, if someone pushed more stuff there. Same goes with the previous stack frames too..
	vector<std::pair<int, int>> result{ { frame_pos, static_cast<int>(vm._value_stack.size()) - frame_pos }};

	while(frame_pos > 1){
		const auto prev_frame_pos = load_int(vm, frame_pos - 1).get_int_value();
		const auto prev_size = (frame_pos - 1) - prev_frame_pos;
		result.push_back(std::pair<int, int>{ frame_pos, prev_size });

		frame_pos = prev_frame_pos;
	}
	return result;
}

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
		const auto pos = 1 + index;
		QUARK_ASSERT(pos >= 0 && pos < vm._value_stack.size());

		const auto value_entry = value_entry_t{
			load_value(vm, pos, it->second._value_type),
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

inline int find_frame_from_address(interpreter_t& vm, int parent_step){
	if(parent_step == 0){
		return vm._current_stack_frame;
	}
	else if(parent_step == -1){
		//	Address 0 holds dummy prevstack for globals.
		return 1;
	}
	else{
		int frame_pos = vm._current_stack_frame;
		for(auto i = 0 ; i < parent_step ; i++){
			frame_pos = load_int(vm, frame_pos - 1).get_int_value();
		}
		return frame_pos;
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
	const auto value_size = sizeof(bc_value_t);
	const auto expression_size = sizeof(bc_expression_t);
	const auto e_count_offset = offsetof(bc_expression_t, _e_count);
	const auto e_offset = offsetof(bc_expression_t, _e);
	const auto value_offset = offsetof(bc_expression_t, _value);

/*
	QUARK_UT_VERIFY(value_size == 16);
	QUARK_UT_VERIFY(expression_size == 40);
	QUARK_UT_VERIFY(e_count_offset == 4);
	QUARK_UT_VERIFY(e_offset == 8);
	QUARK_UT_VERIFY(value_offset == 16);
*/

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
			const auto frame_pos = find_frame_from_address(vm, statement._v._parent_steps);
			const auto pos = frame_pos + statement._v._index;
			QUARK_ASSERT(pos >= 0 && pos < vm._value_stack.size());
			replace_value_same_type(vm, pos, rhs_value, get_type(vm, statement._e[0]._type));
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

			//	These are constants (can be reused for every iteration of the for-loop, or memory slots = also reuse).
			//	Notice that first symbol is the loop-iterator.
			QUARK_ASSERT(body._symbols.size() >= 1);
			const auto new_frame_pos = open_stack_frame2(vm, body, nullptr, 0);

			//??? simplify code -- create a count instead.
			//	open-range
			if(statement._param_x == 0){
				for(int x = start_value_int ; x < end_value_int ; x++){
					replace_value_same_type(vm, new_frame_pos + 0, bc_value_t::make_int(x), typeid_t::make_int());

					//### If statements don't have a RETURN, then we don't need to check for it. Make two loops?
					const auto& return_value = execute_statements(vm, body._statements);

					if(return_value._type == statement_result_t::k_returning){
						close_stack_frame(vm);
						return return_value;
					}
				}
			}

			//	closed-range
			else if(statement._param_x == 1){
				for(int x = start_value_int ; x <= end_value_int ; x++){
					replace_value_same_type(vm, new_frame_pos + 0, bc_value_t::make_int(x), typeid_t::make_int());

					//### If statements don't have a RETURN, then we don't need to check for it. Make two loops?
					const auto& return_value = execute_statements(vm, body._statements);

					if(return_value._type == statement_result_t::k_returning){
						close_stack_frame(vm);
						return return_value;
					}
				}
			}
			else{
				QUARK_ASSERT(false);
			}
			close_stack_frame(vm);
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

	const auto new_frame_pos = open_stack_frame2(vm, body, &init_values[0], init_values.size());
	const auto& r = execute_statements(vm, body._statements);
	close_stack_frame(vm);
	return r;
}
statement_result_t execute_body(interpreter_t& vm, const bc_body_t& body){
	QUARK_ASSERT(vm.check_invariant());

	const auto new_frame_pos = open_stack_frame2(vm, body, nullptr, 0);

	const auto& r = execute_statements(vm, body._statements);
	close_stack_frame(vm);
	return r;
}



//////////////////////////////////////////		EXPRESSIONS




bc_value_t execute_resolve_member_expression(interpreter_t& vm, const bc_expression_t& expr){
	QUARK_ASSERT(vm.check_invariant());

	const auto& parent_value = execute_expression(vm, expr._e[0]);
	QUARK_ASSERT(get_type(vm, expr._e[0]._type).get_base_type() == base_type::k_struct);

	const auto& struct_instance = parent_value.get_struct_value();
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

//	Notice: host calls and floyd calls have the same type -- we cannot detect host calls until we have a callee value.
bc_value_t execute_call_expression(interpreter_t& vm, const bc_expression_t& expr){
	QUARK_ASSERT(vm.check_invariant());
	QUARK_ASSERT(expr.check_invariant());

	const auto& function_value = execute_expression(vm, expr._e[0]);
	const auto& function_def = get_function_def(vm, function_value);

	//	_e[...] contains first callee, then each argument.
	const auto arg_count = expr._e_count - 1;
//	const auto arg_count = function_def._args.size();


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
		//	Notice that arguments are first in the symbol list.
		const auto symbol_count = function_def._body._symbols.size();

		//	NOTICE: the arg expressions are designed to be run in caller's stack frame -- their addresses are relative it that.
		//	execute_expression() may temporarily use the stack, overwriting stack after frame pointer.
		//	This makes it hard to execute the args and store the directly into the right spot of stack.
		//	Need temp to solve this. Find better solution?

		//??? maybe execute arg expressions while stack frame is set *beyond* our new frame?

#if 1
		if(arg_count > 8){
			throw std::runtime_error("Max 8 arguments.");
		}
	    std::array<bc_value_t, 8> temp;
//		vector<bc_value_t> temp;
		for(int i = 0 ; i < arg_count ; i++){
			const auto& arg_expr = expr._e[i + 1];
			QUARK_ASSERT(arg_expr.check_invariant());
			const auto& t = execute_expression(vm, arg_expr);
			temp[i] = t;
		}

		const auto new_frame_pos = open_stack_frame2(vm, function_def._body, &temp[0], arg_count);
#endif
#if 0
		const auto new_frame_pos = open_stack_frame(vm);
		for(int i = 0 ; i < arg_count ; i++){
			vm._value_stack.push_back({});
		}
		vm._current_stack_frame = new_frame_pos + static_cast<int>(arg_count);

		for(int i = 0 ; i < arg_count ; i++){
			const auto& arg_expr = expr._e[i + 1];
			QUARK_ASSERT(arg_expr.check_invariant());
			const auto& t = execute_expression(vm, arg_expr);
			vm._value_stack[new_frame_pos + i] = t;
		}
		vm._current_stack_frame = new_frame_pos;
#endif
		const auto& result = execute_statements(vm, function_def._body._statements);
		close_stack_frame(vm);

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
		const bc_expression_t* elements = &expr._e[0];
		const auto& root_value_type = get_type(vm, expr._type);
		const auto& element_type = root_value_type.get_vector_element_type();
		QUARK_ASSERT(element_type.is_undefined() == false);

		//	An empty vector is encoded as a constant value by pass3, not a vector-definition-expression.
//		QUARK_ASSERT(elements.empty() == false);

		QUARK_ASSERT(root_value_type.is_undefined() == false);
		QUARK_ASSERT(element_type.is_undefined() == false);

		std::vector<bc_value_t> elements2;
		for(int i = 0 ; i < expr._e_count ; i++){
			const auto& element = execute_expression(vm, elements[i]);
			elements2.push_back(element);
		}

	#if DEBUG && FLOYD_BD_DEBUG
		for(const auto& m: elements2){
			QUARK_ASSERT(m.get_debug_type() == element_type);
		}
	#endif
		return construct_value_from_typeid(vm, typeid_t::make_vector(element_type), element_type, elements2);
	}
	else if(basetype == base_type::k_dict){
		const bc_expression_t* elements = expr._e;
		const auto& root_value_type = get_type(vm, expr._type);
		const auto& element_type = root_value_type.get_dict_value_type();

		//	An empty dict is encoded as a constant value pass3, not a dict-definition-expression.
//		QUARK_ASSERT(elements.empty() == false);

		QUARK_ASSERT(root_value_type.is_undefined() == false);
		QUARK_ASSERT(element_type.is_undefined() == false);

		std::vector<bc_value_t> elements2;
		for(auto i = 0 ; i < expr._e_count / 2 ; i++){
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
		for(int i = 0 ; i < expr._e_count ; i++){
			const auto& element = execute_expression(vm, expr._e[i]);
			elements2.push_back(element);
		}
		return construct_value_from_typeid(vm, get_type(vm, expr._type), typeid_t::make_undefined(), elements2);
	}
	else{
		QUARK_ASSERT(expr._e_count == 1);

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
#if FLOYD_BD_DEBUG
	QUARK_ASSERT(left_constant.get_debug_type() == right_constant.get_debug_type());
#endif

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
#if FLOYD_BD_DEBUG
	QUARK_ASSERT(left.get_debug_type() == right.get_debug_type());
#endif

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
		int frame_pos = find_frame_from_address(vm, e._address_parent_step);
		const auto pos = frame_pos + e._address_index;
		QUARK_ASSERT(pos >= 0 && pos < vm._value_stack.size());
		const auto& value = load_value(vm, pos, get_type(vm, e._type));
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
#if FLOYD_BD_DEBUG
		QUARK_ASSERT(left_constant.get_debug_type() == right_constant.get_debug_type());
#endif
		const auto& type = get_type(vm, e._e[0]._type);
		long diff = bc_value_t::compare_value_true_deep(left_constant, right_constant, type);
		return bc_value_t::make_bool(diff <= 0);
	}
	else if(op == bc_expression_opcode::k_expression_comparison_smaller_or_equal__int){
		const auto& left_constant = execute_expression(vm, e._e[0]);
		const auto& right_constant = execute_expression(vm, e._e[1]);
#if FLOYD_BD_DEBUG
		QUARK_ASSERT(left_constant.get_debug_type() == right_constant.get_debug_type());
#endif
		return bc_value_t::make_bool(left_constant.get_int_value_quick() <= right_constant.get_int_value_quick());
	}
	else if(op == bc_expression_opcode::k_expression_comparison_smaller){
		const auto& left_constant = execute_expression(vm, e._e[0]);
		const auto& right_constant = execute_expression(vm, e._e[1]);
#if FLOYD_BD_DEBUG
		QUARK_ASSERT(left_constant.get_debug_type() == right_constant.get_debug_type());
#endif
		const auto& type = get_type(vm, e._e[0]._type);
		long diff = bc_value_t::compare_value_true_deep(left_constant, right_constant, type);
		return bc_value_t::make_bool(diff < 0);
	}
	else if(op == bc_expression_opcode::k_expression_comparison_larger_or_equal){
		const auto& left_constant = execute_expression(vm, e._e[0]);
		const auto& right_constant = execute_expression(vm, e._e[1]);
#if FLOYD_BD_DEBUG
		QUARK_ASSERT(left_constant.get_debug_type() == right_constant.get_debug_type());
#endif
		const auto& type = get_type(vm, e._e[0]._type);
		long diff = bc_value_t::compare_value_true_deep(left_constant, right_constant, type);
		return bc_value_t::make_bool(diff >= 0);
	}
	else if(op == bc_expression_opcode::k_expression_comparison_larger){
		const auto& left_constant = execute_expression(vm, e._e[0]);
		const auto& right_constant = execute_expression(vm, e._e[1]);
#if FLOYD_BD_DEBUG
		QUARK_ASSERT(left_constant.get_debug_type() == right_constant.get_debug_type());
#endif
		const auto& type = get_type(vm, e._e[0]._type);
		long diff = bc_value_t::compare_value_true_deep(left_constant, right_constant, type);
		return bc_value_t::make_bool(diff > 0);
	}


	else if(op == bc_expression_opcode::k_expression_logical_equal){
		const auto& left_constant = execute_expression(vm, e._e[0]);
		const auto& right_constant = execute_expression(vm, e._e[1]);
#if FLOYD_BD_DEBUG
		QUARK_ASSERT(left_constant.get_debug_type() == right_constant.get_debug_type());
#endif
		const auto& type = get_type(vm, e._e[0]._type);
		long diff = bc_value_t::compare_value_true_deep(left_constant, right_constant, type);
		return bc_value_t::make_bool(diff == 0);
	}
	else if(op == bc_expression_opcode::k_expression_logical_nonequal){
		const auto& left_constant = execute_expression(vm, e._e[0]);
		const auto& right_constant = execute_expression(vm, e._e[1]);
#if FLOYD_BD_DEBUG
		QUARK_ASSERT(left_constant.get_debug_type() == right_constant.get_debug_type());
#endif
		const auto& type = get_type(vm, e._e[0]._type);
		long diff = bc_value_t::compare_value_true_deep(left_constant, right_constant, type);
		return bc_value_t::make_bool(diff != 0);
	}

	else if(op == bc_expression_opcode::k_expression_arithmetic_add){
		const auto& left = execute_expression(vm, e._e[0]);
		const auto& right = execute_expression(vm, e._e[1]);
#if FLOYD_BD_DEBUG
		QUARK_ASSERT(left.get_debug_type() == right.get_debug_type());
#endif
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
#if FLOYD_BD_DEBUG
		QUARK_ASSERT(left.get_debug_type() == right.get_debug_type());
#endif
		const int left2 = left.get_int_value_quick();
		const int right2 = right.get_int_value_quick();
		return bc_value_t::make_int(left2 + right2);
	}

	else if(op == bc_expression_opcode::k_expression_arithmetic_subtract){
		const auto& left = execute_expression(vm, e._e[0]);
		const auto& right = execute_expression(vm, e._e[1]);
#if FLOYD_BD_DEBUG
		QUARK_ASSERT(left.get_debug_type() == right.get_debug_type());
#endif
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
#if FLOYD_BD_DEBUG
		QUARK_ASSERT(left.get_debug_type() == right.get_debug_type());
#endif
		const int left2 = left.get_int_value_quick();
		const int right2 = right.get_int_value_quick();
		return bc_value_t::make_int(left2 - right2);
	}
	else if(op == bc_expression_opcode::k_expression_arithmetic_multiply){
		const auto& left = execute_expression(vm, e._e[0]);
		const auto& right = execute_expression(vm, e._e[1]);
#if FLOYD_BD_DEBUG
		QUARK_ASSERT(left.get_debug_type() == right.get_debug_type());
#endif
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
#if FLOYD_BD_DEBUG
		QUARK_ASSERT(left.get_debug_type() == right.get_debug_type());
#endif
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
#if FLOYD_BD_DEBUG
		QUARK_ASSERT(left.get_debug_type() == right.get_debug_type());
#endif
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
#if FLOYD_BD_DEBUG
		QUARK_ASSERT(left.get_debug_type() == right.get_debug_type());
#endif
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
#if FLOYD_BD_DEBUG
		QUARK_ASSERT(left.get_debug_type() == right.get_debug_type());
#endif
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
				int frame_pos = find_frame_from_address(vm, e._address_parent_step);
				const auto pos = frame_pos + e._address_index;
				QUARK_ASSERT(pos >= 0 && pos < vm._value_stack.size());
				const auto& value = load_value(vm, pos, get_type(vm, e._type));
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




//////////////////////////////////////////		interpreter_t





interpreter_t::interpreter_t(const bc_program_t& program){
	QUARK_ASSERT(program.check_invariant());

	_value_stack.reserve(1024);
	_internal_placeholder_object = bc_value_t::make_string("Internal placeholder object");

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

	_current_stack_frame = 0;
	open_stack_frame2(*this, _imm->_program._globals, nullptr, 0);

	//	Run static intialization (basically run global statements before calling main()).
	/*const auto& r =*/ execute_statements(*this, _imm->_program._globals._statements);
	QUARK_ASSERT(check_invariant());
}

interpreter_t::interpreter_t(const interpreter_t& other) :
	_imm(other._imm),
	_internal_placeholder_object(other._internal_placeholder_object),
	_value_stack(other._value_stack),
	_current_stack_frame(other._current_stack_frame),
	_print_output(other._print_output)
{
	QUARK_ASSERT(other.check_invariant());
	QUARK_ASSERT(check_invariant());
}

void interpreter_t::swap(interpreter_t& other) throw(){
	other._imm.swap(this->_imm);
	_internal_placeholder_object.swap(other._internal_placeholder_object);
	other._value_stack.swap(this->_value_stack);
	std::swap(_current_stack_frame, this->_current_stack_frame);
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

	const auto stack_frames = get_stack_frames(vm);
/*
	for(int env_index = 0 ; env_index < stack_frames.size() ; env_index++){
		const auto frame_pos = stack_frames[env_index];

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
*/

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
