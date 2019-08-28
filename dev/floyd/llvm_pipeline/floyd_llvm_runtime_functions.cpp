//
//  floyd_llvm_runtime_functions.cpp
//  floyd
//
//  Created by Marcus Zetterquist on 2019-08-28.
//  Copyright © 2019 Marcus Zetterquist. All rights reserved.
//

#include "floyd_llvm_runtime_functions.h"

#include "floyd_llvm_helpers.h"
#include "floyd_llvm_runtime.h"
#include "value_features.h"

#include <llvm/ExecutionEngine/ExecutionEngine.h>


namespace floyd {




////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//	RUNTIME FUNCTIONS
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

//	Automatically called by the LLVM execution engine.
//	The names of these are computed from the host-id in the symbol table, not the names of the functions/symbols.
//	They must use C calling convention so llvm JIT can find them.
//	Make sure they are not dead-stripped out of binary!
////////////////////////////////////////////////////////////////////////////////



//	VECTOR
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////



////////////////////////////////		allocate_string_from_strptr()



//	Creates a new VEC_T with the contents of the string. Caller owns the result.
static VECTOR_CPPVECTOR_T* floydrt_alloc_kstr(floyd_runtime_t* frp, const char* s, uint64_t size){
	auto& r = get_floyd_runtime(frp);

	const auto a = to_runtime_string(r, std::string(s, s + size));
	return a.vector_cppvector_ptr;
}

static function_bind_t floydrt_alloc_kstr__make(llvm::LLVMContext& context, const llvm_type_lookup& type_lookup){
	llvm::FunctionType* function_type = llvm::FunctionType::get(
		make_generic_vec_type(type_lookup)->getPointerTo(),
		{
			make_frp_type(type_lookup),
			llvm::Type::getInt8PtrTy(context),
			llvm::Type::getInt64Ty(context)
		},
		false
	);
	return { "alloc_kstr", function_type, reinterpret_cast<void*>(floydrt_alloc_kstr) };
}



////////////////////////////////		allocate_vector()



//	Creates a new VEC_T with element_count. All elements are blank. Caller owns the result.
static runtime_value_t floydrt_allocate_vector(floyd_runtime_t* frp, runtime_type_t type, uint64_t element_count){
	auto& r = get_floyd_runtime(frp);

	const auto type1 = lookup_type(r.backend, type);
	if(is_vector_cppvector(type1)){
		return alloc_vector_ccpvector2(r.backend.heap, element_count, element_count, itype_t(type));
	}
	else if(is_vector_hamt(type1)){
		return alloc_vector_hamt(r.backend.heap, element_count, element_count, itype_t(type));
	}
	else{
		QUARK_ASSERT(false);
		throw std::exception();
	}
}

static function_bind_t floydrt_allocate_vector__make(llvm::LLVMContext& context, const llvm_type_lookup& type_lookup){
	llvm::FunctionType* function_type = llvm::FunctionType::get(
		make_generic_vec_type(type_lookup)->getPointerTo(),
		{
			make_frp_type(type_lookup),
			make_runtime_type_type(type_lookup),
			llvm::Type::getInt64Ty(context)
		},
		false
	);
	return { "allocate_vector", function_type, reinterpret_cast<void*>(floydrt_allocate_vector) };
}

llvm::Value* generate_allocate_vector(const runtime_functions_t& functions, llvm::IRBuilder<>& builder, llvm::Value& frp_reg, llvm::Value& vector_type_reg, int64_t element_count){
	const auto element_count_reg = llvm::ConstantInt::get(llvm::Type::getInt64Ty(builder.getContext()), element_count);

	return builder.CreateCall(
		functions.floydrt_allocate_vector.llvm_codegen_f,
		{ &frp_reg, &vector_type_reg, element_count_reg },
		""
	);
}



////////////////////////////////		allocate_vector_fill()



static runtime_value_t floydrt_allocate_vector_fill(floyd_runtime_t* frp, runtime_type_t type, runtime_value_t* elements, uint64_t element_count){
	auto& r = get_floyd_runtime(frp);

	const auto type1 = lookup_type(r.backend, type);
	if(is_vector_cppvector(type1)){
		return alloc_vector_ccpvector2(r.backend.heap, element_count, element_count, itype_t(type));
	}
	else if(is_vector_hamt(type1)){
		return alloc_vector_hamt(r.backend.heap, element_count, element_count, itype_t(type));
	}
	else{
		QUARK_ASSERT(false);
		throw std::exception();
	}
}

static function_bind_t floydrt_allocate_vector_fill__make(llvm::LLVMContext& context, const llvm_type_lookup& type_lookup){
	llvm::FunctionType* function_type = llvm::FunctionType::get(
		make_generic_vec_type(type_lookup)->getPointerTo(),
		{
			make_frp_type(type_lookup),
			make_runtime_type_type(type_lookup),
			make_generic_vec_type(type_lookup)->getPointerTo(),
			llvm::Type::getInt64Ty(context)
		},
		false
	);
	return { "allocate_vector_fill", function_type, reinterpret_cast<void*>(floydrt_allocate_vector_fill) };
}



////////////////////////////////		floydrt_retain_vec()



static void floydrt_retain_vec(floyd_runtime_t* frp, runtime_value_t vec, runtime_type_t type0){
	auto& r = get_floyd_runtime(frp);
#if DEBUG
	const auto type = lookup_type(r.backend, type0);
	QUARK_ASSERT(type.is_string() || type.is_vector());
	QUARK_ASSERT(is_rc_value(type));
#endif

	retain_value(r.backend, vec, itype_t(type0));
}

static void floydrt_retain_vector_hamt(floyd_runtime_t* frp, runtime_value_t vec, runtime_type_t type0){
	auto& r = get_floyd_runtime(frp);
#if DEBUG
	const auto type = lookup_type(r.backend, type0);
	QUARK_ASSERT(type.is_string() || type.is_vector());
	QUARK_ASSERT(is_rc_value(type));
#endif

	retain_vector_hamt(r.backend, vec, itype_t(type0));
}




////////////////////////////////		floydrt_release_vec()



static void floydrt_release_vec(floyd_runtime_t* frp, runtime_value_t vec, runtime_type_t type0){
	auto& r = get_floyd_runtime(frp);
#if DEBUG
	const auto type = lookup_type(r.backend, type0);
	QUARK_ASSERT(type.is_string() || type.is_vector());
#endif

	release_deep(r.backend, vec, itype_t(type0));
}
static void floydrt_release_vector_hamt_pod(floyd_runtime_t* frp, runtime_value_t vec, runtime_type_t type0){
	auto& r = get_floyd_runtime(frp);
#if DEBUG
	const auto type = lookup_type(r.backend, type0);
	QUARK_ASSERT(is_vector_hamt(type));
	QUARK_ASSERT(is_rc_value(type.get_vector_element_type()) == false);
#endif

	release_vector_hamt_pod(r.backend, vec, itype_t(type0));
}



////////////////////////////////		load_vector_element()



static runtime_value_t floydrt_load_vector_element(floyd_runtime_t* frp, runtime_value_t vec, runtime_type_t type, uint64_t index){
	auto& r = get_floyd_runtime(frp);
	(void)r;

	const auto type0 = lookup_type(r.backend, type);
	if(is_vector_cppvector(type0)){
		QUARK_ASSERT(false);
		throw std::exception();
	}
	else if(is_vector_hamt(type0)){
		return vec.vector_hamt_ptr->load_element(index);
	}
	else{
		QUARK_ASSERT(false);
		throw std::exception();
	}
}

static function_bind_t floydrt_load_vector_element__make(llvm::LLVMContext& context, const llvm_type_lookup& type_lookup){
	llvm::FunctionType* function_type = llvm::FunctionType::get(
		make_runtime_value_type(type_lookup),
		{
			make_frp_type(type_lookup),
			make_generic_vec_type(type_lookup)->getPointerTo(),
			make_runtime_type_type(type_lookup),
			llvm::Type::getInt64Ty(context)
		},
		false
	);
	return { "load_vector_element", function_type, reinterpret_cast<void*>(floydrt_load_vector_element) };
}



////////////////////////////////		store_vector_element_mutable()



static void floydrt_store_vector_element_mutable(floyd_runtime_t* frp, runtime_value_t vec, runtime_type_t type, uint64_t index, runtime_value_t element){
	auto& r = get_floyd_runtime(frp);
	(void)r;

	const auto type0 = lookup_type(r.backend, type);
	if(is_vector_cppvector(type0)){
		QUARK_ASSERT(false);
	}
	else if(is_vector_hamt(type0)){
		vec.vector_hamt_ptr->store_mutate(index, element);
	}
	else{
		QUARK_ASSERT(false);
	}
}

static function_bind_t floydrt_store_vector_element_mutable__make(llvm::LLVMContext& context, const llvm_type_lookup& type_lookup){
	llvm::FunctionType* function_type = llvm::FunctionType::get(
		llvm::Type::getVoidTy(context),
		{
			make_frp_type(type_lookup),
			make_generic_vec_type(type_lookup)->getPointerTo(),
			make_runtime_type_type(type_lookup),
			llvm::Type::getInt64Ty(context),
			make_runtime_value_type(type_lookup)
		},
		false
	);
	return { "store_vector_element_mutable", function_type, reinterpret_cast<void*>(floydrt_store_vector_element_mutable) };
}



////////////////////////////////		floydrt_concatunate_vectors()



static runtime_value_t floydrt_concatunate_vectors(floyd_runtime_t* frp, runtime_type_t type, runtime_value_t lhs, runtime_value_t rhs){
	auto& r = get_floyd_runtime(frp);
	QUARK_ASSERT(lhs.check_invariant());
	QUARK_ASSERT(rhs.check_invariant());

	const auto type0 = lookup_type(r.backend, type);
	if(type0.is_string()){
		return concat_strings(r.backend, lhs, rhs);
	}
	else if(is_vector_cppvector(type0)){
		return concat_vector_cppvector(r.backend, type0, lhs, rhs);
	}
	else if(is_vector_hamt(type0)){
		return concat_vector_hamt(r.backend, type0, lhs, rhs);
	}
	else{
		QUARK_ASSERT(false);
		throw std::exception();
	}
}

static function_bind_t floydrt_concatunate_vectors__make(llvm::LLVMContext& context, const llvm_type_lookup& type_lookup){
	llvm::FunctionType* function_type = llvm::FunctionType::get(
		make_generic_vec_type(type_lookup)->getPointerTo(),
		{
			make_frp_type(type_lookup),
			make_runtime_type_type(type_lookup),
			make_generic_vec_type(type_lookup)->getPointerTo(),
			make_generic_vec_type(type_lookup)->getPointerTo()
		},
		false
	);
	return { "concatunate_vectors", function_type, reinterpret_cast<void*>(floydrt_concatunate_vectors) };
}






static runtime_value_t floydrt_push_back_hamt_pod(floyd_runtime_t* frp, runtime_value_t vec, runtime_value_t element){
	return push_back_immutable(vec, element);
}

static function_bind_t floydrt_push_back_hamt_pod__make(llvm::LLVMContext& context, const llvm_type_lookup& type_lookup){
	llvm::FunctionType* function_type = llvm::FunctionType::get(
		make_generic_vec_type(type_lookup)->getPointerTo(),
		{
			make_frp_type(type_lookup),
			make_generic_vec_type(type_lookup)->getPointerTo(),
			make_runtime_value_type(type_lookup)
		},
		false
	);
	return { "push_back_hamt_pod", function_type, reinterpret_cast<void*>(floydrt_push_back_hamt_pod) };
}




//??? value_backend never handle RC automatically = no need to make pod/nonpod access to it.
static runtime_value_t floydrt_load_vector_element_hamt(floyd_runtime_t* frp, runtime_value_t vec, runtime_type_t type, uint64_t index){
	auto& r = get_floyd_runtime(frp);
	(void)r;

#if DEBUG
	const auto type0 = lookup_type(r.backend, type);
	QUARK_ASSERT(is_vector_hamt(type0));
#endif

	return vec.vector_hamt_ptr->load_element(index);
}

static function_bind_t floydrt_load_vector_element_hamt__make(llvm::LLVMContext& context, const llvm_type_lookup& type_lookup){
	llvm::FunctionType* function_type = llvm::FunctionType::get(
		make_runtime_value_type(type_lookup),
		{
			make_frp_type(type_lookup),
			make_generic_vec_type(type_lookup)->getPointerTo(),
			make_runtime_type_type(type_lookup),
			llvm::Type::getInt64Ty(context)
		},
		false
	);
	return { "load_vector_element_hamt", function_type, reinterpret_cast<void*>(floydrt_load_vector_element_hamt) };
}






//	DICT
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////






////////////////////////////////		allocate_dict()



static const runtime_value_t floydrt_allocate_dict(floyd_runtime_t* frp, runtime_type_t type){
	auto& r = get_floyd_runtime(frp);

	const auto type0 = lookup_type(r.backend, type);
	if(is_dict_cppmap(type0)){
		return alloc_dict_cppmap(r.backend.heap, itype_t(type));
	}
	else if(is_dict_hamt(type0)){
		return alloc_dict_hamt(r.backend.heap, itype_t(type));
	}
	else{
		QUARK_ASSERT(false);
		throw std::exception();
	}
}

static function_bind_t floydrt_allocate_dict__make(llvm::LLVMContext& context, const llvm_type_lookup& type_lookup){
	llvm::FunctionType* function_type = llvm::FunctionType::get(
		make_generic_dict_type(type_lookup)->getPointerTo(),
		{
			make_frp_type(type_lookup),
			make_runtime_type_type(type_lookup),
		},
		false
	);
	return { "allocate_dict", function_type, reinterpret_cast<void*>(floydrt_allocate_dict) };
}



////////////////////////////////		floydrt_retain_dict()



static void floydrt_retain_dict(floyd_runtime_t* frp, runtime_value_t dict, runtime_type_t type0){
	auto& r = get_floyd_runtime(frp);
#if DEBUG
	const auto type = lookup_type(r.backend, type0);
	QUARK_ASSERT(is_rc_value(type));
	QUARK_ASSERT(type.is_dict());
#endif

	retain_value(r.backend, dict, itype_t(type0));
}

static function_bind_t floydrt_retain_dict__make(llvm::LLVMContext& context, const llvm_type_lookup& type_lookup){
	llvm::FunctionType* function_type = llvm::FunctionType::get(
		llvm::Type::getVoidTy(context),
		{
			make_frp_type(type_lookup),
			make_generic_dict_type(type_lookup)->getPointerTo(),
			make_runtime_type_type(type_lookup)
		},
		false
	);
	return { "retain_dict", function_type, reinterpret_cast<void*>(floydrt_retain_dict) };
}



////////////////////////////////		floydrt_release_dict()



static void floydrt_release_dict(floyd_runtime_t* frp, runtime_value_t dict, runtime_type_t type0){
	auto& r = get_floyd_runtime(frp);
#if DEBUG
	const auto type = lookup_type(r.backend, type0);
	QUARK_ASSERT(type.is_dict());
#endif

	release_deep(r.backend, dict, itype_t(type0));
}

static function_bind_t floydrt_release_dict__make(llvm::LLVMContext& context, const llvm_type_lookup& type_lookup){
	llvm::FunctionType* function_type = llvm::FunctionType::get(
		llvm::Type::getVoidTy(context),
		{
			make_frp_type(type_lookup),
			make_generic_dict_type(type_lookup)->getPointerTo(),
			make_runtime_type_type(type_lookup)
		},
		false
	);
	return { "release_dict", function_type, reinterpret_cast<void*>(floydrt_release_dict) };
}



////////////////////////////////		lookup_dict()



static runtime_value_t floydrt_lookup_dict(floyd_runtime_t* frp, runtime_value_t dict, runtime_type_t type, runtime_value_t s){
	auto& r = get_floyd_runtime(frp);

	const auto type0 = lookup_type(r.backend, type);
	if(is_dict_cppmap(type0)){
		const auto& m = dict.dict_cppmap_ptr->get_map();
		const auto key_string = from_runtime_string(r, s);
		const auto it = m.find(key_string);
		if(it == m.end()){
			throw std::exception();
		}
		else{
			return it->second;
		}
	}
	else if(is_dict_hamt(type0)){
		const auto& m = dict.dict_hamt_ptr->get_map();
		const auto key_string = from_runtime_string(r, s);
		const auto it = m.find(key_string);
		if(it == nullptr){
			throw std::exception();
		}
		else{
			return *it;
		}
	}
	else{
		QUARK_ASSERT(false);
		throw std::exception();
	}
}

static function_bind_t floydrt_lookup_dict__make(llvm::LLVMContext& context, const llvm_type_lookup& type_lookup){
	llvm::FunctionType* function_type = llvm::FunctionType::get(
		make_runtime_value_type(type_lookup),
		{
			make_frp_type(type_lookup),
			make_generic_dict_type(type_lookup)->getPointerTo(),
			make_runtime_type_type(type_lookup),
			get_exact_llvm_type(type_lookup, typeid_t::make_string())
		},
		false
	);
	return { "lookup_dict", function_type, reinterpret_cast<void*>(floydrt_lookup_dict) };
}



////////////////////////////////		store_dict_mutable()



static void floydrt_store_dict_mutable(floyd_runtime_t* frp, runtime_value_t dict, runtime_type_t type, runtime_value_t key, runtime_value_t element_value){
	auto& r = get_floyd_runtime(frp);

	const auto type0 = lookup_type(r.backend, type);
	if(is_dict_cppmap(type0)){
		const auto key_string = from_runtime_string(r, key);
		dict.dict_cppmap_ptr->get_map_mut().insert_or_assign(key_string, element_value);
	}
	else if(is_dict_hamt(type0)){
		const auto key_string = from_runtime_string(r, key);
		dict.dict_hamt_ptr->get_map_mut() = dict.dict_hamt_ptr->get_map_mut().set(key_string, element_value);
	}
	else{
		QUARK_ASSERT(false);
	}

}

static function_bind_t floydrt_store_dict_mutable__make(llvm::LLVMContext& context, const llvm_type_lookup& type_lookup){
	llvm::FunctionType* function_type = llvm::FunctionType::get(
		llvm::Type::getVoidTy(context),
		{
			make_frp_type(type_lookup),
			make_generic_dict_type(type_lookup)->getPointerTo(),
			make_runtime_type_type(type_lookup),
			get_exact_llvm_type(type_lookup, typeid_t::make_string()),
			make_runtime_value_type(type_lookup),
		},
		false
	);
	return { "store_dict_mutable", function_type, reinterpret_cast<void*>(floydrt_store_dict_mutable) };
}

















//	JSON
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////




////////////////////////////////		allocate_json()

//??? Make named types for all function-argument / return types, like: typedef int16_t* native_json_ptr

static JSON_T* floydrt_allocate_json(floyd_runtime_t* frp, runtime_value_t arg0_value, runtime_type_t arg0_type){
	auto& r = get_floyd_runtime(frp);

	const auto type0 = lookup_type(r.backend, arg0_type);
	const auto value = from_runtime_value(r, arg0_value, type0);

	const auto a = value_to_ast_json(value, json_tags::k_plain);
	auto result = alloc_json(r.backend.heap, a);
	return result;
}

static function_bind_t floydrt_allocate_json__make(llvm::LLVMContext& context, const llvm_type_lookup& type_lookup){
	llvm::FunctionType* function_type = llvm::FunctionType::get(
		get_exact_llvm_type(type_lookup, typeid_t::make_json()),
		{
			make_frp_type(type_lookup),
			make_runtime_value_type(type_lookup),
			make_runtime_type_type(type_lookup)
		},
		false
	);
	return { "allocate_json", function_type, reinterpret_cast<void*>(floydrt_allocate_json) };
}



////////////////////////////////		floydrt_retain_json()



static void floydrt_retain_json(floyd_runtime_t* frp, JSON_T* json, runtime_type_t type0){
	auto& r = get_floyd_runtime(frp);

	const auto type = lookup_type(r.backend, type0);
	QUARK_ASSERT(is_rc_value(type));

	//	NOTICE: Floyd runtime() init will destruct globals, including json::null.
	if(json == nullptr){
	}
	else{
		QUARK_ASSERT(type.is_json());

		inc_rc(json->alloc);
	}
}

static function_bind_t floydrt_retain_json__make(llvm::LLVMContext& context, const llvm_type_lookup& type_lookup){
	llvm::FunctionType* function_type = llvm::FunctionType::get(
		llvm::Type::getVoidTy(context),
		{
			make_frp_type(type_lookup),
			get_exact_llvm_type(type_lookup, typeid_t::make_json()),
			make_runtime_type_type(type_lookup)
		},
		false
	);
	return { "retain_json", function_type, reinterpret_cast<void*>(floydrt_retain_json) };
}



////////////////////////////////		floydrt_release_json()



static void floydrt_release_json(floyd_runtime_t* frp, JSON_T* json, runtime_type_t type0){
	auto& r = get_floyd_runtime(frp);
	const auto type = lookup_type(r.backend, type0);
	QUARK_ASSERT(type.is_json());

	//	NOTICE: Floyd runtime() init will destruct globals, including json::null.
	if(json == nullptr){
	}
	else{
		QUARK_ASSERT(json != nullptr);
		if(dec_rc(json->alloc) == 0){
			dispose_json(*json);
		}
	}
}

static function_bind_t floydrt_release_json__make(llvm::LLVMContext& context, const llvm_type_lookup& type_lookup){
	llvm::FunctionType* function_type = llvm::FunctionType::get(
		llvm::Type::getVoidTy(context),
		{
			make_frp_type(type_lookup),
			get_exact_llvm_type(type_lookup, typeid_t::make_json()),
			make_runtime_type_type(type_lookup)
		},
		false
	);
	return { "release_json", function_type, reinterpret_cast<void*>(floydrt_release_json) };
}



//??? move more non-LLVM specific logic to value_features.cpp

////////////////////////////////		lookup_json()



static JSON_T* floydrt_lookup_json(floyd_runtime_t* frp, JSON_T* json_ptr, runtime_value_t arg0_value, runtime_type_t arg0_type){
	auto& r = get_floyd_runtime(frp);

	const auto& json = json_ptr->get_json();
	const auto type0 = lookup_type(r.backend, arg0_type);
	const auto value = from_runtime_value(r, arg0_value, type0);

	if(json.is_object()){
		if(type0.is_string() == false){
			quark::throw_runtime_error("Attempting to lookup a json-object with a key that is not a string.");
		}

		const auto result = json.get_object_element(value.get_string_value());
		return alloc_json(r.backend.heap, result);
	}
	else if(json.is_array()){
		if(type0.is_int() == false){
			quark::throw_runtime_error("Attempting to lookup a json-object with a key that is not a number.");
		}

		const auto result = json.get_array_n(value.get_int_value());
		auto result2 = alloc_json(r.backend.heap, result);
		return result2;
	}
	else{
		quark::throw_runtime_error("Attempting to lookup a json value -- lookup requires json-array or json-object.");
	}
}

static function_bind_t floydrt_lookup_json__make(llvm::LLVMContext& context, const llvm_type_lookup& type_lookup){
	llvm::FunctionType* function_type = llvm::FunctionType::get(
		get_exact_llvm_type(type_lookup, typeid_t::make_json()),
		{
			make_frp_type(type_lookup),
			get_exact_llvm_type(type_lookup, typeid_t::make_json()),
			make_runtime_value_type(type_lookup),
			make_runtime_type_type(type_lookup)
		},
		false
	);
	return { "lookup_json", function_type, reinterpret_cast<void*>(floydrt_lookup_json) };
}



////////////////////////////////		json_to_string()



static runtime_value_t floydrt_json_to_string(floyd_runtime_t* frp, JSON_T* json_ptr){
	auto& r = get_floyd_runtime(frp);

	const auto& json = json_ptr->get_json();

	if(json.is_string()){
		return to_runtime_string(r, json.get_string());
	}
	else{
		quark::throw_runtime_error("Attempting to assign a non-string JSON to a string.");
	}
}

static function_bind_t floydrt_json_to_string__make(llvm::LLVMContext& context, const llvm_type_lookup& type_lookup){
	llvm::FunctionType* function_type = llvm::FunctionType::get(
		get_exact_llvm_type(type_lookup, typeid_t::make_string()),
		{
			make_frp_type(type_lookup),
			get_exact_llvm_type(type_lookup, typeid_t::make_json())
		},
		false
	);
	return { "json_to_string", function_type, reinterpret_cast<void*>(floydrt_json_to_string) };
}








//	STRUCT
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////



////////////////////////////////		allocate_struct()



//	Creates a new VEC_T with element_count. All elements are blank. Caller owns the result.
static STRUCT_T* floydrt_allocate_struct(floyd_runtime_t* frp, const runtime_type_t type, uint64_t size){
	auto& r = get_floyd_runtime(frp);

	const auto type0 = lookup_type(r.backend, type);
	auto v = alloc_struct(r.backend.heap, size, itype_t(type));
	return v;
}

static function_bind_t floydrt_allocate_struct__make(llvm::LLVMContext& context, const llvm_type_lookup& type_lookup){
	llvm::FunctionType* function_type = llvm::FunctionType::get(
		get_generic_struct_type(type_lookup)->getPointerTo(),
		{
			make_frp_type(type_lookup),
			make_runtime_type_type(type_lookup),
			llvm::Type::getInt64Ty(context)
		},
		false
	);
	return { "allocate_struct", function_type, reinterpret_cast<void*>(floydrt_allocate_struct) };
}



////////////////////////////////		floydrt_retain_struct()



static void floydrt_retain_struct(floyd_runtime_t* frp, STRUCT_T* v, runtime_type_t type0){
	auto& r = get_floyd_runtime(frp);

#if DEBUG
	const auto type = lookup_type(r.backend, type0);
	QUARK_ASSERT(is_rc_value(type));
	QUARK_ASSERT(type.is_struct());
#endif

	retain_struct(r.backend, make_runtime_struct(v), itype_t(type0));
}

static function_bind_t floydrt_retain_struct__make(llvm::LLVMContext& context, const llvm_type_lookup& type_lookup){
	llvm::FunctionType* function_type = llvm::FunctionType::get(
		llvm::Type::getVoidTy(context),
		{
			make_frp_type(type_lookup),
			get_generic_struct_type(type_lookup)->getPointerTo(),
			make_runtime_type_type(type_lookup)
		},
		false
	);
	return { "retain_struct", function_type, reinterpret_cast<void*>(floydrt_retain_struct) };
}



////////////////////////////////		floydrt_release_struct()



static void floydrt_release_struct(floyd_runtime_t* frp, STRUCT_T* v, runtime_type_t type0){
	auto& r = get_floyd_runtime(frp);
#if DEBUG
	const auto type = lookup_type(r.backend, type0);
	QUARK_ASSERT(type.is_struct());
	QUARK_ASSERT(v != nullptr);
#endif

	release_deep(r.backend, make_runtime_struct(v), itype_t(type0));
}

static function_bind_t floydrt_release_struct__make(llvm::LLVMContext& context, const llvm_type_lookup& type_lookup){
	llvm::FunctionType* function_type = llvm::FunctionType::get(
		llvm::Type::getVoidTy(context),
		{
			make_frp_type(type_lookup),
			get_generic_struct_type(type_lookup)->getPointerTo(),
			make_runtime_type_type(type_lookup)
		},
		false
	);
	return { "release_struct", function_type, reinterpret_cast<void*>(floydrt_release_struct) };
}



////////////////////////////////		floydrt_update_struct_member()



//??? move to value_features.cpp
//??? Split all intrinsics into dedicated for each type, even RC/vs simple types.
//??? optimize for speed. Most things can be precalculated.
//??? Generate an add_ref-function for each struct type.
static const STRUCT_T* floydrt_update_struct_member(floyd_runtime_t* frp, STRUCT_T* s, runtime_type_t struct_type, int64_t member_index, runtime_value_t new_value, runtime_type_t new_value_type){
	auto& r = get_floyd_runtime(frp);
	QUARK_ASSERT(s != nullptr);
	QUARK_ASSERT(member_index != -1);

	const auto type0 = lookup_type(r.backend, struct_type);
	const auto new_value_type0 = lookup_type(r.backend, new_value_type);
	QUARK_ASSERT(type0.is_struct());

	const auto source_struct_ptr = s;


	const auto& struct_def = type0.get_struct();

	const auto member_value = from_runtime_value(r, new_value, new_value_type0);

	//	Make copy of struct, overwrite member in copy.

	auto& struct_type_llvm = *get_exact_struct_type(r.type_lookup, type0);

	const llvm::DataLayout& data_layout = r.ee->getDataLayout();
	const llvm::StructLayout* layout = data_layout.getStructLayout(&struct_type_llvm);
	const auto struct_bytes = layout->getSizeInBytes();

	//??? Touches memory twice.
	auto struct_ptr = alloc_struct(r.backend.heap, struct_bytes, itype_t(struct_type));
	auto struct_base_ptr = struct_ptr->get_data_ptr();
	std::memcpy(struct_base_ptr, source_struct_ptr->get_data_ptr(), struct_bytes);

	const auto member_offset = layout->getElementOffset(static_cast<int>(member_index));
	const auto member_ptr0 = reinterpret_cast<void*>(struct_base_ptr + member_offset);
	store_via_ptr(r, new_value_type0, member_ptr0, member_value);

	//	Retain every member of new struct.
	{
		int i = 0;
		for(const auto& e: struct_def._members){
			if(is_rc_value(e._type)){
				const auto offset = layout->getElementOffset(i);
				const auto member_ptr = reinterpret_cast<const runtime_value_t*>(struct_base_ptr + offset);


				const auto member_itype = lookup_itype(r.backend, e._type);
				retain_value(r.backend, *member_ptr, member_itype);
			}
			i++;
		}
	}

	return struct_ptr;
}
static function_bind_t floydrt_update_struct_member__make(llvm::LLVMContext& context, const llvm_type_lookup& type_lookup){
	llvm::FunctionType* function_type = llvm::FunctionType::get(
		get_generic_struct_type(type_lookup)->getPointerTo(),
		{
			make_frp_type(type_lookup),
			get_generic_struct_type(type_lookup)->getPointerTo(),
			make_runtime_type_type(type_lookup),
			llvm::Type::getInt64Ty(context),
			make_runtime_value_type(type_lookup),
			make_runtime_type_type(type_lookup)
		},
		false
	);
	return { "update_struct_member", function_type, reinterpret_cast<void*>(floydrt_update_struct_member) };
}












//	OTHER
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////



////////////////////////////////		compare_values()




static int8_t floydrt_compare_values(floyd_runtime_t* frp, int64_t op, const runtime_type_t type, runtime_value_t lhs, runtime_value_t rhs){
	auto& r = get_floyd_runtime(frp);
	return (int8_t)compare_values(r.backend, op, type, lhs, rhs);
}

static function_bind_t floydrt_compare_values__make(llvm::LLVMContext& context, const llvm_type_lookup& type_lookup){
	llvm::FunctionType* function_type = llvm::FunctionType::get(
		llvm::Type::getInt1Ty(context),
		{
			make_frp_type(type_lookup),
			llvm::Type::getInt64Ty(context),
			make_runtime_type_type(type_lookup),
			make_runtime_value_type(type_lookup),
			make_runtime_value_type(type_lookup)
		},
		false
	);
	return { "compare_values", function_type, reinterpret_cast<void*>(floydrt_compare_values) };
}



static int64_t floydrt_get_profile_time(floyd_runtime_t* frp){
	get_floyd_runtime(frp);

	const std::chrono::time_point<std::chrono::high_resolution_clock> now = std::chrono::high_resolution_clock::now();
	const auto ns = std::chrono::duration_cast<std::chrono::nanoseconds>(now.time_since_epoch()).count();
	const auto result = int64_t(ns);
	return result;
}

static function_bind_t floydrt_get_profile_time__make(llvm::LLVMContext& context, const llvm_type_lookup& type_lookup){
	llvm::FunctionType* function_type = llvm::FunctionType::get(
		llvm::Type::getInt64Ty(context),
		{
			make_frp_type(type_lookup),
		},
		false
	);
	return { "get_profile_time", function_type, reinterpret_cast<void*>(floydrt_get_profile_time) };
}


static int64_t floydrt_analyse_benchmark_samples(floyd_runtime_t* frp, const int64_t* samples, int64_t index){
	const bool trace_flag = false;

	get_floyd_runtime(frp);

	QUARK_ASSERT(index >= 2);
	if(trace_flag){
		uint64_t total_acc = 0;
		for(int64_t i = 0 ; i < index ; i++){
			total_acc += samples[i];
			std::cout << samples[i] << std::endl;
		}
		std::cout << "Samples: " << index << "Total COW time: " << total_acc << std::endl;
	}

	const auto result = analyse_samples(samples, index);
	return result;
}
static function_bind_t floydrt_analyse_benchmark_samples__make(llvm::LLVMContext& context, const llvm_type_lookup& type_lookup){
	llvm::FunctionType* function_type = llvm::FunctionType::get(
		llvm::Type::getInt64Ty(context),
		{
			make_frp_type(type_lookup),
			llvm::Type::getInt64Ty(context)->getPointerTo(),
			llvm::Type::getInt64Ty(context)
		},
		false
	);
	return { "analyse_benchmark_samples", function_type, reinterpret_cast<void*>(floydrt_analyse_benchmark_samples) };
}


//??? Keep typeid_t for each, then convert to LLVM type. Can't go the other way.
std::vector<function_bind_t> get_runtime_function_binds(llvm::LLVMContext& context, const llvm_type_lookup& type_lookup){
	std::vector<function_bind_t> result = {
		floydrt_alloc_kstr__make(context, type_lookup),
		floydrt_allocate_vector__make(context, type_lookup),
		floydrt_allocate_vector_fill__make(context, type_lookup),

		{
			"retain_vec",
			llvm::FunctionType::get(
				llvm::Type::getVoidTy(context),
				{
					make_frp_type(type_lookup),
					make_generic_vec_type(type_lookup)->getPointerTo(),
					make_runtime_type_type(type_lookup)
				},
				false
			),
			reinterpret_cast<void*>(floydrt_retain_vec)
		},
		{
			"retain_vector_hamt",
			llvm::FunctionType::get(
				llvm::Type::getVoidTy(context),
				{
					make_frp_type(type_lookup),
					make_generic_vec_type(type_lookup)->getPointerTo(),
					make_runtime_type_type(type_lookup)
				},
				false
			),
			reinterpret_cast<void*>(floydrt_retain_vector_hamt)
		},

		{
			"release_vec",
			llvm::FunctionType::get(
				llvm::Type::getVoidTy(context),
				{
					make_frp_type(type_lookup),
					make_generic_vec_type(type_lookup)->getPointerTo(),
					make_runtime_type_type(type_lookup)
				},
				false
			),
			reinterpret_cast<void*>(floydrt_release_vec)
		},
		{
			"release_vector_hamt_pod",
			llvm::FunctionType::get(
				llvm::Type::getVoidTy(context),
				{
					make_frp_type(type_lookup),
					make_generic_vec_type(type_lookup)->getPointerTo(),
					make_runtime_type_type(type_lookup)
				},
				false
			),
			reinterpret_cast<void*>(floydrt_release_vector_hamt_pod)
		},


		floydrt_load_vector_element__make(context, type_lookup),
		floydrt_store_vector_element_mutable__make(context, type_lookup),
		floydrt_concatunate_vectors__make(context, type_lookup),
		floydrt_push_back_hamt_pod__make(context, type_lookup),
		floydrt_load_vector_element_hamt__make(context, type_lookup),

		floydrt_allocate_dict__make(context, type_lookup),
		floydrt_retain_dict__make(context, type_lookup),
		floydrt_release_dict__make(context, type_lookup),
		floydrt_lookup_dict__make(context, type_lookup),
		floydrt_store_dict_mutable__make(context, type_lookup),

		floydrt_allocate_json__make(context, type_lookup),
		floydrt_retain_json__make(context, type_lookup),
		floydrt_release_json__make(context, type_lookup),
		floydrt_lookup_json__make(context, type_lookup),
		floydrt_json_to_string__make(context, type_lookup),

		floydrt_allocate_struct__make(context, type_lookup),
		floydrt_retain_struct__make(context, type_lookup),
		floydrt_release_struct__make(context, type_lookup),
		floydrt_update_struct_member__make(context, type_lookup),

		floydrt_compare_values__make(context, type_lookup),
		floydrt_get_profile_time__make(context, type_lookup),
		floydrt_analyse_benchmark_samples__make(context, type_lookup)
	};
	return result;
}

//??? wrap app builder.CreateCalls() makes codegen typesafe too!



////////////////////////////////		runtime_functions_t


static const function_def_t& resolve_func(const std::vector<function_def_t>& function_defs, const std::string& name){
	return find_function_def_from_link_name(function_defs, encode_runtime_func_link_name(name));
}

runtime_functions_t::runtime_functions_t(const std::vector<function_def_t>& function_defs) :
	floydrt_init(resolve_func(function_defs, "init")),
	floydrt_deinit(resolve_func(function_defs, "deinit")),


	floydrt_alloc_kstr(resolve_func(function_defs, "alloc_kstr")),
	floydrt_allocate_vector(resolve_func(function_defs, "allocate_vector")),
	floydrt_allocate_vector_fill(resolve_func(function_defs, "allocate_vector_fill")),
	floydrt_retain_vec(resolve_func(function_defs, "retain_vec")),
	floydrt_retain_vector_hamt(resolve_func(function_defs, "retain_vector_hamt")),
	floydrt_release_vec(resolve_func(function_defs, "release_vec")),
	floydrt_release_vector_hamt_pod(resolve_func(function_defs, "release_vector_hamt_pod")),
	floydrt_load_vector_element(resolve_func(function_defs, "load_vector_element")),
	floydrt_store_vector_element_mutable(resolve_func(function_defs, "store_vector_element_mutable")),
	floydrt_concatunate_vectors(resolve_func(function_defs, "concatunate_vectors")),
	floydrt_push_back_hamt_pod(resolve_func(function_defs, "push_back_hamt_pod")),
	floydrt_load_vector_element_hamt(resolve_func(function_defs, "load_vector_element_hamt")),


	floydrt_allocate_dict(resolve_func(function_defs, "allocate_dict")),
	floydrt_retain_dict(resolve_func(function_defs, "retain_dict")),
	floydrt_release_dict(resolve_func(function_defs, "release_dict")),
	floydrt_lookup_dict(resolve_func(function_defs, "lookup_dict")),
	floydrt_store_dict_mutable(resolve_func(function_defs, "store_dict_mutable")),


	floydrt_allocate_json(resolve_func(function_defs, "allocate_json")),
	floydrt_retain_json(resolve_func(function_defs, "retain_json")),
	floydrt_release_json(resolve_func(function_defs, "release_json")),
	floydrt_lookup_json(resolve_func(function_defs, "lookup_json")),
	floydrt_json_to_string(resolve_func(function_defs, "json_to_string")),


	floydrt_allocate_struct(resolve_func(function_defs, "allocate_struct")),
	floydrt_retain_struct(resolve_func(function_defs, "retain_struct")),
	floydrt_release_struct(resolve_func(function_defs, "release_struct")),
	floydrt_update_struct_member(resolve_func(function_defs, "update_struct_member")),


	floydrt_compare_values(resolve_func(function_defs, "compare_values")),


	floydrt_get_profile_time(resolve_func(function_defs, "get_profile_time")),
	floydrt_analyse_benchmark_samples(resolve_func(function_defs, "analyse_benchmark_samples"))
{
}


} // floyd
