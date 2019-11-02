//
//  semantic_ast.cpp
//  floyd
//
//  Created by Marcus Zetterquist on 2019-08-19.
//  Copyright © 2019 Marcus Zetterquist. All rights reserved.
//

#include "semantic_ast.h"

#include "statement.h"
#include "ast_value.h"
#include "ast_helpers.h"
#include "utils.h"
#include "json_support.h"
#include "floyd_runtime.h"
#include "text_parser.h"
#include "floyd_syntax.h"
#include "collect_used_types.h"

namespace floyd {


//////////////////////////////////////		semantic_ast_t

semantic_ast_t::semantic_ast_t(const general_purpose_ast_t& tree, const intrinsic_signatures_t& intrinsic_signatures) :
	_tree(tree),
	intrinsic_signatures(intrinsic_signatures)
{
	QUARK_ASSERT(tree.check_invariant());

	QUARK_ASSERT(check_invariant());
}

#if DEBUG
bool semantic_ast_t::check_invariant() const{
	QUARK_ASSERT(_tree.check_invariant());
//	QUARK_ASSERT(check_types_resolved(_tree));
	return true;
}
#endif


json_t semantic_ast_to_json(const semantic_ast_t& ast){
	QUARK_ASSERT(ast.check_invariant());

	return gp_ast_to_json(ast._tree);
}


semantic_ast_t json_to_semantic_ast(const json_t& json){
	const auto gp_ast = json_to_gp_ast(json);
	bool resolved = check_types_resolved(gp_ast);
	if(resolved == false){
		throw std::exception();
	}
	return semantic_ast_t{ gp_ast, {} };
}



}	// floyd
