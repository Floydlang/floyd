//
//  collect_used_types.hpp
//  floyd_speak
//
//  Created by Marcus Zetterquist on 2019-07-28.
//  Copyright © 2019 Marcus Zetterquist. All rights reserved.
//

#ifndef collect_used_types_hpp
#define collect_used_types_hpp

#include "quark.h"

#include <string>
#include "ast.h"


namespace floyd {

struct type_interner_t;
struct general_purpose_ast_t;

void collect_used_types(type_interner_t& acc, const general_purpose_ast_t& ast);

}	// floyd

#endif /* collect_used_types_hpp */
