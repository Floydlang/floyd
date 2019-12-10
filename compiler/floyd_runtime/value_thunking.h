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
struct type_t;
struct dict_t;
struct types_t;
struct bc_value_t;


//////////////////////////////////////////		runtime_value_t


runtime_value_t alloc_carray_8bit(value_backend_t& backend, const uint8_t data[], std::size_t count);

runtime_value_t to_runtime_string2(value_backend_t& backend, const std::string& s);
std::string from_runtime_string2(const value_backend_t& backend, runtime_value_t encoded_value);


runtime_value_t to_runtime_value2(value_backend_t& backend, const value_t& value);
value_t from_runtime_value2(const value_backend_t& backend, const runtime_value_t encoded_value, const type_t& type);


runtime_value_t to_runtime_vector(value_backend_t& backend, const value_t& value);
value_t from_runtime_vector(const value_backend_t& backend, const runtime_value_t encoded_value, const type_t& type);

runtime_value_t to_runtime_dict(value_backend_t& backend, const dict_t& exact_type, const value_t& value);
value_t from_runtime_dict(const value_backend_t& backend, const runtime_value_t encoded_value, const type_t& type);


//////////////////////////////////////////		value_t vs bc_value_t


runtime_value_t make_runtime_non_rc(const value_t& value);
bc_value_t make_non_rc(const value_t& value);

value_t bc_to_value(const value_backend_t& backend, const bc_value_t& value);
bc_value_t value_to_bc(value_backend_t& backend, const value_t& value);

bc_value_t bc_from_runtime(value_backend_t& backend, runtime_value_t value, const type_t& type);
runtime_value_t runtime_from_bc(value_backend_t& backend, const bc_value_t& value);



}	// floyd

#endif /* value_thunking_hpp */
