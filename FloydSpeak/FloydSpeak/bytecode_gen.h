//
//  parser_evaluator.h
//  FloydSpeak
//
//  Created by Marcus Zetterquist on 26/07/16.
//  Copyright © 2016 Marcus Zetterquist. All rights reserved.
//

#ifndef bytecode_gen_h
#define bytecode_gen_h

#include "quark.h"

#include <string>
#include <vector>
#include <map>
#include "ast.h"
#include "ast_value.h"
#include "host_functions.hpp"

namespace floyd {
	struct expression_t;
	struct value_t;
	struct statement_t;
	struct bgenerator_t;



	//////////////////////////////////////		bc_vm_t

	typedef uint32_t bc_instruction_t;

	/*
		----------------------------------- -----------------------------------
		66665555 55555544 44444444 33333333 33222222 22221111 11111100 00000000
		32109876 54321098 76543210 98765432 10987654 32109876 54321098 76543210

		XXXXXXXX XXXXXXXX PPPPPPPP PPPPPPPP PPPPPPPP PPPPPPPP PPPPPPPP PPPPPppp
		48bit Intel x86_64 pointer. ppp = low bits, set to 0, X = bit 47

		-----------------------------------
		33222222 22221111 11111100 00000000
		10987654 32109876 54321098 76543210

		INSTRUCTION
		CCCCCCCC AAAAAAAA BBBBBBBB CCCCCCCC

		A = destination register.
		B = lhs register
		C = rhs register

		-----------------------------------------------------------------------
	*/

	enum bc_instr {
		//	Store a into local/global variable a
		k_statement_store,

		//	Needed?
		k_statement_block,

		k_statement_return,
		k_statement_if,

		//	replace by k_statement_if.
		k_statement_for,

		//	replace by k_statement_if.
		k_statement_while,

		//	Not needed. Just use an expression and don't use its result.
		k_statement_expression,

		k_expression_literal,
		k_expression_resolve_member,
		k_expression_lookup_element,
		k_expression_call,
		k_expression_construct_value,
		k_expression_arithmetic_unary_minus,

		//	replace by k_statement_if.
		k_expression_conditional_operator3,

		k_expression_comparison_smaller_or_equal,
		k_expression_comparison_smaller,
		k_expression_comparison_larger_or_equal,
		k_expression_comparison_larger,

		k_expression_logical_equal,
		k_expression_logical_nonequal,

		k_expression_arithmetic_add,
		k_expression_arithmetic_subtract,
		k_expression_arithmetic_multiply,
		k_expression_arithmetic_divide,
		k_expression_arithmetic_remainder,

		k_expression_logical_and,
		k_expression_logical_or
	};

	//	A memory block. Addressed using index. Always 1 cache line big.
	//	Prefetcher likes bigger blocks than this.
	struct bc_node_t {
		uint32_t _words[64 / sizeof(uint32_t)];
	};

	struct bc_instr_t {
		bc_instr _opcode;
		std::shared_ptr<const statement_t> _statement;
	};
	struct bc_program_t {
		public: bool check_invariant() const {
			QUARK_ASSERT(_bcgen_ast.check_invariant());
			return true;
		}

//		const std::vector<const bc_instruction_t> _instructions;
		floyd::ast_t _bcgen_ast;
	};


	//////////////////////////////////////		bgen_statement_result_t


	struct bcgen_context_t {
		public: quark::trace_context_t _tracer;
	};


	//////////////////////////////////////		bgen_statement_result_t


	struct bgen_statement_result_t {
		enum output_type {

			//	_output != nullptr
			k_return_unwind,

			//	_output != nullptr
			k_passive_expression_output,


			//	_output == nullptr
			k_none
		};

		public: static bgen_statement_result_t make_return_unwind(const value_t& return_value){
			return { bgen_statement_result_t::k_return_unwind, return_value };
		}
		public: static bgen_statement_result_t make_passive_expression_output(const value_t& output_value){
			return { bgen_statement_result_t::k_passive_expression_output, output_value };
		}
		public: static bgen_statement_result_t make_no_output(){
			return { bgen_statement_result_t::k_passive_expression_output, value_t::make_null() };
		}

		private: bgen_statement_result_t(output_type type, const value_t& output) :
			_type(type),
			_output(output)
		{
		}

		public: output_type _type;
		public: value_t _output;
	};

	inline bool operator==(const bgen_statement_result_t& lhs, const bgen_statement_result_t& rhs){
		return true
			&& lhs._type == rhs._type
			&& lhs._output == rhs._output;
	}


	//////////////////////////////////////		bcgen_environment_t

	/*
		Runtime scope, similar to a stack frame.
	*/

	struct bcgen_environment_t {
		public: const body_t* _body_ptr;
	};


	//////////////////////////////////////		bgenerator_imm_t


	struct bgenerator_imm_t {
		////////////////////////		STATE
		public: const ast_t _ast_pass3;
	};


	//////////////////////////////////////		bgenerator_t

	/*
		Complete runtime state of the bgenerator.
		MUTABLE
	*/

	struct bgenerator_t {
		public: explicit bgenerator_t(const ast_t& pass3);
		public: bgenerator_t(const bgenerator_t& other);
		public: const bgenerator_t& operator=(const bgenerator_t& other);
#if DEBUG
		public: bool check_invariant() const;
#endif
		public: void swap(bgenerator_t& other) throw();

		////////////////////////		STATE
		public: std::shared_ptr<bgenerator_imm_t> _imm;

		//	Holds all values for all environments.
		public: std::vector<bcgen_environment_t> _call_stack;
		public: std::vector<std::string> _print_output;
	};


	//////////////////////////		run_bggen()




	bc_program_t run_bggen(const quark::trace_context_t& tracer, const floyd::ast_t& pass3);

} //	floyd


#endif /* bytecode_gen_h */
