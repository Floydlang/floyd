//
//  collect_used_types.cpp
//  Floyd
//
//  Created by Marcus Zetterquist on 2019-07-28.
//  Copyright © 2019 Marcus Zetterquist. All rights reserved.
//

#include "collect_used_types.h"



#include "statement.h"
#include "ast_value.h"
#include "utils.h"
#include "json_support.h"
#include "floyd_runtime.h"
#include "text_parser.h"
#include "floyd_syntax.h"
#include "collect_used_types.h"


namespace floyd {

static void collect_used_types_body(type_interner_t& acc, const body_t& body);


static void collect_used_types_expression(type_interner_t& acc, const expression_t& expression){
	struct visitor_t {
		type_interner_t& acc;
		const expression_t& expression;


		void operator()(const expression_t::literal_exp_t& e) const{
			intern_type(acc, e.value.get_type());
		}
		void operator()(const expression_t::arithmetic_t& e) const{
			collect_used_types_expression(acc, *e.lhs);
			collect_used_types_expression(acc, *e.rhs);
		}
		void operator()(const expression_t::comparison_t& e) const{
			collect_used_types_expression(acc, *e.lhs);
			collect_used_types_expression(acc, *e.rhs);
		}
		void operator()(const expression_t::unary_minus_t& e) const{
			collect_used_types_expression(acc, *e.expr);
		}
		void operator()(const expression_t::conditional_t& e) const{
			collect_used_types_expression(acc, *e.condition);
			collect_used_types_expression(acc, *e.a);
			collect_used_types_expression(acc, *e.b);
		}

		void operator()(const expression_t::call_t& e) const{
			collect_used_types_expression(acc, *e.callee);
			for(const auto& a: e.args){
				collect_used_types_expression(acc, a);
			}
		}
		void operator()(const expression_t::corecall_t& e) const{
			for(const auto& a: e.args){
				collect_used_types_expression(acc, a);
			}
		}


		void operator()(const expression_t::struct_definition_expr_t& e) const{
			QUARK_ASSERT(false);
			throw std::exception();
		}
		void operator()(const expression_t::function_definition_expr_t& e) const{
			QUARK_ASSERT(false);
			throw std::exception();
		}
		void operator()(const expression_t::load_t& e) const{
			QUARK_ASSERT(false);
			throw std::exception();
		}
		void operator()(const expression_t::load2_t& e) const{
		}

		void operator()(const expression_t::resolve_member_t& e) const{
			collect_used_types_expression(acc, *e.parent_address);
		}
		void operator()(const expression_t::update_member_t& e) const{
			collect_used_types_expression(acc, *e.parent_address);
		}
		void operator()(const expression_t::lookup_t& e) const{
			collect_used_types_expression(acc, *e.parent_address);
			collect_used_types_expression(acc, *e.lookup_key);
		}
		void operator()(const expression_t::value_constructor_t& e) const{
			intern_type(acc, e.value_type);
			for(const auto& a: e.elements){
				collect_used_types_expression(acc, a);
			}
		}
		void operator()(const expression_t::benchmark_expr_t& e) const{
			collect_used_types_body(acc, *e.body);
		}
	};
	std::visit(visitor_t{ acc, expression }, expression._expression_variant);
	intern_type(acc, expression.get_output_type());
}

static void collect_used_types(type_interner_t& acc, const statement_t& statement){
	QUARK_ASSERT(acc.check_invariant());
	QUARK_ASSERT(statement.check_invariant());

	struct visitor_t {
		type_interner_t& acc;
		const statement_t& statement;


		void operator()(const statement_t::return_statement_t& s) const{
			collect_used_types_expression(acc, s._expression);
		}
		void operator()(const statement_t::define_struct_statement_t& s) const{
			QUARK_ASSERT(false);
			throw std::exception();
		}
		void operator()(const statement_t::define_function_statement_t& s) const{
			QUARK_ASSERT(false);
			throw std::exception();
		}

		void operator()(const statement_t::bind_local_t& s) const{
			intern_type(acc, s._bindtype);
			collect_used_types_expression(acc, s._expression);
		}
		void operator()(const statement_t::assign_t& s) const{
			collect_used_types_expression(acc, s._expression);
		}
		void operator()(const statement_t::assign2_t& s) const{
			collect_used_types_expression(acc, s._expression);
		}
		void operator()(const statement_t::init2_t& s) const{
			collect_used_types_expression(acc, s._expression);
		}
		void operator()(const statement_t::block_statement_t& s) const{
			collect_used_types_body(acc, s._body);
		}

		void operator()(const statement_t::ifelse_statement_t& s) const{
			collect_used_types_expression(acc, s._condition);
			collect_used_types_body(acc, s._then_body);
			collect_used_types_body(acc, s._else_body);
		}
		void operator()(const statement_t::for_statement_t& s) const{
			collect_used_types_expression(acc, s._start_expression);
			collect_used_types_expression(acc, s._end_expression);
			collect_used_types_body(acc, s._body);
		}
		void operator()(const statement_t::while_statement_t& s) const{
			collect_used_types_expression(acc, s._condition);
			collect_used_types_body(acc, s._body);
		}

		void operator()(const statement_t::expression_statement_t& s) const{
			collect_used_types_expression(acc, s._expression);
		}
		void operator()(const statement_t::software_system_statement_t& s) const{
		}
		void operator()(const statement_t::container_def_statement_t& s) const{
		}
		void operator()(const statement_t::benchmark_def_statement_t& s) const{
			collect_used_types_body(acc, s._body);
		}
	};

	std::visit(visitor_t{ acc, statement }, statement._contents);
}

void collect_used_types_symbol(type_interner_t& acc, const std::string& name, const symbol_t& symbol){
	intern_type(acc, symbol.get_type());
}

static void collect_used_types_body(type_interner_t& acc, const body_t& body){
	for(const auto& s: body._statements){
		collect_used_types(acc, s);
	}
	for(const auto& s: body._symbol_table._symbols){
		collect_used_types_symbol(acc, s.first, s.second);
	}
}

//??? Make this into general purpose function that collect all types.
void collect_used_types(type_interner_t& acc, const general_purpose_ast_t& ast){
	collect_used_types_body(acc, ast._globals);
	for(const auto& f: ast._function_defs){
		intern_type(acc, f->_function_type);
		for(const auto& m: f->_args){
			intern_type(acc, m._type);
		}

		const auto floyd_func = std::get_if<function_definition_t::floyd_func_t>(&f->_contents);
		if(floyd_func){
			if(floyd_func->_body){
				collect_used_types_body(acc, *floyd_func->_body);
			}
			else{
			}
		}
	}
}

}	// floyd

