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
	QUARK_ASSERT(peek2(backend.types, type).is_dict());
	QUARK_ASSERT(arg.dict_cppmap_ptr != nullptr);

	QUARK_ASSERT(arg.dict_cppmap_ptr->check_invariant());

	return arg.dict_cppmap_ptr;
}














#if 0

//////////////////////////////////////////		COMPARE



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

static int bc_compare_string(const std::string& left, const std::string& right){
	// ??? Better if it doesn't use c_ptr since that is non-pure string handling.
	return compare(std::strcmp(left.c_str(), right.c_str()));
}

QUARK_TEST("bc_compare_string()", "", "", ""){
	ut_verify_auto(QUARK_POS, bc_compare_string("", ""), 0);
}
QUARK_TEST("bc_compare_string()", "", "", ""){
	ut_verify_auto(QUARK_POS, bc_compare_string("aaa", "aaa"), 0);
}
QUARK_TEST("bc_compare_string()", "", "", ""){
	ut_verify_auto(QUARK_POS, bc_compare_string("b", "a"), 1);
}


static int bc_compare_struct_true_deep(const types_t& types, const std::vector<bc_value_t>& left, const std::vector<bc_value_t>& right, const type_t& type){
	QUARK_ASSERT(types.check_invariant());
	QUARK_ASSERT(type.check_invariant());

	const auto& struct_def = peek2(types, type).get_struct(types);

	for(int i = 0 ; i < struct_def._members.size() ; i++){
		const auto& member_type = struct_def._members[i]._type;
		int diff = bc_compare_value_true_deep(types, left[i], right[i], member_type);
		if(diff != 0){
			return diff;
		}
	}
	return 0;
}

static int bc_compare_vectors_obj(const types_t& types, const immer::vector<bc_external_handle_t>& left, const immer::vector<bc_external_handle_t>& right, const type_t& type){
	QUARK_ASSERT(types.check_invariant());
	QUARK_ASSERT(type.check_invariant());

	const auto& peek = peek2(types, type);
	QUARK_ASSERT(peek.is_vector());

	const auto& shared_count = std::min(left.size(), right.size());
	const auto& element_type = peek.get_vector_element_type(types);
	for(int i = 0 ; i < shared_count ; i++){
		const auto element_result = bc_compare_value_true_deep(
			types,
			bc_value_t(element_type, left[i]),
			bc_value_t(element_type, right[i]),
			element_type
		);
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

static int compare_bools(const bc_inplace_value_t& left, const bc_inplace_value_t& right){
	auto left2 = left.bool_value ? 1 : 0;
	auto right2 = right.bool_value ? 1 : 0;
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

static int compare_ints(const bc_inplace_value_t& left, const bc_inplace_value_t& right){
	if(left.int64_value < right.int64_value){
		return -1;
	}
	else if(left.int64_value > right.int64_value){
		return 1;
	}
	else{
		return 0;
	}
}
static int compare_doubles(const bc_inplace_value_t& left, const bc_inplace_value_t& right){
	if(left.double_value < right.double_value){
		return -1;
	}
	else if(left.double_value > right.double_value){
		return 1;
	}
	else{
		return 0;
	}
}

static int bc_compare_vectors_bool(const immer::vector<bc_inplace_value_t>& left, const immer::vector<bc_inplace_value_t>& right){
	const auto& shared_count = std::min(left.size(), right.size());
	for(int i = 0 ; i < shared_count ; i++){
		int result = compare_bools(left[i], right[i]);
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
static int bc_compare_vectors_int(const immer::vector<bc_inplace_value_t>& left, const immer::vector<bc_inplace_value_t>& right){
	const auto& shared_count = std::min(left.size(), right.size());
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
static int bc_compare_vectors_double(const immer::vector<bc_inplace_value_t>& left, const immer::vector<bc_inplace_value_t>& right){
	const auto& shared_count = std::min(left.size(), right.size());
	for(int i = 0 ; i < shared_count ; i++){
		int result = compare_doubles(left[i], right[i]);
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

static int bc_compare_dicts_obj(const types_t& types, const immer::map<std::string, bc_external_handle_t>& left, const immer::map<std::string, bc_external_handle_t>& right, const type_t& type){
	QUARK_ASSERT(types.check_invariant());
	QUARK_ASSERT(type.check_invariant());

	const auto peek = peek2(types, type);
	const auto& element_type = peek.get_dict_value_type(types);

	auto left_it = left.begin();
	auto left_end_it = left.end();

	auto right_it = right.begin();
	auto right_end_it = right.end();

	while(left_it != left_end_it && right_it != right_end_it){
		auto left_key = (*left_it).first;
		auto right_key = (*right_it).first;

		const auto key_result = bc_compare_string(left_key, right_key);
		if(key_result != 0){
			return key_result;
		}

		const auto element_result = bc_compare_value_true_deep(
			types,
			bc_value_t(element_type, (*left_it).second),
			bc_value_t(element_type, (*right_it).second),
			element_type
		);
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
static int bc_compare_dicts_bool(const immer::map<std::string, bc_inplace_value_t>& left, const immer::map<std::string, bc_inplace_value_t>& right){
	auto left_it = left.begin();
	auto left_end_it = left.end();

	auto right_it = right.begin();
	auto right_end_it = right.end();

	while(left_it != left_end_it && right_it != right_end_it){
		auto left_key = (*left_it).first;
		auto right_key = (*right_it).first;

		const auto key_result = bc_compare_string(left_key, right_key);
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

static int bc_compare_dicts_int(const immer::map<std::string, bc_inplace_value_t>& left, const immer::map<std::string, bc_inplace_value_t>& right){
	auto left_it = left.begin();
	auto left_end_it = left.end();

	auto right_it = right.begin();
	auto right_end_it = right.end();

	while(left_it != left_end_it && right_it != right_end_it){
		auto left_key = (*left_it).first;
		auto right_key = (*right_it).first;

		const auto key_result = bc_compare_string(left_key, right_key);
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

static int bc_compare_dicts_double(const immer::map<std::string, bc_inplace_value_t>& left, const immer::map<std::string, bc_inplace_value_t>& right){
	auto left_it = left.begin();
	auto left_end_it = left.end();

	auto right_it = right.begin();
	auto right_end_it = right.end();

	while(left_it != left_end_it && right_it != right_end_it){
		auto left_key = (*left_it).first;
		auto right_key = (*right_it).first;

		const auto key_result = bc_compare_string(left_key, right_key);
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

static int bc_compare_jsons(const json_t& lhs, const json_t& rhs){
	if(lhs == rhs){
		return 0;
	}
	else{
		const auto lhs_type = get_json_type(lhs);
		const auto rhs_type = get_json_type(rhs);
		int type_diff = rhs_type - lhs_type;
		if(type_diff != 0){
			return type_diff;
		}
		else{
			if(lhs.is_object()){
				//??? NOT IMPLEMENTED YET
				QUARK_ASSERT(false);
				quark::throw_exception();
			}
			else if(lhs.is_array()){
				//??? NOT IMPLEMENTED YET
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
				return 0;
			}
			else if(lhs.is_false()){
				return 0;
			}
			else if(lhs.is_null()){
				return 0;
			}
			else{
				QUARK_ASSERT(false);
				quark::throw_exception();
			}
		}
	}
}

static int bc_compare_value_exts(const types_t& types, const bc_external_handle_t& left, const bc_external_handle_t& right, const type_t& type){
	QUARK_ASSERT(types.check_invariant());
	QUARK_ASSERT(left.check_invariant());
	QUARK_ASSERT(right.check_invariant());
	QUARK_ASSERT(type.check_invariant());

	return bc_compare_value_true_deep(types, bc_value_t(type, left), bc_value_t(type, right), type);
}

int bc_compare_value_true_deep(value_backend_t& backend, const bc_value_t& left, const bc_value_t& right, const type_t& type0){
	QUARK_ASSERT(types.check_invariant());
	QUARK_ASSERT(left._type == right._type);
	QUARK_ASSERT(left.check_invariant());
	QUARK_ASSERT(right.check_invariant());

	auto& types = backend.types;

	const auto type = type0;
	const auto peek = peek2(types, type);

	if(type.is_undefined()){
		return 0;
	}
	else if(peek.is_bool()){
		return (left.get_bool_value() ? 1 : 0) - (right.get_bool_value() ? 1 : 0);
	}
	else if(peek.is_int()){
		return compare(left.get_int_value() - right.get_int_value());
	}
	else if(peek.is_double()){
		const auto a = left.get_double_value();
		const auto b = right.get_double_value();
		if(a > b){
			return 1;
		}
		else if(a < b){
			return -1;
		}
		else{
			return 0;
		}
	}
	else if(peek.is_string()){
		return bc_compare_string(left.get_string_value(), right.get_string_value());
	}
	else if(peek.is_json()){
		return bc_compare_jsons(left.get_json(), right.get_json());
	}
	else if(peek.is_typeid()){
		if(left.get_typeid_value() == right.get_typeid_value()){
			return 0;
		}
		else{
			return -1;//??? Hack -- should return +1 depending on values.
		}
	}
	else if(peek.is_struct()){
		//	Make sure the EXACT struct types are the same -- not only that they are both structs
		return bc_compare_struct_true_deep(types, left.get_struct_value(), right.get_struct_value(), type0);
	}
	else if(peek.is_vector()){
		const auto element_type_peek = peek2(types, peek.get_vector_element_type(types));
		if(false){
		}
		else if(element_type_peek.is_bool()){
			return bc_compare_vectors_bool(left._pod._external->_vector_w_inplace_elements, right._pod._external->_vector_w_inplace_elements);
		}
		else if(element_type_peek.is_int()){
			return bc_compare_vectors_int(left._pod._external->_vector_w_inplace_elements, right._pod._external->_vector_w_inplace_elements);
		}
		else if(element_type_peek.is_double()){
			return bc_compare_vectors_double(left._pod._external->_vector_w_inplace_elements, right._pod._external->_vector_w_inplace_elements);
		}
		else{
			const auto& left_vec = get_vector_external_elements(types, left);
			const auto& right_vec = get_vector_external_elements(types, right);
			return bc_compare_vectors_obj(types, *left_vec, *right_vec, type0);
		}
	}
	else if(peek.is_dict()){
		const auto dict_value_peek = peek2(types, peek.get_dict_value_type(types));
		if(false){
		}
		else if(dict_value_peek.is_bool()){
			return bc_compare_dicts_bool(left._pod._external->_dict_w_inplace_values, right._pod._external->_dict_w_inplace_values);
		}
		else if(dict_value_peek.is_int()){
			return bc_compare_dicts_int(left._pod._external->_dict_w_inplace_values, right._pod._external->_dict_w_inplace_values);
		}
		else if(dict_value_peek.is_double()){
			return bc_compare_dicts_double(left._pod._external->_dict_w_inplace_values, right._pod._external->_dict_w_inplace_values);
		}
		else  {
			QUARK_ASSERT(encode_as_dict_w_inplace_values(types, type) == false);

			const auto& left2 = get_dict_external_values(types, left);
			const auto& right2 = get_dict_external_values(types, right);
			return bc_compare_dicts_obj(types, left2, right2, type0);
		}
	}
	else if(peek2(types, type).is_function()){
		QUARK_ASSERT(false);
		return 0;
	}
	else{
		QUARK_ASSERT(false);
		quark::throw_exception();
	}
}
#endif









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

runtime_value_t update_element__vector(value_backend_t& backend, runtime_value_t obj1, runtime_type_t coll_type, runtime_value_t index, runtime_value_t value){
	QUARK_ASSERT(backend.check_invariant());

	const auto& types = backend.types;
	const auto vector_type = type_t(coll_type);

	const auto& obj1_peek = peek2(types, vector_type);
	const auto element_type = obj1_peek.get_vector_element_type(types);
	const bool element_is_rc = is_rc_value(backend.types, peek2(types, element_type));

	const auto backend_mode = backend.config.vector_backend_mode;
	if(backend_mode == vector_backend::carray){
		return update_element__vector_carray(backend, obj1, coll_type, index, value);
	}
	else if(backend_mode == vector_backend::hamt){
		if(element_is_rc){
			return update_element__vector_hamt_nonpod(backend, obj1, coll_type, index, value);
		}
		else{
			return update_element__vector_hamt_pod(backend, obj1, coll_type, index, value);
		}
	}
	else{
		UNSUPPORTED();
	}
}

runtime_value_t update_dict(value_backend_t& backend, runtime_value_t obj1, runtime_type_t coll_type, runtime_value_t key_value, runtime_value_t value){
	QUARK_ASSERT(backend.check_invariant());

	const auto backend_mode = backend.config.dict_backend_mode;
	if(backend_mode == dict_backend::cppmap){
		return update__dict_cppmap(backend, obj1, coll_type, key_value, value);
	}
	else if(backend_mode == dict_backend::hamt){
		return update__dict_hamt(backend, obj1, coll_type, key_value, value);
	}
	else{
		UNSUPPORTED();
	}
}





const runtime_value_t update_element__vector_carray(value_backend_t& backend, runtime_value_t coll_value, runtime_type_t coll_type, runtime_value_t index, runtime_value_t value){
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
	if(is_rc_value(backend.types, element_itype)){
		retain_value(backend, value, element_itype);
		for(int i = 0 ; i < result.vector_carray_ptr->get_element_count() ; i++){
			retain_value(backend, source_ptr[i], element_itype);
			dest_ptr[i] = source_ptr[i];
		}

		if(is_rc_value(backend.types, type_t(coll_type))){
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
	const auto value_itype = peek2(backend.types, type0).get_dict_value_type(backend.types);

	//	Deep copy dict.
	auto dict2 = alloc_dict_cppmap(backend.heap, type_t(coll_type));
	dict2.dict_cppmap_ptr->get_map_mut() = dict->get_map();

	dict2.dict_cppmap_ptr->get_map_mut().insert_or_assign(key, value);

	if(is_rc_value(backend.types, value_itype)){
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
	const auto value_itype = peek2(backend.types, type0).get_dict_value_type(backend.types);

	//	Deep copy dict.
	auto dict2 = alloc_dict_hamt(backend.heap, type_t(coll_type));
	dict2.dict_hamt_ptr->get_map_mut() = dict->get_map();

	dict2.dict_hamt_ptr->get_map_mut() = dict2.dict_hamt_ptr->get_map_mut().set(key, value);

	if(is_rc_value(backend.types, value_itype)){
		for(const auto& e: dict2.dict_hamt_ptr->get_map()){
			retain_value(backend, e.second, value_itype);
		}
	}

	return dict2;
}





/*
//??? The update mechanism uses strings == slow.
static bc_value_t update_struct_member_shallow(interpreter_t& vm, const bc_value_t& obj, const std::string& member_name, const bc_value_t& new_value){
	QUARK_ASSERT(obj.check_invariant());
	const auto& types = vm._imm->_program._types;
	QUARK_ASSERT(peek2(types, obj._type).is_struct());
	QUARK_ASSERT(member_name.empty() == false);
	QUARK_ASSERT(new_value.check_invariant());

	const auto& values = obj.get_struct_value(vm._backend);
	const auto& struct_def = peek2(types, obj._type).get_struct(types);

	int member_index = find_struct_member_index(struct_def, member_name);
	if(member_index == -1){
		quark::throw_runtime_error("Unknown member.");
	}

#if DEBUG && 0
	QUARK_TRACE(typeid_to_compact_string(new_value._type));
	QUARK_TRACE(typeid_to_compact_string(struct_def._members[member_index]._type));

	const auto dest_member_entry = struct_def._members[member_index];
#endif

	auto values2 = values;
	values2[member_index] = new_value;

	auto s2 = bc_value_t::make_struct_value(vm._backend, obj._type, values2);
	return s2;
}

static bc_value_t update_struct_member_deep(interpreter_t& vm, const bc_value_t& obj, const std::vector<std::string>& path, const bc_value_t& new_value){
	QUARK_ASSERT(obj.check_invariant());
	QUARK_ASSERT(path.empty() == false);
	QUARK_ASSERT(new_value.check_invariant());

	if(path.size() == 1){
		return update_struct_member_shallow(vm, obj, path[0], new_value);
	}
	else{
		const auto& types = vm._imm->_program._types;

		std::vector<std::string> subpath = path;
		subpath.erase(subpath.begin());

		const auto& values = obj.get_struct_value(vm._backend);
		const auto& struct_def = peek2(types, obj._type).get_struct(types);
		int member_index = find_struct_member_index(struct_def, path[0]);
		if(member_index == -1){
			quark::throw_runtime_error("Unknown member.");
		}

		const auto& child_value = values[member_index];
		const auto& child_type = struct_def._members[member_index]._type;
		if(peek2(types, child_type).is_struct() == false){
			quark::throw_runtime_error("Value type not matching struct member type.");
		}

		const auto child2 = update_struct_member_deep(vm, child_value, subpath, new_value);
		const auto obj2 = update_struct_member_shallow(vm, obj, path[0], child2);
		return obj2;
	}
}
*/

//??? Slow
runtime_value_t update_struct_member(value_backend_t& backend, runtime_value_t struct_value, const type_t& struct_type0, int member_index, runtime_value_t value, const type_t& member_type){
	QUARK_ASSERT(backend.check_invariant());
	QUARK_ASSERT(backend.check_invariant());
	QUARK_ASSERT(peek2(backend.types, struct_type0).is_struct());
	QUARK_ASSERT(value.check_invariant());

	QUARK_ASSERT(struct_value.struct_ptr->check_invariant());

	const auto& struct_type_peek = peek2(backend.types, struct_type0);
	const auto& value2 = bc_value_t(backend, peek2(backend.types, member_type), value);
	const auto& values = from_runtime_struct(backend, struct_value, struct_type_peek);

	auto values2 = values;
	values2[member_index] = value2;

	auto result = bc_value_t::make_struct_value(backend, struct_type0, values2);
	retain_value(backend, result._pod, struct_type0);
	return result._pod;
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
	if(is_rc_value(backend.types, element_itype)){
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
	if(is_rc_value(backend.types, element_itype)){
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


//	assert(subset("abc", 1, 3) == "bc");
runtime_value_t subset(value_backend_t& backend, runtime_value_t coll_value, runtime_type_t coll_type, int64_t start, int64_t end){
	QUARK_ASSERT(backend.check_invariant());


	if(start < 0 || end < 0){
		quark::throw_runtime_error("subset() requires start and end to be non-negative.");
	}

	const auto obj_type = type_t(coll_type);
	if(obj_type.is_string()){
		return subset__string(backend, coll_value, coll_type, start, end);
	}
	else if(is_vector_carray(backend.types, backend.config, obj_type)){
		return subset__carray(backend, coll_value, coll_type, start, end);
	}
	else if(is_vector_hamt(backend.types, backend.config, obj_type)){
		return subset__hamt(backend, coll_value, coll_type, start, end);
	}
	else{
		quark::throw_runtime_error("Calling push_back() on unsupported type of value.");
	}
}








static void check_replace_indexes(std::size_t start, std::size_t end){
	if(start < 0 || end < 0){
		quark::throw_runtime_error("replace() requires start and end to be non-negative.");
	}
	if(start > end){
		quark::throw_runtime_error("replace() requires start <= end.");
	}
}

runtime_value_t replace__string(value_backend_t& backend, runtime_value_t coll_value, runtime_type_t coll_type, size_t start, size_t end, runtime_value_t replacement_value, runtime_type_t replacement_type){
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

runtime_value_t replace__carray(value_backend_t& backend, runtime_value_t coll_value, runtime_type_t coll_type, size_t start, size_t end, runtime_value_t replacement_value, runtime_type_t replacement_type){
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

	if(is_rc_value(backend.types, element_itype)){
		for(int i = 0 ; i < len2 ; i++){
			retain_value(backend, vec2.vector_carray_ptr->get_element_ptr()[i], element_itype);
		}
	}

	return vec2;
}
runtime_value_t replace__hamt(value_backend_t& backend, runtime_value_t coll_value, runtime_type_t coll_type, size_t start, size_t end, runtime_value_t replacement_value, runtime_type_t replacement_type){
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

	if(is_rc_value(backend.types, element_itype)){
		for(int i = 0 ; i < len2 ; i++){
			retain_value(backend, vec2.vector_hamt_ptr->load_element(i), element_itype);
		}
	}

	return vec2;
}





//	assert(replace("One ring to rule them all", 4, 7, "rabbit") == "One rabbit to rule them all");
runtime_value_t replace(value_backend_t& backend, runtime_value_t coll_value, runtime_type_t coll_type, int64_t start, int64_t end, runtime_value_t replacement_value, runtime_type_t replacement_type){
	QUARK_ASSERT(backend.check_invariant());

	if(start < 0 || end < 0){
		quark::throw_runtime_error("replace() requires start and end to be non-negative.");
	}
	if(start > end){
		quark::throw_runtime_error("replace() requires start <= end.");
	}
	QUARK_ASSERT(coll_type == replacement_type);

	const auto obj_type = type_t(coll_type);
	if(obj_type.is_string()){
		return replace__string(backend, coll_value, coll_type, start, end, replacement_value, replacement_type);
	}
	else if(is_vector_carray(backend.types, backend.config, obj_type)){
		return replace__carray(backend, coll_value, coll_type, start, end, replacement_value, replacement_type);
	}
	else if(is_vector_hamt(backend.types, backend.config, obj_type)){
		return replace__hamt(backend, coll_value, coll_type, start, end, replacement_value, replacement_type);
	}
	else{
		quark::throw_runtime_error("Calling push_back() on unsupported type of value.");
	}
}







int64_t find__string(value_backend_t& backend, runtime_value_t coll_value, runtime_type_t coll_type, const runtime_value_t value, runtime_type_t value_type){
	QUARK_ASSERT(backend.check_invariant());

	const auto& type1 = lookup_type_ref(backend, value_type);

	QUARK_ASSERT(peek2(backend.types, type1).is_string());

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


int64_t find2(value_backend_t& backend, runtime_value_t coll_value, runtime_type_t coll_type, const runtime_value_t value, runtime_type_t value_type){
	const auto& type0 = lookup_type_ref(backend, coll_type);
	if(peek2(backend.types, type0).is_string()){
		return find__string(backend, coll_value, coll_type, value, value_type);
	}
	else if(is_vector_carray(backend.types, backend.config, type0)){
		return find__carray(backend, coll_value, coll_type, value, value_type);
	}
	else if(is_vector_hamt(backend.types, backend.config, type0)){
		return find__hamt(backend, coll_value, coll_type, value, value_type);
	}
	else{
		//	No other types allowed.
		UNSUPPORTED();
	}
}

//??? use bc_value_t as primitive for all features.

bool exists(value_backend_t& backend, runtime_value_t coll_value, runtime_type_t coll_type, runtime_value_t value, runtime_type_t value_type){
	const auto& types = backend.types;
	const auto& type0 = lookup_type_ref(backend, coll_type);
	const auto& type1 = lookup_type_ref(backend, value_type);
	QUARK_ASSERT(peek2(types, type0).is_dict());

	if(is_dict_cppmap(types, backend.config, type0)){
		const auto& dict = unpack_dict_cppmap_arg(backend, coll_value, coll_type);
		const auto key_string = from_runtime_string2(backend, value);

		const auto& m = dict->get_map();
		const auto it = m.find(key_string);
		return it != m.end() ? 1 : 0;
	}
	else if(is_dict_hamt(types, backend.config, type0)){
		const auto& dict = *coll_value.dict_hamt_ptr;
		const auto key_string = from_runtime_string2(backend, value);

		const auto& m = dict.get_map();
		const auto it = m.find(key_string);
		return it != nullptr ? 1 : 0;
	}
	else{
		QUARK_ASSERT(false);
		throw std::exception();
	}
}

runtime_value_t erase(value_backend_t& backend, runtime_value_t coll_value, runtime_type_t coll_type, runtime_value_t key_value, runtime_type_t key_type){
	const auto& types = backend.types;
	const auto& type0 = lookup_type_ref(backend, coll_type);
	const auto& type1 = lookup_type_ref(backend, key_type);

	QUARK_ASSERT(peek2(types, type0).is_dict());
	QUARK_ASSERT(peek2(types, type1).is_string());

	if(is_dict_cppmap(types, backend.config, type0)){
		const auto& dict = unpack_dict_cppmap_arg(backend, coll_value, coll_type);

		const auto value_type = peek2(types, type0).get_dict_value_type(types);

		//	Deep copy dict.
		auto dict2 = alloc_dict_cppmap(backend.heap, type0);
		auto& m = dict2.dict_cppmap_ptr->get_map_mut();
		m = dict->get_map();

		const auto key_string = from_runtime_string2(backend, key_value);
		m.erase(key_string);

		if(is_rc_value(types, value_type)){
			for(auto& e: m){
				retain_value(backend, e.second, value_type);
			}
		}
		return dict2;
	}
	else if(is_dict_hamt(types, backend.config, type0)){
		const auto& dict = *coll_value.dict_hamt_ptr;

		const auto value_type = peek2(types, type0).get_dict_value_type(types);

		//	Deep copy dict.
		auto dict2 = alloc_dict_hamt(backend.heap, type0);
		auto& m = dict2.dict_hamt_ptr->get_map_mut();
		m = dict.get_map();

		const auto key_string = from_runtime_string2(backend, key_value);
		m = m.erase(key_string);

		if(is_rc_value(types, value_type)){
			for(auto& e: m){
				retain_value(backend, e.second, value_type);
			}
		}
		return dict2;
	}
	else{
		QUARK_ASSERT(false);
		throw std::exception();
	}
}

//??? optimize prio 1
//??? We need to figure out the return type *again*, knowledge we have already in semast.
runtime_value_t get_keys(value_backend_t& backend, runtime_value_t coll_value, runtime_type_t coll_type){
	const auto& types = backend.types;
	const auto& type0 = lookup_type_ref(backend, coll_type);
	QUARK_ASSERT(peek2(types, type0).is_dict());

	if(is_dict_cppmap(types, backend.config, type0)){
		if(backend.config.vector_backend_mode == vector_backend::carray){
			return get_keys__cppmap_carray(backend, coll_value, coll_type);
		}
		else if(backend.config.vector_backend_mode == vector_backend::hamt){
			return get_keys__cppmap_hamt(backend, coll_value, coll_type);
		}
		else{
			QUARK_ASSERT(false);
			throw std::exception();
		}
	}
	else if(is_dict_hamt(types, backend.config, type0)){
		if(backend.config.vector_backend_mode == vector_backend::carray){
			return get_keys__hamtmap_carray(backend, coll_value, coll_type);
		}
		else if(backend.config.vector_backend_mode == vector_backend::hamt){
			return get_keys__hamtmap_hamt(backend, coll_value, coll_type);
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





runtime_value_t get_keys__cppmap_carray(value_backend_t& backend, runtime_value_t dict_value, runtime_type_t dict_type){
	QUARK_ASSERT(backend.check_invariant());

	const auto& type0 = lookup_type_ref(backend, dict_type);

	QUARK_ASSERT(peek2(backend.types, type0).is_dict());

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

	QUARK_ASSERT(peek2(backend.types, type0).is_dict());

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

	QUARK_ASSERT(peek2(backend.types, type0).is_dict());

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

	QUARK_ASSERT(peek2(backend.types, type0).is_dict());

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

	if(is_rc_value(backend.types, element_itype)){
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
	if(is_rc_value(backend.types, element_itype)){
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


runtime_value_t concatunate_vectors(value_backend_t& backend, const type_t& type, runtime_value_t lhs, runtime_value_t rhs){
	QUARK_ASSERT(backend.check_invariant());
	QUARK_ASSERT(type.check_invariant());
	QUARK_ASSERT(lhs.check_invariant());
	QUARK_ASSERT(rhs.check_invariant());

	if(type.is_string()){
		return concat_strings(backend, lhs, rhs);
	}
	else if(is_vector_carray(backend.types, backend.config, type)){
		return concat_vector_carray(backend, type, lhs, rhs);
	}
	else if(is_vector_hamt(backend.types, backend.config, type)){
		return concat_vector_hamt(backend, type, lhs, rhs);
	}
	else{
		QUARK_ASSERT(false);
		throw std::exception();
	}
}


uint64_t get_vector_size(value_backend_t& backend, const type_t& vector_type, runtime_value_t vec){
	QUARK_ASSERT(backend.check_invariant());
	QUARK_ASSERT(vector_type.check_invariant());
	QUARK_ASSERT(vec.check_invariant());

	if(vector_type.is_string()){
		return vec.vector_carray_ptr->get_element_count();
	}
	else if(is_vector_carray(backend.types, backend.config, vector_type)){
		return vec.vector_carray_ptr->get_element_count();
	}
	else if(is_vector_hamt(backend.types, backend.config, vector_type)){
		return vec.vector_hamt_ptr->get_element_count();
	}
	else{
		QUARK_ASSERT(false);
		throw std::exception();
	}
}

runtime_value_t lookup_vector_element(value_backend_t& backend, const type_t& vector_type, runtime_value_t vec, uint64_t index){
	QUARK_ASSERT(backend.check_invariant());
	QUARK_ASSERT(vector_type.check_invariant());
	QUARK_ASSERT(vec.check_invariant());
	QUARK_ASSERT(index >= 0);

	if(vector_type.is_string()){
		//??? slow
		const auto s = from_runtime_string2(backend, vec);
		return runtime_value_t { .int_value = s[index] };
	}
	else if(is_vector_carray(backend.types, backend.config, vector_type)){
#if DEBUG
		const auto size = vec.vector_carray_ptr->get_element_count();
		QUARK_ASSERT(index < size);
#endif

		return vec.vector_carray_ptr->get_element_ptr()[index];
	}
	else if(is_vector_hamt(backend.types, backend.config, vector_type)){
#if DEBUG
		const auto size = vec.vector_hamt_ptr->get_element_count();
		QUARK_ASSERT(index < size);
#endif

		return vec.vector_hamt_ptr->load_element(index);
	}
	else{
		QUARK_ASSERT(false);
		throw std::exception();
	}
}







runtime_value_t lookup_dict_cppmap(value_backend_t& backend, runtime_value_t dict, const type_t& dict_type, runtime_value_t key){
	QUARK_ASSERT(backend.check_invariant());
	QUARK_ASSERT(dict_type.check_invariant());
	QUARK_ASSERT(is_dict_cppmap(backend.types, backend.config, dict_type));

	const auto& m = dict.dict_cppmap_ptr->get_map();
	const auto key_string = from_runtime_string2(backend, key);
	const auto it = m.find(key_string);
	if(it == m.end()){
		throw std::exception();
	}
	else{
		return it->second;
	}
}

//??? make faster key without creating std::string.
runtime_value_t lookup_dict_hamt(value_backend_t& backend, runtime_value_t dict, const type_t& dict_type, runtime_value_t key){
	QUARK_ASSERT(backend.check_invariant());
	QUARK_ASSERT(dict_type.check_invariant());
	QUARK_ASSERT(is_dict_hamt(backend.types, backend.config, dict_type));

	const auto& m = dict.dict_hamt_ptr->get_map();
	const auto key_string = from_runtime_string2(backend, key);
	const auto it = m.find(key_string);
	if(it == nullptr){
		throw std::exception();
	}
	else{
		return *it;
	}
}

runtime_value_t lookup_dict(value_backend_t& backend, runtime_value_t dict, const type_t& dict_type, runtime_value_t key){
	if(is_dict_hamt(backend.types, backend.config, dict_type)){
		return lookup_dict_hamt(backend, dict, dict_type, key);
	}
	else if(is_dict_cppmap(backend.types, backend.config, dict_type)){
		return lookup_dict_cppmap(backend, dict, dict_type, key);
	}
	else{
		QUARK_ASSERT(false);
		throw std::exception();
	}
}



uint64_t get_dict_size(value_backend_t& backend, const type_t& dict_type, runtime_value_t dict){
	QUARK_ASSERT(backend.check_invariant());
	QUARK_ASSERT(dict_type.check_invariant());
	QUARK_ASSERT(dict_type.is_dict());
	QUARK_ASSERT(dict.check_invariant());

	if(is_dict_hamt(backend.types, backend.config, dict_type)){
		return dict.dict_hamt_ptr->size();
	}
	else if(is_dict_cppmap(backend.types, backend.config, dict_type)){
		return dict.dict_cppmap_ptr->size();
	}
	else{
		QUARK_ASSERT(false);
		throw std::exception();
	}
}




// Could specialize further, for vector_hamt<string>, vector_hamt<vector<x>> etc. But it's probably better to inline push_back() instead.


//??? Expensive to push_back since all elements in vector needs their RC bumped!
// Could specialize further, for vector_hamt<string>, vector_hamt<vector<x>> etc. But it's probably better to inline push_back() instead.

runtime_value_t push_back__string(value_backend_t& backend, runtime_value_t vec, const type_t& vec_type, runtime_value_t element){
	QUARK_ASSERT(vec_type.is_string());

	auto value = from_runtime_string2(backend, vec);
	value.push_back((char)element.int_value);
	const auto result2 = to_runtime_string2(backend, value);
	return result2;
}

//??? use memcpy()
runtime_value_t push_back_carray_pod(value_backend_t& backend, runtime_value_t vec, const type_t& vec_type, runtime_value_t element){
	const auto element_count = vec.vector_carray_ptr->get_element_count();
	auto source_ptr = vec.vector_carray_ptr->get_element_ptr();

	auto v2 = alloc_vector_carray(backend.heap, element_count + 1, element_count + 1, type_t(vec_type));
	auto dest_ptr = v2.vector_carray_ptr->get_element_ptr();
	for(int i = 0 ; i < element_count ; i++){
		dest_ptr[i] = source_ptr[i];
	}
	dest_ptr[element_count] = element;
	return v2;
}

runtime_value_t push_back_carray_nonpod(value_backend_t& backend, runtime_value_t vec, const type_t& vec_type, runtime_value_t element){
	type_t element_itype = lookup_vector_element_type(backend, type_t(vec_type));
	const auto element_count = vec.vector_carray_ptr->get_element_count();
	auto source_ptr = vec.vector_carray_ptr->get_element_ptr();

	auto v2 = alloc_vector_carray(backend.heap, element_count + 1, element_count + 1, type_t(vec_type));
	auto dest_ptr = v2.vector_carray_ptr->get_element_ptr();
	for(int i = 0 ; i < element_count ; i++){
		dest_ptr[i] = source_ptr[i];
		retain_value(backend, dest_ptr[i], element_itype);
	}
	dest_ptr[element_count] = element;
	retain_value(backend, dest_ptr[element_count], element_itype);

	return v2;
}

runtime_value_t push_back_hamt_pod(value_backend_t& backend, runtime_value_t vec, const type_t& vec_type, runtime_value_t element){
	return push_back_immutable_hamt(vec, element);
}

runtime_value_t push_back_hamt_nonpod(value_backend_t& backend, runtime_value_t vec, const type_t& vec_type, runtime_value_t element){
	runtime_value_t vec2 = push_back_immutable_hamt(vec, element);
	type_t element_itype = lookup_vector_element_type(backend, type_t(vec_type));

	for(int i = 0 ; i < vec2.vector_hamt_ptr->get_element_count() ; i++){
		const auto& value = vec2.vector_hamt_ptr->load_element(i);
		retain_value(backend, value, element_itype);
	}
	return vec2;
}

runtime_value_t push_back2(value_backend_t& backend, runtime_value_t vec, const type_t& vec_type, runtime_value_t element){
	if(vec_type.is_string()){
		return push_back__string(backend, vec, vec_type, element);
	}
	else if(is_vector_carray(backend.types, backend.config, vec_type)){
		const auto is_rc = is_rc_value(backend.types, vec_type.get_vector_element_type(backend.types));

		if(is_rc){
			return push_back_carray_nonpod(backend, vec, vec_type, element);
		}
		else{
			return push_back_carray_pod(backend, vec, vec_type, element);
		}
	}
	else if(is_vector_hamt(backend.types, backend.config, vec_type)){
		const auto is_rc = is_rc_value(backend.types, vec_type.get_vector_element_type(backend.types));

		if(is_rc){
			return push_back_hamt_nonpod(backend, vec, vec_type, element);
		}
		else{
			return push_back_hamt_pod(backend, vec, vec_type, element);
		}
	}
	else{
		QUARK_ASSERT(false);
		throw std::exception();
	}
}





}	// floyd
