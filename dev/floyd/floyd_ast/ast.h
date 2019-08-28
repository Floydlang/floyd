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

#include "quark.h"

#include <vector>

struct json_t;

namespace floyd {


//////////////////////////////////////////////////		itype_t


struct itype_t {
	explicit itype_t(int32_t itype) : itype(itype){}


	bool check_invariant() const {
		return true;
	}



	bool is_undefined() const {
		return itype == (int)base_type::k_string;
	}

	bool is_string() const {
		return itype == (int)base_type::k_string;
	}

	bool is_json() const {
		return itype == (int)base_type::k_json;
	}

	bool is_struct() const {
		return itype >= 100000000 && itype < 200000000;
	}

	bool is_vector() const {
		return itype >= 200000000 && itype < 300000000;
	}

	bool is_dict() const {
		return itype >= 300000000 && itype < 400000000;
	}
	bool is_function() const {
		return itype >= 400000000;
	}




	////////////////////////////////	STATE
	int32_t itype;
};

inline bool operator<(itype_t lhs, itype_t rhs){
	return lhs.itype < rhs.itype;
}



//////////////////////////////////////////////////		type_interner_t




//	Assigns 32 bit ID to types. You can lookup the type using the ID.
//	This allows us to describe a type using a single 32 bit integer (compact, fast to copy around).
//	Each type has exactly ONE ID.

// Automatically insert all basetype-types so they ALWAYS have EXPLICIT integer IDs as itypes.

struct type_interner_t {
	type_interner_t();

	bool check_invariant() const;


	////////////////////////////////	STATE
	std::vector<std::pair<itype_t, typeid_t>> interned;

	int32_t struct_next_id;
	int32_t vector_next_id;
	int32_t dict_next_id;
	int32_t function_next_id;
};


std::pair<itype_t, typeid_t> intern_type(type_interner_t& interner, const typeid_t& type);
itype_t lookup_itype(const type_interner_t& interner, const typeid_t& type);
typeid_t lookup_type(const type_interner_t& interner, const itype_t& type);



inline bool is_atomic_type(itype_t type){
	const auto i = type.itype;
	if(
		i == (int)base_type::k_undefined
		|| i == (int)base_type::k_any
		|| i == (int)base_type::k_void

		|| i == (int)base_type::k_bool
		|| i == (int)base_type::k_int
		|| i == (int)base_type::k_double
		|| i == (int)base_type::k_string
		|| i == (int)base_type::k_json

		|| i == (int)base_type::k_typeid
		|| i == (int)base_type::k_unresolved
	){
		return true;
	}
	else{
		return false;
	}
}

inline bool is_struct(itype_t type){
	return type.is_struct();
}

inline bool is_vector(itype_t type){
	return type.is_vector();
}

inline bool is_dict(itype_t type){
	return type.is_dict();
}

inline bool is_function(itype_t type){
	return type.is_function();
}

inline base_type get_basetype(itype_t itype){
	if(is_atomic_type(itype)){
		const auto result = static_cast<base_type>(itype.itype);
		return result;
	}
	else{
		if(is_struct(itype)){
			return base_type::k_struct;
		}
		else if(is_vector(itype)){
			return base_type::k_vector;
		}
		else if(is_dict(itype)){
			return base_type::k_dict;
		}
		else if(is_function(itype)){
			return base_type::k_function;
		}
		else{
			QUARK_ASSERT(false);
			throw std::exception();
		}
	}
}

inline itype_t make_itype(base_type type){
	const auto value = static_cast<int32_t>(type);
	return itype_t(value);
}

inline itype_t get_undefined_itype(){
	return make_itype(base_type::k_undefined);
}

inline itype_t get_json_itype(){
	return make_itype(base_type::k_json);
}




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
