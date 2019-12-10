//
//  floyd_llvm_value_thunking.cpp
//  floyd
//
//  Created by Marcus Zetterquist on 2019-08-19.
//  Copyright © 2019 Marcus Zetterquist. All rights reserved.
//

#include "value_thunking.h"

#include "value_backend.h"
#include "compiler_basics.h"


namespace floyd {


//////////////////////////////////////////		runtime_value_t


runtime_value_t alloc_carray_8bit(value_backend_t& backend, const uint8_t data[], std::size_t count){
	QUARK_ASSERT(backend.check_invariant());
	QUARK_ASSERT(data != nullptr || count == 0);

	const auto allocation_count = size_to_allocation_blocks(count);
	auto result = alloc_vector_carray(backend.heap, allocation_count, count, type_t::make_string());

	size_t char_pos = 0;
	int element_index = 0;
	uint64_t acc = 0;
	while(char_pos < count){
		const uint64_t ch = data[char_pos];
		const auto x = (char_pos & 7) * 8;
		acc = acc | (ch << x);
		char_pos++;

		if(((char_pos & 7) == 0) || (char_pos == count)){
			result.vector_carray_ptr->store(element_index, make_runtime_int(static_cast<int64_t>(acc)));
			element_index = element_index + 1;
			acc = 0;
		}
	}
	return result;
}

runtime_value_t to_runtime_string2(value_backend_t& backend, const std::string& s){
	QUARK_ASSERT(backend.check_invariant());

	const uint8_t* p = reinterpret_cast<const uint8_t*>(s.c_str());
	return alloc_carray_8bit(backend, p, s.size());
}


QUARK_TEST("VECTOR_CARRAY_T", "", "", ""){
	auto backend = make_test_value_backend();
	const auto a = to_runtime_string2(backend, "hello, world!");

	QUARK_VERIFY(a.vector_carray_ptr->get_element_count() == 13);

	//	int64_t	'hello, w'
//	QUARK_VERIFY(a.vector_carray_ptr->load_element(0).int_value == 0x68656c6c6f2c2077);
	QUARK_VERIFY(a.vector_carray_ptr->load_element(0).int_value == 0x77202c6f6c6c6568);

	//int_value	int64_t	'orld!\0\0\0'
//	QUARK_VERIFY(a.vector_carray_ptr->load_element(1).int_value == 0x6f726c6421000000);
	QUARK_VERIFY(a.vector_carray_ptr->load_element(1).int_value == 0x00000021646c726f);
}


// backend not used???
std::string from_runtime_string2(const value_backend_t& backend, runtime_value_t encoded_value){
	QUARK_ASSERT(backend.check_invariant());
	QUARK_ASSERT(encoded_value.check_invariant());
	QUARK_ASSERT(encoded_value.vector_carray_ptr != nullptr);

	const size_t size = get_vec_string_size(encoded_value);

	std::string result;

	//	Read 8 characters at a time.
	size_t char_pos = 0;
	const auto begin0 = encoded_value.vector_carray_ptr->begin();
	const auto end0 = encoded_value.vector_carray_ptr->end();
	for(auto it = begin0 ; it != end0 ; it++){
		const size_t copy_chars = std::min(size - char_pos, (size_t)8);
		const uint64_t element = it->int_value;
		for(int i = 0 ; i < copy_chars ; i++){
			const uint64_t x = i * 8;
			const uint64_t ch = (element >> x) & 0xff;
			result.push_back(static_cast<char>(ch));
		}
		char_pos += copy_chars;
	}

	QUARK_ASSERT(result.size() == size);
	return result;
}

QUARK_TEST("VECTOR_CARRAY_T", "", "", ""){
	auto backend = make_test_value_backend();
	const auto a = to_runtime_string2(backend, "hello, world!");

	const auto r = from_runtime_string2(backend, a);
	QUARK_VERIFY(r == "hello, world!");
}

static runtime_value_t to_runtime_struct(value_backend_t& backend, const struct_t& exact_type, const value_t& value){
	QUARK_ASSERT(backend.check_invariant());
	QUARK_ASSERT(value.check_invariant());

	const auto& struct_layout = find_struct_layout(backend, value.get_type());

	auto s = alloc_struct(backend.heap, struct_layout.second.size, value.get_type());
	const auto struct_base_ptr = s->get_data_ptr();

	int member_index = 0;
	const auto& struct_data = value.get_struct_value();

	for(const auto& e: struct_data->_member_values){
		const auto offset = struct_layout.second.members[member_index].offset;
		const auto member_ptr = reinterpret_cast<void*>(struct_base_ptr + offset);
		const auto member_type = e.get_type();
		store_via_ptr2(backend.types, member_ptr, member_type, to_runtime_value2(backend, e));
		member_index++;
	}
	return make_runtime_struct(s);
}

static value_t from_runtime_struct(const value_backend_t& backend, const runtime_value_t encoded_value, const type_t& type){
	QUARK_ASSERT(backend.check_invariant());
	QUARK_ASSERT(encoded_value.check_invariant());
	QUARK_ASSERT(type.check_invariant());

	const auto& type_peek = peek2(backend.types, type);

	const auto& struct_layout = find_struct_layout(backend, type_peek);
	const auto& struct_def = type_peek.get_struct(backend.types);
	const auto struct_base_ptr = encoded_value.struct_ptr->get_data_ptr();

	std::vector<value_t> members;
	int member_index = 0;
	for(const auto& e: struct_def._members){
		const auto offset = struct_layout.second.members[member_index].offset;
		const auto member_ptr = reinterpret_cast<const runtime_value_t*>(struct_base_ptr + offset);
		const auto member_value = from_runtime_value2(backend, *member_ptr, e._type);
		members.push_back(member_value);
		member_index++;
	}
	return value_t::make_struct_value(backend.types, type_peek, members);
}


runtime_value_t to_runtime_vector(value_backend_t& backend, const value_t& value){
	QUARK_ASSERT(backend.check_invariant());
	QUARK_ASSERT(value.check_invariant());

	const auto& type = value.get_type();
	const auto& type_peek = peek2(backend.types, type);
	QUARK_ASSERT(type_peek.is_vector());

	const auto& v0 = value.get_vector_value();
	const auto count = v0.size();

	if(is_vector_carray(backend.types, backend.config, type)){
		auto result = alloc_vector_carray(backend.heap, count, count, type);

		const auto element_type = type_peek.get_vector_element_type(backend.types);
		auto p = result.vector_carray_ptr->get_element_ptr();
		for(int i = 0 ; i < count ; i++){
			const auto& e = v0[i];
			const auto a = to_runtime_value2(backend, e);
	//		retain_value(r, a, element_type);
			p[i] = a;
		}
		return result;
	}
	else if(is_vector_hamt(backend.types, backend.config, type)){
		std::vector<runtime_value_t> temp;
		for(int i = 0 ; i < count ; i++){
			const auto& e = v0[i];
			const auto a = to_runtime_value2(backend, e);
			temp.push_back(a);
		}
		auto result = alloc_vector_hamt(backend.heap, &temp[0], temp.size(), type);
		return result;
	}
	else{
		QUARK_ASSERT(false);
		throw std::exception();
	}
}

value_t from_runtime_vector(const value_backend_t& backend, const runtime_value_t encoded_value, const type_t& type){
	QUARK_ASSERT(backend.check_invariant());
	QUARK_ASSERT(encoded_value.check_invariant());
	QUARK_ASSERT(type.check_invariant());

	const auto& type_peek = peek2(backend.types, type);
	QUARK_ASSERT(type_peek.is_vector());

	if(is_vector_carray(backend.types, backend.config, type)){
		const auto element_type = type_peek.get_vector_element_type(backend.types);
		const auto vec = encoded_value.vector_carray_ptr;

		std::vector<value_t> elements;
		const auto count = vec->get_element_count();
		auto p = vec->get_element_ptr();
		for(int i = 0 ; i < count ; i++){
			const auto value_encoded = p[i];
			const auto value = from_runtime_value2(backend, value_encoded, element_type);
			elements.push_back(value);
		}
		const auto val = value_t::make_vector_value(backend.types, element_type, elements);
		return val;
	}
	else if(is_vector_hamt(backend.types, backend.config, type)){
		const auto element_type = type_peek.get_vector_element_type(backend.types);
		const auto vec = encoded_value.vector_hamt_ptr;

		std::vector<value_t> elements;
		const auto count = vec->get_element_count();
		for(int i = 0 ; i < count ; i++){
			const auto& value_encoded = vec->load_element(i);
			const auto value = from_runtime_value2(backend, value_encoded, element_type);
			elements.push_back(value);
		}
		const auto val = value_t::make_vector_value(backend.types, element_type, elements);
		return val;
	}
	else{
		QUARK_ASSERT(false);
		throw std::exception();
	}
}

runtime_value_t to_runtime_dict(value_backend_t& backend, const dict_t& exact_type, const value_t& value){
	QUARK_ASSERT(backend.check_invariant());
	QUARK_ASSERT(value.check_invariant());

	const auto type = value.get_type();
	const auto& type_peek = peek2(backend.types, type);
	QUARK_ASSERT(type_peek.is_dict());

	if(is_dict_cppmap(backend.types, backend.config, type)){
		const auto& v0 = value.get_dict_value();

		auto result = alloc_dict_cppmap(backend.heap, type);

		const auto element_type = type_peek.get_dict_value_type(backend.types);
		auto& m = result.dict_cppmap_ptr->get_map_mut();
		for(const auto& e: v0){
			const auto a = to_runtime_value2(backend, e.second);
			m.insert({ e.first, a });
		}
		return result;
	}
	else if(is_dict_hamt(backend.types, backend.config, type)){
		const auto& v0 = value.get_dict_value();

		auto result = alloc_dict_hamt(backend.heap, type);

		const auto element_type = type_peek.get_dict_value_type(backend.types);
		auto& m = result.dict_hamt_ptr->get_map_mut();
		for(const auto& e: v0){
			const auto a = to_runtime_value2(backend, e.second);
			m = m.set(e.first, a);
		}
		return result;
	}
	else{
		QUARK_ASSERT(false);
		throw std::exception();
	}
}

value_t from_runtime_dict(const value_backend_t& backend, const runtime_value_t encoded_value, const type_t& type){
	QUARK_ASSERT(backend.check_invariant());
	QUARK_ASSERT(encoded_value.check_invariant());
	QUARK_ASSERT(type.check_invariant());

	const auto& type_peek = peek2(backend.types, type);

	if(is_dict_cppmap(backend.types, backend.config, type)){
		const auto value_type = type_peek.get_dict_value_type(backend.types);
		const auto dict = encoded_value.dict_cppmap_ptr;

		std::map<std::string, value_t> values;
		const auto& map2 = dict->get_map();
		for(const auto& e: map2){
			const auto value = from_runtime_value2(backend, e.second, value_type);
			values.insert({ e.first, value} );
		}
		const auto val = value_t::make_dict_value(backend.types, value_type, values);
		return val;
	}
	else if(is_dict_hamt(backend.types, backend.config, type)){
		const auto value_type = type_peek.get_dict_value_type(backend.types);
		const auto dict = encoded_value.dict_hamt_ptr;

		std::map<std::string, value_t> values;
		const auto& map2 = dict->get_map();
		for(const auto& e: map2){
			const auto value = from_runtime_value2(backend, e.second, value_type);
			values.insert({ e.first, value} );
		}
		const auto val = value_t::make_dict_value(backend.types, value_type, values);
		return val;
	}
	else{
		QUARK_ASSERT(false);
		throw std::exception();
	}
}

runtime_value_t to_runtime_value2(value_backend_t& backend, const value_t& value){
	QUARK_ASSERT(backend.check_invariant());
	QUARK_ASSERT(value.check_invariant());

	const auto type = value.get_type();

	struct visitor_t {
		value_backend_t& backend;
		const value_t& value;

		runtime_value_t operator()(const undefined_t& e) const{
			return make_blank_runtime_value();
		}
		runtime_value_t operator()(const any_t& e) const{
			return make_blank_runtime_value();
		}

		runtime_value_t operator()(const void_t& e) const{
			return make_blank_runtime_value();
		}
		runtime_value_t operator()(const bool_t& e) const{
			return make_runtime_bool(value.get_bool_value());
		}
		runtime_value_t operator()(const int_t& e) const{
			return make_runtime_int(value.get_int_value());
		}
		runtime_value_t operator()(const double_t& e) const{
			return make_runtime_double(value.get_double_value());
		}
		runtime_value_t operator()(const string_t& e) const{
			return to_runtime_string2(backend, value.get_string_value());
		}

		runtime_value_t operator()(const json_type_t& e) const{
//			auto result = new json_t(value.get_json());
//			return runtime_value_t { .json_ptr = result };
			auto result = alloc_json(backend.heap, value.get_json());
			return runtime_value_t { .json_ptr = result };
		}
		runtime_value_t operator()(const typeid_type_t& e) const{
			const auto t0 = value.get_typeid_value();
			return make_runtime_typeid(t0);
		}

		runtime_value_t operator()(const struct_t& e) const{
			return to_runtime_struct(backend, e, value);
		}
		runtime_value_t operator()(const vector_t& e) const{
			return to_runtime_vector(backend, value);
		}
		runtime_value_t operator()(const dict_t& e) const{
			return to_runtime_dict(backend, e, value);
		}
		runtime_value_t operator()(const function_t& e) const{
			const auto id = find_function_by_name(backend, value.get_function_value());
			return runtime_value_t { .function_link_id = id };
		}
		runtime_value_t operator()(const symbol_ref_t& e) const {
			QUARK_ASSERT(false); throw std::exception();
		}
		runtime_value_t operator()(const named_type_t& e) const {
			QUARK_ASSERT(false); throw std::exception();
		}
	};
	return std::visit(visitor_t{ backend, value }, get_type_variant(backend.types, type));
}




value_t from_runtime_value2(const value_backend_t& backend, const runtime_value_t encoded_value, const type_t& type){
	QUARK_ASSERT(backend.check_invariant());
	QUARK_ASSERT(encoded_value.check_invariant());
	QUARK_ASSERT(type.check_invariant());

	struct visitor_t {
		const value_backend_t& backend;
		const runtime_value_t& encoded_value;
		const type_t& type;

		value_t operator()(const undefined_t& e) const{
			UNSUPPORTED();
		}
		value_t operator()(const any_t& e) const{
			UNSUPPORTED();
		}

		value_t operator()(const void_t& e) const{
			UNSUPPORTED();
		}
		value_t operator()(const bool_t& e) const{
			return value_t::make_bool(encoded_value.bool_value == 0 ? false : true);
		}
		value_t operator()(const int_t& e) const{
			return value_t::make_int(encoded_value.int_value);
		}
		value_t operator()(const double_t& e) const{
			return value_t::make_double(encoded_value.double_value);
		}
		value_t operator()(const string_t& e) const{
			return value_t::make_string(from_runtime_string2(backend, encoded_value));
		}

		value_t operator()(const json_type_t& e) const{
			if(encoded_value.json_ptr == nullptr){
				return value_t::make_json(json_t());
			}
			else{
				const auto& j = encoded_value.json_ptr->get_json();
				return value_t::make_json(j);
			}
		}
		value_t operator()(const typeid_type_t& e) const{
			const auto& type1 = lookup_type_ref(backend, encoded_value.typeid_itype);
			const auto& type2 = value_t::make_typeid_value(type1);
			return type2;
		}

		value_t operator()(const struct_t& e) const{
			return from_runtime_struct(backend, encoded_value, type);
		}
		value_t operator()(const vector_t& e) const{
			return from_runtime_vector(backend, encoded_value, type);
		}
		value_t operator()(const dict_t& e) const{
			return from_runtime_dict(backend, encoded_value, type);
		}
		value_t operator()(const function_t& e) const{
			const auto& func_link = lookup_func_link(backend, encoded_value);
			return value_t::make_function_value(backend.types, type, function_id_t { func_link.link_name.s });
		}
		value_t operator()(const symbol_ref_t& e) const {
			QUARK_ASSERT(false); throw std::exception();
		}
		value_t operator()(const named_type_t& e) const {
#if 1
			return from_runtime_value2(backend, encoded_value, peek2(backend.types, e.destination_type));
#else
			const auto result = from_runtime_value2(backend, encoded_value, e.destination_type);
			return value_t::replace_logical_type(result, type);
#endif
		}
	};
#if 1
	return std::visit(visitor_t{ backend, encoded_value, type }, get_type_variant(backend.types, type));
#else
	const auto result = std::visit(visitor_t{ backend, encoded_value, type }, get_type_variant(backend.types, type));
	QUARK_ASSERT(result.get_type() == type);
	return result;
#endif
}


//////////////////////////////////////////		value_t vs bc_value_t


runtime_value_t make_runtime_non_rc(const value_t& value){
	const auto& type = value.get_type();
	QUARK_ASSERT(type.check_invariant());

/*
	if(bt == base_type::k_undefined){
		return make_blank_runtime_value();
	}
	else if(bt == base_type::k_any){
		return make_blank_runtime_value();
	}
	else if(bt == base_type::k_void){
		return make_blank_runtime_value();
	}
	else if(bt == base_type::k_bool){
		return bc_value_t::make_bool(value.get_bool_value());
	}
	else if(bt == base_type::k_bool){
		return bc_value_t::make_bool(value.get_bool_value());
	}
	else if(bt == base_type::k_int){
		return bc_value_t::make_int(value.get_int_value());
	}
	else if(bt == base_type::k_double){
		return bc_value_t::make_double(value.get_double_value());
	}

	else if(bt == base_type::k_string){
		return bc_value_t::make_string(value.get_string_value());
	}
	else if(bt == base_type::k_json){
		return bc_value_t::make_json(value.get_json());
	}
	else if(bt == base_type::k_typeid){
		return bc_value_t::make_typeid_value(value.get_typeid_value());
	}
	else if(bt == base_type::k_struct){
		return bc_value_t::make_struct_value(logical_type, values_to_bcs(types, value.get_struct_value()->_member_values));
	}
*/

	if(type.is_undefined()){
		return make_blank_runtime_value();
	}
	else if(type.is_any()){
		return make_blank_runtime_value();
	}
	else if(type.is_void()){
		return make_blank_runtime_value();
	}
	else if(type.is_bool()){
		return make_runtime_bool(value.get_bool_value());
	}
	else if(type.is_int()){
		return make_runtime_int(value.get_int_value());
	}
	else if(type.is_double()){
		return make_runtime_double(value.get_double_value());
	}
	else if(type.is_typeid()){
		const auto t0 = value.get_typeid_value();
		return make_runtime_typeid(t0);
	}
	else if(type.is_json() && value.get_json().is_null()){
		return runtime_value_t { .json_ptr = nullptr };
	}
	else{
		QUARK_ASSERT(false);
		throw std::exception();
	}
}
bc_value_t make_non_rc(const value_t& value){
	QUARK_ASSERT(value.check_invariant());

	const auto r = make_runtime_non_rc(value);
	return bc_value_t(value.get_type(), r);
}


value_t bc_to_value(const value_backend_t& backend, const bc_value_t& value){
	return from_runtime_value2(backend, value._pod, value._type);
}

bc_value_t value_to_bc(value_backend_t& backend, const value_t& value){
	const auto a = to_runtime_value2(backend, value);
	return bc_value_t(backend, value.get_type(), a);
}

bc_value_t bc_from_runtime(value_backend_t& backend, runtime_value_t value, const type_t& type){
	return bc_value_t(backend, type, value);
}

runtime_value_t runtime_from_bc(value_backend_t& backend, const bc_value_t& value){
	return value._pod;
}


}	// floyd
