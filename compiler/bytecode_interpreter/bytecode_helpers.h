//
//  bytecode_helpers.hpp
//  Floyd
//
//  Created by Marcus Zetterquist on 2019-07-25.
//  Copyright © 2019 Marcus Zetterquist. All rights reserved.
//

#ifndef bytecode_helpers_hpp
#define bytecode_helpers_hpp

namespace floyd {


struct value_t;
struct types_t;
struct bc_value_t;

value_t bc_to_value(const types_t& types, const bc_value_t& value);
bc_value_t value_to_bc(const types_t& types, const value_t& value);

}	// floyd

#endif /* bytecode_helpers_hpp */
