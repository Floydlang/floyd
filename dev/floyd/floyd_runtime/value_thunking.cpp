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




runtime_value_t to_runtime_string2(value_backend_t& backend, const std::string& s){
	QUARK_ASSERT(backend.check_invariant());

	const auto count = static_cast<uint64_t>(s.size());
	const auto allocation_count = size_to_allocation_blocks(s.size());
	auto result = alloc_vector_ccpvector2(backend.heap, allocation_count, count, lookup_itype(backend, typeid_t::make_string()));

	size_t char_pos = 0;
	int element_index = 0;
	uint64_t acc = 0;
	while(char_pos < s.size()){
		const uint64_t ch = s[char_pos];
		const auto x = (char_pos & 7) * 8;
		acc = acc | (ch << x);
		char_pos++;

		if(((char_pos & 7) == 0) || (char_pos == s.size())){
			result.vector_cppvector_ptr->store(element_index, make_runtime_int(static_cast<int64_t>(acc)));
			element_index = element_index + 1;
			acc = 0;
		}
	}
	return result;
}


QUARK_UNIT_TEST("VECTOR_CPPVECTOR_T", "", "", ""){
	auto backend = make_test_value_backend();
	const auto a = to_runtime_string2(backend, "hello, world!");

	QUARK_UT_VERIFY(a.vector_cppvector_ptr->get_element_count() == 13);

	//	int64_t	'hello, w'
//	QUARK_UT_VERIFY(a.vector_cppvector_ptr->load_element(0).int_value == 0x68656c6c6f2c2077);
	QUARK_UT_VERIFY(a.vector_cppvector_ptr->load_element(0).int_value == 0x77202c6f6c6c6568);

	//int_value	int64_t	'orld!\0\0\0'
//	QUARK_UT_VERIFY(a.vector_cppvector_ptr->load_element(1).int_value == 0x6f726c6421000000);
	QUARK_UT_VERIFY(a.vector_cppvector_ptr->load_element(1).int_value == 0x00000021646c726f);
}



std::string from_runtime_string2(const value_backend_t& backend, runtime_value_t encoded_value){
	QUARK_ASSERT(backend.check_invariant());
	QUARK_ASSERT(encoded_value.check_invariant());
	QUARK_ASSERT(encoded_value.vector_cppvector_ptr != nullptr);

	const size_t size = get_vec_string_size(encoded_value);

	std::string result;

	//	Read 8 characters at a time.
	size_t char_pos = 0;
	const auto begin0 = encoded_value.vector_cppvector_ptr->begin();
	const auto end0 = encoded_value.vector_cppvector_ptr->end();
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

QUARK_UNIT_TEST("VECTOR_CPPVECTOR_T", "", "", ""){
	auto backend = make_test_value_backend();
	const auto a = to_runtime_string2(backend, "hello, world!");

	const auto r = from_runtime_string2(backend, a);
	QUARK_UT_VERIFY(r == "hello, world!");
}











static runtime_value_t to_runtime_struct(value_backend_t& backend, const typeid_t::struct_t& exact_type, const value_t& value){
	QUARK_ASSERT(backend.check_invariant());
	QUARK_ASSERT(value.check_invariant());

	const auto& struct_layout = find_struct_layout(backend, value.get_type());

	auto s = alloc_struct(backend.heap, struct_layout.second.size, lookup_runtime_type(backend, value.get_type()));
	const auto struct_base_ptr = s->get_data_ptr();

	int member_index = 0;
	const auto& struct_data = value.get_struct_value();

	for(const auto& e: struct_data->_member_values){
		const auto offset = struct_layout.second.offsets[member_index];
		const auto member_ptr = reinterpret_cast<void*>(struct_base_ptr + offset);
		store_via_ptr2(member_ptr, e.get_type(), to_runtime_value2(backend, e));
		member_index++;
	}
	return make_runtime_struct(s);
}

static value_t from_runtime_struct(const value_backend_t& backend, const runtime_value_t encoded_value, const typeid_t& type){
	QUARK_ASSERT(backend.check_invariant());
	QUARK_ASSERT(encoded_value.check_invariant());
	QUARK_ASSERT(type.check_invariant());

	const auto& struct_layout = find_struct_layout(backend, type);

	const auto& struct_def = type.get_struct();
	const auto struct_base_ptr = encoded_value.struct_ptr->get_data_ptr();

	std::vector<value_t> members;
	int member_index = 0;
	for(const auto& e: struct_def._members){
		const auto offset = struct_layout.second.offsets[member_index];
		const auto member_ptr = reinterpret_cast<const runtime_value_t*>(struct_base_ptr + offset);
		const auto member_value = from_runtime_value2(backend, *member_ptr, e._type);
		members.push_back(member_value);
		member_index++;
	}
	return value_t::make_struct_value(type, members);
}


static runtime_value_t to_runtime_vector(value_backend_t& backend, const value_t& value){
	QUARK_ASSERT(backend.check_invariant());
	QUARK_ASSERT(value.check_invariant());
	QUARK_ASSERT(value.get_type().is_vector());

	const auto& v0 = value.get_vector_value();
	const auto count = v0.size();

	if(is_vector_cppvector(value.get_type())){
		auto result = alloc_vector_ccpvector2(backend.heap, count, count, lookup_itype(backend, value.get_type()));

		const auto element_type = value.get_type().get_vector_element_type();
		auto p = result.vector_cppvector_ptr->get_element_ptr();
		for(int i = 0 ; i < count ; i++){
			const auto& e = v0[i];
			const auto a = to_runtime_value2(backend, e);
	//		retain_value(r, a, element_type);
			p[i] = a;
		}
		return result;
	}
	else if(is_vector_hamt(value.get_type())){
		std::vector<runtime_value_t> temp;
		for(int i = 0 ; i < count ; i++){
			const auto& e = v0[i];
			const auto a = to_runtime_value2(backend, e);
			temp.push_back(a);
		}
		auto result = alloc_vector_hamt(backend.heap, &temp[0], temp.size(), lookup_runtime_type(backend, value.get_type()));
		return result;
	}
	else{
		QUARK_ASSERT(false);
		throw std::exception();
	}
}

static value_t from_runtime_vector(const value_backend_t& backend, const runtime_value_t encoded_value, const typeid_t& type){
	QUARK_ASSERT(backend.check_invariant());
	QUARK_ASSERT(encoded_value.check_invariant());
	QUARK_ASSERT(type.check_invariant());
	QUARK_ASSERT(type.is_vector());

	if(is_vector_cppvector(type)){
		const auto element_type = type.get_vector_element_type();
		const auto vec = encoded_value.vector_cppvector_ptr;

		std::vector<value_t> elements;
		const auto count = vec->get_element_count();
		auto p = vec->get_element_ptr();
		for(int i = 0 ; i < count ; i++){
			const auto value_encoded = p[i];
			const auto value = from_runtime_value2(backend, value_encoded, element_type);
			elements.push_back(value);
		}
		const auto val = value_t::make_vector_value(element_type, elements);
		return val;
	}
	else if(is_vector_hamt(type)){
		const auto element_type = type.get_vector_element_type();
		const auto vec = encoded_value.vector_hamt_ptr;

		std::vector<value_t> elements;
		const auto count = vec->get_element_count();
		for(int i = 0 ; i < count ; i++){
			const auto& value_encoded = vec->load_element(i);
			const auto value = from_runtime_value2(backend, value_encoded, element_type);
			elements.push_back(value);
		}
		const auto val = value_t::make_vector_value(element_type, elements);
		return val;
	}
	else{
		QUARK_ASSERT(false);
		throw std::exception();
	}
}

static runtime_value_t to_runtime_dict(value_backend_t& backend, const typeid_t::dict_t& exact_type, const value_t& value){
	QUARK_ASSERT(backend.check_invariant());
	QUARK_ASSERT(value.check_invariant());
	QUARK_ASSERT(value.get_type().is_dict());

	if(is_dict_cppmap(value.get_type())){
		const auto& v0 = value.get_dict_value();

		auto result = alloc_dict_cppmap(backend.heap, lookup_runtime_type(backend, value.get_type()));

		const auto element_type = value.get_type().get_dict_value_type();
		auto& m = result.dict_cppmap_ptr->get_map_mut();
		for(const auto& e: v0){
			const auto a = to_runtime_value2(backend, e.second);
			m.insert({ e.first, a });
		}
		return result;
	}
	else if(is_dict_hamt(value.get_type())){
		const auto& v0 = value.get_dict_value();

		auto result = alloc_dict_hamt(backend.heap, lookup_runtime_type(backend, value.get_type()));

		const auto element_type = value.get_type().get_dict_value_type();
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

static value_t from_runtime_dict(const value_backend_t& backend, const runtime_value_t encoded_value, const typeid_t& type){
	QUARK_ASSERT(backend.check_invariant());
	QUARK_ASSERT(encoded_value.check_invariant());
	QUARK_ASSERT(type.check_invariant());

	if(is_dict_cppmap(type)){
		const auto value_type = type.get_dict_value_type();
		const auto dict = encoded_value.dict_cppmap_ptr;

		std::map<std::string, value_t> values;
		const auto& map2 = dict->get_map();
		for(const auto& e: map2){
			const auto value = from_runtime_value2(backend, e.second, value_type);
			values.insert({ e.first, value} );
		}
		const auto val = value_t::make_dict_value(type, values);
		return val;
	}
	else if(is_dict_hamt(type)){
		const auto value_type = type.get_dict_value_type();
		const auto dict = encoded_value.dict_hamt_ptr;

		std::map<std::string, value_t> values;
		const auto& map2 = dict->get_map();
		for(const auto& e: map2){
			const auto value = from_runtime_value2(backend, e.second, value_type);
			values.insert({ e.first, value} );
		}
		const auto val = value_t::make_dict_value(type, values);
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
			return make_runtime_bool(value.get_bool_value());
		}
		runtime_value_t operator()(const typeid_t::int_t& e) const{
			return make_runtime_int(value.get_int_value());
		}
		runtime_value_t operator()(const typeid_t::double_t& e) const{
			return make_runtime_double(value.get_double_value());
		}
		runtime_value_t operator()(const typeid_t::string_t& e) const{
			return to_runtime_string2(backend, value.get_string_value());
		}

		runtime_value_t operator()(const typeid_t::json_type_t& e) const{
//			auto result = new json_t(value.get_json());
//			return runtime_value_t { .json_ptr = result };
			auto result = alloc_json(backend.heap, value.get_json());
			return runtime_value_t { .json_ptr = result };
		}
		runtime_value_t operator()(const typeid_t::typeid_type_t& e) const{
			const auto t0 = value.get_typeid_value();
			const auto t1 = lookup_runtime_type(backend, t0);
			return make_runtime_typeid(t1);
		}

		runtime_value_t operator()(const typeid_t::struct_t& e) const{
			return to_runtime_struct(backend, e, value);
		}
		runtime_value_t operator()(const typeid_t::vector_t& e) const{
			return to_runtime_vector(backend, value);
		}
		runtime_value_t operator()(const typeid_t::dict_t& e) const{
			return to_runtime_dict(backend, e, value);
		}
		runtime_value_t operator()(const typeid_t::function_t& e) const{
			NOT_IMPLEMENTED_YET();
			QUARK_ASSERT(false);
		}
		runtime_value_t operator()(const typeid_t::unresolved_t& e) const{
			UNSUPPORTED();
		}
	};
	return std::visit(visitor_t{ backend, value }, type._contents);
}












static link_name_t native_func_ptr_to_link_name(const value_backend_t& backend, void* f){
	QUARK_ASSERT(backend.check_invariant());
	QUARK_ASSERT(f != nullptr);

	const auto& function_vec = backend.native_func_lookup;
	const auto it = std::find_if(
		function_vec.begin(),
		function_vec.end(),
		[&] (auto& e) {
			return f == e.second;
		}
	);
	if(it != function_vec.end()){
		return it->first;
	}
	else{
		return link_name_t { "" };
	}
}


value_t from_runtime_value2(const value_backend_t& backend, const runtime_value_t encoded_value, const typeid_t& type){
	QUARK_ASSERT(backend.check_invariant());
	QUARK_ASSERT(encoded_value.check_invariant());
	QUARK_ASSERT(type.check_invariant());

	struct visitor_t {
		const value_backend_t& backend;
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
			return value_t::make_string(from_runtime_string2(backend, encoded_value));
		}

		value_t operator()(const typeid_t::json_type_t& e) const{
			if(encoded_value.json_ptr == nullptr){
				return value_t::make_json(json_t());
			}
			else{
				const auto& j = encoded_value.json_ptr->get_json();
				return value_t::make_json(j);
			}
		}
		value_t operator()(const typeid_t::typeid_type_t& e) const{
			const auto type1 = lookup_type(backend, encoded_value.typeid_itype);
			const auto type2 = value_t::make_typeid_value(type1);
			return type2;
		}

		value_t operator()(const typeid_t::struct_t& e) const{
			return from_runtime_struct(backend, encoded_value, type);
		}
		value_t operator()(const typeid_t::vector_t& e) const{
			return from_runtime_vector(backend, encoded_value, type);
		}
		value_t operator()(const typeid_t::dict_t& e) const{
			return from_runtime_dict(backend, encoded_value, type);
		}
		value_t operator()(const typeid_t::function_t& e) const{
			const auto link_name = native_func_ptr_to_link_name(backend, encoded_value.function_ptr);
			return value_t::make_function_value(type, function_id_t { link_name.s });
		}
		value_t operator()(const typeid_t::unresolved_t& e) const{
			UNSUPPORTED();
		}
	};
	return std::visit(visitor_t{ backend, encoded_value, type }, type._contents);
}

}	// floyd
