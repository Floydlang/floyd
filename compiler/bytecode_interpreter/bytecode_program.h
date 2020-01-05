//
//  bytecode_program.hpp
//  floyd
//
//  Created by Marcus Zetterquist on 2020-01-05.
//  Copyright © 2020 Marcus Zetterquist. All rights reserved.
//

#ifndef bytecode_program_hpp
#define bytecode_program_hpp

#include "types.h"
#include "value_backend.h"
#include "bytecode_isa.h"

namespace floyd {

//////////////////////////////////////		bc_symbol_t

/*
	Tracks information about a symbol inside a stack frame.
	??? Rename "stack_frame_entry_t"
*/
/*
struct bc_symbol_t {
	enum class type {
		immutable = 10,
		mutable1
	};

	public: bool check_invariant() const {
		QUARK_ASSERT(_init.is_undefined() || _init.get_type() == _value_type);
		return true;
	}


	//////////////////////////////////////		STATE
	type _symbol_type;
	type_t _value_type;
	value_t _init;
};
*/



//////////////////////////////////////		bc_static_frame_t

/*
	Holds information for a single stack frame.
	- The registers it needs
	- What types those registers are.
	- The instruction
*/
struct bc_static_frame_t {
	bc_static_frame_t(
		const types_t& types,
		const std::vector<bc_instruction_t>& instrs2,
		const symbol_table_t& symbols,
		const std::vector<type_t>& args
	);
	bool check_invariant() const;


	//////////////////////////////////////		STATE
	std::vector<bc_instruction_t> _instructions;

	//	Symbols spans all args and locals: first args, then locals.
	symbol_table_t _symbols;
	std::vector<type_t> _symbol_effective_type;

	int _arg_count;
	int _locals_count;
};


//////////////////////////////////////		bc_function_definition_t

/*
	Represents a function, ready to execute.
	The interpreter doesn't use a flat list of instructions for programs, rather a list of functions.
*/
struct bc_function_definition_t {
	public: bool check_invariant() const;


	//////////////////////////////////////		STATE
	func_link_t func_link;

	//	 null if this is a native function (rather than a BC function).
	std::shared_ptr<bc_static_frame_t> _frame_ptr;
};


//////////////////////////////////////		bc_program_t

/*
	A complete, stand-alone, Floyd byte code executable, ready to be executed by interpreter.
*/

struct bc_program_t {
	public: bool check_invariant() const {
		QUARK_ASSERT(_globals.check_invariant());
		for(const auto& e: _function_defs){
			QUARK_ASSERT(e.check_invariant());
		}
		QUARK_ASSERT(_types.check_invariant());
		return true;
	}


	//////////////////////////////////////		STATE
	public: const bc_static_frame_t _globals;
	public: std::vector<bc_function_definition_t> _function_defs;
	public: types_t _types;
	public: software_system_t _software_system;
	public: container_t _container_def;
};



json_t bcprogram_to_json(const bc_program_t& program);


std::vector<std::pair<type_t, struct_layout_t>> bc_make_struct_layouts(const types_t& types);
std::vector<func_link_t> link_functions(const bc_program_t& program);


} // floyd

#endif /* bytecode_program_hpp */
