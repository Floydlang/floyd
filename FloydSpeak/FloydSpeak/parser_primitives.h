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
	struct function_def_expr_t;
	struct struct_def_expr_t;
	struct value_t;
	struct statement_t;
	struct type_identifier_t;
	
	
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


	//////////////////////////////////////////////////		Text parsing primitives, Floyd specific


	/*
		first: skipped whitespaces
		second: all / any text after whitespace.
	*/
	std::string skip_whitespace(const std::string& s);


	bool is_whitespace(char ch);

	/*
		Skip leading whitespace, get string while type-char.
	*/
	seq read_type(const std::string& s);

	/*
		Skip leading whitespace, get string while identifier char.
	*/
	seq read_identifier(const std::string& s);


	bool is_start_char(char c);
	bool is_end_char(char c);
	char start_char_to_end_char(char start_char);


	/*
		First char is the start char, like '(' or '{'.
	*/

	seq get_balanced(const std::string& s);


	std::pair<type_identifier_t, std::string> read_required_type_identifier(const std::string& s);

	//	Get identifier (name of a defined function or constant variable name).
	std::pair<std::string, std::string> read_required_identifier(const std::string& s);


	/*
		Validates that this is a legal string, with legal characters. Exception.
		Does NOT make sure this a known type-identifier.
		String must not be empty.
	*/
	type_identifier_t make_type_identifier(const std::string& s);



	//////////////////////////////////////////////////		trace_vec()



	template<typename T> void trace_vec(const std::string& title, const std::vector<T>& v){
		QUARK_SCOPED_TRACE(title);
		for(const auto i: v){
			trace(i);
		}
	}


	//////////////////////////////////////////////////		parser_i


	struct parser_i {
		public: virtual ~parser_i(){};
		public: virtual bool parser_i__is_declared_function(const std::string& s) const = 0;
		public: virtual bool parser_i__is_declared_constant_value(const std::string& s) const = 0;
		public: virtual bool parser_i__is_known_type(const std::string& s) const = 0;
	};



	//////////////////////////////////////////////////		ast_t

	/*

		VALUE struct: __global_struct
			int: my_global_int
			function: <int>f(string a, string b) ----> function-def.

			struct_def struct1

			struct1 a = make_struct1()

		types
			struct struct1 --> struct_defs/struct1

		struct_defs
			(struct1) {
				int: a
				int: b
			}
	*/

	struct ast_t : public parser_i {
		public: ast_t(){
		}

		public: bool check_invariant() const {
			return true;
		}

		/////////////////////////////		parser_i

		public: virtual bool parser_i__is_declared_function(const std::string& s) const;
		public: virtual bool parser_i__is_declared_constant_value(const std::string& s) const;
		public: virtual bool parser_i__is_known_type(const std::string& s) const;


		/////////////////////////////		STATE
		//### Function names should have namespace etc.
		//??? Should contain all function definitions, unnamed. Address them using hash of their signature + body.
		public: std::map<std::string, std::shared_ptr<const function_def_expr_t> > _functions;

		public: std::map<std::string, std::shared_ptr<const struct_def_expr_t> > _structs;//???  struct_def_t

		//??? Should contain all functions too.
		public: std::map<std::string, std::shared_ptr<const value_t> > _constant_values;

		public: frontend_types_collector_t _types_collector;

		//	### Stuff all globals into a global struct in the floyd world.
		public: std::vector<std::shared_ptr<statement_t> > _top_level_statements;
	};


	
}	//	floyd_parser



#endif /* parser_primitives_hpp */
