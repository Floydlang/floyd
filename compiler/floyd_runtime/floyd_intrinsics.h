//
//  floyd_intrinsics.hpp
//  floyd
//
//  Created by Marcus Zetterquist on 2019-12-29.
//  Copyright © 2019 Marcus Zetterquist. All rights reserved.
//

#ifndef floyd_intrinsics_hpp
#define floyd_intrinsics_hpp

#include "value_backend.h"


namespace floyd {

struct runtime_t;

void intrinsic__assert(runtime_t* frp, rt_pod_t arg);

rt_pod_t intrinsic__to_string(runtime_t* frp, rt_pod_t value, runtime_type_t value_type);
rt_pod_t intrinsic__to_pretty_string(runtime_t* frp, rt_pod_t value, runtime_type_t value_type);
runtime_type_t intrinsic__typeof(runtime_t* frp, rt_pod_t value, runtime_type_t value_type);






const rt_pod_t update_string(runtime_t* frp, rt_pod_t coll_value, runtime_type_t coll_type, rt_pod_t key_value, runtime_type_t key_type, rt_pod_t value, runtime_type_t value_type);
const rt_pod_t update_vector_carray_pod(runtime_t* frp, rt_pod_t coll_value, runtime_type_t coll_type, rt_pod_t key_value, runtime_type_t key_type, rt_pod_t value, runtime_type_t value_type);
const rt_pod_t update_vector_carray_nonpod(runtime_t* frp, rt_pod_t coll_value, runtime_type_t coll_type, rt_pod_t key_value, runtime_type_t key_type, rt_pod_t value, runtime_type_t value_type);
const rt_pod_t update_vector_hamt_pod(runtime_t* frp, rt_pod_t coll_value, runtime_type_t coll_type, rt_pod_t key_value, runtime_type_t key_type, rt_pod_t value, runtime_type_t value_type);
const rt_pod_t update_vector_hamt_nonpod(runtime_t* frp, rt_pod_t coll_value, runtime_type_t coll_type, rt_pod_t key_value, runtime_type_t key_type, rt_pod_t value, runtime_type_t value_type);
const rt_pod_t update_dict_cppmap_pod(runtime_t* frp, rt_pod_t coll_value, runtime_type_t coll_type, rt_pod_t key_value, runtime_type_t key_type, rt_pod_t value, runtime_type_t value_type);
const rt_pod_t update_dict_cppmap_nonpod(runtime_t* frp, rt_pod_t coll_value, runtime_type_t coll_type, rt_pod_t key_value, runtime_type_t key_type, rt_pod_t value, runtime_type_t value_type);
const rt_pod_t update_dict_hamt_pod(runtime_t* frp, rt_pod_t coll_value, runtime_type_t coll_type, rt_pod_t key_value, runtime_type_t key_type, rt_pod_t value, runtime_type_t value_type);
const rt_pod_t update_dict_hamt_nonpod(runtime_t* frp, rt_pod_t coll_value, runtime_type_t coll_type, rt_pod_t key_value, runtime_type_t key_type, rt_pod_t value, runtime_type_t value_type);

rt_pod_t intrinsic__update(
	runtime_t* frp,

	rt_pod_t collection_value,
	runtime_type_t collection_type,

	rt_pod_t key_value,
	runtime_type_t key_type,

	rt_pod_t newvalue_value,
	runtime_type_t newvalue_type
);



int64_t intrinsic__size_string(runtime_t* frp, rt_pod_t vec, runtime_type_t vec_type);
int64_t intrinsic__size_vector_carray(runtime_t* frp, rt_pod_t collection, runtime_type_t collection_type);
int64_t intrinsic__size_vector_hamt(runtime_t* frp, rt_pod_t collection, runtime_type_t collection_type);
int64_t intrinsic__size_dict_cppmap(runtime_t* frp, rt_pod_t collection, runtime_type_t collection_type);
int64_t intrinsic__size_dict_hamt(runtime_t* frp, rt_pod_t collection, runtime_type_t collection_type);
int64_t intrinsic__size_json(runtime_t* frp, rt_pod_t collection, runtime_type_t collection_type);
int64_t intrinsic__size(runtime_t* frp, rt_pod_t coll_value, runtime_type_t coll_type);

int64_t intrinsic__find(
	runtime_t* frp,
	rt_pod_t coll_value,
	runtime_type_t coll_type,
	const rt_pod_t value,
	runtime_type_t value_type
);
uint32_t intrinsic__exists(
	runtime_t* frp,
	rt_pod_t coll_value,
	runtime_type_t coll_type,
	rt_pod_t value,
	runtime_type_t value_type
);
rt_pod_t intrinsic__erase(
	runtime_t* frp,
	rt_pod_t coll_value,
	runtime_type_t coll_type,
	rt_pod_t key_value,
	runtime_type_t key_type
);
rt_pod_t intrinsic__get_keys(runtime_t* frp, rt_pod_t coll_value, runtime_type_t coll_type);


rt_pod_t intrinsic__push_back_string(runtime_t* frp, rt_pod_t vec, runtime_type_t vec_type, rt_pod_t element);
rt_pod_t intrinsic__push_back_carray_pod(runtime_t* frp, rt_pod_t vec, runtime_type_t vec_type, rt_pod_t element);
rt_pod_t intrinsic__push_back_carray_nonpod(runtime_t* frp, rt_pod_t vec, runtime_type_t vec_type, rt_pod_t element);
rt_pod_t intrinsic__push_back_hamt_pod(runtime_t* frp, rt_pod_t vec, runtime_type_t vec_type, rt_pod_t element);
rt_pod_t intrinsic__push_back_hamt_nonpod(runtime_t* frp, rt_pod_t vec, runtime_type_t vec_type, rt_pod_t element);
rt_pod_t intrinsic__push_back(
	runtime_t* frp,

	rt_pod_t collection_value,
	runtime_type_t collection_type,

	rt_pod_t newvalue_value,
	runtime_type_t newvalue_type
);


const rt_pod_t intrinsic__subset(
	runtime_t* frp,
	rt_pod_t elements_vec,
	runtime_type_t elements_vec_type,
	uint64_t start,
	uint64_t end
);
const rt_pod_t intrinsic__replace(
	runtime_t* frp,
	rt_pod_t elements_vec,
	runtime_type_t elements_vec_type,
	uint64_t start,
	uint64_t end,
	rt_pod_t arg3_value,
	runtime_type_t arg3_type
);


/////////////////////////////////////////		HIGHER ORDER


typedef rt_pod_t (*MAP_F)(runtime_t* frp, rt_pod_t e_value, rt_pod_t context_value);

rt_pod_t intrinsic__map__carray(
	runtime_t* frp,
	rt_pod_t elements_vec,
	runtime_type_t elements_vec_type,
	rt_pod_t f_value,
	runtime_type_t f_type,
	rt_pod_t context_value,
	runtime_type_t context_type,
	runtime_type_t result_vec_type
);
rt_pod_t intrinsic__map__hamt(
	runtime_t* frp,
	rt_pod_t elements_vec,
	runtime_type_t elements_vec_type,
	rt_pod_t f_value,
	runtime_type_t f_type,
	rt_pod_t context_value,
	runtime_type_t context_type,
	runtime_type_t result_vec_type
);
rt_pod_t intrinsic__map(
	runtime_t* frp,
	rt_pod_t elements_vec,
	runtime_type_t elements_vec_type,
	rt_pod_t f_value,
	runtime_type_t f_type,
	rt_pod_t context_value,
	runtime_type_t context_type,
	runtime_type_t result_vec_type
);




typedef rt_pod_t (*map_dag_F)(runtime_t* frp, rt_pod_t r_value, rt_pod_t r_vec_value, rt_pod_t context_value);

rt_pod_t intrinsic__map_dag__carray(
	runtime_t* frp,
	rt_pod_t elements_vec,
	runtime_type_t elements_vec_type,
	rt_pod_t depends_on_vec,
	runtime_type_t depends_on_vec_type,
	rt_pod_t f_value,
	runtime_type_t f_value_type,
	rt_pod_t context,
	runtime_type_t context_type
);
rt_pod_t intrinsic__map_dag__hamt(
	runtime_t* frp,
	rt_pod_t elements_vec,
	runtime_type_t elements_vec_type,
	rt_pod_t depends_on_vec,
	runtime_type_t depends_on_vec_type,
	rt_pod_t f_value,
	runtime_type_t f_value_type,
	rt_pod_t context,
	runtime_type_t context_type
);
rt_pod_t intrinsic__map_dag(
	runtime_t* frp,
	rt_pod_t elements_vec,
	runtime_type_t elements_vec_type,
	rt_pod_t depends_on_vec,
	runtime_type_t depends_on_vec_type,
	rt_pod_t f_value,
	runtime_type_t f_value_type,
	rt_pod_t context,
	runtime_type_t context_type
);


typedef rt_pod_t (*FILTER_F)(runtime_t* frp, rt_pod_t element_value, rt_pod_t context);

rt_pod_t intrinsic__filter_carray(
	runtime_t* frp,
	rt_pod_t elements_vec,
	runtime_type_t elements_vec_type,
	rt_pod_t f_value,
	runtime_type_t f_value_type,
	rt_pod_t context,
	runtime_type_t context_type
);
rt_pod_t intrinsic__filter_hamt(
	runtime_t* frp,
	rt_pod_t elements_vec,
	runtime_type_t elements_vec_type,
	rt_pod_t f_value,
	runtime_type_t f_value_type,
	rt_pod_t context,
	runtime_type_t context_type
);
rt_pod_t intrinsic__filter(
	runtime_t* frp,
	rt_pod_t elements_vec,
	runtime_type_t elements_vec_type,
	rt_pod_t f_value,
	runtime_type_t f_value_type,
	rt_pod_t arg2_value,
	runtime_type_t arg2_type
);


typedef rt_pod_t (*REDUCE_F)(runtime_t* frp, rt_pod_t acc_value, rt_pod_t element_value, rt_pod_t context);

rt_pod_t intrinsic__reduce_carray(
	runtime_t* frp,
	rt_pod_t elements_vec,
	runtime_type_t elements_vec_type,
	rt_pod_t init_value,
	runtime_type_t init_value_type,
	rt_pod_t f_value,
	runtime_type_t f_type,
	rt_pod_t context,
	runtime_type_t context_type
);
rt_pod_t intrinsic__reduce_hamt(
	runtime_t* frp,
	rt_pod_t elements_vec,
	runtime_type_t elements_vec_type,
	rt_pod_t init_value,
	runtime_type_t init_value_type,
	rt_pod_t f_value,
	runtime_type_t f_type,
	rt_pod_t context,
	runtime_type_t context_type
);
rt_pod_t intrinsic__reduce(
	runtime_t* frp,
	rt_pod_t elements_vec,
	runtime_type_t elements_vec_type,
	rt_pod_t init_value,
	runtime_type_t init_value_type,
	rt_pod_t f_value,
	runtime_type_t f_type,
	rt_pod_t context,
	runtime_type_t context_type
);


typedef uint8_t (*stable_sort_F)(runtime_t* frp, rt_pod_t left_value, rt_pod_t right_value, rt_pod_t context_value);

rt_pod_t intrinsic__stable_sort_carray(
	runtime_t* frp,
	rt_pod_t elements_vec,
	runtime_type_t elements_vec_type,
	rt_pod_t f_value,
	runtime_type_t f_value_type,
	rt_pod_t context_value,
	runtime_type_t context_value_type
);
rt_pod_t intrinsic__stable_sort_hamt(
	runtime_t* frp,
	rt_pod_t elements_vec,
	runtime_type_t elements_vec_type,
	rt_pod_t f_value,
	runtime_type_t f_value_type,
	rt_pod_t context_value,
	runtime_type_t context_value_type
);
rt_pod_t intrinsic__stable_sort(
	runtime_t* frp,
	rt_pod_t elements_vec,
	runtime_type_t elements_vec_type,
	rt_pod_t f_value,
	runtime_type_t f_value_type,
	rt_pod_t context_value,
	runtime_type_t context_value_type
);


/////////////////////////////////////////		JSON


int64_t intrinsic__get_json_type(runtime_t* frp, rt_pod_t json0);
rt_pod_t intrinsic__generate_json_script(runtime_t* frp, rt_pod_t json0);
rt_pod_t intrinsic__parse_json_script(runtime_t* frp, rt_pod_t string_s0);
rt_pod_t intrinsic__to_json(runtime_t* frp, rt_pod_t value, runtime_type_t value_type);
rt_pod_t intrinsic__from_json(runtime_t* frp, rt_pod_t json0, runtime_type_t target_type);


void intrinsic__print(runtime_t* frp, rt_pod_t value, runtime_type_t value_type);

void intrinsic__send(runtime_t* frp, rt_pod_t dest_process_id0, rt_pod_t message, runtime_type_t message_type);
void intrinsic__exit(runtime_t* frp);


rt_pod_t intrinsic__bw_not(runtime_t* frp, rt_pod_t v);
rt_pod_t intrinsic__bw_and(runtime_t* frp, rt_pod_t a, rt_pod_t b);
rt_pod_t intrinsic__bw_or(runtime_t* frp, rt_pod_t a, rt_pod_t b);
rt_pod_t intrinsic__bw_xor(runtime_t* frp, rt_pod_t a, rt_pod_t b);
rt_pod_t intrinsic__bw_shift_left(runtime_t* frp, rt_pod_t v, rt_pod_t count);
rt_pod_t intrinsic__bw_shift_right(runtime_t* frp, rt_pod_t v, rt_pod_t count);
rt_pod_t intrinsic__bw_shift_right_arithmetic(runtime_t* frp, rt_pod_t v, rt_pod_t count);


} // floyd

#endif /* floyd_intrinsics_hpp */
