//
//  parser_evaluator.cpp
//  Floyd
//
//  Created by Marcus Zetterquist on 26/07/16.
//  Copyright © 2016 Marcus Zetterquist. All rights reserved.
//

/*
	OVERVIEW OF IMPLEMENTATION

	bcgen_scope_t -- holds the symbols and instructions for
	- Global state
	- a function
	- a sub-block of a function
	It is *hiearchical*.

	The functions that generate code all use a bcgen_scope_t& body_acc that holds the current state of a body and where new data is *acc*umulated / added.
	This means that only the body_acc's state is changed, not global state.
	This makes it possible to generate code for an IF-statement's THEN and ELSE bodies first, count their instructions,
	then setup branches and insert the THEN and ELSE instructions.

	bcgen_scope_t and bcgen_instruction_t are used together and forms a tree that matches the lexical scopes of the source code.
	The instructions can reference symbols anywhere up the lexical branch it sits in.
	Then this tree is flattened into one frame for each function and one global frame and instructions are changed to access either function scope or global scope.

	??? IDEA: split codegen and flattening into two passes.

	??? Idea: if zero or few arguments, use shortcuts. Stuff single arg into instruction reg_c etc.
	??? Idea: Do interning of generate_local_const().
	??? Idea: make different types for register vs stack-pos.
*/

#include "bytecode_generator.h"

#include "bytecode_interpreter.h"
#include "floyd_runtime.h"
#include "compiler_basics.h"
#include "semantic_ast.h"
#include "types.h"

#include <cmath>
#include <algorithm>
#include <cstdint>

const auto trace_io_flag = false;

namespace floyd {

struct semantic_ast_t;



//////////////////////////////////////		bcgen_instruction_t

/*
	A temporary, bigger, representation of instructions. Each register can reference the current scope or any parent scope (like the globals).
	At and of codegen, explicit instructions have been generated for locals and global accesses and each reg-slot no longer points to scopes. squeeze_instruction().
*/

struct gen_instruction_t {
	gen_instruction_t(
		bc_opcode opcode,
		symbol_pos_t regA,
		symbol_pos_t regB,
		symbol_pos_t regC
	) :
		_opcode(opcode),
		_field_a(regA),
		_field_b(regB),
		_field_c(regC)
	{
		QUARK_ASSERT(check_invariant());
	}

#if DEBUG
	public: bool check_invariant() const;
#endif

	//////////////////////////////////////		STATE
	bc_opcode _opcode;

	//	Notice: the fields can hold a reference to a symbol or an immediate value. Immediate values are stored in symbol_pos_t::_index.
	symbol_pos_t _field_a;
	symbol_pos_t _field_b;
	symbol_pos_t _field_c;
};

//??? TODO: Use std::variant<symbol_pos_t, uin16_t > as field data type.
//	Magic constant to see a symbol_pos_t doesn't hold a symbol reference, but an immediate value.
static const int k_immediate_value__parent_steps = 666;



//////////////////////////////////////		bcgen_scope_t


//	Used only during codegen - it supports hiearchical scopes and uses fat bcgen_instruction_t.

struct gen_scope_t {
	gen_scope_t(const std::vector<gen_instruction_t>& s) :
		_instrs(s),
		_symbol_table{}
	{
		QUARK_ASSERT(check_invariant());
	}

	gen_scope_t(const std::vector<gen_instruction_t>& instructions, const symbol_table_t& symbols) :
		_instrs(instructions),
		_symbol_table(symbols)
	{
		QUARK_ASSERT(check_invariant());
	}

	public: bool check_invariant() const;


	////////////////////////		STATE
	symbol_table_t _symbol_table;
	std::vector<gen_instruction_t> _instrs;
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


	//////////////////////////////////////		STATE
	public: std::shared_ptr<semantic_ast_t> _ast_imm;

	public: gen_scope_t _globals;
//	public: std::vector<bcgen_scope_t> _scope_stack;
};



//////////////////////////////////////		gen_expr_out_t

//	Why: needed to return from codegen functions that process expressions.
//	Any new instructions have been recorded into _body.

struct gen_expr_out_t {

	//////////////////////////////////////		STATE
	gen_scope_t _body;

	symbol_pos_t _out;

	//	Output type.
	type_t _type;
};


//////////////////////////////////////		FORWARD

/*
	target_reg: if defined, this is where the output value will be stored. If undefined,
	then the expression allocates (or redirect to existing register).
	gen_expr_out_t._out: always holds the output register, no matter who decided it.
*/
static gen_expr_out_t generate_expression(
	bcgenerator_t& gen_acc,
	const symbol_pos_t& target_reg,
	const expression_t& e,
	const gen_scope_t& body
);

static gen_scope_t generate_block(bcgenerator_t& gen_acc, const lexical_scope_t& body);

static gen_expr_out_t generate_call_expression(
	bcgenerator_t& gen_acc,
	const symbol_pos_t& target_reg,
	const type_t& call_output_type,
	const expression_t::call_t& details,
	const gen_scope_t& body
);

static type_t dummy_func(const types_t& types, const type_t& type){ return type; }




static symbol_pos_t generate_local_temp(const types_t& types, gen_scope_t& body_acc, const type_t& type0, const std::string& name){
	QUARK_ASSERT(types.check_invariant());
	QUARK_ASSERT(body_acc.check_invariant());
	QUARK_ASSERT(type0.check_invariant());
	QUARK_ASSERT(name.empty() == false);

	const auto s = symbol_t::make_immutable_reserve(type0);
	body_acc._symbol_table._symbols.push_back(std::pair<std::string, symbol_t>(name, s));
	int id = static_cast<int>(body_acc._symbol_table._symbols.size() - 1);

	return symbol_pos_t::make_stack_pos(0, id);
}

static symbol_pos_t generate_local_const(const types_t& types, gen_scope_t& body_acc, const value_t& value, const std::string& name){
	QUARK_ASSERT(types.check_invariant());
	QUARK_ASSERT(body_acc.check_invariant());
	QUARK_ASSERT(value.check_invariant());
	QUARK_ASSERT(name.empty() == false);

	const auto s = symbol_t::make_immutable_precalc(value.get_type(), value);
	body_acc._symbol_table._symbols.push_back(std::pair<std::string, symbol_t>(name, s));
	const auto id = static_cast<int>(body_acc._symbol_table._symbols.size() - 1);
	return symbol_pos_t::make_stack_pos(0, id);
}



//////////////////////////////////////		INSTRUCTION DETAILS



//	Use to stuff an integer into an instruction, in one of the register slots.
static symbol_pos_t make_imm_int_field(int value){
	return symbol_pos_t::make_stack_pos(k_immediate_value__parent_steps, value);
}

static bc_typeid_t itype_from_type(const type_t& type){
	QUARK_ASSERT(type.check_invariant());

	const auto r = type.get_lookup_index();
	QUARK_ASSERT(r <= INT16_MAX);

	QUARK_ASSERT(sizeof(bc_typeid_t) == sizeof(int16_t));
	return static_cast<bc_typeid_t>(r);
}

static bool check_field(const symbol_pos_t& reg, bool is_reg){
	if(is_reg){
		QUARK_ASSERT(reg._parent_steps != k_immediate_value__parent_steps);
	}
	else{
		QUARK_ASSERT(reg._parent_steps == k_immediate_value__parent_steps || reg.is_empty());
	}
	return true;
}

static bool check_field_nonlocal(const symbol_pos_t& reg, bool is_reg){
	if(is_reg){
		//	Must not be a global -- those should have been turned to global-access opcodes.
		QUARK_ASSERT(reg._parent_steps != symbol_pos_t::k_global_scope);
	}
	else{
		QUARK_ASSERT(reg._parent_steps == k_immediate_value__parent_steps || reg.is_empty());
	}
	return true;
}

static bool check_field__local(const symbol_pos_t& reg, bool is_reg){
	if(is_reg){
		//	Must not be a global -- those should have been turned to global-access opcodes.
		QUARK_ASSERT(reg._parent_steps != symbol_pos_t::k_global_scope);
	}
	else{
		QUARK_ASSERT(reg._parent_steps == k_immediate_value__parent_steps || reg.is_empty());
	}
	return true;
}


//////////////////////////////////////		bcgen_scope_t


 bool gen_scope_t::check_invariant() const {
	for(int i = 0 ; i < _instrs.size() ; i++){
		const auto instruction = _instrs[i];
		QUARK_ASSERT(instruction.check_invariant());
	}
	for(const auto& e: _instrs){
		const auto encoding = k_opcode_info.at(e._opcode)._encoding;
		const auto reg_flags = encoding_to_reg_flags(encoding);

		QUARK_ASSERT(check_field_nonlocal(e._field_a, reg_flags._a));
		QUARK_ASSERT(check_field_nonlocal(e._field_b, reg_flags._b));
		QUARK_ASSERT(check_field_nonlocal(e._field_c, reg_flags._c));
	}
	for(const auto& e: _symbol_table._symbols){
		QUARK_ASSERT(e.first != "");
		QUARK_ASSERT(e.second.check_invariant());
	}
	return true;
}


//////////////////////////////////////		bcgen_instruction_t


#if DEBUG
bool gen_instruction_t::check_invariant() const {
	const auto encoding = k_opcode_info.at(_opcode)._encoding;
	const auto reg_flags = encoding_to_reg_flags(encoding);

	QUARK_ASSERT(check_field(_field_a, reg_flags._a));
	QUARK_ASSERT(check_field(_field_b, reg_flags._b));
	QUARK_ASSERT(check_field(_field_c, reg_flags._c));
	return true;
}
#endif


//////////////////////////////////////		FLATTEN


static symbol_pos_t flatten_field(const symbol_pos_t& r, int offset){
	QUARK_ASSERT(r.check_invariant());

	if(r._parent_steps == k_immediate_value__parent_steps){
		return r;
	}
	else if(r._parent_steps == 0){
		return symbol_pos_t::make_stack_pos(0, r._index + offset);
	}
	else if(r._parent_steps == symbol_pos_t::k_global_scope){
		return r;
	}
	else if(r._parent_steps == symbol_pos_t::k_intrinsic){
		return r;
	}
	else{
		return symbol_pos_t::make_stack_pos(r._parent_steps - 1, r._index);
	}
}
static gen_scope_t flatten_scope(const bcgenerator_t& gen, const gen_scope_t& dest, const gen_scope_t& source){
	QUARK_ASSERT(gen.check_invariant());
	QUARK_ASSERT(dest.check_invariant());
	QUARK_ASSERT(source.check_invariant());

	std::vector<gen_instruction_t> instructions;
	int offset = static_cast<int>(dest._symbol_table._symbols.size());
	for(int i = 0 ; i < source._instrs.size() ; i++){
		//	Decrese parent-step for all local register accesses.
		auto s = source._instrs[i];

		const auto encoding = k_opcode_info.at(s._opcode)._encoding;
		const auto reg_flags = encoding_to_reg_flags(encoding);

		//	Make a new instructions with registered offsetted for flattening. Only offset registrers, not when used as immediates.
		const auto s2 = gen_instruction_t(
			s._opcode,
			reg_flags._a ? flatten_field(s._field_a, offset) : s._field_a,
			reg_flags._b ? flatten_field(s._field_b, offset) : s._field_b,
			reg_flags._c ? flatten_field(s._field_c, offset) : s._field_c
		);
		instructions.push_back(s2);
	}

	auto body_acc = dest;
	body_acc._instrs.insert(body_acc._instrs.end(), instructions.begin(), instructions.end());
	body_acc._symbol_table._symbols.insert(body_acc._symbol_table._symbols.end(), source._symbol_table._symbols.begin(), source._symbol_table._symbols.end());
	return body_acc;
}


//////////////////////////////////////		copy_value
//	Supports globals & locals both as dest and sources.

static gen_scope_t copy_value(const bcgenerator_t& gen, const type_t& type, const symbol_pos_t& dest_reg, const symbol_pos_t& source_reg, const gen_scope_t& body){
	QUARK_ASSERT(gen.check_invariant());
	QUARK_ASSERT(type.check_invariant());
	QUARK_ASSERT(dest_reg.check_invariant());
	QUARK_ASSERT(source_reg.check_invariant());
	QUARK_ASSERT(body.check_invariant());

	auto body_acc = body;
	const auto& types = gen._ast_imm->_tree._types;
	bool is_ext = is_rc_value(types, type);

	//	If this asserts, we should special-case and do nothing.
	QUARK_ASSERT(!(dest_reg == source_reg));


	//	global <= global
	if(dest_reg._parent_steps == symbol_pos_t::k_global_scope && source_reg._parent_steps == symbol_pos_t::k_global_scope){
		QUARK_ASSERT(false);
		quark::throw_exception();
//		body_acc._instrs.push_back(bcgen_instruction_t(is_ext ? bc_opcode::k_load_global_external_value : bc_opcode::k_load_global_inplace_value, dest_reg, make_imm_int_field(source_reg._index), {}));
	}

	//	global <= local
	else if(dest_reg._parent_steps == symbol_pos_t::k_global_scope && source_reg._parent_steps != symbol_pos_t::k_global_scope){
		body_acc._instrs.push_back(gen_instruction_t(is_ext ? bc_opcode::k_store_global_external_value : bc_opcode::k_store_global_inplace_value, make_imm_int_field(dest_reg._index), source_reg, {}));
	}
	//	local <= global
	else if(dest_reg._parent_steps != symbol_pos_t::k_global_scope && source_reg._parent_steps == symbol_pos_t::k_global_scope){
		body_acc._instrs.push_back(gen_instruction_t(is_ext ? bc_opcode::k_load_global_external_value : bc_opcode::k_load_global_inplace_value, dest_reg, make_imm_int_field(source_reg._index), {}));
	}
	//	local <= local
	else{
		body_acc._instrs.push_back(gen_instruction_t(is_ext ? bc_opcode::k_copy_reg_external_value : bc_opcode::k_copy_reg_inplace_value, dest_reg, source_reg, {}));
	}

	QUARK_ASSERT(body_acc.check_invariant());
	return body_acc;
}


//////////////////////////////////////		PROCESS STATEMENTS


//??? need logic that knows that globals can be treated as locals for instructions in global scope.

static gen_scope_t generate_assign2_statement(bcgenerator_t& gen_acc, const statement_t::assign2_t& statement, const gen_scope_t& body){
	QUARK_ASSERT(gen_acc.check_invariant());
	QUARK_ASSERT(body.check_invariant());

	auto body_acc = body;

	//	Shortcut: if destination is a local variable, have the expression write directly to that register.
	if(statement._dest_variable._parent_steps != symbol_pos_t::k_global_scope){
		const auto expr = generate_expression(gen_acc, statement._dest_variable, statement._expression, body_acc);
		body_acc = expr._body;
		QUARK_ASSERT(body_acc.check_invariant());
	}
	else{
		const auto expr = generate_expression(gen_acc, {}, statement._expression, body_acc);
		body_acc = expr._body;
		body_acc = copy_value(gen_acc, statement._expression.get_output_type(), statement._dest_variable, expr._out, body_acc);
		QUARK_ASSERT(body_acc.check_invariant());
	}
	return body_acc;
}


#if 1
static gen_scope_t generate_init2_statement(bcgenerator_t& gen_acc, const statement_t::init2_t& statement, const gen_scope_t& body){
	QUARK_ASSERT(gen_acc.check_invariant());
	QUARK_ASSERT(body.check_invariant());

	//	Treat init2 just like assign2.
	const auto temp = statement_t::assign2_t{ statement._dest_variable, statement._expression };
	return generate_assign2_statement(gen_acc, temp, body);
}
#else
static bcgen_scope_t generate_init2_statement(bcgenerator_t& gen_acc, const statement_t::init2_t& statement, const bcgen_scope_t& body){
	QUARK_ASSERT(gen_acc.check_invariant());
	QUARK_ASSERT(body.check_invariant());

	auto body_acc = body;

	const auto expr = bcgen_expression(gen_acc, {}, statement._expression, body_acc);
	body_acc = expr._body;

	body_acc._instrs.push_back(
		bcgen_instruction_t(bc_opcode::k_init_local, statement._dest_variable, expr._out, {})
	);

	QUARK_ASSERT(body_acc.check_invariant());
	return body_acc;
}
#endif


static gen_scope_t generate_block_statement(bcgenerator_t& gen_acc, const statement_t::block_statement_t& statement, const gen_scope_t& body){
	QUARK_ASSERT(gen_acc.check_invariant());
	QUARK_ASSERT(body.check_invariant());

	const auto body_acc = generate_block(gen_acc, statement._body);
	return flatten_scope(gen_acc, body, body_acc);
}

static gen_scope_t generate_return_statement(bcgenerator_t& gen_acc, const statement_t::return_statement_t& statement, const gen_scope_t& body){
	QUARK_ASSERT(gen_acc.check_invariant());
	QUARK_ASSERT(body.check_invariant());

	auto body_acc = body;
	const auto expr = generate_expression(gen_acc, {}, statement._expression, body);
	body_acc = expr._body;
	body_acc._instrs.push_back(gen_instruction_t(bc_opcode::k_return, expr._out, {}, {}));
	return body_acc;
}

static gen_scope_t generate_ifelse_statement(bcgenerator_t& gen_acc, const statement_t::ifelse_statement_t& statement, const gen_scope_t& body){
	QUARK_ASSERT(gen_acc.check_invariant());
	QUARK_ASSERT(body.check_invariant());

	auto body_acc = body;

	const auto condition_expr = generate_expression(gen_acc, {}, statement._condition, body_acc);
	body_acc = condition_expr._body;

	const auto& types = gen_acc._ast_imm->_tree._types;
	QUARK_ASSERT(peek2(types, statement._condition.get_output_type()).is_bool());

	//???	Needs short-circuit evaluation here!

	//	Notice that we generate the instructions for then/else but we don't store them in body_acc yet, we
	//	keep them and check their sizes so we can calculate jumps around them.
	const auto& then_expr = generate_block(gen_acc, statement._then_body);
	const auto& else_expr = generate_block(gen_acc, statement._else_body);

	body_acc._instrs.push_back(
		gen_instruction_t(
			bc_opcode::k_branch_false_bool,
			condition_expr._out,
			make_imm_int_field(static_cast<int>(then_expr._instrs.size()) + 2),
			{}
		)
	);
	body_acc = flatten_scope(gen_acc, body_acc, then_expr);
	body_acc._instrs.push_back(
		gen_instruction_t(
			bc_opcode::k_branch_always,
			make_imm_int_field(static_cast<int>(else_expr._instrs.size()) + 1),
			{},
			{}
		)
	);
	body_acc = flatten_scope(gen_acc, body_acc, else_expr);
	return body_acc;
}

static int get_count(const std::vector<gen_instruction_t>& instructions){
	return static_cast<int>(instructions.size());
}

/*
	... previous instructions in input-body

	count_reg = start-expression
	end-expression

	B:
	k_branch_smaller_or_equal_int / k_branch_smaller_int FALSE, A

	FOR-BODY-INSTRUCTIONS

	k_add_int
	k_branch_smaller_or_equal_int / k_branch_smaller_int, B TRUE
	A:
*/
static gen_scope_t generate_for_statement(bcgenerator_t& gen_acc, const statement_t::for_statement_t& statement, const gen_scope_t& body){
	QUARK_ASSERT(gen_acc.check_invariant());
	QUARK_ASSERT(body.check_invariant());

	auto body_acc = body;
	const auto& types = gen_acc._ast_imm->_tree._types;

	const auto existing_instructions = get_count(body._instrs);

	const auto start_expr = generate_expression(gen_acc, {}, statement._start_expression, body_acc);
	body_acc = start_expr._body;

	const auto end_expr = generate_expression(gen_acc, {}, statement._end_expression, body_acc);
	body_acc = end_expr._body;

	const auto const1_reg = generate_local_const(types, body_acc, value_t::make_int(1), "integer 1, to decrement with");

	const auto& loop_body = generate_block(gen_acc, statement._body);
	int body_instr_count = get_count(loop_body._instrs);

	QUARK_ASSERT(
		statement._range_type == statement_t::for_statement_t::k_closed_range
		|| statement._range_type ==statement_t::for_statement_t::k_open_range
	);
	const auto condition_opcode1 = statement._range_type == statement_t::for_statement_t::k_closed_range ? bc_opcode::k_branch_smaller_int : bc_opcode::k_branch_smaller_or_equal_int;
	const auto condition_opcode2 = statement._range_type == statement_t::for_statement_t::k_closed_range ? bc_opcode::k_branch_smaller_or_equal_int : bc_opcode::k_branch_smaller_int;

	//	IMPORTANT: Iterator register is the FIRST symbol of the loop body's symbol table.
	const auto counter_reg = symbol_pos_t::make_stack_pos(0, static_cast<int>(body_acc._symbol_table._symbols.size()));
	body_acc._instrs.push_back(gen_instruction_t(bc_opcode::k_copy_reg_inplace_value, counter_reg, start_expr._out, {}));

	// Reuse start value as our counter.
	// Notice: we need to store iterator value in body's first register.
	int leave_pc = existing_instructions + 1 + body_instr_count + 2;

	//	Skip entire loop?
	body_acc._instrs.push_back(gen_instruction_t(condition_opcode1, end_expr._out, counter_reg, make_imm_int_field(leave_pc - get_count(body_acc._instrs))));

	int body_start_pc = get_count(body_acc._instrs);

	body_acc = flatten_scope(gen_acc, body_acc, loop_body);
	body_acc._instrs.push_back(gen_instruction_t(bc_opcode::k_add_int, counter_reg, counter_reg, const1_reg));
	body_acc._instrs.push_back(gen_instruction_t(condition_opcode2, counter_reg, end_expr._out, make_imm_int_field(body_start_pc - get_count(body_acc._instrs))));

	QUARK_ASSERT(body_acc.check_invariant());
	return body_acc;
}

static gen_scope_t generate_while_statement(bcgenerator_t& gen_acc, const statement_t::while_statement_t& statement, const gen_scope_t& body){
	QUARK_ASSERT(gen_acc.check_invariant());
	QUARK_ASSERT(body.check_invariant());

	auto body_acc = body;

	const auto& loop_body = generate_block(gen_acc, statement._body);
	int body_instr_count = static_cast<int>(loop_body._instrs.size());
	const auto condition_pc = static_cast<int>(body_acc._instrs.size());

	const auto condition_expr = generate_expression(gen_acc, {}, statement._condition, body_acc);
	body_acc = condition_expr._body;
	body_acc._instrs.push_back(gen_instruction_t(bc_opcode::k_branch_false_bool, condition_expr._out, make_imm_int_field(body_instr_count + 2), {}));
	body_acc = flatten_scope(gen_acc, body_acc, loop_body);
	const auto body_end_pc = static_cast<int>(body_acc._instrs.size());
	body_acc._instrs.push_back(gen_instruction_t(bc_opcode::k_branch_always, make_imm_int_field(condition_pc - body_end_pc), {}, {} ));
	return body_acc;
}

static gen_scope_t generate_expression_statement(bcgenerator_t& gen_acc, const statement_t::expression_statement_t& statement, const gen_scope_t& body){
	QUARK_ASSERT(gen_acc.check_invariant());
	QUARK_ASSERT(body.check_invariant());

	auto body_acc = body;
	const auto& expr = generate_expression(gen_acc, {}, statement._expression, body);
	body_acc = expr._body;
	return body_acc;
}


static gen_scope_t generate_block_statements(bcgenerator_t& gen_acc, const gen_scope_t& body, const std::vector<statement_t>& statements){
	QUARK_ASSERT(gen_acc.check_invariant());
	QUARK_ASSERT(body.check_invariant());

	auto body_acc = body;

	if(statements.empty() == false){
		for(const auto& statement: statements){
			QUARK_ASSERT(statement.check_invariant());

			struct visitor_t {
				bcgenerator_t& _gen_acc;
				const gen_scope_t body_acc;

				gen_scope_t operator()(const statement_t::return_statement_t& s) const{
					return generate_return_statement(_gen_acc, s, body_acc);
				}

				gen_scope_t operator()(const statement_t::bind_local_t& s) const{
					QUARK_ASSERT(false);
					quark::throw_exception();
				}
				gen_scope_t operator()(const statement_t::assign_t& s) const{
					QUARK_ASSERT(false);
					quark::throw_exception();
				}
				gen_scope_t operator()(const statement_t::assign2_t& s) const{
					return generate_assign2_statement(_gen_acc, s, body_acc);
				}
				gen_scope_t operator()(const statement_t::init2_t& s) const{
					return generate_init2_statement(_gen_acc, s, body_acc);
				}
				gen_scope_t operator()(const statement_t::block_statement_t& s) const{
					return generate_block_statement(_gen_acc, s, body_acc);
				}

				gen_scope_t operator()(const statement_t::ifelse_statement_t& s) const{
					return generate_ifelse_statement(_gen_acc, s, body_acc);
				}
				gen_scope_t operator()(const statement_t::for_statement_t& s) const{
					return generate_for_statement(_gen_acc, s, body_acc);
				}
				gen_scope_t operator()(const statement_t::while_statement_t& s) const{
					return generate_while_statement(_gen_acc, s, body_acc);
				}


				gen_scope_t operator()(const statement_t::expression_statement_t& s) const{
					return generate_expression_statement(_gen_acc, s, body_acc);
				}
				gen_scope_t operator()(const statement_t::software_system_statement_t& s) const{
					return body_acc;
				}
				gen_scope_t operator()(const statement_t::container_def_statement_t& s) const{
					return body_acc;
				}
				gen_scope_t operator()(const statement_t::benchmark_def_statement_t& s) const{
					return body_acc;
				}
				gen_scope_t operator()(const statement_t::test_def_statement_t& s) const{
					return body_acc;
				}
			};

			body_acc = std::visit(visitor_t{gen_acc, body_acc}, statement._contents);
			QUARK_ASSERT(body_acc.check_invariant());
		}
	}

	QUARK_ASSERT(body_acc.check_invariant());
	QUARK_ASSERT(gen_acc.check_invariant());
	return body_acc;
}

static gen_scope_t generate_block(bcgenerator_t& gen_acc, const lexical_scope_t& body){
	QUARK_ASSERT(gen_acc.check_invariant());
	QUARK_ASSERT(body.check_invariant());

	auto body_acc = gen_scope_t({}, body._symbol_table);
	body_acc = generate_block_statements(gen_acc, body_acc, body._statements);

	QUARK_ASSERT(body_acc.check_invariant());
	QUARK_ASSERT(gen_acc.check_invariant());
	return body_acc;
}

static gen_scope_t generate_toplevel_scope(bcgenerator_t& gen_acc, gen_scope_t& body_acc, const lexical_scope_t& body){
	QUARK_ASSERT(gen_acc.check_invariant());
	QUARK_ASSERT(body.check_invariant());

	body_acc = generate_block_statements(gen_acc, body_acc, body._statements);

	//	Append a stop to make sure execution doesn't leave instruction vector.
	if(body_acc._instrs.empty() == true || body_acc._instrs.back()._opcode != bc_opcode::k_return){
		body_acc._instrs.push_back(gen_instruction_t(bc_opcode::k_stop, {}, {}, {}));
	}

	QUARK_ASSERT(body_acc.check_invariant());
	QUARK_ASSERT(gen_acc.check_invariant());
	return body_acc;
}

static void generate_global_scope(bcgenerator_t& gen_acc, const lexical_scope_t& globals){
	gen_acc._globals = gen_scope_t({}, globals._symbol_table);
	generate_toplevel_scope(gen_acc, gen_acc._globals, globals);
}

static gen_scope_t generate_function_definition(bcgenerator_t& gen_acc, const floyd::function_definition_t& function_def){
	if(function_def._optional_body){
		auto body = gen_scope_t({}, function_def._optional_body->_symbol_table);
		const auto body_acc = generate_toplevel_scope(gen_acc, body, *function_def._optional_body.get());
		return body_acc;
	}
	else{
		return gen_scope_t({});
	}
}


//////////////////////////////////////		PROCESS EXPRESSIONS


static gen_expr_out_t generate_resolve_member_expression(bcgenerator_t& gen_acc, const symbol_pos_t& target_reg, const expression_t& e, const expression_t::resolve_member_t& details, const gen_scope_t& body){
	QUARK_ASSERT(gen_acc.check_invariant());

	const auto& types = gen_acc._ast_imm->_tree._types;

	QUARK_ASSERT(peek2(types, details.parent_address->get_output_type()).is_struct());
	QUARK_ASSERT(body.check_invariant());

	auto body_acc = body;

	const auto& parent_expr = generate_expression(gen_acc, {}, *details.parent_address, body);
	body_acc = parent_expr._body;

	const auto struct_type = details.parent_address->get_output_type();
	const auto expr_output_type = e.get_output_type();

	const auto& struct_def = peek2(types, struct_type).get_struct(types);
	int index = find_struct_member_index(struct_def, details.member_name);
	QUARK_ASSERT(index != -1);

	const auto target_reg2 = target_reg.is_empty() ? generate_local_temp(types, body_acc, expr_output_type, "temp: resolve-member output") : target_reg;
	body_acc._instrs.push_back(gen_instruction_t(bc_opcode::k_get_struct_member,
		target_reg2,
		parent_expr._out,
		make_imm_int_field(index)
	));

	QUARK_ASSERT(body_acc.check_invariant());
	return { body_acc, target_reg2, expr_output_type };
}



//	Generates a call to the global function that implements the intrinsic.
static gen_expr_out_t generate_fallthrough_intrinsic(bcgenerator_t& gen_acc, const symbol_pos_t& target_reg, const type_t& call_output_type, const expression_t::intrinsic_t& details, const gen_scope_t& body){
	QUARK_ASSERT(gen_acc.check_invariant());
	QUARK_ASSERT(target_reg.check_invariant());
	QUARK_ASSERT(call_output_type.check_invariant());
	QUARK_ASSERT(body.check_invariant());

	auto body_acc = body;

	const auto& intrinsic_signatures = gen_acc._ast_imm->intrinsic_signatures;

	//	Find function
    const auto it = std::find_if(intrinsic_signatures.vec.begin(), intrinsic_signatures.vec.end(), [&details](const intrinsic_signature_t& e) { return get_intrinsic_opcode(e) == details.call_name; } );
    QUARK_ASSERT(it != intrinsic_signatures.vec.end());
	const auto function_type = std::make_shared<type_t>(it->_function_type);

	const auto index = it - intrinsic_signatures.vec.begin();
	const auto addr = symbol_pos_t::make_stack_pos(symbol_pos_t::k_intrinsic, static_cast<int>(index));

	const auto call_details = expression_t::call_t {
		std::make_shared<expression_t>(expression_t::make_load2(addr, *function_type)),
		details.args
	};

	const auto result = generate_call_expression(gen_acc, target_reg, call_output_type, call_details, body);
	body_acc = result._body;
	const auto target_reg2 = result._out;

	QUARK_ASSERT(body_acc.check_invariant());
	return { body_acc, target_reg2, call_output_type };
}




static gen_expr_out_t generate_update_call(bcgenerator_t& gen_acc, const symbol_pos_t& target_reg, const type_t& output_type, const expression_t& parent, const expression_t& key, const expression_t& new_value, const gen_scope_t& body){
	QUARK_ASSERT(gen_acc.check_invariant());
	QUARK_ASSERT(output_type.check_invariant());
	QUARK_ASSERT(parent.check_invariant());
	QUARK_ASSERT(key.check_invariant());
	QUARK_ASSERT(new_value.check_invariant());
	QUARK_ASSERT(body.check_invariant());

	const auto& signs = gen_acc._ast_imm->intrinsic_signatures;

	const auto intrinsic_details = expression_t::intrinsic_t {
		get_intrinsic_opcode(signs.update),
		{ parent, key, new_value }
	};

	return generate_fallthrough_intrinsic(gen_acc, target_reg, output_type, intrinsic_details, body);
}

static bc_opcode convert_call_to_pushback_opcode(const types_t& types, const type_t& arg1_type){
	QUARK_ASSERT(types.check_invariant());
	QUARK_ASSERT(arg1_type.check_invariant());

	const auto& arg1_peek = peek2(types, arg1_type);
	if(arg1_peek.is_vector()){
		return bc_opcode::k_pushback_vector_w_external_elements;
	}
	else if(arg1_peek.is_string()){
		return bc_opcode::k_pushback_string;
	}
	else{
		return bc_opcode::k_nop;
	}
}

//	push_back intrinsics has special handling so it can emit specialized opcodes instead of function call for some types.
static gen_expr_out_t generate_intrinsic_push_back_expression(bcgenerator_t& gen_acc, const symbol_pos_t& target_reg, const type_t& call_output_type, const expression_t::intrinsic_t& details, const gen_scope_t& body){
	QUARK_ASSERT(gen_acc.check_invariant());
	QUARK_ASSERT(target_reg.check_invariant());
	QUARK_ASSERT(call_output_type.check_invariant());
	QUARK_ASSERT(body.check_invariant());

	QUARK_ASSERT(details.args.size() == 2);

	auto body_acc = body;
	const auto& types = gen_acc._ast_imm->_tree._types;

	QUARK_ASSERT(call_output_type == details.args[0].get_output_type());

	const auto arg1_type = details.args[0].get_output_type();
	bc_opcode opcode = convert_call_to_pushback_opcode(types, arg1_type);
	if(opcode != bc_opcode::k_nop){
		//	push_back() used DYN-arguments which are resolved at runtime. When we make opcodes we need to check at compile time = now.
		if(peek2(types, arg1_type).is_string() && peek2(types, details.args[1].get_output_type()).is_int() == false){
			quark::throw_runtime_error("Bad element to push_back(). Require push_back(string, int)");
		}

		const auto& arg1_expr = generate_expression(gen_acc, {}, details.args[0], body_acc);
		body_acc = arg1_expr._body;

		const auto& arg2_expr = generate_expression(gen_acc, {}, details.args[1], body_acc);
		body_acc = arg2_expr._body;

		const auto target_reg2 = target_reg.is_empty() ? generate_local_temp(types, body_acc, call_output_type, "temp: result for k_pushback_x") : target_reg;

		body_acc._instrs.push_back(gen_instruction_t(opcode, target_reg2, arg1_expr._out, arg2_expr._out));
		QUARK_ASSERT(body_acc.check_invariant());
		return { body_acc, target_reg2, arg1_type };
	}
	else{
		throw std::exception();
	}
}



//	a = size(b)
static bc_opcode convert_call_to_size_opcode(const types_t& types, const type_t& arg1_type){
	QUARK_ASSERT(types.check_invariant());
	QUARK_ASSERT(arg1_type.check_invariant());

	const auto& arg1_peek = peek2(types, arg1_type);
	if(arg1_peek.is_vector()){
		return bc_opcode::k_get_size_vector_w_external_elements;
	}
	else if(arg1_peek.is_dict()){
		return bc_opcode::k_get_size_dict_w_external_values;
	}
	else if(arg1_peek.is_string()){
		return bc_opcode::k_get_size_string;
	}
	else if(arg1_peek.is_json()){
		return bc_opcode::k_get_size_jsonvalue;
	}
	else{
		return bc_opcode::k_nop;
	}
}

static gen_expr_out_t generate_intrinsic_size_expression(bcgenerator_t& gen_acc, const symbol_pos_t& target_reg, const type_t& call_output_type, const expression_t::intrinsic_t& details, const gen_scope_t& body){
	QUARK_ASSERT(gen_acc.check_invariant());
	QUARK_ASSERT(target_reg.check_invariant());
	QUARK_ASSERT(call_output_type.check_invariant());
	QUARK_ASSERT(body.check_invariant());
	QUARK_ASSERT(details.args.size() == 1);

	const auto arg1_type = details.args[0].get_output_type();

	auto body_acc = body;
	const auto& types = gen_acc._ast_imm->_tree._types;
	const auto& signs = gen_acc._ast_imm->intrinsic_signatures;

	bc_opcode opcode = convert_call_to_size_opcode(types, arg1_type);
	if(opcode != bc_opcode::k_nop){
		const auto& arg1_expr = generate_expression(gen_acc, {}, details.args[0], body_acc);
		body_acc = arg1_expr._body;

		const auto target_reg2 = target_reg.is_empty() ? generate_local_temp(types, body_acc, call_output_type, "temp: result for k_get_size_vector_x") : target_reg;
		body_acc._instrs.push_back(gen_instruction_t(opcode, target_reg2, arg1_expr._out, make_imm_int_field(0)));
		QUARK_ASSERT(body_acc.check_invariant());
		return { body_acc, target_reg2, peek2(types, signs.size._function_type).get_function_return(types) };
	}
	else{
		throw std::exception();
	}
}



//	Converts expression to a call to intrinsic() function.
static gen_expr_out_t generate_update_member_expression(bcgenerator_t& gen_acc, const symbol_pos_t& target_reg, const expression_t& e, const expression_t::update_member_t& details, const gen_scope_t& body){
	QUARK_ASSERT(gen_acc.check_invariant());

	const auto& types = gen_acc._ast_imm->_tree._types;

	QUARK_ASSERT(peek2(types, details.parent_address->get_output_type()).is_struct());
	QUARK_ASSERT(body.check_invariant());

	const auto struct_def = peek2(types, e.get_output_type()).get_struct(types);
	const auto member_name = struct_def._members[details.member_index]._name;
	const auto member_name_expr = expression_t::make_literal_string(member_name);
	return generate_update_call(gen_acc, target_reg, e.get_output_type(), *details.parent_address, member_name_expr, *details.new_value, body);
}


static gen_expr_out_t generate_lookup_element_expression(bcgenerator_t& gen_acc, const symbol_pos_t& target_reg, const expression_t& e, const expression_t::lookup_t& details, const gen_scope_t& body){
	QUARK_ASSERT(gen_acc.check_invariant());
	QUARK_ASSERT(e.check_invariant());
	QUARK_ASSERT(body.check_invariant());

	const auto& types = gen_acc._ast_imm->_tree._types;
	auto body_acc = body;

	const auto& parent_expr = generate_expression(gen_acc, {}, *details.parent_address, body_acc);
	body_acc = parent_expr._body;

	const auto& key_expr = generate_expression(gen_acc, {}, *details.lookup_key, body_acc);
	body_acc = key_expr._body;

	const auto parent_type = parent_expr._type;
	const auto parent_peek = peek2(types, parent_type);
	const auto opcode = [&]{
		if(parent_peek.is_string()){
			return bc_opcode::k_lookup_element_string;
		}
		else if(parent_peek.is_json()){
			return bc_opcode::k_lookup_element_json;
		}
		else if(parent_peek.is_vector()){
			return bc_opcode::k_lookup_element_vector_w_external_elements;
		}
		else if(parent_peek.is_dict()){
			return bc_opcode::k_lookup_element_dict_w_external_values;
		}
		else{
			QUARK_ASSERT(false);
			quark::throw_exception();
		}
	}();

	const auto target_reg2 = target_reg.is_empty() ? generate_local_temp(types, body_acc, e.get_output_type(), "temp: resolve-member output") : target_reg;

	body_acc._instrs.push_back(gen_instruction_t(
		opcode,
		target_reg2,
		parent_expr._out,
		key_expr._out
	));
	QUARK_ASSERT(body_acc.check_invariant());
	return { body_acc, target_reg2, e.get_output_type() };
}


//??? Value already sits in a register / global -- no need to generate code to copy it in most cases!
static gen_expr_out_t generate_load2_expression(bcgenerator_t& gen_acc, const symbol_pos_t& target_reg, const expression_t& e, const expression_t::load2_t& details, const gen_scope_t& body){
	QUARK_ASSERT(gen_acc.check_invariant());
	QUARK_ASSERT(e.check_invariant());
	QUARK_ASSERT(body.check_invariant());

	auto body_acc = body;
	const auto& types = gen_acc._ast_imm->_tree._types;
	const auto result_type = e.get_output_type();

	//	Shortcut: If we're loading a local-variable and are free from putting it in target_reg -- just access the register where it sits = no instruction!
	if(target_reg.is_empty() && details.address._parent_steps != symbol_pos_t::k_global_scope){
		QUARK_ASSERT(body_acc.check_invariant());
		return { body_acc, details.address, result_type };
	}
	else{
		const auto target_reg2 = target_reg.is_empty() ? generate_local_temp(types, body_acc, e.get_output_type(), "temp: load2") : target_reg;
		body_acc = copy_value(gen_acc, result_type, target_reg2, details.address, body_acc);

		QUARK_ASSERT(body_acc.check_invariant());
		return { body_acc, target_reg2, result_type };
	}
}

struct call_spec_t {
	type_t _return;
	std::vector<type_t> _args;

	uint32_t _dynbits;
	uint32_t _extbits;
	uint32_t _stack_element_count;
};

static uint32_t pack_bools(const std::vector<bool>& bools){
	QUARK_ASSERT(bools.size() <= 32);

	uint32_t acc = 0x0;
	for(const auto e: bools){
		acc = (acc << 1) | (e ? 1 : 0);
	}
	return acc;
}

/*
	1) Generates code to evaluate all argument expressions
	2) Generates call-instruction, with all push/pops needed to pass arguments.
	Supports DYN-arguments.
*/
struct call_setup_t {
	gen_scope_t _body;
	std::vector<bool> _exts;
	int _stack_count;
};

//	NOTICE: extbits are generated for every value on callstack, even for DYN-types. This is correct.
static call_setup_t generate_call_setup(bcgenerator_t& gen_acc, const std::vector<type_t>& function_def_arg_type, const expression_t* args, int callee_arg_count, const gen_scope_t& body){
	QUARK_ASSERT(gen_acc.check_invariant());
	QUARK_ASSERT(args != nullptr || callee_arg_count == 0);
	QUARK_ASSERT(body.check_invariant());
	QUARK_ASSERT(callee_arg_count == function_def_arg_type.size());

	auto body_acc = body;
	const auto& types = gen_acc._ast_imm->_tree._types;

    const auto dynamic_arg_count = std::count_if(function_def_arg_type.begin(), function_def_arg_type.end(), [&](const auto& e){ return peek2(types, e).is_any(); });
	const auto arg_count = callee_arg_count;

	//	Generate code / symbols for all arguments to the function call. Record where each arg is kept.
	//	This might not create instructions or anything, if arguments are available as constants somewhere.
	std::vector<std::pair<symbol_pos_t, type_t>> argument_regs;
	for(int i = 0 ; i < arg_count ; i++){
		const auto& m2 = generate_expression(gen_acc, {}, args[i], body_acc);
		body_acc = m2._body;
		argument_regs.push_back(std::pair<symbol_pos_t, type_t>(m2._out, m2._type));
	}

	//	We have max 16 extbits when popping stack.
	//	??? Add separate call opcode that can have any number of parameters -- use for collection initialisation
	//	where you might have 10 MB of ints or expressions.
	if(arg_count > 16){
		quark::throw_runtime_error("Max 16 arguments to function.");
	}

	//	Make push-instructions that push args in correct order on callstack, before k_opcode_call is inserted.
	std::vector<bool> exts;;
	for(int i = 0 ; i < arg_count ; i++){
		const auto arg_reg = argument_regs[i].first;
		const auto callee_arg_type = argument_regs[i].second;
		const auto& func_arg_type = function_def_arg_type[i];

		//	Prepend internal-dynamic arguments with the type of the actual callee-argument.
		if(peek2(types, func_arg_type).is_any()){
			const auto arg_type_reg = generate_local_const(types, body_acc, value_t::make_int(itype_from_type(callee_arg_type)), "DYN type arg #" + std::to_string(i));
			body_acc._instrs.push_back(gen_instruction_t(bc_opcode::k_push_inplace_value, arg_type_reg, {}, {} ));

			//	Int don't need extbit.
			exts.push_back(false);
		}

		body_acc._instrs.push_back(gen_instruction_t(bc_opcode::k_push_external_value, arg_reg, {}, {} ));
		exts.push_back(true);
	}
	const auto stack_count = arg_count + dynamic_arg_count;

	QUARK_ASSERT(gen_acc.check_invariant());
	QUARK_ASSERT(body_acc.check_invariant());
	return { body_acc, exts, (int)stack_count };
}

static gen_expr_out_t generate_callee(bcgenerator_t& gen_acc, const expression_t::call_t& details, const gen_scope_t& body0){
	QUARK_ASSERT(gen_acc.check_invariant());
	QUARK_ASSERT(body0.check_invariant());

	auto body_acc = body0;
	const auto& types = gen_acc._ast_imm->_tree._types;
	const auto load2 = std::get_if<expression_t::load2_t>(&details.callee->_expression_variant);
	if(load2 != nullptr && load2->address._parent_steps == symbol_pos_t::k_intrinsic){
		const auto& intrinsic_signatures = gen_acc._ast_imm->intrinsic_signatures;
		QUARK_ASSERT(load2->address._index >= 0 && load2->address._index < intrinsic_signatures.vec.size());
		const auto& intrinsic = intrinsic_signatures.vec[load2->address._index];

		const auto name = intrinsic.name;

		const auto function_type = details.callee->get_output_type();
		const auto value = value_t::make_function_value(types, function_type, module_symbol_t(name));
		const auto r = generate_local_const(types, body_acc, value, "temp: intrinsics-" + name);
		return gen_expr_out_t { body_acc, r, function_type };
	}
	else{
		return generate_expression(gen_acc, {}, *details.callee, body_acc);
	}
}

/*
	Handles a call-expression. Output is one of these:

	- A call expression
	- A hard-coded opcode like size() and push_back().
*/
static gen_expr_out_t generate_call_expression(bcgenerator_t& gen_acc, const symbol_pos_t& target_reg, const type_t& call_output_type, const expression_t::call_t& details, const gen_scope_t& body){
	QUARK_ASSERT(gen_acc.check_invariant());
	QUARK_ASSERT(call_output_type.check_invariant());
	QUARK_ASSERT(body.check_invariant());

	auto body_acc = body;
	const auto& types = gen_acc._ast_imm->_tree._types;

	const auto callee_arg_count = static_cast<int>(details.args.size());

	const auto function_type = details.callee->get_output_type();
	const auto function_def_arg_types = peek2(types, function_type).get_function_args(types);
	QUARK_ASSERT(callee_arg_count == function_def_arg_types.size());
	const auto return_type = call_output_type;

	QUARK_ASSERT(callee_arg_count == function_def_arg_types.size());
	const auto arg_count = callee_arg_count;

	body_acc._instrs.push_back(gen_instruction_t(bc_opcode::k_push_frame_ptr, {}, {}, {} ));

	const auto callee_expr = generate_callee(gen_acc, details, body_acc);
	body_acc = callee_expr._body;

	const auto call_setup = generate_call_setup(gen_acc, function_def_arg_types, &details.args[0], callee_arg_count, body_acc);
	body_acc = call_setup._body;

	const auto target_reg2 = target_reg.is_empty() ? generate_local_temp(types, body_acc, call_output_type, "temp: call return") : target_reg;

	//??? No need to allocate return register if functions returns void.
	QUARK_ASSERT(peek2(types, return_type).is_any() == false && peek2(types, return_type).is_undefined() == false)
	body_acc._instrs.push_back(gen_instruction_t(
		bc_opcode::k_call,
		target_reg2,
		callee_expr._out,
		make_imm_int_field(arg_count)
	));

	const auto extbits = pack_bools(call_setup._exts);
	if(call_setup._stack_count > 0){
		body_acc._instrs.push_back(gen_instruction_t(bc_opcode::k_popn, make_imm_int_field(call_setup._stack_count), make_imm_int_field(extbits), {} ));
	}
	body_acc._instrs.push_back(gen_instruction_t(bc_opcode::k_pop_frame_ptr, {}, {}, {} ));

	QUARK_ASSERT(body_acc.check_invariant());
	return { body_acc, target_reg2, return_type };
}


static gen_expr_out_t generate_intrinsic_expression(bcgenerator_t& gen_acc, const symbol_pos_t& target_reg, const type_t& call_output_type, const expression_t::intrinsic_t& details, const gen_scope_t& body){
	QUARK_ASSERT(gen_acc.check_invariant());
	QUARK_ASSERT(target_reg.check_invariant());
	QUARK_ASSERT(call_output_type.check_invariant());
	QUARK_ASSERT(body.check_invariant());

	const auto& signs = gen_acc._ast_imm->intrinsic_signatures;

	if(details.call_name == get_intrinsic_opcode(signs.assert)){
		return generate_fallthrough_intrinsic(gen_acc, target_reg, call_output_type, details, body);
	}
	else if(details.call_name == get_intrinsic_opcode(signs.to_string)){
		return generate_fallthrough_intrinsic(gen_acc, target_reg, call_output_type, details, body);
	}
	else if(details.call_name == get_intrinsic_opcode(signs.to_pretty_string)){
		return generate_fallthrough_intrinsic(gen_acc, target_reg, call_output_type, details, body);
	}


	else if(details.call_name == get_intrinsic_opcode(signs.typeof_sign)){
		return generate_fallthrough_intrinsic(gen_acc, target_reg, call_output_type, details, body);
	}


	else if(details.call_name == get_intrinsic_opcode(signs.update)){
		return generate_fallthrough_intrinsic(gen_acc, target_reg, call_output_type, details, body);
	}
	else if(details.call_name == get_intrinsic_opcode(signs.size)){
		return generate_intrinsic_size_expression(gen_acc, target_reg, call_output_type, details, body);
	}
	else if(details.call_name == get_intrinsic_opcode(signs.find)){
		return generate_fallthrough_intrinsic(gen_acc, target_reg, call_output_type, details, body);
	}
	else if(details.call_name == get_intrinsic_opcode(signs.exists)){
		return generate_fallthrough_intrinsic(gen_acc, target_reg, call_output_type, details, body);
	}
	else if(details.call_name == get_intrinsic_opcode(signs.erase)){
		return generate_fallthrough_intrinsic(gen_acc, target_reg, call_output_type, details, body);
	}
	else if(details.call_name == get_intrinsic_opcode(signs.get_keys)){
		return generate_fallthrough_intrinsic(gen_acc, target_reg, call_output_type, details, body);
	}
	else if(details.call_name == get_intrinsic_opcode(signs.push_back)){
		return generate_intrinsic_push_back_expression(gen_acc, target_reg, call_output_type, details, body);
	}
	else if(details.call_name == get_intrinsic_opcode(signs.subset)){
		return generate_fallthrough_intrinsic(gen_acc, target_reg, call_output_type, details, body);
	}
	else if(details.call_name == get_intrinsic_opcode(signs.replace)){
		return generate_fallthrough_intrinsic(gen_acc, target_reg, call_output_type, details, body);
	}


	else if(details.call_name == get_intrinsic_opcode(signs.parse_json_script)){
		return generate_fallthrough_intrinsic(gen_acc, target_reg, call_output_type, details, body);
	}
	else if(details.call_name == get_intrinsic_opcode(signs.generate_json_script)){
		return generate_fallthrough_intrinsic(gen_acc, target_reg, call_output_type, details, body);
	}
	else if(details.call_name == get_intrinsic_opcode(signs.to_json)){
		return generate_fallthrough_intrinsic(gen_acc, target_reg, call_output_type, details, body);
	}
	else if(details.call_name == get_intrinsic_opcode(signs.from_json)){
		return generate_fallthrough_intrinsic(gen_acc, target_reg, call_output_type, details, body);
	}


	else if(details.call_name == get_intrinsic_opcode(signs.get_json_type)){
		return generate_fallthrough_intrinsic(gen_acc, target_reg, call_output_type, details, body);
	}


	else if(details.call_name == get_intrinsic_opcode(signs.map)){
		return generate_fallthrough_intrinsic(gen_acc, target_reg, call_output_type, details, body);
	}
	else if(details.call_name == get_intrinsic_opcode(signs.map_dag)){
		return generate_fallthrough_intrinsic(gen_acc, target_reg, call_output_type, details, body);
	}
	else if(details.call_name == get_intrinsic_opcode(signs.filter)){
		return generate_fallthrough_intrinsic(gen_acc, target_reg, call_output_type, details, body);
	}
	else if(details.call_name == get_intrinsic_opcode(signs.reduce)){
		return generate_fallthrough_intrinsic(gen_acc, target_reg, call_output_type, details, body);
	}
	else if(details.call_name == get_intrinsic_opcode(signs.stable_sort)){
		return generate_fallthrough_intrinsic(gen_acc, target_reg, call_output_type, details, body);
	}



	else if(details.call_name == get_intrinsic_opcode(signs.print)){
		return generate_fallthrough_intrinsic(gen_acc, target_reg, call_output_type, details, body);
	}
	else if(details.call_name == get_intrinsic_opcode(signs.send)){
		return generate_fallthrough_intrinsic(gen_acc, target_reg, call_output_type, details, body);
	}
	else if(details.call_name == get_intrinsic_opcode(signs.exit)){
		return generate_fallthrough_intrinsic(gen_acc, target_reg, call_output_type, details, body);
	}


	else if(details.call_name == get_intrinsic_opcode(signs.bw_not)){
		return generate_fallthrough_intrinsic(gen_acc, target_reg, call_output_type, details, body);
	}
	else if(details.call_name == get_intrinsic_opcode(signs.bw_and)){
		return generate_fallthrough_intrinsic(gen_acc, target_reg, call_output_type, details, body);
	}
	else if(details.call_name == get_intrinsic_opcode(signs.bw_or)){
		return generate_fallthrough_intrinsic(gen_acc, target_reg, call_output_type, details, body);
	}
	else if(details.call_name == get_intrinsic_opcode(signs.bw_xor)){
		return generate_fallthrough_intrinsic(gen_acc, target_reg, call_output_type, details, body);
	}
	else if(details.call_name == get_intrinsic_opcode(signs.bw_shift_left)){
		return generate_fallthrough_intrinsic(gen_acc, target_reg, call_output_type, details, body);
	}
	else if(details.call_name == get_intrinsic_opcode(signs.bw_shift_right)){
		return generate_fallthrough_intrinsic(gen_acc, target_reg, call_output_type, details, body);
	}
	else if(details.call_name == get_intrinsic_opcode(signs.bw_shift_right_arithmetic)){
		return generate_fallthrough_intrinsic(gen_acc, target_reg, call_output_type, details, body);
	}


	else{
		QUARK_ASSERT(false);
	}
	throw std::exception();
}



//??? Submit dest-register to all gen-functions = minimize temps.
//??? Wrap type in struct to make it typesafe.
static gen_expr_out_t generate_construct_value_expression(bcgenerator_t& gen_acc, const symbol_pos_t& target_reg, const expression_t& e, const expression_t::value_constructor_t& details, const gen_scope_t& body){
	QUARK_ASSERT(gen_acc.check_invariant());
	QUARK_ASSERT(e.check_invariant());
	QUARK_ASSERT(body.check_invariant());

	const auto& types = gen_acc._ast_imm->_tree._types;
	auto body_acc = body;

	const auto target_type = e.get_output_type();
	const auto target_itype = itype_from_type(target_type);

	const auto callee_arg_count = static_cast<int>(details.elements.size());
	std::vector<type_t> arg_types;
	for(const auto& m: details.elements){
		arg_types.push_back(dummy_func(types, m.get_output_type()));
	}
	const auto arg_count = callee_arg_count;

	const auto call_setup = generate_call_setup(gen_acc, arg_types, &details.elements[0], arg_count, body_acc);
	body_acc = call_setup._body;

	const auto source_itype = arg_count == 0 ? -1 : itype_from_type(dummy_func(types, details.elements[0].get_output_type()));

	const auto target_reg2 = target_reg.is_empty()
		? generate_local_temp(types, body_acc, e.get_output_type(), "temp: construct value result")
		: target_reg;

	const auto target_peek = peek2(types, target_type);
	if(target_peek.is_vector()){
		body_acc._instrs.push_back(gen_instruction_t(
			bc_opcode::k_new_vector_w_external_elements,
			target_reg2,
			make_imm_int_field(target_itype),
			make_imm_int_field(arg_count)
		));
	}
	else if(target_peek.is_dict()){
		body_acc._instrs.push_back(gen_instruction_t(
			bc_opcode::k_new_dict_w_external_values,
			target_reg2,
			make_imm_int_field(target_itype),
			make_imm_int_field(arg_count)
		));
	}
	else if(target_peek.is_struct()){
		body_acc._instrs.push_back(gen_instruction_t(
			bc_opcode::k_new_struct,
			target_reg2,
			make_imm_int_field(target_itype),
			make_imm_int_field(arg_count)
		));
	}
	else{
		QUARK_ASSERT(arg_count == 1);

		body_acc._instrs.push_back(gen_instruction_t(
			bc_opcode::k_new_1,
			target_reg2,
			make_imm_int_field(target_itype),
			make_imm_int_field(source_itype)
		));
	}

	const auto extbits = pack_bools(call_setup._exts);
	if(call_setup._stack_count > 0){
		body_acc._instrs.push_back(gen_instruction_t(bc_opcode::k_popn, make_imm_int_field(call_setup._stack_count), make_imm_int_field(extbits), {} ));
	}

	QUARK_ASSERT(body_acc.check_invariant());
	return { body_acc, target_reg2, target_type };
}

static gen_expr_out_t generate_benchmark_expression(bcgenerator_t& gen_acc, const symbol_pos_t& target_reg, const expression_t& e, const expression_t::benchmark_expr_t& details, const gen_scope_t& body){
	QUARK_ASSERT(gen_acc.check_invariant());
	QUARK_ASSERT(e.check_invariant());
	QUARK_ASSERT(body.check_invariant());

	return generate_expression(gen_acc, target_reg, expression_t::make_literal_int(404), body);
}


static gen_expr_out_t generate_arithmetic_unary_minus_expression(bcgenerator_t& gen_acc, const symbol_pos_t& target_reg, const expression_t& e, const expression_t::unary_minus_t& details, const gen_scope_t& body){
	QUARK_ASSERT(gen_acc.check_invariant());
	QUARK_ASSERT(e.check_invariant());
	QUARK_ASSERT(body.check_invariant());

	const auto& types = gen_acc._ast_imm->_tree._types;
	const auto type = details.expr->get_output_type();
	const auto peek_type = peek2(types, type);

	if(peek_type.is_int()){
		const auto e2 = expression_t::make_arithmetic(expression_type::k_arithmetic_subtract, expression_t::make_literal_int(0), *details.expr, e._output_type);
		return generate_expression(gen_acc, target_reg, e2, body);
	}
	else if(peek_type.is_double()){
		const auto e2 = expression_t::make_arithmetic(expression_type::k_arithmetic_subtract, expression_t::make_literal_double(0), *details.expr, e._output_type);
		return generate_expression(gen_acc, target_reg, e2, body);
	}
	else{
		QUARK_ASSERT(false);
		quark::throw_exception();
	}
}

/*
	temp = a ? b : c

	becomes:

	temp = 0
	if(a){
		temp = b
	}
	else{
		temp = c
	}
*/
static gen_expr_out_t generate_conditional_operator_expression(bcgenerator_t& gen_acc, const symbol_pos_t& target_reg, const expression_t& e, const expression_t::conditional_t& details, const gen_scope_t& body){
	QUARK_ASSERT(gen_acc.check_invariant());
	QUARK_ASSERT(e.check_invariant());
	QUARK_ASSERT(body.check_invariant());

	auto body_acc = body;
	const auto& types = gen_acc._ast_imm->_tree._types;

	const auto& condition_expr = generate_expression(gen_acc, {}, *details.condition, body_acc);
	body_acc = condition_expr._body;

	const auto result_type = dummy_func(types, e.get_output_type());

	const auto target_reg2 = target_reg.is_empty() ? generate_local_temp(types, body_acc, e.get_output_type(), "temp: condition value output") : target_reg;

	int jump1_pc = static_cast<int>(body_acc._instrs.size());
	body_acc._instrs.push_back(
		gen_instruction_t(
			bc_opcode::k_branch_false_bool,
			condition_expr._out,
			make_imm_int_field(0xbeefdeeb),
			{}
		)
	);

	////////	A expression

	const auto& a_expr = generate_expression(gen_acc, target_reg2, *details.a, body_acc);
	body_acc = a_expr._body;

	int jump2_pc = static_cast<int>(body_acc._instrs.size());
	body_acc._instrs.push_back(
		gen_instruction_t(
			bc_opcode::k_branch_always,
			make_imm_int_field(0xfea57be7),
			{},
			{}
		)
	);

	////////	B expression

	int b_pc = static_cast<int>(body_acc._instrs.size());
	const auto& b_expr = generate_expression(gen_acc, target_reg2, *details.b, body_acc);
	body_acc = b_expr._body;

	int end_pc = static_cast<int>(body_acc._instrs.size());

	QUARK_ASSERT(body_acc._instrs[jump1_pc]._field_b._index == 0xbeefdeeb);
	QUARK_ASSERT(body_acc._instrs[jump2_pc]._field_a._index == 0xfea57be7);
	body_acc._instrs[jump1_pc]._field_b._index = b_pc - jump1_pc;
	body_acc._instrs[jump2_pc]._field_a._index = end_pc - jump2_pc;

	QUARK_ASSERT(body_acc.check_invariant());
	return { body_acc, target_reg2, result_type };
}

static gen_expr_out_t generate_comparison_expression(bcgenerator_t& gen_acc, const symbol_pos_t& target_reg, expression_type op, const expression_t& e, const expression_t::comparison_t& details, const gen_scope_t& body){
	QUARK_ASSERT(gen_acc.check_invariant());
	QUARK_ASSERT(e.check_invariant());
	QUARK_ASSERT(body.check_invariant());

	auto body_acc = body;
	const auto& types = gen_acc._ast_imm->_tree._types;

	const auto& left_expr = generate_expression(gen_acc, {}, *details.lhs, body_acc);
	body_acc = left_expr._body;

	const auto& right_expr = generate_expression(gen_acc, {}, *details.rhs, body_acc);
	body_acc = right_expr._body;

	//	Type is the data the opcode works on -- comparing two ints, comparing two strings etc.
	const auto type = details.lhs->get_output_type();

	//	Output reg always a bool.
	const auto output_type = e.get_output_type();
	QUARK_ASSERT(peek2(types, output_type).is_bool());
	const auto target_reg2 = target_reg.is_empty() ? generate_local_temp(types, body_acc, output_type, "temp: comparison flag") : target_reg;

	if(peek2(types, type).is_int()){

		//	Bool tells if to flip left / right.
		static const std::map<expression_type, std::pair<bool, bc_opcode>> conv_opcode_int = {
			{ expression_type::k_comparison_smaller_or_equal,			{ false, bc_opcode::k_comparison_smaller_or_equal_int } },
			{ expression_type::k_comparison_smaller,					{ false, bc_opcode::k_comparison_smaller_int } },
			{ expression_type::k_comparison_larger_or_equal,			{ true, bc_opcode::k_comparison_smaller_or_equal_int } },
			{ expression_type::k_comparison_larger,						{ true, bc_opcode::k_comparison_smaller_int } },

			{ expression_type::k_logical_equal,							{ false, bc_opcode::k_logical_equal_int } },
			{ expression_type::k_logical_nonequal,						{ false, bc_opcode::k_logical_nonequal_int } }
		};

		const auto result = conv_opcode_int.at(details.op);
		if(result.first == false){
			body_acc._instrs.push_back(gen_instruction_t(result.second, target_reg2, left_expr._out, right_expr._out));
		}
		else{
			body_acc._instrs.push_back(gen_instruction_t(result.second, target_reg2, right_expr._out, left_expr._out));
		}
	}
	else{

		//	Bool tells if to flip left / right.
		static const std::map<expression_type, std::pair<bool, bc_opcode>> conv_opcode = {
			{ expression_type::k_comparison_smaller_or_equal,			{ false, bc_opcode::k_comparison_smaller_or_equal } },
			{ expression_type::k_comparison_smaller,					{ false, bc_opcode::k_comparison_smaller } },
			{ expression_type::k_comparison_larger_or_equal,			{ true, bc_opcode::k_comparison_smaller_or_equal } },
			{ expression_type::k_comparison_larger,						{ true, bc_opcode::k_comparison_smaller } },

			{ expression_type::k_logical_equal,							{ false, bc_opcode::k_logical_equal } },
			{ expression_type::k_logical_nonequal,						{ false, bc_opcode::k_logical_nonequal } }
		};

		const auto result = conv_opcode.at(details.op);
		if(result.first == false){
			body_acc._instrs.push_back(gen_instruction_t(result.second, target_reg2, left_expr._out, right_expr._out));
		}
		else{
			body_acc._instrs.push_back(gen_instruction_t(result.second, target_reg2, right_expr._out, left_expr._out));
		}
	}

	QUARK_ASSERT(body_acc.check_invariant());
	return { body_acc, target_reg2, type_t::make_bool() };
}

static gen_expr_out_t generate_arithmetic_expression(bcgenerator_t& gen_acc, const symbol_pos_t& target_reg, expression_type op, const expression_t& e, const expression_t::arithmetic_t& details, const gen_scope_t& body){
	QUARK_ASSERT(gen_acc.check_invariant());
	QUARK_ASSERT(e.check_invariant());
	QUARK_ASSERT(body.check_invariant());

	auto body_acc = body;
	const auto& types = gen_acc._ast_imm->_tree._types;

	const auto& left_expr = generate_expression(gen_acc, {}, *details.lhs, body_acc);
	body_acc = left_expr._body;

	const auto& right_expr = generate_expression(gen_acc, {}, *details.rhs, body_acc);
	body_acc = right_expr._body;

	const auto type = details.lhs->get_output_type();

	const auto target_reg2 = target_reg.is_empty() ? generate_local_temp(types, body_acc, e.get_output_type(), "temp: arithmetic output") : target_reg;

	const auto peek = peek2(types, type);
	const auto opcode = [&]{
		if(peek.is_bool()){
			static const std::map<expression_type, bc_opcode> conv_opcode = {
				{ expression_type::k_arithmetic_add, bc_opcode::k_add_bool },
				{ expression_type::k_arithmetic_subtract, bc_opcode::k_nop },
				{ expression_type::k_arithmetic_multiply, bc_opcode::k_nop },
				{ expression_type::k_arithmetic_divide, bc_opcode::k_nop },
				{ expression_type::k_arithmetic_remainder, bc_opcode::k_nop },

				{ expression_type::k_logical_and, bc_opcode::k_logical_and_bool },
				{ expression_type::k_logical_or, bc_opcode::k_logical_or_bool }
			};
			return conv_opcode.at(details.op);
		}
		else if(peek.is_int()){
			static const std::map<expression_type, bc_opcode> conv_opcode = {
				{ expression_type::k_arithmetic_add, bc_opcode::k_add_int },
				{ expression_type::k_arithmetic_subtract, bc_opcode::k_subtract_int },
				{ expression_type::k_arithmetic_multiply, bc_opcode::k_multiply_int },
				{ expression_type::k_arithmetic_divide, bc_opcode::k_divide_int },
				{ expression_type::k_arithmetic_remainder, bc_opcode::k_remainder_int },

				{ expression_type::k_logical_and, bc_opcode::k_logical_and_int },
				{ expression_type::k_logical_or, bc_opcode::k_logical_or_int }
			};
			return conv_opcode.at(details.op);
		}
		else if(peek.is_double()){
			static const std::map<expression_type, bc_opcode> conv_opcode = {
				{ expression_type::k_arithmetic_add, bc_opcode::k_add_double },
				{ expression_type::k_arithmetic_subtract, bc_opcode::k_subtract_double },
				{ expression_type::k_arithmetic_multiply, bc_opcode::k_multiply_double },
				{ expression_type::k_arithmetic_divide, bc_opcode::k_divide_double },
				{ expression_type::k_arithmetic_remainder, bc_opcode::k_nop },

				{ expression_type::k_logical_and, bc_opcode::k_logical_and_double },
				{ expression_type::k_logical_or, bc_opcode::k_logical_or_double }
			};
			return conv_opcode.at(details.op);
		}
		else if(peek.is_string()){
			static const std::map<expression_type, bc_opcode> conv_opcode = {
				{ expression_type::k_arithmetic_add, bc_opcode::k_concat_strings },
				{ expression_type::k_arithmetic_subtract, bc_opcode::k_nop },
				{ expression_type::k_arithmetic_multiply, bc_opcode::k_nop },
				{ expression_type::k_arithmetic_divide, bc_opcode::k_nop },
				{ expression_type::k_arithmetic_remainder, bc_opcode::k_nop },

				{ expression_type::k_logical_and, bc_opcode::k_nop },
				{ expression_type::k_logical_or, bc_opcode::k_nop }
			};
			return conv_opcode.at(details.op);
		}
		else if(peek.is_vector()){
			static const std::map<expression_type, bc_opcode> conv_opcode = {
				{ expression_type::k_arithmetic_add, bc_opcode::k_concat_vectors_w_external_elements },
				{ expression_type::k_arithmetic_subtract, bc_opcode::k_nop },
				{ expression_type::k_arithmetic_multiply, bc_opcode::k_nop },
				{ expression_type::k_arithmetic_divide, bc_opcode::k_nop },
				{ expression_type::k_arithmetic_remainder, bc_opcode::k_nop },

				{ expression_type::k_logical_and, bc_opcode::k_nop },
				{ expression_type::k_logical_or, bc_opcode::k_nop }
			};
			return conv_opcode.at(details.op);
		}
		else{
			QUARK_ASSERT(false);
			quark::throw_exception();
		}
	}();

	QUARK_ASSERT(opcode != bc_opcode::k_nop);
	body_acc._instrs.push_back(gen_instruction_t(opcode, target_reg2, left_expr._out, right_expr._out));

	QUARK_ASSERT(body_acc.check_invariant());
	return { body_acc, target_reg2, type };
}

static gen_expr_out_t generate_literal_expression(bcgenerator_t& gen_acc, const symbol_pos_t& target_reg, const expression_t& e, const expression_t::literal_exp_t& details, const gen_scope_t& body){
	QUARK_ASSERT(gen_acc.check_invariant());
	QUARK_ASSERT(body.check_invariant());

	auto body_acc = body;
	const auto& types = gen_acc._ast_imm->_tree._types;

	const auto const_temp = generate_local_const(types, body_acc, e.get_literal(), "literal constant");
	if(target_reg.is_empty()){
		QUARK_ASSERT(body_acc.check_invariant());
		return { body_acc, const_temp, e.get_output_type() };
	}

	//	We need to copy the value to the target reg...
	else{
		const auto result_type = e.get_output_type();
		body_acc = copy_value(gen_acc, result_type, target_reg, const_temp, body_acc);

		QUARK_ASSERT(body_acc.check_invariant());
		return { body_acc, target_reg, result_type };
	}
}

static gen_expr_out_t generate_expression(bcgenerator_t& gen_acc, const symbol_pos_t& target_reg, const expression_t& e, const gen_scope_t& body){
	QUARK_ASSERT(gen_acc.check_invariant());
	QUARK_ASSERT(target_reg.check_invariant());
	QUARK_ASSERT(e.check_invariant());
	QUARK_ASSERT(body.check_invariant());

	struct visitor_t {
		bcgenerator_t& gen_acc;
		const symbol_pos_t& target_reg;
		const expression_t& e;
		const gen_scope_t& body;


		gen_expr_out_t operator()(const expression_t::literal_exp_t& expr) const{
			return generate_literal_expression(gen_acc, target_reg, e, expr, body);
		}
		gen_expr_out_t operator()(const expression_t::arithmetic_t& expr) const{
			return generate_arithmetic_expression(gen_acc, target_reg, expr.op, e, expr, body);
		}
		gen_expr_out_t operator()(const expression_t::comparison_t& expr) const{
			return generate_comparison_expression(gen_acc, target_reg, expr.op, e, expr, body);
		}
		gen_expr_out_t operator()(const expression_t::unary_minus_t& expr) const{
			return generate_arithmetic_unary_minus_expression(gen_acc, target_reg, e, expr, body);
		}
		gen_expr_out_t operator()(const expression_t::conditional_t& expr) const{
			return generate_conditional_operator_expression(gen_acc, target_reg, e, expr, body);
		}

		gen_expr_out_t operator()(const expression_t::call_t& expr) const{
			return generate_call_expression(gen_acc, target_reg, e.get_output_type(), expr, body);
		}
		gen_expr_out_t operator()(const expression_t::intrinsic_t& expr) const{
			return generate_intrinsic_expression(gen_acc, target_reg, e.get_output_type(), expr, body);
		}

		gen_expr_out_t operator()(const expression_t::struct_definition_expr_t& expr) const{
			QUARK_ASSERT(false);
			throw std::exception();
		}
		gen_expr_out_t operator()(const expression_t::function_definition_expr_t& expr) const{
			QUARK_ASSERT(false);
			throw std::exception();
		}
		gen_expr_out_t operator()(const expression_t::load_t& expr) const{
			QUARK_ASSERT(false);
			throw std::exception();
		}
		gen_expr_out_t operator()(const expression_t::load2_t& expr) const{
			return generate_load2_expression(gen_acc, target_reg, e, expr, body);
		}

		gen_expr_out_t operator()(const expression_t::resolve_member_t& expr) const{
			return generate_resolve_member_expression(gen_acc, target_reg, e, expr, body);
		}
		gen_expr_out_t operator()(const expression_t::update_member_t& expr) const{
			return generate_update_member_expression(gen_acc, target_reg, e, expr, body);
		}
		gen_expr_out_t operator()(const expression_t::lookup_t& expr) const{
			return generate_lookup_element_expression(gen_acc, target_reg, e, expr, body);
		}
		gen_expr_out_t operator()(const expression_t::value_constructor_t& expr) const{
			return generate_construct_value_expression(gen_acc, target_reg, e, expr, body);
		}
		gen_expr_out_t operator()(const expression_t::benchmark_expr_t& expr) const{
			return generate_benchmark_expression(gen_acc, target_reg, e, expr, body);
		}
	};

	auto result = std::visit(visitor_t{ gen_acc, target_reg, e, body }, e._expression_variant);
	return result;
}


//////////////////////////////////////		bcgenerator_t


bcgenerator_t::bcgenerator_t(const semantic_ast_t& ast) :
	_globals({})
{
	QUARK_ASSERT(ast.check_invariant());

	_ast_imm = std::make_shared<semantic_ast_t>(ast);
	QUARK_ASSERT(check_invariant());
}

bcgenerator_t::bcgenerator_t(const bcgenerator_t& other) :
	_ast_imm(other._ast_imm),
	_globals(other._globals)
{
	QUARK_ASSERT(other.check_invariant());
	QUARK_ASSERT(check_invariant());
}

void bcgenerator_t::swap(bcgenerator_t& other) throw(){
	other._ast_imm.swap(this->_ast_imm);
	std::swap(other._globals, this->_globals);
}

const bcgenerator_t& bcgenerator_t::operator=(const bcgenerator_t& other){
	auto temp = other;
	temp.swap(*this);
	return *this;
}

#if DEBUG
bool bcgenerator_t::check_invariant() const {
	QUARK_ASSERT(_ast_imm->check_invariant());
//	QUARK_ASSERT(_global_body_ptr != nullptr);
	return true;
}
#endif


//////////////////////////////////////		FREE


//	Converts our fat instruction into the destination format, which is just 64 bits.
static bc_instruction_t squeeze_instruction(const gen_instruction_t& instruction){
	QUARK_ASSERT(instruction.check_invariant());

	QUARK_ASSERT(instruction._field_a._parent_steps == 0 || instruction._field_a._parent_steps == symbol_pos_t::k_global_scope || instruction._field_a._parent_steps == k_immediate_value__parent_steps);
	QUARK_ASSERT(instruction._field_b._parent_steps == 0 || instruction._field_b._parent_steps == symbol_pos_t::k_global_scope || instruction._field_b._parent_steps == k_immediate_value__parent_steps);
	QUARK_ASSERT(instruction._field_c._parent_steps == 0 || instruction._field_c._parent_steps == symbol_pos_t::k_global_scope || instruction._field_c._parent_steps == k_immediate_value__parent_steps);
	QUARK_ASSERT(instruction._field_a._index >= INT16_MIN && instruction._field_a._index <= INT16_MAX);
	QUARK_ASSERT(instruction._field_b._index >= INT16_MIN && instruction._field_b._index <= INT16_MAX);
	QUARK_ASSERT(instruction._field_c._index >= INT16_MIN && instruction._field_c._index <= INT16_MAX);

	const auto encoding = k_opcode_info.at(instruction._opcode)._encoding;
	const auto reg_flags = encoding_to_reg_flags(encoding);

	QUARK_ASSERT(check_field__local(instruction._field_a, reg_flags._a));
	QUARK_ASSERT(check_field__local(instruction._field_b, reg_flags._b));
	QUARK_ASSERT(check_field__local(instruction._field_c, reg_flags._c));


	const auto result = bc_instruction_t(
		instruction._opcode,
		static_cast<int16_t>(instruction._field_a._index),
		static_cast<int16_t>(instruction._field_b._index),
		static_cast<int16_t>(instruction._field_c._index)
	);
	return result;
}

static bc_static_frame_t make_frame(const types_t& types, const gen_scope_t& body, const std::vector<type_t>& args){
	QUARK_ASSERT(types.check_invariant());
	QUARK_ASSERT(body.check_invariant());

	std::vector<bc_instruction_t> instrs2;
	for(const auto& e: body._instrs){
		instrs2.push_back(squeeze_instruction(e));
	}

	std::vector<std::pair<std::string, bc_symbol_t>> symbols2;
	for(const auto& e: body._symbol_table._symbols){
		const auto t0 = e.second.get_value_type();
		const auto t = peek2(types, t0);
		if(e.second._symbol_type == symbol_t::symbol_type::named_type){
			const auto e2 = std::pair<std::string, bc_symbol_t>{
				e.first,
				bc_symbol_t{
					bc_symbol_t::type::immutable,
					type_desc_t::make_typeid(),
					value_t::make_typeid_value(t)
				}
			};
			symbols2.push_back(e2);
		}
		else{
			const auto e2 = std::pair<std::string, bc_symbol_t>{
				e.first,
				bc_symbol_t{
					is_mutable(e.second) ? bc_symbol_t::type::mutable1 : bc_symbol_t::type::immutable,
					t,
					e.second._init
				}
			};
			symbols2.push_back(e2);
		}
	}

	return bc_static_frame_t(types, instrs2, symbols2, args);
}

bc_program_t generate_bytecode(const semantic_ast_t& ast){
	QUARK_ASSERT(ast.check_invariant());

	if(trace_io_flag){
		QUARK_TRACE_SS("INPUT:  " << json_to_pretty_string(gp_ast_to_json(ast._tree)));
	}

	bcgenerator_t a(ast);
	const auto& types = ast._tree._types;
	generate_global_scope(a, a._ast_imm->_tree._globals);

	std::vector<bc_function_definition_t> funcs;
	for(auto function_id = 0 ; function_id < ast._tree._function_defs.size() ; function_id++){
		const auto& function_def = ast._tree._function_defs[function_id];

		if(function_def._optional_body){
			const auto body2 = generate_function_definition(a, function_def);

			const auto args2 = peek2(types, function_def._function_type).get_function_args(types);

			const auto frame = make_frame(types, body2, args2);
			const auto frame2 = std::make_shared<bc_static_frame_t>(frame);
			const auto f = bc_function_definition_t {
				func_link_t {
					"user floyd function: " + function_def._definition_name,
					module_symbol_t(function_def._definition_name),
					function_def._function_type,
					count_dyn_args(types, function_def._function_type),
					true,
	//				function_def._named_args,
					frame2.get()
				},
				frame2
			};
			funcs.push_back(f);
		}
		else{
			const auto f = bc_function_definition_t {
				func_link_t {
					"user function: " + function_def._definition_name,
					module_symbol_t(function_def._definition_name),
					function_def._function_type,
					count_dyn_args(types, function_def._function_type),
					false,
					nullptr
				},
				nullptr
			};
			funcs.push_back(f);
		}
	}

	const auto globals2 = make_frame(types, a._globals, std::vector<type_t>{});
	const auto result = bc_program_t{
		globals2,
		funcs,
		types,
		ast._tree._software_system,
		ast._tree._container_def
	};

	if(trace_io_flag){
		QUARK_TRACE_SS("OUTPUT: " << json_to_pretty_string(bcprogram_to_json(result)));
	}

	return result;
}


}	//	floyd
