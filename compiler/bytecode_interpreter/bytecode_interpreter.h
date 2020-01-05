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
#include "json_support.h"
#include "compiler_basics.h"
#include "ast_value.h"
#include "value_backend.h"
#include "floyd_runtime.h"
#include "quark.h"

#include "immer/vector.hpp"
#include "immer/map.hpp"

#include <string>
#include <vector>
#include <map>
#include <atomic>
#include <chrono>

/*
FRAME INFO
stack + 0	prev frame (stack pos)
stack + 1	prev static frame ptr

GLOBAL-FRAME START
stack + 2	arg 0		symbol 0
stack + 3	arg 1		symbol 1
stack + 4	arg 2		symbol 2
stack + 5	local 0		symbol 3
stack + 6	local 1		symbol 4
stack + 7	local 2		symbol 5
stack + 8	local 3		symbol 6
stack + 9	temporary
stack + 10	temporary

Call to function x:

FRAME INFO
stack + 11	prev frame = 2 (stack pos)
stack + 12	prev static frame ptr

FUNCTION X FRAME START
stack + 13	arg 0		symbol 0
stack + 14	arg 1		symbol 1
stack + 15	local 0		symbol 2
stack + 16	local 1		symbol 3
stack + 17	temporary



Call to function Y:

FRAME INFO
stack + 18	prev frame = 13 (stack pos)
stack + 19	prev static frame ptr

FUNCTION Y FRAME START
stack + 20	arg 0		symbol 0
stack + 21	arg 1		symbol 1
stack + 22	local 0		symbol 2
stack + 23	local 1		symbol 3

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


//////////////////////////////////////		bc_opcode

/*
	These are the byte code instructions used by the interpreter.
	Each instruction has an 8bit opcode and 3 x 16-bit fields, called A, B, C.
	Each opcode has description that tells how A-B-C are used.
*/
enum class bc_opcode: uint8_t {
	k_nop = 0,


	/*
		A: Register: where to put result
		B: IMMEDIATE: global index
		C: ---
	*/
	k_load_global_external_value,


	/*
		A: Register: where to put result
		B: IMMEDIATE: global index
		C: ---
	*/
	k_load_global_inplace_value,

	/*
		Overwrites the destination value, without releaseing its RC -- it has no RC since it's not initialized yet.
		A: Register: where to put result
		B: Register: source register
		C: ---
	*/
	k_init_local,


	/*
		A: IMMEDIATE: global index
		B: Register: source reg
		C: ---
	*/
	k_store_global_external_value,

	/*
		A: IMMEDIATE: global index
		B: Register: source reg
		C: ---
	*/
	k_store_global_inplace_value,

	/*
		A: Register: where to put result
		B: Register: source value
		C: ---
	*/
	k_copy_reg_inplace_value,
	/*
		A: Register: where to put result
		B: Register: source value
		C: ---
	*/
	k_copy_reg_external_value,


	/*
		A: Register: where to put result
		B: Register: struct object
		C: IMMEDIATE: member-index
	*/
	k_get_struct_member,

	/*
		A: Register: where to put result
		B: Register: string object/vector object/json/dict
		C: Register: index (int)
	*/
	k_lookup_element_string,
	k_lookup_element_json,
	k_lookup_element_vector_w_external_elements,
	k_lookup_element_vector_w_inplace_elements,
	k_lookup_element_dict_w_external_values,
	k_lookup_element_dict_w_inplace_values,

	/*
		A: Register: where to put result: integer
		B: Register: object
		C: 0
	*/
	k_get_size_vector_w_external_elements,
	k_get_size_vector_w_inplace_elements,
	k_get_size_dict_w_external_values,
	k_get_size_dict_w_inplace_values,
	k_get_size_string,
	k_get_size_jsonvalue,

	/*
		A: Register: where to put result: integer
		B: Register: object
		C: Register: value
	*/
	k_pushback_vector_w_external_elements,
	k_pushback_vector_w_inplace_elements,
	k_pushback_string,

	/*
		A: Register: tells where to put function return
		B: Register: function value to call
		C: IMMEDIATE: argument count. Values are put on stack. Notice that DYN arguments pushes itype first.

		All arguments are pushed to stack, first argument first.
		DYN arguments are pushed as (itype, value)
	*/
	k_call,

	/*
		A: Register: where to put result
		B: Register: lhs
		C: Register: rhs
	*/
	k_add_bool,
	k_add_int,
	k_add_double,

	k_concat_strings,
	k_concat_vectors_w_external_elements,
//	k_concat_vectors_w_inplace_elements,

	k_subtract_double,
	k_subtract_int,
	k_multiply_double,
	k_multiply_int,
	k_divide_double,
	k_divide_int,
	k_remainder,
	k_remainder_int,

	k_logical_and_bool,
	k_logical_and_int,
	k_logical_and_double,
	k_logical_or_bool,
	k_logical_or_int,
	k_logical_or_double,


	//////////////////////////////////////		COMPARISON

	//??? Idea: Remove all conditions. Only have conditional branches.
	/*
		The type-unspecific version is a fallback to handles all types not special-cased.
		A: Register: where to put result BOOL
		B: Register: lhs
		C: Register: rhs

		A: Register: where to put result BOOL
		B: Register: lhs
		C: Register: rhs
	*/
	k_comparison_smaller_or_equal,
	k_comparison_smaller_or_equal_int,
	k_comparison_smaller,
	k_comparison_smaller_int,

	k_logical_equal,
	k_logical_equal_int,
	k_logical_nonequal,
	k_logical_nonequal_int,


	/*
		A: Register: where to put resulting value
		B: IMMEDIATE: Target type - type to create.
		C: IMMEDIATE: Source itype

		Stack hold ONE value, which is the input value
	*/
	k_new_1,

	/*
		Creates a new vector containing object-elements, like strings, structs etc.

		A: Register: where to put resulting value
		B: IMMEDIATE: itype T = [E], describing output type of vector, like [int] or [my_pixel].
		C: IMMEDIATE: Argument count.

		Arguments are put on stack. No DYN arguments. All arguments are of type E.
	*/
	k_new_vector_w_external_elements,

	/*
		A: Register: where to put resulting value
		B: IMMEDIATE: 0
		C: IMMEDIATE: Argument count.

		Arguments are put on stack. All arguments are of type int.
	*/
	k_new_vector_w_inplace_elements,

	/*
		A: Register: where to put resulting value
		B: IMMEDIATE: itype T = [string:V], describing output type of dict, like [string:int] or [string:my_pixel].
		C: IMMEDIATE: Argument count. Each dict entry becomes TWO arguments: (string, V).

		Arguments are put on stack. No DYN arguments. All arguments are of type (string, V).

		Example:
			a = { "chewie": 10.0, "leia": 20.0, "luke": 30.0 }

			Gives:

			B: itype with [string, double]
			C: 6
			Stack: "chewie", 10.0, "leia", 20.0, "luke", 30.0
	*/
	k_new_dict_w_external_values,
	k_new_dict_w_inplace_values,

	/*
		A: Register: where to put resulting value
		B: IMMEDIATE: itype T of the struct
		C: IMMEDIATE: Argument count.

		Arguments are put on stack. No DYN arguments.
		All arguments are of types matching the struct members.
	*/
	k_new_struct,

	/*
		A: Register: value to return
		B: ---
		C: ---
	*/
	k_return,

	/*
		A: ---
		B: ---
		C: ---
	*/
	k_stop,


	//////////////////////////////////////		STACK

	/*
		Pushes the VM frame info to stack.
		A: ---
		B: ---
		C: ---
		STACK 1: a b c
		STACK 2: a b c [prev frame pos] [frame_ptr]
	*/
	k_push_frame_ptr,

	/*
		Pops the VM frame info to stack -- this changes the VM's frame pos & frame ptr.
		A: ---
		B: ---
		C: ---
		STACK 1: a b c [prev frame pos] [frame_ptr]
		STACK 2: a b c
	*/
	k_pop_frame_ptr,

	///??? Could optimize by pushing 3 values with ONE instruction -- use A B C.
	///??? Could optimize by using a byte-stack and only pushing minimal number of bytes. Bool needs 1 byte only.
	/*
		A: Register: where to read V
		B: ---
		C: ---
		STACK 1: a b c
		STACK 2: a b c V
	*/
	k_push_inplace_value,

	/*
		NOTICE: This function bumps the RC of the pushed V-object. This represents the stack-entry co-owning V.
		A: Register: where to read V
		B: ---
		C: ---
		STACK 1: a b c
		STACK 2: a b c V
	*/
	k_push_external_value,

	/*
		A: IMMEDIATE: arg count. 0 to 32.
		B: IMMEDIATE: extbits. bit 0 maps to the next value to be popped from stack.
		C: ---

		Example 1: one OBJ
		n:	1		 -------- -------- -------- -------=
		extbits:	%00000000 00000000 00000000 00000001
		STACK 1: a b c OBJ
		STACK 2: a b c

		Example 2: Three values.
		n:	4		 -------- -------- -------- ----====
		extbits:	%00000000 00000000 00000000 00001110
		STACK 1: a b c OBJ OBJ OBJ INTERN
		STACK 2: a b c
	*/
	k_popn,


	//////////////////////////////////////		BRANCH


	/*
		A: Register: input to test
		B: IMMEDIATE: branch offset (added to PC) on branch.
		C: ---
		NOTICE: These have their own conditions, instead of check bool from k_comparison_smaller.
	*/
	k_branch_false_bool,
	k_branch_true_bool,
	k_branch_zero_int,
	k_branch_notzero_int,

	/*
		A: Register: lhs
		B: Register: rhs
		C: IMMEDIATE: branch offset (added to PC) on branch.
	*/
	k_branch_smaller_int,
	k_branch_smaller_or_equal_int,

	/*
		A: ---
		B: IMMEDIATE: branch offset (added to PC) on branch.
		C: ---
	*/
	k_branch_always
};



//////////////////////////////////////		bc_instruction_t

//	Compile-time information about the different opcodes: it's name, how the fields A-B-C are used.
//	Term: These elements inside an instruction are called "fields".

struct opcode_info_t {
	std::string _as_text;

	//	There is a set of encoding pattern how A-B-C are used that are shared between the instructions.
	//	The patterns are named "e", "q" etc.
	//	0: set operand to 0
	//	r: operand specifies a register
	//	i: operand contains an immediate number
	enum class encoding {
		k_e_0000,
		k_k_0ri0,
		k_l_00i0,
		k_n_0ii0,
		k_o_0rrr,
		k_p_0r00,
		k_q_0rr0,
		k_r_0ir0,
		k_s_0rri,
		k_t_0rii
	};
	encoding _encoding;
};

extern const std::map<bc_opcode, opcode_info_t> k_opcode_info;



//////////////////////////////////////		reg_flags_t


struct reg_flags_t {
	bool _a;
	bool _b;
	bool _c;
};

//	Tells if a register is specified
reg_flags_t encoding_to_reg_flags(opcode_info_t::encoding e);


//////////////////////////////////////		bc_instruction_t

//	The byte code instruction itself, as executed by the interpreter.
//	It's 64-bits big, has an opcode and 3 operands, A-B-C.

struct bc_instruction_t {
	bc_instruction_t(bc_opcode opcode, int16_t a, int16_t b, int16_t c);
	public: bool check_invariant() const;


	//////////////////////////////////////		STATE
	bc_opcode _opcode;
	uint8_t _zero;
	int16_t _a;
	int16_t _b;
	int16_t _c;
};


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


//////////////////////////////////////		frame_pos_t


//	Keeps track where in the interpreter's stack we're executing code right now.
struct frame_pos_t {
	size_t _frame_pos;
	const bc_static_frame_t* _frame_ptr;
};

inline bool operator==(const frame_pos_t& lhs, const frame_pos_t& rhs){
	return lhs._frame_pos == rhs._frame_pos && lhs._frame_ptr == rhs._frame_ptr;
}


//////////////////////////////////////		interpreter_stack_t

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
	0	[int = 0] 		previous stack frame pos, 0 = global
	1	[symbols_ptr frame #0]
	2	[local0]		<- stack frame #0
	3	[local1]
	4	[local2]

	5	[int = 1] //	prev stack frame pos
	6	[symbols_ptr frame #1]
	7	[local1]		<- stack frame #1
	8	[local2]
	9	[local3]
*/

void release_value_safe(value_backend_t& backend, rt_pod_t value, type_t type);

struct interpreter_stack_t {
	public: interpreter_stack_t(value_backend_t* backend, const bc_static_frame_t* global_frame) :
		_backend(backend),
		_current_static_frame(nullptr),
		_current_frame_start_ptr(nullptr),
		_global_frame(global_frame),
		_entries(nullptr),
		_allocated_count(0),
		_stack_size(0)
	{
		QUARK_ASSERT(global_frame != nullptr && global_frame->check_invariant());
		QUARK_ASSERT(backend != nullptr && backend->check_invariant());

		_entries = new rt_pod_t[8192];
		_allocated_count = 8192;
		_current_frame_start_ptr = &_entries[0];

		QUARK_ASSERT(check_invariant());
	}

	public: ~interpreter_stack_t(){
		QUARK_ASSERT(check_invariant());

		delete[] _entries;
		_entries = nullptr;
	}

	public: bool check_invariant() const {
		QUARK_ASSERT(_backend->check_invariant());
		QUARK_ASSERT(_entries != nullptr);
		QUARK_ASSERT(_stack_size >= 0 && _stack_size <= _allocated_count);

		QUARK_ASSERT(_current_frame_start_ptr >= &_entries[0]);

		QUARK_ASSERT(_entry_types.size() == _stack_size);
		for(int i = 0 ; i < _stack_size ; i++){
			QUARK_ASSERT(_entry_types[i].check_invariant());
		}

		const auto frames = get_stack_frames_noci();

		return true;
	}

	public: interpreter_stack_t(const interpreter_stack_t& other) = delete;
	public: interpreter_stack_t& operator=(const interpreter_stack_t& other) = delete;

	public: void swap(interpreter_stack_t& other) throw() {
		QUARK_ASSERT(check_invariant());
		QUARK_ASSERT(other.check_invariant());

		std::swap(other._backend, _backend);
		std::swap(other._entries, _entries);
		std::swap(other._allocated_count, _allocated_count);
		std::swap(other._stack_size, _stack_size);
		other._entry_types.swap(_entry_types);
		std::swap(_current_static_frame, other._current_static_frame);
		std::swap(_current_frame_start_ptr, other._current_frame_start_ptr);

		std::swap(_global_frame, other._global_frame);

		QUARK_ASSERT(check_invariant());
		QUARK_ASSERT(other.check_invariant());
	}

	public: int size() const {
		QUARK_ASSERT(check_invariant());

		return static_cast<int>(_stack_size);
	}




	//////////////////////////////////////		FRAMES


	public: void save_frame();
	public: void restore_frame();
	public: void open_frame_except_args(const bc_static_frame_t& frame, int pushed_arg_count);
	public: void close_frame(const bc_static_frame_t& frame);

	private: frame_pos_t read_frame_info(size_t pos) const;
#if DEBUG
	public: bool check_stack_frame(const frame_pos_t& in_frame) const;
#endif

	public: struct active_frame_t {
		size_t start_pos;
		size_t end_pos;
		const bc_static_frame_t* static_frame;
		size_t temp_count;
	};
	public: std::vector<active_frame_t> get_stack_frames_noci() const;

	public: size_t get_current_frame_pos() const {
//		QUARK_ASSERT(check_invariant());

		const auto frame_pos = _current_frame_start_ptr - &_entries[0];
		return frame_pos;
	}


	//////////////////////////////////////		STACK



	public: void push_external_value(const rt_value_t& value){
		QUARK_ASSERT(check_invariant());
		QUARK_ASSERT(value.check_invariant());

		retain_value(*_backend, value._pod, value._type);
		_entries[_stack_size] = value._pod;
		_stack_size++;
		_entry_types.push_back(value._type);

		QUARK_ASSERT(check_invariant());
	}

	public: void push_external_value_blank(const type_t& type){
		QUARK_ASSERT(check_invariant());
		QUARK_ASSERT(type.check_invariant());

		_entries[_stack_size] = make_uninitialized_magic();
		_stack_size++;
		_entry_types.push_back(type);

		QUARK_ASSERT(check_invariant());
	}

	public: void push_inplace_value(const rt_value_t& value){
		QUARK_ASSERT(check_invariant());
		QUARK_ASSERT(value.check_invariant());

		_entries[_stack_size] = value._pod;
		_stack_size++;
		_entry_types.push_back(value._type);

		QUARK_ASSERT(check_invariant());
	}

	//	returned value will have ownership of obj, if it's an obj.
	//??? should be const function
	public: rt_value_t load_value(int pos, const type_t& type){
		QUARK_ASSERT(check_invariant());
		QUARK_ASSERT(pos >= 0 && pos < _stack_size);
		QUARK_ASSERT(type.check_invariant());
		QUARK_ASSERT(peek2(_backend->types, type) == peek2(_backend->types, _entry_types[pos]));

		const auto& e = _entries[pos];
		const auto result = rt_value_t(*_backend, e, type, rt_value_t::rc_mode::bump);
		return result;
	}

	public: int64_t load_intq(int pos) const{
//		QUARK_ASSERT(check_invariant());
		QUARK_ASSERT(pos >= 0 && pos < _stack_size);
		QUARK_ASSERT(peek2(_backend->types, _entry_types[pos]).is_int());

		return _entries[pos].int_value;
	}

	public: void replace_external_value(int pos, const rt_value_t& value){
		QUARK_ASSERT(check_invariant());
		QUARK_ASSERT(value.check_invariant());
		QUARK_ASSERT(pos >= 0 && pos < _stack_size);
		QUARK_ASSERT(_entry_types[pos] == value._type);

		auto prev_copy = _entries[pos];

		retain_value(*_backend, value._pod, value._type);
		_entries[pos] = value._pod;
		release_value_safe(*_backend, prev_copy, value._type);

		QUARK_ASSERT(check_invariant());
	}

	//	exts[exts.size() - 1] maps to the closed value on stack, the next to be popped.
	public: void pop_batch(const std::vector<type_t>& exts){
		QUARK_ASSERT(check_invariant());
		QUARK_ASSERT(_stack_size >= exts.size());

		auto flag_index = exts.size() - 1;
		for(int i = 0 ; i < exts.size() ; i++){
			pop(exts[flag_index]);
			flag_index--;
		}
		QUARK_ASSERT(check_invariant());
	}

	private: void pop(const type_t& type){
		QUARK_ASSERT(check_invariant());
		QUARK_ASSERT(_stack_size > 0);

		auto copy = _entries[_stack_size - 1];
		_stack_size--;
		_entry_types.pop_back();
		release_value_safe(*_backend, copy, type);

		QUARK_ASSERT(check_invariant());
	}



	////////////////////////		STATE

	public: value_backend_t* _backend;
	public: rt_pod_t* _entries;
	public: size_t _allocated_count;
	public: size_t _stack_size;

	//	These are parallell with _entries, one element for each entry on the stack.
	//??? Kill these - we should have the types in the static frames already.
	public: std::vector<type_t> _entry_types;

	public: const bc_static_frame_t* _current_static_frame;
	public: rt_pod_t* _current_frame_start_ptr;

	public: const bc_static_frame_t* _global_frame;
};

json_t stack_to_json(const interpreter_stack_t& stack, value_backend_t& backend);



//////////////////////////////////////		interpreter_t

/*
	Complete runtime state of the interpreter.
	MUTABLE
	Non-copyable.
*/

struct interpreter_t : runtime_basics_i {
	public: explicit interpreter_t(const std::shared_ptr<bc_program_t>& program, value_backend_t& backend, const config_t& config, runtime_process_i* process_handler, runtime_handler_i& runtime_handler, const std::string& name);
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
		QUARK_ASSERT(global_index >= 0 && global_index < (k_frame_overhead + _stack._global_frame->_symbols._symbols.size()));
		return true;
	}
	public: bool check_global_access_intern(const int global_index) const{
		QUARK_ASSERT(check_invariant());
		QUARK_ASSERT(global_index >= 0 && global_index < (k_frame_overhead + _stack._global_frame->_symbols._symbols.size()));
		return true;
	}


	//////////////////////////////////////		REGISTERS


	public: bool check_reg(int reg) const{
		//	Makes sure register is within current stack frame bounds.
		QUARK_ASSERT(reg >= 0 && reg < _stack._current_static_frame->_symbols._symbols.size());

		//	Makes sure debug types are in sync for this register.
		const auto& symbol_type = _stack._current_static_frame->_symbols._symbols[reg].second._value_type;
		const auto& effective_type = _stack._current_static_frame->_symbol_effective_type[reg];
		const auto& debug_type = _stack._entry_types[_stack.get_current_frame_pos() + reg];
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
			_stack._current_frame_start_ptr[reg],
			_stack._current_static_frame->_symbol_effective_type[reg],
			rt_value_t::rc_mode::bump
		);
		QUARK_ASSERT(result.check_invariant());
		return result;
	}

	public: void write_register(const int reg, const rt_value_t& value){
		QUARK_ASSERT(check_invariant());
		QUARK_ASSERT(check_reg(reg));
		QUARK_ASSERT(value.check_invariant());
		QUARK_ASSERT(peek2(_backend.types, _stack._current_static_frame->_symbol_effective_type[reg]) == peek2(_backend.types, value._type));

		const auto& frame_slot_type = _stack._current_static_frame->_symbol_effective_type[reg];

		const auto prev = _stack._current_frame_start_ptr[reg];
		release_value_safe(_backend, prev, frame_slot_type);
		retain_value(_backend, value._pod, frame_slot_type);
		_stack._current_frame_start_ptr[reg] = value._pod;

		QUARK_ASSERT(check_invariant());
	}

#if DEBUG
	public: bool check_reg_any(const int reg) const{
		QUARK_ASSERT(check_invariant());
		QUARK_ASSERT(check_reg(reg));
		return true;
	}

	public: bool check_reg_bool(const int reg) const{
		QUARK_ASSERT(check_invariant());
		QUARK_ASSERT(check_reg(reg));
		QUARK_ASSERT(peek2(_backend.types, _stack._current_static_frame->_symbol_effective_type[reg]).is_bool());
		return true;
	}

	public: bool check_reg_int(const int reg) const{
		QUARK_ASSERT(check_invariant());
		QUARK_ASSERT(check_reg(reg));
		QUARK_ASSERT(peek2(_backend.types, _stack._current_static_frame->_symbol_effective_type[reg]).is_int());
		return true;
	}

	public: bool check_reg_double(const int reg) const{
		QUARK_ASSERT(check_invariant());
		QUARK_ASSERT(check_reg(reg));
		QUARK_ASSERT(peek2(_backend.types, _stack._current_static_frame->_symbol_effective_type[reg]).is_double());
		return true;
	}

	public: bool check_reg_string(const int reg) const{
		QUARK_ASSERT(check_invariant());
		QUARK_ASSERT(check_reg(reg));
		QUARK_ASSERT(peek2(_backend.types, _stack._current_static_frame->_symbol_effective_type[reg]).is_string());
		return true;
	}

	public: bool check_reg_json(const int reg) const{
		QUARK_ASSERT(check_invariant());
		QUARK_ASSERT(check_reg(reg));
		QUARK_ASSERT(peek2(_backend.types, _stack._current_static_frame->_symbol_effective_type[reg]).is_json());
		return true;
	}

	public: bool check_reg_vector_w_external_elements(const int reg) const{
		QUARK_ASSERT(check_invariant());
		QUARK_ASSERT(check_reg(reg));
		QUARK_ASSERT(peek2(_backend.types, _stack._current_static_frame->_symbol_effective_type[reg]).is_vector());
		return true;
	}

	public: bool check_reg_vector_w_inplace_elements(const int reg) const{
		QUARK_ASSERT(check_invariant());
		QUARK_ASSERT(check_reg(reg));
		QUARK_ASSERT(peek2(_backend.types, _stack._current_static_frame->_symbol_effective_type[reg]).is_vector());
		return true;
	}

	public: bool check_reg_dict_w_external_values(const int reg) const{
		QUARK_ASSERT(check_invariant());
		QUARK_ASSERT(check_reg(reg));
		QUARK_ASSERT(peek2(_backend.types, _stack._current_static_frame->_symbol_effective_type[reg]).is_dict());
		return true;
	}
	public: bool check_reg_dict_w_inplace_values(const int reg) const{
		QUARK_ASSERT(check_invariant());
		QUARK_ASSERT(check_reg(reg));
		QUARK_ASSERT(peek2(_backend.types, _stack._current_static_frame->_symbol_effective_type[reg]).is_dict());
		return true;
	}

	public: bool check_reg_struct(const int reg) const{
		QUARK_ASSERT(check_invariant());
		QUARK_ASSERT(check_reg(reg));
//		QUARK_ASSERT(_stack._current_static_frame->_symbol_effective_type[reg].is_struct());
		return true;
	}

	public: bool check_reg_function(const int reg) const{
		QUARK_ASSERT(check_invariant());
		QUARK_ASSERT(check_reg(reg));
//		QUARK_ASSERT(_stack._current_static_frame->_symbol_effective_type[reg].is_function());
		return true;
	}

	public: bool check_reg__external_value(const int reg) const{
		QUARK_ASSERT(check_invariant());
		QUARK_ASSERT(check_reg(reg));
		return true;
	}

	public: bool check_reg__inplace_value(const int reg) const{
		QUARK_ASSERT(check_invariant());
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
};

void trace_interpreter(interpreter_t& vm, int pc);


//////////////////////////////////////		Free functions


std::vector<func_link_t> link_functions(const bc_program_t& program);

rt_value_t call_function_bc(interpreter_t& vm, const rt_value_t& f, const rt_value_t args[], int arg_count);
json_t interpreter_to_json(interpreter_t& vm);
std::pair<bc_typeid_t, rt_value_t> execute_instructions(interpreter_t& vm, const std::vector<bc_instruction_t>& instructions);


/*
struct value_entry_t {
	bool check_invariant() const {
		QUARK_ASSERT(_value.check_invariant());
		QUARK_ASSERT(_symbol_name.s.empty() == false);
		QUARK_ASSERT(_symbol.check_invariant());
		QUARK_ASSERT(_global_index >= 0);
		return true;
	}

	rt_value_t _value;
	module_symbol_t _symbol_name;
	symbol_t _symbol;
	int _global_index;
};
*/

//	Returns undefined if not found.
rt_value_t load_global(interpreter_t& vm, const module_symbol_t& s);

std::vector<std::pair<type_t, struct_layout_t>> bc_make_struct_layouts(const types_t& types);



//////////////////////////////////////		INLINES



//??? Remove need for this function! BC should overwrite registers by default = no need to release_value() on previous value.
inline void release_value_safe(value_backend_t& backend, rt_pod_t value, type_t type){
	QUARK_ASSERT(backend.check_invariant());
	QUARK_ASSERT(value.check_invariant());
	QUARK_ASSERT(type.check_invariant());

	const auto& peek = peek2(backend.types, type);

	if(is_rc_value(backend.types, peek) && value.int_value == UNINITIALIZED_RUNTIME_VALUE){
	}
	else{
		return release_value(backend, value, type);
	}
}

} //	floyd


#endif /* bytecode_interpreter_hpp */
