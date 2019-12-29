//
//  floyd_llvm_intrinsics.cpp
//  floyd
//
//  Created by Marcus Zetterquist on 2019-08-28.
//  Copyright © 2019 Marcus Zetterquist. All rights reserved.
//

#include "floyd_llvm_intrinsics.h"

#include "floyd_intrinsics.h"
#include "floyd_llvm_helpers.h"
#include "floyd_llvm_runtime.h"
#include "floyd_llvm_codegen_basics.h"
#include "value_features.h"
#include "floyd_runtime.h"
#include "text_parser.h"
#include "utils.h"

namespace floyd {


static const bool k_trace_function_link_map = false;

struct specialization_t {
	eresolved_type required_arg_type;
	llvm_function_bind_t bind;
};


static const llvm_codegen_function_type_t& codegen_lookup_specialization(
	const config_t& config,
	const types_t& types,
	const std::vector<llvm_codegen_function_type_t>& link_map,
	const std::vector<specialization_t>& specialisations,
	const type_t& type
){
	QUARK_ASSERT(type.check_invariant());

	const auto it = std::find_if(
		specialisations.begin(),
		specialisations.end(),
		[&](const specialization_t& s) { return matches_specialization(config, types, s.required_arg_type, type); }
	);
	if(it == specialisations.end()){
		QUARK_ASSERT(false);
		throw std::exception();
	}
	const auto& res = find_function_def_from_link_name(link_map, it->bind.name);
	return res;
}


/////////////////////////////////////////		assert()



static void floyd_llvm_intrinsic__assert(floyd_runtime_t* frp, runtime_value_t arg){
	auto& backend = get_backend(frp);
	(void)backend;

//	QUARK_ASSERT(arg.bool_value == 0 || arg.bool_value == 1);

	bool ok = (arg.bool_value & 0x01) == 0 ? false : true;
	if(!ok){
		on_print(frp, "Assertion failed.");
		throw assert_failed_exception();
//		quark::throw_runtime_error("Assertion failed.");
	}
}



/////////////////////////////////////////		erase()



//??? all types are compile-time only.
static runtime_value_t floyd_llvm_intrinsic__erase(floyd_runtime_t* frp, runtime_value_t coll_value, runtime_type_t coll_type, runtime_value_t key_value, runtime_type_t key_type){
	auto& backend = get_backend(frp);

	const auto& types = backend.types;
	const auto& type0 = lookup_type_ref(backend, coll_type);
	const auto& type1 = lookup_type_ref(backend, key_type);
	QUARK_ASSERT(peek2(types, type0).is_dict());
	QUARK_ASSERT(peek2(types, type1).is_string());
	return erase_dict_value(backend, coll_value, coll_type, key_value, key_type);
}



/////////////////////////////////////////		get_keys()



//??? We need to figure out the return type *again*, knowledge we have already in semast.
static runtime_value_t floyd_llvm_intrinsic__get_keys(floyd_runtime_t* frp, runtime_value_t coll_value, runtime_type_t coll_type){
	auto& backend = get_backend(frp);

	const auto& types = backend.types;
	const auto& type0 = lookup_type_ref(backend, coll_type);
	QUARK_ASSERT(peek2(types, type0).is_dict());
	return get_keys(backend, coll_value, coll_type);
}



/////////////////////////////////////////		exists()



static uint32_t floyd_llvm_intrinsic__exists(floyd_runtime_t* frp, runtime_value_t coll_value, runtime_type_t coll_type, runtime_value_t value, runtime_type_t value_type){
	auto& backend = get_backend(frp);

	const auto& types = backend.types;
	const auto& type0 = lookup_type_ref(backend, coll_type);
//	const auto& type1 = lookup_type_ref(backend, value_type);
	QUARK_ASSERT(peek2(types, type0).is_dict());

	const bool result = exists_dict_value(backend, coll_value, coll_type, value, value_type);
	return result ? 1 : 0;
}



/////////////////////////////////////////		find()



static int64_t floyd_llvm_intrinsic__find(floyd_runtime_t* frp, runtime_value_t coll_value, runtime_type_t coll_type, const runtime_value_t value, runtime_type_t value_type){
	auto& backend = get_backend(frp);
	return find_vector_element(backend, coll_value, coll_type, value, value_type);
}



/////////////////////////////////////////		get_json_type()



static int64_t floyd_llvm_intrinsic__get_json_type(floyd_runtime_t* frp, runtime_value_t json0){
	auto& backend = get_backend(frp);
	(void)backend;
	QUARK_ASSERT(json0.json_ptr != nullptr);

	const auto& json = json0.json_ptr->get_json();
	const auto result = get_json_type(json);
	return result;
}



/////////////////////////////////////////		generate_json_script()



static runtime_value_t floyd_llvm_intrinsic__generate_json_script(floyd_runtime_t* frp, runtime_value_t json0){
	auto& backend = get_backend(frp);
	QUARK_ASSERT(json0.json_ptr != nullptr);

	const auto& json = json0.json_ptr->get_json();
	const std::string s = json_to_compact_string(json);
	return to_runtime_string2(backend, s);
}



/////////////////////////////////////////		from_json()



static runtime_value_t floyd_llvm_intrinsic__from_json(floyd_runtime_t* frp, runtime_value_t json0, runtime_type_t target_type){
	auto& backend = get_backend(frp);
	QUARK_ASSERT(json0.json_ptr != nullptr);

	const auto& json = json0.json_ptr->get_json();
	const auto& target_type2 = lookup_type_ref(backend, target_type);

	const auto result = unflatten_json_to_specific_type(backend.types, json, target_type2);
	return to_runtime_value2(backend, result);
}




/////////////////////////////////////////		map()




//??? Use C++ template to generate these two functions.
typedef runtime_value_t (*MAP_F)(floyd_runtime_t* frp, runtime_value_t e_value, runtime_value_t context_value);

static runtime_value_t map__carray(floyd_runtime_t* frp, runtime_value_t elements_vec, runtime_type_t elements_vec_type, runtime_value_t f_value, runtime_type_t f_type, runtime_value_t context_value, runtime_type_t context_type, runtime_type_t result_vec_type){
	auto& backend = get_backend(frp);
	const auto& types = backend.types;

	QUARK_ASSERT(backend.check_invariant());

#if DEBUG
	const auto& type1 = lookup_type_ref(backend, f_type);

//	const auto& type0 = lookup_type_ref(backend, elements_vec_type);
//	const auto& type2 = lookup_type_ref(backend, context_type);
//	QUARK_ASSERT(check_map_func_type(type0, type1, type2));

//	const auto e_type = peek2(types, type0).get_vector_element_type(types);
	const auto f_arg_types = peek2(types, type1).get_function_args(types);
#endif

	const auto& func_link = lookup_func_link_required(backend, f_value);
	const auto f = reinterpret_cast<MAP_F>(func_link.f);

	const auto count = elements_vec.vector_carray_ptr->get_element_count();
	auto result_vec = alloc_vector_carray(backend.heap, count, count, type_t(result_vec_type));
	for(int i = 0 ; i < count ; i++){
		const auto a = (*f)(frp, elements_vec.vector_carray_ptr->get_element_ptr()[i], context_value);
		result_vec.vector_carray_ptr->get_element_ptr()[i] = a;
	}
	return result_vec;
}

//??? Update 1 element in a big hamt will copy the entire hamt, inc RC on all elements in hamt2. This is not needed since most of hamt is shared. Cheaper if we build in RC for leaf in the hamt itself.
//??? Use batching to speed up hamt creation. Add 32 nodes at a time. Also faster read iteration.
static runtime_value_t map__hamt(floyd_runtime_t* frp, runtime_value_t elements_vec, runtime_type_t elements_vec_type, runtime_value_t f_value, runtime_type_t f_type, runtime_value_t context_value, runtime_type_t context_type, runtime_type_t result_vec_type){
	auto& backend = get_backend(frp);
	QUARK_ASSERT(backend.check_invariant());
	const auto& types = backend.types;

#if DEBUG
	const auto& type1 = lookup_type_ref(backend, f_type);

//	const auto& type0 = lookup_type_ref(backend, elements_vec_type);
//	const auto& type2 = lookup_type_ref(backend, context_type);
//	QUARK_ASSERT(check_map_func_type(type0, type1, type2));

//	const auto e_type = peek2(types, type0).get_vector_element_type(types);
	const auto f_arg_types = peek2(types, type1).get_function_args(types);
#endif

	const auto& func_link = lookup_func_link_required(backend, f_value);
	const auto f = reinterpret_cast<MAP_F>(func_link.f);

	const auto count = elements_vec.vector_hamt_ptr->get_element_count();
	auto result_vec = alloc_vector_hamt(backend.heap, count, count, type_t(result_vec_type));
	for(int i = 0 ; i < count ; i++){
		const auto& element = elements_vec.vector_hamt_ptr->load_element(i);
		const auto a = (*f)(frp, element, context_value);
		result_vec.vector_hamt_ptr->store_mutate(i, a);
	}
	return result_vec;
}

//	[R] map([E] elements, func R (E e, C context) f, C context)
static std::vector<specialization_t> make_map_specializations(llvm::LLVMContext& context, const llvm_type_lookup& type_lookup){
	llvm::FunctionType* function_type = llvm::FunctionType::get(
		make_generic_vec_type_byvalue(type_lookup)->getPointerTo(),
		{
			make_frp_type(type_lookup),

			make_generic_vec_type_byvalue(type_lookup)->getPointerTo(),
			make_runtime_type_type(type_lookup),

			make_runtime_value_type(type_lookup),
			make_runtime_type_type(type_lookup),

			make_runtime_value_type(type_lookup),
			make_runtime_type_type(type_lookup),

			make_runtime_type_type(type_lookup)
		},
		false
	);
	return {
		specialization_t { eresolved_type::k_vector_carray_pod,		{ module_symbol_t("map_carray_pod"), function_type, reinterpret_cast<void*>(map__carray) } },
		specialization_t { eresolved_type::k_vector_carray_nonpod,	{ module_symbol_t("map_carray_nonpod"), function_type, reinterpret_cast<void*>(map__carray) } },
		specialization_t { eresolved_type::k_vector_hamt_pod,		{ module_symbol_t("map_hamt_pod"), function_type, reinterpret_cast<void*>(map__hamt) } },
		specialization_t { eresolved_type::k_vector_hamt_nonpod, 	{ module_symbol_t("map_hamt_nonpod"), function_type, reinterpret_cast<void*>(map__hamt) } }
	};
}

llvm::Value* generate_instrinsic_map(
	llvm_function_generator_t& gen_acc,
	const type_t& resolved_call_type,
	llvm::Value& elements_vec_reg,
	const type_t& elements_vec_type,
	llvm::Value& f_reg,
	const type_t& f_type,
	llvm::Value& context_reg,
	const type_t& context_type)
{
	QUARK_ASSERT(gen_acc.check_invariant());
	QUARK_ASSERT(elements_vec_type.check_invariant());

	const auto& types = gen_acc.gen.type_lookup.state.types;
	auto& builder = gen_acc.get_builder();
	const auto res = codegen_lookup_specialization(gen_acc.gen.settings.config, types, gen_acc.gen.link_map, make_map_specializations(builder.getContext(), gen_acc.gen.type_lookup), elements_vec_type);

	const auto result_vec_type = peek2(types, resolved_call_type).get_function_return(types);
	return builder.CreateCall(
		res.llvm_codegen_f,
		{
			gen_acc.get_callers_fcp(),

			&elements_vec_reg,
			generate_itype_constant(gen_acc.gen, elements_vec_type),

			generate_cast_to_runtime_value(gen_acc.gen, f_reg, f_type),
			generate_itype_constant(gen_acc.gen, f_type),

			generate_cast_to_runtime_value(gen_acc.gen, context_reg, context_type),
			generate_itype_constant(gen_acc.gen, context_type),

			generate_itype_constant(gen_acc.gen, result_vec_type)
		},
		""
	);
}


/////////////////////////////////////////		map_dag()


// [R] map_dag([E] elements, [int] depends_on, func R (E, [R], C context) f, C context)

typedef runtime_value_t (*map_dag_F)(floyd_runtime_t* frp, runtime_value_t r_value, runtime_value_t r_vec_value, runtime_value_t context_value);

static runtime_value_t map_dag__carray(
	floyd_runtime_t* frp,
	runtime_value_t elements_vec,
	runtime_type_t elements_vec_type,
	runtime_value_t depends_on_vec,
	runtime_type_t depends_on_vec_type,
	runtime_value_t f_value,
	runtime_type_t f_value_type,
	runtime_value_t context,
	runtime_type_t context_type
){
	auto& backend = get_backend(frp);
	const auto& types = backend.types;
	const auto& type0 = lookup_type_ref(backend, elements_vec_type);
//	const auto& type1 = lookup_type_ref(backend, depends_on_vec_type);
	const auto& type2 = lookup_type_ref(backend, f_value_type);
//	QUARK_ASSERT(check_map_dag_func_type(type0, type1, type2, lookup_type_ref(backend, context_type)));

	const auto& elements = elements_vec;
#if DEBUG
	const auto& e_type = peek2(types, type0).get_vector_element_type(types);
#endif
	const auto& parents = depends_on_vec;
	const auto& f = f_value;
	const auto& r_type = peek2(types, type2).get_function_return(types);

	QUARK_ASSERT(e_type == peek2(types, type2).get_function_args(types)[0] && r_type == peek2(types, peek2(types, type2).get_function_args(types)[1]).get_vector_element_type(types));

//	QUARK_ASSERT(is_vector_carray(make_vector(e_type)));
//	QUARK_ASSERT(is_vector_carray(make_vector(r_type)));

	const auto return_type = type_t::make_vector(types, r_type);

	const auto& func_link = lookup_func_link_required(backend, f);
	const auto f2 = reinterpret_cast<map_dag_F>(func_link.f);

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

			auto solved_deps2 = alloc_vector_carray(backend.heap, solved_deps.size(), solved_deps.size(), return_type);
			for(int i = 0 ; i < solved_deps.size() ; i++){
				solved_deps2.vector_carray_ptr->store(i, solved_deps[i]);
			}
			runtime_value_t solved_deps3 = solved_deps2;

			const auto result1 = (*f2)(frp, e, solved_deps3, context);

			//	Warning: optimization trick.
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
	auto result_vec = alloc_vector_carray(backend.heap, count, count, return_type);
	for(int i = 0 ; i < count ; i++){
//		retain_value(r, complete[i], r_type);
		result_vec.vector_carray_ptr->store(i, complete[i]);
	}

	return result_vec;
}

static runtime_value_t map_dag__hamt(
	floyd_runtime_t* frp,
	runtime_value_t elements_vec,
	runtime_type_t elements_vec_type,
	runtime_value_t depends_on_vec,
	runtime_type_t depends_on_vec_type,
	runtime_value_t f_value,
	runtime_type_t f_value_type,
	runtime_value_t context,
	runtime_type_t context_type
){
	auto& backend = get_backend(frp);

	const auto& types = backend.types;
	const auto& type0 = lookup_type_ref(backend, elements_vec_type);
//	const auto& type1 = lookup_type_ref(backend, depends_on_vec_type);
	const auto& type2 = lookup_type_ref(backend, f_value_type);
//	QUARK_ASSERT(check_map_dag_func_type(type0, type1, type2, lookup_type_ref(backend, context_type)));

	const auto& elements = elements_vec;
#if DEBUG
	const auto& e_type = peek2(types, type0).get_vector_element_type(types);
#endif
	const auto& parents = depends_on_vec;
	const auto& f = f_value;
	const auto& r_type = peek2(types, type2).get_function_return(types);

	QUARK_ASSERT(e_type == peek2(types, type2).get_function_args(types)[0] && r_type == peek2(types, peek2(types, type2).get_function_args(types)[1]).get_vector_element_type(types));

//	QUARK_ASSERT(is_vector_hamt(make_vector(e_type)));
//	QUARK_ASSERT(is_vector_hamt(make_vector(r_type)));

	const auto return_type = type_t::make_vector(types, r_type);

	const auto& func_link = lookup_func_link_required(backend, f);
	const auto f2 = reinterpret_cast<map_dag_F>(func_link.f);

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

			auto solved_deps2 = alloc_vector_hamt(backend.heap, solved_deps.size(), solved_deps.size(), return_type);
			for(int i = 0 ; i < solved_deps.size() ; i++){
				solved_deps2.vector_hamt_ptr->store_mutate(i, solved_deps[i]);
			}
			runtime_value_t solved_deps3 = solved_deps2;

			const auto result1 = (*f2)(frp, e, solved_deps3, context);

			//	Warning: optimization trick.
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
	auto result_vec = alloc_vector_hamt(backend.heap, count, count, return_type);
	for(int i = 0 ; i < count ; i++){
//		retain_value(r, complete[i], r_type);
		result_vec.vector_hamt_ptr->store_mutate(i, complete[i]);
	}

	return result_vec;
}

// ??? check type at compile time, not runtime.
// [R] map_dag([E] elements, [int] depends_on, func R (E, [R], C context) f, C context)
static runtime_value_t floyd_llvm_intrinsic__map_dag(
	floyd_runtime_t* frp,
	runtime_value_t elements_vec,
	runtime_type_t elements_vec_type,
	runtime_value_t depends_on_vec,
	runtime_type_t depends_on_vec_type,
	runtime_value_t f_value,
	runtime_type_t f_value_type,
	runtime_value_t context,
	runtime_type_t context_type
){
	auto& backend = get_backend(frp);

//	const auto& type0 = lookup_type_ref(backend, elements_vec_type);
	if(is_vector_carray(backend.types, backend.config, type_t(elements_vec_type))){
		return map_dag__carray(frp, elements_vec, elements_vec_type, depends_on_vec, depends_on_vec_type, f_value, f_value_type, context, context_type);
	}
	else if(is_vector_hamt(backend.types, backend.config, type_t(elements_vec_type))){
		return map_dag__hamt(frp, elements_vec, elements_vec_type, depends_on_vec, depends_on_vec_type, f_value, f_value_type, context, context_type);
	}
	else{
		QUARK_ASSERT(false);
		throw std::exception();
	}
}



/////////////////////////////////////////		filter()



typedef runtime_value_t (*FILTER_F)(floyd_runtime_t* frp, runtime_value_t element_value, runtime_value_t context);

static runtime_value_t filter__carray(floyd_runtime_t* frp, runtime_value_t elements_vec, runtime_type_t elements_vec_type, runtime_value_t f_value, runtime_type_t f_value_type, runtime_value_t context, runtime_type_t context_type){
	auto& backend = get_backend(frp);

	const auto& type0 = lookup_type_ref(backend, elements_vec_type);
//	const auto& type1 = lookup_type_ref(backend, f_value_type);
//	const auto& type2 = lookup_type_ref(backend, context_type);
	const auto& return_type = type0;

//	QUARK_ASSERT(check_filter_func_type(type0, type1, type2));
	QUARK_ASSERT(is_vector_carray(backend.types, backend.config, type_t(elements_vec_type)));

	const auto& vec = *elements_vec.vector_carray_ptr;

	const auto& func_link = lookup_func_link_required(backend, f_value);
	const auto f = reinterpret_cast<FILTER_F>(func_link.f);

	auto count = vec.get_element_count();

	const auto e_element_itype = lookup_vector_element_type(backend, type_t(elements_vec_type));

	std::vector<runtime_value_t> acc;
	for(int i = 0 ; i < count ; i++){
		const auto element_value = vec.get_element_ptr()[i];
		const auto keep = (*f)(frp, element_value, context);
		if(keep.bool_value != 0){
			acc.push_back(element_value);

			if(is_rc_value(backend.types, e_element_itype)){
				retain_value(backend, element_value, e_element_itype);
			}
		}
		else{
		}
	}

	const auto count2 = acc.size();
	auto result_vec = alloc_vector_carray(backend.heap, count2, count2, return_type);

	if(count2 > 0){
		//	Count > 0 required to get address to first element in acc.
		copy_elements(result_vec.vector_carray_ptr->get_element_ptr(), &acc[0], count2);
	}
	return result_vec;
}

static runtime_value_t filter__hamt(floyd_runtime_t* frp, runtime_value_t elements_vec, runtime_type_t elements_vec_type, runtime_value_t f_value, runtime_type_t f_value_type, runtime_value_t context, runtime_type_t context_type){
	auto& backend = get_backend(frp);

	const auto& type0 = lookup_type_ref(backend, elements_vec_type);
//	const auto& type1 = lookup_type_ref(backend, f_value_type);
//	const auto& type2 = lookup_type_ref(backend, context_type);
	const auto& return_type = type0;

//	QUARK_ASSERT(check_filter_func_type(type0, type1, type2));
	QUARK_ASSERT(is_vector_hamt(backend.types, backend.config, type_t(elements_vec_type)));

	const auto& vec = *elements_vec.vector_hamt_ptr;

	const auto& func_link = lookup_func_link_required(backend, f_value);
	const auto f = reinterpret_cast<FILTER_F>(func_link.f);

	auto count = vec.get_element_count();

	const auto e_element_itype = lookup_vector_element_type(backend, type_t(elements_vec_type));

	std::vector<runtime_value_t> acc;
	for(int i = 0 ; i < count ; i++){
		const auto element_value = vec.load_element(i);
		const auto keep = (*f)(frp, element_value, context);
		if(keep.bool_value != 0){
			acc.push_back(element_value);

			if(is_rc_value(backend.types, e_element_itype)){
				retain_value(backend, element_value, e_element_itype);
			}
		}
		else{
		}
	}

	const auto count2 = acc.size();
	auto result_vec = alloc_vector_hamt(backend.heap, &acc[0], count2, return_type);
	return result_vec;
}

//??? check type at compile time, not runtime.
//	[E] filter([E] elements, func bool (E e, C context) f, C context)
static runtime_value_t floyd_llvm_intrinsic__filter(floyd_runtime_t* frp, runtime_value_t elements_vec, runtime_type_t elements_vec_type, runtime_value_t f_value, runtime_type_t f_value_type, runtime_value_t arg2_value, runtime_type_t arg2_type){
	auto& backend = get_backend(frp);

//	const auto& type0 = lookup_type_ref(backend, elements_vec_type);
	if(is_vector_carray(backend.types, backend.config, type_t(elements_vec_type))){
		return filter__carray(frp, elements_vec, elements_vec_type, f_value, f_value_type, arg2_value, arg2_type);
	}
	else if(is_vector_hamt(backend.types, backend.config, type_t(elements_vec_type))){
		return filter__hamt(frp, elements_vec, elements_vec_type, f_value, f_value_type, arg2_value, arg2_type);
	}
	else{
		QUARK_ASSERT(false);
		throw std::exception();
	}
}



/////////////////////////////////////////		reduce()



typedef runtime_value_t (*REDUCE_F)(floyd_runtime_t* frp, runtime_value_t acc_value, runtime_value_t element_value, runtime_value_t context);

static runtime_value_t reduce__carray(floyd_runtime_t* frp, runtime_value_t elements_vec, runtime_type_t elements_vec_type, runtime_value_t init_value, runtime_type_t init_value_type, runtime_value_t f_value, runtime_type_t f_type, runtime_value_t context, runtime_type_t context_type){
	auto& backend = get_backend(frp);

//	const auto& type0 = lookup_type_ref(backend, elements_vec_type);
//	const auto& type1 = lookup_type_ref(backend, init_value_type);
//	const auto& type2 = lookup_type_ref(backend, f_type);

//	QUARK_ASSERT(check_reduce_func_type(type0, type1, type2, lookup_type_ref(backend, f_type)));
	QUARK_ASSERT(is_vector_carray(backend.types, backend.config, type_t(elements_vec_type)));

	const auto& vec = *elements_vec.vector_carray_ptr;
	const auto& init = init_value;

	const auto& func_link = lookup_func_link_required(backend, f_value);
	const auto f = reinterpret_cast<REDUCE_F>(func_link.f);

	auto count = vec.get_element_count();
	runtime_value_t acc = init;
	retain_value(backend, acc, type_t(init_value_type));

	for(int i = 0 ; i < count ; i++){
		const auto element_value = vec.get_element_ptr()[i];
		const auto acc2 = (*f)(frp, acc, element_value, context);

		if(is_rc_value(backend.types, type_t(init_value_type))){
			release_value(backend, acc, type_t(init_value_type));
		}
		acc = acc2;
	}
	return acc;
}

static runtime_value_t reduce__hamt(floyd_runtime_t* frp, runtime_value_t elements_vec, runtime_type_t elements_vec_type, runtime_value_t init_value, runtime_type_t init_value_type, runtime_value_t f_value, runtime_type_t f_type, runtime_value_t context, runtime_type_t context_type){
	auto& backend = get_backend(frp);

//	const auto& type0 = lookup_type_ref(backend, elements_vec_type);
//	const auto& type1 = lookup_type_ref(backend, init_value_type);
//	const auto& type2 = lookup_type_ref(backend, f_type);

//	QUARK_ASSERT(check_reduce_func_type(type0, type1, type2, lookup_type_ref(backend, f_type)));
	QUARK_ASSERT(is_vector_hamt(backend.types, backend.config, type_t(elements_vec_type)));

	const auto& vec = *elements_vec.vector_hamt_ptr;
	const auto& init = init_value;

	const auto& func_link = lookup_func_link_required(backend, f_value);
	const auto f = reinterpret_cast<REDUCE_F>(func_link.f);

	auto count = vec.get_element_count();
	runtime_value_t acc = init;
	retain_value(backend, acc, type_t(init_value_type));

	for(int i = 0 ; i < count ; i++){
		const auto element_value = vec.load_element(i);
		const auto acc2 = (*f)(frp, acc, element_value, context);

		if(is_rc_value(backend.types, type_t(init_value_type))){
			release_value(backend, acc, type_t(init_value_type));
		}
		acc = acc2;
	}
	return acc;
}

//	R reduce([E] elements, R accumulator_init, func R (R accumulator, E element, C context) f, C context)

//??? check type at compile time, not runtime.
static runtime_value_t floyd_llvm_intrinsic__reduce(floyd_runtime_t* frp, runtime_value_t elements_vec, runtime_type_t elements_vec_type, runtime_value_t init_value, runtime_type_t init_value_type, runtime_value_t f_value, runtime_type_t f_type, runtime_value_t context, runtime_type_t context_type){
	auto& backend = get_backend(frp);

	if(is_vector_carray(backend.types, backend.config, type_t(elements_vec_type))){
		return reduce__carray(frp, elements_vec, elements_vec_type, init_value, init_value_type, f_value, f_type, context, context_type);
	}
	else if(is_vector_hamt(backend.types, backend.config, type_t(elements_vec_type))){
		return reduce__hamt(frp, elements_vec, elements_vec_type, init_value, init_value_type, f_value, f_type, context, context_type);
	}
	else{
		QUARK_ASSERT(false);
		throw std::exception();
	}
}


/////////////////////////////////////////		stable_sort()



typedef uint8_t (*stable_sort_F)(floyd_runtime_t* frp, runtime_value_t left_value, runtime_value_t right_value, runtime_value_t context_value);

static runtime_value_t stable_sort__carray(
	floyd_runtime_t* frp,
	runtime_value_t elements_vec,
	runtime_type_t elements_vec_type,
	runtime_value_t f_value,
	runtime_type_t f_value_type,
	runtime_value_t context_value,
	runtime_type_t context_value_type
){
	auto& backend = get_backend(frp);

	const auto& types = backend.types;

	const auto& type0 = lookup_type_ref(backend, elements_vec_type);
//	const auto& type1 = lookup_type_ref(backend, f_value_type);
//	const auto& type2 = lookup_type_ref(backend, context_value_type);

//	QUARK_ASSERT(check_stable_sort_func_type(type0, type1, type2));
	QUARK_ASSERT(is_vector_carray(types, backend.config, type_t(elements_vec_type)));

	const auto& elements = elements_vec;
	const auto& f = f_value;
	const auto& context = context_value;

	const auto elements2 = from_runtime_value2(backend, elements, type0);

	const auto& func_link = lookup_func_link_required(backend, f);
	const auto f2 = reinterpret_cast<stable_sort_F>(func_link.f);

	struct sort_functor_r {
		bool operator() (const value_t &a, const value_t &b) {
			auto& backend = get_backend(frp);

			const auto left = to_runtime_value2(backend, a);
			const auto right = to_runtime_value2(backend, b);
			uint8_t result = (*f)(frp, left, right, context);

			release_value(backend, left, a.get_type());
			release_value(backend, right, b.get_type());

			return result == 1 ? true : false;
		}

		floyd_runtime_t* frp;
		runtime_value_t context;
		stable_sort_F f;
	};

	const sort_functor_r sort_functor { frp, context, f2 };

	auto mutate_inplace_elements = elements2.get_vector_value();
	std::stable_sort(mutate_inplace_elements.begin(), mutate_inplace_elements.end(), sort_functor);
	return to_runtime_value2(backend, value_t::make_vector_value(types, peek2(types, type0).get_vector_element_type(types), mutate_inplace_elements));
}

static runtime_value_t stable_sort__hamt(
	floyd_runtime_t* frp,
	runtime_value_t elements_vec,
	runtime_type_t elements_vec_type,
	runtime_value_t f_value,
	runtime_type_t f_value_type,
	runtime_value_t context_value,
	runtime_type_t context_value_type
){
	auto& backend = get_backend(frp);

	const auto& types = backend.types;

	const auto& type0 = lookup_type_ref(backend, elements_vec_type);
//	const auto& type1 = lookup_type_ref(backend, f_value_type);
//	const auto& type2 = lookup_type_ref(backend, context_value_type);

//	QUARK_ASSERT(check_stable_sort_func_type(type0, type1, type2));
	QUARK_ASSERT(is_vector_hamt(types, backend.config, type_t(elements_vec_type)));

	const auto& elements = elements_vec;
	const auto& f = f_value;
	const auto& context = context_value;

	const auto elements2 = from_runtime_value2(backend, elements, type0);

	const auto& func_link = lookup_func_link_required(backend, f);
	const auto f2 = reinterpret_cast<stable_sort_F>(func_link.f);

	struct sort_functor_r {
		bool operator() (const value_t &a, const value_t &b) {
			auto& backend = get_backend(frp);

			const auto left = to_runtime_value2(backend, a);
			const auto right = to_runtime_value2(backend, b);
			uint8_t result = (*f)(frp, left, right, context);

			release_value(backend, left, a.get_type());
			release_value(backend, right, b.get_type());

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
	const auto result = to_runtime_value2(backend, value_t::make_vector_value(types, peek2(types, type0).get_vector_element_type(types), mutate_inplace_elements));
	return result;
}

//	[T] stable_sort([T] elements, bool less(T left, T right, C context), C context)

//??? check type at compile time, not runtime.
static runtime_value_t floyd_llvm_intrinsic__stable_sort(
	floyd_runtime_t* frp,
	runtime_value_t elements_vec,
	runtime_type_t elements_vec_type,
	runtime_value_t f_value,
	runtime_type_t f_value_type,
	runtime_value_t context_value,
	runtime_type_t context_value_type
){
	auto& backend = get_backend(frp);

//	const auto& type0 = lookup_type_ref(r.backend, elements_vec_type);
	if(is_vector_carray(backend.types, backend.config, type_t(elements_vec_type))){
		return stable_sort__carray(frp, elements_vec, elements_vec_type, f_value, f_value_type, context_value, context_value_type);
	}
	else if(is_vector_hamt(backend.types, backend.config, type_t(elements_vec_type))){
		return stable_sort__hamt(frp, elements_vec, elements_vec_type, f_value, f_value_type, context_value, context_value_type);
	}
	else{
		QUARK_ASSERT(false);
		throw std::exception();
	}
}





////////////////////////////////	push_back()




static runtime_value_t floydrt_push_back__string(floyd_runtime_t* frp, runtime_value_t vec, runtime_type_t vec_type, runtime_value_t element){
	auto& backend = get_backend(frp);

	return push_back_vector_element__string(backend, vec, type_t(vec_type), element);
}

static runtime_value_t floydrt_push_back_carray_pod(floyd_runtime_t* frp, runtime_value_t vec, runtime_type_t vec_type, runtime_value_t element){
	auto& backend = get_backend(frp);

	return push_back_vector_element__carray_pod(backend, vec, type_t(vec_type), element);
}

static runtime_value_t floydrt_push_back_carray_nonpod(floyd_runtime_t* frp, runtime_value_t vec, runtime_type_t vec_type, runtime_value_t element){
	auto& backend = get_backend(frp);

	return push_back_vector_element__carray_nonpod(backend, vec, type_t(vec_type), element);
}

static runtime_value_t floydrt_push_back_hamt_pod(floyd_runtime_t* frp, runtime_value_t vec, runtime_type_t vec_type, runtime_value_t element){
	auto& backend = get_backend(frp);

	return push_back_vector_element__hamt_pod(backend, vec, type_t(vec_type), element);
}

static runtime_value_t floydrt_push_back_hamt_nonpod(floyd_runtime_t* frp, runtime_value_t vec, runtime_type_t vec_type, runtime_value_t element){
	auto& backend = get_backend(frp);

	return push_back_vector_element__hamt_nonpod(backend, vec, type_t(vec_type), element);
}


static std::vector<specialization_t> make_push_back_specializations(llvm::LLVMContext& context, const llvm_type_lookup& type_lookup){
	llvm::FunctionType* function_type = llvm::FunctionType::get(
		make_generic_vec_type_byvalue(type_lookup)->getPointerTo(),
		{
			make_frp_type(type_lookup),
			make_generic_vec_type_byvalue(type_lookup)->getPointerTo(),
			make_runtime_type_type(type_lookup),
			make_runtime_value_type(type_lookup)
		},
		false
	);
	return {
//		specialization_t { { "push_back", make_intrinsic_llvm_function_type(type_lookup, make_push_back_signature()), reinterpret_cast<void*>(floyd_llvm_intrinsic__push_back) }, xx),
		specialization_t { eresolved_type::k_string,				{ module_symbol_t("push_back__string"), function_type, reinterpret_cast<void*>(floydrt_push_back__string) } },
		specialization_t { eresolved_type::k_vector_carray_pod,		{ module_symbol_t("push_back_carray_pod"), function_type, reinterpret_cast<void*>(floydrt_push_back_carray_pod) } },
		specialization_t { eresolved_type::k_vector_carray_nonpod,	{ module_symbol_t("push_back_carray_nonpod"), function_type, reinterpret_cast<void*>(floydrt_push_back_carray_nonpod) } },
		specialization_t { eresolved_type::k_vector_hamt_pod,		{ module_symbol_t("push_back_hamt_pod"), function_type, reinterpret_cast<void*>(floydrt_push_back_hamt_pod) } },
		specialization_t { eresolved_type::k_vector_hamt_nonpod, 	{ module_symbol_t("push_back_hamt_nonpod"), function_type, reinterpret_cast<void*>(floydrt_push_back_hamt_nonpod) } }
	};
}

llvm::Value* generate_instrinsic_push_back(llvm_function_generator_t& gen_acc, const type_t& resolved_call_type, llvm::Value& collection_reg, const type_t& collection_type, llvm::Value& value_reg){
	QUARK_ASSERT(gen_acc.check_invariant());
	QUARK_ASSERT(collection_type.check_invariant());

	auto& builder = gen_acc.get_builder();
	const auto& types = gen_acc.gen.type_lookup.state.types;

	const auto res = codegen_lookup_specialization(gen_acc.gen.settings.config, types, gen_acc.gen.link_map, make_push_back_specializations(builder.getContext(), gen_acc.gen.type_lookup), collection_type);
	const auto collection_type_peek = peek2(types, collection_type);

	if(collection_type_peek.is_string()){
		const auto vector_itype_reg = generate_itype_constant(gen_acc.gen, collection_type);
		const auto packed_value_reg = generate_cast_to_runtime_value(gen_acc.gen, value_reg, type_t::make_int());
		return builder.CreateCall(
			res.llvm_codegen_f,
			{ gen_acc.get_callers_fcp(), &collection_reg, vector_itype_reg, packed_value_reg },
			""
		);
	}
	else if(collection_type_peek.is_vector()){
		const auto element_type = collection_type_peek.get_vector_element_type(types);
		const auto vector_itype_reg = generate_itype_constant(gen_acc.gen, collection_type);
		const auto packed_value_reg = generate_cast_to_runtime_value(gen_acc.gen, value_reg, element_type);
		return builder.CreateCall(
			res.llvm_codegen_f,
			{ gen_acc.get_callers_fcp(), &collection_reg, vector_itype_reg, packed_value_reg },
			""
		);
	}
	else{
		QUARK_ASSERT(false);
		throw std::exception();
	}
}




////////////////////////////////	replace()



//??? check type at compile time, not runtime.

//	replace(VECTOR s, int start, int end, VECTOR new)	
static const runtime_value_t floyd_llvm_intrinsic__replace(floyd_runtime_t* frp, runtime_value_t elements_vec, runtime_type_t elements_vec_type, uint64_t start, uint64_t end, runtime_value_t arg3_value, runtime_type_t arg3_type){
	auto& backend = get_backend(frp);

	const auto& type0 = lookup_type_ref(backend, elements_vec_type);
	const auto& type3 = lookup_type_ref(backend, arg3_type);

	QUARK_ASSERT(type3 == type0);

	if(peek2(backend.types, type0).is_string()){
		return replace_vector_range__string(backend, elements_vec, elements_vec_type, start, end, arg3_value, arg3_type);
	}
	else if(is_vector_carray(backend.types, backend.config, type_t(elements_vec_type))){
		return replace_vector_range__carray(backend, elements_vec, elements_vec_type, start, end, arg3_value, arg3_type);
	}
	else if(is_vector_hamt(backend.types, backend.config, type_t(elements_vec_type))){
		return replace_vector_range__hamt(backend, elements_vec, elements_vec_type, start, end, arg3_value, arg3_type);
	}
	else{
		//	No other types allowed.
		UNSUPPORTED();
	}
}




////////////////////////////////	parse_json_script()




static runtime_value_t floyd_llvm_intrinsic__parse_json_script(floyd_runtime_t* frp, runtime_value_t string_s0){
	auto& backend = get_backend(frp);

	const auto string_s = from_runtime_string2(backend, string_s0);

	std::pair<json_t, seq_t> result0 = parse_json(seq_t(string_s));
	return alloc_json(backend.heap, result0.first);
}





////////////////////////////////	size()



static int64_t size__string(floyd_runtime_t* frp, runtime_value_t vec, runtime_type_t vec_type){
	auto& backend = get_backend(frp);

#if DEBUG
	const auto& type0 = lookup_type_ref(backend, vec_type);
	QUARK_ASSERT(peek2(backend.types, type0).is_string());
#endif

	return vec.vector_carray_ptr->get_element_count();
}

static int64_t size_vector_carray(floyd_runtime_t* frp, runtime_value_t collection, runtime_type_t collection_type){
	auto& backend = get_backend(frp);
	(void)backend;
	return collection.vector_carray_ptr->get_element_count();
}
static int64_t size_vector_hamt(floyd_runtime_t* frp, runtime_value_t collection, runtime_type_t collection_type){
	auto& backend = get_backend(frp);
	(void)backend;
	return collection.vector_hamt_ptr->get_element_count();
}
static int64_t size_dict_cppmap(floyd_runtime_t* frp, runtime_value_t collection, runtime_type_t collection_type){
	auto& backend = get_backend(frp);
	(void)backend;
	return collection.dict_cppmap_ptr->size();
}
static int64_t size_dict_hamt(floyd_runtime_t* frp, runtime_value_t collection, runtime_type_t collection_type){
	auto& backend = get_backend(frp);
	(void)backend;
	return collection.dict_hamt_ptr->size();
}
static int64_t size_json(floyd_runtime_t* frp, runtime_value_t collection, runtime_type_t collection_type){
	auto& backend = get_backend(frp);
	(void)backend;

	const auto& json = collection.json_ptr->get_json();

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

static std::vector<specialization_t> make_size_specializations(llvm::LLVMContext& context, const llvm_type_lookup& type_lookup){
	llvm::FunctionType* function_type1 = llvm::FunctionType::get(
		llvm::Type::getInt64Ty(context),
		{
			make_frp_type(type_lookup),
			make_generic_vec_type_byvalue(type_lookup)->getPointerTo(),
			make_runtime_type_type(type_lookup)
		},
		false
	);
	llvm::FunctionType* function_type2 = llvm::FunctionType::get(
		llvm::Type::getInt64Ty(context),
		{
			make_frp_type(type_lookup),
			make_generic_dict_type_byvalue(type_lookup)->getPointerTo(),
			make_runtime_type_type(type_lookup)
		},
		false
	);
	llvm::FunctionType* function_type3 = llvm::FunctionType::get(
		llvm::Type::getInt64Ty(context),
		{
			make_frp_type(type_lookup),
			make_json_type_byvalue(type_lookup)->getPointerTo(),
			make_runtime_type_type(type_lookup)
		},
		false
	);
	return {
//		specialization_t { eresolved_type::k_string,					{ "size", make_intrinsic_llvm_function_type(type_lookup, make_size_signature()), reinterpret_cast<void*>(floyd_llvm_intrinsic__size) },
		specialization_t { eresolved_type::k_string,					{ module_symbol_t("size__string"), function_type1, reinterpret_cast<void*>(size__string) } },

		specialization_t { eresolved_type::k_vector_carray_pod,			{ module_symbol_t("size_vector_carray"), function_type1, reinterpret_cast<void*>(size_vector_carray) } },
		specialization_t { eresolved_type::k_vector_carray_nonpod,		{ module_symbol_t("size_vector_carray"), function_type1, reinterpret_cast<void*>(size_vector_carray) } },
		specialization_t { eresolved_type::k_vector_hamt_pod,			{ module_symbol_t("size_vector_hamt"), function_type1, reinterpret_cast<void*>(size_vector_hamt) } },
		specialization_t { eresolved_type::k_vector_hamt_nonpod,		{ module_symbol_t("size_vector_hamt"), function_type1, reinterpret_cast<void*>(size_vector_hamt) } },

		specialization_t { eresolved_type::k_dict_cppmap_pod,			{ module_symbol_t("size_dict_cppmap"), function_type2, reinterpret_cast<void*>(size_dict_cppmap) } },
		specialization_t { eresolved_type::k_dict_cppmap_nonpod,		{ module_symbol_t("size_dict_cppmap"), function_type2, reinterpret_cast<void*>(size_dict_cppmap) } },
		specialization_t { eresolved_type::k_dict_hamt_pod,				{ module_symbol_t("size_dict_hamt"), function_type2, reinterpret_cast<void*>(size_dict_hamt) } },
		specialization_t { eresolved_type::k_dict_hamt_nonpod,			{ module_symbol_t("size_dict_hamt"), function_type2, reinterpret_cast<void*>(size_dict_hamt) } },

		specialization_t { eresolved_type::k_json,						{ module_symbol_t("size_json"), function_type3, reinterpret_cast<void*>(size_json) } }
	};
}

llvm::Value* generate_instrinsic_size(llvm_function_generator_t& gen_acc, const type_t& resolved_call_type, llvm::Value& collection_reg, const type_t& collection_type){
	QUARK_ASSERT(gen_acc.check_invariant());
	QUARK_ASSERT(collection_type.check_invariant());

	auto& builder = gen_acc.get_builder();
	const auto& types = gen_acc.gen.type_lookup.state.types;

	const auto res = codegen_lookup_specialization(gen_acc.gen.settings.config, types, gen_acc.gen.link_map, make_size_specializations(builder.getContext(), gen_acc.gen.type_lookup), collection_type);
	const auto collection_itype = generate_itype_constant(gen_acc.gen, collection_type);
	return builder.CreateCall(
		res.llvm_codegen_f,
		{ gen_acc.get_callers_fcp(), &collection_reg, collection_itype },
		""
	);
}



/////////////////////////////////////////		subset()


//??? check type at compile time, not runtime.

// VECTOR subset(VECTOR s, int start, int end)
static const runtime_value_t floyd_llvm_intrinsic__subset(floyd_runtime_t* frp, runtime_value_t elements_vec, runtime_type_t elements_vec_type, uint64_t start, uint64_t end){
	auto& backend = get_backend(frp);

	const auto& type0 = lookup_type_ref(backend, elements_vec_type);
	if(peek2(backend.types, type0).is_string()){
		return subset_vector_range__string(backend, elements_vec, elements_vec_type, start, end);
	}
	else if(is_vector_carray(backend.types, backend.config, type_t(elements_vec_type))){
		return subset_vector_range__carray(backend, elements_vec, elements_vec_type, start, end);
	}
	else if(is_vector_hamt(backend.types, backend.config, type_t(elements_vec_type))){
		return subset_vector_range__hamt(backend, elements_vec, elements_vec_type, start, end);
	}
	else{
		//	No other types allowed.
		UNSUPPORTED();
	}
}


/////////////////////////////////////////		to_pretty_string()


static runtime_value_t floyd_llvm_intrinsic__to_pretty_string(floyd_runtime_t* frp, runtime_value_t value, runtime_type_t value_type){
	auto& backend = get_backend(frp);

	const auto& type0 = lookup_type_ref(backend, value_type);
	const auto& value2 = from_runtime_value2(backend, value, type0);
	const auto json = value_to_json(backend.types, value2);
	const auto s = json_to_pretty_string(json, 0, pretty_t{ 80, 4 });
	return to_runtime_string2(backend, s);
}



/////////////////////////////////////////		to_string()



static runtime_value_t floyd_llvm_intrinsic__to_string(floyd_runtime_t* frp, runtime_value_t value, runtime_type_t value_type){
	auto& backend = get_backend(frp);

	const auto s = gen_to_string(backend, value, type_t(value_type));
	return to_runtime_string2(backend, s);
}



/////////////////////////////////////////		typeof()



static runtime_type_t floyd_llvm_intrinsic__typeof(floyd_runtime_t* frp, runtime_value_t value, runtime_type_t value_type){
	auto& backend = get_backend(frp);

#if DEBUG
	const auto& type0 = lookup_type_ref(backend, value_type);
	QUARK_ASSERT(type0.check_invariant());
#endif
	return value_type;
}



/////////////////////////////////////////		update()




static const runtime_value_t update_string(floyd_runtime_t* frp, runtime_value_t coll_value, runtime_type_t coll_type, runtime_value_t key_value, runtime_type_t key_type, runtime_value_t value, runtime_type_t value_type){
	auto& backend = get_backend(frp);
#if DEBUG
	const auto& type0 = lookup_type_ref(backend, coll_type);
	const auto& type1 = lookup_type_ref(backend, key_type);
	const auto& type2 = lookup_type_ref(backend, value_type);

	QUARK_ASSERT(type0.check_invariant());
	QUARK_ASSERT(type1.check_invariant());
	QUARK_ASSERT(type2.check_invariant());
	QUARK_ASSERT(peek2(backend.types, type0).is_string());
	QUARK_ASSERT(peek2(backend.types, type1).is_int());
	QUARK_ASSERT(peek2(backend.types, type2).is_int());
#endif
	return update__string(backend, coll_value, key_value, value);
}


static const runtime_value_t update_vector_carray_pod(floyd_runtime_t* frp, runtime_value_t coll_value, runtime_type_t coll_type, runtime_value_t key_value, runtime_type_t key_type, runtime_value_t value, runtime_type_t value_type){
	auto& backend = get_backend(frp);

#if DEBUG
	const auto& type0 = lookup_type_ref(backend, coll_type);
	const auto& type1 = lookup_type_ref(backend, key_type);
	const auto& type2 = lookup_type_ref(backend, value_type);

	QUARK_ASSERT(type0.check_invariant());
	QUARK_ASSERT(type1.check_invariant());
	QUARK_ASSERT(type2.check_invariant());
	QUARK_ASSERT(peek2(backend.types, type1).is_int());
#endif

	return update_element__vector_carray(backend, coll_value, coll_type, key_value, value);
}
static const runtime_value_t update_vector_carray_nonpod(floyd_runtime_t* frp, runtime_value_t coll_value, runtime_type_t coll_type, runtime_value_t key_value, runtime_type_t key_type, runtime_value_t value, runtime_type_t value_type){
	auto& backend = get_backend(frp);

#if DEBUG
	const auto& type0 = lookup_type_ref(backend, coll_type);
	const auto& type1 = lookup_type_ref(backend, key_type);
	const auto& type2 = lookup_type_ref(backend, value_type);

	QUARK_ASSERT(type0.check_invariant());
	QUARK_ASSERT(type1.check_invariant());
	QUARK_ASSERT(type2.check_invariant());
	QUARK_ASSERT(peek2(backend.types, type1).is_int());
#endif

	return update_element__vector_carray(backend, coll_value, coll_type, key_value, value);
}
static const runtime_value_t update_vector_hamt_pod(floyd_runtime_t* frp, runtime_value_t coll_value, runtime_type_t coll_type, runtime_value_t key_value, runtime_type_t key_type, runtime_value_t value, runtime_type_t value_type){
	auto& backend = get_backend(frp);

#if DEBUG
	const auto& type0 = lookup_type_ref(backend, coll_type);
	const auto& type1 = lookup_type_ref(backend, key_type);
	const auto& type2 = lookup_type_ref(backend, value_type);

	QUARK_ASSERT(type0.check_invariant());
	QUARK_ASSERT(type1.check_invariant());
	QUARK_ASSERT(type2.check_invariant());
	QUARK_ASSERT(peek2(backend.types, type1).is_int());
#endif

	return update_element__vector_hamt_pod(backend, coll_value, coll_type, key_value, value);
}
static const runtime_value_t update_vector_hamt_nonpod(floyd_runtime_t* frp, runtime_value_t coll_value, runtime_type_t coll_type, runtime_value_t key_value, runtime_type_t key_type, runtime_value_t value, runtime_type_t value_type){
	auto& backend = get_backend(frp);

#if DEBUG
	const auto& type0 = lookup_type_ref(backend, coll_type);
	const auto& type1 = lookup_type_ref(backend, key_type);
	const auto& type2 = lookup_type_ref(backend, value_type);

	QUARK_ASSERT(type0.check_invariant());
	QUARK_ASSERT(type1.check_invariant());
	QUARK_ASSERT(type2.check_invariant());
	QUARK_ASSERT(peek2(backend.types, type1).is_int());
#endif

	return update_element__vector_hamt_nonpod(backend, coll_value, coll_type, key_value, value);
}


static const runtime_value_t update_dict_cppmap_pod(floyd_runtime_t* frp, runtime_value_t coll_value, runtime_type_t coll_type, runtime_value_t key_value, runtime_type_t key_type, runtime_value_t value, runtime_type_t value_type){
	auto& backend = get_backend(frp);

#if DEBUG
	const auto& type0 = lookup_type_ref(backend, coll_type);
	const auto& type1 = lookup_type_ref(backend, key_type);
	const auto& type2 = lookup_type_ref(backend, value_type);

	QUARK_ASSERT(type0.check_invariant());
	QUARK_ASSERT(type1.check_invariant());
	QUARK_ASSERT(type2.check_invariant());
	QUARK_ASSERT(peek2(backend.types, type1).is_string());
#endif

	return update__dict_cppmap(backend, coll_value, coll_type, key_value, value);
}
static const runtime_value_t update_dict_cppmap_nonpod(floyd_runtime_t* frp, runtime_value_t coll_value, runtime_type_t coll_type, runtime_value_t key_value, runtime_type_t key_type, runtime_value_t value, runtime_type_t value_type){
	auto& backend = get_backend(frp);

#if DEBUG
	const auto& type0 = lookup_type_ref(backend, coll_type);
	const auto& type1 = lookup_type_ref(backend, key_type);
	const auto& type2 = lookup_type_ref(backend, value_type);

	QUARK_ASSERT(type0.check_invariant());
	QUARK_ASSERT(type1.check_invariant());
	QUARK_ASSERT(type2.check_invariant());
	QUARK_ASSERT(peek2(backend.types, type1).is_string());
#endif

	return update__dict_cppmap(backend, coll_value, coll_type, key_value, value);
}
static const runtime_value_t update_dict_hamt_pod(floyd_runtime_t* frp, runtime_value_t coll_value, runtime_type_t coll_type, runtime_value_t key_value, runtime_type_t key_type, runtime_value_t value, runtime_type_t value_type){
	auto& backend = get_backend(frp);

#if DEBUG
	const auto& type0 = lookup_type_ref(backend, coll_type);
	const auto& type1 = lookup_type_ref(backend, key_type);
	const auto& type2 = lookup_type_ref(backend, value_type);

	QUARK_ASSERT(type0.check_invariant());
	QUARK_ASSERT(type1.check_invariant());
	QUARK_ASSERT(type2.check_invariant());
	QUARK_ASSERT(peek2(backend.types, type1).is_string());
#endif
	return update__dict_hamt(backend, coll_value, coll_type, key_value, value);
}
static const runtime_value_t update_dict_hamt_nonpod(floyd_runtime_t* frp, runtime_value_t coll_value, runtime_type_t coll_type, runtime_value_t key_value, runtime_type_t key_type, runtime_value_t value, runtime_type_t value_type){
	auto& backend = get_backend(frp);

#if DEBUG
	const auto& type0 = lookup_type_ref(backend, coll_type);
	const auto& type1 = lookup_type_ref(backend, key_type);
	const auto& type2 = lookup_type_ref(backend, value_type);

	QUARK_ASSERT(type0.check_invariant());
	QUARK_ASSERT(type1.check_invariant());
	QUARK_ASSERT(type2.check_invariant());
	QUARK_ASSERT(peek2(backend.types, type1).is_string());
#endif
	return update__dict_hamt(backend, coll_value, coll_type, key_value, value);
}

static std::vector<specialization_t> make_update_specializations(llvm::LLVMContext& context, const llvm_type_lookup& type_lookup){
	llvm::FunctionType* function_type1 = llvm::FunctionType::get(
		make_generic_vec_type_byvalue(type_lookup)->getPointerTo(),
		{
			make_frp_type(type_lookup),

			make_generic_vec_type_byvalue(type_lookup)->getPointerTo(),
			make_runtime_type_type(type_lookup),

			llvm::Type::getInt64Ty(context),
			make_runtime_type_type(type_lookup),

			make_runtime_value_type(type_lookup),
			make_runtime_type_type(type_lookup),
		},
		false
	);
	llvm::FunctionType* function_type2 = llvm::FunctionType::get(
		make_generic_dict_type_byvalue(type_lookup)->getPointerTo(),
		{
			make_frp_type(type_lookup),

			make_generic_dict_type_byvalue(type_lookup)->getPointerTo(),
			make_runtime_type_type(type_lookup),

			make_generic_vec_type_byvalue(type_lookup)->getPointerTo(),
			make_runtime_type_type(type_lookup),

			make_runtime_value_type(type_lookup),
			make_runtime_type_type(type_lookup)
		},
		false
	);
	return {
//		specialization_t{ eresolved_type::k_string, { "update", make_intrinsic_llvm_function_type(type_lookup, make_update_signature()), reinterpret_cast<void*>(floyd_llvm_intrinsic__update) } }
		specialization_t { eresolved_type::k_string,					{ module_symbol_t("update__string"), function_type1, reinterpret_cast<void*>(update_string) } },

		specialization_t { eresolved_type::k_vector_carray_pod,			{ module_symbol_t("update_vector_carray"), function_type1, reinterpret_cast<void*>(update_vector_carray_pod) } },
		specialization_t { eresolved_type::k_vector_carray_nonpod,		{ module_symbol_t("update_vector_carray"), function_type1, reinterpret_cast<void*>(update_vector_carray_nonpod) } },
		specialization_t { eresolved_type::k_vector_hamt_pod,			{ module_symbol_t("update_vector_hamt_pod"), function_type1, reinterpret_cast<void*>(update_vector_hamt_pod) } },
		specialization_t { eresolved_type::k_vector_hamt_nonpod,		{ module_symbol_t("update_vector_hamt_nonpod"), function_type1, reinterpret_cast<void*>(update_vector_hamt_nonpod) } },

		specialization_t { eresolved_type::k_dict_cppmap_pod,			{ module_symbol_t("update_dict_cppmap_pod"), function_type2, reinterpret_cast<void*>(update_dict_cppmap_pod) } },
		specialization_t { eresolved_type::k_dict_cppmap_nonpod,		{ module_symbol_t("update_dict_cppmap_nonpod"), function_type2, reinterpret_cast<void*>(update_dict_cppmap_nonpod) } },
		specialization_t { eresolved_type::k_dict_hamt_pod,				{ module_symbol_t("update_dict_hamt_pod"), function_type2, reinterpret_cast<void*>(update_dict_hamt_pod) } },
		specialization_t { eresolved_type::k_dict_hamt_nonpod,			{ module_symbol_t("update_dict_hamt_nonpod"), function_type2, reinterpret_cast<void*>(update_dict_hamt_nonpod) } },
	};
}

llvm::Value* generate_instrinsic_update(llvm_function_generator_t& gen_acc, const type_t& resolved_call_type, llvm::Value& collection_reg, const type_t& collection_type, llvm::Value& key_reg, llvm::Value& value_reg){
	QUARK_ASSERT(collection_type.check_invariant());

	auto& builder = gen_acc.get_builder();
	const auto& types = gen_acc.gen.type_lookup.state.types;

	const auto res = codegen_lookup_specialization(
		gen_acc.gen.settings.config,
		types,
		gen_acc.gen.link_map,
		make_update_specializations(builder.getContext(), gen_acc.gen.type_lookup),
		collection_type
	);
	const auto collection_itype = generate_itype_constant(gen_acc.gen, collection_type);
	const auto collection_type_peek = peek2(types, collection_type);

	if(collection_type_peek.is_string()){
		const auto key_itype = generate_itype_constant(gen_acc.gen, type_t::make_int());
		const auto value_itype = generate_itype_constant(gen_acc.gen, type_t::make_int());
		return builder.CreateCall(
			res.llvm_codegen_f,
			{ gen_acc.get_callers_fcp(), &collection_reg, collection_itype, &key_reg, key_itype, &value_reg, value_itype },
			""
		);
	}
	else if(collection_type_peek.is_vector()){
		const auto key_itype = generate_itype_constant(gen_acc.gen, type_t::make_int());
		const auto element_type = collection_type_peek.get_vector_element_type(types);

		const auto packed_value_reg = generate_cast_to_runtime_value(gen_acc.gen, value_reg, element_type);
		const auto value_itype = generate_itype_constant(gen_acc.gen, element_type);
		return builder.CreateCall(
			res.llvm_codegen_f,
			{ gen_acc.get_callers_fcp(), &collection_reg, collection_itype, &key_reg, key_itype, packed_value_reg, value_itype },
			""
		);
	}
	else if(collection_type_peek.is_dict()){
		const auto key_itype = generate_itype_constant(gen_acc.gen, type_t::make_string());
		const auto element_type = collection_type_peek.get_dict_value_type(types);
		const auto value_itype = generate_itype_constant(gen_acc.gen, element_type);
		const auto packed_value_reg = generate_cast_to_runtime_value(gen_acc.gen, value_reg, element_type);
		return builder.CreateCall(
			res.llvm_codegen_f,
			{ gen_acc.get_callers_fcp(), &collection_reg, collection_itype, &key_reg, key_itype, packed_value_reg, value_itype },
			""
		);
	}
	else{
		QUARK_ASSERT(false);
		throw std::exception();
	}
}



/////////////////////////////////////////		to_json()



static runtime_value_t floyd_llvm_intrinsic__to_json(floyd_runtime_t* frp, runtime_value_t value, runtime_type_t value_type){
	auto& backend = get_backend(frp);

	const auto& type0 = lookup_type_ref(backend, value_type);
	const auto value0 = from_runtime_value2(backend, value, type0);
	const auto j = value_to_json(backend.types, value0);
	return alloc_json(backend.heap, j);
}


/////////////////////////////////////////		REGISTRY



//	These intrinsics have exactly one native function.
static func_link_t make_intri(const llvm_type_lookup& type_lookup, const intrinsic_signature_t& sign, void* f){
	QUARK_ASSERT(type_lookup.check_invariant());
	QUARK_ASSERT(f != nullptr);

	const auto function_type = sign._function_type;
	const auto function_type2 = get_llvm_function_type(type_lookup, function_type);
	const auto def = func_link_t {
		"intrinsic",
		module_symbol_t(sign.name),
		function_type,
		func_link_t::emachine::k_native,
		f,
		{},
		(native_type_t*)function_type2
	};
	return def;
}

//	These intrinsics have exactly one native function.
static std::vector<func_link_t> get_one_to_one_intrinsic_binds2(
	llvm::LLVMContext& context,
	const llvm_type_lookup& type_lookup,
	const intrinsic_signatures_t& intrinsic_signatures
){
	QUARK_ASSERT(type_lookup.check_invariant());

	//	??? copy
	auto types = type_lookup.state.types;

	const std::vector<func_link_t> defs = {
		make_intri(type_lookup, make_assert_signature(types), (void*)&floyd_llvm_intrinsic__assert),
		make_intri(type_lookup, make_to_string_signature(types), (void*)&floyd_llvm_intrinsic__to_string),
		make_intri(type_lookup, make_to_pretty_string_signature(types), (void*)&floyd_llvm_intrinsic__to_pretty_string),
		make_intri(type_lookup, make_typeof_signature(types), (void*)&floyd_llvm_intrinsic__typeof),

//		make_intri(type_lookup, make_update_signature(types), (void*)&floyd_llvm_intrinsic__update),
//		make_intri(type_lookup, make_size_signature(types), (void*)&floyd_llvm_intrinsic__size),
		make_intri(type_lookup, make_find_signature(types), (void*)&floyd_llvm_intrinsic__find),
		make_intri(type_lookup, make_exists_signature(types), (void*)&floyd_llvm_intrinsic__exists),
		make_intri(type_lookup, make_erase_signature(types), (void*)&floyd_llvm_intrinsic__erase),
		make_intri(type_lookup, make_get_keys_signature(types), (void*)&floyd_llvm_intrinsic__get_keys),
//		make_intri(type_lookup, make_push_back_signature(types), (void*)&floyd_llvm_intrinsic__push_back),
		make_intri(type_lookup, make_subset_signature(types), (void*)&floyd_llvm_intrinsic__subset),
		make_intri(type_lookup, make_replace_signature(types), (void*)&floyd_llvm_intrinsic__replace),

		make_intri(type_lookup, make_generate_json_script_signature(types), (void*)&floyd_llvm_intrinsic__generate_json_script),
		make_intri(type_lookup, make_from_json_signature(types), (void*)&floyd_llvm_intrinsic__from_json),
		make_intri(type_lookup, make_parse_json_script_signature(types), (void*)&floyd_llvm_intrinsic__parse_json_script),
		make_intri(type_lookup, make_to_json_signature(types), (void*)&floyd_llvm_intrinsic__to_json),

		make_intri(type_lookup, make_get_json_type_signature(types), (void*)&floyd_llvm_intrinsic__get_json_type),

//		make_intri(type_lookup, make_map_signature(types), (void*)&floyd_llvm_intrinsic__map),
//		make_intri(type_lookup, make_map_string_signature(types), (void*)&floyd_llvm_intrinsic__map_string),
		make_intri(type_lookup, make_map_dag_signature(types), (void*)&floyd_llvm_intrinsic__map_dag),
		make_intri(type_lookup, make_filter_signature(types), (void*)&floyd_llvm_intrinsic__filter),
		make_intri(type_lookup, make_reduce_signature(types), (void*)&floyd_llvm_intrinsic__reduce),
		make_intri(type_lookup, make_stable_sort_signature(types), (void*)&floyd_llvm_intrinsic__stable_sort),

		make_intri(type_lookup, make_print_signature(types), (void*)&unified_intrinsic__print),
		make_intri(type_lookup, make_send_signature(types), (void*)&unified_intrinsic__send),
		make_intri(type_lookup, make_exit_signature(types), (void*)&unified_intrinsic__exit),

/*
		make_intri(type_lookup, make_bw_not_signature(types), (void*)&floyd_llvm_intrinsic__dummy),
		make_intri(type_lookup, make_bw_and_signature(types), (void*)&floyd_llvm_intrinsic__dummy),
		make_intri(type_lookup, make_bw_or_signature(types), (void*)&floyd_llvm_intrinsic__dummy),
		make_intri(type_lookup, make_bw_xor_signature(types), (void*)&floyd_llvm_intrinsic__dummy),
		make_intri(type_lookup, make_bw_shift_left_signature(types), (void*)&floyd_llvm_intrinsic__dummy),
		make_intri(type_lookup, make_bw_shift_right_signature(types), (void*)&floyd_llvm_intrinsic__dummy),
		make_intri(type_lookup, make_bw_shift_right_arithmetic_signature(types), (void*)&floyd_llvm_intrinsic__dummy))
*/
	};

	QUARK_ASSERT(types.nodes.size() == type_lookup.state.types.nodes.size());

	return defs;
}

//	Skips duplicates.
static std::vector<func_link_t> make_specialized_link_entries(const intrinsic_signatures_t& intrinsic_signatures, const std::vector<specialization_t>& specializations){
	const auto binds = mapf<llvm_function_bind_t>(specializations, [](auto& e){ return e.bind; });

	std::vector<func_link_t> result;
	for(const auto& bind: binds){
		auto signature_it = std::find_if(
			intrinsic_signatures.vec.begin(),
			intrinsic_signatures.vec.end(),
			[&] (const intrinsic_signature_t& m) { return module_symbol_t(m.name) == bind.name; }
		);
		const auto function_type = signature_it != intrinsic_signatures.vec.end() ? signature_it->_function_type : type_t::make_undefined();

		const auto link_name = bind.name;
		const auto exists_it = std::find_if(
			result.begin(),
			result.end(),
			[&](const auto& e){ return e.module_symbol == link_name; }
		);

		if(exists_it == result.end()){
			QUARK_ASSERT(bind.llvm_function_type != nullptr);
			const auto def = func_link_t {
				"intrinsic",
				link_name,
				function_type,
				func_link_t::emachine::k_native,
				bind.native_f,
				{},
				(native_type_t*)bind.llvm_function_type,
			};
			result.push_back(def);
		}
	}
	return result;
}

std::vector<func_link_t> make_intrinsics_link_map(llvm::LLVMContext& context, const llvm_type_lookup& type_lookup, const intrinsic_signatures_t& intrinsic_signatures){
	QUARK_ASSERT(type_lookup.check_invariant());

	const auto& types = type_lookup.state.types;

	auto result = get_one_to_one_intrinsic_binds2(context, type_lookup, intrinsic_signatures);
	result = concat(result, make_specialized_link_entries(intrinsic_signatures, make_push_back_specializations(context, type_lookup)));
	result = concat(result, make_specialized_link_entries(intrinsic_signatures, make_size_specializations(context, type_lookup)));
	result = concat(result, make_specialized_link_entries(intrinsic_signatures, make_update_specializations(context, type_lookup)));
	result = concat(result, make_specialized_link_entries(intrinsic_signatures, make_map_specializations(context, type_lookup)));

	if(k_trace_function_link_map){
		trace_function_link_map(types, result);
	}

	//	Specialized functions don't get a function_type_optional!
/*
	for(const auto& e: result){
		llvm::FunctionType* llvm_function_type = get_llvm_function_type(type_lookup, e.func_link.function_type_optional);
//		QUARK_ASSERT(llvm_function_type == e.llvm_function_type);
	}
*/

	return result;
}


} // floyd
