//
//  parser_ast.cpp
//  FloydSpeak
//
//  Created by Marcus Zetterquist on 10/08/16.
//  Copyright © 2016 Marcus Zetterquist. All rights reserved.
//

#include "parser_ast.h"

#include "statements.h"
#include "parser_value.h"
#include "text_parser.h"
#include "parser_primitives.h"

#include <string>
#include <memory>
#include <map>
#include <iostream>
#include <cmath>
#include "parts/sha1_class.h"
#include "utils.h"
#include "json_support.h"
#include "json_writer.h"


namespace floyd {
	using std::vector;
	using std::string;
	using std::pair;
	using std::make_shared;



	////////////////////////			ast_t


	ast_t::ast_t(){
		QUARK_ASSERT(check_invariant());
	}

	ast_t::ast_t(std::vector<std::shared_ptr<statement_t> > statements) :
		_statements(statements)
	{
		QUARK_ASSERT(check_invariant());
	}

	bool ast_t::check_invariant() const {
		return true;
	}


	json_t ast_to_json(const ast_t& ast){
		QUARK_ASSERT(ast.check_invariant());

		return json_t::make_object(
			{
				{ "statements", statements_to_json(ast._statements) }
			}
		);
	}




} //	floyd
