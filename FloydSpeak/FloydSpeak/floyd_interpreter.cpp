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


//??? Make several opcodes for construct-value: construct-struct, vector, dict, basic. ALSO casting 1:1 between types.
//??? Make a clever box that encode stuff into stackframe/struct etc and lets us quickly access them. No need to encode type in instruction.
//??? get_type() should have all basetypes as first IDs = no need to looup.

//??? flatten all function-defs into ONE big list of instructions or not?

//??? special-case collection types: vector<bool>, vector<int> etc. Don't always tore vector<bc_value_t>


inline const typeid_t& get_type(const interpreter_t& vm, const bc_typeid_t& type){
	return vm._imm->_program._types[type];
}
inline const base_type get_basetype(const interpreter_t& vm, const bc_typeid_t& type){
	return vm._imm->_program._types[type].get_base_type();
}



//////////////////////////////////////////		STACK FRAME SUPPORT








int interpreter_stack_t::read_prev_frame_pos(int frame_pos) const{
//	QUARK_ASSERT(vm.check_invariant());
	QUARK_ASSERT(frame_pos >= k_frame_overhead);

	const auto v = load_intq(frame_pos - 2);
	QUARK_ASSERT(v < frame_pos);
	QUARK_ASSERT(v >= 0);
	return v;
}

const bc_frame_t* interpreter_stack_t::read_prev_frame(int frame_pos) const{
//	QUARK_ASSERT(vm.check_invariant());
	QUARK_ASSERT(frame_pos >= k_frame_overhead);

	const auto v = load_frame_ptr(frame_pos - 1);
	QUARK_ASSERT(v != nullptr);
	QUARK_ASSERT(v->check_invariant());
	return v;
}



frame_pos_t interpreter_stack_t::read_prev_frame_pos2(int frame_pos) const{
	QUARK_ASSERT(frame_pos >= k_frame_overhead);

	const auto pos = load_intq(frame_pos - 2);
	QUARK_ASSERT(pos < frame_pos);
	QUARK_ASSERT(pos >= 0);

	const auto ptr = load_frame_ptr(frame_pos - 1);
	QUARK_ASSERT(ptr != nullptr);
	QUARK_ASSERT(ptr->check_invariant());
	return frame_pos_t{pos, ptr};
}





int get_global_n_pos(int n){
	return k_frame_overhead + n;
}
int get_local_n_pos(int frame_pos, int n){
	return frame_pos + n;
}

#if DEBUG
bool interpreter_stack_t::check_stack_frame(int frame_pos, const bc_frame_t* frame) const{
	if(frame_pos == 0){
		return true;
	}
	else{
		const auto prev_frame = read_prev_frame(frame_pos);
		const auto prev_frame_pos = read_prev_frame_pos(frame_pos);
		QUARK_ASSERT(frame != nullptr && frame->check_invariant());
		QUARK_ASSERT(prev_frame_pos >= 0 && prev_frame_pos < frame_pos);

		for(int i = 0 ; i < frame->_symbols.size() ; i++){
			const auto& symbol = frame->_symbols[i];

			bool symbol_ext = bc_value_t::is_bc_ext(symbol.second._value_type.get_base_type());
			int local_pos = get_local_n_pos(frame_pos, i);

			bool stack_ext = is_ext(local_pos);
			QUARK_ASSERT(symbol_ext == stack_ext);
		}
		if(prev_frame_pos > 2){
			return check_stack_frame(prev_frame_pos, prev_frame);
		}
		else{
			return true;
		}
	}
}
#endif


//	#0 is top of stack, last element is bottom.
//	first: frame_pos, second: framesize-1. Does not include the first slot, which is the prev_frame_pos.
vector<std::pair<int, int>> interpreter_stack_t::get_stack_frames(int frame_pos) const{
	QUARK_ASSERT(check_invariant());

	//	We use the entire current stack to calc top frame's size. This can be wrong, if someone pushed more stuff there. Same goes with the previous stack frames too..
	vector<std::pair<int, int>> result{ { frame_pos, static_cast<int>(size()) - frame_pos }};

	while(frame_pos > 2){
		const auto prev_frame_pos = read_prev_frame_pos(frame_pos);
		const auto prev_size = (frame_pos - k_frame_overhead) - prev_frame_pos;
		result.push_back(std::pair<int, int>{ frame_pos, prev_size });

		frame_pos = prev_frame_pos;
	}
	return result;
}

json_t interpreter_stack_t::stack_to_json() const{
	const int size = static_cast<int>(_stack_size);

	const auto stack_frames = get_stack_frames(get_current_frame_pos()._frame_pos);

	vector<json_t> frames;
	for(int i = 0 ; i < stack_frames.size() ; i++){
		auto a = json_t::make_array({
			json_t(i),
			json_t(stack_frames[i].first),
			json_t(stack_frames[i].second)
		});
		frames.push_back(a);
	}

	vector<json_t> elements;
	for(int i = 0 ; i < size ; i++){
		const auto& frame_it = std::find_if(
			stack_frames.begin(),
			stack_frames.end(),
			[&i](const std::pair<int, int>& e) { return e.first == i; }
		);

		bool frame_start_flag = frame_it != stack_frames.end();

		if(frame_start_flag){
			elements.push_back(json_t(""));
		}

#if DEBUG
#if DEBUG
		const auto debug_type = _debug_types[i];
		const auto ext = bc_value_t::is_bc_ext(debug_type.get_base_type());
#endif
		const auto bc_pod = _entries[i];
#if DEBUG
		const auto bc = bc_value_t(debug_type, bc_pod, ext);
#else
		const auto bc = bc_value_t(bc_pod, ext);
#endif

		bool unwritten = ext && bc._pod._ext->_is_unwritten_ext_value;

		auto a = json_t::make_array({
			json_t(i),
			typeid_to_ast_json(debug_type, json_tags::k_plain)._value,
			unwritten ? json_t("UNWRITTEN") : value_to_ast_json(bc_to_value(bc, debug_type), json_tags::k_plain)._value
		});
		elements.push_back(a);
#endif
	}

	return json_t::make_object({
		{ "size", json_t(size) },
		{ "frames", json_t::make_array(frames) },
		{ "elements", json_t::make_array(elements) }
	});
}




//////////////////////////////////////////		GLOBAL FUNCTIONS



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
		const auto index = static_cast<int>(it - symbols.begin());
		const auto pos = get_global_n_pos(index);
		QUARK_ASSERT(pos >= 0 && pos < vm._stack.size());

		const auto value_entry = value_entry_t{
			vm._stack.load_value_slow(pos, it->second._value_type),
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
	QUARK_ASSERT(vm.check_invariant());

	return get_global(vm, s);
}
value_t get_global(const interpreter_t& vm, const std::string& name){
	QUARK_ASSERT(vm.check_invariant());

	const auto& result = find_global_symbol2(vm, name);
	if(result == nullptr){
		throw std::runtime_error("Cannot find global.");
	}
	else{
		return bc_to_value(result->_value, result->_symbol._value_type);
	}
}

inline const bc_function_definition_t& get_function_def(const interpreter_t& vm, int function_id){
	QUARK_ASSERT(vm.check_invariant());

	QUARK_ASSERT(function_id >= 0 && function_id < vm._imm->_program._function_defs.size())

	const auto& function_def = vm._imm->_program._function_defs[function_id];
	return function_def;
}


bc_value_t construct_value_from_typeid(interpreter_t& vm, const typeid_t& type, const typeid_t& arg0_type, const vector<bc_value_t>& arg_values){
	QUARK_ASSERT(vm.check_invariant());
	QUARK_ASSERT(type.check_invariant());

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
//		QUARK_TRACE(to_compact_string2(instance));

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

	const auto& function_def = get_function_def(vm, f.get_function_value());
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

		vm._stack.save_frame();


		//	We push the values to the stack = the stack will take RC ownership of the values.
		vector<bool> exts;
		for(int i = 0 ; i < args.size() ; i++){
			const auto bc = value_to_bc(args[i]);
			bool is_ext = bc_value_t::is_bc_ext(args[i].get_basetype());
			exts.push_back(is_ext);
			vm._stack.push_value(bc, is_ext);
		}

		vm._stack.open_frame(*function_def._frame, static_cast<int>(args.size()));
		const auto& result = execute_instructions(vm, function_def._frame->_instrs2);
		vm._stack.close_frame(*function_def._frame);
		vm._stack.pop_batch(exts);
		vm._stack.restore_frame();

		if(result.first){
			return bc_to_value(result.second, f.get_type().get_function_return());
		}
		else{
			return value_t::make_undefined();
		}
	}
}





//////////////////////////////////////////		INSTRUCTIONS



QUARK_UNIT_TEST("", "", "", ""){
	const auto s = sizeof(bc_instruction_t);
	QUARK_UT_VERIFY(s == 32);
}



QUARK_UNIT_TEST("", "", "", ""){
	const auto value_size = sizeof(bc_value_t);

/*
	QUARK_UT_VERIFY(value_size == 16);
	QUARK_UT_VERIFY(expression_size == 40);
	QUARK_UT_VERIFY(e_count_offset == 4);
	QUARK_UT_VERIFY(e_offset == 8);
	QUARK_UT_VERIFY(value_offset == 16);
*/



//	QUARK_UT_VERIFY(sizeof(temp) == 56);
}

QUARK_UNIT_TEST("", "", "", ""){
	const auto s = sizeof(variable_address_t);
	QUARK_UT_VERIFY(s == 8);
}
QUARK_UNIT_TEST("", "", "", ""){
	const auto s = sizeof(bc_value_t);
//	QUARK_UT_VERIFY(s == 16);
}



//??? Make special casting-opcode. This way new_1 don't need to use stack.
//	IMPORTANT: NO arguments are passed as DYN arguments.
void execute_new_1(interpreter_t& vm, const bc_instruction2_t& instruction){
	QUARK_ASSERT(vm.check_invariant());
	QUARK_ASSERT(instruction.check_invariant());

	const auto dest_reg = instruction._a;
	const auto target_itype = instruction._b;
	const auto source_itype = instruction._c;


	const auto& target_type = get_type(vm, target_itype);
	QUARK_ASSERT(target_type.is_vector() == false && target_type.is_dict() == false && target_type.is_struct() == false);

	const int arg0_stack_pos = vm._stack.size() - 1;
	const auto input_value_type = get_type(vm, source_itype);
	const auto input_value = vm._stack.load_value_slow(arg0_stack_pos + 0, input_value_type);

	const bc_value_t result = [&]{
		if(target_type.is_bool() || target_type.is_int() || target_type.is_float() || target_type.is_typeid()){
			return input_value;
		}
		else if(target_type.is_string()){
			if(input_value_type.is_json_value() && input_value.get_json_value().is_string()){
				return bc_value_t::make_string(input_value.get_json_value().get_string());
			}
			else{
				return input_value;
			}
		}
		else if(target_type.is_json_value()){
			const auto arg = bc_to_value(input_value, input_value_type);
			const auto value = value_to_ast_json(arg, json_tags::k_plain);
			return bc_value_t::make_json_value(value._value);
		}
		else{
			return input_value;
		}
	}();

	vm._stack.write_register(dest_reg, result);
}

//??? should use itype internaly, not typeid_t.
//??? Split out instruction unpacking to client.
//	IMPORTANT: NO arguments are passed as DYN arguments.
void execute_new_vector(interpreter_t& vm, const bc_instruction2_t& instruction){
	QUARK_ASSERT(vm.check_invariant());
	QUARK_ASSERT(instruction.check_invariant());

	const auto dest_reg = instruction._a;
	const auto target_itype = instruction._b;
	const auto arg_count = instruction._c;

	const auto& target_type = get_type(vm, target_itype);
	QUARK_ASSERT(target_type.is_vector());

	const auto& element_type = target_type.get_vector_element_type();
	QUARK_ASSERT(element_type.is_undefined() == false);
	QUARK_ASSERT(target_type.is_undefined() == false);

	const int arg0_stack_pos = vm._stack.size() - arg_count;
	std::vector<bc_value_t> elements2;
	for(int i = 0 ; i < arg_count ; i++){
		//??? Replace load_value_slow().
		const auto arg_bc = vm._stack.load_value_slow(arg0_stack_pos + i, element_type);
		elements2.push_back(arg_bc);
	}

	const auto result = bc_value_t::make_vector_value(element_type, elements2);
	vm._stack.write_register_obj(dest_reg, result);
}

//	IMPORTANT: NO arguments are passed as DYN arguments.
void execute_new_dict(interpreter_t& vm, const bc_instruction2_t& instruction){
	QUARK_ASSERT(vm.check_invariant());
	QUARK_ASSERT(instruction.check_invariant());

	const auto dest_reg = instruction._a;
	const auto target_itype = instruction._b;
	const auto arg_count = instruction._c;

	const auto& target_type = get_type(vm, target_itype);
	QUARK_ASSERT(target_type.is_dict());

	const int arg0_stack_pos = vm._stack.size() - arg_count;

	const auto& element_type = target_type.get_dict_value_type();
	QUARK_ASSERT(target_type.is_undefined() == false);
	QUARK_ASSERT(element_type.is_undefined() == false);

	const auto string_type = typeid_t::make_string();

	std::map<string, bc_value_t> elements2;
	int dict_element_count = arg_count / 2;
	for(auto i = 0 ; i < dict_element_count ; i++){
		//??? Replace load_value_slow().
		const auto key = vm._stack.load_value_slow(arg0_stack_pos + i * 2 + 0, string_type);
		const auto value = vm._stack.load_value_slow(arg0_stack_pos + i * 2 + 1, element_type);
		const auto key2 = key.get_string_value();
		elements2.insert({ key2, value });
	}

	const auto result = bc_value_t::make_dict_value(element_type, elements2);
	vm._stack.write_register_obj(dest_reg, result);
}

void execute_new_struct(interpreter_t& vm, const bc_instruction2_t& instruction){
	QUARK_ASSERT(vm.check_invariant());
	QUARK_ASSERT(instruction.check_invariant());

	const auto dest_reg = instruction._a;
	const auto target_itype = instruction._b;
	const auto arg_count = instruction._c;

	const auto& target_type = get_type(vm, target_itype);
	QUARK_ASSERT(target_type.is_struct());

	const int arg0_stack_pos = vm._stack.size() - arg_count;

	const auto& struct_def = target_type.get_struct();
	std::vector<bc_value_t> elements2;
	for(int i = 0 ; i < arg_count ; i++){
		const auto member_type = struct_def._members[i]._type;
		const auto value = vm._stack.load_value_slow(arg0_stack_pos + i, member_type);
		elements2.push_back(value);
	}

	const auto result = bc_value_t::make_struct_value(target_type, elements2);
//	QUARK_TRACE(to_compact_string2(instance));

	vm._stack.write_register_obj(dest_reg, result);
}






QUARK_UNIT_TEST("", "", "", ""){
	float a = 10.0f;
	float b = 23.3f;

	bool r = a && b;
	QUARK_UT_VERIFY(r == true);
}




#if 0
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

	}
}
#endif



//??? use type of the register/frame, not the instruction!! Fix for all instructions!
//??? pass returns value(s) via parameters instead.
//???	Future: support dynamic Floyd functions too.

std::pair<bool, bc_value_t> execute_instructions(interpreter_t& vm, const std::vector<bc_instruction2_t>& instructions){
	QUARK_ASSERT(vm.check_invariant());

	interpreter_stack_t& stack = vm._stack;
	const bc_frame_t* frame_ptr = stack._current_frame;
	auto frame_pos = stack._current_frame_pos;

	bc_pod_value_t* registers = stack._current_frame_entry_ptr;
	bc_pod_value_t* globals = &stack._entries[k_frame_overhead];


	const typeid_t* type_lookup = &vm._imm->_program._types[0];
	const auto type_count = vm._imm->_program._types.size();

//	QUARK_TRACE_SS("STACK:  " << json_to_pretty_string(stack.stack_to_json()));

	const auto instruction_count = instructions.size();
	int pc = 0;
	while(true){
		if(pc == instruction_count){
			return { false, bc_value_t::make_undefined() };
		}
		QUARK_ASSERT(pc >= 0);
		QUARK_ASSERT(pc < instruction_count);
		const auto& instruction = instructions[pc];

		QUARK_ASSERT(vm.check_invariant());
		QUARK_ASSERT(instruction.check_invariant());

		QUARK_ASSERT(frame_pos == stack._current_frame_pos);
		QUARK_ASSERT(frame_ptr == stack._current_frame);
		QUARK_ASSERT(registers == stack._current_frame_entry_ptr);


		const auto opcode = instruction._opcode;
		switch(opcode){


		//////////////////////////////////////////		ACCESS GLOBALS


		case bc_opcode::k_load_global_obj: {
			QUARK_ASSERT(instruction._instr_type == k_no_bctypeid);
			QUARK_ASSERT(stack.check_register_access_obj(instruction._a));
			QUARK_ASSERT(stack.check_global_access_obj(instruction._b));

			bc_value_t::release_ext(registers[instruction._a]._ext);
			const auto& new_value_pod = globals[instruction._b];
			registers[instruction._a] = new_value_pod;
			new_value_pod._ext->_rc++;
			pc++;
			break;
		}
		case bc_opcode::k_load_global_intern: {
			QUARK_ASSERT(instruction._instr_type == k_no_bctypeid);
			QUARK_ASSERT(stack.check_register_access_intern(instruction._a));

			registers[instruction._a] = globals[instruction._b];
			pc++;
			break;
		}


		case bc_opcode::k_store_global_obj: {
			QUARK_ASSERT(instruction._instr_type == k_no_bctypeid);
			QUARK_ASSERT(stack.check_global_access_obj(instruction._a));
			QUARK_ASSERT(stack.check_register_access_obj(instruction._b));

			bc_value_t::release_ext(globals[instruction._a]._ext);
			const auto& new_value_pod = registers[instruction._b];
			globals[instruction._a] = new_value_pod;
			new_value_pod._ext->_rc++;
			pc++;
			break;
		}
		case bc_opcode::k_store_global_intern: {
			QUARK_ASSERT(instruction._instr_type == k_no_bctypeid);
			QUARK_ASSERT(stack.check_global_access_intern(instruction._a));
			QUARK_ASSERT(stack.check_register_access_intern(instruction._b));

			globals[instruction._a] = registers[instruction._b];
			pc++;
			break;
		}


		//////////////////////////////////////////		ACCESS LOCALS


		case bc_opcode::k_store_local_intern: {
			QUARK_ASSERT(instruction._instr_type == k_no_bctypeid);
			QUARK_ASSERT(stack.check_register_access_intern(instruction._a));
			QUARK_ASSERT(stack.check_register_access_intern(instruction._b));

			registers[instruction._a] = registers[instruction._b];
			pc++;
			break;
		}
		case bc_opcode::k_store_local_obj: {
			QUARK_ASSERT(instruction._instr_type == k_no_bctypeid);
			QUARK_ASSERT(stack.check_register_access_obj(instruction._a));
			QUARK_ASSERT(stack.check_register_access_obj(instruction._b));

			bc_value_t::release_ext(registers[instruction._a]._ext);
			const auto& new_value_pod = registers[instruction._b];
			registers[instruction._a] = new_value_pod;
			new_value_pod._ext->_rc++;
			pc++;
			break;
		}


		//////////////////////////////////////////		STACK


		case bc_opcode::k_return: {
			QUARK_ASSERT(instruction._instr_type == k_no_bctypeid);
			bool is_ext = frame_ptr->_exts[instruction._a];

			QUARK_ASSERT(
				(is_ext && stack.check_register_access_obj(instruction._a))
				|| (!is_ext && stack.check_register_access_intern(instruction._a))
			);

#if DEBUG
			return { true, bc_value_t(frame_ptr->_symbols[instruction._a].second._value_type, registers[instruction._a], is_ext) };
#else
			return { true, bc_value_t(registers[instruction._a], is_ext) };
#endif
		}

		case bc_opcode::k_push_frame_ptr: {
			QUARK_ASSERT(vm.check_invariant());
			QUARK_ASSERT((stack._stack_size + 2) < stack._allocated_count)

			stack._entries[stack._stack_size + 0]._int = frame_pos;
			stack._entries[stack._stack_size + 1]._frame_ptr = frame_ptr;
			stack._stack_size += 2;
#if DEBUG
			stack._debug_types.push_back(typeid_t::make_int());
			stack._debug_types.push_back(typeid_t::make_void());
#endif
			pc++;
			break;
		}
		case bc_opcode::k_pop_frame_ptr: {
			stack.restore_frame();

			frame_ptr = stack._current_frame;
			frame_pos = stack._current_frame_pos;
			registers = stack._current_frame_entry_ptr;

			QUARK_ASSERT(frame_pos == stack._current_frame_pos);
			QUARK_ASSERT(frame_ptr == stack._current_frame);
			QUARK_ASSERT(registers == stack._current_frame_entry_ptr);
			pc++;
			break;
		}

		case bc_opcode::k_push_intern: {
			QUARK_ASSERT(instruction._instr_type == k_no_bctypeid);

			QUARK_ASSERT(stack.check_reg(instruction._a));
#if DEBUG
			const auto info = stack.get_register_info2(instruction._a);
			QUARK_ASSERT(bc_value_t::is_bc_ext(info->second._value_type.get_base_type()) == false);
#endif

			stack._entries[stack._stack_size] = registers[instruction._a];
			stack._stack_size++;
#if DEBUG
			stack._debug_types.push_back(stack._debug_types[frame_pos + instruction._a]);
#endif
			QUARK_ASSERT(stack.check_invariant());
			pc++;
			break;
		}
		case bc_opcode::k_push_obj: {
			QUARK_ASSERT(instruction._instr_type == k_no_bctypeid);

			const auto value = stack.read_register_obj(instruction._a);
			stack.push_value(value, true);
			pc++;
			break;
		}
		case bc_opcode::k_popn: {
			QUARK_ASSERT(instruction._instr_type == k_no_bctypeid);

			const uint32_t n = instruction._a;
			const uint32_t extbits = instruction._b;
			stack.pop_batch(n, extbits);
			pc++;
			break;
		}


		//////////////////////////////////////////		BRANCHING


		case bc_opcode::k_branch_false_bool: {
			QUARK_ASSERT(instruction._instr_type == k_no_bctypeid);
			QUARK_ASSERT(stack.check_register_access_bool(instruction._a));

			const auto value = registers[instruction._a]._bool;
			if(value == false){
				const auto offset = instruction._b;
				pc = pc + offset;
			}
			else{
				pc++;
			}
			break;
		}
		case bc_opcode::k_branch_true_bool: {
			QUARK_ASSERT(instruction._instr_type == k_no_bctypeid);
			QUARK_ASSERT(stack.check_register_access_bool(instruction._a));

			const auto value = registers[instruction._a]._bool;
			if(value == true){
				const auto offset = instruction._b;
				pc = pc + offset;
			}
			else{
				pc++;
			}
			break;
		}
		case bc_opcode::k_branch_zero_int: {
			QUARK_ASSERT(instruction._instr_type == k_no_bctypeid);
			QUARK_ASSERT(stack.check_register_access_int(instruction._a));

			const auto value = registers[instruction._a]._int;
			if(value == 0){
				const auto offset = instruction._b;
				pc = pc + offset;
			}
			else{
				pc++;
			}
			break;
		}
		case bc_opcode::k_branch_notzero_int: {
			QUARK_ASSERT(instruction._instr_type == k_no_bctypeid);
			QUARK_ASSERT(stack.check_register_access_int(instruction._a));

			const auto value = registers[instruction._a]._int;
			if(value != 0){
				const auto offset = instruction._b;
				pc = pc + offset;
			}
			else{
				pc++;
			}
			break;
		}
		case bc_opcode::k_branch_always: {
			const auto offset = instruction._a;
			pc = pc + offset;
			break;
		}


		//////////////////////////////////////////		COMPLEX


		case bc_opcode::k_resolve_member: {
			QUARK_ASSERT(instruction._instr_type >= 0 && instruction._instr_type < type_count);
			const auto& parent_value = stack.read_register_obj(instruction._b);
			const auto& member_index = instruction._c;
			QUARK_ASSERT(member_index != -1);

			const auto& struct_instance = parent_value.get_struct_value();
			const bc_value_t value = struct_instance[member_index];
			stack.write_register(instruction._a, value);
			pc++;
			break;
		}


		case bc_opcode::k_lookup_element_string: {
			QUARK_ASSERT(instruction._instr_type == k_no_bctypeid);

			const auto& s = stack.peek_register_string(instruction._b);
			QUARK_ASSERT(stack.check_register_access_int(instruction._c));
			const auto lookup_index = registers[instruction._c]._int;
			if(lookup_index < 0 || lookup_index >= s.size()){
				throw std::runtime_error("Lookup in string: out of bounds.");
			}
			else{
				const char ch = s[lookup_index];
				const auto value2 = string(1, ch);
				stack.write_register_string(instruction._a, value2);
			}
			pc++;
			break;
		}

		case bc_opcode::k_lookup_element_json_value: {
			QUARK_ASSERT(instruction._instr_type >= 0 && instruction._instr_type < type_count);

			//	Notice: the exact type of value in the json_value is only known at runtime = must be checked in interpreter.
			const auto& parent_value = stack.read_register_obj(instruction._b);
			const auto& parent_json_value = parent_value.get_json_value();
			if(parent_json_value.is_object()){
				const auto& lookup_key = stack.peek_register_string(instruction._c);

				//??? Optimize json_value:int to be intern.

				//	get_object_element() throws if key can't be found.
				const auto& value = parent_json_value.get_object_element(lookup_key);
				const auto value2 = bc_value_t::make_json_value(value);
				stack.write_register_obj(instruction._a, value2);
			}
			else if(parent_json_value.is_array()){
				QUARK_ASSERT(stack.check_register_access_int(instruction._c));
				const auto lookup_index = registers[instruction._c]._int;

				if(lookup_index < 0 || lookup_index >= parent_json_value.get_array_size()){
					throw std::runtime_error("Lookup in json_value array: out of bounds.");
				}
				else{
					const auto& value = parent_json_value.get_array_n(lookup_index);
					const auto value2 = bc_value_t::make_json_value(value);
					stack.write_register_obj(instruction._a, value2);
				}
			}
			else{
				throw std::runtime_error("Lookup using [] on json_value only works on objects and arrays.");
			}
			pc++;
			break;
		}

		case bc_opcode::k_lookup_element_vector: {
			const auto* vec = stack.peek_register_vector(instruction._b);

			QUARK_ASSERT(stack.check_register_access_int(instruction._c));
			const auto lookup_index = registers[instruction._c]._int;

			if(lookup_index < 0 || lookup_index >= (*vec).size()){
				throw std::runtime_error("Lookup in vector: out of bounds.");
			}
			else{
				QUARK_ASSERT(instruction._instr_type >= 0 && instruction._instr_type < type_count);
				const auto& parent_type = type_lookup[instruction._instr_type];
				const auto element_type = parent_type.get_vector_element_type();
				const bc_value_t value = (*vec)[lookup_index];
				stack.write_register(instruction._a, value);
			}
			pc++;
			break;
		}

		case bc_opcode::k_lookup_element_dict: {
			QUARK_ASSERT(instruction._instr_type >= 0 && instruction._instr_type < type_count);

			const auto& parent_value = stack.read_register_obj(instruction._b);
			const auto& lookup_key = stack.peek_register_string(instruction._c);
			const auto& entries = parent_value.get_dict_value();
			const auto& found_it = entries.find(lookup_key);
			if(found_it == entries.end()){
				throw std::runtime_error("Lookup in dict: key not found.");
			}
			else{
				const bc_value_t value = found_it->second;
				stack.write_register(instruction._a, value);
			}
			pc++;
			break;
		}

		/*
			??? IDEA Don't use push/pop to send arguments for calls -- copy arguments directly to new stackframe.

			??? Make stub bc_frame_t for each host function to make call conventions same as Floyd functions.
		*/

		/*
			Performs call to a function
			1) Reads arguments from (leaves them there).
			2) Sets up stack frame for new function and puts argments into its locals.
			3) Runs the instructions in the function.
			4) Destroys functions stack frame.
			5) Writes the function return via the call-instruction's output register.
		*/

		//	Notice: host calls and floyd calls have the same type -- we cannot detect host calls until we have a callee value.
		case bc_opcode::k_call: {
			QUARK_ASSERT(vm.check_invariant());
			QUARK_ASSERT(instruction._instr_type == k_no_bctypeid);

			const int callee_arg_count = instruction._c;

			const int function_id = stack.read_register_function(instruction._b);
			QUARK_ASSERT(function_id >= 0 && function_id < vm._imm->_program._function_defs.size())
			const auto& function_def = vm._imm->_program._function_defs[function_id];
			const int function_def_dynamic_arg_count = function_def._dyn_arg_count;
			const auto& function_return_type = function_def._function_type.get_function_return();

			//	_e[...] contains first callee, then each argument.
			//	We need to examine the callee, since we support magic argument lists of varying size.

			QUARK_ASSERT(function_def._args.size() == callee_arg_count);

			if(function_def._host_function_id != 0){
				const auto& host_function = vm._imm->_host_functions.at(function_def._host_function_id);

				const int arg0_stack_pos = stack.size() - (function_def_dynamic_arg_count + callee_arg_count);
				int stack_pos = arg0_stack_pos;

				//??? We can now access parameters using registers!
				//	Notice that dynamic functions will have each DYN argument with a leading itype as an extra argument.
				const auto function_def_arg_count = function_def._args.size();
				std::vector<value_t> arg_values;
				for(int i = 0 ; i < function_def_arg_count ; i++){
					const auto& func_arg_type = function_def._args[i]._type;
					if(func_arg_type.is_internal_dynamic()){
						const auto arg_itype = stack.load_intq(stack_pos);
						const auto& arg_type = get_type(vm, static_cast<int16_t>(arg_itype));
						const auto arg_bc = stack.load_value_slow(stack_pos + 1, arg_type);

						const auto arg_value = bc_to_value(arg_bc, arg_type);
						arg_values.push_back(arg_value);
						stack_pos += 2;
					}
					else{
						const auto arg_bc = stack.load_value_slow(stack_pos + 0, func_arg_type);
						const auto arg_value = bc_to_value(arg_bc, func_arg_type);
						arg_values.push_back(arg_value);
						stack_pos++;
					}
				}

				const auto& result = (host_function)(vm, arg_values);
				const auto bc_result = value_to_bc(result);

				if(function_return_type.is_void() == true){
				}
				else if(function_return_type.is_internal_dynamic()){
					stack.write_register(instruction._a, bc_result);
				}
				else{
					stack.write_register(instruction._a, bc_result);
				}
			}
			else{
				QUARK_ASSERT(function_def_dynamic_arg_count == 0);

				//	We need to remember the global pos where to store return value, since we're switching frame to call function.
				int result_reg_pos = stack._current_frame_pos + instruction._a;

				stack.open_frame(*function_def._frame, callee_arg_count);
				const auto& result = execute_instructions(vm, function_def._frame->_instrs2);
				stack.close_frame(*function_def._frame);


				//	Update our cached pointers.
				frame_ptr = stack._current_frame;
				frame_pos = stack._current_frame_pos;
				registers = stack._current_frame_entry_ptr;

				QUARK_ASSERT(result.first);
				if(function_return_type.is_void() == false){

					//	Cannot store via register, we have not yet executed k_pop_frame_ptr that restores our frame.
					if(function_def._return_is_ext){
						stack.replace_obj(result_reg_pos, result.second);
					}
					else{
						stack.replace_intern(result_reg_pos, result.second);
					}
				}
			}

			QUARK_ASSERT(frame_pos == stack._current_frame_pos);
			QUARK_ASSERT(frame_ptr == stack._current_frame);
			QUARK_ASSERT(registers == stack._current_frame_entry_ptr);
			QUARK_ASSERT(vm.check_invariant());
			pc++;
			break;
		}

		case bc_opcode::k_new_1: {
			execute_new_1(vm, instruction);
			pc++;
			break;
		}

		case bc_opcode::k_new_vector: {
			execute_new_vector(vm, instruction);
			pc++;
			break;
		}

		case bc_opcode::k_new_dict: {
			execute_new_dict(vm, instruction);
			pc++;
			break;
		}
		case bc_opcode::k_new_struct: {
			execute_new_struct(vm, instruction);
			pc++;
			break;
		}


		//////////////////////////////		COMPARISON


		case bc_opcode::k_comparison_smaller_or_equal: {
			QUARK_ASSERT(instruction._instr_type >= 0 && instruction._instr_type < type_count);
			const auto& type = type_lookup[instruction._instr_type];
			QUARK_ASSERT(type.is_int() == false);
			QUARK_ASSERT(stack.check_register_access_bool(instruction._a));

			const auto left = stack.read_register(instruction._b);
			const auto right = stack.read_register(instruction._c);
			QUARK_ASSERT(left.get_debug_type() == right.get_debug_type());

			long diff = bc_value_t::compare_value_true_deep(left, right, type);

			registers[instruction._a]._bool = diff <= 0;
			pc++;
			break;
		}
		case bc_opcode::k_comparison_smaller_or_equal_int: {
			QUARK_ASSERT(stack.check_register_access_bool(instruction._a));
			QUARK_ASSERT(stack.check_register_access_int(instruction._b));
			QUARK_ASSERT(stack.check_register_access_int(instruction._c));

			const auto left = registers[instruction._b]._int;
			const auto right = registers[instruction._c]._int;
			registers[instruction._a]._bool = left <= right;
			pc++;
			break;
		}

		case bc_opcode::k_comparison_smaller: {
			QUARK_ASSERT(instruction._instr_type >= 0 && instruction._instr_type < type_count);
			QUARK_ASSERT(stack.check_register_access_bool(instruction._a));
			const auto& type = type_lookup[instruction._instr_type];
			QUARK_ASSERT(type.is_int() == false);
			const auto left = stack.read_register(instruction._b);
			const auto right = stack.read_register(instruction._c);
		#if DEBUG
			QUARK_ASSERT(left.get_debug_type() == right.get_debug_type());
		#endif
			long diff = bc_value_t::compare_value_true_deep(left, right, type);

			registers[instruction._a]._bool = diff < 0;
			pc++;
			break;
		}
		case bc_opcode::k_comparison_smaller_int: {
			QUARK_ASSERT(stack.check_register_access_bool(instruction._a));
			QUARK_ASSERT(stack.check_register_access_int(instruction._b));
			QUARK_ASSERT(stack.check_register_access_int(instruction._c));

			const auto left = registers[instruction._b]._int;
			const auto right = registers[instruction._c]._int;
			registers[instruction._a]._bool = left < right;
			pc++;
			break;
		}

		case bc_opcode::k_comparison_larger_or_equal: {
			QUARK_ASSERT(stack.check_register_access_bool(instruction._a));
			QUARK_ASSERT(instruction._instr_type >= 0 && instruction._instr_type < type_count);
			const auto& type = type_lookup[instruction._instr_type];
			QUARK_ASSERT(type.is_int() == false);
			const auto left = stack.read_register(instruction._b);
			const auto right = stack.read_register(instruction._c);
			QUARK_ASSERT(left.get_debug_type() == right.get_debug_type());

			long diff = bc_value_t::compare_value_true_deep(left, right, type);
			registers[instruction._a]._bool = diff >= 0;
			pc++;
			break;
		}
		case bc_opcode::k_comparison_larger_or_equal_int: {
			QUARK_ASSERT(stack.check_register_access_bool(instruction._a));
			QUARK_ASSERT(stack.check_register_access_int(instruction._b));
			QUARK_ASSERT(stack.check_register_access_int(instruction._c));

			const auto left = registers[instruction._b]._int;
			const auto right = registers[instruction._c]._int;
			registers[instruction._a]._bool = left >= right;
			pc++;
			break;
		}

		case bc_opcode::k_comparison_larger: {
			QUARK_ASSERT(instruction._instr_type >= 0 && instruction._instr_type < type_count);
			QUARK_ASSERT(stack.check_register_access_bool(instruction._a));
			const auto& type = type_lookup[instruction._instr_type];
			QUARK_ASSERT(type.is_int() == false);
			const auto left = stack.read_register(instruction._b);
			const auto right = stack.read_register(instruction._c);
		#if DEBUG
			QUARK_ASSERT(left.get_debug_type() == right.get_debug_type());
		#endif
			long diff = bc_value_t::compare_value_true_deep(left, right, type);
			registers[instruction._a]._bool = diff > 0;
			pc++;
			break;
		}
		case bc_opcode::k_comparison_larger_int: {
			QUARK_ASSERT(stack.check_register_access_bool(instruction._a));
			QUARK_ASSERT(stack.check_register_access_int(instruction._b));
			QUARK_ASSERT(stack.check_register_access_int(instruction._c));

			const auto left = registers[instruction._b]._int;
			const auto right = registers[instruction._c]._int;

			registers[instruction._a]._bool = left > right;
			pc++;
			break;
		}

		case bc_opcode::k_logical_equal: {
			QUARK_ASSERT(instruction._instr_type >= 0 && instruction._instr_type < type_count);
			QUARK_ASSERT(stack.check_register_access_bool(instruction._a));
			const auto& type = type_lookup[instruction._instr_type];
			QUARK_ASSERT(type.is_int() == false);
			const auto left = stack.read_register(instruction._b);
			const auto right = stack.read_register(instruction._c);
		#if DEBUG
			QUARK_ASSERT(left.get_debug_type() == right.get_debug_type());
		#endif
			long diff = bc_value_t::compare_value_true_deep(left, right, type);
			registers[instruction._a]._bool = diff == 0;
			pc++;
			break;
		}
		case bc_opcode::k_logical_equal_int: {
			QUARK_ASSERT(stack.check_register_access_bool(instruction._a));
			QUARK_ASSERT(stack.check_register_access_int(instruction._b));
			QUARK_ASSERT(stack.check_register_access_int(instruction._c));
			const auto left = registers[instruction._b]._int;
			const auto right = registers[instruction._c]._int;

			registers[instruction._a]._bool = left == right;
			pc++;
			break;
		}

		case bc_opcode::k_logical_nonequal: {
			QUARK_ASSERT(instruction._instr_type >= 0 && instruction._instr_type < type_count);
			QUARK_ASSERT(stack.check_register_access_bool(instruction._a));
			const auto& type = type_lookup[instruction._instr_type];
			QUARK_ASSERT(type.is_int() == false);
			const auto left = stack.read_register(instruction._b);
			const auto right = stack.read_register(instruction._c);
		#if DEBUG
			QUARK_ASSERT(left.get_debug_type() == right.get_debug_type());
		#endif
			long diff = bc_value_t::compare_value_true_deep(left, right, type);
			registers[instruction._a]._bool = diff != 0;
			pc++;
			break;
		}
		case bc_opcode::k_logical_nonequal_int: {
			QUARK_ASSERT(stack.check_register_access_bool(instruction._a));
			QUARK_ASSERT(stack.check_register_access_int(instruction._b));
			QUARK_ASSERT(stack.check_register_access_int(instruction._c));

			const auto left = registers[instruction._b]._int;
			const auto right = registers[instruction._c]._int;
			registers[instruction._a]._bool = left != right;
			pc++;
			break;
		}


		//////////////////////////////		ARITHMETICS


		case bc_opcode::k_add: {
			QUARK_ASSERT(instruction._instr_type >= 0 && instruction._instr_type < type_count);
			const auto& type = type_lookup[instruction._instr_type];
			const auto left = stack.read_register(instruction._b);
			const auto right = stack.read_register(instruction._c);
		#if DEBUG
			QUARK_ASSERT(left.get_debug_type() == right.get_debug_type());
		#endif
			const auto basetype = type.get_base_type();

			//	bool
			if(basetype == base_type::k_bool){
				const bool left2 = left.get_bool_value();
				const bool right2 = right.get_bool_value();
				registers[instruction._a]._bool = left2 + right2;
				pc++;
				break;
			}
			else if(basetype == base_type::k_int){
				QUARK_ASSERT(false);
			}
			//	float
			else if(basetype == base_type::k_float){
				const float left2 = left.get_float_value();
				const float right2 = right.get_float_value();
				stack.write_register_float(instruction._a, left2 + right2);
				pc++;
				break;
			}

			//	string
			else if(basetype == base_type::k_string){
				const auto& left2 = left.get_string_value();
				const auto& right2 = right.get_string_value();
				stack.write_register_string(instruction._a, left2 + right2);
				pc++;
				break;
			}

			//	vector
			else if(basetype == base_type::k_vector){
				const auto& element_type = type.get_vector_element_type();

				//	Copy vector into elements.
				auto elements2 = *left.get_vector_value();

				const auto& right_elements = right.get_vector_value();
				elements2.insert(elements2.end(), right_elements->begin(), right_elements->end());
				const auto& value2 = bc_value_t::make_vector_value(element_type, elements2);
				stack.write_register_obj(instruction._a, value2);
				pc++;
				break;
			}
			else{
				QUARK_ASSERT(false);
				throw std::exception();
			}
		}
		case bc_opcode::k_add_int: {
			QUARK_ASSERT(instruction._instr_type == k_no_bctypeid);
			QUARK_ASSERT(stack.check_register_access_int(instruction._a));
			QUARK_ASSERT(stack.check_register_access_int(instruction._b));
			QUARK_ASSERT(stack.check_register_access_int(instruction._c));

			const auto left = registers[instruction._b]._int;
			const auto right = registers[instruction._c]._int;
			registers[instruction._a]._int = left + right;

			pc++;
			break;
		}
		case bc_opcode::k_subtract: {
			QUARK_ASSERT(instruction._instr_type >= 0 && instruction._instr_type < type_count);
			const auto& type = type_lookup[instruction._instr_type];
			const auto left = stack.read_register(instruction._b);
			const auto right = stack.read_register(instruction._c);
		#if DEBUG
			QUARK_ASSERT(left.get_debug_type() == right.get_debug_type());
		#endif
			const auto basetype = type.get_base_type();

			//	int
			if(basetype == base_type::k_int){
				QUARK_ASSERT(false);
			}

			//	float
			else if(basetype == base_type::k_float){
				const float left2 = left.get_float_value();
				const float right2 = right.get_float_value();
				stack.write_register_float(instruction._a, left2 - right2);
				pc++;
				break;
			}

			else{
				QUARK_ASSERT(false);
				throw std::exception();
			}
		}
		case bc_opcode::k_subtract_int: {
			QUARK_ASSERT(instruction._instr_type == k_no_bctypeid);
			QUARK_ASSERT(stack.check_register_access_int(instruction._a));
			QUARK_ASSERT(stack.check_register_access_int(instruction._b));
			QUARK_ASSERT(stack.check_register_access_int(instruction._c));

			const auto left = registers[instruction._b]._int;
			const auto right = registers[instruction._c]._int;
			registers[instruction._a]._int = left - right;
			pc++;
			break;
		}
		case bc_opcode::k_multiply: {
			QUARK_ASSERT(instruction._instr_type >= 0 && instruction._instr_type < type_count);
			const auto& type = type_lookup[instruction._instr_type];
			const auto left = stack.read_register(instruction._b);
			const auto right = stack.read_register(instruction._c);
		#if DEBUG
			QUARK_ASSERT(left.get_debug_type() == right.get_debug_type());
		#endif
			const auto basetype = type.get_base_type();

			//	int
			if(basetype == base_type::k_int){
				QUARK_ASSERT(false);
			}

			//	float
			else if(basetype == base_type::k_float){
				const float left2 = left.get_float_value();
				const float right2 = right.get_float_value();
				stack.write_register_float(instruction._a, left2 * right2);
				pc++;
				break;
			}

			else{
				QUARK_ASSERT(false);
				throw std::exception();
			}
		}
		case bc_opcode::k_multiply_int: {
			QUARK_ASSERT(instruction._instr_type == k_no_bctypeid);
			QUARK_ASSERT(stack.check_register_access_int(instruction._a));
			QUARK_ASSERT(stack.check_register_access_int(instruction._c));
			QUARK_ASSERT(stack.check_register_access_int(instruction._c));

			const auto left = registers[instruction._b]._int;
			const auto right = registers[instruction._c]._int;
			registers[instruction._a]._int = left * right;
			pc++;
			break;
		}
		case bc_opcode::k_divide: {
			QUARK_ASSERT(instruction._instr_type >= 0 && instruction._instr_type < type_count);
			const auto& type = type_lookup[instruction._instr_type];
			const auto left = stack.read_register(instruction._b);
			const auto right = stack.read_register(instruction._c);
		#if DEBUG
			QUARK_ASSERT(left.get_debug_type() == right.get_debug_type());
		#endif
			const auto basetype = type.get_base_type();

			//	int
			if(basetype == base_type::k_int){
				QUARK_ASSERT(false);
			}

			//	float
			else if(basetype == base_type::k_float){
				const float left2 = left.get_float_value();
				const float right2 = right.get_float_value();
				if(right2 == 0.0f){
					throw std::runtime_error("EEE_DIVIDE_BY_ZERO");
				}
				stack.write_register_float(instruction._a, left2 / right2);
				pc++;
				break;
			}

			else{
				QUARK_ASSERT(false);
				throw std::exception();
			}
		}
		case bc_opcode::k_divide_int: {
			QUARK_ASSERT(instruction._instr_type == k_no_bctypeid);
			QUARK_ASSERT(stack.check_register_access_int(instruction._a));
			QUARK_ASSERT(stack.check_register_access_int(instruction._b));
			QUARK_ASSERT(stack.check_register_access_int(instruction._c));

			const auto left = registers[instruction._b]._int;
			const auto right = registers[instruction._c]._int;
			if(right == 0.0f){
				throw std::runtime_error("EEE_DIVIDE_BY_ZERO");
			}
			registers[instruction._a]._int = left / right;
			pc++;
			break;
		}
		case bc_opcode::k_remainder: {
			QUARK_ASSERT(instruction._instr_type >= 0 && instruction._instr_type < type_count);
			const auto& type = type_lookup[instruction._instr_type];
			const auto left = stack.read_register(instruction._b);
			const auto right = stack.read_register(instruction._c);
		#if DEBUG
			QUARK_ASSERT(left.get_debug_type() == right.get_debug_type());
		#endif
			const auto basetype = type.get_base_type();

			//	int
			if(basetype == base_type::k_int){
				QUARK_ASSERT(false);
			}

			else{
				QUARK_ASSERT(false);
				throw std::exception();
			}
		}
		case bc_opcode::k_remainder_int: {
			QUARK_ASSERT(instruction._instr_type == k_no_bctypeid);
			QUARK_ASSERT(stack.check_register_access_int(instruction._a));
			QUARK_ASSERT(stack.check_register_access_int(instruction._b));
			QUARK_ASSERT(stack.check_register_access_int(instruction._c));

			const auto left = registers[instruction._b]._int;
			const auto right = registers[instruction._c]._int;

			if(right == 0){
				throw std::runtime_error("EEE_DIVIDE_BY_ZERO");
			}
			registers[instruction._a]._int = left % right;
			pc++;
			break;
		}
		case bc_opcode::k_logical_and: {
			QUARK_ASSERT(instruction._instr_type >= 0 && instruction._instr_type < type_count);
			QUARK_ASSERT(stack.check_register_access_bool(instruction._a));
			const auto& type = type_lookup[instruction._instr_type];
			const auto left = stack.read_register(instruction._b);
			const auto right = stack.read_register(instruction._c);
		#if DEBUG
			QUARK_ASSERT(left.get_debug_type() == right.get_debug_type());
		#endif
			const auto basetype = type.get_base_type();

			//	bool
			if(basetype == base_type::k_bool){
				const bool left2 = left.get_bool_value();
				const bool right2 = right.get_bool_value();
				registers[instruction._a]._bool = left2 && right2;
				pc++;
				break;
			}

			//	int
			else if(basetype == base_type::k_int){
				QUARK_ASSERT(false);
			}

			//	float
			else if(basetype == base_type::k_float){
				const float left2 = left.get_float_value();
				const float right2 = right.get_float_value();
				registers[instruction._a]._bool = (left2 != 0.0f) && (right2 != 0.0f);
				pc++;
				break;
			}

			else{
				QUARK_ASSERT(false);
				throw std::exception();
			}
		}
				//### Could be replaced by feature to convert any value to bool -- they use a generic comparison for && and ||
		case bc_opcode::k_logical_and_int: {
			QUARK_ASSERT(instruction._instr_type == k_no_bctypeid);
			QUARK_ASSERT(stack.check_register_access_bool(instruction._a));
			QUARK_ASSERT(stack.check_register_access_int(instruction._b));
			QUARK_ASSERT(stack.check_register_access_int(instruction._c));

			const auto left = registers[instruction._b]._int;
			const auto right = registers[instruction._c]._int;

			registers[instruction._a]._bool = (left != 0) && (right != 0);
			pc++;
			break;
		}
		case bc_opcode::k_logical_or: {
			QUARK_ASSERT(instruction._instr_type >= 0 && instruction._instr_type < type_count);
			QUARK_ASSERT(stack.check_register_access_bool(instruction._a));
			const auto& type = type_lookup[instruction._instr_type];
			const auto left = stack.read_register(instruction._b);
			const auto right = stack.read_register(instruction._c);
		#if DEBUG
			QUARK_ASSERT(left.get_debug_type() == right.get_debug_type());
		#endif
			const auto basetype = type.get_base_type();

			//	bool
			if(basetype == base_type::k_bool){
				const bool left2 = left.get_bool_value();
				const bool right2 = right.get_bool_value();
				registers[instruction._a]._bool = left2 || right2;
				pc++;
				break;
			}

			//	int
			else if(basetype == base_type::k_int){
				QUARK_ASSERT(false);
			}

			//	float
			else if(basetype == base_type::k_float){
				const float left2 = left.get_float_value();
				const float right2 = right.get_float_value();
				registers[instruction._a]._bool = (left2 != 0.0f) || (right2 != 0.0f);
				pc++;
				break;
			}

			else{
				QUARK_ASSERT(false);
				throw std::exception();
			}
		}
		case bc_opcode::k_logical_or_int: {
			QUARK_ASSERT(instruction._instr_type == k_no_bctypeid);
			QUARK_ASSERT(stack.check_register_access_bool(instruction._a));
			QUARK_ASSERT(stack.check_register_access_int(instruction._b));
			QUARK_ASSERT(stack.check_register_access_int(instruction._c));

			const auto left = registers[instruction._b]._int;
			const auto right = registers[instruction._c]._int;
			registers[instruction._a]._bool = (left != 0) || (right != 0);
			pc++;
			break;
		}


		//////////////////////////////		NONE


		default:
			QUARK_ASSERT(false);
			throw std::exception();
		}
	}
	return { false, bc_value_t::make_undefined() };
}






//////////////////////////////////////////		interpreter_t





interpreter_t::interpreter_t(const bc_program_t& program) :
	_stack(nullptr)
{
	QUARK_ASSERT(program.check_invariant());

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

	interpreter_stack_t temp(&_imm->_program._globals);
	temp.swap(_stack);
	_stack.save_frame();
	_stack.open_frame(_imm->_program._globals, 0);

	//	Run static intialization (basically run global instructions before calling main()).
	/*const auto& r =*/ execute_instructions(*this, _imm->_program._globals._instrs2);
	QUARK_ASSERT(check_invariant());
}

/*
interpreter_t::interpreter_t(const interpreter_t& other) :
	_imm(other._imm),
	_stack(other._stack),
	_current_stack_frame(other._current_stack_frame),
	_print_output(other._print_output)
{
	QUARK_ASSERT(other.check_invariant());
	QUARK_ASSERT(check_invariant());
}
*/

void interpreter_t::swap(interpreter_t& other) throw(){
	other._imm.swap(this->_imm);
	other._stack.swap(this->_stack);
	other._print_output.swap(this->_print_output);
}

/*
const interpreter_t& interpreter_t::operator=(const interpreter_t& other){
	auto temp = other;
	temp.swap(*this);
	return *this;
}
*/

#if DEBUG
bool interpreter_t::check_invariant() const {
	QUARK_ASSERT(_imm->_program.check_invariant());
	QUARK_ASSERT(_stack.check_invariant());
	return true;
}
#endif



//////////////////////////////////////////		FUNCTIONS



json_t interpreter_to_json(const interpreter_t& vm){
	vector<json_t> callstack;
	QUARK_ASSERT(vm.check_invariant());

	const auto stack = vm._stack.stack_to_json();

	return json_t::make_object({
		{ "ast", bcprogram_to_json(vm._imm->_program) },
		{ "callstack", stack }
	});
}


value_t call_host_function(interpreter_t& vm, int function_id, const std::vector<floyd::value_t>& args){
	QUARK_ASSERT(vm.check_invariant());
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
	QUARK_ASSERT(vm.check_invariant());

	if(vm._print_output.empty() == false){
		std::cout << "print output:\n";
		for(const auto& line: vm._print_output){
			std::cout << line << "\n";
		}
	}
}

std::shared_ptr<interpreter_t> run_global(const interpreter_context_t& context, const string& source){
	auto program = program_to_ast2(context, source);
	auto vm = make_shared<interpreter_t>(program);
//	QUARK_TRACE(json_to_pretty_string(interpreter_to_json(vm)));
	print_vm_printlog(*vm);
	return vm;
}

std::pair<std::shared_ptr<interpreter_t>, value_t> run_main(const interpreter_context_t& context, const string& source, const vector<floyd::value_t>& args){
	auto program = program_to_ast2(context, source);

	//	Runs global code.
	auto vm = make_shared<interpreter_t>(program);

	const auto& main_function = find_global_symbol2(*vm, "main");
	if(main_function != nullptr){
		const auto& result = call_function(*vm, bc_to_value(main_function->_value, main_function->_symbol._value_type), args);
		return { vm, result };
	}
	else{
		return {vm, value_t::make_undefined()};
	}
}

std::pair<std::shared_ptr<interpreter_t>, value_t> run_program(const interpreter_context_t& context, const bc_program_t& program, const vector<floyd::value_t>& args){
	auto vm = make_shared<interpreter_t>(program);

	const auto& main_func = find_global_symbol2(*vm, "main");
	if(main_func != nullptr){
		const auto& r = call_function(*vm, bc_to_value(main_func->_value, main_func->_symbol._value_type), args);
		return { vm, r };
	}
	else{
		return { vm, value_t::make_undefined() };
	}
}


}	//	floyd
