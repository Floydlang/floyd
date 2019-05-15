//
//  parser_evaluator.cpp
//  FloydSpeak
//
//  Created by Marcus Zetterquist on 26/07/16.
//  Copyright © 2016 Marcus Zetterquist. All rights reserved.
//

#include "bytecode_generator.h"
#include "pass3.h"
#include "floyd_interpreter.h"

#include <cmath>
#include <algorithm>
#include <cstdint>

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

	bcgen_body_t(const std::vector<bcgen_instruction_t>& instructions, const symbol_table_t& symbols) :
		_instrs(instructions),
		_symbols(symbols)
	{
		QUARK_ASSERT(check_invariant());
	}

	public: bool check_invariant() const;


	////////////////////////		STATE
	symbol_table_t _symbols;
	std::vector<bcgen_instruction_t> _instrs;
};

bc_static_frame_t make_frame(const bcgen_body_t& body, const std::vector<typeid_t>& args);


//////////////////////////////////////		bcgen_environment_t

/*
	Runtime scope, similar to a stack frame.
*/

struct bcgen_environment_t {

	//////////////////////////////////////		STATE
	public: const bcgen_body_t* _body_ptr;
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

	//	Holds all values for all environments.
	public: std::vector<bcgen_environment_t> _call_stack;

	public: std::vector<typeid_t> _types;
};


//////////////////////////////////////		bcgenerator_t

//	Why: needed to return from codegen functions that process expressions. Any new instructions have been recorded into _body.

struct expression_gen_t {

	//////////////////////////////////////		STATE
	bcgen_body_t _body;

	variable_address_t _out;

	//	Output type.
	bc_typeid_t _type;
};


/*
	target_reg: if defined, this is where the output value will be stored. If undefined, then the expression allocates (or redirect to existing register).
	expression_gen_t._out: always holds the output register, no matter who decided it.
*/
expression_gen_t bcgen_expression(bcgenerator_t& vm, const variable_address_t& target_reg, const expression_t& e, const bcgen_body_t& body);
bcgen_body_t bcgen_body_top(bcgenerator_t& vm, const body_t& body);
bcgen_body_t bcgen_body_block(bcgenerator_t& vm, const body_t& body);




variable_address_t add_local_temp(bcgen_body_t& body_acc, const typeid_t& type, const std::string& name){
	int id = add_temp(body_acc._symbols, name, type);
	return variable_address_t::make_variable_address(0, id);
}

variable_address_t add_local_const(bcgen_body_t& body_acc, const value_t& value, const std::string& name){
	int id = add_constant_literal(body_acc._symbols, name, value);
	return variable_address_t::make_variable_address(0, id);
}

//	Use to stuff an integer into an instruction, in one of the register slots.
variable_address_t make_imm_int(int value){
	return variable_address_t::make_variable_address(666, value);
}

bc_typeid_t intern_type(bcgenerator_t& vm, const typeid_t& type){
	QUARK_ASSERT(vm.check_invariant());
	QUARK_ASSERT(type.check_invariant());

    const auto it = std::find_if(vm._types.begin(), vm._types.end(), [&type](const typeid_t& e) { return e == type; });
	if(it != vm._types.end()){
		const auto pos = static_cast<bc_typeid_t>(it - vm._types.begin());
		return pos;
	}
	else{
		vm._types.push_back(type);
		return static_cast<bc_typeid_t>(vm._types.size() - 1);
	}

	QUARK_ASSERT(vm.check_invariant());
}

reg_t flatten_reg(const reg_t& r, int offset){
	QUARK_ASSERT(r.check_invariant());

	if(r._parent_steps == 666){
		return r;
	}
	else if(r._parent_steps == 0){
		return reg_t::make_variable_address(0, r._index + offset);
	}
	else if(r._parent_steps == -1){
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
		QUARK_ASSERT(reg._parent_steps == 666 || (reg._parent_steps == -1 && reg._index == -1));
	}
	return true;
}

bool check_register_nonlocal(const reg_t& reg, bool is_reg){
	if(is_reg){
		//	Must not be a global -- those should have been turned to global-access opcodes.
		QUARK_ASSERT(reg._parent_steps != -1);
	}
	else{
		QUARK_ASSERT(reg._parent_steps == 666 || (reg._parent_steps == -1 && reg._index == -1));
	}
	return true;
}

bool check_register__local(const reg_t& reg, bool is_reg){
	if(is_reg){
		//	Must not be a global -- those should have been turned to global-access opcodes.
		QUARK_ASSERT(reg._parent_steps != -1);
	}
	else{
		QUARK_ASSERT(reg._parent_steps == 666 || (reg._parent_steps == -1 && reg._index == -1));
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
	for(const auto& e: _symbols._symbols){
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


bcgen_body_t flatten_body(bcgenerator_t& vm, const bcgen_body_t& dest, const bcgen_body_t& source){
	QUARK_ASSERT(vm.check_invariant());
	QUARK_ASSERT(dest.check_invariant());
	QUARK_ASSERT(source.check_invariant());

	std::vector<bcgen_instruction_t> instructions;
	int offset = static_cast<int>(dest._symbols._symbols.size());
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
	body_acc._symbols._symbols.insert(body_acc._symbols._symbols.end(), source._symbols._symbols.begin(), source._symbols._symbols.end());
	return body_acc;
}



//	Supports globals & locals both as dest and sources.
bcgen_body_t copy_value(bcgenerator_t& vm, const typeid_t& type, const reg_t& dest_reg, const reg_t& source_reg, const bcgen_body_t& body){
	QUARK_ASSERT(vm.check_invariant());
	QUARK_ASSERT(type.check_invariant());
	QUARK_ASSERT(dest_reg.check_invariant());
	QUARK_ASSERT(source_reg.check_invariant());

	auto body_acc = body;
	bool is_ext = encode_as_external(type);

	//	If this asserts, we should special-case and do nothing.
	QUARK_ASSERT(!(dest_reg == source_reg));


	//	global <= global
	if(dest_reg._parent_steps == -1 && source_reg._parent_steps == -1){
		QUARK_ASSERT(false);
		quark::throw_exception();
//		body_acc._instrs.push_back(bcgen_instruction_t(is_ext ? bc_opcode::k_load_global_external_value : bc_opcode::k_load_global_inplace_value, dest_reg, make_imm_int(source_reg._index), {}));
	}

	//	global <= local
	else if(dest_reg._parent_steps == -1 && source_reg._parent_steps != -1){
		body_acc._instrs.push_back(bcgen_instruction_t(is_ext ? bc_opcode::k_store_global_external_value : bc_opcode::k_store_global_inplace_value, make_imm_int(dest_reg._index), source_reg, {}));
	}
	//	local <= global
	else if(dest_reg._parent_steps != -1 && source_reg._parent_steps == -1){
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

bcgen_body_t bcgen_store2_statement(bcgenerator_t& vm, const statement_t::store2_t& statement, const bcgen_body_t& body){
	QUARK_ASSERT(vm.check_invariant());
	QUARK_ASSERT(body.check_invariant());

	auto body_acc = body;

	//	Shortcut: if destinatio is a local variable, have the expression write directly to that register.
	if(statement._dest_variable._parent_steps != -1){
		const auto expr = bcgen_expression(vm, statement._dest_variable, statement._expression, body_acc);
		body_acc = expr._body;
		QUARK_ASSERT(body_acc.check_invariant());
	}
	else{
		const auto expr = bcgen_expression(vm, {}, statement._expression, body_acc);
		body_acc = expr._body;
		body_acc = copy_value(vm, statement._expression.get_output_type(), statement._dest_variable, expr._out, body_acc);
		QUARK_ASSERT(body_acc.check_invariant());
	}
	return body_acc;
}

bcgen_body_t bcgen_block_statement(bcgenerator_t& vm, const statement_t::block_statement_t& statement, const bcgen_body_t& body){
	QUARK_ASSERT(vm.check_invariant());
	QUARK_ASSERT(body.check_invariant());

	const auto body_acc = bcgen_body_block(vm, statement._body);
	return flatten_body(vm, body, body_acc);
}

bcgen_body_t bcgen_return_statement(bcgenerator_t& vm, const statement_t::return_statement_t& statement, const bcgen_body_t& body){
	QUARK_ASSERT(vm.check_invariant());
	QUARK_ASSERT(body.check_invariant());

	auto body_acc = body;
	const auto expr = bcgen_expression(vm, {}, statement._expression, body);
	body_acc = expr._body;
	body_acc._instrs.push_back(bcgen_instruction_t(bc_opcode::k_return, expr._out, {}, {}));
	return body_acc;
}

bcgen_body_t bcgen_ifelse_statement(bcgenerator_t& vm, const statement_t::ifelse_statement_t& statement, const bcgen_body_t& body){
	QUARK_ASSERT(vm.check_invariant());
	QUARK_ASSERT(body.check_invariant());

	auto body_acc = body;

	const auto condition_expr = bcgen_expression(vm, {}, statement._condition, body_acc);
	body_acc = condition_expr._body;
	QUARK_ASSERT(statement._condition.get_output_type().is_bool());

	const auto& then_expr = bcgen_body_block(vm, statement._then_body);
	const auto& else_expr = bcgen_body_block(vm, statement._else_body);

	body_acc._instrs.push_back(
		bcgen_instruction_t(
			bc_opcode::k_branch_false_bool,
			condition_expr._out,
			make_imm_int(static_cast<int>(then_expr._instrs.size()) + 2),
			{}
		)
	);
	body_acc = flatten_body(vm, body_acc, then_expr);
	body_acc._instrs.push_back(
		bcgen_instruction_t(
			bc_opcode::k_branch_always,
			make_imm_int(static_cast<int>(else_expr._instrs.size()) + 1),
			{},
			{}
		)
	);
	body_acc = flatten_body(vm, body_acc, else_expr);
	return body_acc;
}

/*
	count_reg = start-expression
	end-expression

	B:
	k_branch_smaller_or_equal_int / k_branch_smaller_int FALSE, A

	BODY

	k_add_int
	k_branch_smaller_or_equal_int / k_branch_smaller_int, B TRUE
	A:
*/
int get_count(const std::vector<bcgen_instruction_t>& instructions){
	return static_cast<int>(instructions.size());
}

bcgen_body_t bcgen_for_statement(bcgenerator_t& vm, const statement_t::for_statement_t& statement, const bcgen_body_t& body){
	QUARK_ASSERT(vm.check_invariant());
	QUARK_ASSERT(body.check_invariant());

	auto body_acc = body;

	const auto start_expr = bcgen_expression(vm, {}, statement._start_expression, body_acc);
	body_acc = start_expr._body;

	const auto end_expr = bcgen_expression(vm, {}, statement._end_expression, body_acc);
	body_acc = end_expr._body;

	const auto const1_reg = add_local_const(body_acc, value_t::make_int(1), "integer 1, to decrement with");

	const auto& loop_body = bcgen_body_block(vm, statement._body);
	int body_instr_count = get_count(loop_body._instrs);

	QUARK_ASSERT(
		statement._range_type == statement_t::for_statement_t::k_closed_range
		|| statement._range_type ==statement_t::for_statement_t::k_open_range
	);
	const auto condition_opcode = statement._range_type == statement_t::for_statement_t::k_closed_range ? bc_opcode::k_branch_smaller_or_equal_int : bc_opcode::k_branch_smaller_int;

	//	IMPORTANT: Iterator register is the FIRST symbol of the loop body's symbol table.
	const auto counter_reg = variable_address_t::make_variable_address(0, static_cast<int>(body_acc._symbols._symbols.size()));
	body_acc._instrs.push_back(bcgen_instruction_t(bc_opcode::k_copy_reg_inplace_value, counter_reg, start_expr._out, {}));

	// Reuse start value as our counter.
	// Notice: we need to store iterator value in body's first register.
	int leave_pc = 1 + body_instr_count + 2;
	body_acc._instrs.push_back(bcgen_instruction_t(condition_opcode, end_expr._out, counter_reg, make_imm_int(leave_pc - get_count(body_acc._instrs))));

	int body_start_pc = get_count(body_acc._instrs);

	body_acc = flatten_body(vm, body_acc, loop_body);
	body_acc._instrs.push_back(bcgen_instruction_t(bc_opcode::k_add_int, counter_reg, counter_reg, const1_reg));
	body_acc._instrs.push_back(bcgen_instruction_t(condition_opcode, counter_reg, end_expr._out, make_imm_int(body_start_pc - get_count(body_acc._instrs))));

	QUARK_ASSERT(body_acc.check_invariant());
	return body_acc;
}

bcgen_body_t bcgen_while_statement(bcgenerator_t& vm, const statement_t::while_statement_t& statement, const bcgen_body_t& body){
	QUARK_ASSERT(vm.check_invariant());
	QUARK_ASSERT(body.check_invariant());

	auto body_acc = body;

	const auto& loop_body = bcgen_body_block(vm, statement._body);
	int body_instr_count = static_cast<int>(loop_body._instrs.size());
	const auto condition_pc = static_cast<int>(body_acc._instrs.size());

	const auto condition_expr = bcgen_expression(vm, {}, statement._condition, body_acc);
	body_acc = condition_expr._body;
	body_acc._instrs.push_back(bcgen_instruction_t(bc_opcode::k_branch_false_bool, condition_expr._out, make_imm_int(body_instr_count + 2), {}));
	body_acc = flatten_body(vm, body_acc, loop_body);
	const auto body_end_pc = static_cast<int>(body_acc._instrs.size());
	body_acc._instrs.push_back(bcgen_instruction_t(bc_opcode::k_branch_always, make_imm_int(condition_pc - body_end_pc), {}, {} ));
	return body_acc;
}

bcgen_body_t bcgen_expression_statement(bcgenerator_t& vm, const statement_t::expression_statement_t& statement, const bcgen_body_t& body){
	QUARK_ASSERT(vm.check_invariant());
	QUARK_ASSERT(body.check_invariant());

	auto body_acc = body;
	const auto& expr = bcgen_expression(vm, {}, statement._expression, body);
	body_acc = expr._body;
	return body_acc;
}

bcgen_body_t bcgen_body_block(bcgenerator_t& vm, const body_t& body){
	QUARK_ASSERT(vm.check_invariant());
	QUARK_ASSERT(body.check_invariant());

	auto body_acc = bcgen_body_t({}, body._symbols);
	if(body._statements.empty() == false){
		vm._call_stack.push_back(bcgen_environment_t{ &body_acc });

		for(const auto& statement: body._statements){
			QUARK_ASSERT(statement.check_invariant());

			struct visitor_t {
				bcgenerator_t& vm;
				const bcgen_body_t body_acc;

				bcgen_body_t operator()(const statement_t::return_statement_t& s) const{
					return bcgen_return_statement(vm, s, body_acc);
				}
				bcgen_body_t operator()(const statement_t::define_struct_statement_t& s) const{
					QUARK_ASSERT(false);
					quark::throw_exception();
				}
				bcgen_body_t operator()(const statement_t::define_protocol_statement_t& s) const{
					QUARK_ASSERT(false);
					quark::throw_exception();
				}
				bcgen_body_t operator()(const statement_t::define_function_statement_t& s) const{
					QUARK_ASSERT(false);
					quark::throw_exception();
				}

				bcgen_body_t operator()(const statement_t::bind_local_t& s) const{
					QUARK_ASSERT(false);
					quark::throw_exception();
				}
				bcgen_body_t operator()(const statement_t::store_t& s) const{
					QUARK_ASSERT(false);
					quark::throw_exception();
				}
				bcgen_body_t operator()(const statement_t::store2_t& s) const{
					return bcgen_store2_statement(vm, s, body_acc);
				}
				bcgen_body_t operator()(const statement_t::block_statement_t& s) const{
					return bcgen_block_statement(vm, s, body_acc);
				}

				bcgen_body_t operator()(const statement_t::ifelse_statement_t& s) const{
					return bcgen_ifelse_statement(vm, s, body_acc);
				}
				bcgen_body_t operator()(const statement_t::for_statement_t& s) const{
					return bcgen_for_statement(vm, s, body_acc);
				}
				bcgen_body_t operator()(const statement_t::while_statement_t& s) const{
					return bcgen_while_statement(vm, s, body_acc);
				}


				bcgen_body_t operator()(const statement_t::expression_statement_t& s) const{
					return bcgen_expression_statement(vm, s, body_acc);
				}
				bcgen_body_t operator()(const statement_t::software_system_statement_t& s) const{
					return body_acc;
				}
				bcgen_body_t operator()(const statement_t::container_def_statement_t& s) const{
					return body_acc;
				}
			};

			body_acc = std::visit(visitor_t{vm, body_acc}, statement._contents);
			QUARK_ASSERT(body_acc.check_invariant());
		}

		vm._call_stack.pop_back();
	}

	QUARK_ASSERT(body_acc.check_invariant());
	QUARK_ASSERT(vm.check_invariant());
	return body_acc;
}

bcgen_body_t bcgen_body_top(bcgenerator_t& vm, const body_t& body){
	QUARK_ASSERT(vm.check_invariant());
	QUARK_ASSERT(body.check_invariant());

	auto body_acc = bcgen_body_block(vm, body);

	//	Append a stop to make sure execution doesn't leave instruction vector.
	if(body_acc._instrs.empty() == true || body_acc._instrs.back()._opcode != bc_opcode::k_return){
		body_acc._instrs.push_back(bcgen_instruction_t(bc_opcode::k_stop, {}, {}, {}));
	}

	QUARK_ASSERT(body_acc.check_invariant());
	QUARK_ASSERT(vm.check_invariant());
	return body_acc;
}


//////////////////////////////////////		PROCESS EXPRESSIONS


expression_gen_t bcgen_resolve_member_expression(bcgenerator_t& vm, const variable_address_t& target_reg, const expression_t& e, const bcgen_body_t& body){
	QUARK_ASSERT(vm.check_invariant());
	QUARK_ASSERT(e._input_exprs[0].get_output_type().is_struct());
	QUARK_ASSERT(body.check_invariant());

	auto body_acc = body;

	const auto& parent_expr = bcgen_expression(vm, {}, e._input_exprs[0], body);
	body_acc = parent_expr._body;

	const auto& struct_def = e._input_exprs[0].get_output_type().get_struct();
	int index = find_struct_member_index(struct_def, e._variable_name);
	QUARK_ASSERT(index != -1);

	const auto target_reg2 = target_reg.is_empty() ? add_local_temp(body_acc, e.get_output_type(), "temp: resolve-member output") : target_reg;
	body_acc._instrs.push_back(bcgen_instruction_t(bc_opcode::k_get_struct_member,
		target_reg2,
		parent_expr._out,
		make_imm_int(index)
	));

	QUARK_ASSERT(body_acc.check_invariant());
	return { body_acc, target_reg2, intern_type(vm, *e._output_type) };
}

expression_gen_t bcgen_lookup_element_expression(bcgenerator_t& vm, const variable_address_t& target_reg, const expression_t& e, const bcgen_body_t& body){
	QUARK_ASSERT(vm.check_invariant());
	QUARK_ASSERT(e.check_invariant());
	QUARK_ASSERT(body.check_invariant());

	auto body_acc = body;

	const auto& parent_expr = bcgen_expression(vm, {}, e._input_exprs[0], body_acc);
	body_acc = parent_expr._body;

	const auto& key_expr = bcgen_expression(vm, {}, e._input_exprs[1], body_acc);
	body_acc = key_expr._body;

	const auto parent_type = vm._types[parent_expr._type];
	const auto opcode = [&parent_type]{
		if(parent_type.is_string()){
			return bc_opcode::k_lookup_element_string;
		}
		else if(parent_type.is_json_value()){
			return bc_opcode::k_lookup_element_json_value;
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
	return { body_acc, target_reg2, intern_type(vm, e.get_output_type()) };
}

//??? Value already sits in a register / global -- no need to generate code to copy it in most cases!
expression_gen_t bcgen_load2_expression(bcgenerator_t& vm, const variable_address_t& target_reg, const expression_t& e, const bcgen_body_t& body){
	QUARK_ASSERT(vm.check_invariant());
	QUARK_ASSERT(e.check_invariant());
	QUARK_ASSERT(body.check_invariant());

	auto body_acc = body;
	const auto result_type = e.get_output_type();

	//	Shortcut: If we're loading a local-variable and are free from putting it in target_reg -- just acces the register where it sits = no instruction!
	if(target_reg.is_empty() && e._address._parent_steps != -1){
		QUARK_ASSERT(body_acc.check_invariant());
		return { body_acc, e._address, intern_type(vm, result_type) };
	}
	else{
		const auto target_reg2 = target_reg.is_empty() ? add_local_temp(body_acc, e.get_output_type(), "temp: load2") : target_reg;
		body_acc = copy_value(vm, result_type, target_reg2, e._address, body_acc);

		QUARK_ASSERT(body_acc.check_invariant());
		return { body_acc, target_reg2, intern_type(vm, result_type) };
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

//	NOTICE: extbits are generated for every value on callstack, even for DYN-types.
call_setup_t gen_call_setup(bcgenerator_t& vm, const std::vector<typeid_t>& function_def_arg_type, const expression_t* args, int callee_arg_count, const bcgen_body_t& body){
	QUARK_ASSERT(vm.check_invariant());
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
		const auto& m2 = bcgen_expression(vm, {}, args[i], body_acc);
		body_acc = m2._body;
		argument_regs.push_back(std::pair<reg_t, bc_typeid_t>(m2._out, m2._type));
	}

	//	We have max 32 extbits when popping stack.
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
		if(func_arg_type.is_internal_dynamic()){
			const auto arg_type_reg = add_local_const(body_acc, value_t::make_int(callee_arg_type), "DYN type arg #" + std::to_string(i));
			body_acc._instrs.push_back(bcgen_instruction_t(bc_opcode::k_push_inplace_value, arg_type_reg, {}, {} ));

			//	Int don't need extbit.
			exts.push_back(false);
		}

		const auto arg_type_full = vm._types[callee_arg_type];
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

	QUARK_ASSERT(vm.check_invariant());
	QUARK_ASSERT(body_acc.check_invariant());
	return { body_acc, exts, stack_count };
}

int get_host_function_id(bcgenerator_t& vm, const expression_t& e){
	if(e._input_exprs[0]._operation == expression_type::k_load2 && e._input_exprs[0]._address._parent_steps == -1){
		const auto global_index = e._input_exprs[0]._address._index;
		const auto& global_symbol = vm._call_stack[0]._body_ptr->_symbols._symbols[global_index];
		if(global_symbol.second._const_value.is_function()){
			const auto function_id = global_symbol.second._const_value.get_function_value();
			const auto& function_def = vm._ast_imm->_checked_ast._function_defs[function_id];
			return function_def->_host_function_id;
		}
		else{
			return -1;
		}
	}
	else{
		return -1;
	}
}

//	a = size(b)
bc_opcode convert_call_to_size_opcode(const typeid_t& arg1_type){
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
	else if(arg1_type.is_json_value()){
		return bc_opcode::k_get_size_jsonvalue;
	}
	else{
		return bc_opcode::k_nop;
	}
}

bc_opcode convert_call_to_pushback_opcode(const typeid_t& arg1_type){
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

expression_gen_t bcgen_call_expression(bcgenerator_t& vm, const variable_address_t& target_reg, const expression_t& e, const bcgen_body_t& body){
	QUARK_ASSERT(vm.check_invariant());
	QUARK_ASSERT(e.check_invariant());
	QUARK_ASSERT(body.check_invariant());

	auto body_acc = body;

	//	_input_exprs[0] is callee, rest are arguments.
	const auto callee_arg_count = static_cast<int>(e._input_exprs.size()) - 1;

	const auto function_type = e._input_exprs[0].get_output_type();
	const auto function_def_arg_types = function_type.get_function_args();
	QUARK_ASSERT(callee_arg_count == function_def_arg_types.size());
	const auto return_type = e.get_output_type();

	QUARK_ASSERT(callee_arg_count == function_def_arg_types.size());
	const auto arg_count = callee_arg_count;

	int host_function_id = get_host_function_id(vm, e);

	//	a = size(b)
	if(host_function_id == 1007 && arg_count == 1){
		const auto arg1_type = e._input_exprs[1].get_output_type();

		bc_opcode opcode = convert_call_to_size_opcode(arg1_type);
		if(opcode != bc_opcode::k_nop){
			const auto& arg1_expr = bcgen_expression(vm, {}, e._input_exprs[1], body_acc);
			body_acc = arg1_expr._body;

			const auto target_reg2 = target_reg.is_empty() ? add_local_temp(body_acc, e.get_output_type(), "temp: result for k_get_size_vector_x") : target_reg;
			body_acc._instrs.push_back(bcgen_instruction_t(opcode, target_reg2, arg1_expr._out, make_imm_int(0)));
			QUARK_ASSERT(body_acc.check_invariant());
			return { body_acc, target_reg2, intern_type(vm, return_type) };
		}
		else{
		}
	}

	//	a = push_back(b, c)
	else if(host_function_id == 1011 && arg_count == 2){
		QUARK_ASSERT(e.get_output_type() == e._input_exprs[1].get_output_type());

		const auto arg1_type = e._input_exprs[1].get_output_type();
		bc_opcode opcode = convert_call_to_pushback_opcode(arg1_type);
		if(opcode != bc_opcode::k_nop){
			//	push_back() used DYN-arguments which are resolved at runtime. When we make opcodes we need to check at compile time = now.
			if(arg1_type.is_string() && e._input_exprs[2].get_output_type().is_int() == false){
				quark::throw_runtime_error("Bad element to push_back(). Require push_back(string, int)");
			}

			const auto& arg1_expr = bcgen_expression(vm, {}, e._input_exprs[1], body_acc);
			body_acc = arg1_expr._body;

			const auto& arg2_expr = bcgen_expression(vm, {}, e._input_exprs[2], body_acc);
			body_acc = arg2_expr._body;

			const auto target_reg2 = target_reg.is_empty() ? add_local_temp(body_acc, e.get_output_type(), "temp: result for k_pushback_x") : target_reg;

			body_acc._instrs.push_back(bcgen_instruction_t(opcode, target_reg2, arg1_expr._out, arg2_expr._out));
			QUARK_ASSERT(body_acc.check_invariant());
			return { body_acc, target_reg2, intern_type(vm, return_type) };
		}
		else{
		}
	}


	//	Normal function call.
	{
		body_acc._instrs.push_back(bcgen_instruction_t(bc_opcode::k_push_frame_ptr, {}, {}, {} ));

		const auto& callee_expr = bcgen_expression(vm, {}, e._input_exprs[0], body_acc);
		body_acc = callee_expr._body;

		const auto call_setup = gen_call_setup(vm, function_def_arg_types, &e._input_exprs[1], callee_arg_count, body_acc);
		body_acc = call_setup._body;

		const auto target_reg2 = target_reg.is_empty() ? add_local_temp(body_acc, e.get_output_type(), "temp: call return") : target_reg;

		//??? No need to allocate return register if functions returns void.
		QUARK_ASSERT(return_type.is_internal_dynamic() == false && return_type.is_undefined() == false)
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
		return { body_acc, target_reg2, intern_type(vm, return_type) };
	}
}

//??? Submit dest-register to all gen-functions = minimize temps.
//??? Wrap itype in struct to make it typesafe.

expression_gen_t bcgen_construct_value_expression(bcgenerator_t& vm, const variable_address_t& target_reg, const expression_t& e, const bcgen_body_t& body){
	QUARK_ASSERT(vm.check_invariant());
	QUARK_ASSERT(e.check_invariant());
	QUARK_ASSERT(body.check_invariant());

	auto body_acc = body;

	const auto target_type = e.get_output_type();
	const auto target_itype = intern_type(vm, target_type);

	const auto callee_arg_count = static_cast<int>(e._input_exprs.size());
	std::vector<typeid_t> arg_types;
	for(const auto& m: e._input_exprs){
		arg_types.push_back(m.get_output_type());
	}
	const auto arg_count = callee_arg_count;

	const auto call_setup = gen_call_setup(vm, arg_types, &e._input_exprs[0], arg_count, body_acc);
	body_acc = call_setup._body;

	const auto source_itype = arg_count == 0 ? -1 : intern_type(vm, e._input_exprs[0].get_output_type());

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

expression_gen_t bcgen_arithmetic_unary_minus_expression(bcgenerator_t& vm, const variable_address_t& target_reg, const expression_t& e, const bcgen_body_t& body){
	QUARK_ASSERT(vm.check_invariant());
	QUARK_ASSERT(e.check_invariant());
	QUARK_ASSERT(body.check_invariant());

	const auto type = e._input_exprs[0].get_output_type();

	if(type.is_int()){
		const auto e2 = expression_t::make_simple_expression__2(expression_type::k_arithmetic_subtract__2, expression_t::make_literal_int(0), e._input_exprs[0], e._output_type);
		return bcgen_expression(vm, target_reg, e2, body);
	}
	else if(type.is_double()){
		const auto e2 = expression_t::make_simple_expression__2(expression_type::k_arithmetic_subtract__2, expression_t::make_literal_double(0), e._input_exprs[0], e._output_type);
		return bcgen_expression(vm, target_reg, e2, body);
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
expression_gen_t bcgen_conditional_operator_expression(bcgenerator_t& vm, const variable_address_t& target_reg, const expression_t& e, const bcgen_body_t& body){
	QUARK_ASSERT(vm.check_invariant());
	QUARK_ASSERT(e.check_invariant());
	QUARK_ASSERT(body.check_invariant());

	auto body_acc = body;

	const auto& condition_expr = bcgen_expression(vm, {}, e._input_exprs[0], body_acc);
	body_acc = condition_expr._body;

	const auto result_type = e.get_output_type();
	const auto result_itype = intern_type(vm, result_type);

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

	const auto& a_expr = bcgen_expression(vm, target_reg2, e._input_exprs[1], body_acc);
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
	const auto& b_expr = bcgen_expression(vm, target_reg2, e._input_exprs[2], body_acc);
	body_acc = b_expr._body;

	int end_pc = static_cast<int>(body_acc._instrs.size());

	QUARK_ASSERT(body_acc._instrs[jump1_pc]._reg_b._index == 0xbeefdeeb);
	QUARK_ASSERT(body_acc._instrs[jump2_pc]._reg_a._index == 0xfea57be7);
	body_acc._instrs[jump1_pc]._reg_b._index = b_pc - jump1_pc;
	body_acc._instrs[jump2_pc]._reg_a._index = end_pc - jump2_pc;

	QUARK_ASSERT(body_acc.check_invariant());
	return { body_acc, target_reg2, result_itype };
}

expression_gen_t bcgen_comparison_expression(bcgenerator_t& vm, const variable_address_t& target_reg, expression_type op, const expression_t& e, const bcgen_body_t& body){
	QUARK_ASSERT(vm.check_invariant());
	QUARK_ASSERT(e.check_invariant());
	QUARK_ASSERT(body.check_invariant());

	auto body_acc = body;

	const auto& left_expr = bcgen_expression(vm, {}, e._input_exprs[0], body_acc);
	body_acc = left_expr._body;

//	const auto right_value_reg = add_local_temp(body_acc, e._input_exprs[1].get_output_type(), "temp: left value");
	const auto& right_expr = bcgen_expression(vm, {}, e._input_exprs[1], body_acc);
	body_acc = right_expr._body;

	//	Type is the data the opcode works on -- comparing two ints, comparing two strings etc.
	const auto type = e._input_exprs[0].get_output_type();

	//	Output reg always a bool.
	QUARK_ASSERT(e.get_output_type().is_bool());
	const auto target_reg2 = target_reg.is_empty() ? add_local_temp(body_acc, e.get_output_type(), "temp: comparison flag") : target_reg;

	if(type.is_int()){

		//	Bool tells if to flip left / right.
		static const std::map<expression_type, std::pair<bool, bc_opcode>> conv_opcode_int = {
			{ expression_type::k_comparison_smaller_or_equal__2,			{ false, bc_opcode::k_comparison_smaller_or_equal_int } },
			{ expression_type::k_comparison_smaller__2,						{ false, bc_opcode::k_comparison_smaller_int } },
			{ expression_type::k_comparison_larger_or_equal__2,				{ true, bc_opcode::k_comparison_smaller_or_equal_int } },
			{ expression_type::k_comparison_larger__2,						{ true, bc_opcode::k_comparison_smaller_int } },

			{ expression_type::k_logical_equal__2,							{ false, bc_opcode::k_logical_equal_int } },
			{ expression_type::k_logical_nonequal__2,						{ false, bc_opcode::k_logical_nonequal_int } }
		};

		const auto result = conv_opcode_int.at(e._operation);
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
			{ expression_type::k_comparison_smaller_or_equal__2,			{ false, bc_opcode::k_comparison_smaller_or_equal } },
			{ expression_type::k_comparison_smaller__2,						{ false, bc_opcode::k_comparison_smaller } },
			{ expression_type::k_comparison_larger_or_equal__2,				{ true, bc_opcode::k_comparison_smaller_or_equal } },
			{ expression_type::k_comparison_larger__2,						{ true, bc_opcode::k_comparison_smaller } },

			{ expression_type::k_logical_equal__2,							{ false, bc_opcode::k_logical_equal } },
			{ expression_type::k_logical_nonequal__2,						{ false, bc_opcode::k_logical_nonequal } }
		};

		const auto result = conv_opcode.at(e._operation);
		if(result.first == false){
			body_acc._instrs.push_back(bcgen_instruction_t(result.second, target_reg2, left_expr._out, right_expr._out));
		}
		else{
			body_acc._instrs.push_back(bcgen_instruction_t(result.second, target_reg2, right_expr._out, left_expr._out));
		}
	}

	QUARK_ASSERT(body_acc.check_invariant());
	return { body_acc, target_reg2, intern_type(vm, typeid_t::make_bool()) };
}

expression_gen_t bcgen_arithmetic_expression(bcgenerator_t& vm, const variable_address_t& target_reg, expression_type op, const expression_t& e, const bcgen_body_t& body){
	QUARK_ASSERT(vm.check_invariant());
	QUARK_ASSERT(e.check_invariant());
	QUARK_ASSERT(body.check_invariant());

	auto body_acc = body;

	const auto& left_expr = bcgen_expression(vm, {}, e._input_exprs[0], body_acc);
	body_acc = left_expr._body;

	const auto& right_expr = bcgen_expression(vm, {}, e._input_exprs[1], body_acc);
	body_acc = right_expr._body;

	const auto type = e._input_exprs[0].get_output_type();
	const auto itype = intern_type(vm, type);

	const auto target_reg2 = target_reg.is_empty() ? add_local_temp(body_acc, e.get_output_type(), "temp: arithmetic output") : target_reg;

	const auto opcode = [&type, &e]{
		if(type.is_bool()){
			static const std::map<expression_type, bc_opcode> conv_opcode = {
				{ expression_type::k_arithmetic_add__2, bc_opcode::k_add_bool },
				{ expression_type::k_arithmetic_subtract__2, bc_opcode::k_nop },
				{ expression_type::k_arithmetic_multiply__2, bc_opcode::k_nop },
				{ expression_type::k_arithmetic_divide__2, bc_opcode::k_nop },
				{ expression_type::k_arithmetic_remainder__2, bc_opcode::k_nop },

				{ expression_type::k_logical_and__2, bc_opcode::k_logical_and_bool },
				{ expression_type::k_logical_or__2, bc_opcode::k_logical_or_bool }
			};
			return conv_opcode.at(e._operation);
		}
		else if(type.is_int()){
			static const std::map<expression_type, bc_opcode> conv_opcode = {
				{ expression_type::k_arithmetic_add__2, bc_opcode::k_add_int },
				{ expression_type::k_arithmetic_subtract__2, bc_opcode::k_subtract_int },
				{ expression_type::k_arithmetic_multiply__2, bc_opcode::k_multiply_int },
				{ expression_type::k_arithmetic_divide__2, bc_opcode::k_divide_int },
				{ expression_type::k_arithmetic_remainder__2, bc_opcode::k_remainder_int },

				{ expression_type::k_logical_and__2, bc_opcode::k_logical_and_int },
				{ expression_type::k_logical_or__2, bc_opcode::k_logical_or_int }
			};
			return conv_opcode.at(e._operation);
		}
		else if(type.is_double()){
			static const std::map<expression_type, bc_opcode> conv_opcode = {
				{ expression_type::k_arithmetic_add__2, bc_opcode::k_add_double },
				{ expression_type::k_arithmetic_subtract__2, bc_opcode::k_subtract_double },
				{ expression_type::k_arithmetic_multiply__2, bc_opcode::k_multiply_double },
				{ expression_type::k_arithmetic_divide__2, bc_opcode::k_divide_double },
				{ expression_type::k_arithmetic_remainder__2, bc_opcode::k_nop },

				{ expression_type::k_logical_and__2, bc_opcode::k_logical_and_double },
				{ expression_type::k_logical_or__2, bc_opcode::k_logical_or_double }
			};
			return conv_opcode.at(e._operation);
		}
		else if(type.is_string()){
			static const std::map<expression_type, bc_opcode> conv_opcode = {
				{ expression_type::k_arithmetic_add__2, bc_opcode::k_concat_strings },
				{ expression_type::k_arithmetic_subtract__2, bc_opcode::k_nop },
				{ expression_type::k_arithmetic_multiply__2, bc_opcode::k_nop },
				{ expression_type::k_arithmetic_divide__2, bc_opcode::k_nop },
				{ expression_type::k_arithmetic_remainder__2, bc_opcode::k_nop },

				{ expression_type::k_logical_and__2, bc_opcode::k_nop },
				{ expression_type::k_logical_or__2, bc_opcode::k_nop }
			};
			return conv_opcode.at(e._operation);
		}
		else if(type.is_vector()){
			if(encode_as_vector_w_inplace_elements(type)){
				static const std::map<expression_type, bc_opcode> conv_opcode = {
					{ expression_type::k_arithmetic_add__2, bc_opcode::k_concat_vectors_w_inplace_elements },
					{ expression_type::k_arithmetic_subtract__2, bc_opcode::k_nop },
					{ expression_type::k_arithmetic_multiply__2, bc_opcode::k_nop },
					{ expression_type::k_arithmetic_divide__2, bc_opcode::k_nop },
					{ expression_type::k_arithmetic_remainder__2, bc_opcode::k_nop },

					{ expression_type::k_logical_and__2, bc_opcode::k_nop },
					{ expression_type::k_logical_or__2, bc_opcode::k_nop }
				};
				return conv_opcode.at(e._operation);
			}
			else{
				static const std::map<expression_type, bc_opcode> conv_opcode = {
					{ expression_type::k_arithmetic_add__2, bc_opcode::k_concat_vectors_w_external_elements },
					{ expression_type::k_arithmetic_subtract__2, bc_opcode::k_nop },
					{ expression_type::k_arithmetic_multiply__2, bc_opcode::k_nop },
					{ expression_type::k_arithmetic_divide__2, bc_opcode::k_nop },
					{ expression_type::k_arithmetic_remainder__2, bc_opcode::k_nop },

					{ expression_type::k_logical_and__2, bc_opcode::k_nop },
					{ expression_type::k_logical_or__2, bc_opcode::k_nop }
				};
				return conv_opcode.at(e._operation);
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

expression_gen_t bcgen_literal_expression(bcgenerator_t& vm, const variable_address_t& target_reg, const expression_t& e, const bcgen_body_t& body){
	QUARK_ASSERT(vm.check_invariant());
	QUARK_ASSERT(body.check_invariant());

	auto body_acc = body;

	const auto const_temp = add_local_const(body_acc, e.get_literal(), "literal constant");
	if(target_reg.is_empty()){
		QUARK_ASSERT(body_acc.check_invariant());
		return { body_acc, const_temp, intern_type(vm, *e._output_type) };
	}

	//	We need to copy the value to the target reg...
	else{
		const auto result_type = e.get_output_type();
		body_acc = copy_value(vm, result_type, target_reg, const_temp, body_acc);

		QUARK_ASSERT(body_acc.check_invariant());
		return { body_acc, target_reg, intern_type(vm, result_type) };
	}
}

expression_gen_t bcgen_expression(bcgenerator_t& vm, const variable_address_t& target_reg, const expression_t& e, const bcgen_body_t& body){
	QUARK_ASSERT(vm.check_invariant());
	QUARK_ASSERT(target_reg.check_invariant());
	QUARK_ASSERT(e.check_invariant());
	QUARK_ASSERT(body.check_invariant());

	const auto op = e.get_operation();
	if(op == expression_type::k_literal){
		return bcgen_literal_expression(vm, target_reg, e, body);
	}
	else if(op == expression_type::k_resolve_member){
		return bcgen_resolve_member_expression(vm, target_reg, e, body);
	}
	else if(op == expression_type::k_lookup_element){
		return bcgen_lookup_element_expression(vm, target_reg, e, body);
	}
	else if(op == expression_type::k_load2){
		return bcgen_load2_expression(vm, target_reg, e, body);
	}
	else if(op == expression_type::k_call){
		return bcgen_call_expression(vm, target_reg, e, body);
	}
	else if(op == expression_type::k_value_constructor){
		return bcgen_construct_value_expression(vm, target_reg, e, body);
	}
	else if(op == expression_type::k_arithmetic_unary_minus__1){
		return bcgen_arithmetic_unary_minus_expression(vm, target_reg, e, body);
	}
	else if(op == expression_type::k_conditional_operator3){
		return bcgen_conditional_operator_expression(vm, target_reg, e, body);
	}
	else if (is_arithmetic_expression(op)){
		return bcgen_arithmetic_expression(vm, target_reg, op, e, body);
	}
	else if (is_comparison_expression(op)){
		return bcgen_comparison_expression(vm, target_reg, op, e, body);
	}
	else{
		QUARK_ASSERT(false);
	}
	quark::throw_exception();
}


//////////////////////////////////////		bcgenerator_t


bcgenerator_t::bcgenerator_t(const semantic_ast_t& ast){
	QUARK_ASSERT(ast.check_invariant());

	_ast_imm = std::make_shared<semantic_ast_t>(ast);
	QUARK_ASSERT(check_invariant());
}

bcgenerator_t::bcgenerator_t(const bcgenerator_t& other) :
	_ast_imm(other._ast_imm),
	_call_stack(other._call_stack),
	_types(other._types)
{
	QUARK_ASSERT(other.check_invariant());
	QUARK_ASSERT(check_invariant());
}

void bcgenerator_t::swap(bcgenerator_t& other) throw(){
	other._ast_imm.swap(this->_ast_imm);
	_call_stack.swap(this->_call_stack);
	_types.swap(this->_types);
}

const bcgenerator_t& bcgenerator_t::operator=(const bcgenerator_t& other){
	auto temp = other;
	temp.swap(*this);
	return *this;
}

#if DEBUG
bool bcgenerator_t::check_invariant() const {
	QUARK_ASSERT(_ast_imm->check_invariant());
	return true;
}
#endif


//////////////////////////////////////		FREE


bc_instruction_t squeeze_instruction(const bcgen_instruction_t& instruction){
	QUARK_ASSERT(instruction._reg_a._parent_steps == 0 || instruction._reg_a._parent_steps == -1 || instruction._reg_a._parent_steps == 666);
	QUARK_ASSERT(instruction._reg_b._parent_steps == 0 || instruction._reg_b._parent_steps == -1 || instruction._reg_b._parent_steps == 666);
	QUARK_ASSERT(instruction._reg_c._parent_steps == 0 || instruction._reg_c._parent_steps == -1 || instruction._reg_c._parent_steps == 666);
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
	for(const auto& e: body._symbols._symbols){
		const auto e2 = std::pair<std::string, bc_symbol_t>{
			e.first,
			bc_symbol_t{
				e.second._symbol_type == symbol_t::immutable_local ? bc_symbol_t::immutable_local : bc_symbol_t::mutable_local,
				e.second._value_type,
				value_to_bc(e.second._const_value)
			}
		};
		symbols2.push_back(e2);
	}

	return bc_static_frame_t(instrs2, symbols2, args);
}

bc_program_t generate_bytecode(const semantic_ast_t& ast){
	QUARK_ASSERT(ast.check_invariant());

//	QUARK_SCOPED_TRACE("generate_bytecode");
//	QUARK_TRACE_SS("INPUT:  " << json_to_pretty_string(ast_to_json(ast._checked_ast)._value));

	bcgenerator_t a(ast._checked_ast);

	const auto global_body = bcgen_body_top(a, a._ast_imm->_checked_ast._globals);
	const auto globals2 = make_frame(global_body, {});
	a._call_stack.push_back(bcgen_environment_t{ &global_body });

	std::vector<bc_function_definition_t> function_defs2;
	for(int function_id = 0 ; function_id < ast._checked_ast._function_defs.size() ; function_id++){
		const auto& function_def = *ast._checked_ast._function_defs[function_id];

		if(function_def._host_function_id != k_no_host_function_id){
			const auto function_def2 = bc_function_definition_t{
				function_def._function_type,
				function_def._args,
				nullptr,
				function_def._host_function_id
			};
			function_defs2.push_back(function_def2);
		}
		else{
			const auto body2 = function_def._body ? bcgen_body_top(a, *function_def._body) : bcgen_body_t({});
			const auto frame = make_frame(body2, function_def._function_type.get_function_args());
			const auto function_def2 = bc_function_definition_t{
				function_def._function_type,
				function_def._args,
				std::make_shared<bc_static_frame_t>(frame),
				function_def._host_function_id
			};
			function_defs2.push_back(function_def2);
		}
	}

	const auto result = bc_program_t{ globals2, function_defs2, a._types, ast._checked_ast._software_system, ast._checked_ast._container_def };

//	QUARK_TRACE_SS("OUTPUT: " << json_to_pretty_string(bcprogram_to_json(result)));

	return result;
}


}	//	floyd
