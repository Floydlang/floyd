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

int compare_values(value_mgr_t& value_mgr, int64_t op, const runtime_type_t type, runtime_value_t lhs, runtime_value_t rhs);

}	// floyd

#endif /* value_features_hpp */
