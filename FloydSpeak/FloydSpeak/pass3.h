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
}


namespace floyd_pass3 {
	struct analyser_t;

	floyd::value_t unflatten_json_to_specific_type(const json_t& v);


	//////////////////////////////////////		statement_result_t



	struct interpreter_context_t {
		public: quark::trace_context_t _tracer;
	};


	//////////////////////////////////////		statement_result_t


	struct statement_result_t {
		enum output_type {

			//	_output != nullptr
			k_return_unwind,

			//	_output != nullptr
			k_passive_expression_output,


			//	_output == nullptr
			k_none
		};

		public: static statement_result_t make_return_unwind(const floyd::value_t& return_value){
			return { statement_result_t::k_return_unwind, return_value };
		}
		public: static statement_result_t make_passive_expression_output(const floyd::value_t& output_value){
			return { statement_result_t::k_passive_expression_output, output_value };
		}
		public: static statement_result_t make_no_output(){
			return { statement_result_t::k_passive_expression_output, floyd::value_t::make_null() };
		}

		private: statement_result_t(output_type type, const floyd::value_t& output) :
			_type(type),
			_output(output)
		{
		}

		public: output_type _type;
		public: floyd::value_t _output;
	};

	inline bool operator==(const statement_result_t& lhs, const statement_result_t& rhs){
		return true
			&& lhs._type == rhs._type
			&& lhs._output == rhs._output;
	}


	//////////////////////////////////////		environment_t

	/*
		Runtime scope, similar to a stack frame.
	*/

	struct symbol_t {
		enum type {
			immutable_local = 10,
			mutable_local
//			,
//			function_definition,
//			struct_definition
		};

		type _symbol_type;
		floyd::typeid_t _value_type;
		floyd::value_t _default_value;

		//	Requires to use symbol_t as value in std::map() and use its [].
		public: symbol_t() :
			_value_type(floyd::typeid_t::make_unresolved_type_identifier("xyz"))
		{
		}


		public: symbol_t(type symbol_type, const floyd::typeid_t& value_type, const floyd::value_t& default_value) :
			_symbol_type(symbol_type),
			_value_type(value_type),
			_default_value(default_value)
		{
		}


		public: floyd::typeid_t get_type() const {
			return _value_type;
		}

		public: static symbol_t make_immutable_local(const floyd::typeid_t value_type)
		{
			return symbol_t{ type::immutable_local, value_type, {} };
		}

		public: static symbol_t make_mutable_local(const floyd::typeid_t value_type)
		{
			return symbol_t{ type::mutable_local, value_type, {} };
		}

		public: static symbol_t make_constant(const floyd::value_t& value)
		{
			return symbol_t{ type::immutable_local, value.get_type(), value };
		}

/*
		public: static symbol_t make_struct_def(const floyd::typeid_t& struct_typeid)
		{
			return symbol_t{ type::struct_definition, struct_typeid, {} };
		}
*/

	};


	struct environment_t {
		public: std::shared_ptr<environment_t> _parent_env;
		public: std::map<std::string, symbol_t> _symbols;


		public: bool check_invariant() const;

		public: static std::shared_ptr<environment_t> make_environment(
			const analyser_t& vm,
			std::shared_ptr<environment_t>& parent_env
		);
	};


	//////////////////////////////////////		analyser_t

	/*
		Complete runtime state of the interpreter.
		MUTABLE
	*/

	struct analyser_t {
		public: analyser_t(const floyd::ast_t& ast);
		public: analyser_t(const analyser_t& other);
		public: const analyser_t& operator=(const analyser_t& other);
#if DEBUG
		public: bool check_invariant() const;
#endif

		////////////////////////		STATE
		public: std::chrono::time_point<std::chrono::high_resolution_clock> _start_time;


		//	Constant!
		public: std::shared_ptr<const floyd::ast_t> _ast;


		//	Non-constant. Last scope is the current one. First scope is the root.
		public: std::vector<std::shared_ptr<environment_t>> _call_stack;

		public: std::vector<std::string> _print_output;
	};


	std::pair<analyser_t, statement_result_t> call_host_function(const analyser_t& vm, int function_id, const std::vector<floyd::value_t> args);

	json_t analyser_to_json(const analyser_t& vm);


	/*
		analyses an expression as far as possible.
		return == _constant != nullptr:	the expression was completely analysed and resulted in a constant value.
		return == _constant == nullptr: the expression was partially analyse.
	*/
	std::pair<analyser_t, floyd::expression_t> analyse_expression(const analyser_t& vm, const floyd::expression_t& e);

	std::pair<analyser_t, statement_result_t> call_function(
		const analyser_t& vm,
		const floyd::value_t& f,
		const std::vector<floyd::value_t>& args
	);


	/*
		Return value:
			null = statements were all executed through.
			value = return statement returned a value.
	*/
	std::pair<analyser_t, statement_result_t> analyse_statements(const analyser_t& vm, const std::vector<std::shared_ptr<floyd::statement_t>>& statements);



	symbol_t find_global_symbol(const analyser_t& vm, const std::string& s);

	floyd::typeid_t resolve_type_using_env(const analyser_t& vm, const floyd::typeid_t& type);




	/*
		Semantic Analysis -> SYMBOL TABLE + annotated AST
	*/
	floyd::ast_t run_pass3(const quark::trace_context_t& tracer, const floyd::ast_t& ast_pass2);
}
#endif /* pass3_hpp */


