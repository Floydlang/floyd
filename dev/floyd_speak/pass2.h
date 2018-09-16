//
//  pass2.hpp
//  FloydSpeak
//
//  Created by Marcus Zetterquist on 09/08/16.
//  Copyright © 2016 Marcus Zetterquist. All rights reserved.
//

#ifndef pass2_hpp
#define pass2_hpp

/*
	Why: converts and AST from (1) JSON-data to (2) C++ AST (statement_t, and expression_t, typeid_t) and back to JSON data.
	Transform is *mechanical* and non-lossy roundtrip. This means that the JSON and the C++AST can hold anything constructs the parser can deliver -- including some redundant things that are syntactical shortcuts.
	But: going to C++ AST may throw exceptions if the data is too loose to store in C++ AST. Identifier names are checked for OK characters and so on. The C++ AST is stricter.

	??? verify roundtrip works 100%

	Parser reads source and generates the AST as JSON. Pass2 translates it to C++ AST.
	Future: generate AST as JSON, process AST as JSON etc.
*/

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
	struct symbol_t;


	const std::vector<std::shared_ptr<statement_t> > astjson_to_statements(const quark::trace_context_t& tracer, const ast_json_t& p);

	ast_json_t statement_to_json(const statement_t& e);
	ast_json_t body_to_json(const body_t& e);

	ast_json_t symbol_to_json(const symbol_t& e);
	std::vector<json_t> symbols_to_json(const std::vector<std::pair<std::string, symbol_t>>& symbols);


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
