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


namespace floyd {
	struct value_t;
	struct typeid_t;




	//	Keywords in source code.
	struct keyword_t {
		static const std::string k_return;
		static const std::string k_while;
		static const std::string k_for;
		static const std::string k_if;
		static const std::string k_else;

		static const std::string k_false;
		static const std::string k_true;
		static const std::string k_bool;

		static const std::string k_int;
		static const std::string k_float;
		static const std::string k_string;
		static const std::string k_typeid;
		static const std::string k_json_value;
		static const std::string k_struct;

		static const std::string k_mutable;

		//	"null" is not a keyword, since it can be used in source code.
/*
		"typeid",

		"assert",
		"print",
		"to_string",
		"update",
		"size",
*/

/*
		"catch",
		"deserialize()",
		"diff()",
		"ensure",
		"foreach",
		"hash()",
		"invariant",
		"log",
		"namespace",
		"private",
		"property",
		"prove",
		"require",
		"serialize()",
		"swap",
		"switch",
		"tag",
		"test",
		"this",
		"try",
		"typecast",
		"typeof",
*/

/*
	const std::vector<std::string> basic_types {
		"char",
		code_point",
		double",
		"float32",
		"float80",
		hash",
		"int16",
		"int32",
		"int64",
		"int8",
		path",
		text"
	};
	const std::vector<std::string> advanced_types {
		clock",
		defect_exception",
		dyn",
		dyn**<>",
		enum",
		exception",
		"dict",
		protocol",
		rights",
		runtime_exception",
		"seq",
		typedef",
	};
*/
	};


	json_t values_to_json_array(const std::vector<value_t>& values);

}


#endif /* floyd_basics_hpp */
