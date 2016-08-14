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
struct json_value_t;

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

	typedef std::shared_ptr<scope_def_t> scope_ref_t;



	//////////////////////////////////////		base_type

	/*
		This type is tracked by compiler, not stored in the value-type.
	*/
	enum base_type {
		k_null,
//		k_float,	??? implemented in some places!? Inconsistent.
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
	void trace(const type_def_t& t, const std::string& label);



	//////////////////////////////////////		type_def_t

	/*
		Describes a frontend type. All sub-types may or may not be known yet.

		TODO
		- Add memory layout calculation and storage
		- Add support for alternative layout.
		- Add support for optional value (using "?").
	*/
	struct type_def_t {
		public: type_def_t() :
			_base_type(k_null)
		{
		}
		public: static type_def_t make_int(){
			type_def_t a;
			a._base_type = k_int;
			return a;
		}
		public: bool check_invariant() const;
		public: bool operator==(const type_def_t& other) const;


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
		A string naming a type. "int", "string", "my_struct" etc.
		It is guaranteed to contain correct characters.
		It is NOT guaranteed to map to an actual type in the language or program.
	*/

	struct type_identifier_t {
		public: type_identifier_t() :
			_type_magic("null")
		{
		}

		public: static type_identifier_t make_bool(){
			return make("bool");
		}

		public: static type_identifier_t make(const std::string& s);

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
		public: type_identifier_t operator=(const type_identifier_t& other);

		public: bool operator==(const type_identifier_t& other) const;
		public: bool operator!=(const type_identifier_t& other) const;

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



	//////////////////////////////////////////////////		executable_t


	struct host_data_i {
		public: virtual ~host_data_i(){};
	};

	typedef value_t (*hosts_function_t)(const std::shared_ptr<host_data_i>& param, const std::vector<value_t>& args);

	struct executable_t {
		public: executable_t(hosts_function_t host_function, std::shared_ptr<host_data_i> host_function_param);
		public: executable_t(const std::vector<std::shared_ptr<statement_t> >& statements);
		public: bool check_invariant() const;
		public: bool operator==(const executable_t& other) const;


		/////////////////////////////		STATE

		//	_host_function != nullptr: this is host code.
		//	_host_function == nullptr: _statements contains statements to interpret.
		public: hosts_function_t _host_function = nullptr;
		public: std::shared_ptr<host_data_i> _host_function_param;

		//	INSTRUCTIONS - either _host_function or _statements is used.

		//	Code, if any.
		public: std::vector<std::shared_ptr<statement_t> > _statements;
	};



	//////////////////////////////////////		arg_t

	/*
		Describes a function argument - it's type and the argument name.
	*/
	struct arg_t {
		public: bool check_invariant() const {
			QUARK_ASSERT(_type.check_invariant());
			QUARK_ASSERT(_identifier.size() > 0);
			return true;
		}
		public: bool operator==(const arg_t& other) const{
			QUARK_ASSERT(check_invariant());
			QUARK_ASSERT(other.check_invariant());

			return _type == other._type && _identifier == other._identifier;
		}

		public: type_identifier_t _type;
		public: std::string _identifier;
	};

	void trace(const arg_t& arg);



	//////////////////////////////////////		function_def_t

	//### Function names should have namespace etc.


	struct function_def_t {
		public: function_def_t(
			const type_identifier_t& name,
			const type_identifier_t& return_type,
			const std::vector<arg_t>& args,
			const scope_ref_t parent_scope,
			const executable_t& executable,
			const types_collector_t& types_collector
		);
		public: bool check_invariant() const;

		//	Function scopes must point to the same parent_scope-object to be equal.
		public: bool operator==(const function_def_t& other) const;


		///////////////////		STATE
		public: const type_identifier_t _name;
		public: const type_identifier_t _return_type;
		public: const std::vector<arg_t> _args;

		//	Also contains statements.
		public: scope_ref_t _function_scope;
	};

	void trace(const function_def_t& v);
	void trace(const std::vector<std::shared_ptr<statement_t>>& e);
	function_def_t make_function_def(
		const type_identifier_t& name,
		const type_identifier_t& return_type,
		const std::vector<arg_t>& args,
		const scope_ref_t parent_scope,
		const executable_t& executable,
		const types_collector_t& types_collector
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
		public: static struct_def_t make2(
			const type_identifier_t& name,
			const std::vector<member_t>& members,
			const scope_ref_t parent_scope
		);

		private: struct_def_t(){}
		public: bool check_invariant() const;

		//	Function scopes must point to the same parent_scope-object to be equal.
		public: bool operator==(const struct_def_t& other) const;



		///////////////////		STATE
		public: type_identifier_t _name;
		public: std::vector<member_t> _members;
		public: scope_ref_t _struct_scope;
	};

	void trace(const struct_def_t& e);
	std::string to_signature(const struct_def_t& t);



	//////////////////////////////////////		vector_def_t


	/*
		Notice that vector has no scope of its own.
	*/
	struct vector_def_t {
		public: static vector_def_t make2(
			const type_identifier_t& name,
			const type_identifier_t& element_type
		);

		public: vector_def_t(){};
		public: bool check_invariant() const;
		public: bool operator==(const vector_def_t& other) const;


		///////////////////		STATE
		public: type_identifier_t _name;
		public: type_identifier_t _element_type;
	};

	void trace(const vector_def_t& e);
	std::string to_signature(const vector_def_t& t);


	//////////////////////////////////////////////////		scope_def_t

//??? make private data, immutable
//??? add scope name!

	/*
		WARNING: We mutate this during parsing, adding executable, types while it exists.
		WARNING 2: this object forms an intrusive hiearchy between scopes and sub-scopes -- give it a new address (move / copy) breaks this hearchy.

		Represents
		- global scope
		- struct definition, with member data and functions
		- function definition, with arguments
		- function body
		- function sub-scope - {}, for(){}, while{}, if(){}, else{}.
	*/
	struct scope_def_t {
		public: static scope_ref_t make2(const scope_ref_t parent_scope, const executable_t& executable, const types_collector_t& types_collector);
		public: static scope_ref_t make_global_scope();
		private: explicit scope_def_t(const scope_ref_t parent_scope, const executable_t& executable, const types_collector_t& types_collector);
		public: scope_def_t(const scope_def_t& other);

		public: bool check_invariant() const;
		public: bool shallow_check_invariant() const;

		//	Must point to the same parent_scope-object, or both nullptr.
		public: bool operator==(const scope_def_t& other) const;


		/////////////////////////////		STATE

		public: std::weak_ptr<scope_def_t> _parent_scope;
		public: executable_t _executable;
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

	json_value_t scope_def_to_json(const scope_def_t& scope_def);


	//////////////////////////////////////////////////		xxxscope_node_t


	/*
		Idea for non-intrusive scope tree.
	*/

	struct xxxscope_node_t {
		public: bool check_invariant() const {
			return true;
		};

		scope_def_t _scope;
		std::vector<scope_def_t> _children;
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
	*/

	struct ast_t {
		public: ast_t() :
			_global_scope(
				scope_def_t::make_global_scope()
			)
		{
		}

		public: bool check_invariant() const {
			QUARK_ASSERT(_global_scope->check_invariant());
			return true;
		}


		/////////////////////////////		STATE
		public: scope_ref_t _global_scope;
	};

	void trace(const ast_t& program);




	////////////////////	Helpers for making tests.



	//////////////////////////////////////////////////		trace_vec()



	template<typename T> void trace_vec(const std::string& title, const std::vector<T>& v){
		QUARK_SCOPED_TRACE(title);
		for(const auto i: v){
			trace(i);
		}
	}

	struct_def_t make_struct0(scope_ref_t scope_def);
	struct_def_t make_struct1(scope_ref_t scope_def);

	/*
		struct struct2 {
		}
	*/
	struct_def_t make_struct2(scope_ref_t scope_def);

	/*
		struct struct3 {
			int a
			string b
		}
	*/
	struct_def_t make_struct3(scope_ref_t scope_def);

	/*
		struct struct4 {
			string x
			<struct_1> y
			string z
		}
	*/
	struct_def_t make_struct4(scope_ref_t scope_def);

	struct_def_t make_struct5(scope_ref_t scope_def);

	//	Test all types of members.
	struct_def_t make_struct6(scope_ref_t scope_def);


}	//	floyd_parser

#endif /* parser_ast_hpp */
