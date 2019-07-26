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
		itype_t(int32_t itype) : itype(itype){}

		int32_t itype;
	};



	//////////////////////////////////////////////////		type_interner_t




	//	Assigns 32 bit ID to types. You can lookup the type using the ID.
	//	This allows us to describe a type using a single 32 bit integer (compact, fast to copy around).
	//	Each type has exactly ONE ID.



	/*

		enum class base_type {
			k_undefined,

			k_any,

			k_void,

			k_bool,
			k_int,
			k_double,
			k_string,
			k_json,

			k_typeid,

			k_struct,
			k_vector,
			k_dict,
			k_function,

			k_unresolved
		};


	*/

	// Automaticall insert all basetype-types so they ALWAYS have EXPLICIT integer IDs as itypes.

	struct type_interner_t {
		type_interner_t();

		bool check_invariant() const;



		bool is_simple(itype_t type){
			return type.itype >= 0 && type.itype < 100000000;
		}
		bool is_struct(itype_t type){
			return type.itype >= 100000000 && type.itype < 200000000;
		}
		bool is_vector(itype_t type){
			return type.itype >= 200000000 && type.itype < 300000000;
		}
		bool is_dict(itype_t type){
			return type.itype >= 300000000 && type.itype < 400000000;
		}
		bool is_function(itype_t type){
			return type.itype >= 400000000;
		}




		////////////////////////////////	STATE
		std::vector<std::pair<itype_t, typeid_t>> interned;

		int32_t simple_next_id;
		int32_t struct_next_id;
		int32_t vector_next_id;
		int32_t dict_next_id;
		int32_t function_next_id;
	};


	std::pair<itype_t, typeid_t> intern_type(type_interner_t& interner, const typeid_t& type);
	itype_t lookup_itype(const type_interner_t& interner, const typeid_t& type);
	typeid_t lookup_type(const type_interner_t& interner, const itype_t& type);






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
