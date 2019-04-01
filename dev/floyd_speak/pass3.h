//
//  pass2.hpp
//  FloydSpeak
//
//  Created by Marcus Zetterquist on 09/08/16.
//  Copyright © 2016 Marcus Zetterquist. All rights reserved.
//

#ifndef pass3_hpp
#define pass3_hpp

/*
	Performs semantic analysis of a Floyd program.

	Converts an ast_t to a semantic_ast_t.

	- All language-level syntax checks passed.
	- Builds symbol tables, resolves all symbols.
	- Checks types.
	- Infers types when not specified.
	- Replaces operations with other, equivalent operations.
	- has the ast_t and symbol tables for all lexical scopes.
	- Inserts host functions.
	- Insert built-in types.
*/

#include "quark.h"

#include <string>
#include "ast.h"

namespace floyd {


//////////////////////////////////////		semantic_ast_t

/*
	The semantic_ast_t is a ready-to-run program, all symbols resolved, all semantics are OK.
*/
struct semantic_ast_t {
	public: explicit semantic_ast_t(const ast_t& checked_ast);

#if DEBUG
	public: bool check_invariant() const;
#endif


	////////////////////////////////	STATE
	public: ast_t _checked_ast;
};


/*
	Semantic Analysis -> SYMBOL TABLE + annotated AST
*/
semantic_ast_t run_semantic_analysis(const ast_t& ast);


}	// Floyd
#endif /* pass3_hpp */


