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

	//	Number at end of name tells number of input expressions operation has.
	enum class expression_type {

		//	a + b
		k_arithmetic_add__2 = 10,

		//	a - b
		k_arithmetic_subtract__2,

		//	a * b
		k_arithmetic_multiply__2,

		//	a / b
		k_arithmetic_divide__2,

		//	a % b
		k_arithmetic_remainder__2,


		//	a <= b
		k_comparison_smaller_or_equal__2,

		//	a < b
		k_comparison_smaller__2,

		//	a >= b
		k_comparison_larger_or_equal__2,

		//	a > b
		k_comparison_larger__2,


		//	a == b
		k_logical_equal__2,

		//	a != b
		k_logical_nonequal__2,


		//	a && b
		k_logical_and__2,

		//	a || b
		k_logical_or__2,

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
