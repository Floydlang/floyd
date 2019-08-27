//
//  floyd_llvm_runtime.cpp
//  Floyd
//
//  Created by Marcus Zetterquist on 2019-04-24.
//  Copyright © 2019 Marcus Zetterquist. All rights reserved.
//

#include "floyd_llvm_runtime.h"

#include "floyd_llvm_codegen.h"
#include "floyd_runtime.h"
#include "floyd_llvm_corelib.h"
#include "value_features.h"

#include "text_parser.h"
#include "os_process.h"
#include "compiler_helpers.h"

#include <llvm/ExecutionEngine/ExecutionEngine.h>
#include <llvm/ExecutionEngine/GenericValue.h>
#include <llvm/ExecutionEngine/MCJIT.h>
#include <llvm/Support/TargetSelect.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/IR/DataLayout.h>

//#include "llvm/Bitcode/BitstreamWriter.h"


#include <thread>
#include <deque>
#include <condition_variable>
#include <iostream>



namespace floyd {


static const bool k_trace_messaging = false;




/*
@variable = global i32 21
define i32 @main() {
%1 = load i32, i32* @variable ; load the global variable
%2 = mul i32 %1, 2
store i32 %2, i32* @variable ; store instruction to write to global variable
ret i32 %2
}
*/

llvm_execution_engine_t& get_floyd_runtime(floyd_runtime_t* frp){
	QUARK_ASSERT(frp != nullptr);

	auto ptr = reinterpret_cast<llvm_execution_engine_t*>(frp);
	QUARK_ASSERT(ptr != nullptr);
	QUARK_ASSERT(ptr->debug_magic == k_debug_magic);
	QUARK_ASSERT(ptr->check_invariant());
	return *ptr;
}

static floyd_runtime_t* make_runtime_ptr(llvm_execution_engine_t* ee){
	return reinterpret_cast<floyd_runtime_t*>(ee);
}






const function_def_t& find_function_def_from_link_name(const std::vector<function_def_t>& function_defs, const link_name_t& link_name){
	auto it = std::find_if(function_defs.begin(), function_defs.end(), [&] (const function_def_t& e) { return e.link_name == link_name; } );
	QUARK_ASSERT(it != function_defs.end());

	QUARK_ASSERT(it->llvm_codegen_f != nullptr);
	return *it;
}






void* get_global_ptr(const llvm_execution_engine_t& ee, const std::string& name){
	QUARK_ASSERT(ee.check_invariant());
	QUARK_ASSERT(name.empty() == false);

	const auto addr = ee.ee->getGlobalValueAddress(name);
	return  (void*)addr;
}

static void* get_function_ptr(const llvm_execution_engine_t& ee, const link_name_t& name){
	QUARK_ASSERT(ee.check_invariant());
	QUARK_ASSERT(name.s.empty() == false);

	const auto addr = ee.ee->getFunctionAddress(name.s);
	return (void*)addr;
}


std::pair<void*, typeid_t> bind_global(const llvm_execution_engine_t& ee, const std::string& name){
	QUARK_ASSERT(ee.check_invariant());
	QUARK_ASSERT(name.empty() == false);

	const auto global_ptr = get_global_ptr(ee, name);
	if(global_ptr != nullptr){
		auto symbol = find_symbol(ee.global_symbols, name);
		QUARK_ASSERT(symbol != nullptr);

		return { global_ptr, symbol->get_type() };
	}
	else{
		return { nullptr, typeid_t::make_undefined() };
	}
}

static value_t load_via_ptr(const llvm_execution_engine_t& runtime, const void* value_ptr, const typeid_t& type){
	QUARK_ASSERT(runtime.check_invariant());
	QUARK_ASSERT(value_ptr != nullptr);
	QUARK_ASSERT(type.check_invariant());

	const auto result = load_via_ptr2(value_ptr, type);
	const auto result2 = from_runtime_value(runtime, result, type);
	return result2;
}

value_t load_global(const llvm_execution_engine_t& ee, const std::pair<void*, typeid_t>& v){
	QUARK_ASSERT(v.first != nullptr);
	QUARK_ASSERT(v.second.is_undefined() == false);

	return load_via_ptr(ee, v.first, v.second);
}

static void store_via_ptr(llvm_execution_engine_t& runtime, const typeid_t& member_type, void* value_ptr, const value_t& value){
	QUARK_ASSERT(runtime.check_invariant());
	QUARK_ASSERT(member_type.check_invariant());
	QUARK_ASSERT(value_ptr != nullptr);
	QUARK_ASSERT(value.check_invariant());

	const auto value2 = to_runtime_value(runtime, value);
	store_via_ptr2(value_ptr, member_type, value2);
}


llvm_bind_t bind_function2(llvm_execution_engine_t& ee, const link_name_t& name){
	QUARK_ASSERT(ee.check_invariant());
	QUARK_ASSERT(name.s.empty() == false);

	const auto f = get_function_ptr(ee, name);
	if(f != nullptr){
		const auto def = find_function_def_from_link_name(ee.function_defs, name);
		const auto function_type = def.floyd_fundef._function_type;
		return llvm_bind_t {
			name,
			f,
			function_type
		};
	}
	else{
		return llvm_bind_t {
			name,
			nullptr,
			typeid_t::make_undefined()
		};
	}
}

int64_t llvm_call_main(llvm_execution_engine_t& ee, const llvm_bind_t& f, const std::vector<std::string>& main_args){
	QUARK_ASSERT(f.address != nullptr);

	//??? Check this earlier.
	if(f.type == get_main_signature_arg_impure() || f.type == get_main_signature_arg_pure()){
		const auto f2 = reinterpret_cast<FLOYD_RUNTIME_MAIN_ARGS_IMPURE>(f.address);

		//??? Slow path via value_t
		std::vector<value_t> main_args2;
		for(const auto& e: main_args){
			main_args2.push_back(value_t::make_string(e));
		}
		const auto main_args3 = value_t::make_vector_value(typeid_t::make_string(), main_args2);
		const auto main_args4 = to_runtime_value(ee, main_args3);
		const auto main_result_int = (*f2)(make_runtime_ptr(&ee), main_args4);
		release_deep(ee.backend, main_args4, typeid_t::make_vector(typeid_t::make_string()));
		return main_result_int;
	}
	else if(f.type == get_main_signature_no_arg_impure() || f.type == get_main_signature_no_arg_pure()){
		const auto f2 = reinterpret_cast<FLOYD_RUNTIME_MAIN_NO_ARGS_IMPURE>(f.address);
		const auto main_result_int = (*f2)(make_runtime_ptr(&ee));
		return main_result_int;
	}
	else{
		throw std::exception();
	}
}




////////////////////////////////	HELPERS FOR RUNTIME CALLBACKS




static std::string gen_to_string(llvm_execution_engine_t& runtime, runtime_value_t arg_value, runtime_type_t arg_type){
	QUARK_ASSERT(runtime.check_invariant());

	const auto type = lookup_type(runtime.backend, arg_type);
	const auto value = from_runtime_value(runtime, arg_value, type);
	const auto a = to_compact_string2(value);
	return a;
}





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
		return alloc_vector_ccpvector2(r.backend.heap, element_count, element_count, type1);
	}
	else if(is_vector_hamt(type1)){
		return alloc_vector_hamt(r.backend.heap, element_count, element_count, type1);
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



////////////////////////////////		allocate_vector_fill()



static runtime_value_t floydrt_allocate_vector_fill(floyd_runtime_t* frp, runtime_type_t type, runtime_value_t* elements, uint64_t element_count){
	auto& r = get_floyd_runtime(frp);

	const auto type1 = lookup_type(r.backend, type);
	if(is_vector_cppvector(type1)){
		return alloc_vector_ccpvector2(r.backend.heap, element_count, element_count, type1);
	}
	else if(is_vector_hamt(type1)){
		return alloc_vector_hamt(r.backend.heap, element_count, element_count, type1);
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
	const auto type = lookup_type(r.backend, type0);
#if DEBUG
	QUARK_ASSERT(type.is_string() || type.is_vector());
	QUARK_ASSERT(is_rc_value(type));
#endif

	retain_value(r.backend, vec, type);
}

static void floydrt_retain_vector_hamt(floyd_runtime_t* frp, runtime_value_t vec, runtime_type_t type0){
	auto& r = get_floyd_runtime(frp);
	const auto type = lookup_type(r.backend, type0);
#if DEBUG
	QUARK_ASSERT(type.is_string() || type.is_vector());
	QUARK_ASSERT(is_rc_value(type));
#endif

	retain_vector_hamt(r.backend, vec, type);
}




////////////////////////////////		floydrt_release_vec()



static void floydrt_release_vec(floyd_runtime_t* frp, runtime_value_t vec, runtime_type_t type0){
	auto& r = get_floyd_runtime(frp);
	const auto type = lookup_type(r.backend, type0);
	QUARK_ASSERT(type.is_string() || type.is_vector());

	release_deep(r.backend, vec, type);
}
static void floydrt_release_vector_hamt_pod(floyd_runtime_t* frp, runtime_value_t vec, runtime_type_t type0){
	auto& r = get_floyd_runtime(frp);
	const auto type = lookup_type(r.backend, type0);
	QUARK_ASSERT(is_vector_hamt(type));

	QUARK_ASSERT(is_rc_value(type.get_vector_element_type()) == false);

	release_vector_hamt_pod(r.backend, vec, type);
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






static runtime_value_t floydrt_push_back__hamt__pod64(floyd_runtime_t* frp, runtime_value_t vec, runtime_value_t element){
	return push_back_immutable(vec, element);
}

static function_bind_t floydrt_push_back__hamt__pod64__make(llvm::LLVMContext& context, const llvm_type_lookup& type_lookup){
	llvm::FunctionType* function_type = llvm::FunctionType::get(
		make_generic_vec_type(type_lookup)->getPointerTo(),
		{
			make_frp_type(type_lookup),
			make_generic_vec_type(type_lookup)->getPointerTo(),
			make_runtime_value_type(type_lookup)
		},
		false
	);
	return { "push_back__hamt__pod64", function_type, reinterpret_cast<void*>(floydrt_push_back__hamt__pod64) };
}




//	DICT
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////






////////////////////////////////		allocate_dict()



static const runtime_value_t floydrt_allocate_dict(floyd_runtime_t* frp, runtime_type_t type){
	auto& r = get_floyd_runtime(frp);

	const auto type0 = lookup_type(r.backend, type);
	if(is_dict_cppmap(type0)){
		return alloc_dict_cppmap(r.backend.heap, type0);
	}
	else if(is_dict_hamt(type0)){
		return alloc_dict_hamt(r.backend.heap, type0);
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
	const auto type = lookup_type(r.backend, type0);
	QUARK_ASSERT(is_rc_value(type));
	QUARK_ASSERT(type.is_dict());

	retain_value(r.backend, dict, type);
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
	const auto type = lookup_type(r.backend, type0);
	QUARK_ASSERT(type.is_dict());

	release_deep(r.backend, dict, type);
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
	auto v = alloc_struct(r.backend.heap, size, type0);
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

	const auto type = lookup_type(r.backend, type0);
	QUARK_ASSERT(is_rc_value(type));
	QUARK_ASSERT(type.is_struct());

	retain_struct(r.backend, make_runtime_struct(v), type);
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
	const auto type = lookup_type(r.backend, type0);
	QUARK_ASSERT(type.is_struct());

	QUARK_ASSERT(v != nullptr);
	release_deep(r.backend, make_runtime_struct(v), type);
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
	auto struct_ptr = alloc_struct(r.backend.heap, struct_bytes, type0);
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
				retain_value(r.backend, *member_ptr, e._type);
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
std::vector<function_bind_t> get_runtime_functions(llvm::LLVMContext& context, const llvm_type_lookup& type_lookup){
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
		floydrt_push_back__hamt__pod64__make(context, type_lookup),

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

static runtime_value_t floyd_llvm_intrinsic__erase(floyd_runtime_t* frp, runtime_value_t arg0_value, runtime_type_t arg0_type, runtime_value_t arg1_value, runtime_type_t arg1_type){
	auto& r = get_floyd_runtime(frp);

	const auto type0 = lookup_type(r.backend, arg0_type);
	const auto type1 = lookup_type(r.backend, arg1_type);

	QUARK_ASSERT(type0.is_dict());
	QUARK_ASSERT(type1.is_string());

	if(is_dict_cppmap(type0)){
		const auto& dict = unpack_dict_cppmap_arg(r.backend, arg0_value, arg0_type);

		const auto value_type = type0.get_dict_value_type();

		//	Deep copy dict.
		auto dict2 = alloc_dict_cppmap(r.backend.heap, type0);
		auto& m = dict2.dict_cppmap_ptr->get_map_mut();
		m = dict->get_map();

		const auto key_string = from_runtime_string(r, arg1_value);
		m.erase(key_string);

		if(is_rc_value(value_type)){
			for(auto& e: m){
				retain_value(r.backend, e.second, value_type);
			}
		}
		return dict2;
	}
	else if(is_dict_hamt(type0)){
		const auto& dict = *arg0_value.dict_hamt_ptr;

		const auto value_type = type0.get_dict_value_type();

		//	Deep copy dict.
		auto dict2 = alloc_dict_hamt(r.backend.heap, type0);
		auto& m = dict2.dict_hamt_ptr->get_map_mut();
		m = dict.get_map();

		const auto key_string = from_runtime_string(r, arg1_value);
		m = m.erase(key_string);

		if(is_rc_value(value_type)){
			for(auto& e: m){
				retain_value(r.backend, e.second, value_type);
			}
		}
		return dict2;
	}
	else{
		QUARK_ASSERT(false);
		throw std::exception();
	}
}





//??? We need to figure out the return type *again*, knowledge we have already in semast.
static runtime_value_t floyd_llvm_intrinsic__get_keys(floyd_runtime_t* frp, runtime_value_t arg0_value, runtime_type_t arg0_type){
	auto& r = get_floyd_runtime(frp);

	const auto type0 = lookup_type(r.backend, arg0_type);
	QUARK_ASSERT(type0.is_dict());

	if(is_dict_cppmap(type0)){
		if(k_global_vector_type == vector_backend::cppvector){
			return get_keys__cppmap_cppvector(r.backend, arg0_value, arg0_type);
		}
		else if(k_global_vector_type == vector_backend::hamt){
			return get_keys__cppmap_hamt(r.backend, arg0_value, arg0_type);
		}
		else{
			QUARK_ASSERT(false);
			throw std::exception();
		}
	}
	else if(is_dict_hamt(type0)){
		if(k_global_vector_type == vector_backend::cppvector){
			return get_keys__hamtmap_cppvector(r.backend, arg0_value, arg0_type);
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

//??? kill unpack_dict_cppmap_arg()
static uint32_t floyd_llvm_intrinsic__exists(floyd_runtime_t* frp, runtime_value_t arg0_value, runtime_type_t arg0_type, runtime_value_t arg1_value, runtime_type_t arg1_type){
	auto& r = get_floyd_runtime(frp);

	const auto type0 = lookup_type(r.backend, arg0_type);
	const auto type1 = lookup_type(r.backend, arg1_type);
	QUARK_ASSERT(type0.is_dict());

	if(is_dict_cppmap(type0)){
		const auto& dict = unpack_dict_cppmap_arg(r.backend, arg0_value, arg0_type);
		const auto key_string = from_runtime_string(r, arg1_value);

		const auto& m = dict->get_map();
		const auto it = m.find(key_string);
		return it != m.end() ? 1 : 0;
	}
	else if(is_dict_hamt(type0)){
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



static int64_t floyd_llvm_intrinsic__find(floyd_runtime_t* frp, runtime_value_t arg0_value, runtime_type_t arg0_type, const runtime_value_t arg1_value, runtime_type_t arg1_type){
	auto& r = get_floyd_runtime(frp);

	const auto type0 = lookup_type(r.backend, arg0_type);
	if(type0.is_string()){
		return find__string(r.backend, arg0_value, arg0_type, arg1_value, arg1_type);
	}
	else if(is_vector_cppvector(type0)){
		return find__cppvector(r.backend, arg0_value, arg0_type, arg1_value, arg1_type);
	}
	else if(is_vector_hamt(type0)){
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
	const auto target_type2 = lookup_type(r.backend, target_type);

	const auto result = unflatten_json_to_specific_type(json, target_type2);
	const auto result2 = to_runtime_value(r, result);
	return result2;
}


//??? No need to call lookup_type() to check which basic type it is!












typedef runtime_value_t (*MAP_F)(floyd_runtime_t* frp, runtime_value_t arg0_value, runtime_value_t arg1_value);

static runtime_value_t map__cppvector(floyd_runtime_t* frp, value_backend_t& backend, runtime_value_t elements_vec, runtime_type_t elements_vec_type, runtime_value_t f_value, runtime_type_t f_value_type, runtime_value_t context_value, runtime_type_t context_value_type){
	QUARK_ASSERT(backend.check_invariant());

	const auto type0 = lookup_type(backend, elements_vec_type);
	const auto type1 = lookup_type(backend, f_value_type);
	const auto type2 = lookup_type(backend, context_value_type);
	QUARK_ASSERT(check_map_func_type(type0, type1, type2));

	const auto e_type = type0.get_vector_element_type();
	const auto f_arg_types = type1.get_function_args();
	const auto r_type = type1.get_function_return();
	const auto f = reinterpret_cast<MAP_F>(f_value.function_ptr);

	const auto return_type = typeid_t::make_vector(r_type);
	const auto count = elements_vec.vector_cppvector_ptr->get_element_count();
	auto result_vec = alloc_vector_ccpvector2(backend.heap, count, count, return_type);
	for(int i = 0 ; i < count ; i++){
		const auto a = (*f)(frp, elements_vec.vector_cppvector_ptr->get_element_ptr()[i], context_value);
		result_vec.vector_cppvector_ptr->get_element_ptr()[i] = a;
	}
	return result_vec;
}

static runtime_value_t map__hamt(floyd_runtime_t* frp, value_backend_t& backend, runtime_value_t elements_vec, runtime_type_t elements_vec_type, runtime_value_t f_value, runtime_type_t f_value_type, runtime_value_t context_value, runtime_type_t context_value_type){
	QUARK_ASSERT(backend.check_invariant());

	const auto type0 = lookup_type(backend, elements_vec_type);
	const auto type1 = lookup_type(backend, f_value_type);
	const auto type2 = lookup_type(backend, context_value_type);
	QUARK_ASSERT(check_map_func_type(type0, type1, type2));

	const auto e_type = type0.get_vector_element_type();
	const auto f_arg_types = type1.get_function_args();
	const auto r_type = type1.get_function_return();
	const auto f = reinterpret_cast<MAP_F>(f_value.function_ptr);

	const auto return_type = typeid_t::make_vector(r_type);
	const auto count = elements_vec.vector_hamt_ptr->get_element_count();
	auto result_vec = alloc_vector_hamt(backend.heap, count, count, return_type);
	for(int i = 0 ; i < count ; i++){
		const auto& element = elements_vec.vector_hamt_ptr->load_element(i);
		const auto a = (*f)(frp, element, context_value);
		result_vec.vector_hamt_ptr->store_mutate(i, a);
	}
	return result_vec;
}

//	[R] map([E] elements, func R (E e, C context) f, C context)
static runtime_value_t floyd_llvm_intrinsic__map(floyd_runtime_t* frp, runtime_value_t arg0_value, runtime_type_t arg0_type, runtime_value_t arg1_value, runtime_type_t arg1_type, runtime_value_t arg2_value, runtime_type_t arg2_type){
	auto& r = get_floyd_runtime(frp);

	const auto type0 = lookup_type(r.backend, arg0_type);
	if(is_vector_cppvector(type0)){
		return map__cppvector(frp, r.backend, arg0_value, arg0_type, arg1_value, arg1_type, arg2_value, arg2_type);
	}
	else if(is_vector_hamt(type0)){
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
		lookup_type(r.backend, func_type),
		lookup_type(r.backend, context_type)
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

static runtime_value_t map_dag__cppvector(
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

	const auto type0 = lookup_type(backend, arg0_type);
	const auto type1 = lookup_type(backend, arg1_type);
	const auto type2 = lookup_type(backend, arg2_type);
	QUARK_ASSERT(check_map_dag_func_type(type0, type1, type2, lookup_type(backend, context_type)));

	const auto& elements = arg0_value;
	const auto& e_type = type0.get_vector_element_type();
	const auto& parents = arg1_value;
	const auto& f = arg2_value;
	const auto& r_type = type2.get_function_return();

	QUARK_ASSERT(e_type == type2.get_function_args()[0] && r_type == type2.get_function_args()[1].get_vector_element_type());

	QUARK_ASSERT(is_vector_cppvector(typeid_t::make_vector(e_type)));
	QUARK_ASSERT(is_vector_cppvector(typeid_t::make_vector(r_type)));

	const auto return_type = typeid_t::make_vector(r_type);

	const auto f2 = reinterpret_cast<map_dag_F>(f.function_ptr);

	const auto elements2 = elements.vector_cppvector_ptr;
	const auto parents2 = parents.vector_cppvector_ptr;

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

			auto solved_deps2 = alloc_vector_ccpvector2(backend.heap, solved_deps.size(), solved_deps.size(), return_type);
			for(int i = 0 ; i < solved_deps.size() ; i++){
				solved_deps2.vector_cppvector_ptr->store(i, solved_deps[i]);
			}
			runtime_value_t solved_deps3 = solved_deps2;

			const auto result1 = (*f2)(frp, e, solved_deps3, context);

			//	Release just the vec, **not the elements**. The elements are aliases for complete-vector.
			if(dec_rc(solved_deps2.vector_cppvector_ptr->alloc) == 0){
				dispose_vector_cppvector(solved_deps2);
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
	auto result_vec = alloc_vector_ccpvector2(backend.heap, count, count, return_type);
	for(int i = 0 ; i < count ; i++){
//		retain_value(r, complete[i], r_type);
		result_vec.vector_cppvector_ptr->store(i, complete[i]);
	}

	return result_vec;
}

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

	const auto type0 = lookup_type(backend, arg0_type);
	const auto type1 = lookup_type(backend, arg1_type);
	const auto type2 = lookup_type(backend, arg2_type);
	QUARK_ASSERT(check_map_dag_func_type(type0, type1, type2, lookup_type(backend, context_type)));

	const auto& elements = arg0_value;
	const auto& e_type = type0.get_vector_element_type();
	const auto& parents = arg1_value;
	const auto& f = arg2_value;
	const auto& r_type = type2.get_function_return();

	QUARK_ASSERT(e_type == type2.get_function_args()[0] && r_type == type2.get_function_args()[1].get_vector_element_type());

	QUARK_ASSERT(is_vector_hamt(typeid_t::make_vector(e_type)));
	QUARK_ASSERT(is_vector_hamt(typeid_t::make_vector(r_type)));

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

			auto solved_deps2 = alloc_vector_hamt(backend.heap, solved_deps.size(), solved_deps.size(), return_type);
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
	auto result_vec = alloc_vector_hamt(backend.heap, count, count, return_type);
	for(int i = 0 ; i < count ; i++){
//		retain_value(r, complete[i], r_type);
		result_vec.vector_hamt_ptr->store_mutate(i, complete[i]);
	}

	return result_vec;
}

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

	const auto type0 = lookup_type(r.backend, arg0_type);
	if(is_vector_cppvector(type0)){
		return map_dag__cppvector(frp, r.backend, arg0_value, arg0_type, arg1_value, arg1_type, arg2_value, arg2_type, context, context_type);
	}
	else if(is_vector_hamt(type0)){
		return map_dag__hamt(frp, r.backend, arg0_value, arg0_type, arg1_value, arg1_type, arg2_value, arg2_type, context, context_type);
	}
	else{
		QUARK_ASSERT(false);
		throw std::exception();
	}
}







typedef runtime_value_t (*FILTER_F)(floyd_runtime_t* frp, runtime_value_t element_value, runtime_value_t context);

static runtime_value_t filter__cppvector(floyd_runtime_t* frp, value_backend_t& backend, runtime_value_t arg0_value, runtime_type_t arg0_type, runtime_value_t arg1_value, runtime_type_t arg1_type, runtime_value_t context, runtime_type_t context_type){
	QUARK_ASSERT(backend.check_invariant());

	auto& r = get_floyd_runtime(frp);

	const auto type0 = lookup_type(r.backend, arg0_type);
	const auto type1 = lookup_type(r.backend, arg1_type);
	const auto type2 = lookup_type(r.backend, context_type);
	const auto return_type = type0;

	QUARK_ASSERT(check_filter_func_type(type0, type1, type2));
	QUARK_ASSERT(is_vector_cppvector(type0));

	const auto& vec = *arg0_value.vector_cppvector_ptr;
	const auto f = reinterpret_cast<FILTER_F>(arg1_value.function_ptr);

	auto count = vec.get_element_count();

	const auto e_element_type = type0.get_vector_element_type();

	std::vector<runtime_value_t> acc;
	for(int i = 0 ; i < count ; i++){
		const auto element_value = vec.get_element_ptr()[i];
		const auto keep = (*f)(frp, element_value, context);
		if(keep.bool_value != 0){
			acc.push_back(element_value);

			if(is_rc_value(e_element_type)){
				retain_value(r.backend, element_value, e_element_type);
			}
		}
		else{
		}
	}

	const auto count2 = acc.size();
	auto result_vec = alloc_vector_ccpvector2(r.backend.heap, count2, count2, return_type);

	if(count2 > 0){
		//	Count > 0 required to get address to first element in acc.
		copy_elements(result_vec.vector_cppvector_ptr->get_element_ptr(), &acc[0], count2);
	}
	return result_vec;
}

static runtime_value_t filter__hamt(floyd_runtime_t* frp, value_backend_t& backend, runtime_value_t arg0_value, runtime_type_t arg0_type, runtime_value_t arg1_value, runtime_type_t arg1_type, runtime_value_t context, runtime_type_t context_type){
	QUARK_ASSERT(backend.check_invariant());

	auto& r = get_floyd_runtime(frp);

	const auto type0 = lookup_type(r.backend, arg0_type);
	const auto type1 = lookup_type(r.backend, arg1_type);
	const auto type2 = lookup_type(r.backend, context_type);
	const auto return_type = type0;

	QUARK_ASSERT(check_filter_func_type(type0, type1, type2));
	QUARK_ASSERT(is_vector_hamt(type0));

	const auto& vec = *arg0_value.vector_hamt_ptr;
	const auto f = reinterpret_cast<FILTER_F>(arg1_value.function_ptr);

	auto count = vec.get_element_count();

	const auto e_element_type = type0.get_vector_element_type();

	std::vector<runtime_value_t> acc;
	for(int i = 0 ; i < count ; i++){
		const auto element_value = vec.load_element(i);
		const auto keep = (*f)(frp, element_value, context);
		if(keep.bool_value != 0){
			acc.push_back(element_value);

			if(is_rc_value(e_element_type)){
				retain_value(r.backend, element_value, e_element_type);
			}
		}
		else{
		}
	}

	const auto count2 = acc.size();
	auto result_vec = alloc_vector_hamt(r.backend.heap, &acc[0], count2, return_type);
	return result_vec;
}

//	[E] filter([E] elements, func bool (E e, C context) f, C context)
static runtime_value_t floyd_llvm_intrinsic__filter(floyd_runtime_t* frp, runtime_value_t arg0_value, runtime_type_t arg0_type, runtime_value_t arg1_value, runtime_type_t arg1_type, runtime_value_t arg2_value, runtime_type_t arg2_type){
	auto& r = get_floyd_runtime(frp);

	const auto type0 = lookup_type(r.backend, arg0_type);
	if(is_vector_cppvector(type0)){
		return filter__cppvector(frp, r.backend, arg0_value, arg0_type, arg1_value, arg1_type, arg2_value, arg2_type);
	}
	else if(is_vector_hamt(type0)){
		return filter__hamt(frp, r.backend, arg0_value, arg0_type, arg1_value, arg1_type, arg2_value, arg2_type);
	}
	else{
		QUARK_ASSERT(false);
		throw std::exception();
	}
}



typedef runtime_value_t (*REDUCE_F)(floyd_runtime_t* frp, runtime_value_t acc_value, runtime_value_t element_value, runtime_value_t context);

static runtime_value_t reduce__cppvector(floyd_runtime_t* frp, value_backend_t& backend, runtime_value_t arg0_value, runtime_type_t arg0_type, runtime_value_t arg1_value, runtime_type_t arg1_type, runtime_value_t arg2_value, runtime_type_t arg2_type, runtime_value_t context, runtime_type_t context_type){
	QUARK_ASSERT(backend.check_invariant());

	const auto type0 = lookup_type(backend, arg0_type);
	const auto type1 = lookup_type(backend, arg1_type);
	const auto type2 = lookup_type(backend, arg2_type);

	QUARK_ASSERT(check_reduce_func_type(type0, type1, type2, lookup_type(backend, context_type)));
	QUARK_ASSERT(is_vector_cppvector(type0));

	const auto& vec = *arg0_value.vector_cppvector_ptr;
	const auto& init = arg1_value;
	const auto f = reinterpret_cast<REDUCE_F>(arg2_value.function_ptr);

	auto count = vec.get_element_count();
	runtime_value_t acc = init;
	retain_value(backend, acc, type1);

	for(int i = 0 ; i < count ; i++){
		const auto element_value = vec.get_element_ptr()[i];
		const auto acc2 = (*f)(frp, acc, element_value, context);

		release_deep(backend, acc, type1);
		acc = acc2;
	}
	return acc;
}

static runtime_value_t reduce__hamt(floyd_runtime_t* frp, value_backend_t& backend, runtime_value_t arg0_value, runtime_type_t arg0_type, runtime_value_t arg1_value, runtime_type_t arg1_type, runtime_value_t arg2_value, runtime_type_t arg2_type, runtime_value_t context, runtime_type_t context_type){
	QUARK_ASSERT(backend.check_invariant());

	const auto type0 = lookup_type(backend, arg0_type);
	const auto type1 = lookup_type(backend, arg1_type);
	const auto type2 = lookup_type(backend, arg2_type);

	QUARK_ASSERT(check_reduce_func_type(type0, type1, type2, lookup_type(backend, context_type)));
	QUARK_ASSERT(is_vector_hamt(type0));

	const auto& vec = *arg0_value.vector_hamt_ptr;
	const auto& init = arg1_value;
	const auto f = reinterpret_cast<REDUCE_F>(arg2_value.function_ptr);

	auto count = vec.get_element_count();
	runtime_value_t acc = init;
	retain_value(backend, acc, type1);

	for(int i = 0 ; i < count ; i++){
		const auto element_value = vec.load_element(i);
		const auto acc2 = (*f)(frp, acc, element_value, context);

		release_deep(backend, acc, type1);
		acc = acc2;
	}
	return acc;
}

//	R reduce([E] elements, R accumulator_init, func R (R accumulator, E element, C context) f, C context)
static runtime_value_t floyd_llvm_intrinsic__reduce(floyd_runtime_t* frp, runtime_value_t arg0_value, runtime_type_t arg0_type, runtime_value_t arg1_value, runtime_type_t arg1_type, runtime_value_t arg2_value, runtime_type_t arg2_type, runtime_value_t context, runtime_type_t context_type){
	auto& r = get_floyd_runtime(frp);

	const auto type0 = lookup_type(r.backend, arg0_type);
	if(is_vector_cppvector(type0)){
		return reduce__cppvector(frp, r.backend, arg0_value, arg0_type, arg1_value, arg1_type, arg2_value, arg2_type, context, context_type);
	}
	else if(is_vector_hamt(type0)){
		return reduce__hamt(frp, r.backend, arg0_value, arg0_type, arg1_value, arg1_type, arg2_value, arg2_type, context, context_type);
	}
	else{
		QUARK_ASSERT(false);
		throw std::exception();
	}
}





typedef uint8_t (*stable_sort_F)(floyd_runtime_t* frp, runtime_value_t arg0_value, runtime_value_t arg1_value, runtime_value_t arg2_value);

static runtime_value_t stable_sort__cppvector(
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

	const auto type0 = lookup_type(backend, arg0_type);
	const auto type1 = lookup_type(backend, arg1_type);
	const auto type2 = lookup_type(backend, arg2_type);

	QUARK_ASSERT(check_stable_sort_func_type(type0, type1, type2));
	QUARK_ASSERT(is_vector_cppvector(type0));

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

	const auto result = to_runtime_value2(backend, value_t::make_vector_value(type0, mutate_inplace_elements));
	return result;
}

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

	const auto type0 = lookup_type(backend, arg0_type);
	const auto type1 = lookup_type(backend, arg1_type);
	const auto type2 = lookup_type(backend, arg2_type);

	QUARK_ASSERT(check_stable_sort_func_type(type0, type1, type2));
	QUARK_ASSERT(is_vector_hamt(type0));

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

	const auto result = to_runtime_value2(backend, value_t::make_vector_value(type0, mutate_inplace_elements));
	return result;
}

//	[T] stable_sort([T] elements, bool less(T left, T right, C context), C context)
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

	const auto type0 = lookup_type(r.backend, arg0_type);
	if(is_vector_cppvector(type0)){
		return stable_sort__cppvector(frp, r.backend, arg0_value, arg0_type, arg1_value, arg1_type, arg2_value, arg2_type);
	}
	else if(is_vector_hamt(type0)){
		return stable_sort__hamt(frp, r.backend, arg0_value, arg0_type, arg1_value, arg1_type, arg2_value, arg2_type);
	}
	else{
		QUARK_ASSERT(false);
		throw std::exception();
	}
}








void floyd_llvm_intrinsic__print(floyd_runtime_t* frp, runtime_value_t arg0_value, runtime_type_t arg0_type){
	auto& r = get_floyd_runtime(frp);

	const auto s = gen_to_string(r, arg0_value, arg0_type);
	printf("%s\n", s.c_str());
	r._print_output.push_back(s);
}





static runtime_value_t floyd_llvm_intrinsic__push_back(floyd_runtime_t* frp, runtime_value_t arg0_value, runtime_type_t arg0_type, runtime_value_t arg1_value, runtime_type_t arg1_type){
	auto& r = get_floyd_runtime(frp);

	const auto type0 = lookup_type(r.backend, arg0_type);
	const auto type1 = lookup_type(r.backend, arg1_type);
	const auto return_type = type0;
	if(type0.is_string()){
		auto value = from_runtime_string(r, arg0_value);

		QUARK_ASSERT(type1.is_int());

		value.push_back((char)arg1_value.int_value);
		const auto result2 = to_runtime_string(r, value);
		return result2;
	}
	else if(is_vector_cppvector(type0)){
		const auto vs = unpack_vector_cppvector_arg(r.backend, arg0_value, arg0_type);

		QUARK_ASSERT(type1 == type0.get_vector_element_type());
		QUARK_ASSERT(is_vector_cppvector(type0));

		const auto element = arg1_value;
		const auto element_type = type1;

		const auto element_count2 = vs->get_element_count() + 1;
		auto v2 = alloc_vector_ccpvector2(r.backend.heap, element_count2, element_count2, return_type);

		auto dest_ptr = v2.vector_cppvector_ptr->get_element_ptr();
		auto source_ptr = vs->get_element_ptr();

		if(is_rc_value(element_type)){
			retain_value(r.backend, element, element_type);

			for(int i = 0 ; i < vs->get_element_count() ; i++){
				retain_value(r.backend, source_ptr[i], element_type);
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
	else if(is_vector_hamt(type0)){
		QUARK_ASSERT(type1 == type0.get_vector_element_type());

		const auto element_type = type1;

		runtime_value_t vec2 = push_back_immutable(arg0_value, arg1_value);

		if(is_rc_value(element_type)){
			for(int i = 0 ; i < vec2.vector_hamt_ptr->get_element_count() ; i++){
				const auto& value = vec2.vector_hamt_ptr->load_element(i);
				retain_value(r.backend, value, element_type);
			}
		}
		return vec2;
	}
	else{
		//	No other types allowed.
		UNSUPPORTED();
	}
}




static const runtime_value_t floyd_llvm_intrinsic__replace(floyd_runtime_t* frp, runtime_value_t arg0_value, runtime_type_t arg0_type, uint64_t start, uint64_t end, runtime_value_t arg3_value, runtime_type_t arg3_type){
	auto& r = get_floyd_runtime(frp);

	const auto type0 = lookup_type(r.backend, arg0_type);
	const auto type3 = lookup_type(r.backend, arg3_type);

	QUARK_ASSERT(type3 == type0);

	if(type0.is_string()){
		return replace__string(r.backend, arg0_value, arg0_type, start, end, arg3_value, arg3_type);
	}
	else if(is_vector_cppvector(type0)){
		return replace__cppvector(r.backend, arg0_value, arg0_type, start, end, arg3_value, arg3_type);
	}
	else if(is_vector_hamt(type0)){
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

static void floyd_llvm_intrinsic__send(floyd_runtime_t* frp, runtime_value_t process_id0, const JSON_T* message_json_ptr){
	auto& r = get_floyd_runtime(frp);

	QUARK_ASSERT(message_json_ptr != nullptr);

	const auto& process_id = from_runtime_string(r, process_id0);
	const auto& message_json = message_json_ptr->get_json();

	if(k_trace_messaging){
		QUARK_TRACE_SS("send(\"" << process_id << "\"," << json_to_pretty_string(message_json) <<")");
	}

	r._handler->on_send(process_id, message_json);
}

static int64_t floyd_llvm_intrinsic__size(floyd_runtime_t* frp, runtime_value_t arg0_value, runtime_type_t arg0_type){
	auto& r = get_floyd_runtime(frp);

	const auto type0 = lookup_type(r.backend, arg0_type);

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
	else if(is_vector_cppvector(type0)){
		const auto vs = unpack_vector_cppvector_arg(r.backend, arg0_value, arg0_type);
		return vs->get_element_count();
	}
	else if(is_vector_hamt(type0)){
		return arg0_value.vector_hamt_ptr->get_element_count();
	}

	else if(is_dict_cppmap(type0)){
		DICT_CPPMAP_T* dict = unpack_dict_cppmap_arg(r.backend, arg0_value, arg0_type);
		return dict->size();
	}
	else if(is_dict_hamt(type0)){
		const auto& dict = *arg0_value.dict_hamt_ptr;
		return dict.size();
	}
	else{
		//	No other types allowed.
		UNSUPPORTED();
	}
}





static const runtime_value_t floyd_llvm_intrinsic__subset(floyd_runtime_t* frp, runtime_value_t arg0_value, runtime_type_t arg0_type, uint64_t start, uint64_t end){
	auto& r = get_floyd_runtime(frp);

	const auto type0 = lookup_type(r.backend, arg0_type);
	if(type0.is_string()){
		return subset__string(r.backend, arg0_value, arg0_type, start, end);
	}
	else if(is_vector_cppvector(type0)){
		return subset__cppvector(r.backend, arg0_value, arg0_type, start, end);
	}
	else if(is_vector_hamt(type0)){
		return subset__hamt(r.backend, arg0_value, arg0_type, start, end);
	}
	else{
		//	No other types allowed.
		UNSUPPORTED();
	}
}





static runtime_value_t floyd_llvm_intrinsic__to_pretty_string(floyd_runtime_t* frp, runtime_value_t arg0_value, runtime_type_t arg0_type){
	auto& r = get_floyd_runtime(frp);

	const auto type0 = lookup_type(r.backend, arg0_type);
	const auto& value = from_runtime_value(r, arg0_value, type0);
	const auto json = value_to_ast_json(value, json_tags::k_plain);
	const auto s = json_to_pretty_string(json, 0, pretty_t{ 80, 4 });
	return to_runtime_string(r, s);
}

static runtime_value_t floyd_llvm_intrinsic__to_string(floyd_runtime_t* frp, runtime_value_t arg0_value, runtime_type_t arg0_type){
	auto& r = get_floyd_runtime(frp);

	const auto s = gen_to_string(r, arg0_value, arg0_type);
	return to_runtime_string(r, s);
}



static runtime_type_t floyd_llvm_intrinsic__typeof(floyd_runtime_t* frp, runtime_value_t arg0_value, runtime_type_t arg0_type){
	auto& r = get_floyd_runtime(frp);

#if DEBUG
	const auto type0 = lookup_type(r.backend, arg0_type);
	QUARK_ASSERT(type0.check_invariant());
#endif
	return arg0_type;
}






//??? Move implementation elsewhere.
static const runtime_value_t floyd_llvm_intrinsic__update(floyd_runtime_t* frp, runtime_value_t arg0_value, runtime_type_t arg0_type, runtime_value_t arg1_value, runtime_type_t arg1_type, runtime_value_t arg2_value, runtime_type_t arg2_type){
	auto& r = get_floyd_runtime(frp);

	const auto type0 = lookup_type(r.backend, arg0_type);
	const auto type1 = lookup_type(r.backend, arg1_type);
	const auto type2 = lookup_type(r.backend, arg2_type);
	if(type0.is_string()){
		return update__string(r.backend, arg0_value, arg0_type, arg1_value, arg1_type, arg2_value, arg2_type);
	}
	else if(is_vector_cppvector(type0)){
		return update__cppvector(r.backend, arg0_value, arg0_type, arg1_value, arg1_type, arg2_value, arg2_type);
	}
	else if(is_vector_hamt(type0)){
		return update__vector_hamt(r.backend, arg0_value, arg0_type, arg1_value, arg1_type, arg2_value, arg2_type);
	}
	else if(is_dict_cppmap(type0)){
		return update__dict_cppmap(r.backend, arg0_value, arg0_type, arg1_value, arg1_type, arg2_value, arg2_type);
	}
	else if(is_dict_hamt(type0)){
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

	const auto type0 = lookup_type(r.backend, arg0_type);
	const auto value0 = from_runtime_value(r, arg0_value, type0);
	const auto j = value_to_ast_json(value0, json_tags::k_plain);
	auto result = alloc_json(r.backend.heap, j);
	return result;
}








/////////////////////////////////////////		REGISTRY




static std::map<std::string, void*> get_intrinsic_c_function_ptrs(){

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


#if DEBUG && 1
//	Verify that all global functions can be accessed. If *one* is unresolved, then all return NULL!?
static void check_nulls(llvm_execution_engine_t& ee2, const llvm_ir_program_t& p){
	int index = 0;
	for(const auto& e: p.debug_globals._symbols){
		if(e.second.get_type().is_function()){
			const auto global_var = (FLOYD_RUNTIME_HOST_FUNCTION*)floyd::get_global_ptr(ee2, e.first);
			QUARK_ASSERT(global_var != nullptr);

			const auto f = *global_var;
//				QUARK_ASSERT(f != nullptr);

			const std::string suffix = f == nullptr ? " NULL POINTER" : "";
//			const uint64_t addr = reinterpret_cast<uint64_t>(f);
//			QUARK_TRACE_SS(index << " " << e.first << " " << addr << suffix);
		}
		else{
		}
		index++;
	}
}
#endif

static std::map<link_name_t, void*> register_c_functions(llvm::LLVMContext& context, const llvm_type_lookup& type_lookup){

	////////	Functions to support the runtime

	const auto runtime_functions = get_runtime_functions(context, type_lookup);
	std::map<link_name_t, void*> runtime_functions_map;
	for(const auto& e: runtime_functions){
		const auto link_name = encode_runtime_func_link_name(e.name);
		const auto e2 = std::pair<link_name_t, void*>(link_name, e.implementation_f);
		runtime_functions_map.insert(e2);
	}

	std::map<link_name_t, void*> function_map = runtime_functions_map;

	////////	intrinsics
	const auto intrinsics0 = get_intrinsic_c_function_ptrs();
	std::map<link_name_t, void*> intrinsics;
	for(const auto& e: intrinsics0){
		intrinsics.insert({ encode_floyd_func_link_name(e.first), e.second });
	}
	function_map.insert(intrinsics.begin(), intrinsics.end());

	////////	Corelib
	const auto corelib_function_map0 = get_corelib_c_function_ptrs();
	std::map<link_name_t, void*> corelib_function_map;
	for(const auto& e: corelib_function_map0){
		corelib_function_map.insert({ encode_floyd_func_link_name(e.first), e.second });
	}
	function_map.insert(corelib_function_map.begin(), corelib_function_map.end());

	return function_map;
}

static int64_t floyd_llvm_intrinsic__dummy(floyd_runtime_t* frp){
	auto& r = get_floyd_runtime(frp);
	(void)r;
	quark::throw_runtime_error("Attempting to calling unimplemented function.");
}





////////////////////////////////		llvm_execution_engine_t



llvm_execution_engine_t::~llvm_execution_engine_t(){
	QUARK_ASSERT(check_invariant());

	if(inited){
		auto f = reinterpret_cast<FLOYD_RUNTIME_INIT>(get_function_ptr(*this, encode_runtime_func_link_name("deinit")));
		QUARK_ASSERT(f != nullptr);

		int64_t result = (*f)(make_runtime_ptr(this));
		QUARK_ASSERT(result == 668);
		inited = false;
	};

//	const auto leaks = heap.count_used();
//	QUARK_ASSERT(leaks == 0);

//	detect_leaks(heap);
}

bool llvm_execution_engine_t::check_invariant() const {
	QUARK_ASSERT(ee);
	QUARK_ASSERT(backend.check_invariant());
	return true;
}



//??? Do this onces and store in llvm_execution_engine_t.
static std::vector<std::pair<link_name_t, void*>> collection_native_func_ptrs(llvm::ExecutionEngine& ee, const std::vector<function_def_t>& function_defs){
	std::vector<std::pair<link_name_t, void*>> result;
	for(const auto& e: function_defs){
		const auto f = (void*)ee.getFunctionAddress(e.link_name.s);
//		auto f = get_function_ptr(runtime, e.link_name);
		result.push_back({ e.link_name, f });
	}

	if(k_trace_messaging){
		QUARK_SCOPED_TRACE("linked functions");
		for(const auto& e: result){
			QUARK_TRACE_SS(e.first.s << " = " << (e.second == nullptr ? "nullptr" : ptr_to_hexstring(e.second)));
		}
	}

	return result;
}
//??? LLVM codegen unlinks functions not called: need to mark functions external.



static std::vector<std::pair<typeid_t, struct_layout_t>> make_struct_layouts(const llvm_type_lookup& type_lookup, const llvm::DataLayout& data_layout){
	QUARK_ASSERT(type_lookup.check_invariant());

	std::vector<std::pair<typeid_t, struct_layout_t>> result;

	for(const auto& e: type_lookup.state.types){
		if(e.type.is_struct()){
			auto t2 = get_exact_struct_type(type_lookup, e.type);
			const llvm::StructLayout* layout = data_layout.getStructLayout(t2);

			const auto struct_bytes = layout->getSizeInBytes();
			std::vector<size_t> member_offsets;
			for(int member_index = 0 ; member_index < e.type.get_struct()._members.size() ; member_index++){
				const auto offset = layout->getElementOffset(member_index);
				member_offsets.push_back(offset);
			}

			result.push_back( { e.type, struct_layout_t{ member_offsets, struct_bytes } } );
		}
	}
	return result;
}

static std::map<runtime_type_t, typeid_t> make_type_lookup(const llvm_type_lookup& type_lookup){
	QUARK_ASSERT(type_lookup.check_invariant());

	std::map<runtime_type_t, typeid_t> result;
	for(const auto& e: type_lookup.state.types){
		result.insert( { make_runtime_type(e.itype.itype), e.type } );
	}
	return result;
}

//	Destroys program, can only run it once!
static std::unique_ptr<llvm_execution_engine_t> make_engine_no_init(llvm_instance_t& instance, llvm_ir_program_t& program_breaks){
	QUARK_ASSERT(instance.check_invariant());
	QUARK_ASSERT(program_breaks.check_invariant());

	std::string collectedErrors;

	//	WARNING: Destroys p -- uses std::move().
	llvm::ExecutionEngine* exeEng = llvm::EngineBuilder(std::move(program_breaks.module))
		.setErrorStr(&collectedErrors)
		.setOptLevel(llvm::CodeGenOpt::Level::None)
		.setVerifyModules(true)
		.setEngineKind(llvm::EngineKind::JIT)
		.create();

	if (exeEng == nullptr){
		std::string error = "Unable to construct execution engine: " + collectedErrors;
		perror(error.c_str());
		throw std::exception();
	}
	QUARK_ASSERT(collectedErrors.empty());

	const auto start_time = std::chrono::high_resolution_clock::now();

	auto ee1 = std::shared_ptr<llvm::ExecutionEngine>(exeEng);

	auto function_map = register_c_functions(instance.context, program_breaks.type_lookup);

	//	Resolve all unresolved functions.
	{
		//	https://stackoverflow.com/questions/33328562/add-mapping-to-c-lambda-from-llvm
		auto lambda = [&](const std::string& s) -> void* {
			QUARK_ASSERT(s.empty() == false);
			//QUARK_ASSERT(s[0] == '_');
#ifndef __APPLE__
			const auto s2 = s; //s.substr(1);
#else
			const auto s2 = s.substr(1);
#endif
			const auto it = function_map.find(link_name_t{ s2 });
			if(it != function_map.end()){
				return it->second;
			}
			else{
				return (void*)&floyd_llvm_intrinsic__dummy;
//				throw std::exception();
			}

			return nullptr;
		};
		std::function<void*(const std::string&)> on_lazy_function_creator2 = lambda;

		//	NOTICE! Patch during finalizeObject() only, then restore!
		ee1->InstallLazyFunctionCreator(on_lazy_function_creator2);
		ee1->finalizeObject();
		ee1->InstallLazyFunctionCreator(nullptr);

	//	ee2.ee->DisableGVCompilation(false);
	//	ee2.ee->DisableSymbolSearching(false);
	}

	auto ee2 = std::unique_ptr<llvm_execution_engine_t>(
		new llvm_execution_engine_t{
			k_debug_magic,
			value_backend_t(
				collection_native_func_ptrs(*ee1, program_breaks.function_defs),
				make_struct_layouts(program_breaks.type_lookup, ee1->getDataLayout()),
				make_type_lookup(program_breaks.type_lookup)
			),
			program_breaks.type_lookup,
			program_breaks.container_def,
			&instance,
			ee1,
			program_breaks.debug_globals,
			program_breaks.function_defs,
			{},
			nullptr,
			start_time,
			llvm_bind_t{ link_name_t {}, nullptr, typeid_t::make_undefined() },
			false
		}
	);
	QUARK_ASSERT(ee2->check_invariant());

#if DEBUG
	check_nulls(*ee2, program_breaks);
#endif

//	llvm::WriteBitcodeToFile(exeEng->getVerifyModules(), raw_ostream &Out);
	return ee2;
}

//	Destroys program, can only run it once!
//	Automatically runs floyd_runtime_init() to execute Floyd's global functions and initialize global constants.
std::unique_ptr<llvm_execution_engine_t> init_program(llvm_ir_program_t& program_breaks){
	QUARK_ASSERT(program_breaks.check_invariant());

	auto ee = make_engine_no_init(*program_breaks.instance, program_breaks);

	trace_heap(ee->backend.heap);

	//	Make sure linking went well - test that by trying to resolve a function we know exists.
#if DEBUG
	{
		{
			const auto print_global_ptr_ptr = (FLOYD_RUNTIME_HOST_FUNCTION*)floyd::get_global_ptr(*ee, "print");
			QUARK_ASSERT(print_global_ptr_ptr != nullptr);
			const auto print_ptr = *print_global_ptr_ptr;
			QUARK_ASSERT(print_ptr != nullptr);
		}
	}
#endif

	ee->main_function = bind_function2(*ee, encode_floyd_func_link_name("main"));

	auto a_func = reinterpret_cast<FLOYD_RUNTIME_INIT>(get_function_ptr(*ee, encode_runtime_func_link_name("init")));
	QUARK_ASSERT(a_func != nullptr);

	int64_t init_result = (*a_func)(make_runtime_ptr(ee.get()));
	QUARK_ASSERT(init_result == 667);


	QUARK_ASSERT(init_result == 667);
	ee->inited = true;

	trace_heap(ee->backend.heap);
//	const auto leaks = ee->heap.count_used();
//	QUARK_ASSERT(leaks == 0);

	return ee;
}




//////////////////////////////////////		llvm_process_runtime_t



/*
	We use only one LLVM execution engine to run main() and all Floyd processes.
	They each have a separate llvm_process_t-instance and their own runtime-pointer.
*/

/*
	Try using C++ multithreading maps etc?
	Explore std::packaged_task
*/
//	https://en.cppreference.com/w/cpp/thread/condition_variable/wait


struct process_interface {
	virtual ~process_interface(){};
	virtual void on_message(const json_t& message) = 0;
	virtual void on_init() = 0;
};


//	NOTICE: Each process inbox has its own mutex + condition variable.
//	No mutex protects cout.
struct llvm_process_t {
	std::condition_variable _inbox_condition_variable;
	std::mutex _inbox_mutex;
	std::deque<json_t> _inbox;

	std::string _name_key;
	std::string _function_key;
	std::thread::id _thread_id;

//	std::shared_ptr<interpreter_t> _interpreter;
	std::shared_ptr<llvm_bind_t> _init_function;
	std::shared_ptr<llvm_bind_t> _process_function;
	value_t _process_state;
	std::shared_ptr<process_interface> _processor;
};

struct llvm_process_runtime_t {
	container_t _container;
	std::map<std::string, std::string> _process_infos;
	std::thread::id _main_thread_id;

	llvm_execution_engine_t* ee;

	std::vector<std::shared_ptr<llvm_process_t>> _processes;
	std::vector<std::thread> _worker_threads;
};

/*
??? have ONE runtime PER computer or one per interpreter?
??? Separate system-interpreter (all processes and many clock busses) vs ONE thread of execution?
*/

static void send_message(llvm_process_runtime_t& runtime, int process_id, const json_t& message){
	auto& process = *runtime._processes[process_id];

    {
        std::lock_guard<std::mutex> lk(process._inbox_mutex);
        process._inbox.push_front(message);
        if(k_trace_messaging){
        	QUARK_TRACE("Notifying...");
		}
    }
    process._inbox_condition_variable.notify_one();
//    process._inbox_condition_variable.notify_all();
}

static void run_process(llvm_process_runtime_t& runtime, int process_id){
	auto& process = *runtime._processes[process_id];
	bool stop = false;

	const auto thread_name = get_current_thread_name();

	const typeid_t process_state_type = process._init_function != nullptr ? process._init_function->type.get_function_return() : typeid_t::make_undefined();

	if(process._processor){
		process._processor->on_init();
	}

	if(process._init_function != nullptr){
		//	!!! This validation should be done earlier in the startup process / compilation process.
		if(process._init_function->type != make_process_init_type(process_state_type)){
			quark::throw_runtime_error("Invalid function prototype for process-init");
		}

		auto f = reinterpret_cast<FLOYD_RUNTIME_PROCESS_INIT>(process._init_function->address);
		const auto result = (*f)(make_runtime_ptr(runtime.ee));
		process._process_state = from_runtime_value(*runtime.ee, result, process._init_function->type.get_function_return());
	}

	while(stop == false){
		json_t message;
		{
			std::unique_lock<std::mutex> lk(process._inbox_mutex);

			if(k_trace_messaging){
        		QUARK_TRACE_SS(thread_name << ": waiting......");
			}
			process._inbox_condition_variable.wait(lk, [&]{ return process._inbox.empty() == false; });
			if(k_trace_messaging){
        		QUARK_TRACE_SS(thread_name << ": continue");
			}

			//	Pop message.
			QUARK_ASSERT(process._inbox.empty() == false);
			message = process._inbox.back();
			process._inbox.pop_back();
		}

		if(k_trace_messaging){
			QUARK_TRACE_SS("RECEIVED: " << json_to_pretty_string(message));
		}

		if(message.is_string() && message.get_string() == "stop"){
			stop = true;
			if(k_trace_messaging){
        		QUARK_TRACE_SS(thread_name << ": STOP");
			}
		}
		else{
			if(process._processor){
				process._processor->on_message(message);
			}

			if(process._process_function != nullptr){
				//	!!! This validation should be done earlier in the startup process / compilation process.
				if(process._process_function->type != make_process_message_handler_type(process_state_type)){
					quark::throw_runtime_error("Invalid function prototype for process message handler");
				}

				auto f = reinterpret_cast<FLOYD_RUNTIME_PROCESS_MESSAGE>(process._process_function->address);
				const auto state2 = to_runtime_value(*runtime.ee, process._process_state);
				const auto message2 = to_runtime_value(*runtime.ee, value_t::make_json(message));
				const auto result = (*f)(make_runtime_ptr(runtime.ee), state2, message2);
				process._process_state = from_runtime_value(*runtime.ee, result, process._process_function->type.get_function_return());
			}
		}
	}
}


static std::map<std::string, value_t> run_processes(llvm_execution_engine_t& ee){
	if(ee.container_def._clock_busses.empty() == true){
		return {};
	}
	else{
		llvm_process_runtime_t runtime;
		runtime._main_thread_id = std::this_thread::get_id();
		runtime.ee = &ee;

		runtime._container = ee.container_def;

		runtime._process_infos = reduce(runtime._container._clock_busses, std::map<std::string, std::string>(), [](const std::map<std::string, std::string>& acc, const std::pair<std::string, clock_bus_t>& e){
			auto acc2 = acc;
			acc2.insert(e.second._processes.begin(), e.second._processes.end());
			return acc2;
		});

		struct my_interpreter_handler_t : public runtime_handler_i {
			my_interpreter_handler_t(llvm_process_runtime_t& runtime) : _runtime(runtime) {}

			virtual void on_send(const std::string& process_id, const json_t& message){
				const auto it = std::find_if(_runtime._processes.begin(), _runtime._processes.end(), [&](const std::shared_ptr<llvm_process_t>& process){ return process->_name_key == process_id; });
				if(it != _runtime._processes.end()){
					const auto process_index = it - _runtime._processes.begin();
					send_message(_runtime, static_cast<int>(process_index), message);
				}
			}

			llvm_process_runtime_t& _runtime;
		};
		auto my_interpreter_handler = my_interpreter_handler_t{runtime};

	//??? We need to pass unique runtime-object to each Floyd process -- not the ee!

		ee._handler = &my_interpreter_handler;

		for(const auto& t: runtime._process_infos){
			auto process = std::make_shared<llvm_process_t>();
			process->_name_key = t.first;
			process->_function_key = t.second;
	//		process->_interpreter = std::make_shared<interpreter_t>(program, &my_interpreter_handler);

			process->_init_function = std::make_shared<llvm_bind_t>(bind_function2(*runtime.ee, encode_floyd_func_link_name(t.second + "__init")));
			process->_process_function = std::make_shared<llvm_bind_t>(bind_function2(*runtime.ee, encode_floyd_func_link_name(t.second)));

			runtime._processes.push_back(process);
		}

		//	Remember that current thread (main) is also a thread, no need to create a worker thread for one process.
		runtime._processes[0]->_thread_id = runtime._main_thread_id;

		for(int process_id = 1 ; process_id < runtime._processes.size() ; process_id++){
			runtime._worker_threads.push_back(std::thread([&](int process_id){

	//			const auto native_thread = thread::native_handle();

				std::stringstream thread_name;
				thread_name << std::string() << "process " << process_id << " thread";
	#ifdef __APPLE__
				pthread_setname_np(/*pthread_self(),*/ thread_name.str().c_str());
	#endif

				run_process(runtime, process_id);
			}, process_id));
		}

		run_process(runtime, 0);

		for(auto &t: runtime._worker_threads){
			t.join();
		}

		return {};
	}
}



run_output_t run_program(llvm_execution_engine_t& ee, const std::vector<std::string>& main_args){
	if(ee.main_function.address != nullptr){
		const auto main_result_int = llvm_call_main(ee, ee.main_function, main_args);
		return { main_result_int, {} };
	}
	else{
		const auto result = run_processes(ee);
		return { 0, result };
	}
}








std::vector<bench_t> collect_benchmarks(const llvm_execution_engine_t& ee){
	std::pair<void*, typeid_t> benchmark_registry_bind = bind_global(ee, k_global_benchmark_registry);
	QUARK_ASSERT(benchmark_registry_bind.first != nullptr);
	QUARK_ASSERT(benchmark_registry_bind.second == typeid_t::make_vector(make_benchmark_def_t()));

	const value_t reg = load_global(ee, benchmark_registry_bind);
	const auto v = reg.get_vector_value();

	std::vector<bench_t> result;
	for(const auto& e: v){
		const auto s = e.get_struct_value();
		const auto name = s->_member_values[0].get_string_value();
		const auto f_link_name_str = s->_member_values[1].get_function_value().name;
		const auto f_link_name = link_name_t{ f_link_name_str };
		result.push_back(bench_t{ benchmark_id_t { "", name }, f_link_name });
	}
	return result;
}

std::vector<benchmark_result2_t> run_benchmarks(llvm_execution_engine_t& ee, const std::vector<bench_t>& tests){
	std::vector<benchmark_result2_t> result;
	for(const auto& b: tests){
		const auto name = b.benchmark_id.test;
		const auto f_link_name = b.f;

		const auto f_bind = bind_function2(ee, f_link_name);
		QUARK_ASSERT(f_bind.address != nullptr);
		auto f2 = reinterpret_cast<FLOYD_BENCHMARK_F>(f_bind.address);
		const auto bench_result = (*f2)(make_runtime_ptr(&ee));
		const auto result2 = from_runtime_value(ee, bench_result, typeid_t::make_vector(make_benchmark_result_t()));

//			QUARK_TRACE(value_and_type_to_string(result2));

		std::vector<benchmark_result2_t> test_result;
		const auto& vec_result = result2.get_vector_value();
		for(const auto& m: vec_result){
			const auto& struct_result = m.get_struct_value();
			const auto result3 = benchmark_result_t {
				struct_result->_member_values[0].get_int_value(),
				struct_result->_member_values[1].get_json()
			};
			const auto x = benchmark_result2_t { b.benchmark_id, result3 };
			test_result.push_back(x);
		}
		result = concat(result, test_result);
	}

	return result;
}


std::vector<bench_t> filter_benchmarks(const std::vector<bench_t>& b, const std::vector<std::string>& run_tests){
	std::vector<bench_t> filtered;
	for(const auto& wanted_test: run_tests){
		const auto it = std::find_if(b.begin(), b.end(), [&] (const bench_t& b2) { return b2.benchmark_id.test == wanted_test; } );
		if(it != b.end()){
			filtered.push_back(*it);
		}
	}

	return filtered;
}

}	//	namespace floyd
