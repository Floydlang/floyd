//
//  floyd_parser.h
//  FloydSpeak
//
//  Created by Marcus Zetterquist on 10/04/16.
//  Copyright © 2016 Marcus Zetterquist. All rights reserved.
//

#ifndef floyd_parser_h
#define floyd_parser_h

#include "quark.h"

#include "parser_primitives.h"

namespace floyd_parser {
	struct expression_t;
	struct statement_t;
	struct function_def_expr_t;
	struct value_t;


	//////////////////////////////////////////////////		identifiers_t



	struct identifiers_t {
		identifiers_t(){
		}

		public: bool check_invariant() const {
			return true;
		}

		//### Function names should have namespace etc.
		std::map<std::string, std::shared_ptr<const function_def_expr_t> > _functions;

		std::map<std::string, std::shared_ptr<const value_t> > _constant_values;
	};


	//////////////////////////////////////////////////		ast_t


	struct ast_t : public parser_i {
		public: ast_t(){
		}

		public: bool check_invariant() const {
			return true;
		}

		/////////////////////////////		parser_i

		public: virtual bool parser_i_is_declared_function(const std::string& s) const;
		public: virtual bool parser_i_is_declared_constant_value(const std::string& s) const;


		/////////////////////////////		STATE
		public: frontend_types_collector_t _types_collector;
		public: identifiers_t _identifiers;
		public: std::vector<statement_t> _top_level_statements;
	};


	ast_t program_to_ast(const identifiers_t& builtins, const std::string& program);

	/*
		Evaluates an expression as far as possible.
	*/
	expression_t evaluate3(const ast_t& ast, const expression_t& e);


	value_t run_function(const ast_t& ast, const function_def_expr_t& f, const std::vector<value_t>& args);

}	//	floyd_parser



#endif /* floyd_parser_h */
