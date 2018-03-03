//
//  pass2.hpp
//  FloydSpeak
//
//  Created by Marcus Zetterquist on 09/08/16.
//  Copyright © 2016 Marcus Zetterquist. All rights reserved.
//

#ifndef pass2_hpp
#define pass2_hpp

#include "quark.h"
#include <vector>
#include <memory>

struct json_t;

namespace floyd {
	struct ast_json_t;
	struct ast_t;
	struct expression_t;
	struct body_t;
	struct statement_t;

	/*
		Input is an array of statements from parser.
		WARNING: Will convert the JSON to other types of data.
	*/
	const std::vector<std::shared_ptr<statement_t> > parser_statements_to_ast__lossy(const quark::trace_context_t& tracer, const ast_json_t& p);


	const std::vector<std::shared_ptr<statement_t> > statements_to_ast__nonlossy(const quark::trace_context_t& tracer, const ast_json_t& p);


	ast_json_t statement_to_json(const statement_t& e);
	ast_json_t body_to_json(const body_t& e);



	ast_json_t expressions_to_json(const std::vector<expression_t> v);

	/*
		An expression is a json array where entries may be other json arrays.
		["+", ["+", 1, 2], ["k", 10]]
	*/
	ast_json_t expression_to_json(const expression_t& e);

	std::string expression_to_json_string(const expression_t& e);



	floyd::ast_t run_pass2(const quark::trace_context_t& tracer, const ast_json_t& parse_tree);
}
#endif /* pass2_hpp */
