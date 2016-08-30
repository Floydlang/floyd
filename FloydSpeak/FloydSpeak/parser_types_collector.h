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


struct json_value_t;

namespace floyd_parser {
	struct type_def_t;
	struct types_collector_t;
	struct statement_t;
	struct value_t;
	struct scope_def_t;



	//////////////////////////////////////		types_collector_t

	/*
		What we know about a type-identifier so far.
		It can be:
		A) an alias for another type-identifier
		B) defined using a type-definition
		C) Neither = the type-identifier is declared but not defined.
	*/
	struct type_indentifier_data_ref {
		//	Used only when this type_identifier is a straigh up alias of an existing type.
		//	The _optional_def is never used in this case.
		//	Instead we chain the type-identifiers.
		public: std::string _alias_type_identifier;

		//	Can be empty. Only way it can be filled is if identifier is later updated
		public: std::shared_ptr<type_def_t> _optional_def;

		public: bool operator==(const type_indentifier_data_ref& other) const;
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
			new_identifier: can be empty (for unnamed type definition)
			type_def: must be valid type def.
			
		*/
		public: types_collector_t define_type_xyz(const std::string& new_identifier, const std::shared_ptr<type_def_t>& type_def) const;

		/*
			return empty: the identifier is unknown or has no type-definition.
			NOTICE: any found alias is resolved recursively.
		*/
		public: std::shared_ptr<const type_def_t> resolve_identifier(const std::string& s) const;



		//////////////////////////////////////		INTERNALS

		/*
			return empty: the identifier is unknown.
			return non-empty: the identifier is known, examine type_indentifier_data_ref to see if it's bound.
			NOTICE: any found alias is resolved recursively.
		*/
		private: std::shared_ptr<const type_indentifier_data_ref> lookup_identifier_deep(const std::string& s) const;

		/*
			existing_identifier: must already be registered (exception).
			new_identifier: must not be registered (exception).
		*/
		private: types_collector_t define_alias_identifier(const std::string& new_identifier, const std::string& existing_identifier) const;

		/*
			Search type definitions for signature.
		*/
		private: std::shared_ptr<const type_def_t> lookup_signature(const std::string& signature) const;

		/*
			new_identifier: must be a valid type identifier.
			type_def == empty: just declare the new type-identifier, don't bind to a type-definition yet.
		*/
		private: types_collector_t define_type_identifier(const std::string& new_identifier, const std::shared_ptr<type_def_t>& type_def) const;

		/*
			Returns true if this type identifier is registered and defined (is an alias or is bound to a type-definition.
		*/
		private: bool is_type_identifier_fully_defined(const std::string& type_identifier) const;

		/*
			return empty: the identifier is unknown.
			return non-empty: the identifier is known, examine type_indentifier_data_ref to see if it's bound.
		*/
		private: std::shared_ptr<type_indentifier_data_ref> lookup_identifier_shallow(const std::string& s) const;


		friend json_value_t types_collector_to_json(const types_collector_t& types);


		//////////////////////////////////////		STATE

		//	Key is the type identifier.
		//	Value refers to a type_def_t stored in _type_definition.
		public: std::map<std::string, type_indentifier_data_ref > _identifiers;

		//	Key is the signature string. De-duplicated.
		//	These are difficult to share between scopes since they have parent-scope references etc.
		public: std::map<std::string, std::shared_ptr<type_def_t> > _type_definitions;
	};




	///////////////////		STRUCTs


	/*
		new_identifier == "": no identifier is registerd for the struct, it is anonymous.
		You can define a type identifier
	*/
	types_collector_t define_struct_type(const types_collector_t& types, const std::string& new_identifier, const std::shared_ptr<const scope_def_t>& struct_def);


	/*
		returns empty if type is unknown or the type is not fully defined.
	*/
	std::shared_ptr<const scope_def_t> resolve_struct_type(const types_collector_t& types, const std::string& s);


	///////////////////		FUNCTIONs


	/*
		new_identifier == "": no identifier is registerd for the struct, it is anonymous.
		You can define a type identifier
	*/
	types_collector_t define_function_type(const types_collector_t& types, const std::string& new_identifier, const std::shared_ptr<const scope_def_t>& function_def);

	/*
		returns empty if type is unknown or the type is not fully defined.
	*/
	std::shared_ptr<const scope_def_t> resolve_function_type(const types_collector_t& types, const std::string& s);


}	//	floyd_parser


#endif /* parser_types_collector_hpp */
