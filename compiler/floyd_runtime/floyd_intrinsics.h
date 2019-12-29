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

void unified_intrinsic__print(floyd_runtime_t* frp, runtime_value_t value, runtime_type_t value_type);

void unified_intrinsic__send(floyd_runtime_t* frp, runtime_value_t dest_process_id0, runtime_value_t message, runtime_type_t message_type);
void unified_intrinsic__exit(floyd_runtime_t* frp);


} // floyd

#endif /* floyd_intrinsics_hpp */
