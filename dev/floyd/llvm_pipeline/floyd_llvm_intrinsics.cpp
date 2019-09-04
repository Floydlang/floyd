//
//  floyd_llvm_intrinsics.cpp
//  floyd
//
//  Created by Marcus Zetterquist on 2019-08-28.
//  Copyright © 2019 Marcus Zetterquist. All rights reserved.
//

#include "floyd_llvm_intrinsics.h"


#include "floyd_llvm_helpers.h"
#include "floyd_llvm_runtime.h"
#include "value_features.h"
#include "floyd_runtime.h"
#include "text_parser.h"

#include <llvm/ExecutionEngine/ExecutionEngine.h>

namespace floyd {







////////////////////////////////	HELPERS FOR RUNTIME CALLBACKS




static std::string gen_to_string(llvm_execution_engine_t& runtime, runtime_value_t arg_value, runtime_type_t arg_type){
	QUARK_ASSERT(runtime.check_invariant());

	const auto& type = lookup_type_ref(runtime.backend, arg_type);
	const auto value = from_runtime_value(runtime, arg_value, type);
	const auto a = to_compact_string2(value);
	return a;
}







////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//	INTRINSICS
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////





static void floyd_llvm_intrinsic__assert(floyd_runtime_t* frp, runtime_value_t arg){
	auto& r = get_floyd_runtime(frp);

	QUARK_ASSERT(arg.bool_value == 0 || arg.bool_value == 1);

	bool ok = arg.bool_value == 0 ? false : true;
	if(!ok){
		r._print_output.push_back("Assertion failed.");
		quark::throw_runtime_error("Floyd assertion failed.");
	}
}

//??? optimize prio 1
static runtime_value_t floyd_llvm_intrinsic__erase(floyd_runtime_t* frp, runtime_value_t arg0_value, runtime_type_t arg0_type, runtime_value_t arg1_value, runtime_type_t arg1_type){
	auto& r = get_floyd_runtime(frp);

	const auto& type0 = lookup_type_ref(r.backend, arg0_type);
	const auto& type1 = lookup_type_ref(r.backend, arg1_type);

	QUARK_ASSERT(type0.is_dict());
	QUARK_ASSERT(type1.is_string());

	if(is_dict_cppmap(itype_t(arg0_type))){
		const auto& dict = unpack_dict_cppmap_arg(r.backend, arg0_value, arg0_type);

		const auto value_type = type0.get_dict_value_type();

		//	Deep copy dict.
		auto dict2 = alloc_dict_cppmap(r.backend.heap, itype_t(arg0_type));
		auto& m = dict2.dict_cppmap_ptr->get_map_mut();
		m = dict->get_map();

		const auto key_string = from_runtime_string(r, arg1_value);
		m.erase(key_string);

		const auto value_itype = lookup_itype(r.backend, value_type);
		if(is_rc_value(value_itype)){
			for(auto& e: m){
				retain_value(r.backend, e.second, value_itype);
			}
		}
		return dict2;
	}
	else if(is_dict_hamt(itype_t(arg0_type))){
		const auto& dict = *arg0_value.dict_hamt_ptr;

		const auto value_type = type0.get_dict_value_type();

		//	Deep copy dict.
		auto dict2 = alloc_dict_hamt(r.backend.heap, itype_t(arg0_type));
		auto& m = dict2.dict_hamt_ptr->get_map_mut();
		m = dict.get_map();

		const auto key_string = from_runtime_string(r, arg1_value);
		m = m.erase(key_string);

		const auto value_itype = lookup_itype(r.backend, value_type);
		if(is_rc_value(value_itype)){
			for(auto& e: m){
				retain_value(r.backend, e.second, value_itype);
			}
		}
		return dict2;
	}
	else{
		QUARK_ASSERT(false);
		throw std::exception();
	}
}





//??? optimize prio 1
//??? We need to figure out the return type *again*, knowledge we have already in semast.
static runtime_value_t floyd_llvm_intrinsic__get_keys(floyd_runtime_t* frp, runtime_value_t arg0_value, runtime_type_t arg0_type){
	auto& r = get_floyd_runtime(frp);

	const auto& type0 = lookup_type_ref(r.backend, arg0_type);
	QUARK_ASSERT(type0.is_dict());

	if(is_dict_cppmap(itype_t(arg0_type))){
		if(k_global_vector_type == vector_backend::carray){
			return get_keys__cppmap_carray(r.backend, arg0_value, arg0_type);
		}
		else if(k_global_vector_type == vector_backend::hamt){
			return get_keys__cppmap_hamt(r.backend, arg0_value, arg0_type);
		}
		else{
			QUARK_ASSERT(false);
			throw std::exception();
		}
	}
	else if(is_dict_hamt(itype_t(arg0_type))){
		if(k_global_vector_type == vector_backend::carray){
			return get_keys__hamtmap_carray(r.backend, arg0_value, arg0_type);
		}
		else if(k_global_vector_type == vector_backend::hamt){
			return get_keys__hamtmap_hamt(r.backend, arg0_value, arg0_type);
		}
		else{
			QUARK_ASSERT(false);
			throw std::exception();
		}
	}
	else{
		QUARK_ASSERT(false);
		throw std::exception();
	}
}

//??? optimize prio 1
//??? kill unpack_dict_cppmap_arg()
static uint32_t floyd_llvm_intrinsic__exists(floyd_runtime_t* frp, runtime_value_t arg0_value, runtime_type_t arg0_type, runtime_value_t arg1_value, runtime_type_t arg1_type){
	auto& r = get_floyd_runtime(frp);

	const auto& type0 = lookup_type_ref(r.backend, arg0_type);
	const auto& type1 = lookup_type_ref(r.backend, arg1_type);
	QUARK_ASSERT(type0.is_dict());

	if(is_dict_cppmap(itype_t(arg0_type))){
		const auto& dict = unpack_dict_cppmap_arg(r.backend, arg0_value, arg0_type);
		const auto key_string = from_runtime_string(r, arg1_value);

		const auto& m = dict->get_map();
		const auto it = m.find(key_string);
		return it != m.end() ? 1 : 0;
	}
	else if(is_dict_hamt(itype_t(arg0_type))){
		const auto& dict = *arg0_value.dict_hamt_ptr;
		const auto key_string = from_runtime_string(r, arg1_value);

		const auto& m = dict.get_map();
		const auto it = m.find(key_string);
		return it != nullptr ? 1 : 0;
	}
	else{
		QUARK_ASSERT(false);
		throw std::exception();
	}
}



//??? optimize prio 1
static int64_t floyd_llvm_intrinsic__find(floyd_runtime_t* frp, runtime_value_t arg0_value, runtime_type_t arg0_type, const runtime_value_t arg1_value, runtime_type_t arg1_type){
	auto& r = get_floyd_runtime(frp);

	const auto& type0 = lookup_type_ref(r.backend, arg0_type);
	if(type0.is_string()){
		return find__string(r.backend, arg0_value, arg0_type, arg1_value, arg1_type);
	}
	else if(is_vector_carray(itype_t(arg0_type))){
		return find__carray(r.backend, arg0_value, arg0_type, arg1_value, arg1_type);
	}
	else if(is_vector_hamt(itype_t(arg0_type))){
		return find__hamt(r.backend, arg0_value, arg0_type, arg1_value, arg1_type);
	}
	else{
		//	No other types allowed.
		UNSUPPORTED();
	}
}

static int64_t floyd_llvm_intrinsic__get_json_type(floyd_runtime_t* frp, JSON_T* json_ptr){
	auto& r = get_floyd_runtime(frp);
	(void)r;
	QUARK_ASSERT(json_ptr != nullptr);

	const auto& json = json_ptr->get_json();
	const auto result = get_json_type(json);
	return result;
}


static runtime_value_t floyd_llvm_intrinsic__generate_json_script(floyd_runtime_t* frp, JSON_T* json_ptr){
	auto& r = get_floyd_runtime(frp);
	QUARK_ASSERT(json_ptr != nullptr);

	const auto& json = json_ptr->get_json();

	const std::string s = json_to_compact_string(json);
	return to_runtime_string(r, s);
}

static runtime_value_t floyd_llvm_intrinsic__from_json(floyd_runtime_t* frp, JSON_T* json_ptr, runtime_type_t target_type){
	auto& r = get_floyd_runtime(frp);
	QUARK_ASSERT(json_ptr != nullptr);

	const auto& json = json_ptr->get_json();
	const auto& target_type2 = lookup_type_ref(r.backend, target_type);

	const auto result = unflatten_json_to_specific_type(json, target_type2);
	const auto result2 = to_runtime_value(r, result);
	return result2;
}








//??? Record all types at compile time, provide as arguments here.

typedef runtime_value_t (*MAP_F)(floyd_runtime_t* frp, runtime_value_t arg0_value, runtime_value_t arg1_value);

//??? Use C++ template to generate these two functions.
//??? optimize prio 1
static runtime_value_t map__carray(floyd_runtime_t* frp, value_backend_t& backend, runtime_value_t elements_vec, runtime_type_t elements_vec_type, runtime_value_t f_value, runtime_type_t f_value_type, runtime_value_t context_value, runtime_type_t context_value_type){

	const auto& type1 = lookup_type_ref(backend, f_value_type);
#if DEBUG
	QUARK_ASSERT(backend.check_invariant());

	const auto& type0 = lookup_type_ref(backend, elements_vec_type);
	const auto& type2 = lookup_type_ref(backend, context_value_type);
	QUARK_ASSERT(check_map_func_type(type0, type1, type2));

	const auto e_type = type0.get_vector_element_type();
	const auto f_arg_types = type1.get_function_args();
#endif
	const auto r_type = type1.get_function_return();
	const auto f = reinterpret_cast<MAP_F>(f_value.function_ptr);

	const auto return_type = typeid_t::make_vector(r_type);
	const auto count = elements_vec.vector_carray_ptr->get_element_count();
	auto result_vec = alloc_vector_carray(backend.heap, count, count, lookup_itype(backend, return_type));
	for(int i = 0 ; i < count ; i++){
		const auto a = (*f)(frp, elements_vec.vector_carray_ptr->get_element_ptr()[i], context_value);
		result_vec.vector_carray_ptr->get_element_ptr()[i] = a;
	}
	return result_vec;
}

//??? Use C++ template to generate these two functions.
//??? optimize prio 1
static runtime_value_t map__hamt(floyd_runtime_t* frp, value_backend_t& backend, runtime_value_t elements_vec, runtime_type_t elements_vec_type, runtime_value_t f_value, runtime_type_t f_value_type, runtime_value_t context_value, runtime_type_t context_value_type){

	const auto& type1 = lookup_type_ref(backend, f_value_type);
#if DEBUG
	QUARK_ASSERT(backend.check_invariant());

	const auto& type0 = lookup_type_ref(backend, elements_vec_type);
	const auto& type2 = lookup_type_ref(backend, context_value_type);
	QUARK_ASSERT(check_map_func_type(type0, type1, type2));

	const auto e_type = type0.get_vector_element_type();
	const auto f_arg_types = type1.get_function_args();
#endif
	const auto r_type = type1.get_function_return();
	const auto f = reinterpret_cast<MAP_F>(f_value.function_ptr);

	const auto return_type = typeid_t::make_vector(r_type);
	const auto count = elements_vec.vector_hamt_ptr->get_element_count();
	auto result_vec = alloc_vector_hamt(backend.heap, count, count, lookup_itype(backend, return_type));
	for(int i = 0 ; i < count ; i++){
		const auto& element = elements_vec.vector_hamt_ptr->load_element(i);
		const auto a = (*f)(frp, element, context_value);
		result_vec.vector_hamt_ptr->store_mutate(i, a);
	}
	return result_vec;
}

//??? optimize prio 1: check type at compile time, not runtime.

//	[R] map([E] elements, func R (E e, C context) f, C context)
static runtime_value_t floyd_llvm_intrinsic__map(floyd_runtime_t* frp, runtime_value_t arg0_value, runtime_type_t arg0_type, runtime_value_t arg1_value, runtime_type_t arg1_type, runtime_value_t arg2_value, runtime_type_t arg2_type){
	auto& r = get_floyd_runtime(frp);

	if(is_vector_carray(itype_t(arg0_type))){
		return map__carray(frp, r.backend, arg0_value, arg0_type, arg1_value, arg1_type, arg2_value, arg2_type);
	}
	else if(is_vector_hamt(itype_t(arg0_type))){
		return map__hamt(frp, r.backend, arg0_value, arg0_type, arg1_value, arg1_type, arg2_value, arg2_type);
	}
	else{
		QUARK_ASSERT(false);
		throw std::exception();
	}
}



//??? Can mutate the acc string internally.

typedef runtime_value_t (*MAP_STRING_F)(floyd_runtime_t* frp, runtime_value_t s, runtime_value_t context);

static runtime_value_t floyd_llvm_intrinsic__map_string(floyd_runtime_t* frp, runtime_value_t input_string0, runtime_value_t func, runtime_type_t func_type, runtime_value_t context, runtime_type_t context_type){
	auto& r = get_floyd_runtime(frp);

	QUARK_ASSERT(check_map_string_func_type(
		typeid_t::make_string(),
		lookup_type_ref(r.backend, func_type),
		lookup_type_ref(r.backend, context_type)
	));

	const auto f = reinterpret_cast<MAP_STRING_F>(func.function_ptr);

	const auto input_string = from_runtime_string(r, input_string0);

	auto count = input_string.size();

	std::string acc;
	for(int i = 0 ; i < count ; i++){
		const int64_t ch = input_string[i];
		const auto ch2 = make_runtime_int(ch);
		const auto temp = (*f)(frp, ch2, context);

		const auto temp2 = temp.int_value;
		acc.push_back(static_cast<char>(temp2));
	}
	return to_runtime_string(r, acc);
}





// [R] map_dag([E] elements, [int] depends_on, func R (E, [R], C context) f, C context)

typedef runtime_value_t (*map_dag_F)(floyd_runtime_t* frp, runtime_value_t arg0_value, runtime_value_t arg1_value, runtime_value_t context);

//??? optimize prio 2
static runtime_value_t map_dag__carray(
	floyd_runtime_t* frp,
	value_backend_t& backend,
	runtime_value_t arg0_value,
	runtime_type_t arg0_type,
	runtime_value_t arg1_value,
	runtime_type_t arg1_type,
	runtime_value_t arg2_value,
	runtime_type_t arg2_type,
	runtime_value_t context,
	runtime_type_t context_type
){
	QUARK_ASSERT(frp != nullptr);
	QUARK_ASSERT(backend.check_invariant());

	const auto& type0 = lookup_type_ref(backend, arg0_type);
	const auto& type1 = lookup_type_ref(backend, arg1_type);
	const auto& type2 = lookup_type_ref(backend, arg2_type);
	QUARK_ASSERT(check_map_dag_func_type(type0, type1, type2, lookup_type_ref(backend, context_type)));

	const auto& elements = arg0_value;
#if DEBUG
	const auto& e_type = type0.get_vector_element_type();
#endif
	const auto& parents = arg1_value;
	const auto& f = arg2_value;
	const auto& r_type = type2.get_function_return();

	QUARK_ASSERT(e_type == type2.get_function_args()[0] && r_type == type2.get_function_args()[1].get_vector_element_type());

//	QUARK_ASSERT(is_vector_carray(typeid_t::make_vector(e_type)));
//	QUARK_ASSERT(is_vector_carray(typeid_t::make_vector(r_type)));

	const auto return_type = typeid_t::make_vector(r_type);

	const auto f2 = reinterpret_cast<map_dag_F>(f.function_ptr);

	const auto elements2 = elements.vector_carray_ptr;
	const auto parents2 = parents.vector_carray_ptr;

	if(elements2->get_element_count() != parents2->get_element_count()) {
		quark::throw_runtime_error("map_dag() requires elements and parents be the same count.");
	}

	auto elements_todo = elements2->get_element_count();
	std::vector<int> rcs(elements2->get_element_count(), 0);

	std::vector<runtime_value_t> complete(elements2->get_element_count(), runtime_value_t());

	for(int i = 0 ; i < parents2->get_element_count() ; i++){
		const auto& e = parents2->load_element(i);
		const auto parent_index = e.int_value;

		const auto count = static_cast<int64_t>(elements2->get_element_count());
		QUARK_ASSERT(parent_index >= -1);
		QUARK_ASSERT(parent_index < count);

		if(parent_index != -1){
			rcs[parent_index]++;
		}
	}

	while(elements_todo > 0){
		//	Find all elements that are free to process -- they are not blocked on a depenency.
		std::vector<int> pass_ids;
		for(int i = 0 ; i < elements2->get_element_count() ; i++){
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
			const auto& e = elements2->load_element(element_index);

			//	Make list of the element's inputs -- they must all be complete now.
			std::vector<runtime_value_t> solved_deps;
			for(int element_index2 = 0 ; element_index2 < parents2->get_element_count() ; element_index2++){
				const auto& p = parents2->load_element(element_index2);
				const auto parent_index = p.int_value;
				if(parent_index == element_index){
					QUARK_ASSERT(element_index2 != -1);
					QUARK_ASSERT(element_index2 >= -1 && element_index2 < elements2->get_element_count());
					QUARK_ASSERT(rcs[element_index2] == -1);

					const auto& solved = complete[element_index2];
					solved_deps.push_back(solved);
				}
			}

			auto solved_deps2 = alloc_vector_carray(backend.heap, solved_deps.size(), solved_deps.size(), lookup_itype(backend, return_type));
			for(int i = 0 ; i < solved_deps.size() ; i++){
				solved_deps2.vector_carray_ptr->store(i, solved_deps[i]);
			}
			runtime_value_t solved_deps3 = solved_deps2;

			const auto result1 = (*f2)(frp, e, solved_deps3, context);

			//	Release just the vec, **not the elements**. The elements are aliases for complete-vector.
			if(dec_rc(solved_deps2.vector_carray_ptr->alloc) == 0){
				dispose_vector_carray(solved_deps2);
			}

			const auto parent_index = parents2->load_element(element_index).int_value;
			if(parent_index != -1){
				rcs[parent_index]--;
			}
			complete[element_index] = result1;
			elements_todo--;
		}
	}

	//??? No need to copy all elements -- could store them directly into the VEC_T.
	const auto count = complete.size();
	auto result_vec = alloc_vector_carray(backend.heap, count, count, lookup_itype(backend, return_type));
	for(int i = 0 ; i < count ; i++){
//		retain_value(r, complete[i], r_type);
		result_vec.vector_carray_ptr->store(i, complete[i]);
	}

	return result_vec;
}

//??? optimize prio 2
static runtime_value_t map_dag__hamt(
	floyd_runtime_t* frp,
	value_backend_t& backend,
	runtime_value_t arg0_value,
	runtime_type_t arg0_type,
	runtime_value_t arg1_value,
	runtime_type_t arg1_type,
	runtime_value_t arg2_value,
	runtime_type_t arg2_type,
	runtime_value_t context,
	runtime_type_t context_type
){
	QUARK_ASSERT(frp != nullptr);
	QUARK_ASSERT(backend.check_invariant());

	const auto& type0 = lookup_type_ref(backend, arg0_type);
	const auto& type1 = lookup_type_ref(backend, arg1_type);
	const auto& type2 = lookup_type_ref(backend, arg2_type);
	QUARK_ASSERT(check_map_dag_func_type(type0, type1, type2, lookup_type_ref(backend, context_type)));

	const auto& elements = arg0_value;
#if DEBUG
	const auto& e_type = type0.get_vector_element_type();
#endif
	const auto& parents = arg1_value;
	const auto& f = arg2_value;
	const auto& r_type = type2.get_function_return();

	QUARK_ASSERT(e_type == type2.get_function_args()[0] && r_type == type2.get_function_args()[1].get_vector_element_type());

//	QUARK_ASSERT(is_vector_hamt(typeid_t::make_vector(e_type)));
//	QUARK_ASSERT(is_vector_hamt(typeid_t::make_vector(r_type)));

	const auto return_type = typeid_t::make_vector(r_type);

	const auto f2 = reinterpret_cast<map_dag_F>(f.function_ptr);

	const auto elements2 = elements.vector_hamt_ptr;
	const auto parents2 = parents.vector_hamt_ptr;

	if(elements2->get_element_count() != parents2->get_element_count()) {
		quark::throw_runtime_error("map_dag() requires elements and parents be the same count.");
	}

	auto elements_todo = elements2->get_element_count();
	std::vector<int> rcs(elements2->get_element_count(), 0);

	std::vector<runtime_value_t> complete(elements2->get_element_count(), runtime_value_t());

	for(int i = 0 ; i < parents2->get_element_count() ; i++){
		const auto& e = parents2->load_element(i);
		const auto parent_index = e.int_value;

		//??? remove cast? static_cast<int64_t>()
		const auto count = static_cast<int64_t>(elements2->get_element_count());
		QUARK_ASSERT(parent_index >= -1);
		QUARK_ASSERT(parent_index < count);

		if(parent_index != -1){
			rcs[parent_index]++;
		}
	}

	while(elements_todo > 0){
		//	Find all elements that are free to process -- they are not blocked on a depenency.
		std::vector<int> pass_ids;
		for(int i = 0 ; i < elements2->get_element_count() ; i++){
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
			const auto& e = elements2->load_element(element_index);

			//	Make list of the element's inputs -- they must all be complete now.
			std::vector<runtime_value_t> solved_deps;
			for(int element_index2 = 0 ; element_index2 < parents2->get_element_count() ; element_index2++){
				const auto& p = parents2->load_element(element_index2);
				const auto parent_index = p.int_value;
				if(parent_index == element_index){
					QUARK_ASSERT(element_index2 != -1);
					QUARK_ASSERT(element_index2 >= -1 && element_index2 < elements2->get_element_count());
					QUARK_ASSERT(rcs[element_index2] == -1);

					const auto& solved = complete[element_index2];
					solved_deps.push_back(solved);
				}
			}

			auto solved_deps2 = alloc_vector_hamt(backend.heap, solved_deps.size(), solved_deps.size(), lookup_itype(backend, return_type));
			for(int i = 0 ; i < solved_deps.size() ; i++){
				solved_deps2.vector_hamt_ptr->store_mutate(i, solved_deps[i]);
			}
			runtime_value_t solved_deps3 = solved_deps2;

			const auto result1 = (*f2)(frp, e, solved_deps3, context);

			//	Release just the vec, **not the elements**. The elements are aliases for complete-vector.
			if(dec_rc(solved_deps2.vector_hamt_ptr->alloc) == 0){
				dispose_vector_hamt(solved_deps2);
			}

			const auto parent_index = parents2->load_element(element_index).int_value;
			if(parent_index != -1){
				rcs[parent_index]--;
			}
			complete[element_index] = result1;
			elements_todo--;
		}
	}

	//??? No need to copy all elements -- could store them directly into the VEC_T.
	const auto count = complete.size();
	auto result_vec = alloc_vector_hamt(backend.heap, count, count, lookup_itype(backend, return_type));
	for(int i = 0 ; i < count ; i++){
//		retain_value(r, complete[i], r_type);
		result_vec.vector_hamt_ptr->store_mutate(i, complete[i]);
	}

	return result_vec;
}

//??? optimize prio 1: check type at compile time, not runtime.
static runtime_value_t floyd_llvm_intrinsic__map_dag(
	floyd_runtime_t* frp,
	runtime_value_t arg0_value,
	runtime_type_t arg0_type,
	runtime_value_t arg1_value,
	runtime_type_t arg1_type,
	runtime_value_t arg2_value,
	runtime_type_t arg2_type,
	runtime_value_t context,
	runtime_type_t context_type
){
	auto& r = get_floyd_runtime(frp);

	const auto& type0 = lookup_type_ref(r.backend, arg0_type);
	if(is_vector_carray(itype_t(arg0_type))){
		return map_dag__carray(frp, r.backend, arg0_value, arg0_type, arg1_value, arg1_type, arg2_value, arg2_type, context, context_type);
	}
	else if(is_vector_hamt(itype_t(arg0_type))){
		return map_dag__hamt(frp, r.backend, arg0_value, arg0_type, arg1_value, arg1_type, arg2_value, arg2_type, context, context_type);
	}
	else{
		QUARK_ASSERT(false);
		throw std::exception();
	}
}







typedef runtime_value_t (*FILTER_F)(floyd_runtime_t* frp, runtime_value_t element_value, runtime_value_t context);

//??? optimize prio 1
static runtime_value_t filter__carray(floyd_runtime_t* frp, value_backend_t& backend, runtime_value_t arg0_value, runtime_type_t arg0_type, runtime_value_t arg1_value, runtime_type_t arg1_type, runtime_value_t context, runtime_type_t context_type){
	QUARK_ASSERT(backend.check_invariant());

	auto& r = get_floyd_runtime(frp);

	const auto& type0 = lookup_type_ref(r.backend, arg0_type);
	const auto& type1 = lookup_type_ref(r.backend, arg1_type);
	const auto& type2 = lookup_type_ref(r.backend, context_type);
	const auto& return_type = type0;

	QUARK_ASSERT(check_filter_func_type(type0, type1, type2));
	QUARK_ASSERT(is_vector_carray(itype_t(arg0_type)));

	const auto& vec = *arg0_value.vector_carray_ptr;
	const auto f = reinterpret_cast<FILTER_F>(arg1_value.function_ptr);

	auto count = vec.get_element_count();

	const auto e_element_itype = lookup_vector_element_itype(backend, itype_t(arg0_type));

	std::vector<runtime_value_t> acc;
	for(int i = 0 ; i < count ; i++){
		const auto element_value = vec.get_element_ptr()[i];
		const auto keep = (*f)(frp, element_value, context);
		if(keep.bool_value != 0){
			acc.push_back(element_value);

			if(is_rc_value(e_element_itype)){
				retain_value(r.backend, element_value, e_element_itype);
			}
		}
		else{
		}
	}

	const auto count2 = acc.size();
	auto result_vec = alloc_vector_carray(r.backend.heap, count2, count2, lookup_itype(backend, return_type));

	if(count2 > 0){
		//	Count > 0 required to get address to first element in acc.
		copy_elements(result_vec.vector_carray_ptr->get_element_ptr(), &acc[0], count2);
	}
	return result_vec;
}

//??? optimize prio 1
static runtime_value_t filter__hamt(floyd_runtime_t* frp, value_backend_t& backend, runtime_value_t arg0_value, runtime_type_t arg0_type, runtime_value_t arg1_value, runtime_type_t arg1_type, runtime_value_t context, runtime_type_t context_type){
	QUARK_ASSERT(backend.check_invariant());

	auto& r = get_floyd_runtime(frp);

	const auto& type0 = lookup_type_ref(r.backend, arg0_type);
	const auto& type1 = lookup_type_ref(r.backend, arg1_type);
	const auto& type2 = lookup_type_ref(r.backend, context_type);
	const auto& return_type = type0;

	QUARK_ASSERT(check_filter_func_type(type0, type1, type2));
	QUARK_ASSERT(is_vector_hamt(itype_t(arg0_type)));

	const auto& vec = *arg0_value.vector_hamt_ptr;
	const auto f = reinterpret_cast<FILTER_F>(arg1_value.function_ptr);

	auto count = vec.get_element_count();

	const auto e_element_itype = lookup_vector_element_itype(backend, itype_t(arg0_type));

	std::vector<runtime_value_t> acc;
	for(int i = 0 ; i < count ; i++){
		const auto element_value = vec.load_element(i);
		const auto keep = (*f)(frp, element_value, context);
		if(keep.bool_value != 0){
			acc.push_back(element_value);

			if(is_rc_value(e_element_itype)){
				retain_value(r.backend, element_value, e_element_itype);
			}
		}
		else{
		}
	}

	const auto count2 = acc.size();
	auto result_vec = alloc_vector_hamt(r.backend.heap, &acc[0], count2, lookup_itype(backend, return_type));
	return result_vec;
}

//??? optimize prio 1: check type at compile time, not runtime.
//	[E] filter([E] elements, func bool (E e, C context) f, C context)
static runtime_value_t floyd_llvm_intrinsic__filter(floyd_runtime_t* frp, runtime_value_t arg0_value, runtime_type_t arg0_type, runtime_value_t arg1_value, runtime_type_t arg1_type, runtime_value_t arg2_value, runtime_type_t arg2_type){
	auto& r = get_floyd_runtime(frp);

	const auto& type0 = lookup_type_ref(r.backend, arg0_type);
	if(is_vector_carray(itype_t(arg0_type))){
		return filter__carray(frp, r.backend, arg0_value, arg0_type, arg1_value, arg1_type, arg2_value, arg2_type);
	}
	else if(is_vector_hamt(itype_t(arg0_type))){
		return filter__hamt(frp, r.backend, arg0_value, arg0_type, arg1_value, arg1_type, arg2_value, arg2_type);
	}
	else{
		QUARK_ASSERT(false);
		throw std::exception();
	}
}



typedef runtime_value_t (*REDUCE_F)(floyd_runtime_t* frp, runtime_value_t acc_value, runtime_value_t element_value, runtime_value_t context);

//??? optimize prio 1
static runtime_value_t reduce__carray(floyd_runtime_t* frp, value_backend_t& backend, runtime_value_t arg0_value, runtime_type_t arg0_type, runtime_value_t arg1_value, runtime_type_t arg1_type, runtime_value_t arg2_value, runtime_type_t arg2_type, runtime_value_t context, runtime_type_t context_type){
	QUARK_ASSERT(backend.check_invariant());

	const auto& type0 = lookup_type_ref(backend, arg0_type);
	const auto& type1 = lookup_type_ref(backend, arg1_type);
	const auto& type2 = lookup_type_ref(backend, arg2_type);

	QUARK_ASSERT(check_reduce_func_type(type0, type1, type2, lookup_type_ref(backend, context_type)));
	QUARK_ASSERT(is_vector_carray(itype_t(arg0_type)));

	const auto& vec = *arg0_value.vector_carray_ptr;
	const auto& init = arg1_value;
	const auto f = reinterpret_cast<REDUCE_F>(arg2_value.function_ptr);

	auto count = vec.get_element_count();
	runtime_value_t acc = init;
	retain_value(backend, acc, itype_t(arg1_type));

	for(int i = 0 ; i < count ; i++){
		const auto element_value = vec.get_element_ptr()[i];
		const auto acc2 = (*f)(frp, acc, element_value, context);

		if(is_rc_value(itype_t(arg1_type))){
			release_value(backend, acc, itype_t(arg1_type));
		}
		acc = acc2;
	}
	return acc;
}

//??? optimize prio 1
static runtime_value_t reduce__hamt(floyd_runtime_t* frp, value_backend_t& backend, runtime_value_t arg0_value, runtime_type_t arg0_type, runtime_value_t arg1_value, runtime_type_t arg1_type, runtime_value_t arg2_value, runtime_type_t arg2_type, runtime_value_t context, runtime_type_t context_type){
	QUARK_ASSERT(backend.check_invariant());

	const auto& type0 = lookup_type_ref(backend, arg0_type);
	const auto& type1 = lookup_type_ref(backend, arg1_type);
	const auto& type2 = lookup_type_ref(backend, arg2_type);

	QUARK_ASSERT(check_reduce_func_type(type0, type1, type2, lookup_type_ref(backend, context_type)));
	QUARK_ASSERT(is_vector_hamt(itype_t(arg0_type)));

	const auto& vec = *arg0_value.vector_hamt_ptr;
	const auto& init = arg1_value;
	const auto f = reinterpret_cast<REDUCE_F>(arg2_value.function_ptr);

	auto count = vec.get_element_count();
	runtime_value_t acc = init;
	retain_value(backend, acc, itype_t(arg1_type));

	for(int i = 0 ; i < count ; i++){
		const auto element_value = vec.load_element(i);
		const auto acc2 = (*f)(frp, acc, element_value, context);

		if(is_rc_value(itype_t(arg1_type))){
			release_value(backend, acc, itype_t(arg1_type));
		}
		acc = acc2;
	}
	return acc;
}

//	R reduce([E] elements, R accumulator_init, func R (R accumulator, E element, C context) f, C context)

//??? optimize prio 1
//??? check type at compile time, not runtime.
static runtime_value_t floyd_llvm_intrinsic__reduce(floyd_runtime_t* frp, runtime_value_t arg0_value, runtime_type_t arg0_type, runtime_value_t arg1_value, runtime_type_t arg1_type, runtime_value_t arg2_value, runtime_type_t arg2_type, runtime_value_t context, runtime_type_t context_type){
	auto& r = get_floyd_runtime(frp);

	if(is_vector_carray(itype_t(arg0_type))){
		return reduce__carray(frp, r.backend, arg0_value, arg0_type, arg1_value, arg1_type, arg2_value, arg2_type, context, context_type);
	}
	else if(is_vector_hamt(itype_t(arg0_type))){
		return reduce__hamt(frp, r.backend, arg0_value, arg0_type, arg1_value, arg1_type, arg2_value, arg2_type, context, context_type);
	}
	else{
		QUARK_ASSERT(false);
		throw std::exception();
	}
}





typedef uint8_t (*stable_sort_F)(floyd_runtime_t* frp, runtime_value_t arg0_value, runtime_value_t arg1_value, runtime_value_t arg2_value);

//??? optimize prio 1

static runtime_value_t stable_sort__carray(
	floyd_runtime_t* frp,
	value_backend_t& backend,
	runtime_value_t arg0_value,
	runtime_type_t arg0_type,
	runtime_value_t arg1_value,
	runtime_type_t arg1_type,
	runtime_value_t arg2_value,
	runtime_type_t arg2_type
){
	QUARK_ASSERT(frp != nullptr);
	QUARK_ASSERT(backend.check_invariant());

	const auto& type0 = lookup_type_ref(backend, arg0_type);
	const auto& type1 = lookup_type_ref(backend, arg1_type);
	const auto& type2 = lookup_type_ref(backend, arg2_type);

	QUARK_ASSERT(check_stable_sort_func_type(type0, type1, type2));
	QUARK_ASSERT(is_vector_carray(itype_t(arg0_type)));

	const auto& elements = arg0_value;
	const auto& f = arg1_value;
	const auto& context = arg2_value;

	const auto elements2 = from_runtime_value2(backend, elements, type0);
	const auto f2 = reinterpret_cast<stable_sort_F>(f.function_ptr);

	struct sort_functor_r {
		bool operator() (const value_t &a, const value_t &b) {
			auto& r = get_floyd_runtime(frp);
			const auto left = to_runtime_value(r, a);
			const auto right = to_runtime_value(r, b);
			uint8_t result = (*f)(frp, left, right, context);
			return result == 1 ? true : false;
		}

		floyd_runtime_t* frp;
		runtime_value_t context;
		stable_sort_F f;
	};

	const sort_functor_r sort_functor { frp, context, f2 };

	auto mutate_inplace_elements = elements2.get_vector_value();
	std::stable_sort(mutate_inplace_elements.begin(), mutate_inplace_elements.end(), sort_functor);

	//??? optimize
	const auto result = to_runtime_value2(backend, value_t::make_vector_value(type0.get_vector_element_type(), mutate_inplace_elements));
	return result;
}

//??? optimize prio 1

static runtime_value_t stable_sort__hamt(
	floyd_runtime_t* frp,
	value_backend_t& backend,
	runtime_value_t arg0_value,
	runtime_type_t arg0_type,
	runtime_value_t arg1_value,
	runtime_type_t arg1_type,
	runtime_value_t arg2_value,
	runtime_type_t arg2_type
){
	QUARK_ASSERT(frp != nullptr);
	QUARK_ASSERT(backend.check_invariant());

	const auto& type0 = lookup_type_ref(backend, arg0_type);
	const auto& type1 = lookup_type_ref(backend, arg1_type);
	const auto& type2 = lookup_type_ref(backend, arg2_type);

	QUARK_ASSERT(check_stable_sort_func_type(type0, type1, type2));
	QUARK_ASSERT(is_vector_hamt(itype_t(arg0_type)));

	const auto& elements = arg0_value;
	const auto& f = arg1_value;
	const auto& context = arg2_value;

	const auto elements2 = from_runtime_value2(backend, elements, type0);
	const auto f2 = reinterpret_cast<stable_sort_F>(f.function_ptr);

	struct sort_functor_r {
		bool operator() (const value_t &a, const value_t &b) {
			auto& r = get_floyd_runtime(frp);
			const auto left = to_runtime_value(r, a);
			const auto right = to_runtime_value(r, b);
			uint8_t result = (*f)(frp, left, right, context);
			return result == 1 ? true : false;
		}

		floyd_runtime_t* frp;
		runtime_value_t context;
		stable_sort_F f;
	};

	const sort_functor_r sort_functor { frp, context, f2 };

	auto mutate_inplace_elements = elements2.get_vector_value();
	std::stable_sort(mutate_inplace_elements.begin(), mutate_inplace_elements.end(), sort_functor);

	//??? optimize
	const auto result = to_runtime_value2(backend, value_t::make_vector_value(type0.get_vector_element_type(), mutate_inplace_elements));
	return result;
}

//	[T] stable_sort([T] elements, bool less(T left, T right, C context), C context)

//??? optimize prio 1
//??? check type at compile time, not runtime.
static runtime_value_t floyd_llvm_intrinsic__stable_sort(
	floyd_runtime_t* frp,
	runtime_value_t arg0_value,
	runtime_type_t arg0_type,
	runtime_value_t arg1_value,
	runtime_type_t arg1_type,
	runtime_value_t arg2_value,
	runtime_type_t arg2_type
){
	auto& r = get_floyd_runtime(frp);

	const auto& type0 = lookup_type_ref(r.backend, arg0_type);
	if(is_vector_carray(itype_t(arg0_type))){
		return stable_sort__carray(frp, r.backend, arg0_value, arg0_type, arg1_value, arg1_type, arg2_value, arg2_type);
	}
	else if(is_vector_hamt(itype_t(arg0_type))){
		return stable_sort__hamt(frp, r.backend, arg0_value, arg0_type, arg1_value, arg1_type, arg2_value, arg2_type);
	}
	else{
		QUARK_ASSERT(false);
		throw std::exception();
	}
}







//??? optimize prio 2

void floyd_llvm_intrinsic__print(floyd_runtime_t* frp, runtime_value_t arg0_value, runtime_type_t arg0_type){
	auto& r = get_floyd_runtime(frp);

	const auto s = gen_to_string(r, arg0_value, arg0_type);
	printf("%s\n", s.c_str());
	r._print_output.push_back(s);
}




//??? optimize prio 1
//??? check type at compile time, not runtime.

static runtime_value_t floyd_llvm_intrinsic__push_back(floyd_runtime_t* frp, runtime_value_t arg0_value, runtime_type_t arg0_type, runtime_value_t arg1_value, runtime_type_t arg1_type){
	auto& r = get_floyd_runtime(frp);

	const auto& type0 = lookup_type_ref(r.backend, arg0_type);
	const auto& type1 = lookup_type_ref(r.backend, arg1_type);
	const auto return_type = type0;
	if(type0.is_string()){
		auto value = from_runtime_string(r, arg0_value);

		QUARK_ASSERT(type1.is_int());

		value.push_back((char)arg1_value.int_value);
		const auto result2 = to_runtime_string(r, value);
		return result2;
	}
	else if(is_vector_carray(itype_t(arg0_type))){
		const auto vs = unpack_vector_carray_arg(r.backend, arg0_value, arg0_type);

		QUARK_ASSERT(type1 == type0.get_vector_element_type());
		QUARK_ASSERT(is_vector_carray(itype_t(arg0_type)));

		const auto element = arg1_value;
		const auto element_type = type1;
		const auto element_itype = itype_t(arg1_type);

		const auto element_count2 = vs->get_element_count() + 1;
		auto v2 = alloc_vector_carray(r.backend.heap, element_count2, element_count2, lookup_itype(r.backend, return_type));

		auto dest_ptr = v2.vector_carray_ptr->get_element_ptr();
		auto source_ptr = vs->get_element_ptr();

		if(is_rc_value(element_itype)){
			retain_value(r.backend, element, itype_t(arg1_type));

			for(int i = 0 ; i < vs->get_element_count() ; i++){
				retain_value(r.backend, source_ptr[i], itype_t(arg1_type));
				dest_ptr[i] = source_ptr[i];
			}
			dest_ptr[vs->get_element_count()] = element;
		}
		else{
			for(int i = 0 ; i < vs->get_element_count() ; i++){
				dest_ptr[i] = source_ptr[i];
			}
			dest_ptr[vs->get_element_count()] = element;
		}
		return v2;
	}
	else if(is_vector_hamt(itype_t(arg0_type))){
		QUARK_ASSERT(type1 == type0.get_vector_element_type());

		const auto element_type = type1;
		const auto element_itype = itype_t(arg1_type);

		runtime_value_t vec2 = push_back_immutable(arg0_value, arg1_value);

		if(is_rc_value(element_itype)){
			for(int i = 0 ; i < vec2.vector_hamt_ptr->get_element_count() ; i++){
				const auto& value = vec2.vector_hamt_ptr->load_element(i);
				retain_value(r.backend, value, itype_t(arg1_type));
			}
		}
		return vec2;
	}
	else{
		//	No other types allowed.
		UNSUPPORTED();
	}
}



//??? optimize prio 1
//??? check type at compile time, not runtime.

static const runtime_value_t floyd_llvm_intrinsic__replace(floyd_runtime_t* frp, runtime_value_t arg0_value, runtime_type_t arg0_type, uint64_t start, uint64_t end, runtime_value_t arg3_value, runtime_type_t arg3_type){
	auto& r = get_floyd_runtime(frp);

	const auto& type0 = lookup_type_ref(r.backend, arg0_type);
	const auto& type3 = lookup_type_ref(r.backend, arg3_type);

	QUARK_ASSERT(type3 == type0);

	if(type0.is_string()){
		return replace__string(r.backend, arg0_value, arg0_type, start, end, arg3_value, arg3_type);
	}
	else if(is_vector_carray(itype_t(arg0_type))){
		return replace__carray(r.backend, arg0_value, arg0_type, start, end, arg3_value, arg3_type);
	}
	else if(is_vector_hamt(itype_t(arg0_type))){
		return replace__hamt(r.backend, arg0_value, arg0_type, start, end, arg3_value, arg3_type);
	}
	else{
		//	No other types allowed.
		UNSUPPORTED();
	}
}




static JSON_T* floyd_llvm_intrinsic__parse_json_script(floyd_runtime_t* frp, runtime_value_t string_s0){
	auto& r = get_floyd_runtime(frp);

	const auto string_s = from_runtime_string(r, string_s0);

	std::pair<json_t, seq_t> result0 = parse_json(seq_t(string_s));
	auto result = alloc_json(r.backend.heap, result0.first);
	return result;
}

//??? optimize prio 2

static void floyd_llvm_intrinsic__send(floyd_runtime_t* frp, runtime_value_t process_id0, const JSON_T* message_json_ptr){
	auto& r = get_floyd_runtime(frp);

	QUARK_ASSERT(message_json_ptr != nullptr);

	const auto& process_id = from_runtime_string(r, process_id0);
	const auto& message_json = message_json_ptr->get_json();

	if(k_trace_process_messaging){
		QUARK_TRACE_SS("send(\"" << process_id << "\"," << json_to_pretty_string(message_json) <<")");
	}

	r._handler->on_send(process_id, message_json);
}

//??? optimize prio 1
//??? check type at compile time, not runtime.
static int64_t floyd_llvm_intrinsic__size(floyd_runtime_t* frp, runtime_value_t arg0_value, runtime_type_t arg0_type){
	auto& r = get_floyd_runtime(frp);

	const auto& type0 = lookup_type_ref(r.backend, arg0_type);

	if(type0.is_string()){
		return get_vec_string_size(arg0_value);
	}
	else if(type0.is_json()){
		const auto& json = arg0_value.json_ptr->get_json();

		if(json.is_object()){
			return json.get_object_size();
		}
		else if(json.is_array()){
			return json.get_array_size();
		}
		else if(json.is_string()){
			return json.get_string().size();
		}
		else{
			quark::throw_runtime_error("Calling size() on unsupported type of value.");
		}
	}
	else if(is_vector_carray(itype_t(arg0_type))){
		const auto vs = unpack_vector_carray_arg(r.backend, arg0_value, arg0_type);
		return vs->get_element_count();
	}
	else if(is_vector_hamt(itype_t(arg0_type))){
		return arg0_value.vector_hamt_ptr->get_element_count();
	}

	else if(is_dict_cppmap(itype_t(arg0_type))){
		DICT_CPPMAP_T* dict = unpack_dict_cppmap_arg(r.backend, arg0_value, arg0_type);
		return dict->size();
	}
	else if(is_dict_hamt(itype_t(arg0_type))){
		const auto& dict = *arg0_value.dict_hamt_ptr;
		return dict.size();
	}
	else{
		//	No other types allowed.
		UNSUPPORTED();
	}
}



//??? optimize prio 1
//??? check type at compile time, not runtime.

static const runtime_value_t floyd_llvm_intrinsic__subset(floyd_runtime_t* frp, runtime_value_t arg0_value, runtime_type_t arg0_type, uint64_t start, uint64_t end){
	auto& r = get_floyd_runtime(frp);

	const auto& type0 = lookup_type_ref(r.backend, arg0_type);
	if(type0.is_string()){
		return subset__string(r.backend, arg0_value, arg0_type, start, end);
	}
	else if(is_vector_carray(itype_t(arg0_type))){
		return subset__carray(r.backend, arg0_value, arg0_type, start, end);
	}
	else if(is_vector_hamt(itype_t(arg0_type))){
		return subset__hamt(r.backend, arg0_value, arg0_type, start, end);
	}
	else{
		//	No other types allowed.
		UNSUPPORTED();
	}
}




static runtime_value_t floyd_llvm_intrinsic__to_pretty_string(floyd_runtime_t* frp, runtime_value_t arg0_value, runtime_type_t arg0_type){
	auto& r = get_floyd_runtime(frp);

	const auto& type0 = lookup_type_ref(r.backend, arg0_type);
	const auto& value = from_runtime_value(r, arg0_value, type0);
	const auto json = value_to_ast_json(value, json_tags::k_plain);
	const auto s = json_to_pretty_string(json, 0, pretty_t{ 80, 4 });
	return to_runtime_string(r, s);
}

//??? optimize prio 2
static runtime_value_t floyd_llvm_intrinsic__to_string(floyd_runtime_t* frp, runtime_value_t arg0_value, runtime_type_t arg0_type){
	auto& r = get_floyd_runtime(frp);

	const auto s = gen_to_string(r, arg0_value, arg0_type);
	return to_runtime_string(r, s);
}



static runtime_type_t floyd_llvm_intrinsic__typeof(floyd_runtime_t* frp, runtime_value_t arg0_value, runtime_type_t arg0_type){
	auto& r = get_floyd_runtime(frp);

#if DEBUG
	const auto& type0 = lookup_type_ref(r.backend, arg0_type);
	QUARK_ASSERT(type0.check_invariant());
#endif
	return arg0_type;
}



//??? optimize prio 1
//??? check type at compile time, not runtime.

static const runtime_value_t floyd_llvm_intrinsic__update(floyd_runtime_t* frp, runtime_value_t arg0_value, runtime_type_t arg0_type, runtime_value_t arg1_value, runtime_type_t arg1_type, runtime_value_t arg2_value, runtime_type_t arg2_type){
	auto& r = get_floyd_runtime(frp);

	const auto& type0 = lookup_type_ref(r.backend, arg0_type);
	const auto& type1 = lookup_type_ref(r.backend, arg1_type);
	const auto& type2 = lookup_type_ref(r.backend, arg2_type);
	if(type0.is_string()){
		return update__string(r.backend, arg0_value, arg0_type, arg1_value, arg1_type, arg2_value, arg2_type);
	}
	else if(is_vector_carray(itype_t(arg0_type))){
		return update__carray(r.backend, arg0_value, arg0_type, arg1_value, arg1_type, arg2_value, arg2_type);
	}
	else if(is_vector_hamt(itype_t(arg0_type))){
		return update__vector_hamt(r.backend, arg0_value, arg0_type, arg1_value, arg1_type, arg2_value, arg2_type);
	}
	else if(is_dict_cppmap(itype_t(arg0_type))){
		return update__dict_cppmap(r.backend, arg0_value, arg0_type, arg1_value, arg1_type, arg2_value, arg2_type);
	}
	else if(is_dict_hamt(itype_t(arg0_type))){
		return update__dict_hamt(r.backend, arg0_value, arg0_type, arg1_value, arg1_type, arg2_value, arg2_type);
	}
	else if(type0.is_struct()){
		QUARK_ASSERT(false);
	}
	else{
		//	No other types allowed.
		UNSUPPORTED();
	}
	throw std::exception();
}

static JSON_T* floyd_llvm_intrinsic__to_json(floyd_runtime_t* frp, runtime_value_t arg0_value, runtime_type_t arg0_type){
	auto& r = get_floyd_runtime(frp);

	const auto& type0 = lookup_type_ref(r.backend, arg0_type);
	const auto value0 = from_runtime_value(r, arg0_value, type0);
	const auto j = value_to_ast_json(value0, json_tags::k_plain);
	auto result = alloc_json(r.backend.heap, j);
	return result;
}





/////////////////////////////////////////		REGISTRY




std::map<std::string, void*> get_intrinsic_binds(){

	////////////////////////////////		CORE FUNCTIONS AND HOST FUNCTIONS
	const std::map<std::string, void*> host_functions_map = {

		////////////////////////////////		INTRINSICS

		{ "assert", reinterpret_cast<void *>(&floyd_llvm_intrinsic__assert) },
		{ "to_string", reinterpret_cast<void *>(&floyd_llvm_intrinsic__to_string) },
		{ "to_pretty_string", reinterpret_cast<void *>(&floyd_llvm_intrinsic__to_pretty_string) },
		{ "typeof", reinterpret_cast<void *>(&floyd_llvm_intrinsic__typeof) },

		{ "update", reinterpret_cast<void *>(&floyd_llvm_intrinsic__update) },
		{ "size", reinterpret_cast<void *>(&floyd_llvm_intrinsic__size) },
		{ "find", reinterpret_cast<void *>(&floyd_llvm_intrinsic__find) },
		{ "exists", reinterpret_cast<void *>(&floyd_llvm_intrinsic__exists) },
		{ "erase", reinterpret_cast<void *>(&floyd_llvm_intrinsic__erase) },
		{ "get_keys", reinterpret_cast<void *>(&floyd_llvm_intrinsic__get_keys) },
		{ "push_back", reinterpret_cast<void *>(&floyd_llvm_intrinsic__push_back) },
		{ "subset", reinterpret_cast<void *>(&floyd_llvm_intrinsic__subset) },
		{ "replace", reinterpret_cast<void *>(&floyd_llvm_intrinsic__replace) },

		{ "generate_json_script", reinterpret_cast<void *>(&floyd_llvm_intrinsic__generate_json_script) },
		{ "from_json", reinterpret_cast<void *>(&floyd_llvm_intrinsic__from_json) },
		{ "parse_json_script", reinterpret_cast<void *>(&floyd_llvm_intrinsic__parse_json_script) },
		{ "to_json", reinterpret_cast<void *>(&floyd_llvm_intrinsic__to_json) },

		{ "get_json_type", reinterpret_cast<void *>(&floyd_llvm_intrinsic__get_json_type) },

		{ "map", reinterpret_cast<void *>(&floyd_llvm_intrinsic__map) },
		{ "map_string", reinterpret_cast<void *>(&floyd_llvm_intrinsic__map_string) },
		{ "map_dag", reinterpret_cast<void *>(&floyd_llvm_intrinsic__map_dag) },
		{ "filter", reinterpret_cast<void *>(&floyd_llvm_intrinsic__filter) },
		{ "reduce", reinterpret_cast<void *>(&floyd_llvm_intrinsic__reduce) },
		{ "stable_sort", reinterpret_cast<void *>(&floyd_llvm_intrinsic__stable_sort) },

		{ "print", reinterpret_cast<void *>(&floyd_llvm_intrinsic__print) },
		{ "send", reinterpret_cast<void *>(&floyd_llvm_intrinsic__send) },

/*
		{ "bw_not", reinterpret_cast<void *>(&floyd_llvm_intrinsic__dummy) },
		{ "bw_and", reinterpret_cast<void *>(&floyd_llvm_intrinsic__dummy) },
		{ "bw_or", reinterpret_cast<void *>(&floyd_llvm_intrinsic__dummy) },
		{ "bw_xor", reinterpret_cast<void *>(&floyd_llvm_intrinsic__dummy) },
		{ "bw_shift_left", reinterpret_cast<void *>(&floyd_llvm_intrinsic__dummy) },
		{ "bw_shift_right", reinterpret_cast<void *>(&floyd_llvm_intrinsic__dummy) },
		{ "bw_shift_right_arithmetic", reinterpret_cast<void *>(&floyd_llvm_intrinsic__dummy) },
*/
	};
	return host_functions_map;
}

} // floyd
