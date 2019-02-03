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


//////////////////////////////////////		ast_json_t



/*
Used to hold an AST encoded as a JSON.

It's a JSON array with each program statement in order.
Each statement is a JSON array like this:

	["return", EXPRESSION_JSON_OBJECT ]
	["bind", "string", "local_name", EXPRESSION_JSON_OBJECT ]
	["def_struct", STRUCT_DEF_JSON_OBJECT ]
	["define_function", FUNCTION_DEF_JSON_OBJECT ]

Element 0: if its a number, this is an extra field before the opcode and it contains the byte position inside compilation unit text.
If a string, this is the node opcode.
	[ 135500, "bind", "^double", "cmath_pi", ["k", 3.14159, "^double"] ],
	[ "bind", "^double", "cmath_pi", ["k", 3.14159, "^double"] ],

- Record byte position inside source file: requires source file to figure out line number.
- Offset is text position inside compilaton unit source -- different ranges maps to different source files, like preheader and includes.




Expression:

	["k", CONSTANT_VALUE, TYPEID_WITH_RESOLVE_STATE_JSON ]
	["@", IDENTIFIER_STRING ]
	["->", EXPRESSION, IDENTIFIER_STRING ]
	["[]", EXPRESSION, EXPRESSION ]
	["+", EXPRESSION, EXPRESSION ]
	["-", EXPRESSION, EXPRESSION ]
	["*", EXPRESSION, EXPRESSION ]
	["/", EXPRESSION, EXPRESSION ]
	["%", EXPRESSION, EXPRESSION ]
	["<=", EXPRESSION, EXPRESSION ]
	["<", EXPRESSION, EXPRESSION ]
	[">=", EXPRESSION, EXPRESSION ]
	[">", EXPRESSION, EXPRESSION ]
	["==", EXPRESSION, EXPRESSION ]
	["!=", EXPRESSION, EXPRESSION ]
	["&&", EXPRESSION, EXPRESSION ]
	["||", EXPRESSION, EXPRESSION ]
	["?:", EXPRESSION, EXPRESSION, EXPRESSION ]
	["call", EXPRESSION, [EXPRESSION] ]

	["unary_minus", EXPRESSION ]


	["construct-value", TYPEID_WITH_RESOLVE_STATE_JSON, [EXPRESSION] ]
	["construct-value", TYPEID_WITH_RESOLVE_STATE_JSON, [EXPRESSION] ]
*/

struct ast_json_t {
	private: ast_json_t(const json_t& v) :
		_value(v)
	{
	}

	public: static ast_json_t make(const json_t& v){
		return { v };
	}
	public: static ast_json_t make(const std::vector<json_t>& v){
		return { json_t::make_array(v) };
	}


	json_t _value;
};



struct location2_t {
	location2_t(const std::string& source_file_path, int line_number, int column, std::size_t start, std::size_t end, const std::string& line) :
		source_file_path(source_file_path),
		line_number(line_number),
		column(column),
		start(start),
		end(end),
		line(line)
	{
	}

	std::string source_file_path;
	int line_number;
	int column;
	std::size_t start;
	std::size_t end;
	std::string line;
};

struct location_t {
	explicit location_t(std::size_t offset) :
		offset(offset)
	{
	}

	std::size_t offset;
};

inline bool operator==(const location_t& lhs, const location_t& rhs){
	return lhs.offset == rhs.offset;
}
extern const location_t k_no_location;


//??? move operation codes here.

//	Keywords in source code.
struct keyword_t {
	static const std::string k_return;
	static const std::string k_while;
	static const std::string k_for;
	static const std::string k_if;
	static const std::string k_else;
	static const std::string k_func;
	static const std::string k_impure;

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
	static const std::string k_protocol;

	static const std::string k_mutable;
	static const std::string k_let;

	static const std::string k_software_system;
	static const std::string k_container_def;

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

}	//	floyd


#endif /* floyd_basics_hpp */
