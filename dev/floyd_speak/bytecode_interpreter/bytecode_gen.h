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
#include "ast_typeid.h"
#include "ast_value.h"
#include "pass3.h"
#include "floyd_interpreter.h"


namespace floyd {
	struct semantic_ast_t;



	//	Replace by int when we have flattened local bodies.
	typedef variable_address_t reg_t;


	//////////////////////////////////////		bcgen_instruction_t


	struct bcgen_instruction_t {
		bcgen_instruction_t(
			bc_opcode opcode,
			variable_address_t regA,
			variable_address_t regB,
			variable_address_t regC
		) :
			_opcode(opcode),
			_reg_a(regA),
			_reg_b(regB),
			_reg_c(regC)
		{
			QUARK_ASSERT(check_invariant());
		}

#if DEBUG
		public: bool check_invariant() const;
#endif

		//////////////////////////////////////		STATE
		bc_opcode _opcode;
		variable_address_t _reg_a;
		variable_address_t _reg_b;
		variable_address_t _reg_c;
	};


	//////////////////////////////////////		bcgen_body_t


	struct bcgen_body_t {
		bcgen_body_t(const std::vector<bcgen_instruction_t>& s) :
			_instrs(s),
			_symbols{}
		{
			QUARK_ASSERT(check_invariant());
		}

		bcgen_body_t(const std::vector<bcgen_instruction_t>& instructions, const std::vector<std::pair<std::string, symbol_t>>& symbols) :
			_instrs(instructions),
			_symbols(symbols)
		{
			QUARK_ASSERT(check_invariant());
		}

		public: bool check_invariant() const;


		//////////////////////////////////////		STATE

		std::vector<std::pair<std::string, symbol_t>> _symbols;
		std::vector<bcgen_instruction_t> _instrs;
	};

	bc_frame_t make_frame(const bcgen_body_t& body, const std::vector<typeid_t>& args);


	//////////////////////////////////////		bcgen_context_t


	struct bcgen_context_t {
		public: quark::trace_context_t _tracer;
	};


	//////////////////////////////////////		bcgen_environment_t

	/*
		Runtime scope, similar to a stack frame.
	*/

	struct bcgen_environment_t {
		public: const bcgen_body_t* _body_ptr;
	};


	//////////////////////////////////////		bgen_imm_t


	struct bgen_imm_t {
		////////////////////////		STATE
		public: const semantic_ast_t _ast;
	};


	//////////////////////////////////////		bcgenerator_t

	/*
		Complete runtime state of the bgenerator.
		MUTABLE
	*/

	struct bcgenerator_t {
		public: explicit bcgenerator_t(const semantic_ast_t& ast);
		public: bcgenerator_t(const bcgenerator_t& other);
		public: const bcgenerator_t& operator=(const bcgenerator_t& other);
#if DEBUG
		public: bool check_invariant() const;
#endif
		public: void swap(bcgenerator_t& other) throw();

		////////////////////////		STATE
		public: std::shared_ptr<bgen_imm_t> _imm;

		//	Holds all values for all environments.
		public: std::vector<bcgen_environment_t> _call_stack;

		public: std::vector<const typeid_t> _types;
	};


	//////////////////////////		generate_bytecode()


	bc_program_t generate_bytecode(const quark::trace_context_t& tracer, const semantic_ast_t& ast);


} //	floyd


#endif /* bytecode_gen_h */
