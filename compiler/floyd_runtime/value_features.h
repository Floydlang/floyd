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


//	Use subset of samples -- assume first sample is warm-up.
int64_t analyse_samples(const int64_t* samples, int64_t count);

std::string gen_to_string(value_backend_t& backend, rt_pod_t arg_value, type_t arg_type);

}	// floyd

#endif /* value_features_hpp */
