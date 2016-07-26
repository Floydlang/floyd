//
//  parser_primitives.hpp
//  FloydSpeak
//
//  Created by Marcus Zetterquist on 26/07/16.
//  Copyright © 2016 Marcus Zetterquist. All rights reserved.
//

#ifndef parser_primitives_hpp
#define parser_primitives_hpp

#include "quark.h"
#include "text_parser.h"
#include "parser_types.h"

#include <string>
#include <vector>
#include <map>



namespace floyd_parser {


const std::vector<std::string> basic_types {
	"bool",
	"char???",
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
	"-namespace???",
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



const std::string whitespace_chars = " \n\t";
const std::string identifier_chars = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_";
const std::string brackets = "(){}[]<>";
const std::string open_brackets = "({[<";
const std::string type_chars = identifier_chars + brackets;
const std::string number_chars = "0123456789.";
const std::string operator_chars = "+-*/.";






//////////////////////////////////////////////////		Text parsing primitives




/*
	first: skipped whitespaces
	second: all / any text after whitespace.
*/
std::string skip_whitespace(const std::string& s);


bool is_whitespace(char ch);

/*
	Skip leading whitespace, get string while type-char.
*/
seq get_type(const std::string& s);

/*
	Skip leading whitespace, get string while identifier char.
*/
seq get_identifier(const std::string& s);


bool is_start_char(char c);

bool is_end_char(char c);

char start_char_to_end_char(char start_char);


/*
	First char is the start char, like '(' or '{'.
*/

seq get_balanced(const std::string& s);




/*
	These functions knows about the Floyd syntax.
*/


std::pair<type_identifier_t, std::string> read_required_type_identifier(const std::string& s);

//	Get identifier (name of a defined function or constant variable name).
std::pair<std::string, std::string> read_required_identifier(const std::string& s);


	/*
		Validates that this is a legal string, with legal characters. Exception.
		Does NOT make sure this a known type-identifier.
		String must not be empty.
	*/
	type_identifier_t make_type_identifier(const std::string& s);







template<typename T> void trace_vec(const std::string& title, const std::vector<T>& v){
	QUARK_SCOPED_TRACE(title);
	for(const auto i: v){
		trace(i);
	}
}


//??? Move to floyd_parser.h, give parser_expression an interface instead so it can look up variables.

//////////////////////////////////////////////////		identifiers_t

	struct function_def_expr_t;
	struct value_t;

struct identifiers_t {
	identifiers_t(){
	}

	public: bool check_invariant() const {
		return true;
	}

	//### Function names should have namespace etc.
	std::map<std::string, std::shared_ptr<const function_def_expr_t> > _functions;

	std::map<std::string, std::shared_ptr<const value_t> > _constant_values;
};



}	//	floyd_parser




#endif /* parser_primitives_hpp */
