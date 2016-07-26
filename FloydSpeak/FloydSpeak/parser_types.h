//
//  parse_types.h
//  FloydSpeak
//
//  Created by Marcus Zetterquist on 24/07/16.
//  Copyright © 2016 Marcus Zetterquist. All rights reserved.
//

#ifndef parser_types_hpp
#define parser_types_hpp

#include <cstdio>
#include <vector>
#include <string>
#include <map>


/*
	type signature:	a string the defines one level of any type. It can be
	typedef: a typesafe identifier for any time.
*/

namespace floyd_parser {
	struct type_definition_t;
	struct frontend_types_collector_t;


	////////////////////////			frontend_base_type


	/*
		This type is tracked by compiler, not stored in the value-type.
	*/
	enum frontend_base_type {
//		k_float,
		k_int32,
		k_bool,
		k_string,

//		k_enum,
		k_struct,
//		k_map,
		k_vector
//		k_seq,
//		k_dyn,
//		k_function
	};



	//////////////////////////////////////////////////		type_identifier_t



	struct type_identifier_t {
		public: static type_identifier_t make_type(std::string s);
		public: bool operator==(const type_identifier_t& other) const;
		public: bool operator!=(const type_identifier_t& other) const;

		private: type_identifier_t(){};
		public: explicit type_identifier_t(const char s[]);
		public: explicit type_identifier_t(const std::string& s);
		public: void swap(type_identifier_t& other);
		public: std::string to_string() const;
		public: bool check_invariant() const;


		///////////////////		STATE
		/*
			The name of the type, including its path using :
			"null"

			"bool"
			"int"
			"float"
			"function"

			//	Specifies a data type.
			"value_type"


			"metronome"
			"map<string, metronome>"
			"game_engine:sprite"
			"vector<game_engine:sprite>"
			"int (string, vector<game_engine:sprite>)"
		*/
		private: std::string _type_magic;
	};



	////////////////////////			member_t



	struct member_t {
		member_t(const std::string& name, const std::string& type_identifier);
		public: bool check_invariant() const;

		std::string _name;
		std::string _type_identifier;
	};


	////////////////////////			struct_def_t

	/*
		TODO
		- Add member functions / vtable
		- Add memory layout calculation and storage
		- Add support for alternative layout.
		- Add support for optional value (using "?").
	*/

	struct struct_def_t {
		public: struct_def_t(){};
		public: bool check_invariant() const;


		///////////////////		STATE
		public: std::vector<member_t> _members;
	};


	////////////////////////			struct_def_t


	struct vector_def_t {
		public: vector_def_t(){};
		public: bool check_invariant() const;


		///////////////////		STATE
		public: std::string _value_type_identifier;
		public: std::string _key_type_identifier;
	};


	////////////////////////			type_definition_t

	/*
		Recursively describes a frontend type.

		TODO
		- Add memory layout calculation and storage
		- Add support for alternative layout.
		- Add support for optional value (using "?").
	*/
	struct type_definition_t {
		public: type_definition_t(){};
		public: bool check_invariant() const;


		///////////////////		STATE

		/*
			Plain types only use the _base_type.
			### Add support for int-ranges etc.
		*/
		public: frontend_base_type _base_type;
		public: std::shared_ptr<struct_def_t> _struct_def;
		public: std::shared_ptr<vector_def_t> _vector_def;
	};





	void trace_frontend_type(const type_definition_t& t, const std::string& label);

	/*
		Returns a normalized signature string unique for this data type.
		Use to compare types.

		"<float>"									float
		"<string>"									string
		"<vector>[<float>]"							vector containing floats
		"<float>(<string>,<float>)"				function returning float, with string and float arguments
		"<struct>{<string>a,<string>b,<float>c}”			composite with three named members.
		"<struct>{<string>,<string>,<float>}”			composite with UNNAMED members.
	*/
	std::string to_signature(const type_definition_t& t);

	typedef std::pair<std::size_t, std::size_t> byte_range_t;

	/*
		s: all types must be fully defined, deeply, for this function to work.
		result, item[0] the memory range for the entire struct.
				item[1] the first member in the struct. Members may be mapped in any order in memory!
				item[2] the first member in the struct.
	*/
	std::vector<byte_range_t> calc_struct_default_memory_layout(const frontend_types_collector_t& types, const type_definition_t& s);



	////////////////////////			frontend_types_collector_t



	struct type_indentifier_data_ref {
		//	Used only when this type_identifier is a straigh up alias of an existing type.
		//	The _optional_def is never used in this case.
		//	Instead we chain the type-identifiers.
		public: std::string _alias_type_identifier;

		//	Can be empty. Only way it can be filled is if identifier is later updated
		public: std::shared_ptr<type_definition_t> _optional_def;
	};


	////////////////////////			frontend_types_collector_t


	/*
		Holds all types of program:
			- built-ins like float, bool.
			- vector<XYZ>, map etc.
			- custom structs
			- both named and unnamed types.

		A type can be declared but not yet completely defined (maybe forward declared or a member type is still unknown).


		This object is only used during parsing.
		It support partially defines types and slow lookups.

		When parsing is complete, a faster compile-time object is created instead.
	*/
	struct frontend_types_collector_t {
		public: frontend_types_collector_t();
		public: bool check_invariant() const;

		/*
			Returns true if this type identifier is registerd and defined (is an alias or is bound to a type-definition.
		*/
		public: bool is_type_identifier_fully_defined(const std::string& type_identifier) const;


		/*
			existing_identifier: must already be registered (exception).
			new_identifier: must not be registered (exception).
		*/
		public: frontend_types_collector_t define_alias_identifier(const std::string& new_identifier, const std::string& existing_identifier) const;

		/*
			type_def == empty: just declare the new type-identifier, don't bind to a type-definition yet.
		*/
		public: frontend_types_collector_t define_type_identifier(const std::string& new_identifier, const std::shared_ptr<type_definition_t>& type_def) const;

		/*
			Adds type-definition for a struct.
			If the exact same struct (same signature) already exists, the old one is returned. No duplicates.
		*/
		public: std::pair<std::shared_ptr<type_definition_t>, frontend_types_collector_t> define_struct_type(const std::vector<member_t>& members) const;

		/*
			new_identifier == "": no identifier is registerd for the struct, it is anonymous.
			You can define a type identifier
		*/
		public: frontend_types_collector_t define_struct_type(const std::string& new_identifier, std::vector<member_t> members) const;


		/*
			return empty: the identifier is unknown.
			return non-empty: the identifier is known, examine type_indentifier_data_ref to see if it's bound.
		*/
		public: std::shared_ptr<type_indentifier_data_ref> lookup_identifier_shallow(const std::string& s) const;

		/*
			return empty: the identifier is unknown.
			return non-empty: the identifier is known, examine type_indentifier_data_ref to see if it's bound.
			NOTICE: any found alias is resolved recursively.
		*/
		public: std::shared_ptr<type_indentifier_data_ref> lookup_identifier_deep(const std::string& s) const;


		/*
			return empty: the identifier is unknown or has no type-definition.
			NOTICE: any found alias is resolved recursively.
		*/
		public: std::shared_ptr<type_definition_t> resolve_identifier(const std::string& s) const;


		public: std::shared_ptr<type_definition_t> lookup_signature(const std::string& s) const;


		///////////////////		STATE

		//	Key is the type identifier.
		//	Value refers to a type_definition_t stored in _type_definition.
		private: std::map<std::string, type_indentifier_data_ref > _identifiers;

		//	Key is the signature string. De-duplicated.
		private: std::map<std::string, std::shared_ptr<type_definition_t> > _type_definitions;
	};


}	//	floyd_parser


#endif
