//
//  runtime_types.hpp
//  FloydSpeak
//
//  Created by Marcus Zetterquist on 24/07/16.
//  Copyright © 2016 Marcus Zetterquist. All rights reserved.
//

#ifndef runtime_types_hpp
#define runtime_types_hpp

#include <cstdio>
#include <vector>
#include <string>
#include <map>

#include "runtime_core.h"


/*
	type signature:	a string the defines one level of any type. It can be
	typedef: a typesafe identifier for any time.
*/

namespace runtime_types {
	struct type_definition_t;


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


/*
	struct identifier {
		std::string _s;
	};
*/


	////////////////////////			member_t



	struct member_t {
		member_t(const std::string& name, const std::string& type_identifier, const std::shared_ptr<type_definition_t>& type);
		public: bool check_invariant() const;

		std::string _name;
		std::string _type_identifier;
		std::shared_ptr<type_definition_t> _type;
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
		public: std::shared_ptr<type_definition_t> _value_type;

		public: std::shared_ptr<type_definition_t> _key_type;
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
		result, item[0] the memory range for the entire struct.
				item[1] the first member in the struct. Members may be mapped in any order in memory!
				item[2] the first member in the struct.
	*/
	std::vector<byte_range_t> calc_struct_default_memory_layout(const type_definition_t& s);


	////////////////////////			frontend_types_t


	/*
		Holds all types of program:
			- built-ins like float, bool.
			- vector<XYZ>, map etc.
			- custom structs
			- both named and unnamed types.

		A type can be declared but not yet completely defined (maybe forward declared or a member type is still unknown).
	*/
	struct frontend_types_t {
		public: frontend_types_t();
		public: bool check_invariant() const;

		public: frontend_types_t define_alias(const std::string& new_identifier, const std::string& existing_identifier) const;

		/*
			new_identifier == "": no identifier is registerd for the struct, it is anonymous.
		*/
		public: frontend_types_t define_struct_type(const std::string& new_identifier, std::vector<member_t> members) const;

		public: member_t make_member(const std::string& name, const std::string& type_identifier) const;

		/*
			returns
				empty ptr: the identifier is unknown
				object: the identifer is known, examine its contents to see if it is fully defined.
		*/
		public: std::shared_ptr<type_definition_t> lookup_identifier(const std::string& s) const;
		public: std::shared_ptr<type_definition_t> lookup_signature(const std::string& s) const;



		///////////////////		STATE

		//	Key is the type identifier.
		//	Value refers to a type_definition_t stored in _type_definition.
		public: std::map<std::string, std::shared_ptr<type_definition_t> > _identifiers;

		//	Key is the signature string. De-duplicated.
		public: std::map<std::string, std::shared_ptr<type_definition_t> > _type_definitions;
	};


}	//	runtime_value


#endif /* runtime_types_hpp */
