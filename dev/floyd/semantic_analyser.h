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



	DETAILED OPERATIONS


	- Resolve identifer in lexical scope (and parent scopes). Replace load with load2.

	- Resolve typeid_t::unresolved_t in lexical scope (and parent scopes).

	- Compute ANY return type from the call's actual argument types.

	- assign -> assign2
	- bind -> init2, create local symbol





	- analyse_for_statement
		insert loop-iterator as first symbol of loop-body



	- resolve_member_expression

	- corecalls--update
		make_update_member()
		OR
		make_corecall()


	- analyse_construct_value_expression
		tricks for JSON targets, auto convert


	- call
		Handle corecalls
		Convert call-to-type -> make_construct_value_expr()
		Make call-expression

	- Automatically insert make_construct_value_expr() to convert int/double(string/bool -> JSON and convert vector->json,dict -> json


	- Insert builtin types, corecalls and constants into globoal symbol table.
	- Check that all types are resolved


	??? Rename load and load2 to load_names, load_resolved
	??? Rename assign and assign2 to assign_name, assign_resolved
	??? Rename assign and assign2 to assign_name, assign_resolved
	??? Rename bind_local_t -> init_local
	??? Make pass that punches out specialised calls for corecalls, comparisons etc instead of having ONE.
*/

namespace floyd {

struct semantic_ast_t;
struct unchecked_ast_t;

semantic_ast_t run_semantic_analysis(const unchecked_ast_t& ast);

}	// Floyd

#endif /* semantic_analyser_hpp */


