//
//  ast_helpers.hpp
//  floyd
//
//  Created by Marcus Zetterquist on 2019-07-30.
//  Copyright © 2019 Marcus Zetterquist. All rights reserved.
//

#ifndef ast_helpers_hpp
#define ast_helpers_hpp

#include <vector>

namespace floyd {

struct expression_t;
struct function_definition_t;

bool check_types_resolved(const expression_t& e);
bool check_types_resolved(const std::vector<expression_t>& expressions);

bool check_types_resolved(const function_definition_t& def);

}	//	floyd


#endif /* ast_helpers_hpp */
