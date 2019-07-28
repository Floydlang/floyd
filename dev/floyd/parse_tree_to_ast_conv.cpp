//
//  parse_tree_to_ast_conv.cpp
//  floyd_speak
//
//  Created by Marcus Zetterquist on 2019-04-13.
//  Copyright © 2019 Marcus Zetterquist. All rights reserved.
//

#include "parse_tree_to_ast_conv.h"

#include "floyd_parser.h"
#include "statement.h"


namespace floyd {


unchecked_ast_t parse_tree_to_ast(const parser::parse_tree_t& parse_tree){
	//	Parse tree contains an array of statements, with hierachical functions and types.
	if(parse_tree._value.is_array()){
		const auto program_body = ast_json_to_statements(parse_tree._value);
		const auto gp_ast = general_purpose_ast_t{
			body_t{ program_body },
			{},
			{},
			{},
			{}
		};
		return unchecked_ast_t{ gp_ast };
	}
	else{
		QUARK_ASSERT(false);
		throw std::exception();
	}



//	const auto gp_ast = json_to_gp_ast(json._value);
//	return unchecked_ast_t{ gp_ast };
}


}	//	floyd
