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
#include "floyd_syntax.h"

struct seq_t;

namespace floyd {

struct value_t;

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




void ut_verify_json_and_rest(const quark::call_context_t& context, const std::pair<ast_json_t, seq_t>& result_pair, const std::string& expected_json, const std::string& expected_rest);

void ut_verify_values(const quark::call_context_t& context, const value_t& result, const value_t& expected);

void ut_verify(const quark::call_context_t& context, const std::pair<std::string, seq_t>& result, const std::pair<std::string, seq_t>& expected);


}	//	floyd

#endif /* floyd_basics_hpp */
