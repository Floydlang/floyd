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







//	[R] map([E] elements, func R (E e, C context) f, C context)
static rt_value_t bc_intrinsic__map(interpreter_t& vm, const rt_value_t args[], int arg_count){
	QUARK_ASSERT(vm.check_invariant());
	QUARK_ASSERT(arg_count == 3);
//	QUARK_ASSERT(check_map_func_type(args[0]._type, args[1]._type, args[2]._type));

	auto& backend = vm._backend;

//	const auto e_type = peek2(backend.types, args[0]._type).get_vector_element_type(backend.types);
	const auto f = args[1];
	const auto f_type_peek = peek2(backend.types, f._type);
	const auto f_arg_types = f_type_peek.get_function_args(backend.types);
	const auto r_type = f_type_peek.get_function_return(backend.types);

	const auto& context = args[2];

	const auto input_vec = get_vector_elements(backend, args[0]);
	immer::vector<rt_value_t> vec2;
	for(const auto& e: input_vec){
		const rt_value_t f_args[] = { e, context };
		const auto result1 = call_function_bc(vm, f, f_args, 2);
		QUARK_ASSERT(result1.check_invariant());
		vec2 = vec2.push_back(result1);
	}

	const auto result = make_vector_value(backend, r_type, vec2);

	if(k_trace && false){
		const auto debug = value_and_type_to_json(backend.types, rt_to_value(backend, result));
		QUARK_TRACE(json_to_pretty_string(debug));
	}
	QUARK_ASSERT(result.check_invariant());
	return result;
}



/////////////////////////////////////////		PURE -- map_dag()



//	[R] map_dag([E] elements, [int] depends_on, func R (E, [R], C context) f, C context)
static rt_value_t bc_intrinsic__map_dag(interpreter_t& vm, const rt_value_t args[], int arg_count){
	QUARK_ASSERT(vm.check_invariant());
	QUARK_ASSERT(arg_count == 4);
//	QUARK_ASSERT(check_map_dag_func_type(args[0]._type, args[1]._type, args[2]._type, args[3]._type));

	auto& backend = vm._backend;
	const auto& elements = args[0];
//	const auto& e_type = elements._type.get_vector_element_type();
	const auto& parents = args[1];
	const auto& f = args[2];
	const auto f_type_peek = peek2(backend.types, f._type);
	const auto& r_type = f_type_peek.get_function_return(backend.types);
	const auto& context = args[3];

	const auto elements2 = get_vector_elements(backend, elements);
	const auto parents2 = get_vector_elements(backend, parents);

	if(elements2.size() != parents2.size()) {
		quark::throw_runtime_error("map_dag() requires elements and parents be the same count.");
	}

	auto elements_todo = elements2.size();
	std::vector<int> rcs(elements2.size(), 0);

	immer::vector<rt_value_t> complete(elements2.size(), rt_value_t());

	for(const auto& e: parents2){
		const auto parent_index = e.get_int_value();

		const auto count = static_cast<int64_t>(elements2.size());
		QUARK_ASSERT(parent_index >= -1);
		QUARK_ASSERT(parent_index < count);

		if(parent_index != -1){
			rcs[parent_index]++;
		}
	}

	while(elements_todo > 0){
		std::vector<int> pass_ids;
		for(int i = 0 ; i < elements2.size() ; i++){
			const auto rc = rcs[i];
			if(rc == 0){
				pass_ids.push_back(i);
				rcs[i] = -1;
			}
		}

		if(pass_ids.empty()){
			quark::throw_runtime_error("map_dag() dependency cycle error.");
		}

		for(const auto element_index: pass_ids){
			const auto& e = elements2[element_index];

			//	Make list of the element's inputs -- the must all be complete now.
			immer::vector<rt_value_t> solved_deps;
			for(int element_index2 = 0 ; element_index2 < parents2.size() ; element_index2++){
				const auto& p = parents2[element_index2];
				const auto parent_index = p.get_int_value();
				if(parent_index == element_index){
					QUARK_ASSERT(element_index2 != -1);
					QUARK_ASSERT(element_index2 >= -1 && element_index2 < elements2.size());
					QUARK_ASSERT(rcs[element_index2] == -1);
					QUARK_ASSERT(complete[element_index2]._type.is_undefined() == false);
					const auto& solved = complete[element_index2];
					solved_deps = solved_deps.push_back(solved);
				}
			}

			const rt_value_t f_args[] = { e, make_vector_value(backend, r_type, solved_deps), context };
			const auto result1 = call_function_bc(vm, f, f_args, 3);

			const auto parent_index = parents2[element_index].get_int_value();
			if(parent_index != -1){
				rcs[parent_index]--;
			}
			complete = complete.set(element_index, result1);
			elements_todo--;
		}
	}

	const auto result = make_vector_value(backend, r_type, complete);

	if(k_trace && false){
		const auto debug = value_and_type_to_json(backend.types, rt_to_value(backend, result));
		QUARK_TRACE(json_to_pretty_string(debug));
	}

	return result;
}





/////////////////////////////////////////		PURE -- map_dag2()

//	Input dependencies are specified for as 1... many integers per E, in order. [-1] or [a, -1] or [a, b, -1 ] etc.
//
//	[R] map_dag([E] elements, [int] depends_on, func R (E, [R], C context) f, C context)

struct dep_t {
	int64_t incomplete_count;
	std::vector<int64_t> depends_in_element_index;
};


static rt_value_t bc_intrinsic__map_dag2(interpreter_t& vm, const rt_value_t args[], int arg_count){
	QUARK_ASSERT(vm.check_invariant());
	QUARK_ASSERT(arg_count == 4);
//	QUARK_ASSERT(check_map_dag_func_type(args[0]._type, args[1]._type, args[2]._type, args[3]._type));

	auto& backend = vm._backend;
	const auto& elements = args[0];
	const auto& e_type = peek2(backend.types, elements._type).get_vector_element_type(backend.types);
	const auto& dependencies = args[1];
	const auto& f = args[2];
	const auto f_type_peek = peek2(backend.types, f._type);
	const auto& r_type = f_type_peek.get_function_return(backend.types);

	const auto& context = args[3];

	if(
		e_type == f_type_peek.get_function_args(backend.types)[0]
		&& r_type == peek2(backend.types, f_type_peek.get_function_args(backend.types)[1]).get_vector_element_type(backend.types)
	){
	}
	else {
		quark::throw_runtime_error("R map_dag([E] elements, R init_value, R (R acc, E element) f");
	}

	const auto elements2 = get_vector_elements(backend, elements);
	const auto dependencies2 = get_vector_elements(backend, dependencies);


	immer::vector<rt_value_t> complete(elements2.size(), rt_value_t());

	std::vector<dep_t> element_dependencies(elements2.size(), dep_t{ 0, {} });
	{
		const auto element_count = static_cast<int64_t>(elements2.size());
		const auto dep_index_count = static_cast<int64_t>(dependencies2.size());
		int element_index = 0;
		int dep_index = 0;
		while(element_index < element_count){
			std::vector<int64_t> deps;
			const auto& e = dependencies2[dep_index];
			auto e_int = e.get_int_value();
			while(e_int != -1){
				deps.push_back(e_int);

				dep_index++;
				QUARK_ASSERT(dep_index < dep_index_count);

				e_int = dependencies2[dep_index].get_int_value();
			}
			QUARK_ASSERT(element_index < element_count);
			element_dependencies[element_index] = dep_t{ static_cast<int64_t>(deps.size()), deps };
			element_index++;
		}
		QUARK_ASSERT(element_index == element_count);
		QUARK_ASSERT(dep_index == dep_index_count);
	}

	auto elements_todo = elements2.size();
	while(elements_todo > 0){

		//	Make list of elements that should be processed this pass.
		std::vector<int> pass_ids;
		for(int i = 0 ; i < elements2.size() ; i++){
			const auto rc = element_dependencies[i].incomplete_count;
			if(rc == 0){
				pass_ids.push_back(i);
				element_dependencies[i].incomplete_count = -1;
			}
		}

		if(pass_ids.empty()){
			quark::throw_runtime_error("map_dag() dependency cycle error.");
		}

		for(const auto element_index: pass_ids){
			const auto& e = elements2[element_index];

			immer::vector<rt_value_t> ready_elements;
			for(const auto& dep_e: element_dependencies[element_index].depends_in_element_index){
				const auto& ready = complete[dep_e];
				ready_elements = ready_elements.push_back(ready);
			}
			const auto ready_elements2 = make_vector_value(backend, r_type, ready_elements);
			const rt_value_t f_args[] = { e, ready_elements2, context };

			const auto result1 = call_function_bc(vm, f, f_args, 3);

			//	Decrement incomplete_count for every element that specifies *element_index* as a input dependency.
			for(auto& x: element_dependencies){
				const auto it = std::find(x.depends_in_element_index.begin(), x.depends_in_element_index.end(), element_index);
				if(it != x.depends_in_element_index.end()){
					x.incomplete_count--;
				}
			}

			//	Copy value to output.
			complete = complete.set(element_index, result1);
			elements_todo--;
		}
	}

	const auto result = make_vector_value(backend, r_type, complete);

	if(k_trace && false){
		const auto debug = value_and_type_to_json(backend.types, rt_to_value(backend, result));
		QUARK_TRACE(json_to_pretty_string(debug));
	}

	return result;
}




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
		make_intr2(make_assert_signature(types), (void*)unified_intrinsic__assert),
		make_intr2(make_to_string_signature(types), (void*)unified_intrinsic__to_string),
		make_intr2(make_to_pretty_string_signature(types), (void*)unified_intrinsic__to_pretty_string),
		make_intr2(make_typeof_signature(types), (void*)unified_intrinsic__typeof),


		make_intr2(make_update_signature(types), (void*)unified_intrinsic__update),

		//	size() is translated to BC opcode
		make_intr2(make_size_signature(types), (void*)unified_intrinsic__size),
		make_intr2(make_find_signature(types), (void*)unified_intrinsic__find),
		make_intr2(make_exists_signature(types), (void*)unified_intrinsic__exists),
		make_intr2(make_erase_signature(types), (void*)unified_intrinsic__erase),
		make_intr2(make_get_keys_signature(types), (void*)unified_intrinsic__get_keys),

		//	push_back() is translated to BC opcode
		make_intr2(make_push_back_signature(types), (void*)unified_intrinsic__push_back),
		make_intr2(make_subset_signature(types), (void*)unified_intrinsic__subset),
		make_intr2(make_replace_signature(types), (void*)unified_intrinsic__replace),


		make_intr2(make_get_json_type_signature(types), (void*)unified_intrinsic__get_json_type),
		make_intr2(make_generate_json_script_signature(types), (void*)unified_intrinsic__generate_json_script),
		make_intr2(make_parse_json_script_signature(types), (void*)unified_intrinsic__parse_json_script),
		make_intr2(make_to_json_signature(types), (void*)unified_intrinsic__to_json),
		make_intr2(make_from_json_signature(types), (void*)unified_intrinsic__from_json),


		make_intr(make_map_signature(types), bc_intrinsic__map),
//		make_intr2(make_map_signature(types), (void*)unified_intrinsic__map),
		make_intr(make_filter_signature(types), bc_intrinsic__filter),
		make_intr(make_reduce_signature(types), bc_intrinsic__reduce),
		make_intr(make_map_dag_signature(types), bc_intrinsic__map_dag),

		make_intr(make_stable_sort_signature(types), bc_intrinsic__stable_sort),


		make_intr2(make_print_signature(types), (void*)unified_intrinsic__print),
		make_intr2(make_send_signature(types), (void*)unified_intrinsic__send),
		make_intr2(make_exit_signature(types), (void*)unified_intrinsic__exit),


		make_intr2(make_bw_not_signature(types), (void*)unified_intrinsic__bw_not),
		make_intr2(make_bw_and_signature(types), (void*)unified_intrinsic__bw_and),
		make_intr2(make_bw_or_signature(types), (void*)unified_intrinsic__bw_or),
		make_intr2(make_bw_xor_signature(types), (void*)unified_intrinsic__bw_xor),
		make_intr2(make_bw_shift_left_signature(types), (void*)unified_intrinsic__bw_shift_left),
		make_intr2(make_bw_shift_right_signature(types), (void*)unified_intrinsic__bw_shift_right),
		make_intr2(make_bw_shift_right_arithmetic_signature(types), (void*)unified_intrinsic__bw_shift_right_arithmetic)
	};
}


}	// floyd
