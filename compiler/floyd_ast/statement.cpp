//
//  parser_statement.cpp
//  Floyd
//
//  Created by Marcus Zetterquist on 26/07/16.
//  Copyright © 2016 Marcus Zetterquist. All rights reserved.
//

#include "statement.h"

#include "ast_helpers.h"


namespace floyd {




////////////////////////////////////////////		symbol_t


static std::string symbol_type_to_string(symbol_t::symbol_type type){
	switch(type){
		case symbol_t::symbol_type::immutable_reserve:
			return "immutable_reserve";

		case symbol_t::symbol_type::immutable_arg:
			return "immutable_arg";

		case symbol_t::symbol_type::immutable_precalc:
			return "immutable_precalc";

		case symbol_t::symbol_type::named_type:
			return "named_type";

		case symbol_t::symbol_type::mutable_reserve:
			return "mutable_reserve";
		default:
			QUARK_ASSERT(false);
			throw std::exception();
	}
}

static symbol_t::symbol_type symbol_type_from_string(const std::string& s){
	if(s == "immutable_reserve"){
		return symbol_t::symbol_type::immutable_reserve;
	}
	else if(s == "immutable_arg"){
		return symbol_t::symbol_type::immutable_arg;
	}

	else if(s == "immutable_precalc"){
		return symbol_t::symbol_type::immutable_precalc;
	}


	else if(s == "named_type"){
		return symbol_t::symbol_type::named_type;
	}

	else if(s == "mutable_reserve"){
		return symbol_t::symbol_type::mutable_reserve;
	}
	else{
		QUARK_ASSERT(false);
		throw std::exception();
	}
}

std::string symbol_to_string(const type_interner_t& interner, const symbol_t& s){
	std::stringstream out;
	out << "{ "
		<< symbol_type_to_string(s._symbol_type)
		<< ", type: " << itype_to_compact_string(interner, s._value_type, resolve_named_types::dont_resolve)
		<< ", init: " << (s._init.is_undefined() ? "<none>" : value_and_type_to_string(interner, s._init))
	<< " }";
	return out.str();
}


static json_t symbol_to_json(const type_interner_t& interner, const symbol_t& symbol){
	const auto symbol_type_str = symbol_type_to_string(symbol._symbol_type);
	const auto value_type = itype_to_json(interner, symbol._value_type);

	const auto e2 = json_t::make_object({
		{ "symbol_type", symbol_type_str },
		{ "value_type", value_type },
		{ "init", value_to_ast_json(interner, symbol._init) }
	});
	return e2;
}

static symbol_t json_to_symbol(type_interner_t& interner, const json_t& e){
	QUARK_ASSERT(interner.check_invariant());

	const auto symbol_type = e.get_object_element("symbol_type").get_string();
	const auto value_type = e.get_object_element("value_type");
	const auto init = e.get_object_element("init");

	const auto symbol_type2 = symbol_type_from_string(symbol_type);
	const auto value_type2 = itype_from_json(interner, value_type);
	const auto value_type1 = value_type2;
	value_t init_value2 = init.is_null() ? value_t::make_undefined() : ast_json_to_value(interner, value_type1, init);

	const auto result = symbol_t(symbol_type2, value_type2, init_value2);
	return result;
}



////////////////////////////////////////////		symbol_table_t




const symbol_t* find_symbol(const symbol_table_t& symbol_table, const std::string& name){
	const auto it = std::find_if(
		symbol_table._symbols.begin(), symbol_table._symbols.end(),
		[&name](const std::pair<std::string, symbol_t>& x) { return x.first == name; }
	);
	
	return (it == symbol_table._symbols.end()) ? nullptr : &(it->second);
}
const symbol_t& find_symbol_required(const symbol_table_t& symbol_table, const std::string& name){
	const auto it = std::find_if(
		symbol_table._symbols.begin(), symbol_table._symbols.end(),
		[&name](const std::pair<std::string, symbol_t>& x) { return x.first == name; }
	);
	if(it == symbol_table._symbols.end()){
		throw std::exception();
	}
	return it->second;
}

std::vector<json_t> symbols_to_json(const type_interner_t& interner, const symbol_table_t& symbol_table){
	std::vector<json_t> r;
	int symbol_index = 0;
	for(const auto& e: symbol_table._symbols){
		const auto symbol1 = symbol_to_json(interner, e.second);
		const auto e2 = json_t::make_array({
			symbol_index,
			e.first,
			symbol1
		});
		r.push_back(e2);
		symbol_index++;
	}
	return r;
}

symbol_table_t ast_json_to_symbols(type_interner_t& interner, const json_t& p){
	QUARK_ASSERT(interner.check_invariant());

	std::vector<std::pair<std::string, symbol_t>> result;
	const auto json_array = p.get_array();
	for(const auto& e: json_array){
		const auto symbol_array_json = e;
		const auto symbol_index = e.get_array_n(0).get_number();
		const auto symbol_name = e.get_array_n(1).get_string();
		const auto symbol = json_to_symbol(interner, e.get_array_n(2));

		while(result.size() < symbol_index){
			result.push_back({"dummy_symbol #" + std::to_string(result.size()), symbol});
		}

		QUARK_ASSERT(result.size() == symbol_index);
		result.push_back({ symbol_name, symbol } );
	}
	return symbol_table_t{ result };
}



////////////////////////////////////////////		BODY




bool body_t::check_invariant() const {
	for(const auto& i: _statements){
		QUARK_ASSERT(i.check_invariant());
	};
	return true;
}

json_t body_to_json(const type_interner_t& interner, const body_t& e){
	std::vector<json_t> statements;
	for(const auto& i: e._statements){
		statements.push_back(statement_to_json(interner, i));
	}

	const auto symbols = symbols_to_json(interner, e._symbol_table);

	return json_t::make_object({
		std::pair<std::string, json_t>{ "statements", json_t::make_array(statements) },
		std::pair<std::string, json_t>{ "symbols", json_t::make_array(symbols) }
	});
}

body_t json_to_body(type_interner_t& interner, const json_t& json){
	QUARK_ASSERT(interner.check_invariant());

	const auto statements = json.does_object_element_exist("statements") ? json.get_object_element("statements") : json_t();
	const auto statements1 = statements.is_null() ? std::vector<statement_t>() : ast_json_to_statements(interner, statements);

	const auto symbols = json.does_object_element_exist("symbols") ? json.get_object_element("symbols") : json_t();
	const auto symbols1 = symbols.is_null() ? symbol_table_t{} : ast_json_to_symbols(interner, symbols);

	return body_t(statements1, symbols1);
}

bool operator==(const body_t& lhs, const body_t& rhs){
	return
		lhs._statements == rhs._statements
		&& lhs._symbol_table == rhs._symbol_table;
}




////////////////////////////////////////////		statement_t



static statement_t ast_json_to_statement(type_interner_t& interner, const json_t& statement0){
	QUARK_ASSERT(interner.check_invariant());
	QUARK_ASSERT(statement0.check_invariant());
	QUARK_ASSERT(statement0.is_array());

	const auto statement1 = unpack_loc(statement0);	
	const auto loc = statement1.second;
	const auto statement = statement1.first;
	const std::string type = statement.get_array_n(0).get_string();

	if(type == statement_opcode_t::k_return){
		QUARK_ASSERT(statement.get_array_size() == 2);
		const auto expr = ast_json_to_expression(interner, statement.get_array_n(1));
		return statement_t::make__return_statement(loc, expr);
	}

	//	Last element is a list of meta info, like "mutable" etc.
	else if(type == statement_opcode_t::k_init_local){
		QUARK_ASSERT(statement.get_array_size() == 4 || statement.get_array_size() == 5);
		const auto bind_type = statement.get_array_n(1);
		const auto name = statement.get_array_n(2);
		const auto expr = statement.get_array_n(3);
		const auto meta = statement.get_array_size() >= 5 ? statement.get_array_n(4) : json_t();

		const auto bind_type2 = itype_from_json(interner, bind_type);
		const auto name2 = name.get_string();
		const auto expr2 = ast_json_to_expression(interner, expr);
		bool mutable_flag = !meta.is_null() && meta.does_object_element_exist("mutable");
		const auto mutable_mode = mutable_flag ? statement_t::bind_local_t::k_mutable : statement_t::bind_local_t::k_immutable;
		return statement_t::make__bind_local(loc, name2, bind_type2, expr2, mutable_mode);
	}

	else if(type == statement_opcode_t::k_assign){
		QUARK_ASSERT(statement.get_array_size() == 3);
		const auto name = statement.get_array_n(1);
		const auto expr = statement.get_array_n(2);

		const auto name2 = name.get_string();
		const auto expr2 = ast_json_to_expression(interner, expr);
		return statement_t::make__assign(loc, name2, expr2);
	}

	else if(type == statement_opcode_t::k_assign2){
		QUARK_ASSERT(statement.get_array_size() == 4);
		const auto parent_index = (int)statement.get_array_n(1).get_number();
		const auto variable_index = (int)statement.get_array_n(2).get_number();
		const auto expr = statement.get_array_n(3);

		const auto expr2 = ast_json_to_expression(interner, expr);
		return statement_t::make__assign2(loc, symbol_pos_t::make_stack_pos(parent_index, variable_index), expr2);
	}

	else if(type == statement_opcode_t::k_init_local2){
		QUARK_ASSERT(statement.get_array_size() == 4);
		const auto parent_index = (int)statement.get_array_n(1).get_number();
		const auto variable_index = (int)statement.get_array_n(2).get_number();
		const auto expr = statement.get_array_n(3);

		const auto expr2 = ast_json_to_expression(interner, expr);
		return statement_t::make__init2(loc, symbol_pos_t::make_stack_pos(parent_index, variable_index), expr2);
	}

	else if(type == statement_opcode_t::k_block){
		QUARK_ASSERT(statement.get_array_size() == 2);

		const auto statements = statement.get_array_n(1);
		const auto r = ast_json_to_statements(interner, statements);

		const auto body = body_t(r);
		return statement_t::make__block_statement(loc, body);
	}

	else if(type == statement_opcode_t::k_if){
		QUARK_ASSERT(statement.get_array_size() == 3 || statement.get_array_size() == 4);
		const auto condition_expression = statement.get_array_n(1);
		const auto then_statements = statement.get_array_n(2);
		const auto else_statements = statement.get_array_size() == 4 ? statement.get_array_n(3) : json_t::make_array();

		const auto condition_expression2 = ast_json_to_expression(interner, condition_expression);
		const auto then_statements2 = ast_json_to_statements(interner, then_statements);
		const auto else_statements2 = ast_json_to_statements(interner, else_statements);

		return statement_t::make__ifelse_statement(
			loc,
			condition_expression2,
			body_t{ then_statements2 },
			body_t{ else_statements2 }
		);
	}
	else if(type == statement_opcode_t::k_for){
		QUARK_ASSERT(statement.get_array_size() == 6);
		const auto for_mode = statement.get_array_n(1);
		const auto iterator_name = statement.get_array_n(2);
		const auto start_expression = statement.get_array_n(3);
		const auto end_expression = statement.get_array_n(4);
		const auto body_statements = statement.get_array_n(5);

		const auto start_expression2 = ast_json_to_expression(interner, start_expression);
		const auto end_expression2 = ast_json_to_expression(interner, end_expression);
		const auto body_statements2 = ast_json_to_statements(interner, body_statements);

		const auto range_type = for_mode.get_string() == "open-range" ? statement_t::for_statement_t::k_open_range : statement_t::for_statement_t::k_closed_range;
		return statement_t::make__for_statement(
			loc,
			iterator_name.get_string(),
			start_expression2,
			end_expression2,
			body_t{ body_statements2 },
			range_type
		);
	}
	else if(type == statement_opcode_t::k_while){
		QUARK_ASSERT(statement.get_array_size() == 3);
		const auto expression = statement.get_array_n(1);
		const auto body_statements = statement.get_array_n(2);

		const auto expression2 = ast_json_to_expression(interner, expression);
		const auto body_statements2 = ast_json_to_statements(interner, body_statements);//??? should use json_to_body()!?

		return statement_t::make__while_statement(loc, expression2, body_t{ body_statements2 });
	}

	else if(type == statement_opcode_t::k_expression_statement){
		QUARK_ASSERT(statement.get_array_size() == 2);
		const auto expr = statement.get_array_n(1);
		const auto expr2 = ast_json_to_expression(interner, expr);
		return statement_t::make__expression_statement(loc, expr2);
	}

	else if(type == statement_opcode_t::k_software_system_def){
		QUARK_ASSERT(statement.get_array_size() == 2);
		const auto json_data = statement.get_array_n(1);

		return statement_t::make__software_system_statement(loc, json_data);
	}
	else if(type == statement_opcode_t::k_container_def){
		QUARK_ASSERT(statement.get_array_size() == 2);
		const auto json_data = statement.get_array_n(1);

		return statement_t::make__container_def_statement(loc, json_data);
	}

	else if(type == statement_opcode_t::k_benchmark_def){
		QUARK_ASSERT(statement.get_array_size() == 3);
		const auto name = statement.get_array_n(1);
		const auto body = statement.get_array_n(2);
		const auto body_statements2 = ast_json_to_statements(interner, body);//??? should use json_to_body()!?

		return statement_t::make__benchmark_def_statement(loc, name.get_string(), body_t { body_statements2 } );
	}

	else{
		quark::throw_runtime_error("Illegal statement.");
	}
}

const std::vector<statement_t> ast_json_to_statements(type_interner_t& interner, const json_t& p){
	QUARK_ASSERT(interner.check_invariant());
	QUARK_ASSERT(p.check_invariant());
	QUARK_ASSERT(p.is_array());

	std::vector<statement_t> statements2;
	for(const auto& statement: p.get_array()){
		const auto s2 = ast_json_to_statement(interner, statement);
		statements2.push_back(s2);
	}
	return statements2;
}

json_t statement_to_json(const type_interner_t& interner, const statement_t& e){
	QUARK_ASSERT(e.check_invariant());

	struct visitor_t {
		const type_interner_t& interner;
		const statement_t& statement;

		json_t operator()(const statement_t::return_statement_t& s) const{
			return make_ast_node(k_no_location, statement_opcode_t::k_return, { expression_to_json(interner, s._expression) });
		}

		json_t operator()(const statement_t::bind_local_t& s) const{
			bool mutable_flag = s._locals_mutable_mode == statement_t::bind_local_t::k_mutable;
			const auto meta = mutable_flag
				? json_t::make_object({
					std::pair<std::string, json_t>{"mutable", mutable_flag}
				})
				: json_t();

			return make_ast_node(
				statement.location,
				statement_opcode_t::k_init_local,
				{
					s._new_local_name,
					itype_to_json(interner, s._bindtype),
					expression_to_json(interner, s._expression),
					meta
				}
			);
		}
		json_t operator()(const statement_t::assign_t& s) const{
			return make_ast_node(
				statement.location,
				statement_opcode_t::k_assign,
				{
					s._local_name,
					expression_to_json(interner, s._expression)
				}
			);
		}
		json_t operator()(const statement_t::assign2_t& s) const{
			return make_ast_node(
				statement.location,
				statement_opcode_t::k_assign2,
				{
					s._dest_variable._parent_steps,
					s._dest_variable._index,
					expression_to_json(interner, s._expression)
				}
			);
		}
		json_t operator()(const statement_t::init2_t& s) const{
			return make_ast_node(
				statement.location,
				statement_opcode_t::k_init_local2,
				{
					s._dest_variable._parent_steps,
					s._dest_variable._index,
					expression_to_json(interner, s._expression)
				}
			);
		}
		json_t operator()(const statement_t::block_statement_t& s) const{
			return make_ast_node(k_no_location, statement_opcode_t::k_block, { body_to_json(interner, s._body) } );
		}

		json_t operator()(const statement_t::ifelse_statement_t& s) const{
			return make_ast_node(
				statement.location,
				statement_opcode_t::k_if,
				{
					expression_to_json(interner, s._condition),
					body_to_json(interner, s._then_body),
					body_to_json(interner, s._else_body)
				}
			);
		}
		json_t operator()(const statement_t::for_statement_t& s) const{
			return make_ast_node(
				statement.location,
				statement_opcode_t::k_for,
				{
					s._range_type == statement_t::for_statement_t::k_open_range ? json_t("open_range") : json_t("closed_range"),
					expression_to_json(interner, s._start_expression),
					expression_to_json(interner, s._end_expression),
					body_to_json(interner, s._body)
				}
			);
		}
		json_t operator()(const statement_t::while_statement_t& s) const{
			return make_ast_node(
				statement.location,
				statement_opcode_t::k_while,
				{
					expression_to_json(interner, s._condition),
					body_to_json(interner, s._body)
				}
			);
		}


		json_t operator()(const statement_t::expression_statement_t& s) const{
			return make_ast_node(
				statement.location,
				statement_opcode_t::k_expression_statement,
				{ expression_to_json(interner, s._expression) }
			);
		}
		json_t operator()(const statement_t::software_system_statement_t& s) const{
			return make_ast_node(
				statement.location,
				statement_opcode_t::k_software_system_def,
				{ s._json_data }
			);
		}
		json_t operator()(const statement_t::container_def_statement_t& s) const{
			return make_ast_node(
				statement.location,
				statement_opcode_t::k_container_def,
				{ s._json_data }
			);
		}
		json_t operator()(const statement_t::benchmark_def_statement_t& s) const{
			return make_ast_node(
				statement.location,
				statement_opcode_t::k_benchmark_def,
				{ s.name, body_to_json(interner, s._body) }
			);
		}
	};

	return std::visit(visitor_t{ interner, e }, e._contents);
}




QUARK_TEST("", "", "", ""){
	const auto a = statement_t::make__block_statement(k_no_location, {});
	const auto b = statement_t::make__block_statement(k_no_location, {});
	QUARK_UT_VERIFY(a == b);
}


}	// floyd
