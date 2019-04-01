//
//  parser_ast.hpp
//  FloydSpeak
//
//  Created by Marcus Zetterquist on 10/08/16.
//  Copyright © 2016 Marcus Zetterquist. All rights reserved.
//

#ifndef parser_ast_hpp
#define parser_ast_hpp

#include <vector>
#include "quark.h"
#include "statement.h"
#include "software_system.h"

struct json_t;

namespace floyd {
	struct ast_json_t;


	//////////////////////////////////////////////////		parse_tree_json_t


	struct parse_tree_json_t {
		explicit parse_tree_json_t(const json_t& v): _value(v)
		{
		}

		json_t _value;
	};


	//////////////////////////////////////////////////		ast_t

	/*
		The Abstract Syntax Tree. It may contain unresolved symbols.
		It can optionally be annotated with all expression types OR NOT.
		Immutable
	*/

	struct ast_t {
		public: bool check_invariant() const{
			QUARK_ASSERT(_globals.check_invariant());
			return true;
		}

		/////////////////////////////		STATE
		public: body_t _globals;
		public: std::vector<std::shared_ptr<const floyd::function_definition_t>> _function_defs;
		public: software_system_t _software_system;
		public: container_t _container_def;
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
	ast_json_t ast_to_json(const ast_t& ast);
	ast_t parse_tree_to_ast(const parse_tree_json_t& parse_tree);



	json_t body_to_json(const body_t& e);

	json_t symbol_to_json(const symbol_t& e);
	std::vector<json_t> symbols_to_json(const std::vector<std::pair<std::string, symbol_t>>& symbols);


}	//	floyd

#endif /* parser_ast_hpp */
