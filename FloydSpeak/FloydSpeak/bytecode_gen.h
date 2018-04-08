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
#include "floyd_interpreter.h"


namespace floyd {
	struct bgenerator_t;
	struct bc_instruction_t;

	struct semantic_ast_t;


	enum {
		k_no_bctypeid = -3
	};


	inline int bc_limit(int value, int min, int max){
		if(value < min){
			return min;
		}
		else if(value > max){
			return max;
		}
		else{
			return value;
		}
	}




	//////////////////////////////////////		bcgen_context_t


	struct bcgen_context_t {
		public: quark::trace_context_t _tracer;
	};


	//////////////////////////////////////		bcgen_environment_t

	/*
		Runtime scope, similar to a stack frame.
	*/

	struct bcgen_environment_t {
		public: const bc_body_t* _body_ptr;
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

		public: std::vector<const typeid_t> _types;
	};


	//////////////////////////		run_bggen()


	bc_program_t run_bggen(const quark::trace_context_t& tracer, const semantic_ast_t& pass3);


} //	floyd


#endif /* bytecode_gen_h */
