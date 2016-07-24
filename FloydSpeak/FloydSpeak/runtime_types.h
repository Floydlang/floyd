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

#include "runtime_core.h"



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



	////////////////////////			member_t

//??? Separate typedef from signature. Typedef is an alias for a signature.


	struct member_t {
		member_t(const std::string& name, const std::shared_ptr<frontend_type_t>& type);
		public: bool check_invariant() const;

		std::string _name;
		std::shared_ptr<frontend_type_t> _type;
	};


	////////////////////////			frontend_type_t

	/*
		Recursively describes a frontend type.
	*/
	struct frontend_type_t {
		private: frontend_type_t(){};
		public: bool check_invariant() const;


		public: static frontend_type_t make_int32(const std::string& name);
		public: static frontend_type_t make_string(const std::string& name);

		/*	name == "": nameless which is OK. ### add type for symbol strings.
		*/
		public: static frontend_type_t make_struct(const std::string& name, const std::vector<member_t>& members);



		///////////////////		STATE
		std::string _typedef;

		/*
			Plain types only use the _base_type.
			### Add support for int-ranges etc.
		*/
		frontend_base_type _base_type;

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

		std::vector<member_t> _members;
		std::shared_ptr<frontend_type_t> _value_type;
		std::shared_ptr<frontend_type_t> _key_type;

//		std::vector<std::pair<std::string, int32_t> > _enum_constants;
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






	//////////////////////////////////////		example structs

	struct test_struct_1 {
		float _a;
		int _b;
	};

	struct test_struct_2 {
		float _c;
		std::shared_ptr<const test_struct_1> _d;
		std::shared_ptr<const test_struct_1> _e;
		int _f;
	};






}	//	runtime_value


#endif /* runtime_types_hpp */
