//
//  parse_tree_to_ast_conv.hpp
//  Floyd
//
//  Created by Marcus Zetterquist on 2019-04-13.
//  Copyright © 2019 Marcus Zetterquist. All rights reserved.
//

#ifndef parse_tree_to_ast_conv_hpp
#define parse_tree_to_ast_conv_hpp


#include "ast.h"


namespace floyd {

namespace parser {
	struct parse_tree_t;
}

struct parse_tree_t;

unchecked_ast_t parse_tree_to_ast(const parser::parse_tree_t& parse_tree);

}	//	floyd



#endif /* parse_tree_to_ast_conv_hpp */
