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
#include "ast.h"

namespace floyd {
	struct expression_t;
	struct value_t;
	struct statement_t;
	struct bgenerator_t;

	struct bc_body_t;


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
		k_nop,

		//	Store _e[0] -> _v
		k_statement_store,

		//	Needed?
		//	execute body_x
		k_statement_block,

		//	_e[0]
		k_statement_return,

		//	if _e[0] execute _body_x, else execute _body_y
		k_statement_if,

		k_statement_for,

		k_statement_while,

		//	Not needed. Just use an expression and don't use its result.
		k_statement_expression,

/*
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
*/

	};

	//	A memory block. Addressed using index. Always 1 cache line big.
	//	Prefetcher likes bigger blocks than this.
	struct bc_node_t {
		uint32_t _words[64 / sizeof(uint32_t)];
	};

	//??? Start by only using k_statement_store and k_fallback_to_checking_statement_manually.
	struct bc_instr_t {
		bc_instr _opcode;

		std::vector<expression_t> _e;
		variable_address_t _v;
		std::string _name;
		std::shared_ptr<const bc_body_t> _body_x;
		std::shared_ptr<const bc_body_t> _body_y;


		public: bool check_invariant() const {
			return true;
		}
	};



	struct bc_body_t {
		const std::vector<std::pair<std::string, symbol_t>> _symbols;

		std::vector<bc_instr_t> _statements;

		bc_body_t(const std::vector<bc_instr_t>& s) :
			_statements(s),
			_symbols{}
		{
		}
		bc_body_t(const std::vector<bc_instr_t>& statements, const std::vector<std::pair<std::string, symbol_t>>& symbols) :
			_statements(statements),
			_symbols(symbols)
		{
		}
	};

	struct bc_function_definition_t {
		public: bool check_invariant() const {
			if(_host_function_id != 0){
				QUARK_ASSERT(!_body);
			}
			else{
				QUARK_ASSERT(_body);
			}
			return true;
		}


		typeid_t _function_type;
		std::vector<member_t> _args;
		std::shared_ptr<bc_body_t> _body;
		int _host_function_id;
	};

	struct bc_program_t {
		public: bool check_invariant() const {
//			QUARK_ASSERT(_globals.check_invariant());
			return true;
		}

		public: const bc_body_t _globals;
		public: std::vector<std::shared_ptr<const bc_function_definition_t>> _function_defs;
	};


	//////////////////////////////////////		bcgen_context_t


	struct bcgen_context_t {
		public: quark::trace_context_t _tracer;
	};


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
		public: std::vector<std::shared_ptr<const bc_function_definition_t>> _function_defs;
	};


	//////////////////////////		run_bggen()


	bc_program_t run_bggen(const quark::trace_context_t& tracer, const ast_t& pass3);

	json_t bcprogram_to_json(const bc_program_t& program);

} //	floyd


#endif /* bytecode_gen_h */
