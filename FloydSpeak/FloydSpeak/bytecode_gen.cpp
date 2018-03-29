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
	variable_address_t _output_register;

	//	Output type.
	bc_typeid_t _type;
};

expr_info_t bcgen_expression(bgenerator_t& vm, const expression_t& e, const bc_body_t& body);
bc_body_t bcgen_body(bgenerator_t& vm, const body_t& body);

variable_address_t add_temp(bc_body_t& body_acc, const typeid_t& type, const std::string& name){
	int id = add_temp(body_acc._symbols, name, type);
	return variable_address_t::make_variable_address(0, id);
}

variable_address_t add_const(bc_body_t& body_acc, const value_t& value, const std::string& name){
	int id = add_constant_literal(body_acc._symbols, name, value);
	return variable_address_t::make_variable_address(0, id);
}

const std::vector<std::shared_ptr<statement_t>> make_statements(const std::vector<statement_t>& statements){
	std::vector<std::shared_ptr<statement_t>> result;
	for(const auto& e: statements){
		result.push_back(std::make_shared<statement_t>(e));
	}
	return result;
}

//	Use to stuff an integer into an instruction, in one of the register slots.
variable_address_t make_immediate_int(int value){
	return variable_address_t::make_variable_address(666, value);
}





	std::vector<bc_value_t> values_to_bcs(const std::vector<value_t>& values){
		std::vector<bc_value_t> result;
		for(const auto e: values){
			result.push_back(value_to_bc(e));
		}
		return result;
	}

	std::vector<value_t> bcs_to_values__same_types(const std::vector<bc_value_t>& values, const typeid_t& shared_type){
		std::vector<value_t> result;
		for(const auto e: values){
			result.push_back(bc_to_value(e, shared_type));
		}
		return result;
	}

	value_t bc_to_value(const bc_value_t& value, const typeid_t& type){
		QUARK_ASSERT(value.check_invariant());

		const auto basetype = type.get_base_type();

		if(basetype == base_type::k_internal_undefined){
			return value_t::make_undefined();
		}
		else if(basetype == base_type::k_internal_dynamic){
			return value_t::make_internal_dynamic();
		}
		else if(basetype == base_type::k_void){
			return value_t::make_void();
		}
		else if(basetype == base_type::k_bool){
			return value_t::make_bool(value.get_bool_value());
		}
		else if(basetype == base_type::k_int){
			return value_t::make_int(value.get_int_value());
		}
		else if(basetype == base_type::k_float){
			return value_t::make_float(value.get_float_value());
		}
		else if(basetype == base_type::k_string){
			return value_t::make_string(value.get_string_value());
		}
		else if(basetype == base_type::k_json_value){
			return value_t::make_json_value(value.get_json_value());
		}
		else if(basetype == base_type::k_typeid){
			return value_t::make_typeid_value(value.get_typeid_value());
		}
		else if(basetype == base_type::k_struct){
			const auto& struct_def = type.get_struct();
			const auto& members = value.get_struct_value();
			std::vector<value_t> members2;
			for(int i = 0 ; i < members.size() ; i++){
				const auto& member_type = struct_def._members[i]._type;
				const auto& member_value = members[i];
				const auto& member_value2 = bc_to_value(member_value, member_type);
				members2.push_back(member_value2);
			}
			return value_t::make_struct_value(type, members2);
		}
		else if(basetype == base_type::k_vector){
			const auto& element_type  = type.get_vector_element_type();
			return value_t::make_vector_value(element_type, bcs_to_values__same_types(value.get_vector_value(), element_type));
		}
		else if(basetype == base_type::k_dict){
			const auto value_type = type.get_dict_value_type();
			const auto entries = value.get_dict_value();
			std::map<std::string, value_t> entries2;
			for(const auto& e: entries){
				entries2.insert({e.first, bc_to_value(e.second, value_type)});
			}
			return value_t::make_dict_value(value_type, entries2);
		}
		else if(basetype == base_type::k_function){
			return value_t::make_function_value(type, value.get_function_value());
		}
		else{
			QUARK_ASSERT(false);
			throw std::exception();
		}
	}

	bc_value_t value_to_bc(const value_t& value){
		return bc_value_t::from_value(value);
	}

	std::string to_compact_string2(const bc_value_t& value) {
		return "xxyyzz";
//		return to_compact_string2(value._backstore);
	}







bc_typeid_t intern_type(bgenerator_t& vm, const typeid_t& type){
    const auto it = std::find_if(vm._types.begin(), vm._types.end(), [&type](const typeid_t& e) { return e == type; });
	if(it != vm._types.end()){
		const auto pos = static_cast<bc_typeid_t>(it - vm._types.begin());
		return pos;
	}
	else{
		vm._types.push_back(type);
		return static_cast<bc_typeid_t>(vm._types.size() - 1);
	}
}

/*
bc_typeid_t intern_type(bgenerator_t& vm, const typeid_t& type){
	return intern_type(vm, typeid_t(type));
}
*/



int bc_compare_string(const std::string& left, const std::string& right){
	// ### Better if it doesn't use c_ptr since that is non-pure string handling.
	return bc_limit(std::strcmp(left.c_str(), right.c_str()), -1, 1);
}

QUARK_UNIT_TESTQ("bc_compare_string()", ""){
	QUARK_TEST_VERIFY(bc_compare_string("", "") == 0);
}
QUARK_UNIT_TESTQ("bc_compare_string()", ""){
	QUARK_TEST_VERIFY(bc_compare_string("aaa", "aaa") == 0);
}
QUARK_UNIT_TESTQ("bc_compare_string()", ""){
	QUARK_TEST_VERIFY(bc_compare_string("b", "a") == 1);
}

//??? Slow!.
int bc_compare_struct_true_deep(const std::vector<bc_value_t>& left, const std::vector<bc_value_t>& right, const typeid_t& type){
	const auto& struct_def = type.get_struct();

	for(int i = 0 ; i < struct_def._members.size() ; i++){
		const auto& member_type = struct_def._members[i]._type;

		int diff = bc_value_t::compare_value_true_deep(left[i], right[i], member_type);
		if(diff != 0){
			return diff;
		}
	}
	return 0;
}

//	Compare vector element by element.
//	### Think more of equality when vectors have different size and shared elements are equal.
int bc_compare_vector_true_deep(const std::vector<bc_value_t>& left, const std::vector<bc_value_t>& right, const typeid_t& type){
//	QUARK_ASSERT(left.check_invariant());
//	QUARK_ASSERT(right.check_invariant());
//	QUARK_ASSERT(left._element_type == right._element_type);

	const auto& shared_count = std::min(left.size(), right.size());
	const auto& element_type = typeid_t(type.get_vector_element_type());
	for(int i = 0 ; i < shared_count ; i++){
		const auto element_result = bc_value_t::compare_value_true_deep(left[i], right[i], element_type);
		if(element_result != 0){
			return element_result;
		}
	}
	if(left.size() == right.size()){
		return 0;
	}
	else if(left.size() > right.size()){
		return -1;
	}
	else{
		return +1;
	}
}

template <typename Map>
bool bc_map_compare (Map const &lhs, Map const &rhs) {
    // No predicate needed because there is operator== for pairs already.
    return lhs.size() == rhs.size()
        && std::equal(lhs.begin(), lhs.end(),
                      rhs.begin());
}


int bc_compare_dict_true_deep(const std::map<std::string, bc_value_t>& left, const std::map<std::string, bc_value_t>& right, const typeid_t& type){
	const auto& element_type = typeid_t(type.get_dict_value_type());

	auto left_it = left.begin();
	auto left_end_it = left.end();

	auto right_it = right.begin();
	auto right_end_it = right.end();

	while(left_it != left_end_it && right_it != right_end_it){
		const auto key_result = bc_compare_string(left_it->first, right_it->first);
		if(key_result != 0){
			return key_result;
		}

		const auto element_result = bc_value_t::compare_value_true_deep(left_it->second, right_it->second, element_type);
		if(element_result != 0){
			return element_result;
		}

		left_it++;
		right_it++;
	}

	if(left_it == left_end_it && right_it == right_end_it){
		return 0;
	}
	else if(left_it == left_end_it && right_it != right_end_it){
		return 1;
	}
	else if(left_it != left_end_it && right_it == right_end_it){
		return -1;
	}
	QUARK_ASSERT(false)
	throw std::exception();
}

int bc_compare_json_values(const json_t& lhs, const json_t& rhs){
	if(lhs == rhs){
		return 0;
	}
	else{
		// ??? implement compare.
		assert(false);
	}
}

int bc_value_t::compare_value_true_deep(const bc_value_t& left, const bc_value_t& right, const typeid_t& type0){
	QUARK_ASSERT(left.check_invariant());
	QUARK_ASSERT(right.check_invariant());

	const auto type = type0;
	if(type.is_undefined()){
		return 0;
	}
	else if(type.is_bool()){
		return (left.get_bool_value() ? 1 : 0) - (right.get_bool_value() ? 1 : 0);
	}
	else if(type.is_int()){
		return bc_limit(left.get_int_value() - right.get_int_value(), -1, 1);
	}
	else if(type.is_float()){
		const auto a = left.get_float_value();
		const auto b = right.get_float_value();
		if(a > b){
			return 1;
		}
		else if(a < b){
			return -1;
		}
		else{
			return 0;
		}
	}
	else if(type.is_string()){
		return bc_compare_string(left.get_string_value(), right.get_string_value());
	}
	else if(type.is_json_value()){
		return bc_compare_json_values(left.get_json_value(), right.get_json_value());
	}
	else if(type.is_typeid()){
	//???
		if(left.get_typeid_value() == right.get_typeid_value()){
			return 0;
		}
		else{
			return -1;//??? Hack -- should return +1 depending on values.
		}
	}
	else if(type.is_struct()){
		//	Make sure the EXACT struct types are the same -- not only that they are both structs
//		if(left.get_type() != right.get_type()){
//			throw std::runtime_error("Cannot compare structs of different type.");
//		}

/*
		//	Shortcut: same obejct == we know values are same without having to check them.
		if(left.get_struct_value() == right.get_struct_value()){
			return 0;
		}
		else{
*/
			return bc_compare_struct_true_deep(left.get_struct_value(), right.get_struct_value(), type0);
//		}
	}
	else if(type.is_vector()){
		//	Make sure the EXACT types are the same -- not only that they are both vectors.
//		if(left.get_type() != right.get_type()){

		const auto& left_vec = left.get_vector_value();
		const auto& right_vec = right.get_vector_value();
		return bc_compare_vector_true_deep(left_vec, right_vec, type0);
	}
	else if(type.is_dict()){
		//	Make sure the EXACT types are the same -- not only that they are both dicts.
//		if(left.get_type() != right.get_type()){
		const auto& left2 = left.get_dict_value();
		const auto& right2 = right.get_dict_value();
		return bc_compare_dict_true_deep(left2, right2, type0);
	}
	else if(type.is_function()){
		QUARK_ASSERT(false);
		return 0;
	}
	else{
		QUARK_ASSERT(false);
		throw std::exception();
	}
}








bcgen_environment_t* find_env_from_address(bgenerator_t& vm, const variable_address_t& a){
	if(a._parent_steps == -1){
		return &vm._call_stack[0];
	}
	else{
		return &vm._call_stack[vm._call_stack.size() - 1 - a._parent_steps];
	}
}
const bcgen_environment_t* find_env_from_address(const bgenerator_t& vm, const variable_address_t& a){
	if(a._parent_steps == -1){
		return &vm._call_stack[0];
	}
	else{
		return &vm._call_stack[vm._call_stack.size() - 1 - a._parent_steps];
	}
}


bc_body_t bcgen_store2_statement(bgenerator_t& vm, const statement_t::store2_t& statement, const bc_body_t& body){
	QUARK_ASSERT(vm.check_invariant());

	auto body_acc = body;
	const auto expr = bcgen_expression(vm, statement._expression, body_acc);
	body_acc = expr._body;

	body_acc._statements.push_back(bc_instruction_t(bc_opcode::k_store_resolve, expr._type, statement._dest_variable, expr._output_register, {}));
	return body_acc;
}

bc_body_t bcgen_block_statement(bgenerator_t& vm, const statement_t::block_statement_t& statement, const bc_body_t& body){
	QUARK_ASSERT(vm.check_invariant());

//???			const auto& b = bcgen_body(vm, statement._block->_body);
//			body2._statements.push_back(bc_instruction_t{ bc_opcode::k_statement_block, intern_type(vm, typeid_t::make_undefined()), 0, {}, {}, { b} });
	return body;
}

bc_body_t bcgen_return_statement(bgenerator_t& vm, const statement_t::return_statement_t& statement, const bc_body_t& body){
	QUARK_ASSERT(vm.check_invariant());

	auto body_acc = body;
	const auto expr = bcgen_expression(vm, statement._expression, body);
	body_acc = expr._body;
	body_acc._statements.push_back(bc_instruction_t(bc_opcode::k_return, expr._type, expr._output_register,{}, {}));
	return body_acc;
}

//??? shortcut evaluation of conditions!


bc_body_t bcgen_ifelse_statement(bgenerator_t& vm, const statement_t::ifelse_statement_t& statement, const bc_body_t& body){
	QUARK_ASSERT(vm.check_invariant());

	auto body_acc = body;

	const auto condition_expr = bcgen_expression(vm, statement._condition, body_acc);
	body_acc = condition_expr._body;
	QUARK_ASSERT(statement._condition.get_output_type().is_bool());

	const auto& then_expr = bcgen_body(vm, statement._then_body);
	const auto& else_expr = bcgen_body(vm, statement._else_body);

	std::vector<bc_instruction_t> instructions;
	instructions.push_back(
		bc_instruction_t(
			bc_opcode::k_branch_zero,
			{},
			condition_expr._output_register,
			make_immediate_int(static_cast<int>(then_expr._statements.size()) + 1),
			{}
		)
	);
	instructions.insert(instructions.end(), then_expr._statements.begin(), then_expr._statements.end());
	instructions.push_back(
		bc_instruction_t(
			bc_opcode::k_jump,
			{},
			make_immediate_int(static_cast<int>(else_expr._statements.size()) + 0),
			{},
			{}
		)
	);
	instructions.insert(instructions.end(), else_expr._statements.begin(), else_expr._statements.end());

	body_acc._statements.insert(body_acc._statements.end(), instructions.begin(), instructions.end());
	return body_acc;
}

//??? check range-type too!
bc_body_t bcgen_for_statement(bgenerator_t& vm, const statement_t::for_statement_t& statement, const bc_body_t& body){
	QUARK_ASSERT(vm.check_invariant());

	auto body_acc = body;

	const auto start_expr = bcgen_expression(vm, statement._start_expression, body_acc);
	body_acc = start_expr._body;

	const auto end_expr = bcgen_expression(vm, statement._end_expression, body_acc);
	body_acc = end_expr._body;

	const auto& loop_body = bcgen_body(vm, statement._body);

	std::vector<bc_instruction_t> instructions;

	const auto counter_reg = add_temp(body_acc, typeid_t::make_int(), "for-loop-counter");
	const auto const1_reg = add_const(body_acc, value_t::make_int(1), "for-loop, decrement int 1");

	const auto itype_int = intern_type(vm, typeid_t::make_int());

	instructions.push_back(
		bc_instruction_t(
			bc_opcode::k_arithmetic_subtract,
			itype_int,
			counter_reg,
			const1_reg,
			{}
		)
	);
	instructions.push_back(
		bc_instruction_t(
			bc_opcode::k_branch_notzero,
			{},
			counter_reg,
			make_immediate_int(static_cast<int>(loop_body._statements.size()) + 2),
			{}
		)
	);
	instructions.insert(instructions.end(), loop_body._statements.begin(), loop_body._statements.end());

	instructions.push_back(
		bc_instruction_t(
			bc_opcode::k_arithmetic_subtract,
			itype_int,
			counter_reg,
			const1_reg,
			{}
		)
	);
	instructions.push_back(
		bc_instruction_t(
			bc_opcode::k_branch_notzero,
			{},
			counter_reg,
			make_immediate_int(0 - 1 - static_cast<int>(loop_body._statements.size())),
			{}
		)
	);

	body_acc._statements.insert(body_acc._statements.end(), instructions.begin(), instructions.end());
	return body_acc;
}

bc_body_t bcgen_while_statement(bgenerator_t& vm, const statement_t::while_statement_t& statement, const bc_body_t& body){
	QUARK_ASSERT(vm.check_invariant());

	auto body_acc = body;

	const auto condition_start = static_cast<int>(body._symbols.size());
	const auto condition_expr = bcgen_expression(vm, statement._condition, body_acc);
	body_acc = condition_expr._body;

	const auto& loop_body = bcgen_body(vm, statement._body);

	std::vector<bc_instruction_t> instructions;
	const auto flag_reg = add_temp(body_acc, typeid_t::make_bool(), "while-flag");

	instructions.push_back(
		bc_instruction_t(
			bc_opcode::k_branch_zero,
			{},
			flag_reg,
			make_immediate_int(static_cast<int>(loop_body._statements.size()) + 1),
			{}
		)
	);
	instructions.insert(instructions.end(), loop_body._statements.begin(), loop_body._statements.end());

	instructions.push_back(
		bc_instruction_t(
			bc_opcode::k_jump,
			{},
			make_immediate_int(condition_start -static_cast<int>(instructions.size())),
			{},
			{}
		)
	);

	body_acc._statements.insert(body_acc._statements.end(), instructions.begin(), instructions.end());
	return body_acc;
}

bc_body_t bcgen_expression_statement(bgenerator_t& vm, const statement_t::expression_statement_t& statement, const bc_body_t& body){
	QUARK_ASSERT(vm.check_invariant());

	auto body_acc = body;
	const auto& expr = bcgen_expression(vm, statement._expression, body);
	body_acc = expr._body;
	return body_acc;
}

bc_body_t bcgen_body(bgenerator_t& vm, const body_t& body){
	QUARK_ASSERT(vm.check_invariant());
	QUARK_ASSERT(body.check_invariant());

	vm._call_stack.push_back(bcgen_environment_t{ &body });

	auto body2 = bc_body_t({}, body._symbols);
	for(const auto& s: body._statements){
		const auto& statement = *s;
		QUARK_ASSERT(statement.check_invariant());

		if(statement._store2){
			body2 = bcgen_store2_statement(vm, *statement._store2, body2);
		}
		else if(statement._block){
			body2 = bcgen_block_statement(vm, *statement._block, body2);
		}
		else if(statement._return){
			body2 = bcgen_return_statement(vm, *statement._return, body2);
		}
		else if(statement._if){
			body2 = bcgen_ifelse_statement(vm, *statement._if, body2);
		}
		else if(statement._for){
			body2 = bcgen_for_statement(vm, *statement._for, body2);
		}
		else if(statement._while){
			body2 = bcgen_while_statement(vm, *statement._while, body2);
		}
		else if(statement._expression){
			body2 = bcgen_expression_statement(vm, *statement._expression, body2);
		}
		else{
			QUARK_ASSERT(false);
			throw std::exception();
		}
	}

	vm._call_stack.pop_back();

	return body2;
}



//////////////////////////////////////		EXPRESSIONS



expr_info_t bcgen_resolve_member_expression(bgenerator_t& vm, const expression_t& e, const bc_body_t& body){
	QUARK_ASSERT(vm.check_invariant());
	QUARK_ASSERT(e._input_exprs[0].get_output_type().is_struct());

	auto body_acc = body;

	const auto& parent_expr = bcgen_expression(vm, e._input_exprs[0], body);
	body_acc = parent_expr._body;

	const auto& struct_def = e._input_exprs[0].get_output_type().get_struct();
	int index = find_struct_member_index(struct_def, e._variable_name);
	QUARK_ASSERT(index != -1);

	const auto type = e.get_output_type();
	const auto itype = intern_type(vm, type);

	const auto temp = add_temp(body_acc, type, "resolve-member output register");
	body_acc._statements.push_back(bc_instruction_t(bc_opcode::k_resolve_member,
		itype,
		temp,
		parent_expr._output_register,
		make_immediate_int(index)
	));

	return { body_acc, temp, intern_type(vm, *e._output_type) };
}

expr_info_t bcgen_lookup_element_expression(bgenerator_t& vm, const expression_t& e, const bc_body_t& body){
	QUARK_ASSERT(vm.check_invariant());

	auto body_acc = body;

	const auto& parent_expr = bcgen_expression(vm, e._input_exprs[0], body);
	body_acc = parent_expr._body;

	const auto& key_expr = bcgen_expression(vm, e._input_exprs[1], body);
	body_acc = key_expr._body;

	const auto type = e.get_output_type();
	const auto itype = intern_type(vm, type);

	const auto temp = add_temp(body_acc, type, "lookup-element output register");
	body_acc._statements.push_back(bc_instruction_t(bc_opcode::k_lookup_element,
		itype,
		temp,
		parent_expr._output_register,
		key_expr._output_register
	));

	return { body_acc, temp, itype };
}


expr_info_t bcgen_load2_expression(bgenerator_t& vm, const expression_t& e, const bc_body_t& body){
	QUARK_ASSERT(vm.check_invariant());

	return { body, e._address, intern_type(vm, *e._output_type) };
}
//??? Do interning of add_const().
//??? make different types for register vs stack-pos.
expr_info_t bcgen_call_expression(bgenerator_t& vm, const expression_t& e, const bc_body_t& body){
	QUARK_ASSERT(vm.check_invariant());
	QUARK_ASSERT(e.check_invariant());

	auto body_acc = body;

	//	_input_exprs[0] is callee, rest are arguments.
	const auto& callee_expr = bcgen_expression(vm, e._input_exprs[0], body);
	body_acc = callee_expr._body;

	const auto callee_arg_count = static_cast<int>(e._input_exprs.size()) - 1;

	const auto function_type = e._input_exprs[0].get_output_type();
	const auto function_return_type = e.get_output_type();
	int dynamic_arg_count = count_function_dynamic_args(function_type);
	const auto function_def_arg_types = function_type.get_function_args();

	//	Generate code / symbols for all arguments to the function call. Record where each arg is kept.
	//	This might not create instructions or anything, if arguments are available as constants somewhere.
	vector<std::pair<reg_t, bc_typeid_t>> argument_regs;
	for(int i = 0 ; i < callee_arg_count ; i++){
		const auto& m2 = bcgen_expression(vm, e._input_exprs[i + 1], body_acc);
		body_acc = m2._body;
		argument_regs.push_back(std::pair<reg_t, bc_typeid_t>(m2._output_register, m2._type));
	}

	QUARK_ASSERT(callee_arg_count == function_def_arg_types.size());
	const auto arg_count = callee_arg_count;

	const auto itype_int = intern_type(vm, typeid_t::make_int());

	//	We have max 32 extbits when popping stack.
	if(arg_count > 16){
		throw std::runtime_error("Max 16 arguments to function.");
	}

	uint32_t extbits = 0x00000000;

	//	Make push-instructions that push args in correct order on callstack, before k_opcode_call is inserted.
	for(int i = 0 ; i < arg_count ; i++){
		const auto arg_reg = argument_regs[i].first;
		const auto callee_arg_type = argument_regs[i].second;
		const auto& func_arg_type = function_def_arg_types[i];

		//	Prepend internal-dynamic arguments with the itype of the actual callee-argument.
		if(func_arg_type.is_internal_dynamic()){
			const auto arg_type_reg = add_const(body_acc, value_t::make_int(callee_arg_type), "DYN type arg #" + std::to_string(i));
			body_acc._statements.push_back(bc_instruction_t(bc_opcode::k_push, itype_int, arg_type_reg, {}, {} ));
		}

		const auto arg_type_full = vm._types[callee_arg_type];
		bool ext = bc_value_t::is_bc_ext(arg_type_full.get_base_type());
		extbits = extbits << 1;
		if(ext){
			extbits = extbits | 1;
		}

		body_acc._statements.push_back(bc_instruction_t(bc_opcode::k_push, callee_arg_type, arg_reg, {}, {} ));
	}
	int stack_count = arg_count + dynamic_arg_count;

//	const auto itype = intern_type(vm, function_type);

	const auto function_result_reg = add_temp(body_acc, function_type.get_function_return(), "call result register");
	body_acc._statements.push_back(bc_instruction_t(bc_opcode::k_call,
		{},
		function_result_reg,
		callee_expr._output_register,
		make_immediate_int(arg_count)
	));
	body_acc._statements.push_back(bc_instruction_t(bc_opcode::k_popn, {}, make_immediate_int(stack_count), make_immediate_int(extbits), {} ));

	return { body_acc, function_result_reg, intern_type(vm, function_return_type) };
}

expr_info_t bcgen_construct_value_expression(bgenerator_t& vm, const expression_t& e, const bc_body_t& body){
	QUARK_ASSERT(vm.check_invariant());

	auto body_acc = body;

	vector<variable_address_t> arg_temps;
	for(int i = 0 ; i < e._input_exprs.size() ; i++){
		const auto& m2 = bcgen_expression(vm, e._input_exprs[i], body_acc);
		body_acc = m2._body;
		arg_temps.push_back(m2._output_register);
	}

	const auto type = e.get_output_type();
	const auto itype = intern_type(vm, type);

	//	All args follow first arg.
	const auto function_result_reg = add_temp(body_acc, type, "construct-value output register");
	body_acc._statements.push_back(bc_instruction_t(bc_opcode::k_call,
		itype,
		function_result_reg,
		arg_temps[0],
		make_immediate_int(static_cast<int>(arg_temps.size()))
	));

	return { body_acc, function_result_reg, itype };
}

expr_info_t bcgen_arithmetic_unary_minus_expression(bgenerator_t& vm, const expression_t& e, const bc_body_t& body){
	QUARK_ASSERT(vm.check_invariant());

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

	auto body_acc = body;

	const auto& condition_expr = bcgen_expression(vm, e._input_exprs[0], body_acc);
	body_acc = condition_expr._body;

	const auto type = e.get_output_type();
	const auto itype = intern_type(vm, type);

	const auto result_reg = add_temp(body_acc, type, "conditional_operator output register");

	const std::vector<std::shared_ptr<statement_t>> then_statements = {
		make_shared<statement_t>(statement_t::make__store2(result_reg, e._input_exprs[1]))
	};
	const std::vector<std::shared_ptr<statement_t>> else_statements = {
		make_shared<statement_t>(statement_t::make__store2(result_reg, e._input_exprs[2]))
	};
	const auto ifelse_statement = statement_t::make__ifelse_statement(e._input_exprs[0], body_t(then_statements), body_t(else_statements));
	body_acc = bcgen_ifelse_statement(vm, *ifelse_statement._if, body_acc);

	return { body_acc, result_reg, itype };
}

expr_info_t bcgen_comparison_expression(bgenerator_t& vm, expression_type op, const expression_t& e, const bc_body_t& body){
	QUARK_ASSERT(vm.check_invariant());

	static const std::map<expression_type, bc_opcode> conv_opcode = {
		{ expression_type::k_comparison_smaller_or_equal__2, bc_opcode::k_comparison_smaller_or_equal },
		{ expression_type::k_comparison_smaller__2, bc_opcode::k_comparison_smaller },
		{ expression_type::k_comparison_larger_or_equal__2, bc_opcode::k_comparison_larger_or_equal },
		{ expression_type::k_comparison_larger__2, bc_opcode::k_comparison_larger },

		{ expression_type::k_logical_equal__2, bc_opcode::k_logical_equal },
		{ expression_type::k_logical_nonequal__2, bc_opcode::k_logical_nonequal }
	};

	auto body_acc = body;
	const auto& left_expr = bcgen_expression(vm, e._input_exprs[0], body_acc);
	body_acc = left_expr._body;

	const auto& right_expr = bcgen_expression(vm, e._input_exprs[1], body_acc);
	body_acc = right_expr._body;

	const auto type = e._input_exprs[0].get_output_type();
	const auto itype = intern_type(vm, type);
	const auto temp = add_temp(body_acc, type, "comparison expression output register");

	body_acc._statements.push_back(bc_instruction_t(conv_opcode.at(e._operation),
		itype,
		temp,
		left_expr._output_register,
		right_expr._output_register
	));
	return { body_acc, temp, itype };
}


expr_info_t bcgen_arithmetic_expression(bgenerator_t& vm, expression_type op, const expression_t& e, const bc_body_t& body){
	QUARK_ASSERT(vm.check_invariant());

	static const std::map<expression_type, bc_opcode> conv_opcode = {
		{ expression_type::k_arithmetic_add__2, bc_opcode::k_arithmetic_add },
		{ expression_type::k_arithmetic_subtract__2, bc_opcode::k_arithmetic_subtract },
		{ expression_type::k_arithmetic_multiply__2, bc_opcode::k_arithmetic_multiply },
		{ expression_type::k_arithmetic_divide__2, bc_opcode::k_arithmetic_divide },
		{ expression_type::k_arithmetic_remainder__2, bc_opcode::k_arithmetic_remainder },

		{ expression_type::k_logical_and__2, bc_opcode::k_logical_and },
		{ expression_type::k_logical_or__2, bc_opcode::k_logical_or }
	};

	auto body_acc = body;
	const auto& left_expr = bcgen_expression(vm, e._input_exprs[0], body_acc);
	body_acc = left_expr._body;

	const auto& right_expr = bcgen_expression(vm, e._input_exprs[1], body_acc);
	body_acc = right_expr._body;

	const auto type = e._input_exprs[0].get_output_type();
	const auto itype = intern_type(vm, type);
	const auto temp = add_temp(body_acc, type, "arithmetic output register");

	body_acc._statements.push_back(bc_instruction_t(conv_opcode.at(e._operation),
		itype,
		temp,
		left_expr._output_register,
		right_expr._output_register
	));
	return { body_acc, temp, itype };
}


expr_info_t bcgen_expression(bgenerator_t& vm, const expression_t& e, const bc_body_t& body){
	QUARK_ASSERT(vm.check_invariant());
	QUARK_ASSERT(e.check_invariant());

	const auto op = e.get_operation();
	const auto output_type = e.get_output_type();
	if(op == expression_type::k_literal){
		auto body_acc = body;
		const auto const_temp = add_const(body_acc, e.get_literal(), "literal constant");
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





//////////////////////////////////////		globals



json_t bcprogram_to_json(const bc_program_t& program){
	vector<json_t> callstack;
/*
	for(int env_index = 0 ; env_index < vm._call_stack.size() ; env_index++){
		const auto e = &vm._call_stack[vm._call_stack.size() - 1 - env_index];

		const auto local_end = (env_index == (vm._call_stack.size() - 1)) ? vm._value_stack.size() : vm._call_stack[vm._call_stack.size() - 1 - env_index + 1]._values_offset;
		const auto local_count = local_end - e->_values_offset;
		std::vector<json_t> values;
		for(int local_index = 0 ; local_index < local_count ; local_index++){
			const auto& v = vm._value_stack[e->_values_offset + local_index];
			const auto& a = value_and_type_to_ast_json(v);
			values.push_back(a._value);
		}

		const auto& env = json_t::make_object({
			{ "values", values }
		});
		callstack.push_back(env);
	}
*/

	return json_t::make_object({
//		{ "ast", ast_to_json(vm._imm->_ast_pass3)._value },
//		{ "callstack", json_t::make_array(callstack) }
	});
}

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

	std::vector<const bc_function_definition_t> function_defs2;
	for(int function_id = 0 ; function_id < pass3._checked_ast._function_defs.size() ; function_id++){
		const auto& function_def = *pass3._checked_ast._function_defs[function_id];
		const auto body2 = function_def._body ? bcgen_body(a, *function_def._body) : bc_body_t({});
		const auto function_def2 = bc_function_definition_t{
			function_def._function_type,
			function_def._args,
			body2,
			function_def._host_function_id
		};
		function_defs2.push_back(function_def2);
	}

	const auto body2 = bcgen_body(a, a._imm->_ast_pass3._globals);
	const auto result = bc_program_t{ body2, function_defs2, a._types };


//	const auto result = bc_program_t{pass3._globals, pass3._function_defs};

	QUARK_CONTEXT_TRACE_SS(tracer, "OUTPUT: " << json_to_pretty_string(bcprogram_to_json(result)));

	return result;
}


}	//	floyd
