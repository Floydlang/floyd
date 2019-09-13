//
//  semantic_analyser_hpp
//  Floyd
//
//  Created by Marcus Zetterquist on 09/08/16.
//  Copyright © 2016 Marcus Zetterquist. All rights reserved.
//

#ifndef semantic_analyser_hpp
#define semantic_analyser_hpp

/*
	Performs semantic analysis of a Floyd program.

	Converts an unchecked_ast_t to a semantic_ast_t.

	- All language-level syntax checks passed.
	- Builds symbol tables, resolves all symbols.
	- Checks types.
	- Infers types when not specified.
	- Replaces operations with other, equivalent operations.
	- has the unchecked_ast_t and symbol tables for all lexical scopes.
	- Inserts host functions.
	- Insert built-in types.

	- All nested blocks remain and have their own symbols and statements.

	Output is a program that is correct with no type/semantic errors.
*/

namespace floyd {

struct semantic_ast_t;
struct unchecked_ast_t;

semantic_ast_t run_semantic_analysis(const unchecked_ast_t& ast);

}	// Floyd

#endif /* semantic_analyser_hpp */


