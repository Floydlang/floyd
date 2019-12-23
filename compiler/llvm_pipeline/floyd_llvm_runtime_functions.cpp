//
//  floyd_llvm_runtime_functions.cpp
//  floyd
//
//  Created by Marcus Zetterquist on 2019-08-28.
//  Copyright © 2019 Marcus Zetterquist. All rights reserved.
//

#include "floyd_llvm_runtime_functions.h"

#include "floyd_llvm_helpers.h"
#include "floyd_llvm_types.h"
#include "floyd_llvm_runtime.h"
#include "floyd_llvm_codegen_basics.h"
#include "value_features.h"

#include <llvm/ExecutionEngine/ExecutionEngine.h>


namespace floyd {



static const llvm_function_link_entry_t& resolve_func(const std::vector<llvm_function_link_entry_t>& function_defs, const module_symbol_t& name){
	return find_function_def_from_link_name(function_defs, name);
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
static runtime_value_t floydrt_alloc_kstr(floyd_runtime_t* frp, const char* s, uint64_t size){
	auto& r = get_floyd_runtime(frp);
	auto& backend = r.ee->backend;
	return alloc_carray_8bit(backend, reinterpret_cast<const uint8_t*>(s), size);
}

static std::vector<function_bind_t> floydrt_alloc_kstr__make(llvm::LLVMContext& context, const llvm_type_lookup& type_lookup){
	llvm::FunctionType* function_type = llvm::FunctionType::get(
		make_generic_vec_type_byvalue(type_lookup)->getPointerTo(),
		{
			make_frp_type(type_lookup),
			llvm::Type::getInt8PtrTy(context),
			llvm::Type::getInt64Ty(context)
		},
		false
	);
	return {{ module_symbol_t("alloc_kstr"), function_type, reinterpret_cast<void*>(floydrt_alloc_kstr) }};
}



////////////////////////////////		allocate_vector()



//	Creates a new VEC_T with element_count. All elements are blank. Caller owns the result.
static runtime_value_t floydrt_allocate_vector_carray(floyd_runtime_t* frp, runtime_type_t type, uint64_t element_count){
	auto& r = get_floyd_runtime(frp);
	auto& backend = r.ee->backend;
	return alloc_vector_carray(backend.heap, element_count, element_count, type_t(type));
}

static runtime_value_t floydrt_allocate_vector_hamt(floyd_runtime_t* frp, runtime_type_t type, uint64_t element_count){
	auto& r = get_floyd_runtime(frp);
	auto& backend = r.ee->backend;
	return alloc_vector_hamt(backend.heap, element_count, element_count, type_t(type));
}

static std::vector<function_bind_t> floydrt_allocate_vector__make(llvm::LLVMContext& context, const llvm_type_lookup& type_lookup){
	llvm::FunctionType* function_type = llvm::FunctionType::get(
		make_generic_vec_type_byvalue(type_lookup)->getPointerTo(),
		{
			make_frp_type(type_lookup),
			make_runtime_type_type(type_lookup),
			llvm::Type::getInt64Ty(context)
		},
		false
	);

	return {
		{ module_symbol_t("allocate_vector_carray"), function_type, reinterpret_cast<void*>(floydrt_allocate_vector_carray) },
		{ module_symbol_t("allocate_vector_hamt"), function_type, reinterpret_cast<void*>(floydrt_allocate_vector_hamt) }
	};
}

llvm::Value* generate_allocate_vector(llvm_function_generator_t& gen_acc, const type_t& vector_type, int64_t element_count, vector_backend vector_backend){
	QUARK_ASSERT(gen_acc.check_invariant());
	QUARK_ASSERT(vector_type.check_invariant());
	QUARK_ASSERT(element_count >= 0);

	auto& frp_reg = *gen_acc.get_callers_fcp();
	auto& builder = gen_acc.get_builder();

	const auto vector_itype_reg = generate_itype_constant(gen_acc.gen, vector_type);
	const auto element_count_reg = llvm::ConstantInt::get(llvm::Type::getInt64Ty(builder.getContext()), element_count);

	std::string n;
	if(vector_backend == vector_backend::carray){
		n = "allocate_vector_carray";
	}
	else if(vector_backend == vector_backend::hamt){
		n = "allocate_vector_hamt";
	}
	else{
		QUARK_ASSERT(false);
		throw std::exception();
	}

	const auto res = resolve_func(gen_acc.gen.link_map, module_symbol_t(n));
	return builder.CreateCall(res.llvm_codegen_f, { &frp_reg, vector_itype_reg, element_count_reg }, "");
}











////////////////////////////////		allocate_vector_fill()



static runtime_value_t floydrt_allocate_vector_fill(floyd_runtime_t* frp, runtime_type_t type, runtime_value_t* elements, uint64_t element_count){
	auto& r = get_floyd_runtime(frp);
	auto& backend = r.ee->backend;

	if(is_vector_carray(backend.types, backend.config, type_t(type))){
		return alloc_vector_carray(backend.heap, element_count, element_count, type_t(type));
	}
	else if(is_vector_hamt(backend.types, backend.config, type_t(type))){
		return alloc_vector_hamt(backend.heap, element_count, element_count, type_t(type));
	}
	else{
		QUARK_ASSERT(false);
		throw std::exception();
	}
}

static std::vector<function_bind_t> floydrt_allocate_vector_fill__make(llvm::LLVMContext& context, const llvm_type_lookup& type_lookup){
	llvm::FunctionType* function_type = llvm::FunctionType::get(
		make_generic_vec_type_byvalue(type_lookup)->getPointerTo(),
		{
			make_frp_type(type_lookup),
			make_runtime_type_type(type_lookup),
			make_generic_vec_type_byvalue(type_lookup)->getPointerTo(),
			llvm::Type::getInt64Ty(context)
		},
		false
	);
	return {{ module_symbol_t("allocate_vector_fill"), function_type, reinterpret_cast<void*>(floydrt_allocate_vector_fill) }};
}







////////////////////////////////		store_vector_element_hamt_mutable()


static void floydrt_store_vector_element_hamt_mutable(floyd_runtime_t* frp, runtime_value_t vec, runtime_type_t type, uint64_t index, runtime_value_t element){
	auto& r = get_floyd_runtime(frp);
	auto& backend = r.ee->backend;

	QUARK_ASSERT(is_vector_hamt(backend.types, backend.config, type_t(type)));
	vec.vector_hamt_ptr->store_mutate(index, element);
}

static std::vector<function_bind_t> floydrt_store_vector_element_mutable__make(llvm::LLVMContext& context, const llvm_type_lookup& type_lookup){
	llvm::FunctionType* function_type = llvm::FunctionType::get(
		llvm::Type::getVoidTy(context),
		{
			make_frp_type(type_lookup),
			make_generic_vec_type_byvalue(type_lookup)->getPointerTo(),
			make_runtime_type_type(type_lookup),
			llvm::Type::getInt64Ty(context),
			make_runtime_value_type(type_lookup)
		},
		false
	);
	return {{ module_symbol_t("store_vector_element_hamt_mutable"), function_type, reinterpret_cast<void*>(floydrt_store_vector_element_hamt_mutable) }};
}



////////////////////////////////		floydrt_concatunate_vectors()



static runtime_value_t floydrt_concatunate_vectors(floyd_runtime_t* frp, runtime_type_t type, runtime_value_t lhs, runtime_value_t rhs){
	auto& r = get_floyd_runtime(frp);
	auto& backend = r.ee->backend;
	QUARK_ASSERT(lhs.check_invariant());
	QUARK_ASSERT(rhs.check_invariant());

	return concatunate_vectors(backend, type_t(type), lhs, rhs);
}

static std::vector<function_bind_t> floydrt_concatunate_vectors__make(llvm::LLVMContext& context, const llvm_type_lookup& type_lookup){
	llvm::FunctionType* function_type = llvm::FunctionType::get(
		make_generic_vec_type_byvalue(type_lookup)->getPointerTo(),
		{
			make_frp_type(type_lookup),
			make_runtime_type_type(type_lookup),
			make_generic_vec_type_byvalue(type_lookup)->getPointerTo(),
			make_generic_vec_type_byvalue(type_lookup)->getPointerTo()
		},
		false
	);
	return {{ module_symbol_t("concatunate_vectors"), function_type, reinterpret_cast<void*>(floydrt_concatunate_vectors) }};
}





//	Notice: value_backend never handle RC automatically = no need to make pod/nonpod access to it.
static runtime_value_t floydrt_load_vector_element_hamt(floyd_runtime_t* frp, runtime_value_t vec, runtime_type_t type, uint64_t index){
	auto& r = get_floyd_runtime(frp);
	auto& backend = r.ee->backend;

	QUARK_ASSERT(is_vector_hamt(backend.types, backend.config, type_t(type)));

	const auto size = vec.vector_hamt_ptr->get_element_count();
	if(index >= size){
		std::stringstream what;
		what << "Bounds error. Read HAMT vector index " << index << ", vector size: " << size << ".";
		throw std::out_of_range(what.str());
	}

	return vec.vector_hamt_ptr->load_element(index);
}

static std::vector<function_bind_t> floydrt_load_vector_element__make(llvm::LLVMContext& context, const llvm_type_lookup& type_lookup){
	llvm::FunctionType* function_type = llvm::FunctionType::get(
		make_runtime_value_type(type_lookup),
		{
			make_frp_type(type_lookup),
			make_generic_vec_type_byvalue(type_lookup)->getPointerTo(),
			make_runtime_type_type(type_lookup),
			llvm::Type::getInt64Ty(context)
		},
		false
	);
	return {{ module_symbol_t("load_vector_element_hamt"), function_type, reinterpret_cast<void*>(floydrt_load_vector_element_hamt) }};
}






//	DICT
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////






////////////////////////////////		allocate_dict()



//??? split into several functions
static const runtime_value_t floydrt_allocate_dict(floyd_runtime_t* frp, runtime_type_t type){
	auto& r = get_floyd_runtime(frp);
	auto& backend = r.ee->backend;

	if(is_dict_cppmap(backend.types, backend.config, type_t(type))){
		return alloc_dict_cppmap(backend.heap, type_t(type));
	}
	else if(is_dict_hamt(backend.types, backend.config, type_t(type))){
		return alloc_dict_hamt(backend.heap, type_t(type));
	}
	else{
		QUARK_ASSERT(false);
		throw std::exception();
	}
}

static std::vector<function_bind_t> floydrt_allocate_dict__make(llvm::LLVMContext& context, const llvm_type_lookup& type_lookup){
	llvm::FunctionType* function_type = llvm::FunctionType::get(
		make_generic_dict_type_byvalue(type_lookup)->getPointerTo(),
		{
			make_frp_type(type_lookup),
			make_runtime_type_type(type_lookup),
		},
		false
	);
	return {{ module_symbol_t("allocate_dict"), function_type, reinterpret_cast<void*>(floydrt_allocate_dict) }};
}




////////////////////////////////		lookup_dict()



static runtime_value_t floydrt_lookup_dict_cppmap(floyd_runtime_t* frp, runtime_value_t dict, runtime_type_t type, runtime_value_t key){
	auto& r = get_floyd_runtime(frp);
	auto& backend = r.ee->backend;

	return lookup_dict_cppmap(backend, dict, type_t(type), key);
}

//??? make faster key without creating std::string.
static runtime_value_t floydrt_lookup_dict_hamt(floyd_runtime_t* frp, runtime_value_t dict, runtime_type_t type, runtime_value_t key){
	auto& r = get_floyd_runtime(frp);
	auto& backend = r.ee->backend;

	return lookup_dict_hamt(backend, dict, type_t(type), key);
}

static std::vector<function_bind_t> floydrt_lookup_dict_cppmap__make(llvm::LLVMContext& context, const llvm_type_lookup& type_lookup){
	llvm::FunctionType* function_type = llvm::FunctionType::get(
		make_runtime_value_type(type_lookup),
		{
			make_frp_type(type_lookup),
			make_generic_dict_type_byvalue(type_lookup)->getPointerTo(),
			make_runtime_type_type(type_lookup),
			get_llvm_type_as_arg(type_lookup, type_t::make_string())
		},
		false
	);
	return {{ module_symbol_t("lookup_dict_cppmap"), function_type, reinterpret_cast<void*>(floydrt_lookup_dict_cppmap) }};
}
static std::vector<function_bind_t> floydrt_lookup_dict_hamt__make(llvm::LLVMContext& context, const llvm_type_lookup& type_lookup){
	llvm::FunctionType* function_type = llvm::FunctionType::get(
		make_runtime_value_type(type_lookup),
		{
			make_frp_type(type_lookup),
			make_generic_dict_type_byvalue(type_lookup)->getPointerTo(),
			make_runtime_type_type(type_lookup),
			get_llvm_type_as_arg(type_lookup, type_t::make_string())
		},
		false
	);
	return {{ module_symbol_t("lookup_dict_hamt"), function_type, reinterpret_cast<void*>(floydrt_lookup_dict_hamt) }};
}

//??? Rewrite all code that generates calls to use a floyd-function-type and use a shared LLVM-generator for runtime functions & other llvm calls.
llvm::Value* generate_lookup_dict(llvm_function_generator_t& gen_acc, llvm::Value& dict_reg, const type_t& dict_type, llvm::Value& key_reg, dict_backend dict_mode){
	QUARK_ASSERT(gen_acc.check_invariant());
	QUARK_ASSERT(dict_type.check_invariant());

	const auto& types = gen_acc.gen.type_lookup.state.types;
	QUARK_ASSERT(is_dict_cppmap(types, gen_acc.gen.settings.config, dict_type) || is_dict_hamt(types, gen_acc.gen.settings.config, dict_type));

	const auto res = resolve_func(gen_acc.gen.link_map, dict_mode == dict_backend::hamt ? module_symbol_t("lookup_dict_hamt") : module_symbol_t("lookup_dict_cppmap"));

	const auto dict_peek = peek2(types, dict_type);
	const auto element_type0 = dict_peek.get_dict_value_type(types);
	const auto dict_itype_reg = generate_itype_constant(gen_acc.gen, dict_type);
	auto& builder = gen_acc.get_builder();

	std::vector<llvm::Value*> args2 = {
		gen_acc.get_callers_fcp(),
		&dict_reg,
		dict_itype_reg,
		&key_reg
	};

	auto element_value_uint64_reg = builder.CreateCall(res.llvm_codegen_f, args2, "");
	auto result_reg = generate_cast_from_runtime_value(gen_acc.gen, *element_value_uint64_reg, element_type0);
	return result_reg;
}



////////////////////////////////		store_dict_mutable()



static void floydrt_store_dict_mutable_cppmap(floyd_runtime_t* frp, runtime_value_t dict, runtime_type_t type, runtime_value_t key, runtime_value_t element_value){
	auto& r = get_floyd_runtime(frp);
	auto& backend = r.ee->backend;

	const auto& types = backend.types;
	QUARK_ASSERT(is_dict_cppmap(types, backend.config, type_t(type)));
	const auto key_string = from_runtime_string(r, key);
	dict.dict_cppmap_ptr->get_map_mut().insert_or_assign(key_string, element_value);
}
static void floydrt_store_dict_mutable_hamt(floyd_runtime_t* frp, runtime_value_t dict, runtime_type_t type, runtime_value_t key, runtime_value_t element_value){
	auto& r = get_floyd_runtime(frp);
	auto& backend = r.ee->backend;

	const auto& types = backend.types;
	const auto peek = peek2(types, type_t(type));
	QUARK_ASSERT(is_dict_hamt(types, backend.config, peek));
	const auto key_string = from_runtime_string(r, key);
	dict.dict_hamt_ptr->get_map_mut() = dict.dict_hamt_ptr->get_map_mut().set(key_string, element_value);
}

static std::vector<function_bind_t> floydrt_store_dict_mutable__make(llvm::LLVMContext& context, const llvm_type_lookup& type_lookup){
	llvm::FunctionType* function_type = llvm::FunctionType::get(
		llvm::Type::getVoidTy(context),
		{
			make_frp_type(type_lookup),
			make_generic_dict_type_byvalue(type_lookup)->getPointerTo(),
			make_runtime_type_type(type_lookup),
			get_llvm_type_as_arg(type_lookup, type_t::make_string()),
			make_runtime_value_type(type_lookup),
		},
		false
	);
	return {
		{ module_symbol_t("store_dict_mutable_cppmap"), function_type, reinterpret_cast<void*>(floydrt_store_dict_mutable_cppmap) },
		{ module_symbol_t("store_dict_mutable_hamt"), function_type, reinterpret_cast<void*>(floydrt_store_dict_mutable_hamt) }
	};
}

void generate_store_dict_mutable(llvm_function_generator_t& gen_acc, llvm::Value& dict_reg, const type_t& dict_type, llvm::Value& key_reg, llvm::Value& value_reg, dict_backend dict_mode){
	QUARK_ASSERT(gen_acc.check_invariant());
	QUARK_ASSERT(dict_type.check_invariant());

	const auto& types = gen_acc.gen.type_lookup.state.types;
	QUARK_ASSERT(is_dict_cppmap(types, gen_acc.gen.settings.config, dict_type) || is_dict_hamt(types, gen_acc.gen.settings.config, dict_type));

	const auto res = resolve_func(gen_acc.gen.link_map, dict_mode == dict_backend::hamt ? module_symbol_t("store_dict_mutable_hamt") : module_symbol_t("store_dict_mutable_cppmap"));
	const auto dict_peek = peek2(types, dict_type);

	const auto& element_type0 = dict_peek.get_dict_value_type(types);
	auto& dict_itype_reg = *generate_itype_constant(gen_acc.gen, dict_type);
	auto& builder = gen_acc.get_builder();

	std::vector<llvm::Value*> args2 = {
		gen_acc.get_callers_fcp(),
		&dict_reg,
		&dict_itype_reg,
		&key_reg,
		generate_cast_to_runtime_value(gen_acc.gen, value_reg, element_type0)
	};
	builder.CreateCall(res.llvm_codegen_f, args2, "");
}













//	JSON
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////




////////////////////////////////		allocate_json()

//	NOTICE: There is a special backdoor here that lets us create json nulls: call with type = json, json_ptr = nullptr. k_init_local
//??? Use better storage of JSON!?
static runtime_value_t floydrt_allocate_json(floyd_runtime_t* frp, runtime_value_t arg0_value, runtime_type_t arg0_type){
	auto& r = get_floyd_runtime(frp);
	auto& backend = r.ee->backend;

	const auto& type0 = lookup_type_ref(backend, arg0_type);

#if 1
	const auto value = from_runtime_value(r, arg0_value, type0);
	const auto a = value_to_json(backend.types, value);
	return alloc_json(backend.heap, a);
#else
	if(arg0_value.json_ptr == nullptr){
		return alloc_json(backend.heap, json_t());
	}
	else{
		const auto value = from_runtime_value(r, arg0_value, type0);
		const auto a = value_to_json(backend.types, value);
		return alloc_json(backend.heap, a);
	}
#endif
}

static std::vector<function_bind_t> floydrt_allocate_json__make(llvm::LLVMContext& context, const llvm_type_lookup& type_lookup){
	llvm::FunctionType* function_type = llvm::FunctionType::get(
		get_llvm_type_as_arg(type_lookup, type_t::make_json()),
		{
			make_frp_type(type_lookup),
			make_runtime_value_type(type_lookup),
			make_runtime_type_type(type_lookup)
		},
		false
	);
	return {{ module_symbol_t("allocate_json"), function_type, reinterpret_cast<void*>(floydrt_allocate_json) }};
}





//??? move more non-LLVM specific logic to value_features.cpp

////////////////////////////////		lookup_json()


//??? optimize
static runtime_value_t floydrt_lookup_json(floyd_runtime_t* frp, runtime_value_t json0, runtime_value_t arg0_value, runtime_type_t arg0_type){
	auto& r = get_floyd_runtime(frp);
	auto& backend = r.ee->backend;

	const auto& json = json0.json_ptr->get_json();
	const auto& type0 = lookup_type_ref(backend, arg0_type);
	const auto type0_peek = peek2(backend.types, type0);
	const auto value = from_runtime_value(r, arg0_value, type0);

	if(json.is_object()){
		if(type0_peek.is_string() == false){
			quark::throw_runtime_error("Attempting to lookup a json-object with a key that is not a string.");
		}

		const auto result = json.get_object_element(value.get_string_value());
		return alloc_json(backend.heap, result);
	}
	else if(json.is_array()){
		if(type0_peek.is_int() == false){
			quark::throw_runtime_error("Attempting to lookup a json-object with a key that is not a number.");
		}

		const auto result = json.get_array_n(value.get_int_value());
		auto result2 = alloc_json(backend.heap, result);
		return result2;
	}
	else{
		quark::throw_runtime_error("Attempting to lookup a json value -- lookup requires json-array or json-object.");
	}
}

static std::vector<function_bind_t> floydrt_lookup_json__make(llvm::LLVMContext& context, const llvm_type_lookup& type_lookup){
	llvm::FunctionType* function_type = llvm::FunctionType::get(
		get_llvm_type_as_arg(type_lookup, type_t::make_json()),
		{
			make_frp_type(type_lookup),
			get_llvm_type_as_arg(type_lookup, type_t::make_json()),
			make_runtime_value_type(type_lookup),
			make_runtime_type_type(type_lookup)
		},
		false
	);
	return {{ module_symbol_t("lookup_json"), function_type, reinterpret_cast<void*>(floydrt_lookup_json) }};
}



////////////////////////////////		json_to_string()



static runtime_value_t floydrt_json_to_string(floyd_runtime_t* frp, runtime_value_t json0){
	auto& r = get_floyd_runtime(frp);

	const auto& json = json0.json_ptr->get_json();

	if(json.is_string()){
		return to_runtime_string(r, json.get_string());
	}
	else{
		quark::throw_runtime_error("Attempting to assign a non-string JSON to a string.");
	}
}

static std::vector<function_bind_t> floydrt_json_to_string__make(llvm::LLVMContext& context, const llvm_type_lookup& type_lookup){
	llvm::FunctionType* function_type = llvm::FunctionType::get(
		get_llvm_type_as_arg(type_lookup, type_t::make_string()),
		{
			make_frp_type(type_lookup),
			get_llvm_type_as_arg(type_lookup, type_t::make_json())
		},
		false
	);
	return {{ module_symbol_t("json_to_string"), function_type, reinterpret_cast<void*>(floydrt_json_to_string) }};
}








//	STRUCT
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////



////////////////////////////////		allocate_struct()



//	Creates a new VEC_T with element_count. All elements are blank. Caller owns the result.
static runtime_value_t floydrt_allocate_struct(floyd_runtime_t* frp, const runtime_type_t type, uint64_t size){
	auto& r = get_floyd_runtime(frp);
	auto& backend = r.ee->backend;

#if DEBUG
	const auto& type0 = lookup_type_ref(backend, type);
	QUARK_ASSERT(type0.check_invariant());
#endif
	return alloc_struct(backend.heap, size, type_t(type));
}

static std::vector<function_bind_t> floydrt_allocate_struct__make(llvm::LLVMContext& context, const llvm_type_lookup& type_lookup){
	llvm::FunctionType* function_type = llvm::FunctionType::get(
		get_generic_struct_type_byvalue(type_lookup)->getPointerTo(),
		{
			make_frp_type(type_lookup),
			make_runtime_type_type(type_lookup),
			llvm::Type::getInt64Ty(context)
		},
		false
	);
	return {{ module_symbol_t("allocate_struct"), function_type, reinterpret_cast<void*>(floydrt_allocate_struct) }};
}


////////////////////////////////		generate_load_struct_member()

llvm::Value* generate_load_struct_member(llvm_function_generator_t& gen_acc, llvm::Value& struct_ptr_reg, const type_t& struct_type, int member_index){
	QUARK_ASSERT(gen_acc.check_invariant());
	QUARK_ASSERT(struct_type.check_invariant());

	const auto& types = gen_acc.gen.type_lookup.state.types;

	QUARK_ASSERT(peek2(types, struct_type).is_struct());


	QUARK_ASSERT(member_index >= 0 && member_index < peek2(types, struct_type).get_struct(types)._members.size());

	auto& builder = gen_acc.get_builder();
	auto& struct_type_llvm = *get_exact_struct_type_byvalue(gen_acc.gen.type_lookup, struct_type);

	auto base_ptr_reg = generate_get_struct_base_ptr(gen_acc, struct_ptr_reg, struct_type);

	const auto gep = std::vector<llvm::Value*>{
		//	Struct array index.
		builder.getInt32(0),

		//	Struct member index.
		builder.getInt32(member_index)
	};
	llvm::Value* member_ptr_reg = builder.CreateGEP(&struct_type_llvm, base_ptr_reg, gep, "");
	auto member_value_reg = builder.CreateLoad(member_ptr_reg);

	return member_value_reg;
}






////////////////////////////////		floydrt_update_struct_member()



//??? optimize for speed. Most things can be precalculated.
//??? struct_type find_struct_layout() is SLOW right now.
//??? Generate an add_ref-function for each struct type.
//??? Inline store of new member -- same as generate_resolve_member_expression().

//	Make copy of struct, overwrite member in copy.
static const runtime_value_t floydrt_update_struct_member_nonpod(floyd_runtime_t* frp, runtime_value_t s0, runtime_type_t struct_type, int64_t member_index, runtime_value_t new_value, runtime_type_t new_value_type){
	auto& r = get_floyd_runtime(frp);
	auto& backend = r.ee->backend;
	QUARK_ASSERT(s0.struct_ptr != nullptr);
	QUARK_ASSERT(member_index != -1);

	const std::pair<type_t, struct_layout_t>& struct_layout_info = find_struct_layout(backend, type_t(struct_type));

	const auto struct_bytes = struct_layout_info.second.size;
	auto struct_ptr = alloc_struct_copy(backend.heap, reinterpret_cast<const uint64_t*>(s0.struct_ptr->get_data_ptr()), struct_bytes, peek2(backend.types, type_t(struct_type)));
	auto struct_base_ptr = struct_ptr.struct_ptr->get_data_ptr();

	const auto member_offset = struct_layout_info.second.members[member_index].offset;
	const auto member_ptr0 = reinterpret_cast<runtime_value_t*>(struct_base_ptr + member_offset);

	*member_ptr0 = new_value;

	//	Retain every member of new struct.
	for(const auto& e: struct_layout_info.second.members){
		const auto member_itype = e.type;
		if(is_rc_value(backend.types, member_itype)){
			const auto offset = e.offset;
			const auto member_ptr = reinterpret_cast<const runtime_value_t*>(struct_base_ptr + offset);
			retain_value(backend, *member_ptr, member_itype);
		}
	}

	return struct_ptr;
}

static const runtime_value_t floydrt_copy_struct(floyd_runtime_t* frp, runtime_value_t s0, uint64_t struct_size, runtime_type_t struct_type){
	auto& r = get_floyd_runtime(frp);
	auto& backend = r.ee->backend;
	QUARK_ASSERT(s0.struct_ptr != nullptr);
	QUARK_ASSERT(struct_size > 0);
#if DEBUG
	const std::pair<type_t, struct_layout_t>& struct_layout_info = find_struct_layout(backend, type_t(struct_type));
	const auto struct_size2 = struct_layout_info.second.size;
	QUARK_ASSERT(struct_size == struct_size2);
#endif
	return alloc_struct_copy(backend.heap, reinterpret_cast<const uint64_t*>(s0.struct_ptr->get_data_ptr()), struct_size, peek2(backend.types, type_t(struct_type)));
}

static std::vector<function_bind_t> floydrt_update_struct_member__make(llvm::LLVMContext& context, const llvm_type_lookup& type_lookup){
	return {
		{
			module_symbol_t("update_struct_member_nonpod"),
			llvm::FunctionType::get(
				get_generic_struct_type_byvalue(type_lookup)->getPointerTo(),
				{
					make_frp_type(type_lookup),
					get_generic_struct_type_byvalue(type_lookup)->getPointerTo(),
					make_runtime_type_type(type_lookup),
					llvm::Type::getInt64Ty(context),
					make_runtime_value_type(type_lookup),
					make_runtime_type_type(type_lookup)
				},
				false
			),
			reinterpret_cast<void*>(floydrt_update_struct_member_nonpod)
		},
		{
			module_symbol_t("copy_struct"),
			llvm::FunctionType::get(
				get_generic_struct_type_byvalue(type_lookup)->getPointerTo(),
				{
					make_frp_type(type_lookup),
					get_generic_struct_type_byvalue(type_lookup)->getPointerTo(),
					llvm::Type::getInt64Ty(context),
					make_runtime_type_type(type_lookup)
				},
				false
			),
			reinterpret_cast<void*>(floydrt_copy_struct)
		}
	};
}

static void generate_store_struct_member_mutate(llvm_function_generator_t& gen_acc, llvm::Value& struct_ptr_reg, const type_t& struct_type, int member_index, llvm::Value& value_reg){
	QUARK_ASSERT(gen_acc.check_invariant());
	QUARK_ASSERT(struct_type.check_invariant());

	const auto& types = gen_acc.gen.type_lookup.state.types;
	QUARK_ASSERT(member_index >= 0 && member_index < peek2(types, struct_type).get_struct(types)._members.size());

	auto& builder = gen_acc.get_builder();

//	const auto& struct_def = peek2(types, struct_type).get_struct(types);
//	const auto member_type = struct_def._members[member_index]._type;

	auto& struct_type_llvm = *get_exact_struct_type_byvalue(gen_acc.gen.type_lookup, struct_type);
	auto base_ptr_reg = generate_get_struct_base_ptr(gen_acc, struct_ptr_reg, struct_type);

	const auto gep = std::vector<llvm::Value*>{
		//	Struct array index.
		builder.getInt32(0),

		//	Struct member index.
		builder.getInt32(member_index)
	};
	llvm::Value* member_ptr_reg = builder.CreateGEP(&struct_type_llvm, base_ptr_reg, gep, "");
	builder.CreateStore(&value_reg, member_ptr_reg);
}

llvm::Value* generate_update_struct_member(llvm_function_generator_t& gen_acc, llvm::Value& struct_ptr_reg, const type_t& struct_type, int member_index, llvm::Value& value_reg){
	QUARK_ASSERT(gen_acc.check_invariant());
	QUARK_ASSERT(struct_type.check_invariant());

	const auto& types = gen_acc.gen.type_lookup.state.types;
	QUARK_ASSERT(member_index >= 0 && member_index < peek2(types, struct_type).get_struct(types)._members.size());

	auto& builder = gen_acc.get_builder();

	const auto& struct_def = peek2(types, struct_type).get_struct(types);

	auto member_index_reg = llvm::ConstantInt::get(builder.getInt64Ty(), member_index);
	const auto member_type = struct_def._members[member_index]._type;

	const bool pod = is_struct_pod(types, struct_def);

	if(pod){
		const auto copy_struct_f = resolve_func(gen_acc.gen.link_map, module_symbol_t("copy_struct"));

		auto& exact_struct_type = *get_exact_struct_type_byvalue(gen_acc.gen.type_lookup, struct_type);

		const llvm::DataLayout& data_layout = gen_acc.gen.module->getDataLayout();
		const llvm::StructLayout* layout = data_layout.getStructLayout(&exact_struct_type);
		const auto struct_bytes = layout->getSizeInBytes();

		std::vector<llvm::Value*> args = {
			gen_acc.get_callers_fcp(),

			&struct_ptr_reg,
			llvm::ConstantInt::get(builder.getInt64Ty(), struct_bytes),
			generate_itype_constant(gen_acc.gen, struct_type),
		};
		auto copy_reg = builder.CreateCall(copy_struct_f.llvm_codegen_f, args, "");
		generate_store_struct_member_mutate(gen_acc, *copy_reg, struct_type, member_index, value_reg);
		return copy_reg;
	}
	else{
		const auto res = resolve_func(gen_acc.gen.link_map, module_symbol_t("update_struct_member_nonpod"));

		std::vector<llvm::Value*> args = {
			gen_acc.get_callers_fcp(),

			&struct_ptr_reg,
			generate_itype_constant(gen_acc.gen, struct_type),

			member_index_reg,
			generate_cast_to_runtime_value(gen_acc.gen, value_reg, member_type),
			generate_itype_constant(gen_acc.gen, member_type)
		};
		auto copy_reg = builder.CreateCall(res.llvm_codegen_f, args, "");

		return copy_reg;
	}
}








//	OTHER
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////



////////////////////////////////		compare_values()



//??? optimize. It is not used for simple types right now.
static int8_t floydrt_compare_values(floyd_runtime_t* frp, int64_t op, const runtime_type_t type, runtime_value_t lhs, runtime_value_t rhs){
	auto& r = get_floyd_runtime(frp);
	auto& backend = r.ee->backend;
	return (int8_t)compare_values(backend, op, type, lhs, rhs);
}

static std::vector<function_bind_t> floydrt_compare_values__make(llvm::LLVMContext& context, const llvm_type_lookup& type_lookup){
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
	return {{ module_symbol_t("compare_values"), function_type, reinterpret_cast<void*>(floydrt_compare_values) }};
}



static int64_t floydrt_get_profile_time(floyd_runtime_t* frp){
	get_floyd_runtime(frp);

	const std::chrono::time_point<std::chrono::high_resolution_clock> now = std::chrono::high_resolution_clock::now();
	const auto ns = std::chrono::duration_cast<std::chrono::nanoseconds>(now.time_since_epoch()).count();
	const auto result = int64_t(ns);
	return result;
}

static std::vector<function_bind_t> floydrt_get_profile_time__make(llvm::LLVMContext& context, const llvm_type_lookup& type_lookup){
	llvm::FunctionType* function_type = llvm::FunctionType::get(
		llvm::Type::getInt64Ty(context),
		{
			make_frp_type(type_lookup),
		},
		false
	);
	return {{ module_symbol_t("get_profile_time"), function_type, reinterpret_cast<void*>(floydrt_get_profile_time) }};
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
static std::vector<function_bind_t> floydrt_analyse_benchmark_samples__make(llvm::LLVMContext& context, const llvm_type_lookup& type_lookup){
	llvm::FunctionType* function_type = llvm::FunctionType::get(
		llvm::Type::getInt64Ty(context),
		{
			make_frp_type(type_lookup),
			llvm::Type::getInt64Ty(context)->getPointerTo(),
			llvm::Type::getInt64Ty(context)
		},
		false
	);
	return {{ module_symbol_t("analyse_benchmark_samples"), function_type, reinterpret_cast<void*>(floydrt_analyse_benchmark_samples) }};
}
















////////////////////////////////		RETAIN



static void floydrt_retain_vector_carray(floyd_runtime_t* frp, runtime_value_t vec, runtime_type_t type0){
	auto& r = get_floyd_runtime(frp);
	auto& backend = r.ee->backend;
#if DEBUG
	const auto& type = lookup_type_ref(backend, type0);
	QUARK_ASSERT(peek2(backend.types, type).is_string() || peek2(backend.types, type).is_vector());
	QUARK_ASSERT(is_rc_value(backend.types, type_t(type0)));
#endif

	retain_vector_carray(backend, vec, type_t(type0));
}

static void floydrt_retain_vector_hamt(floyd_runtime_t* frp, runtime_value_t vec, runtime_type_t type0){
	auto& r = get_floyd_runtime(frp);
	auto& backend = r.ee->backend;
#if DEBUG
	const auto& type = lookup_type_ref(backend, type0);
	QUARK_ASSERT(peek2(backend.types, type).is_string() || peek2(backend.types, type).is_vector());
	QUARK_ASSERT(is_rc_value(backend.types, type_t(type0)));
#endif

	retain_vector_hamt(backend, vec, type_t(type0));
}



static void floydrt_retain_dict_cppmap(floyd_runtime_t* frp, runtime_value_t dict, runtime_type_t type0){
	auto& r = get_floyd_runtime(frp);
	auto& backend = r.ee->backend;
	const auto& type = lookup_type_ref(backend, type0);
#if DEBUG
	QUARK_ASSERT(is_rc_value(backend.types, type));
	QUARK_ASSERT(peek2(backend.types, type).is_dict());
	QUARK_ASSERT(is_dict_cppmap(backend.types, backend.config, type));
#endif

	retain_dict_cppmap(backend, dict, type);
}
static void floydrt_retain_dict_hamt(floyd_runtime_t* frp, runtime_value_t dict, runtime_type_t type0){
	auto& r = get_floyd_runtime(frp);
	auto& backend = r.ee->backend;
#if DEBUG
	const auto& type = lookup_type_ref(backend, type0);
	QUARK_ASSERT(is_rc_value(backend.types, type));
	QUARK_ASSERT(peek2(backend.types, type).is_dict());
	QUARK_ASSERT(is_dict_hamt(backend.types, backend.config, type));
#endif

	retain_dict_hamt(backend, dict, type_t(type0));
}



static void floydrt_retain_json(floyd_runtime_t* frp, runtime_value_t json, runtime_type_t type0){
	auto& r = get_floyd_runtime(frp);
	auto& backend = r.ee->backend;

	const auto& type = lookup_type_ref(backend, type0);
	QUARK_ASSERT(is_rc_value(backend.types, type));

	//	NOTICE: Floyd runtime() init will destruct globals, including json::null.
	if(json.json_ptr == nullptr){
	}
	else{
		QUARK_ASSERT(peek2(backend.types, type).is_json());
		retain_json(backend, json);
	}
}


static void floydrt_retain_struct(floyd_runtime_t* frp, runtime_value_t v0, runtime_type_t type0){
	auto& r = get_floyd_runtime(frp);
	auto& backend = r.ee->backend;
	QUARK_ASSERT(v0.struct_ptr != nullptr);

const auto& type = lookup_type_ref(backend, type0);
#if DEBUG
	QUARK_ASSERT(is_rc_value(backend.types, type));
	QUARK_ASSERT(peek2(backend.types, type).is_struct());
#endif

	retain_struct(backend, make_runtime_struct(v0.struct_ptr), type);
}



void generate_retain(llvm_function_generator_t& gen_acc, llvm::Value& value_reg, const type_t& type0){
	QUARK_ASSERT(gen_acc.gen.type_lookup.check_invariant());
	QUARK_ASSERT(type0.check_invariant());

	const auto& types = gen_acc.gen.type_lookup.state.types;
	const auto type_peek = peek2(types, type0);

	auto& frp_reg = *gen_acc.get_callers_fcp();
	auto& itype_reg = *generate_itype_constant(gen_acc.gen, type_peek);
	auto& builder = gen_acc.get_builder();

	if(is_rc_value(types, type0)){
		if(type_peek.is_string()){
			const auto res = resolve_func(gen_acc.gen.link_map, module_symbol_t("retain_vector_carray"));
			builder.CreateCall(res.llvm_codegen_f, { &frp_reg, &value_reg, &itype_reg }, "");
		}
		else if(type_peek.is_vector()){
			if(is_vector_carray(types, gen_acc.gen.settings.config, type0)){
				const auto res = resolve_func(gen_acc.gen.link_map, module_symbol_t("retain_vector_carray"));
				builder.CreateCall(res.llvm_codegen_f, { &frp_reg, &value_reg, &itype_reg }, "");
			}
			else if(is_vector_hamt(types, gen_acc.gen.settings.config, type0)){
				const auto res = resolve_func(gen_acc.gen.link_map, module_symbol_t("retain_vector_hamt"));
				builder.CreateCall(res.llvm_codegen_f, { &frp_reg, &value_reg, &itype_reg }, "");
			}
			else{
				QUARK_ASSERT(false);
			}
		}
		else if(type_peek.is_dict()){
			if(is_dict_cppmap(types, gen_acc.gen.settings.config, type0)){
				const auto res = resolve_func(gen_acc.gen.link_map, module_symbol_t("retain_dict_cppmap"));
				builder.CreateCall(res.llvm_codegen_f, { &frp_reg, &value_reg, &itype_reg }, "");
			}
			else if(is_dict_hamt(types, gen_acc.gen.settings.config, type0)){
				const auto res = resolve_func(gen_acc.gen.link_map, module_symbol_t("retain_dict_hamt"));
				builder.CreateCall(res.llvm_codegen_f, { &frp_reg, &value_reg, &itype_reg }, "");
			}
			else{
				QUARK_ASSERT(false);
			}
		}
		else if(type_peek.is_json()){
			const auto res = resolve_func(gen_acc.gen.link_map, module_symbol_t("retain_json"));
			builder.CreateCall(res.llvm_codegen_f, { &frp_reg, &value_reg, &itype_reg }, "");
		}
		else if(type_peek.is_struct()){
			auto generic_vec_reg = builder.CreateCast(llvm::Instruction::CastOps::BitCast, &value_reg, get_generic_struct_type_byvalue(gen_acc.gen.type_lookup)->getPointerTo(), "");
			const auto res = resolve_func(gen_acc.gen.link_map, module_symbol_t("retain_struct"));
			builder.CreateCall(res.llvm_codegen_f, { &frp_reg, generic_vec_reg, &itype_reg }, "");
		}
		else{
			QUARK_ASSERT(false);
		}
	}
	else{
	}
}

static llvm::FunctionType* make_retain(llvm::LLVMContext& context, const llvm_type_lookup& type_lookup, llvm::Type& collection_type){
	return llvm::FunctionType::get(
		llvm::Type::getVoidTy(context),
		{
			make_frp_type(type_lookup),
			&collection_type,
			make_runtime_type_type(type_lookup)
		},
		false
	);
}

static std::vector<function_bind_t> retain_funcs(llvm::LLVMContext& context, const llvm_type_lookup& type_lookup){
	return std::vector<function_bind_t> {
		function_bind_t{ module_symbol_t("retain_vector_carray"), make_retain(context, type_lookup, *make_generic_vec_type_byvalue(type_lookup)->getPointerTo()), reinterpret_cast<void*>(floydrt_retain_vector_carray) },
		function_bind_t{ module_symbol_t("retain_vector_hamt"), make_retain(context, type_lookup, *make_generic_vec_type_byvalue(type_lookup)->getPointerTo()), reinterpret_cast<void*>(floydrt_retain_vector_hamt) },
		function_bind_t{ module_symbol_t("retain_dict_cppmap"), make_retain(context, type_lookup, *make_generic_dict_type_byvalue(type_lookup)->getPointerTo()), reinterpret_cast<void*>(floydrt_retain_dict_cppmap) },
		function_bind_t{ module_symbol_t("retain_dict_hamt"), make_retain(context, type_lookup, *make_generic_dict_type_byvalue(type_lookup)->getPointerTo()), reinterpret_cast<void*>(floydrt_retain_dict_hamt) },
		function_bind_t{ module_symbol_t("retain_json"), make_retain(context, type_lookup, *get_llvm_type_as_arg(type_lookup, type_t::make_json())), reinterpret_cast<void*>(floydrt_retain_json) },
		function_bind_t{ module_symbol_t("retain_struct"), make_retain(context, type_lookup, *get_generic_struct_type_byvalue(type_lookup)->getPointerTo()), reinterpret_cast<void*>(floydrt_retain_struct) }
	};
}






////////////////////////////////		RELEASE




static void floydrt_release_vector_carray_pod(floyd_runtime_t* frp, runtime_value_t vec, runtime_type_t type0){
	auto& r = get_floyd_runtime(frp);
	auto& backend = r.ee->backend;
	const auto& type = lookup_type_ref(backend, type0);
#if DEBUG
	QUARK_ASSERT(peek2(backend.types, type).is_string() || is_vector_carray(backend.types, backend.config, type));
	if(peek2(backend.types, type).is_vector()){
		QUARK_ASSERT(is_rc_value(backend.types, peek2(backend.types, type).get_vector_element_type(backend.types)) == false);
	}
#endif

	//	Check really only required when unwinding locals.
	if(vec.vector_carray_ptr != nullptr){
		release_vector_carray_pod(backend, vec, type);
	}
}

static void floydrt_release_vector_carray_nonpod(floyd_runtime_t* frp, runtime_value_t vec, runtime_type_t type0){
	auto& r = get_floyd_runtime(frp);
	auto& backend = r.ee->backend;
	const auto& type = lookup_type_ref(backend, type0);
#if DEBUG
	QUARK_ASSERT(is_vector_carray(backend.types, backend.config, type));
	QUARK_ASSERT(is_rc_value(backend.types, peek2(backend.types, type).get_vector_element_type(backend.types)) == true);
#endif

	//	Check really only required when unwinding locals.
	if(vec.vector_carray_ptr != nullptr){
		release_vector_carray_nonpod(backend, vec, type);
	}
}

static void floydrt_release_vector_hamt_pod(floyd_runtime_t* frp, runtime_value_t vec, runtime_type_t type0){
	auto& r = get_floyd_runtime(frp);
	auto& backend = r.ee->backend;
	const auto& type = lookup_type_ref(backend, type0);
#if DEBUG
	QUARK_ASSERT(is_vector_hamt(backend.types, backend.config, type));
	QUARK_ASSERT(is_rc_value(backend.types, peek2(backend.types, type).get_vector_element_type(backend.types)) == false);
#endif

	//	Check really only required when unwinding locals.
	if(vec.vector_hamt_ptr != nullptr){
		release_vector_hamt_pod(backend, vec, type);
	}
}

static void floydrt_release_vector_hamt_nonpod(floyd_runtime_t* frp, runtime_value_t vec, runtime_type_t type0){
	auto& r = get_floyd_runtime(frp);
	auto& backend = r.ee->backend;
	const auto& type = lookup_type_ref(backend, type0);
#if DEBUG
	QUARK_ASSERT(is_vector_hamt(backend.types, backend.config, type));
	QUARK_ASSERT(is_rc_value(backend.types, peek2(backend.types, type).get_vector_element_type(backend.types)) == true);
#endif

	//	Check really only required when unwinding locals.
	if(vec.vector_hamt_ptr != nullptr){
		release_vector_hamt_nonpod(backend, vec, type);
	}
}


static void floydrt_release_dict_cppmap(floyd_runtime_t* frp, runtime_value_t dict, runtime_type_t type0){
	auto& r = get_floyd_runtime(frp);
	auto& backend = r.ee->backend;
	const auto& type = lookup_type_ref(backend, type0);
#if DEBUG
	QUARK_ASSERT(is_dict_cppmap(backend.types, backend.config, type));
#endif

	//	Check really only required when unwinding locals.
	if(dict.dict_cppmap_ptr != nullptr){
		release_dict_cppmap(backend, dict, type);
	}
}
static void floydrt_release_dict_hamt(floyd_runtime_t* frp, runtime_value_t dict, runtime_type_t type0){
	auto& r = get_floyd_runtime(frp);
	auto& backend = r.ee->backend;
	const auto& type = lookup_type_ref(backend, type0);
#if DEBUG
	QUARK_ASSERT(is_dict_hamt(backend.types, backend.config, type));
#endif

	//	Check really only required when unwinding locals.
	if(dict.dict_hamt_ptr != nullptr){
		release_dict_hamt(backend, dict, type);
	}
}



static void floydrt_release_json(floyd_runtime_t* frp, runtime_value_t json, runtime_type_t type0){
	auto& r = get_floyd_runtime(frp);
	auto& backend = r.ee->backend;
	const auto& type = lookup_type_ref(backend, type0);
	QUARK_ASSERT(peek2(backend.types, type).is_json());

	//	NOTICE: Floyd runtime() init will destruct globals, including json::null.
	if(json.json_ptr == nullptr){
	}
	else{
		release_json(backend, json);
	}
}

static void floydrt_release_struct(floyd_runtime_t* frp, runtime_value_t v0, runtime_type_t type0){
	auto& r = get_floyd_runtime(frp);
	auto& backend = r.ee->backend;
	const auto& type = lookup_type_ref(backend, type0);
#if DEBUG
	QUARK_ASSERT(peek2(backend.types, type).is_struct());
#endif

	//	Check really only required when unwinding locals.
	if(v0.struct_ptr != nullptr){
		release_struct(backend, make_runtime_struct(v0.struct_ptr), type);
	}
}


static llvm::FunctionType* make_release(llvm::LLVMContext& context, const llvm_type_lookup& type_lookup, llvm::Type& collection_type){
	return llvm::FunctionType::get(
		llvm::Type::getVoidTy(context),
		{
			make_frp_type(type_lookup),
			&collection_type,
			make_runtime_type_type(type_lookup)
		},
		false
	);
}

static std::vector<function_bind_t> release_funcs(llvm::LLVMContext& context, const llvm_type_lookup& type_lookup){
	return std::vector<function_bind_t> {
		function_bind_t{ module_symbol_t("release_vector_carray_pod"), make_release(context, type_lookup, *make_generic_vec_type_byvalue(type_lookup)->getPointerTo()), reinterpret_cast<void*>(floydrt_release_vector_carray_pod) },
		function_bind_t{ module_symbol_t("release_vector_carray_nonpod"), make_release(context, type_lookup, *make_generic_vec_type_byvalue(type_lookup)->getPointerTo()), reinterpret_cast<void*>(floydrt_release_vector_carray_nonpod) },
		function_bind_t{ module_symbol_t("release_vector_hamt_pod"), make_release(context, type_lookup, *make_generic_vec_type_byvalue(type_lookup)->getPointerTo()), reinterpret_cast<void*>(floydrt_release_vector_hamt_pod) },
		function_bind_t{ module_symbol_t("release_vector_hamt_nonpod"), make_release(context, type_lookup, *make_generic_vec_type_byvalue(type_lookup)->getPointerTo()), reinterpret_cast<void*>(floydrt_release_vector_hamt_nonpod) },
		function_bind_t{ module_symbol_t("release_dict_cppmap"), make_release(context, type_lookup, *make_generic_dict_type_byvalue(type_lookup)->getPointerTo()), reinterpret_cast<void*>(floydrt_release_dict_cppmap) },
		function_bind_t{ module_symbol_t("release_dict_hamt"), make_release(context, type_lookup, *make_generic_dict_type_byvalue(type_lookup)->getPointerTo()), reinterpret_cast<void*>(floydrt_release_dict_hamt) },
		function_bind_t{ module_symbol_t("release_json"), make_release(context, type_lookup, *get_llvm_type_as_arg(type_lookup, type_t::make_json())), reinterpret_cast<void*>(floydrt_release_json) },
		function_bind_t{ module_symbol_t("release_struct"), make_release(context, type_lookup, *get_generic_struct_type_byvalue(type_lookup)->getPointerTo()), reinterpret_cast<void*>(floydrt_release_struct) }
	};
}

void generate_release(llvm_function_generator_t& gen_acc, llvm::Value& value_reg, const type_t& type){
	QUARK_ASSERT(gen_acc.check_invariant());
	QUARK_ASSERT(type.check_invariant());

	auto& frp_reg = *gen_acc.get_callers_fcp();
	auto& itype_reg = *generate_itype_constant(gen_acc.gen, type);
	auto& builder = gen_acc.get_builder();
	const auto& types = gen_acc.gen.type_lookup.state.types;
	const auto peek = peek2(types, type);

	if(is_rc_value(types, type)){
		if(peek.is_string()){
			const auto res = resolve_func(gen_acc.gen.link_map, module_symbol_t("release_vector_carray_pod"));
			builder.CreateCall(res.llvm_codegen_f, { &frp_reg, &value_reg, &itype_reg });
		}
		else if(peek.is_vector()){
			const bool is_element_pod = is_rc_value(types, peek.get_vector_element_type(types)) ? false : true;

			if(is_vector_carray(types, gen_acc.gen.settings.config, type) && is_element_pod == true){
				const auto res = resolve_func(gen_acc.gen.link_map, module_symbol_t("release_vector_carray_pod"));
				builder.CreateCall(res.llvm_codegen_f, { &frp_reg, &value_reg, &itype_reg });
			}
			else if(is_vector_carray(types, gen_acc.gen.settings.config, type) && is_element_pod == false){
				const auto res = resolve_func(gen_acc.gen.link_map, module_symbol_t("release_vector_carray_nonpod"));
				builder.CreateCall(res.llvm_codegen_f, { &frp_reg, &value_reg, &itype_reg });
			}
			else if(is_vector_hamt(types, gen_acc.gen.settings.config, type) && is_element_pod == true){
				const auto res = resolve_func(gen_acc.gen.link_map, module_symbol_t("release_vector_hamt_pod"));
				builder.CreateCall(res.llvm_codegen_f, { &frp_reg, &value_reg, &itype_reg });
			}
			else if(is_vector_hamt(types, gen_acc.gen.settings.config, type) && is_element_pod == false){
				const auto res = resolve_func(gen_acc.gen.link_map, module_symbol_t("release_vector_hamt_nonpod"));
				builder.CreateCall(res.llvm_codegen_f, { &frp_reg, &value_reg, &itype_reg });
			}
			else{
				QUARK_ASSERT(false);
			}
		}
		else if(peek.is_dict()){
			if(is_dict_cppmap(types, gen_acc.gen.settings.config, type)){
				const auto res = resolve_func(gen_acc.gen.link_map, module_symbol_t("release_dict_cppmap"));
				builder.CreateCall(res.llvm_codegen_f, { &frp_reg, &value_reg, &itype_reg });
			}
			else if(is_dict_hamt(types, gen_acc.gen.settings.config, type)){
				const auto res = resolve_func(gen_acc.gen.link_map, module_symbol_t("release_dict_hamt"));
				builder.CreateCall(res.llvm_codegen_f, { &frp_reg, &value_reg, &itype_reg });
			}
			else{
				QUARK_ASSERT(false);
			}
		}
		else if(peek.is_json()){
			const auto res = resolve_func(gen_acc.gen.link_map, module_symbol_t("release_json"));
			builder.CreateCall(res.llvm_codegen_f, { &frp_reg, &value_reg, &itype_reg });
		}
		else if(peek.is_struct()){
			const auto res = resolve_func(gen_acc.gen.link_map, module_symbol_t("release_struct"));
			builder.CreateCall(res.llvm_codegen_f, { &frp_reg, &value_reg, &itype_reg });
		}
		else{
			QUARK_ASSERT(false);
		}
	}
	else{
	}
}








std::vector<function_bind_t> get_runtime_function_binds(llvm::LLVMContext& context, const llvm_type_lookup& type_lookup){
	const std::vector<std::vector<function_bind_t>> result0 = {
		floydrt_alloc_kstr__make(context, type_lookup),
		floydrt_allocate_vector__make(context, type_lookup),
		floydrt_allocate_vector_fill__make(context, type_lookup),

		floydrt_store_vector_element_mutable__make(context, type_lookup),
		floydrt_concatunate_vectors__make(context, type_lookup),
		floydrt_load_vector_element__make(context, type_lookup),

		floydrt_allocate_dict__make(context, type_lookup),
		floydrt_lookup_dict_cppmap__make(context, type_lookup),
		floydrt_lookup_dict_hamt__make(context, type_lookup),
		floydrt_store_dict_mutable__make(context, type_lookup),

		floydrt_allocate_json__make(context, type_lookup),
		floydrt_lookup_json__make(context, type_lookup),
		floydrt_json_to_string__make(context, type_lookup),

		floydrt_allocate_struct__make(context, type_lookup),
		floydrt_update_struct_member__make(context, type_lookup),

		floydrt_compare_values__make(context, type_lookup),
		floydrt_get_profile_time__make(context, type_lookup),
		floydrt_analyse_benchmark_samples__make(context, type_lookup),

		retain_funcs(context, type_lookup),
		release_funcs(context, type_lookup)
	};

	std::vector<function_bind_t> result;
	for(const auto&e: result0){
		result.insert(result.end(), e.begin(), e.end());
	}

	return result;
}




////////////////////////////////		runtime_functions_t



runtime_functions_t::runtime_functions_t(const std::vector<llvm_function_link_entry_t>& function_defs) :
	floydrt_init(resolve_func(function_defs, module_symbol_t("init"))),
	floydrt_deinit(resolve_func(function_defs, module_symbol_t("deinit"))),


	floydrt_alloc_kstr(resolve_func(function_defs, module_symbol_t("alloc_kstr"))),
	floydrt_allocate_vector_fill(resolve_func(function_defs, module_symbol_t("allocate_vector_fill"))),

	floydrt_store_vector_element_hamt_mutable(resolve_func(function_defs, module_symbol_t("store_vector_element_hamt_mutable"))),
	floydrt_concatunate_vectors(resolve_func(function_defs, module_symbol_t("concatunate_vectors"))),
	floydrt_load_vector_element_hamt(resolve_func(function_defs, module_symbol_t("load_vector_element_hamt"))),


	floydrt_allocate_dict(resolve_func(function_defs, module_symbol_t("allocate_dict"))),


	floydrt_allocate_json(resolve_func(function_defs, module_symbol_t("allocate_json"))),
	floydrt_lookup_json(resolve_func(function_defs, module_symbol_t("lookup_json"))),
	floydrt_json_to_string(resolve_func(function_defs, module_symbol_t("json_to_string"))),


	floydrt_allocate_struct(resolve_func(function_defs, module_symbol_t("allocate_struct"))),

	floydrt_compare_values(resolve_func(function_defs, module_symbol_t("compare_values"))),


	floydrt_get_profile_time(resolve_func(function_defs, module_symbol_t("get_profile_time"))),
	floydrt_analyse_benchmark_samples(resolve_func(function_defs, module_symbol_t("analyse_benchmark_samples")))
{
}


} // floyd
