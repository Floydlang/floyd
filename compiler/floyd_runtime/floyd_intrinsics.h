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

void unified_intrinsic__print(floyd_runtime_t* frp, runtime_value_t value, runtime_type_t value_type);

} // floyd

#endif /* floyd_intrinsics_hpp */
