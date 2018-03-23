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
std::vector<bc_instruction_t> bcgen_statements(bgenerator_t& vm, const std::vector<std::shared_ptr<statement_t>>& statements);
bc_body_t bcgen_body(bgenerator_t& vm, const body_t& body);
bc_instruction_t bcgen_statement(bgenerator_t& vm, const statement_t& statement);







int bc_limit(int value, int min, int max){
	if(value < min){
		return min;
	}
	else if(value > max){
		return max;
	}
	else{
		return value;
	}
}

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


int bc_compare_struct_true_deep(const std::vector<bc_value_t>& left, const std::vector<bc_value_t>& right){
	std::vector<bc_value_t>::const_iterator a_it = left.begin();
	std::vector<bc_value_t>::const_iterator b_it = right.begin();

	while(a_it !=left.end()){
		int diff = bc_value_t::compare_value_true_deep(*a_it, *b_it);
		if(diff != 0){
			return diff;
		}

		a_it++;
		b_it++;
	}
	return 0;
}

//	Compare vector element by element.
//	### Think more of equality when vectors have different size and shared elements are equal.
int bc_compare_vector_true_deep(const std::vector<bc_value_t>& left, const std::vector<bc_value_t>& right){
//	QUARK_ASSERT(left.check_invariant());
//	QUARK_ASSERT(right.check_invariant());
//	QUARK_ASSERT(left._element_type == right._element_type);

	const auto& shared_count = std::min(left.size(), right.size());
	for(int i = 0 ; i < shared_count ; i++){
		const auto element_result = bc_value_t::compare_value_true_deep(left[i], right[i]);
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


int bc_compare_dict_true_deep(const std::map<std::string, bc_value_t>& left, const std::map<std::string, bc_value_t>& right){
	auto left_it = left.begin();
	auto left_end_it = left.end();

	auto right_it = right.begin();
	auto right_end_it = right.end();

	while(left_it != left_end_it && right_it != right_end_it && *left_it == *right_it){
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
	else if(left_it != left_end_it && right_it != right_end_it){
		int key_diff = bc_compare_string(left_it->first, right_it->first);
		if(key_diff != 0){
			return key_diff;
		}
		else {
			return bc_value_t::compare_value_true_deep(left_it->second, right_it->second);
		}
	}
	else{
		QUARK_ASSERT(false)
		throw std::exception();
	}
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

int bc_value_t::compare_value_true_deep(const bc_value_t& left, const bc_value_t& right){
	QUARK_ASSERT(left.check_invariant());
	QUARK_ASSERT(right.check_invariant());
	QUARK_ASSERT(left.get_debug_type() == right.get_debug_type());

	if(left.is_null()){
		return 0;
	}
	else if(left.is_bool()){
		return (left.get_bool_value() ? 1 : 0) - (right.get_bool_value() ? 1 : 0);
	}
	else if(left.is_int()){
		return bc_limit(left.get_int_value() - right.get_int_value(), -1, 1);
	}
	else if(left.is_float()){
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
	else if(left.is_string()){
		return bc_compare_string(left.get_string_value(), right.get_string_value());
	}
	else if(left.is_json_value()){
		return bc_compare_json_values(left.get_json_value(), right.get_json_value());
	}
	else if(left.is_typeid()){
	//???
		if(left.get_typeid_value() == right.get_typeid_value()){
			return 0;
		}
		else{
			return -1;//??? Hack -- should return +1 depending on values.
		}
	}
	else if(left.is_struct()){
		//	Make sure the EXACT struct types are the same -- not only that they are both structs
//		if(left.get_type() != right.get_type()){
//			throw std::runtime_error("Cannot compare structs of different type.");
//		}

		//	Shortcut: same obejct == we know values are same without having to check them.
		if(left.get_struct_value() == right.get_struct_value()){
			return 0;
		}
		else{
			return bc_compare_struct_true_deep(left.get_struct_value(), right.get_struct_value());
		}
	}
	else if(left.is_vector()){
		//	Make sure the EXACT types are the same -- not only that they are both vectors.
//		if(left.get_type() != right.get_type()){

		const auto& left_vec = left.get_vector_value();
		const auto& right_vec = right.get_vector_value();
		return bc_compare_vector_true_deep(left_vec, right_vec);
	}
	else if(left.is_dict()){
		//	Make sure the EXACT types are the same -- not only that they are both dicts.
//		if(left.get_type() != right.get_type()){
		const auto& left2 = left.get_dict_value();
		const auto& right2 = right.get_dict_value();
		return bc_compare_dict_true_deep(left2, right2);
	}
	else if(left.is_function()){
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

std::vector<bc_instruction_t> bcgen_statements(bgenerator_t& vm, const std::vector<std::shared_ptr<statement_t>>& statements){
	QUARK_ASSERT(vm.check_invariant());
	for(const auto i: statements){ QUARK_ASSERT(i->check_invariant()); };

	std::vector<bc_instruction_t> result;
	int statement_index = 0;
	while(statement_index < statements.size()){
		const auto& statement = statements[statement_index];
		const auto& r = bcgen_statement(vm, *statement);
		if(r._opcode != bc_statement_opcode::k_nop){
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


bc_instruction_t bcgen_store2_statement(bgenerator_t& vm, const statement_t::store2_t& statement){
	QUARK_ASSERT(vm.check_invariant());

	const auto& expr = bcgen_expression(vm, statement._expression);
	return bc_instruction_t{ bc_statement_opcode::k_statement_store, expr._type, {expr}, statement._dest_variable,  {}};
}

bc_instruction_t bcgen_return_statement(bgenerator_t& vm, const statement_t::return_statement_t& statement){
	QUARK_ASSERT(vm.check_invariant());

	const auto& expr = bcgen_expression(vm, statement._expression);
	return bc_instruction_t{ bc_statement_opcode::k_statement_return, expr._type, {expr}, {}, {}};
}

bc_instruction_t bcgen_ifelse_statement(bgenerator_t& vm, const statement_t::ifelse_statement_t& statement){
	QUARK_ASSERT(vm.check_invariant());

	const auto& condition_expr = bcgen_expression(vm, statement._condition);
	QUARK_ASSERT(statement._condition.get_annotated_type().is_bool());
	const auto& then_expr = bcgen_body(vm, statement._then_body);
	const auto& else_expr = bcgen_body(vm, statement._else_body);
	return bc_instruction_t{ bc_statement_opcode::k_statement_if, bc_typeid_t(), {condition_expr}, {}, { then_expr, else_expr }};
}

bc_instruction_t bcgen_for_statement(bgenerator_t& vm, const statement_t::for_statement_t& statement){
	QUARK_ASSERT(vm.check_invariant());

	const auto& start_expr = bcgen_expression(vm, statement._start_expression);
	const auto& end_expr = bcgen_expression(vm, statement._end_expression);
	const auto& body = bcgen_body(vm, statement._body);
	return bc_instruction_t{ bc_statement_opcode::k_statement_for, bc_typeid_t(), {start_expr, end_expr}, {}, { body } };
}

bc_instruction_t bcgen_while_statement(bgenerator_t& vm, const statement_t::while_statement_t& statement){
	QUARK_ASSERT(vm.check_invariant());

	const auto& condition_expr = bcgen_expression(vm, statement._condition);
	const auto& body = bcgen_body(vm, statement._body);
	return bc_instruction_t{ bc_statement_opcode::k_statement_while, bc_typeid_t(), {condition_expr}, {}, {body} };
}

bc_instruction_t bcgen_expression_statement(bgenerator_t& vm, const statement_t::expression_statement_t& statement){
	QUARK_ASSERT(vm.check_invariant());

	const auto& expr = bcgen_expression(vm, statement._expression);
	return bc_instruction_t{ bc_statement_opcode::k_statement_expression, bc_typeid_t(), {expr}, {}, {} };
}

bc_instruction_t bcgen_statement(bgenerator_t& vm, const statement_t& statement){
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
		return bc_instruction_t{ bc_statement_opcode::k_statement_block, bc_typeid_t(), {}, {}, { body} };
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
	const auto member_name = expr._member_name;

	const auto& struct_def = expr._parent_address->get_annotated_type().get_struct();
	int index = find_struct_member_index(struct_def, member_name);
	QUARK_ASSERT(index != -1);

	//	Store member index in address field.
	return bc_expression_t{ bc_expression_opcode::k_expression_resolve_member, bc_typeid_t(e.get_annotated_type()), {parent_expr}, variable_address_t::make_variable_address(0, index), {}, {} };
}

bc_expression_t bcgen_lookup_element_expression(bgenerator_t& vm, const expression_t& e){
	QUARK_ASSERT(vm.check_invariant());

	const auto& expr = *e.get_lookup();
	const auto& parent_expr = bcgen_expression(vm, *expr._parent_address);
	const auto& key_expr = bcgen_expression(vm, *expr._lookup_key);
	return bc_expression_t{ bc_expression_opcode::k_expression_lookup_element, bc_typeid_t(e.get_annotated_type()), {parent_expr, key_expr}, {}, {}, {} };
}

bc_expression_t bcgen_load2_expression(bgenerator_t& vm, const expression_t& e){
	QUARK_ASSERT(vm.check_invariant());

	const auto& expr = *e.get_load2();
	const auto& address = expr._address;
	return bc_expression_t{ bc_expression_opcode::k_expression_load, bc_typeid_t(e.get_annotated_type()), {}, address, {}, {} };
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

	const auto extra_type = args2.size() > 0 ? args2[0]._type : bc_typeid_t{};
	return bc_expression_t{ bc_expression_opcode::k_expression_construct_value, bc_typeid_t(value_type), args2, {}, {}, extra_type };
}

bc_expression_t bcgen_arithmetic_unary_minus_expression(bgenerator_t& vm, const expression_t& e){
	QUARK_ASSERT(vm.check_invariant());

	const auto& expr = *e.get_unary_minus();
	const auto& exprx = bcgen_expression(vm, *expr._expr);
	return bc_expression_t{ bc_expression_opcode::k_expression_arithmetic_unary_minus, bc_typeid_t(e.get_annotated_type()), {exprx}, {}, {}, {} };
}

bc_expression_t bcgen_conditional_operator_expression(bgenerator_t& vm, const expression_t& e){
	QUARK_ASSERT(vm.check_invariant());

	const auto& expr = *e.get_conditional();
	const auto& condition_expr = bcgen_expression(vm, *expr._condition);
	const auto& a_expr = bcgen_expression(vm, *expr._a);
	const auto& b_expr = bcgen_expression(vm, *expr._b);
	return bc_expression_t{ bc_expression_opcode::k_expression_conditional_operator3, bc_typeid_t(e.get_annotated_type()), {condition_expr, a_expr, b_expr}, {}, {}, {} };
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
	return bc_expression_t{ opcode, bc_typeid_t(e.get_annotated_type()), {left_expr, right_expr}, {}, {}, {} };
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
	return bc_expression_t{ opcode, bc_typeid_t(e.get_annotated_type()), {left_expr, right_expr}, {}, {}, {} };
}

bc_expression_t bcgen_expression(bgenerator_t& vm, const expression_t& e){
	QUARK_ASSERT(vm.check_invariant());
	QUARK_ASSERT(e.check_invariant());

	const auto& op = e.get_operation();

	if(op == expression_type::k_literal){
		return bc_expression_t{ bc_expression_opcode::k_expression_literal, bc_typeid_t(e.get_annotated_type()), {}, {}, value_to_bc(e.get_literal()), {} };
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
	return bc_expression_t{ bc_expression_opcode::k_expression_call, bc_typeid_t(e.get_annotated_type()), all, {}, {}, {} };
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

	std::vector<const bc_function_definition_t> function_defs2;
	for(int function_id = 0 ; function_id < pass3._function_defs.size() ; function_id++){
		const auto& function_def = *pass3._function_defs[function_id];
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
	const auto result = bc_program_t{ body2, function_defs2 };


//	const auto result = bc_program_t{pass3._globals, pass3._function_defs};

	QUARK_CONTEXT_TRACE_SS(tracer, "OUTPUT: " << json_to_pretty_string(bcprogram_to_json(result)));

	return result;
}


}	//	floyd
