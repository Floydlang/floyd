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

namespace floyd {
	struct ast_json_t;


	//////////////////////////////////////////////////		ast_t


	/*
		Represents the root of the parse tree - the Abstract Syntax Tree
		Immutable
	*/
	struct ast_t {
		public: bool check_invariant() const;


		/////////////////////////////		STATE
		public: body_t _globals;
		public: std::vector<const std::shared_ptr<const floyd::function_definition_t>> _function_defs;
	};

	ast_json_t ast_to_json(const ast_t& ast);


}	//	floyd

#endif /* parser_ast_hpp */
