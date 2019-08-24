//
//  floyd_llvm_value_thunking.hpp
//  floyd
//
//  Created by Marcus Zetterquist on 2019-08-19.
//  Copyright © 2019 Marcus Zetterquist. All rights reserved.
//

#ifndef floyd_llvm_value_thunking_hpp
#define floyd_llvm_value_thunking_hpp


#include "value_backend.h"
#include "ast_value.h"


namespace floyd {


////////////////////////////////		struct_layout_t


struct struct_layout_t {
	std::vector<size_t> offsets;
	size_t size;
};



////////////////////////////////		value_mgr_t



struct value_mgr_t {
	value_mgr_t(
		const std::vector<std::pair<link_name_t, void*>>& native_func_lookup,
		const std::vector<std::pair<typeid_t, struct_layout_t>>& struct_layouts,
		const std::map<runtime_type_t, typeid_t>& itype_to_typeid
	) :
		heap(),
		itype_to_typeid(itype_to_typeid),
		native_func_lookup(native_func_lookup),
		struct_layouts(struct_layouts)
	{
	}

	bool check_invariant() const {
		QUARK_ASSERT(heap.check_invariant());
		return true;
	}


	////////////////////////////////		STATE

	heap_t heap;
	std::map<runtime_type_t, typeid_t> itype_to_typeid;
	std::vector<std::pair<link_name_t, void*>> native_func_lookup;
	std::vector<std::pair<typeid_t, struct_layout_t>> struct_layouts;
};


runtime_type_t lookup_runtime_type(const value_mgr_t& value_mgr, const typeid_t& type);
typeid_t lookup_type(const value_mgr_t& value_mgr, runtime_type_t itype);


runtime_value_t to_runtime_string2(value_mgr_t& value_mgr, const std::string& s);
std::string from_runtime_string2(const value_mgr_t& value_mgr, runtime_value_t encoded_value);

runtime_value_t to_runtime_value2(value_mgr_t& value_mgr, const value_t& value);
value_t from_runtime_value2(const value_mgr_t& value_mgr, const runtime_value_t encoded_value, const typeid_t& type);






void retain_value(value_mgr_t& value_mgr, runtime_value_t value, const typeid_t& type);
void release_deep(value_mgr_t& value_mgr, runtime_value_t value, const typeid_t& type);
void release_dict_deep(value_mgr_t& value_mgr, DICT_CPPMAP_T* dict, const typeid_t& type);
void release_vec_deep(value_mgr_t& value_mgr, runtime_value_t& vec, const typeid_t& type);
void release_struct_deep(value_mgr_t& value_mgr, STRUCT_T* s, const typeid_t& type);



runtime_value_t concat_strings(value_mgr_t& value_mgr, const runtime_value_t& lhs, const runtime_value_t& rhs);
runtime_value_t concat_vector_cppvector(value_mgr_t& value_mgr, const typeid_t& type, const runtime_value_t& lhs, const runtime_value_t& rhs);
runtime_value_t concat_vector_hamt(value_mgr_t& value_mgr, const typeid_t& type, const runtime_value_t& lhs, const runtime_value_t& rhs);



/*
	vector_cppvector_pod
	vector_cppvector_rc

	vector_hamt_pod
	vector_hamt_rc
*/

inline bool is_vector_cppvector(const typeid_t& t){
	return t.is_vector() && k_global_vector_type == vector_backend::cppvector;
}
inline bool is_vector_hamt(const typeid_t& t){
	return t.is_vector() && k_global_vector_type == vector_backend::hamt;
}




}	// floyd

#endif /* floyd_llvm_value_thunking_hpp */
