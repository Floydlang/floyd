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



	//////////////////////////////////////////////////		itype_t


	struct itype_t {
		int32_t itype;
	};


	//	Assigns 32bit ID to types. You can lookup the type using the ID.
	//	This allows us to describe a type using a single 32bit integer (compact, fast to copy around).
	//	Each type has exactly ONE ID.
	struct type_interner_t {
		bool check_invariant() const {
			return true;
		}
		std::vector<std::pair<itype_t, typeid_t>> interned;
	};


	//////////////////////////////////////////////////		type_interner_t


	std::pair<itype_t, typeid_t> intern_type(type_interner_t& interner, const typeid_t& type);


	//////////////////////////////////////////////////		general_purpose_ast_t

	struct general_purpose_ast_t {
		public: bool check_invariant() const{
			QUARK_ASSERT(_globals.check_invariant());
			return true;
		}

		/////////////////////////////		STATE
		public: body_t _globals;
		public: std::vector<std::shared_ptr<const floyd::function_definition_t>> _function_defs;
		public: type_interner_t _interned_types;
		public: software_system_t _software_system;
		public: container_t _container_def;
	};



	json_t gp_ast_to_json(const general_purpose_ast_t& ast);
	general_purpose_ast_t json_to_gp_ast(const json_t& json);


}	//	floyd

#endif /* parser_ast_hpp */
