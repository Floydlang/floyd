//
//  ast_helpers.hpp
//  floyd
//
//  Created by Marcus Zetterquist on 2019-07-30.
//  Copyright © 2019 Marcus Zetterquist. All rights reserved.
//

#ifndef ast_helpers_hpp
#define ast_helpers_hpp


namespace floyd {

struct expression_t;

bool check_types_resolved(const expression_t& e);

}	//	floyd


#endif /* ast_helpers_hpp */
