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



const function_def_t& find_function_def2(const std::vector<function_def_t>& function_defs, const std::string& function_name){
	auto it = std::find_if(function_defs.begin(), function_defs.end(), [&] (const function_def_t& e) { return e.def_name == function_name; } );
	QUARK_ASSERT(it != function_defs.end());

	QUARK_ASSERT(it->llvm_f != nullptr);
	return *it;
}




//////////////////////////////////////////		COMPARE
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////



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
	QUARK_ASSERT(check_invariant_vector(left));
	QUARK_ASSERT(check_invariant_vector(right));

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
	QUARK_ASSERT(check_invariant_vector(left));
	QUARK_ASSERT(check_invariant_vector(right));

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
	QUARK_ASSERT(check_invariant_vector(left));
	QUARK_ASSERT(check_invariant_vector(right));

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
	QUARK_ASSERT(check_invariant_vector(left));
	QUARK_ASSERT(check_invariant_vector(right));

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
	int64_t return_encoded = (*function_ptr)(&ee, "?dummy arg to main()?");

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



VEC_T* unpack_vec_arg(const llvm_execution_engine_t& r, int64_t arg_value, int64_t arg_type){
	const auto type = unpack_itype(r, arg_type);
	QUARK_ASSERT(type.is_vector());
	const auto vec = (VEC_T*)arg_value;
	QUARK_ASSERT(vec != nullptr);

	QUARK_ASSERT(check_invariant_vector(*vec));

	return vec;
}



value_t llvm_global_to_value(const llvm_execution_engine_t& runtime, const void* global_ptr, const typeid_t& type){
	QUARK_ASSERT(runtime.check_invariant());
	QUARK_ASSERT(global_ptr != nullptr);
	QUARK_ASSERT(type.check_invariant());

	return llvm_valueptr_to_value(runtime, global_ptr, type);
}




//??? Lose concept of "encoded" value ASAP.

value_t runtime_llvm_to_value(const llvm_execution_engine_t& runtime, const uint64_t encoded_value, const typeid_t& type){
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
		const auto type1 = unpack_itype(runtime, encoded_value);
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
			const auto member_ptr = reinterpret_cast<int64_t*>(encoded_value + offset);
			const auto member_value = llvm_valueptr_to_value(runtime, member_ptr, e._type);
			members.push_back(member_value);
			member_index++;
		}
		return value_t::make_struct_value(type, members);
	}
	else if(type.is_vector()){
		const auto element_type = type.get_vector_element_type();
		const auto vec = unpack_vec_arg(runtime, encoded_value, pack_itype(runtime, type));

		std::vector<value_t> elements;
		const int count = vec->element_count;
		for(int i = 0 ; i < count ; i++){
			const auto value_encoded = vec->element_ptr[i];
			const auto value = runtime_llvm_to_value(runtime, value_encoded, element_type);
			elements.push_back(value);
		}
		const auto val = value_t::make_vector_value(typeid_t::make_string(), elements);
		return val;
	}
	else if(type.is_dict()){
		NOT_IMPLEMENTED_YET();
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




typeid_t unpack_itype(const llvm_execution_engine_t& runtime, int64_t itype){
	QUARK_ASSERT(runtime.check_invariant());

	const itype_t t(static_cast<uint32_t>(itype));
	return lookup_type(runtime.type_interner, t);
}

int64_t pack_itype(const llvm_execution_engine_t& runtime, const typeid_t& type){
	QUARK_ASSERT(runtime.check_invariant());
	QUARK_ASSERT(type.check_invariant());

	return lookup_itype(runtime.type_interner, type).itype;
}



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
		const auto type_value = unpack_itype(runtime, value);
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
			const auto member_ptr = reinterpret_cast<const int64_t*>(struct_ptr + member_offset);
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

void hook(const std::string& s, void* floyd_runtime_ptr, int64_t arg){
	std:: cout << s << arg << " arg: " << std::endl;
	auto& r = get_floyd_runtime(floyd_runtime_ptr);
	throw std::runtime_error("HOST FUNCTION NOT IMPLEMENTED FOR LLVM");
}



std::string gen_to_string(llvm_execution_engine_t& runtime, int64_t arg_value, int64_t arg_type){
	QUARK_ASSERT(runtime.check_invariant());

	const auto type = unpack_itype(runtime, arg_type);

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




}	//	namespace floyd
