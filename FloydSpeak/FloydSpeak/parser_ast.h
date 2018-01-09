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



	//////////////////////////////////////		member_t

	/*
		Definition of a struct-member.
	*/

	struct member_t {
		public: member_t(const floyd_basics::typeid_t& type, const std::string& name);
		public: member_t(const floyd_basics::typeid_t& type, const std::shared_ptr<value_t>& value, const std::string& name);
		bool operator==(const member_t& other) const;
		public: bool check_invariant() const;


		/////////////////////////////		STATE
		public: floyd_basics::typeid_t _type;

		//	Optional -- must have same type as _type.
		public: std::shared_ptr<const value_t> _value;

		public: std::string _name;
	};

	void trace(const member_t& member);


	void trace(const std::vector<std::shared_ptr<statement_t>>& e);

	std::vector<floyd_basics::typeid_t> get_member_types(const std::vector<member_t>& m);
	json_t members_to_json(const std::vector<member_t>& members);



	//////////////////////////////////////		vector_def_t


	/*
		Notice that vector has no scope of its own.
	*/
	struct vector_def_t {
		public: static vector_def_t make2(const floyd_basics::typeid_t& element_type);

		public: vector_def_t(){};
		public: bool check_invariant() const;
		public: bool operator==(const vector_def_t& other) const;


		/////////////////////////////		STATE
		public: floyd_basics::typeid_t _element_type;
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
