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

//	SIZE()
int64_t unified_intrinsic__find(floyd_runtime_t* frp, runtime_value_t coll_value, runtime_type_t coll_type, const runtime_value_t value, runtime_type_t value_type);
uint32_t unified_intrinsic__exists(floyd_runtime_t* frp, runtime_value_t coll_value, runtime_type_t coll_type, runtime_value_t value, runtime_type_t value_type);
runtime_value_t unified_intrinsic__erase(floyd_runtime_t* frp, runtime_value_t coll_value, runtime_type_t coll_type, runtime_value_t key_value, runtime_type_t key_type);
runtime_value_t unified_intrinsic__get_keys(floyd_runtime_t* frp, runtime_value_t coll_value, runtime_type_t coll_type);

//	PUSHBACK()

const runtime_value_t unified_intrinsic__subset(floyd_runtime_t* frp, runtime_value_t elements_vec, runtime_type_t elements_vec_type, uint64_t start, uint64_t end);
const runtime_value_t unified_intrinsic__replace(floyd_runtime_t* frp, runtime_value_t elements_vec, runtime_type_t elements_vec_type, uint64_t start, uint64_t end, runtime_value_t arg3_value, runtime_type_t arg3_type);





void unified_intrinsic__print(floyd_runtime_t* frp, runtime_value_t value, runtime_type_t value_type);

void unified_intrinsic__send(floyd_runtime_t* frp, runtime_value_t dest_process_id0, runtime_value_t message, runtime_type_t message_type);
void unified_intrinsic__exit(floyd_runtime_t* frp);


} // floyd

#endif /* floyd_intrinsics_hpp */
