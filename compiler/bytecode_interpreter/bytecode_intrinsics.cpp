//
//  bytecode_intrinsics.cpp
//  Floyd
//
//  Created by Marcus Zetterquist on 2018-02-23.
//  Copyright © 2018 Marcus Zetterquist. All rights reserved.
//

#include "bytecode_intrinsics.h"

#include "floyd_runtime.h"
#include "floyd_intrinsics.h"


namespace floyd {

static func_link_t make_intr(const intrinsic_signature_t& sign, void* f){
	return func_link_t {
		std::string() + "bc-intrinsics-impl:" + sign.name,
		module_symbol_t(sign.name),
		sign._function_type,
		func_link_t::emachine::k_native__floydcc,
		(void*)f,
		{},
		nullptr
	};
}

std::vector<func_link_t> bc_get_intrinsics(types_t& types){
	QUARK_ASSERT(types.check_invariant());

	return {
		make_intr(make_assert_signature(types), (void*)intrinsic__assert),
		make_intr(make_to_string_signature(types), (void*)intrinsic__to_string),
		make_intr(make_to_pretty_string_signature(types), (void*)intrinsic__to_pretty_string),
		make_intr(make_typeof_signature(types), (void*)intrinsic__typeof),

		make_intr(make_update_signature(types), (void*)intrinsic__update),

		//	size() is translated to BC opcode
		make_intr(make_size_signature(types), (void*)intrinsic__size),
		make_intr(make_find_signature(types), (void*)intrinsic__find),
		make_intr(make_exists_signature(types), (void*)intrinsic__exists),
		make_intr(make_erase_signature(types), (void*)intrinsic__erase),
		make_intr(make_get_keys_signature(types), (void*)intrinsic__get_keys),

		//	push_back() is translated to BC opcode
		make_intr(make_push_back_signature(types), (void*)intrinsic__push_back),
		make_intr(make_subset_signature(types), (void*)intrinsic__subset),
		make_intr(make_replace_signature(types), (void*)intrinsic__replace),

		make_intr(make_get_json_type_signature(types), (void*)intrinsic__get_json_type),
		make_intr(make_generate_json_script_signature(types), (void*)intrinsic__generate_json_script),
		make_intr(make_parse_json_script_signature(types), (void*)intrinsic__parse_json_script),
		make_intr(make_to_json_signature(types), (void*)intrinsic__to_json),
		make_intr(make_from_json_signature(types), (void*)intrinsic__from_json),

		make_intr(make_map_signature(types), (void*)intrinsic__map),
		make_intr(make_filter_signature(types), (void*)intrinsic__filter),
		make_intr(make_reduce_signature(types), (void*)intrinsic__reduce),
		make_intr(make_map_dag_signature(types), (void*)intrinsic__map_dag),

		make_intr(make_stable_sort_signature(types), (void*)intrinsic__stable_sort),

		make_intr(make_print_signature(types), (void*)intrinsic__print),
		make_intr(make_send_signature(types), (void*)intrinsic__send),
		make_intr(make_exit_signature(types), (void*)intrinsic__exit),

		make_intr(make_bw_not_signature(types), (void*)intrinsic__bw_not),
		make_intr(make_bw_and_signature(types), (void*)intrinsic__bw_and),
		make_intr(make_bw_or_signature(types), (void*)intrinsic__bw_or),
		make_intr(make_bw_xor_signature(types), (void*)intrinsic__bw_xor),
		make_intr(make_bw_shift_left_signature(types), (void*)intrinsic__bw_shift_left),
		make_intr(make_bw_shift_right_signature(types), (void*)intrinsic__bw_shift_right),
		make_intr(make_bw_shift_right_arithmetic_signature(types), (void*)intrinsic__bw_shift_right_arithmetic)
	};
}

}	// floyd
