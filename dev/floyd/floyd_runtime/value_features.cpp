//
//  value_features.cpp
//  floyd
//
//  Created by Marcus Zetterquist on 2019-08-24.
//  Copyright © 2019 Marcus Zetterquist. All rights reserved.
//

#include "value_features.h"

#include "value_thunking.h"

#include "expression.h"


namespace floyd {



VECTOR_CPPVECTOR_T* unpack_vector_cppvector_arg(const value_backend_t& backend, runtime_value_t arg, runtime_type_t arg_type){
	QUARK_ASSERT(backend.check_invariant());
#if DEBUG
	const auto& type = lookup_type_ref(backend, arg_type);
#endif
	QUARK_ASSERT(type.is_vector());
	QUARK_ASSERT(arg.vector_cppvector_ptr != nullptr);
	QUARK_ASSERT(arg.vector_cppvector_ptr->check_invariant());

	return arg.vector_cppvector_ptr;
}

DICT_CPPMAP_T* unpack_dict_cppmap_arg(const value_backend_t& backend, runtime_value_t arg, runtime_type_t arg_type){
	QUARK_ASSERT(backend.check_invariant());
#if DEBUG
	const auto& type = lookup_type_ref(backend, arg_type);
#endif
	QUARK_ASSERT(type.is_dict());
	QUARK_ASSERT(arg.dict_cppmap_ptr != nullptr);

	QUARK_ASSERT(arg.dict_cppmap_ptr->check_invariant());

	return arg.dict_cppmap_ptr;
}


int compare_values(value_backend_t& backend, int64_t op, const runtime_type_t type, runtime_value_t lhs, runtime_value_t rhs){
	QUARK_ASSERT(backend.check_invariant());

	const auto& value_type = lookup_type_ref(backend, type);

	const auto left_value = from_runtime_value2(backend, lhs, value_type);
	const auto right_value = from_runtime_value2(backend, rhs, value_type);

	const int result = value_t::compare_value_true_deep(left_value, right_value);
//	int result = runtime_compare_value_true_deep((const uint64_t)lhs, (const uint64_t)rhs, vector_type);
	const auto op2 = static_cast<expression_type>(op);
	if(op2 == expression_type::k_comparison_smaller_or_equal){
		return result <= 0 ? 1 : 0;
	}
	else if(op2 == expression_type::k_comparison_smaller){
		return result < 0 ? 1 : 0;
	}
	else if(op2 == expression_type::k_comparison_larger_or_equal){
		return result >= 0 ? 1 : 0;
	}
	else if(op2 == expression_type::k_comparison_larger){
		return result > 0 ? 1 : 0;
	}

	else if(op2 == expression_type::k_logical_equal){
		return result == 0 ? 1 : 0;
	}
	else if(op2 == expression_type::k_logical_nonequal){
		return result != 0 ? 1 : 0;
	}
	else{
		QUARK_ASSERT(false);
		throw std::exception();
	}
}






const runtime_value_t update__string(value_backend_t& backend, runtime_value_t arg0, runtime_type_t arg0_type, runtime_value_t arg1, runtime_type_t arg1_type, runtime_value_t arg2, runtime_type_t arg2_type){
	QUARK_ASSERT(backend.check_invariant());

	const auto& type0 = lookup_type_ref(backend, arg0_type);
	const auto& type1 = lookup_type_ref(backend, arg1_type);
	const auto& type2 = lookup_type_ref(backend, arg2_type);

	QUARK_ASSERT(type1.is_int());
	QUARK_ASSERT(type2.is_int());

	const auto str = from_runtime_string2(backend, arg0);
	const auto index = arg1.int_value;
	const auto new_char = (char)arg2.int_value;

	const auto len = str.size();

	if(index < 0 || index >= len){
		quark::throw_runtime_error("Position argument to update() is outside collection span.");
	}

	auto result = str;
	result[index] = new_char;
	const auto result2 = to_runtime_string2(backend, result);
	return result2;
}

const runtime_value_t update__cppvector(value_backend_t& backend, runtime_value_t arg0, runtime_type_t arg0_type, runtime_value_t arg1, runtime_type_t arg1_type, runtime_value_t arg2, runtime_type_t arg2_type){
	QUARK_ASSERT(backend.check_invariant());

	const auto& type0 = lookup_type_ref(backend, arg0_type);
	const auto& type1 = lookup_type_ref(backend, arg1_type);
	const auto& type2 = lookup_type_ref(backend, arg2_type);

	QUARK_ASSERT(type1.is_int());

	const auto vec = unpack_vector_cppvector_arg(backend, arg0, arg0_type);
	const auto index = arg1.int_value;
	const auto element_itype = lookup_vector_element_itype(backend, itype_t(arg0_type));

	if(index < 0 || index >= vec->get_element_count()){
		quark::throw_runtime_error("Position argument to update() is outside collection span.");
	}

	auto result = alloc_vector_ccpvector2(backend.heap, vec->get_element_count(), vec->get_element_count(), lookup_itype(backend, type0));
	auto dest_ptr = result.vector_cppvector_ptr->get_element_ptr();
	auto source_ptr = vec->get_element_ptr();
	if(is_rc_value(element_itype)){
		retain_value(backend, arg2, element_itype);
		for(int i = 0 ; i < result.vector_cppvector_ptr->get_element_count() ; i++){
			retain_value(backend, source_ptr[i], element_itype);
			dest_ptr[i] = source_ptr[i];
		}

		if(is_rc_value(itype_t(arg0_type))){
			release_deep2(backend, dest_ptr[index], itype_t(arg0_type));
		}
		dest_ptr[index] = arg2;
	}
	else{
		for(int i = 0 ; i < result.vector_cppvector_ptr->get_element_count() ; i++){
			dest_ptr[i] = source_ptr[i];
		}
		dest_ptr[index] = arg2;
	}

	return result;
}

const runtime_value_t update__vector_hamt(value_backend_t& backend, runtime_value_t arg0, runtime_type_t arg0_type, runtime_value_t arg1, runtime_type_t arg1_type, runtime_value_t arg2, runtime_type_t arg2_type){
	QUARK_ASSERT(backend.check_invariant());

	const auto& type0 = lookup_type_ref(backend, arg0_type);
	const auto& type1 = lookup_type_ref(backend, arg1_type);
	const auto& type2 = lookup_type_ref(backend, arg2_type);

	QUARK_ASSERT(type1.is_int());

	const auto vec = arg0.vector_hamt_ptr;
	const auto index = arg1.int_value;
	const auto element_itype = lookup_vector_element_itype(backend, itype_t(arg0_type));

	if(index < 0 || index >= vec->get_element_count()){
		quark::throw_runtime_error("Position argument to update() is outside collection span.");
	}

	if(is_rc_value(element_itype)){
		const auto result = store_immutable(arg0, index, arg2);
		for(int i = 0 ; i < result.vector_hamt_ptr->get_element_count() ; i++){
			auto v = result.vector_hamt_ptr->load_element(i);
			retain_value(backend, v, element_itype);
		}
		return result;
	}
	else{
		return store_immutable(arg0, index, arg2);
	}
}

const runtime_value_t update__dict_cppmap(value_backend_t& backend, runtime_value_t arg0, runtime_type_t arg0_type, runtime_value_t arg1, runtime_type_t arg1_type, runtime_value_t arg2, runtime_type_t arg2_type){
	QUARK_ASSERT(backend.check_invariant());

	const auto& type0 = lookup_type_ref(backend, arg0_type);
	const auto& type1 = lookup_type_ref(backend, arg1_type);
	const auto& type2 = lookup_type_ref(backend, arg2_type);

	QUARK_ASSERT(type1.is_string());

	const auto key = from_runtime_string2(backend, arg1);
	const auto dict = unpack_dict_cppmap_arg(backend, arg0, arg0_type);
	const auto value_type = type0.get_dict_value_type();
	const auto value_itype = lookup_itype(backend, value_type);

	//	Deep copy dict.
	auto dict2 = alloc_dict_cppmap(backend.heap, itype_t(arg0_type));
	dict2.dict_cppmap_ptr->get_map_mut() = dict->get_map();

	dict2.dict_cppmap_ptr->get_map_mut().insert_or_assign(key, arg2);

	if(is_rc_value(value_itype)){
		for(const auto& e: dict2.dict_cppmap_ptr->get_map()){
			retain_value(backend, e.second, value_itype);
		}
	}

	return dict2;
}

const runtime_value_t update__dict_hamt(value_backend_t& backend, runtime_value_t arg0, runtime_type_t arg0_type, runtime_value_t arg1, runtime_type_t arg1_type, runtime_value_t arg2, runtime_type_t arg2_type){
	QUARK_ASSERT(backend.check_invariant());

	const auto& type0 = lookup_type_ref(backend, arg0_type);
	const auto& type1 = lookup_type_ref(backend, arg1_type);
	const auto& type2 = lookup_type_ref(backend, arg2_type);

	QUARK_ASSERT(type1.is_string());

	const auto key = from_runtime_string2(backend, arg1);
	const auto dict = arg0.dict_hamt_ptr;
	const auto value_type = type0.get_dict_value_type();
	const auto value_itype = lookup_itype(backend, value_type);

	//	Deep copy dict.
	auto dict2 = alloc_dict_hamt(backend.heap, itype_t(arg1_type));
	dict2.dict_hamt_ptr->get_map_mut() = dict->get_map();

	dict2.dict_hamt_ptr->get_map_mut() = dict2.dict_hamt_ptr->get_map_mut().set(key, arg2);

	if(is_rc_value(value_itype)){
		for(const auto& e: dict2.dict_hamt_ptr->get_map()){
			retain_value(backend, e.second, value_itype);
		}
	}

	return dict2;
}






const runtime_value_t subset__string(value_backend_t& backend, runtime_value_t arg0, runtime_type_t arg0_type, uint64_t start, uint64_t end){
	QUARK_ASSERT(backend.check_invariant());

	if(start < 0 || end < 0){
		quark::throw_runtime_error("subset() requires start and end to be non-negative.");
	}

	const auto& type0 = lookup_type_ref(backend, arg0_type);
	const auto value = from_runtime_string2(backend, arg0);
	const auto len = get_vec_string_size(arg0);
	const auto end2 = std::min(end, len);
	const auto start2 = std::min(start, end2);
	const auto len2 = end2 - start2;

	const auto s = std::string(&value[start2], &value[start2 + len2]);
	const auto result = to_runtime_string2(backend, s);
	return result;
}

const runtime_value_t subset__cppvector(value_backend_t& backend, runtime_value_t arg0, runtime_type_t arg0_type, uint64_t start, uint64_t end){
	QUARK_ASSERT(backend.check_invariant());

	if(start < 0 || end < 0){
		quark::throw_runtime_error("subset() requires start and end to be non-negative.");
	}

	const auto& type0 = lookup_type_ref(backend, arg0_type);
	const auto vec = unpack_vector_cppvector_arg(backend, arg0, arg0_type);
	const auto end2 = std::min(end, vec->get_element_count());
	const auto start2 = std::min(start, end2);
	const auto len2 = end2 - start2;
	if(len2 >= INT64_MAX){
		throw std::exception();
	}

	const auto element_itype = lookup_vector_element_itype(backend, itype_t(arg0_type));

	auto vec2 = alloc_vector_ccpvector2(backend.heap, len2, len2, lookup_itype(backend, type0));
	if(is_rc_value(element_itype)){
		for(int i = 0 ; i < len2 ; i++){
			const auto& value = vec->get_element_ptr()[start2 + i];
			vec2.vector_cppvector_ptr->get_element_ptr()[i] = value;
			retain_value(backend, value, element_itype);
		}
	}
	else{
		for(int i = 0 ; i < len2 ; i++){
			const auto& value = vec->get_element_ptr()[start2 + i];
			vec2.vector_cppvector_ptr->get_element_ptr()[i] = value;
		}
	}
	return vec2;
}

const runtime_value_t subset__hamt(value_backend_t& backend, runtime_value_t arg0, runtime_type_t arg0_type, uint64_t start, uint64_t end){
	QUARK_ASSERT(backend.check_invariant());

	if(start < 0 || end < 0){
		quark::throw_runtime_error("subset() requires start and end to be non-negative.");
	}

	const auto& type0 = lookup_type_ref(backend, arg0_type);
	const auto& vec = *arg0.vector_hamt_ptr;
	const auto end2 = std::min(end, vec.get_element_count());
	const auto start2 = std::min(start, end2);
	const auto len2 = end2 - start2;
	if(len2 >= INT64_MAX){
		throw std::exception();
	}

	const auto element_itype = lookup_vector_element_itype(backend, itype_t(arg0_type));

	auto vec2 = alloc_vector_hamt(backend.heap, len2, len2, itype_t(arg0_type));
	if(is_rc_value(element_itype)){
		for(int i = 0 ; i < len2 ; i++){
			const auto& value = vec.load_element(start2 + i);
			vec2.vector_hamt_ptr->store_mutate(i, value);
			retain_value(backend, value, element_itype);
		}
	}
	else{
		for(int i = 0 ; i < len2 ; i++){
			const auto& value = vec.load_element(start2 + i);
			vec2.vector_hamt_ptr->store_mutate(i, value);
		}
	}
	return vec2;
}












static void check_replace_indexes(std::size_t start, std::size_t end){
	if(start < 0 || end < 0){
		quark::throw_runtime_error("replace() requires start and end to be non-negative.");
	}
	if(start > end){
		quark::throw_runtime_error("replace() requires start <= end.");
	}
}

const runtime_value_t replace__string(value_backend_t& backend, runtime_value_t arg0, runtime_type_t arg0_type, size_t start, size_t end, runtime_value_t arg3, runtime_type_t arg3_type){
	QUARK_ASSERT(backend.check_invariant());

	check_replace_indexes(start, end);
	const auto& type0 = lookup_type_ref(backend, arg0_type);
	const auto& type3 = lookup_type_ref(backend, arg3_type);

	QUARK_ASSERT(type3 == type0);

	const auto s = from_runtime_string2(backend, arg0);
	const auto replace = from_runtime_string2(backend, arg3);

	auto s_len = s.size();
	auto replace_len = replace.size();

	auto end2 = std::min(end, s_len);
	auto start2 = std::min(start, end2);

	const std::size_t len2 = start2 + replace_len + (s_len - end2);
	auto s2 = reinterpret_cast<char*>(std::malloc(len2 + 1));
	std::memcpy(&s2[0], &s[0], start2);
	std::memcpy(&s2[start2], &replace[0], replace_len);
	std::memcpy(&s2[start2 + replace_len], &s[end2], s_len - end2);

	const std::string ret(s2, &s2[start2 + replace_len + (s_len - end2)]);
	const auto result2 = to_runtime_string2(backend, ret);
	return result2;
}

const runtime_value_t replace__cppvector(value_backend_t& backend, runtime_value_t arg0, runtime_type_t arg0_type, size_t start, size_t end, runtime_value_t arg3, runtime_type_t arg3_type){
	QUARK_ASSERT(backend.check_invariant());

	check_replace_indexes(start, end);

	const auto& type0 = lookup_type_ref(backend, arg0_type);
	QUARK_ASSERT(lookup_type_ref(backend, arg3_type) == type0);

	const auto element_itype = lookup_vector_element_itype(backend, itype_t(arg0_type));

	const auto vec = unpack_vector_cppvector_arg(backend, arg0, arg0_type);
	const auto replace_vec = unpack_vector_cppvector_arg(backend, arg3, arg3_type);

	auto end2 = std::min(end, (size_t)vec->get_element_count());
	auto start2 = std::min(start, end2);

	const auto section1_len = start2;
	const auto section2_len = replace_vec->get_element_count();
	const auto section3_len = vec->get_element_count() - end2;

	const auto len2 = section1_len + section2_len + section3_len;
	auto vec2 = alloc_vector_ccpvector2(backend.heap, len2, len2, lookup_itype(backend, type0));
	copy_elements(&vec2.vector_cppvector_ptr->get_element_ptr()[0], &vec->get_element_ptr()[0], section1_len);
	copy_elements(&vec2.vector_cppvector_ptr->get_element_ptr()[section1_len], &replace_vec->get_element_ptr()[0], section2_len);
	copy_elements(&vec2.vector_cppvector_ptr->get_element_ptr()[section1_len + section2_len], &vec->get_element_ptr()[end2], section3_len);

	if(is_rc_value(element_itype)){
		for(int i = 0 ; i < len2 ; i++){
			retain_value(backend, vec2.vector_cppvector_ptr->get_element_ptr()[i], element_itype);
		}
	}

	return vec2;
}
const runtime_value_t replace__hamt(value_backend_t& backend, runtime_value_t arg0, runtime_type_t arg0_type, size_t start, size_t end, runtime_value_t arg3, runtime_type_t arg3_type){
	QUARK_ASSERT(backend.check_invariant());

	check_replace_indexes(start, end);

	const auto& type0 = lookup_type_ref(backend, arg0_type);
	QUARK_ASSERT(lookup_type_ref(backend, arg3_type) == type0);

	const auto element_itype = lookup_vector_element_itype(backend, itype_t(arg0_type));

	const auto& vec = *arg0.vector_hamt_ptr;
	const auto& replace_vec = *arg3.vector_hamt_ptr;

	auto end2 = std::min(end, (size_t)vec.get_element_count());
	auto start2 = std::min(start, end2);

	const auto section1_len = start2;
	const auto section2_len = replace_vec.get_element_count();
	const auto section3_len = vec.get_element_count() - end2;

	const auto len2 = section1_len + section2_len + section3_len;
	auto vec2 = alloc_vector_hamt(backend.heap, len2, len2, itype_t(arg0_type));
	for(size_t i = 0 ; i < section1_len ; i++){
		const auto& value = vec.load_element(0 + i);
		vec2.vector_hamt_ptr->store_mutate(0 + i, value);
	}
	for(size_t i = 0 ; i < section2_len ; i++){
		const auto& value = replace_vec.load_element(0 + i);
		vec2.vector_hamt_ptr->store_mutate(section1_len + i, value);
	}
	for(size_t i = 0 ; i < section3_len ; i++){
		const auto& value = vec.load_element(end2 + i);
		vec2.vector_hamt_ptr->store_mutate(section1_len + section2_len + i, value);
	}

	if(is_rc_value(element_itype)){
		for(int i = 0 ; i < len2 ; i++){
			retain_value(backend, vec2.vector_hamt_ptr->load_element(i), element_itype);
		}
	}

	return vec2;
}









int64_t find__string(value_backend_t& backend, runtime_value_t arg0, runtime_type_t arg0_type, const runtime_value_t arg1, runtime_type_t arg1_type){
	QUARK_ASSERT(backend.check_invariant());

	const auto& type0 = lookup_type_ref(backend, arg0_type);
	const auto& type1 = lookup_type_ref(backend, arg1_type);

	QUARK_ASSERT(type1.is_string());

	const auto str = from_runtime_string2(backend, arg0);
	const auto wanted2 = from_runtime_string2(backend, arg1);
	const auto pos = str.find(wanted2);
	const auto result = pos == std::string::npos ? -1 : static_cast<int64_t>(pos);
	return result;
}
int64_t find__cppvector(value_backend_t& backend, runtime_value_t arg0, runtime_type_t arg0_type, const runtime_value_t arg1, runtime_type_t arg1_type){
	QUARK_ASSERT(backend.check_invariant());

	const auto& type0 = lookup_type_ref(backend, arg0_type);
	const auto& type1 = lookup_type_ref(backend, arg1_type);

	QUARK_ASSERT(type1 == type0.get_vector_element_type());

	const auto vec = unpack_vector_cppvector_arg(backend, arg0, arg0_type);

//		auto it = std::find_if(function_defs.begin(), function_defs.end(), [&] (const function_def_t& e) { return e.def_name == function_name; } );
	const auto it = std::find_if(
		vec->get_element_ptr(),
		vec->get_element_ptr() + vec->get_element_count(),
		[&] (const runtime_value_t& e) {
			return compare_values(backend, static_cast<int64_t>(expression_type::k_logical_equal), arg1_type, e, arg1) == 1;
		}
	);
	if(it == vec->get_element_ptr() + vec->get_element_count()){
		return -1;
	}
	else{
		const auto pos = it - vec->get_element_ptr();
		return pos;
	}
}
int64_t find__hamt(value_backend_t& backend, runtime_value_t arg0, runtime_type_t arg0_type, const runtime_value_t arg1, runtime_type_t arg1_type){
	QUARK_ASSERT(backend.check_invariant());

	const auto& type0 = lookup_type_ref(backend, arg0_type);
	const auto& type1 = lookup_type_ref(backend, arg1_type);

	QUARK_ASSERT(type1 == type0.get_vector_element_type());

	const auto& vec = *arg0.vector_hamt_ptr;
	QUARK_ASSERT(vec.check_invariant());

	const auto count = vec.get_element_count();
	int64_t index = 0;
	while(index < count && compare_values(backend, static_cast<int64_t>(expression_type::k_logical_equal), arg1_type, vec.load_element(index), arg1) != 1){
		index++;
	}
	if(index == count){
		return -1;
	}
	else{
		return index;
	}
}






runtime_value_t get_keys__cppmap_cppvector(value_backend_t& backend, runtime_value_t dict_value, runtime_type_t dict_type){
	QUARK_ASSERT(backend.check_invariant());

	const auto& type0 = lookup_type_ref(backend, dict_type);

	QUARK_ASSERT(type0.is_dict());

	const auto& dict = unpack_dict_cppmap_arg(backend, dict_value, dict_type);
	const auto& m = dict->get_map();
	const auto count = (uint64_t)m.size();

	auto result_vec = alloc_vector_ccpvector2(backend.heap, count, count, lookup_itype(backend, typeid_t::make_vector(typeid_t::make_string())));

	int index = 0;
	for(const auto& e: m){
		//	Notice that the internal representation of dictionary keys are std::string, not floyd-strings,
		//	so we need to create new key-strings from scratch.
		const auto key = to_runtime_string2(backend, e.first);
		result_vec.vector_cppvector_ptr->get_element_ptr()[index] = key;
		index++;
	}
	return result_vec;
}
runtime_value_t get_keys__cppmap_hamt(value_backend_t& backend, runtime_value_t dict_value, runtime_type_t dict_type){
	QUARK_ASSERT(backend.check_invariant());

	const auto& type0 = lookup_type_ref(backend, dict_type);

	QUARK_ASSERT(type0.is_dict());

	const auto& dict = unpack_dict_cppmap_arg(backend, dict_value, dict_type);
	const auto& m = dict->get_map();
	const auto count = (uint64_t)m.size();

	auto result_vec = alloc_vector_hamt(backend.heap, count, count, lookup_itype(backend, typeid_t::make_vector(typeid_t::make_string())));

	int index = 0;
	for(const auto& e: m){
		//	Notice that the internal representation of dictionary keys are std::string, not floyd-strings,
		//	so we need to create new key-strings from scratch.
		const auto key = to_runtime_string2(backend, e.first);
		result_vec.vector_hamt_ptr->store_mutate(index, key);
		index++;
	}
	return result_vec;
}


runtime_value_t get_keys__hamtmap_cppvector(value_backend_t& backend, runtime_value_t dict_value, runtime_type_t dict_type){
	QUARK_ASSERT(backend.check_invariant());

	const auto& type0 = lookup_type_ref(backend, dict_type);

	QUARK_ASSERT(type0.is_dict());

	const auto& dict = dict_value.dict_hamt_ptr;
	const auto& m = dict->get_map();
	const auto count = (uint64_t)m.size();

	auto result_vec = alloc_vector_ccpvector2(backend.heap, count, count, lookup_itype(backend, typeid_t::make_vector(typeid_t::make_string())));

	int index = 0;
	for(const auto& e: m){
		//	Notice that the internal representation of dictionary keys are std::string, not floyd-strings,
		//	so we need to create new key-strings from scratch.
		const auto key = to_runtime_string2(backend, e.first);
		result_vec.vector_cppvector_ptr->get_element_ptr()[index] = key;
		index++;
	}
	return result_vec;
}
runtime_value_t get_keys__hamtmap_hamt(value_backend_t& backend, runtime_value_t dict_value, runtime_type_t dict_type){
	QUARK_ASSERT(backend.check_invariant());

	const auto& type0 = lookup_type_ref(backend, dict_type);

	QUARK_ASSERT(type0.is_dict());

	const auto& dict = dict_value.dict_hamt_ptr;
	const auto& m = dict->get_map();
	const auto count = (uint64_t)m.size();

	auto result_vec = alloc_vector_hamt(backend.heap, count, count, lookup_itype(backend, typeid_t::make_vector(typeid_t::make_string())));

	int index = 0;
	for(const auto& e: m){
		//	Notice that the internal representation of dictionary keys are std::string, not floyd-strings,
		//	so we need to create new key-strings from scratch.
		const auto key = to_runtime_string2(backend, e.first);
		result_vec.vector_hamt_ptr->store_mutate(index, key);
		index++;
	}
	return result_vec;
}







int64_t analyse_samples(const int64_t* samples, int64_t count){
	const auto use_ptr = &samples[1];
	const auto use_count = count - 1;

	auto smallest_acc = use_ptr[0];
	auto largest_acc = use_ptr[0];
	for(int64_t i = 0 ; i < use_count ; i++){
		const auto value = use_ptr[i];
		if(value < smallest_acc){
			smallest_acc = value;
		}
		if(value > largest_acc){
			largest_acc = value;
		}
	}
//	std::cout << "smallest: " << smallest_acc << std::endl;
//	std::cout << "largest: " << largest_acc << std::endl;
	return smallest_acc;
}







runtime_value_t concat_strings(value_backend_t& backend, const runtime_value_t& lhs, const runtime_value_t& rhs){
	QUARK_ASSERT(backend.check_invariant());
	QUARK_ASSERT(lhs.check_invariant());
	QUARK_ASSERT(rhs.check_invariant());

	const auto result = from_runtime_string2(backend, lhs) + from_runtime_string2(backend, rhs);
	return to_runtime_string2(backend, result);
}

runtime_value_t concat_vector_cppvector(value_backend_t& backend, const itype_t& type, const runtime_value_t& lhs, const runtime_value_t& rhs){
	QUARK_ASSERT(backend.check_invariant());
	QUARK_ASSERT(type.check_invariant());
	QUARK_ASSERT(lhs.check_invariant());
	QUARK_ASSERT(rhs.check_invariant());

	const auto count2 = lhs.vector_cppvector_ptr->get_element_count() + rhs.vector_cppvector_ptr->get_element_count();

	auto result = alloc_vector_ccpvector2(backend.heap, count2, count2, type);

	//??? warning: assumes element = allocation.

	const auto element_itype = lookup_vector_element_itype(backend, type);

	auto dest_ptr = result.vector_cppvector_ptr->get_element_ptr();
	auto dest_ptr2 = dest_ptr + lhs.vector_cppvector_ptr->get_element_count();
	auto lhs_ptr = lhs.vector_cppvector_ptr->get_element_ptr();
	auto rhs_ptr = rhs.vector_cppvector_ptr->get_element_ptr();

	if(is_rc_value(element_itype)){
		for(int i = 0 ; i < lhs.vector_cppvector_ptr->get_element_count() ; i++){
			retain_value(backend, lhs_ptr[i], element_itype);
			dest_ptr[i] = lhs_ptr[i];
		}
		for(int i = 0 ; i < rhs.vector_cppvector_ptr->get_element_count() ; i++){
			retain_value(backend, rhs_ptr[i], element_itype);
			dest_ptr2[i] = rhs_ptr[i];
		}
	}
	else{
		for(int i = 0 ; i < lhs.vector_cppvector_ptr->get_element_count() ; i++){
			dest_ptr[i] = lhs_ptr[i];
		}
		for(int i = 0 ; i < rhs.vector_cppvector_ptr->get_element_count() ; i++){
			dest_ptr2[i] = rhs_ptr[i];
		}
	}
	return result;
}

runtime_value_t concat_vector_hamt(value_backend_t& backend, const itype_t& type, const runtime_value_t& lhs, const runtime_value_t& rhs){
	QUARK_ASSERT(backend.check_invariant());
	QUARK_ASSERT(type.check_invariant());
	QUARK_ASSERT(lhs.check_invariant());
	QUARK_ASSERT(rhs.check_invariant());

	const auto lhs_count = lhs.vector_hamt_ptr->get_element_count();
	const auto rhs_count = rhs.vector_hamt_ptr->get_element_count();

	const auto count2 = lhs_count + rhs_count;

	auto result = alloc_vector_hamt(backend.heap, count2, count2, type);

	//??? warning: assumes element = allocation.

	const auto element_itype = lookup_vector_element_itype(backend, type);

	//??? Causes a full path copy for EACH ELEMENT = slow. better to make new hamt in one go.
	if(is_rc_value(element_itype)){
		for(int i = 0 ; i < lhs_count ; i++){
			auto value = lhs.vector_hamt_ptr->load_element(i);
			retain_value(backend, value, element_itype);
			result.vector_hamt_ptr->store_mutate(i, value);
		}
		for(int i = 0 ; i < rhs_count ; i++){
			auto value = rhs.vector_hamt_ptr->load_element(i);
			retain_value(backend, value, element_itype);
			result.vector_hamt_ptr->store_mutate(lhs_count + i, value);
		}
	}
	else{
		for(int i = 0 ; i < lhs_count ; i++){
			auto value = lhs.vector_hamt_ptr->load_element(i);
			result.vector_hamt_ptr->store_mutate(i, value);
		}
		for(int i = 0 ; i < rhs_count ; i++){
			auto value = rhs.vector_hamt_ptr->load_element(i);
			result.vector_hamt_ptr->store_mutate(lhs_count + i, value);
		}
	}
	return result;
}

}	// floyd
