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



??? remove usage of typeid_t. Use itype & types[]?
??? All functions should be the same type of function-values: host-functions and Floyd functions: _function_id should be in the VALUE not function definition!
??? Less code + faster to generate increc, decref instructions instead of make *_external_value, *_internal_value opcodes.
*/

#include "typeid.h"
#include "json_support.h"
#include "software_system.h"
#include "compiler_basics.h"
#include "ast_value.h"
#include "type_interner.h"
#include "quark.h"

#include "immer/vector.hpp"
#include "immer/map.hpp"

#include <string>
#include <vector>
#include <map>
#include <atomic>
#include <chrono>



namespace floyd {

struct itype_t;
struct type_interner_t;

struct interpreter_t;
struct bc_program_t;
struct bc_static_frame_t;
struct runtime_handler_i;

struct bc_value_t;
union bc_pod_value_t;
struct bc_external_value_t;
struct bc_external_handle_t;


typedef bc_value_t (*BC_NATIVE_FUNCTION_PTR)(interpreter_t& vm, const bc_value_t args[], int arg_count);
typedef int16_t bc_typeid_t;


//////////////////////////////////////		bc_inplace_value_t

//	A value that is inplace == all its data sits in this struct, no external memory required.
//	These values are always pods and can be copied, no referencing counting required.

union bc_inplace_value_t {
	bool bool_value;
	int64_t int64_value;
	double double_value;

	const bc_static_frame_t* _frame_ptr;
};


//////////////////////////////////////		bc_pod_value_t

//	Holds any type of value, both an inplace and external values.
//	IMPORTANT: Has no constructor, destructor etc!! POD.
//	Does NOT handle reference counting if this is an external value. Use bc_value_t for that!

union bc_pod_value_t {
	const bc_external_value_t* _external;
	bc_inplace_value_t _inplace;
};

void release_pod_external(bc_pod_value_t& value);


//////////////////////////////////////		value_encoding

//	Tells how a specific type of value needs to be store in the interpreter.

enum class value_encoding {
	k_none,
	k_inplace__bool,
	k_inplace__int_as_uint64,
	k_inplace__double,
	k_external__string,
	k_external__json,

	k_external__typeid,

	k_external__struct,
	k_external__vector,
	k_external__vector_pod64,
	k_external__dict,
	k_external__function
};

bool encode_as_inplace(const itype_t& type);

bool encode_as_vector_w_inplace_elements(const type_interner_t& interner, const itype_t& type);

bool encode_as_dict_w_inplace_values(const type_interner_t& interner, const itype_t& type);

value_encoding type_to_encoding(const type_interner_t& interner, const itype_t& type);

//	Will this type of value require an ext ? bc_external_value_t to be used?
bool encode_as_external(value_encoding encoding);
bool encode_as_external(const typeid_t& type);
bool encode_as_external(const itype_t& type);


//////////////////////////////////////		bc_value_t

/*
	Efficent representation of any value supported by the interpreter.
	It's immutable and uses value-semantics.
	Holds either an inplace value or an external value. Handles reference counting automatically when required.
	??? replace with variant<>
*/

struct bc_value_t {
	public: bool check_invariant() const;
	public: bc_value_t();
	public: ~bc_value_t();
	public: bc_value_t(const bc_value_t& other);
	public: bc_value_t& operator=(const bc_value_t& other);
	public: void swap(bc_value_t& other);
	public: explicit bc_value_t(const bc_static_frame_t* frame_ptr);

	enum class mode {
		k_unwritten_ext_value
	};
	public: inline explicit bc_value_t(const itype_t& type, mode mode);


	//////////////////////////////////////		internal-undefined type
	public: static bc_value_t make_undefined();


	//////////////////////////////////////		internal-dynamic type
	public: static bc_value_t make_any();


	//////////////////////////////////////		void
	public: static bc_value_t make_void();


	//////////////////////////////////////		bool
	public: static bc_value_t make_bool(bool v);
	public: bool get_bool_value() const;
	private: explicit bc_value_t(bool value);


	//////////////////////////////////////		int
	public: static bc_value_t make_int(int64_t v);
	public: int64_t get_int_value() const;
	private: explicit bc_value_t(int64_t value);


	//////////////////////////////////////		double
	public: static bc_value_t make_double(double v);
	public: double get_double_value() const;
	private: explicit bc_value_t(double value);


	//////////////////////////////////////		string
	public: static bc_value_t make_string(const std::string& v);
	public: std::string get_string_value() const;
	private: explicit bc_value_t(const std::string& value);


	//////////////////////////////////////		json
	public: static bc_value_t make_json(const json_t& v);
	public: json_t get_json() const;
	private: explicit bc_value_t(const std::shared_ptr<json_t>& value);


	//////////////////////////////////////		typeid
	public: static bc_value_t make_typeid_value(const itype_t& type_id);
	public: itype_t get_typeid_value() const;
	private: explicit bc_value_t(const itype_t& type_id);


	//////////////////////////////////////		struct
	public: static bc_value_t make_struct_value(const itype_t& struct_type, const std::vector<bc_value_t>& values);
	public: const std::vector<bc_value_t>& get_struct_value() const;
	private: explicit bc_value_t(const itype_t& struct_type, const std::vector<bc_value_t>& values, bool struct_tag);


	//////////////////////////////////////		function
	public: static bc_value_t make_function_value(const itype_t& function_type, const function_id_t& function_id);
	public: function_id_t get_function_value() const;
	private: explicit bc_value_t(const itype_t& function_type, const function_id_t& function_id, bool dummy);


	//	Bumps RC if needed.
	public: explicit bc_value_t(const itype_t& type, const bc_pod_value_t& internals);

	//	Won't bump RC.
	public: bc_value_t(const itype_t& type, const bc_inplace_value_t& pod64);

	//	Bumps RC.
	public: explicit bc_value_t(const itype_t& type, const bc_external_handle_t& handle);


	//////////////////////////////////////		STATE
	//??? make private, also check other classes.
	public: itype_t _type;
	public: bc_pod_value_t _pod;
};



//////////////////////////////////////		bc_external_handle_t

//	Wraps an external value and handles reference counting for it.
//	Immutable, value-semantics.
//	It does not its type, just that it's an external value.
//	These values are put in collections.

struct bc_external_handle_t {
	public: bc_external_handle_t(const bc_external_handle_t& other);
	public: explicit bc_external_handle_t(const bc_external_value_t* ext);
	public: explicit bc_external_handle_t(const bc_value_t& value);
	public: void swap(bc_external_handle_t& other);
	public: bc_external_handle_t& operator=(const bc_external_handle_t& other);
	public: ~bc_external_handle_t();

	public: bool check_invariant() const;


	//////////////////////////////////////		STATE
	//	Uses intrusive reference counting, that's why this isn't just a shared_ptr<>
	public: const bc_external_value_t* _external;
};


bool check_external_deep(const type_interner_t& interner, const itype_t& type, const bc_external_value_t* ext);


//////////////////////////////////////		bc_external_value_t

/*
	This object contains the internals of values too big to be stored inplace inside bc_value_t / bc_pod_value_t.
	The bc_external_value_t:s are allocated on the heap and are reference counted.

	TODO: Right now wastes resouces by containing *all* types of external values! Should use std::variant.
*/

struct bc_external_value_t {
	public: bc_external_value_t(const std::string& s);
	public: bc_external_value_t(const std::shared_ptr<json_t>& s);
	public: bc_external_value_t(const itype_t& type, const function_id_t& function_id);

	public: bc_external_value_t(const itype_t& s);
	public: bc_external_value_t(const itype_t& type, const std::vector<bc_value_t>& s, bool struct_tag);
	public: bc_external_value_t(const itype_t& type, const immer::vector<bc_external_handle_t>& s);
	public: bc_external_value_t(const itype_t& type, const immer::vector<bc_inplace_value_t>& s);
	public: bc_external_value_t(const itype_t& type, const immer::map<std::string, bc_external_handle_t>& s);
	public: bc_external_value_t(const itype_t& type, const immer::map<std::string, bc_inplace_value_t>& s);

#if DEBUG
	public: bool check_invariant() const;
#endif
	public: bool operator==(const bc_external_value_t& other) const;


	//////////////////////////////////////		STATE
	public: mutable std::atomic<int> _rc;
#if DEBUG
	public: bool _debug__is_unwritten_external_value = false;
#endif
#if DEBUG
	public: itype_t _debug_type;
//	public: value_encoding _debug_encoding;
#endif
	public: std::string _string;
	public: std::shared_ptr<json_t> _json;
	public: function_id_t _function_id;
	public: itype_t _typeid_value = itype_t::make_undefined();
	public: std::vector<bc_value_t> _struct_members;
	public: immer::vector<bc_external_handle_t> _vector_w_external_elements;
	public: immer::vector<bc_inplace_value_t> _vector_w_inplace_elements;
	public: immer::map<std::string, bc_external_handle_t> _dict_w_external_values;
	public: immer::map<std::string, bc_inplace_value_t> _dict_w_inplace_values;
};


////////////////////////////////////////////			FREE


const immer::vector<bc_value_t> get_vector(const type_interner_t& interner, const bc_value_t& value);
const immer::vector<bc_external_handle_t>* get_vector_external_elements(const type_interner_t& interner, const bc_value_t& value);
const immer::vector<bc_inplace_value_t>* get_vector_inplace_elements(const type_interner_t& interner, const bc_value_t& value);

bc_value_t make_vector(const type_interner_t& interner, const itype_t& element_type, const immer::vector<bc_value_t>& elements);
bc_value_t make_vector(const type_interner_t& interner, const itype_t& element_type, const immer::vector<bc_external_handle_t>& elements);
bc_value_t make_vector(const type_interner_t& interner, const itype_t& element_type, const immer::vector<bc_inplace_value_t>& elements);

const immer::map<std::string, bc_external_handle_t>& get_dict_external_values(const type_interner_t& interner, const bc_value_t& value);
const immer::map<std::string, bc_inplace_value_t>& get_dict_inplace_values(const type_interner_t& interner, const bc_value_t& value);

bc_value_t make_dict(const type_interner_t& interner, const itype_t& value_type, const immer::map<std::string, bc_external_handle_t>& entries);
bc_value_t make_dict(const type_interner_t& interner, const itype_t& value_type, const immer::map<std::string, bc_inplace_value_t>& entries);

json_t bcvalue_to_json(const type_interner_t& interner, const bc_value_t& v);
int bc_compare_value_true_deep(const type_interner_t& interner, const bc_value_t& left, const bc_value_t& right, const itype_t& type);
int bc_compare_value_exts(const type_interner_t& interner, const bc_external_handle_t& left, const bc_external_handle_t& right, const itype_t& type);



//////////////////////////////////////		bc_symbol_t

/*
	Tracks information about a symbol inside a stack frame.
*/

struct bc_symbol_t {
	enum class type {
		immutable = 10,
		mutable1,
		named_type
	};

	public: bool check_invariant() const {
		QUARK_ASSERT(_const_value._type.is_undefined() || _const_value._type == _value_type);
		return true;
	}


	//////////////////////////////////////		STATE
	type _symbol_type;
	itype_t _value_type;
	floyd::bc_value_t _const_value;
};


//////////////////////////////////////		bc_opcode

/*
	These are the instructions used byte the byte code.
	Each instruction has 3 16-bit fields, called A, B, C. Each opcode has description that tells how A-B-B are used.
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
	k_concat_vectors_w_inplace_elements,

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

	//??? Remove all conditions. Only have conditional branches.
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
#if DEBUG
	public: bool check_invariant() const;
#endif


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
	- What types are those registers are.
	- The instruction
*/
struct bc_static_frame_t {
	bc_static_frame_t(
		const type_interner_t& interner,
		const std::vector<bc_instruction_t>& instrs2,
		const std::vector<std::pair<std::string, bc_symbol_t>>& symbols,
		const std::vector<itype_t>& args
	);
	bool check_invariant() const;


	//////////////////////////////////////		STATE
	std::vector<bc_instruction_t> _instructions;

	//??? Optimize how we store this data for quick access + compactness.
	std::vector<std::pair<std::string, bc_symbol_t>> _symbols;
	std::vector<itype_t> _args;

	//	True if equivalent symbol is an external value.
	//??? unify with _locals_exts.
	//??? also redundant with _symbols._value_type
	std::vector<bool> _exts;

	//	Is the local value external values?
	//	This doesn't count arguments.
	std::vector<bool> _locals_exts;
	std::vector<bc_value_t> _locals;
};


//////////////////////////////////////		bc_function_definition_t

/*
	Represents a function, ready to execute.
	The interpreter doesn't use a flat list of instructions for programs, rather a list of functions.
*/

struct bc_function_definition_t {
	bc_function_definition_t(
		const type_interner_t& interner, 
		const itype_t& function_type,
		const std::vector<member_t>& args,
		const std::shared_ptr<bc_static_frame_t>& frame,
		function_id_t function_id
	);
#if DEBUG
	public: bool check_invariant() const;
#endif


	//////////////////////////////////////		STATE
	itype_t _function_type;
	std::vector<member_t> _args;
	std::shared_ptr<bc_static_frame_t> _frame_ptr;
	function_id_t _function_id;

	int _dyn_arg_count;
	bool _return_is_ext;
};


//////////////////////////////////////		bc_program_t

/*
	A complete, stand-alone, Floyd byte code executable, ready to be executed by interpreter.
*/

struct bc_program_t {
#if DEBUG
	public: bool check_invariant() const {
//			QUARK_ASSERT(_globals.check_invariant());
		return true;
	}
#endif


	//////////////////////////////////////		STATE
	public: const bc_static_frame_t _globals;
	public: std::map<function_id_t, bc_function_definition_t> _function_defs;
	public: type_interner_t _types;
	public: software_system_t _software_system;
	public: container_t _container_def;
};

json_t bcprogram_to_json(const bc_program_t& program);


//////////////////////////////////////		frame_pos_t


//	Keeps track where in the interpreter's stack we're executing code right now.
struct frame_pos_t {
	int _frame_pos;
	const bc_static_frame_t* _frame_ptr;
};

inline bool operator==(const frame_pos_t& lhs, const frame_pos_t& rhs){
	return lhs._frame_pos == rhs._frame_pos && lhs._frame_ptr == rhs._frame_ptr;
}


//////////////////////////////////////		interpreter_stack_t

/*
	The interpreters's stack -- each element contains a bc_pod_value_t.
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

struct interpreter_stack_t {
	public: interpreter_stack_t(const bc_static_frame_t* global_frame) :
		_current_frame_ptr(nullptr),
		_current_frame_entry_ptr(nullptr),
		_global_frame(global_frame),
		_entries(nullptr),
		_allocated_count(0),
		_stack_size(0)
	{
		_entries = new bc_pod_value_t[8192];
		_allocated_count = 8192;
		_current_frame_entry_ptr = &_entries[0];

		QUARK_ASSERT(check_invariant());
	}

	public: ~interpreter_stack_t(){
		QUARK_ASSERT(check_invariant());

		delete[] _entries;
		_entries = nullptr;
	}

	public: bool check_invariant() const {
		QUARK_ASSERT(_entries != nullptr);
		QUARK_ASSERT(_stack_size >= 0 && _stack_size <= _allocated_count);

		QUARK_ASSERT(_current_frame_entry_ptr >= &_entries[0]);

		QUARK_ASSERT(_debug_types.size() == _stack_size);
#if 0
		for(int i = 0 ; i < _stack_size ; i++){
			QUARK_ASSERT(_debug_types[i].check_invariant());
		}
#endif
		return true;
	}

	public: interpreter_stack_t(const interpreter_stack_t& other) = delete;
	public: interpreter_stack_t& operator=(const interpreter_stack_t& other) = delete;

	public: void swap(interpreter_stack_t& other) throw() {
		QUARK_ASSERT(check_invariant());
		QUARK_ASSERT(other.check_invariant());

		std::swap(other._entries, _entries);
		std::swap(other._allocated_count, _allocated_count);
		std::swap(other._stack_size, _stack_size);
#if DEBUG
		other._debug_types.swap(_debug_types);
#endif
		std::swap(_current_frame_ptr, other._current_frame_ptr);
		std::swap(_current_frame_entry_ptr, other._current_frame_entry_ptr);

		std::swap(_global_frame, other._global_frame);

		QUARK_ASSERT(check_invariant());
		QUARK_ASSERT(other.check_invariant());
	}

	public: inline int size() const {
		QUARK_ASSERT(check_invariant());

		return static_cast<int>(_stack_size);
	}


	//////////////////////////////////////		GLOBAL VARIABLES


	public: bool check_global_access_obj(const int global_index) const{
		QUARK_ASSERT(check_invariant());
		QUARK_ASSERT(global_index >= 0 && global_index < (k_frame_overhead + _global_frame->_symbols.size()));
		QUARK_ASSERT(_global_frame->_exts[global_index] == true);
		return true;
	}
	public: bool check_global_access_intern(const int global_index) const{
		QUARK_ASSERT(check_invariant());
		QUARK_ASSERT(global_index >= 0 && global_index < (k_frame_overhead + _global_frame->_symbols.size()));
		QUARK_ASSERT(_global_frame->_exts[global_index] == false);
		return true;
	}


	//////////////////////////////////////		FRAMES & REGISTERS


	private: frame_pos_t read_prev_frame(int frame_pos) const;
#if DEBUG
	public: bool check_stack_frame(const frame_pos_t& in_frame) const;
#endif

	//	??? This function should just allocate a block for frame, then have a list of writes.
	//	???	ALTERNATIVELY: generate instructions to do this in the VM? Nah, that's always slower.
	public: void open_frame(const bc_static_frame_t& frame, int values_already_on_stack){
		QUARK_ASSERT(check_invariant());
		QUARK_ASSERT(frame.check_invariant());
		QUARK_ASSERT(values_already_on_stack == frame._args.size());

		const auto stack_end = size();
		const auto parameter_count = static_cast<int>(frame._args.size());

		//	Carefully position the new stack frame so its includes the parameters that already sits in the stack.
		//	The stack frame already has symbols/registers mapped for those parameters.
		const auto new_frame_pos = stack_end - parameter_count;

		for(int i = 0 ; i < frame._locals.size() ; i++){
			bool ext = frame._locals_exts[i];
			const auto& local = frame._locals[i];
			if(ext){
				push_external_value(local);
			}
			else{
				push_inplace_value(local);
			}
		}
		_current_frame_ptr = &frame;
		_current_frame_entry_ptr = &_entries[new_frame_pos];
	}


	//	Pops all locals, decrementing RC when needed.
	//	Decrements all stack frame object RCs.
	//	Caller handles RC for parameters, this function don't.
	public: void close_frame(const bc_static_frame_t& frame){
		QUARK_ASSERT(check_invariant());
		QUARK_ASSERT(frame.check_invariant());

		//	Using symbol table to figure out which stack-frame values needs RC. Decrement them all.
		pop_batch(frame._locals_exts);
	}

	public: std::vector<std::pair<int, int>> get_stack_frames(int frame_pos) const;

	public: bool check_reg(int reg) const{
		//	Makes sure register is within current stack frame bounds.
		QUARK_ASSERT(reg >= 0 && reg < _current_frame_ptr->_symbols.size());

		//	Makes sure debug types are in sync for this register.
		QUARK_ASSERT(_current_frame_ptr->_symbols[reg].second._value_type == _debug_types[get_current_frame_start() + reg]);
		return true;
	}

	//	Slow since it looks up the type of the register at runtime.
	public: bc_value_t read_register(const int reg) const{
		QUARK_ASSERT(check_invariant());
		QUARK_ASSERT(check_reg(reg));

//			bool is_ext = _current_frame_ptr->_exts[reg];
#if DEBUG
		const auto result = bc_value_t(_current_frame_ptr->_symbols[reg].second._value_type, _current_frame_entry_ptr[reg]);
#else
//			const auto result = bc_value_t(_current_frame_entry_ptr[reg], is_ext);
		const auto result = bc_value_t(_current_frame_ptr->_symbols[reg].second._value_type, _current_frame_entry_ptr[reg]);
#endif
		QUARK_ASSERT(result.check_invariant());
		return result;
	}

	public: void write_register(const int reg, const bc_value_t& value){
		QUARK_ASSERT(check_invariant());
		QUARK_ASSERT(check_reg(reg));
		QUARK_ASSERT(value.check_invariant());

		bool is_ext = _current_frame_ptr->_exts[reg];
		if(is_ext){
			auto prev_copy = _current_frame_entry_ptr[reg];
			value._pod._external->_rc++;
			_current_frame_entry_ptr[reg] = value._pod;
			release_pod_external(prev_copy);
		}
		else{
			_current_frame_entry_ptr[reg] = value._pod;
		}

		QUARK_ASSERT(check_invariant());
	}

	public: void write_register__external_value(const int reg, const bc_value_t& value){
		QUARK_ASSERT(check_invariant());
		QUARK_ASSERT(encode_as_external(value._type));
		QUARK_ASSERT(check_reg__external_value(reg));
		QUARK_ASSERT(value.check_invariant());
		QUARK_ASSERT(_current_frame_ptr->_symbols[reg].second._value_type == value._type);

		auto prev_copy = _current_frame_entry_ptr[reg];
		value._pod._external->_rc++;
		_current_frame_entry_ptr[reg] = value._pod;
		release_pod_external(prev_copy);

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
		QUARK_ASSERT(_current_frame_ptr->_symbols[reg].second._value_type.is_bool());
		return true;
	}

	public: bool check_reg_int(const int reg) const{
		QUARK_ASSERT(check_invariant());
		QUARK_ASSERT(check_reg(reg));
		QUARK_ASSERT(_current_frame_ptr->_symbols[reg].second._value_type.is_int());
		return true;
	}

	public: bool check_reg_double(const int reg) const{
		QUARK_ASSERT(check_invariant());
		QUARK_ASSERT(check_reg(reg));
		QUARK_ASSERT(_current_frame_ptr->_symbols[reg].second._value_type.is_double());
		return true;
	}

	public: bool check_reg_string(const int reg) const{
		QUARK_ASSERT(check_invariant());
		QUARK_ASSERT(check_reg(reg));
		QUARK_ASSERT(_current_frame_ptr->_symbols[reg].second._value_type.is_string());
		return true;
	}

	public: bool check_reg_json(const int reg) const{
		QUARK_ASSERT(check_invariant());
		QUARK_ASSERT(check_reg(reg));
		QUARK_ASSERT(_current_frame_ptr->_symbols[reg].second._value_type.is_json());
		return true;
	}

	public: bool check_reg_vector_w_external_elements(const type_interner_t& interner, const int reg) const{
		QUARK_ASSERT(check_invariant());
		QUARK_ASSERT(interner.check_invariant());
		QUARK_ASSERT(check_reg(reg));
		QUARK_ASSERT(_current_frame_ptr->_symbols[reg].second._value_type.is_vector());
		QUARK_ASSERT(encode_as_vector_w_inplace_elements(interner, _current_frame_ptr->_symbols[reg].second._value_type) == false);
		return true;
	}

	public: bool check_reg_vector_w_inplace_elements(const type_interner_t& interner, const int reg) const{
		QUARK_ASSERT(check_invariant());
		QUARK_ASSERT(interner.check_invariant());
		QUARK_ASSERT(check_reg(reg));
		QUARK_ASSERT(_current_frame_ptr->_symbols[reg].second._value_type.is_vector());
		QUARK_ASSERT(encode_as_vector_w_inplace_elements(interner, _current_frame_ptr->_symbols[reg].second._value_type) == true);
		return true;
	}

	public: bool check_reg_dict_w_external_values(const type_interner_t& interner, const int reg) const{
		QUARK_ASSERT(check_invariant());
		QUARK_ASSERT(interner.check_invariant());
		QUARK_ASSERT(check_reg(reg));
		QUARK_ASSERT(_current_frame_ptr->_symbols[reg].second._value_type.is_dict());
		QUARK_ASSERT(encode_as_dict_w_inplace_values(interner, _current_frame_ptr->_symbols[reg].second._value_type) == false);
		return true;
	}
	public: bool check_reg_dict_w_inplace_values(const type_interner_t& interner, const int reg) const{
		QUARK_ASSERT(check_invariant());
		QUARK_ASSERT(interner.check_invariant());
		QUARK_ASSERT(check_reg(reg));
		QUARK_ASSERT(_current_frame_ptr->_symbols[reg].second._value_type.is_dict());
		QUARK_ASSERT(encode_as_dict_w_inplace_values(interner, _current_frame_ptr->_symbols[reg].second._value_type) == true);
		return true;
	}

	public: bool check_reg_struct(const int reg) const{
		QUARK_ASSERT(check_invariant());
		QUARK_ASSERT(check_reg(reg));
		QUARK_ASSERT(_current_frame_ptr->_symbols[reg].second._value_type.is_struct());
		return true;
	}

	public: bool check_reg_function(const int reg) const{
		QUARK_ASSERT(check_invariant());
		QUARK_ASSERT(check_reg(reg));
		QUARK_ASSERT(_current_frame_ptr->_symbols[reg].second._value_type.is_function());
		return true;
	}

	public: bool check_reg__external_value(const int reg) const{
		QUARK_ASSERT(check_invariant());
		QUARK_ASSERT(check_reg(reg));
		QUARK_ASSERT(_current_frame_ptr->_exts[reg] == true);
		return true;
	}

	public: bool check_reg__inplace_value(const int reg) const{
		QUARK_ASSERT(check_invariant());
		QUARK_ASSERT(check_reg(reg));
		QUARK_ASSERT(_current_frame_ptr->_exts[reg] == false);
		return true;
	}
#endif

	public: void save_frame(){
		const auto frame_pos = bc_value_t::make_int(get_current_frame_start());
		push_inplace_value(frame_pos);

		const auto frame_ptr = bc_value_t(_current_frame_ptr);
		push_inplace_value(frame_ptr);
	}

	public: void restore_frame(){
		QUARK_ASSERT(check_invariant());
		QUARK_ASSERT(_stack_size >= k_frame_overhead);

		const auto frame_pos = _entries[_stack_size - k_frame_overhead + 0]._inplace.int64_value;
		const auto frame_ptr = _entries[_stack_size - k_frame_overhead + 1]._inplace._frame_ptr;
		_stack_size -= k_frame_overhead;
#if DEBUG
		_debug_types.pop_back();
		_debug_types.pop_back();
#endif
		_current_frame_ptr = frame_ptr;
		_current_frame_entry_ptr = &_entries[frame_pos];
	}



	//////////////////////////////////////		STACK


	public: inline void push_external_value(const bc_value_t& value){
		QUARK_ASSERT(check_invariant());
		QUARK_ASSERT(value.check_invariant());
#if DEBUG
		QUARK_ASSERT(encode_as_external(value._type) == true);
#endif

		value._pod._external->_rc++;
		_entries[_stack_size] = value._pod;
		_stack_size++;
#if DEBUG
		_debug_types.push_back(value._type);
#endif

		QUARK_ASSERT(check_invariant());
	}

	public: inline void push_inplace_value(const bc_value_t& value){
		QUARK_ASSERT(check_invariant());
		QUARK_ASSERT(value.check_invariant());
#if DEBUG
		QUARK_ASSERT(encode_as_external(value._type) == false);
#endif

		_entries[_stack_size] = value._pod;
		_stack_size++;
#if DEBUG
		_debug_types.push_back(value._type);
#endif

		QUARK_ASSERT(check_invariant());
	}

	//	returned value will have ownership of obj, if it's an obj.
	public: inline bc_value_t load_value(int pos, const itype_t& type) const{
		QUARK_ASSERT(check_invariant());
		QUARK_ASSERT(pos >= 0 && pos < _stack_size);
		QUARK_ASSERT(type.check_invariant());
		QUARK_ASSERT(type == _debug_types[pos]);

		const auto& e = _entries[pos];
		const auto result = bc_value_t(type, e);
		return result;
	}

	public: inline int64_t load_intq(int pos) const{
		QUARK_ASSERT(check_invariant());
		QUARK_ASSERT(pos >= 0 && pos < _stack_size);
		QUARK_ASSERT(_debug_types[pos].is_int());

		return _entries[pos]._inplace.int64_value;
	}

	public: inline void replace_inplace_value(int pos, const bc_value_t& value){
		QUARK_ASSERT(check_invariant());
		QUARK_ASSERT(value.check_invariant());
		QUARK_ASSERT(pos >= 0 && pos < _stack_size);
#if FLOYD_BC_VALUE_DEBUG_TYPE
		QUARK_ASSERT(encode_as_external(value._type.get_base_type()) == false);
#endif
		QUARK_ASSERT(_debug_types[pos] == value._type);

		_entries[pos] = value._pod;

		QUARK_ASSERT(check_invariant());
	}

	public: inline void replace_external_value(int pos, const bc_value_t& value){
		QUARK_ASSERT(check_invariant());
		QUARK_ASSERT(value.check_invariant());
		QUARK_ASSERT(pos >= 0 && pos < _stack_size);
#if FLOYD_BC_VALUE_DEBUG_TYPE
		QUARK_ASSERT(encode_as_external(value._type.get_base_type()) == true);
#endif
		QUARK_ASSERT(_debug_types[pos] == value._type);

		auto prev_copy = _entries[pos];
		value._pod._external->_rc++;
		_entries[pos] = value._pod;
		release_pod_external(prev_copy);

		QUARK_ASSERT(check_invariant());
	}

	//	exts[exts.size() - 1] maps to the closed value on stack, the next to be popped.
	public: inline void pop_batch(const std::vector<bool>& exts){
		QUARK_ASSERT(check_invariant());
		QUARK_ASSERT(_stack_size >= exts.size());

		auto flag_index = exts.size() - 1;
		for(int i = 0 ; i < exts.size() ; i++){
			pop(exts[flag_index]);
			flag_index--;
		}
		QUARK_ASSERT(check_invariant());
	}

	private: inline void pop(bool ext){
		QUARK_ASSERT(check_invariant());
		QUARK_ASSERT(_stack_size > 0);
		QUARK_ASSERT(encode_as_external(_debug_types.back()) == ext);

		auto copy = _entries[_stack_size - 1];
		_stack_size--;
#if DEBUG
		_debug_types.pop_back();
#endif
		if(ext){
			release_pod_external(copy);
		}

		QUARK_ASSERT(check_invariant());
	}

#if DEBUG
	private: bool debug_is_ext(int pos) const{
		QUARK_ASSERT(check_invariant());
		QUARK_ASSERT(pos >= 0 && pos < _stack_size);
		return encode_as_external(_debug_types[pos]);
	}
#endif

	public: int get_current_frame_start() const {
		QUARK_ASSERT(check_invariant());

		const auto frame_pos = _current_frame_entry_ptr - &_entries[0];
		return static_cast<int>(frame_pos);
	}

	public: json_t stack_to_json(const type_interner_t& interner) const;


	////////////////////////		STATE

	public: bc_pod_value_t* _entries;
	public: size_t _allocated_count;
	public: size_t _stack_size;

	//	These are DEEP copies = do not share RC with non-debug values.
#if DEBUG
	public: std::vector<itype_t> _debug_types;
#endif

	public: const bc_static_frame_t* _current_frame_ptr;
	public: bc_pod_value_t* _current_frame_entry_ptr;

	public: const bc_static_frame_t* _global_frame;
};



//////////////////////////////////////		interpreter_imm_t

//	Holds static = immutable state the interpreter wants to keep around.

struct interpreter_imm_t {
	public: const std::chrono::time_point<std::chrono::high_resolution_clock> _start_time;
	public: const bc_program_t _program;
	public: const std::map<function_id_t, BC_NATIVE_FUNCTION_PTR> _native_functions;
};


//////////////////////////////////////		value_entry_t

//	Allows the interpreter's clients to access values in the interpreter.

struct value_entry_t {
	bool check_invariant() const {
		QUARK_ASSERT(_value.check_invariant());
		QUARK_ASSERT(_symbol_name.empty() == false);
		QUARK_ASSERT(_symbol.check_invariant());
		QUARK_ASSERT(_global_index >= 0);
		return true;
	}

	bc_value_t _value;
	std::string _symbol_name;
	bc_symbol_t _symbol;
	int _global_index;
};


//////////////////////////////////////		interpreter_t

/*
	Complete runtime state of the interpreter.
	MUTABLE
	Non-copyable.
*/

struct interpreter_t {
	public: explicit interpreter_t(const bc_program_t& program);
	public: explicit interpreter_t(const bc_program_t& program, runtime_handler_i* handler);
	public: interpreter_t(const interpreter_t& other) = delete;
	public: const interpreter_t& operator=(const interpreter_t& other)= delete;
#if DEBUG
	public: bool check_invariant() const;
#endif
	public: void swap(interpreter_t& other) throw();


	////////////////////////		STATE
	public: std::shared_ptr<interpreter_imm_t> _imm;
	public: runtime_handler_i* _handler;

	//	Holds all values for all environments.
	//	Notice: stack holds refs to RC-counted objects!
	public: interpreter_stack_t _stack;
	public: std::vector<std::string> _print_output;
};


//////////////////////////////////////		Free functions


int get_global_n_pos(int n);

bc_value_t call_function_bc(interpreter_t& vm, const bc_value_t& f, const bc_value_t args[], int arg_count);
json_t interpreter_to_json(const interpreter_t& vm);
std::pair<bc_typeid_t, bc_value_t> execute_instructions(interpreter_t& vm, const std::vector<bc_instruction_t>& instructions);

std::shared_ptr<value_entry_t> find_global_symbol2(const interpreter_t& vm, const std::string& s);

bc_value_t update_element(interpreter_t& vm, const bc_value_t& obj1, const bc_value_t& lookup_key, const bc_value_t& new_value);


} //	floyd


#endif /* bytecode_interpreter_hpp */
