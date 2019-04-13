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



	//////////////////////////////////////////////////		general_purpose_ast_t

	struct general_purpose_ast_t {
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

	json_t gp_ast_to_json(const general_purpose_ast_t& ast);
	general_purpose_ast_t json_to_gp_ast(const json_t& json);

	json_t body_to_json(const body_t& e);
	body_t json_to_body(const json_t& json);

	std::vector<json_t> symbols_to_json(const symbol_table_t& symbols);


}	//	floyd

#endif /* parser_ast_hpp */
