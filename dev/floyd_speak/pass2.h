//
//  pass2.hpp
//  floyd_speak
//
//  Created by Marcus Zetterquist on 2019-04-13.
//  Copyright © 2019 Marcus Zetterquist. All rights reserved.
//

#ifndef pass2_hpp
#define pass2_hpp


#include "ast.h"


namespace floyd {

	//////////////////////////////////////////////////		pass2_ast_t

	/*
		The Abstract Syntax Tree. It may contain unresolved symbols.
		It can optionally be annotated with all expression types OR NOT.
		Immutable
	*/

	struct pass2_ast_t {
		public: bool check_invariant() const{
			QUARK_ASSERT(_tree.check_invariant());
			return true;
		}

		/////////////////////////////		STATE
		public: general_purpose_ast_t _tree;
	};




	/*
		Why: converts and AST from (1) JSON-data to (2) C++ AST (statement_t, and expression_t, typeid_t) and back to JSON data.
		Transform is *mechanical* and non-lossy roundtrip. This means that the JSON and the C++AST can hold anything
		constructs the parser can deliver -- including some redundant things that are syntactical shortcuts.
		But: going to C++ AST may throw exceptions if the data is too loose to store in C++ AST. Identifier names are
		checked for OK characters and so on. The C++ AST is stricter.

		??? verify roundtrip works 100%

		Parser reads source and generates the AST as JSON. Pass2 translates it to C++ AST.
		Future: generate AST as JSON, process AST as JSON etc.
	*/
	ast_json_t pass2_ast_to_json(const pass2_ast_t& ast);
	pass2_ast_t parse_tree_to_pass2_ast(const parse_tree_json_t& parse_tree);




}	//	floyd



#endif /* pass2_hpp */
