//
//  parser_evaluator.h
//  Floyd
//
//  Created by Marcus Zetterquist on 26/07/16.
//  Copyright © 2016 Marcus Zetterquist. All rights reserved.
//

#ifndef bytecode_interpreter_hpp
#define bytecode_interpreter_hpp

/*
	The Floyd byte code interpreter, used to run Floyd byte code programs.

	inplace: data is stored directly inside the value.
	external: data is allocated externally and value points to it.

	??? Less code + faster to generate increc, decref instructions instead of make *_external_value, *_internal_value opcodes.
*/

#include "types.h"
#include "value_backend.h"
#include "floyd_runtime.h"
#include "bytecode_stack.h"
#include "bytecode_isa.h"
#include "bytecode_program.h"
#include "quark.h"

#include <string>
#include <vector>

/*
	Global frame has no args.
0:	stack + 0	prev frame (stack pos)	always 0x0000000000000000
	stack + 1	prev static frame ptr	always 0x0000000000000000
	stack + 2	local 0		symbol 0
	stack + 3	local 1		symbol 1
	stack + 4	local 2		symbol 2
	stack + 5	local 3		symbol 3
	stack + 6	local 4		symbol 4
	stack + 7	local 5		symbol 5
	stack + 8	local 6		symbol 6
	stack + 9	runtime temporary
	stack + 10	runtime temporary

 Runtime temporaries are pushed / popped while executing and not part of symbols.


	Call to function x:


1:	stack + 11	prev frame = 0 (stack pos)
	stack + 12	prev static frame ptr
	stack + 13	arg 0		symbol 0
	stack + 14	arg 1		symbol 1
	stack + 15	local 0		symbol 2
	stack + 16	local 1		symbol 3
	stack + 17	runtime temporary


	Call to function Y:


2:	stack + 18	prev frame = 11 (stack pos)
	stack + 19	prev static frame ptr
	stack + 20	arg 0		symbol 0
	stack + 21	arg 1		symbol 1
	stack + 22	local 0		symbol 2
	stack + 23	local 1		symbol 3
	stack + 24	runtime temporary


	Notice: symbol table maps to parameters AND locals.
	Notice: because of symbol table, the type & symbol infois always available for all stack entries.
	Notice: a frame starts where symbol 0 is.

	Registers are integers into current frame's symbols / stack. Register 0 is always symbol 0.
*/

namespace floyd {

struct type_t;
struct types_t;
struct runtime_handler_i;
struct runtime_process_i;
struct interpreter_t;
struct bc_program_t;

struct rt_value_t;


typedef rt_value_t (*BC_NATIVE_FUNCTION_PTR)(interpreter_t& vm, const rt_value_t args[], int arg_count);

//??? rt_type_t is 32 bits, we need to bump bc_typeid_t to match. This requires changes to opcode encoding
typedef int16_t bc_typeid_t;


//////////////////////////////////////		frame_pos_t


//	Keeps track where in the interpreter's stack we're executing code right now.
struct frame_pos_t {
	size_t _frame_pos;
	const bc_static_frame_t* _static_frame_ptr;
};

inline bool operator==(const frame_pos_t& lhs, const frame_pos_t& rhs){
	return lhs._frame_pos == rhs._frame_pos && lhs._static_frame_ptr == rhs._static_frame_ptr;
}


/*
	The interpreters's stack -- each element contains a rt_pod_t.
	For each stack entry we need to keep track of if it's an inplace or external value so we can
	keep the external value's reference counting OK.

	Each stack frame will be mapped to a range of the interpreter stack.
	The stack frame's registers are really mapped to entries in the stack.
*/
enum {
	//	We store prev-frame-pos & symbol-ptr.
	k_frame_overhead = 2
};


/*
1:	stack + 11	prev frame = 2 (stack pos)
	stack + 12	prev static frame ptr

	FUNCTION X FRAME START
	stack + 13	arg 0		symbol 0
	stack + 14	arg 1		symbol 1
	stack + 15	local 0		symbol 2
	stack + 16	local 1		symbol 3
	stack + 17	runtime temporary
*/

struct active_frame_t {
	active_frame_t(
		const std::string& info,
		size_t start_pos,
		size_t effective_size,
		bool symbols_flag,
		const bc_static_frame_t* static_frame,
		size_t runtime_temp_count
	) :
		info(info),
		start_pos(start_pos),
		effective_end(start_pos + effective_size),
		symbols_flag(symbols_flag),
		static_frame(static_frame),
		runtime_temp_count(runtime_temp_count)
	{
		QUARK_ASSERT(check_invariant());
	}

	bool check_invariant() const {
		QUARK_ASSERT(start_pos >= 0);
		QUARK_ASSERT(effective_end >= start_pos);
		QUARK_ASSERT(static_frame != nullptr);
		QUARK_ASSERT(runtime_temp_count >= 0 && runtime_temp_count < 100000);

		return true;
	}

	size_t get_symbol_start() const {
		QUARK_ASSERT(check_invariant());

		return start_pos + k_frame_overhead;
	}
	size_t get_symbol_end() const {
		QUARK_ASSERT(check_invariant());

		return symbols_flag
			? start_pos + k_frame_overhead + static_frame->_symbols._symbols.size()
			: get_symbol_start();
	}
	size_t get_end() const {
		QUARK_ASSERT(check_invariant());

		return effective_end;
	}

	//////////////////////////////////////		STATE

	std::string info;
	size_t start_pos;
	size_t effective_end;
	bool symbols_flag;
	size_t runtime_temp_count;

	const bc_static_frame_t* static_frame;
};


//////////////////////////////////////		interpreter_t

/*
	Complete runtime state of the interpreter.
	MUTABLE
	Non-copyable.
*/

struct interpreter_t : runtime_basics_i {
	public: explicit interpreter_t(
		const std::shared_ptr<bc_program_t>& program,
		value_backend_t& backend,
		const config_t& config,
		runtime_process_i* process_handler,
		runtime_handler_i& runtime_handler,
		const std::string& name
	);
	public: interpreter_t(const interpreter_t& other) = delete;
	public: ~interpreter_t();

	public: void unwind_stack();

	public: const interpreter_t& operator=(const interpreter_t& other)= delete;
	public: bool check_invariant() const;
	public: bool check_invariant_thread_safe() const;
	public: void swap(interpreter_t& other) throw();

	void runtime_basics__on_print(const std::string& s) override;
	type_t runtime_basics__get_global_symbol_type(const std::string& s) override;
	rt_value_t runtime_basics__call_thunk(const rt_value_t& f, const rt_value_t args[], int arg_count) override;


	//////////////////////////////////////		GLOBAL VARIABLES


	public: bool check_global_access_obj(const int global_index) const{
		QUARK_ASSERT(check_invariant());
		QUARK_ASSERT(global_index >= 0 && global_index < (k_frame_overhead + _program->_globals._symbols._symbols.size()));
		return true;
	}
	public: bool check_global_access_intern(const int global_index) const{
		QUARK_ASSERT(check_invariant());
		QUARK_ASSERT(global_index >= 0 && global_index < (k_frame_overhead + _program->_globals._symbols._symbols.size()));
		return true;
	}


	//////////////////////////////////////		FRAMES


	public: size_t get_current_frame_pos() const {
//		QUARK_ASSERT(check_invariant());

		return _current_frame._frame_pos;
	}
	public: rt_pod_t* get_current_frame_ptr() const {
//		QUARK_ASSERT(check_invariant());

		return &_stack._entries[_current_frame._frame_pos];
	}

	public: rt_pod_t* get_current_frame_regs() const {
//		QUARK_ASSERT(check_invariant());

		return &_stack._entries[_current_frame._frame_pos + k_frame_overhead];
	}

	public: frame_pos_t get_current_frame() const {
		return _current_frame;
	}

	//////////////////////////////////////		REGISTERS


	public: bool check_reg(int reg) const{
		//	Makes sure register is within current stack frame bounds.
		QUARK_ASSERT(reg >= 0 && reg < _current_frame._static_frame_ptr->_symbols._symbols.size());

		//	Makes sure debug types are in sync for this register.
		const auto& effective_type = _current_frame._static_frame_ptr->_symbol_effective_type[reg];
		const auto& debug_type = _stack._entry_types[get_current_frame_pos() + k_frame_overhead + reg];
		QUARK_ASSERT(peek2(_backend.types, effective_type) == peek2(_backend.types, debug_type) || debug_type.is_undefined());
		return true;
	}

	//	Slow since it looks up the type of the register at runtime.
	//??? Notice: this function should be const in the future!
	public: rt_value_t read_register(const int reg){
		QUARK_ASSERT(check_invariant());
		QUARK_ASSERT(check_reg(reg));

		const auto result = rt_value_t(
			_backend,
			get_current_frame_regs()[reg],
			_current_frame._static_frame_ptr->_symbol_effective_type[reg],
			rt_value_t::rc_mode::bump
		);
		QUARK_ASSERT(result.check_invariant());
		return result;
	}

	public: void write_register(const int reg, const rt_value_t& value){
		QUARK_ASSERT(check_invariant());
		QUARK_ASSERT(check_reg(reg));
		QUARK_ASSERT(value.check_invariant());
		QUARK_ASSERT(
			peek2(_backend.types, _current_frame._static_frame_ptr->_symbol_effective_type[reg]) == peek2(_backend.types, value._type)
		);

		const auto& frame_slot_type = _current_frame._static_frame_ptr->_symbol_effective_type[reg];

		const auto prev = get_current_frame_regs()[reg];
		release_value_safe(_backend, prev, frame_slot_type);
		retain_value(_backend, value._pod, frame_slot_type);
		get_current_frame_regs()[reg] = value._pod;

		QUARK_ASSERT(check_invariant());
	}

	public: void write_register2(int dest_reg, const type_t& dest_type, rt_pod_t value){
		QUARK_ASSERT(check_invariant());
		QUARK_ASSERT(check_reg(dest_reg));
		QUARK_ASSERT(dest_type.check_invariant());

		release_value_safe(_backend, get_current_frame_regs()[dest_reg], dest_type);
		retain_value(_backend, value, dest_type);
		get_current_frame_regs()[dest_reg] = value;

		QUARK_ASSERT(check_invariant());
	}

#if DEBUG
	public: bool check_reg_any(const int reg) const{
		QUARK_ASSERT(check_reg(reg));
		return true;
	}

	public: bool check_reg_bool(const int reg) const{
		QUARK_ASSERT(check_reg(reg));
		QUARK_ASSERT(peek2(_backend.types, _current_frame._static_frame_ptr->_symbol_effective_type[reg]).is_bool());
		return true;
	}

	public: bool check_reg_int(const int reg) const{
		QUARK_ASSERT(check_reg(reg));
		QUARK_ASSERT(peek2(_backend.types, _current_frame._static_frame_ptr->_symbol_effective_type[reg]).is_int());
		return true;
	}

	public: bool check_reg_double(const int reg) const{
		QUARK_ASSERT(check_reg(reg));
		QUARK_ASSERT(peek2(_backend.types, _current_frame._static_frame_ptr->_symbol_effective_type[reg]).is_double());
		return true;
	}

	public: bool check_reg_string(const int reg) const{
		QUARK_ASSERT(check_reg(reg));
		QUARK_ASSERT(peek2(_backend.types, _current_frame._static_frame_ptr->_symbol_effective_type[reg]).is_string());
		return true;
	}

	public: bool check_reg_json(const int reg) const{
		QUARK_ASSERT(check_reg(reg));
		QUARK_ASSERT(peek2(_backend.types, _current_frame._static_frame_ptr->_symbol_effective_type[reg]).is_json());
		return true;
	}

	public: bool check_reg_vector_w_external_elements(const int reg) const{
		QUARK_ASSERT(check_reg(reg));
		QUARK_ASSERT(peek2(_backend.types, _current_frame._static_frame_ptr->_symbol_effective_type[reg]).is_vector());
		return true;
	}

	public: bool check_reg_vector_w_inplace_elements(const int reg) const{
		QUARK_ASSERT(check_reg(reg));
		QUARK_ASSERT(peek2(_backend.types, _current_frame._static_frame_ptr->_symbol_effective_type[reg]).is_vector());
		return true;
	}

	public: bool check_reg_dict_w_external_values(const int reg) const{
		QUARK_ASSERT(check_reg(reg));
		QUARK_ASSERT(peek2(_backend.types, _current_frame._static_frame_ptr->_symbol_effective_type[reg]).is_dict());
		return true;
	}
	public: bool check_reg_dict_w_inplace_values(const int reg) const{
		QUARK_ASSERT(check_reg(reg));
		QUARK_ASSERT(peek2(_backend.types, _current_frame._static_frame_ptr->_symbol_effective_type[reg]).is_dict());
		return true;
	}

	public: bool check_reg_struct(const int reg) const{
		QUARK_ASSERT(check_reg(reg));
//		QUARK_ASSERT(_stack._current_frame._static_frame_ptr->_symbol_effective_type[reg].is_struct());
		return true;
	}

	public: bool check_reg_function(const int reg) const{
		QUARK_ASSERT(check_reg(reg));
//		QUARK_ASSERT(_stack._current_frame._static_frame_ptr->_symbol_effective_type[reg].is_function());
		return true;
	}

	public: bool check_reg__external_value(const int reg) const{
		QUARK_ASSERT(check_reg(reg));
		return true;
	}

	public: bool check_reg__inplace_value(const int reg) const{
		QUARK_ASSERT(check_reg(reg));
		return true;
	}
#endif


	////////////////////////		STATE
	public: std::shared_ptr<bc_program_t> _program;
	public: runtime_process_i* _process_handler;
	public: runtime_handler_i* _runtime_handler;

	public: value_backend_t& _backend;
	public: std::string _name;


	//	Holds all values for all environments.
	//	Notice: stack holds refs to RC-counted objects!
	public: interpreter_stack_t _stack;
	public: frame_pos_t _current_frame;
};

void trace_stack(interpreter_stack_t& stack, const frame_pos_t& current_frame);
void trace_interpreter(interpreter_t& vm, size_t pc);


//////////////////////////////////////		Free functions



rt_value_t call_function_bc(interpreter_t& vm, const rt_value_t& f, const rt_value_t args[], int arg_count);
json_t interpreter_to_json(interpreter_t& vm);
std::pair<bc_typeid_t, rt_value_t> execute_instructions(interpreter_t& vm, const std::vector<bc_instruction_t>& instructions);

//	Returns undefined if not found.
rt_value_t load_global(interpreter_t& vm, const module_symbol_t& s);


} //	floyd

#endif /* bytecode_interpreter_hpp */
