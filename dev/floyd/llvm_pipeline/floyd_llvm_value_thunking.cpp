//
//  floyd_llvm_value_thunking.cpp
//  floyd
//
//  Created by Marcus Zetterquist on 2019-08-19.
//  Copyright © 2019 Marcus Zetterquist. All rights reserved.
//

#include "floyd_llvm_value_thunking.h"




namespace floyd {




////////////////////////////////		VALUES




static runtime_value_t to_runtime_string0(heap_t& heap, const std::string& s){
	QUARK_ASSERT(heap.check_invariant());

	const auto count = static_cast<uint64_t>(s.size());
	const auto allocation_count = size_to_allocation_blocks(s.size());
	auto result = alloc_vector_ccpvector2(heap, allocation_count, count);

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
	heap_t heap;
	const auto a = to_runtime_string0(heap, "hello, world!");

	QUARK_UT_VERIFY(a.vector_cppvector_ptr->get_element_count() == 13);

	//	int64_t	'hello, w'
//	QUARK_UT_VERIFY(a.vector_cppvector_ptr->operator[](0).int_value == 0x68656c6c6f2c2077);
	QUARK_UT_VERIFY(a.vector_cppvector_ptr->operator[](0).int_value == 0x77202c6f6c6c6568);

	//int_value	int64_t	'orld!\0\0\0'
//	QUARK_UT_VERIFY(a.vector_cppvector_ptr->operator[](1).int_value == 0x6f726c6421000000);
	QUARK_UT_VERIFY(a.vector_cppvector_ptr->operator[](1).int_value == 0x00000021646c726f);
}



static std::string from_runtime_string0(runtime_value_t encoded_value){
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
	heap_t heap;
	const auto a = to_runtime_string0(heap, "hello, world!");

	const auto r = from_runtime_string0(a);
	QUARK_UT_VERIFY(r == "hello, world!");
}





runtime_value_t to_runtime_string2(value_mgr_t& value_mgr, const std::string& s){
	QUARK_ASSERT(value_mgr.check_invariant());

	return to_runtime_string0(value_mgr.heap, s);
}



std::string from_runtime_string2(const value_mgr_t& value_mgr, runtime_value_t encoded_value){
	QUARK_ASSERT(value_mgr.check_invariant());
	QUARK_ASSERT(encoded_value.check_invariant());

	return from_runtime_string0(encoded_value);
}


static runtime_value_t to_runtime_struct(value_mgr_t& value_mgr, const typeid_t::struct_t& exact_type, const value_t& value){
	QUARK_ASSERT(value_mgr.check_invariant());
	QUARK_ASSERT(value.check_invariant());

	auto t2 = get_exact_struct_type(value_mgr.type_lookup, value.get_type());
	const llvm::StructLayout* layout = value_mgr.data_layout.getStructLayout(t2);

	const auto struct_bytes = layout->getSizeInBytes();

	auto s = alloc_struct(value_mgr.heap, struct_bytes);
	const auto struct_base_ptr = s->get_data_ptr();

	int member_index = 0;
	const auto& struct_data = value.get_struct_value();

	for(const auto& e: struct_data->_member_values){
		const auto offset = layout->getElementOffset(member_index);
		const auto member_ptr = reinterpret_cast<void*>(struct_base_ptr + offset);
		store_via_ptr2(member_ptr, e.get_type(), to_runtime_value2(value_mgr, e));
		member_index++;
	}
	return make_runtime_struct(s);
}

static value_t from_runtime_struct(const value_mgr_t& value_mgr, const runtime_value_t encoded_value, const typeid_t& type){
	QUARK_ASSERT(value_mgr.check_invariant());
	QUARK_ASSERT(encoded_value.check_invariant());
	QUARK_ASSERT(type.check_invariant());

	const auto& struct_def = type.get_struct();
	const auto struct_base_ptr = encoded_value.struct_ptr->get_data_ptr();

	auto t2 = get_exact_struct_type(value_mgr.type_lookup, type);
	const llvm::StructLayout* layout = value_mgr.data_layout.getStructLayout(t2);

	std::vector<value_t> members;
	int member_index = 0;
	for(const auto& e: struct_def._members){
		const auto offset = layout->getElementOffset(member_index);
		const auto member_ptr = reinterpret_cast<const runtime_value_t*>(struct_base_ptr + offset);
		const auto member_value = from_runtime_value2(value_mgr, *member_ptr, e._type);
		members.push_back(member_value);
		member_index++;
	}
	return value_t::make_struct_value(type, members);
}


static runtime_value_t to_runtime_vector(value_mgr_t& value_mgr, const value_t& value){
	QUARK_ASSERT(value_mgr.check_invariant());
	QUARK_ASSERT(value.check_invariant());
	QUARK_ASSERT(value.get_type().is_vector());

	const auto& v0 = value.get_vector_value();
	const auto count = v0.size();

	if(is_vector_cppvector(value.get_type())){
		auto result = alloc_vector_ccpvector2(value_mgr.heap, count, count);

		const auto element_type = value.get_type().get_vector_element_type();
		auto p = result.vector_cppvector_ptr->get_element_ptr();
		for(int i = 0 ; i < count ; i++){
			const auto& e = v0[i];
			const auto a = to_runtime_value2(value_mgr, e);
	//		retain_value(r, a, element_type);
			p[i] = a;
		}
		return result;
	}
	else if(is_vector_hamt(value.get_type())){
		std::vector<runtime_value_t> temp;
		for(int i = 0 ; i < count ; i++){
			const auto& e = v0[i];
			const auto a = to_runtime_value2(value_mgr, e);
			temp.push_back(a);
		}
		auto result = alloc_vector_hamt2(value_mgr.heap, &temp[0], temp.size());
		return result;
	}
	else{
		QUARK_ASSERT(false);
		throw std::exception();
	}
	__dead2;
}

static value_t from_runtime_vector(const value_mgr_t& value_mgr, const runtime_value_t encoded_value, const typeid_t& type){
	QUARK_ASSERT(value_mgr.check_invariant());
	QUARK_ASSERT(encoded_value.check_invariant());
	QUARK_ASSERT(type.check_invariant());
	QUARK_ASSERT(type.is_vector());

	const auto element_type = type.get_vector_element_type();
	if(is_vector_cppvector(type)){
		const auto vec = encoded_value.vector_cppvector_ptr;

		std::vector<value_t> elements;
		const auto count = vec->get_element_count();
		auto p = vec->get_element_ptr();
		for(int i = 0 ; i < count ; i++){
			const auto value_encoded = p[i];
			const auto value = from_runtime_value2(value_mgr, value_encoded, element_type);
			elements.push_back(value);
		}
		const auto val = value_t::make_vector_value(element_type, elements);
		return val;
	}
	else if(is_vector_hamt(type)){
		const auto vec = encoded_value.vector_hamt_ptr;

		std::vector<value_t> elements;
		const auto count = vec->get_element_count();
		for(int i = 0 ; i < count ; i++){
			const auto& value_encoded = vec->operator[](i);
			const auto value = from_runtime_value2(value_mgr, value_encoded, element_type);
			elements.push_back(value);
		}
		const auto val = value_t::make_vector_value(element_type, elements);
		return val;
	}
	else{
		QUARK_ASSERT(false);
	}
}

static runtime_value_t to_runtime_dict(value_mgr_t& value_mgr, const typeid_t::dict_t& exact_type, const value_t& value){
	QUARK_ASSERT(value_mgr.check_invariant());
	QUARK_ASSERT(value.check_invariant());
	QUARK_ASSERT(value.get_type().is_dict());

	const auto& v0 = value.get_dict_value();

	auto result = alloc_dict_cppmap2(value_mgr.heap);

	const auto element_type = value.get_type().get_dict_value_type();
	auto& m = result.dict_cppmap_ptr->get_map_mut();
	for(const auto& e: v0){
		const auto a = to_runtime_value2(value_mgr, e.second);
		m.insert({ e.first, a });
	}
	return result;
}

static value_t from_runtime_dict(const value_mgr_t& value_mgr, const runtime_value_t encoded_value, const typeid_t& type){
	QUARK_ASSERT(value_mgr.check_invariant());
	QUARK_ASSERT(encoded_value.check_invariant());
	QUARK_ASSERT(type.check_invariant());

	const auto value_type = type.get_dict_value_type();
	const auto dict = encoded_value.dict_cppmap_ptr;

	std::map<std::string, value_t> values;
	const auto& map2 = dict->get_map();
	for(const auto& e: map2){
		const auto value = from_runtime_value2(value_mgr, e.second, value_type);
		values.insert({ e.first, value} );
	}
	const auto val = value_t::make_dict_value(type, values);
	return val;
}

runtime_value_t to_runtime_value2(value_mgr_t& value_mgr, const value_t& value){
	QUARK_ASSERT(value_mgr.check_invariant());
	QUARK_ASSERT(value.check_invariant());

	const auto type = value.get_type();

	struct visitor_t {
		value_mgr_t& value_mgr;
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
			return to_runtime_string2(value_mgr, value.get_string_value());
		}

		runtime_value_t operator()(const typeid_t::json_type_t& e) const{
//			auto result = new json_t(value.get_json());
//			return runtime_value_t { .json_ptr = result };
			auto result = alloc_json(value_mgr.heap, value.get_json());
			return runtime_value_t { .json_ptr = result };
		}
		runtime_value_t operator()(const typeid_t::typeid_type_t& e) const{
			const auto t0 = value.get_typeid_value();
			const auto t1 = lookup_runtime_type(value_mgr.type_lookup, t0);
			return make_runtime_typeid(t1);
		}

		runtime_value_t operator()(const typeid_t::struct_t& e) const{
			return to_runtime_struct(value_mgr, e, value);
		}
		runtime_value_t operator()(const typeid_t::vector_t& e) const{
			return to_runtime_vector(value_mgr, value);
		}
		runtime_value_t operator()(const typeid_t::dict_t& e) const{
			return to_runtime_dict(value_mgr, e, value);
		}
		runtime_value_t operator()(const typeid_t::function_t& e) const{
			NOT_IMPLEMENTED_YET();
			QUARK_ASSERT(false);
		}
		runtime_value_t operator()(const typeid_t::unresolved_t& e) const{
			UNSUPPORTED();
		}
	};
	return std::visit(visitor_t{ value_mgr, value }, type._contents);
}












static link_name_t native_func_ptr_to_link_name(const value_mgr_t& value_mgr, void* f){
	QUARK_ASSERT(value_mgr.check_invariant());
	QUARK_ASSERT(f != nullptr);

	const auto& function_vec = value_mgr.native_func_lookup;
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


value_t from_runtime_value2(const value_mgr_t& value_mgr, const runtime_value_t encoded_value, const typeid_t& type){
	QUARK_ASSERT(value_mgr.check_invariant());
	QUARK_ASSERT(encoded_value.check_invariant());
	QUARK_ASSERT(type.check_invariant());

	struct visitor_t {
		const value_mgr_t& value_mgr;
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
			return value_t::make_string(from_runtime_string2(value_mgr, encoded_value));
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
			const auto type1 = lookup_type(value_mgr.type_lookup, encoded_value.typeid_itype);
			const auto type2 = value_t::make_typeid_value(type1);
			return type2;
		}

		value_t operator()(const typeid_t::struct_t& e) const{
			return from_runtime_struct(value_mgr, encoded_value, type);
		}
		value_t operator()(const typeid_t::vector_t& e) const{
			return from_runtime_vector(value_mgr, encoded_value, type);
		}
		value_t operator()(const typeid_t::dict_t& e) const{
			return from_runtime_dict(value_mgr, encoded_value, type);
		}
		value_t operator()(const typeid_t::function_t& e) const{
			const auto link_name = native_func_ptr_to_link_name(value_mgr, encoded_value.function_ptr);
			return value_t::make_function_value(type, function_id_t { link_name.s });
		}
		value_t operator()(const typeid_t::unresolved_t& e) const{
			UNSUPPORTED();
		}
	};
	return std::visit(visitor_t{ value_mgr, encoded_value, type }, type._contents);
}
















void retain_value(value_mgr_t& value_mgr, runtime_value_t value, const typeid_t& type){
	QUARK_ASSERT(value_mgr.check_invariant());
	QUARK_ASSERT(value.check_invariant());
	QUARK_ASSERT(type.check_invariant());

	if(is_rc_value(type)){
		if(type.is_string()){
			inc_rc(value.vector_cppvector_ptr->alloc);
		}
		else if(is_vector_cppvector(type)){
			inc_rc(value.vector_cppvector_ptr->alloc);
		}
		else if(is_vector_hamt(type)){
			inc_rc(value.vector_hamt_ptr->alloc);
		}
		else if(type.is_dict()){
			inc_rc(value.dict_cppmap_ptr->alloc);
		}
		else if(type.is_json()){
			inc_rc(value.json_ptr->alloc);
		}
		else if(type.is_struct()){
			inc_rc(value.struct_ptr->alloc);
		}
		else{
			QUARK_ASSERT(false);
		}
	}
}

void release_deep(value_mgr_t& value_mgr, runtime_value_t value, const typeid_t& type){
	QUARK_ASSERT(value_mgr.check_invariant());
	QUARK_ASSERT(value.check_invariant());
	QUARK_ASSERT(type.check_invariant());

	if(is_rc_value(type)){
		if(type.is_string()){
			release_vec_deep(value_mgr, value, type);
		}
		else if(type.is_vector()){
			release_vec_deep(value_mgr, value, type);
		}
		else if(type.is_dict()){
			release_dict_deep(value_mgr, value.dict_cppmap_ptr, type);
		}
		else if(type.is_json()){
			if(dec_rc(value.json_ptr->alloc) == 0){
				dispose_json(*value.json_ptr);
			}
		}
		else if(type.is_struct()){
			release_struct_deep(value_mgr, value.struct_ptr, type);
		}
		else{
			QUARK_ASSERT(false);
		}
	}
}

void release_dict_deep(value_mgr_t& value_mgr, DICT_CPPMAP_T* dict, const typeid_t& type){
	QUARK_ASSERT(value_mgr.check_invariant());
	QUARK_ASSERT(dict != nullptr);
	QUARK_ASSERT(type.is_dict());

	if(dec_rc(dict->alloc) == 0){

		//	Release all elements.
		const auto element_type = type.get_dict_value_type();
		if(is_rc_value(element_type)){
			auto m = dict->get_map();
			for(const auto& e: m){
				release_deep(value_mgr, e.second, element_type);
			}
		}
		auto temp = make_runtime_dict_cppmap(dict);
		dispose_dict_cppmap(temp);
	}
}


void release_vec_deep(value_mgr_t& value_mgr, runtime_value_t& vec, const typeid_t& type){
	QUARK_ASSERT(value_mgr.check_invariant());
	QUARK_ASSERT(vec.check_invariant());
	QUARK_ASSERT(type.is_string() || type.is_vector());

	if(type.is_string()){
		if(dec_rc(vec.vector_cppvector_ptr->alloc) == 0){
			//	String has no elements to release.

			dispose_vector_cppvector(vec);
		}
	}
	else if(is_vector_cppvector(type)){
		if(dec_rc(vec.vector_cppvector_ptr->alloc) == 0){
			//	Release all elements.
			{
				const auto element_type = type.get_vector_element_type();
				if(is_rc_value(element_type)){
					auto element_ptr = vec.vector_cppvector_ptr->get_element_ptr();
					for(int i = 0 ; i < vec.vector_cppvector_ptr->get_element_count() ; i++){
						const auto& element = element_ptr[i];
						release_deep(value_mgr, element, element_type);
					}
				}
			}
			dispose_vector_cppvector(vec);
		}
	}
	else if(is_vector_hamt(type)){
		if(dec_rc(vec.vector_hamt_ptr->alloc) == 0){
			//	Release all elements.
			{
				const auto element_type = type.get_vector_element_type();
				if(is_rc_value(element_type)){
					for(int i = 0 ; i < vec.vector_hamt_ptr->get_element_count() ; i++){
						const auto& element = vec.vector_hamt_ptr->operator[](i);
						release_deep(value_mgr, element, element_type);
					}
				}
			}
			dispose_vector_hamt(vec);
		}
	}
	else{
		QUARK_ASSERT(false);
	}
}







void release_struct_deep(value_mgr_t& value_mgr, STRUCT_T* s, const typeid_t& type){
	QUARK_ASSERT(value_mgr.check_invariant());
	QUARK_ASSERT(s != nullptr);
	QUARK_ASSERT(type.check_invariant());
	QUARK_ASSERT(type.is_struct());

	if(dec_rc(s->alloc) == 0){
		const auto& struct_def = type.get_struct();
		const auto struct_base_ptr = s->get_data_ptr();

		auto t2 = get_exact_struct_type(value_mgr.type_lookup, type);
		const llvm::StructLayout* layout = value_mgr.data_layout.getStructLayout(t2);

		int member_index = 0;
		for(const auto& e: struct_def._members){
			if(is_rc_value(e._type)){
				const auto offset = layout->getElementOffset(member_index);
				const auto member_ptr = reinterpret_cast<const runtime_value_t*>(struct_base_ptr + offset);
				release_deep(value_mgr, *member_ptr, e._type);
			}
			member_index++;
		}
		dispose_struct(*s);
	}
}







runtime_value_t concat_strings(value_mgr_t& value_mgr, const runtime_value_t& lhs, const runtime_value_t& rhs){
	QUARK_ASSERT(value_mgr.check_invariant());
	QUARK_ASSERT(lhs.check_invariant());
	QUARK_ASSERT(rhs.check_invariant());

	const auto result = from_runtime_string2(value_mgr, lhs) + from_runtime_string2(value_mgr, rhs);
	return to_runtime_string2(value_mgr, result);
}

runtime_value_t concat_vector_cppvector(value_mgr_t& value_mgr, const typeid_t& type, const runtime_value_t& lhs, const runtime_value_t& rhs){
	QUARK_ASSERT(value_mgr.check_invariant());
	QUARK_ASSERT(type.check_invariant());
	QUARK_ASSERT(lhs.check_invariant());
	QUARK_ASSERT(rhs.check_invariant());

	const auto count2 = lhs.vector_cppvector_ptr->get_element_count() + rhs.vector_cppvector_ptr->get_element_count();

	auto result = alloc_vector_ccpvector2(value_mgr.heap, count2, count2);

	//??? warning: assumes element = allocation.

	const auto element_type = type.get_vector_element_type();

	auto dest_ptr = result.vector_cppvector_ptr->get_element_ptr();
	auto dest_ptr2 = dest_ptr + lhs.vector_cppvector_ptr->get_element_count();
	auto lhs_ptr = lhs.vector_cppvector_ptr->get_element_ptr();
	auto rhs_ptr = rhs.vector_cppvector_ptr->get_element_ptr();

	if(is_rc_value(element_type)){
		for(int i = 0 ; i < lhs.vector_cppvector_ptr->get_element_count() ; i++){
			retain_value(value_mgr, lhs_ptr[i], element_type);
			dest_ptr[i] = lhs_ptr[i];
		}
		for(int i = 0 ; i < rhs.vector_cppvector_ptr->get_element_count() ; i++){
			retain_value(value_mgr, rhs_ptr[i], element_type);
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

runtime_value_t concat_vector_hamt(value_mgr_t& value_mgr, const typeid_t& type, const runtime_value_t& lhs, const runtime_value_t& rhs){
	QUARK_ASSERT(value_mgr.check_invariant());
	QUARK_ASSERT(type.check_invariant());
	QUARK_ASSERT(lhs.check_invariant());
	QUARK_ASSERT(rhs.check_invariant());

	const auto lhs_count = lhs.vector_hamt_ptr->get_element_count();
	const auto rhs_count = rhs.vector_hamt_ptr->get_element_count();

	const auto count2 = lhs_count + rhs_count;

	auto result = alloc_vector_hamt2(value_mgr.heap, count2, count2);

	//??? warning: assumes element = allocation.

	const auto element_type = type.get_vector_element_type();

	//??? Causes a full path copy for EACH ELEMENT = slow. better to make new hamt in one go.
	if(is_rc_value(element_type)){
		for(int i = 0 ; i < lhs_count ; i++){
			auto value = lhs.vector_hamt_ptr->operator[](i);
			retain_value(value_mgr, value, element_type);
			result.vector_hamt_ptr->store_mutate(i, value);
		}
		for(int i = 0 ; i < rhs_count ; i++){
			auto value = rhs.vector_hamt_ptr->operator[](i);
			retain_value(value_mgr, value, element_type);
			result.vector_hamt_ptr->store_mutate(lhs_count + i, value);
		}
	}
	else{
		for(int i = 0 ; i < lhs_count ; i++){
			auto value = lhs.vector_hamt_ptr->operator[](i);
			result.vector_hamt_ptr->store_mutate(i, value);
		}
		for(int i = 0 ; i < rhs_count ; i++){
			auto value = rhs.vector_hamt_ptr->operator[](i);
			result.vector_hamt_ptr->store_mutate(lhs_count + i, value);
		}
	}
	return result;
}




}	// floyd
