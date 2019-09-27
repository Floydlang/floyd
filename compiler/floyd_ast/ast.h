//
//  parser_ast.hpp
//  Floyd
//
//  Created by Marcus Zetterquist on 10/08/16.
//  Copyright © 2016 Marcus Zetterquist. All rights reserved.
//

#ifndef parser_ast_hpp
#define parser_ast_hpp

#include "statement.h"
#include "software_system.h"
#include "types.h"

#include "quark.h"

#include <vector>

struct json_t;

namespace floyd {


//////////////////////////////////////////////////		general_purpose_ast_t


struct general_purpose_ast_t {
	public: bool check_invariant() const{
		QUARK_ASSERT(_globals.check_invariant());
		return true;
	}

	/////////////////////////////		STATE
	public: body_t _globals;
	public: std::vector<floyd::function_definition_t> _function_defs;
	public: type_interner_t _interned_types;
	public: software_system_t _software_system;
	public: container_t _container_def;
};




json_t gp_ast_to_json(const general_purpose_ast_t& ast);
general_purpose_ast_t json_to_gp_ast(const json_t& json);




//////////////////////////////////////////////////		unchecked_ast_t

/*
	An AST that may contain unresolved symbols.
	It can optionally be annotated with all expression types OR NOT.
	Immutable
*/

struct unchecked_ast_t {
	public: bool check_invariant() const{
		QUARK_ASSERT(_tree.check_invariant());
		return true;
	}

	/////////////////////////////		STATE
	public: general_purpose_ast_t _tree;
};



}	//	floyd

#endif /* parser_ast_hpp */
