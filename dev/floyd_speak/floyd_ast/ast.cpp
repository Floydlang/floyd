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

	////////////////////////			ast_t


	ast_json_t ast_to_json(const ast_t& ast){
		QUARK_ASSERT(ast.check_invariant());

		std::vector<json_t> fds;
		for(const auto& e: ast._function_defs){
			ast_json_t fd = function_def_to_ast_json(*e);
			fds.push_back(fd._value);
		}

		const auto function_defs_json = json_t::make_array(fds);
		return ast_json_t{json_t::make_object(
			{
				{ "globals", body_to_json(ast._globals)._value },
				{ "function_defs", function_defs_json }
			}
		)};
	}

} //	floyd
