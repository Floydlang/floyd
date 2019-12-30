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

struct floyd_runtime_t;

void unified_intrinsic__assert(floyd_runtime_t* frp, runtime_value_t arg);

runtime_value_t unified_intrinsic__to_string(floyd_runtime_t* frp, runtime_value_t value, runtime_type_t value_type);
runtime_value_t unified_intrinsic__to_pretty_string(floyd_runtime_t* frp, runtime_value_t value, runtime_type_t value_type);
runtime_type_t unified_intrinsic__typeof(floyd_runtime_t* frp, runtime_value_t value, runtime_type_t value_type);


runtime_value_t unified_intrinsic__update(
	floyd_runtime_t* frp,

	runtime_value_t collection_value,
	runtime_type_t collection_type,

	runtime_value_t key_value,
	runtime_type_t key_type,

	runtime_value_t newvalue_value,
	runtime_type_t newvalue_type
);

int64_t unified_intrinsic__size(floyd_runtime_t* frp, runtime_value_t coll_value, runtime_type_t coll_type);
int64_t unified_intrinsic__find(
	floyd_runtime_t* frp,
	runtime_value_t coll_value,
	runtime_type_t coll_type,
	const runtime_value_t value,
	runtime_type_t value_type
);
uint32_t unified_intrinsic__exists(
	floyd_runtime_t* frp,
	runtime_value_t coll_value,
	runtime_type_t coll_type,
	runtime_value_t value,
	runtime_type_t value_type
);
runtime_value_t unified_intrinsic__erase(
	floyd_runtime_t* frp,
	runtime_value_t coll_value,
	runtime_type_t coll_type,
	runtime_value_t key_value,
	runtime_type_t key_type
);
runtime_value_t unified_intrinsic__get_keys(floyd_runtime_t* frp, runtime_value_t coll_value, runtime_type_t coll_type);

runtime_value_t unified_intrinsic__push_back(
	floyd_runtime_t* frp,

	runtime_value_t collection_value,
	runtime_type_t collection_type,

	runtime_value_t newvalue_value,
	runtime_type_t newvalue_type
);


const runtime_value_t unified_intrinsic__subset(
	floyd_runtime_t* frp,
	runtime_value_t elements_vec,
	runtime_type_t elements_vec_type,
	uint64_t start,
	uint64_t end
);
const runtime_value_t unified_intrinsic__replace(
	floyd_runtime_t* frp,
	runtime_value_t elements_vec,
	runtime_type_t elements_vec_type,
	uint64_t start,
	uint64_t end,
	runtime_value_t arg3_value,
	runtime_type_t arg3_type
);


/////////////////////////////////////////		HIGHER ORDER


typedef runtime_value_t (*MAP_F)(floyd_runtime_t* frp, runtime_value_t e_value, runtime_value_t context_value);

runtime_value_t unified_intrinsic__map__carray(
	floyd_runtime_t* frp,
	runtime_value_t elements_vec,
	runtime_type_t elements_vec_type,
	runtime_value_t f_value,
	runtime_type_t f_type,
	runtime_value_t context_value,
	runtime_type_t context_type,
	runtime_type_t result_vec_type
);
runtime_value_t unified_intrinsic__map__hamt(
	floyd_runtime_t* frp,
	runtime_value_t elements_vec,
	runtime_type_t elements_vec_type,
	runtime_value_t f_value,
	runtime_type_t f_type,
	runtime_value_t context_value,
	runtime_type_t context_type,
	runtime_type_t result_vec_type
);
runtime_value_t unified_intrinsic__map(
	floyd_runtime_t* frp,
	runtime_value_t elements_vec,
	runtime_type_t elements_vec_type,
	runtime_value_t f_value,
	runtime_type_t f_type,
	runtime_value_t context_value,
	runtime_type_t context_type,
	runtime_type_t result_vec_type
);




typedef runtime_value_t (*map_dag_F)(floyd_runtime_t* frp, runtime_value_t r_value, runtime_value_t r_vec_value, runtime_value_t context_value);

runtime_value_t unified_intrinsic__map_dag__carray(
	floyd_runtime_t* frp,
	runtime_value_t elements_vec,
	runtime_type_t elements_vec_type,
	runtime_value_t depends_on_vec,
	runtime_type_t depends_on_vec_type,
	runtime_value_t f_value,
	runtime_type_t f_value_type,
	runtime_value_t context,
	runtime_type_t context_type
);
runtime_value_t unified_intrinsic__map_dag__hamt(
	floyd_runtime_t* frp,
	runtime_value_t elements_vec,
	runtime_type_t elements_vec_type,
	runtime_value_t depends_on_vec,
	runtime_type_t depends_on_vec_type,
	runtime_value_t f_value,
	runtime_type_t f_value_type,
	runtime_value_t context,
	runtime_type_t context_type
);
runtime_value_t unified_intrinsic__map_dag(
	floyd_runtime_t* frp,
	runtime_value_t elements_vec,
	runtime_type_t elements_vec_type,
	runtime_value_t depends_on_vec,
	runtime_type_t depends_on_vec_type,
	runtime_value_t f_value,
	runtime_type_t f_value_type,
	runtime_value_t context,
	runtime_type_t context_type
);


typedef runtime_value_t (*FILTER_F)(floyd_runtime_t* frp, runtime_value_t element_value, runtime_value_t context);

runtime_value_t unified_intrinsic__filter_carray(
	floyd_runtime_t* frp,
	runtime_value_t elements_vec,
	runtime_type_t elements_vec_type,
	runtime_value_t f_value,
	runtime_type_t f_value_type,
	runtime_value_t context,
	runtime_type_t context_type
);
runtime_value_t unified_intrinsic__filter_hamt(
	floyd_runtime_t* frp,
	runtime_value_t elements_vec,
	runtime_type_t elements_vec_type,
	runtime_value_t f_value,
	runtime_type_t f_value_type,
	runtime_value_t context,
	runtime_type_t context_type
);
runtime_value_t unified_intrinsic__filter(
	floyd_runtime_t* frp,
	runtime_value_t elements_vec,
	runtime_type_t elements_vec_type,
	runtime_value_t f_value,
	runtime_type_t f_value_type,
	runtime_value_t arg2_value,
	runtime_type_t arg2_type
);


typedef runtime_value_t (*REDUCE_F)(floyd_runtime_t* frp, runtime_value_t acc_value, runtime_value_t element_value, runtime_value_t context);

runtime_value_t unified_intrinsic__reduce_carray(
	floyd_runtime_t* frp,
	runtime_value_t elements_vec,
	runtime_type_t elements_vec_type,
	runtime_value_t init_value,
	runtime_type_t init_value_type,
	runtime_value_t f_value,
	runtime_type_t f_type,
	runtime_value_t context,
	runtime_type_t context_type
);
runtime_value_t unified_intrinsic__reduce_hamt(
	floyd_runtime_t* frp,
	runtime_value_t elements_vec,
	runtime_type_t elements_vec_type,
	runtime_value_t init_value,
	runtime_type_t init_value_type,
	runtime_value_t f_value,
	runtime_type_t f_type,
	runtime_value_t context,
	runtime_type_t context_type
);
runtime_value_t unified_intrinsic__reduce(
	floyd_runtime_t* frp,
	runtime_value_t elements_vec,
	runtime_type_t elements_vec_type,
	runtime_value_t init_value,
	runtime_type_t init_value_type,
	runtime_value_t f_value,
	runtime_type_t f_type,
	runtime_value_t context,
	runtime_type_t context_type
);


typedef uint8_t (*stable_sort_F)(floyd_runtime_t* frp, runtime_value_t left_value, runtime_value_t right_value, runtime_value_t context_value);

runtime_value_t unified_intrinsic__stable_sort_carray(
	floyd_runtime_t* frp,
	runtime_value_t elements_vec,
	runtime_type_t elements_vec_type,
	runtime_value_t f_value,
	runtime_type_t f_value_type,
	runtime_value_t context_value,
	runtime_type_t context_value_type
);
runtime_value_t unified_intrinsic__stable_sort_hamt(
	floyd_runtime_t* frp,
	runtime_value_t elements_vec,
	runtime_type_t elements_vec_type,
	runtime_value_t f_value,
	runtime_type_t f_value_type,
	runtime_value_t context_value,
	runtime_type_t context_value_type
);
runtime_value_t unified_intrinsic__stable_sort(
	floyd_runtime_t* frp,
	runtime_value_t elements_vec,
	runtime_type_t elements_vec_type,
	runtime_value_t f_value,
	runtime_type_t f_value_type,
	runtime_value_t context_value,
	runtime_type_t context_value_type
);


/////////////////////////////////////////		JSON


int64_t unified_intrinsic__get_json_type(floyd_runtime_t* frp, runtime_value_t json0);
runtime_value_t unified_intrinsic__generate_json_script(floyd_runtime_t* frp, runtime_value_t json0);
runtime_value_t unified_intrinsic__parse_json_script(floyd_runtime_t* frp, runtime_value_t string_s0);
runtime_value_t unified_intrinsic__to_json(floyd_runtime_t* frp, runtime_value_t value, runtime_type_t value_type);
runtime_value_t unified_intrinsic__from_json(floyd_runtime_t* frp, runtime_value_t json0, runtime_type_t target_type);


void unified_intrinsic__print(floyd_runtime_t* frp, runtime_value_t value, runtime_type_t value_type);

void unified_intrinsic__send(floyd_runtime_t* frp, runtime_value_t dest_process_id0, runtime_value_t message, runtime_type_t message_type);
void unified_intrinsic__exit(floyd_runtime_t* frp);


runtime_value_t unified_intrinsic__bw_not(floyd_runtime_t* frp, runtime_value_t v);
runtime_value_t unified_intrinsic__bw_and(floyd_runtime_t* frp, runtime_value_t a, runtime_value_t b);
runtime_value_t unified_intrinsic__bw_or(floyd_runtime_t* frp, runtime_value_t a, runtime_value_t b);
runtime_value_t unified_intrinsic__bw_xor(floyd_runtime_t* frp, runtime_value_t a, runtime_value_t b);
runtime_value_t unified_intrinsic__bw_shift_left(floyd_runtime_t* frp, runtime_value_t v, runtime_value_t count);
runtime_value_t unified_intrinsic__bw_shift_right(floyd_runtime_t* frp, runtime_value_t v, runtime_value_t count);
runtime_value_t unified_intrinsic__bw_shift_right_arithmetic(floyd_runtime_t* frp, runtime_value_t v, runtime_value_t count);


} // floyd

#endif /* floyd_intrinsics_hpp */
