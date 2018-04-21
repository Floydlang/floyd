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

		static const std::string k_internal_undefined;
		static const std::string k_internal_dynamic;
		static const std::string k_void;
		static const std::string k_false;
		static const std::string k_true;
		static const std::string k_bool;
		static const std::string k_int;
		static const std::string k_double;
		static const std::string k_string;
		static const std::string k_typeid;
		static const std::string k_json_value;
		static const std::string k_struct;

		static const std::string k_mutable;

		static const std::string k_json_object;
		static const std::string k_json_array;
		static const std::string k_json_string;
		static const std::string k_json_number;
		static const std::string k_json_true;
		static const std::string k_json_false;
		static const std::string k_json_null;

/*
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
		"float",
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

}


#endif /* floyd_basics_hpp */
