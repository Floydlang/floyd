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

namespace floyd {

using std::vector;
using std::string;
using std::pair;
using std::shared_ptr;
using std::unique_ptr;
using std::make_shared;


bc_expression_t bcgen_call_expression(bgenerator_t& vm, const expression_t& e);
bc_expression_t bcgen_expression(bgenerator_t& vm, const expression_t& e);
std::vector<bc_instr_t> bcgen_statements(bgenerator_t& vm, const std::vector<std::shared_ptr<statement_t>>& statements);
bc_body_t bcgen_body(bgenerator_t& vm, const body_t& body);
bc_instr_t bcgen_statement(bgenerator_t& vm, const statement_t& statement);



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

std::vector<bc_instr_t> bcgen_statements(bgenerator_t& vm, const std::vector<std::shared_ptr<statement_t>>& statements){
	QUARK_ASSERT(vm.check_invariant());
	for(const auto i: statements){ QUARK_ASSERT(i->check_invariant()); };

	std::vector<bc_instr_t> result;
	int statement_index = 0;
	while(statement_index < statements.size()){
		const auto& statement = statements[statement_index];
		const auto& r = bcgen_statement(vm, *statement);
		if(r._opcode != bc_instr::k_nop){
			result.push_back(r);
		}
		statement_index++;
	}
	return result;
}

bc_body_t bcgen_body(bgenerator_t& vm, const body_t& body){
	QUARK_ASSERT(vm.check_invariant());

	vm._call_stack.push_back(bcgen_environment_t{ &body });
	const auto& r = bcgen_statements(vm, body._statements);
	vm._call_stack.pop_back();

	return bc_body_t(r, body._symbols);
}

bc_instr_t bcgen_store2_statement(bgenerator_t& vm, const statement_t::store2_t& statement){
	QUARK_ASSERT(vm.check_invariant());

	const auto& expr = bcgen_expression(vm, statement._expression);
	return bc_instr_t{ bc_instr::k_statement_store, {expr}, statement._dest_variable,  {}};
}

bc_instr_t bcgen_return_statement(bgenerator_t& vm, const statement_t::return_statement_t& statement){
	QUARK_ASSERT(vm.check_invariant());

	const auto& expr = bcgen_expression(vm, statement._expression);
	return bc_instr_t{ bc_instr::k_statement_return, {expr}, {}, {}};
}

bc_instr_t bcgen_ifelse_statement(bgenerator_t& vm, const statement_t::ifelse_statement_t& statement){
	QUARK_ASSERT(vm.check_invariant());

	const auto& condition_expr = bcgen_expression(vm, statement._condition);
	QUARK_ASSERT(statement._condition.get_annotated_type().is_bool());
	const auto& then_expr = bcgen_body(vm, statement._then_body);
	const auto& else_expr = bcgen_body(vm, statement._else_body);
	return bc_instr_t{ bc_instr::k_statement_if, {condition_expr}, {}, { then_expr, else_expr }};
}

bc_instr_t bcgen_for_statement(bgenerator_t& vm, const statement_t::for_statement_t& statement){
	QUARK_ASSERT(vm.check_invariant());

	const auto& start_expr = bcgen_expression(vm, statement._start_expression);
	const auto& end_expr = bcgen_expression(vm, statement._end_expression);
	const auto& body = bcgen_body(vm, statement._body);
	return bc_instr_t{ bc_instr::k_statement_for, {start_expr, end_expr}, {}, { body } };
}

bc_instr_t bcgen_while_statement(bgenerator_t& vm, const statement_t::while_statement_t& statement){
	QUARK_ASSERT(vm.check_invariant());

	const auto& condition_expr = bcgen_expression(vm, statement._condition);
	const auto& body = bcgen_body(vm, statement._body);
	return bc_instr_t{ bc_instr::k_statement_while, {condition_expr}, {}, {body} };
}

bc_instr_t bcgen_expression_statement(bgenerator_t& vm, const statement_t::expression_statement_t& statement){
	QUARK_ASSERT(vm.check_invariant());

	const auto& expr = bcgen_expression(vm, statement._expression);
	return bc_instr_t{ bc_instr::k_statement_expression, {expr}, {}, {} };
}

bc_instr_t bcgen_statement(bgenerator_t& vm, const statement_t& statement){
	QUARK_ASSERT(vm.check_invariant());
	QUARK_ASSERT(statement.check_invariant());

	if(statement._bind_local){
		//	Bind is converted to symbol table + store-local in pass3.
		QUARK_ASSERT(false);
	}
	else if(statement._store){
		QUARK_ASSERT(false);
	}
	else
	if(statement._store2){
		return bcgen_store2_statement(vm, *statement._store2);
	}
	else if(statement._block){
		const auto& body = bcgen_body(vm, statement._block->_body);
		return bc_instr_t{ bc_instr::k_statement_block, {}, {}, { body} };
	}
	else if(statement._return){
		return bcgen_return_statement(vm, *statement._return);
	}
	else if(statement._def_struct){
		QUARK_ASSERT(false);
	}
	else if(statement._if){
		return bcgen_ifelse_statement(vm, *statement._if);
	}
	else if(statement._for){
		return bcgen_for_statement(vm, *statement._for);
	}
	else if(statement._while){
		return bcgen_while_statement(vm, *statement._while);
	}
	else if(statement._expression){
		return bcgen_expression_statement(vm, *statement._expression);
	}
	else{
		QUARK_ASSERT(false);
	}
	throw std::exception();
}

bc_expression_t bcgen_resolve_member_expression(bgenerator_t& vm, const expression_t& e){
	QUARK_ASSERT(vm.check_invariant());

	const auto& expr = *e.get_resolve_member();
	QUARK_ASSERT(expr._parent_address->get_annotated_type().is_struct());

	const auto& parent_expr = bcgen_expression(vm, *expr._parent_address);
	//??? SHould use an address index, not a string!
	const auto member_name = expr._member_name;
	return bc_expression_t{ bc_expression_opcode::k_expression_resolve_member, e.get_annotated_type(), {parent_expr}, member_name, {}, {} };
}

bc_expression_t bcgen_lookup_element_expression(bgenerator_t& vm, const expression_t& e){
	QUARK_ASSERT(vm.check_invariant());

	const auto& expr = *e.get_lookup();
	const auto& parent_expr = bcgen_expression(vm, *expr._parent_address);
	const auto& key_expr = bcgen_expression(vm, *expr._lookup_key);
	return bc_expression_t{ bc_expression_opcode::k_expression_lookup_element, e.get_annotated_type(), {parent_expr, key_expr}, "", {}, {} };
}

bc_expression_t bcgen_load2_expression(bgenerator_t& vm, const expression_t& e){
	QUARK_ASSERT(vm.check_invariant());

	const auto& expr = *e.get_load2();
	const auto& address = expr._address;
	return bc_expression_t{ bc_expression_opcode::k_expression_load, e.get_annotated_type(), {}, "", address, {} };
}

bc_expression_t bcgen_construct_value_expression(bgenerator_t& vm, const expression_t& e){
	QUARK_ASSERT(vm.check_invariant());

	const auto& expr = *e.get_construct_value();
	const auto& value_type = expr._value_type2;

	std::vector<bc_expression_t> args2;
	for(const auto& m: expr._args){
		const auto& m2 = bcgen_expression(vm, m);
		args2.push_back(m2);
	}

	return bc_expression_t{ bc_expression_opcode::k_expression_construct_value, value_type, args2, "", {}, {} };
}

bc_expression_t bcgen_arithmetic_unary_minus_expression(bgenerator_t& vm, const expression_t& e){
	QUARK_ASSERT(vm.check_invariant());

	const auto& expr = *e.get_unary_minus();
	const auto& exprx = bcgen_expression(vm, *expr._expr);
	return bc_expression_t{ bc_expression_opcode::k_expression_arithmetic_unary_minus, e.get_annotated_type(), {exprx}, "", {}, {} };
}

bc_expression_t bcgen_conditional_operator_expression(bgenerator_t& vm, const expression_t& e){
	QUARK_ASSERT(vm.check_invariant());

	const auto& expr = *e.get_conditional();
	const auto& condition_expr = bcgen_expression(vm, *expr._condition);
	const auto& a_expr = bcgen_expression(vm, *expr._a);
	const auto& b_expr = bcgen_expression(vm, *expr._b);
	return bc_expression_t{ bc_expression_opcode::k_expression_conditional_operator3, e.get_annotated_type(), {condition_expr, a_expr, b_expr}, "", {}, {} };
}

bc_expression_t bcgen_comparison_expression(bgenerator_t& vm, expression_type op, const expression_t& e){
	QUARK_ASSERT(vm.check_invariant());

	const auto& expr = *e.get_simple__2();
	const auto& left_expr = bcgen_expression(vm, *expr._left);
	const auto& right_expr = bcgen_expression(vm, *expr._right);

	const std::map<expression_type, bc_expression_opcode> conv_opcode = {
		{ expression_type::k_comparison_smaller_or_equal__2, bc_expression_opcode::k_expression_comparison_smaller_or_equal },
		{ expression_type::k_comparison_smaller__2, bc_expression_opcode::k_expression_comparison_smaller },
		{ expression_type::k_comparison_larger_or_equal__2, bc_expression_opcode::k_expression_comparison_larger_or_equal },
		{ expression_type::k_comparison_larger__2, bc_expression_opcode::k_expression_comparison_larger },

		{ expression_type::k_logical_equal__2, bc_expression_opcode::k_expression_logical_equal },
		{ expression_type::k_logical_nonequal__2, bc_expression_opcode::k_expression_logical_nonequal }
	};
	const auto opcode = conv_opcode.at(expr._op);
	return bc_expression_t{ opcode, e.get_annotated_type(), {left_expr, right_expr}, "", {}, {} };
}

bc_expression_t bcgen_arithmetic_expression(bgenerator_t& vm, expression_type op, const expression_t& e){
	QUARK_ASSERT(vm.check_invariant());

	const auto& expr = *e.get_simple__2();
	const auto& left_expr = bcgen_expression(vm, *expr._left);
	const auto& right_expr = bcgen_expression(vm, *expr._right);

	const std::map<expression_type, bc_expression_opcode> conv_opcode = {
		{ expression_type::k_arithmetic_add__2, bc_expression_opcode::k_expression_arithmetic_add },
		{ expression_type::k_arithmetic_subtract__2, bc_expression_opcode::k_expression_arithmetic_subtract },
		{ expression_type::k_arithmetic_multiply__2, bc_expression_opcode::k_expression_arithmetic_multiply },
		{ expression_type::k_arithmetic_divide__2, bc_expression_opcode::k_expression_arithmetic_divide },
		{ expression_type::k_arithmetic_remainder__2, bc_expression_opcode::k_expression_arithmetic_remainder },

		{ expression_type::k_logical_and__2, bc_expression_opcode::k_expression_logical_and },
		{ expression_type::k_logical_or__2, bc_expression_opcode::k_expression_logical_or }
	};

	const auto opcode = conv_opcode.at(expr._op);
	return bc_expression_t{ opcode, e.get_annotated_type(), {left_expr, right_expr}, "", {}, {} };
}

bc_expression_t bcgen_expression(bgenerator_t& vm, const expression_t& e){
	QUARK_ASSERT(vm.check_invariant());
	QUARK_ASSERT(e.check_invariant());

	const auto& op = e.get_operation();

	if(op == expression_type::k_literal){
		return bc_expression_t{ bc_expression_opcode::k_expression_literal, e.get_annotated_type(), {}, "", {}, e.get_literal() };
	}
	else if(op == expression_type::k_resolve_member){
		return bcgen_resolve_member_expression(vm, e);
	}
	else if(op == expression_type::k_lookup_element){
		return bcgen_lookup_element_expression(vm, e);
	}
	else if(op == expression_type::k_load){
		QUARK_ASSERT(false);
	}
	else if(op == expression_type::k_load2){
		return bcgen_load2_expression(vm, e);
	}

	else if(op == expression_type::k_call){
		return bcgen_call_expression(vm, e);
	}

	else if(op == expression_type::k_define_struct){
		QUARK_ASSERT(false);
	}

	else if(op == expression_type::k_define_function){
		QUARK_ASSERT(false);
//		return e;
//		const auto& expr = e.get_function_definition();
//		return value_t::make_function_value(expr->_def);
	}

	else if(op == expression_type::k_construct_value){
		return bcgen_construct_value_expression(vm, e);
	}

	//	This can be desugared at compile time.
	else if(op == expression_type::k_arithmetic_unary_minus__1){
		return bcgen_arithmetic_unary_minus_expression(vm, e);
	}

	//	Special-case since it uses 3 expressions & uses shortcut evaluation.
	else if(op == expression_type::k_conditional_operator3){
		return bcgen_conditional_operator_expression(vm, e);
	}
	else if (false
		|| op == expression_type::k_comparison_smaller_or_equal__2
		|| op == expression_type::k_comparison_smaller__2
		|| op == expression_type::k_comparison_larger_or_equal__2
		|| op == expression_type::k_comparison_larger__2

		|| op == expression_type::k_logical_equal__2
		|| op == expression_type::k_logical_nonequal__2
	){
		return bcgen_comparison_expression(vm, op, e);
	}
	else if (false
		|| op == expression_type::k_arithmetic_add__2
		|| op == expression_type::k_arithmetic_subtract__2
		|| op == expression_type::k_arithmetic_multiply__2
		|| op == expression_type::k_arithmetic_divide__2
		|| op == expression_type::k_arithmetic_remainder__2

		|| op == expression_type::k_logical_and__2
		|| op == expression_type::k_logical_or__2
	){
		return bcgen_arithmetic_expression(vm, op, e);
	}
	else{
		QUARK_ASSERT(false);
	}
	throw std::exception();
}


bc_expression_t bcgen_call_expression(bgenerator_t& vm, const expression_t& e){
	QUARK_ASSERT(vm.check_invariant());
	QUARK_ASSERT(e.check_invariant());

	const auto& expr = *e.get_call();
	const auto& callee_expr = bcgen_expression(vm, *expr._callee);
	std::vector<bc_expression_t> args2;
	for(const auto& m: expr._args){
		const auto& m2 = bcgen_expression(vm, m);
		args2.push_back(m2);
	}
	vector<bc_expression_t> all = { callee_expr };
	all.insert(all.end(), args2.begin(), args2.end());
	return bc_expression_t{ bc_expression_opcode::k_expression_call, e.get_annotated_type(), all, "", {}, {} };
}

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
	_call_stack(other._call_stack)
{
	QUARK_ASSERT(other.check_invariant());
	QUARK_ASSERT(check_invariant());
}

void bgenerator_t::swap(bgenerator_t& other) throw(){
	other._imm.swap(this->_imm);
	_call_stack.swap(this->_call_stack);
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


bc_program_t run_bggen(const quark::trace_context_t& tracer, const ast_t& pass3){
	QUARK_ASSERT(pass3.check_invariant());

	QUARK_CONTEXT_SCOPED_TRACE(tracer, "run_bggen");

	QUARK_CONTEXT_TRACE_SS(tracer, "INPUT:  " << json_to_pretty_string(ast_to_json(pass3)._value));

	bgenerator_t a(pass3);

	std::vector<std::shared_ptr<const bc_function_definition_t>> function_defs2;
	for(int function_id = 0 ; function_id < pass3._function_defs.size() ; function_id++){
		const auto& function_def = *pass3._function_defs[function_id];
		const auto body2_ptr = function_def._body ? make_shared<bc_body_t>(bcgen_body(a, *function_def._body)) : nullptr;
		const auto function_def2 = bc_function_definition_t{
			function_def._function_type,
			function_def._args,
			body2_ptr,
			function_def._host_function_id
		};
		auto b = make_shared<const bc_function_definition_t>(function_def2);
		function_defs2.push_back(b);
	}

	const auto body2 = bcgen_body(a, a._imm->_ast_pass3._globals);
	const auto result = bc_program_t{ body2, function_defs2 };


//	const auto result = bc_program_t{pass3._globals, pass3._function_defs};

	QUARK_CONTEXT_TRACE_SS(tracer, "OUTPUT: " << json_to_pretty_string(bcprogram_to_json(result)));

	return result;
}


}	//	floyd
