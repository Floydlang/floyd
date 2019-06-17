//
//  compiler_helpers.cpp
//  floyd_speak
//
//  Created by Marcus Zetterquist on 2019-03-25.
//  Copyright © 2019 Marcus Zetterquist. All rights reserved.
//

#include "compiler_helpers.h"

#include "parser_primitives.h"
#include "floyd_parser.h"

#include "pass2.h"
#include "pass3.h"
#include "bytecode_host_functions.h"
#include "bytecode_generator.h"
#include "compiler_helpers.h"
#include "compiler_basics.h"
#include "floyd_filelib.h"

#include <thread>
#include <deque>
#include <future>

#include <pthread.h>
#include <condition_variable>

namespace floyd {

parse_tree_t parse_program__errors(const compilation_unit_t& cu){
	try {
		const auto parse_tree = parse_program2(cu.prefix_source + cu.program_text);
		return parse_tree;
	}
	catch(const compiler_error& e){
		const auto refined = refine_compiler_error_with_loc2(cu, e);
		throw_compiler_error(refined.first, refined.second);
	}
	catch(const std::exception& e){
		throw;
//		const auto location = find_source_line(const std::string& program, const std::string& file, bool corelib, const location_t& loc){
	}
}


compilation_unit_t make_compilation_unit_nolib(const std::string& source_code, const std::string& source_path){
	return compilation_unit_t{
		.prefix_source = "",
		.program_text = source_code,
		.source_file_path = source_path
	};
}

compilation_unit_t make_compilation_unit_lib(const std::string& source_code, const std::string& source_path){
	return compilation_unit_t{
		.prefix_source = k_filelib_builtin_types_and_constants + "\n",
		.program_text = source_code,
		.source_file_path = source_path
	};
}

compilation_unit_t make_compilation_unit(const std::string& source_code, const std::string& source_path, compilation_unit_mode mode){
	if(mode == compilation_unit_mode::k_include_core_lib){
		return make_compilation_unit_lib(source_code, source_path);
	}
	else if(mode == compilation_unit_mode::k_no_core_lib){
		return make_compilation_unit_nolib(source_code, source_path);
	}
	else{
		QUARK_ASSERT(false);
		throw std::exception();
	}
}



semantic_ast_t run_semantic_analysis__errors(const pass2_ast_t& pass2, const compilation_unit_t& cu){
	try {
		const auto pass3 = run_semantic_analysis(pass2);
		return pass3;
	}
	catch(const compiler_error& e){
		const auto refined = refine_compiler_error_with_loc2(cu, e);
		throw_compiler_error(refined.first, refined.second);
	}
	catch(const std::exception& e){
		throw;
//		const auto location = find_source_line(const std::string& program, const std::string& file, bool corelib, const location_t& loc){
	}
}










struct desugar_t {
	bool check_invariant() const {
		return true;
	}


	general_purpose_ast_t tree;
};

void intern_type(desugar_t& acc, const typeid_t& type){
}


static body_t desugar_body(desugar_t& acc, const body_t& body);


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
			QUARK_ASSERT(false);
			throw std::exception();
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
	};
	const auto r = std::visit(visitor_t{ acc, expression }, expression._expression_variant);
	return r;
}

static function_definition_t desugar_function_def(desugar_t& acc, const function_definition_t& def){
	struct visitor_t {
		desugar_t& acc;
		const floyd::function_definition_t& function_def;

		function_definition_t operator()(const function_definition_t::empty_t& e) const{
			QUARK_ASSERT(false);
			throw std::exception();
		}
		function_definition_t operator()(const function_definition_t::floyd_func_t& e) const{
			const auto body = desugar_body(acc, *e._body);
			const auto body2 = std::make_shared<body_t>(body);
			return function_definition_t::make_floyd_func(
				function_def._location,
				function_def._definition_name,
				function_def._function_type,
				function_def._args,
				body2
			);
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
		statement_t operator()(const statement_t::define_function_statement_t& s) const{
			auto f2 = desugar_function_def(acc, *s._def);
			const auto f3 = std::make_shared<function_definition_t>(f2);
			return statement_t::make__define_function_statement(statement.location, statement_t::define_function_statement_t{ s._name, f3 });
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

	std::vector<std::shared_ptr<const floyd::function_definition_t>> function_defs;
	for(const auto& f: ast._function_defs){
		const auto f2 = desugar_function_def(acc, *f);
		const auto f3 = std::make_shared<function_definition_t>(f2);
		function_defs.push_back(f3);
	}
	result._function_defs = function_defs;

	result._interned_types = ast._interned_types;
	result._software_system = ast._software_system;
	result._container_def = ast._container_def;

	return result;
}




pass2_ast_t desugar_pass(const pass2_ast_t& pass2){
//	QUARK_TRACE_SS("INPUT:  " << json_to_pretty_string(gp_ast_to_json(pass2._tree)));

	desugar_t acc;
	const auto result = desugar(acc, pass2._tree);

//	QUARK_TRACE_SS("OUTPUT:  " << json_to_pretty_string(gp_ast_to_json(result)));
	return pass2_ast_t { result };
}




semantic_ast_t compile_to_sematic_ast__errors(const compilation_unit_t& cu){

//	QUARK_CONTEXT_TRACE(context._tracer, json_to_pretty_string(statements_pos.first._value));
	const auto parse_tree = parse_program__errors(cu);
	const auto pass2 = parse_tree_to_pass2_ast(parse_tree);
	const auto pass2b = desugar_pass(pass2);
	const auto pass3 = run_semantic_analysis__errors(pass2b, cu);
	return pass3;
}


}	//	floyd
