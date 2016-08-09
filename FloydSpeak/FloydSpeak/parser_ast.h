//
//  parser_ast.hpp
//  FloydSpeak
//
//  Created by Marcus Zetterquist on 10/08/16.
//  Copyright © 2016 Marcus Zetterquist. All rights reserved.
//

#ifndef parser_ast_hpp
#define parser_ast_hpp

#include "quark.h"
#include <string>
#include <vector>
#include <map>
#include "parser_types_collector.h"

struct TSHA1;

namespace floyd_parser {
	struct type_def_t;
	struct types_collector_t;
	struct statement_t;
	struct struct_def_t;
	struct function_def_t;
	struct value_t;
	struct scope_def_t;
	struct struct_instance_t;
	struct vector_def_t;


	//////////////////////////////////////		base_type

	/*
		This type is tracked by compiler, not stored in the value-type.
	*/
	enum base_type {
//		k_float,
		k_int,
		k_bool,
		k_string,

//		k_enum,
		k_struct,
//		k_map,
		k_vector,
//		k_seq,
//		k_dyn,
		k_function
	};

	std::string to_string(const base_type t);
	void trace_frontend_type(const type_def_t& t, const std::string& label);


	//////////////////////////////////////		type_def_t

	/*
		Describes a frontend type. All sub-types may or may not be known yet.

		TODO
		- Add memory layout calculation and storage
		- Add support for alternative layout.
		- Add support for optional value (using "?").
	*/
	struct type_def_t {
		public: type_def_t(){};
		public: bool check_invariant() const;

		///////////////////		STATE

		/*
			Plain types only use the _base_type.
			### Add support for int-ranges etc.
		*/
		public: base_type _base_type;
		public: std::shared_ptr<struct_def_t> _struct_def;
		public: std::shared_ptr<vector_def_t> _vector_def;
		public: std::shared_ptr<function_def_t> _function_def;
	};



	/*
		Returns a normalized signature string unique for this data type.
		Use to compare types.

		"<float>"									float
		"<string>"									string
		"<vector>[<float>]"							vector containing floats
		"<float>(<string>,<float>)"				function returning float, with string and float arguments
		"<struct>{<string>a,<string>b,<float>c}”			composite with three named members.
		"<struct>{<string>,<string>,<float>}”			composite with UNNAMED members.
	*/
	std::string to_signature(const type_def_t& t);


	//////////////////////////////////////		type_identifier_t

	/*
		Textual specification of a type-identifier.
		It only contains valid characters.
		There is no guarantee this type actually exists.
	*/

	struct type_identifier_t {
		public: static type_identifier_t make(std::string s);
		public: static type_identifier_t make_int(){
			return make("int");
		}
		public: static type_identifier_t make_bool(){
			return make("bool");
		}
		public: static type_identifier_t make_float(){
			return make("float");
		}
		public: static type_identifier_t make_string(){
			return make("string");
		}

		public: type_identifier_t(const type_identifier_t& other);
		public: type_identifier_t operator=(const type_identifier_t& other);

		public: bool operator==(const type_identifier_t& other) const;
		public: bool operator!=(const type_identifier_t& other) const;

		private: type_identifier_t(){};
		public: explicit type_identifier_t(const char s[]);
		public: explicit type_identifier_t(const std::string& s);
		public: void swap(type_identifier_t& other);
		public: std::string to_string() const;
		public: bool check_invariant() const;


		///////////////////		STATE
		/*
			The name of the type, including its path using :
			"null"

			"bool"
			"int"
			"float"
			"function"

			//	Specifies a data type.
			"value_type"


			"metronome"
			"map<string, metronome>"
			"game_engine:sprite"
			"vector<game_engine:sprite>"
			"int (string, vector<game_engine:sprite>)"
		*/
		private: std::string _type_magic;
	};

	void trace(const type_identifier_t& v);



	//////////////////////////////////////		arg_t

	/*
		Describes a function argument - it's type and the argument name.
	*/
	struct arg_t {
		bool check_invariant() const {
			QUARK_ASSERT(_type.check_invariant());
			QUARK_ASSERT(_identifier.size() > 0);
			return true;
		}
		bool operator==(const arg_t& other) const{
			return _type == other._type && _identifier == other._identifier;
		}

		type_identifier_t _type;
		std::string _identifier;
	};

	void trace(const arg_t& arg);


	//////////////////////////////////////		function_def_t


	struct host_data_i {
		public: virtual ~host_data_i(){};
	};

	typedef value_t (*hosts_function_t)(const std::shared_ptr<host_data_i>& param, const std::vector<value_t>& args);

	struct function_def_t {
		public: function_def_t(
			const type_identifier_t& name,
			const type_identifier_t& return_type,
			const std::vector<arg_t>& args,
			const std::shared_ptr<const scope_def_t>& function_scope
		);
		public: bool check_invariant() const;
		public: bool operator==(const function_def_t& other) const;


		///////////////////		STATE
		public: const type_identifier_t _name;
		public: const type_identifier_t _return_type;
		public: const std::vector<arg_t> _args;
		public: std::shared_ptr<const scope_def_t> _function_scope;
	};

	void trace(const function_def_t& v);
	void trace(const std::vector<std::shared_ptr<statement_t>>& e);
	function_def_t make_function_def(
		const type_identifier_t& name,
		const type_identifier_t& return_type,
		const std::vector<arg_t>& args,
		const std::shared_ptr<const scope_def_t>& function_scope
	);
	TSHA1 calc_function_body_hash(const function_def_t& f);



	//////////////////////////////////////		member_t

	/*
		Definition of a struct-member.
		??? support expressions for default values.
	*/

	struct member_t {
		public: member_t(const type_identifier_t& type, const std::string& name, const value_t& init_value);
		public: member_t(const type_identifier_t& type, const std::string& name);
		bool operator==(const member_t& other) const;
		public: bool check_invariant() const;

		public: std::shared_ptr<type_identifier_t> _type;

		//	Optional -- must have same type as _type.
		public: std::shared_ptr<value_t> _value;

		public: std::string _name;
	};

	void trace(const member_t& member);


	//////////////////////////////////////		struct_def_t

	/*
		Defines a struct, including all members.

		TODO
		- Add member functions / vtable
		- Add memory layout calculation and storage
		- Add support for alternative layout.
		- Add support for optional value (using "?").
	*/

	struct struct_def_t {
		public: static struct_def_t make(
			const type_identifier_t& name,
			const std::vector<member_t>& members,
			const std::shared_ptr<const scope_def_t>& struct_scope
		);

		private: struct_def_t(const type_identifier_t& name, const std::vector<member_t>& members, const std::shared_ptr<const scope_def_t>& struct_scope) :
			_name(name),
			_members(members),
			_struct_scope(struct_scope)
		{
		}
		public: bool check_invariant() const;
		bool operator==(const struct_def_t& other) const;



		///////////////////		STATE
		public: const type_identifier_t _name;
		public: const std::vector<member_t> _members;
		public: const std::shared_ptr<const scope_def_t> _struct_scope;
	};

	void trace(const struct_def_t& e);
	std::string to_signature(const struct_def_t& t);



	//////////////////////////////////////		vector_def_t


	struct vector_def_t {
		public: vector_def_t(){};
		public: bool check_invariant() const;


		///////////////////		STATE
		public: std::string _value_type_identifier;
		public: std::string _key_type_identifier;
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

	void trace(const ast_t& program);

/*
	struct parser_state_t {
		const ast_t _ast;
		scope_def_t _open;
	};
*/




	////////////////////	Helpers for making tests.



	//////////////////////////////////////////////////		trace_vec()



	template<typename T> void trace_vec(const std::string& title, const std::vector<T>& v){
		QUARK_SCOPED_TRACE(title);
		for(const auto i: v){
			trace(i);
		}
	}

	struct_def_t make_struct0(const scope_def_t& scope_def);
	struct_def_t make_struct1(const scope_def_t& scope_def);

	/*
		struct struct2 {
		}
	*/
	struct_def_t make_struct2(const scope_def_t& scope_def);

	/*
		struct struct3 {
			int a
			string b
		}
	*/
	struct_def_t make_struct3(const scope_def_t& scope_def);

	/*
		struct struct4 {
			string x
			<struct_1> y
			string z
		}
	*/
	struct_def_t make_struct4(const scope_def_t& scope_def);

	struct_def_t make_struct5(const scope_def_t& scope_def);



}	//	floyd_parser

#endif /* parser_ast_hpp */
