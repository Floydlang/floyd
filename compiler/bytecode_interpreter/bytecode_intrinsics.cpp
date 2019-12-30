//
//  bytecode_intrinsics.cpp
//  Floyd
//
//  Created by Marcus Zetterquist on 2018-02-23.
//  Copyright © 2018 Marcus Zetterquist. All rights reserved.
//

#include "bytecode_intrinsics.h"

#include "json_support.h"

#include "text_parser.h"
#include "ast_value.h"
#include "floyd_interpreter.h"
#include "floyd_runtime.h"
#include "value_features.h"
#include "floyd_intrinsics.h"

#include <algorithm>


namespace floyd {

static const bool k_trace = false;



/////////////////////////////////////////		PURE -- map_dag2()




/////////////////////////////////////////		PURE -- filter()



//	[E] filter([E] elements, func bool (E e, C context) f, C context)
static rt_value_t bc_intrinsic__filter(interpreter_t& vm, const rt_value_t args[], int arg_count){
	QUARK_ASSERT(vm.check_invariant());
	QUARK_ASSERT(arg_count == 3);
//	QUARK_ASSERT(check_filter_func_type(args[0]._type, args[1]._type, args[2]._type));

	auto& backend = vm._backend;
	const auto& elements = args[0];
	const auto& f = args[1];
	const auto& e_type = peek2(backend.types, elements._type).get_vector_element_type(backend.types);
	const auto& context = args[2];

	const auto input_vec = get_vector_elements(backend, elements);
	immer::vector<rt_value_t> vec2;

	for(const auto& e: input_vec){
		const rt_value_t f_args[] = { e, context };
		const auto result1 = call_function_bc(vm, f, f_args, 2);
		QUARK_ASSERT(peek2(backend.types, result1._type).is_bool());

		if(result1.get_bool_value()){
			vec2 = vec2.push_back(e);
		}
	}

	const auto result = make_vector_value(backend, e_type, vec2);

	if(k_trace && false){
		const auto debug = value_and_type_to_json(backend.types, rt_to_value(backend, result));
		QUARK_TRACE(json_to_pretty_string(debug));
	}

	return result;
}



/////////////////////////////////////////		PURE -- reduce()



//	R reduce([E] elements, R accumulator_init, func R (R accumulator, E element, C context) f, C context)
static rt_value_t bc_intrinsic__reduce(interpreter_t& vm, const rt_value_t args[], int arg_count){
	QUARK_ASSERT(vm.check_invariant());
	QUARK_ASSERT(arg_count == 4);
//	QUARK_ASSERT(check_reduce_func_type(args[0]._type, args[1]._type, args[2]._type, args[3]._type));

	auto& backend = vm._backend;
	const auto& elements = args[0];
	const auto& init = args[1];
	const auto& f = args[2];
	const auto& context = args[3];
	const auto input_vec = get_vector_elements(backend, elements);

	rt_value_t acc = init;
	for(const auto& e: input_vec){
		const rt_value_t f_args[] = { acc, e, context };
		const auto result1 = call_function_bc(vm, f, f_args, 3);
		acc = result1;
	}

	const auto result = acc;

if(k_trace && false){
	const auto debug = value_and_type_to_json(backend.types, rt_to_value(backend, result));
	QUARK_TRACE(json_to_pretty_string(debug));
}

	return result;
}




/////////////////////////////////////////		PURE -- stable_sort()


//	[T] stable_sort([T] elements, bool less(T left, T right, C context), C context)
static rt_value_t bc_intrinsic__stable_sort(interpreter_t& vm, const rt_value_t args[], int arg_count){
	QUARK_ASSERT(vm.check_invariant());
	QUARK_ASSERT(arg_count == 3);
//	QUARK_ASSERT(check_stable_sort_func_type(args[0]._type, args[1]._type, args[2]._type));

	auto& backend = vm._backend;
	const auto& elements = args[0];
	const auto& f = args[1];
	const auto& e_type = peek2(backend.types, elements._type).get_vector_element_type(backend.types);
	const auto& context = args[2];

	const auto input_vec = get_vector_elements(backend, elements);
	std::vector<rt_value_t> mutate_inplace_elements(input_vec.begin(), input_vec.end());

	struct sort_functor_r {
		bool operator() (const rt_value_t &a, const rt_value_t &b) {
			const rt_value_t f_args[] = { a, b, context };
			const auto result1 = call_function_bc(vm, f, f_args, 3);
			QUARK_ASSERT(peek2(backend.types, result1._type).is_bool());
			return result1.get_bool_value();
		}

		interpreter_t& vm;
		rt_value_t context;
		rt_value_t f;
		const value_backend_t& backend;
	};

	const sort_functor_r sort_functor { vm, context, f, backend };
	std::stable_sort(mutate_inplace_elements.begin(), mutate_inplace_elements.end(), sort_functor);

	const auto mutate_inplace_elements2 = immer::vector<rt_value_t>(mutate_inplace_elements.begin(), mutate_inplace_elements.end());
	const auto result = make_vector_value(backend, e_type, mutate_inplace_elements2);

if(k_trace && false){
	const auto debug = value_and_type_to_json(backend.types, rt_to_value(backend, result));
	QUARK_TRACE(json_to_pretty_string(debug));
}

	return result;
}




/////////////////////////////////////////		bc_get_intrinsics()


static func_link_t make_intr(const intrinsic_signature_t& sign, BC_NATIVE_FUNCTION_PTR f){
	return func_link_t {
		std::string() + "bc-intrinsics-impl:" + sign.name,
		module_symbol_t(sign.name),
		sign._function_type,
		func_link_t::emachine::k_native,
		(void*)f,
		{},
		nullptr
	};
}

static func_link_t make_intr2(const intrinsic_signature_t& sign, void* f){
	return func_link_t {
		std::string() + "bc-intrinsics-impl:" + sign.name,
		module_symbol_t(sign.name),
		sign._function_type,
		func_link_t::emachine::k_native2,
		(void*)f,
		{},
		nullptr
	};
}

std::vector<func_link_t> bc_get_intrinsics(types_t& types){
	QUARK_ASSERT(types.check_invariant());

	return {
		make_intr2(make_assert_signature(types), (void*)intrinsic__assert),
		make_intr2(make_to_string_signature(types), (void*)intrinsic__to_string),
		make_intr2(make_to_pretty_string_signature(types), (void*)intrinsic__to_pretty_string),
		make_intr2(make_typeof_signature(types), (void*)intrinsic__typeof),


		make_intr2(make_update_signature(types), (void*)intrinsic__update),

		//	size() is translated to BC opcode
		make_intr2(make_size_signature(types), (void*)intrinsic__size),
		make_intr2(make_find_signature(types), (void*)intrinsic__find),
		make_intr2(make_exists_signature(types), (void*)intrinsic__exists),
		make_intr2(make_erase_signature(types), (void*)intrinsic__erase),
		make_intr2(make_get_keys_signature(types), (void*)intrinsic__get_keys),

		//	push_back() is translated to BC opcode
		make_intr2(make_push_back_signature(types), (void*)intrinsic__push_back),
		make_intr2(make_subset_signature(types), (void*)intrinsic__subset),
		make_intr2(make_replace_signature(types), (void*)intrinsic__replace),


		make_intr2(make_get_json_type_signature(types), (void*)intrinsic__get_json_type),
		make_intr2(make_generate_json_script_signature(types), (void*)intrinsic__generate_json_script),
		make_intr2(make_parse_json_script_signature(types), (void*)intrinsic__parse_json_script),
		make_intr2(make_to_json_signature(types), (void*)intrinsic__to_json),
		make_intr2(make_from_json_signature(types), (void*)intrinsic__from_json),


		make_intr2(make_map_signature(types), (void*)intrinsic__map),
		make_intr(make_filter_signature(types), bc_intrinsic__filter),
		make_intr(make_reduce_signature(types), bc_intrinsic__reduce),
		make_intr2(make_map_dag_signature(types), (void*)intrinsic__map_dag),

		make_intr(make_stable_sort_signature(types), bc_intrinsic__stable_sort),


		make_intr2(make_print_signature(types), (void*)intrinsic__print),
		make_intr2(make_send_signature(types), (void*)intrinsic__send),
		make_intr2(make_exit_signature(types), (void*)intrinsic__exit),


		make_intr2(make_bw_not_signature(types), (void*)intrinsic__bw_not),
		make_intr2(make_bw_and_signature(types), (void*)intrinsic__bw_and),
		make_intr2(make_bw_or_signature(types), (void*)intrinsic__bw_or),
		make_intr2(make_bw_xor_signature(types), (void*)intrinsic__bw_xor),
		make_intr2(make_bw_shift_left_signature(types), (void*)intrinsic__bw_shift_left),
		make_intr2(make_bw_shift_right_signature(types), (void*)intrinsic__bw_shift_right),
		make_intr2(make_bw_shift_right_arithmetic_signature(types), (void*)intrinsic__bw_shift_right_arithmetic)
	};
}


}	// floyd
