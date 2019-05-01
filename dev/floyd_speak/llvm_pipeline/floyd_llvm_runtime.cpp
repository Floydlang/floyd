//
//  floyd_llvm_runtime.cpp
//  floyd_speak
//
//  Created by Marcus Zetterquist on 2019-04-24.
//  Copyright © 2019 Marcus Zetterquist. All rights reserved.
//

#include "floyd_llvm_runtime.h"

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


namespace floyd {



//	The names of these are computed from the host-id in the symbol table, not the names of the functions/symbols.
//	They must use C calling convention so llvm JIT can find them.
//	Make sure they are not dead-stripped out of binary!
void floyd_runtime__unresolved_func(void* floyd_runtime_ptr);

llvm_execution_engine_t& get_floyd_runtime(void* floyd_runtime_ptr);

value_t runtime_llvm_to_value(const llvm_execution_engine_t& runtime, const encoded_native_value_t encoded_value, const typeid_t& type);

value_t llvm_global_to_value(const llvm_execution_engine_t& runtime, const void* global_ptr, const typeid_t& type);
value_t llvm_valueptr_to_value(const llvm_execution_engine_t& runtime, const void* value_ptr, const typeid_t& type);


const function_def_t& find_function_def2(const std::vector<function_def_t>& function_defs, const std::string& function_name){
	auto it = std::find_if(function_defs.begin(), function_defs.end(), [&] (const function_def_t& e) { return e.def_name == function_name; } );
	QUARK_ASSERT(it != function_defs.end());

	QUARK_ASSERT(it->llvm_f != nullptr);
	return *it;
}




//////////////////////////////////////////		COMPARE



struct runtime_external_handle_t {
};


int runtime_compare_value_true_deep(const llvm_execution_engine_t& runtime, const value_t& left, const value_t& right, const typeid_t& type0);


static int compare(int64_t value){
	if(value < 0){
		return -1;
	}
	else if(value > 0){
		return 1;
	}
	else{
		return 0;
	}
}

int runtime_compare_string(const std::string& left, const std::string& right){
	// ??? Better if it doesn't use c_ptr since that is non-pure string handling.
	return compare(std::strcmp(left.c_str(), right.c_str()));
}

QUARK_UNIT_TEST("runtime_compare_string()", "", "", ""){
	ut_verify_auto(QUARK_POS, runtime_compare_string("", ""), 0);
}
QUARK_UNIT_TEST("runtime_compare_string()", "", "", ""){
	ut_verify_auto(QUARK_POS, runtime_compare_string("aaa", "aaa"), 0);
}
QUARK_UNIT_TEST("runtime_compare_string()", "", "", ""){
	ut_verify_auto(QUARK_POS, runtime_compare_string("b", "a"), 1);
}


int runtime_compare_struct_true_deep(const llvm_execution_engine_t& runtime, const struct_value_t& left, const struct_value_t& right, const typeid_t& type){
	const auto& struct_def = type.get_struct();

	for(int i = 0 ; i < struct_def._members.size() ; i++){
		const auto& member_type = struct_def._members[i]._type;
		int diff = runtime_compare_value_true_deep(runtime, left._member_values[i], right._member_values[i], member_type);
		if(diff != 0){
			return diff;
		}
	}
	return 0;
}

/*
int runtime_compare_vectors_obj(const std::vector<runtime_external_handle_t>& left, const std::vector<runtime_external_handle_t>& right, const typeid_t& type){
	QUARK_ASSERT(type.is_vector());

	const auto shared_count = std::min(left.size(), right.size());
	const auto& element_type = typeid_t(type.get_vector_element_type());
	for(int i = 0 ; i < shared_count ; i++){
		const auto element_result = runtime_compare_value_true_deep(bc_value_t(element_type, left[i]), bc_value_t(element_type, right[i]), element_type);
		if(element_result != 0){
			return element_result;
		}
	}
	if(left.size() == right.size()){
		return 0;
	}
	else if(left.size() > right.size()){
		return -1;
	}
	else{
		return +1;
	}
}
*/

int compare_bools(const bool left, const bool right){
	auto left2 = left ? 1 : 0;
	auto right2 = right ? 1 : 0;
	if(left2 < right2){
		return -1;
	}
	else if(left2 > right2){
		return 1;
	}
	else{
		return 0;
	}
}

int compare_ints(const int64_t left, const int64_t right){
	if(left < right){
		return -1;
	}
	else if(left > right){
		return 1;
	}
	else{
		return 0;
	}
}
int compare_doubles(const double left, const double right){
	if(left < right){
		return -1;
	}
	else if(left > right){
		return 1;
	}
	else{
		return 0;
	}
}

int runtime_compare_vectors_bool(const VEC_T& left, const VEC_T& right){
	QUARK_ASSERT(left.check_invariant());
	QUARK_ASSERT(right.check_invariant());

	const auto shared_count = std::min(left.size(), right.size());
	for(int i = 0 ; i < shared_count ; i++){
		int result = compare_bools(left[i] != 0x00, right[i] != 0x00);
		if(result != 0){
			return result;
		}
	}
	if(left.size() == right.size()){
		return 0;
	}
	else if(left.size() > right.size()){
		return -1;
	}
	else{
		return +1;
	}
}
int runtime_compare_vectors_int(const VEC_T& left, const VEC_T& right){
	QUARK_ASSERT(left.check_invariant());
	QUARK_ASSERT(right.check_invariant());

	const auto shared_count = std::min(left.size(), right.size());
	for(int i = 0 ; i < shared_count ; i++){
		int result = compare_ints(left[i], right[i]);
		if(result != 0){
			return result;
		}
	}
	if(left.size() == right.size()){
		return 0;
	}
	else if(left.size() > right.size()){
		return -1;
	}
	else{
		return +1;
	}
}
int runtime_compare_vectors_double(const VEC_T& left, const VEC_T& right){
	QUARK_ASSERT(left.check_invariant());
	QUARK_ASSERT(right.check_invariant());

	const auto shared_count = std::min(left.size(), right.size());
	for(int i = 0 ; i < shared_count ; i++){
		const uint64_t left2 = left[i];
		const uint64_t right2 = right[i];
		int result = compare_doubles(*(double*)&left2, *(double*)&right2);
		if(result != 0){
			return result;
		}
	}
	if(left.size() == right.size()){
		return 0;
	}
	else if(left.size() > right.size()){
		return -1;
	}
	else{
		return +1;
	}
}

int runtime_compare_vectors_string(const VEC_T& left, const VEC_T& right){
	QUARK_ASSERT(left.check_invariant());
	QUARK_ASSERT(right.check_invariant());

	const auto shared_count = std::min(left.size(), right.size());
	for(int i = 0 ; i < shared_count ; i++){
		int result = runtime_compare_string((const char*)left[i], (const char*)right[i]);
		if(result != 0){
			return result;
		}
	}
	if(left.size() == right.size()){
		return 0;
	}
	else if(left.size() > right.size()){
		return -1;
	}
	else{
		return +1;
	}
}



template <typename Map> bool bc_map_compare(Map const &lhs, Map const &rhs) {
	// No predicate needed because there is operator== for pairs already.
	return lhs.size() == rhs.size() && std::equal(lhs.begin(), lhs.end(), rhs.begin());
}
/*
int bc_compare_dicts_obj(const std::map<std::string, runtime_external_handle_t>& left, const std::map<std::string, runtime_external_handle_t>& right, const typeid_t& type){
	const auto& element_type = typeid_t(type.get_dict_value_type());

	auto left_it = left.begin();
	auto left_end_it = left.end();

	auto right_it = right.begin();
	auto right_end_it = right.end();

	while(left_it != left_end_it && right_it != right_end_it){
		auto left_key = (*left_it).first;
		auto right_key = (*right_it).first;

		const auto key_result = runtime_compare_string(left_key, right_key);
		if(key_result != 0){
			return key_result;
		}

		const auto element_result = runtime_compare_value_true_deep(bc_value_t(element_type, (*left_it).second), bc_value_t(element_type, (*right_it).second), element_type);
		if(element_result != 0){
			return element_result;
		}

		left_it++;
		right_it++;
	}

	if(left_it == left_end_it && right_it == right_end_it){
		return 0;
	}
	else if(left_it == left_end_it && right_it != right_end_it){
		return 1;
	}
	else if(left_it != left_end_it && right_it == right_end_it){
		return -1;
	}
	QUARK_ASSERT(false)
	quark::throw_exception();
}

//??? make template.
int bc_compare_dicts_bool(const std::map<std::string, uint64_t>& left, const std::map<std::string, uint64_t>& right){
	auto left_it = left.begin();
	auto left_end_it = left.end();

	auto right_it = right.begin();
	auto right_end_it = right.end();

	while(left_it != left_end_it && right_it != right_end_it){
		auto left_key = (*left_it).first;
		auto right_key = (*right_it).first;

		const auto key_result = runtime_compare_string(left_key, right_key);
		if(key_result != 0){
			return key_result;
		}

		int result = compare_bools((*left_it).second, (*right_it).second);
		if(result != 0){
			return result;
		}

		left_it++;
		right_it++;
	}

	if(left_it == left_end_it && right_it == right_end_it){
		return 0;
	}
	else if(left_it == left_end_it && right_it != right_end_it){
		return 1;
	}
	else if(left_it != left_end_it && right_it == right_end_it){
		return -1;
	}
	QUARK_ASSERT(false)
	quark::throw_exception();
}

int bc_compare_dicts_int(const std::map<std::string, uint64_t>& left, const std::map<std::string, uint64_t>& right){
	auto left_it = left.begin();
	auto left_end_it = left.end();

	auto right_it = right.begin();
	auto right_end_it = right.end();

	while(left_it != left_end_it && right_it != right_end_it){
		auto left_key = (*left_it).first;
		auto right_key = (*right_it).first;

		const auto key_result = runtime_compare_string(left_key, right_key);
		if(key_result != 0){
			return key_result;
		}

		int result = compare_ints((*left_it).second, (*right_it).second);
		if(result != 0){
			return result;
		}

		left_it++;
		right_it++;
	}

	if(left_it == left_end_it && right_it == right_end_it){
		return 0;
	}
	else if(left_it == left_end_it && right_it != right_end_it){
		return 1;
	}
	else if(left_it != left_end_it && right_it == right_end_it){
		return -1;
	}
	QUARK_ASSERT(false)
	quark::throw_exception();
}

int runtime_compare_dicts_double(const std::map<std::string, uint64_t>& left, const std::map<std::string, uint64_t>& right){
	auto left_it = left.begin();
	auto left_end_it = left.end();

	auto right_it = right.begin();
	auto right_end_it = right.end();

	while(left_it != left_end_it && right_it != right_end_it){
		auto left_key = (*left_it).first;
		auto right_key = (*right_it).first;

		const auto key_result = runtime_compare_string(left_key, right_key);
		if(key_result != 0){
			return key_result;
		}

		int result = compare_doubles((*left_it).second, (*right_it).second);
		if(result != 0){
			return result;
		}

		left_it++;
		right_it++;
	}

	if(left_it == left_end_it && right_it == right_end_it){
		return 0;
	}
	else if(left_it == left_end_it && right_it != right_end_it){
		return 1;
	}
	else if(left_it != left_end_it && right_it == right_end_it){
		return -1;
	}
	QUARK_ASSERT(false)
	quark::throw_exception();
}
*/


static int json_value_type_to_int(const json_t& value){
	if(value.is_object()){
		return 0;
	}
	else if(value.is_array()){
		return 1;
	}
	else if(value.is_string()){
		return 2;
	}
	else if(value.is_number()){
		return 4;
	}
	else if(value.is_true()){
		return 5;
	}
	else if(value.is_false()){
		return 6;
	}
	else if(value.is_null()){
		return 7;
	}
	else{
		QUARK_ASSERT(false);
		quark::throw_exception();
	}
}


int runtime_compare_json_values(const json_t& lhs, const json_t& rhs){
	if(lhs == rhs){
		return 0;
	}
	else{
		const auto lhs_type = json_value_type_to_int(lhs);
		const auto rhs_type = json_value_type_to_int(rhs);
		int type_diff = rhs_type - lhs_type;
		if(type_diff != 0){
			return type_diff;
		}
		else{
			if(lhs.is_object()){
				QUARK_ASSERT(false);
				quark::throw_exception();
			}
			else if(lhs.is_array()){
				QUARK_ASSERT(false);
				quark::throw_exception();
			}
			else if(lhs.is_string()){
				const auto diff = std::strcmp(lhs.get_string().c_str(), rhs.get_string().c_str());
				return diff < 0 ? -1 : 1;
			}
			else if(lhs.is_number()){
				const auto lhs_number = lhs.get_number();
				const auto rhs_number = rhs.get_number();
				return lhs_number < rhs_number ? -1 : 1;
			}
			else if(lhs.is_true()){
				QUARK_ASSERT(false);
				quark::throw_exception();
			}
			else if(lhs.is_false()){
				QUARK_ASSERT(false);
				quark::throw_exception();
			}
			else if(lhs.is_null()){
				QUARK_ASSERT(false);
				quark::throw_exception();
			}
			else{
				QUARK_ASSERT(false);
				quark::throw_exception();
			}
		}
	}
}

/*
int runtime_compare_value_exts(const runtime_external_handle_t& left, const runtime_external_handle_t& right, const typeid_t& type){
	return runtime_compare_value_true_deep(bc_value_t(type, left), bc_value_t(type, right), type);
}
*/

int runtime_compare_value_true_deep(const llvm_execution_engine_t& runtime, const value_t& left, const value_t& right, const typeid_t& type0){
//	QUARK_ASSERT(left._type == right._type);
//	QUARK_ASSERT(left.check_invariant());
//	QUARK_ASSERT(right.check_invariant());

	const auto type = type0;
	if(type.is_undefined()){
		return 0;
	}
	else if(type.is_bool()){
		return (left.get_bool_value() == false ? 0 : 1) - (right.get_bool_value() == false ? 0 : 1);
	}
	else if(type.is_int()){
		return compare_ints(left.get_int_value(), right.get_int_value());
	}
	else if(type.is_double()){
		return compare_doubles(left.get_double_value(), right.get_double_value());
	}
	else if(type.is_string()){
		return runtime_compare_string(left.get_string_value().c_str(), right.get_string_value().c_str());
	}
#if 0
	else if(type.is_json_value()){
		return runtime_compare_json_values(left.get_json_value(), right.get_json_value());
	}
	else if(type.is_typeid()){
		if(left.get_typeid_value() == right.get_typeid_value()){
			return 0;
		}
		else{
			return -1;//??? Hack -- should return +1 depending on values.
		}
	}
#endif
	else if(type.is_struct()){
		//	Make sure the EXACT struct types are the same -- not only that they are both structs
		return runtime_compare_struct_true_deep(runtime, *left.get_struct_value(), *right.get_struct_value(), type0);
	}
	else if(type.is_vector()){
//		const auto left2 = *reinterpret_cast<const VEC_T*>(left);
//		const auto right2 = *reinterpret_cast<const VEC_T*>(right));
		return -1;
#if 0
		if(false){
		}
		else if(type.get_vector_element_type().is_bool()){
			return runtime_compare_vectors_bool(*reinterpret_cast<const VEC_T*>(left), *reinterpret_cast<const VEC_T*>(right));
		}
		else if(type.get_vector_element_type().is_int()){
			return runtime_compare_vectors_int(*(const VEC_T*)left, *(const VEC_T*)right);
		}
		else if(type.get_vector_element_type().is_double()){
			return runtime_compare_vectors_double(*(const VEC_T*)left, *(const VEC_T*)right);
		}
		else if(type.get_vector_element_type().is_string()){
			return runtime_compare_vectors_string(*(const VEC_T*)left, *(const VEC_T*)right);
		}
#endif

/*
		else{
			NOT_IMPLEMENTED_YET();
			const auto& left_vec = get_vector_external_elements(left);
			const auto& right_vec = get_vector_external_elements(right);
			return runtime_compare_vectors_obj(*left_vec, *right_vec, type0);
		}
*/
	}
	else if(type.is_dict()){
		NOT_IMPLEMENTED_YET();
/*
		if(false){
		}
		else if(type.get_dict_value_type().is_bool()){
			return bc_compare_dicts_bool(left._pod._external->_dict_w_inplace_values, right._pod._external->_dict_w_inplace_values);
		}
		else if(type.get_dict_value_type().is_int()){
			return bc_compare_dicts_int(left._pod._external->_dict_w_inplace_values, right._pod._external->_dict_w_inplace_values);
		}
		else if(type.get_dict_value_type().is_double()){
			return runtime_compare_dicts_double(left._pod._external->_dict_w_inplace_values, right._pod._external->_dict_w_inplace_values);
		}
		else  {
			const auto& left2 = get_dict_value(left);
			const auto& right2 = get_dict_value(right);
			return bc_compare_dicts_obj(left2, right2, type0);
		}
*/

	}
	else if(type.is_function()){
		QUARK_ASSERT(false);
		return 0;
	}
	else{
		QUARK_ASSERT(false);
		quark::throw_exception();
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

value_t call_function(llvm_execution_engine_t& ee, const std::pair<void*, typeid_t>& f){
	QUARK_ASSERT(f.first != nullptr);
	QUARK_ASSERT(f.second.is_function());

	const auto function_ptr = reinterpret_cast<FLOYD_RUNTIME_F*>(f.first);
	encoded_native_value_t return_encoded = (*function_ptr)(&ee, "?dummy arg to main()?");

	const auto return_type = f.second.get_function_return();
	return runtime_llvm_to_value(ee, return_encoded, return_type);
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

value_t load_global(llvm_execution_engine_t& ee, const std::pair<void*, typeid_t>& v){
	QUARK_ASSERT(v.first != nullptr);
	QUARK_ASSERT(v.second.is_undefined() == false);

	return llvm_global_to_value(ee, v.first, v.second);
}


itype_t unpack_encoded_itype(encoded_native_value_t itype){
	return itype_t(static_cast<uint32_t>(itype));
}

encoded_native_value_t pack_encoded_itype(const itype_t& itype){
	return itype.itype;
}

static VEC_T* unpack_vec_arg(const llvm_execution_engine_t& r, encoded_native_value_t arg_value, encoded_native_value_t arg_type){
	const auto type = lookup_type(r.type_interner, unpack_encoded_itype(arg_type));
	QUARK_ASSERT(type.is_vector());
	const auto vec = (VEC_T*)arg_value;
	QUARK_ASSERT(vec != nullptr);

	QUARK_ASSERT(vec->check_invariant());

	return vec;
}

static DICT_T* unpack_dict_arg(const llvm_execution_engine_t& r, encoded_native_value_t arg_value, encoded_native_value_t arg_type){
	const auto type = lookup_type(r.type_interner, unpack_encoded_itype(arg_type));
	QUARK_ASSERT(type.is_dict());
	const auto dict = (DICT_T*)arg_value;
	QUARK_ASSERT(dict != nullptr);

	QUARK_ASSERT(dict->check_invariant());

	return dict;
}



value_t llvm_global_to_value(const llvm_execution_engine_t& runtime, const void* global_ptr, const typeid_t& type){
	QUARK_ASSERT(runtime.check_invariant());
	QUARK_ASSERT(global_ptr != nullptr);
	QUARK_ASSERT(type.check_invariant());

	return llvm_valueptr_to_value(runtime, global_ptr, type);
}




//??? Lose concept of "encoded" value ASAP.

value_t runtime_llvm_to_value(const llvm_execution_engine_t& runtime, const encoded_native_value_t encoded_value, const typeid_t& type){
	QUARK_ASSERT(type.check_invariant());

	//??? more types.
	if(type.is_undefined()){
		NOT_IMPLEMENTED_YET();
	}
	else if(type.is_bool()){
		//??? How is int1 encoded by LLVM?
		const auto temp = static_cast<const uint8_t>(encoded_value);
		return value_t::make_bool(temp == 0 ? false : true);
	}
	else if(type.is_int()){
		const auto temp = static_cast<const uint64_t>(encoded_value);
		return value_t::make_int(temp);
	}
	else if(type.is_double()){
		const auto temp = *reinterpret_cast<const double*>(&encoded_value);
		return value_t::make_double(temp);
	}
	else if(type.is_string()){
		const char* s = (const char*)(encoded_value);
		return value_t::make_string(s);
	}
	else if(type.is_json_value()){
		NOT_IMPLEMENTED_YET();
	}
	else if(type.is_typeid()){
		const auto type1 = lookup_type(runtime.type_interner, unpack_encoded_itype(encoded_value));
		const auto type2 = value_t::make_typeid_value(type1);
		return type2;
	}
	else if(type.is_struct()){
		const auto& struct_def = type.get_struct();
		std::vector<value_t> members;
		int member_index = 0;
		auto t2 = make_struct_type(runtime.instance->context, type);
		const llvm::DataLayout& data_layout = runtime.ee->getDataLayout();
		const llvm::StructLayout* layout = data_layout.getStructLayout(t2);
		for(const auto& e: struct_def._members){
			const auto offset = layout->getElementOffset(member_index);
			const auto member_ptr = reinterpret_cast<const void*>(encoded_value + offset);
			const auto member_value = llvm_valueptr_to_value(runtime, member_ptr, e._type);
			members.push_back(member_value);
			member_index++;
		}
		return value_t::make_struct_value(type, members);
	}
	else if(type.is_vector()){
		const auto element_type = type.get_vector_element_type();

		//??? test with non-string vectors.
		const auto encoded_itype = pack_encoded_itype(lookup_itype(runtime.type_interner, type));
		const auto vec = unpack_vec_arg(runtime, encoded_value, encoded_itype);

		std::vector<value_t> elements;
		const int count = vec->element_count;
		for(int i = 0 ; i < count ; i++){
			const auto value_encoded = vec->element_ptr[i];
			const auto value = runtime_llvm_to_value(runtime, value_encoded, element_type);
			elements.push_back(value);
		}
		const auto val = value_t::make_vector_value(element_type, elements);
		return val;
	}
	else if(type.is_dict()){
		const auto value_type = type.get_dict_value_type();

		const auto encoded_itype = pack_encoded_itype(lookup_itype(runtime.type_interner, type));
		const auto dict = unpack_dict_arg(runtime, encoded_value, encoded_itype);

		std::map<std::string, value_t> values;
		const auto& map2 = dict->body_ptr->map;
		for(const auto& e: map2){
			const auto value_encoded = e.second;
			const auto value = runtime_llvm_to_value(runtime, value_encoded, value_type);
			values.insert({ e.first, value} );
		}
		const auto val = value_t::make_dict_value(type, values);
		return val;
	}
	else if(type.is_function()){
		NOT_IMPLEMENTED_YET();
	}
	else{
		NOT_IMPLEMENTED_YET();
	}
	QUARK_ASSERT(false);
	throw std::exception();
}


//??? Use visitor for typeid
//??? rename runtime_*
value_t llvm_valueptr_to_value(const llvm_execution_engine_t& runtime, const void* value_ptr, const typeid_t& type){
	QUARK_ASSERT(runtime.check_invariant());
	QUARK_ASSERT(value_ptr != nullptr);
	QUARK_ASSERT(type.check_invariant());

	//??? more types.
	if(type.is_undefined()){
	}
	else if(type.is_bool()){
		//??? How is int1 encoded by LLVM?
		const auto temp = *static_cast<const uint8_t*>(value_ptr);
		return value_t::make_bool(temp == 0 ? false : true);
	}
	else if(type.is_int()){
		const auto temp = *static_cast<const uint64_t*>(value_ptr);
		return value_t::make_int(temp);
	}
	else if(type.is_double()){
		const auto temp = *static_cast<const double*>(value_ptr);
		return value_t::make_double(temp);
	}
	else if(type.is_string()){
		const char* s = *(const char**)(value_ptr);
		return value_t::make_string(s);
	}
	else if(type.is_json_value()){
		NOT_IMPLEMENTED_YET();
	}
	else if(type.is_typeid()){
		const auto value = *static_cast<const int32_t*>(value_ptr);
		const auto type_value = lookup_type(runtime.type_interner, unpack_encoded_itype(value));
		return value_t::make_typeid_value(type_value);
	}
	else if(type.is_struct()){
		const auto& struct_def = type.get_struct();
		std::vector<value_t> members;
		int member_index = 0;
		auto t2 = make_struct_type(runtime.instance->context, type);

		//	Read struct pointer.
		const auto struct_ptr_as_int = *reinterpret_cast<const uint64_t*>(value_ptr);
		const auto struct_ptr = reinterpret_cast<const uint8_t*>(struct_ptr_as_int);

		const llvm::DataLayout& data_layout = runtime.ee->getDataLayout();
		const llvm::StructLayout* layout = data_layout.getStructLayout(t2);

		for(const auto& e: struct_def._members){
			const auto member_offset = layout->getElementOffset(member_index);
			const auto member_ptr = reinterpret_cast<const void*>(struct_ptr + member_offset);
			const auto member_value = llvm_valueptr_to_value(runtime, member_ptr, e._type);
			members.push_back(member_value);
			member_index++;
		}
		return value_t::make_struct_value(type, members);
	}
	else if(type.is_vector()){
		auto vec = *static_cast<const VEC_T*>(value_ptr);
		std::vector<value_t> vec2;

		const auto element_type = type.get_vector_element_type();
		if(element_type.is_string()){
			for(int i = 0 ; i < vec.element_count ; i++){
				auto s = (const char*)vec.element_ptr[i];
				const auto a = std::string(s);
				vec2.push_back(value_t::make_string(a));
			}
			return value_t::make_vector_value(element_type, vec2);
		}
		else if(element_type.is_bool()){
			for(int i = 0 ; i < vec.element_count ; i++){
				auto s = vec.element_ptr[i];
				vec2.push_back(value_t::make_bool(s != 0x00));
			}
			return value_t::make_vector_value(element_type, vec2);
		}
		else if(element_type.is_int()){
			for(int i = 0 ; i < vec.element_count ; i++){
				auto s = vec.element_ptr[i];
				vec2.push_back(value_t::make_int(s));
			}
			return value_t::make_vector_value(element_type, vec2);
		}
		else if(element_type.is_double()){
			for(int i = 0 ; i < vec.element_count ; i++){
				auto s = vec.element_ptr[i];
				auto d = *(double*)&s;
				vec2.push_back(value_t::make_double(d));
			}
			return value_t::make_vector_value(element_type, vec2);
		}
		else{
			NOT_IMPLEMENTED_YET();
		}
	}
/*
!!!
	else if(type.is_vector()){
		auto vec = *static_cast<const VEC_T*>(value_ptr);
		std::vector<value_t> vec2;
		if(type.get_vector_element_type().is_string()){
			for(int i = 0 ; i < vec.element_count ; i++){
				auto s = (const char*)vec.element_ptr[i];
				const auto a = std::string(s);
				vec2.push_back(value_t::make_string(a));
			}
			return value_t::make_vector_value(typeid_t::make_string(), vec2);
		}
		else if(type.get_vector_element_type().is_bool()){
			for(int i = 0 ; i < vec.element_count ; i++){
				auto s = vec.element_ptr[i / 64];
				bool masked = s & (1 << (i & 63));
				vec2.push_back(value_t::make_bool(masked == 0 ? false : true));
			}
			return value_t::make_vector_value(typeid_t::make_bool(), vec2);
		}
		else{
		NOT_IMPLEMENTED_YET();
		}
	}
*/
	else if(type.is_dict()){
		NOT_IMPLEMENTED_YET();
	}
	else if(type.is_function()){
		NOT_IMPLEMENTED_YET();
	}
	else{
	}
	NOT_IMPLEMENTED_YET();
	QUARK_ASSERT(false);
	throw std::exception();
}






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

void hook(const std::string& s, void* floyd_runtime_ptr, encoded_native_value_t arg){
	std:: cout << s << arg << " arg: " << std::endl;
	auto& r = get_floyd_runtime(floyd_runtime_ptr);
	throw std::runtime_error("HOST FUNCTION NOT IMPLEMENTED FOR LLVM");
}



std::string gen_to_string(llvm_execution_engine_t& runtime, encoded_native_value_t arg_value, dyn_value_type_argument_t arg_type){
	QUARK_ASSERT(runtime.check_invariant());

	const auto type = lookup_type(runtime.type_interner, unpack_encoded_itype(arg_type));
	const auto value = runtime_llvm_to_value(runtime, arg_value, type);
	const auto a = to_compact_string2(value);
	return a;
}


//	The names of these are computed from the host-id in the symbol table, not the names of the functions/symbols.
//	They must use C calling convention so llvm JIT can find them.
//	Make sure they are not dead-stripped out of binary!

void floyd_runtime__unresolved_func(void* floyd_runtime_ptr){
	std:: cout << __FUNCTION__ << std::endl;
}

















//	Host functions, automatically called by the LLVM execution engine.
////////////////////////////////////////////////////////////////////////////////


int32_t floyd_runtime__compare_strings(void* floyd_runtime_ptr, int64_t op, const char* lhs, const char* rhs){
	auto& r = get_floyd_runtime(floyd_runtime_ptr);

	/*
		strcmp()
			Greater than zero ( >0 ): A value greater than zero is returned when the first not matching character in leftStr have the greater ASCII value than the corresponding character in rightStr or we can also say

			Less than Zero ( <0 ): A value less than zero is returned when the first not matching character in leftStr have lesser ASCII value than the corresponding character in rightStr.

	*/
	const auto result = std::strcmp(lhs, rhs);

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

host_func_t floyd_runtime__compare_strings__make(llvm::LLVMContext& context){
	llvm::FunctionType* function_type = llvm::FunctionType::get(
		llvm::Type::getInt32Ty(context),
		{
			make_frp_type(context),
			llvm::Type::getInt64Ty(context),
			llvm::Type::getInt8PtrTy(context),
			llvm::Type::getInt8PtrTy(context)
		},
		false
	);
	return { "floyd_runtime__compare_strings", function_type, reinterpret_cast<void*>(floyd_runtime__compare_strings) };
}


const char* floyd_runtime__concatunate_strings(void* floyd_runtime_ptr, const char* lhs, const char* rhs){
	auto& r = get_floyd_runtime(floyd_runtime_ptr);
	QUARK_ASSERT(lhs != nullptr);
	QUARK_ASSERT(rhs != nullptr);
	QUARK_TRACE_SS(__FUNCTION__ << " " << std::string(lhs) << " comp " <<std::string(rhs));

	std::size_t len = strlen(lhs) + strlen(rhs);
	char* s = reinterpret_cast<char*>(std::malloc(len + 1));
	strcpy(s, lhs);
	strcpy(s + strlen(lhs), rhs);
	return s;
}

host_func_t floyd_runtime__concatunate_strings__make(llvm::LLVMContext& context){
	llvm::FunctionType* function_type = llvm::FunctionType::get(
		llvm::Type::getInt8PtrTy(context),
		{
			make_frp_type(context),
			llvm::Type::getInt8PtrTy(context),
			llvm::Type::getInt8PtrTy(context)
		},
		false
	);
	return { "floyd_runtime__concatunate_strings", function_type, reinterpret_cast<void*>(floyd_runtime__concatunate_strings) };
}


const uint8_t* floyd_runtime__allocate_memory(void* floyd_runtime_ptr, int64_t bytes){
	auto s = reinterpret_cast<uint8_t*>(std::calloc(1, bytes));
	return s;
}

host_func_t floyd_runtime__allocate_memory__make(llvm::LLVMContext& context){
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
WIDE_RETURN_T floyd_runtime__allocate_vector(void* floyd_runtime_ptr, uint32_t element_count){
	auto& r = get_floyd_runtime(floyd_runtime_ptr);

	VEC_T v = make_vec(element_count);
	return make_wide_return_vec(v);
}

host_func_t floyd_runtime__allocate_vector__make(llvm::LLVMContext& context){
	llvm::FunctionType* function_type = llvm::FunctionType::get(
		make_wide_return_type(context),
		{
			make_frp_type(context),
			llvm::Type::getInt32Ty(context)
		},
		false
	);
	return { "floyd_runtime__allocate_vector", function_type, reinterpret_cast<void*>(floyd_runtime__allocate_vector) };
}


////////////////////////////////		delete_vector()


void floyd_runtime__delete_vector(void* floyd_runtime_ptr, VEC_T* vec){
	auto& r = get_floyd_runtime(floyd_runtime_ptr);
	QUARK_ASSERT(vec != nullptr);
	QUARK_ASSERT(vec->check_invariant());

	delete_vec(*vec);
}

host_func_t floyd_runtime__delete_vector__make(llvm::LLVMContext& context){
	llvm::FunctionType* function_type = llvm::FunctionType::get(
		llvm::Type::getVoidTy(context),
		{
			make_frp_type(context),
			make_vec_type(context)->getPointerTo()
		},
		false
	);
	return { "floyd_runtime__delete_vector", function_type, reinterpret_cast<void*>(floyd_runtime__delete_vector) };
}

WIDE_RETURN_T floyd_runtime__concatunate_vectors(void* floyd_runtime_ptr, const VEC_T* lhs, const VEC_T* rhs){
	auto& r = get_floyd_runtime(floyd_runtime_ptr);
	QUARK_ASSERT(lhs != nullptr);
	QUARK_ASSERT(lhs->check_invariant());
	QUARK_ASSERT(rhs != nullptr);
	QUARK_ASSERT(rhs->check_invariant());

	auto result = make_vec(lhs->element_count + rhs->element_count);
	for(int i = 0 ; i < lhs->element_count ; i++){
		result.element_ptr[i] = lhs->element_ptr[i];
	}
	for(int i = 0 ; i < rhs->element_count ; i++){
		result.element_ptr[lhs->element_count + i] = rhs->element_ptr[i];
	}
	return make_wide_return_vec(result);
}

host_func_t floyd_runtime__concatunate_vectors__make(llvm::LLVMContext& context){
	llvm::FunctionType* function_type = llvm::FunctionType::get(
		make_wide_return_type(context),
		{
			make_frp_type(context),
			make_vec_type(context)->getPointerTo(),
			make_vec_type(context)->getPointerTo()
		},
		false
	);
	return { "floyd_runtime__concatunate_vectors", function_type, reinterpret_cast<void*>(floyd_runtime__concatunate_vectors) };
}




////////////////////////////////		allocate_dict()


WIDE_RETURN_T floyd_runtime__allocate_dict(void* floyd_runtime_ptr){
	auto& r = get_floyd_runtime(floyd_runtime_ptr);

	auto v = make_dict();
	return make_wide_return_dict(v);
}

host_func_t floyd_runtime__allocate_dict__make(llvm::LLVMContext& context){
	llvm::FunctionType* function_type = llvm::FunctionType::get(
		make_wide_return_type(context),
		{
			make_frp_type(context)
		},
		false
	);
	return { "floyd_runtime__allocate_dict", function_type, reinterpret_cast<void*>(floyd_runtime__allocate_dict) };
}


////////////////////////////////		delete_dict()


void floyd_runtime__delete_dict(void* floyd_runtime_ptr, DICT_T* dict){
	auto& r = get_floyd_runtime(floyd_runtime_ptr);
	QUARK_ASSERT(dict != nullptr);
	QUARK_ASSERT(dict->check_invariant());

	delete_dict(*dict);
}

host_func_t floyd_runtime__delete_dict__make(llvm::LLVMContext& context){
	llvm::FunctionType* function_type = llvm::FunctionType::get(
		llvm::Type::getVoidTy(context),
		{
			make_frp_type(context),
			make_vec_type(context)->getPointerTo()
		},
		false
	);
	return { "floyd_runtime__delete_dict", function_type, reinterpret_cast<void*>(floyd_runtime__delete_dict) };
}


////////////////////////////////		store_dict()


WIDE_RETURN_T floyd_runtime__store_dict(void* floyd_runtime_ptr, DICT_T* dict, const char* key_string, encoded_native_value_t element_value, dyn_value_type_argument_t element_type){
	auto& r = get_floyd_runtime(floyd_runtime_ptr);

	const auto element_type2 = lookup_type(r.type_interner, unpack_encoded_itype(element_type));

	auto v = make_dict();

	//	Deep copy dict.
	v.body_ptr->map = dict->body_ptr->map;
	if(element_type2.is_int()){
		v.body_ptr->map.insert_or_assign(std::string(key_string), element_value);
		return make_wide_return_dict(v);
	}
	else{
		NOT_IMPLEMENTED_YET();
	}
}

host_func_t floyd_runtime__store_dict__make(llvm::LLVMContext& context){
	llvm::FunctionType* function_type = llvm::FunctionType::get(
		make_wide_return_type(context),
		{
			make_frp_type(context),
			make_dict_type(context)->getPointerTo(),
			llvm::Type::getInt8PtrTy(context),
			make_dyn_value_type(context),
			make_dyn_value_type_type(context)
		},
		false
	);
	return { "floyd_runtime__store_dict", function_type, reinterpret_cast<void*>(floyd_runtime__store_dict) };
}



////////////////////////////////		lookup_dict()


encoded_native_value_t floyd_runtime__lookup_dict(void* floyd_runtime_ptr, DICT_T* dict, const char* key_string){
	auto& r = get_floyd_runtime(floyd_runtime_ptr);

//	const auto element_type2 = lookup_type(r.type_interner, unpack_encoded_itype(element_type));
	const auto it = dict->body_ptr->map.find(std::string(key_string));
	if(it == dict->body_ptr->map.end()){
		throw std::exception();
	}
	else{
		return it->second;
	}
}

host_func_t floyd_runtime__lookup_dict__make(llvm::LLVMContext& context){
	llvm::FunctionType* function_type = llvm::FunctionType::get(
		make_encoded_native_value_type(context),
		{
			make_frp_type(context),
			make_dict_type(context)->getPointerTo(),
			llvm::Type::getInt8PtrTy(context)
		},
		false
	);
	return { "floyd_runtime__lookup_dict", function_type, reinterpret_cast<void*>(floyd_runtime__lookup_dict) };
}






////////////////////////////////		compare_values()


int32_t floyd_runtime__compare_values(void* floyd_runtime_ptr, int64_t op, const uint64_t type, dyn_value_argument_t lhs, dyn_value_argument_t rhs){
	auto& r = get_floyd_runtime(floyd_runtime_ptr);

	const auto value_type = lookup_type(r.type_interner, unpack_encoded_itype(type));

	const auto left_value = runtime_llvm_to_value(r, lhs, value_type);
	const auto right_value = runtime_llvm_to_value(r, rhs, value_type);

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

host_func_t floyd_runtime__compare_values__make(llvm::LLVMContext& context){
	llvm::FunctionType* function_type = llvm::FunctionType::get(
		llvm::Type::getInt32Ty(context),
		{
			make_frp_type(context),
			llvm::Type::getInt64Ty(context),
			llvm::Type::getInt64Ty(context),

			make_dyn_value_type(context),
			make_dyn_value_type(context)
		},
		false
	);
	return { "floyd_runtime__compare_values", function_type, reinterpret_cast<void*>(floyd_runtime__compare_values) };
}




std::vector<host_func_t> get_runtime_functions(llvm::LLVMContext& context){
	std::vector<host_func_t> result = {
		floyd_runtime__compare_strings__make(context),
		floyd_runtime__concatunate_strings__make(context),

		floyd_runtime__allocate_memory__make(context),

		floyd_runtime__allocate_vector__make(context),
		floyd_runtime__delete_vector__make(context),
		floyd_runtime__concatunate_vectors__make(context),

		floyd_runtime__allocate_dict__make(context),
		floyd_runtime__delete_dict__make(context),
		floyd_runtime__store_dict__make(context),
		floyd_runtime__lookup_dict__make(context),

		floyd_runtime__compare_values__make(context)
	};
	return result;
}






























//??? should be an int1?
void floyd_funcdef__assert(void* floyd_runtime_ptr, int64_t arg){
	auto& r = get_floyd_runtime(floyd_runtime_ptr);

	bool ok = arg == 0 ? false : true;
	if(!ok){
		r._print_output.push_back("Assertion failed.");
		quark::throw_runtime_error("Floyd assertion failed.");
	}
}

void floyd_host_function_1001(void* floyd_runtime_ptr, int64_t arg){
	hook(__FUNCTION__, floyd_runtime_ptr, arg);
}

void floyd_host_function_1002(void* floyd_runtime_ptr, int64_t arg){
	hook(__FUNCTION__, floyd_runtime_ptr, arg);
}

void floyd_host_function_1003(void* floyd_runtime_ptr, int64_t arg){
	hook(__FUNCTION__, floyd_runtime_ptr, arg);
}

void floyd_host_function_1004(void* floyd_runtime_ptr, int64_t arg){
	hook(__FUNCTION__, floyd_runtime_ptr, arg);
}

void floyd_host_function_1005(void* floyd_runtime_ptr, int64_t arg){
	hook(__FUNCTION__, floyd_runtime_ptr, arg);
}

WIDE_RETURN_T floyd_host_function__erase(void* floyd_runtime_ptr, dyn_value_argument_t arg0_value, dyn_value_type_argument_t arg0_type, dyn_value_argument_t arg1_value, dyn_value_type_argument_t arg1_type){
	auto& r = get_floyd_runtime(floyd_runtime_ptr);

	const auto type0 = lookup_type(r.type_interner, unpack_encoded_itype(arg0_type));
	const auto type1 = lookup_type(r.type_interner, unpack_encoded_itype(arg1_type));

	if(type0.is_dict()){
		const auto& dict = unpack_dict_arg(r, arg0_value, arg0_type);

		//	Deep copy dict.
		auto dict2 = make_dict();
		dict2.body_ptr->map = dict->body_ptr->map;

		const auto key_strptr = reinterpret_cast<const char*>(arg1_value);
		const auto key_string = std::string(key_strptr);
		const auto erase_count = dict2.body_ptr->map.erase(key_string);
		return make_wide_return_dict(dict2);
	}
	else{
		NOT_IMPLEMENTED_YET();
	}
}

uint32_t floyd_funcdef__exists(void* floyd_runtime_ptr, dyn_value_argument_t arg0_value, dyn_value_type_argument_t arg0_type, dyn_value_argument_t arg1_value, dyn_value_type_argument_t arg1_type){
	auto& r = get_floyd_runtime(floyd_runtime_ptr);

	const auto type0 = lookup_type(r.type_interner, unpack_encoded_itype(arg0_type));
	const auto type1 = lookup_type(r.type_interner, unpack_encoded_itype(arg1_type));

	if(type0.is_dict()){
		const auto& dict = unpack_dict_arg(r, arg0_value, arg0_type);
		const auto key_strptr = reinterpret_cast<const char*>(arg1_value);
		const auto key_string = std::string(key_strptr);

		const auto it = dict->body_ptr->map.find(key_string);
		return it != dict->body_ptr->map.end() ? 1 : 0;
	}
	else{
		NOT_IMPLEMENTED_YET();
	}
}

void floyd_host_function_1008(void* floyd_runtime_ptr, int64_t arg){
	hook(__FUNCTION__, floyd_runtime_ptr, arg);
}



int64_t floyd_funcdef__find__string(llvm_execution_engine_t& floyd_runtime_ptr, const char s[], const char find[]){
	QUARK_ASSERT(s != nullptr);
	QUARK_ASSERT(find != nullptr);

	const auto str = std::string(s);
	const auto wanted2 = std::string(find);

	const auto r = str.find(wanted2);
	const auto result = r == std::string::npos ? -1 : static_cast<int64_t>(r);
	return result;
}


//??? use int32 for types.

int64_t floyd_funcdef__find(void* floyd_runtime_ptr, dyn_value_argument_t arg0_value, dyn_value_type_argument_t arg0_type, dyn_value_argument_t arg1_value, dyn_value_type_argument_t arg1_type){
	auto& r = get_floyd_runtime(floyd_runtime_ptr);

	const auto type0 = lookup_type(r.type_interner, unpack_encoded_itype(arg0_type));
	const auto type1 = lookup_type(r.type_interner, unpack_encoded_itype(arg1_type));

	if(type0.is_string()){
		if(type1.is_string() == false){
			quark::throw_runtime_error("find(string) requires argument 2 to be a string.");
		}
		return floyd_funcdef__find__string(r, (const char*)arg0_value, (const char*)arg1_value);
	}
	else if(type0.is_vector()){
		if(type1 != type0.get_vector_element_type()){
			quark::throw_runtime_error("find([]) requires argument 2 to be of vector's element type.");
		}

		const auto vec = unpack_vec_arg(r, arg0_value, arg0_type);

		const auto it = std::find(vec->element_ptr, vec->element_ptr + vec->element_count, arg1_value);
		if(it == vec->element_ptr + vec->element_count){
			return -1;
		}
		else{
			const auto pos = it - vec->element_ptr;
			return pos;
		}
	}
	else{
		NOT_IMPLEMENTED_YET();
	}
}



void floyd_host_function_1010(void* floyd_runtime_ptr, int64_t arg){
	hook(__FUNCTION__, floyd_runtime_ptr, arg);
}

void floyd_host_function_1011(void* floyd_runtime_ptr, int64_t arg){
	hook(__FUNCTION__, floyd_runtime_ptr, arg);
}

void floyd_host_function_1012(void* floyd_runtime_ptr, int64_t arg){
	hook(__FUNCTION__, floyd_runtime_ptr, arg);
}

void floyd_host_function_1013(void* floyd_runtime_ptr, int64_t arg){
	hook(__FUNCTION__, floyd_runtime_ptr, arg);
}

void floyd_host_function_1014(void* floyd_runtime_ptr, int64_t arg){
	hook(__FUNCTION__, floyd_runtime_ptr, arg);
}

void floyd_host_function_1015(void* floyd_runtime_ptr, int64_t arg){
	hook(__FUNCTION__, floyd_runtime_ptr, arg);
}

void floyd_host_function_1016(void* floyd_runtime_ptr, int64_t arg){
	hook(__FUNCTION__, floyd_runtime_ptr, arg);
}

void floyd_host_function_1017(void* floyd_runtime_ptr, int64_t arg){
	hook(__FUNCTION__, floyd_runtime_ptr, arg);
}

void floyd_host_function_1018(void* floyd_runtime_ptr, int64_t arg){
	hook(__FUNCTION__, floyd_runtime_ptr, arg);
}

void floyd_host_function_1019(void* floyd_runtime_ptr, int64_t arg){
	hook(__FUNCTION__, floyd_runtime_ptr, arg);
}




//	??? Make visitor to handle different types.
void floyd_funcdef__print(void* floyd_runtime_ptr, dyn_value_argument_t arg0_value, int64_t arg0_type){
	auto& r = get_floyd_runtime(floyd_runtime_ptr);
	const auto s = gen_to_string(r, arg0_value, arg0_type);
	r._print_output.push_back(s);
}




const WIDE_RETURN_T floyd_funcdef__push_back(void* floyd_runtime_ptr, dyn_value_argument_t arg0_value, int64_t arg0_type, dyn_value_argument_t arg1_value, int64_t arg1_type){
	auto& r = get_floyd_runtime(floyd_runtime_ptr);

	const auto type0 = lookup_type(r.type_interner, unpack_encoded_itype(arg0_type));
	const auto type1 = lookup_type(r.type_interner, unpack_encoded_itype(arg1_type));
	if(type0.is_string()){
		const auto value = (const char*)arg0_value;

		QUARK_ASSERT(type1.is_int());

		std::size_t len = strlen(value);
		char* s = reinterpret_cast<char*>(std::malloc(len + 1 + 1));
		strcpy(s, value);
		s[len + 0] = (char)arg1_value;
		s[len + 1] = 0x00;

		return make_wide_return_charptr(s);
	}
	else if(type0.is_vector()){
		const auto vs = unpack_vec_arg(r, arg0_value, arg0_type);

		QUARK_ASSERT(type1 == type0.get_vector_element_type());

		const auto element = arg1_value;

		WIDE_RETURN_T va = floyd_runtime__allocate_vector(floyd_runtime_ptr, vs->element_count + 1);
		auto v2 = wider_return_to_vec(va);
		for(int i = 0 ; i < vs->element_count ; i++){
			v2.element_ptr[i] = vs->element_ptr[i];
		}
		v2.element_ptr[vs->element_count] = element;
		return make_wide_return_vec(v2);
	}
	else{
		NOT_IMPLEMENTED_YET();
	}
}

void floyd_host_function_1022(void* floyd_runtime_ptr, int64_t arg){
	hook(__FUNCTION__, floyd_runtime_ptr, arg);
}

void floyd_host_function_1023(void* floyd_runtime_ptr, int64_t arg){
	hook(__FUNCTION__, floyd_runtime_ptr, arg);
}

void floyd_host_function_1024(void* floyd_runtime_ptr, int64_t arg){
	hook(__FUNCTION__, floyd_runtime_ptr, arg);
}

const char* floyd_funcdef__replace__string(llvm_execution_engine_t& floyd_runtime_ptr, const char s[], std::size_t start, std::size_t end, const char replace[]){
	auto s_len = std::strlen(s);
	auto replace_len = std::strlen(replace);

	auto end2 = std::min(end, s_len);
	auto start2 = std::min(start, end2);

	const std::size_t len2 = start2 + replace_len + (s_len - end2);
	auto s2 = reinterpret_cast<char*>(std::malloc(len2 + 1));
	std::memcpy(&s2[0], &s[0], start2);
	std::memcpy(&s2[start2], &replace[0], replace_len);
	std::memcpy(&s2[start2 + replace_len], &s[end2], s_len - end2);
	s2[start2 + replace_len + (s_len - end2)] = 0x00;
	return s2;
}

void copy_elements(uint64_t dest[], uint64_t source[], uint32_t count){
	for(auto i = 0 ; i < count ; i++){
		dest[i] = source[i];
	}
}

const WIDE_RETURN_T floyd_funcdef__replace(void* floyd_runtime_ptr, dyn_value_argument_t arg0_value, int64_t arg0_type, int64_t start, int64_t end, dyn_value_argument_t arg3_value, int64_t arg3_type){
	auto& r = get_floyd_runtime(floyd_runtime_ptr);

	if(start < 0 || end < 0){
		quark::throw_runtime_error("replace() requires start and end to be non-negative.");
	}
	if(start > end){
		quark::throw_runtime_error("replace() requires start <= end.");
	}

	const auto type0 = lookup_type(r.type_interner, unpack_encoded_itype(arg0_type));
	const auto type3 = lookup_type(r.type_interner, unpack_encoded_itype(arg3_type));

	if(type3 != type0){
		quark::throw_runtime_error("replace() requires argument 4 to be same type of collection.");
	}

	if(type0.is_string()){
		const auto ret = floyd_funcdef__replace__string(r, (const char*)arg0_value, (std::size_t)start, (std::size_t)end, (const char*)arg3_value);
		return make_wide_return_charptr(ret);
	}
	else if(type0.is_vector()){
		const auto vec = unpack_vec_arg(r, arg0_value, arg0_type);
		const auto replace_vec = unpack_vec_arg(r, arg3_value, arg3_type);

		auto end2 = std::min(static_cast<uint32_t>(end), vec->element_count);
		auto start2 = std::min(static_cast<uint32_t>(start), end2);

		const int32_t section1_len = start2;
		const int32_t section2_len = replace_vec->element_count;
		const int32_t section3_len = vec->element_count - end2;

		const uint32_t len2 = section1_len + section2_len + section3_len;
		auto vec2 = make_vec(len2);
		copy_elements(&vec2.element_ptr[0], &vec->element_ptr[0], section1_len);
		copy_elements(&vec2.element_ptr[section2_len], &replace_vec->element_ptr[0], section2_len);
		copy_elements(&vec2.element_ptr[section1_len + section2_len], &vec->element_ptr[end2], section3_len);
		return make_wide_return_vec(vec2);
	}
	else{
		NOT_IMPLEMENTED_YET();
	}
}

void floyd_host_function_1026(void* floyd_runtime_ptr, int64_t arg){
	hook(__FUNCTION__, floyd_runtime_ptr, arg);
}

void floyd_host_function_1027(void* floyd_runtime_ptr, int64_t arg){
	hook(__FUNCTION__, floyd_runtime_ptr, arg);
}

int64_t floyd_funcdef__size(void* floyd_runtime_ptr, dyn_value_argument_t arg0_value, int64_t arg0_type){
	auto& r = get_floyd_runtime(floyd_runtime_ptr);

	const auto type0 = lookup_type(r.type_interner, unpack_encoded_itype(arg0_type));
	if(type0.is_string()){
		const auto value = (const char*)arg0_value;
		return std::strlen(value);
	}
	else if(type0.is_vector()){
		const auto vs = unpack_vec_arg(r, arg0_value, arg0_type);
		return vs->element_count;
	}
	else if(type0.is_dict()){
		DICT_T* dict = unpack_dict_arg(r, arg0_value, arg0_type);
		return dict->body_ptr->map.size();
	}

	else{
		NOT_IMPLEMENTED_YET();
	}
}

const WIDE_RETURN_T floyd_funcdef__subset(void* floyd_runtime_ptr, dyn_value_argument_t arg0_value, int64_t arg0_type, int64_t start, int64_t end){
	auto& r = get_floyd_runtime(floyd_runtime_ptr);

	if(start < 0 || end < 0){
		quark::throw_runtime_error("subset() requires start and end to be non-negative.");
	}

	const auto type0 = lookup_type(r.type_interner, unpack_encoded_itype(arg0_type));
	if(type0.is_string()){
		const auto value = (const char*)arg0_value;

		std::size_t len = strlen(value);
		const std::size_t end2 = std::min(static_cast<std::size_t>(end), len);
		const std::size_t start2 = std::min(static_cast<std::size_t>(start), end2);
		std::size_t len2 = end2 - start2;

		char* s = reinterpret_cast<char*>(std::malloc(len2 + 1));
		std::memcpy(&s[0], &value[start2], len2);
		s[len2] = 0x00;
		return make_wide_return_charptr(s);
	}
	else if(type0.is_vector()){
		const auto vec = unpack_vec_arg(r, arg0_value, arg0_type);

		const std::size_t end2 = std::min(static_cast<std::size_t>(end), static_cast<std::size_t>(vec->element_count));
		const std::size_t start2 = std::min(static_cast<std::size_t>(start), end2);
		std::size_t len2 = end2 - start2;

		if(len2 >= INT32_MAX){
			throw std::exception();
		}
		VEC_T vec2 = make_vec(static_cast<int32_t>(len2));
		for(int i = 0 ; i < len2 ; i++){
			vec2.element_ptr[i] = vec->element_ptr[start2 + i];
		}
		return make_wide_return_vec(vec2);
	}
	else{
		NOT_IMPLEMENTED_YET();
	}
}




void floyd_host_function_1030(void* floyd_runtime_ptr, int64_t arg){
	hook(__FUNCTION__, floyd_runtime_ptr, arg);
}

void floyd_host_function_1031(void* floyd_runtime_ptr, int64_t arg){
	hook(__FUNCTION__, floyd_runtime_ptr, arg);
}

const char* floyd_host__to_string(void* floyd_runtime_ptr, dyn_value_argument_t arg0_value, int64_t arg0_type){
	auto& r = get_floyd_runtime(floyd_runtime_ptr);

	const auto s = gen_to_string(r, arg0_value, arg0_type);

	//??? leaks.
	return strdup(s.c_str());
}



//		make_rec("typeof", host__typeof, 1004, typeid_t::make_function(typeid_t::make_typeid(), { DYN }, epure::pure)),

int32_t floyd_host__typeof(void* floyd_runtime_ptr, dyn_value_argument_t arg0_value, int64_t arg0_type){
	auto& r = get_floyd_runtime(floyd_runtime_ptr);

	const auto type0 = lookup_type(r.type_interner, unpack_encoded_itype(arg0_type));
	const auto type_value = static_cast<int32_t>(arg0_type);
	return type_value;
}

//	??? promote update() to a statement, rather than a function call. Replace all statement and expressions with function calls? LISP!
//	???	Update of structs should resolve member-name at compile time, replace with index.
//	??? Range should be integers, not DYN! Change host function prototype.
const WIDE_RETURN_T floyd_funcdef__update(void* floyd_runtime_ptr, dyn_value_argument_t arg0_value, int64_t arg0_type, dyn_value_argument_t arg1_value, int64_t arg1_type, dyn_value_argument_t arg2_value, int64_t arg2_type){
	auto& r = get_floyd_runtime(floyd_runtime_ptr);

	auto& context = r.instance->context;

	const auto type0 = lookup_type(r.type_interner, unpack_encoded_itype(arg0_type));
	const auto type1 = lookup_type(r.type_interner, unpack_encoded_itype(arg1_type));
	const auto type2 = lookup_type(r.type_interner, unpack_encoded_itype(arg2_type));
	if(type0.is_string()){
		if(type1.is_int() == false){
			throw std::exception();
		}
		if(type2.is_int() == false){
			throw std::exception();
		}

		const auto str = (const char*)arg0_value;
		const auto index = arg1_value;
		const auto new_char = (char)arg2_value;

		const auto len = strlen(str);

		if(index < 0 || index >= len){
			throw std::runtime_error("Position argument to update() is outside collection span.");
		}

		auto result = strdup(str);
		result[index] = new_char;
		return make_wide_return_charptr(result);
	}
	else if(type0.is_vector()){
		if(type1.is_int() == false){
			throw std::exception();
		}

		const auto vec = unpack_vec_arg(r, arg0_value, arg0_type);
		const auto element_type = type0.get_vector_element_type();
		const auto index = arg1_value;
		if(element_type.is_string()){
			if(type2.is_string() == false){
				throw std::exception();
			}
			if(index < 0 || index >= vec->element_count){
				throw std::runtime_error("Position argument to update() is outside collection span.");
			}

			auto result = make_vec(vec->element_count);
			for(int i = 0 ; i < result.element_count ; i++){
				result.element_ptr[i] = vec->element_ptr[i];
			}
			result.element_ptr[index] = arg2_value;
			return make_wide_return_vec(result);
		}
		else if(element_type.is_bool()){
			if(type2.is_bool() == false){
				throw std::exception();
			}

			if(index < 0 || index >= vec->element_count){
				throw std::runtime_error("Position argument to update() is outside collection span.");
			}

			auto result = make_vec(vec->element_count);
			for(int i = 0 ; i < result.element_count ; i++){
				result.element_ptr[i] = vec->element_ptr[i];
			}
			result.element_ptr[index] = static_cast<encoded_native_value_t>(arg2_value ? 1 : 0);
			return make_wide_return_vec(result);
		}
		else{
			NOT_IMPLEMENTED_YET();
		}
	}
	else if(type0.is_dict()){
		if(type1.is_string() == false){
			throw std::exception();
		}
		const auto key_strptr = reinterpret_cast<const char*>(arg1_value);
		const auto dict = unpack_dict_arg(r, arg0_value, arg0_type);

		//	Deep copy dict.
		auto dict2 = make_dict();
		dict2.body_ptr->map = dict->body_ptr->map;

		dict2.body_ptr->map.insert_or_assign(std::string(key_strptr), arg2_value);
		return make_wide_return_dict(dict2);
	}
	else if(type0.is_struct()){
		if(type1.is_string() == false){
			throw std::exception();
		}
		if(type2.is_void() == true){
			throw std::exception();
		}

		const auto source_struct_ptr = reinterpret_cast<void*>(arg0_value);

		const auto member_name = runtime_llvm_to_value(r, arg1_value, typeid_t::make_string()).get_string_value();
		if(member_name == ""){
			throw std::runtime_error("Must provide name of struct member to update().");
		}

		const auto& struct_def = type0.get_struct();
		int member_index = find_struct_member_index(struct_def, member_name);
		if(member_index == -1){
			throw std::runtime_error("Position argument to update() is outside collection span.");
		}

		const auto member_value = runtime_llvm_to_value(r, arg2_value, type2);


		//	Make copy of struct, overwrite member in copy.
 
		auto& struct_type_llvm = *make_struct_type(context, type0);

		const llvm::DataLayout& data_layout = r.ee->getDataLayout();
		const llvm::StructLayout* layout = data_layout.getStructLayout(&struct_type_llvm);
		const auto struct_bytes = layout->getSizeInBytes();

		//??? Touches memory twice.
		auto struct_ptr = reinterpret_cast<uint8_t*>(std::calloc(1, struct_bytes));
		std::memcpy(struct_ptr, source_struct_ptr, struct_bytes);

		const auto member_offset = layout->getElementOffset(member_index);
		const auto member_ptr = reinterpret_cast<const void*>(struct_ptr + member_offset);

		const auto member_type = struct_def._members[member_index]._type;

		if(type2 != member_type){
			throw std::runtime_error("New value must be same type as struct member's type.");
		}

		if(member_type.is_string()){
			auto dest = *(char**)(member_ptr);
			const char* source = member_value.get_string_value().c_str();
			char* new_string = strdup(source);
			dest = new_string;
		}
		else if(member_type.is_int()){
			auto dest = (int64_t*)member_ptr;
			*dest = member_value.get_int_value();
		}
		else{
			NOT_IMPLEMENTED_YET();
		}
		return make_wide_return_structptr(reinterpret_cast<const void*>(struct_ptr));
	}
	else{
		NOT_IMPLEMENTED_YET();
	}
}

void floyd_host_function_1035(void* floyd_runtime_ptr, int64_t arg){
	hook(__FUNCTION__, floyd_runtime_ptr, arg);
}

void floyd_host_function_1036(void* floyd_runtime_ptr, int64_t arg){
	hook(__FUNCTION__, floyd_runtime_ptr, arg);
}

void floyd_host_function_1037(void* floyd_runtime_ptr, int64_t arg){
	hook(__FUNCTION__, floyd_runtime_ptr, arg);
}

void floyd_host_function_1038(void* floyd_runtime_ptr, int64_t arg){
	hook(__FUNCTION__, floyd_runtime_ptr, arg);
}

void floyd_host_function_1039(void* floyd_runtime_ptr, int64_t arg){
	hook(__FUNCTION__, floyd_runtime_ptr, arg);
}




///////////////		TEST

void floyd_host_function_2002(void* floyd_runtime_ptr, int64_t arg){
	std::cout << __FUNCTION__ << arg << std::endl;
}

void floyd_host_function_2003(void* floyd_runtime_ptr, int64_t arg){
	std::cout << __FUNCTION__ << arg << std::endl;
}




std::map<std::string, void*> get_host_functions_map2(){

	////////////////////////////////		CORE FUNCTIONS AND HOST FUNCTIONS
	const std::map<std::string, void*> host_functions_map = {
		{ "floyd_funcdef__assert", reinterpret_cast<void *>(&floyd_funcdef__assert) },
		{ "floyd_funcdef__calc_binary_sha1", reinterpret_cast<void *>(&floyd_host_function_1001) },
		{ "floyd_funcdef__calc_string_sha1", reinterpret_cast<void *>(&floyd_host_function_1002) },
		{ "floyd_funcdef__create_directory_branch", reinterpret_cast<void *>(&floyd_host_function_1003) },
		{ "floyd_funcdef__delete_fsentry_deep", reinterpret_cast<void *>(&floyd_host_function_1004) },
		{ "floyd_funcdef__does_fsentry_exist", reinterpret_cast<void *>(&floyd_host_function_1005) },
		{ "floyd_funcdef__erase", reinterpret_cast<void *>(&floyd_host_function__erase) },
		{ "floyd_funcdef__exists", reinterpret_cast<void *>(&floyd_funcdef__exists) },
		{ "floyd_funcdef__filter", reinterpret_cast<void *>(&floyd_host_function_1008) },
		{ "floyd_funcdef__find", reinterpret_cast<void *>(&floyd_funcdef__find) },

		{ "floyd_funcdef__get_fs_environment", reinterpret_cast<void *>(&floyd_host_function_1010) },
		{ "floyd_funcdef__get_fsentries_deep", reinterpret_cast<void *>(&floyd_host_function_1011) },
		{ "floyd_funcdef__get_fsentries_shallow", reinterpret_cast<void *>(&floyd_host_function_1012) },
		{ "floyd_funcdef__get_fsentry_info", reinterpret_cast<void *>(&floyd_host_function_1013) },
		{ "floyd_funcdef__get_json_type", reinterpret_cast<void *>(&floyd_host_function_1014) },
		{ "floyd_funcdef__get_time_of_day", reinterpret_cast<void *>(&floyd_host_function_1015) },
		{ "floyd_funcdef__jsonvalue_to_script", reinterpret_cast<void *>(&floyd_host_function_1016) },
		{ "floyd_funcdef__jsonvalue_to_value", reinterpret_cast<void *>(&floyd_host_function_1017) },
		{ "floyd_funcdef__map", reinterpret_cast<void *>(&floyd_host_function_1018) },
		{ "floyd_funcdef__map_string", reinterpret_cast<void *>(&floyd_host_function_1019) },

		{ "floyd_funcdef__print", reinterpret_cast<void *>(&floyd_funcdef__print) },
		{ "floyd_funcdef__push_back", reinterpret_cast<void *>(&floyd_funcdef__push_back) },
		{ "floyd_funcdef__read_text_file", reinterpret_cast<void *>(&floyd_host_function_1022) },
		{ "floyd_funcdef__reduce", reinterpret_cast<void *>(&floyd_host_function_1023) },
		{ "floyd_funcdef__rename_fsentry", reinterpret_cast<void *>(&floyd_host_function_1024) },
		{ "floyd_funcdef__replace", reinterpret_cast<void *>(&floyd_funcdef__replace) },
		{ "floyd_funcdef__script_to_jsonvalue", reinterpret_cast<void *>(&floyd_host_function_1026) },
		{ "floyd_funcdef__send", reinterpret_cast<void *>(&floyd_host_function_1027) },
		{ "floyd_funcdef__size", reinterpret_cast<void *>(&floyd_funcdef__size) },
		{ "floyd_funcdef__subset", reinterpret_cast<void *>(&floyd_funcdef__subset) },

		{ "floyd_funcdef__supermap", reinterpret_cast<void *>(&floyd_host_function_1030) },
		{ "floyd_funcdef__to_pretty_string", reinterpret_cast<void *>(&floyd_host_function_1031) },
		{ "floyd_funcdef__to_string", reinterpret_cast<void *>(&floyd_host__to_string) },
		{ "floyd_funcdef__typeof", reinterpret_cast<void *>(&floyd_host__typeof) },
		{ "floyd_funcdef__update", reinterpret_cast<void *>(&floyd_funcdef__update) },
		{ "floyd_funcdef__value_to_jsonvalue", reinterpret_cast<void *>(&floyd_host_function_1035) },
		{ "floyd_funcdef__write_text_file", reinterpret_cast<void *>(&floyd_host_function_1036) }
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


}	//	namespace floyd
