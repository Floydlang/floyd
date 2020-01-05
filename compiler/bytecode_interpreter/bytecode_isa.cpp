//
//  bytecode_isa.cpp
//  floyd
//
//  Created by Marcus Zetterquist on 2020-01-05.
//  Copyright © 2020 Marcus Zetterquist. All rights reserved.
//

#include "bytecode_isa.h"

#include "statement.h"
#include "quark.h"
#include "value_backend.h"


namespace floyd {


std::string opcode_to_string(bc_opcode opcode){
	return k_opcode_info.at(opcode)._as_text;
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
		quark::throw_defective_request();
	}
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





std::vector<json_t> bc_symbols_to_json(value_backend_t& backend, const symbol_table_t& symbols){
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


} //	floyd
