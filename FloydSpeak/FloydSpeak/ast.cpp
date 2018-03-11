//
//  parser_ast.cpp
//  FloydSpeak
//
//  Created by Marcus Zetterquist on 10/08/16.
//  Copyright © 2016 Marcus Zetterquist. All rights reserved.
//

#include "ast.h"


#include "floyd_basics.h"
#include "statement.h"
#include "pass2.h"


namespace floyd {
	using std::vector;



	////////////////////////			ast_t


	ast_t::ast_t() :
		_globals({})
	{
		QUARK_ASSERT(check_invariant());
	}

	ast_t::ast_t(const body_t& globals) :
		_globals(globals)
	{
		QUARK_ASSERT(check_invariant());
	}

	bool ast_t::check_invariant() const {
		return true;
	}


	ast_json_t ast_to_json(const ast_t& ast){
		QUARK_ASSERT(ast.check_invariant());

		return ast_json_t{json_t::make_object(
			{
				{ "globals", body_to_json(ast._globals)._value }
			}
		)};
	}




} //	floyd
