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



VECTOR_CARRAY_T* unpack_vector_carray_arg(const value_backend_t& backend, runtime_value_t arg, runtime_type_t arg_type){
	QUARK_ASSERT(backend.check_invariant());
#if DEBUG
	const auto& type = lookup_type_ref(backend, arg_type);
#endif
	QUARK_ASSERT(peek2(backend.types, type).is_vector());
	QUARK_ASSERT(arg.vector_carray_ptr != nullptr);
	QUARK_ASSERT(arg.vector_carray_ptr->check_invariant());

	return arg.vector_carray_ptr;
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







const runtime_value_t update__vector_carray(value_backend_t& backend, runtime_value_t coll_value, runtime_type_t coll_type, runtime_value_t index, runtime_value_t value){
	QUARK_ASSERT(backend.check_invariant());

	const auto vec = unpack_vector_carray_arg(backend, coll_value, coll_type);
	const auto index2 = index.int_value;
	const auto element_itype = lookup_vector_element_type(backend, type_t(coll_type));

	if(index2 < 0 || index2 >= vec->get_element_count()){
		quark::throw_runtime_error("Position argument to update() is outside collection span.");
	}

	auto result = alloc_vector_carray(backend.heap, vec->get_element_count(), vec->get_element_count(), type_t(coll_type));
	auto dest_ptr = result.vector_carray_ptr->get_element_ptr();
	auto source_ptr = vec->get_element_ptr();
	if(is_rc_value(peek2(backend.types, element_itype))){
		retain_value(backend, value, element_itype);
		for(int i = 0 ; i < result.vector_carray_ptr->get_element_count() ; i++){
			retain_value(backend, source_ptr[i], element_itype);
			dest_ptr[i] = source_ptr[i];
		}

		if(is_rc_value(peek2(backend.types, type_t(coll_type)))){
			release_value(backend, dest_ptr[index2], type_t(coll_type));
		}
		dest_ptr[index2] = value;
	}
	else{
		for(int i = 0 ; i < result.vector_carray_ptr->get_element_count() ; i++){
			dest_ptr[i] = source_ptr[i];
		}
		dest_ptr[index2] = value;
	}

	return result;
}


const runtime_value_t update__dict_cppmap(value_backend_t& backend, runtime_value_t coll_value, runtime_type_t coll_type, runtime_value_t key_value, runtime_value_t value){
	QUARK_ASSERT(backend.check_invariant());

	//??? move to compile time
	const auto& type0 = lookup_type_ref(backend, coll_type);

	const auto key = from_runtime_string2(backend, key_value);
	const auto dict = unpack_dict_cppmap_arg(backend, coll_value, coll_type);

	//??? compile time
	const auto value_itype = type0.get_dict_value_type(backend.types);

	//	Deep copy dict.
	auto dict2 = alloc_dict_cppmap(backend.heap, type_t(coll_type));
	dict2.dict_cppmap_ptr->get_map_mut() = dict->get_map();

	dict2.dict_cppmap_ptr->get_map_mut().insert_or_assign(key, value);

	if(is_rc_value(peek2(backend.types, value_itype))){
		for(const auto& e: dict2.dict_cppmap_ptr->get_map()){
			retain_value(backend, e.second, value_itype);
		}
	}

	return dict2;
}

const runtime_value_t update__dict_hamt(value_backend_t& backend, runtime_value_t coll_value, runtime_type_t coll_type, runtime_value_t key_value, runtime_value_t value){
	QUARK_ASSERT(backend.check_invariant());

	//??? move to compile time
	const auto& type0 = lookup_type_ref(backend, coll_type);

	const auto key = from_runtime_string2(backend, key_value);
	const auto dict = coll_value.dict_hamt_ptr;

	//??? compile time
	const auto value_itype = type0.get_dict_value_type(backend.types);

	//	Deep copy dict.
	auto dict2 = alloc_dict_hamt(backend.heap, type_t(coll_type));
	dict2.dict_hamt_ptr->get_map_mut() = dict->get_map();

	dict2.dict_hamt_ptr->get_map_mut() = dict2.dict_hamt_ptr->get_map_mut().set(key, value);

	if(is_rc_value(peek2(backend.types, value_itype))){
		for(const auto& e: dict2.dict_hamt_ptr->get_map()){
			retain_value(backend, e.second, value_itype);
		}
	}

	return dict2;
}






const runtime_value_t subset__string(value_backend_t& backend, runtime_value_t coll_value, runtime_type_t coll_type, uint64_t start, uint64_t end){
	QUARK_ASSERT(backend.check_invariant());

	if(start < 0 || end < 0){
		quark::throw_runtime_error("subset() requires start and end to be non-negative.");
	}

	const auto value = from_runtime_string2(backend, coll_value);
	const auto len = get_vec_string_size(coll_value);
	const auto end2 = std::min(end, len);
	const auto start2 = std::min(start, end2);
	const auto len2 = end2 - start2;

	const auto s = std::string(&value[start2], &value[start2 + len2]);
	const auto result = to_runtime_string2(backend, s);
	return result;
}

const runtime_value_t subset__carray(value_backend_t& backend, runtime_value_t coll_value, runtime_type_t coll_type, uint64_t start, uint64_t end){
	QUARK_ASSERT(backend.check_invariant());

	if(start < 0 || end < 0){
		quark::throw_runtime_error("subset() requires start and end to be non-negative.");
	}

	const auto& type0 = lookup_type_ref(backend, coll_type);
	const auto vec = unpack_vector_carray_arg(backend, coll_value, coll_type);
	const auto end2 = std::min(end, vec->get_element_count());
	const auto start2 = std::min(start, end2);
	const auto len2 = end2 - start2;
	if(len2 >= INT64_MAX){
		throw std::exception();
	}

	const auto element_itype = lookup_vector_element_type(backend, type_t(coll_type));

	auto vec2 = alloc_vector_carray(backend.heap, len2, len2, type0);
	if(is_rc_value(peek2(backend.types, element_itype))){
		for(int i = 0 ; i < len2 ; i++){
			const auto& value = vec->get_element_ptr()[start2 + i];
			vec2.vector_carray_ptr->get_element_ptr()[i] = value;
			retain_value(backend, value, element_itype);
		}
	}
	else{
		for(int i = 0 ; i < len2 ; i++){
			const auto& value = vec->get_element_ptr()[start2 + i];
			vec2.vector_carray_ptr->get_element_ptr()[i] = value;
		}
	}
	return vec2;
}

const runtime_value_t subset__hamt(value_backend_t& backend, runtime_value_t coll_value, runtime_type_t coll_type, uint64_t start, uint64_t end){
	QUARK_ASSERT(backend.check_invariant());

	if(start < 0 || end < 0){
		quark::throw_runtime_error("subset() requires start and end to be non-negative.");
	}

	const auto& vec = *coll_value.vector_hamt_ptr;
	const auto end2 = std::min(end, vec.get_element_count());
	const auto start2 = std::min(start, end2);
	const auto len2 = end2 - start2;
	if(len2 >= INT64_MAX){
		throw std::exception();
	}

	const auto element_itype = lookup_vector_element_type(backend, type_t(coll_type));

	auto vec2 = alloc_vector_hamt(backend.heap, len2, len2, type_t(coll_type));
	if(is_rc_value(peek2(backend.types, element_itype))){
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

const runtime_value_t replace__string(value_backend_t& backend, runtime_value_t coll_value, runtime_type_t coll_type, size_t start, size_t end, runtime_value_t replacement_value, runtime_type_t replacement_type){
	QUARK_ASSERT(backend.check_invariant());

	check_replace_indexes(start, end);
	const auto& type0 = lookup_type_ref(backend, coll_type);
	const auto& type3 = lookup_type_ref(backend, replacement_type);

	QUARK_ASSERT(type3 == type0);

	const auto s = from_runtime_string2(backend, coll_value);
	const auto replace = from_runtime_string2(backend, replacement_value);

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

const runtime_value_t replace__carray(value_backend_t& backend, runtime_value_t coll_value, runtime_type_t coll_type, size_t start, size_t end, runtime_value_t replacement_value, runtime_type_t replacement_type){
	QUARK_ASSERT(backend.check_invariant());

	check_replace_indexes(start, end);

	const auto& type0 = lookup_type_ref(backend, coll_type);
	QUARK_ASSERT(lookup_type_ref(backend, replacement_type) == type0);

	const auto element_itype = lookup_vector_element_type(backend, type_t(coll_type));

	const auto vec = unpack_vector_carray_arg(backend, coll_value, coll_type);
	const auto replace_vec = unpack_vector_carray_arg(backend, replacement_value, replacement_type);

	auto end2 = std::min(end, (size_t)vec->get_element_count());
	auto start2 = std::min(start, end2);

	const auto section1_len = start2;
	const auto section2_len = replace_vec->get_element_count();
	const auto section3_len = vec->get_element_count() - end2;

	const auto len2 = section1_len + section2_len + section3_len;
	auto vec2 = alloc_vector_carray(backend.heap, len2, len2, type0);
	copy_elements(&vec2.vector_carray_ptr->get_element_ptr()[0], &vec->get_element_ptr()[0], section1_len);
	copy_elements(&vec2.vector_carray_ptr->get_element_ptr()[section1_len], &replace_vec->get_element_ptr()[0], section2_len);
	copy_elements(&vec2.vector_carray_ptr->get_element_ptr()[section1_len + section2_len], &vec->get_element_ptr()[end2], section3_len);

	if(is_rc_value(peek2(backend.types, element_itype))){
		for(int i = 0 ; i < len2 ; i++){
			retain_value(backend, vec2.vector_carray_ptr->get_element_ptr()[i], element_itype);
		}
	}

	return vec2;
}
const runtime_value_t replace__hamt(value_backend_t& backend, runtime_value_t coll_value, runtime_type_t coll_type, size_t start, size_t end, runtime_value_t replacement_value, runtime_type_t replacement_type){
	QUARK_ASSERT(backend.check_invariant());

	check_replace_indexes(start, end);

	const auto& type0 = lookup_type_ref(backend, coll_type);
	QUARK_ASSERT(lookup_type_ref(backend, replacement_type) == type0);

	const auto element_itype = lookup_vector_element_type(backend, type_t(coll_type));

	const auto& vec = *coll_value.vector_hamt_ptr;
	const auto& replace_vec = *replacement_value.vector_hamt_ptr;

	auto end2 = std::min(end, (size_t)vec.get_element_count());
	auto start2 = std::min(start, end2);

	const auto section1_len = start2;
	const auto section2_len = replace_vec.get_element_count();
	const auto section3_len = vec.get_element_count() - end2;

	const auto len2 = section1_len + section2_len + section3_len;
	auto vec2 = alloc_vector_hamt(backend.heap, len2, len2, type_t(coll_type));
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

	if(is_rc_value(peek2(backend.types, element_itype))){
		for(int i = 0 ; i < len2 ; i++){
			retain_value(backend, vec2.vector_hamt_ptr->load_element(i), element_itype);
		}
	}

	return vec2;
}









int64_t find__string(value_backend_t& backend, runtime_value_t coll_value, runtime_type_t coll_type, const runtime_value_t value, runtime_type_t value_type){
	QUARK_ASSERT(backend.check_invariant());

	const auto& type1 = lookup_type_ref(backend, value_type);

	QUARK_ASSERT(type1.is_string());

	const auto str = from_runtime_string2(backend, coll_value);
	const auto wanted2 = from_runtime_string2(backend, value);
	const auto pos = str.find(wanted2);
	const auto result = pos == std::string::npos ? -1 : static_cast<int64_t>(pos);
	return result;
}
int64_t find__carray(value_backend_t& backend, runtime_value_t coll_value, runtime_type_t coll_type, const runtime_value_t value, runtime_type_t value_type){
	QUARK_ASSERT(backend.check_invariant());

	const auto& type0 = lookup_type_ref(backend, coll_type);
	const auto& type1 = lookup_type_ref(backend, value_type);

	QUARK_ASSERT(type1 == peek2(backend.types, type0).get_vector_element_type(backend.types));

	const auto vec = unpack_vector_carray_arg(backend, coll_value, coll_type);

//		auto it = std::find_if(function_defs.begin(), function_defs.end(), [&] (const function_def_t& e) { return e.def_name == function_name; } );
	const auto it = std::find_if(
		vec->get_element_ptr(),
		vec->get_element_ptr() + vec->get_element_count(),
		[&] (const runtime_value_t& e) {
			return compare_values(backend, static_cast<int64_t>(expression_type::k_logical_equal), value_type, e, value) == 1;
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
int64_t find__hamt(value_backend_t& backend, runtime_value_t coll_value, runtime_type_t coll_type, const runtime_value_t value, runtime_type_t value_type){
	QUARK_ASSERT(backend.check_invariant());

	const auto& type0 = lookup_type_ref(backend, coll_type);
	const auto& type1 = lookup_type_ref(backend, value_type);

	QUARK_ASSERT(type1 == peek2(backend.types, type0).get_vector_element_type(backend.types));

	const auto& vec = *coll_value.vector_hamt_ptr;
	QUARK_ASSERT(vec.check_invariant());

	const auto count = vec.get_element_count();
	int64_t index = 0;
	while(index < count && compare_values(backend, static_cast<int64_t>(expression_type::k_logical_equal), value_type, vec.load_element(index), value) != 1){
		index++;
	}
	if(index == count){
		return -1;
	}
	else{
		return index;
	}
}






runtime_value_t get_keys__cppmap_carray(value_backend_t& backend, runtime_value_t dict_value, runtime_type_t dict_type){
	QUARK_ASSERT(backend.check_invariant());

	const auto& type0 = lookup_type_ref(backend, dict_type);

	QUARK_ASSERT(type0.is_dict());

	const auto& dict = unpack_dict_cppmap_arg(backend, dict_value, dict_type);
	const auto& m = dict->get_map();
	const auto count = (uint64_t)m.size();

	auto result_vec = alloc_vector_carray(backend.heap, count, count, make_vector(backend.types, type_t::make_string()));

	int index = 0;
	for(const auto& e: m){
		//	Notice that the internal representation of dictionary keys are std::string, not floyd-strings,
		//	so we need to create new key-strings from scratch.
		const auto key = to_runtime_string2(backend, e.first);
		result_vec.vector_carray_ptr->get_element_ptr()[index] = key;
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

	auto result_vec = alloc_vector_hamt(backend.heap, count, count, make_vector(backend.types, type_t::make_string()));

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


runtime_value_t get_keys__hamtmap_carray(value_backend_t& backend, runtime_value_t dict_value, runtime_type_t dict_type){
	QUARK_ASSERT(backend.check_invariant());

	const auto& type0 = lookup_type_ref(backend, dict_type);

	QUARK_ASSERT(type0.is_dict());

	const auto& dict = dict_value.dict_hamt_ptr;
	const auto& m = dict->get_map();
	const auto count = (uint64_t)m.size();

	auto result_vec = alloc_vector_carray(backend.heap, count, count, make_vector(backend.types, type_t::make_string()));

	int index = 0;
	for(const auto& e: m){
		//	Notice that the internal representation of dictionary keys are std::string, not floyd-strings,
		//	so we need to create new key-strings from scratch.
		const auto key = to_runtime_string2(backend, e.first);
		result_vec.vector_carray_ptr->get_element_ptr()[index] = key;
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

	auto result_vec = alloc_vector_hamt(backend.heap, count, count, make_vector(backend.types, type_t::make_string()));

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

runtime_value_t concat_vector_carray(value_backend_t& backend, const type_t& type, const runtime_value_t& lhs, const runtime_value_t& rhs){
	QUARK_ASSERT(backend.check_invariant());
	QUARK_ASSERT(type.check_invariant());
	QUARK_ASSERT(lhs.check_invariant());
	QUARK_ASSERT(rhs.check_invariant());

	const auto count2 = lhs.vector_carray_ptr->get_element_count() + rhs.vector_carray_ptr->get_element_count();

	auto result = alloc_vector_carray(backend.heap, count2, count2, type);

	//??? warning: assumes element = allocation.

	const auto element_itype = lookup_vector_element_type(backend, type);

	auto dest_ptr = result.vector_carray_ptr->get_element_ptr();
	auto dest_ptr2 = dest_ptr + lhs.vector_carray_ptr->get_element_count();
	auto lhs_ptr = lhs.vector_carray_ptr->get_element_ptr();
	auto rhs_ptr = rhs.vector_carray_ptr->get_element_ptr();

	if(is_rc_value(peek2(backend.types, element_itype))){
		for(int i = 0 ; i < lhs.vector_carray_ptr->get_element_count() ; i++){
			retain_value(backend, lhs_ptr[i], element_itype);
			dest_ptr[i] = lhs_ptr[i];
		}
		for(int i = 0 ; i < rhs.vector_carray_ptr->get_element_count() ; i++){
			retain_value(backend, rhs_ptr[i], element_itype);
			dest_ptr2[i] = rhs_ptr[i];
		}
	}
	else{
		for(int i = 0 ; i < lhs.vector_carray_ptr->get_element_count() ; i++){
			dest_ptr[i] = lhs_ptr[i];
		}
		for(int i = 0 ; i < rhs.vector_carray_ptr->get_element_count() ; i++){
			dest_ptr2[i] = rhs_ptr[i];
		}
	}
	return result;
}

runtime_value_t concat_vector_hamt(value_backend_t& backend, const type_t& type, const runtime_value_t& lhs, const runtime_value_t& rhs){
	QUARK_ASSERT(backend.check_invariant());
	QUARK_ASSERT(type.check_invariant());
	QUARK_ASSERT(lhs.check_invariant());
	QUARK_ASSERT(rhs.check_invariant());

	const auto lhs_count = lhs.vector_hamt_ptr->get_element_count();
	const auto rhs_count = rhs.vector_hamt_ptr->get_element_count();

	const auto count2 = lhs_count + rhs_count;

	auto result = alloc_vector_hamt(backend.heap, count2, count2, type);

	//??? warning: assumes element = allocation.

	const auto element_itype = lookup_vector_element_type(backend, type);

	//??? Causes a full path copy for EACH ELEMENT = slow. better to make new hamt in one go.
	if(is_rc_value(peek2(backend.types, element_itype))){
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
