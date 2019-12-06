//
//  bytecode_helpers.hpp
//  Floyd
//
//  Created by Marcus Zetterquist on 2019-07-25.
//  Copyright © 2019 Marcus Zetterquist. All rights reserved.
//

#ifndef bytecode_helpers_hpp
#define bytecode_helpers_hpp

#include "value_backend.h"

namespace floyd {


struct value_t;
struct types_t;
struct bc_value_t;



value_t bc_to_value(const types_t& types, const bc_value_t& value);
bc_value_t value_to_bc(const types_t& types, const value_t& value);


bc_value_t bc_from_runtime(const value_backend_t& backend, runtime_value_t value, const type_t& type);
runtime_value_t runtime_from_bc(value_backend_t& backend, const bc_value_t& value);


}	// floyd

#endif /* bytecode_helpers_hpp */
