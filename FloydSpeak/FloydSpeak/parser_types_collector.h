//
//  parser_types_collector.hpp
//  FloydSpeak
//
//  Created by Marcus Zetterquist on 05/08/16.
//  Copyright © 2016 Marcus Zetterquist. All rights reserved.
//

#ifndef parser_types_collector_hpp
#define parser_types_collector_hpp


#include "quark.h"

#include <cstdio>
#include <vector>
#include <string>
#include <map>

#if 0

struct json_value_t;

namespace floyd_parser {
	struct type_def_t;
	struct types_collector_t;
	struct statement_t;
	struct value_t;
	struct scope_def_t;


	bool is_valid_identifier(const std::string& name);


	//////////////////////////////////////		type_name_entry_t

	/*
		What we know about a type-identifier so far.
		It can be:

		A) an alias for another type-identifier
		B) defined using a type-definition
		C) Neither = the type-identifier is declared but not defined.
	*/
	struct type_name_entry_t {
		/*
			Owns all known types with this name.
			Can be empty if the type has not been defined (yet).
		*/
		//??? Make const.
		public: std::vector<std::shared_ptr<type_def_t>> _defs;

		public: bool check_invariant() const;
		public: bool operator==(const type_name_entry_t& other) const;
	};



	//////////////////////////////////////		types_collector_t

	/*
		This object is used during parsing.
		It support partially defined types and slow lookups.
		You can do lookups recursively but it's pretty slow.

		Holds all types of program:
			- built-ins like float, bool.
			- vector<XYZ>, map etc.
			- custom structs
			- both named and unnamed types.
			- type-defs (aliases)

		A type can be declared but not yet completely defined (maybe forward declared or a member type is still unknown).
	*/
	struct types_collector_t {
		public: types_collector_t();
		public: bool check_invariant() const;

		public: bool operator==(const types_collector_t& other) const;

		/*
			new_identifier. Cannot be empty.
			type_def: must be valid type def.
			
		*/
		public: types_collector_t define_type_xyz(const std::string& new_identifier, const std::shared_ptr<type_def_t>& type_def) const;

		/*
			return empty: the identifier is unknown or has no type-definition.
			NOTICE: any found alias is resolved recursively.
		*/
		public: std::vector<std::shared_ptr<type_def_t>> resolve_identifier(const std::string& name) const;



		//////////////////////////////////////		INTERNALS



		friend json_value_t types_collector_to_json(const types_collector_t& types);


		//////////////////////////////////////		STATE

		//	Key is the type identifier.
		public: std::map<std::string, type_name_entry_t> _types;
	};




	///////////////////		STRUCTs


	/*
		new_identifier == "": no identifier is registerd for the struct, it is anonymous.
		You can define a type identifier
	*/
	types_collector_t define_struct_type(const types_collector_t& types, const std::string& new_name, const std::shared_ptr<const scope_def_t>& struct_def);


	/*
		returns empty if type is unknown or the type is not fully defined.
	*/
	std::shared_ptr<const scope_def_t> resolve_struct_type(const types_collector_t& types, const std::string& name);


	///////////////////		FUNCTIONs


	/*
		new_identifier == "": no identifier is registerd for the struct, it is anonymous.
		You can define a type identifier
	*/
	types_collector_t define_function_type(const types_collector_t& types, const std::string& new_name, const std::shared_ptr<const scope_def_t>& function_def);

	/*
		returns empty if type is unknown or the type is not fully defined.
	*/
	std::shared_ptr<const scope_def_t> resolve_function_type(const types_collector_t& types, const std::string& name);


}	//	floyd_parser
#endif


#endif /* parser_types_collector_hpp */
