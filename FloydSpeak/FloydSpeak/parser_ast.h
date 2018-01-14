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


namespace floyd {
	struct statement_t;


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

}	//	floyd

#endif /* parser_ast_hpp */
