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
#include "utils.h"
#include "parser_primitives.h"
#include "floyd_basics.h"

struct json_t;


namespace floyd_ast {
	struct expression_t;
	struct statement_t;
	struct value_t;
	struct lexical_scope_t;
	struct struct_instance_t;
	struct vector_def_t;
	struct ast_t;


	//??? Move to floyd_basics?
	//////////////////////////////////////		typeid_t


	struct typeid_t;
	json_t typeid_to_json(const typeid_t& t);

	struct typeid_t {

		public: static typeid_t make_null(){
			return { floyd_basics::base_type::k_null, {}, {}, {} };
		}

		public: static typeid_t make_bool(){
			return { floyd_basics::base_type::k_bool, {}, {}, {} };
		}

		public: static typeid_t make_int(){
			return { floyd_basics::base_type::k_int, {}, {}, {} };
		}

		public: static typeid_t make_float(){
			return { floyd_basics::base_type::k_float, {}, {}, {} };
		}

		public: static typeid_t make_string(){
			return { floyd_basics::base_type::k_string, {}, {}, {} };
		}

		public: static typeid_t make_unresolved_symbol(const std::string& s){
			return { floyd_basics::base_type::k_null, {}, {}, s };
		}

		public: bool is_null() const {
			return _base_type == floyd_basics::base_type::k_null;
		}

		public: static typeid_t make_struct(const std::string& struct_def_id){
			return { floyd_basics::base_type::k_struct, {}, struct_def_id, {} };
		}

		public: static typeid_t make_vector(const typeid_t& element_type){
			return { floyd_basics::base_type::k_vector, { element_type }, {}, {} };
		}

		public: static typeid_t make_function(const typeid_t& ret, const std::vector<typeid_t>& args){
			//	Functions use _parts[0] for return type always. _parts[1] is first argument, if any.
			std::vector<typeid_t> parts = { ret };
			parts.insert(parts.end(), args.begin(), args.end());
			return { floyd_basics::base_type::k_function, parts, {}, {} };
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
			??? use json instead.
		*/
		public: std::string to_string() const;

		public: floyd_basics::base_type get_base_type() const{
			return _base_type;
		}


		/////////////////////////////		STATE


		/*
			"coord_t"
			"coord_t/8000"
			"int (float a, float b)"
			"[string]"
			"[string([bool(float,string),pixel)])"
			"[coord_t/8000]"
			"pixel_coord_t = coord_t/8000"
		*/
		floyd_basics::base_type _base_type;
		std::vector<typeid_t> _parts;
		std::string _struct_def_id;

		//	This is used it overrides _base_type (which will be null).
		std::string _unresolved_type_symbol;
	};

	json_t typeid_to_json(const typeid_t& t);



	//////////////////////////////////////		member_t

	/*
		Definition of a struct-member.
	*/

	struct member_t {
		public: member_t(const typeid_t& type, const std::string& name);
		public: member_t(const typeid_t& type, const std::shared_ptr<value_t>& value, const std::string& name);
		bool operator==(const member_t& other) const;
		public: bool check_invariant() const;


		/////////////////////////////		STATE
		public: typeid_t _type;

		//	Optional -- must have same type as _type.
		public: std::shared_ptr<const value_t> _value;

		public: std::string _name;
	};

	void trace(const member_t& member);


	void trace(const std::vector<std::shared_ptr<statement_t>>& e);

	std::vector<typeid_t> get_member_types(const std::vector<member_t>& m);
	json_t members_to_json(const std::vector<member_t>& members);



	//////////////////////////////////////		vector_def_t


	/*
		Notice that vector has no scope of its own.
	*/
	struct vector_def_t {
		public: static vector_def_t make2(const typeid_t& element_type);

		public: vector_def_t(){};
		public: bool check_invariant() const;
		public: bool operator==(const vector_def_t& other) const;


		/////////////////////////////		STATE
		public: typeid_t _element_type;
	};

	void trace(const vector_def_t& e);
	json_t vector_def_to_json(const vector_def_t& s);



	typedef floyd_ast::value_t (*HOST_FUNCTION)(const std::vector<value_t>& args);



	//////////////////////////////////////////////////		lexical_scope2_t


	struct lexical_scope2_t {
		/////////////////////////////		STATE
		public: std::vector<member_t> _state;
		public: std::vector<std::shared_ptr<statement_t> > _statements;
		public: std::map<int, std::shared_ptr<const lexical_scope_t> > _objects;
	};



	//////////////////////////////////////////////////		lexical_scope_t

	/*
		This is a core internal building block of the AST. It represents a static, compile-time scope.
		lexical_scope_t:s are used to define

		- The global scope
		- struct definition, with member data and functions
		- function definition, with arguments
		- block = function sub-scope - {}, for(){}, while{}, if(){}, else{}.

		The lexical_scope_t includes optional code, optional member variables and optional local types.
	*/
	struct lexical_scope_t {
		public: enum class etype {
			k_function_scope,
			k_struct_scope,
			k_global_scope,
			k_block,
			k_host_function_scope
		};

		public: static std::shared_ptr<const lexical_scope_t> make_struct_object(const std::vector<member_t>& members);

		public: static std::shared_ptr<const lexical_scope_t> make_function_object(
			const std::vector<member_t>& args,
			const std::vector<member_t>& locals,
			const std::vector<std::shared_ptr<statement_t> >& statements,
			const typeid_t& return_type,
			const std::map<int, std::shared_ptr<const lexical_scope_t> > objects
		);


		public: static std::shared_ptr<const lexical_scope_t> make_host_function_object(
			const std::vector<member_t>& args,
			const typeid_t& return_type,
			HOST_FUNCTION host_function
		);

		public: static std::shared_ptr<const lexical_scope_t> make_block_object(
			const std::vector<member_t>& locals,
			const std::vector<std::shared_ptr<statement_t> >& statements,
			const std::map<int, std::shared_ptr<const lexical_scope_t> > objects
		);

		public: static std::shared_ptr<const lexical_scope_t> make_global_scope(
			const std::vector<std::shared_ptr<statement_t> >& statements,
			const std::vector<member_t>& globals,
			const std::map<int, std::shared_ptr<const lexical_scope_t> > objects
		);

		public: bool check_invariant() const;
		public: bool shallow_check_invariant() const;

		public: bool operator==(const lexical_scope_t& other) const;


		public: const std::map<int, std::shared_ptr<const lexical_scope_t> >& get_objects() const {
			return _objects;
		}


		/////////////////////////////		STATE
		public: etype _type;
		public: std::vector<member_t> _args;
		public: std::vector<member_t> _state;
		public: std::vector<std::shared_ptr<statement_t> > _statements;
		public: typeid_t _return_type;

		public: std::map<int, std::shared_ptr<const lexical_scope_t> > _objects;
		public: HOST_FUNCTION _host_function;
	};

	json_t lexical_scope_to_json(const lexical_scope_t& v);
	void trace(const std::shared_ptr<const lexical_scope_t>& e);



	//////////////////////////////////////////////////		function_object_t


	struct function_object_t {
		public: static std::shared_ptr<const function_object_t> make_function_object(
			const std::vector<member_t>& args,
			const std::vector<member_t>& locals,
			const std::vector<std::shared_ptr<statement_t> >& statements,
			const typeid_t& return_type,
			const std::map<int, std::shared_ptr<const function_object_t> > objects
		);

		public: static std::shared_ptr<const function_object_t> make_host_function_object(
			const std::vector<member_t>& args,
			const typeid_t& return_type,
			HOST_FUNCTION host_function
		);

		public: bool check_invariant() const;
		public: bool shallow_check_invariant() const;

		public: bool operator==(const function_object_t& other) const;


		public: const std::map<int, std::shared_ptr<const function_object_t> >& get_objects() const {
			return _objects;
		}

		/////////////////////////////		STATE
		public: std::vector<member_t> _args;
		public: std::vector<member_t> _state;
		public: std::vector<std::shared_ptr<statement_t> > _statements;
		public: typeid_t _return_type;

		public: std::map<int, std::shared_ptr<const function_object_t> > _objects;
		public: HOST_FUNCTION _host_function;
	};

	json_t function_object_to_json(const function_object_t& v);
	void trace(const std::shared_ptr<const function_object_t>& e);



	//////////////////////////////////////////////////		ast_t


	/*
		Represents the root of the parse tree - the Abstract Syntax Tree
		Immutable
	*/
	struct ast_t {
		public: ast_t();
		public: explicit ast_t(std::shared_ptr<const lexical_scope_t> global_scope);
		public: bool check_invariant() const;

		public: const std::shared_ptr<const lexical_scope_t>& get_global_scope() const {
			return _global_scope;
		}


		/////////////////////////////		STATE
		private: std::shared_ptr<const lexical_scope_t> _global_scope;
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


	struct function_reg_t {
		typeid_t _function_type;
		std::shared_ptr<const lexical_scope_t> _function_obj;

		int _function_id;

		std::shared_ptr<floyd_ast::value_t> _function_value;
	};

	function_reg_t make_host_function_reg(const typeid_t& return_type, const std::vector<member_t>& args, HOST_FUNCTION host_function, int id);

}	//	floyd_ast

#endif /* parser_ast_hpp */
