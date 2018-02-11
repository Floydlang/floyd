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

namespace floyd {
	struct statement_t;
	struct ast_json_t;


	//////////////////////////////////////////////////		ast_t


	/*
		Represents the root of the parse tree - the Abstract Syntax Tree
		Immutable
	*/
	struct ast_t {
		public: ast_t();
		public: explicit ast_t(const std::vector<std::shared_ptr<statement_t> > statements);
		public: bool check_invariant() const;


		/////////////////////////////		STATE
		std::vector<std::shared_ptr<statement_t> > _statements;
	};

	ast_json_t ast_to_json(const ast_t& ast);


}	//	floyd

#endif /* parser_ast_hpp */
