//
//  parse_tree_to_ast_conv.cpp
//  Floyd
//
//  Created by Marcus Zetterquist on 2019-04-13.
//  Copyright © 2019 Marcus Zetterquist. All rights reserved.
//

#include "parse_tree_to_ast_conv.h"

#include "floyd_parser.h"
#include "statement.h"


namespace floyd {

//	NOTICE: Implementation rigth now works because parse-tree JSON uses same scheme as
//	AST tree = shaky. Better to manually create a unchecked_ast_t from the parse tree JSON.
unchecked_ast_t parse_tree_to_ast(const parser::parse_tree_t& parse_tree){
	//	Parse tree contains an array of statements, with hierachical functions and types.
	QUARK_ASSERT(parse_tree._value.is_array());
	const type_interner_t interner;
	const auto program_body = ast_json_to_statements(interner, parse_tree._value);
	const auto gp_ast = general_purpose_ast_t{
		body_t{ program_body },
		{},
		{},
		{},
		{}
	};
	return unchecked_ast_t{ gp_ast };
}


}	//	floyd
