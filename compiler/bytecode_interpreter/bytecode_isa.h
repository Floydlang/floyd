//
//  bytecode_isa.hpp
//  floyd
//
//  Created by Marcus Zetterquist on 2020-01-05.
//  Copyright © 2020 Marcus Zetterquist. All rights reserved.
//

#ifndef bytecode_isa_hpp
#define bytecode_isa_hpp

#include <string>
#include <vector>
#include <map>

namespace floyd {




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



} //	floyd

#endif /* bytecode_isa_hpp */
