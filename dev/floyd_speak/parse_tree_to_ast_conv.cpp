//
//  parse_tree_to_ast_conv.cpp
//  floyd_speak
//
//  Created by Marcus Zetterquist on 2019-04-13.
//  Copyright © 2019 Marcus Zetterquist. All rights reserved.
//

#include "parse_tree_to_ast_conv.h"

#include "statement.h"
#include "floyd_parser.h"


namespace floyd {


unchecked_ast_t parse_tree_to_ast(const parse_tree_t& json){
	const auto gp_ast = json_to_gp_ast(json._value);
	return unchecked_ast_t{ gp_ast };
}


}	//	floyd
