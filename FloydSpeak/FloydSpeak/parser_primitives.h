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
#include "parser_types.h"
#include "parser_types_collector.h"
#include "parser_value.h"

#include <string>
#include <vector>
#include <map>


namespace floyd_parser {
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

	bool is_start_char(char c);
	bool is_end_char(char c);
	char start_char_to_end_char(char start_char);

	/*
		First char is the start char, like '(' or '{'.
	*/

	seq get_balanced(const std::string& s);



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
	seq read_required_single_symbol(const std::string& s);


	//////////////////////////////////////		TYPE IDENTIFIERS


	/*
		Skip leading whitespace, get string while type-char.
	*/
	seq read_type(const std::string& s);


	std::pair<type_identifier_t, std::string> read_required_type_identifier(const std::string& s);

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


	//////////////////////////////////////////////////		type_definition2_t


	struct type_definition2_t {
		public: type_definition2_t(){};
		public: bool check_invariant() const;


		///////////////////		STATE

		public: std::string _symbol;

		// ### generate collections / templates, do not require them to be instantiated to be be able to make instances.
		public: enum {
			k_struct_def,
			k_function_def
		};

		/*
			Plain types only use the _base_type.
			### Add support for int-ranges etc.
		*/
		public: frontend_base_type _base_type;
		public: std::shared_ptr<struct_def_t> _struct_def;
//		public: std::shared_ptr<vector_def_t> _vector_def;
		public: std::shared_ptr<function_def_t> _function_def;
	};


	//////////////////////////////////////////////////		scope_def_t


	/*
		Represents
		- global scope
		- struct definition, with member data and functions
		- function definition, with arguments
		- function body
		- function sub-scope - {}, for(){}, while{}, if(){}, else{}.
	*/
	struct scope_def_t {
		public: static std::shared_ptr<scope_def_t> make_subscope(const scope_def_t& parent_scope){
			return std::make_shared<scope_def_t>(scope_def_t(true, &parent_scope));
		}
		public: static std::shared_ptr<scope_def_t> make_global_scope(){
			return std::make_shared<scope_def_t>(scope_def_t(true, nullptr));
		}
		private: explicit scope_def_t(bool dummy, const scope_def_t* parent_scope) :
			_parent_scope(parent_scope)
		{
		}
		public: scope_def_t(const scope_def_t& other) :
			_parent_scope(other._parent_scope),
			_statements(other._statements),
			_host_function(other._host_function),
			_host_function_param(other._host_function_param),
			_types_collector(other._types_collector)
		{
		};
		public: bool check_invariant() const;
		public: bool operator==(const scope_def_t& other) const;


		/////////////////////////////		STATE

		//	Used for finding parent symbols, for making symbol paths.
		public: const scope_def_t* _parent_scope = nullptr;

		//	INSTRUCTIONS - either _host_function or _statements is used.

			//	Code, if any.
			public: std::vector<std::shared_ptr<statement_t> > _statements;

			//	Either _host_function or _statements is used.
			public: hosts_function_t _host_function = nullptr;
			public: std::shared_ptr<host_data_i> _host_function_param;

		public: types_collector_t _types_collector;

		/*
			Key is symbol name or a random string if unnamed.
			### Allow many types to have the same symbol name (pixel, pixel several pixel constructor etc.).
				Maybe use better key that encodes those differences?
		*/
		//		public: std::map<std::string, std::vector<type_def_t>> _symbolsxxxx;

		//	Specification of values to store in each instance.
		//		public: std::vector<member_t> _runtime_value_spec;
	};

	struct scope_instance_t {
		public: const scope_def_t* _def = nullptr;

		//	### idea: Values are indexes same as scope_def_t::_runtime_value_spec.
		//	key string is name of variable.
		public: std::map<std::string, value_t> _values;
	};


	//////////////////////////////////////////////////		ast_t

	/*
		Function definitions have a type and a body and an optional name.


		This is a function definition for a function definition called "function4":
			"int function4(int a, string b){
				return a + 1;
			}"


		This is an unnamed function function definition.
			"int(int a, string b){
				return a + 1;
			}"

		Here a constant x points to the function definition.
			auto x = function4

		Functions and structs do not normally become values, they become *types*.


		{
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

		}
		//### Function names should have namespace etc.
		//	### Stuff all globals into a global struct in the floyd world.
	*/

	struct ast_t {
		public: ast_t() :
			_global_scope(scope_def_t::make_global_scope())
		{
		}

		public: bool check_invariant() const {
			return true;
		}


		/////////////////////////////		STATE
		public: std::shared_ptr<scope_def_t> _global_scope;
	};


/*
	struct parser_state_t {
		const ast_t _ast;
		scope_def_t _open;
	};
*/

}	//	floyd_parser



#endif /* parser_primitives_hpp */
