//
//  desugar_pass.cpp
//  floyd
//
//  Created by Marcus Zetterquist on 2019-07-29.
//  Copyright © 2019 Marcus Zetterquist. All rights reserved.
//

#include "desugar_pass.h"

#include "ast.h"


namespace floyd {

struct desugar_t {
	bool check_invariant() const {
		return true;
	}

	general_purpose_ast_t tree;
};

void intern_type(desugar_t& acc, const typeid_t& type){
}


static body_t desugar_body(desugar_t& acc, const body_t& body);
static function_definition_t desugar_function_def(desugar_t& acc, const function_definition_t& def);


static expression_t desugar_expression(desugar_t& acc, const expression_t& expression){
	struct visitor_t {
		desugar_t& acc;
		const expression_t& expression;


		expression_t operator()(const expression_t::literal_exp_t& e) const{
			intern_type(acc, e.value.get_type());
			return expression;
		}
		expression_t operator()(const expression_t::arithmetic_t& e) const{
			const auto lhs = desugar_expression(acc, *e.lhs);
			const auto rhs = desugar_expression(acc, *e.rhs);
			return expression_t::make_arithmetic(e.op, lhs, rhs, expression._output_type);
		}
		expression_t operator()(const expression_t::comparison_t& e) const{
			const auto lhs = desugar_expression(acc, *e.lhs);
			const auto rhs = desugar_expression(acc, *e.rhs);
			return expression_t::make_comparison(e.op, lhs, rhs, expression._output_type);
		}
		expression_t operator()(const expression_t::unary_minus_t& e) const{
			const auto e2 = desugar_expression(acc, *e.expr);
			return expression_t::make_unary_minus(e2, expression._output_type);
		}
		expression_t operator()(const expression_t::conditional_t& e) const{
			const auto condition = desugar_expression(acc, *e.condition);
			const auto a = desugar_expression(acc, *e.a);
			const auto b = desugar_expression(acc, *e.b);
			return expression_t::make_conditional_operator(condition, a, b, expression._output_type);
		}

		expression_t operator()(const expression_t::call_t& e) const{
			const auto callee = desugar_expression(acc, *e.callee);
			std::vector<expression_t> args;
			for(const auto& a: e.args){
				args.push_back(desugar_expression(acc, a));
			}

//	No need to desugar update() anymore. That is built into parser now.
#if 0
			const auto load = std::get_if<expression_t::load_t>(&callee._contents);
			if(load){
//				QUARK_TRACE_SS(load->variable_name);

				//	TURN update(a, b, c) into an update-expression.
				if(load->variable_name == "update" && args.size() == 3){
					return expression_t::make_update(args[0], args[1], args[2], expression._output_type);
				}
			}
			const auto load2 = std::get_if<expression_t::load2_t>(&callee._contents);
			if(load2){
				QUARK_TRACE_SS(load2->address._parent_steps);
				QUARK_TRACE_SS(load2->address._index);
			}
#endif

			return expression_t::make_call(callee, args, expression._output_type);
		}
		expression_t operator()(const expression_t::corecall_t& e) const{
			std::vector<expression_t> args;
			for(const auto& a: e.args){
				args.push_back(desugar_expression(acc, a));
			}
			return expression_t::make_corecall(e.call_name, args, expression._output_type);
		}


		expression_t operator()(const expression_t::struct_definition_expr_t& e) const{
			QUARK_ASSERT(false);
			throw std::exception();
		}
		expression_t operator()(const expression_t::function_definition_expr_t& e) const{
			const auto a = desugar_function_def(acc, e.def);
			return expression_t::make_function_definition(a);
		}
		expression_t operator()(const expression_t::load_t& e) const{
			return expression;
		}
		expression_t operator()(const expression_t::load2_t& e) const{
			return expression;
		}

		expression_t operator()(const expression_t::resolve_member_t& e) const{
			const auto a = desugar_expression(acc, *e.parent_address);
			return expression_t::make_resolve_member(a, e.member_name, expression._output_type);
		}
		expression_t operator()(const expression_t::update_member_t& e) const{
			const auto parent = desugar_expression(acc, *e.parent_address);
			const auto new_value = desugar_expression(acc, *e.new_value);
			return expression_t::make_update_member(parent, e.member_index, new_value, expression._output_type);
		}
		expression_t operator()(const expression_t::lookup_t& e) const{
			const auto parent = desugar_expression(acc, *e.parent_address);
			const auto key = desugar_expression(acc, *e.lookup_key);
			return expression_t::make_lookup(parent, key, expression._output_type);
		}
		expression_t operator()(const expression_t::value_constructor_t& e) const{
			std::vector<expression_t> elements;
			for(const auto& a: e.elements){
				elements.push_back(desugar_expression(acc, a));
			}
			return expression_t::make_construct_value_expr(e.value_type, elements);
		}
		expression_t operator()(const expression_t::benchmark_expr_t& e) const{
			const auto body = desugar_body(acc, *e.body);
			return expression_t::make_benchmark_expr(body);
		}
	};
	const auto r = std::visit(visitor_t{ acc, expression }, expression._expression_variant);
	return r;
}

static function_definition_t desugar_function_def(desugar_t& acc, const function_definition_t& def){
	struct visitor_t {
		desugar_t& acc;
		const floyd::function_definition_t& function_def;

		function_definition_t operator()(const function_definition_t::empty_t& e) const{
			return function_def;
		}
		function_definition_t operator()(const function_definition_t::floyd_func_t& e) const{
			if(e._body){
				const auto body = desugar_body(acc, *e._body);
				const auto body2 = std::make_shared<body_t>(body);
				return function_definition_t::make_floyd_func(
					function_def._location,
					function_def._definition_name,
					function_def._function_type,
					function_def._named_args,
					body2
				);
			}
			else{
				return function_definition_t::make_floyd_func(
					function_def._location,
					function_def._definition_name,
					function_def._function_type,
					function_def._named_args,
					{}
				);
			}
		}
		function_definition_t operator()(const function_definition_t::host_func_t& e) const{
			return function_def;
		}
	};
	const auto f2 = std::visit(visitor_t{ acc, def }, def._contents);
	return f2;
}

static statement_t desugar_statement(desugar_t& acc, const statement_t& statement){
	QUARK_ASSERT(acc.check_invariant());
	QUARK_ASSERT(statement.check_invariant());

	struct visitor_t {
		desugar_t& acc;
		const statement_t& statement;


		statement_t operator()(const statement_t::return_statement_t& s) const{
			const auto e = desugar_expression(acc, s._expression);
			return statement_t::make__return_statement(statement.location, e);
		}
		statement_t operator()(const statement_t::define_struct_statement_t& s) const{
			return statement;
		}

		statement_t operator()(const statement_t::bind_local_t& s) const{
			const auto e = desugar_expression(acc, s._expression);
			return statement_t::make__bind_local(statement.location, s._new_local_name, s._bindtype, e, s._locals_mutable_mode);
		}
		statement_t operator()(const statement_t::assign_t& s) const{
			const auto e = desugar_expression(acc, s._expression);
			return statement_t::make__assign(statement.location, s._local_name, e);
		}
		statement_t operator()(const statement_t::assign2_t& s) const{
			const auto e = desugar_expression(acc, s._expression);
			return statement_t::make__assign2(statement.location, s._dest_variable, e);
		}
		statement_t operator()(const statement_t::init2_t& s) const{
			const auto e = desugar_expression(acc, s._expression);
			return statement_t::make__init2(statement.location, s._dest_variable, e);
		}
		statement_t operator()(const statement_t::block_statement_t& s) const{
			const auto e = desugar_body(acc, s._body);
			return statement_t::make__block_statement(statement.location, e);
		}

		statement_t operator()(const statement_t::ifelse_statement_t& s) const{
			const auto condition = desugar_expression(acc, s._condition);
			const auto then_body = desugar_body(acc, s._then_body);
			const auto else_body = desugar_body(acc, s._else_body);
			return statement_t::make__ifelse_statement(statement.location, condition, then_body, else_body);
		}
		statement_t operator()(const statement_t::for_statement_t& s) const{
			const auto start = desugar_expression(acc, s._start_expression);
			const auto end = desugar_expression(acc, s._end_expression);
			const auto body = desugar_body(acc, s._body);
			return statement_t::make__for_statement(statement.location, s._iterator_name, start, end, body, s._range_type);
		}
		statement_t operator()(const statement_t::while_statement_t& s) const{
			const auto condition = desugar_expression(acc, s._condition);
			const auto body = desugar_body(acc, s._body);
			return statement_t::make__while_statement(statement.location, condition, body);
		}

		statement_t operator()(const statement_t::expression_statement_t& s) const{
			const auto e = desugar_expression(acc, s._expression);
			return statement_t::make__expression_statement(statement.location, e);
		}
		statement_t operator()(const statement_t::software_system_statement_t& s) const{
			return statement;
		}
		statement_t operator()(const statement_t::container_def_statement_t& s) const{
			return statement;
		}
		statement_t operator()(const statement_t::benchmark_def_statement_t& s) const{
			return statement;
		}
	};

	return std::visit(visitor_t{ acc, statement }, statement._contents);
}

static std::pair<std::string, floyd::symbol_t> desugar_symbol(desugar_t& acc, const std::string& name, const floyd::symbol_t& symbol){
	return { name, symbol };
}

static body_t desugar_body(desugar_t& acc, const body_t& body){
	body_t result;
	for(const auto& s: body._statements){
		const auto s2 = desugar_statement(acc, s);
		result._statements.push_back(s2);
	}
	for(const auto& s: body._symbol_table._symbols){
		const auto s2 = desugar_symbol(acc, s.first, s.second);
		result._symbol_table._symbols.push_back(s2);
	}
	return result;
}

//??? Make this into general purpose function that collect all types.
general_purpose_ast_t desugar(desugar_t& acc, const general_purpose_ast_t& ast){
	general_purpose_ast_t result;

	result._globals = desugar_body(acc, ast._globals);

	std::vector<floyd::function_definition_t> function_defs;
	for(const auto& f: ast._function_defs){
		const auto f2 = desugar_function_def(acc, f);
		function_defs.push_back(f2);
	}
	result._function_defs = function_defs;

	result._interned_types = ast._interned_types;
	result._software_system = ast._software_system;
	result._container_def = ast._container_def;

	return result;
}




unchecked_ast_t desugar_pass(const unchecked_ast_t& unchecked_ast){
//	QUARK_TRACE_SS("INPUT:  " << json_to_pretty_string(gp_ast_to_json(unchecked_ast._tree)));

	desugar_t acc;
	const auto result = desugar(acc, unchecked_ast._tree);

//	QUARK_TRACE_SS("OUTPUT:  " << json_to_pretty_string(gp_ast_to_json(result)));
	return unchecked_ast_t { result };
}


}	// floyd
