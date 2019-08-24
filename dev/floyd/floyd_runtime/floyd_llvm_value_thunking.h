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

runtime_value_t to_runtime_value2(value_mgr_t& value_mgr, const value_t& value);
value_t from_runtime_value2(const value_mgr_t& value_mgr, const runtime_value_t encoded_value, const typeid_t& type);


}	// floyd

#endif /* floyd_llvm_value_thunking_hpp */
