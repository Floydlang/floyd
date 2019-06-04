//
//  floyd_llvm_runtime.cpp
//  floyd_speak
//
//  Created by Marcus Zetterquist on 2019-04-24.
//  Copyright © 2019 Marcus Zetterquist. All rights reserved.
//

#include "floyd_llvm_runtime.h"

#include "floyd_llvm_codegen.h"
#include "floyd_runtime.h"

#include "sha1_class.h"
#include "text_parser.h"
#include "host_functions.h"
#include "file_handling.h"
#include "os_process.h"
#include "compiler_helpers.h"
#include "pass3.h"

#include <llvm/ADT/APInt.h>
#include <llvm/IR/Verifier.h>
#include <llvm/ExecutionEngine/ExecutionEngine.h>
#include <llvm/ExecutionEngine/GenericValue.h>
#include <llvm/ExecutionEngine/MCJIT.h>
#include <llvm/IR/Argument.h>
#include <llvm/IR/BasicBlock.h>
#include <llvm/IR/Constants.h>
#include <llvm/IR/DerivedTypes.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/InstrTypes.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Type.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/Support/Casting.h>
#include <llvm/Support/TargetSelect.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/IR/DataLayout.h>

#include "llvm/Bitcode/BitstreamWriter.h"

#include <iostream>
#include <fstream>

#include <thread>
#include <deque>
#include <future>

#include <condition_variable>

namespace floyd {






llvm_execution_engine_t& get_floyd_runtime(void* floyd_runtime_ptr);

value_t from_runtime_value(const llvm_execution_engine_t& runtime, const runtime_value_t encoded_value, const typeid_t& type);
runtime_value_t to_runtime_value(llvm_execution_engine_t& runtime, const value_t& value);



const function_def_t& find_function_def2(const std::vector<function_def_t>& function_defs, const std::string& function_name){
	auto it = std::find_if(function_defs.begin(), function_defs.end(), [&] (const function_def_t& e) { return e.def_name == function_name; } );
	QUARK_ASSERT(it != function_defs.end());

	QUARK_ASSERT(it->llvm_f != nullptr);
	return *it;
}


void copy_elements(runtime_value_t dest[], runtime_value_t source[], uint64_t count){
	for(auto i = 0 ; i < count ; i++){
		dest[i] = source[i];
	}
}





void* get_global_ptr(llvm_execution_engine_t& ee, const std::string& name){
	QUARK_ASSERT(ee.check_invariant());
	QUARK_ASSERT(name.empty() == false);

	const auto addr = ee.ee->getGlobalValueAddress(name);
	return  (void*)addr;
}

void* get_global_function(llvm_execution_engine_t& ee, const std::string& name){
	QUARK_ASSERT(ee.check_invariant());
	QUARK_ASSERT(name.empty() == false);

	const auto addr = ee.ee->getFunctionAddress(name);
	return (void*)addr;
}


std::pair<void*, typeid_t> bind_function(llvm_execution_engine_t& ee, const std::string& name){
	QUARK_ASSERT(ee.check_invariant());
	QUARK_ASSERT(name.empty() == false);

	const auto f = reinterpret_cast<FLOYD_RUNTIME_F*>(get_global_function(ee, name));
	if(f != nullptr){
		const function_def_t def = find_function_def2(ee.function_defs, std::string() + "floyd_funcdef__" + name);
		const auto function_type = def.floyd_fundef._function_type;
		return { f, function_type };
	}
	else{
		return { nullptr, typeid_t::make_undefined() };
	}
}

std::pair<void*, typeid_t> bind_global(llvm_execution_engine_t& ee, const std::string& name){
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

value_t load_via_ptr(const llvm_execution_engine_t& runtime, const void* value_ptr, const typeid_t& type){
	QUARK_ASSERT(runtime.check_invariant());
	QUARK_ASSERT(value_ptr != nullptr);
	QUARK_ASSERT(type.check_invariant());

	const auto result = load_via_ptr2(value_ptr, type);
	const auto result2 = from_runtime_value(runtime, result, type);
	return result2;
}

value_t load_global(llvm_execution_engine_t& ee, const std::pair<void*, typeid_t>& v){
	QUARK_ASSERT(v.first != nullptr);
	QUARK_ASSERT(v.second.is_undefined() == false);

	return load_via_ptr(ee, v.first, v.second);
}

void store_via_ptr(llvm_execution_engine_t& runtime, const typeid_t& member_type, void* value_ptr, const value_t& value){
	QUARK_ASSERT(runtime.check_invariant());
	QUARK_ASSERT(member_type.check_invariant());
	QUARK_ASSERT(value_ptr != nullptr);
	QUARK_ASSERT(value.check_invariant());

	const auto value2 = to_runtime_value(runtime, value);
	store_via_ptr2(value_ptr, member_type, value2);
}



llvm_bind_t bind_function2(llvm_execution_engine_t& ee, const std::string& name){
	std::pair<void*, typeid_t> a = bind_function(ee, name);
	return llvm_bind_t {
		name,
		a.first,
		a.second
	};
}

/*
value_t call_function(llvm_execution_engine_t& ee, llvm_bind_t& f){
	//value_t call_function(llvm_execution_engine_t& ee, const std::pair<void*, typeid_t>& f);

	const auto result = call_function(ee, std::pair<void*, typeid_t>(f.address, f.type));
	return result;
}
*/


int64_t llvm_call_main(llvm_execution_engine_t& ee, const std::pair<void*, typeid_t>& f, const std::vector<std::string>& main_args){
	QUARK_ASSERT(f.first != nullptr);

	//??? Check this earlier.
	if(f.second == get_main_signature_arg_impure() || f.second == get_main_signature_arg_pure()){
		const auto f2 = *reinterpret_cast<FLOYD_RUNTIME_MAIN_ARGS_IMPURE*>(f.first);

		//??? Slow path via value_t
		std::vector<value_t> main_args2;
		for(const auto& e: main_args){
			main_args2.push_back(value_t::make_string(e));
		}
		const auto main_args3 = value_t::make_vector_value(typeid_t::make_string(), main_args2);
		const auto main_args4 = to_runtime_value(ee, main_args3);
		const auto main_result_int = (*f2)(&ee, main_args4);
		return main_result_int;
	}
	else if(f.second == get_main_signature_no_arg_impure() || f.second == get_main_signature_no_arg_pure()){
		const auto f2 = *reinterpret_cast<FLOYD_RUNTIME_MAIN_NO_ARGS_IMPURE*>(f.first);
		const auto main_result_int = (*f2)(&ee);
		return main_result_int;
	}
	else{
		throw std::exception();
	}
}


//	TO RUNTIME_VALUE_T AND BACK
///////////////////////////////////////////////////////////////////////////////////////




runtime_value_t to_runtime_string(llvm_execution_engine_t& r, const std::string& s){
	QUARK_ASSERT(r.check_invariant());

	const auto count = static_cast<uint64_t>(s.size());
	const auto allocation_count = size_to_allocation_blocks(s.size());
	auto v = alloc_vec(r.heap, allocation_count, count);
	auto result = runtime_value_t{ .vector_ptr = v };

	char* char_ptr = get_vec_chars(result);
	for(int i = 0 ; i < count ; i++){
		char_ptr[i] = s[i];
	}
	return result;
}

std::string from_runtime_string(const llvm_execution_engine_t& r, runtime_value_t encoded_value){
	QUARK_ASSERT(r.check_invariant());
	QUARK_ASSERT(encoded_value.vector_ptr != nullptr);

	const auto ptr = get_vec_chars(encoded_value);
	const auto size = get_vec_string_size(encoded_value);

	return std::string(ptr, ptr + size);
}




runtime_value_t to_runtime_struct(llvm_execution_engine_t& runtime, const typeid_t::struct_t& exact_type, const value_t& value){
	QUARK_ASSERT(runtime.check_invariant());
	QUARK_ASSERT(value.check_invariant());

	const llvm::DataLayout& data_layout = runtime.ee->getDataLayout();
	auto t2 = get_exact_struct_type(runtime.type_interner, value.get_type());
	const llvm::StructLayout* layout = data_layout.getStructLayout(t2);

	const auto struct_bytes = layout->getSizeInBytes();

	auto s = alloc_struct(runtime.heap, struct_bytes);
	const auto struct_base_ptr = s->get_data_ptr();

	int member_index = 0;
	const auto& struct_data = value.get_struct_value();

	for(const auto& e: struct_data->_member_values){
		const auto offset = layout->getElementOffset(member_index);
		const auto member_ptr = reinterpret_cast<void*>(struct_base_ptr + offset);
		store_via_ptr(runtime, e.get_type(), member_ptr, e);
		member_index++;
	}
	return make_runtime_struct(s);
}

value_t from_runtime_struct(const llvm_execution_engine_t& runtime, const runtime_value_t encoded_value, const typeid_t& type){
	QUARK_ASSERT(type.check_invariant());

	auto& context = runtime.instance->context;

	const auto& struct_def = type.get_struct();
	const auto struct_base_ptr = encoded_value.struct_ptr->get_data_ptr();

	const llvm::DataLayout& data_layout = runtime.ee->getDataLayout();
	auto t2 = get_exact_struct_type(runtime.type_interner, type);
	const llvm::StructLayout* layout = data_layout.getStructLayout(t2);

	std::vector<value_t> members;
	int member_index = 0;
	for(const auto& e: struct_def._members){
		const auto offset = layout->getElementOffset(member_index);
		const auto member_ptr = reinterpret_cast<const runtime_value_t*>(struct_base_ptr + offset);
		const auto member_value = from_runtime_value(runtime, *member_ptr, e._type);
		members.push_back(member_value);
		member_index++;
	}
	return value_t::make_struct_value(type, members);
}



runtime_value_t to_runtime_vector(llvm_execution_engine_t& r, const value_t& value){
	QUARK_ASSERT(r.check_invariant());
	QUARK_ASSERT(value.check_invariant());
	QUARK_ASSERT(value.get_type().is_vector());

	const auto& v0 = value.get_vector_value();

	const auto count = v0.size();
	auto v = alloc_vec(r.heap, count, count);
	auto result = runtime_value_t{ .vector_ptr = v };

	auto p = v->get_element_ptr();
	for(int i = 0 ; i < count ; i++){
		const auto& e = v0[i];
		const auto a = to_runtime_value(r, e);
		p[i] = a;
	}
	return result;
}

value_t from_runtime_vector(const llvm_execution_engine_t& runtime, const runtime_value_t encoded_value, const typeid_t& type){
	QUARK_ASSERT(runtime.check_invariant());
	QUARK_ASSERT(type.check_invariant());

	const auto element_type = type.get_vector_element_type();
	const auto vec = encoded_value.vector_ptr;

	std::vector<value_t> elements;
	const auto count = vec->get_element_count();
	auto p = vec->get_element_ptr();
	for(int i = 0 ; i < count ; i++){
		const auto value_encoded = p[i];
		const auto value = from_runtime_value(runtime, value_encoded, element_type);
		elements.push_back(value);
	}
	const auto val = value_t::make_vector_value(element_type, elements);
	return val;
}

runtime_value_t to_runtime_dict(const llvm_execution_engine_t& runtime, const typeid_t::dict_t& exact_type, const value_t& value){
	QUARK_ASSERT(runtime.check_invariant());
	QUARK_ASSERT(value.check_invariant());
	QUARK_ASSERT(value.get_type().is_dict());

	NOT_IMPLEMENTED_YET();
	return runtime_value_t{ .vector_ptr = nullptr };
}

value_t from_runtime_dict(const llvm_execution_engine_t& runtime, const runtime_value_t encoded_value, const typeid_t& type){
	QUARK_ASSERT(runtime.check_invariant());
	QUARK_ASSERT(encoded_value.check_invariant());
	QUARK_ASSERT(type.check_invariant());

	const auto value_type = type.get_dict_value_type();
	const auto dict = encoded_value.dict_ptr;

	std::map<std::string, value_t> values;
	const auto& map2 = dict->get_map();
	for(const auto& e: map2){
		const auto value = from_runtime_value(runtime, e.second, value_type);
		values.insert({ e.first, value} );
	}
	const auto val = value_t::make_dict_value(type, values);
	return val;
}

runtime_value_t to_runtime_value(llvm_execution_engine_t& runtime, const value_t& value){
	QUARK_ASSERT(runtime.check_invariant());
	QUARK_ASSERT(value.check_invariant());

	const auto type = value.get_type();

	struct visitor_t {
		llvm_execution_engine_t& runtime;
		const value_t& value;

		runtime_value_t operator()(const typeid_t::undefined_t& e) const{
			UNSUPPORTED();
		}
		runtime_value_t operator()(const typeid_t::any_t& e) const{
			UNSUPPORTED();
		}

		runtime_value_t operator()(const typeid_t::void_t& e) const{
			UNSUPPORTED();
		}
		runtime_value_t operator()(const typeid_t::bool_t& e) const{
			return { .bool_value = (uint8_t)(value.get_bool_value() ? 1 : 0) };
		}
		runtime_value_t operator()(const typeid_t::int_t& e) const{
			return { .int_value = value.get_int_value() };
		}
		runtime_value_t operator()(const typeid_t::double_t& e) const{
			return { .double_value = value.get_double_value() };
		}
		runtime_value_t operator()(const typeid_t::string_t& e) const{
			return to_runtime_string(runtime, value.get_string_value());
		}

		runtime_value_t operator()(const typeid_t::json_type_t& e) const{
//			auto result = new json_t(value.get_json_value());
//			return runtime_value_t { .json_ptr = result };
			auto result = alloc_json(runtime.heap, value.get_json_value());
			return runtime_value_t { .json_ptr = result };
		}
		runtime_value_t operator()(const typeid_t::typeid_type_t& e) const{
			const auto t0 = value.get_typeid_value();
			const auto t1 = lookup_runtime_type(runtime.type_interner.interner, t0);
			return make_runtime_typeid(t1);
		}

		runtime_value_t operator()(const typeid_t::struct_t& e) const{
			return to_runtime_struct(runtime, e, value);
		}
		runtime_value_t operator()(const typeid_t::vector_t& e) const{
			return to_runtime_vector(runtime, value);
		}
		runtime_value_t operator()(const typeid_t::dict_t& e) const{
			return to_runtime_dict(runtime, e, value);
		}
		runtime_value_t operator()(const typeid_t::function_t& e) const{
			UNSUPPORTED();
#if 0
			//	If function returns a DYN, it must have a dyn_return.
			QUARK_ASSERT(e._parts[0].is_any() == false || e.dyn_return != return_dyn_type::none);

			QUARK_ASSERT(e._parts.size() >= 1);

			for(const auto& m: e._parts){
				QUARK_ASSERT(m.check_invariant());
			}
#endif
			QUARK_ASSERT(false);
		}
		runtime_value_t operator()(const typeid_t::unresolved_t& e) const{
			UNSUPPORTED();
		}
	};
	return std::visit(visitor_t{ runtime, value }, type._contents);
}

value_t from_runtime_value(const llvm_execution_engine_t& runtime, const runtime_value_t encoded_value, const typeid_t& type){
	QUARK_ASSERT(type.check_invariant());

	struct visitor_t {
		const llvm_execution_engine_t& runtime;
		const runtime_value_t& encoded_value;
		const typeid_t& type;

		value_t operator()(const typeid_t::undefined_t& e) const{
			UNSUPPORTED();
		}
		value_t operator()(const typeid_t::any_t& e) const{
			UNSUPPORTED();
		}

		value_t operator()(const typeid_t::void_t& e) const{
			UNSUPPORTED();
		}
		value_t operator()(const typeid_t::bool_t& e) const{
			return value_t::make_bool(encoded_value.bool_value == 0 ? false : true);
		}
		value_t operator()(const typeid_t::int_t& e) const{
			return value_t::make_int(encoded_value.int_value);
		}
		value_t operator()(const typeid_t::double_t& e) const{
			return value_t::make_double(encoded_value.double_value);
		}
		value_t operator()(const typeid_t::string_t& e) const{
			return value_t::make_string(from_runtime_string(runtime, encoded_value));
		}

		value_t operator()(const typeid_t::json_type_t& e) const{
			if(encoded_value.json_ptr == nullptr){
				return value_t::make_json_value(json_t());
			}
			else{
				const auto& j = encoded_value.json_ptr->get_json();
				return value_t::make_json_value(j);
			}
		}
		value_t operator()(const typeid_t::typeid_type_t& e) const{
			const auto type1 = lookup_type(runtime.type_interner.interner, encoded_value.typeid_itype);
			const auto type2 = value_t::make_typeid_value(type1);
			return type2;
		}

		value_t operator()(const typeid_t::struct_t& e) const{
			return from_runtime_struct(runtime, encoded_value, type);
		}
		value_t operator()(const typeid_t::vector_t& e) const{
			return from_runtime_vector(runtime, encoded_value, type);
		}
		value_t operator()(const typeid_t::dict_t& e) const{
			return from_runtime_dict(runtime, encoded_value, type);
		}
		value_t operator()(const typeid_t::function_t& e) const{
			UNSUPPORTED();
#if 0
			//	If function returns a DYN, it must have a dyn_return.
			QUARK_ASSERT(e._parts[0].is_any() == false || e.dyn_return != return_dyn_type::none);

			QUARK_ASSERT(e._parts.size() >= 1);

			for(const auto& m: e._parts){
				QUARK_ASSERT(m.check_invariant());
			}
#endif
			QUARK_ASSERT(false);
		}
		value_t operator()(const typeid_t::unresolved_t& e) const{
			UNSUPPORTED();
		}
	};
	return std::visit(visitor_t{ runtime, encoded_value, type }, type._contents);
}




////////////////////////////////	HELPERS FOR RUNTIME CALLBACKS




/*
@variable = global i32 21
define i32 @main() {
%1 = load i32, i32* @variable ; load the global variable
%2 = mul i32 %1, 2
store i32 %2, i32* @variable ; store instruction to write to global variable
ret i32 %2
}
*/

llvm_execution_engine_t& get_floyd_runtime(void* floyd_runtime_ptr){
	QUARK_ASSERT(floyd_runtime_ptr != nullptr);

	auto ptr = reinterpret_cast<llvm_execution_engine_t*>(floyd_runtime_ptr);
	QUARK_ASSERT(ptr != nullptr);
	QUARK_ASSERT(ptr->debug_magic == k_debug_magic);
	return *ptr;
}

void hook(const std::string& s, void* floyd_runtime_ptr, runtime_value_t arg){
	auto& r = get_floyd_runtime(floyd_runtime_ptr);
	throw std::runtime_error("HOST FUNCTION NOT IMPLEMENTED FOR LLVM");
}



std::string gen_to_string(llvm_execution_engine_t& runtime, runtime_value_t arg_value, runtime_type_t arg_type){
	QUARK_ASSERT(runtime.check_invariant());

	const auto type = lookup_type(runtime.type_interner.interner, arg_type);
	const auto value = from_runtime_value(runtime, arg_value, type);
	const auto a = to_compact_string2(value);
	return a;
}





static void retain_value(llvm_execution_engine_t& runtime, runtime_value_t value, const typeid_t& type){
	if(is_rc_value(type)){
		if(type.is_string()){
			vec_addref(*value.vector_ptr);
		}
		else if(type.is_vector()){
			vec_addref(*value.vector_ptr);
		}
		else if(type.is_dict()){
			dict_addref(*value.dict_ptr);
		}
		else if(type.is_json_value()){
			json_addref(*value.json_ptr);
		}
		else if(type.is_struct()){
			struct_addref(*value.struct_ptr);
		}
		else{
			QUARK_ASSERT(false);
		}
	}
}

//??? use these in all runtime code
static void release_dict_deep(llvm_execution_engine_t& runtime, DICT_T* dict, const typeid_t& type);
static void release_vec_deep(llvm_execution_engine_t& runtime, VEC_T* vec, const typeid_t& type);
static void release_struct_deep(llvm_execution_engine_t& runtime, STRUCT_T* s, const typeid_t& type);

static void release_deep(llvm_execution_engine_t& runtime, runtime_value_t value, const typeid_t& type){
	if(is_rc_value(type)){
		if(type.is_string()){
			release_vec_deep(runtime, value.vector_ptr, type);
		}
		else if(type.is_vector()){
			release_vec_deep(runtime, value.vector_ptr, type);
		}
		else if(type.is_dict()){
			release_dict_deep(runtime, value.dict_ptr, type);
		}
		else if(type.is_json_value()){
			json_releaseref(value.json_ptr);
		}
		else if(type.is_struct()){
			release_struct_deep(runtime, value.struct_ptr, type);
		}
		else{
			QUARK_ASSERT(false);
		}
	}
}

static void release_dict_deep(llvm_execution_engine_t& runtime, DICT_T* dict, const typeid_t& type){
	QUARK_ASSERT(dict != nullptr);
	QUARK_ASSERT(type.is_dict());

	//??? Make atomic. CAS?
	//??? unsafe hack. Some other code could stop rc from becoming 0.
	if(dict->alloc.rc == 1){

		//	Release all elements.
		const auto element_type = type.get_dict_value_type();
		if(is_rc_value(element_type)){
			auto m = dict->get_map();
			for(const auto& e: m){
				release_deep(runtime, e.second, element_type);
			}
		}
	}

	dict_releaseref(dict);
}

static void release_vec_deep(llvm_execution_engine_t& runtime, VEC_T* vec, const typeid_t& type){
	QUARK_ASSERT(vec != nullptr);
	QUARK_ASSERT(type.is_string() || type.is_vector());

	//??? Make atomic. CAS?
	//??? unsafe hack. Some other code could stop rc from becoming 0.
	if(vec->alloc.rc == 1){

		if(type.is_string()){
			//	String has no elements to release.
		}
		else if(type.is_vector()){
			//	Release all elements.
			const auto element_type = type.get_vector_element_type();
			if(is_rc_value(element_type)){
				auto element_ptr = vec->get_element_ptr();
				for(int i = 0 ; i < vec->get_element_count() ; i++){
					const auto& element = element_ptr[i];
					release_deep(runtime, element, element_type);
				}
			}
		}
		else{
			QUARK_ASSERT(false);
		}
	}

	vec_releaseref(vec);
}


static void release_struct_deep(llvm_execution_engine_t& runtime, STRUCT_T* s, const typeid_t& type){
	QUARK_ASSERT(s != nullptr);

	//??? Make atomic. CAS?
	//??? unsafe hack. Some other code could stop rc from becoming 0.
	if(s->alloc.rc == 1){
		const auto& struct_def = type.get_struct();
		const auto struct_base_ptr = s->get_data_ptr();

		const llvm::DataLayout& data_layout = runtime.ee->getDataLayout();
		auto t2 = get_exact_struct_type(runtime.type_interner, type);
		const llvm::StructLayout* layout = data_layout.getStructLayout(t2);

		int member_index = 0;
		for(const auto& e: struct_def._members){
			if(is_rc_value(e._type)){
				const auto offset = layout->getElementOffset(member_index);
				const auto member_ptr = reinterpret_cast<const runtime_value_t*>(struct_base_ptr + offset);
				release_deep(runtime, *member_ptr, e._type);
				member_index++;
			}
		}
	}

	struct_releaseref(s);
}





//	Host functions, automatically called by the LLVM execution engine.
//	The names of these are computed from the host-id in the symbol table, not the names of the functions/symbols.
//	They must use C calling convention so llvm JIT can find them.
//	Make sure they are not dead-stripped out of binary!
////////////////////////////////////////////////////////////////////////////////


////////////////////////////////		fr_retain_vec()


void fr_retain_vec(void* floyd_runtime_ptr, VEC_T* vec, runtime_type_t type0){
	auto& r = get_floyd_runtime(floyd_runtime_ptr);
	QUARK_ASSERT(vec != nullptr);
	const auto type = lookup_type(r.type_interner.interner, type0);
	QUARK_ASSERT(is_rc_value(type));

	QUARK_ASSERT(type.is_string() || type.is_vector());

	vec_addref(*vec);
}

host_func_t fr_retain_vec__make(llvm::LLVMContext& context, const llvm_type_interner_t& interner){
	llvm::FunctionType* function_type = llvm::FunctionType::get(
		llvm::Type::getVoidTy(context),
		{
			make_frp_type(context),
			make_vec_type(interner)->getPointerTo(),
			make_runtime_type_type(context)
		},
		false
	);
	return { "fr_retain_vec", function_type, reinterpret_cast<void*>(fr_retain_vec) };
}


////////////////////////////////		fr_release_vec()


void fr_release_vec(void* floyd_runtime_ptr, VEC_T* vec, runtime_type_t type0){
	auto& r = get_floyd_runtime(floyd_runtime_ptr);
	QUARK_ASSERT(vec != nullptr);
	const auto type = lookup_type(r.type_interner.interner, type0);
	QUARK_ASSERT(type.is_string() || type.is_vector());

	release_vec_deep(r, vec, type);
}

host_func_t fr_release_vec__make(llvm::LLVMContext& context, const llvm_type_interner_t& interner){
	llvm::FunctionType* function_type = llvm::FunctionType::get(
		llvm::Type::getVoidTy(context),
		{
			make_frp_type(context),
			make_vec_type(interner)->getPointerTo(),
			make_runtime_type_type(context)
		},
		false
	);
	return { "fr_release_vec", function_type, reinterpret_cast<void*>(fr_release_vec) };
}





////////////////////////////////		fr_retain_dict()


void fr_retain_dict(void* floyd_runtime_ptr, DICT_T* dict, runtime_type_t type0){
	auto& r = get_floyd_runtime(floyd_runtime_ptr);
	QUARK_ASSERT(dict != nullptr);
	const auto type = lookup_type(r.type_interner.interner, type0);
	QUARK_ASSERT(is_rc_value(type));

	QUARK_ASSERT(type.is_dict());

	dict_addref(*dict);
}

host_func_t fr_retain_dict__make(llvm::LLVMContext& context, const llvm_type_interner_t& interner){
	llvm::FunctionType* function_type = llvm::FunctionType::get(
		llvm::Type::getVoidTy(context),
		{
			make_frp_type(context),
			make_dict_type(interner)->getPointerTo(),
			make_runtime_type_type(context)
		},
		false
	);
	return { "fr_retain_dict", function_type, reinterpret_cast<void*>(fr_retain_dict) };
}


////////////////////////////////		fr_release_dict()



void fr_release_dict(void* floyd_runtime_ptr, DICT_T* dict, runtime_type_t type0){
	auto& r = get_floyd_runtime(floyd_runtime_ptr);
	QUARK_ASSERT(dict != nullptr);
	const auto type = lookup_type(r.type_interner.interner, type0);
	QUARK_ASSERT(type.is_dict());

	release_dict_deep(r, dict, type);
}

host_func_t fr_release_dict__make(llvm::LLVMContext& context, const llvm_type_interner_t& interner){
	llvm::FunctionType* function_type = llvm::FunctionType::get(
		llvm::Type::getVoidTy(context),
		{
			make_frp_type(context),
			make_dict_type(interner)->getPointerTo(),
			make_runtime_type_type(context)
		},
		false
	);
	return { "fr_release_dict", function_type, reinterpret_cast<void*>(fr_release_dict) };
}


////////////////////////////////		fr_retain_json()


void fr_retain_json(void* floyd_runtime_ptr, JSON_T* json, runtime_type_t type0){
	auto& r = get_floyd_runtime(floyd_runtime_ptr);

	const auto type = lookup_type(r.type_interner.interner, type0);
	QUARK_ASSERT(is_rc_value(type));

	//	NOTICE: Floyd runtime() init will destruct globals, including json_value::null.
	if(json == nullptr){
	}
	else{
		QUARK_ASSERT(type.is_json_value());

		json_addref(*json);
	}
}

host_func_t fr_retain_json__make(llvm::LLVMContext& context, const llvm_type_interner_t& interner){
	llvm::FunctionType* function_type = llvm::FunctionType::get(
		llvm::Type::getVoidTy(context),
		{
			make_frp_type(context),
			intern_type(interner, typeid_t::make_json_value()),
			make_runtime_type_type(context)
		},
		false
	);
	return { "fr_retain_json", function_type, reinterpret_cast<void*>(fr_retain_json) };
}


////////////////////////////////		fr_release_json()


void fr_release_json(void* floyd_runtime_ptr, JSON_T* json, runtime_type_t type0){
	auto& r = get_floyd_runtime(floyd_runtime_ptr);
	const auto type = lookup_type(r.type_interner.interner, type0);
	QUARK_ASSERT(type.is_json_value());

	//	NOTICE: Floyd runtime() init will destruct globals, including json_value::null.
	if(json == nullptr){
	}
	else{
		QUARK_ASSERT(json != nullptr);
		json_releaseref(json);
	}
}

host_func_t fr_release_json__make(llvm::LLVMContext& context, const llvm_type_interner_t& interner){
	llvm::FunctionType* function_type = llvm::FunctionType::get(
		llvm::Type::getVoidTy(context),
		{
			make_frp_type(context),
			intern_type(interner, typeid_t::make_json_value()),
			make_runtime_type_type(context)
		},
		false
	);
	return { "fr_release_json", function_type, reinterpret_cast<void*>(fr_release_json) };
}










////////////////////////////////		fr_retain_struct()


void fr_retain_struct(void* floyd_runtime_ptr, STRUCT_T* v, runtime_type_t type0){
	auto& r = get_floyd_runtime(floyd_runtime_ptr);

	const auto type = lookup_type(r.type_interner.interner, type0);
	QUARK_ASSERT(is_rc_value(type));
	QUARK_ASSERT(type.is_struct());

	struct_addref(*v);
}

host_func_t fr_retain_struct__make(llvm::LLVMContext& context, const llvm_type_interner_t& interner){
	llvm::FunctionType* function_type = llvm::FunctionType::get(
		llvm::Type::getVoidTy(context),
		{
			make_frp_type(context),
			get_generic_struct_type(interner)->getPointerTo(),
			make_runtime_type_type(context)
		},
		false
	);
	return { "fr_retain_struct", function_type, reinterpret_cast<void*>(fr_retain_struct) };
}


////////////////////////////////		fr_release_struct()


void fr_release_struct(void* floyd_runtime_ptr, STRUCT_T* v, runtime_type_t type0){
	auto& r = get_floyd_runtime(floyd_runtime_ptr);
	const auto type = lookup_type(r.type_interner.interner, type0);
	QUARK_ASSERT(type.is_struct());

	QUARK_ASSERT(v != nullptr);
	release_struct_deep(r, v, type);
}

host_func_t fr_release_struct__make(llvm::LLVMContext& context, const llvm_type_interner_t& interner){
	llvm::FunctionType* function_type = llvm::FunctionType::get(
		llvm::Type::getVoidTy(context),
		{
			make_frp_type(context),
			get_generic_struct_type(interner)->getPointerTo(),
			make_runtime_type_type(context)
		},
		false
	);
	return { "fr_release_struct", function_type, reinterpret_cast<void*>(fr_release_struct) };
}








////////////////////////////////		allocate_memory()


const uint8_t* floyd_runtime__allocate_memory(void* floyd_runtime_ptr, int64_t bytes){
	auto s = reinterpret_cast<uint8_t*>(std::calloc(1, bytes));
	return s;
}

host_func_t floyd_runtime__allocate_memory__make(llvm::LLVMContext& context, const llvm_type_interner_t& interner){
	llvm::FunctionType* function_type = llvm::FunctionType::get(
		llvm::Type::getVoidTy(context)->getPointerTo(),
		{
			make_frp_type(context),
			llvm::Type::getInt64Ty(context)
		},
		false
	);
	return { "floyd_runtime__allocate_memory", function_type, reinterpret_cast<void*>(floyd_runtime__allocate_memory) };
}




////////////////////////////////		allocate_vector()


//	Creates a new VEC_T with element_count. All elements are blank. Caller owns the result.
VEC_T* floyd_runtime__allocate_vector(void* floyd_runtime_ptr, uint64_t element_count){
	auto& r = get_floyd_runtime(floyd_runtime_ptr);

	auto v = alloc_vec(r.heap, element_count, element_count);
	return v;
}

host_func_t floyd_runtime__allocate_vector__make(llvm::LLVMContext& context, const llvm_type_interner_t& interner){
	llvm::FunctionType* function_type = llvm::FunctionType::get(
		make_vec_type(interner)->getPointerTo(),
		{
			make_frp_type(context),
			llvm::Type::getInt64Ty(context)
		},
		false
	);
	return { "floyd_runtime__allocate_vector", function_type, reinterpret_cast<void*>(floyd_runtime__allocate_vector) };
}


////////////////////////////////		allocate_string_from_strptr()


//	Creates a new VEC_T with element_count. All elements are blank. Caller owns the result.
VEC_T* fr_alloc_kstr(void* floyd_runtime_ptr, const char* s, uint64_t size){
	auto& r = get_floyd_runtime(floyd_runtime_ptr);

	const auto a = to_runtime_string(r, std::string(s, s + size));
	return a.vector_ptr;
}

host_func_t fr_alloc_kstr__make(llvm::LLVMContext& context, const llvm_type_interner_t& interner){
	llvm::FunctionType* function_type = llvm::FunctionType::get(
		make_vec_type(interner)->getPointerTo(),
		{
			make_frp_type(context),
			llvm::Type::getInt8PtrTy(context),
			llvm::Type::getInt64Ty(context)
		},
		false
	);
	return { "fr_alloc_kstr", function_type, reinterpret_cast<void*>(fr_alloc_kstr) };
}


////////////////////////////////		floyd_runtime__concatunate_vectors()


VEC_T* floyd_runtime__concatunate_vectors(void* floyd_runtime_ptr, runtime_type_t type, VEC_T* lhs, VEC_T* rhs){
	auto& r = get_floyd_runtime(floyd_runtime_ptr);
	QUARK_ASSERT(lhs != nullptr);
	QUARK_ASSERT(lhs->check_invariant());
	QUARK_ASSERT(rhs != nullptr);
	QUARK_ASSERT(rhs->check_invariant());

	const auto type0 = lookup_type(r.type_interner.interner, type);
	if(type0.is_string()){
		const auto result = from_runtime_string(r, runtime_value_t{ .vector_ptr = lhs }) + from_runtime_string(r, runtime_value_t{ .vector_ptr = rhs } );
		return to_runtime_string(r, result).vector_ptr;
	}
	else{
		auto count2 = lhs->get_element_count() + rhs->get_element_count();

		auto result = alloc_vec(r.heap, count2, count2);

		const auto element_type = type0.get_vector_element_type();

		auto dest_ptr = result->get_element_ptr();
		auto dest_ptr2 = dest_ptr + lhs->get_element_count();
		auto lhs_ptr = lhs->get_element_ptr();
		auto rhs_ptr = rhs->get_element_ptr();

		if(is_rc_value(element_type)){
			for(int i = 0 ; i < lhs->get_element_count() ; i++){
				retain_value(r, lhs_ptr[i], element_type);
				dest_ptr[i] = lhs_ptr[i];
			}
			for(int i = 0 ; i < rhs->get_element_count() ; i++){
				retain_value(r, rhs_ptr[i], element_type);
				dest_ptr2[i] = rhs_ptr[i];
			}
		}
		else{
			for(int i = 0 ; i < lhs->get_element_count() ; i++){
				dest_ptr[i] = lhs_ptr[i];
			}
			for(int i = 0 ; i < rhs->get_element_count() ; i++){
				dest_ptr2[i] = rhs_ptr[i];
			}
		}
		return result;
	}
}

host_func_t floyd_runtime__concatunate_vectors__make(llvm::LLVMContext& context, const llvm_type_interner_t& interner){
	llvm::FunctionType* function_type = llvm::FunctionType::get(
		make_vec_type(interner)->getPointerTo(),
		{
			make_frp_type(context),
			make_runtime_type_type(context),
			make_vec_type(interner)->getPointerTo(),
			make_vec_type(interner)->getPointerTo()
		},
		false
	);
	return { "floyd_runtime__concatunate_vectors", function_type, reinterpret_cast<void*>(floyd_runtime__concatunate_vectors) };
}




////////////////////////////////		allocate_dict()


void* floyd_runtime__allocate_dict(void* floyd_runtime_ptr){
	auto& r = get_floyd_runtime(floyd_runtime_ptr);

	auto v = alloc_dict(r.heap);
	return v;
}

host_func_t floyd_runtime__allocate_dict__make(llvm::LLVMContext& context, const llvm_type_interner_t& interner){
	llvm::FunctionType* function_type = llvm::FunctionType::get(
		make_dict_type(interner)->getPointerTo(),
		{
			make_frp_type(context)
		},
		false
	);
	return { "floyd_runtime__allocate_dict", function_type, reinterpret_cast<void*>(floyd_runtime__allocate_dict) };
}



////////////////////////////////		store_dict()


void floyd_runtime__store_dict_mutable(void* floyd_runtime_ptr, DICT_T* dict, runtime_value_t key, runtime_value_t element_value, runtime_type_t element_type){
	auto& r = get_floyd_runtime(floyd_runtime_ptr);

	const auto key_string = from_runtime_string(r, key);

	dict->get_map_mut().insert_or_assign(key_string, element_value);
}

host_func_t floyd_runtime__store_dict_mutable__make(llvm::LLVMContext& context, const llvm_type_interner_t& interner){
	llvm::FunctionType* function_type = llvm::FunctionType::get(
		llvm::Type::getVoidTy(context),
		{
			make_frp_type(context),
			make_dict_type(interner)->getPointerTo(),
			intern_type(interner, typeid_t::make_string()),
			make_runtime_value_type(context),
			make_runtime_type_type(context)
		},
		false
	);
	return { "floyd_runtime__store_dict_mutable", function_type, reinterpret_cast<void*>(floyd_runtime__store_dict_mutable) };
}



////////////////////////////////		lookup_dict()


runtime_value_t floyd_runtime__lookup_dict(void* floyd_runtime_ptr, DICT_T* dict, runtime_value_t s){
	auto& r = get_floyd_runtime(floyd_runtime_ptr);

	const auto& m = dict->get_map();
	const auto key_string = from_runtime_string(r, s);
	const auto it = m.find(key_string);
	if(it == m.end()){
		throw std::exception();
	}
	else{
		return it->second;
	}
}

host_func_t floyd_runtime__lookup_dict__make(llvm::LLVMContext& context, const llvm_type_interner_t& interner){
	llvm::FunctionType* function_type = llvm::FunctionType::get(
		make_runtime_value_type(context),
		{
			make_frp_type(context),
			make_dict_type(interner)->getPointerTo(),
			intern_type(interner, typeid_t::make_string())
		},
		false
	);
	return { "floyd_runtime__lookup_dict", function_type, reinterpret_cast<void*>(floyd_runtime__lookup_dict) };
}




////////////////////////////////		allocate_json()

//??? Make named types for all function-argument / return types, like: typedef int16_t* native_json_ptr

JSON_T* floyd_runtime__allocate_json(void* floyd_runtime_ptr, runtime_value_t arg0_value, runtime_type_t arg0_type){
	auto& r = get_floyd_runtime(floyd_runtime_ptr);

	const auto type0 = lookup_type(r.type_interner.interner, arg0_type);
	const auto value = from_runtime_value(r, arg0_value, type0);

	const auto a = value_to_ast_json(value, json_tags::k_plain);
	auto result = alloc_json(r.heap, a);
	return result;
}

host_func_t floyd_runtime__allocate_json__make(llvm::LLVMContext& context, const llvm_type_interner_t& interner){
	llvm::FunctionType* function_type = llvm::FunctionType::get(
		intern_type(interner, typeid_t::make_json_value()),
		{
			make_frp_type(context),
			make_runtime_value_type(context),
			make_runtime_type_type(context)
		},
		false
	);
	return { "floyd_runtime__allocate_json", function_type, reinterpret_cast<void*>(floyd_runtime__allocate_json) };
}




////////////////////////////////		lookup_json()



JSON_T* floyd_runtime__lookup_json(void* floyd_runtime_ptr, JSON_T* json_ptr, runtime_value_t arg0_value, runtime_type_t arg0_type){
	auto& r = get_floyd_runtime(floyd_runtime_ptr);

	const auto& json = json_ptr->get_json();
	const auto type0 = lookup_type(r.type_interner.interner, arg0_type);
	const auto value = from_runtime_value(r, arg0_value, type0);

	if(json.is_object()){
		if(type0.is_string() == false){
			quark::throw_runtime_error("Attempting to lookup a json-object with a key that is not a string.");
		}

		const auto result = json.get_object_element(value.get_string_value());
		return alloc_json(r.heap, result);
	}
	else if(json.is_array()){
		if(type0.is_int() == false){
			quark::throw_runtime_error("Attempting to lookup a json-object with a key that is not a number.");
		}

		const auto result = json.get_array_n(value.get_int_value());
		auto result2 = alloc_json(r.heap, result);
		return result2;
	}
	else{
		quark::throw_runtime_error("Attempting to lookup a json value -- lookup requires json-array or json-object.");
	}
}

host_func_t floyd_runtime__lookup_json__make(llvm::LLVMContext& context, const llvm_type_interner_t& interner){
	llvm::FunctionType* function_type = llvm::FunctionType::get(
		intern_type(interner, typeid_t::make_json_value()),
		{
			make_frp_type(context),
			intern_type(interner, typeid_t::make_json_value()),
			make_runtime_value_type(context),
			make_runtime_type_type(context)
		},
		false
	);
	return { "floyd_runtime__lookup_json", function_type, reinterpret_cast<void*>(floyd_runtime__lookup_json) };
}







////////////////////////////////		json_to_string()


runtime_value_t floyd_runtime__json_to_string(void* floyd_runtime_ptr, JSON_T* json_ptr){
	auto& r = get_floyd_runtime(floyd_runtime_ptr);

	const auto& json = json_ptr->get_json();

	if(json.is_string()){
		return to_runtime_string(r, json.get_string());
	}
	else{
		quark::throw_runtime_error("Attempting to assign a non-string JSON to a string.");
	}
}

host_func_t floyd_runtime__json_to_string__make(llvm::LLVMContext& context, const llvm_type_interner_t& interner){
	llvm::FunctionType* function_type = llvm::FunctionType::get(
		intern_type(interner, typeid_t::make_string()),
		{
			make_frp_type(context),
			intern_type(interner, typeid_t::make_json_value())
		},
		false
	);
	return { "floyd_runtime__json_to_string", function_type, reinterpret_cast<void*>(floyd_runtime__json_to_string) };
}






////////////////////////////////		compare_values()


int32_t floyd_runtime__compare_values(void* floyd_runtime_ptr, int64_t op, const runtime_type_t type, runtime_value_t lhs, runtime_value_t rhs){
	auto& r = get_floyd_runtime(floyd_runtime_ptr);

	const auto value_type = lookup_type(r.type_interner.interner, type);

	const auto left_value = from_runtime_value(r, lhs, value_type);
	const auto right_value = from_runtime_value(r, rhs, value_type);

	const int result = value_t::compare_value_true_deep(left_value, right_value);
//	int result = runtime_compare_value_true_deep((const uint64_t)lhs, (const uint64_t)rhs, vector_type);
	const auto op2 = static_cast<expression_type>(op);
	if(op2 == expression_type::k_comparison_smaller_or_equal__2){
		return result <= 0 ? 1 : 0;
	}
	else if(op2 == expression_type::k_comparison_smaller__2){
		return result < 0 ? 1 : 0;
	}
	else if(op2 == expression_type::k_comparison_larger_or_equal__2){
		return result >= 0 ? 1 : 0;
	}
	else if(op2 == expression_type::k_comparison_larger__2){
		return result > 0 ? 1 : 0;
	}

	else if(op2 == expression_type::k_logical_equal__2){
		return result == 0 ? 1 : 0;
	}
	else if(op2 == expression_type::k_logical_nonequal__2){
		return result != 0 ? 1 : 0;
	}
	else{
		QUARK_ASSERT(false);
		throw std::exception();
	}
}

host_func_t floyd_runtime__compare_values__make(llvm::LLVMContext& context, const llvm_type_interner_t& interner){
	llvm::FunctionType* function_type = llvm::FunctionType::get(
		llvm::Type::getInt32Ty(context),
		{
			make_frp_type(context),
			llvm::Type::getInt64Ty(context),
			make_runtime_type_type(context),
			make_runtime_value_type(context),
			make_runtime_value_type(context)
		},
		false
	);
	return { "floyd_runtime__compare_values", function_type, reinterpret_cast<void*>(floyd_runtime__compare_values) };
}


////////////////////////////////		allocate_vector()


//	Creates a new VEC_T with element_count. All elements are blank. Caller owns the result.
STRUCT_T* floyd_runtime__allocate_struct(void* floyd_runtime_ptr, uint64_t size){
	auto& r = get_floyd_runtime(floyd_runtime_ptr);

	auto v = alloc_struct(r.heap, size);
	return v;
}

host_func_t floyd_runtime__allocate_struct__make(llvm::LLVMContext& context, const llvm_type_interner_t& interner){
	llvm::FunctionType* function_type = llvm::FunctionType::get(
		get_generic_struct_type(interner)->getPointerTo(),
		{
			make_frp_type(context),
			llvm::Type::getInt64Ty(context)
		},
		false
	);
	return { "floyd_runtime__allocate_struct", function_type, reinterpret_cast<void*>(floyd_runtime__allocate_vector) };
}




std::vector<host_func_t> get_runtime_functions(llvm::LLVMContext& context, const llvm_type_interner_t& interner){
	std::vector<host_func_t> result = {
		fr_retain_vec__make(context, interner),
		fr_release_vec__make(context, interner),

		fr_retain_dict__make(context, interner),
		fr_release_dict__make(context, interner),

		fr_retain_json__make(context, interner),
		fr_release_json__make(context, interner),

		fr_retain_struct__make(context, interner),
		fr_release_struct__make(context, interner),

		floyd_runtime__allocate_memory__make(context, interner),

		floyd_runtime__allocate_vector__make(context, interner),
		fr_alloc_kstr__make(context, interner),
		floyd_runtime__concatunate_vectors__make(context, interner),

		floyd_runtime__allocate_dict__make(context, interner),
		floyd_runtime__store_dict_mutable__make(context, interner),
		floyd_runtime__lookup_dict__make(context, interner),

		floyd_runtime__allocate_json__make(context, interner),
		floyd_runtime__lookup_json__make(context, interner),

		floyd_runtime__json_to_string__make(context, interner),

		floyd_runtime__compare_values__make(context, interner),

		floyd_runtime__allocate_struct__make(context, interner)
	};
	return result;
}






struct native_binary_t {
	VEC_T* ascii40;
};

struct native_sha1_t {
	VEC_T* ascii40;
};




void floyd_funcdef__assert(void* floyd_runtime_ptr, runtime_value_t arg){
	auto& r = get_floyd_runtime(floyd_runtime_ptr);

	bool ok = arg.bool_value == 0 ? false : true;
	if(!ok){
		r._print_output.push_back("Assertion failed.");
		quark::throw_runtime_error("Floyd assertion failed.");
	}
}

STRUCT_T* floyd_funcdef__calc_binary_sha1(void* floyd_runtime_ptr, STRUCT_T* binary_ptr){
	auto& r = get_floyd_runtime(floyd_runtime_ptr);
	QUARK_ASSERT(binary_ptr != nullptr);

	const auto& binary = *reinterpret_cast<const native_binary_t*>(binary_ptr->get_data_ptr());

	const auto& s = from_runtime_string(r, runtime_value_t { .vector_ptr = binary.ascii40 });
	const auto sha1 = CalcSHA1(s);
	const auto ascii40 = SHA1ToStringPlain(sha1);

	const auto a = value_t::make_struct_value(
		typeid_t::make_struct2({ member_t{ typeid_t::make_string(), "ascii40" } }),
		{ value_t::make_string(ascii40) }
	);

	auto result = to_runtime_value(r, a);
	return result.struct_ptr;
}

STRUCT_T* floyd_funcdef__calc_string_sha1(void* floyd_runtime_ptr, runtime_value_t s0){
	auto& r = get_floyd_runtime(floyd_runtime_ptr);

	const auto& s = from_runtime_string(r, s0);
	const auto sha1 = CalcSHA1(s);
	const auto ascii40 = SHA1ToStringPlain(sha1);

	const auto a = value_t::make_struct_value(
		typeid_t::make_struct2({ member_t{ typeid_t::make_string(), "ascii40" } }),
		{ value_t::make_string(ascii40) }
	);

	auto result = to_runtime_value(r, a);
	return result.struct_ptr;
}

void floyd_funcdef__create_directory_branch(void* floyd_runtime_ptr, runtime_value_t path0){
	auto& r = get_floyd_runtime(floyd_runtime_ptr);

	const auto path = from_runtime_string(r, path0);
	if(is_valid_absolute_dir_path(path) == false){
		quark::throw_runtime_error("create_directory_branch() illegal input path.");
	}

	MakeDirectoriesDeep(path);
}

void floyd_funcdef__delete_fsentry_deep(void* floyd_runtime_ptr, runtime_value_t path0){
	auto& r = get_floyd_runtime(floyd_runtime_ptr);

	const auto path = from_runtime_string(r, path0);
	if(is_valid_absolute_dir_path(path) == false){
		quark::throw_runtime_error("delete_fsentry_deep() illegal input path.");
	}

	DeleteDeep(path);
}

uint8_t floyd_funcdef__does_fsentry_exist(void* floyd_runtime_ptr, runtime_value_t path0){
	auto& r = get_floyd_runtime(floyd_runtime_ptr);

	const auto path = from_runtime_string(r, path0);
	if(is_valid_absolute_dir_path(path) == false){
		quark::throw_runtime_error("does_fsentry_exist() illegal input path.");
	}

	bool exists = DoesEntryExist(path);
	const auto result = value_t::make_bool(exists);

#if 1
	const auto debug = value_and_type_to_ast_json(result);
	QUARK_TRACE(json_to_pretty_string(debug));
#endif
	return exists ? 0x01 : 0x00;
}



WIDE_RETURN_T floyd_host_function__erase(void* floyd_runtime_ptr, runtime_value_t arg0_value, runtime_type_t arg0_type, runtime_value_t arg1_value, runtime_type_t arg1_type){
	auto& r = get_floyd_runtime(floyd_runtime_ptr);

	const auto type0 = lookup_type(r.type_interner.interner, arg0_type);
	const auto type1 = lookup_type(r.type_interner.interner, arg1_type);

	if(type0.is_dict()){
		if(type1.is_string() == false){
			quark::throw_runtime_error("Argument 2 needs to be a string key.");
		}

		const auto& dict = unpack_dict_arg(r.type_interner.interner, arg0_value, arg0_type);

		//	Deep copy dict.
		auto dict2 = alloc_dict(r.heap);
		dict2->get_map_mut() = dict->get_map();

		const auto key_string = from_runtime_string(r, arg1_value);
		dict2->get_map_mut().erase(key_string);
		return make_wide_return_dict(dict2);
	}
	else{
		UNSUPPORTED();
	}
}

uint32_t floyd_funcdef__exists(void* floyd_runtime_ptr, runtime_value_t arg0_value, runtime_type_t arg0_type, runtime_value_t arg1_value, runtime_type_t arg1_type){
	auto& r = get_floyd_runtime(floyd_runtime_ptr);

	const auto type0 = lookup_type(r.type_interner.interner, arg0_type);
	const auto type1 = lookup_type(r.type_interner.interner, arg1_type);

	if(type0.is_dict()){
		const auto& dict = unpack_dict_arg(r.type_interner.interner, arg0_value, arg0_type);
		const auto key_string = from_runtime_string(r, arg1_value);

		const auto& m = dict->get_map();
		const auto it = m.find(key_string);
		return it != m.end() ? 1 : 0;
	}
	else{
		UNSUPPORTED();
	}
}





typedef runtime_value_t (*FILTER_F)(void* floyd_runtime_ptr, runtime_value_t element_value);


//	[E] filter([E], bool f(E e))
WIDE_RETURN_T floyd_funcdef__filter(void* floyd_runtime_ptr, runtime_value_t arg0_value, runtime_type_t arg0_type, runtime_value_t arg1_value, runtime_type_t arg1_type){
	auto& r = get_floyd_runtime(floyd_runtime_ptr);

	const auto type0 = lookup_type(r.type_interner.interner, arg0_type);
	const auto type1 = lookup_type(r.type_interner.interner, arg1_type);

	if(type0.is_vector() == false){
		quark::throw_runtime_error("filter() requires argument 1 to be a vector.");
	}
	if(type1.is_function() == false){
		quark::throw_runtime_error("filter() requires argument 2 to be a function.");
	}
	if(type1.get_function_args().size() != 1){
		quark::throw_runtime_error("filter() requires argument 2 function to take 1 argument.");
	}

	const auto& vec = *arg0_value.vector_ptr;
	const auto f = reinterpret_cast<FILTER_F>(arg1_value.function_ptr);

	auto count = vec.get_element_count();

	const auto e_element_type = type0.get_vector_element_type();

	std::vector<runtime_value_t> acc;
	for(int i = 0 ; i < count ; i++){
		const auto element_value = vec.get_element_ptr()[i];
		const auto keep = (*f)(floyd_runtime_ptr, element_value);
		if(keep.bool_value != 0){
			acc.push_back(element_value);

			if(is_rc_value(e_element_type)){
				retain_value(r, element_value, e_element_type);
			}
		}
		else{
		}
	}

	const auto count2 = (int32_t)acc.size();
	auto result_vec = alloc_vec(r.heap, count2, count2);

	if(count2 > 0){
		//	Count > 0 required to get address to first element in acc.
		copy_elements(result_vec->get_element_ptr(), &acc[0], count2);
	}
	return make_wide_return_vec(result_vec);
}
	
	


int64_t floyd_funcdef__find__string(llvm_execution_engine_t& floyd_runtime_ptr, const std::string& str, const std::string& wanted2){
	const auto r = str.find(wanted2);
	const auto result = r == std::string::npos ? -1 : static_cast<int64_t>(r);
	return result;
}

int64_t floyd_funcdef__find(void* floyd_runtime_ptr, runtime_value_t arg0_value, runtime_type_t arg0_type, const runtime_value_t arg1_value, runtime_type_t arg1_type){
	auto& r = get_floyd_runtime(floyd_runtime_ptr);

	const auto type0 = lookup_type(r.type_interner.interner, arg0_type);
	const auto type1 = lookup_type(r.type_interner.interner, arg1_type);

	if(type0.is_string()){
		if(type1.is_string() == false){
			quark::throw_runtime_error("find(string) requires argument 2 to be a string.");
		}
		return floyd_funcdef__find__string(r, from_runtime_string(r, arg0_value), from_runtime_string(r, arg1_value));
	}
	else if(type0.is_vector()){
		if(type1 != type0.get_vector_element_type()){
			quark::throw_runtime_error("find([]) requires argument 2 to be of vector's element type.");
		}

		const auto vec = unpack_vec_arg(r.type_interner.interner, arg0_value, arg0_type);

//		auto it = std::find_if(function_defs.begin(), function_defs.end(), [&] (const function_def_t& e) { return e.def_name == function_name; } );
		const auto it = std::find_if(vec->get_element_ptr(), vec->get_element_ptr() + vec->get_element_count(), [&] (const runtime_value_t& e) { return e.int_value == arg1_value.int_value; } );
//		const auto it = std::find(vec->element_ptr, vec->element_ptr + vec->element_count, arg1_value);
		if(it == vec->get_element_ptr() + vec->get_element_count()){
			return -1;
		}
		else{
			const auto pos = it - vec->get_element_ptr();
			return pos;
		}
	}
	else{
		//	No other types allowed.
		UNSUPPORTED();
	}
}



runtime_value_t floyd_funcdef__get_fs_environment(void* floyd_runtime_ptr){
	auto& r = get_floyd_runtime(floyd_runtime_ptr);

	const auto dirs = GetDirectories();

	const auto result = value_t::make_struct_value(
		make__fs_environment_t__type(),
		{
			value_t::make_string(dirs.home_dir),
			value_t::make_string(dirs.documents_dir),
			value_t::make_string(dirs.desktop_dir),

			value_t::make_string(dirs.application_support),
			value_t::make_string(dirs.preferences_dir),
			value_t::make_string(dirs.cache_dir),
			value_t::make_string(dirs.temp_dir),

			value_t::make_string(dirs.process_dir)
		}
	);

#if 1
	const auto debug = value_and_type_to_ast_json(result);
	QUARK_TRACE(json_to_pretty_string(debug));
#endif

	const auto v = to_runtime_value(r, result);
	return v;
}

VEC_T* floyd_funcdef__get_fsentries_deep(void* floyd_runtime_ptr, runtime_value_t path0){
	auto& r = get_floyd_runtime(floyd_runtime_ptr);

	const auto path = from_runtime_string(r, path0);
	if(is_valid_absolute_dir_path(path) == false){
		quark::throw_runtime_error("get_fsentries_shallow() illegal input path.");
	}

	const auto a = GetDirItemsDeep(path);
	const auto elements = directory_entries_to_values(a);
	const auto k_fsentry_t__type = make__fsentry_t__type();
	const auto vec2 = value_t::make_vector_value(k_fsentry_t__type, elements);

#if 1
	const auto debug = value_and_type_to_ast_json(vec2);
	QUARK_TRACE(json_to_pretty_string(debug));
#endif

	const auto v = to_runtime_value(r, vec2);
	return v.vector_ptr;
}


VEC_T* floyd_funcdef__get_fsentries_shallow(void* floyd_runtime_ptr, runtime_value_t path0){
	auto& r = get_floyd_runtime(floyd_runtime_ptr);

	const auto path = from_runtime_string(r, path0);
	if(is_valid_absolute_dir_path(path) == false){
		quark::throw_runtime_error("get_fsentries_shallow() illegal input path.");
	}

	const auto a = GetDirItems(path);
	const auto elements = directory_entries_to_values(a);
	const auto k_fsentry_t__type = make__fsentry_t__type();
	const auto vec2 = value_t::make_vector_value(k_fsentry_t__type, elements);

#if 1
	const auto debug = value_and_type_to_ast_json(vec2);
	QUARK_TRACE(json_to_pretty_string(debug));
#endif

	const auto v = to_runtime_value(r, vec2);
	return v.vector_ptr;
}

STRUCT_T* floyd_funcdef__get_fsentry_info(void* floyd_runtime_ptr, runtime_value_t path0){
	auto& r = get_floyd_runtime(floyd_runtime_ptr);

	const auto result = impl__get_fsentry_info(from_runtime_string(r, path0));
	const auto v = to_runtime_value(r, result);
	return v.struct_ptr;
}

int64_t floyd_host_function__get_json_type(void* floyd_runtime_ptr, JSON_T* json_ptr){
	auto& r = get_floyd_runtime(floyd_runtime_ptr);
	QUARK_ASSERT(json_ptr != nullptr);

	const auto& json = json_ptr->get_json();
	const auto result = get_json_type(json);
	return result;
}

int64_t floyd_funcdef__get_time_of_day(void* floyd_runtime_ptr){
	auto& r = get_floyd_runtime(floyd_runtime_ptr);

	std::chrono::time_point<std::chrono::high_resolution_clock> t = std::chrono::high_resolution_clock::now();
	std::chrono::duration<double> elapsed_seconds = t - r._start_time;
	const auto ms = elapsed_seconds.count() * 1000.0;
	return static_cast<int64_t>(ms);
}

runtime_value_t floyd_funcdef__jsonvalue_to_script(void* floyd_runtime_ptr, JSON_T* json_ptr){
	auto& r = get_floyd_runtime(floyd_runtime_ptr);
	QUARK_ASSERT(json_ptr != nullptr);

	const auto& json = json_ptr->get_json();

	const std::string s = json_to_compact_string(json);
	return to_runtime_string(r, s);
}

runtime_value_t floyd_funcdef__jsonvalue_to_value(void* floyd_runtime_ptr, JSON_T* json_ptr, runtime_type_t target_type){
	auto& r = get_floyd_runtime(floyd_runtime_ptr);
	QUARK_ASSERT(json_ptr != nullptr);

	const auto& json_value = json_ptr->get_json();
	const auto target_type2 = lookup_type(r.type_interner.interner, target_type);

	const auto result = unflatten_json_to_specific_type(json_value, target_type2);
	const auto result2 = to_runtime_value(r, result);
	return result2;
}


//??? No need to call lookup_type() to check which basic type it is!



	typedef WIDE_RETURN_T (*MAP_F)(void* floyd_runtime_ptr, runtime_value_t arg0_value);

WIDE_RETURN_T floyd_funcdef__map(void* floyd_runtime_ptr, runtime_value_t arg0_value, runtime_type_t arg0_type, runtime_value_t arg1_value, runtime_type_t arg1_type){
	auto& r = get_floyd_runtime(floyd_runtime_ptr);

	const auto type0 = lookup_type(r.type_interner.interner, arg0_type);
	const auto type1 = lookup_type(r.type_interner.interner, arg1_type);
	if(type0.is_vector() == false){
		quark::throw_runtime_error("map() arg 1 must be a vector.");
	}
	const auto element_type = type0.get_vector_element_type();

	if(type1.is_function() == false){
		quark::throw_runtime_error("map() requires start and end to be integers.");
	}
	const auto f_arg_types = type1.get_function_args();
	const auto r_type = type1.get_function_return();

	if(f_arg_types.size() != 1){
		quark::throw_runtime_error("map() function f requries 1 argument.");
	}

	if(f_arg_types[0] != element_type){
		quark::throw_runtime_error("map() function f must accept collection elements as its argument.");
	}

	const auto input_element_type = f_arg_types[0];
	const auto output_element_type = r_type;

	const auto f = reinterpret_cast<MAP_F>(arg1_value.function_ptr);

	auto count = arg0_value.vector_ptr->get_element_count();
	auto result_vec = alloc_vec(r.heap, count, count);
	for(int i = 0 ; i < count ; i++){
		const auto wide_result1 = (*f)(floyd_runtime_ptr, arg0_value.vector_ptr->get_element_ptr()[i]);
		result_vec->get_element_ptr()[i] = wide_result1.a;
	}
	return make_wide_return_vec(result_vec);
}





typedef runtime_value_t (*MAP_STRING_F)(void* floyd_runtime_ptr, runtime_value_t s);

runtime_value_t floyd_funcdef__map_string(void* floyd_runtime_ptr, runtime_value_t input_string0, runtime_value_t func){
	auto& r = get_floyd_runtime(floyd_runtime_ptr);

	const auto f = reinterpret_cast<MAP_STRING_F>(func.function_ptr);

	const auto input_string = from_runtime_string(r, input_string0);

	auto count = input_string.size();

	std::string acc;
	for(int i = 0 ; i < count ; i++){
		const auto element = { input_string[i] };
		const auto temp = (*f)(floyd_runtime_ptr, to_runtime_string(r, element));
		const auto temp2 = from_runtime_string(r, temp);
		acc.insert(acc.end(), temp2.begin(), temp2.end());
	}
	return to_runtime_string(r, acc);
}




void floyd_funcdef__print(void* floyd_runtime_ptr, runtime_value_t arg0_value, runtime_type_t arg0_type){
	auto& r = get_floyd_runtime(floyd_runtime_ptr);

	const auto s = gen_to_string(r, arg0_value, arg0_type);
	printf("%s\n", s.c_str());
	r._print_output.push_back(s);
}




WIDE_RETURN_T floyd_funcdef__push_back(void* floyd_runtime_ptr, runtime_value_t arg0_value, runtime_type_t arg0_type, runtime_value_t arg1_value, runtime_type_t arg1_type){
	auto& r = get_floyd_runtime(floyd_runtime_ptr);

	const auto type0 = lookup_type(r.type_interner.interner, arg0_type);
	const auto type1 = lookup_type(r.type_interner.interner, arg1_type);
	if(type0.is_string()){
		auto value = from_runtime_string(r, arg0_value);

		QUARK_ASSERT(type1.is_int());

		value.push_back((char)arg1_value.int_value);
		const auto result2 = to_runtime_string(r, value);
		return make_wide_return_1x64(result2);
	}
	else if(type0.is_vector()){
		const auto vs = unpack_vec_arg(r.type_interner.interner, arg0_value, arg0_type);

		QUARK_ASSERT(type1 == type0.get_vector_element_type());

		const auto element = arg1_value;
		const auto element_type = type1;

		auto v2 = floyd_runtime__allocate_vector(floyd_runtime_ptr, vs->get_element_count() + 1);

		auto dest_ptr = v2->get_element_ptr();
		auto source_ptr = vs->get_element_ptr();

		if(is_rc_value(element_type)){
			retain_value(r, element, element_type);

			for(int i = 0 ; i < vs->get_element_count() ; i++){
				retain_value(r, source_ptr[i], element_type);
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
		return make_wide_return_vec(v2);
	}
	else{
		//	No other types allowed.
		UNSUPPORTED();
	}
}

void floyd_host_function_1022(void* floyd_runtime_ptr, runtime_value_t arg){
	hook(__FUNCTION__, floyd_runtime_ptr, arg);
}



//		make_rec("reduce", host__reduce, 1035, typeid_t::make_function_dyn_return({ DYN, DYN, DYN }, epure::pure, typeid_t::return_dyn_type::arg1)),

	typedef runtime_value_t (*REDUCE_F)(void* floyd_runtime_ptr, runtime_value_t acc_value, runtime_value_t element_value);

//	R map([E] elements, R init, R f(R acc, E e))
WIDE_RETURN_T floyd_funcdef__reduce(void* floyd_runtime_ptr, runtime_value_t arg0_value, runtime_type_t arg0_type, runtime_value_t arg1_value, runtime_type_t arg1_type, runtime_value_t arg2_value, runtime_type_t arg2_type){
	auto& r = get_floyd_runtime(floyd_runtime_ptr);

	const auto type0 = lookup_type(r.type_interner.interner, arg0_type);
	const auto type1 = lookup_type(r.type_interner.interner, arg1_type);
	const auto type2 = lookup_type(r.type_interner.interner, arg2_type);

	if(type0.is_vector() == false || type2.is_function() == false || type2.get_function_args().size () != 2){
		quark::throw_runtime_error("reduce() parameter error.");
	}

	const auto& vec = *arg0_value.vector_ptr;
	const auto& init = arg1_value;
	const auto f = reinterpret_cast<REDUCE_F>(arg2_value.function_ptr);

	auto count = vec.get_element_count();
	runtime_value_t acc = init;
	for(int i = 0 ; i < count ; i++){
		const auto element_value = vec.get_element_ptr()[i];
		const auto acc2 = (*f)(floyd_runtime_ptr, acc, element_value);
		acc = acc2;
	}
	return make_wide_return_2x64(acc, {} );
}


void floyd_funcdef__rename_fsentry(void* floyd_runtime_ptr, runtime_value_t path0, runtime_value_t name0){
	auto& r = get_floyd_runtime(floyd_runtime_ptr);

	const auto path = from_runtime_string(r, path0);

	if(is_valid_absolute_dir_path(path) == false){
		quark::throw_runtime_error("rename_fsentry() illegal input path.");
	}
	const auto n = from_runtime_string(r, name0);
	if(n.empty()){
		quark::throw_runtime_error("rename_fsentry() illegal input name.");
	}

	RenameEntry(path, n);
}


std::string floyd_funcdef__replace__string(llvm_execution_engine_t& floyd_runtime_ptr, const std::string& s, std::size_t start, std::size_t end, const std::string& replace){
	auto s_len = s.size();
	auto replace_len = replace.size();

	auto end2 = std::min(end, s_len);
	auto start2 = std::min(start, end2);

	const std::size_t len2 = start2 + replace_len + (s_len - end2);
	auto s2 = reinterpret_cast<char*>(std::malloc(len2 + 1));
	std::memcpy(&s2[0], &s[0], start2);
	std::memcpy(&s2[start2], &replace[0], replace_len);
	std::memcpy(&s2[start2 + replace_len], &s[end2], s_len - end2);

	const std::string result(s2, &s2[start2 + replace_len + (s_len - end2)]);
	return result;
}


const WIDE_RETURN_T floyd_funcdef__replace(void* floyd_runtime_ptr, runtime_value_t arg0_value, runtime_type_t arg0_type, uint64_t start, uint64_t end, runtime_value_t arg3_value, runtime_type_t arg3_type){
	auto& r = get_floyd_runtime(floyd_runtime_ptr);

	if(start < 0 || end < 0){
		quark::throw_runtime_error("replace() requires start and end to be non-negative.");
	}
	if(start > end){
		quark::throw_runtime_error("replace() requires start <= end.");
	}

	const auto type0 = lookup_type(r.type_interner.interner, arg0_type);
	const auto type3 = lookup_type(r.type_interner.interner, arg3_type);

	if(type3 != type0){
		quark::throw_runtime_error("replace() requires argument 4 to be same type of collection.");
	}

	if(type0.is_string()){
		const auto s = from_runtime_string(r, arg0_value);
		const auto replace = from_runtime_string(r, arg3_value);
		auto ret = floyd_funcdef__replace__string(r, s, (std::size_t)start, (std::size_t)end, replace);
		const auto result2 = to_runtime_string(r, ret);
		return make_wide_return_1x64(result2);
	}
	else if(type0.is_vector()){
		const auto vec = unpack_vec_arg(r.type_interner.interner, arg0_value, arg0_type);
		const auto replace_vec = unpack_vec_arg(r.type_interner.interner, arg3_value, arg3_type);

		auto end2 = std::min(end, vec->get_element_count());
		auto start2 = std::min(start, end2);

		const auto section1_len = start2;
		const auto section2_len = replace_vec->get_element_count();
		const auto section3_len = vec->get_element_count() - end2;

		const auto len2 = section1_len + section2_len + section3_len;
		auto vec2 = alloc_vec(r.heap, len2, len2);
		copy_elements(&vec2->get_element_ptr()[0], &vec->get_element_ptr()[0], section1_len);
		copy_elements(&vec2->get_element_ptr()[section1_len], &replace_vec->get_element_ptr()[0], section2_len);
		copy_elements(&vec2->get_element_ptr()[section1_len + section2_len], &vec->get_element_ptr()[end2], section3_len);
		return make_wide_return_vec(vec2);
	}
	else{
		//	No other types allowed.
		UNSUPPORTED();
	}
}

JSON_T* floyd_funcdef__script_to_jsonvalue(void* floyd_runtime_ptr, runtime_value_t string_s0){
	auto& r = get_floyd_runtime(floyd_runtime_ptr);

	const auto string_s = from_runtime_string(r, string_s0);

	std::pair<json_t, seq_t> result0 = parse_json(seq_t(string_s));
	auto result = alloc_json(r.heap, result0.first);
	return result;
}

void floyd_funcdef__send(void* floyd_runtime_ptr, runtime_value_t process_id0, const JSON_T* message_json_ptr){
	auto& r = get_floyd_runtime(floyd_runtime_ptr);

	QUARK_ASSERT(message_json_ptr != nullptr);

	const auto& process_id = from_runtime_string(r, process_id0);
	const auto& message_json = message_json_ptr->get_json();

	QUARK_TRACE_SS("send(\"" << process_id << "\"," << json_to_pretty_string(message_json) <<")");

	r._handler->on_send(process_id, message_json);
}



int64_t floyd_funcdef__size(void* floyd_runtime_ptr, runtime_value_t arg0_value, runtime_type_t arg0_type){
	auto& r = get_floyd_runtime(floyd_runtime_ptr);

	const auto type0 = lookup_type(r.type_interner.interner, arg0_type);
	if(type0.is_string()){
		return get_vec_string_size(arg0_value);
	}
	else if(type0.is_json_value()){
		const auto& json_value = arg0_value.json_ptr->get_json();

		if(json_value.is_object()){
			return json_value.get_object_size();
		}
		else if(json_value.is_array()){
			return json_value.get_array_size();
		}
		else if(json_value.is_string()){
			return json_value.get_string().size();
		}
		else{
			quark::throw_runtime_error("Calling size() on unsupported type of value.");
		}
	}
	else if(type0.is_vector()){
		const auto vs = unpack_vec_arg(r.type_interner.interner, arg0_value, arg0_type);
		return vs->get_element_count();
	}
	else if(type0.is_dict()){
		DICT_T* dict = unpack_dict_arg(r.type_interner.interner, arg0_value, arg0_type);
		return dict->size();
	}
	else{
		//	No other types allowed.
		UNSUPPORTED();
	}
}

const WIDE_RETURN_T floyd_funcdef__subset(void* floyd_runtime_ptr, runtime_value_t arg0_value, runtime_type_t arg0_type, uint64_t start, uint64_t end){
	auto& r = get_floyd_runtime(floyd_runtime_ptr);

	if(start < 0 || end < 0){
		quark::throw_runtime_error("subset() requires start and end to be non-negative.");
	}

	const auto type0 = lookup_type(r.type_interner.interner, arg0_type);
	if(type0.is_string()){
		const auto value = from_runtime_string(r, arg0_value);

		const auto len = get_vec_string_size(arg0_value);
		const auto end2 = std::min(end, len);
		const auto start2 = std::min(start, end2);
		const auto len2 = end2 - start2;

		const auto s = std::string(&value[start2], &value[start2 + len2]);
		const auto result = to_runtime_string(r, s);
		return make_wide_return_1x64(result);
	}
	else if(type0.is_vector()){
		const auto vec = unpack_vec_arg(r.type_interner.interner, arg0_value, arg0_type);

		const auto end2 = std::min(end, vec->get_element_count());
		const auto start2 = std::min(start, end2);
		const auto len2 = end2 - start2;

		if(len2 >= INT32_MAX){
			throw std::exception();
		}
		VEC_T* vec2 = alloc_vec(r.heap, len2, len2);
		for(int i = 0 ; i < len2 ; i++){
			vec2->get_element_ptr()[i] = vec->get_element_ptr()[start2 + i];
		}
		return make_wide_return_vec(vec2);
	}
	else{
		//	No other types allowed.
		UNSUPPORTED();
	}
}


typedef WIDE_RETURN_T (*SUPERMAP_F)(void* floyd_runtime_ptr, runtime_value_t arg0_value, runtime_value_t arg1_value);

WIDE_RETURN_T floyd_funcdef__supermap(
	void* floyd_runtime_ptr,
	runtime_value_t arg0_value,
	runtime_type_t arg0_type,
	runtime_value_t arg1_value,
	runtime_type_t arg1_type,
	runtime_value_t arg2_value,
	runtime_type_t arg2_type
){
	auto& r = get_floyd_runtime(floyd_runtime_ptr);

	const auto type0 = lookup_type(r.type_interner.interner, arg0_type);
	const auto type1 = lookup_type(r.type_interner.interner, arg1_type);
	const auto type2 = lookup_type(r.type_interner.interner, arg2_type);

	//	Check topology.
	if(type0.is_vector() && type1 == typeid_t::make_vector(typeid_t::make_int()) && type2.is_function() && type2.get_function_args().size () == 2){
	}
	else{
		quark::throw_runtime_error("supermap() arguments are wrong.");
	}



	const auto& elements = arg0_value;
	const auto& e_type = type0.get_vector_element_type();
	const auto& parents = arg1_value;
	const auto& f = arg2_value;
	const auto& r_type = type2.get_function_return();
	if(
		e_type == type2.get_function_args()[0]
		&& r_type == type2.get_function_args()[1].get_vector_element_type()
	){
	}
	else {
		quark::throw_runtime_error("R supermap([E] elements, R init_value, R (R acc, E element) f");
	}

	const auto f2 = reinterpret_cast<SUPERMAP_F>(f.function_ptr);

	const auto elements2 = elements.vector_ptr;
	const auto parents2 = parents.vector_ptr;

	if(elements2->get_element_count() != parents2->get_element_count()) {
		quark::throw_runtime_error("supermap() requires elements and parents be the same count.");
	}

	auto elements_todo = elements2->get_element_count();
	std::vector<int> rcs(elements2->get_element_count(), 0);

	std::vector<runtime_value_t> complete(elements2->get_element_count(), runtime_value_t());

	for(int i = 0 ; i < parents2->get_element_count() ; i++){
		const auto& e = parents2->get_element_ptr()[i];
		const auto parent_index = e.int_value;

		const auto count = static_cast<int64_t>(elements2->get_element_count());
		QUARK_ASSERT(parent_index >= -1);
		QUARK_ASSERT(parent_index < count);

		if(parent_index != -1){
			rcs[parent_index]++;
		}
	}

	while(elements_todo > 0){
		std::vector<int> pass_ids;
		for(int i = 0 ; i < elements2->get_element_count() ; i++){
			const auto rc = rcs[i];
			if(rc == 0){
				pass_ids.push_back(i);
				rcs[i] = -1;
			}
		}

		if(pass_ids.empty()){
			quark::throw_runtime_error("supermap() dependency cycle error.");
		}

		for(const auto element_index: pass_ids){
			const auto& e = elements2->get_element_ptr()[element_index];

			//	Make list of the element's inputs -- the must all be complete now.
			std::vector<runtime_value_t> solved_deps;
			for(int element_index2 = 0 ; element_index2 < parents2->get_element_count() ; element_index2++){
				const auto& p = parents2->get_element_ptr()[element_index2];
				const auto parent_index = p.int_value;
				if(parent_index == element_index){
					QUARK_ASSERT(element_index2 != -1);
					QUARK_ASSERT(element_index2 >= -1 && element_index2 < elements2->get_element_count());
					QUARK_ASSERT(rcs[element_index2] == -1);
					const auto& solved = complete[element_index2];
					solved_deps.push_back(solved);
				}
			}

			auto solved_deps2 = alloc_vec(r.heap, solved_deps.size(), solved_deps.size());
			for(int i = 0 ; i < solved_deps.size() ; i++){
				solved_deps2->get_element_ptr()[i] = solved_deps[i];
			}
			runtime_value_t solved_deps3 { .vector_ptr = solved_deps2 };

			const auto wide_result = (*f2)(floyd_runtime_ptr, e, solved_deps3);
			const auto result1 = wide_result.a;

			const auto parent_index = parents2->get_element_ptr()[element_index].int_value;
			if(parent_index != -1){
				rcs[parent_index]--;
			}
			complete[element_index] = result1;
			elements_todo--;
		}
	}

//	const auto result = make_vector(r_type, complete);

	const auto count = complete.size();
	auto result_vec = alloc_vec(r.heap, count, count);
	for(int i = 0 ; i < count ; i++){
		result_vec->get_element_ptr()[i] = complete[i];
	}

#if 1
	const auto vec_r = runtime_value_t{ .vector_ptr = result_vec };
	const auto result_value = from_runtime_value(r, vec_r, typeid_t::make_vector(r_type));
	const auto debug = value_and_type_to_ast_json(result_value);
	QUARK_TRACE(json_to_pretty_string(debug));
#endif

	return make_wide_return_vec(result_vec);

/*
	const auto element_type = type0.get_vector_element_type();

	if(type1.is_function() == false){
		quark::throw_runtime_error("map() requires start and end to be integers.");
	}
	const auto f_arg_types = type1.get_function_args();
	const auto r_type = type1.get_function_return();

	if(f_arg_types.size() != 1){
		quark::throw_runtime_error("map() function f requries 1 argument.");
	}

	if(f_arg_types[0] != element_type){
		quark::throw_runtime_error("map() function f must accept collection elements as its argument.");
	}

	const auto input_element_type = f_arg_types[0];
	const auto output_element_type = r_type;

	const auto f = reinterpret_cast<MAP_F>(arg1_value.function_ptr);

	auto count = arg0_value.vector_ptr->get_element_count();
	auto result_vec = new VEC_T(make_vec(count));
	for(int i = 0 ; i < count ; i++){
		const auto wide_result1 = (*f)(floyd_runtime_ptr, arg0_value.vector_ptr->get_element_ptr()[i]);
		result_vec->get_element_ptr()[i] = wide_result1.a;
	}
	return make_wide_return_vec(result_vec);
*/
}

runtime_value_t floyd_funcdef__to_pretty_string(void* floyd_runtime_ptr, runtime_value_t arg0_value, runtime_type_t arg0_type){
	auto& r = get_floyd_runtime(floyd_runtime_ptr);

	const auto type0 = lookup_type(r.type_interner.interner, arg0_type);
	const auto& value = from_runtime_value(r, arg0_value, type0);
	const auto json = value_to_ast_json(value, json_tags::k_plain);
	const auto s = json_to_pretty_string(json, 0, pretty_t{ 80, 4 });
	return to_runtime_string(r, s);
}

runtime_value_t floyd_host__to_string(void* floyd_runtime_ptr, runtime_value_t arg0_value, runtime_type_t arg0_type){
	auto& r = get_floyd_runtime(floyd_runtime_ptr);

	const auto s = gen_to_string(r, arg0_value, arg0_type);
	return to_runtime_string(r, s);
}



//		make_rec("typeof", host__typeof, 1004, typeid_t::make_function(typeid_t::make_typeid(), { DYN }, epure::pure)),

int32_t floyd_host__typeof(void* floyd_runtime_ptr, runtime_value_t arg0_value, runtime_type_t arg0_type){
	auto& r = get_floyd_runtime(floyd_runtime_ptr);

#if DEBUG
	const auto type0 = lookup_type(r.type_interner.interner, arg0_type);
	QUARK_ASSERT(type0.check_invariant());
#endif
	return arg0_type;
}






const WIDE_RETURN_T floyd_funcdef__update(void* floyd_runtime_ptr, runtime_value_t arg0_value, runtime_type_t arg0_type, runtime_value_t arg1_value, runtime_type_t arg1_type, runtime_value_t arg2_value, runtime_type_t arg2_type){
	auto& r = get_floyd_runtime(floyd_runtime_ptr);

	auto& context = r.instance->context;

	const auto type0 = lookup_type(r.type_interner.interner, arg0_type);
	const auto type1 = lookup_type(r.type_interner.interner, arg1_type);
	const auto type2 = lookup_type(r.type_interner.interner, arg2_type);
	if(type0.is_string()){
		if(type1.is_int() == false){
			throw std::exception();
		}
		if(type2.is_int() == false){
			throw std::exception();
		}

		const auto str = from_runtime_string(r, arg0_value);
		const auto index = arg1_value.int_value;
		const auto new_char = (char)arg2_value.int_value;

		const auto len = str.size();

		if(index < 0 || index >= len){
			throw std::runtime_error("Position argument to update() is outside collection span.");
		}

		auto result = str;
		result[index] = new_char;
		const auto result2 = to_runtime_string(r, result);
		return make_wide_return_1x64(result2);
	}
	else if(type0.is_vector()){
		if(type1.is_int() == false){
			throw std::exception();
		}

		const auto vec = unpack_vec_arg(r.type_interner.interner, arg0_value, arg0_type);
		const auto element_type = type0.get_vector_element_type();
		const auto index = arg1_value.int_value;

		if(element_type != type2){
			throw std::runtime_error("New value's type must match vector's element type.");
		}

		if(index < 0 || index >= vec->get_element_count()){
			throw std::runtime_error("Position argument to update() is outside collection span.");
		}

		auto result = alloc_vec(r.heap, vec->get_element_count(), vec->get_element_count());
		auto dest_ptr = result->get_element_ptr();
		auto source_ptr = vec->get_element_ptr();
		if(is_rc_value(element_type)){
			retain_value(r, arg2_value, element_type);
			for(int i = 0 ; i < result->get_element_count() ; i++){
				retain_value(r, source_ptr[i], element_type);
				dest_ptr[i] = source_ptr[i];
			}

			release_vec_deep(r, dest_ptr[index].vector_ptr, element_type);
			dest_ptr[index] = arg2_value;
		}
		else{
			for(int i = 0 ; i < result->get_element_count() ; i++){
				dest_ptr[i] = source_ptr[i];
			}
			dest_ptr[index] = arg2_value;
		}

		return make_wide_return_vec(result);
	}
	else if(type0.is_dict()){
		if(type1.is_string() == false){
			throw std::exception();
		}
		const auto key = from_runtime_string(r, arg1_value);
		const auto dict = unpack_dict_arg(r.type_interner.interner, arg0_value, arg0_type);
		const auto value_type = type0.get_dict_value_type();

		//	Deep copy dict.
		auto dict2 = alloc_dict(r.heap);
		dict2->get_map_mut() = dict->get_map();

		dict2->get_map_mut().insert_or_assign(key, arg2_value);

		if(is_rc_value(value_type)){
			for(const auto& e: dict2->get_map()){
				retain_value(r, e.second, value_type);
			}
		}


		return make_wide_return_dict(dict2);
	}
	else if(type0.is_struct()){
		if(type1.is_string() == false){
			throw std::exception();
		}
		if(type2.is_void() == true){
			throw std::exception();
		}

		const auto source_struct_ptr = arg0_value.struct_ptr;

		const auto member_name = from_runtime_string(r, arg1_value);
		if(member_name == ""){
			throw std::runtime_error("Must provide name of struct member to update().");
		}

		const auto& struct_def = type0.get_struct();
		int member_index = find_struct_member_index(struct_def, member_name);
		if(member_index == -1){
			throw std::runtime_error("Position argument to update() is outside collection span.");
		}

		const auto member_value = from_runtime_value(r, arg2_value, type2);

		//	Make copy of struct, overwrite member in copy.
 
		auto& struct_type_llvm = *get_exact_struct_type(r.type_interner, type0);

		const llvm::DataLayout& data_layout = r.ee->getDataLayout();
		const llvm::StructLayout* layout = data_layout.getStructLayout(&struct_type_llvm);
		const auto struct_bytes = layout->getSizeInBytes();

		//??? Touches memory twice.
		auto struct_ptr = alloc_struct(r.heap, struct_bytes);
		auto struct_base_ptr = struct_ptr->get_data_ptr();
		std::memcpy(struct_base_ptr, source_struct_ptr->get_data_ptr(), struct_bytes);

		const auto member_offset = layout->getElementOffset(member_index);
		const auto member_ptr = reinterpret_cast<void*>(struct_base_ptr + member_offset);

		const auto member_type = struct_def._members[member_index]._type;

		if(type2 != member_type){
			throw std::runtime_error("New value must be same type as struct member's type.");
		}
		store_via_ptr(r, member_type, member_ptr, member_value);

		//	Retain every member of new struct.
		{
			int member_index = 0;
			for(const auto& e: struct_def._members){
				if(is_rc_value(e._type)){
					const auto offset = layout->getElementOffset(member_index);
					const auto member_ptr = reinterpret_cast<const runtime_value_t*>(struct_base_ptr + offset);
					retain_value(r, *member_ptr, e._type);
					member_index++;
				}
			}
		}

		return make_wide_return_structptr(struct_ptr);
	}
	else{
		//	No other types allowed.
		UNSUPPORTED();
	}
}

JSON_T* floyd_funcdef__value_to_jsonvalue(void* floyd_runtime_ptr, runtime_value_t arg0_value, runtime_type_t arg0_type){
	auto& r = get_floyd_runtime(floyd_runtime_ptr);

	const auto type0 = lookup_type(r.type_interner.interner, arg0_type);
	const auto value0 = from_runtime_value(r, arg0_value, type0);
	const auto j = value_to_ast_json(value0, json_tags::k_plain);
	auto result = alloc_json(r.heap, j);
	return result;
}



	static void write_text_file(const std::string& abs_path, const std::string& data){
		const auto up = UpDir2(abs_path);

		MakeDirectoriesDeep(up.first);

		std::ofstream outputFile;
		outputFile.open(abs_path);
		if (outputFile.fail()) {
			quark::throw_exception();
		}

		outputFile << data;
		outputFile.close();
	}

void floyd_funcdef__write_text_file(void* floyd_runtime_ptr, runtime_value_t path0, runtime_value_t data0){
	auto& r = get_floyd_runtime(floyd_runtime_ptr);

	const auto path = from_runtime_string(r, path0);
	const auto file_contents = from_runtime_string(r, data0);
	write_text_file(path, file_contents);
}

void floyd_host_function_1037(void* floyd_runtime_ptr, runtime_value_t arg){
	hook(__FUNCTION__, floyd_runtime_ptr, arg);
}

void floyd_host_function_1038(void* floyd_runtime_ptr, runtime_value_t arg){
	hook(__FUNCTION__, floyd_runtime_ptr, arg);
}

void floyd_host_function_1039(void* floyd_runtime_ptr, runtime_value_t arg){
	hook(__FUNCTION__, floyd_runtime_ptr, arg);
}




///////////////		TEST

void floyd_host_function_2002(void* floyd_runtime_ptr, runtime_value_t arg){
	hook(__FUNCTION__, floyd_runtime_ptr, arg);
}

void floyd_host_function_2003(void* floyd_runtime_ptr, runtime_value_t arg){
	hook(__FUNCTION__, floyd_runtime_ptr, arg);
}



std::map<std::string, void*> get_host_functions_map2(){

	////////////////////////////////		CORE FUNCTIONS AND HOST FUNCTIONS
	const std::map<std::string, void*> host_functions_map = {
		{ "floyd_funcdef__assert", reinterpret_cast<void *>(&floyd_funcdef__assert) },
		{ "floyd_funcdef__calc_binary_sha1", reinterpret_cast<void *>(&floyd_funcdef__calc_binary_sha1) },
		{ "floyd_funcdef__calc_string_sha1", reinterpret_cast<void *>(&floyd_funcdef__calc_string_sha1) },
		{ "floyd_funcdef__create_directory_branch", reinterpret_cast<void *>(&floyd_funcdef__create_directory_branch) },
		{ "floyd_funcdef__delete_fsentry_deep", reinterpret_cast<void *>(&floyd_funcdef__delete_fsentry_deep) },
		{ "floyd_funcdef__does_fsentry_exist", reinterpret_cast<void *>(&floyd_funcdef__does_fsentry_exist) },
		{ "floyd_funcdef__erase", reinterpret_cast<void *>(&floyd_host_function__erase) },
		{ "floyd_funcdef__exists", reinterpret_cast<void *>(&floyd_funcdef__exists) },
		{ "floyd_funcdef__filter", reinterpret_cast<void *>(&floyd_funcdef__filter) },
		{ "floyd_funcdef__find", reinterpret_cast<void *>(&floyd_funcdef__find) },

		{ "floyd_funcdef__get_fs_environment", reinterpret_cast<void *>(&floyd_funcdef__get_fs_environment) },
		{ "floyd_funcdef__get_fsentries_deep", reinterpret_cast<void *>(&floyd_funcdef__get_fsentries_deep) },
		{ "floyd_funcdef__get_fsentries_shallow", reinterpret_cast<void *>(&floyd_funcdef__get_fsentries_shallow) },
		{ "floyd_funcdef__get_fsentry_info", reinterpret_cast<void *>(&floyd_funcdef__get_fsentry_info) },
		{ "floyd_funcdef__get_json_type", reinterpret_cast<void *>(&floyd_host_function__get_json_type) },
		{ "floyd_funcdef__get_time_of_day", reinterpret_cast<void *>(&floyd_funcdef__get_time_of_day) },
		{ "floyd_funcdef__jsonvalue_to_script", reinterpret_cast<void *>(&floyd_funcdef__jsonvalue_to_script) },
		{ "floyd_funcdef__jsonvalue_to_value", reinterpret_cast<void *>(&floyd_funcdef__jsonvalue_to_value) },
		{ "floyd_funcdef__map", reinterpret_cast<void *>(&floyd_funcdef__map) },
		{ "floyd_funcdef__map_string", reinterpret_cast<void *>(&floyd_funcdef__map_string) },

		{ "floyd_funcdef__print", reinterpret_cast<void *>(&floyd_funcdef__print) },
		{ "floyd_funcdef__push_back", reinterpret_cast<void *>(&floyd_funcdef__push_back) },
		{ "floyd_funcdef__read_text_file", reinterpret_cast<void *>(&floyd_host_function_1022) },
		{ "floyd_funcdef__reduce", reinterpret_cast<void *>(&floyd_funcdef__reduce) },
		{ "floyd_funcdef__rename_fsentry", reinterpret_cast<void *>(&floyd_funcdef__rename_fsentry) },
		{ "floyd_funcdef__replace", reinterpret_cast<void *>(&floyd_funcdef__replace) },
		{ "floyd_funcdef__script_to_jsonvalue", reinterpret_cast<void *>(&floyd_funcdef__script_to_jsonvalue) },
		{ "floyd_funcdef__send", reinterpret_cast<void *>(&floyd_funcdef__send) },
		{ "floyd_funcdef__size", reinterpret_cast<void *>(&floyd_funcdef__size) },
		{ "floyd_funcdef__subset", reinterpret_cast<void *>(&floyd_funcdef__subset) },

		{ "floyd_funcdef__supermap", reinterpret_cast<void *>(&floyd_funcdef__supermap) },
		{ "floyd_funcdef__to_pretty_string", reinterpret_cast<void *>(&floyd_funcdef__to_pretty_string) },
		{ "floyd_funcdef__to_string", reinterpret_cast<void *>(&floyd_host__to_string) },
		{ "floyd_funcdef__typeof", reinterpret_cast<void *>(&floyd_host__typeof) },
		{ "floyd_funcdef__update", reinterpret_cast<void *>(&floyd_funcdef__update) },
		{ "floyd_funcdef__value_to_jsonvalue", reinterpret_cast<void *>(&floyd_funcdef__value_to_jsonvalue) },
		{ "floyd_funcdef__write_text_file", reinterpret_cast<void *>(&floyd_funcdef__write_text_file) }
	};
	return host_functions_map;
}

uint64_t call_floyd_runtime_init(llvm_execution_engine_t& ee){
	QUARK_ASSERT(ee.check_invariant());

	auto a_func = reinterpret_cast<FLOYD_RUNTIME_INIT>(get_global_function(ee, "floyd_runtime_init"));
	QUARK_ASSERT(a_func != nullptr);

	int64_t a_result = (*a_func)((void*)&ee);
	QUARK_ASSERT(a_result == 667);
	return a_result;
}

uint64_t call_floyd_runtime_deinit(llvm_execution_engine_t& ee){
	QUARK_ASSERT(ee.check_invariant());

	auto a_func = reinterpret_cast<FLOYD_RUNTIME_INIT>(get_global_function(ee, "floyd_runtime_deinit"));
	QUARK_ASSERT(a_func != nullptr);

	int64_t a_result = (*a_func)((void*)&ee);
	QUARK_ASSERT(a_result == 668);
	return a_result;
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
        QUARK_TRACE("Notifying...");
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

		auto f = *reinterpret_cast<FLOYD_RUNTIME_PROCESS_INIT*>(process._init_function->address);
		const auto result = (*f)(runtime.ee);
		process._process_state = from_runtime_value(*runtime.ee, result, process._init_function->type.get_function_return());
	}

	while(stop == false){
		json_t message;
		{
			std::unique_lock<std::mutex> lk(process._inbox_mutex);

        	QUARK_TRACE_SS(thread_name << ": waiting......");
			process._inbox_condition_variable.wait(lk, [&]{ return process._inbox.empty() == false; });
        	QUARK_TRACE_SS(thread_name << ": continue");

			//	Pop message.
			QUARK_ASSERT(process._inbox.empty() == false);
			message = process._inbox.back();
			process._inbox.pop_back();
		}
		QUARK_TRACE_SS("RECEIVED: " << json_to_pretty_string(message));

		if(message.is_string() && message.get_string() == "stop"){
			stop = true;
        	QUARK_TRACE_SS(thread_name << ": STOP");
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

				auto f = *reinterpret_cast<FLOYD_RUNTIME_PROCESS_MESSAGE*>(process._process_function->address);
				const auto state2 = to_runtime_value(*runtime.ee, process._process_state);
				const auto message2 = to_runtime_value(*runtime.ee, value_t::make_json_value(message));
				const auto result = (*f)(runtime.ee, state2, message2);
				process._process_state = from_runtime_value(*runtime.ee, result, process._process_function->type.get_function_return());
			}
		}
	}
}


static std::map<std::string, value_t> run_container_int(llvm_ir_program_t& program_breaks, const std::vector<std::string>& main_args, const std::string& container_key){
	QUARK_ASSERT(container_key.empty() == false);

	llvm_execution_engine_t ee = make_engine_run_init(*program_breaks.instance, program_breaks);

	llvm_process_runtime_t runtime;
	runtime._main_thread_id = std::this_thread::get_id();
	runtime.ee = &ee;

	//??? Confusing. Support several containers!
	if(std::find(program_breaks.software_system._containers.begin(), program_breaks.software_system._containers.end(), container_key) == program_breaks.software_system._containers.end()){
		quark::throw_runtime_error("Unknown container-key");
	}

	if(program_breaks.container_def._name != container_key){
		quark::throw_runtime_error("Unknown container-key");
	}

	runtime._container = program_breaks.container_def;

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

		process->_init_function = std::make_shared<llvm_bind_t>(bind_function2(*runtime.ee, t.second + "__init"));
		process->_process_function = std::make_shared<llvm_bind_t>(bind_function2(*runtime.ee, t.second));

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

	call_floyd_runtime_deinit(ee);

	return {};
}



std::map<std::string, value_t> run_llvm_container(llvm_ir_program_t& program_breaks, const std::vector<std::string>& main_args, const std::string& container_key){
	if(container_key.empty()){
		// ??? instance is already known via program_breaks.
		llvm_execution_engine_t ee = make_engine_run_init(*program_breaks.instance, program_breaks);

		const auto main_function = bind_function(ee, "main");
		if(main_function.first != nullptr){
			const auto main_result_int = llvm_call_main(ee, main_function, main_args);
			const auto result = value_t::make_int(main_result_int);

			call_floyd_runtime_deinit(ee);

			detect_leaks(ee.heap);

			return {{ "main()", result }};
		}
		else{
			call_floyd_runtime_deinit(ee);
			return {{ "global", value_t::make_void() }};
		}
	}
	else{
		return run_container_int(program_breaks, main_args, container_key);
	}
}




#if DEBUG && 1
//	Verify that all global functions can be accessed. If *one* is unresolved, then all return NULL!?
void check_nulls(llvm_execution_engine_t& ee2, const llvm_ir_program_t& p){
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

static std::map<std::string, void*> register_c_functions(llvm::LLVMContext& context, const llvm_type_interner_t& interner){
	const auto runtime_functions = get_runtime_functions(context, interner);

	std::map<std::string, void*> runtime_functions_map;
	for(const auto& e: runtime_functions){
		const auto e2 = std::pair<std::string, void*>(e.name_key, e.implementation_f);
		runtime_functions_map.insert(e2);
	}
	const auto host_functions_map = get_host_functions_map2();

	std::map<std::string, void*> function_map = runtime_functions_map;
	function_map.insert(host_functions_map.begin(), host_functions_map.end());

	return function_map;
}


//??? Move init functions to runtime source file.
//	Destroys program, can only run it once!
static llvm_execution_engine_t make_engine_no_init(llvm_instance_t& instance, llvm_ir_program_t& program_breaks){
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
	auto ee2 = llvm_execution_engine_t{
		k_debug_magic,
		&instance,
		ee1,
		program_breaks.type_interner,
		program_breaks.debug_globals,
		program_breaks.function_defs,
		{},
		nullptr,
		start_time
	};
	QUARK_ASSERT(ee2.check_invariant());

	auto function_map = register_c_functions(instance.context, program_breaks.type_interner);

	//	Resolve all unresolved functions.
	{
		//	https://stackoverflow.com/questions/33328562/add-mapping-to-c-lambda-from-llvm
		auto lambda = [&](const std::string& s) -> void* {
			QUARK_ASSERT(s.empty() == false);
			QUARK_ASSERT(s[0] == '_');
			const auto s2 = s.substr(1);

			const auto it = function_map.find(s2);
			if(it != function_map.end()){
				return it->second;
			}
			else{
			}

			return nullptr;
		};
		std::function<void*(const std::string&)> on_lazy_function_creator2 = lambda;

		//	NOTICE! Patch during finalizeObject() only, then restore!
		ee2.ee->InstallLazyFunctionCreator(on_lazy_function_creator2);
		ee2.ee->finalizeObject();
		ee2.ee->InstallLazyFunctionCreator(nullptr);

	//	ee2.ee->DisableGVCompilation(false);
	//	ee2.ee->DisableSymbolSearching(false);
	}

	check_nulls(ee2, program_breaks);

//	llvm::WriteBitcodeToFile(exeEng->getVerifyModules(), raw_ostream &Out);
	return ee2;
}

//	Destroys program, can only run it once!
//	Automatically runs floyd_runtime_init() to execute Floyd's global functions and initialize global constants.
llvm_execution_engine_t make_engine_run_init(llvm_instance_t& instance, llvm_ir_program_t& program_breaks){
	QUARK_ASSERT(instance.check_invariant());
	QUARK_ASSERT(program_breaks.check_invariant());

	llvm_execution_engine_t ee = make_engine_no_init(instance, program_breaks);

	trace_heap(ee.heap);

#if DEBUG
	{
		const auto print_global_ptr = (FLOYD_RUNTIME_HOST_FUNCTION*)floyd::get_global_ptr(ee, "print");
		QUARK_ASSERT(print_global_ptr != nullptr);

		const auto print_f = *print_global_ptr;
		QUARK_ASSERT(print_f != nullptr);
		if(print_f){

//			(*print_f)(&ee, 109);
		}
	}

	{
		auto a_func = reinterpret_cast<FLOYD_RUNTIME_INIT>(get_global_function(ee, "floyd_runtime_init"));
		QUARK_ASSERT(a_func != nullptr);
	}
#endif

	const auto init_result = call_floyd_runtime_init(ee);
	QUARK_ASSERT(init_result == 667);

//	check_nulls(ee, program_breaks);

	trace_heap(ee.heap);

	return ee;
}

}	//	namespace floyd



////////////////////////////////		TESTS



QUARK_UNIT_TEST("", "From source: Check that floyd_runtime_init() runs and sets 'result' global", "", ""){
	const auto cu = floyd::make_compilation_unit_nolib("let int result = 1 + 2 + 3", "myfile.floyd");
	const auto pass3 = compile_to_sematic_ast__errors(cu);

	floyd::llvm_instance_t instance;
	auto program = generate_llvm_ir_program(instance, pass3, "myfile.floyd");
	auto ee = make_engine_run_init(instance, *program);

	const auto result = *static_cast<uint64_t*>(floyd::get_global_ptr(ee, "result"));
	QUARK_ASSERT(result == 6);

	call_floyd_runtime_deinit(ee);

//	QUARK_TRACE_SS("result = " << floyd::print_program(*program));
}

//	BROKEN!
QUARK_UNIT_TEST("", "From JSON: Simple function call, call print() from floyd_runtime_init()", "", ""){
	const auto cu = floyd::make_compilation_unit_nolib("print(5)", "myfile.floyd");
	const auto pass3 = compile_to_sematic_ast__errors(cu);

	floyd::llvm_instance_t instance;
	auto program = generate_llvm_ir_program(instance, pass3, "myfile.floyd");
	auto ee = make_engine_run_init(instance, *program);
	QUARK_ASSERT(ee._print_output == std::vector<std::string>{"5"});
	call_floyd_runtime_deinit(ee);
}


