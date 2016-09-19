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
#include "json_support.h"
#include "parser_primitives.h"

struct TSHA1;
struct json_value_t;



//?? AST is not part of floyd_parser!


namespace floyd_parser {
	struct type_def_t;
	struct statement_t;
	struct value_t;
	struct scope_def_t;
	struct struct_instance_t;
	struct vector_def_t;
	struct ast_t;

	typedef std::shared_ptr<const scope_def_t> scope_ref_t;



	//////////////////////////////////////		base_type

	/*
		This type is tracked by compiler, not stored in the value-type.
	*/
	enum class base_type {
		k_null,
		k_bool,
		k_int,
		k_float,
		k_string,

		k_struct,
		k_vector,
		k_function,
		k_subscope
	};

	std::string base_type_to_string(const base_type t);
	void trace2(const type_def_t& t, const std::string& label);






	//////////////////////////////////////		member_t

	/*
		Definition of a struct-member.
	*/

	struct member_t {
		//??? init_value should be an expression.
		public: member_t(const std::shared_ptr<const type_def_t>& type, const std::string& name, const value_t& init_value);
		public: member_t(const std::shared_ptr<const type_def_t>& type, const std::string& name);
		bool operator==(const member_t& other) const;
		public: bool check_invariant() const;

		//??? Skip shared_ptr<>
		public: std::shared_ptr<const type_def_t> _type;

		//	Optional -- must have same type as _type.
		public: std::shared_ptr<const value_t> _value;

		public: std::string _name;
	};

	void trace(const member_t& member);






	struct host_data_i {
		public: virtual ~host_data_i(){};
	};

	typedef value_t (*hosts_function_t)(const ast_t& ast, const std::shared_ptr<host_data_i>& param, const std::vector<value_t>& args);



	void trace(const std::vector<std::shared_ptr<statement_t>>& e);

	TSHA1 calc_function_body_hash(const scope_ref_t& f);




	//////////////////////////////////////		vector_def_t


	/*
		Notice that vector has no scope of its own.
	*/
	struct vector_def_t {
		public: static vector_def_t make2(
			const type_identifier_t& name,
			const std::shared_ptr<type_def_t>& element_type
		);

		public: vector_def_t(){};
		public: bool check_invariant() const;
		public: bool operator==(const vector_def_t& other) const;


		///////////////////		STATE
		public: type_identifier_t _name;
		public: std::shared_ptr<type_def_t> _element_type;
	};

	void trace(const vector_def_t& e);
	std::string to_signature(const vector_def_t& t);
	json_value_t vector_def_to_json(const vector_def_t& s);




	//////////////////////////////////////////////////		scope_def_t



	struct host_func_spec_t {
	};



	//////////////////////////////////////////////////		scope_def_t

	/*
		This is a core piece of the AST. It represents a static, compile-time scope. scope_def_t:s are used to define
		- The global scope
		- struct definition, with member data and functions
		- function definition, with arguments
		- function body
		- function sub-scope - {}, for(){}, while{}, if(){}, else{}.

		The scope_def_t includes optional code, optional member variables and optional local types.
	*/
	struct scope_def_t {
		public: enum class etype {
			k_function_scope,
			k_struct_scope,
			k_global_scope,
			k_subscope
		};

		//	??? Merge with etype, but that effects clients.
		public: enum class efunc_variant {
			k_not_relevant,
			k_interpreted,
			k_default_constructor
		};

		public: static scope_ref_t make_struct(const type_identifier_t& name, const std::vector<member_t>& members);

		public: static scope_ref_t make_function_def(
			const type_identifier_t& name,
			const std::vector<member_t>& args,
			const std::vector<member_t>& local_variables,
			const std::vector<std::shared_ptr<statement_t> >& statements,
			const std::shared_ptr<const type_def_t>& return_type
		);

		public: static scope_ref_t make_builtin_function_def(
			const type_identifier_t& name,
			efunc_variant function_variant,
			const std::shared_ptr<const type_def_t>& type
		);

		public: static scope_ref_t make_host_function_def(
			const type_identifier_t& name,
			const std::vector<member_t>& args,
			const host_func_spec_t& host_func,
			const type_identifier_t& return_type
		);

		public: static scope_ref_t make_global_scope();

		public: scope_def_t(const scope_def_t& other);

		public: bool check_invariant() const;
		public: bool shallow_check_invariant() const;

		public: bool operator==(const scope_def_t& other) const;

		private: explicit scope_def_t(
			etype type,
			const type_identifier_t& name,
			const std::vector<member_t>& args,
			const std::vector<member_t>& local_variables,
			const std::vector<member_t>& members,
			const std::vector<std::shared_ptr<statement_t> >& statements,
			const std::shared_ptr<const type_def_t>& return_type,
			const efunc_variant& function_variant
		);


		/////////////////////////////		STATE
		public: etype _type;
		public: type_identifier_t _name;
		public: std::vector<member_t> _args;
		public: std::vector<member_t> _local_variables;
		public: std::vector<member_t> _members;
		public: const std::vector<std::shared_ptr<statement_t> > _statements;
		public: std::shared_ptr<const type_def_t> _return_type;

		public: efunc_variant _function_variant;
	};

	json_value_t scope_def_to_json(const scope_def_t& scope_def);
	void trace(const scope_ref_t& e);
	std::string to_signature(const scope_ref_t& t);




	//////////////////////////////////////		type_def_t

	/*
		Describes a frontend type. All sub-types may or may not be known yet.
	*/
	struct type_def_t {
		private: type_def_t(base_type type) :
			_base_type(type)
		{
		}
		public: bool check_invariant() const;
		public: bool operator==(const type_def_t& other) const;
		public: void swap(type_def_t& rhs);
		public: base_type get_type() const {
			return _base_type;
		}


		public: static std::shared_ptr<type_def_t> make_null_typedef(){
			return std::make_shared<type_def_t>(type_def_t(base_type::k_null));
		}
		public: static std::shared_ptr<type_def_t> make_bool_typedef(){
			return std::make_shared<type_def_t>(type_def_t(base_type::k_bool));
		}
		public: static std::shared_ptr<type_def_t> make_int_typedef(){
			return std::make_shared<type_def_t>(type_def_t(base_type::k_int));
		}
		public: static std::shared_ptr<type_def_t> make_float_typedef(){
			return std::make_shared<type_def_t>(type_def_t(base_type::k_float));
		}
		public: static std::shared_ptr<type_def_t> make_string_typedef(){
			return std::make_shared<type_def_t>(type_def_t(base_type::k_string));
		}

		public: static std::shared_ptr<type_def_t> make_struct_type_def(const std::shared_ptr<const scope_def_t>& struct_def){
			type_def_t a(base_type::k_struct);
			a._struct_def = struct_def;
			return std::make_shared<type_def_t>(a);
		}
		public: static std::shared_ptr<type_def_t> make_vector_type_def(const std::shared_ptr<const vector_def_t>& vector_def){
			type_def_t a(base_type::k_vector);
			a._vector_def = vector_def;
			return std::make_shared<type_def_t>(a);
		}
		public: static std::shared_ptr<type_def_t> make_function_type_def(const std::shared_ptr<const scope_def_t>& function_def){
			type_def_t a(base_type::k_function);
			a._function_def = function_def;
			return std::make_shared<type_def_t>(a);
		}

		public: std::string to_string() const;

		public: bool is_subscope() const {
			return _base_type == base_type::k_function || _base_type == base_type::k_struct;
		}

		public: std::shared_ptr<const scope_def_t> get_subscope() const {
			QUARK_ASSERT(is_subscope());
			if(_base_type == base_type::k_function){
				return _function_def;
			}
			else if(_base_type == base_type::k_struct){
				return _struct_def;
			}
			QUARK_ASSERT(false);
		}

/*
		public: type_def_t replace_subscope(const std::shared_ptr<const scope_def_t>& new_scope) const {
			QUARK_ASSERT(is_subscope());
			QUARK_ASSERT(
				(get_type() == base_type::k_struct && new_scope->_type == scope_def_t::etype::k_struct_scope)
				|| (get_type() == base_type::k_function && (new_scope->_type == scope_def_t::etype::k_function_scope || new_scope->_type== scope_def_t::etype::k_subscope))
			);
			QUARK_ASSERT(new_scope && new_scope->check_invariant());

			if(_base_type == base_type::k_struct){
				return make_struct_def(new_scope);
			}
			else if(_base_type == base_type::k_function){
				return make_function_def(new_scope);
			}
			else{
				QUARK_ASSERT(false);
			}
		}
*/

		public: std::shared_ptr<const scope_def_t> get_struct_def() const {
			QUARK_ASSERT(_base_type == base_type::k_struct);
			return _struct_def;
		}
		public: std::shared_ptr<const vector_def_t> get_vector_def() const {
			QUARK_ASSERT(_base_type == base_type::k_vector);
			return _vector_def;
		}
		public: std::shared_ptr<const scope_def_t> get_function_def() const {
			QUARK_ASSERT(_base_type == base_type::k_function);
			return _function_def;
		}


		///////////////////		STATE

		/*
			Plain types only use the _base_type.
			### Add support for int-ranges etc.
		*/
		private: base_type _base_type;
		private: std::shared_ptr<const scope_def_t> _struct_def;
		private: std::shared_ptr<const vector_def_t> _vector_def;
		private: std::shared_ptr<const scope_def_t> _function_def;
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

	json_value_t type_def_to_json(const type_def_t& type_def);










	//////////////////////////////////////////////////		ast_t


	/*
		Represents the root of the parse tree - the Abstract Syntax Tree
	*/
	struct ast_t {
		public: ast_t();
		public: bool check_invariant() const;


		/////////////////////////////		STATE
		public: std::shared_ptr<const scope_def_t> _global_scope;

		//	Keyed on "$1000" etc.
		public: std::map<std::string, std::shared_ptr<type_def_t>> _symbols;
	};

	void trace(const ast_t& program);
	json_value_t ast_to_json(const ast_t& ast);


	//////////////////////////////////////////////////		trace_vec()



	template<typename T> void trace_vec(const std::string& title, const std::vector<T>& v){
		QUARK_SCOPED_TRACE(title);
		for(const auto i: v){
			trace(i);
		}
	}

#if false
	scope_ref_t make_struct0(scope_ref_t scope_def);
	scope_ref_t make_struct1(scope_ref_t scope_def);
#endif

	/*
		struct struct2 {
		}
	*/
	scope_ref_t make_struct2(scope_ref_t scope_def);

	/*
		struct struct3 {
			int a
			string b
		}
	*/
	scope_ref_t make_struct3(scope_ref_t scope_def);

	/*
		struct struct4 {
			string x
			<struct_1> y
			string z
		}
	*/
	scope_ref_t make_struct4(scope_ref_t scope_def);

	scope_ref_t make_struct5(scope_ref_t scope_def);

	//	Test all types of members.
	scope_ref_t make_struct6(scope_ref_t scope_def);


}	//	floyd_parser

#endif /* parser_ast_hpp */
