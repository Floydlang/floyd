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

namespace floyd {


VECTOR_CPPVECTOR_T* unpack_vector_cppvector_arg(const value_backend_t& backend, runtime_value_t arg, runtime_type_t arg_type);
DICT_CPPMAP_T* unpack_dict_cppmap_arg(const value_backend_t& backend, runtime_value_t arg, runtime_type_t arg_type);


int compare_values(value_backend_t& backend, int64_t op, const runtime_type_t type, runtime_value_t lhs, runtime_value_t rhs);





const runtime_value_t update__string(value_backend_t& backend, runtime_value_t arg0, runtime_type_t arg0_type, runtime_value_t arg1, runtime_type_t arg1_type, runtime_value_t arg2, runtime_type_t arg2_type);

const runtime_value_t update__cppvector(value_backend_t& backend, runtime_value_t arg0, runtime_type_t arg0_type, runtime_value_t arg1, runtime_type_t arg1_type, runtime_value_t arg2, runtime_type_t arg2_type);

const runtime_value_t update__hamt(value_backend_t& backend, runtime_value_t arg0, runtime_type_t arg0_type, runtime_value_t arg1, runtime_type_t arg1_type, runtime_value_t arg2, runtime_type_t arg2_type);

const runtime_value_t update__dict(value_backend_t& backend, runtime_value_t arg0, runtime_type_t arg0_type, runtime_value_t arg1, runtime_type_t arg1_type, runtime_value_t arg2, runtime_type_t arg2_type);




const runtime_value_t subset__string(value_backend_t& backend, runtime_value_t arg0, runtime_type_t arg0_type, uint64_t start, uint64_t end);
const runtime_value_t subset__cppvector(value_backend_t& backend, runtime_value_t arg0, runtime_type_t arg0_type, uint64_t start, uint64_t end);
const runtime_value_t subset__hamt(value_backend_t& backend, runtime_value_t arg0, runtime_type_t arg0_type, uint64_t start, uint64_t end);




const runtime_value_t replace__string(value_backend_t& backend, runtime_value_t arg0, runtime_type_t arg0_type, size_t start, size_t end, runtime_value_t arg3_value, runtime_type_t arg3_type);
const runtime_value_t replace__cppvector(value_backend_t& backend, runtime_value_t arg0, runtime_type_t arg0_type, size_t start, size_t end, runtime_value_t arg3_value, runtime_type_t arg3_type);
const runtime_value_t replace__hamt(value_backend_t& backend, runtime_value_t arg0, runtime_type_t arg0_type, size_t start, size_t end, runtime_value_t arg3_value, runtime_type_t arg3_type);





int64_t find__string(value_backend_t& backend, runtime_value_t arg0, runtime_type_t arg0_type, const runtime_value_t arg1, runtime_type_t arg1_type);
int64_t find__cppvector(value_backend_t& backend, runtime_value_t arg0, runtime_type_t arg0_type, const runtime_value_t arg1, runtime_type_t arg1_type);
int64_t find__hamt(value_backend_t& backend, runtime_value_t arg0, runtime_type_t arg0_type, const runtime_value_t arg1, runtime_type_t arg1_type);



runtime_value_t get_keys__cppvector(value_backend_t& backend, runtime_value_t dict_value, runtime_type_t dict_type);
runtime_value_t get_keys__hamt(value_backend_t& backend, runtime_value_t dict_value, runtime_type_t dict_type);


//	Use subset of samples -- assume first sample is warm-up.
int64_t analyse_samples(const int64_t* samples, int64_t count);



runtime_value_t concat_strings(value_backend_t& backend, const runtime_value_t& lhs, const runtime_value_t& rhs);
runtime_value_t concat_vector_cppvector(value_backend_t& backend, const typeid_t& type, const runtime_value_t& lhs, const runtime_value_t& rhs);
runtime_value_t concat_vector_hamt(value_backend_t& backend, const typeid_t& type, const runtime_value_t& lhs, const runtime_value_t& rhs);


}	// floyd

#endif /* value_features_hpp */
