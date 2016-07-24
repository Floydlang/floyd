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

#include "runtime_core.h"



namespace runtime_types {



	/*
		This type is tracked by compiler, not stored in the value-type.
	*/
	enum frontend_base_type {
		k_float,
		k_int32,
		k_string,
		k_function,
		k_struct,
		k_vector,
		k_mapruntime_value
	};

	/*
		Recursively describes a frontend type.
	*/
	struct frontend_type_t {
		/*
			Plain types only use the _base_type.
			### Add support for int-ranges etc.
		*/
		frontend_base_type _base_type;

		/*
			Function
				_output_type is function return-type.
				_input_types are function arguements.

			Struct
				_inputs_types are struct members.

			Map
				_output_type is value type
				_inputs_types[0] is key type.

			Vector
				_output_type is value type
		*/

		std::shared_ptr<const frontend_type_t> _output_type;
		std::vector<std::shared_ptr<frontend_type_t> > _input_types;
	};




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
