//
//  parser_evaluator.cpp
//  Floyd
//
//  Created by Marcus Zetterquist on 26/07/16.
//  Copyright © 2016 Marcus Zetterquist. All rights reserved.
//

/*
	??? "body" is a bad term. Maybe use "scope", "lexical scope", "block"?

	bcgen_body_t -- holds the symbols and instructions for
		- Global state
		- a function
		- a sub-block of a function

	The functions that generate code all use a bcgen_body_t& body_acc that holds the current state of a body and where new data is *acc*umulated / added.
	This means that only the body_acc's state is changed, not global state.
	This makes it possible to generate code for an IF-statement's THEN and ELSE bodies first, count their instructions,
	then setup branches and insert the THEN and ELSE instructions.
*/

#include "bytecode_generator.h"

#include "semantic_analyser.h"
#include "floyd_interpreter.h"
#include "floyd_runtime.h"

#include <cmath>
#include <algorithm>
#include <cstdint>

const auto trace_io_flag = false;

namespace floyd {
struct semantic_ast_t;



//	Replace by int when we have flattened local bodies.
typedef variable_address_t reg_t;


//////////////////////////////////////		bcgen_instruction_t

/*
	A temporary, bigger, representation of instructions. Each register can reference the current scope or any parent scope (like the globals).
	At and of codegen, explicit instructions have been generated for locals and global accesses and each reg-slot no longer points to scopes. squeeze_instruction().
*/

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
		_symbol_table{}
	{
		QUARK_ASSERT(check_invariant());
	}

	bcgen_body_t(const std::vector<bcgen_instruction_t>& instructions, const symbol_table_t& symbols) :
		_instrs(instructions),
		_symbol_table(symbols)
	{
		QUARK_ASSERT(check_invariant());
	}

	public: bool check_invariant() const;


	////////////////////////		STATE
	symbol_table_t _symbol_table;
	std::vector<bcgen_instruction_t> _instrs;
};

bc_static_frame_t make_frame(const bcgen_body_t& body, const std::vector<typeid_t>& args);


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

	public: bcgen_body_t _globals;
	public: std::vector<typeid_t> _types;
};



//////////////////////////////////////		expression_gen_t

//	Why: needed to return from codegen functions that process expressions.
//	Any new instructions have been recorded into _body.

struct expression_gen_t {

	//////////////////////////////////////		STATE
	bcgen_body_t _body;

	variable_address_t _out;

	//	Output type.
	bc_typeid_t _type;
};


/*
	target_reg: if defined, this is where the output value will be stored. If undefined,
	then the expression allocates (or redirect to existing register).
	expression_gen_t._out: always holds the output register, no matter who decided it.
*/
expression_gen_t bcgen_expression(bcgenerator_t& gen_acc, const variable_address_t& target_reg, const expression_t& e, const bcgen_body_t& body);

bcgen_body_t bcgen_body_block(bcgenerator_t& gen_acc, const body_t& body);

static expression_gen_t bcgen_call_expression(bcgenerator_t& gen_acc, const variable_address_t& target_reg, const typeid_t& call_output_type, const expression_t::call_t& details, const bcgen_body_t& body);



static int add_temp(symbol_table_t& symbols, const std::string& name, const floyd::typeid_t& value_type){
	const auto s = symbol_t::make_immutable_reserve(value_type);
	symbols._symbols.push_back(std::pair<std::string, symbol_t>(name, s));
	return static_cast<int>(symbols._symbols.size() - 1);
}

variable_address_t add_local_temp(bcgen_body_t& body_acc, const typeid_t& type, const std::string& name){
	QUARK_ASSERT(body_acc.check_invariant());
	QUARK_ASSERT(type.check_invariant());
	QUARK_ASSERT(name.empty() == false);

	int id = add_temp(body_acc._symbol_table, name, type);
	return variable_address_t::make_variable_address(0, id);
}

int add_constant_literal(symbol_table_t& symbols, const std::string& name, const floyd::value_t& value){
	const auto s = symbol_t::make_immutable_precalc(value);
	symbols._symbols.push_back(std::pair<std::string, symbol_t>(name, s));
	return static_cast<int>(symbols._symbols.size() - 1);
}

variable_address_t add_local_const(bcgen_body_t& body_acc, const value_t& value, const std::string& name){
	QUARK_ASSERT(body_acc.check_invariant());
	QUARK_ASSERT(value.check_invariant());
	QUARK_ASSERT(name.empty() == false);

	int id = add_constant_literal(body_acc._symbol_table, name, value);
	return variable_address_t::make_variable_address(0, id);
}

//	Use to stuff an integer into an instruction, in one of the register slots.
variable_address_t make_imm_int(int value){
	return variable_address_t::make_variable_address(666, value);
}

bc_typeid_t intern_type(bcgenerator_t& gen_acc, const typeid_t& type){
	QUARK_ASSERT(gen_acc.check_invariant());
	QUARK_ASSERT(type.check_invariant());

    const auto it = std::find_if(gen_acc._types.begin(), gen_acc._types.end(), [&type](const typeid_t& e) { return e == type; });
	if(it != gen_acc._types.end()){
		const auto pos = static_cast<bc_typeid_t>(it - gen_acc._types.begin());
		return pos;
	}
	else{
		gen_acc._types.push_back(type);
		return static_cast<bc_typeid_t>(gen_acc._types.size() - 1);
	}

	QUARK_ASSERT(gen_acc.check_invariant());
}

reg_t flatten_reg(const reg_t& r, int offset){
	QUARK_ASSERT(r.check_invariant());

	if(r._parent_steps == 666){
		return r;
	}
	else if(r._parent_steps == 0){
		return reg_t::make_variable_address(0, r._index + offset);
	}
	else if(r._parent_steps == variable_address_t::k_global_scope){
		return r;
	}
	else{
		return reg_t::make_variable_address(r._parent_steps - 1, r._index);
	}
}

bool check_register(const reg_t& reg, bool is_reg){
	if(is_reg){
		QUARK_ASSERT(reg._parent_steps != 666);
	}
	else{
		QUARK_ASSERT(reg._parent_steps == 666 || reg.is_empty());
	}
	return true;
}

bool check_register_nonlocal(const reg_t& reg, bool is_reg){
	if(is_reg){
		//	Must not be a global -- those should have been turned to global-access opcodes.
		QUARK_ASSERT(reg._parent_steps != variable_address_t::k_global_scope);
	}
	else{
		QUARK_ASSERT(reg._parent_steps == 666 || reg.is_empty());
	}
	return true;
}

bool check_register__local(const reg_t& reg, bool is_reg){
	if(is_reg){
		//	Must not be a global -- those should have been turned to global-access opcodes.
		QUARK_ASSERT(reg._parent_steps != variable_address_t::k_global_scope);
	}
	else{
		QUARK_ASSERT(reg._parent_steps == 666 || reg.is_empty());
	}
	return true;
}


//////////////////////////////////////		bcgen_body_t


 bool bcgen_body_t::check_invariant() const {
	for(int i = 0 ; i < _instrs.size() ; i++){
		const auto instruction = _instrs[i];
		QUARK_ASSERT(instruction.check_invariant());
	}
	for(const auto& e: _instrs){
		const auto encoding = k_opcode_info.at(e._opcode)._encoding;
		const auto reg_flags = encoding_to_reg_flags(encoding);

		QUARK_ASSERT(check_register_nonlocal(e._reg_a, reg_flags._a));
		QUARK_ASSERT(check_register_nonlocal(e._reg_b, reg_flags._b));
		QUARK_ASSERT(check_register_nonlocal(e._reg_c, reg_flags._c));
	}
	for(const auto& e: _symbol_table._symbols){
		QUARK_ASSERT(e.first != "");
		QUARK_ASSERT(e.second.check_invariant());
	}
	return true;
}


//////////////////////////////////////		bcgen_instruction_t


#if DEBUG
bool bcgen_instruction_t::check_invariant() const {
	const auto encoding = k_opcode_info.at(_opcode)._encoding;
	const auto reg_flags = encoding_to_reg_flags(encoding);

	QUARK_ASSERT(check_register(_reg_a, reg_flags._a));
	QUARK_ASSERT(check_register(_reg_b, reg_flags._b));
	QUARK_ASSERT(check_register(_reg_c, reg_flags._c));
	return true;
}
#endif


//////////////////////////////////////		Free functions


bcgen_body_t flatten_body(const bcgenerator_t& gen, const bcgen_body_t& dest, const bcgen_body_t& source){
	QUARK_ASSERT(gen.check_invariant());
	QUARK_ASSERT(dest.check_invariant());
	QUARK_ASSERT(source.check_invariant());

	std::vector<bcgen_instruction_t> instructions;
	int offset = static_cast<int>(dest._symbol_table._symbols.size());
	for(int i = 0 ; i < source._instrs.size() ; i++){
		//	Decrese parent-step for all local register accesses.
		auto s = source._instrs[i];

		const auto encoding = k_opcode_info.at(s._opcode)._encoding;
		const auto reg_flags = encoding_to_reg_flags(encoding);

		//	Make a new instructions with registered offsetted for flattening. Only offset registrers, not when used as immediates.
		const auto s2 = bcgen_instruction_t(
			s._opcode,
			reg_flags._a ? flatten_reg(s._reg_a, offset) : s._reg_a,
			reg_flags._b ? flatten_reg(s._reg_b, offset) : s._reg_b,
			reg_flags._c ? flatten_reg(s._reg_c, offset) : s._reg_c
		);
		instructions.push_back(s2);
	}

	auto body_acc = dest;
	body_acc._instrs.insert(body_acc._instrs.end(), instructions.begin(), instructions.end());
	body_acc._symbol_table._symbols.insert(body_acc._symbol_table._symbols.end(), source._symbol_table._symbols.begin(), source._symbol_table._symbols.end());
	return body_acc;
}



//	Supports globals & locals both as dest and sources.
bcgen_body_t copy_value(const typeid_t& type, const reg_t& dest_reg, const reg_t& source_reg, const bcgen_body_t& body){
	QUARK_ASSERT(type.check_invariant());
	QUARK_ASSERT(dest_reg.check_invariant());
	QUARK_ASSERT(source_reg.check_invariant());
	QUARK_ASSERT(body.check_invariant());

	auto body_acc = body;
	bool is_ext = encode_as_external(type);

	//	If this asserts, we should special-case and do nothing.
	QUARK_ASSERT(!(dest_reg == source_reg));


	//	global <= global
	if(dest_reg._parent_steps == variable_address_t::k_global_scope && source_reg._parent_steps == variable_address_t::k_global_scope){
		QUARK_ASSERT(false);
		quark::throw_exception();
//		body_acc._instrs.push_back(bcgen_instruction_t(is_ext ? bc_opcode::k_load_global_external_value : bc_opcode::k_load_global_inplace_value, dest_reg, make_imm_int(source_reg._index), {}));
	}

	//	global <= local
	else if(dest_reg._parent_steps == variable_address_t::k_global_scope && source_reg._parent_steps != variable_address_t::k_global_scope){
		body_acc._instrs.push_back(bcgen_instruction_t(is_ext ? bc_opcode::k_store_global_external_value : bc_opcode::k_store_global_inplace_value, make_imm_int(dest_reg._index), source_reg, {}));
	}
	//	local <= global
	else if(dest_reg._parent_steps != variable_address_t::k_global_scope && source_reg._parent_steps == variable_address_t::k_global_scope){
		body_acc._instrs.push_back(bcgen_instruction_t(is_ext ? bc_opcode::k_load_global_external_value : bc_opcode::k_load_global_inplace_value, dest_reg, make_imm_int(source_reg._index), {}));
	}
	//	local <= local
	else{
		body_acc._instrs.push_back(bcgen_instruction_t(is_ext ? bc_opcode::k_copy_reg_external_value : bc_opcode::k_copy_reg_inplace_value, dest_reg, source_reg, {}));
	}

	QUARK_ASSERT(body_acc.check_invariant());
	return body_acc;
}



//////////////////////////////////////		PROCESS STATEMENTS

//??? need logic that knows that globals can be treated as locals for instructions in global scope.

bcgen_body_t bcgen_assign2_statement(bcgenerator_t& gen_acc, const statement_t::assign2_t& statement, const bcgen_body_t& body){
	QUARK_ASSERT(gen_acc.check_invariant());
	QUARK_ASSERT(body.check_invariant());

	auto body_acc = body;

	//	Shortcut: if destination is a local variable, have the expression write directly to that register.
	if(statement._dest_variable._parent_steps != variable_address_t::k_global_scope){
		const auto expr = bcgen_expression(gen_acc, statement._dest_variable, statement._expression, body_acc);
		body_acc = expr._body;
		QUARK_ASSERT(body_acc.check_invariant());
	}
	else{
		const auto expr = bcgen_expression(gen_acc, {}, statement._expression, body_acc);
		body_acc = expr._body;
		body_acc = copy_value(statement._expression.get_output_type(), statement._dest_variable, expr._out, body_acc);
		QUARK_ASSERT(body_acc.check_invariant());
	}
	return body_acc;
}
bcgen_body_t bcgen_init2_statement(bcgenerator_t& gen_acc, const statement_t::init2_t& statement, const bcgen_body_t& body){
	QUARK_ASSERT(gen_acc.check_invariant());
	QUARK_ASSERT(body.check_invariant());

	//	Treat init2 just like assign2.
	const auto temp = statement_t::assign2_t{ statement._dest_variable, statement._expression };
	return bcgen_assign2_statement(gen_acc, temp, body);
}

bcgen_body_t bcgen_block_statement(bcgenerator_t& gen_acc, const statement_t::block_statement_t& statement, const bcgen_body_t& body){
	QUARK_ASSERT(gen_acc.check_invariant());
	QUARK_ASSERT(body.check_invariant());

	const auto body_acc = bcgen_body_block(gen_acc, statement._body);
	return flatten_body(gen_acc, body, body_acc);
}

bcgen_body_t bcgen_return_statement(bcgenerator_t& gen_acc, const statement_t::return_statement_t& statement, const bcgen_body_t& body){
	QUARK_ASSERT(gen_acc.check_invariant());
	QUARK_ASSERT(body.check_invariant());

	auto body_acc = body;
	const auto expr = bcgen_expression(gen_acc, {}, statement._expression, body);
	body_acc = expr._body;
	body_acc._instrs.push_back(bcgen_instruction_t(bc_opcode::k_return, expr._out, {}, {}));
	return body_acc;
}

bcgen_body_t bcgen_ifelse_statement(bcgenerator_t& gen_acc, const statement_t::ifelse_statement_t& statement, const bcgen_body_t& body){
	QUARK_ASSERT(gen_acc.check_invariant());
	QUARK_ASSERT(body.check_invariant());

	auto body_acc = body;

	const auto condition_expr = bcgen_expression(gen_acc, {}, statement._condition, body_acc);
	body_acc = condition_expr._body;
	QUARK_ASSERT(statement._condition.get_output_type().is_bool());

	//???	Needs short-circuit evaluation here!

	//	Notice that we generate the instructions for then/else but we don't store them in body_acc yet, we
	//	keep them and check their sizes so we can calculate jumps around them.
	const auto& then_expr = bcgen_body_block(gen_acc, statement._then_body);
	const auto& else_expr = bcgen_body_block(gen_acc, statement._else_body);

	body_acc._instrs.push_back(
		bcgen_instruction_t(
			bc_opcode::k_branch_false_bool,
			condition_expr._out,
			make_imm_int(static_cast<int>(then_expr._instrs.size()) + 2),
			{}
		)
	);
	body_acc = flatten_body(gen_acc, body_acc, then_expr);
	body_acc._instrs.push_back(
		bcgen_instruction_t(
			bc_opcode::k_branch_always,
			make_imm_int(static_cast<int>(else_expr._instrs.size()) + 1),
			{},
			{}
		)
	);
	body_acc = flatten_body(gen_acc, body_acc, else_expr);
	return body_acc;
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
int get_count(const std::vector<bcgen_instruction_t>& instructions){
	return static_cast<int>(instructions.size());
}

bcgen_body_t bcgen_for_statement(bcgenerator_t& gen_acc, const statement_t::for_statement_t& statement, const bcgen_body_t& body){
	QUARK_ASSERT(gen_acc.check_invariant());
	QUARK_ASSERT(body.check_invariant());

	auto body_acc = body;

	const auto existing_instructions = get_count(body._instrs);

	const auto start_expr = bcgen_expression(gen_acc, {}, statement._start_expression, body_acc);
	body_acc = start_expr._body;

	const auto end_expr = bcgen_expression(gen_acc, {}, statement._end_expression, body_acc);
	body_acc = end_expr._body;

	const auto const1_reg = add_local_const(body_acc, value_t::make_int(1), "integer 1, to decrement with");

	const auto& loop_body = bcgen_body_block(gen_acc, statement._body);
	int body_instr_count = get_count(loop_body._instrs);

	QUARK_ASSERT(
		statement._range_type == statement_t::for_statement_t::k_closed_range
		|| statement._range_type ==statement_t::for_statement_t::k_open_range
	);
	const auto condition_opcode1 = statement._range_type == statement_t::for_statement_t::k_closed_range ? bc_opcode::k_branch_smaller_int : bc_opcode::k_branch_smaller_or_equal_int;
	const auto condition_opcode2 = statement._range_type == statement_t::for_statement_t::k_closed_range ? bc_opcode::k_branch_smaller_or_equal_int : bc_opcode::k_branch_smaller_int;

	//	IMPORTANT: Iterator register is the FIRST symbol of the loop body's symbol table.
	const auto counter_reg = variable_address_t::make_variable_address(0, static_cast<int>(body_acc._symbol_table._symbols.size()));
	body_acc._instrs.push_back(bcgen_instruction_t(bc_opcode::k_copy_reg_inplace_value, counter_reg, start_expr._out, {}));

	// Reuse start value as our counter.
	// Notice: we need to store iterator value in body's first register.
	int leave_pc = existing_instructions + 1 + body_instr_count + 2;

	//	Skip entire loop?
	body_acc._instrs.push_back(bcgen_instruction_t(condition_opcode1, end_expr._out, counter_reg, make_imm_int(leave_pc - get_count(body_acc._instrs))));

	int body_start_pc = get_count(body_acc._instrs);

	body_acc = flatten_body(gen_acc, body_acc, loop_body);
	body_acc._instrs.push_back(bcgen_instruction_t(bc_opcode::k_add_int, counter_reg, counter_reg, const1_reg));
	body_acc._instrs.push_back(bcgen_instruction_t(condition_opcode2, counter_reg, end_expr._out, make_imm_int(body_start_pc - get_count(body_acc._instrs))));

	QUARK_ASSERT(body_acc.check_invariant());
	return body_acc;
}

bcgen_body_t bcgen_while_statement(bcgenerator_t& gen_acc, const statement_t::while_statement_t& statement, const bcgen_body_t& body){
	QUARK_ASSERT(gen_acc.check_invariant());
	QUARK_ASSERT(body.check_invariant());

	auto body_acc = body;

	const auto& loop_body = bcgen_body_block(gen_acc, statement._body);
	int body_instr_count = static_cast<int>(loop_body._instrs.size());
	const auto condition_pc = static_cast<int>(body_acc._instrs.size());

	const auto condition_expr = bcgen_expression(gen_acc, {}, statement._condition, body_acc);
	body_acc = condition_expr._body;
	body_acc._instrs.push_back(bcgen_instruction_t(bc_opcode::k_branch_false_bool, condition_expr._out, make_imm_int(body_instr_count + 2), {}));
	body_acc = flatten_body(gen_acc, body_acc, loop_body);
	const auto body_end_pc = static_cast<int>(body_acc._instrs.size());
	body_acc._instrs.push_back(bcgen_instruction_t(bc_opcode::k_branch_always, make_imm_int(condition_pc - body_end_pc), {}, {} ));
	return body_acc;
}

bcgen_body_t bcgen_expression_statement(bcgenerator_t& gen_acc, const statement_t::expression_statement_t& statement, const bcgen_body_t& body){
	QUARK_ASSERT(gen_acc.check_invariant());
	QUARK_ASSERT(body.check_invariant());

	auto body_acc = body;
	const auto& expr = bcgen_expression(gen_acc, {}, statement._expression, body);
	body_acc = expr._body;
	return body_acc;
}


bcgen_body_t bcgen_body_block_statements(bcgenerator_t& gen_acc, const bcgen_body_t& body, const std::vector<statement_t>& statements){
	QUARK_ASSERT(gen_acc.check_invariant());
	QUARK_ASSERT(body.check_invariant());

	auto body_acc = body;

	if(statements.empty() == false){
		for(const auto& statement: statements){
			QUARK_ASSERT(statement.check_invariant());

			struct visitor_t {
				bcgenerator_t& _gen_acc;
				const bcgen_body_t body_acc;

				bcgen_body_t operator()(const statement_t::return_statement_t& s) const{
					return bcgen_return_statement(_gen_acc, s, body_acc);
				}
				bcgen_body_t operator()(const statement_t::define_struct_statement_t& s) const{
					QUARK_ASSERT(false);
					quark::throw_exception();
				}

				bcgen_body_t operator()(const statement_t::bind_local_t& s) const{
					QUARK_ASSERT(false);
					quark::throw_exception();
				}
				bcgen_body_t operator()(const statement_t::assign_t& s) const{
					QUARK_ASSERT(false);
					quark::throw_exception();
				}
				bcgen_body_t operator()(const statement_t::assign2_t& s) const{
					return bcgen_assign2_statement(_gen_acc, s, body_acc);
				}
				bcgen_body_t operator()(const statement_t::init2_t& s) const{
					return bcgen_init2_statement(_gen_acc, s, body_acc);
				}
				bcgen_body_t operator()(const statement_t::block_statement_t& s) const{
					return bcgen_block_statement(_gen_acc, s, body_acc);
				}

				bcgen_body_t operator()(const statement_t::ifelse_statement_t& s) const{
					return bcgen_ifelse_statement(_gen_acc, s, body_acc);
				}
				bcgen_body_t operator()(const statement_t::for_statement_t& s) const{
					return bcgen_for_statement(_gen_acc, s, body_acc);
				}
				bcgen_body_t operator()(const statement_t::while_statement_t& s) const{
					return bcgen_while_statement(_gen_acc, s, body_acc);
				}


				bcgen_body_t operator()(const statement_t::expression_statement_t& s) const{
					return bcgen_expression_statement(_gen_acc, s, body_acc);
				}
				bcgen_body_t operator()(const statement_t::software_system_statement_t& s) const{
					return body_acc;
				}
				bcgen_body_t operator()(const statement_t::container_def_statement_t& s) const{
					return body_acc;
				}
				bcgen_body_t operator()(const statement_t::benchmark_def_statement_t& s) const{
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

bcgen_body_t bcgen_body_block(bcgenerator_t& gen_acc, const body_t& body){
	QUARK_ASSERT(gen_acc.check_invariant());
	QUARK_ASSERT(body.check_invariant());

	auto body_acc = bcgen_body_t({}, body._symbol_table);
	body_acc = bcgen_body_block_statements(gen_acc, body_acc, body._statements);

	QUARK_ASSERT(body_acc.check_invariant());
	QUARK_ASSERT(gen_acc.check_invariant());
	return body_acc;
}

bcgen_body_t bcgen_body_top(bcgenerator_t& gen_acc, bcgen_body_t& body_acc, const body_t& body){
	QUARK_ASSERT(gen_acc.check_invariant());
	QUARK_ASSERT(body.check_invariant());

	body_acc = bcgen_body_block_statements(gen_acc, body_acc, body._statements);

	//	Append a stop to make sure execution doesn't leave instruction vector.
	if(body_acc._instrs.empty() == true || body_acc._instrs.back()._opcode != bc_opcode::k_return){
		body_acc._instrs.push_back(bcgen_instruction_t(bc_opcode::k_stop, {}, {}, {}));
	}

	QUARK_ASSERT(body_acc.check_invariant());
	QUARK_ASSERT(gen_acc.check_invariant());
	return body_acc;
}

void bcgen_globals(bcgenerator_t& gen_acc, const body_t& globals){
	gen_acc._globals = bcgen_body_t({}, globals._symbol_table);
	bcgen_body_top(gen_acc, gen_acc._globals, globals);
}

static bcgen_body_t bcgen_function(bcgenerator_t& gen_acc, const floyd::function_definition_t& function_def){
	const auto floyd_func = std::get_if<function_definition_t::floyd_func_t>(&function_def._contents);
	if(floyd_func){
		if(floyd_func->_body){
			auto body = bcgen_body_t({}, floyd_func->_body->_symbol_table);
			const auto body_acc = bcgen_body_top(gen_acc, body, *floyd_func->_body.get());
			return body_acc;
		}
		else{
			return bcgen_body_t({});
		}
	}
	else{
		return bcgen_body_t({});
	}
}



//////////////////////////////////////		PROCESS EXPRESSIONS


expression_gen_t bcgen_resolve_member_expression(bcgenerator_t& gen_acc, const variable_address_t& target_reg, const expression_t& e, const expression_t::resolve_member_t& details, const bcgen_body_t& body){
	QUARK_ASSERT(gen_acc.check_invariant());
	QUARK_ASSERT(details.parent_address->get_output_type().is_struct());
	QUARK_ASSERT(body.check_invariant());

	auto body_acc = body;

	const auto& parent_expr = bcgen_expression(gen_acc, {}, *details.parent_address, body);
	body_acc = parent_expr._body;

	const auto& struct_def = details.parent_address->get_output_type().get_struct();
	int index = find_struct_member_index(struct_def, details.member_name);
	QUARK_ASSERT(index != -1);

	const auto target_reg2 = target_reg.is_empty() ? add_local_temp(body_acc, e.get_output_type(), "temp: resolve-member output") : target_reg;
	body_acc._instrs.push_back(bcgen_instruction_t(bc_opcode::k_get_struct_member,
		target_reg2,
		parent_expr._out,
		make_imm_int(index)
	));

	QUARK_ASSERT(body_acc.check_invariant());
	return { body_acc, target_reg2, intern_type(gen_acc, *e._output_type) };
}




//	Generates a call to the global function that implements the corecall.
expression_gen_t bcgen_make_fallthrough_corecall(bcgenerator_t& gen_acc, const variable_address_t& target_reg, const typeid_t& call_output_type, const expression_t::corecall_t& details, const bcgen_body_t& body){
	QUARK_ASSERT(gen_acc.check_invariant());
	QUARK_ASSERT(target_reg.check_invariant());
	QUARK_ASSERT(call_output_type.check_invariant());
	QUARK_ASSERT(body.check_invariant());

	auto body_acc = body;


	//	Find function
	const auto& symbols = gen_acc._globals._symbol_table._symbols;
    const auto it = std::find_if(symbols.begin(), symbols.end(), [&details](const std::pair<std::string, symbol_t>& e) {return (std::string("$") + e.first) == details.call_name; } );
    QUARK_ASSERT(it != symbols.end());
	const auto function_type = std::make_shared<typeid_t>(it->second._value_type);
	int global_index = static_cast<int>(it - symbols.begin());

	const auto call_details = expression_t::call_t {
		std::make_shared<expression_t>(expression_t::make_load2(variable_address_t::make_variable_address(variable_address_t::k_global_scope, global_index), function_type)),
		details.args
	};

	const auto result = bcgen_call_expression(gen_acc, target_reg, call_output_type, call_details, body);
	body_acc = result._body;
	const auto target_reg2 = result._out;

	QUARK_ASSERT(body_acc.check_invariant());
	return { body_acc, target_reg2, intern_type(gen_acc, call_output_type) };
}




expression_gen_t make_update_call(bcgenerator_t& gen_acc, const variable_address_t& target_reg, const typeid_t& output_type, const expression_t& parent, const expression_t& key, const expression_t& new_value, const bcgen_body_t& body){
	QUARK_ASSERT(gen_acc.check_invariant());
	QUARK_ASSERT(output_type.check_invariant());
	QUARK_ASSERT(parent.check_invariant());
	QUARK_ASSERT(key.check_invariant());
	QUARK_ASSERT(new_value.check_invariant());
	QUARK_ASSERT(body.check_invariant());

	const auto corecall_details = expression_t::corecall_t {
		get_corecall_opcode(make_update_signature()),
		{ parent, key, new_value }
	};

	return bcgen_make_fallthrough_corecall(gen_acc, target_reg, output_type, corecall_details, body);
}

static bc_opcode convert_call_to_pushback_opcode(const typeid_t& arg1_type){
	QUARK_ASSERT(arg1_type.check_invariant());

	if(arg1_type.is_vector()){
		if(encode_as_vector_w_inplace_elements(arg1_type)){
			return bc_opcode::k_pushback_vector_w_inplace_elements;
		}
		else{
			return bc_opcode::k_pushback_vector_w_external_elements;
		}
	}
	else if(arg1_type.is_string()){
		return bc_opcode::k_pushback_string;
	}
	else{
		return bc_opcode::k_nop;
	}
}

static expression_gen_t bcgen_corecall_push_back_expression(bcgenerator_t& gen_acc, const variable_address_t& target_reg, const typeid_t& call_output_type, const expression_t::corecall_t& details, const bcgen_body_t& body){
	QUARK_ASSERT(gen_acc.check_invariant());
	QUARK_ASSERT(target_reg.check_invariant());
	QUARK_ASSERT(call_output_type.check_invariant());
	QUARK_ASSERT(body.check_invariant());

	QUARK_ASSERT(details.args.size() == 2);

	auto body_acc = body;

	QUARK_ASSERT(call_output_type == details.args[0].get_output_type());

	const auto arg1_type = details.args[0].get_output_type();
	bc_opcode opcode = convert_call_to_pushback_opcode(arg1_type);
	if(opcode != bc_opcode::k_nop){
		//	push_back() used DYN-arguments which are resolved at runtime. When we make opcodes we need to check at compile time = now.
		if(arg1_type.is_string() && details.args[1].get_output_type().is_int() == false){
			quark::throw_runtime_error("Bad element to push_back(). Require push_back(string, int)");
		}

		const auto& arg1_expr = bcgen_expression(gen_acc, {}, details.args[0], body_acc);
		body_acc = arg1_expr._body;

		const auto& arg2_expr = bcgen_expression(gen_acc, {}, details.args[1], body_acc);
		body_acc = arg2_expr._body;

		const auto target_reg2 = target_reg.is_empty() ? add_local_temp(body_acc, call_output_type, "temp: result for k_pushback_x") : target_reg;

		body_acc._instrs.push_back(bcgen_instruction_t(opcode, target_reg2, arg1_expr._out, arg2_expr._out));
		QUARK_ASSERT(body_acc.check_invariant());
		return { body_acc, target_reg2, intern_type(gen_acc, arg1_type) };
	}
	else{
		throw std::exception();
	}
}



//	a = size(b)
static bc_opcode convert_call_to_size_opcode(const typeid_t& arg1_type){
	QUARK_ASSERT(arg1_type.check_invariant());

	if(arg1_type.is_vector()){
		if(encode_as_vector_w_inplace_elements(arg1_type)){
			return bc_opcode::k_get_size_vector_w_inplace_elements;
		}
		else{
			return bc_opcode::k_get_size_vector_w_external_elements;
		}
	}
	else if(arg1_type.is_dict()){
		if(encode_as_dict_w_inplace_values(arg1_type)){
			return bc_opcode::k_get_size_dict_w_inplace_values;
		}
		else{
			return bc_opcode::k_get_size_dict_w_external_values;
		}
	}
	else if(arg1_type.is_string()){
		return bc_opcode::k_get_size_string;
	}
	else if(arg1_type.is_json()){
		return bc_opcode::k_get_size_jsonvalue;
	}
	else{
		return bc_opcode::k_nop;
	}
}

static expression_gen_t bcgen_corecall_size_expression(bcgenerator_t& gen_acc, const variable_address_t& target_reg, const typeid_t& call_output_type, const expression_t::corecall_t& details, const bcgen_body_t& body){
	QUARK_ASSERT(gen_acc.check_invariant());
	QUARK_ASSERT(target_reg.check_invariant());
	QUARK_ASSERT(call_output_type.check_invariant());
	QUARK_ASSERT(body.check_invariant());
	QUARK_ASSERT(details.args.size() == 1);

	const auto arg1_type = details.args[0].get_output_type();

	auto body_acc = body;

	bc_opcode opcode = convert_call_to_size_opcode(arg1_type);
	if(opcode != bc_opcode::k_nop){
		const auto& arg1_expr = bcgen_expression(gen_acc, {}, details.args[0], body_acc);
		body_acc = arg1_expr._body;

		const auto target_reg2 = target_reg.is_empty() ? add_local_temp(body_acc, call_output_type, "temp: result for k_get_size_vector_x") : target_reg;
		body_acc._instrs.push_back(bcgen_instruction_t(opcode, target_reg2, arg1_expr._out, make_imm_int(0)));
		QUARK_ASSERT(body_acc.check_invariant());
		return { body_acc, target_reg2, intern_type(gen_acc, make_size_signature()._function_type.get_function_return()) };
	}
	else{
		throw std::exception();
	}
}



//	Converts expression to a call to corecall() function.
expression_gen_t bcgen_update_member_expression(bcgenerator_t& gen_acc, const variable_address_t& target_reg, const expression_t& e, const expression_t::update_member_t& details, const bcgen_body_t& body){
	QUARK_ASSERT(gen_acc.check_invariant());
	QUARK_ASSERT(details.parent_address->get_output_type().is_struct());
	QUARK_ASSERT(body.check_invariant());

	const auto struct_def = e.get_output_type().get_struct();
	const auto member_name = struct_def._members[details.member_index]._name;
	const auto member_name_expr = expression_t::make_literal_string(member_name);
	return make_update_call(gen_acc, target_reg, e.get_output_type(), *details.parent_address, member_name_expr, *details.new_value, body);
}


expression_gen_t bcgen_lookup_element_expression(bcgenerator_t& gen_acc, const variable_address_t& target_reg, const expression_t& e, const expression_t::lookup_t& details, const bcgen_body_t& body){
	QUARK_ASSERT(gen_acc.check_invariant());
	QUARK_ASSERT(e.check_invariant());
	QUARK_ASSERT(body.check_invariant());

	auto body_acc = body;

	const auto& parent_expr = bcgen_expression(gen_acc, {}, *details.parent_address, body_acc);
	body_acc = parent_expr._body;

	const auto& key_expr = bcgen_expression(gen_acc, {}, *details.lookup_key, body_acc);
	body_acc = key_expr._body;

	const auto parent_type = gen_acc._types[parent_expr._type];
	const auto opcode = [&parent_type]{
		if(parent_type.is_string()){
			return bc_opcode::k_lookup_element_string;
		}
		else if(parent_type.is_json()){
			return bc_opcode::k_lookup_element_json;
		}
		else if(parent_type.is_vector()){
			if(encode_as_vector_w_inplace_elements(parent_type)){
				return bc_opcode::k_lookup_element_vector_w_inplace_elements;
			}
			else{
				return bc_opcode::k_lookup_element_vector_w_external_elements;
			}
		}
		else if(parent_type.is_dict()){
			if(encode_as_dict_w_inplace_values(parent_type)){
				return bc_opcode::k_lookup_element_dict_w_inplace_values;
			}
			else{
				return bc_opcode::k_lookup_element_dict_w_external_values;
			}
		}
		else{
			QUARK_ASSERT(false);
			quark::throw_exception();
		}
	}();

	const auto target_reg2 = target_reg.is_empty() ? add_local_temp(body_acc, e.get_output_type(), "temp: resolve-member output") : target_reg;

	body_acc._instrs.push_back(bcgen_instruction_t(
		opcode,
		target_reg2,
		parent_expr._out,
		key_expr._out
	));
	QUARK_ASSERT(body_acc.check_invariant());
	return { body_acc, target_reg2, intern_type(gen_acc, e.get_output_type()) };
}

//??? Value already sits in a register / global -- no need to generate code to copy it in most cases!
expression_gen_t bcgen_load2_expression(bcgenerator_t& gen_acc, const variable_address_t& target_reg, const expression_t& e, const expression_t::load2_t& details, const bcgen_body_t& body){
	QUARK_ASSERT(gen_acc.check_invariant());
	QUARK_ASSERT(e.check_invariant());
	QUARK_ASSERT(body.check_invariant());

	auto body_acc = body;
	const auto result_type = e.get_output_type();

	//	Shortcut: If we're loading a local-variable and are free from putting it in target_reg -- just access the register where it sits = no instruction!
	if(target_reg.is_empty() && details.address._parent_steps != variable_address_t::k_global_scope){
		QUARK_ASSERT(body_acc.check_invariant());
		return { body_acc, details.address, intern_type(gen_acc, result_type) };
	}
	else{
		const auto target_reg2 = target_reg.is_empty() ? add_local_temp(body_acc, e.get_output_type(), "temp: load2") : target_reg;
		body_acc = copy_value(result_type, target_reg2, details.address, body_acc);

		QUARK_ASSERT(body_acc.check_invariant());
		return { body_acc, target_reg2, intern_type(gen_acc, result_type) };
	}
}

struct call_spec_t {
	typeid_t _return;
	std::vector<typeid_t> _args;

	uint32_t _dynbits;
	uint32_t _extbits;
	uint32_t _stack_element_count;
};

uint32_t pack_bools(const std::vector<bool>& bools){
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
	bcgen_body_t _body;
	std::vector<bool> _exts;
	int _stack_count;
};

//??? if zero or few arguments, use shortcuts. Stuff single arg into instruction reg_c etc.
//??? Do interning of add_local_const().
//??? make different types for register vs stack-pos.

//	NOTICE: extbits are generated for every value on callstack, even for DYN-types. This is correct.
call_setup_t gen_call_setup(bcgenerator_t& gen_acc, const std::vector<typeid_t>& function_def_arg_type, const expression_t* args, int callee_arg_count, const bcgen_body_t& body){
	QUARK_ASSERT(gen_acc.check_invariant());
	QUARK_ASSERT(args != nullptr || callee_arg_count == 0);
	QUARK_ASSERT(body.check_invariant());
	QUARK_ASSERT(callee_arg_count == function_def_arg_type.size());

	auto body_acc = body;

	int dynamic_arg_count = count_function_dynamic_args(function_def_arg_type);
	const auto arg_count = callee_arg_count;

	//	Generate code / symbols for all arguments to the function call. Record where each arg is kept.
	//	This might not create instructions or anything, if arguments are available as constants somewhere.
	std::vector<std::pair<reg_t, bc_typeid_t>> argument_regs;
	for(int i = 0 ; i < arg_count ; i++){
		const auto& m2 = bcgen_expression(gen_acc, {}, args[i], body_acc);
		body_acc = m2._body;
		argument_regs.push_back(std::pair<reg_t, bc_typeid_t>(m2._out, m2._type));
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

		//	Prepend internal-dynamic arguments with the itype of the actual callee-argument.
		if(func_arg_type.is_any()){
			const auto arg_type_reg = add_local_const(body_acc, value_t::make_int(callee_arg_type), "DYN type arg #" + std::to_string(i));
			body_acc._instrs.push_back(bcgen_instruction_t(bc_opcode::k_push_inplace_value, arg_type_reg, {}, {} ));

			//	Int don't need extbit.
			exts.push_back(false);
		}

		const auto arg_type_full = gen_acc._types[callee_arg_type];
		bool ext = encode_as_external(arg_type_full);
		if(ext){
			body_acc._instrs.push_back(bcgen_instruction_t(bc_opcode::k_push_external_value, arg_reg, {}, {} ));
			exts.push_back(true);
		}
		else{
			body_acc._instrs.push_back(bcgen_instruction_t(bc_opcode::k_push_inplace_value, arg_reg, {}, {} ));
			exts.push_back(false);
		}
	}
	int stack_count = arg_count + dynamic_arg_count;

	QUARK_ASSERT(gen_acc.check_invariant());
	QUARK_ASSERT(body_acc.check_invariant());
	return { body_acc, exts, stack_count };
}


/*
	Handles a call-expression. Output is one of these:

	- A call expression
	- A hard-coded opcode like size() and push_back().
*/
static expression_gen_t bcgen_call_expression(bcgenerator_t& gen_acc, const variable_address_t& target_reg, const typeid_t& call_output_type, const expression_t::call_t& details, const bcgen_body_t& body){
	QUARK_ASSERT(gen_acc.check_invariant());
	QUARK_ASSERT(call_output_type.check_invariant());
	QUARK_ASSERT(body.check_invariant());

	auto body_acc = body;

	const auto callee_arg_count = static_cast<int>(details.args.size());

	const auto function_type = details.callee->get_output_type();
	const auto function_def_arg_types = function_type.get_function_args();
	QUARK_ASSERT(callee_arg_count == function_def_arg_types.size());
	const auto return_type = call_output_type;

	QUARK_ASSERT(callee_arg_count == function_def_arg_types.size());
	const auto arg_count = callee_arg_count;

	body_acc._instrs.push_back(bcgen_instruction_t(bc_opcode::k_push_frame_ptr, {}, {}, {} ));

	const auto& callee_expr = bcgen_expression(gen_acc, {}, *details.callee, body_acc);
	body_acc = callee_expr._body;

	const auto call_setup = gen_call_setup(gen_acc, function_def_arg_types, &details.args[0], callee_arg_count, body_acc);
	body_acc = call_setup._body;

	const auto target_reg2 = target_reg.is_empty() ? add_local_temp(body_acc, call_output_type, "temp: call return") : target_reg;

	//??? No need to allocate return register if functions returns void.
	QUARK_ASSERT(return_type.is_any() == false && return_type.is_undefined() == false)
	body_acc._instrs.push_back(bcgen_instruction_t(
		bc_opcode::k_call,
		target_reg2,
		callee_expr._out,
		make_imm_int(arg_count)
	));

	const auto extbits = pack_bools(call_setup._exts);
	body_acc._instrs.push_back(bcgen_instruction_t(bc_opcode::k_popn, make_imm_int(call_setup._stack_count), make_imm_int(extbits), {} ));
	body_acc._instrs.push_back(bcgen_instruction_t(bc_opcode::k_pop_frame_ptr, {}, {}, {} ));

	QUARK_ASSERT(body_acc.check_invariant());
	return { body_acc, target_reg2, intern_type(gen_acc, return_type) };
}









static expression_gen_t bcgen_corecall_expression(bcgenerator_t& gen_acc, const variable_address_t& target_reg, const typeid_t& call_output_type, const expression_t::corecall_t& details, const bcgen_body_t& body){
	QUARK_ASSERT(gen_acc.check_invariant());
	QUARK_ASSERT(target_reg.check_invariant());
	QUARK_ASSERT(call_output_type.check_invariant());
	QUARK_ASSERT(body.check_invariant());


	if(details.call_name == get_corecall_opcode(make_assert_signature())){
		return bcgen_make_fallthrough_corecall(gen_acc, target_reg, call_output_type, details, body);
	}
	else if(details.call_name == get_corecall_opcode(make_to_string_signature())){
		return bcgen_make_fallthrough_corecall(gen_acc, target_reg, call_output_type, details, body);
	}
	else if(details.call_name == get_corecall_opcode(make_to_pretty_string_signature())){
		return bcgen_make_fallthrough_corecall(gen_acc, target_reg, call_output_type, details, body);
	}


	else if(details.call_name == get_corecall_opcode(make_typeof_signature())){
		return bcgen_make_fallthrough_corecall(gen_acc, target_reg, call_output_type, details, body);
	}


	else if(details.call_name == get_corecall_opcode(make_update_signature())){
		return bcgen_make_fallthrough_corecall(gen_acc, target_reg, call_output_type, details, body);
	}
	else if(details.call_name == get_corecall_opcode(make_size_signature())){
		return bcgen_corecall_size_expression(gen_acc, target_reg, call_output_type, details, body);
	}
	else if(details.call_name == get_corecall_opcode(make_find_signature())){
		return bcgen_make_fallthrough_corecall(gen_acc, target_reg, call_output_type, details, body);
	}
	else if(details.call_name == get_corecall_opcode(make_exists_signature())){
		return bcgen_make_fallthrough_corecall(gen_acc, target_reg, call_output_type, details, body);
	}
	else if(details.call_name == get_corecall_opcode(make_erase_signature())){
		return bcgen_make_fallthrough_corecall(gen_acc, target_reg, call_output_type, details, body);
	}
	else if(details.call_name == get_corecall_opcode(make_get_keys_signature())){
		return bcgen_make_fallthrough_corecall(gen_acc, target_reg, call_output_type, details, body);
	}
	else if(details.call_name == get_corecall_opcode(make_push_back_signature())){
		return bcgen_corecall_push_back_expression(gen_acc, target_reg, call_output_type, details, body);
	}
	else if(details.call_name == get_corecall_opcode(make_subset_signature())){
		return bcgen_make_fallthrough_corecall(gen_acc, target_reg, call_output_type, details, body);
	}
	else if(details.call_name == get_corecall_opcode(make_replace_signature())){
		return bcgen_make_fallthrough_corecall(gen_acc, target_reg, call_output_type, details, body);
	}


	else if(details.call_name == get_corecall_opcode(make_parse_json_script_signature())){
		return bcgen_make_fallthrough_corecall(gen_acc, target_reg, call_output_type, details, body);
	}
	else if(details.call_name == get_corecall_opcode(make_generate_json_script_signature())){
		return bcgen_make_fallthrough_corecall(gen_acc, target_reg, call_output_type, details, body);
	}
	else if(details.call_name == get_corecall_opcode(make_to_json_signature())){
		return bcgen_make_fallthrough_corecall(gen_acc, target_reg, call_output_type, details, body);
	}
	else if(details.call_name == get_corecall_opcode(make_from_json_signature())){
		return bcgen_make_fallthrough_corecall(gen_acc, target_reg, call_output_type, details, body);
	}


	else if(details.call_name == get_corecall_opcode(make_get_json_type_signature())){
		return bcgen_make_fallthrough_corecall(gen_acc, target_reg, call_output_type, details, body);
	}


	else if(details.call_name == get_corecall_opcode(make_map_signature())){
		return bcgen_make_fallthrough_corecall(gen_acc, target_reg, call_output_type, details, body);
	}
	else if(details.call_name == get_corecall_opcode(make_map_string_signature())){
		return bcgen_make_fallthrough_corecall(gen_acc, target_reg, call_output_type, details, body);
	}
	else if(details.call_name == get_corecall_opcode(make_map_dag_signature())){
		return bcgen_make_fallthrough_corecall(gen_acc, target_reg, call_output_type, details, body);
	}
	else if(details.call_name == get_corecall_opcode(make_filter_signature())){
		return bcgen_make_fallthrough_corecall(gen_acc, target_reg, call_output_type, details, body);
	}
	else if(details.call_name == get_corecall_opcode(make_reduce_signature())){
		return bcgen_make_fallthrough_corecall(gen_acc, target_reg, call_output_type, details, body);
	}
	else if(details.call_name == get_corecall_opcode(make_stable_sort_signature())){
		return bcgen_make_fallthrough_corecall(gen_acc, target_reg, call_output_type, details, body);
	}



	else if(details.call_name == get_corecall_opcode(make_print_signature())){
		return bcgen_make_fallthrough_corecall(gen_acc, target_reg, call_output_type, details, body);
	}
	else if(details.call_name == get_corecall_opcode(make_send_signature())){
		return bcgen_make_fallthrough_corecall(gen_acc, target_reg, call_output_type, details, body);
	}


	else if(details.call_name == get_corecall_opcode(make_bw_not_signature())){
		return bcgen_make_fallthrough_corecall(gen_acc, target_reg, call_output_type, details, body);
	}
	else if(details.call_name == get_corecall_opcode(make_bw_and_signature())){
		return bcgen_make_fallthrough_corecall(gen_acc, target_reg, call_output_type, details, body);
	}
	else if(details.call_name == get_corecall_opcode(make_bw_or_signature())){
		return bcgen_make_fallthrough_corecall(gen_acc, target_reg, call_output_type, details, body);
	}
	else if(details.call_name == get_corecall_opcode(make_bw_xor_signature())){
		return bcgen_make_fallthrough_corecall(gen_acc, target_reg, call_output_type, details, body);
	}
	else if(details.call_name == get_corecall_opcode(make_bw_shift_left_signature())){
		return bcgen_make_fallthrough_corecall(gen_acc, target_reg, call_output_type, details, body);
	}
	else if(details.call_name == get_corecall_opcode(make_bw_shift_right_signature())){
		return bcgen_make_fallthrough_corecall(gen_acc, target_reg, call_output_type, details, body);
	}
	else if(details.call_name == get_corecall_opcode(make_bw_shift_right_arithmetic_signature())){
		return bcgen_make_fallthrough_corecall(gen_acc, target_reg, call_output_type, details, body);
	}


	else{
		QUARK_ASSERT(false);
	}
	throw std::exception();
}



//??? Submit dest-register to all gen-functions = minimize temps.
//??? Wrap itype in struct to make it typesafe.
//??? Only use itypes in interpreter

expression_gen_t bcgen_construct_value_expression(bcgenerator_t& gen_acc, const variable_address_t& target_reg, const expression_t& e, const expression_t::value_constructor_t& details, const bcgen_body_t& body){
	QUARK_ASSERT(gen_acc.check_invariant());
	QUARK_ASSERT(e.check_invariant());
	QUARK_ASSERT(body.check_invariant());

	auto body_acc = body;

	const auto target_type = e.get_output_type();
	const auto target_itype = intern_type(gen_acc, target_type);

	const auto callee_arg_count = static_cast<int>(details.elements.size());
	std::vector<typeid_t> arg_types;
	for(const auto& m: details.elements){
		arg_types.push_back(m.get_output_type());
	}
	const auto arg_count = callee_arg_count;

	const auto call_setup = gen_call_setup(gen_acc, arg_types, &details.elements[0], arg_count, body_acc);
	body_acc = call_setup._body;

	const auto source_itype = arg_count == 0 ? -1 : intern_type(gen_acc, details.elements[0].get_output_type());

	const auto target_reg2 = target_reg.is_empty() ? add_local_temp(body_acc, e.get_output_type(), "temp: construct value result") : target_reg;

	if(target_type.is_vector()){
		if(encode_as_vector_w_inplace_elements(target_type)){
			body_acc._instrs.push_back(bcgen_instruction_t(
				bc_opcode::k_new_vector_w_inplace_elements,
				target_reg2,
				make_imm_int(0),
				make_imm_int(arg_count)
			));
		}
		else{
			body_acc._instrs.push_back(bcgen_instruction_t(
				bc_opcode::k_new_vector_w_external_elements,
				target_reg2,
				make_imm_int(target_itype),
				make_imm_int(arg_count)
			));
		}
	}
	else if(target_type.is_dict()){
		if(encode_as_dict_w_inplace_values(target_type)){
			body_acc._instrs.push_back(bcgen_instruction_t(
				bc_opcode::k_new_dict_w_inplace_values,
				target_reg2,
				make_imm_int(target_itype),
				make_imm_int(arg_count)
			));
		}
		else{
			body_acc._instrs.push_back(bcgen_instruction_t(
				bc_opcode::k_new_dict_w_external_values,
				target_reg2,
				make_imm_int(target_itype),
				make_imm_int(arg_count)
			));
		}
	}
	else if(target_type.is_struct()){
		body_acc._instrs.push_back(bcgen_instruction_t(
			bc_opcode::k_new_struct,
			target_reg2,
			make_imm_int(target_itype),
			make_imm_int(arg_count)
		));
	}
	else{
		QUARK_ASSERT(arg_count == 1);

		body_acc._instrs.push_back(bcgen_instruction_t(
			bc_opcode::k_new_1,
			target_reg2,
			make_imm_int(target_itype),
			make_imm_int(source_itype)
		));
	}

	const auto extbits = pack_bools(call_setup._exts);
	body_acc._instrs.push_back(bcgen_instruction_t(bc_opcode::k_popn, make_imm_int(call_setup._stack_count), make_imm_int(extbits), {} ));

	QUARK_ASSERT(body_acc.check_invariant());
	return { body_acc, target_reg2, target_itype };
}

expression_gen_t bcgen_benchmark_expression(bcgenerator_t& gen_acc, const variable_address_t& target_reg, const expression_t& e, const expression_t::benchmark_expr_t& details, const bcgen_body_t& body){
	QUARK_ASSERT(gen_acc.check_invariant());
	QUARK_ASSERT(e.check_invariant());
	QUARK_ASSERT(body.check_invariant());

	return bcgen_expression(gen_acc, target_reg, expression_t::make_literal_int(404), body);
}



expression_gen_t bcgen_arithmetic_unary_minus_expression(bcgenerator_t& gen_acc, const variable_address_t& target_reg, const expression_t& e, const expression_t::unary_minus_t& details, const bcgen_body_t& body){
	QUARK_ASSERT(gen_acc.check_invariant());
	QUARK_ASSERT(e.check_invariant());
	QUARK_ASSERT(body.check_invariant());

	const auto type = details.expr->get_output_type();

	if(type.is_int()){
		const auto e2 = expression_t::make_arithmetic(expression_type::k_arithmetic_subtract, expression_t::make_literal_int(0), *details.expr, e._output_type);
		return bcgen_expression(gen_acc, target_reg, e2, body);
	}
	else if(type.is_double()){
		const auto e2 = expression_t::make_arithmetic(expression_type::k_arithmetic_subtract, expression_t::make_literal_double(0), *details.expr, e._output_type);
		return bcgen_expression(gen_acc, target_reg, e2, body);
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
expression_gen_t bcgen_conditional_operator_expression(bcgenerator_t& gen_acc, const variable_address_t& target_reg, const expression_t& e, const expression_t::conditional_t& details, const bcgen_body_t& body){
	QUARK_ASSERT(gen_acc.check_invariant());
	QUARK_ASSERT(e.check_invariant());
	QUARK_ASSERT(body.check_invariant());

	auto body_acc = body;

	const auto& condition_expr = bcgen_expression(gen_acc, {}, *details.condition, body_acc);
	body_acc = condition_expr._body;

	const auto result_type = e.get_output_type();
	const auto result_itype = intern_type(gen_acc, result_type);

	const auto target_reg2 = target_reg.is_empty() ? add_local_temp(body_acc, e.get_output_type(), "temp: condition value output") : target_reg;

	int jump1_pc = static_cast<int>(body_acc._instrs.size());
	body_acc._instrs.push_back(
		bcgen_instruction_t(
			bc_opcode::k_branch_false_bool,
			condition_expr._out,
			make_imm_int(0xbeefdeeb),
			{}
		)
	);

	////////	A expression

	const auto& a_expr = bcgen_expression(gen_acc, target_reg2, *details.a, body_acc);
	body_acc = a_expr._body;

	int jump2_pc = static_cast<int>(body_acc._instrs.size());
	body_acc._instrs.push_back(
		bcgen_instruction_t(
			bc_opcode::k_branch_always,
			make_imm_int(0xfea57be7),
			{},
			{}
		)
	);

	////////	B expression

	int b_pc = static_cast<int>(body_acc._instrs.size());
	const auto& b_expr = bcgen_expression(gen_acc, target_reg2, *details.b, body_acc);
	body_acc = b_expr._body;

	int end_pc = static_cast<int>(body_acc._instrs.size());

	QUARK_ASSERT(body_acc._instrs[jump1_pc]._reg_b._index == 0xbeefdeeb);
	QUARK_ASSERT(body_acc._instrs[jump2_pc]._reg_a._index == 0xfea57be7);
	body_acc._instrs[jump1_pc]._reg_b._index = b_pc - jump1_pc;
	body_acc._instrs[jump2_pc]._reg_a._index = end_pc - jump2_pc;

	QUARK_ASSERT(body_acc.check_invariant());
	return { body_acc, target_reg2, result_itype };
}

expression_gen_t bcgen_comparison_expression(bcgenerator_t& gen_acc, const variable_address_t& target_reg, expression_type op, const expression_t& e, const expression_t::comparison_t& details, const bcgen_body_t& body){
	QUARK_ASSERT(gen_acc.check_invariant());
	QUARK_ASSERT(e.check_invariant());
	QUARK_ASSERT(body.check_invariant());

	auto body_acc = body;

	const auto& left_expr = bcgen_expression(gen_acc, {}, *details.lhs, body_acc);
	body_acc = left_expr._body;

	const auto& right_expr = bcgen_expression(gen_acc, {}, *details.rhs, body_acc);
	body_acc = right_expr._body;

	//	Type is the data the opcode works on -- comparing two ints, comparing two strings etc.
	const auto type = details.lhs->get_output_type();

	//	Output reg always a bool.
	QUARK_ASSERT(e.get_output_type().is_bool());
	const auto target_reg2 = target_reg.is_empty() ? add_local_temp(body_acc, e.get_output_type(), "temp: comparison flag") : target_reg;

	if(type.is_int()){

		//	Bool tells if to flip left / right.
		static const std::map<expression_type, std::pair<bool, bc_opcode>> conv_opcode_int = {
			{ expression_type::k_comparison_smaller_or_equal,			{ false, bc_opcode::k_comparison_smaller_or_equal_int } },
			{ expression_type::k_comparison_smaller,						{ false, bc_opcode::k_comparison_smaller_int } },
			{ expression_type::k_comparison_larger_or_equal,				{ true, bc_opcode::k_comparison_smaller_or_equal_int } },
			{ expression_type::k_comparison_larger,						{ true, bc_opcode::k_comparison_smaller_int } },

			{ expression_type::k_logical_equal,							{ false, bc_opcode::k_logical_equal_int } },
			{ expression_type::k_logical_nonequal,						{ false, bc_opcode::k_logical_nonequal_int } }
		};

		const auto result = conv_opcode_int.at(details.op);
		if(result.first == false){
			body_acc._instrs.push_back(bcgen_instruction_t(result.second, target_reg2, left_expr._out, right_expr._out));
		}
		else{
			body_acc._instrs.push_back(bcgen_instruction_t(result.second, target_reg2, right_expr._out, left_expr._out));
		}
	}
	else{

		//	Bool tells if to flip left / right.
		static const std::map<expression_type, std::pair<bool, bc_opcode>> conv_opcode = {
			{ expression_type::k_comparison_smaller_or_equal,			{ false, bc_opcode::k_comparison_smaller_or_equal } },
			{ expression_type::k_comparison_smaller,						{ false, bc_opcode::k_comparison_smaller } },
			{ expression_type::k_comparison_larger_or_equal,				{ true, bc_opcode::k_comparison_smaller_or_equal } },
			{ expression_type::k_comparison_larger,						{ true, bc_opcode::k_comparison_smaller } },

			{ expression_type::k_logical_equal,							{ false, bc_opcode::k_logical_equal } },
			{ expression_type::k_logical_nonequal,						{ false, bc_opcode::k_logical_nonequal } }
		};

		const auto result = conv_opcode.at(details.op);
		if(result.first == false){
			body_acc._instrs.push_back(bcgen_instruction_t(result.second, target_reg2, left_expr._out, right_expr._out));
		}
		else{
			body_acc._instrs.push_back(bcgen_instruction_t(result.second, target_reg2, right_expr._out, left_expr._out));
		}
	}

	QUARK_ASSERT(body_acc.check_invariant());
	return { body_acc, target_reg2, intern_type(gen_acc, typeid_t::make_bool()) };
}

expression_gen_t bcgen_arithmetic_expression(bcgenerator_t& gen_acc, const variable_address_t& target_reg, expression_type op, const expression_t& e, const expression_t::arithmetic_t& details, const bcgen_body_t& body){
	QUARK_ASSERT(gen_acc.check_invariant());
	QUARK_ASSERT(e.check_invariant());
	QUARK_ASSERT(body.check_invariant());

	auto body_acc = body;

	const auto& left_expr = bcgen_expression(gen_acc, {}, *details.lhs, body_acc);
	body_acc = left_expr._body;

	const auto& right_expr = bcgen_expression(gen_acc, {}, *details.rhs, body_acc);
	body_acc = right_expr._body;

	const auto type = details.lhs->get_output_type();
	const auto itype = intern_type(gen_acc, type);

	const auto target_reg2 = target_reg.is_empty() ? add_local_temp(body_acc, e.get_output_type(), "temp: arithmetic output") : target_reg;

	const auto opcode = [&type, &e, &details]{
		if(type.is_bool()){
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
		else if(type.is_int()){
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
		else if(type.is_double()){
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
		else if(type.is_string()){
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
		else if(type.is_vector()){
			if(encode_as_vector_w_inplace_elements(type)){
				static const std::map<expression_type, bc_opcode> conv_opcode = {
					{ expression_type::k_arithmetic_add, bc_opcode::k_concat_vectors_w_inplace_elements },
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
		}
		else{
			QUARK_ASSERT(false);
			quark::throw_exception();
		}
	}();

	QUARK_ASSERT(opcode != bc_opcode::k_nop);
	body_acc._instrs.push_back(bcgen_instruction_t(opcode, target_reg2, left_expr._out, right_expr._out));

	QUARK_ASSERT(body_acc.check_invariant());
	return { body_acc, target_reg2, itype };
}

expression_gen_t bcgen_literal_expression(bcgenerator_t& gen_acc, const variable_address_t& target_reg, const expression_t& e, const expression_t::literal_exp_t& details, const bcgen_body_t& body){
	QUARK_ASSERT(gen_acc.check_invariant());
	QUARK_ASSERT(body.check_invariant());

	auto body_acc = body;

	const auto const_temp = add_local_const(body_acc, e.get_literal(), "literal constant");
	if(target_reg.is_empty()){
		QUARK_ASSERT(body_acc.check_invariant());
		return { body_acc, const_temp, intern_type(gen_acc, *e._output_type) };
	}

	//	We need to copy the value to the target reg...
	else{
		const auto result_type = e.get_output_type();
		body_acc = copy_value(result_type, target_reg, const_temp, body_acc);

		QUARK_ASSERT(body_acc.check_invariant());
		return { body_acc, target_reg, intern_type(gen_acc, result_type) };
	}
}

expression_gen_t bcgen_expression(bcgenerator_t& gen_acc, const variable_address_t& target_reg, const expression_t& e, const bcgen_body_t& body){
	QUARK_ASSERT(gen_acc.check_invariant());
	QUARK_ASSERT(target_reg.check_invariant());
	QUARK_ASSERT(e.check_invariant());
	QUARK_ASSERT(body.check_invariant());

	struct visitor_t {
		bcgenerator_t& gen_acc;
		const variable_address_t& target_reg;
		const expression_t& e;
		const bcgen_body_t& body;


		expression_gen_t operator()(const expression_t::literal_exp_t& expr) const{
			return bcgen_literal_expression(gen_acc, target_reg, e, expr, body);
		}
		expression_gen_t operator()(const expression_t::arithmetic_t& expr) const{
			return bcgen_arithmetic_expression(gen_acc, target_reg, expr.op, e, expr, body);
		}
		expression_gen_t operator()(const expression_t::comparison_t& expr) const{
			return bcgen_comparison_expression(gen_acc, target_reg, expr.op, e, expr, body);
		}
		expression_gen_t operator()(const expression_t::unary_minus_t& expr) const{
			return bcgen_arithmetic_unary_minus_expression(gen_acc, target_reg, e, expr, body);
		}
		expression_gen_t operator()(const expression_t::conditional_t& expr) const{
			return bcgen_conditional_operator_expression(gen_acc, target_reg, e, expr, body);
		}

		expression_gen_t operator()(const expression_t::call_t& expr) const{
			return bcgen_call_expression(gen_acc, target_reg, e.get_output_type(), expr, body);
		}
		expression_gen_t operator()(const expression_t::corecall_t& expr) const{
			return bcgen_corecall_expression(gen_acc, target_reg, e.get_output_type(), expr, body);
		}

		expression_gen_t operator()(const expression_t::struct_definition_expr_t& expr) const{
			QUARK_ASSERT(false);
			throw std::exception();
		}
		expression_gen_t operator()(const expression_t::function_definition_expr_t& expr) const{
			QUARK_ASSERT(false);
			throw std::exception();
		}
		expression_gen_t operator()(const expression_t::load_t& expr) const{
			QUARK_ASSERT(false);
			throw std::exception();
		}
		expression_gen_t operator()(const expression_t::load2_t& expr) const{
			return bcgen_load2_expression(gen_acc, target_reg, e, expr, body);
		}

		expression_gen_t operator()(const expression_t::resolve_member_t& expr) const{
			return bcgen_resolve_member_expression(gen_acc, target_reg, e, expr, body);
		}
		expression_gen_t operator()(const expression_t::update_member_t& expr) const{
			return bcgen_update_member_expression(gen_acc, target_reg, e, expr, body);
		}
		expression_gen_t operator()(const expression_t::lookup_t& expr) const{
			return bcgen_lookup_element_expression(gen_acc, target_reg, e, expr, body);
		}
		expression_gen_t operator()(const expression_t::value_constructor_t& expr) const{
			return bcgen_construct_value_expression(gen_acc, target_reg, e, expr, body);
		}
		expression_gen_t operator()(const expression_t::benchmark_expr_t& expr) const{
			return bcgen_benchmark_expression(gen_acc, target_reg, e, expr, body);
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
	_globals(other._globals),
	_types(other._types)
{
	QUARK_ASSERT(other.check_invariant());
	QUARK_ASSERT(check_invariant());
}

void bcgenerator_t::swap(bcgenerator_t& other) throw(){
	other._ast_imm.swap(this->_ast_imm);
	std::swap(other._globals, this->_globals);
	other._types.swap(this->_types);
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


bc_instruction_t squeeze_instruction(const bcgen_instruction_t& instruction){
	QUARK_ASSERT(instruction.check_invariant());

	QUARK_ASSERT(instruction._reg_a._parent_steps == 0 || instruction._reg_a._parent_steps == variable_address_t::k_global_scope || instruction._reg_a._parent_steps == 666);
	QUARK_ASSERT(instruction._reg_b._parent_steps == 0 || instruction._reg_b._parent_steps == variable_address_t::k_global_scope || instruction._reg_b._parent_steps == 666);
	QUARK_ASSERT(instruction._reg_c._parent_steps == 0 || instruction._reg_c._parent_steps == variable_address_t::k_global_scope || instruction._reg_c._parent_steps == 666);
	QUARK_ASSERT(instruction._reg_a._index >= INT16_MIN && instruction._reg_a._index <= INT16_MAX);
	QUARK_ASSERT(instruction._reg_b._index >= INT16_MIN && instruction._reg_b._index <= INT16_MAX);
	QUARK_ASSERT(instruction._reg_c._index >= INT16_MIN && instruction._reg_c._index <= INT16_MAX);

	const auto encoding = k_opcode_info.at(instruction._opcode)._encoding;
	const auto reg_flags = encoding_to_reg_flags(encoding);

	QUARK_ASSERT(check_register__local(instruction._reg_a, reg_flags._a));
	QUARK_ASSERT(check_register__local(instruction._reg_b, reg_flags._b));
	QUARK_ASSERT(check_register__local(instruction._reg_c, reg_flags._c));


	const auto result = bc_instruction_t(
		instruction._opcode,
		static_cast<int16_t>(instruction._reg_a._index),
		static_cast<int16_t>(instruction._reg_b._index),
		static_cast<int16_t>(instruction._reg_c._index)
	);
	return result;
}

bc_static_frame_t make_frame(const bcgen_body_t& body, const std::vector<typeid_t>& args){
	QUARK_ASSERT(body.check_invariant());

	std::vector<bc_instruction_t> instrs2;
	for(const auto& e: body._instrs){
		instrs2.push_back(squeeze_instruction(e));
	}

	std::vector<std::pair<std::string, bc_symbol_t>> symbols2;
	for(const auto& e: body._symbol_table._symbols){
		const auto e2 = std::pair<std::string, bc_symbol_t>{
			e.first,
			bc_symbol_t{
				e.second._mutable_mode == symbol_t::mutable_mode::immutable ? bc_symbol_t::immutable : bc_symbol_t::mutable1,
				e.second._value_type,
				value_to_bc(e.second._init)
			}
		};
		symbols2.push_back(e2);
	}

	return bc_static_frame_t(instrs2, symbols2, args);
}

bc_program_t generate_bytecode(const semantic_ast_t& ast){
	QUARK_ASSERT(ast.check_invariant());

	if(trace_io_flag){
	//	QUARK_SCOPED_TRACE("generate_bytecode");
		QUARK_TRACE_SS("INPUT:  " << json_to_pretty_string(gp_ast_to_json(ast._tree)));
	}

	bcgenerator_t a(ast);

	bcgen_globals(a, a._ast_imm->_tree._globals);

	std::map<function_id_t, bc_function_definition_t> function_defs2;
	for(auto function_id = 0 ; function_id < ast._tree._function_defs.size() ; function_id++){
		const auto& function_def = ast._tree._function_defs[function_id];

		struct visitor_t {
			bcgenerator_t& gen_acc;
			const floyd::function_definition_t& function_def;

			bc_function_definition_t operator()(const function_definition_t::empty_t& e) const{
				QUARK_ASSERT(false);
				throw std::exception();
			}
			bc_function_definition_t operator()(const function_definition_t::floyd_func_t& e) const{
				if(e._body){
					const auto body2 = bcgen_function(gen_acc, function_def);

					const auto frame = make_frame(body2, function_def._function_type.get_function_args());
					const auto f = bc_function_definition_t{
						function_def._function_type,
						function_def._named_args,
						std::make_shared<bc_static_frame_t>(frame),
						function_id_t { function_def._definition_name }
					};
					return f;
				}
				else{
					const auto f = bc_function_definition_t{
						function_def._function_type,
						function_def._named_args,
						std::shared_ptr<bc_static_frame_t>(),
						function_id_t { function_def._definition_name }
					};
					return f;
				}
			}
			bc_function_definition_t operator()(const function_definition_t::host_func_t& e) const{
				const auto f = bc_function_definition_t{
					function_def._function_type,
					function_def._named_args,
					nullptr,
					function_id_t { function_def._definition_name }
				};
				return f;
			}
		};
		const auto function_def2 = std::visit(visitor_t{ a, function_def }, function_def._contents);
		function_defs2.insert({ function_def2._function_id, function_def2 });
	}

	const auto globals2 = make_frame(a._globals, {});
	const auto result = bc_program_t{
		globals2,
		function_defs2,
		a._types, ast._tree._software_system,
		ast._tree._container_def
	};

	if(trace_io_flag){
		QUARK_TRACE_SS("OUTPUT: " << json_to_pretty_string(bcprogram_to_json(result)));
	}

	return result;
}


}	//	floyd
