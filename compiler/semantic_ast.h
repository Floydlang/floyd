//
//  semantic_ast.hpp
//  floyd
//
//  Created by Marcus Zetterquist on 2019-08-19.
//  Copyright © 2019 Marcus Zetterquist. All rights reserved.
//

#ifndef semantic_ast_hpp
#define semantic_ast_hpp

#include "quark.h"

#include <string>
#include "ast.h"

namespace floyd {

struct unchecked_ast_t;

//////////////////////////////////////		semantic_ast_t

/*
	The semantic_ast_t is a correct program, all symbols resolved, all types resolved, all semantics are OK.
*/
struct semantic_ast_t {
	public: explicit semantic_ast_t(const general_purpose_ast_t& tree, const intrinsic_signatures_t& intrinsic_signatures);

#if DEBUG
	public: bool check_invariant() const;
#endif


	////////////////////////////////	STATE
	public: general_purpose_ast_t _tree;
	public: intrinsic_signatures_t intrinsic_signatures;
};

json_t semantic_ast_to_json(const semantic_ast_t& ast);
semantic_ast_t json_to_semantic_ast(const json_t& json);


}	// Floyd

#endif /* semantic_ast_hpp */
