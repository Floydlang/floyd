//
//  parser_primitives.h
//  FloydSpeak
//
//  Created by Marcus Zetterquist on 26/07/16.
//  Copyright © 2016 Marcus Zetterquist. All rights reserved.
//

/*
	These functions knows about the Floyd syntax.
*/

#ifndef parser_primitives_hpp
#define parser_primitives_hpp

#include "quark.h"
#include "text_parser.h"

#include <string>
#include <vector>
#include <map>


struct json_t;

namespace floyd_parser {
	struct statement_t;

	
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


	const std::string whitespace_chars = " \n\t";
	const std::string identifier_chars = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_";
	const std::string brackets = "(){}[]<>";
	const std::string open_brackets = "({[<";
	const std::string type_chars = identifier_chars + brackets;
	const std::string number_chars = "0123456789.";
	const std::string operator_chars = "+-*/.";


	//////////////////////////////////////////////////		Text parsing primitives, Floyd specific


	/*
		first: skipped whitespaces
		second: all / any text after whitespace.
	*/
	std::string skip_whitespace(const std::string& s);
	seq_t skip_whitespace(const seq_t& s);


	bool is_whitespace(char ch);

	bool is_start_char(char c);
	bool is_end_char(char c);
	char start_char_to_end_char(char start_char);

	/*
		First char is the start char, like '(' or '{'.
	*/

	std::pair<std::string, seq_t> get_balanced(const seq_t& s);



	//////////////////////////////////////		SYMBOLS

	/*
		Reads an identifier, like a variable name or function name.
		DOES NOT struct members.
			"hello xxx"
			"hello()xxx"
			"hello+xxx"

		Does not skip whitespace on the rest of the string.
			"\thello\txxx" => "hello" + "\txxx"
	*/
	std::pair<std::string, seq_t> read_required_single_symbol(const seq_t& s);


	//////////////////////////////////////		TYPE IDENTIFIERS



	//////////////////////////////////////		type_identifier_t

	/*
		A string naming a type. "int", "string", "my_struct" etc.
		It is guaranteed to contain correct characters.
		It is NOT guaranteed to map to an actual type in the language or program.

		The name of the type:
			"null"

			"bool"
			"int"
			"float"
			"function"

			"value_type"

			"metronome"
			"map<string, metronome>"
			"game_engine:sprite"
			"vector<game_engine:sprite>"
			"int (string, vector<game_engine:sprite>)"
	*/

	struct type_identifier_t {
		public: type_identifier_t() :
			_type_magic("null")
		{
			QUARK_ASSERT(check_invariant());
		}

		public: static type_identifier_t make(const std::string& s);

		public: static type_identifier_t make_bool(){
			return make("bool");
		}

		public: static type_identifier_t make_int(){
			return make("int");
		}

		public: static type_identifier_t make_float(){
			return make("float");
		}

		public: static type_identifier_t make_string(){
			return make("string");
		}

		public: type_identifier_t(const type_identifier_t& other);
//		public: type_identifier_t operator=(const type_identifier_t& other);

		public: bool operator==(const type_identifier_t& other) const;
		public: bool operator!=(const type_identifier_t& other) const;

		public: explicit type_identifier_t(const char s[]);
		public: explicit type_identifier_t(const std::string& s);
		public: void swap(type_identifier_t& other);
		public: std::string to_string() const;
		public: bool check_invariant() const;

		public: bool is_null() const{
			QUARK_ASSERT(check_invariant());
			return _type_magic == "null";
		}

		///////////////////		STATE
		private: std::string _type_magic;
	};

	void trace(const type_identifier_t& v);


	/*
		Skip leading whitespace, get string while type-char.
	*/
	std::pair<std::string, seq_t> read_type(const seq_t& s);


	/*
		Validates that this is a legal string, with legal characters. Exception.
		Does NOT make sure this a known type-identifier.
		String must not be empty.
	*/
	std::pair<type_identifier_t, seq_t> read_required_type_identifier(const seq_t& s);

	bool is_valid_type_identifier(const std::string& s);

	bool is_math2_op(const std::string& op);



	/*
		Used for:
			struct member variable
			function's local variable


		type is optional, can be "".
		expr is optional, can be null.
		member
		{
			"type": "<int>",
			"name": "my_local",
			"expr": "[\"k\", 1000]
		}
	*/
	json_t make_member_def(const std::string& type, const std::string& name, const json_t& expression);

	json_t make_scope_def();


	/*
		This is a JSON object with all types for global scope.
	*/
	json_t make_builtin_types();


}	//	floyd_parser


#endif /* parser_primitives_hpp */
