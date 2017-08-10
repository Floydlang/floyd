//
//  floyd_basics.hpp
//  FloydSpeak
//
//  Created by Marcus Zetterquist on 2017-08-09.
//  Copyright © 2017 Marcus Zetterquist. All rights reserved.
//

#ifndef floyd_basics_hpp
#define floyd_basics_hpp

#include <string>
#include <vector>

#include "json_support.h"

namespace floyd_basics {

	const std::vector<std::string> basic_types {
		"bool",
		"char",
		"-code_point",
		"-double",
		"float",
		"float32",
		"float80",
		"-hash",
		"int",
		"int16",
		"int32",
		"int64",
		"int8",
		"-path",
		"string",
		"-text"
	};

	const std::vector<std::string> advanced_types {
		"-clock",
		"-defect_exception",
		"-dyn",
		"-dyn**<>",
		"-enum",
		"-exception",
		"map",
		"-protocol",
		"-rights",
		"-runtime_exception",
		"seq",
		"struct",
		"-typedef",
		"-vector"
	};

	const std::vector<std::string> keywords {
		"assert",
		"-catch",
		"-deserialize()",
		"-diff()",
		"else",
		"-ensure",
		"false",
		"foreach",
		"-hash()",
		"if",
		"-invariant",
		"log",
		"mutable",
		"-namespace",
		"-null",
		"-private",
		"-property",
		"-prove",
		"-require",
		"return",
		"-serialize()",
		"-swap",
		"-switch",
		"-tag",
		"-test",
		"-this",
		"true",
		"-try",
		"-typecast",
		"-typeof",
		"while"
	};


	//////////////////////////////////////		base_type



	//??? Split and categories better. Logic vs equality vs math.

	enum class expression_type {

		//	a + b
		k_math2_add = 10,

		//	a - b
		k_math2_subtract,

		//	a * b
		k_math2_multiply,

		//	a / b
		k_math2_divide,

		//	a % b
		k_math2_remainder,


		//	a <= b
		k_math2_smaller_or_equal,

		//	a < b
		k_math2_smaller,

		//	a >= b
		k_math2_larger_or_equal,

		//	a > b
		k_math2_larger,

		//	a == b
		k_logical_equal,

		//	a != b
		k_logical_nonequal,


		//	a && b
		k_logical_and,

		//	a || b
		k_logical_or,

		//	!a
//			k_logical_not,

		k_constant,

		//	-a
		k_unary_minus,

		//	cond ? a : b
		k_conditional_operator3,
		k_call,

		k_variable,
		k_resolve_member,

		k_lookup_element
	};




	//////////////////////////////////////		base_type

	/*
		This type is tracked by compiler, not stored in the value-type.
	*/
	enum class base_type {
		k_null,
		k_bool,
		k_int,
		k_float,
		k_string,

		k_struct,
		k_vector,
		k_function
	};

	std::string base_type_to_string(const base_type t);


}


#endif /* floyd_basics_hpp */
