//
//  pass2.hpp
//  FloydSpeak
//
//  Created by Marcus Zetterquist on 09/08/16.
//  Copyright © 2016 Marcus Zetterquist. All rights reserved.
//

#ifndef pass3_hpp
#define pass3_hpp


#include "quark.h"

#include <string>
#include <vector>
#include <map>
#include "ast.h"
#include "ast_value.h"


namespace floyd {
	struct expression_t;
	struct value_t;
	struct statement_t;
	struct host_function_signature_t;
}


namespace floyd {
	struct analyser_t;

	floyd::value_t unflatten_json_to_specific_type(const json_t& v);
	bool check_types_resolved(const floyd::ast_t& ast);


	//////////////////////////////////////		lexical_scope_t


	struct lexical_scope_t {
		public: std::vector<std::pair<std::string, floyd::symbol_t>> _symbols;
	};


	//////////////////////////////////////		semantic_ast_t


	/*
		This is an ast where:
		- All language-level syntactical transforms (replacying operations with simpler ones).
		- All language-level syntax checks passed.
		- All symbols have been resolved.
		- All built-in types and host functions are inserted.

		An instance of semantic_ast_t is guaranteed to be resolved.
	*/
	struct semantic_ast_t {
		semantic_ast_t(const floyd::ast_t& checked_ast){
			QUARK_ASSERT(checked_ast.check_invariant());
			QUARK_ASSERT(check_types_resolved(checked_ast));

			_checked_ast = checked_ast;
		}
#if DEBUG
		public: bool check_invariant() const{
			QUARK_ASSERT(_checked_ast.check_invariant());
			QUARK_ASSERT(check_types_resolved(_checked_ast));
			return true;
		}
#endif

		public: floyd::ast_t _checked_ast;
	};


	//////////////////////////////////////		analyser_t

	/*
		Complete runtime state of the interpreter.
		MUTABLE
	*/

	struct analyzer_imm_t {
		//??? Remove _ast so we don't confuse it with real statements.
		public: floyd::ast_t _ast;

		public: std::map<std::string, floyd::host_function_signature_t> _host_functions;
	};

	struct analyser_t {
		public: analyser_t(const floyd::ast_t& ast);
		public: analyser_t(const analyser_t& other);
		public: const analyser_t& operator=(const analyser_t& other);
#if DEBUG
		public: bool check_invariant() const;
#endif
		public: void swap(analyser_t& other) throw();


		////////////////////////		STATE

		public: std::shared_ptr<const analyzer_imm_t> _imm;


		//	Non-constant. Last scope is the current one. First scope is the root.
		public: std::vector<std::shared_ptr<lexical_scope_t>> _call_stack;

		public: std::vector<std::shared_ptr<const floyd::function_definition_t>> _function_defs;
	};

	json_t analyser_to_json(const analyser_t& vm);


	/*
		analyses an expression as far as possible.
		return == _constant != nullptr:	the expression was completely analysed and resulted in a constant value.
		return == _constant == nullptr: the expression was partially analyse.
	*/
	std::pair<analyser_t, floyd::expression_t> analyse_expression_to_target(const analyser_t& vm, const floyd::expression_t& e, const floyd::typeid_t& target_type);
	std::pair<analyser_t, floyd::expression_t> analyse_expression_no_target(const analyser_t& vm, const floyd::expression_t& e);


	/*
		Return value:
			null = statements were all executed through.
			value = return statement returned a value.
	*/
	std::pair<analyser_t, std::vector<std::shared_ptr<floyd::statement_t>> > analyse_statements(const analyser_t& vm, const std::vector<std::shared_ptr<floyd::statement_t>>& statements);

	floyd::symbol_t find_global_symbol(const analyser_t& vm, const std::string& s);

	/*
		Semantic Analysis -> SYMBOL TABLE + annotated AST
	*/
	semantic_ast_t run_pass3(const quark::trace_context_t& tracer, const floyd::ast_t& ast_pass2);
}
#endif /* pass3_hpp */


