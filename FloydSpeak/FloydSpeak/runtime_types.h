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
	struct frontend_type_t;


	////////////////////////			frontend_base_type


	/*
		This type is tracked by compiler, not stored in the value-type.
	*/
	enum frontend_base_type {
//		k_float,
		k_int32,
//		k_bool,
		k_string,

//		k_enum,
		k_struct,
//		k_map,
		k_vector
//		k_seq,
//		k_dyn,
//		k_function
	};



	struct identifier {
		std::string _s;
	};


	////////////////////////			member_t



	struct member_t {
		member_t(const std::string& name, const std::string& type_identifier, const std::shared_ptr<frontend_type_t>& type);
		public: bool check_invariant() const;

		std::string _name;
		std::string _type_identifier;
		std::shared_ptr<frontend_type_t> _type;
	};


	////////////////////////			frontend_type_t

	/*
		Recursively describes a frontend type.

		TODO
		- Add member functions / vtable
		- Add memory layout calculation and storage
		- Add support for alternative layout.
		- Add support for optional value (using "?").
	*/
	struct frontend_type_t {
		public: frontend_type_t(){};
		public: bool check_invariant() const;


		///////////////////		STATE

		/*
			Plain types only use the _base_type.
			### Add support for int-ranges etc.
		*/
		public: frontend_base_type _base_type;

		/*
			Struct
				_members are struct members.

			Map
				_value_type is value type
				_key_type is key type.

			Vector
				_value_type is value type


			Function
				_members are function arguments.
				_value_type is function return-type.
		*/

		public: std::vector<member_t> _members;

		public: std::string _value_type_identifier;
		public: std::shared_ptr<frontend_type_t> _value_type;

		public: std::shared_ptr<frontend_type_t> _key_type;

//		public: std::vector<std::pair<std::string, int32_t> > _enum_constants;
	};


	void trace_frontend_type(const frontend_type_t& t);

	/*
		Returns a normalized signature string unique for this data type.
		Use to compare types.
		If the type has a name, it will be prefixed like this: "my_label: ".


		"<null>"									null
		"<float>"									float
		"<string>"									string
		"<vector>[<float>]"							vector containing floats
		"<float>(<string>, <float>)"				function returning float, with string and float arguments
		"{ <string> a, <string> b, <float> c }”			composite with three named members.

		"my_pixel: { <string> a, <string> b, <float> c }”			NAMED composite with three named members.
	*/
	std::string to_signature(const frontend_type_t& t);





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
		public: std::shared_ptr<frontend_type_t> lookup_identifier(const std::string& s) const;
		public: std::shared_ptr<frontend_type_t> lookup_signature(const std::string& s) const;



		///////////////////		STATE

		//	Key is the type identifier.
		//	Value refers to a frontend_type_t stored in _type_definition.
		public: std::map<std::string, std::shared_ptr<frontend_type_t> > _identifiers;

		//	Key is the signature string. De-duplicated.
		public: std::map<std::string, std::shared_ptr<frontend_type_t> > _type_definitions;
	};


}	//	runtime_value


#endif /* runtime_types_hpp */
