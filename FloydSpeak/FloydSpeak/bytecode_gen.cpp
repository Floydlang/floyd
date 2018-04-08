//
//  parser_evaluator.cpp
//  FloydSpeak
//
//  Created by Marcus Zetterquist on 26/07/16.
//  Copyright © 2016 Marcus Zetterquist. All rights reserved.
//

#include "bytecode_gen.h"

#include <cmath>
#include <algorithm>
#include <cstdint>

#include "pass3.h"

namespace floyd {

using std::vector;
using std::string;
using std::pair;
using std::shared_ptr;
using std::make_shared;

struct semantic_ast_t;




struct expr_info_t {
	bc_body_t _body;
	variable_address_t _output_reg;

	//	Output type.
	bc_typeid_t _type;
};

expr_info_t bcgen_expression(bgenerator_t& vm, const expression_t& e, const bc_body_t& body);
bc_body_t bcgen_body_top(bgenerator_t& vm, const body_t& body);
bc_body_t bcgen_body_block(bgenerator_t& vm, const body_t& body);

variable_address_t add_local_temp(bc_body_t& body_acc, const typeid_t& type, const std::string& name){
	int id = add_temp(body_acc._symbols, name, type);
	return variable_address_t::make_variable_address(0, id);
}

variable_address_t add_local_const(bc_body_t& body_acc, const value_t& value, const std::string& name){
	int id = add_constant_literal(body_acc._symbols, name, value);
	return variable_address_t::make_variable_address(0, id);
}

//	Use to stuff an integer into an instruction, in one of the register slots.
variable_address_t make_imm_int(int value){
	return variable_address_t::make_variable_address(666, value);
}








bc_typeid_t intern_type(bgenerator_t& vm, const typeid_t& type){
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

/*
bc_typeid_t intern_type(bgenerator_t& vm, const typeid_t& type){
	return intern_type(vm, typeid_t(type));
}
*/






//??? Shortcut evaluation of conditions!
//?? registers should ALWAYS bein current stack frame. What about upvalues?


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





 bool bc_body_t::check_invariant() const {
	for(int i = 0 ; i < _instrs.size() ; i++){
		const auto instruction = _instrs[i];
		QUARK_ASSERT(instruction.check_invariant());
	}
	for(const auto& e: _instrs){
		const auto encoding = k_opcode_info.at(e._opcode)._encoding;
		const auto reg_flags = encoding_to_reg_flags(encoding);

		QUARK_ASSERT((reg_flags._type == false && e._instr_type == k_no_bctypeid) || (reg_flags._type == true && e._instr_type >= 0));

		QUARK_ASSERT(check_register_nonlocal(e._reg_a, reg_flags._a));
		QUARK_ASSERT(check_register_nonlocal(e._reg_b, reg_flags._b));
		QUARK_ASSERT(check_register_nonlocal(e._reg_c, reg_flags._c));
	}
	for(const auto& e: _symbols){
		QUARK_ASSERT(e.first != "");
		QUARK_ASSERT(e.second.check_invariant());
	}
	return true;
}




#if DEBUG
bool bc_instruction_t::check_invariant() const {
	const auto encoding = k_opcode_info.at(_opcode)._encoding;
	const auto reg_flags = encoding_to_reg_flags(encoding);

	if(reg_flags._type){
	}
	else{
		QUARK_ASSERT(_instr_type == k_no_bctypeid);
	}

	QUARK_ASSERT(check_register(_reg_a, reg_flags._a));
	QUARK_ASSERT(check_register(_reg_b, reg_flags._b));
	QUARK_ASSERT(check_register(_reg_c, reg_flags._c));
	return true;
}
#endif





bc_body_t flatten_body(bgenerator_t& vm, const bc_body_t& dest, const bc_body_t& source){
	QUARK_ASSERT(vm.check_invariant());
	QUARK_ASSERT(dest.check_invariant());
	QUARK_ASSERT(source.check_invariant());

	std::vector<bc_instruction_t> instructions;
	int offset = static_cast<int>(dest._symbols.size());
	for(int i = 0 ; i < source._instrs.size() ; i++){
		//	Decrese parent-step for all local register accesses.
		auto s = source._instrs[i];

		const auto encoding = k_opcode_info.at(s._opcode)._encoding;
		const auto reg_flags = encoding_to_reg_flags(encoding);

		//	Make a new instructions with registered offsetted for flattening. Only offset registrers, not when used as immediates.
		const auto s2 = bc_instruction_t(
			s._opcode,
			s._instr_type,
			reg_flags._a ? flatten_reg(s._reg_a, offset) : s._reg_a,
			reg_flags._b ? flatten_reg(s._reg_b, offset) : s._reg_b,
			reg_flags._c ? flatten_reg(s._reg_c, offset) : s._reg_c
		);
		instructions.push_back(s2);
	}

	auto body_acc = dest;
	body_acc._instrs.insert(body_acc._instrs.end(), instructions.begin(), instructions.end());
	body_acc._symbols.insert(body_acc._symbols.end(), source._symbols.begin(), source._symbols.end());
	return body_acc;
}

bc_body_t bcgen_store2_statement(bgenerator_t& vm, const statement_t::store2_t& statement, const bc_body_t& body){
	QUARK_ASSERT(vm.check_invariant());
	QUARK_ASSERT(body.check_invariant());

	auto body_acc = body;
	const auto expr = bcgen_expression(vm, statement._expression, body_acc);
	body_acc = expr._body;

	const auto dest_variable = statement._dest_variable;
	const auto result_type = vm._types[expr._type];
	bool is_ext = bc_value_t::is_bc_ext(result_type.get_base_type());

	if(dest_variable._parent_steps == -1){
		if(is_ext){
			body_acc._instrs.push_back(bc_instruction_t(bc_opcode::k_store_global_obj, k_no_bctypeid, make_imm_int(dest_variable._index), expr._output_reg, {}));
		}
		else{
			body_acc._instrs.push_back(bc_instruction_t(bc_opcode::k_store_global_intern, k_no_bctypeid, make_imm_int(dest_variable._index), expr._output_reg, {}));
		}
	}
	else{
		if(is_ext){
			body_acc._instrs.push_back(bc_instruction_t(bc_opcode::k_store_local_obj, k_no_bctypeid, dest_variable, expr._output_reg, {}));
		}
		else{
			body_acc._instrs.push_back(bc_instruction_t(bc_opcode::k_store_local_intern, k_no_bctypeid, dest_variable, expr._output_reg, {}));
		}
	}

	return body_acc;
}

bc_body_t bcgen_block_statement(bgenerator_t& vm, const statement_t::block_statement_t& statement, const bc_body_t& body){
	QUARK_ASSERT(vm.check_invariant());
	QUARK_ASSERT(body.check_invariant());

	const auto body_acc = bcgen_body_block(vm, statement._body);
	return flatten_body(vm, body, body_acc);
}

bc_body_t bcgen_return_statement(bgenerator_t& vm, const statement_t::return_statement_t& statement, const bc_body_t& body){
	QUARK_ASSERT(vm.check_invariant());
	QUARK_ASSERT(body.check_invariant());

	auto body_acc = body;
	const auto expr = bcgen_expression(vm, statement._expression, body);
	body_acc = expr._body;
	body_acc._instrs.push_back(bc_instruction_t(bc_opcode::k_return, k_no_bctypeid, expr._output_reg, {}, {}));
	return body_acc;
}

bc_body_t bcgen_ifelse_statement(bgenerator_t& vm, const statement_t::ifelse_statement_t& statement, const bc_body_t& body){
	QUARK_ASSERT(vm.check_invariant());
	QUARK_ASSERT(body.check_invariant());

	auto body_acc = body;

	const auto condition_expr = bcgen_expression(vm, statement._condition, body_acc);
	body_acc = condition_expr._body;
	QUARK_ASSERT(statement._condition.get_output_type().is_bool());

	const auto& then_expr = bcgen_body_block(vm, statement._then_body);
	const auto& else_expr = bcgen_body_block(vm, statement._else_body);

	body_acc._instrs.push_back(
		bc_instruction_t(
			bc_opcode::k_branch_false_bool,
			k_no_bctypeid,
			condition_expr._output_reg,
			make_imm_int(static_cast<int>(then_expr._instrs.size()) + 2),
			{}
		)
	);
	body_acc = flatten_body(vm, body_acc, then_expr);
	body_acc._instrs.push_back(
		bc_instruction_t(
			bc_opcode::k_branch_always,
			k_no_bctypeid,
			make_imm_int(static_cast<int>(else_expr._instrs.size()) + 1),
			{},
			{}
		)
	);
	body_acc = flatten_body(vm, body_acc, else_expr);
	return body_acc;
}

/*
	start-expression
	end-expression
	k_store_local_intern, count_reg, start_expr_reg

	B:
	k_comparison_smaller_or_equal_int / k_comparison_smaller_int
	k_branch_false_bool: A

	BODY

	k_add_int
	k_branch_always B
	A:
*/
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

int get_count(const std::vector<bc_instruction_t>& instructions){
	return static_cast<int>(instructions.size());
}

bc_body_t bcgen_for_statement(bgenerator_t& vm, const statement_t::for_statement_t& statement, const bc_body_t& body){
	QUARK_ASSERT(vm.check_invariant());
	QUARK_ASSERT(body.check_invariant());

	auto body_acc = body;

	const auto start_expr = bcgen_expression(vm, statement._start_expression, body_acc);
	body_acc = start_expr._body;

	const auto end_expr = bcgen_expression(vm, statement._end_expression, body_acc);
	body_acc = end_expr._body;
	const auto end_count_reg = end_expr._output_reg;

	const auto const1_reg = add_local_const(body_acc, value_t::make_int(1), "integer 1, to decrement with");

	const auto& loop_body = bcgen_body_block(vm, statement._body);
	int body_instr_count = get_count(loop_body._instrs);

	QUARK_ASSERT(
		statement._range_type == statement_t::for_statement_t::k_closed_range
		|| statement._range_type ==statement_t::for_statement_t::k_open_range
	);
	const auto condition_opcode = statement._range_type == statement_t::for_statement_t::k_closed_range ? bc_opcode::k_branch_smaller_or_equal_int : bc_opcode::k_branch_smaller_int;


	//	IMPORTANT: Iterator register is the FIRST symbol of the loop body's symbol table.
	const auto counter_reg = variable_address_t::make_variable_address(0, static_cast<int>(body_acc._symbols.size()));
	body_acc._instrs.push_back(bc_instruction_t(bc_opcode::k_store_local_intern, k_no_bctypeid, counter_reg, start_expr._output_reg, {}));

	// Reuse start value as our counter.
	// Notice: we need to store iterator value in body's first register.
	int leave_pc = 1 + body_instr_count + 2;
	body_acc._instrs.push_back(bc_instruction_t(condition_opcode, k_no_bctypeid, end_count_reg, counter_reg, make_imm_int(leave_pc - get_count(body_acc._instrs))));

	int body_start_pc = get_count(body_acc._instrs);

	body_acc = flatten_body(vm, body_acc, loop_body);
	body_acc._instrs.push_back(bc_instruction_t(bc_opcode::k_add_int, k_no_bctypeid, counter_reg, counter_reg, const1_reg));
	body_acc._instrs.push_back(bc_instruction_t(condition_opcode, k_no_bctypeid, counter_reg, end_count_reg, make_imm_int(body_start_pc - get_count(body_acc._instrs))));

	QUARK_ASSERT(body_acc.check_invariant());
	return body_acc;
}

bc_body_t bcgen_while_statement(bgenerator_t& vm, const statement_t::while_statement_t& statement, const bc_body_t& body){
	QUARK_ASSERT(vm.check_invariant());
	QUARK_ASSERT(body.check_invariant());

	auto body_acc = body;

	const auto& loop_body = bcgen_body_block(vm, statement._body);

	int body_instr_count = static_cast<int>(loop_body._instrs.size());

	const auto condition_pc = static_cast<int>(body_acc._instrs.size());
	const auto condition_expr = bcgen_expression(vm, statement._condition, body_acc);
	body_acc = condition_expr._body;
	body_acc._instrs.push_back(bc_instruction_t(bc_opcode::k_branch_false_bool, k_no_bctypeid, condition_expr._output_reg, make_imm_int(body_instr_count + 2), {}));
	body_acc = flatten_body(vm, body_acc, loop_body);
	const auto body_end_pc = static_cast<int>(body_acc._instrs.size());
	body_acc._instrs.push_back(bc_instruction_t(bc_opcode::k_branch_always, k_no_bctypeid, make_imm_int(condition_pc - body_end_pc), {}, {} ));
	return body_acc;
}

bc_body_t bcgen_expression_statement(bgenerator_t& vm, const statement_t::expression_statement_t& statement, const bc_body_t& body){
	QUARK_ASSERT(vm.check_invariant());
	QUARK_ASSERT(body.check_invariant());

	auto body_acc = body;
	const auto& expr = bcgen_expression(vm, statement._expression, body);
	body_acc = expr._body;
	return body_acc;
}

bc_body_t bcgen_body_block(bgenerator_t& vm, const body_t& body){
	QUARK_ASSERT(vm.check_invariant());
	QUARK_ASSERT(body.check_invariant());

	auto body_acc = bc_body_t({}, body._symbols);
	if(body._statements.empty() == false){
		vm._call_stack.push_back(bcgen_environment_t{ &body_acc });

		for(const auto& s: body._statements){
			const auto& statement = *s;
			QUARK_ASSERT(statement.check_invariant());

			if(statement._store2){
				body_acc = bcgen_store2_statement(vm, *statement._store2, body_acc);
				QUARK_ASSERT(body_acc.check_invariant());
			}
			else if(statement._block){
				body_acc = bcgen_block_statement(vm, *statement._block, body_acc);
				QUARK_ASSERT(body_acc.check_invariant());
			}
			else if(statement._return){
				body_acc = bcgen_return_statement(vm, *statement._return, body_acc);
				QUARK_ASSERT(body_acc.check_invariant());
			}
			else if(statement._if){
				body_acc = bcgen_ifelse_statement(vm, *statement._if, body_acc);
				QUARK_ASSERT(body_acc.check_invariant());
			}
			else if(statement._for){
				body_acc = bcgen_for_statement(vm, *statement._for, body_acc);
				QUARK_ASSERT(body_acc.check_invariant());
			}
			else if(statement._while){
				body_acc = bcgen_while_statement(vm, *statement._while, body_acc);
				QUARK_ASSERT(body_acc.check_invariant());
			}
			else if(statement._expression){
				body_acc = bcgen_expression_statement(vm, *statement._expression, body_acc);
				QUARK_ASSERT(body_acc.check_invariant());
			}
			else{
				QUARK_ASSERT(false);
				throw std::exception();
			}
		}

		vm._call_stack.pop_back();
	}

	QUARK_ASSERT(body_acc.check_invariant());
	QUARK_ASSERT(vm.check_invariant());
	return body_acc;
}

bc_body_t bcgen_body_top(bgenerator_t& vm, const body_t& body){
	QUARK_ASSERT(vm.check_invariant());
	QUARK_ASSERT(body.check_invariant());

	auto body_acc = bcgen_body_block(vm, body);

	//	Append a stop to make sure execution doesn't leave instruction vector.
	if(body_acc._instrs.empty() == true || body_acc._instrs.back()._opcode != bc_opcode::k_return){
		body_acc._instrs.push_back(bc_instruction_t(bc_opcode::k_stop, k_no_bctypeid, {}, {}, {}));
	}

	QUARK_ASSERT(body_acc.check_invariant());
	QUARK_ASSERT(vm.check_invariant());
	return body_acc;
}



//////////////////////////////////////		EXPRESSIONS



expr_info_t bcgen_resolve_member_expression(bgenerator_t& vm, const expression_t& e, const bc_body_t& body){
	QUARK_ASSERT(vm.check_invariant());
	QUARK_ASSERT(e._input_exprs[0].get_output_type().is_struct());
	QUARK_ASSERT(body.check_invariant());

	auto body_acc = body;

	const auto& parent_expr = bcgen_expression(vm, e._input_exprs[0], body);
	body_acc = parent_expr._body;

	const auto& struct_def = e._input_exprs[0].get_output_type().get_struct();
	int index = find_struct_member_index(struct_def, e._variable_name);
	QUARK_ASSERT(index != -1);

	const auto type = e.get_output_type();
	const auto itype = intern_type(vm, type);

	const auto temp = add_local_temp(body_acc, type, "resolve-member output register");
	body_acc._instrs.push_back(bc_instruction_t(bc_opcode::k_get_struct_member,
		k_no_bctypeid,
		temp,
		parent_expr._output_reg,
		make_imm_int(index)
	));

	QUARK_ASSERT(body_acc.check_invariant());
	return { body_acc, temp, intern_type(vm, *e._output_type) };
}

expr_info_t bcgen_lookup_element_expression(bgenerator_t& vm, const expression_t& e, const bc_body_t& body){
	QUARK_ASSERT(vm.check_invariant());
	QUARK_ASSERT(e.check_invariant());
	QUARK_ASSERT(body.check_invariant());

	auto body_acc = body;

	const auto& parent_expr = bcgen_expression(vm, e._input_exprs[0], body_acc);
	body_acc = parent_expr._body;

	const auto& key_expr = bcgen_expression(vm, e._input_exprs[1], body_acc);
	body_acc = key_expr._body;

	const auto type = e.get_output_type();
	const auto itype = intern_type(vm, type);

	const auto parent_type = vm._types[parent_expr._type];

	const auto opcode = [&parent_type]{
		if(parent_type.is_string()){
			return bc_opcode::k_lookup_element_string;
		}
		else if(parent_type.is_json_value()){
			return bc_opcode::k_lookup_element_json_value;
		}
		else if(parent_type.is_vector()){
			return bc_opcode::k_lookup_element_vector;
		}
		else if(parent_type.is_dict()){
			return bc_opcode::k_lookup_element_dict;
		}
		else{
			QUARK_ASSERT(false);
			throw std::exception();
		}
	}();

	const auto temp_reg = add_local_temp(body_acc, type, "lookup-element output register");
	body_acc._instrs.push_back(bc_instruction_t(
		opcode,
		k_no_bctypeid,
		temp_reg,
		parent_expr._output_reg,
		key_expr._output_reg
	));
	QUARK_ASSERT(body_acc.check_invariant());
	return { body_acc, temp_reg, itype };
}

expr_info_t bcgen_load2_expression(bgenerator_t& vm, const expression_t& e, const bc_body_t& body){
	QUARK_ASSERT(vm.check_invariant());
	QUARK_ASSERT(e.check_invariant());
	QUARK_ASSERT(body.check_invariant());

	const auto result_type = *e._output_type;
	if(e._address._parent_steps == -1){
		auto body_acc = body;
		const auto temp_reg = add_local_temp(body_acc, result_type, "copied from global");
		bool ext = bc_value_t::is_bc_ext(result_type.get_base_type());
		if(ext){
			body_acc._instrs.push_back(bc_instruction_t(
				bc_opcode::k_load_global_obj,
				k_no_bctypeid,
				temp_reg,
				make_imm_int(e._address._index),
				{}
			));
		}
		else{
			body_acc._instrs.push_back(bc_instruction_t(
				bc_opcode::k_load_global_intern,
				k_no_bctypeid,
				temp_reg,
				make_imm_int(e._address._index),
				{}
			));
		}

		return { body_acc, temp_reg, intern_type(vm, result_type) };
	}
	else{
		return { body, e._address, intern_type(vm, result_type) };
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
	bc_body_t _body;
	vector<bool> _exts;
	int _stack_count;
};

//??? if zero or few arguments, use shortcuts. Stuff single arg into instruction reg_c etc.
//??? Do interning of add_local_const().
//??? make different types for register vs stack-pos.

//	NOTICE: extbits are generated for every value on callstack, even for DYN-types.
call_setup_t gen_call_setup(bgenerator_t& vm, const std::vector<typeid_t>& function_def_arg_type, const expression_t* args, int callee_arg_count, const bc_body_t& body){
	QUARK_ASSERT(vm.check_invariant());
	QUARK_ASSERT(args != nullptr || callee_arg_count == 0);
	QUARK_ASSERT(body.check_invariant());
	QUARK_ASSERT(callee_arg_count == function_def_arg_type.size());

	auto body_acc = body;

	int dynamic_arg_count = count_function_dynamic_args(function_def_arg_type);
	const auto arg_count = callee_arg_count;

	//	Generate code / symbols for all arguments to the function call. Record where each arg is kept.
	//	This might not create instructions or anything, if arguments are available as constants somewhere.
	vector<std::pair<reg_t, bc_typeid_t>> argument_regs;
	for(int i = 0 ; i < arg_count ; i++){
		const auto& m2 = bcgen_expression(vm, args[i], body_acc);
		body_acc = m2._body;
		argument_regs.push_back(std::pair<reg_t, bc_typeid_t>(m2._output_reg, m2._type));
	}

	//	We have max 32 extbits when popping stack.
	if(arg_count > 16){
		throw std::runtime_error("Max 16 arguments to function.");
	}

	//	Make push-instructions that push args in correct order on callstack, before k_opcode_call is inserted.
	vector<bool> exts;;
	for(int i = 0 ; i < arg_count ; i++){
		const auto arg_reg = argument_regs[i].first;
		const auto callee_arg_type = argument_regs[i].second;
		const auto& func_arg_type = function_def_arg_type[i];

		//	Prepend internal-dynamic arguments with the itype of the actual callee-argument.
		if(func_arg_type.is_internal_dynamic()){
			const auto arg_type_reg = add_local_const(body_acc, value_t::make_int(callee_arg_type), "DYN type arg #" + std::to_string(i));
			body_acc._instrs.push_back(bc_instruction_t(bc_opcode::k_push_intern, k_no_bctypeid, arg_type_reg, {}, {} ));

			//	Int don't need extbit.
			exts.push_back(false);
		}

		const auto arg_type_full = vm._types[callee_arg_type];
		bool ext = bc_value_t::is_bc_ext(arg_type_full.get_base_type());
		if(ext){
			body_acc._instrs.push_back(bc_instruction_t(bc_opcode::k_push_obj, k_no_bctypeid, arg_reg, {}, {} ));
			exts.push_back(true);
		}
		else{
			body_acc._instrs.push_back(bc_instruction_t(bc_opcode::k_push_intern, k_no_bctypeid, arg_reg, {}, {} ));
			exts.push_back(false);
		}
	}
	int stack_count = arg_count + dynamic_arg_count;

	QUARK_ASSERT(vm.check_invariant());
	QUARK_ASSERT(body_acc.check_invariant());
	return { body_acc, exts, stack_count };
}

expr_info_t bcgen_call_expression(bgenerator_t& vm, const expression_t& e, const bc_body_t& body){
	QUARK_ASSERT(vm.check_invariant());
	QUARK_ASSERT(e.check_invariant());
	QUARK_ASSERT(body.check_invariant());

	auto body_acc = body;

	body_acc._instrs.push_back(bc_instruction_t(bc_opcode::k_push_frame_ptr, k_no_bctypeid, {}, {}, {} ));

	//	_input_exprs[0] is callee, rest are arguments.
	const auto& callee_expr = bcgen_expression(vm, e._input_exprs[0], body_acc);
	body_acc = callee_expr._body;
	const auto function_reg = callee_expr._output_reg;
	const auto callee_arg_count = static_cast<int>(e._input_exprs.size()) - 1;

	const auto function_type = e._input_exprs[0].get_output_type();
	const auto function_def_arg_types = function_type.get_function_args();
	QUARK_ASSERT(callee_arg_count == function_def_arg_types.size());

	const auto return_type = e.get_output_type();

	QUARK_ASSERT(callee_arg_count == function_def_arg_types.size());
	const auto arg_count = callee_arg_count;

	const auto call_setup = gen_call_setup(vm, function_def_arg_types, &e._input_exprs[1], callee_arg_count, body_acc);
	body_acc = call_setup._body;

	//??? No need to allocate return register if functions returns void.
	QUARK_ASSERT(return_type.is_internal_dynamic() == false && return_type.is_undefined() == false)
	const auto function_result_reg = add_local_temp(body_acc, return_type, "call result register");
	body_acc._instrs.push_back(bc_instruction_t(
		bc_opcode::k_call,
		k_no_bctypeid,
		function_result_reg,
		function_reg,
		make_imm_int(arg_count)
	));

	const auto extbits = pack_bools(call_setup._exts);
	body_acc._instrs.push_back(bc_instruction_t(bc_opcode::k_popn, k_no_bctypeid, make_imm_int(call_setup._stack_count), make_imm_int(extbits), {} ));
	body_acc._instrs.push_back(bc_instruction_t(bc_opcode::k_pop_frame_ptr, k_no_bctypeid, {}, {}, {} ));
	return { body_acc, function_result_reg, intern_type(vm, return_type) };
}

//??? Submit dest-register to all gen-functions = minimize temps.
//??? make struct to make itype typesafe.


expr_info_t bcgen_construct_value_expression(bgenerator_t& vm, const expression_t& e, const bc_body_t& body){
	QUARK_ASSERT(vm.check_invariant());
	QUARK_ASSERT(e.check_invariant());
	QUARK_ASSERT(body.check_invariant());

	auto body_acc = body;

	const auto target_type = e.get_output_type();
	const auto target_itype = intern_type(vm, target_type);

	const auto callee_arg_count = static_cast<int>(e._input_exprs.size());
	vector<typeid_t> arg_types;
	for(const auto& m: e._input_exprs){
		arg_types.push_back(m.get_output_type());
	}
	const auto arg_count = callee_arg_count;

	const auto call_setup = gen_call_setup(vm, arg_types, &e._input_exprs[0], arg_count, body_acc);
	body_acc = call_setup._body;

	const auto source_itype = arg_count == 0 ? -1 : intern_type(vm, e._input_exprs[0].get_output_type());

	const auto function_result_reg = add_local_temp(body_acc, target_type, "Construct-value result register");

	if(target_type.is_vector()){
		body_acc._instrs.push_back(bc_instruction_t(
			bc_opcode::k_new_vector,
			k_no_bctypeid,
			function_result_reg,
			make_imm_int(target_itype),
			make_imm_int(arg_count)
		));
	}
	else if(target_type.is_dict()){
		body_acc._instrs.push_back(bc_instruction_t(
			bc_opcode::k_new_dict,
			k_no_bctypeid,
			function_result_reg,
			make_imm_int(target_itype),
			make_imm_int(arg_count)
		));
	}
	else if(target_type.is_struct()){
		body_acc._instrs.push_back(bc_instruction_t(
			bc_opcode::k_new_struct,
			k_no_bctypeid,
			function_result_reg,
			make_imm_int(target_itype),
			make_imm_int(arg_count)
		));
	}
	else{
		QUARK_ASSERT(arg_count == 1);

		body_acc._instrs.push_back(bc_instruction_t(
			bc_opcode::k_new_1,
			k_no_bctypeid,
			function_result_reg,
			make_imm_int(target_itype),
			make_imm_int(source_itype)
		));
	}

	const auto extbits = pack_bools(call_setup._exts);
	body_acc._instrs.push_back(bc_instruction_t(bc_opcode::k_popn, k_no_bctypeid, make_imm_int(call_setup._stack_count), make_imm_int(extbits), {} ));
	return { body_acc, function_result_reg, target_itype };
}

expr_info_t bcgen_arithmetic_unary_minus_expression(bgenerator_t& vm, const expression_t& e, const bc_body_t& body){
	QUARK_ASSERT(vm.check_invariant());
	QUARK_ASSERT(e.check_invariant());
	QUARK_ASSERT(body.check_invariant());

	const auto type = e._input_exprs[0].get_output_type();

	if(type.is_int()){
		const auto e2 = expression_t::make_simple_expression__2(expression_type::k_arithmetic_subtract__2, expression_t::make_literal_int(0), e._input_exprs[0], e._output_type);
		return bcgen_expression(vm,e2, body);
	}
	else if(type.is_float()){
		const auto e2 = expression_t::make_simple_expression__2(expression_type::k_arithmetic_subtract__2, expression_t::make_literal_float(0), e._input_exprs[0], e._output_type);
		return bcgen_expression(vm,e2, body);
	}
	else{
		QUARK_ASSERT(false);
		throw std::exception();
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

expr_info_t bcgen_conditional_operator_expression(bgenerator_t& vm, const expression_t& e, const bc_body_t& body){
	QUARK_ASSERT(vm.check_invariant());
	QUARK_ASSERT(e.check_invariant());
	QUARK_ASSERT(body.check_invariant());

	auto body_acc = body;

	const auto& condition_expr = bcgen_expression(vm, e._input_exprs[0], body_acc);
	body_acc = condition_expr._body;

	const auto result_type = e.get_output_type();
	const auto result_itype = intern_type(vm, result_type);

	const auto result_reg = add_local_temp(body_acc, result_type, "conditional_operator output register");

	int jump1_pc = static_cast<int>(body_acc._instrs.size());
	body_acc._instrs.push_back(
		bc_instruction_t(
			bc_opcode::k_branch_false_bool,
			k_no_bctypeid,
			condition_expr._output_reg,
			make_imm_int(0xbeefdeeb),
			{}
		)
	);

	////////	A expression

	const auto& a_expr = bcgen_expression(vm, e._input_exprs[1], body_acc);
	body_acc = a_expr._body;

	///??? real need a store-operation?
	//	Copy result to result_reg.
	if(bc_value_t::is_bc_ext(result_type.get_base_type())){
		body_acc._instrs.push_back(bc_instruction_t(bc_opcode::k_store_local_obj, k_no_bctypeid, result_reg, a_expr._output_reg, {} ));
	}
	else{
		body_acc._instrs.push_back(bc_instruction_t(bc_opcode::k_store_local_intern, k_no_bctypeid, result_reg, a_expr._output_reg, {} ));
	}

	int jump2_pc = static_cast<int>(body_acc._instrs.size());
	body_acc._instrs.push_back(
		bc_instruction_t(
			bc_opcode::k_branch_always,
			k_no_bctypeid,
			make_imm_int(0xfea57be7),
			{},
			{}
		)
	);

	////////	B expression

	int b_pc = static_cast<int>(body_acc._instrs.size());
	const auto& b_expr = bcgen_expression(vm, e._input_exprs[2], body_acc);
	body_acc = b_expr._body;

	if(bc_value_t::is_bc_ext(result_type.get_base_type())){
		body_acc._instrs.push_back(bc_instruction_t(bc_opcode::k_store_local_obj, k_no_bctypeid, result_reg, b_expr._output_reg, {} ));
	}
	else{
		body_acc._instrs.push_back(bc_instruction_t(bc_opcode::k_store_local_intern, k_no_bctypeid, result_reg, b_expr._output_reg, {} ));
	}

	int end_pc = static_cast<int>(body_acc._instrs.size());

	QUARK_ASSERT(body_acc._instrs[jump1_pc]._reg_b._index == 0xbeefdeeb);
	QUARK_ASSERT(body_acc._instrs[jump2_pc]._reg_a._index == 0xfea57be7);
	body_acc._instrs[jump1_pc]._reg_b._index = b_pc - jump1_pc;
	body_acc._instrs[jump2_pc]._reg_a._index = end_pc - jump2_pc;
	return { body_acc, result_reg, result_itype };
}

//??? shortcut evaluation!
expr_info_t bcgen_comparison_expression(bgenerator_t& vm, expression_type op, const expression_t& e, const bc_body_t& body){
	QUARK_ASSERT(vm.check_invariant());
	QUARK_ASSERT(e.check_invariant());
	QUARK_ASSERT(body.check_invariant());

	auto body_acc = body;
	const auto& left_expr = bcgen_expression(vm, e._input_exprs[0], body_acc);
	body_acc = left_expr._body;

	const auto& right_expr = bcgen_expression(vm, e._input_exprs[1], body_acc);
	body_acc = right_expr._body;

	//	Type is the data the opcode works on -- comparing two ints, comparing two strings etc.
	const auto type = e._input_exprs[0].get_output_type();
	const auto itype = intern_type(vm, type);

	//	Output reg always a bool.

	const auto result_reg = add_local_temp(body_acc, typeid_t::make_bool(), "comparison expression output register");

	if(type.is_int()){

		//	Bool tells if to flip left / right.
		static const std::map<expression_type, std::pair<bool, bc_opcode>> conv_opcode_int = {
			{ expression_type::k_comparison_smaller_or_equal__2,			{ false, bc_opcode::k_comparison_smaller_or_equal_int } },
			{ expression_type::k_comparison_smaller__2,						{ false, bc_opcode::k_comparison_smaller_int } },
			{ expression_type::k_comparison_larger_or_equal__2,				{ true, bc_opcode::k_comparison_smaller_int } },
			{ expression_type::k_comparison_larger__2,						{ true, bc_opcode::k_comparison_smaller_or_equal_int } },

			{ expression_type::k_logical_equal__2,							{ false, bc_opcode::k_logical_equal_int } },
			{ expression_type::k_logical_nonequal__2,						{ false, bc_opcode::k_logical_nonequal_int } }
		};

		const auto result = conv_opcode_int.at(e._operation);
		if(result.first == false){
			body_acc._instrs.push_back(bc_instruction_t(result.second, k_no_bctypeid, result_reg, left_expr._output_reg, right_expr._output_reg));
		}
		else{
			body_acc._instrs.push_back(bc_instruction_t(result.second, k_no_bctypeid, result_reg, right_expr._output_reg, left_expr._output_reg));
		}
	}
	else{

		//	Bool tells if to flip left / right.
		static const std::map<expression_type, std::pair<bool, bc_opcode>> conv_opcode = {
			{ expression_type::k_comparison_smaller_or_equal__2,			{ false, bc_opcode::k_comparison_smaller_or_equal } },
			{ expression_type::k_comparison_smaller__2,						{ false, bc_opcode::k_comparison_smaller } },
			{ expression_type::k_comparison_larger_or_equal__2,				{ true, bc_opcode::k_comparison_smaller } },
			{ expression_type::k_comparison_larger__2,						{ true, bc_opcode::k_comparison_smaller_or_equal } },

			{ expression_type::k_logical_equal__2,							{ false, bc_opcode::k_logical_equal } },
			{ expression_type::k_logical_nonequal__2,						{ false, bc_opcode::k_logical_nonequal } }
		};

		const auto result = conv_opcode.at(e._operation);
		if(result.first == false){
			body_acc._instrs.push_back(bc_instruction_t(result.second, k_no_bctypeid, result_reg, left_expr._output_reg, right_expr._output_reg));
		}
		else{
			body_acc._instrs.push_back(bc_instruction_t(result.second, k_no_bctypeid, result_reg, right_expr._output_reg, left_expr._output_reg));
		}
	}

	return { body_acc, result_reg, intern_type(vm, typeid_t::make_bool()) };
}


expr_info_t bcgen_arithmetic_expression(bgenerator_t& vm, expression_type op, const expression_t& e, const bc_body_t& body){
	QUARK_ASSERT(vm.check_invariant());
	QUARK_ASSERT(e.check_invariant());
	QUARK_ASSERT(body.check_invariant());

	auto body_acc = body;
	const auto& left_expr = bcgen_expression(vm, e._input_exprs[0], body_acc);
	body_acc = left_expr._body;

	const auto& right_expr = bcgen_expression(vm, e._input_exprs[1], body_acc);
	body_acc = right_expr._body;

	const auto type = e._input_exprs[0].get_output_type();
	const auto itype = intern_type(vm, type);
	const auto temp = add_local_temp(body_acc, type, "arithmetic output register");

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
		else if(type.is_float()){
			static const std::map<expression_type, bc_opcode> conv_opcode = {
				{ expression_type::k_arithmetic_add__2, bc_opcode::k_add_float },
				{ expression_type::k_arithmetic_subtract__2, bc_opcode::k_subtract_float },
				{ expression_type::k_arithmetic_multiply__2, bc_opcode::k_multiply_float },
				{ expression_type::k_arithmetic_divide__2, bc_opcode::k_divide_float },
				{ expression_type::k_arithmetic_remainder__2, bc_opcode::k_nop },

				{ expression_type::k_logical_and__2, bc_opcode::k_logical_and_float },
				{ expression_type::k_logical_or__2, bc_opcode::k_logical_or_float }
			};
			return conv_opcode.at(e._operation);
		}
		else if(type.is_string()){
			static const std::map<expression_type, bc_opcode> conv_opcode = {
				{ expression_type::k_arithmetic_add__2, bc_opcode::k_add_string },
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
			static const std::map<expression_type, bc_opcode> conv_opcode = {
				{ expression_type::k_arithmetic_add__2, bc_opcode::k_add_vector },
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
			QUARK_ASSERT(false);
			throw std::exception();
		}
	}();

	QUARK_ASSERT(opcode != bc_opcode::k_nop);
	body_acc._instrs.push_back(bc_instruction_t(opcode, k_no_bctypeid, temp, left_expr._output_reg, right_expr._output_reg));
	return { body_acc, temp, itype };
}

expr_info_t bcgen_expression(bgenerator_t& vm, const expression_t& e, const bc_body_t& body){
	QUARK_ASSERT(vm.check_invariant());
	QUARK_ASSERT(e.check_invariant());
	QUARK_ASSERT(body.check_invariant());

	const auto op = e.get_operation();
	const auto output_type = e.get_output_type();
	if(op == expression_type::k_literal){
		auto body_acc = body;
		const auto const_temp = add_local_const(body_acc, e.get_literal(), "literal constant");
		return { body_acc, const_temp, intern_type(vm, output_type) };
	}
	else if(op == expression_type::k_resolve_member){
		return bcgen_resolve_member_expression(vm, e, body);
	}
	else if(op == expression_type::k_lookup_element){
		return bcgen_lookup_element_expression(vm, e, body);
	}
	else if(op == expression_type::k_load2){
		return bcgen_load2_expression(vm, e, body);
	}

	else if(op == expression_type::k_call){
		return bcgen_call_expression(vm, e, body);
	}

	else if(op == expression_type::k_construct_value){
		return bcgen_construct_value_expression(vm, e, body);
	}

	else if(op == expression_type::k_arithmetic_unary_minus__1){
		return bcgen_arithmetic_unary_minus_expression(vm, e, body);
	}

	//	Special-case since it uses 3 expressions & uses shortcut evaluation.
	else if(op == expression_type::k_conditional_operator3){
		return bcgen_conditional_operator_expression(vm, e, body);
	}
	else if (is_arithmetic_expression(op)){
		return bcgen_arithmetic_expression(vm, op, e, body);
	}
	else if (is_comparison_expression(op)){
		return bcgen_comparison_expression(vm, op, e, body);
	}
	else{
		QUARK_ASSERT(false);
	}
	throw std::exception();
}



//////////////////////////////////////		bc_instruction2_t



#if DEBUG
bool bc_instruction2_t::check_invariant() const {
	const auto encoding = k_opcode_info.at(_opcode)._encoding;
	const auto reg_flags = encoding_to_reg_flags(encoding);

	if(reg_flags._type){
	}
	else{
//		QUARK_ASSERT(_instr_type == k_no_bctypeid);
	}

/*
	QUARK_ASSERT(check_register(_reg_a, reg_flags._a));
	QUARK_ASSERT(check_register(_reg_b, reg_flags._b));
	QUARK_ASSERT(check_register(_reg_c, reg_flags._c));
*/
	return true;
}
#endif



//////////////////////////////////////		bc_frame_t



bc_instruction2_t squeeze_instruction(const bc_instruction_t& instruction){
	QUARK_ASSERT(instruction._instr_type == k_no_bctypeid);
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


	const auto result = bc_instruction2_t(
		instruction._opcode,
		static_cast<int16_t>(instruction._reg_a._index),
		static_cast<int16_t>(instruction._reg_b._index),
		static_cast<int16_t>(instruction._reg_c._index)
	);
	return result;
}

bc_frame_t make_frame(const bc_body_t& body, const std::vector<typeid_t>& args){
	QUARK_ASSERT(body.check_invariant());
	std::vector<bc_instruction2_t> instrs2;

	for(const auto& e: body._instrs){
		instrs2.push_back(squeeze_instruction(e));
	}

	return bc_frame_t(instrs2, body._symbols, args);
}



//////////////////////////////////////		bgenerator_t


/*
void test__bcgen_expression(const expression_t& e, const expression_t& expected_value){
	const ast_t ast;
	bgenerator_t interpreter(ast);
	const auto& e3 = bcgen_expression(interpreter, e);

	ut_compare_jsons(expression_to_json(e3)._value, expression_to_json(expected_value)._value);
}
*/

bgenerator_t::bgenerator_t(const ast_t& pass3){
	QUARK_ASSERT(pass3.check_invariant());

	_imm = std::make_shared<bgenerator_imm_t>(bgenerator_imm_t{pass3});
	QUARK_ASSERT(check_invariant());
}

bgenerator_t::bgenerator_t(const bgenerator_t& other) :
	_imm(other._imm),
	_call_stack(other._call_stack),
	_types(other._types)
{
	QUARK_ASSERT(other.check_invariant());
	QUARK_ASSERT(check_invariant());
}

void bgenerator_t::swap(bgenerator_t& other) throw(){
	other._imm.swap(this->_imm);
	_call_stack.swap(this->_call_stack);
	_types.swap(this->_types);
}

const bgenerator_t& bgenerator_t::operator=(const bgenerator_t& other){
	auto temp = other;
	temp.swap(*this);
	return *this;
}

#if DEBUG
bool bgenerator_t::check_invariant() const {
	QUARK_ASSERT(_imm->_ast_pass3.check_invariant());
	return true;
}
#endif


bc_program_t run_bggen(const quark::trace_context_t& tracer, const semantic_ast_t& pass3){
	QUARK_ASSERT(pass3.check_invariant());

	QUARK_CONTEXT_SCOPED_TRACE(tracer, "run_bggen");

	QUARK_CONTEXT_TRACE_SS(tracer, "INPUT:  " << json_to_pretty_string(ast_to_json(pass3._checked_ast)._value));

	bgenerator_t a(pass3._checked_ast);

	const auto global_body = bcgen_body_top(a, a._imm->_ast_pass3._globals);
	const auto globals2 = make_frame(global_body, {});
	a._call_stack.push_back(bcgen_environment_t{ &global_body });

	std::vector<const bc_function_definition_t> function_defs2;
	for(int function_id = 0 ; function_id < pass3._checked_ast._function_defs.size() ; function_id++){
		const auto& function_def = *pass3._checked_ast._function_defs[function_id];

		if(function_def._host_function_id){
			const auto function_def2 = bc_function_definition_t{
				function_def._function_type,
				function_def._args,
				nullptr,
				function_def._host_function_id
			};
			function_defs2.push_back(function_def2);
		}
		else{
			const auto body2 = function_def._body ? bcgen_body_top(a, *function_def._body) : bc_body_t({});
			const auto frame = make_frame(body2, function_def._function_type.get_function_args());
			const auto function_def2 = bc_function_definition_t{
				function_def._function_type,
				function_def._args,
				make_shared<bc_frame_t>(frame),
				function_def._host_function_id
			};
			function_defs2.push_back(function_def2);
		}
	}

	const auto result = bc_program_t{ globals2, function_defs2, a._types };


//	const auto result = bc_program_t{pass3._globals, pass3._function_defs};

	QUARK_CONTEXT_TRACE_SS(tracer, "OUTPUT: " << json_to_pretty_string(bcprogram_to_json(result)));

	return result;
}


}	//	floyd
