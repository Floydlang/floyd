//
//  pass2.cpp
//  floyd_speak
//
//  Created by Marcus Zetterquist on 2019-04-13.
//  Copyright © 2019 Marcus Zetterquist. All rights reserved.
//

#include "pass2.h"

#include "statement.h"
#include "floyd_parser.h"


namespace floyd {



ast_json_t pass2_ast_to_json(const pass2_ast_t& ast){
	QUARK_ASSERT(ast.check_invariant());

	return ast_json_t::make(gp_ast_to_json(ast._tree));
}

pass2_ast_t parse_tree_to_pass2_ast(const parse_tree_t& json){
	const auto gp_ast = json_to_gp_ast(json._value);
	return pass2_ast_t{ gp_ast };
}


}	//	floyd
