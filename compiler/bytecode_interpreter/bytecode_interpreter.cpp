//
//  parser_evaluator.cpp
//  Floyd
//
//  Created by Marcus Zetterquist on 26/07/16.
//  Copyright © 2016 Marcus Zetterquist. All rights reserved.
//

#include "bytecode_interpreter.h"

//??? temp
#include "floyd_network_component.h"

#include "floyd_runtime.h"
#include "bytecode_intrinsics.h"
#include "text_parser.h"
#include "ast_value.h"
#include "types.h"
#include "value_features.h"
#include "format_table.h"
#include <ffi.h>

#include <algorithm>

#include "floyd_corelib.h"

namespace floyd {

static const bool k_trace_stepping = false;


static std::string opcode_to_string(bc_opcode opcode);
static std::vector<json_t> bc_symbols_to_json(value_backend_t& backend, const symbol_table_t& symbols);


static type_t lookup_full_type(const interpreter_t& vm, const bc_typeid_t& type){
	const auto& backend = vm._backend;
	const auto& types = backend.types;
	return lookup_type_from_index(types, type);
}

static int get_global_n_pos(int n){
	return k_frame_overhead + n;
}
static int get_local_n_pos(int frame_pos, int n){
	return frame_pos + n;
}

//	Check memory layouts.
QUARK_TEST("", "", "", ""){
	const auto int_size = sizeof(int);
	QUARK_ASSERT(int_size == 4);

	const auto pod_size = sizeof(runtime_value_t);
	QUARK_ASSERT(pod_size == 8);

	const auto bcvalue_size = sizeof(rt_value_t);
	(void)bcvalue_size;
//	QUARK_ASSERT(bcvalue_size == 72);

	const auto instruction2_size = sizeof(bc_instruction_t);
	QUARK_ASSERT(instruction2_size == 8);


	const auto immer_vec_bool_size = sizeof(immer::vector<bool>);
	const auto immer_vec_int_size = sizeof(immer::vector<int>);
	const auto immer_vec_string_size = sizeof(immer::vector<std::string>);

	QUARK_ASSERT(immer_vec_bool_size == 32);
	QUARK_ASSERT(immer_vec_int_size == 32);
	QUARK_ASSERT(immer_vec_string_size == 32);


	const auto immer_stringkey_map_size = sizeof(immer::map<std::string, double>);
	QUARK_ASSERT(immer_stringkey_map_size == 16);

	const auto immer_intkey_map_size = sizeof(immer::map<uint32_t, double>);
	QUARK_ASSERT(immer_intkey_map_size == 16);
}

extern const std::map<bc_opcode, opcode_info_t> k_opcode_info = {
	{ bc_opcode::k_nop, { "nop", opcode_info_t::encoding::k_e_0000 }},

	{ bc_opcode::k_load_global_external_value, { "load_global_external_value", opcode_info_t::encoding::k_k_0ri0 } },
	{ bc_opcode::k_load_global_inplace_value, { "load_global_inplace_value", opcode_info_t::encoding::k_k_0ri0 } },

	{ bc_opcode::k_init_local, { "init_local", opcode_info_t::encoding::k_q_0rr0 } },

	{ bc_opcode::k_store_global_external_value, { "store_global_external_value", opcode_info_t::encoding::k_r_0ir0 } },
	{ bc_opcode::k_store_global_inplace_value, { "store_global_inplace_value", opcode_info_t::encoding::k_r_0ir0 } },

	{ bc_opcode::k_copy_reg_inplace_value, { "copy_reg_inplace_value", opcode_info_t::encoding::k_q_0rr0 } },
	{ bc_opcode::k_copy_reg_external_value, { "copy_reg_external_value", opcode_info_t::encoding::k_q_0rr0 } },

	{ bc_opcode::k_get_struct_member, { "get_struct_member", opcode_info_t::encoding::k_s_0rri } },

	{ bc_opcode::k_lookup_element_string, { "lookup_element_string", opcode_info_t::encoding::k_o_0rrr } },
	{ bc_opcode::k_lookup_element_json, { "lookup_element_jsonvalue", opcode_info_t::encoding::k_o_0rrr } },
	{ bc_opcode::k_lookup_element_vector_w_external_elements, { "lookup_element_vector_w_external_elements", opcode_info_t::encoding::k_o_0rrr } },
	{ bc_opcode::k_lookup_element_vector_w_inplace_elements, { "lookup_element_vector_w_inplace_elements", opcode_info_t::encoding::k_o_0rrr } },
	{ bc_opcode::k_lookup_element_dict_w_external_values, { "lookup_element_dict_w_external_values", opcode_info_t::encoding::k_o_0rrr } },
	{ bc_opcode::k_lookup_element_dict_w_inplace_values, { "lookup_element_dict_w_inplace_values", opcode_info_t::encoding::k_o_0rrr } },

	{ bc_opcode::k_get_size_vector_w_external_elements, { "get_size_vector_w_external_elements", opcode_info_t::encoding::k_q_0rr0 } },
	{ bc_opcode::k_get_size_vector_w_inplace_elements, { "get_size_vector_w_inplace_elements", opcode_info_t::encoding::k_q_0rr0 } },
	{ bc_opcode::k_get_size_dict_w_external_values, { "get_size_dict_w_external_values", opcode_info_t::encoding::k_q_0rr0 } },
	{ bc_opcode::k_get_size_dict_w_inplace_values, { "get_size_dict_w_inplace_values", opcode_info_t::encoding::k_q_0rr0 } },
	{ bc_opcode::k_get_size_string, { "get_size_string", opcode_info_t::encoding::k_q_0rr0 } },
	{ bc_opcode::k_get_size_jsonvalue, { "get_size_jsonvalue", opcode_info_t::encoding::k_q_0rr0 } },

	{ bc_opcode::k_pushback_vector_w_external_elements, { "pushback_vector_w_external_elements", opcode_info_t::encoding::k_o_0rrr } },
	{ bc_opcode::k_pushback_vector_w_inplace_elements, { "pushback_vector_w_inplace_elements", opcode_info_t::encoding::k_o_0rrr } },
	{ bc_opcode::k_pushback_string, { "pushback_string", opcode_info_t::encoding::k_o_0rrr } },

	{ bc_opcode::k_call, { "call", opcode_info_t::encoding::k_s_0rri } },

	{ bc_opcode::k_add_bool, { "add_bool", opcode_info_t::encoding::k_o_0rrr } },
	{ bc_opcode::k_add_int, { "add_int", opcode_info_t::encoding::k_o_0rrr } },
	{ bc_opcode::k_add_double, { "add_double", opcode_info_t::encoding::k_o_0rrr } },
	{ bc_opcode::k_concat_strings, { "concat_strings", opcode_info_t::encoding::k_o_0rrr } },
	{ bc_opcode::k_concat_vectors_w_external_elements, { "concat_vectors_w_external_elements", opcode_info_t::encoding::k_o_0rrr } },
//	{ bc_opcode::k_concat_vectors_w_inplace_elements, { "concat_vectors_pod64", opcode_info_t::encoding::k_o_0rrr } },
	{ bc_opcode::k_subtract_double, { "subtract_double", opcode_info_t::encoding::k_o_0rrr } },
	{ bc_opcode::k_subtract_int, { "subtract_int", opcode_info_t::encoding::k_o_0rrr } },
	{ bc_opcode::k_multiply_double, { "multiply_double", opcode_info_t::encoding::k_o_0rrr } },
	{ bc_opcode::k_multiply_int, { "multiply_int", opcode_info_t::encoding::k_o_0rrr } },
	{ bc_opcode::k_divide_double, { "divide_double", opcode_info_t::encoding::k_o_0rrr } },
	{ bc_opcode::k_divide_int, { "divide_int", opcode_info_t::encoding::k_o_0rrr } },
	{ bc_opcode::k_remainder_int, { "remainder_int", opcode_info_t::encoding::k_o_0rrr } },

	{ bc_opcode::k_logical_and_bool, { "logical_and_bool", opcode_info_t::encoding::k_o_0rrr } },
	{ bc_opcode::k_logical_and_int, { "logical_and_int", opcode_info_t::encoding::k_o_0rrr } },
	{ bc_opcode::k_logical_and_double, { "logical_and_double", opcode_info_t::encoding::k_o_0rrr } },
	{ bc_opcode::k_logical_or_bool, { "logical_or_bool", opcode_info_t::encoding::k_o_0rrr } },
	{ bc_opcode::k_logical_or_int, { "logical_or_int", opcode_info_t::encoding::k_o_0rrr } },
	{ bc_opcode::k_logical_or_double, { "logical_or_double", opcode_info_t::encoding::k_o_0rrr } },


	{ bc_opcode::k_comparison_smaller_or_equal, { "comparison_smaller_or_equal", opcode_info_t::encoding::k_o_0rrr } },
	{ bc_opcode::k_comparison_smaller_or_equal_int, { "comparison_smaller_or_equal_int", opcode_info_t::encoding::k_o_0rrr } },

	{ bc_opcode::k_comparison_smaller, { "comparison_smaller", opcode_info_t::encoding::k_o_0rrr } },
	{ bc_opcode::k_comparison_smaller_int, { "comparison_smaller_int", opcode_info_t::encoding::k_o_0rrr } },

	{ bc_opcode::k_logical_equal, { "logical_equal", opcode_info_t::encoding::k_o_0rrr } },
	{ bc_opcode::k_logical_equal_int, { "logical_equal_int", opcode_info_t::encoding::k_o_0rrr } },

	{ bc_opcode::k_logical_nonequal, { "logical_nonequal", opcode_info_t::encoding::k_o_0rrr } },
	{ bc_opcode::k_logical_nonequal_int, { "logical_nonequal_int", opcode_info_t::encoding::k_o_0rrr } },


	{ bc_opcode::k_new_1, { "new_1", opcode_info_t::encoding::k_t_0rii } },
	{ bc_opcode::k_new_vector_w_external_elements, { "new_vector_w_external_elements", opcode_info_t::encoding::k_t_0rii } },
	{ bc_opcode::k_new_vector_w_inplace_elements, { "new_vector_w_inplace_elements", opcode_info_t::encoding::k_t_0rii } },
	{ bc_opcode::k_new_dict_w_external_values, { "new_dict_w_external_values", opcode_info_t::encoding::k_t_0rii } },
	{ bc_opcode::k_new_dict_w_inplace_values, { "new_dict_w_inplace_values", opcode_info_t::encoding::k_t_0rii } },
	{ bc_opcode::k_new_struct, { "new_struct", opcode_info_t::encoding::k_t_0rii } },

	{ bc_opcode::k_return, { "return", opcode_info_t::encoding::k_p_0r00 } },
	{ bc_opcode::k_stop, { "stop", opcode_info_t::encoding::k_e_0000 } },

	{ bc_opcode::k_push_frame_ptr, { "push_frame_ptr", opcode_info_t::encoding::k_e_0000 } },
	{ bc_opcode::k_pop_frame_ptr, { "pop_frame_ptr", opcode_info_t::encoding::k_e_0000 } },
	{ bc_opcode::k_push_inplace_value, { "push_inplace_value", opcode_info_t::encoding::k_p_0r00 } },
	{ bc_opcode::k_push_external_value, { "push_external_value", opcode_info_t::encoding::k_p_0r00 } },
	{ bc_opcode::k_popn, { "popn", opcode_info_t::encoding::k_n_0ii0 } },

	{ bc_opcode::k_branch_false_bool, { "branch_false_bool", opcode_info_t::encoding::k_k_0ri0 } },
	{ bc_opcode::k_branch_true_bool, { "branch_true_bool", opcode_info_t::encoding::k_k_0ri0 } },
	{ bc_opcode::k_branch_zero_int, { "branch_zero_int", opcode_info_t::encoding::k_k_0ri0 } },
	{ bc_opcode::k_branch_notzero_int, { "branch_notzero_int", opcode_info_t::encoding::k_k_0ri0 } },

	{ bc_opcode::k_branch_smaller_int, { "branch_smaller_int", opcode_info_t::encoding::k_s_0rri } },
	{ bc_opcode::k_branch_smaller_or_equal_int, { "branch_smaller_or_equal_int", opcode_info_t::encoding::k_s_0rri } },

	{ bc_opcode::k_branch_always, { "branch_always", opcode_info_t::encoding::k_l_00i0 } }


};



//////////////////////////////////////////		bc_instruction_t


bc_instruction_t::bc_instruction_t(bc_opcode opcode, int16_t a, int16_t b, int16_t c) :
	_opcode(opcode),
	_zero(0),
	_a(a),
	_b(b),
	_c(c)
{
	QUARK_ASSERT(check_invariant());
}

#if DEBUG
bool bc_instruction_t::check_invariant() const {
	return true;
}
#endif


//////////////////////////////////////////		bc_static_frame_t


bc_static_frame_t::bc_static_frame_t(const types_t& types, const std::vector<bc_instruction_t>& instrs2, const symbol_table_t& symbols, const std::vector<type_t>& args) :
	_instructions(instrs2),
	_symbols(symbols),
	_arg_count((int)args.size()),
	_locals_count((int)_symbols._symbols.size() - (int)args.size())
{
	QUARK_ASSERT(types.check_invariant());

	for(int symbol_index = 0 ; symbol_index < symbols._symbols.size() ; symbol_index++){
		const auto& symbol_kv = symbols._symbols[symbol_index];
		const auto& symbol = symbol_kv.second;
		const auto type = symbol._value_type;

		if(symbol._symbol_type == symbol_t::symbol_type::immutable_reserve){
			_symbol_effective_type.push_back(type);
		}
		else if(symbol._symbol_type == symbol_t::symbol_type::immutable_arg){
			_symbol_effective_type.push_back(type);
		}
		else if(symbol._symbol_type == symbol_t::symbol_type::immutable_precalc){
			_symbol_effective_type.push_back(type);
			QUARK_ASSERT(type == symbol._init.get_type());
		}
		else if(symbol._symbol_type == symbol_t::symbol_type::named_type){
			_symbol_effective_type.push_back(type_t::make_typeid());
		}
		else if(symbol._symbol_type == symbol_t::symbol_type::mutable_reserve){
			_symbol_effective_type.push_back(type);
		}
		else {
			QUARK_ASSERT(false);
			throw std::exception();
		}
	}

	QUARK_ASSERT(check_invariant());
}

bool bc_static_frame_t::check_invariant() const {
	QUARK_ASSERT(_instructions.size() > 0);
	QUARK_ASSERT(_instructions.size() < 65000);

	QUARK_ASSERT(_symbols.check_invariant());
	QUARK_ASSERT(_arg_count >= 0 && _arg_count <= _symbols._symbols.size());
	QUARK_ASSERT(_locals_count >= 0 && _locals_count <= _symbols._symbols.size());
	QUARK_ASSERT(_arg_count + _locals_count == _symbols._symbols.size());
	QUARK_ASSERT(_symbol_effective_type.size() == _symbols._symbols.size());
	return true;
}


//////////////////////////////////////////		bc_function_definition_t


#if DEBUG
bool bc_function_definition_t::check_invariant() const {
	return true;
}
#endif



//////////////////////////////////////////		interpreter_stack_t



frame_pos_t interpreter_stack_t::read_frame_info(size_t pos) const{
	QUARK_ASSERT(check_invariant());

	const auto v = load_intq((int)(pos + 0));
	QUARK_ASSERT(v >= 0);

	const auto ptr = (const bc_static_frame_t*)_entries[pos + 1].frame_ptr;
	QUARK_ASSERT(ptr == nullptr || ptr->check_invariant());

	return frame_pos_t{ (size_t)v, ptr };
}

#if DEBUG
bool interpreter_stack_t::check_stack_frame(const frame_pos_t& in_frame) const{
	QUARK_ASSERT(in_frame._frame_ptr != nullptr && in_frame._frame_ptr->check_invariant());

	if(in_frame._frame_pos == 0){
		return true;
	}
	else{
		QUARK_ASSERT(in_frame._frame_ptr->_symbols.check_invariant());

		for(int i = 0 ; i < in_frame._frame_ptr->_symbols._symbols.size() ; i++){
			const auto& symbol = in_frame._frame_ptr->_symbols._symbols[i];

//			bool symbol_ext = encode_as_external(_types, symbol.second._value_type);
//			int local_pos = get_local_n_pos(in_frame._frame_pos, i);

//			bool stack_ext = debug_is_ext(local_pos);
//			QUARK_ASSERT(symbol_ext == stack_ext);
		}

		const auto prev_frame = read_frame_info(in_frame._frame_pos - k_frame_overhead);
		QUARK_ASSERT(prev_frame._frame_pos >= 0 && prev_frame._frame_pos < in_frame._frame_pos);
		if(prev_frame._frame_pos > k_frame_overhead){
			return check_stack_frame(prev_frame);
		}
		else{
			return true;
		}
	}
}
#endif

//	Returned elements are sorted with smaller stack positions first.
//	Walks the stack from the active frame towards the start of the stack, the globals frame.
std::vector<interpreter_stack_t::active_frame_t> interpreter_stack_t::get_stack_frames() const{
	QUARK_ASSERT(check_invariant());

	const auto stack_size = static_cast<int>(size());

	std::vector<active_frame_t> result;
	const auto frame_start0 = get_current_frame_start();
	{
		const auto frame_size0 = _current_static_frame->_symbols._symbols.size();
		const auto frame_end0 = frame_start0 + frame_size0;

		//??? Notice: during a call sequence: push_frame_ptr, push arguments, call, popn, pop_frame_ptr -- the stack size vs frames is slighly out of sync and gives us negative temporary stack entries.
		const auto temp_count0 = stack_size >= frame_end0 ? stack_size - frame_end0 : 0;
		const auto e = active_frame_t { frame_start0, frame_end0, _current_static_frame, temp_count0 };
		result.push_back(e);
	}

	auto info_pos = frame_start0 - k_frame_overhead;
	auto frame = read_frame_info(info_pos);
	while(frame._frame_ptr != nullptr){
		const auto frame_start = frame._frame_pos;
		const auto frame_size = frame._frame_ptr->_symbols._symbols.size();
		const auto frame_end = frame_start + frame_size;
		const auto temp_count = info_pos >= frame_end ? info_pos - frame_end : 0;

		const auto e = active_frame_t { frame_start, frame_end, frame._frame_ptr, temp_count };
		result.push_back(e);

		info_pos = frame_start - k_frame_overhead;
		frame = read_frame_info(info_pos);
	}

	std::reverse(result.begin(), result.end());
	return result;
}

json_t stack_to_json(const interpreter_stack_t& stack, value_backend_t& backend){
	QUARK_ASSERT(backend.check_invariant());

	const int size = static_cast<int>(stack._stack_size);

	const auto stack_frames = stack.get_stack_frames();

	std::vector<json_t> frames;
	for(int64_t i = 0 ; i < stack_frames.size() ; i++){
		auto a = json_t::make_array({
			json_t(i),
			json_t((int)stack_frames[i].start_pos),
			json_t((int)stack_frames[i].end_pos)
		});
		frames.push_back(a);
	}

	std::vector<json_t> elements;
	for(int64_t i = 0 ; i < size ; i++){
		const auto& frame_it = std::find_if(
			stack_frames.begin(),
			stack_frames.end(),
			[&i](const interpreter_stack_t::active_frame_t& e) { return e.start_pos == i; }
		);

		bool frame_start_flag = frame_it != stack_frames.end();

		if(frame_start_flag){
			elements.push_back(json_t("--- frame ---"));
		}

#if DEBUG
		const auto debug_type = stack._entry_types[i];
//		const auto ext = encode_as_external(_types, debug_type);
		const auto bc_pod = stack._entries[i];
		const auto bc = rt_value_t(backend, debug_type, bc_pod, rt_value_t::rc_mode::bump);

		auto a = json_t::make_array({
			json_t(i),
			type_to_json(backend.types, debug_type),
			/*???unwritten ? json_t("UNWRITTEN") :*/ bcvalue_to_json(backend, rt_value_t{ bc })
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


//??? support k_native_2
//??? use code from do_call() -- share code.
rt_value_t call_function_bc(interpreter_t& vm, const rt_value_t& f, const rt_value_t args[], int arg_count){
	const auto& backend = vm._backend;
	const auto& types = backend.types;

#if DEBUG
	QUARK_ASSERT(vm.check_invariant());
	QUARK_ASSERT(f.check_invariant());
	for(int i = 0 ; i < arg_count ; i++){ QUARK_ASSERT(args[i].check_invariant()); };
	QUARK_ASSERT(peek2(types, f._type).is_function());
#endif

	const auto func_link_ptr = lookup_func_link(backend, f._pod);
	if(func_link_ptr == nullptr){
		quark::throw_runtime_error("Attempting to calling unimplemented function.");
	}
	const auto& func_link = *func_link_ptr;

	if(func_link.machine == func_link_t::emachine::k_native){
		//	arity
//		QUARK_ASSERT(args.size() == host_function._function_type.get_function_args().size());
		QUARK_ASSERT(func_link.f != nullptr)
		const auto f2 = (BC_NATIVE_FUNCTION_PTR)func_link.f;
		const auto& result = (*f2)(vm, &args[0], arg_count);
		QUARK_ASSERT(result.check_invariant());
		return result;
	}
	else{
#if DEBUG
		const auto& arg_types = peek2(types, f._type).get_function_args(types);

		//	arity
		QUARK_ASSERT(arg_count == arg_types.size());

		for(int i = 0 ; i < arg_count; i++){
			if(peek2(types, args[i]._type) != peek2(types, arg_types[i])){
				QUARK_ASSERT(false);
			}
		}
#endif

		vm._stack.save_frame();

		//	We push the values to the stack = the stack will take RC ownership of the values.
		std::vector<type_t> arg_types2;
		for(int i = 0 ; i < arg_count ; i++){
			const auto& bc = args[i];
			const auto is_ext = args[i]._type;
			arg_types2.push_back(is_ext);
			vm._stack.push_external_value(bc);
		}

		QUARK_ASSERT(func_link.f != nullptr);
		auto frame_ptr = (const bc_static_frame_t*)func_link.f;
		vm._stack.open_frame_except_args(*frame_ptr, arg_count);
		const auto& result = execute_instructions(vm, frame_ptr->_instructions);
		vm._stack.close_frame(*frame_ptr);
		vm._stack.pop_batch(arg_types2);
		vm._stack.restore_frame();

		if(peek2(types, lookup_type_from_index(types, result.first)).is_void() == false){
			QUARK_ASSERT(result.second.check_invariant());
			return result.second;
		}
		else{
			return rt_value_t::make_undefined();
		}
	}
}


//??? Use enum with register / immediate / unused, current info is not enough.

reg_flags_t encoding_to_reg_flags(opcode_info_t::encoding e){
	if(e == opcode_info_t::encoding::k_e_0000){
		return { false, false, false };
	}
	else if(e == opcode_info_t::encoding::k_k_0ri0){
		return { true, false, false };
	}
	else if(e == opcode_info_t::encoding::k_l_00i0){
		return { false, false, false };
	}
	else if(e == opcode_info_t::encoding::k_n_0ii0){
		return { false, false, false };
	}
	else if(e == opcode_info_t::encoding::k_o_0rrr){
		return { true, true, true };
	}
	else if(e == opcode_info_t::encoding::k_p_0r00){
		return { true, false, false };
	}
	else if(e == opcode_info_t::encoding::k_q_0rr0){
		return { true, true, false };
	}
	else if(e == opcode_info_t::encoding::k_r_0ir0){
		return { false, true, false };
	}
	else if(e == opcode_info_t::encoding::k_s_0rri){
		return { true, true, false };
	}
	else if(e == opcode_info_t::encoding::k_t_0rii){
		return { true, false, false };
	}

	else{
		QUARK_ASSERT(false);
		quark::throw_exception();
	}
}


//////////////////////////////////////////		interpreter_t



std::vector<std::pair<type_t, struct_layout_t>> bc_make_struct_layouts(const types_t& types){
	QUARK_ASSERT(types.check_invariant());

	std::vector<std::pair<type_t, struct_layout_t>> result;

	for(int i = 0 ; i < types.nodes.size() ; i++){
		const auto& type = lookup_type_from_index(types, i);
		const auto peek_type = peek2(types, type);
		if(peek_type.is_struct() && is_fully_defined(types, peek_type)){
			const auto& source_struct_def = peek_type.get_struct(types);

			const auto struct_bytes = source_struct_def._members.size() * 8;
			std::vector<member_info_t> member_infos;
			for(int member_index = 0 ; member_index < source_struct_def._members.size() ; member_index++){
				const auto& member = source_struct_def._members[member_index];
				const size_t offset = member_index * 8;
				member_infos.push_back(member_info_t { offset, member._type } );
			}

			result.push_back( { type, struct_layout_t{ member_infos, struct_bytes } } );
		}
	}
	return result;
}


	static std::map<std::string, void*> get_corelib_and_network_binds(){
		const std::map<std::string, void*> corelib_binds = get_unified_corelib_binds();
		const auto network_binds = get_network_component_binds();

		std::map<std::string, void*> merge = corelib_binds;
		merge.insert(network_binds.begin(), network_binds.end());

		return merge;
	}

static std::vector<func_link_t> link_functions(const bc_program_t& program){
	QUARK_ASSERT(program.check_invariant());

	auto temp_types = program._types;
	const auto intrinsics2 = bc_get_intrinsics(temp_types);
	const auto corelib_native_funcs = get_corelib_and_network_binds();

	const auto funcs = mapf<func_link_t>(program._function_defs, [&corelib_native_funcs](const auto& e){
		const auto& function_name_symbol = e.func_link.module_symbol;
		const auto it = corelib_native_funcs.find(function_name_symbol.s);

		//	There us a native implementation of this function:
		if(it != corelib_native_funcs.end()){
			return func_link_t {
				e.func_link.module,
				function_name_symbol,
				e.func_link.function_type_optional,
				func_link_t::emachine::k_native2,
				(void*)it->second,
				{},
				nullptr
			};
		}

		//	There is a BC implementation.
		else if(e._frame_ptr != nullptr){
			return func_link_t {
				e.func_link.module,
				function_name_symbol,
				e.func_link.function_type_optional,
				e.func_link.machine,
				(void*)e._frame_ptr.get(),
				{},
				nullptr
			};
		}

		//	No implementation.
		else{
			return func_link_t { "", k_no_module_symbol, {}, func_link_t::emachine::k_bytecode, nullptr, {}, nullptr };
		}
	});

	//	Remove functions that don't have an implementation.
	const auto funcs2 = filterf<func_link_t>(funcs, [](const auto& e){ return e.module_symbol.s.empty() == false; });

	const auto func_lookup = concat(funcs2, intrinsics2);
	return func_lookup;
}





void interpreter_t::runtime_basics__on_print(const std::string& s){
	_runtime_handler->on_print(s);
}

type_t interpreter_t::runtime_basics__get_global_symbol_type(const std::string& s){
	const auto& symbols = _imm->_program._globals._symbols;

	const auto it = std::find_if(symbols._symbols.begin(), symbols._symbols.end(), [&s](const auto& e){ return e.first == s; });
	if(it == symbols._symbols.end()){
		QUARK_ASSERT(false);
		throw std::exception();
	}

	return it->second._value_type;
}





interpreter_t::interpreter_t(const bc_program_t& program, const config_t& config, runtime_process_i* process_handler, runtime_handler_i& runtime_handler) :
	_imm(std::make_shared<interpreter_imm_t>(interpreter_imm_t{ std::chrono::high_resolution_clock::now(), program })),
	_process_handler(process_handler),
	_runtime_handler(&runtime_handler),
	_backend{ link_functions(program), bc_make_struct_layouts(program._types), program._types, config },
	_stack { &_backend, &_imm->_program._globals }
{
	QUARK_ASSERT(program.check_invariant());
	QUARK_ASSERT(check_invariant());

	{
		_stack.save_frame();
		_stack.open_frame_except_args(_imm->_program._globals, 0);

		QUARK_ASSERT(check_invariant());
		if(false) trace_interpreter(*this, 0);

		//	Run static intialization (basically run global instructions before calling main()).
		execute_instructions(*this, _imm->_program._globals._instructions);

		if(false) trace_interpreter(*this, 0);

		//	close_frame() is called in destructor. This allows clients to read globals etc before interpreter is destructed.
//		_stack.close_frame(_imm->_program._globals);
	}
	QUARK_ASSERT(check_invariant());
}

interpreter_t::~interpreter_t(){
	QUARK_ASSERT(check_invariant());
}

void interpreter_t::unwind_stack(){
	QUARK_ASSERT(check_invariant());

	QUARK_ASSERT(_stack._current_static_frame == &_imm->_program._globals);
	_stack.close_frame(_imm->_program._globals);

	QUARK_ASSERT(check_invariant());
}

//??? Unify this debug code with trace_value_backend()
static std::vector<std::string> make(value_backend_t& backend, size_t i, runtime_value_t bc_pod, const type_t& debug_type, const interpreter_stack_t::active_frame_t& frame, int frame_index, int symbol_index, bool is_symbol){
	const bool is_rc = is_rc_value(backend.types, debug_type);

	std::string value_str = "";
	std::string rc_str = "";
	std::string alloc_id_str = "";

	std::string symbol_effective_type = type_to_compact_string(backend.types, frame.static_frame->_symbol_effective_type[symbol_index]);

	if(is_rc){
		//??? Related to k_init_local
		const bool illegal_rc_value = is_rc && (bc_pod.int_value == UNINITIALIZED_RUNTIME_VALUE);

		if(illegal_rc_value){
			value_str = "***RC VALUE: UNINITIALIZED***";
		}
		else{
			//	Hack for now, gets the alloc struct for any type of RC-value, but uses struct_ptr.
			const auto& alloc = bc_pod.struct_ptr->alloc;
			const auto alloc_id = alloc.alloc_id;
			const int32_t rc = alloc.rc;

			rc_str = std::to_string(rc);
			alloc_id_str = std::to_string(alloc_id);

			if(rc < 0){
				value_str = "***RC VALUE: NEGATIVE RC???***";
			}
			else if(rc == 0){
				value_str = "***RC VALUE: DELETED***";
			}
			else {
				const auto bc_that_owns_rc = rt_value_t(backend, debug_type, bc_pod, rt_value_t::rc_mode::bump);
				value_str = json_to_compact_string(bcvalue_to_json(backend, bc_that_owns_rc));
			}
		}
	}
	else{
		const auto bc_that_owns_rc = rt_value_t(backend, debug_type, bc_pod, rt_value_t::rc_mode::bump);
		value_str = json_to_compact_string(bcvalue_to_json(backend, bc_that_owns_rc));
	}

	const std::string frame_str = std::to_string(frame_index) + (frame_index == 0 ? " GLOBAL" : "");
	if(is_symbol){
		const auto& symbol = frame.static_frame->_symbols._symbols[symbol_index];
		const std::string symname = symbol.first;
		const std::string symbol_value_type = type_to_compact_string(backend.types, symbol.second._value_type);

		const auto line = std::vector<std::string> {
			std::to_string(i) + ":" + std::to_string(symbol_index),
			type_to_compact_string(backend.types, debug_type),
			value_str,
			rc_str,
			alloc_id_str,

			frame_str,
			symname,
			symbol_value_type,
			symbol_effective_type
		};
		return line;
	}
	else{
		const std::string symname = std::string() + "stack-temp" + std::to_string(symbol_index);
		const std::string symbol_value_type = "";

		const auto line = std::vector<std::string> {
			std::to_string(i) + ":" + std::to_string(symbol_index),
			type_to_compact_string(backend.types, debug_type),
			value_str,
			rc_str,
			alloc_id_str,

			frame_str,
			symname,
			symbol_value_type,
			symbol_effective_type
		};
		return line;
	}
}

void trace_interpreter(interpreter_t& vm, int pc){
	QUARK_SCOPED_TRACE("INTERPRETER STATE");

	auto& backend = vm._backend;
	const auto& stack = vm._stack;

	{
		QUARK_SCOPED_TRACE("STACK");

		const auto stack_frames = stack.get_stack_frames();

		std::vector<std::vector<std::string>> matrix;

		size_t stack_pos = 0;
		for(int frame_index = 0 ; frame_index < stack_frames.size() ; frame_index++){
			const auto line0 = std::vector<std::string> {
				std::to_string(stack_pos),
				"",
				std::to_string(stack._entries[stack_pos].int_value),
				"",
				"",

				"",
				"prev frame pos",
				"",
				""
			};
			matrix.push_back(line0);
			stack_pos++;

			const auto line1 = std::vector<std::string> {
				std::to_string(stack_pos),
				"",
				ptr_to_hexstring(stack._entries[stack_pos].frame_ptr),
				"",
				"",

				"",
				"prev frame ptr",
				"",
				""
			};
			matrix.push_back(line1);
			stack_pos++;

			const auto& frame = stack_frames[frame_index];

			for(int symbol_index = 0 ; symbol_index < (frame.end_pos - frame.start_pos) ; symbol_index++){
				const auto bc_pod = stack._entries[stack_pos];
				const auto debug_type = stack._entry_types[stack_pos];
				const auto line = make(backend, stack_pos, bc_pod, debug_type, frame, frame_index, symbol_index, true);
				matrix.push_back(line);
				stack_pos++;
			}
			for(int temp_index = 0 ; temp_index < frame.temp_count ; temp_index++){
				const auto bc_pod = stack._entries[stack_pos];
				const auto debug_type = stack._entry_types[stack_pos];
				const auto line = make(backend, stack_pos, bc_pod, debug_type, frame, frame_index, temp_index, false);
				matrix.push_back(line);
				stack_pos++;
			}
		}

		QUARK_SCOPED_TRACE("STACK ENTRIES");
		const auto result = generate_table_type1({ "#", "debug type", "value", "RC", "alloc ID", "frame", "symname", "sym.value type", "sym.effective type" }, matrix);
		QUARK_TRACE(result);
	}

	trace_value_backend_dynamic(backend);

	const auto& frame = *vm._stack._current_static_frame;
	{
		std::vector<json_t> instructions;
		int pos = 0;
		for(const auto& e: frame._instructions){
			const auto cursor = pos == pc ? "===>" : "    ";

			const auto i = json_t::make_array({
				cursor,
				pos,
				opcode_to_string(e._opcode),
				json_t(e._a),
				json_t(e._b),
				json_t(e._c)
			});
			instructions.push_back(i);
			pos++;
		}

/*
		const auto t = json_t::make_object({
			{ "symbols", json_t::make_array(bc_symbols_to_json(backend, frame._symbols)) },
			{ "instructions", json_t::make_array(instructions) }
		});
*/
		QUARK_SCOPED_TRACE("INSTRUCTIONS");
		QUARK_TRACE(json_to_pretty_string(json_t::make_array(instructions)));
	}
}

void interpreter_t::swap(interpreter_t& other) throw(){
	other._imm.swap(this->_imm);
	std::swap(other._process_handler, this->_process_handler);
	std::swap(other._runtime_handler, this->_runtime_handler);
	other._stack.swap(this->_stack);
}

#if DEBUG
bool interpreter_t::check_invariant() const {
	QUARK_ASSERT(_imm->_program.check_invariant());
	QUARK_ASSERT(_stack.check_invariant());
//	QUARK_ASSERT(_process_handler != nullptr);
	QUARK_ASSERT(_runtime_handler != nullptr);
	return true;
}
#endif



//////////////////////////////////////////		INSTRUCTIONS



//	IMPORTANT: NO arguments are passed as DYN arguments.
static void execute_new_1(interpreter_t& vm, int16_t dest_reg, int16_t target_itype, int16_t source_itype){
	QUARK_ASSERT(vm.check_invariant());

	auto& backend = vm._backend;
	const auto& types = backend.types;

	const auto& target_type = lookup_full_type(vm, target_itype);
	const auto target_peek = peek2(types, target_type);
	QUARK_ASSERT(target_peek.is_vector() == false && target_peek.is_dict() == false && target_peek.is_struct() == false);

	const int arg0_stack_pos = vm._stack.size() - 1;
	const auto source_itype2 = lookup_type_from_index(types, source_itype);
	const auto input_value = vm._stack.load_value(arg0_stack_pos + 0, source_itype2);

	const rt_value_t result = [&]{
		if(target_peek.is_bool() || target_peek.is_int() || target_peek.is_double() || target_peek.is_typeid()){
			return input_value;
		}

		//	Automatically transform a json::string => string at runtime?
		else if(target_peek.is_string() && peek2(types, source_itype2).is_json()){
			if(input_value.get_json().is_string()){
				return rt_value_t::make_string(backend, input_value.get_json().get_string());
			}
			else{
				quark::throw_runtime_error("Attempting to assign a non-string JSON to a string.");
			}
		}
		else if(target_peek.is_json()){
			const auto arg = bcvalue_to_json(backend, input_value);
			return rt_value_t::make_json(backend, arg);
		}
		else{
			return input_value;
		}
	}();

	vm._stack.write_register(dest_reg, result);
}

//	IMPORTANT: No arguments are passed as DYN arguments.
static void execute_new_vector_obj(interpreter_t& vm, int16_t dest_reg, int16_t target_itype, int16_t arg_count){
	QUARK_ASSERT(vm.check_invariant());

	auto& backend = vm._backend;
	const auto& types = backend.types;
	if(false) trace_types(types);

	const auto& target_type = lookup_full_type(vm, target_itype);
	const auto target_peek = peek2(types, target_type);
	QUARK_ASSERT(target_peek.is_vector());

	const auto& element_type = target_peek.get_vector_element_type(types);
	QUARK_ASSERT(element_type.is_undefined() == false);
	QUARK_ASSERT(target_type.is_undefined() == false);

	const int arg0_stack_pos = vm._stack.size() - arg_count;

	immer::vector<rt_value_t> elements2;
	for(int i = 0 ; i < arg_count ; i++){
		const auto pos = arg0_stack_pos + i;
		QUARK_ASSERT(peek2(types, vm._stack._entry_types[pos]) == type_t(peek2(types, element_type)));
		const auto& e = vm._stack._entries[pos];
		const auto e2 = rt_value_t(backend, element_type, e, rt_value_t::rc_mode::bump);
		elements2 = elements2.push_back(e2);
	}

	const auto result = make_vector_value(backend, element_type, elements2);
	vm._stack.write_register(dest_reg, result);
}

static void execute_new_dict_obj(interpreter_t& vm, int16_t dest_reg, int16_t target_itype, int16_t arg_count){
	QUARK_ASSERT(vm.check_invariant());

	auto& backend = vm._backend;
	const auto& types = backend.types;

	const auto& target_type = lookup_full_type(vm, target_itype);
	const auto target_peek = peek2(types, target_type);
	QUARK_ASSERT(target_peek.is_dict());

	const int arg0_stack_pos = vm._stack.size() - arg_count;

	const auto& element_type = target_peek.get_dict_value_type(types);
	QUARK_ASSERT(target_type.is_undefined() == false);
	QUARK_ASSERT(element_type.is_undefined() == false);

	const auto string_type = type_t::make_string();

	immer::map<std::string, rt_value_t> elements2;
	int dict_element_count = arg_count / 2;
	for(auto i = 0 ; i < dict_element_count ; i++){
		const auto key = vm._stack.load_value(arg0_stack_pos + i * 2 + 0, string_type);
		const auto value = vm._stack.load_value(arg0_stack_pos + i * 2 + 1, element_type);
		const auto key2 = key.get_string_value(backend);
		elements2 = elements2.insert({ key2, value });
	}

	const auto result = make_dict_value(backend, element_type, elements2);
	vm._stack.write_register(dest_reg, result);
}

static void execute_new_dict_pod64(interpreter_t& vm, int16_t dest_reg, int16_t target_itype, int16_t arg_count){
	QUARK_ASSERT(vm.check_invariant());

	execute_new_dict_obj(vm, dest_reg, target_itype, arg_count);
/*
	const auto& types = vm._imm->_program._types;
	const auto& target_type = lookup_full_type(vm, target_itype);
	const auto target_peek = peek2(types, target_type);
	QUARK_ASSERT(target_peek.is_dict());

	const int arg0_stack_pos = vm._stack.size() - arg_count;

	const auto& element_type = target_peek.get_dict_value_type(types);
	QUARK_ASSERT(target_type.is_undefined() == false);
	QUARK_ASSERT(element_type.is_undefined() == false);

	const auto string_type = type_t::make_string();

	immer::map<std::string, bc_inplace_value_t> elements2;
	int dict_element_count = arg_count / 2;
	for(auto i = 0 ; i < dict_element_count ; i++){
		const auto key = vm._stack.load_value(arg0_stack_pos + i * 2 + 0, string_type);
		const auto value = vm._stack.load_value(arg0_stack_pos + i * 2 + 1, element_type);
		const auto key2 = key.get_string_value();
		elements2 = elements2.insert({ key2, value._pod._inplace });
	}

	const auto result = make_dict(types, element_type, elements2);
	vm._stack.write_register(dest_reg, result);
*/
}

static void execute_new_struct(interpreter_t& vm, int16_t dest_reg, int16_t target_itype, int16_t arg_count){
	QUARK_ASSERT(vm.check_invariant());

	auto& backend = vm._backend;
	const auto& types = backend.types;

	const auto& target_type = lookup_full_type(vm, target_itype);
	const auto& target_peek = peek2(types, target_type);
	QUARK_ASSERT(target_peek.is_struct());

	const int arg0_stack_pos = vm._stack.size() - arg_count;

	const auto& struct_def = target_peek.get_struct(types);
	std::vector<rt_value_t> elements2;
	for(int i = 0 ; i < arg_count ; i++){
		const auto member_type0 = struct_def._members[i]._type;
		const auto member_type = peek2(types, member_type0);
		const auto value = vm._stack.load_value(arg0_stack_pos + i, member_type);
		elements2.push_back(value);
	}

	const auto result = rt_value_t::make_struct_value(backend, target_type, elements2);
	QUARK_ASSERT(result.check_invariant());

	vm._stack.write_register(dest_reg, result);
}

//	??? Make stub bc_static_frame_t for each host function to make call conventions same as Floyd functions.

//	Notice: host calls and floyd calls have the same type -- we cannot detect host calls until we have a callee value.
static void call_native(interpreter_t& vm, int target_reg, const func_link_t& func_link, int callee_arg_count){
	QUARK_ASSERT(vm.check_invariant());

	const auto& backend = vm._backend;
	const auto& types = backend.types;

	interpreter_stack_t& stack = vm._stack;

	QUARK_ASSERT(func_link.f != nullptr);
	const auto f = (BC_NATIVE_FUNCTION_PTR)func_link.f;

	const auto function_type_peek = peek2(types, func_link.function_type_optional);

	const auto function_type_args = function_type_peek.get_function_args(types);
	const auto function_def_dynamic_arg_count = count_dyn_args(types, func_link.function_type_optional);

	const int arg0_stack_pos = (int)(stack.size() - (function_def_dynamic_arg_count + callee_arg_count));
	int stack_pos = arg0_stack_pos;

	//	Notice that dynamic functions will have each DYN argument with a leading itype as an extra argument.
	const auto function_type_args_size = function_type_args.size();
	std::vector<rt_value_t> arg_values;
	for(int a = 0 ; a < function_type_args_size ; a++){
		const auto func_arg_type = function_type_args[a];
		if(peek2(types, func_arg_type).is_any()){
			const auto arg_itype = stack.load_intq(stack_pos);
			const auto& arg_type = lookup_full_type(vm, static_cast<int16_t>(arg_itype));
			const auto arg_value = stack.load_value(stack_pos + 1, arg_type);
			arg_values.push_back(arg_value);
			stack_pos += 2;
		}
		else{
			const auto arg_value = stack.load_value(stack_pos + 0, func_arg_type);
			arg_values.push_back(arg_value);
			stack_pos++;
		}
	}

	const auto& result = (*f)(vm, &arg_values[0], static_cast<int>(arg_values.size()));
	const auto bc_result = result;

	const auto& function_return_type = function_type_peek.get_function_return(types);
	const auto function_return_type_peek = peek2(types, function_return_type);
	if(function_return_type_peek.is_void() == true){
	}
	else{
		stack.write_register(target_reg, bc_result);
	}
}

typedef void(*VOID_VOID_F)(void);

struct cif_t {
	ffi_cif cif;
	std::vector<ffi_type*> arg_types;

	ffi_type* return_type;

	//	Those type we need to malloc are owned by this vector so we can delete them.
	std::vector<ffi_type*> args_types_owning;
};

/*
 #define UNIX64_RET_VOID		0
 #define UNIX64_RET_UINT8	1
 #define UNIX64_RET_UINT16	2
 #define UNIX64_RET_UINT32	3
 #define UNIX64_RET_SINT8	4
 #define UNIX64_RET_SINT16	5
 #define UNIX64_RET_SINT32	6
 #define UNIX64_RET_INT64	7
 #define UNIX64_RET_XMM32	8
 #define UNIX64_RET_XMM64	9
 #define UNIX64_RET_X87		10
 #define UNIX64_RET_X87_2	11
 #define UNIX64_RET_ST_XMM0_RAX	12
 #define UNIX64_RET_ST_RAX_XMM0	13
 #define UNIX64_RET_ST_XMM0_XMM1	14
 #define UNIX64_RET_ST_RAX_RDX	15

 #define UNIX64_RET_LAST		15

 #define UNIX64_FLAG_RET_IN_MEM	(1 << 10)
 #define UNIX64_FLAG_XMM_ARGS	(1 << 11)
 #define UNIX64_SIZE_SHIFT	12
*/

//	??? To add support for complex types, we need to wrap ffi_type in a struct that also owns sub-types.
static ffi_type* make_ffi_type(const type_t& type){
	if(type.is_void()){
		return &ffi_type_void;
	}
	else if(type.is_bool()){
		return &ffi_type_uint8;
	}
	else if(type.is_int()){
		return &ffi_type_sint64;
	}
	else if(type.is_double()){
		return &ffi_type_double;
	}
	else if(type.is_string()){
		return &ffi_type_pointer;
	}
	else if(type.is_json()){
		return &ffi_type_pointer;
	}
	else if(type.is_typeid()){
		return &ffi_type_sint64;
	}
	else if(type.is_struct()){
		return &ffi_type_pointer;
	}
	else if(type.is_vector()){
		return &ffi_type_pointer;
	}
	else if(type.is_dict()){
		return &ffi_type_pointer;
	}
	else if(type.is_function()){
		return &ffi_type_pointer;
	}
	else{
		UNSUPPORTED();
	}
}

static cif_t make_cif(const value_backend_t& backend, const type_t& function_type){
	QUARK_ASSERT(backend.check_invariant());
	const auto& types = backend.types;

	const auto function_type_peek = peek2(types, function_type);
	const auto function_type_args = function_type_peek.get_function_args(types);
	const auto return_type = function_type_peek.get_function_return(types);

	cif_t result;

	//	This is the runtime pointer, passed as first argument to all functions.
	result.arg_types.push_back(&ffi_type_pointer);

	const auto function_type_args_size = function_type_args.size();
	for(int arg_index = 0 ; arg_index < function_type_args_size ; arg_index++){
		const auto func_arg_type = function_type_args[arg_index];

		//	Notice that ANY will represent this function type arg as TWO elements in BC stack and in FFI argument list
		if(func_arg_type.is_any()){
			//	runtime_value_t
			result.arg_types.push_back(&ffi_type_uint64);

			//	runtime_type_t
			result.arg_types.push_back(&ffi_type_uint64);
		}
		else{
			const auto f = make_ffi_type(peek2(types, func_arg_type));
			result.arg_types.push_back(f);
		}
	}

	result.return_type = make_ffi_type(peek2(types, return_type));

	if (ffi_prep_cif(&result.cif, FFI_DEFAULT_ABI, (int)result.arg_types.size(), result.return_type, &result.arg_types[0]) != FFI_OK) {
		QUARK_ASSERT(false);
		throw std::exception();
	}
	return result;
}

static void call_via_libffi(interpreter_t& vm, int target_reg, const func_link_t& func_link, int callee_arg_count){
	QUARK_ASSERT(vm.check_invariant());

	auto& backend = vm._backend;
	const auto& types = backend.types;

	auto cif = make_cif(backend, func_link.function_type_optional);

	interpreter_stack_t& stack = vm._stack;


	QUARK_ASSERT(func_link.f != nullptr);

	const auto function_type_peek = peek2(types, func_link.function_type_optional);

	const auto function_type_args = function_type_peek.get_function_args(types);
	const auto function_def_dynamic_arg_count = count_dyn_args(types, func_link.function_type_optional);

	const int arg0_stack_pos = (int)(stack.size() - (function_def_dynamic_arg_count + callee_arg_count));
	int stack_pos = arg0_stack_pos;

	const auto runtime_ptr = floyd_runtime_t { &backend, &vm, vm._process_handler };

	std::vector<void*> values;

	//	This is the runtime pointer, passed as first argument to all functions.
	auto runtime_ptr_ptr = &runtime_ptr;
	values.push_back((void*)&runtime_ptr_ptr);

	const auto function_type_args_size = function_type_args.size();

	std::vector<rt_value_t> storage;
	storage.reserve(function_type_args_size + function_def_dynamic_arg_count);

	for(int arg_index = 0 ; arg_index < function_type_args_size ; arg_index++){
		const auto func_arg_type = function_type_args[arg_index];

		//	Notice that ANY will represent this function type arg as TWO elements in BC stack and in FFI argument list
		if(func_arg_type.is_any()){
			const auto arg_itype = stack._entries[stack_pos + 0].int_value;
			const auto arg_pod = stack._entries[stack_pos + 1];

			const auto arg_type = lookup_full_type(vm, static_cast<int16_t>(arg_itype));

			storage.push_back(rt_value_t::make_int(arg_type.get_data()));

			//	Store the value as a 64 bit integer always.
			storage.push_back(rt_value_t::make_int(arg_pod.int_value));

			//	Notice: in ABI, any is passed like: [ runtime_value_t value, runtime_type_t value_type ], notice swapped order!
			//	Use a pointer directly into storage. This is tricky. Requires reserve().
			values.push_back(&storage[storage.size() - 1]._pod);
			values.push_back(&storage[storage.size() - 2]._pod);

			stack_pos++;
			stack_pos++;
		}
		else{
			const auto arg_value = stack.load_value(stack_pos + 0, func_arg_type);
			storage.push_back(arg_value);

			//	Use a pointer directly into storage. This is tricky. Requires reserve().
			values.push_back(&storage[storage.size() - 1]._pod);
			stack_pos++;
		}
	}

	ffi_arg return_value;

	ffi_call(&cif.cif, (VOID_VOID_F)func_link.f, &return_value, &values[0]);

	const auto& function_return_type = function_type_peek.get_function_return(types);
	const auto function_return_type_peek = peek2(types, function_return_type);
	if(function_return_type_peek.is_void() == true){
	}
	else{
		const auto result2 = *(runtime_value_t*)&return_value;
		const auto result3 = rt_value_t(backend, function_return_type_peek, result2, rt_value_t::rc_mode::adopt);
		stack.write_register(target_reg, result3);
	}
}

//	We need to examine the callee, since we support magic argument lists of varying size.
static void do_call(interpreter_t& vm, int target_reg, const runtime_value_t callee, int callee_arg_count){
	QUARK_ASSERT(vm.check_invariant());

	interpreter_stack_t& stack = vm._stack;

	const auto& backend = vm._backend;
	const auto& types = backend.types;

	const auto func_link_ptr = lookup_func_link(backend, callee);
	if(func_link_ptr == nullptr){
		quark::throw_runtime_error("Attempting to calling unimplemented function.");
	}

	const auto& func_link = *func_link_ptr;

	if(func_link.f == nullptr){
		quark::throw_runtime_error("Attempting to calling unimplemented function.");
	}

	if(func_link.machine == func_link_t::emachine::k_bytecode){
		//	This is a floyd function, with a frame_ptr to execute.
		QUARK_ASSERT(func_link.f != nullptr);
		QUARK_ASSERT(func_link.function_type_optional.get_function_args(types).size() == callee_arg_count);

		const auto& function_return_type = peek2(types, func_link.function_type_optional).get_function_return(types);

		QUARK_ASSERT(count_dyn_args(types, func_link.function_type_optional) == 0);

		//	We need to remember the global pos where to store return value, since we're switching frame to call function.
		int result_reg_pos = static_cast<int>(stack._current_frame_start_ptr - &stack._entries[0]) + target_reg;

		auto frame_ptr = (const bc_static_frame_t*)func_link.f;

		stack.open_frame_except_args(*frame_ptr, callee_arg_count);
		const auto& result = execute_instructions(vm, frame_ptr->_instructions);
		QUARK_ASSERT(result.second.check_invariant());
		stack.close_frame(*frame_ptr);

		const auto function_return_type_peek = peek2(types, function_return_type);
		if(function_return_type_peek.is_void() == false){
			//	Cannot store via register, we store on absolute stacok pos (relative to start of
			//	stack. We have not yet executed k_pop_frame_ptr that restores our frame.
			stack.replace_external_value(result_reg_pos, result.second);
		}
	}
	else if(func_link.machine == func_link_t::emachine::k_native){
		call_native(vm, target_reg, func_link, callee_arg_count);
	}
	else if(func_link.machine == func_link_t::emachine::k_native2){
		call_via_libffi(vm, target_reg, func_link, callee_arg_count);
	}
	else{
		QUARK_ASSERT(false);
		throw std::exception();
	}
}

inline void write_reg_rc(value_backend_t& backend, runtime_value_t regs[], int dest_reg, const type_t& dest_type, runtime_value_t value){
	QUARK_ASSERT(backend.check_invariant());
	QUARK_ASSERT(dest_type.check_invariant());

	release_value_safe(backend, regs[dest_reg], dest_type);
	retain_value(backend, value, dest_type);
	regs[dest_reg] = value;
}

std::pair<bc_typeid_t, rt_value_t> execute_instructions(interpreter_t& vm, const std::vector<bc_instruction_t>& instructions){
	QUARK_ASSERT(vm.check_invariant());
	QUARK_ASSERT(instructions.empty() == true || (instructions.back()._opcode == bc_opcode::k_return || instructions.back()._opcode == bc_opcode::k_stop));

	auto& backend = vm._backend;
	const auto& types = backend.types;

	interpreter_stack_t& stack = vm._stack;
	const bc_static_frame_t* frame_ptr = stack._current_static_frame;
	auto* regs = stack._current_frame_start_ptr;
	auto* globals = &stack._entries[k_frame_overhead];

//	QUARK_TRACE_SS("STACK:  " << json_to_pretty_string(stack.stack_to_json()));

	int pc = 0;

	if(k_trace_stepping){
		trace_interpreter(vm, pc);
	}

	while(true){
		QUARK_ASSERT(pc >= 0);
		QUARK_ASSERT(pc < instructions.size());
		const auto i = instructions[pc];

		QUARK_ASSERT(vm.check_invariant());
		QUARK_ASSERT(i.check_invariant());

		QUARK_ASSERT(frame_ptr == stack._current_static_frame);
		QUARK_ASSERT(regs == stack._current_frame_start_ptr);


		const auto opcode = i._opcode;

		switch(opcode){

		case bc_opcode::k_nop:
			break;


		//////////////////////////////////////////		ACCESS GLOBALS


		case bc_opcode::k_load_global_external_value: {
			QUARK_ASSERT(stack.check_reg__external_value(i._a));
			QUARK_ASSERT(stack.check_global_access_obj(i._b));

			const auto& type = frame_ptr->_symbols._symbols[i._a].second._value_type;
			release_value_safe(backend, regs[i._a], type);
			const auto& new_value_pod = globals[i._b];
			regs[i._a] = new_value_pod;
			retain_value(backend, new_value_pod, type);
			break;
		}
		case bc_opcode::k_load_global_inplace_value: {
			QUARK_ASSERT(stack.check_reg__inplace_value(i._a));

			regs[i._a] = globals[i._b];
			break;
		}

		case bc_opcode::k_init_local: {
			const auto& type = frame_ptr->_symbols._symbols[i._a].second._value_type;
			const auto& new_value_pod = regs[i._b];
			regs[i._a] = new_value_pod;
			retain_value(backend, new_value_pod, type);
			break;
		}

		case bc_opcode::k_store_global_external_value: {
			QUARK_ASSERT(stack.check_global_access_obj(i._a));
			QUARK_ASSERT(stack.check_reg__external_value(i._b));

			const auto& type = frame_ptr->_symbols._symbols[i._b].second._value_type;
			release_value_safe(backend, globals[i._a], type);
			const auto& new_value_pod = regs[i._b];
			globals[i._a] = new_value_pod;
			retain_value(backend, new_value_pod, type);
			break;
		}
		case bc_opcode::k_store_global_inplace_value: {
			QUARK_ASSERT(stack.check_global_access_intern(i._a));
			QUARK_ASSERT(stack.check_reg__inplace_value(i._b));

			globals[i._a] = regs[i._b];
			break;
		}


		//////////////////////////////////////////		ACCESS LOCALS


		case bc_opcode::k_copy_reg_inplace_value: {
			QUARK_ASSERT(stack.check_reg__inplace_value(i._a));
			QUARK_ASSERT(stack.check_reg__inplace_value(i._b));

			regs[i._a] = regs[i._b];
			break;
		}
		case bc_opcode::k_copy_reg_external_value: {
			QUARK_ASSERT(stack.check_reg__external_value(i._a));
			QUARK_ASSERT(stack.check_reg__external_value(i._b));

			const auto& type = frame_ptr->_symbols._symbols[i._a].second._value_type;
			release_value_safe(backend, regs[i._a], type);
			const auto& new_value_pod = regs[i._b];
			regs[i._a] = new_value_pod;
			retain_value(backend, new_value_pod, type);
			break;
		}


		//////////////////////////////////////////		STACK


		case bc_opcode::k_return: {
			const auto& type = frame_ptr->_symbols._symbols[i._a].second._value_type;
			return { true, rt_value_t(backend, type, regs[i._a], rt_value_t::rc_mode::bump) };
		}

		case bc_opcode::k_stop: {
			return { false, rt_value_t::make_undefined() };
		}

		case bc_opcode::k_push_frame_ptr: {
			QUARK_ASSERT(vm.check_invariant());
			QUARK_ASSERT((stack._stack_size + k_frame_overhead) < stack._allocated_count)

			stack._entries[stack._stack_size + 0].int_value = static_cast<int64_t>(stack._current_frame_start_ptr - &stack._entries[0]);
			stack._entries[stack._stack_size + 1].frame_ptr = (void*)frame_ptr;
			stack._stack_size += k_frame_overhead;
			stack._entry_types.push_back(type_t::make_int());
			stack._entry_types.push_back(type_t::make_void());
			QUARK_ASSERT(vm.check_invariant());
			break;
		}

		case bc_opcode::k_pop_frame_ptr: {
			QUARK_ASSERT(vm.check_invariant());
			QUARK_ASSERT(stack._stack_size >= k_frame_overhead);

			const auto frame_pos = stack._entries[stack._stack_size - k_frame_overhead + 0].int_value;
			frame_ptr = (bc_static_frame_t*)stack._entries[stack._stack_size - k_frame_overhead + 1].frame_ptr;
			stack._stack_size -= k_frame_overhead;

			stack._entry_types.pop_back();
			stack._entry_types.pop_back();

			regs = &stack._entries[frame_pos];

			//??? do we need stack._current_static_frame, stack._current_frame_pos? Only use local variables to track these?
			stack._current_static_frame = frame_ptr;
			stack._current_frame_start_ptr = regs;

			QUARK_ASSERT(frame_ptr == stack._current_static_frame);
			QUARK_ASSERT(regs == stack._current_frame_start_ptr);
			QUARK_ASSERT(vm.check_invariant());
			break;
		}

		case bc_opcode::k_push_inplace_value: {
			QUARK_ASSERT(stack.check_reg__inplace_value(i._a));
			const auto type = stack._entry_types[stack.get_current_frame_start() + i._a];
			stack._entries[stack._stack_size] = regs[i._a];
			stack._stack_size++;
			stack._entry_types.push_back(type);
			QUARK_ASSERT(stack.check_invariant());
			break;
		}
		case bc_opcode::k_push_external_value: {
			QUARK_ASSERT(stack.check_reg__external_value(i._a));
			const auto& type = frame_ptr->_symbols._symbols[i._a].second._value_type;
			const auto& new_value_pod = regs[i._a];
			retain_value(backend, new_value_pod, type);
			stack._entries[stack._stack_size] = new_value_pod;
			stack._stack_size++;
			stack._entry_types.push_back(type);
			break;
		}

		//??? Idea: popn specifies *where* we can find a list with the types to pop.
		//??? popn with a = 0 is a NOP ==> never emit these!
		case bc_opcode::k_popn: {
			QUARK_ASSERT(vm.check_invariant());

			const uint32_t n = i._a;
			//const uint32_t extbits = i._b;

			QUARK_ASSERT(stack._stack_size >= n);
			QUARK_ASSERT(n >= 0);
			QUARK_ASSERT(n <= 32);

			int pos = static_cast<int>(stack._stack_size) - 1;
			for(int m = 0 ; m < n ; m++){
				const auto type = stack._entry_types.back();
				release_value_safe(backend, stack._entries[pos], type);
				pos--;
				stack._entry_types.pop_back();
			}
			stack._stack_size -= n;

			QUARK_ASSERT(vm.check_invariant());
			break;
		}


		//////////////////////////////////////////		BRANCHING


		case bc_opcode::k_branch_false_bool: {
			QUARK_ASSERT(stack.check_reg_bool(i._a));

			//	Notice that pc will be incremented too, hence the - 1.
			pc = regs[i._a].bool_value ? pc : pc + i._b - 1;
			break;
		}
		case bc_opcode::k_branch_true_bool: {
			QUARK_ASSERT(stack.check_reg_bool(i._a));

			//	Notice that pc will be incremented too, hence the - 1.
			pc = regs[i._a].bool_value ? pc + i._b - 1: pc;
			break;
		}
		case bc_opcode::k_branch_zero_int: {
			QUARK_ASSERT(stack.check_reg_int(i._a));

			//	Notice that pc will be incremented too, hence the - 1.
			pc = regs[i._a].int_value == 0 ? pc + i._b - 1 : pc;
			break;
		}
		case bc_opcode::k_branch_notzero_int: {
			QUARK_ASSERT(stack.check_reg_int(i._a));

			//	Notice that pc will be incremented too, hence the - 1.
			pc = regs[i._a].int_value == 0 ? pc : pc + i._b - 1;
			break;
		}
		case bc_opcode::k_branch_smaller_int: {
			QUARK_ASSERT(stack.check_reg_int(i._a));
			QUARK_ASSERT(stack.check_reg_int(i._b));

			//	Notice that pc will be incremented too, hence the - 1.
			pc = regs[i._a].int_value < regs[i._b].int_value ? pc + i._c - 1 : pc;
			break;
		}
		case bc_opcode::k_branch_smaller_or_equal_int: {
			QUARK_ASSERT(stack.check_reg_int(i._a));
			QUARK_ASSERT(stack.check_reg_int(i._b));

			//	Notice that pc will be incremented too, hence the - 1.
			pc = regs[i._a].int_value <= regs[i._b].int_value ? pc + i._c - 1 : pc;
			break;
		}
		case bc_opcode::k_branch_always: {
			//	Notice that pc will be incremented too, hence the - 1.
			pc = pc + i._a - 1;
			break;
		}


		//////////////////////////////////////////		COMPLEX


		//??? Make obj/intern version.
		case bc_opcode::k_get_struct_member: {
			QUARK_ASSERT(vm.check_invariant());
			QUARK_ASSERT(stack.check_reg_any(i._a));
			QUARK_ASSERT(stack.check_reg_struct(i._b));

			const auto& struct_type = frame_ptr->_symbols._symbols[i._b].second._value_type;
			const auto data_ptr = regs[i._b].struct_ptr->get_data_ptr();
			const auto member_index = i._c;
			const auto r = load_struct_member(backend, data_ptr, struct_type, member_index);
			const auto& member_type = r.second;

			release_value_safe(backend, regs[i._a], member_type);
			retain_value(backend, r.first, member_type);
			regs[i._a] = r.first;

			QUARK_ASSERT(vm.check_invariant());
			break;
		}

		case bc_opcode::k_lookup_element_string: {
			QUARK_ASSERT(vm.check_invariant());
			QUARK_ASSERT(stack.check_reg_int(i._a));
			QUARK_ASSERT(stack.check_reg_string(i._b));
			QUARK_ASSERT(stack.check_reg_int(i._c));

			//	??? faster to lookup via char* directly.
			const auto s = from_runtime_string2(backend, regs[i._b]);
			const auto lookup_index = regs[i._c].int_value;
			if(lookup_index < 0 || lookup_index >= s.size()){
				quark::throw_runtime_error("Lookup in string: out of bounds.");
			}
			else{
				const char ch = s[lookup_index];
				regs[i._a].int_value = ch;
			}
			QUARK_ASSERT(vm.check_invariant());
			break;
		}

		//	??? Simple JSON-values should not require ext. null, int, bool, empty object, empty array.
		case bc_opcode::k_lookup_element_json: {
			QUARK_ASSERT(vm.check_invariant());
			QUARK_ASSERT(stack.check_reg_json(i._a));
			QUARK_ASSERT(stack.check_reg_json(i._b));

			// reg c points to different types depending on the runtime-type of the json.
			QUARK_ASSERT(stack.check_reg_any(i._c));

			const auto& parent_json = regs[i._b].json_ptr->get_json();

			if(parent_json.is_object()){
				QUARK_ASSERT(stack.check_reg_string(i._c));

				const auto& lookup_key = from_runtime_string2(backend, regs[i._c]);

				//	get_object_element() throws if key can't be found.
				const auto& value = parent_json.get_object_element(lookup_key);

				//??? no need to create full rt_value_t here! We only need pod.
				const auto value2 = rt_value_t::make_json(backend, value);

				release_value_safe(backend, regs[i._a], value2._type);
				retain_value(backend, value2._pod, value2._type);
				regs[i._a] = value2._pod;
			}
			else if(parent_json.is_array()){
				QUARK_ASSERT(stack.check_reg_int(i._c));

				const auto lookup_index = regs[i._c].int_value;
				if(lookup_index < 0 || lookup_index >= parent_json.get_array_size()){
					quark::throw_runtime_error("Lookup in json array: out of bounds.");
				}
				else{
					const auto& value = parent_json.get_array_n(lookup_index);

					//??? value2 will soon go out of scope - avoid creating rt_value_t all together.
					//??? no need to create full rt_value_t here! We only need pod.
					const auto value2 = rt_value_t::make_json(backend, value);

					release_value_safe(backend, regs[i._a], value2._type);
					retain_value(backend, value2._pod, value2._type);
					regs[i._a] = value2._pod;
				}
			}
			else{
				quark::throw_runtime_error("Lookup using [] on json only works on objects and arrays.");
			}
			QUARK_ASSERT(vm.check_invariant());
			break;
		}

		case bc_opcode::k_lookup_element_vector_w_external_elements:
		case bc_opcode::k_lookup_element_vector_w_inplace_elements:
		{
			QUARK_ASSERT(vm.check_invariant());
			QUARK_ASSERT(stack.check_reg__external_value(i._a));
			QUARK_ASSERT(stack.check_reg_int(i._c));

			const auto& vector_type = frame_ptr->_symbols._symbols[i._b].second._value_type;
			const auto lookup_index = regs[i._c].int_value;
			const auto size = get_vector_size(backend, vector_type, regs[i._b]);
			if(lookup_index < 0 || lookup_index >= size){
				quark::throw_runtime_error("Lookup in vector: out of bounds.");
			}
			else{
				const auto element_type = peek2(types, vector_type).get_vector_element_type(types);
				const auto e = lookup_vector_element(backend, vector_type, regs[i._b], lookup_index);
				write_reg_rc(backend, regs, i._a, element_type, e);
			}
			QUARK_ASSERT(vm.check_invariant());
			break;
		}

		case bc_opcode::k_lookup_element_dict_w_external_values:
		case bc_opcode::k_lookup_element_dict_w_inplace_values:
		{
			QUARK_ASSERT(stack.check_reg__external_value(i._a));
			QUARK_ASSERT(stack.check_reg_dict_w_external_values(i._b));
			QUARK_ASSERT(stack.check_reg_string(i._c));

			const auto& dict_type = frame_ptr->_symbols._symbols[i._b].second._value_type;
			const auto key = regs[i._c];
//				quark::throw_runtime_error("Lookup in dict: key not found.");
			const auto element_type = peek2(types, dict_type).get_dict_value_type(types);
			const auto e = lookup_dict(backend, regs[i._b], dict_type, key);
			write_reg_rc(backend, regs, i._a, element_type, e);
			break;
		}

		case bc_opcode::k_get_size_vector_w_external_elements: {
			QUARK_ASSERT(vm.check_invariant());
			QUARK_ASSERT(stack.check_reg_int(i._a));
			QUARK_ASSERT(stack.check_reg_vector_w_external_elements(i._b));
			QUARK_ASSERT(i._c == 0);

			const auto& type_b = frame_ptr->_symbols._symbols[i._b].second._value_type;
			regs[i._a].int_value = get_vector_size(backend, type_b, regs[i._b]);
			QUARK_ASSERT(vm.check_invariant());
			break;
		}

		case bc_opcode::k_get_size_vector_w_inplace_elements: {
			QUARK_ASSERT(vm.check_invariant());
			QUARK_ASSERT(stack.check_reg_int(i._a));
			QUARK_ASSERT(stack.check_reg_vector_w_inplace_elements(i._b));
			QUARK_ASSERT(i._c == 0);

			const auto& type_b = frame_ptr->_symbols._symbols[i._b].second._value_type;
			regs[i._a].int_value = get_vector_size(backend, type_b, regs[i._b]);
			QUARK_ASSERT(vm.check_invariant());
			break;
		}

		case bc_opcode::k_get_size_dict_w_external_values: {
			QUARK_ASSERT(vm.check_invariant());
			QUARK_ASSERT(stack.check_reg_int(i._a));
			QUARK_ASSERT(stack.check_reg_dict_w_external_values(i._b));
			QUARK_ASSERT(i._c == 0);

			const auto& type_b = frame_ptr->_symbols._symbols[i._b].second._value_type;
			regs[i._a].int_value = get_dict_size(backend, type_b, regs[i._b]);
			QUARK_ASSERT(vm.check_invariant());
			break;
		}
		case bc_opcode::k_get_size_dict_w_inplace_values: {
			QUARK_ASSERT(vm.check_invariant());
			QUARK_ASSERT(stack.check_reg_int(i._a));
			QUARK_ASSERT(stack.check_reg_dict_w_inplace_values(i._b));
			QUARK_ASSERT(i._c == 0);

			const auto& type_b = frame_ptr->_symbols._symbols[i._b].second._value_type;
			regs[i._a].int_value = get_dict_size(backend, type_b, regs[i._b]);
			QUARK_ASSERT(vm.check_invariant());
			break;
		}

		case bc_opcode::k_get_size_string: {
			QUARK_ASSERT(vm.check_invariant());
			QUARK_ASSERT(stack.check_reg_int(i._a));
			QUARK_ASSERT(stack.check_reg_string(i._b));
			QUARK_ASSERT(i._c == 0);

			regs[i._a].int_value = get_vector_size(backend, type_t::make_string(), regs[i._b]);
			QUARK_ASSERT(vm.check_invariant());
			break;
		}
		case bc_opcode::k_get_size_jsonvalue: {
			QUARK_ASSERT(vm.check_invariant());
			QUARK_ASSERT(stack.check_reg_int(i._a));
			QUARK_ASSERT(stack.check_reg_json(i._b));
			QUARK_ASSERT(i._c == 0);

			const auto& json = regs[i._b].json_ptr->get_json();
			if(json.is_object()){
				regs[i._a].int_value = json.get_object_size();
			}
			else if(json.is_array()){
				regs[i._a].int_value = json.get_array_size();
			}
			else if(json.is_string()){
				regs[i._a].int_value = json.get_string().size();
			}
			else{
				quark::throw_runtime_error("Calling size() on unsupported type of value.");
			}
			QUARK_ASSERT(vm.check_invariant());
			break;
		}

		case bc_opcode::k_pushback_vector_w_external_elements: {
			QUARK_ASSERT(vm.check_invariant());
			QUARK_ASSERT(stack.check_reg_vector_w_external_elements(i._a));
			QUARK_ASSERT(stack.check_reg_vector_w_external_elements(i._b));
			QUARK_ASSERT(stack.check_reg__external_value(i._c));

			const auto type_b = frame_ptr->_symbols._symbols[i._b].second._value_type;
			write_reg_rc(backend, regs, i._a, type_b, push_back_vector_element(backend, regs[i._b], type_b, regs[i._c]));

			QUARK_ASSERT(vm.check_invariant());
			break;
		}

		case bc_opcode::k_pushback_vector_w_inplace_elements: {
			QUARK_ASSERT(vm.check_invariant());
			QUARK_ASSERT(stack.check_reg_vector_w_inplace_elements(i._a));
			QUARK_ASSERT(stack.check_reg_vector_w_inplace_elements(i._b));
			QUARK_ASSERT(stack.check_reg(i._c));

			const auto type_b = frame_ptr->_symbols._symbols[i._b].second._value_type;
			write_reg_rc(backend, regs, i._a, type_b, push_back_vector_element(backend, regs[i._b], type_b, regs[i._c]));

			QUARK_ASSERT(vm.check_invariant());
			break;
		}

		case bc_opcode::k_pushback_string: {
			QUARK_ASSERT(vm.check_invariant());
			QUARK_ASSERT(stack.check_reg_string(i._a));
			QUARK_ASSERT(stack.check_reg_string(i._b));
			QUARK_ASSERT(stack.check_reg_int(i._c));

			const auto type_b = frame_ptr->_symbols._symbols[i._b].second._value_type;
			write_reg_rc(backend, regs, i._a, type_b, push_back_vector_element(backend, regs[i._b], type_b, regs[i._c]));

			QUARK_ASSERT(vm.check_invariant());
			break;
		}


		case bc_opcode::k_call: {
			QUARK_ASSERT(vm.check_invariant());
			QUARK_ASSERT(stack.check_reg_function(i._b));

			do_call(vm, i._a, regs[i._b], i._c);

			frame_ptr = stack._current_static_frame;
			regs = stack._current_frame_start_ptr;

			QUARK_ASSERT(frame_ptr == stack._current_static_frame);
			QUARK_ASSERT(regs == stack._current_frame_start_ptr);
			QUARK_ASSERT(vm.check_invariant());
			break;
		}

		case bc_opcode::k_new_1: {
			QUARK_ASSERT(stack.check_reg(i._a));

			const auto dest_reg = i._a;
			const auto target_itype = i._b;
			const auto source_itype = i._c;
			const auto& target_type = lookup_full_type(vm, target_itype);
			QUARK_ASSERT(peek2(types, target_type).is_vector() == false && peek2(types, target_type).is_dict() == false && peek2(types, target_type).is_struct() == false);
			execute_new_1(vm, dest_reg, target_itype, source_itype);
			break;
		}

		case bc_opcode::k_new_vector_w_external_elements: {
			QUARK_ASSERT(stack.check_reg_vector_w_external_elements(i._a));
			QUARK_ASSERT(i._b >= 0);
			QUARK_ASSERT(i._c >= 0);

			const auto dest_reg = i._a;
			const auto target_itype = i._b;
			const auto arg_count = i._c;
			const auto& vector_type = lookup_full_type(vm, target_itype);
			QUARK_ASSERT(peek2(types, vector_type).is_vector());

			execute_new_vector_obj(vm, dest_reg, target_itype, arg_count);
			break;
		}

		case bc_opcode::k_new_vector_w_inplace_elements: {
			QUARK_ASSERT(vm.check_invariant());
			QUARK_ASSERT(stack.check_reg_vector_w_inplace_elements(i._a));
			QUARK_ASSERT(i._b == 0);
			QUARK_ASSERT(i._c >= 0);

			const auto dest_reg = i._a;
			const auto target_itype = i._b;
			const auto arg_count = i._c;
			execute_new_vector_obj(vm, dest_reg, target_itype, arg_count);

			QUARK_ASSERT(vm.check_invariant());
			break;
		}

		case bc_opcode::k_new_dict_w_external_values: {
			const auto dest_reg = i._a;
			const auto target_itype = i._b;
			const auto arg_count = i._c;
			const auto& target_type = lookup_full_type(vm, target_itype);
			const auto peek = peek2(types, target_type);
			QUARK_ASSERT(peek.is_dict());
			execute_new_dict_obj(vm, dest_reg, target_itype, arg_count);
			break;
		}
		case bc_opcode::k_new_dict_w_inplace_values: {
			const auto dest_reg = i._a;
			const auto target_itype = i._b;
			const auto arg_count = i._c;
			const auto& target_type = lookup_full_type(vm, target_itype);
			QUARK_ASSERT(peek2(types, target_type).is_dict());
			execute_new_dict_pod64(vm, dest_reg, target_itype, arg_count);
			break;
		}
		case bc_opcode::k_new_struct: {
			const auto dest_reg = i._a;
			const auto target_itype = i._b;
			const auto arg_count = i._c;
			const auto& target_type = lookup_full_type(vm, target_itype);
			QUARK_ASSERT(peek2(types, target_type).is_struct());
			execute_new_struct(vm, dest_reg, target_itype, arg_count);
			break;
		}


		//////////////////////////////		COMPARISON


		case bc_opcode::k_comparison_smaller_or_equal: {
			QUARK_ASSERT(stack.check_reg_bool(i._a));
			QUARK_ASSERT(stack.check_reg_any(i._b));
			QUARK_ASSERT(stack.check_reg_any(i._c));

			const auto& type = frame_ptr->_symbols._symbols[i._b].second._value_type;
			QUARK_ASSERT(peek2(types, type).is_int() == false);

			const auto left = stack.read_register(i._b);
			const auto right = stack.read_register(i._c);
			long diff = compare_value_true_deep(backend, left, right, type);

			regs[i._a].bool_value = diff <= 0;
			break;
		}
		case bc_opcode::k_comparison_smaller_or_equal_int: {
			QUARK_ASSERT(stack.check_reg_bool(i._a));
			QUARK_ASSERT(stack.check_reg_int(i._b));
			QUARK_ASSERT(stack.check_reg_int(i._c));

			regs[i._a].bool_value = regs[i._b].int_value <= regs[i._c].int_value;
			break;
		}

		case bc_opcode::k_comparison_smaller: {
			QUARK_ASSERT(stack.check_reg_bool(i._a));
			QUARK_ASSERT(stack.check_reg_any(i._b));
			QUARK_ASSERT(stack.check_reg_any(i._c));

			const auto& type = frame_ptr->_symbols._symbols[i._b].second._value_type;
			QUARK_ASSERT(peek2(types, type).is_int() == false);
			const auto left = stack.read_register(i._b);
			const auto right = stack.read_register(i._c);
			long diff = compare_value_true_deep(backend, left, right, type);

			regs[i._a].bool_value = diff < 0;
			break;
		}
		case bc_opcode::k_comparison_smaller_int:
			QUARK_ASSERT(stack.check_reg_bool(i._a));
			QUARK_ASSERT(stack.check_reg_int(i._b));
			QUARK_ASSERT(stack.check_reg_int(i._c));

			regs[i._a].bool_value = regs[i._b].int_value < regs[i._c].int_value;
			break;

		case bc_opcode::k_logical_equal: {
			QUARK_ASSERT(stack.check_reg_bool(i._a));
			QUARK_ASSERT(stack.check_reg_any(i._b));
			QUARK_ASSERT(stack.check_reg_any(i._c));

			const auto& type = frame_ptr->_symbols._symbols[i._b].second._value_type;
			QUARK_ASSERT(peek2(types, type).is_int() == false);
			const auto left = stack.read_register(i._b);
			const auto right = stack.read_register(i._c);
			long diff = compare_value_true_deep(backend, left, right, type);

			regs[i._a].bool_value = diff == 0;
			break;
		}
		case bc_opcode::k_logical_equal_int: {
			QUARK_ASSERT(stack.check_reg_bool(i._a));
			QUARK_ASSERT(stack.check_reg_int(i._b));
			QUARK_ASSERT(stack.check_reg_int(i._c));

			regs[i._a].bool_value = regs[i._b].int_value == regs[i._c].int_value;
			break;
		}

		case bc_opcode::k_logical_nonequal: {
			QUARK_ASSERT(stack.check_reg_bool(i._a));
			QUARK_ASSERT(stack.check_reg_any(i._b));
			QUARK_ASSERT(stack.check_reg_any(i._c));

			const auto& type = frame_ptr->_symbols._symbols[i._b].second._value_type;
			QUARK_ASSERT(peek2(types, type).is_int() == false);
			const auto left = stack.read_register(i._b);
			const auto right = stack.read_register(i._c);
			long diff = compare_value_true_deep(backend, left, right, type);

			regs[i._a].bool_value = diff != 0;
			break;
		}
		case bc_opcode::k_logical_nonequal_int: {
			QUARK_ASSERT(stack.check_reg_bool(i._a));
			QUARK_ASSERT(stack.check_reg_int(i._b));
			QUARK_ASSERT(stack.check_reg_int(i._c));

			regs[i._a].bool_value = regs[i._b].int_value != regs[i._c].int_value;
			break;
		}


		//////////////////////////////		ARITHMETICS


		//??? Replace by a | b opcode.
		case bc_opcode::k_add_bool: {
			QUARK_ASSERT(stack.check_reg_bool(i._a));
			QUARK_ASSERT(stack.check_reg_bool(i._b));
			QUARK_ASSERT(stack.check_reg_bool(i._c));

			regs[i._a].bool_value = regs[i._b].bool_value + regs[i._c].bool_value;
			break;
		}
		case bc_opcode::k_add_int: {
			QUARK_ASSERT(stack.check_reg_int(i._a));
			QUARK_ASSERT(stack.check_reg_int(i._b));
			QUARK_ASSERT(stack.check_reg_int(i._c));

			regs[i._a].int_value = regs[i._b].int_value + regs[i._c].int_value;
			break;
		}
		case bc_opcode::k_add_double: {
			QUARK_ASSERT(stack.check_reg_double(i._a));
			QUARK_ASSERT(stack.check_reg_double(i._b));
			QUARK_ASSERT(stack.check_reg_double(i._c));

			regs[i._a].double_value = regs[i._b].double_value + regs[i._c].double_value;
			break;
		}

		case bc_opcode::k_concat_strings: {
			QUARK_ASSERT(stack.check_reg_string(i._a));
			QUARK_ASSERT(stack.check_reg_string(i._b));
			QUARK_ASSERT(stack.check_reg_string(i._c));

			const auto type = type_t::make_string();

			const auto s2 = concat_strings(backend, regs[i._b], regs[i._c]);
			release_value_safe(backend, regs[i._a], type);
			regs[i._a] = s2;
			break;
		}

		case bc_opcode::k_concat_vectors_w_external_elements: {
			QUARK_ASSERT(stack.check_reg_vector_w_external_elements(i._a));
			QUARK_ASSERT(stack.check_reg_vector_w_external_elements(i._b));
			QUARK_ASSERT(stack.check_reg_vector_w_external_elements(i._c));

			const auto& vector_type = frame_ptr->_symbols._symbols[i._a].second._value_type;
			const auto r = concatunate_vectors(backend, vector_type, regs[i._b], regs[i._c]);
			const auto& result = rt_value_t(backend, vector_type, r, rt_value_t::rc_mode::adopt);
			stack.write_register(i._a, result);
			break;
		}

		case bc_opcode::k_subtract_double: {
			QUARK_ASSERT(stack.check_reg_double(i._a));
			QUARK_ASSERT(stack.check_reg_double(i._b));
			QUARK_ASSERT(stack.check_reg_double(i._c));

			regs[i._a].double_value = regs[i._b].double_value - regs[i._c].double_value;
			break;
		}
		case bc_opcode::k_subtract_int: {
			QUARK_ASSERT(stack.check_reg_int(i._a));
			QUARK_ASSERT(stack.check_reg_int(i._b));
			QUARK_ASSERT(stack.check_reg_int(i._c));

			regs[i._a].int_value = regs[i._b].int_value - regs[i._c].int_value;
			break;
		}
		case bc_opcode::k_multiply_double: {
			QUARK_ASSERT(stack.check_reg_double(i._a));
			QUARK_ASSERT(stack.check_reg_double(i._c));
			QUARK_ASSERT(stack.check_reg_double(i._c));

			regs[i._a].double_value = regs[i._b].double_value * regs[i._c].double_value;
			break;
		}
		case bc_opcode::k_multiply_int: {
			QUARK_ASSERT(stack.check_reg_int(i._a));
			QUARK_ASSERT(stack.check_reg_int(i._c));
			QUARK_ASSERT(stack.check_reg_int(i._c));

			regs[i._a].int_value = regs[i._b].int_value * regs[i._c].int_value;
			break;
		}
		case bc_opcode::k_divide_double: {
			QUARK_ASSERT(stack.check_reg_double(i._a));
			QUARK_ASSERT(stack.check_reg_double(i._b));
			QUARK_ASSERT(stack.check_reg_double(i._c));

			const auto right = regs[i._c].double_value;
			if(right == 0.0f){
				quark::throw_runtime_error("EEE_DIVIDE_BY_ZERO");
			}
			regs[i._a].double_value = regs[i._b].double_value / right;
			break;
		}
		case bc_opcode::k_divide_int: {
			QUARK_ASSERT(stack.check_reg_int(i._a));
			QUARK_ASSERT(stack.check_reg_int(i._b));
			QUARK_ASSERT(stack.check_reg_int(i._c));

			const auto right = regs[i._c].int_value;
			if(right == 0.0f){
				quark::throw_runtime_error("EEE_DIVIDE_BY_ZERO");
			}
			regs[i._a].int_value = regs[i._b].int_value / right;
			break;
		}
		case bc_opcode::k_remainder_int: {
			QUARK_ASSERT(stack.check_reg_int(i._a));
			QUARK_ASSERT(stack.check_reg_int(i._b));
			QUARK_ASSERT(stack.check_reg_int(i._c));

			const auto right = regs[i._c].int_value;
			if(right == 0){
				quark::throw_runtime_error("EEE_DIVIDE_BY_ZERO");
			}
			regs[i._a].int_value = regs[i._b].int_value % right;
			break;
		}


		case bc_opcode::k_logical_and_bool: {
			QUARK_ASSERT(stack.check_reg_bool(i._a));
			QUARK_ASSERT(stack.check_reg_bool(i._b));
			QUARK_ASSERT(stack.check_reg_bool(i._c));

			regs[i._a].bool_value = regs[i._b].bool_value  && regs[i._c].bool_value;
			break;
		}
		case bc_opcode::k_logical_and_int: {
			QUARK_ASSERT(stack.check_reg_bool(i._a));
			QUARK_ASSERT(stack.check_reg_int(i._b));
			QUARK_ASSERT(stack.check_reg_int(i._c));

			regs[i._a].bool_value = (regs[i._b].int_value != 0) && (regs[i._c].int_value != 0);
			break;
		}
		case bc_opcode::k_logical_and_double: {
			QUARK_ASSERT(stack.check_reg_bool(i._a));
			QUARK_ASSERT(stack.check_reg_double(i._b));
			QUARK_ASSERT(stack.check_reg_double(i._c));

			regs[i._a].bool_value = (regs[i._b].double_value != 0) && (regs[i._c].double_value != 0);
			break;
		}

		case bc_opcode::k_logical_or_bool: {
			QUARK_ASSERT(stack.check_reg_bool(i._a));
			QUARK_ASSERT(stack.check_reg_bool(i._b));
			QUARK_ASSERT(stack.check_reg_bool(i._c));

			regs[i._a].bool_value = regs[i._b].bool_value || regs[i._c].bool_value;
			break;
		}
		case bc_opcode::k_logical_or_int: {
			QUARK_ASSERT(stack.check_reg_bool(i._a));
			QUARK_ASSERT(stack.check_reg_int(i._b));
			QUARK_ASSERT(stack.check_reg_int(i._c));

			regs[i._a].bool_value = (regs[i._b].int_value != 0) || (regs[i._c].int_value != 0);
			break;
		}
		case bc_opcode::k_logical_or_double: {
			QUARK_ASSERT(stack.check_reg_bool(i._a));
			QUARK_ASSERT(stack.check_reg_double(i._b));
			QUARK_ASSERT(stack.check_reg_double(i._c));

			regs[i._a].bool_value = (regs[i._b].double_value != 0.0f) || (regs[i._c].double_value != 0.0f);
			break;
		}


		//////////////////////////////		NONE


		default:
			QUARK_ASSERT(false);
			quark::throw_exception();
		}
		pc++;

		if(k_trace_stepping){
			trace_interpreter(vm, pc);
		}
	}
	return { false, rt_value_t::make_undefined() };
}


//////////////////////////////////////////		FUNCTIONS



std::shared_ptr<value_entry_t> find_global_symbol2(interpreter_t& vm, const module_symbol_t& s){
	QUARK_ASSERT(vm.check_invariant());
	QUARK_ASSERT(s.s.size() > 0);

	const auto& symbols = vm._imm->_program._globals._symbols;
    const auto& it = std::find_if(
    	symbols._symbols.begin(),
    	symbols._symbols.end(),
    	[&s](const std::pair<std::string, symbol_t>& e) { return e.first == s.s; }
	);
	if(it != symbols._symbols.end()){
		const auto index = static_cast<int>(it - symbols._symbols.begin());
		const auto pos = get_global_n_pos(index);
		QUARK_ASSERT(pos >= 0 && pos < vm._stack.size());

		const auto value_entry = value_entry_t{
			vm._stack.load_value(pos, it->second._value_type),
			module_symbol_t(it->first),
			it->second,
			static_cast<int>(index)
		};
        QUARK_ASSERT(value_entry.check_invariant());
		auto a = std::make_shared<value_entry_t>(value_entry);
        QUARK_ASSERT(a->check_invariant());
        return a;
	}
	else{
		return nullptr;
	}
}


static std::string opcode_to_string(bc_opcode opcode){
	return k_opcode_info.at(opcode)._as_text;
}

json_t interpreter_to_json(interpreter_t& vm){
	std::vector<json_t> callstack;
	QUARK_ASSERT(vm.check_invariant());

	const auto stack = stack_to_json(vm._stack, vm._backend);

	return json_t::make_object({
		{ "ast", bcprogram_to_json(vm._imm->_program) },
		{ "callstack", stack }
	});
}

static std::vector<json_t> bc_symbols_to_json(value_backend_t& backend, const symbol_table_t& symbols){
	QUARK_ASSERT(backend.check_invariant());

	return symbols_to_json(backend.types, symbols);
/*
	std::vector<json_t> r;
	int symbol_index = 0;
	for(const auto& e: symbols._symbols){
		const auto& symbol = e.second;
		const auto symbol_type_str = symbol._symbol_type == bc_symbol_t::type::immutable ? "immutable" : "mutable";
		const auto e2 = json_t::make_array({
			symbol_index,
			e.first,
			symbol_type_str,
			json_t::make_object({
				{ "value_type", type_to_json(backend.types, symbol._value_type) },
			}),
			value_and_type_to_json(backend.types, symbol._init)
		});
		r.push_back(e2);
		symbol_index++;
	}
	return r;
*/

}

static json_t frame_to_json(value_backend_t& backend, const bc_static_frame_t& frame){
	QUARK_ASSERT(backend.check_invariant());

	std::vector<json_t> instructions;
	int pc = 0;
	for(const auto& e: frame._instructions){
		const auto i = json_t::make_array({
			pc,
			opcode_to_string(e._opcode),
			json_t(e._a),
			json_t(e._b),
			json_t(e._c)
		});
		instructions.push_back(i);
		pc++;
	}

	return json_t::make_object({
		{ "symbols", json_t::make_array(bc_symbols_to_json(backend, frame._symbols)) },
		{ "instructions", json_t::make_array(instructions) }
	});
}

static json_t types_to_json(const types_t& types, const std::vector<type_t>& types2){
	std::vector<json_t> r;
	int id = 0;
	for(const auto& e: types2){
		const auto i = json_t::make_array({
			id,
			type_to_json(types, e)
		});
		r.push_back(i);
		id++;
	}
	return json_t::make_array(r);
}

static json_t functiondef_to_json(value_backend_t& backend, const bc_function_definition_t& def){
	QUARK_ASSERT(backend.check_invariant());

	return json_t::make_array({
		func_link_to_json(backend.types, def.func_link),
		def._frame_ptr ? frame_to_json(backend, *def._frame_ptr) : json_t("no BC frame = native func")
	});
}

json_t bcprogram_to_json(const bc_program_t& program){
	auto backend = value_backend_t({}, bc_make_struct_layouts(program._types), program._types, config_t { vector_backend::hamt, dict_backend::hamt, false });

	std::vector<json_t> callstack;
	std::vector<json_t> function_defs;
	for(const auto& e: program._function_defs){
		function_defs.push_back(json_t::make_array({
			functiondef_to_json(backend, e)
		}));
	}

	return json_t::make_object({
		{ "globals", frame_to_json(backend, program._globals) },
		{ "types", types_to_json(program._types) },
		{ "function_defs", json_t::make_array(function_defs) }
//		{ "callstack", json_t::make_array(callstack) }
	});
}

}	//	floyd
