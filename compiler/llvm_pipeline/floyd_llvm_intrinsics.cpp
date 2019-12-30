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
		specialization_t { eresolved_type::k_vector_carray_pod,		{ module_symbol_t("map_carray_pod"), function_type, reinterpret_cast<void*>(unified_intrinsic__map__carray) } },
		specialization_t { eresolved_type::k_vector_carray_nonpod,	{ module_symbol_t("map_carray_nonpod"), function_type, reinterpret_cast<void*>(unified_intrinsic__map__carray) } },
		specialization_t { eresolved_type::k_vector_hamt_pod,		{ module_symbol_t("map_hamt_pod"), function_type, reinterpret_cast<void*>(unified_intrinsic__map__hamt) } },
		specialization_t { eresolved_type::k_vector_hamt_nonpod, 	{ module_symbol_t("map_hamt_nonpod"), function_type, reinterpret_cast<void*>(unified_intrinsic__map__hamt) } }
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
		specialization_t { eresolved_type::k_string,				{ module_symbol_t("push_back__string"), function_type, reinterpret_cast<void*>(unified_intrinsic__push_back_string) } },
		specialization_t { eresolved_type::k_vector_carray_pod,		{ module_symbol_t("push_back_carray_pod"), function_type, reinterpret_cast<void*>(unified_intrinsic__push_back_carray_pod) } },
		specialization_t { eresolved_type::k_vector_carray_nonpod,	{ module_symbol_t("push_back_carray_nonpod"), function_type, reinterpret_cast<void*>(unified_intrinsic__push_back_carray_nonpod) } },
		specialization_t { eresolved_type::k_vector_hamt_pod,		{ module_symbol_t("push_back_hamt_pod"), function_type, reinterpret_cast<void*>(unified_intrinsic__push_back_hamt_pod) } },
		specialization_t { eresolved_type::k_vector_hamt_nonpod, 	{ module_symbol_t("push_back_hamt_nonpod"), function_type, reinterpret_cast<void*>(unified_intrinsic__push_back_hamt_nonpod) } }
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
		func_link_t::emachine::k_native2,
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
		make_intri(type_lookup, make_assert_signature(types), (void*)&unified_intrinsic__assert),
		make_intri(type_lookup, make_to_string_signature(types), (void*)&unified_intrinsic__to_string),
		make_intri(type_lookup, make_to_pretty_string_signature(types), (void*)&unified_intrinsic__to_pretty_string),
		make_intri(type_lookup, make_typeof_signature(types), (void*)&unified_intrinsic__typeof),

		make_intri(type_lookup, make_update_signature(types), (void*)&unified_intrinsic__update),
		make_intri(type_lookup, make_size_signature(types), (void*)&unified_intrinsic__size),
		make_intri(type_lookup, make_find_signature(types), (void*)&unified_intrinsic__find),
		make_intri(type_lookup, make_exists_signature(types), (void*)&unified_intrinsic__exists),
		make_intri(type_lookup, make_erase_signature(types), (void*)&unified_intrinsic__erase),
		make_intri(type_lookup, make_get_keys_signature(types), (void*)&unified_intrinsic__get_keys),
		make_intri(type_lookup, make_push_back_signature(types), (void*)&unified_intrinsic__push_back),
		make_intri(type_lookup, make_subset_signature(types), (void*)&unified_intrinsic__subset),
		make_intri(type_lookup, make_replace_signature(types), (void*)&unified_intrinsic__replace),

		make_intri(type_lookup, make_get_json_type_signature(types), (void*)&unified_intrinsic__get_json_type),
		make_intri(type_lookup, make_generate_json_script_signature(types), (void*)&unified_intrinsic__generate_json_script),
		make_intri(type_lookup, make_parse_json_script_signature(types), (void*)&unified_intrinsic__parse_json_script),
		make_intri(type_lookup, make_to_json_signature(types), (void*)&unified_intrinsic__to_json),
		make_intri(type_lookup, make_from_json_signature(types), (void*)&unified_intrinsic__from_json),


		make_intri(type_lookup, make_map_signature(types), (void*)&unified_intrinsic__map),
		make_intri(type_lookup, make_map_dag_signature(types), (void*)&unified_intrinsic__map_dag),
		make_intri(type_lookup, make_filter_signature(types), (void*)&unified_intrinsic__filter),
		make_intri(type_lookup, make_reduce_signature(types), (void*)&unified_intrinsic__reduce),
		make_intri(type_lookup, make_stable_sort_signature(types), (void*)&unified_intrinsic__stable_sort),

		make_intri(type_lookup, make_print_signature(types), (void*)&unified_intrinsic__print),
		make_intri(type_lookup, make_send_signature(types), (void*)&unified_intrinsic__send),
		make_intri(type_lookup, make_exit_signature(types), (void*)&unified_intrinsic__exit),

		make_intri(type_lookup, make_bw_not_signature(types), (void*)&unified_intrinsic__bw_not),
		make_intri(type_lookup, make_bw_and_signature(types), (void*)&unified_intrinsic__bw_and),
		make_intri(type_lookup, make_bw_or_signature(types), (void*)&unified_intrinsic__bw_or),
		make_intri(type_lookup, make_bw_xor_signature(types), (void*)&unified_intrinsic__bw_xor),
		make_intri(type_lookup, make_bw_shift_left_signature(types), (void*)&unified_intrinsic__bw_shift_left),
		make_intri(type_lookup, make_bw_shift_right_signature(types), (void*)&unified_intrinsic__bw_shift_right),
		make_intri(type_lookup, make_bw_shift_right_arithmetic_signature(types), (void*)&unified_intrinsic__bw_shift_right_arithmetic)
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
