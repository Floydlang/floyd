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
//#include "parser_value.h"

struct TSHA1;
struct json_t;



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
		k_function
	};

	std::string base_type_to_string(const base_type t);
	void trace2(const type_def_t& t, const std::string& label);


	//////////////////////////////////////		typeid_t


	struct typeid_t;
	json_t typeid_to_json(const typeid_t& t);

	struct typeid_t {

		public: static typeid_t make_null(){
			return { base_type::k_null, {}, {} };
		}

		public: static typeid_t make_bool(){
			return { base_type::k_bool, {}, {} };
		}

		public: static typeid_t make_int(){
			return { base_type::k_int, {}, {} };
		}

		public: static typeid_t make_float(){
			return { base_type::k_float, {}, {} };
		}

		public: static typeid_t make_string(){
			return { base_type::k_string, {}, {} };
		}

		public: bool is_null() const {
			return _base_type == base_type::k_null;
		}

		public: static typeid_t make_struct(const std::string& struct_def_id){
			return { base_type::k_struct, {}, struct_def_id };
		}

		public: static typeid_t make_vector(const typeid_t& element_type){
			return { base_type::k_vector, { element_type }, {} };
		}

		public: static typeid_t make_function(const typeid_t& ret, const std::vector<typeid_t>& args){
			//	Functions use _parts[0] for return type always. _parts[1] is first argument, if any.
			std::vector<typeid_t> parts = { ret };
			parts.insert(parts.end(), args.begin(), args.end());
			return { base_type::k_function, parts, {} };
		}


		public: bool operator==(const typeid_t& other) const{
			return _base_type == other._base_type && _parts == other._parts && _struct_def_id == other._struct_def_id;
		}

		public: bool check_invariant() const;

		public: void swap(typeid_t& other);

		/*
			"int"
			"[int]"
			"int (float, [int])"
			"coord_t/8000"
		*/
		public: std::string to_string() const;

		public: base_type get_base_type() const{
			return _base_type;
		}


		/////////////////////////////		STATE


		/*
			"coord_t/8000"
			"int (float a, float b)"
			"[string]"
			"[coord_t/8000]"
			"pixel_coord_t = coord_t/8000"
		*/

		base_type _base_type;
		std::vector<typeid_t> _parts;
		std::string _struct_def_id;
	};

	json_t typeid_to_json(const typeid_t& t);

	
	

	//////////////////////////////////////		member_t

	/*
		Definition of a struct-member.
	*/

	struct member_t {
		//??? init_value should be an expression.
		public: member_t(const typeid_t& type, const std::string& name, const value_t& init_value);
		public: member_t(const typeid_t& type, const std::string& name);
		bool operator==(const member_t& other) const;
		public: bool check_invariant() const;


		/////////////////////////////		STATE
		public: typeid_t _type;

		//	Optional -- must have same type as _type.
		public: std::shared_ptr<const value_t> _value;

		public: std::string _name;
	};

	void trace(const member_t& member);



/*
	struct host_data_i {
		public: virtual ~host_data_i(){};
	};

	typedef value_t (*hosts_function_t)(const ast_t& ast, const std::shared_ptr<host_data_i>& param, const std::vector<value_t>& args);
*/


	void trace(const std::vector<std::shared_ptr<statement_t>>& e);




	//////////////////////////////////////		vector_def_t


	/*
		Notice that vector has no scope of its own.
	*/
	struct vector_def_t {
		public: static vector_def_t make2(
			const type_identifier_t& name,
			const typeid_t& element_type
		);

		public: vector_def_t(){};
		public: bool check_invariant() const;
		public: bool operator==(const vector_def_t& other) const;


		/////////////////////////////		STATE
		public: type_identifier_t _name;
		public: typeid_t _element_type;
	};

	void trace(const vector_def_t& e);
	json_t vector_def_to_json(const vector_def_t& s);



	//////////////////////////////////////////////////		scope_def_t

	/*
		This is a core internal building block of the AST. It represents a static, compile-time scope.
		scope_def_t:s are used to define

		- The global scope
		- struct definition, with member data and functions
		- function definition, with arguments
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
			const typeid_t& return_type
		);

		public: static scope_ref_t make_builtin_function_def(
			const type_identifier_t& name,
			efunc_variant function_variant,
			const typeid_t& type
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
			const typeid_t& return_type,
			const efunc_variant& function_variant
		);


		/////////////////////////////		STATE
		public: etype _type;
		public: type_identifier_t _name;
		public: std::vector<member_t> _args;
		public: std::vector<member_t> _local_variables;
		public: std::vector<member_t> _members;
		public: const std::vector<std::shared_ptr<statement_t> > _statements;
		public: typeid_t _return_type;

		public: efunc_variant _function_variant;
	};

	json_t scope_def_to_json(const scope_def_t& scope_def);
	void trace(const scope_ref_t& e);



	//////////////////////////////////////////////////		ast_t


	/*
		"get_grey": { "value": "function_constant_values/8000" },
	*/
	struct symbol_t {
		enum symbol_type {
			k_null,
			k_function_def_object,
			k_struct_def_object,
			k_constant
		};

		symbol_type _type;
		std::string _object_id;
		std::shared_ptr<value_t> _constant;
		typeid_t _typeid;
	};


	//////////////////////////////////////////////////		ast_t


	/*
		Represents the root of the parse tree - the Abstract Syntax Tree
		Immutable
	*/
	struct ast_t {
		public: ast_t();
		public: ast_t(
			std::shared_ptr<const scope_def_t> global_scope,
			const std::map<std::string, symbol_t>& symbols,
			const std::map<std::string, std::shared_ptr<const scope_def_t> > objects
		);
		public: bool check_invariant() const;

		public: const std::shared_ptr<const scope_def_t>& get_global_scope() const {
			return _global_scope;
		}

		public: const std::map<std::string, symbol_t>& get_symbols() const {
			return _symbols;
		}
		public: const std::map<std::string, std::shared_ptr<const scope_def_t> >& get_objects() const {
			return _objects;
		}

		/////////////////////////////		STATE
		private: std::shared_ptr<const scope_def_t> _global_scope;

		private: std::map<std::string, symbol_t> _symbols;
		private: std::map<std::string, std::shared_ptr<const scope_def_t> > _objects;
	};

	void trace(const ast_t& program);
	json_t ast_to_json(const ast_t& ast);


	//////////////////////////////////////////////////		trace_vec()



	template<typename T> void trace_vec(const std::string& title, const std::vector<T>& v){
		QUARK_SCOPED_TRACE(title);
		for(const auto i: v){
			trace(i);
		}
	}

}	//	floyd_parser

#endif /* parser_ast_hpp */
