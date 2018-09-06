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
//#include "pass3.h"

namespace floyd {
	struct ast_json_t;


	//??? rename to raw_ast_t
	//////////////////////////////////////////////////		ast_t


	/*
		Represents the root of the parse tree - the Abstract Syntax Tree
		Immutable
	*/
	struct ast_t {
/*
		ast_t(const ast_t& a) :
			_globals(a._globals),
			_function_defs(a._function_defs)
		{
			QUARK_ASSERT(a.check_invariant());
			QUARK_ASSERT(a._function_defs.empty());
			QUARK_ASSERT(a._globals._symbols.empty());

			QUARK_ASSERT(check_invariant());
		}
*/
		public: bool check_invariant() const{
			QUARK_ASSERT(_globals.check_invariant());
			return true;
		}

		/////////////////////////////		STATE
		public: body_t _globals;
		public: std::vector<std::shared_ptr<const floyd::function_definition_t>> _function_defs;
	};

	ast_json_t ast_to_json(const ast_t& ast);

}	//	floyd

#endif /* parser_ast_hpp */
