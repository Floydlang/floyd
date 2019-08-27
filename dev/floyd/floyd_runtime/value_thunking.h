//
//  value_thunking.hpp
//  floyd
//
//  Created by Marcus Zetterquist on 2019-08-19.
//  Copyright © 2019 Marcus Zetterquist. All rights reserved.
//

#ifndef value_thunking_hpp
#define value_thunking_hpp

#include <string>

namespace floyd {

struct value_backend_t;
struct value_t;
union runtime_value_t;
struct typeid_t;

runtime_value_t to_runtime_string2(value_backend_t& backend, const std::string& s);
std::string from_runtime_string2(const value_backend_t& backend, runtime_value_t encoded_value);


runtime_value_t to_runtime_value2(value_backend_t& backend, const value_t& value);
value_t from_runtime_value2(const value_backend_t& backend, const runtime_value_t encoded_value, const typeid_t& type);


}	// floyd

#endif /* value_thunking_hpp */
