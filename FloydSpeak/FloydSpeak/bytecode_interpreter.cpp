//
//  parser_evaluator.cpp
//  FloydSpeak
//
//  Created by Marcus Zetterquist on 26/07/16.
//  Copyright © 2016 Marcus Zetterquist. All rights reserved.
//

#include "bytecode_interpreter.h"

#include "ast_value.h"
#include "json_support.h"
#include "host_functions.hpp"

#include <cmath>
#include <sys/time.h>

#include <thread>
#include <chrono>
#include <algorithm>


namespace floyd {

using std::vector;
using std::string;
using std::pair;
using std::shared_ptr;
using std::make_shared;

#define ASSERT QUARK_ASSERT



inline const typeid_t& lookup_full_type(const interpreter_t& vm, const bc_typeid_t& type){
	return vm._imm->_program._types[type];
}
inline const base_type lookup_full_basetype(const interpreter_t& vm, const bc_typeid_t& type){
	return vm._imm->_program._types[type].get_base_type();
}

int get_global_n_pos(int n){
	return k_frame_overhead + n;
}
int get_local_n_pos(int frame_pos, int n){
	return frame_pos + n;
}


QUARK_UNIT_TEST("", "", "", ""){
	const auto pod_size = sizeof(bc_pod_value_t);
	QUARK_ASSERT(pod_size == 8);

	const auto value_size = sizeof(bc_value_t);
	QUARK_ASSERT(value_size == 64);

	struct mockup_value_t {
		private: bool _is_ext;
		public: bc_pod_value_t _pod;
	};

	const auto mockup_value_size = sizeof(mockup_value_t);
	QUARK_ASSERT(mockup_value_size == 16);

	const auto instruction2_size = sizeof(bc_instruction_t);
	QUARK_ASSERT(instruction2_size == 8);


	const auto immer_vec_bool_size = sizeof(immer::vector<bool>);
	const auto immer_vec_int_size = sizeof(immer::vector<int>);
	const auto immer_vec_string_size = sizeof(immer::vector<string>);

	QUARK_ASSERT(immer_vec_bool_size == 32);
	QUARK_ASSERT(immer_vec_int_size == 32);
	QUARK_ASSERT(immer_vec_string_size == 32);


	const auto immer_stringkey_map_size = sizeof(immer::map<std::string, float>);
	QUARK_ASSERT(immer_stringkey_map_size == 16);

	const auto immer_intkey_map_size = sizeof(immer::map<uint32_t, float>);
	QUARK_ASSERT(immer_intkey_map_size == 16);
}


//////////////////////////////////////////		COMPARE


inline int bc_limit(int value, int min, int max){
	if(value < min){
		return min;
	}
	else if(value > max){
		return max;
	}
	else{
		return value;
	}
}

int bc_compare_string(const std::string& left, const std::string& right){
	// ### Better if it doesn't use c_ptr since that is non-pure string handling.
	return bc_limit(std::strcmp(left.c_str(), right.c_str()), -1, 1);
}

QUARK_UNIT_TESTQ("bc_compare_string()", ""){
	QUARK_TEST_VERIFY(bc_compare_string("", "") == 0);
}
QUARK_UNIT_TESTQ("bc_compare_string()", ""){
	QUARK_TEST_VERIFY(bc_compare_string("aaa", "aaa") == 0);
}
QUARK_UNIT_TESTQ("bc_compare_string()", ""){
	QUARK_TEST_VERIFY(bc_compare_string("b", "a") == 1);
}


//??? Slow!.
int bc_compare_struct_true_deep(const std::vector<bc_value_t>& left, const std::vector<bc_value_t>& right, const typeid_t& type){
	const auto& struct_def = type.get_struct();

	for(int i = 0 ; i < struct_def._members.size() ; i++){
		const auto& member_type = struct_def._members[i]._type;

		int diff = bc_compare_value_true_deep(left[i], right[i], member_type);
		if(diff != 0){
			return diff;
		}
	}
	return 0;
}

//	Compare vector element by element.
//	### Think more of equality when vectors have different size and shared elements are equal.
int bc_compare_vector_true_deep(const immer::vector<bc_value_t>& left, const immer::vector<bc_value_t>& right, const typeid_t& type){
//	QUARK_ASSERT(left.check_invariant());
//	QUARK_ASSERT(right.check_invariant());
//	QUARK_ASSERT(left._element_type == right._element_type);

	const auto& shared_count = std::min(left.size(), right.size());
	const auto& element_type = typeid_t(type.get_vector_element_type());
	for(int i = 0 ; i < shared_count ; i++){
		const auto element_result = bc_compare_value_true_deep(left[i], right[i], element_type);
		if(element_result != 0){
			return element_result;
		}
	}
	if(left.size() == right.size()){
		return 0;
	}
	else if(left.size() > right.size()){
		return -1;
	}
	else{
		return +1;
	}
}

template <typename Map>
bool bc_map_compare (Map const &lhs, Map const &rhs) {
	// No predicate needed because there is operator== for pairs already.
	return lhs.size() == rhs.size() && std::equal(lhs.begin(), lhs.end(), rhs.begin());
}

int bc_compare_dict_true_deep(const immer::map<std::string, bc_value_t>& left, const immer::map<std::string, bc_value_t>& right, const typeid_t& type){
	const auto& element_type = typeid_t(type.get_dict_value_type());

	auto left_it = left.begin();
	auto left_end_it = left.end();

	auto right_it = right.begin();
	auto right_end_it = right.end();

	while(left_it != left_end_it && right_it != right_end_it){
		auto left_key = (*left_it).first;
		auto right_key = (*right_it).first;

		const auto key_result = bc_compare_string(left_key, right_key);
		if(key_result != 0){
			return key_result;
		}

		const auto element_result = bc_compare_value_true_deep((*left_it).second, (*right_it).second, element_type);
		if(element_result != 0){
			return element_result;
		}

		left_it++;
		right_it++;
	}

	if(left_it == left_end_it && right_it == right_end_it){
		return 0;
	}
	else if(left_it == left_end_it && right_it != right_end_it){
		return 1;
	}
	else if(left_it != left_end_it && right_it == right_end_it){
		return -1;
	}
	QUARK_ASSERT(false)
	throw std::exception();
}

int bc_compare_json_values(const json_t& lhs, const json_t& rhs){
	if(lhs == rhs){
		return 0;
	}
	else{
		// ??? implement compare.
		assert(false);
	}
}

int bc_compare_value_true_deep(const bc_value_t& left, const bc_value_t& right, const typeid_t& type0){
	QUARK_ASSERT(left._type == right._type);
	QUARK_ASSERT(left.check_invariant());
	QUARK_ASSERT(right.check_invariant());

	const auto type = type0;
	if(type.is_undefined()){
		return 0;
	}
	else if(type.is_bool()){
		return (left.get_bool_value() ? 1 : 0) - (right.get_bool_value() ? 1 : 0);
	}
	else if(type.is_int()){
		return bc_limit(left.get_int_value() - right.get_int_value(), -1, 1);
	}
	else if(type.is_float()){
		const auto a = left.get_float_value();
		const auto b = right.get_float_value();
		if(a > b){
			return 1;
		}
		else if(a < b){
			return -1;
		}
		else{
			return 0;
		}
	}
	else if(type.is_string()){
		return bc_compare_string(left.get_string_value(), right.get_string_value());
	}
	else if(type.is_json_value()){
		return bc_compare_json_values(left.get_json_value(), right.get_json_value());
	}
	else if(type.is_typeid()){
		if(left.get_typeid_value() == right.get_typeid_value()){
			return 0;
		}
		else{
			return -1;//??? Hack -- should return +1 depending on values.
		}
	}
	else if(type.is_struct()){
		//	Make sure the EXACT struct types are the same -- not only that they are both structs
		return bc_compare_struct_true_deep(left.get_struct_value(), right.get_struct_value(), type0);
	}
	else if(type.is_vector()){
		const auto& left_vec = get_vector_value(left);
		const auto& right_vec = get_vector_value(right);
		return bc_compare_vector_true_deep(*left_vec, *right_vec, type0);
	}
	else if(type.is_dict()){
		const auto& left2 = get_dict_value(left);
		const auto& right2 = get_dict_value(right);
		return bc_compare_dict_true_deep(left2, right2, type0);
	}
	else if(type.is_function()){
		QUARK_ASSERT(false);
		return 0;
	}
	else{
		QUARK_ASSERT(false);
		throw std::exception();
	}
}

extern const std::map<bc_opcode, opcode_info_t> k_opcode_info = {
	{ bc_opcode::k_nop, { "nop", opcode_info_t::encoding::k_e_0000 }},

	{ bc_opcode::k_load_global_obj, { "load_global_obj", opcode_info_t::encoding::k_k_0ri0 } },
	{ bc_opcode::k_load_global_intern, { "load_global_intern", opcode_info_t::encoding::k_k_0ri0 } },

	{ bc_opcode::k_store_global_obj, { "store_global_obj", opcode_info_t::encoding::k_r_0ir0 } },
	{ bc_opcode::k_store_global_intern, { "store_global_intern", opcode_info_t::encoding::k_r_0ir0 } },

	{ bc_opcode::k_copy_reg_intern, { "copy_reg_intern", opcode_info_t::encoding::k_q_0rr0 } },
	{ bc_opcode::k_copy_reg_obj, { "copy_reg_obj", opcode_info_t::encoding::k_q_0rr0 } },

	{ bc_opcode::k_get_struct_member, { "get_struct_member", opcode_info_t::encoding::k_s_0rri } },

	{ bc_opcode::k_lookup_element_string, { "lookup_element_string", opcode_info_t::encoding::k_o_0rrr } },
	{ bc_opcode::k_lookup_element_json_value, { "lookup_element_jsonvalue", opcode_info_t::encoding::k_o_0rrr } },
	{ bc_opcode::k_lookup_element_vector, { "lookup_element_vector", opcode_info_t::encoding::k_o_0rrr } },
	{ bc_opcode::k_lookup_element_dict, { "lookup_element_dict", opcode_info_t::encoding::k_o_0rrr } },

	{ bc_opcode::k_call, { "call", opcode_info_t::encoding::k_s_0rri } },

	{ bc_opcode::k_add_bool, { "add_bool", opcode_info_t::encoding::k_o_0rrr } },
	{ bc_opcode::k_add_int, { "add_int", opcode_info_t::encoding::k_o_0rrr } },
	{ bc_opcode::k_add_float, { "add_float", opcode_info_t::encoding::k_o_0rrr } },
	{ bc_opcode::k_add_string, { "add_string", opcode_info_t::encoding::k_o_0rrr } },
	{ bc_opcode::k_add_vector, { "add_vector", opcode_info_t::encoding::k_o_0rrr } },
	{ bc_opcode::k_subtract_float, { "subtract_float", opcode_info_t::encoding::k_o_0rrr } },
	{ bc_opcode::k_subtract_int, { "subtract_int", opcode_info_t::encoding::k_o_0rrr } },
	{ bc_opcode::k_multiply_float, { "multiply_float", opcode_info_t::encoding::k_o_0rrr } },
	{ bc_opcode::k_multiply_int, { "multiply_int", opcode_info_t::encoding::k_o_0rrr } },
	{ bc_opcode::k_divide_float, { "divide_float", opcode_info_t::encoding::k_o_0rrr } },
	{ bc_opcode::k_divide_int, { "divide_int", opcode_info_t::encoding::k_o_0rrr } },
	{ bc_opcode::k_remainder_int, { "remainder_int", opcode_info_t::encoding::k_o_0rrr } },

	{ bc_opcode::k_logical_and_bool, { "logical_and_bool", opcode_info_t::encoding::k_o_0rrr } },
	{ bc_opcode::k_logical_and_int, { "logical_and_int", opcode_info_t::encoding::k_o_0rrr } },
	{ bc_opcode::k_logical_and_float, { "logical_and_float", opcode_info_t::encoding::k_o_0rrr } },
	{ bc_opcode::k_logical_or_bool, { "logical_or_bool", opcode_info_t::encoding::k_o_0rrr } },
	{ bc_opcode::k_logical_or_int, { "logical_or_int", opcode_info_t::encoding::k_o_0rrr } },
	{ bc_opcode::k_logical_or_float, { "logical_or_float", opcode_info_t::encoding::k_o_0rrr } },


	{ bc_opcode::k_comparison_smaller_or_equal, { "comparison_smaller_or_equal", opcode_info_t::encoding::k_o_0rrr } },
	{ bc_opcode::k_comparison_smaller_or_equal_int, { "comparison_smaller_or_equal_int", opcode_info_t::encoding::k_o_0rrr } },

	{ bc_opcode::k_comparison_smaller, { "comparison_smaller", opcode_info_t::encoding::k_o_0rrr } },
	{ bc_opcode::k_comparison_smaller_int, { "comparison_smaller_int", opcode_info_t::encoding::k_o_0rrr } },

	{ bc_opcode::k_logical_equal, { "logical_equal", opcode_info_t::encoding::k_o_0rrr } },
	{ bc_opcode::k_logical_equal_int, { "logical_equal_int", opcode_info_t::encoding::k_o_0rrr } },

	{ bc_opcode::k_logical_nonequal, { "logical_nonequal", opcode_info_t::encoding::k_o_0rrr } },
	{ bc_opcode::k_logical_nonequal_int, { "logical_nonequal_int", opcode_info_t::encoding::k_o_0rrr } },


	{ bc_opcode::k_new_1, { "new_1", opcode_info_t::encoding::k_t_0rii } },
	{ bc_opcode::k_new_vector, { "new_vector", opcode_info_t::encoding::k_t_0rii } },
	{ bc_opcode::k_new_dict, { "new_dict", opcode_info_t::encoding::k_t_0rii } },
	{ bc_opcode::k_new_struct, { "new_struct", opcode_info_t::encoding::k_t_0rii } },

	{ bc_opcode::k_return, { "return", opcode_info_t::encoding::k_p_0r00 } },
	{ bc_opcode::k_stop, { "stop", opcode_info_t::encoding::k_e_0000 } },

	{ bc_opcode::k_push_frame_ptr, { "push_frame_ptr", opcode_info_t::encoding::k_e_0000 } },
	{ bc_opcode::k_pop_frame_ptr, { "pop_frame_ptr", opcode_info_t::encoding::k_e_0000 } },
	{ bc_opcode::k_push_intern, { "push_intern", opcode_info_t::encoding::k_p_0r00 } },
	{ bc_opcode::k_push_obj, { "push_obj", opcode_info_t::encoding::k_p_0r00 } },
	{ bc_opcode::k_popn, { "popn", opcode_info_t::encoding::k_n_0ii0 } },

	{ bc_opcode::k_branch_false_bool, { "branch_false_bool", opcode_info_t::encoding::k_k_0ri0 } },
	{ bc_opcode::k_branch_true_bool, { "branch_true_bool", opcode_info_t::encoding::k_k_0ri0 } },
	{ bc_opcode::k_branch_zero_int, { "branch_zero_int", opcode_info_t::encoding::k_k_0ri0 } },
	{ bc_opcode::k_branch_notzero_int, { "branch_notzero_int", opcode_info_t::encoding::k_k_0ri0 } },

	{ bc_opcode::k_branch_smaller_int, { "branch_smaller_int", opcode_info_t::encoding::k_s_0rri } },
	{ bc_opcode::k_branch_smaller_or_equal_int, { "branch_smaller_or_equal_int", opcode_info_t::encoding::k_s_0rri } },

	{ bc_opcode::k_branch_always, { "branch_always", opcode_info_t::encoding::k_l_00i0 } }


};


//??? Use enum with register / immediate / unused, current info is not enough.

reg_flags_t encoding_to_reg_flags(opcode_info_t::encoding e){
	if(e == opcode_info_t::encoding::k_e_0000){
		return { false, false, false };
	}
	else if(e == opcode_info_t::encoding::k_f_trr0){
		return { true, true, false };
	}
	else if(e == opcode_info_t::encoding::k_g_trri){
		return { true, true, false };
	}
	else if(e == opcode_info_t::encoding::k_h_trrr){
		return { true, true, true };
	}
	else if(e == opcode_info_t::encoding::k_i_trii){
		return { true, false, false };
	}
	else if(e == opcode_info_t::encoding::k_j_tr00){
		return { true, false, false };
	}
	else if(e == opcode_info_t::encoding::k_k_0ri0){
		return { true, false, false };
	}
	else if(e == opcode_info_t::encoding::k_l_00i0){
		return { false, false, false };
	}
	else if(e == opcode_info_t::encoding::k_m_tr00){
		return { true, false, false };
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
		throw std::exception();
	}
}


//////////////////////////////////////////		bc_instruction_t


#if DEBUG
bool bc_instruction_t::check_invariant() const {
	const auto encoding = k_opcode_info.at(_opcode)._encoding;
	const auto reg_flags = encoding_to_reg_flags(encoding);

/*
	QUARK_ASSERT(check_register(_reg_a, reg_flags._a));
	QUARK_ASSERT(check_register(_reg_b, reg_flags._b));
	QUARK_ASSERT(check_register(_reg_c, reg_flags._c));
*/
	return true;
}
#endif


//////////////////////////////////////////		bc_frame_t


bc_frame_t::bc_frame_t(const std::vector<bc_instruction_t>& instrs2, const std::vector<std::pair<std::string, bc_symbol_t>>& symbols, const std::vector<typeid_t>& args) :
	_instrs2(instrs2),
	_symbols(symbols),
	_args(args)
{
	const auto parameter_count = static_cast<int>(_args.size());

	for(int i = 0 ; i < _symbols.size() ; i++){
		const auto type = _symbols[i].second._value_type;
		const bool ext = is_encoded_as_ext(type);
		_exts.push_back(ext);
	}

	//	Process the locals & temps. They go after any parameters, which already sits on stack.
	for(vector<bc_value_t>::size_type i = parameter_count ; i < _symbols.size() ; i++){
		const auto& symbol = _symbols[i];
		bool is_ext = _exts[i];

		_locals_exts.push_back(is_ext);

		//	Variable slot.
		//	This is just a variable slot without constant. We need to put something there, but that don't confuse RC.
		//	Problem is that IF this is an RC_object, it WILL be decremented when written to.
		//	Use a placeholder object of correct type.
		if(symbol.second._const_value._type.get_base_type() == base_type::k_internal_undefined){
			if(is_ext){
				const auto value = bc_value_t(symbol.second._value_type, bc_value_t::mode::k_unwritten_ext_value);
				_locals.push_back(value);
			}
			else{
				const auto value = make_def(symbol.second._value_type);
				const auto bc = value_to_bc(value);
				_locals.push_back(bc);
			}
		}

		//	Constant.
		else{
			_locals.push_back(symbol.second._const_value);
		}
	}

	QUARK_ASSERT(check_invariant());
}

bool bc_frame_t::check_invariant() const {
//	QUARK_ASSERT(_body.check_invariant());
	QUARK_ASSERT(_symbols.size() == _exts.size());

	for(const auto& e: _instrs2){
		const auto encoding = k_opcode_info.at(e._opcode)._encoding;
		const auto reg_flags = encoding_to_reg_flags(encoding);

/*
		QUARK_ASSERT(check_register__local(e._reg_a, reg_flags._a));
		QUARK_ASSERT(check_register__local(e._reg_b, reg_flags._b));
		QUARK_ASSERT(check_register__local(e._reg_c, reg_flags._c));
*/

	}
	return true;
}


//////////////////////////////////////////		interpreter_stack_t



frame_pos_t interpreter_stack_t::read_prev_frame(int frame_pos) const{
//	QUARK_ASSERT(vm.check_invariant());
	QUARK_ASSERT(frame_pos >= k_frame_overhead);

	const auto v = load_intq(frame_pos - k_frame_overhead + 0);
	QUARK_ASSERT(v < frame_pos);
	QUARK_ASSERT(v >= 0);

	const auto ptr = _entries[frame_pos - k_frame_overhead + 1]._frame_ptr;
	QUARK_ASSERT(ptr != nullptr);
	QUARK_ASSERT(ptr->check_invariant());

	return frame_pos_t{ v, ptr };
}

#if DEBUG
bool interpreter_stack_t::check_stack_frame(const frame_pos_t& in_frame) const{
	QUARK_ASSERT(in_frame._frame_ptr != nullptr && in_frame._frame_ptr->check_invariant());

	if(in_frame._frame_pos == 0){
		return true;
	}
	else{
		for(int i = 0 ; i < in_frame._frame_ptr->_symbols.size() ; i++){
			const auto& symbol = in_frame._frame_ptr->_symbols[i];

			bool symbol_ext = is_encoded_as_ext(symbol.second._value_type);
			int local_pos = get_local_n_pos(in_frame._frame_pos, i);

			bool stack_ext = is_ext(local_pos);
			QUARK_ASSERT(symbol_ext == stack_ext);
		}

		const auto prev_frame = read_prev_frame(in_frame._frame_pos);
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

vector<std::pair<int, int>> interpreter_stack_t::get_stack_frames(int frame_pos) const{
	QUARK_ASSERT(check_invariant());

	//	We use the entire current stack to calc top frame's size. This can be wrong, if
	//	someone pushed more stuff there. Same goes with the previous stack frames too..
	vector<std::pair<int, int>> result{ { frame_pos, static_cast<int>(size()) - frame_pos }};

	while(frame_pos > k_frame_overhead){
		const auto prev_frame = read_prev_frame(frame_pos);
		const auto prev_size = (frame_pos - k_frame_overhead) - prev_frame._frame_pos;
		result.push_back(std::pair<int, int>{ frame_pos, prev_size });

		frame_pos = prev_frame._frame_pos;
	}
	return result;
}

json_t interpreter_stack_t::stack_to_json() const{
	const int size = static_cast<int>(_stack_size);

	const auto stack_frames = get_stack_frames(get_current_frame_start());

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
		const auto ext = is_encoded_as_ext(debug_type);
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
			unwritten ? json_t("UNWRITTEN") : bcvalue_to_json(bc_value_t{bc})
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


inline const bc_function_definition_t& get_function_def(const interpreter_t& vm, int function_id){
	QUARK_ASSERT(vm.check_invariant());

	QUARK_ASSERT(function_id >= 0 && function_id < vm._imm->_program._function_defs.size())

	const auto& function_def = vm._imm->_program._function_defs[function_id];
	return function_def;
}


//??? Use bc_value_t:s instead of bc_value_t -- types are known via function-signature.
bc_value_t call_function_bc(interpreter_t& vm, const bc_value_t& f, const bc_value_t args[], int arg_count){
#if DEBUG
	QUARK_ASSERT(vm.check_invariant());
	QUARK_ASSERT(f.check_invariant());
	for(int i = 0 ; i < arg_count ; i++){ QUARK_ASSERT(args[i].check_invariant()); };
	QUARK_ASSERT(f._type.is_function());
#endif

	const auto& function_def = get_function_def(vm, f.get_function_value());
	if(function_def._host_function_id != 0){
		const auto host_function_id = function_def._host_function_id;
		QUARK_ASSERT(host_function_id >= 0);

		const auto& host_function = vm._imm->_host_functions.at(host_function_id);

		//	arity
	//	QUARK_ASSERT(args.size() == host_function._function_type.get_function_args().size());

		const auto& result = (host_function)(vm, &args[0], arg_count);
		return result;
	}
	else{
#if DEBUG
		const auto& arg_types = f._type.get_function_args();

		//	arity
		QUARK_ASSERT(arg_count == arg_types.size());

		for(int i = 0 ; i < arg_count; i++){
			if(args[i]._type != arg_types[i]){
				QUARK_ASSERT(false);
			}
		}
#endif

		vm._stack.save_frame();

		//??? use exts-info inside function_def.
		//	We push the values to the stack = the stack will take RC ownership of the values.
		vector<bool> exts;
		for(int i = 0 ; i < arg_count ; i++){
			const auto& bc = args[i];
			bool is_ext = is_encoded_as_ext(args[i]._type);
			exts.push_back(is_ext);
			if(is_ext){
				vm._stack.push_obj(bc);
			}
			else{
				vm._stack.push_intern(bc);
			}
		}

		vm._stack.open_frame(*function_def._frame_ptr, arg_count);
		const auto& result = execute_instructions(vm, function_def._frame_ptr->_instrs2);
		vm._stack.close_frame(*function_def._frame_ptr);
		vm._stack.pop_batch(exts);
		vm._stack.restore_frame();

		if(vm._imm->_program._types[result.first].is_void() == false){
			return result.second;
		}
		else{
			return bc_value_t::make_undefined();
		}
	}
}


json_t bcvalue_to_json(const bc_value_t& v){
	if(v._type.is_undefined()){
		return json_t();
	}
	else if(v._type.is_internal_dynamic()){
		return json_t();
	}
	else if(v._type.is_void()){
		return json_t();
	}
	else if(v._type.is_bool()){
		return json_t(v.get_bool_value());
	}
	else if(v._type.is_int()){
		return json_t(static_cast<double>(v.get_int_value()));
	}
	else if(v._type.is_float()){
		return json_t(static_cast<double>(v.get_float_value()));
	}
	else if(v._type.is_string()){
		return json_t(v.get_string_value());
	}
	else if(v._type.is_json_value()){
		return v.get_json_value();
	}
	else if(v._type.is_typeid()){
		return typeid_to_ast_json(v.get_typeid_value(), json_tags::k_plain)._value;
	}
	else if(v._type.is_struct()){
		const auto& struct_value = v.get_struct_value();
		std::map<string, json_t> obj2;
		const auto& struct_def = v._type.get_struct();
		for(int i = 0 ; i < struct_def._members.size() ; i++){
			const auto& member = struct_def._members[i];
			const auto& key = member._name;
			const auto& type = member._type;
			const auto& value = struct_value[i];
			const auto& value2 = bcvalue_to_json(value);
			obj2[key] = value2;
		}
		return json_t::make_object(obj2);
	}
	else if(v._type.is_vector()){
		const auto vec = get_vector_value(v);
		std::vector<json_t> result;
		for(int i = 0 ; i < vec->size() ; i++){
			const auto element_value2 = vec->operator[](i);
			result.push_back(bcvalue_to_json(element_value2));
		}
		return result;
	}
	else if(v._type.is_dict()){
		const auto entries = get_dict_value(v);
		std::map<string, json_t> result;
		for(const auto& e: entries){
			const auto value2 = e.second;
			result[e.first] = bcvalue_to_json(value2);
		}
		return result;
	}
	else if(v._type.is_function()){
		return json_t::make_object(
			{
				{ "funtyp", typeid_to_ast_json(v._type, json_tags::k_plain)._value }
			}
		);
	}
	else{
		throw std::exception();
	}
}


json_t bcvalue_and_type_to_json(const bc_value_t& v){
	return json_t::make_array({
		typeid_to_ast_json(v._type, json_tags::k_plain)._value,
		bcvalue_to_json(v)
	});
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


//////////////////////////////////////////		INSTRUCTIONS


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


//	IMPORTANT: NO arguments are passed as DYN arguments.
void execute_new_1(interpreter_t& vm, int16_t dest_reg, int16_t target_itype, int16_t source_itype){
	QUARK_ASSERT(vm.check_invariant());

	const auto& target_type = lookup_full_type(vm, target_itype);
	QUARK_ASSERT(target_type.is_vector() == false && target_type.is_dict() == false && target_type.is_struct() == false);

	const int arg0_stack_pos = vm._stack.size() - 1;
	const auto input_value_type = lookup_full_type(vm, source_itype);
	const auto input_value = vm._stack.load_value(arg0_stack_pos + 0, input_value_type);

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
			const auto arg = bcvalue_to_json(input_value);
			return bc_value_t::make_json_value(arg);
		}
		else{
			return input_value;
		}
	}();

	vm._stack.write_register(dest_reg, result);
}


//	IMPORTANT: NO arguments are passed as DYN arguments.
void execute_new_vector(interpreter_t& vm, int16_t dest_reg, int16_t target_itype, int16_t arg_count){
	QUARK_ASSERT(vm.check_invariant());

	const auto& target_type = lookup_full_type(vm, target_itype);
	QUARK_ASSERT(target_type.is_vector());

	const auto& element_type = target_type.get_vector_element_type();
	QUARK_ASSERT(element_type.is_undefined() == false);
	QUARK_ASSERT(target_type.is_undefined() == false);

	const int arg0_stack_pos = vm._stack.size() - arg_count;
	bool is_element_ext = is_encoded_as_ext(element_type);

	immer::vector<bc_value_t> elements2;
	for(int i = 0 ; i < arg_count ; i++){
		const auto pos = arg0_stack_pos + i;
		QUARK_ASSERT(vm._stack._debug_types[pos] == element_type);
#if DEBUG
		const auto result = bc_value_t(element_type, vm._stack._entries[pos], is_element_ext);
#else
		const auto result = bc_value_t(vm._stack._entries[pos], is_element_ext);
#endif
		elements2 = elements2.push_back(result);
	}

	const auto result = make_vector_value(element_type, elements2);
	vm._stack.write_register_obj(dest_reg, result);
}


//	IMPORTANT: NO arguments are passed as DYN arguments.
void execute_new_dict(interpreter_t& vm, int16_t dest_reg, int16_t target_itype, int16_t arg_count){
	QUARK_ASSERT(vm.check_invariant());

	const auto& target_type = lookup_full_type(vm, target_itype);
	QUARK_ASSERT(target_type.is_dict());

	const int arg0_stack_pos = vm._stack.size() - arg_count;

	const auto& element_type = target_type.get_dict_value_type();
	QUARK_ASSERT(target_type.is_undefined() == false);
	QUARK_ASSERT(element_type.is_undefined() == false);

	const auto string_type = typeid_t::make_string();

	immer::map<string, bc_value_t> elements2;
	int dict_element_count = arg_count / 2;
	for(auto i = 0 ; i < dict_element_count ; i++){
		const auto key = vm._stack.load_value(arg0_stack_pos + i * 2 + 0, string_type);
		const auto value = vm._stack.load_value(arg0_stack_pos + i * 2 + 1, element_type);
		const auto key2 = key.get_string_value();
		elements2 = elements2.insert({ key2, value });
	}

	const auto result = make_dict_value(element_type, elements2);
	vm._stack.write_register_obj(dest_reg, result);
}

void execute_new_struct(interpreter_t& vm, int16_t dest_reg, int16_t target_itype, int16_t arg_count){
	QUARK_ASSERT(vm.check_invariant());

	const auto& target_type = lookup_full_type(vm, target_itype);
	QUARK_ASSERT(target_type.is_struct());

	const int arg0_stack_pos = vm._stack.size() - arg_count;

	const auto& struct_def = target_type.get_struct();
	std::vector<bc_value_t> elements2;
	for(int i = 0 ; i < arg_count ; i++){
		const auto member_type = struct_def._members[i]._type;
		const auto value = vm._stack.load_value(arg0_stack_pos + i, member_type);
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


//??? pass returns value(s) via parameters instead.
//???	Future: support dynamic Floyd functions too.

std::pair<bc_typeid_t, bc_value_t> execute_instructions(interpreter_t& vm, const std::vector<bc_instruction_t>& instructions){
	ASSERT(vm.check_invariant());
	ASSERT(instructions.empty() == true || (instructions.back()._opcode == bc_opcode::k_return || instructions.back()._opcode == bc_opcode::k_stop));

	interpreter_stack_t& stack = vm._stack;
	const bc_frame_t* frame_ptr = stack._current_frame_ptr;
	bc_pod_value_t* regs = stack._current_frame_entry_ptr;
	bc_pod_value_t* globals = &stack._entries[k_frame_overhead];

	const typeid_t* type_lookup = &vm._imm->_program._types[0];
	const auto type_count = vm._imm->_program._types.size();

//	QUARK_TRACE_SS("STACK:  " << json_to_pretty_string(stack.stack_to_json()));

	int pc = 0;
	while(true){
		ASSERT(pc >= 0);
		ASSERT(pc < instructions.size());
		const auto i = instructions[pc];

		ASSERT(vm.check_invariant());
		ASSERT(i.check_invariant());

		ASSERT(frame_ptr == stack._current_frame_ptr);
		ASSERT(regs == stack._current_frame_entry_ptr);


		const auto opcode = i._opcode;
		switch(opcode){

		case bc_opcode::k_nop:
			break;


		//////////////////////////////////////////		ACCESS GLOBALS


		case bc_opcode::k_load_global_obj: {
			ASSERT(stack.check_reg_obj(i._a));
			ASSERT(stack.check_global_access_obj(i._b));

			bc_value_t::release_ext_pod(regs[i._a]);
			const auto& new_value_pod = globals[i._b];
			regs[i._a] = new_value_pod;
			new_value_pod._ext->_rc++;
			break;
		}
		case bc_opcode::k_load_global_intern: {
			ASSERT(stack.check_reg_intern(i._a));

			regs[i._a] = globals[i._b];
			break;
		}


		case bc_opcode::k_store_global_obj: {
			ASSERT(stack.check_global_access_obj(i._a));
			ASSERT(stack.check_reg_obj(i._b));

			bc_value_t::release_ext_pod(globals[i._a]);
			const auto& new_value_pod = regs[i._b];
			globals[i._a] = new_value_pod;
			new_value_pod._ext->_rc++;
			break;
		}
		case bc_opcode::k_store_global_intern: {
			ASSERT(stack.check_global_access_intern(i._a));
			ASSERT(stack.check_reg_intern(i._b));

			globals[i._a] = regs[i._b];
			break;
		}


		//////////////////////////////////////////		ACCESS LOCALS


		case bc_opcode::k_copy_reg_intern: {
			ASSERT(stack.check_reg_intern(i._a));
			ASSERT(stack.check_reg_intern(i._b));

			regs[i._a] = regs[i._b];
			break;
		}
		case bc_opcode::k_copy_reg_obj: {
			ASSERT(stack.check_reg_obj(i._a));
			ASSERT(stack.check_reg_obj(i._b));

			bc_value_t::release_ext_pod(regs[i._a]);
			const auto& new_value_pod = regs[i._b];
			regs[i._a] = new_value_pod;
			new_value_pod._ext->_rc++;
			break;
		}


		//////////////////////////////////////////		STACK


		case bc_opcode::k_return: {
			bool is_ext = frame_ptr->_exts[i._a];
			ASSERT(
				(is_ext && stack.check_reg_obj(i._a))
				|| (!is_ext && stack.check_reg_intern(i._a))
			);

#if DEBUG
			return { true, bc_value_t(frame_ptr->_symbols[i._a].second._value_type, regs[i._a], is_ext) };
#else
			return { true, bc_value_t(regs[i._a], is_ext) };
#endif
		}

		case bc_opcode::k_stop: {
			return { false, bc_value_t::make_undefined() };
		}

		case bc_opcode::k_push_frame_ptr: {
			ASSERT(vm.check_invariant());
			ASSERT((stack._stack_size + k_frame_overhead) < stack._allocated_count)

			stack._entries[stack._stack_size + 0]._int = static_cast<int>(stack._current_frame_entry_ptr - &stack._entries[0]);
			stack._entries[stack._stack_size + 1]._frame_ptr = frame_ptr;
			stack._stack_size += k_frame_overhead;
#if DEBUG
			stack._debug_types.push_back(typeid_t::make_int());
			stack._debug_types.push_back(typeid_t::make_void());
#endif
			ASSERT(vm.check_invariant());
			break;
		}

		case bc_opcode::k_pop_frame_ptr: {
			ASSERT(vm.check_invariant());
			ASSERT(stack._stack_size >= k_frame_overhead);

			const auto frame_pos = stack._entries[stack._stack_size - k_frame_overhead + 0]._int;
			frame_ptr = stack._entries[stack._stack_size - k_frame_overhead + 1]._frame_ptr;
			stack._stack_size -= k_frame_overhead;

#if DEBUG
			stack._debug_types.pop_back();
			stack._debug_types.pop_back();
#endif

			regs = &stack._entries[frame_pos];

			//??? do we need stack._current_frame_ptr, stack._current_frame_pos? Only use local variables to track these?
			stack._current_frame_ptr = frame_ptr;
			stack._current_frame_entry_ptr = regs;

			ASSERT(frame_ptr == stack._current_frame_ptr);
			ASSERT(regs == stack._current_frame_entry_ptr);
			ASSERT(vm.check_invariant());
			break;
		}

		case bc_opcode::k_push_intern: {
			ASSERT(stack.check_reg_intern(i._a));
#if DEBUG
			const auto debug_type = stack._debug_types[stack.get_current_frame_start() + i._a];
#endif

			stack._entries[stack._stack_size] = regs[i._a];
			stack._stack_size++;
#if DEBUG
			stack._debug_types.push_back(debug_type);
#endif
			ASSERT(stack.check_invariant());
			break;
		}
		case bc_opcode::k_push_obj: {
			ASSERT(stack.check_reg_obj(i._a));

#if DEBUG
			const auto debug_type = stack._debug_types[stack.get_current_frame_start() + i._a];
#endif

			const auto& new_value_pod = regs[i._a];
			new_value_pod._ext->_rc++;
			stack._entries[stack._stack_size] = new_value_pod;
			stack._stack_size++;
#if DEBUG
			stack._debug_types.push_back(debug_type);
#endif
			break;
		}

		case bc_opcode::k_popn: {
			ASSERT(vm.check_invariant());

			const uint32_t n = i._a;
			const uint32_t extbits = i._b;

			ASSERT(stack._stack_size >= n);
			ASSERT(n >= 0);
			ASSERT(n <= 32);

			uint32_t bits = extbits;
			int pos = static_cast<int>(stack._stack_size) - 1;
			for(int m = 0 ; m < n ; m++){
				bool ext = (bits & 1) ? true : false;

				ASSERT(is_encoded_as_ext(stack._debug_types.back()) == ext);
	#if DEBUG
				stack._debug_types.pop_back();
	#endif
				if(ext){
					bc_value_t::release_ext_pod(stack._entries[pos]);
				}
				pos--;
				bits = bits >> 1;
			}
			stack._stack_size -= n;

			ASSERT(vm.check_invariant());
			break;
		}


		//////////////////////////////////////////		BRANCHING


		case bc_opcode::k_branch_false_bool: {
			ASSERT(stack.check_reg_bool(i._a));

			//	Notice that pc will be incremented too, hence the - 1.
			pc = regs[i._a]._bool ? pc : pc + i._b - 1;
			break;
		}
		case bc_opcode::k_branch_true_bool: {
			ASSERT(stack.check_reg_bool(i._a));

			//	Notice that pc will be incremented too, hence the - 1.
			pc = regs[i._a]._bool ? pc + i._b - 1: pc;
			break;
		}
		case bc_opcode::k_branch_zero_int: {
			ASSERT(stack.check_reg_int(i._a));

			//	Notice that pc will be incremented too, hence the - 1.
			pc = regs[i._a]._int == 0 ? pc + i._b - 1 : pc;
			break;
		}
		case bc_opcode::k_branch_notzero_int: {
			ASSERT(stack.check_reg_int(i._a));

			//	Notice that pc will be incremented too, hence the - 1.
			pc = regs[i._a]._int == 0 ? pc : pc + i._b - 1;
			break;
		}
		case bc_opcode::k_branch_smaller_int: {
			ASSERT(stack.check_reg_int(i._a));
			ASSERT(stack.check_reg_int(i._b));

			//	Notice that pc will be incremented too, hence the - 1.
			pc = regs[i._a]._int < regs[i._b]._int ? pc + i._c - 1 : pc;
			break;
		}
		case bc_opcode::k_branch_smaller_or_equal_int: {
			ASSERT(stack.check_reg_int(i._a));
			ASSERT(stack.check_reg_int(i._b));

			//	Notice that pc will be incremented too, hence the - 1.
			pc = regs[i._a]._int <= regs[i._b]._int ? pc + i._c - 1 : pc;
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
			ASSERT(vm.check_invariant());
			ASSERT(stack.check_reg_any(i._a));
			ASSERT(stack.check_reg_struct(i._b));

			const auto& value_pod = regs[i._b]._ext->_struct_members[i._c]._pod;
			bool ext = frame_ptr->_exts[i._a];
			if(ext){
				bc_value_t::release_ext_pod(regs[i._a]);
				value_pod._ext->_rc++;
			}
			regs[i._a] = value_pod;
			ASSERT(vm.check_invariant());
			break;
		}

		//??? Better to return uint8 or int than a new string.
		case bc_opcode::k_lookup_element_string: {
			ASSERT(vm.check_invariant());
			ASSERT(stack.check_reg_string(i._a));
			ASSERT(stack.check_reg_string(i._b));
			ASSERT(stack.check_reg_int(i._c));

			const auto& s = regs[i._b]._ext->_string;
			const auto lookup_index = regs[i._c]._int;

			if(lookup_index < 0 || lookup_index >= s.size()){
				throw std::runtime_error("Lookup in string: out of bounds.");
			}
			else{
				const char ch = s[lookup_index];
				const auto str2 = string(1, ch);
				const auto value2 = bc_value_t::make_string(str2);

				value2._pod._ext->_rc++;
				bc_value_t::release_ext_pod(regs[i._a]);
				regs[i._a] = value2._pod;
			}
			ASSERT(vm.check_invariant());
			break;
		}

		//	??? Simple JSON-values should not require ext. null, int, bool, empty object, empty array.
		case bc_opcode::k_lookup_element_json_value: {
			ASSERT(vm.check_invariant());
			ASSERT(stack.check_reg_json(i._a));
			ASSERT(stack.check_reg_json(i._b));

			// reg c points to different types depending on the runtime-type of the json_value.
			ASSERT(stack.check_reg_any(i._c));

			const auto& parent_json_value = regs[i._b]._ext->_json_value;

			if(parent_json_value->is_object()){
				ASSERT(stack.check_reg_string(i._c));

				const auto& lookup_key = regs[i._c]._ext->_string;

				//	get_object_element() throws if key can't be found.
				const auto& value = parent_json_value->get_object_element(lookup_key);

				//??? no need to create full bc_value_t here! We only need pod.
				const auto value2 = bc_value_t::make_json_value(value);

				value2._pod._ext->_rc++;
				bc_value_t::release_ext_pod(regs[i._a]);
				regs[i._a] = value2._pod;
			}
			else if(parent_json_value->is_array()){
				ASSERT(stack.check_reg_int(i._c));

				const auto lookup_index = regs[i._c]._int;
				if(lookup_index < 0 || lookup_index >= parent_json_value->get_array_size()){
					throw std::runtime_error("Lookup in json_value array: out of bounds.");
				}
				else{
					const auto& value = parent_json_value->get_array_n(lookup_index);

					//??? value2 will soon go out of scope - avoid creating bc_value_t all together.
					//??? no need to create full bc_value_t here! We only need pod.
					const auto value2 = bc_value_t::make_json_value(value);

					value2._pod._ext->_rc++;
					bc_value_t::release_ext_pod(regs[i._a]);
					regs[i._a] = value2._pod;
				}
			}
			else{
				throw std::runtime_error("Lookup using [] on json_value only works on objects and arrays.");
			}
			ASSERT(vm.check_invariant());
			break;
		}

		//??? Make obj/intern version.
		case bc_opcode::k_lookup_element_vector: {
			ASSERT(vm.check_invariant());
			ASSERT(stack.check_reg_any(i._a));
			ASSERT(stack.check_reg_vector(i._b));
			ASSERT(stack.check_reg_int(i._c));

//			const auto& element_type = frame_ptr->_symbols[i._b].second._value_type.get_vector_element_type();

			const auto* vec = &regs[i._b]._ext->_vector_elements;
			const auto lookup_index = regs[i._c]._int;
			if(lookup_index < 0 || lookup_index >= (*vec).size()){
				throw std::runtime_error("Lookup in vector: out of bounds.");
			}
			else{
				const bc_value_t& value = (*vec)[lookup_index];

				//	Always use symbol table as TRUTH about the register's type. ??? fix all code.
				ASSERT(value._type == frame_ptr->_symbols[i._a].second._value_type);

				bool is_ext = frame_ptr->_exts[i._a];
				if(is_ext){
					bc_value_t::release_ext_pod(regs[i._a]);
					value._pod._ext->_rc++;
				}
				regs[i._a] = value._pod;
			}
			ASSERT(vm.check_invariant());
			break;
		}

		//??? Make obj/intern version.
		case bc_opcode::k_lookup_element_dict: {
			ASSERT(stack.check_reg_any(i._a));
			ASSERT(stack.check_reg_dict(i._b));
			ASSERT(stack.check_reg_string(i._c));

			const auto& entries = regs[i._b]._ext->_dict_entries;
			const auto& lookup_key = regs[i._c]._ext->_string;
			const auto found_ptr = entries.find(lookup_key);
			if(found_ptr == nullptr){
				throw std::runtime_error("Lookup in dict: key not found.");
			}
			else{
				const bc_value_t& value = *found_ptr;

				bool is_ext = frame_ptr->_exts[i._a];
				if(is_ext){
					bc_value_t::release_ext_pod(regs[i._a]);
					value._pod._ext->_rc++;
				}
				regs[i._a] = value._pod;
			}
			break;
		}

		/*
			??? Make stub bc_frame_t for each host function to make call conventions same as Floyd functions.
		*/

		//	Notice: host calls and floyd calls have the same type -- we cannot detect host calls until we have a callee value.
		case bc_opcode::k_call: {
			ASSERT(vm.check_invariant());
			ASSERT(stack.check_reg_function(i._b));

			const int function_id = regs[i._b]._function_id;
			const int callee_arg_count = i._c;
			ASSERT(function_id >= 0 && function_id < vm._imm->_program._function_defs.size())

			const auto& function_def = vm._imm->_program._function_defs[function_id];
			const int function_def_dynamic_arg_count = function_def._dyn_arg_count;
			const auto& function_return_type = function_def._function_type.get_function_return();

			//	_e[...] contains first callee, then each argument.
			//	We need to examine the callee, since we support magic argument lists of varying size.

			ASSERT(function_def._args.size() == callee_arg_count);

			if(function_def._host_function_id != 0){
				const auto& host_function = vm._imm->_host_functions.at(function_def._host_function_id);

				const int arg0_stack_pos = stack.size() - (function_def_dynamic_arg_count + callee_arg_count);
				int stack_pos = arg0_stack_pos;

				//	Notice that dynamic functions will have each DYN argument with a leading itype as an extra argument.
				const auto function_def_arg_count = function_def._args.size();
				std::vector<bc_value_t> arg_values;
				for(int a = 0 ; a < function_def_arg_count ; a++){
					const auto& func_arg_type = function_def._args[a]._type;
					if(func_arg_type.is_internal_dynamic()){
						const auto arg_itype = stack.load_intq(stack_pos);
						const auto& arg_type = lookup_full_type(vm, static_cast<int16_t>(arg_itype));
						const auto arg_value = stack.load_value(stack_pos + 1, arg_type);
						arg_values.push_back(arg_value);
						stack_pos += k_frame_overhead;
					}
					else{
						const auto arg_value = stack.load_value(stack_pos + 0, func_arg_type);
						arg_values.push_back(arg_value);
						stack_pos++;
					}
				}

				const auto& result = (host_function)(vm, &arg_values[0], static_cast<int>(arg_values.size()));
				const auto bc_result = result;

				if(function_return_type.is_void() == true){
				}
				else if(function_return_type.is_internal_dynamic()){
					stack.write_register(i._a, bc_result);
				}
				else{
					stack.write_register(i._a, bc_result);
				}
			}
			else{
				ASSERT(function_def_dynamic_arg_count == 0);

				//	We need to remember the global pos where to store return value, since we're switching frame to call function.
				int result_reg_pos = static_cast<int>(stack._current_frame_entry_ptr - &stack._entries[0]) + i._a;

				stack.open_frame(*function_def._frame_ptr, callee_arg_count);
				const auto& result = execute_instructions(vm, function_def._frame_ptr->_instrs2);
				stack.close_frame(*function_def._frame_ptr);

				//	Update our cached pointers.
				frame_ptr = stack._current_frame_ptr;
				regs = stack._current_frame_entry_ptr;

				ASSERT(result.first);
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

			ASSERT(frame_ptr == stack._current_frame_ptr);
			ASSERT(regs == stack._current_frame_entry_ptr);
			ASSERT(vm.check_invariant());
			break;
		}

		case bc_opcode::k_new_1: {
			ASSERT(stack.check_reg(i._a));

			const auto dest_reg = i._a;
			const auto target_itype = i._b;
			const auto source_itype = i._c;
			const auto& target_type = lookup_full_type(vm, target_itype);
			QUARK_ASSERT(target_type.is_vector() == false && target_type.is_dict() == false && target_type.is_struct() == false);
			execute_new_1(vm, dest_reg, target_itype, source_itype);
			break;
		}

		case bc_opcode::k_new_vector: {
			ASSERT(stack.check_reg_vector(i._a));
			ASSERT(i._b >= 0);
			ASSERT(i._c >= 0);

			const auto dest_reg = i._a;
			const auto target_itype = i._b;
			const auto arg_count = i._c;
			const auto& target_type = lookup_full_type(vm, target_itype);
			QUARK_ASSERT(target_type.is_vector());
			execute_new_vector(vm, dest_reg, target_itype, arg_count);
			break;
		}

		case bc_opcode::k_new_vector_uint64: {
			ASSERT(vm.check_invariant());
			ASSERT(stack.check_reg_vector(i._a));
			ASSERT(i._c >= 0);

			const auto dest_reg = i._a;
			const auto arg_count = i._c;

			const int arg0_stack_pos = vm._stack.size() - arg_count;
			immer::vector<uint64_t> elements2;
			for(int a = 0 ; a < arg_count ; a++){
				const auto pos = arg0_stack_pos + a;
				elements2 = elements2.push_back(stack._entries[pos]._value64);
			}

			const auto& type = frame_ptr->_symbols[i._a].second._value_type;
			const auto& element_type = type.get_vector_element_type();

			const auto result = make_vector64_value(element_type, elements2);
			vm._stack.write_register_obj(dest_reg, result);

			ASSERT(vm.check_invariant());
			break;
		}

		case bc_opcode::k_new_dict: {
			const auto dest_reg = i._a;
			const auto target_itype = i._b;
			const auto arg_count = i._c;
			const auto& target_type = lookup_full_type(vm, target_itype);
			QUARK_ASSERT(target_type.is_dict());
			execute_new_dict(vm, dest_reg, target_itype, arg_count);
			break;
		}
		case bc_opcode::k_new_struct: {
			const auto dest_reg = i._a;
			const auto target_itype = i._b;
			const auto arg_count = i._c;
			const auto& target_type = lookup_full_type(vm, target_itype);
			QUARK_ASSERT(target_type.is_struct());
			execute_new_struct(vm, dest_reg, target_itype, arg_count);
			break;
		}


		//////////////////////////////		COMPARISON

		//??? inline compare_value_true_deep().

		case bc_opcode::k_comparison_smaller_or_equal: {
			ASSERT(stack.check_reg_bool(i._a));
			ASSERT(stack.check_reg_any(i._b));
			ASSERT(stack.check_reg_any(i._c));

			const auto& type = frame_ptr->_symbols[i._b].second._value_type;
			ASSERT(type.is_int() == false);

			const auto left = stack.read_register(i._b);
			const auto right = stack.read_register(i._c);
			long diff = bc_compare_value_true_deep(left, right, type);

			regs[i._a]._bool = diff <= 0;
			break;
		}
		case bc_opcode::k_comparison_smaller_or_equal_int: {
			ASSERT(stack.check_reg_bool(i._a));
			ASSERT(stack.check_reg_int(i._b));
			ASSERT(stack.check_reg_int(i._c));

			regs[i._a]._bool = regs[i._b]._int <= regs[i._c]._int;
			break;
		}

		case bc_opcode::k_comparison_smaller: {
			ASSERT(stack.check_reg_bool(i._a));
			ASSERT(stack.check_reg_any(i._b));
			ASSERT(stack.check_reg_any(i._c));

			const auto& type = frame_ptr->_symbols[i._b].second._value_type;
			ASSERT(type.is_int() == false);
			const auto left = stack.read_register(i._b);
			const auto right = stack.read_register(i._c);
			long diff = bc_compare_value_true_deep(left, right, type);

			regs[i._a]._bool = diff < 0;
			break;
		}
		case bc_opcode::k_comparison_smaller_int: ASSERT(stack.check_reg_bool(i._a)); ASSERT(stack.check_reg_int(i._b)); ASSERT(stack.check_reg_int(i._c)); regs[i._a]._bool = regs[i._b]._int < regs[i._c]._int; break;

		case bc_opcode::k_logical_equal: {
			ASSERT(stack.check_reg_bool(i._a));
			ASSERT(stack.check_reg_any(i._b));
			ASSERT(stack.check_reg_any(i._c));

			const auto& type = frame_ptr->_symbols[i._b].second._value_type;
			ASSERT(type.is_int() == false);
			const auto left = stack.read_register(i._b);
			const auto right = stack.read_register(i._c);
			long diff = bc_compare_value_true_deep(left, right, type);

			regs[i._a]._bool = diff == 0;
			break;
		}
		case bc_opcode::k_logical_equal_int: {
			ASSERT(stack.check_reg_bool(i._a));
			ASSERT(stack.check_reg_int(i._b));
			ASSERT(stack.check_reg_int(i._c));

			regs[i._a]._bool = regs[i._b]._int == regs[i._c]._int;
			break;
		}

		case bc_opcode::k_logical_nonequal: {
			ASSERT(stack.check_reg_bool(i._a));
			ASSERT(stack.check_reg_any(i._b));
			ASSERT(stack.check_reg_any(i._c));

			const auto& type = frame_ptr->_symbols[i._b].second._value_type;
			ASSERT(type.is_int() == false);
			const auto left = stack.read_register(i._b);
			const auto right = stack.read_register(i._c);
			long diff = bc_compare_value_true_deep(left, right, type);

			regs[i._a]._bool = diff != 0;
			break;
		}
		case bc_opcode::k_logical_nonequal_int: {
			ASSERT(stack.check_reg_bool(i._a));
			ASSERT(stack.check_reg_int(i._b));
			ASSERT(stack.check_reg_int(i._c));

			regs[i._a]._bool = regs[i._b]._int != regs[i._c]._int;
			break;
		}


		//////////////////////////////		ARITHMETICS


		//??? Replace by a | b opcode.
		case bc_opcode::k_add_bool: {
			ASSERT(stack.check_reg_bool(i._a));
			ASSERT(stack.check_reg_bool(i._b));
			ASSERT(stack.check_reg_bool(i._c));

			regs[i._a]._bool = regs[i._b]._bool + regs[i._c]._bool;
			break;
		}
		case bc_opcode::k_add_int: {
			ASSERT(stack.check_reg_int(i._a));
			ASSERT(stack.check_reg_int(i._b));
			ASSERT(stack.check_reg_int(i._c));

			regs[i._a]._int = regs[i._b]._int + regs[i._c]._int;
			break;
		}
		case bc_opcode::k_add_float: {
			ASSERT(stack.check_reg_float(i._a));
			ASSERT(stack.check_reg_float(i._b));
			ASSERT(stack.check_reg_float(i._c));

			regs[i._a]._float = regs[i._b]._float + regs[i._c]._float;
			break;
		}
		case bc_opcode::k_add_string: {
			ASSERT(stack.check_reg_string(i._a));
			ASSERT(stack.check_reg_string(i._b));
			ASSERT(stack.check_reg_string(i._c));

			//	??? No need to create bc_value_t here.
			const auto s = regs[i._b]._ext->_string + regs[i._c]._ext->_string;
			const auto value = bc_value_t::make_string(s);
			auto prev_copy = regs[i._a];
			value._pod._ext->_rc++;
			regs[i._a] = value._pod;
			bc_value_t::release_ext_pod(prev_copy);
			break;
		}

		//??? inline
		//??? Use itypes.
		case bc_opcode::k_add_vector: {
			ASSERT(stack.check_reg_vector(i._a));
			ASSERT(stack.check_reg_vector(i._b));
			ASSERT(stack.check_reg_vector(i._c));

			const auto& element_type = frame_ptr->_symbols[i._a].second._value_type.get_vector_element_type();

			//	Copy left into new vector.
			immer::vector<bc_value_t> elements2 = regs[i._b]._ext->_vector_elements;

			const auto& right_elements = regs[i._c]._ext->_vector_elements;
			for(const auto& e: right_elements){
				elements2 = elements2.push_back(e);
			}
			const auto& value2 = make_vector_value(element_type, elements2);
			stack.write_register_obj(i._a, value2);
			break;
		}

		case bc_opcode::k_subtract_float: {
			ASSERT(stack.check_reg_float(i._a));
			ASSERT(stack.check_reg_float(i._b));
			ASSERT(stack.check_reg_float(i._c));

			regs[i._a]._float = regs[i._b]._float - regs[i._c]._float;
			break;
		}
		case bc_opcode::k_subtract_int: {
			ASSERT(stack.check_reg_int(i._a));
			ASSERT(stack.check_reg_int(i._b));
			ASSERT(stack.check_reg_int(i._c));

			regs[i._a]._int = regs[i._b]._int - regs[i._c]._int;
			break;
		}
		case bc_opcode::k_multiply_float: {
			ASSERT(stack.check_reg_float(i._a));
			ASSERT(stack.check_reg_float(i._c));
			ASSERT(stack.check_reg_float(i._c));

			regs[i._a]._float = regs[i._b]._float * regs[i._c]._float;
			break;
		}
		case bc_opcode::k_multiply_int: {
			ASSERT(stack.check_reg_int(i._a));
			ASSERT(stack.check_reg_int(i._c));
			ASSERT(stack.check_reg_int(i._c));

			regs[i._a]._int = regs[i._b]._int * regs[i._c]._int;
			break;
		}
		case bc_opcode::k_divide_float: {
			ASSERT(stack.check_reg_float(i._a));
			ASSERT(stack.check_reg_float(i._b));
			ASSERT(stack.check_reg_float(i._c));

			const auto right = regs[i._c]._float;
			if(right == 0.0f){
				throw std::runtime_error("EEE_DIVIDE_BY_ZERO");
			}
			regs[i._a]._float = regs[i._b]._float / right;
			break;
		}
		case bc_opcode::k_divide_int: {
			ASSERT(stack.check_reg_int(i._a));
			ASSERT(stack.check_reg_int(i._b));
			ASSERT(stack.check_reg_int(i._c));

			const auto right = regs[i._c]._int;
			if(right == 0.0f){
				throw std::runtime_error("EEE_DIVIDE_BY_ZERO");
			}
			regs[i._a]._int = regs[i._b]._int / right;
			break;
		}
		case bc_opcode::k_remainder_int: {
			ASSERT(stack.check_reg_int(i._a));
			ASSERT(stack.check_reg_int(i._b));
			ASSERT(stack.check_reg_int(i._c));

			const auto right = regs[i._c]._int;
			if(right == 0){
				throw std::runtime_error("EEE_DIVIDE_BY_ZERO");
			}
			regs[i._a]._int = regs[i._b]._int % right;
			break;
		}


		//### Could be replaced by feature to convert any value to bool -- they use a generic comparison for && and ||
		case bc_opcode::k_logical_and_bool: {
			ASSERT(stack.check_reg_bool(i._a));
			ASSERT(stack.check_reg_bool(i._b));
			ASSERT(stack.check_reg_bool(i._c));

			regs[i._a]._bool = regs[i._b]._bool  && regs[i._c]._bool;
			break;
		}
		case bc_opcode::k_logical_and_int: {
			ASSERT(stack.check_reg_bool(i._a));
			ASSERT(stack.check_reg_int(i._b));
			ASSERT(stack.check_reg_int(i._c));

			regs[i._a]._bool = (regs[i._b]._int != 0) && (regs[i._c]._int != 0);
			break;
		}
		case bc_opcode::k_logical_and_float: {
			ASSERT(stack.check_reg_bool(i._a));
			ASSERT(stack.check_reg_float(i._b));
			ASSERT(stack.check_reg_float(i._c));

			regs[i._a]._bool = (regs[i._b]._float != 0) && (regs[i._c]._float != 0);
			break;
		}

		case bc_opcode::k_logical_or_bool: {
			ASSERT(stack.check_reg_bool(i._a));
			ASSERT(stack.check_reg_bool(i._b));
			ASSERT(stack.check_reg_bool(i._c));

			regs[i._a]._bool = regs[i._b]._bool || regs[i._c]._bool;
			break;
		}
		case bc_opcode::k_logical_or_int: {
			ASSERT(stack.check_reg_bool(i._a));
			ASSERT(stack.check_reg_int(i._b));
			ASSERT(stack.check_reg_int(i._c));

			regs[i._a]._bool = (regs[i._b]._int != 0) || (regs[i._c]._int != 0);
			break;
		}
		case bc_opcode::k_logical_or_float: {
			ASSERT(stack.check_reg_bool(i._a));
			ASSERT(stack.check_reg_float(i._b));
			ASSERT(stack.check_reg_float(i._c));

			regs[i._a]._bool = (regs[i._b]._float != 0.0f) || (regs[i._c]._float != 0.0f);
			break;
		}


		//////////////////////////////		NONE


		default:
			ASSERT(false);
			throw std::exception();
		}
		pc++;
	}
	return { false, bc_value_t::make_undefined() };
}


//////////////////////////////////////////		FUNCTIONS



std::shared_ptr<value_entry_t> find_global_symbol2(const interpreter_t& vm, const std::string& s){
	QUARK_ASSERT(vm.check_invariant());
	QUARK_ASSERT(s.size() > 0);

	const auto& symbols = vm._imm->_program._globals._symbols;
    const auto& it = std::find_if(
    	symbols.begin(),
    	symbols.end(),
    	[&s](const std::pair<std::string, bc_symbol_t>& e) { return e.first == s; }
	);
	if(it != symbols.end()){
		const auto index = static_cast<int>(it - symbols.begin());
		const auto pos = get_global_n_pos(index);
		QUARK_ASSERT(pos >= 0 && pos < vm._stack.size());

		const auto value_entry = value_entry_t{
			vm._stack.load_value(pos, it->second._value_type),
			it->first,
			it->second,
			static_cast<int>(index)
		};
		return make_shared<value_entry_t>(value_entry);
	}
	else{
		return nullptr;
	}
}


std::string opcode_to_string(bc_opcode opcode){
	return k_opcode_info.at(opcode)._as_text;
}

json_t interpreter_to_json(const interpreter_t& vm){
	vector<json_t> callstack;
	QUARK_ASSERT(vm.check_invariant());

	const auto stack = vm._stack.stack_to_json();

	return json_t::make_object({
		{ "ast", bcprogram_to_json(vm._imm->_program) },
		{ "callstack", stack }
	});
}

std::vector<json_t> bc_symbols_to_json(const std::vector<std::pair<std::string, bc_symbol_t>>& symbols){
	std::vector<json_t> r;
	int symbol_index = 0;
	for(const auto& e: symbols){
		const auto& symbol = e.second;
		const auto symbol_type_str = symbol._symbol_type == bc_symbol_t::immutable_local ? "immutable_local" : "mutable_local";

		if(symbol._const_value._type.is_undefined() == false){
			const auto e2 = json_t::make_array({
				symbol_index,
				e.first,
				"CONST",
				bcvalue_and_type_to_json(symbol._const_value)
			});
			r.push_back(e2);
		}
		else{
			const auto e2 = json_t::make_array({
				symbol_index,
				e.first,
				"LOCAL",
				json_t::make_object({
					{ "value_type", typeid_to_ast_json(symbol._value_type, json_tags::k_tag_resolve_state)._value },
					{ "type", symbol_type_str }
				})
			});
			r.push_back(e2);
		}

		symbol_index++;
	}
	return r;
}

json_t frame_to_json(const bc_frame_t& frame){
	vector<json_t> exts;
	for(int i = 0 ; i < frame._exts.size() ; i++){
		const auto& e = frame._exts[i];
		exts.push_back(json_t::make_array({
			json_t(i),
			json_t(e)
		}));
	}

	vector<json_t> instructions;
	int pc = 0;
	for(const auto& e: frame._instrs2){
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
		{ "symbols", json_t::make_array(bc_symbols_to_json(frame._symbols)) },
		{ "instructions", json_t::make_array(instructions) },
		{ "exts", json_t::make_array(exts) }
	});
}

json_t types_to_json(const std::vector<const typeid_t>& types){
	vector<json_t> r;
	int id = 0;
	for(const auto& e: types){
		const auto i = json_t::make_array({
			id,
			typeid_to_ast_json(e, json_tags::k_plain)._value
		});
		r.push_back(i);
		id++;
	}
	return json_t::make_array(r);
}

json_t functiondef_to_json(const bc_function_definition_t& def){
	return json_t::make_array({
		json_t(typeid_to_compact_string(def._function_type)),
		members_to_json(def._args),
		def._frame_ptr ? frame_to_json(*def._frame_ptr) : json_t(),
		json_t(def._host_function_id)
	});
}

json_t bcprogram_to_json(const bc_program_t& program){
	vector<json_t> callstack;
/*
	for(int env_index = 0 ; env_index < vm._call_stack.size() ; env_index++){
		const auto e = &vm._call_stack[vm._call_stack.size() - 1 - env_index];

		const auto local_end = (env_index == (vm._call_stack.size() - 1)) ? vm._value_stack.size() : vm._call_stack[vm._call_stack.size() - 1 - env_index + 1]._values_offset;
		const auto local_count = local_end - e->_values_offset;
		std::vector<json_t> values;
		for(int local_index = 0 ; local_index < local_count ; local_index++){
			const auto& v = vm._value_stack[e->_values_offset + local_index];
			const auto& a = value_and_type_to_ast_json(v);
			values.push_back(a._value);
		}

		const auto& env = json_t::make_object({
			{ "values", values }
		});
		callstack.push_back(env);
	}
*/
	vector<json_t> function_defs;
	for(int i = 0 ; i < program._function_defs.size() ; i++){
		const auto& function_def = program._function_defs[i];
		function_defs.push_back(json_t::make_array({
			json_t(i),
			functiondef_to_json(function_def)
		}));
	}

	return json_t::make_object({
		{ "globals", frame_to_json(program._globals) },
		{ "types", types_to_json(program._types) },
		{ "function_defs", json_t::make_array(function_defs) }
//		{ "callstack", json_t::make_array(callstack) }
	});
}

}	//	floyd
