//
//  value_features.hpp
//  floyd
//
//  Created by Marcus Zetterquist on 2019-08-24.
//  Copyright © 2019 Marcus Zetterquist. All rights reserved.
//

#ifndef value_features_hpp
#define value_features_hpp

#include "value_backend.h"
#include "value_thunking.h"

namespace floyd {




VECTOR_CARRAY_T* unpack_vector_carray_arg(const value_backend_t& backend, runtime_value_t arg, runtime_type_t arg_type);
DICT_CPPMAP_T* unpack_dict_cppmap_arg(const value_backend_t& backend, runtime_value_t arg, runtime_type_t arg_type);


int compare_values(value_backend_t& backend, int64_t op, const runtime_type_t type, runtime_value_t lhs, runtime_value_t rhs);





inline const runtime_value_t update__string(value_backend_t& backend, runtime_value_t s, runtime_value_t key_value, runtime_value_t value);



const runtime_value_t update__vector_carray(value_backend_t& backend, runtime_value_t coll_value, runtime_type_t coll_type, runtime_value_t index, runtime_value_t value);
inline const runtime_value_t update__vector_hamt_pod(value_backend_t& backend, runtime_value_t coll_value, runtime_type_t coll_type, runtime_value_t index, runtime_value_t value, runtime_type_t value_type);
inline const runtime_value_t update__vector_hamt_nonpod(value_backend_t& backend, runtime_value_t coll_value, runtime_type_t coll_type, runtime_value_t index, runtime_value_t value, runtime_type_t value_type);

runtime_value_t update_vector(value_backend_t& backend, runtime_value_t obj1, runtime_type_t coll_type, runtime_value_t index, runtime_value_t value);




const runtime_value_t update__dict_cppmap(value_backend_t& backend, runtime_value_t coll_value, runtime_type_t coll_type, runtime_value_t key_value, runtime_value_t value);
const runtime_value_t update__dict_hamt(value_backend_t& backend, runtime_value_t coll_value, runtime_type_t coll_type, runtime_value_t key_value, runtime_value_t value);

runtime_value_t update_dict(value_backend_t& backend, runtime_value_t obj1, runtime_type_t coll_type, runtime_value_t key_value, runtime_value_t value);



const runtime_value_t subset__string(value_backend_t& backend, runtime_value_t coll_value, runtime_type_t coll_type, uint64_t start, uint64_t end);
const runtime_value_t subset__carray(value_backend_t& backend, runtime_value_t coll_value, runtime_type_t coll_type, uint64_t start, uint64_t end);
const runtime_value_t subset__hamt(value_backend_t& backend, runtime_value_t coll_value, runtime_type_t coll_type, uint64_t start, uint64_t end);
runtime_value_t subset(value_backend_t& backend, runtime_value_t coll_value, runtime_type_t coll_type, int64_t start, int64_t end);



runtime_value_t replace__string(value_backend_t& backend, runtime_value_t coll_value, runtime_type_t coll_type, size_t start, size_t end, runtime_value_t replacement_value, runtime_type_t replacement_type);
runtime_value_t replace__carray(value_backend_t& backend, runtime_value_t coll_value, runtime_type_t coll_type, size_t start, size_t end, runtime_value_t replacement_value, runtime_type_t replacement_type);
runtime_value_t replace__hamt(value_backend_t& backend, runtime_value_t coll_value, runtime_type_t coll_type, size_t start, size_t end, runtime_value_t replacement_value, runtime_type_t replacement_type);

runtime_value_t replace(value_backend_t& backend, runtime_value_t coll_value, runtime_type_t coll_type, int64_t start, int64_t end, runtime_value_t replacement_value, runtime_type_t replacement_type);




int64_t find__string(value_backend_t& backend, runtime_value_t coll_value, runtime_type_t coll_type, const runtime_value_t value, runtime_type_t value_type);
int64_t find__carray(value_backend_t& backend, runtime_value_t coll_value, runtime_type_t coll_type, const runtime_value_t value, runtime_type_t value_type);
int64_t find__hamt(value_backend_t& backend, runtime_value_t coll_value, runtime_type_t coll_type, const runtime_value_t value, runtime_type_t value_type);

int64_t find2(value_backend_t& backend, runtime_value_t coll_value, runtime_type_t coll_type, const runtime_value_t value, runtime_type_t value_type);



bool exists(value_backend_t& backend, runtime_value_t coll_value, runtime_type_t coll_type, runtime_value_t value, runtime_type_t value_type);

runtime_value_t erase(value_backend_t& backend, runtime_value_t coll_value, runtime_type_t coll_type, runtime_value_t key_value, runtime_type_t key_type);
runtime_value_t get_keys(value_backend_t& backend, runtime_value_t coll_value, runtime_type_t coll_type);



///??? Split into two passes so we don't get 2 x 2. Generate code mixup? Have one function with two checks?
runtime_value_t get_keys__cppmap_carray(value_backend_t& backend, runtime_value_t dict_value, runtime_type_t dict_type);
runtime_value_t get_keys__cppmap_hamt(value_backend_t& backend, runtime_value_t dict_value, runtime_type_t dict_type);

runtime_value_t get_keys__hamtmap_carray(value_backend_t& backend, runtime_value_t dict_value, runtime_type_t dict_type);
runtime_value_t get_keys__hamtmap_hamt(value_backend_t& backend, runtime_value_t dict_value, runtime_type_t dict_type);


//	Use subset of samples -- assume first sample is warm-up.
int64_t analyse_samples(const int64_t* samples, int64_t count);



runtime_value_t concat_strings(value_backend_t& backend, const runtime_value_t& lhs, const runtime_value_t& rhs);
runtime_value_t concat_vector_carray(value_backend_t& backend, const type_t& type, const runtime_value_t& lhs, const runtime_value_t& rhs);
runtime_value_t concat_vector_hamt(value_backend_t& backend, const type_t& type, const runtime_value_t& lhs, const runtime_value_t& rhs);
runtime_value_t concatunate_vectors(value_backend_t& backend, const type_t& type, runtime_value_t lhs, runtime_value_t rhs);


uint64_t get_vector_size(value_backend_t& backend, const type_t& vector_type, runtime_value_t vec);
runtime_value_t lookup_vector_element(value_backend_t& backend, const type_t& vector_type, runtime_value_t vec, uint64_t index);





/////////////////////////////////////////		INLINES


//??? optimize by copying alloc64_t directly, then overwrite 1 character.
inline const runtime_value_t update__string(value_backend_t& backend, runtime_value_t s, runtime_value_t key_value, runtime_value_t value){
	QUARK_ASSERT(backend.check_invariant());

	const auto str = from_runtime_string2(backend, s);
	const auto i = key_value.int_value;
	const auto new_char = (char)value.int_value;

	const auto len = str.size();

	if(i < 0 || i >= len){
		quark::throw_runtime_error("Position argument to update() is outside collection span.");
	}

	auto result = str;
	result[i] = new_char;
	const auto result2 = to_runtime_string2(backend, result);
	return result2;
}

inline const runtime_value_t update__vector_hamt_pod(value_backend_t& backend, runtime_value_t coll_value, runtime_type_t coll_type, runtime_value_t index, runtime_value_t value){
	QUARK_ASSERT(backend.check_invariant());

	const auto vec = coll_value.vector_hamt_ptr;
	const auto i = index.int_value;
	if(i < 0 || i >= vec->get_element_count()){
		quark::throw_runtime_error("Position argument to update() is outside collection span.");
	}
	return store_immutable(coll_value, i, value);
}
inline const runtime_value_t update__vector_hamt_nonpod(value_backend_t& backend, runtime_value_t coll_value, runtime_type_t coll_type, runtime_value_t index, runtime_value_t value){
	QUARK_ASSERT(backend.check_invariant());

	const auto vec = coll_value.vector_hamt_ptr;
	const auto i = index.int_value;
	if(i < 0 || i >= vec->get_element_count()){
		quark::throw_runtime_error("Position argument to update() is outside collection span.");
	}

	//??? compile time. Provide as a constaint integer arg
	const auto element_itype = lookup_vector_element_type(backend, type_t(coll_type));

	const auto result = store_immutable(coll_value, i, value);

	for(int x = 0 ; x < result.vector_hamt_ptr->get_element_count() ; x++){
		auto v = result.vector_hamt_ptr->load_element(x);
		retain_value(backend, v, element_itype);
	}
	return result;
}


}	// floyd

#endif /* value_features_hpp */
