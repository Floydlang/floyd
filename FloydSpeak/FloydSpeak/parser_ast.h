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
			"int"
			"coord_t"
			"coord_t/8000"
			"int (float a, float b)"
			"[string]"
			"[string([bool(float,string),pixel)])"
			"[coord_t/8000]"
			"pixel_coord_t = coord_t/8000"
		*/
		public: floyd_basics::base_type _base_type;
		public: std::vector<typeid_t> _parts;
		public: std::string _struct_def_id;

		//	This is used it overrides _base_type (which will be null).
		public: std::string _unresolved_type_symbol;
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





	//////////////////////////////////////////////////		ast_t


	/*
		Represents the root of the parse tree - the Abstract Syntax Tree
		Immutable
	*/
	struct ast_t {
		public: ast_t();
		public: explicit ast_t(const std::vector<std::shared_ptr<statement_t> > statements);
		public: bool check_invariant() const;


		/////////////////////////////		STATE
		std::vector<std::shared_ptr<statement_t> > _statements;
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

}	//	floyd_ast

#endif /* parser_ast_hpp */
