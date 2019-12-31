//
//  parser_statement.h
//  Floyd
//
//  Created by Marcus Zetterquist on 26/07/16.
//  Copyright © 2016 Marcus Zetterquist. All rights reserved.
//

#ifndef parser_statement_hpp
#define parser_statement_hpp

#include "expression.h"
#include "quark.h"

#include <vector>
#include <string>
//#include "../parts/xcode-libcxx-xcode9/variant"	//	https://github.com/youknowone/xcode-libcxx
#include <variant>


namespace floyd {

#define DEBUG_STATEMENT_DEBUG_STRING 0

struct statement_t;
struct expression_t;

json_t statement_to_json(const types_t& types, const statement_t& e);


////////////////////////////////////////		statement_opcode_t



//	String keys used to specify statement type inside the AST JSON.

namespace statement_opcode_t {
	const std::string k_return = "return";

	const std::string k_init_local = "init-local";
	const std::string k_init_local2 = "init-local2";
	const std::string k_assign = "assign";
	const std::string k_assign2 = "assign2";
	const std::string k_block = "block";


	const std::string k_if = "if";
	const std::string k_for = "for";
	const std::string k_while = "while";

	const std::string k_expression_statement = "expression-statement";

	const std::string k_software_system_def = "software-system-def";
	const std::string k_container_def = "container-def";
	const std::string k_benchmark_def = "benchmark-def";
	const std::string k_test_def = "test-def";
};




//////////////////////////////////////		symbol_t

/*
	This is an entry in the symbol table.
	There is one symbol table for globals and one for each stack frame, like function body, inside an if-body etc.

	Variable slot: When you make a local variable it gets an entry in symbol table, with a type and name but no value. Like a reservered slot.
	Preinit: You can also add precalculated constants directly to the symbol table. Only works for some basic types right now.
*/

struct symbol_t {
	enum class symbol_type {
		immutable_reserve,
		immutable_arg,
		immutable_precalc,
		named_type,
		mutable_reserve
	};

	bool operator==(const symbol_t& other) const {
		return true
			&& _symbol_type == other._symbol_type
			&& _value_type == other._value_type
			&& _init == other._init
			;
	}

	public: bool check_invariant() const {
		if(_symbol_type == symbol_type::immutable_reserve){
//			QUARK_ASSERT(is_empty(_value_type) == false);
			QUARK_ASSERT(_init.is_undefined());
		}
		else if(_symbol_type == symbol_type::immutable_arg){
//			QUARK_ASSERT(is_empty(_value_type) == false);
			QUARK_ASSERT(_init.is_undefined());
		}
		else if(_symbol_type == symbol_type::immutable_precalc){
//			QUARK_ASSERT(is_empty(_value_type) == false);
			QUARK_ASSERT(_init.is_undefined() == false);
		}
		else if(_symbol_type == symbol_type::named_type){
//			QUARK_ASSERT(is_empty(_value_type) == false);
			QUARK_ASSERT(_init.is_undefined());
		}
		else if(_symbol_type == symbol_type::mutable_reserve){
//			QUARK_ASSERT(is_empty(_value_type) == false);
			QUARK_ASSERT(_init.is_undefined());
		}
		else{
			quark::throw_defective_request();
		}
		return true;
	}

	public: symbol_t(symbol_type symbol_type, const type_t& value_type, const value_t& init_value) :
		_symbol_type(symbol_type),
		_value_type(value_type),
		_init(init_value)
	{
		QUARK_ASSERT(check_invariant());
	}

	public: type_t get_value_type() const {
		QUARK_ASSERT(check_invariant());

		return _value_type;
	}

	public: static symbol_t make_immutable_reserve(const type_t& value_type){
		return symbol_t{ symbol_type::immutable_reserve, value_type, {} };
	}

	public: static symbol_t make_immutable_arg(const type_t& value_type){
		return symbol_t{ symbol_type::immutable_arg, value_type, {} };
	}

	//??? Mutable could support init-value too!?
	public: static symbol_t make_mutable(const type_t& value_type){
		return symbol_t{ symbol_type::mutable_reserve, value_type, {} };
	}

	public: static symbol_t make_immutable_precalc(const type_t& value_type, const value_t& init_value){
//		QUARK_ASSERT(is_floyd_literal(init_value.get_type()));

		return symbol_t{ symbol_type::immutable_precalc, value_type, init_value };
	}


	public: static symbol_t make_named_type(const type_t& name){
		return symbol_t{ symbol_type::named_type, name, value_t::make_undefined() };
	}



	//////////////////////////////////////		STATE
	symbol_type _symbol_type;
	type_t _value_type;

	//	If there is no initialization value, this member must be value_t::make_undefined();
	value_t _init;
};

inline bool is_mutable(const symbol_t& s){
	return s._symbol_type == symbol_t::symbol_type::mutable_reserve;
}


std::string symbol_to_string(const types_t& types, const symbol_t& symbol);



//////////////////////////////////////		symbol_table_t

//
struct symbol_table_t {
	bool check_invariant() const {
		for(const auto& e: _symbols){
			QUARK_ASSERT(e.first.size() > 0 && e.first.size() < 10000);
			QUARK_ASSERT(e.second.check_invariant());
		}
		return true;
	}

	bool operator==(const symbol_table_t& other) const {
		return _symbols == other._symbols;
	}


	public: std::vector<std::pair<std::string, symbol_t>> _symbols;
};

const symbol_t* find_symbol(const symbol_table_t& symbol_table, const std::string& name);
const symbol_t& find_symbol_required(const symbol_table_t& symbol_table, const std::string& name);

std::vector<json_t> symbols_to_json(const types_t& types, const symbol_table_t& symbols);
symbol_table_t ast_json_to_symbols(types_t& types, const json_t& p);




//////////////////////////////////////		lexical_scope_t

/*
	Lexical scope, aka static scope, aka body.

	We use "lexical scope" for what it IS. And "body" for what it's used for.

	As in "the function body is optional and is its own lexical scope."
*/

struct lexical_scope_t {
	lexical_scope_t(){
	}

	lexical_scope_t(const std::vector<statement_t>& s) :
		_statements(s),
		_symbol_table{}
	{
	}

	lexical_scope_t(const std::vector<statement_t>& statements, const symbol_table_t& symbols) :
		_statements(statements),
		_symbol_table(symbols)
	{
	}

	bool check_invariant() const;


	////////////////////		STATE
	std::vector<statement_t> _statements;
	symbol_table_t _symbol_table;
};

bool operator==(const lexical_scope_t& lhs, const lexical_scope_t& rhs);


json_t scope_to_json(const types_t& types, const lexical_scope_t& e);
lexical_scope_t json_to_scope(types_t& types, const json_t& json);





//////////////////////////////////////		statement_t


/*
	Defines a statement, like "return" including any needed expression trees for the statement.
	Immutable
*/
struct statement_t {

	//////////////////////////////////////		return_statement_t



	struct return_statement_t {
		bool operator==(const return_statement_t& other) const {
			return _expression == other._expression;
		}

		expression_t _expression;
	};
	public: static statement_t make__return_statement(const location_t& location, const expression_t& expression){
		return statement_t(location, { return_statement_t{ expression } });
	}


	//////////////////////////////////////		bind_local_t

	//	Created a new name in current lexical scope and initialises it with an expression.

	struct bind_local_t {
		enum mutable_mode {
			k_mutable = 2,
			k_immutable
		};

		bool operator==(const bind_local_t& other) const {
			return _new_local_name == other._new_local_name
				&& _bindtype == other._bindtype
				&& _expression == other._expression
				&& _locals_mutable_mode == other._locals_mutable_mode;
		}

		std::string _new_local_name;
		type_t _bindtype;
		expression_t _expression;
		mutable_mode _locals_mutable_mode;
	};
	public: static statement_t make__bind_local(const location_t& location, const std::string& new_local_name, const type_t& bindtype, const expression_t& expression, bind_local_t::mutable_mode locals_mutable_mode){
		return statement_t(location, { bind_local_t{ new_local_name, bindtype, expression, locals_mutable_mode } });
	}


	//////////////////////////////////////		assign_t

	//	Mutate an existing variable, specified by name.

	struct assign_t {
		bool operator==(const assign_t& other) const {
			return _local_name == other._local_name
				&& _expression == other._expression;
		}

		std::string _local_name;
		expression_t _expression;
	};
	public: static statement_t make__assign(const location_t& location, const std::string& local_name, const expression_t& expression){
		return statement_t(location, { assign_t{ local_name, expression} });
	}


	//////////////////////////////////////		assign2_t


	//	Mutate an existing variable, specified by resolved scope ID.

	struct assign2_t {
		bool operator==(const assign2_t& other) const {
			return _dest_variable == other._dest_variable
				&& _expression == other._expression;
		}

		symbol_pos_t _dest_variable;
		expression_t _expression;
	};

	public: static statement_t make__assign2(const location_t& location, const symbol_pos_t& dest_variable, const expression_t& expression){
		return statement_t(location, { assign2_t{ dest_variable, expression} });
	}


	//////////////////////////////////////		init2_t


	//	Initialise an existing variable, specified by resolved scope ID.

	struct init2_t {
		bool operator==(const init2_t& other) const {
			return _dest_variable == other._dest_variable
				&& _expression == other._expression;
		}

		symbol_pos_t _dest_variable;
		expression_t _expression;
	};

	public: static statement_t make__init2(const location_t& location, const symbol_pos_t& dest_variable, const expression_t& expression){
		return statement_t(location, { init2_t{ dest_variable, expression} });
	}


	//////////////////////////////////////		block_statement_t


	struct block_statement_t {
		bool operator==(const block_statement_t& other) const {
			return _body == other._body;
		}

		lexical_scope_t _body;
	};

	public: static statement_t make__block_statement(const location_t& location, const lexical_scope_t& body){
		return statement_t(location, { block_statement_t{ body} });
	}


	//////////////////////////////////////		ifelse_statement_t


	struct ifelse_statement_t {
		bool operator==(const ifelse_statement_t& other) const {
			return
				_condition == other._condition
				&& _condition == other._condition
				&& _then_body == other._then_body
				&& _else_body == other._else_body
				;
		}

		expression_t _condition;
		lexical_scope_t _then_body;
		lexical_scope_t _else_body;
	};
	public: static statement_t make__ifelse_statement(
		const location_t& location,
		const expression_t& condition,
		const lexical_scope_t& then_body,
		const lexical_scope_t& else_body
	){
		return statement_t(location, { ifelse_statement_t{ condition, then_body, else_body} });
	}


	//////////////////////////////////////		for_statement_t


	struct for_statement_t {
		enum range_type {
			k_open_range,	//	..<
			k_closed_range	//	...
		};

		bool operator==(const for_statement_t& other) const {
			return
				_iterator_name == other._iterator_name
				&& _start_expression == other._start_expression
				&& _end_expression == other._end_expression
				&& _body == other._body
				&& _range_type == other._range_type
				;
		}

		std::string _iterator_name;
		expression_t _start_expression;
		expression_t _end_expression;
		lexical_scope_t _body;
		range_type _range_type;
	};
	public: static statement_t make__for_statement(
		const location_t& location,
		const std::string iterator_name,
		const expression_t& start_expression,
		const expression_t& end_expression,
		const lexical_scope_t& body,
		for_statement_t::range_type range_type
	){
		return statement_t(location, { for_statement_t{ iterator_name, start_expression, end_expression, body, range_type } });
	}


	//////////////////////////////////////		software_system_statement_t


	struct software_system_statement_t {
		bool operator==(const software_system_statement_t& other) const {
			return _system == other._system;
		}

		software_system_t _system;
	};

	public: static statement_t make__software_system_statement(
		const location_t& location,
		software_system_t system
	){
		return statement_t(location, { software_system_statement_t{ system } });
	}


	//////////////////////////////////////		container_def_statement_t


	struct container_def_statement_t {
		bool operator==(const container_def_statement_t& other) const {
			return _container == other._container;
		}

		container_t _container;
	};

	public: static statement_t make__container_def_statement(
		const location_t& location,
		const container_t& container
	){
		return statement_t(location, { container_def_statement_t{ container } });
	}


	//////////////////////////////////////		benchmark_def_statement_t


	struct benchmark_def_statement_t {
		bool operator==(const benchmark_def_statement_t& other) const {
			return name == other.name && _body == other._body;
		}

		std::string name;
		lexical_scope_t _body;
	};

	public: static statement_t make__benchmark_def_statement(
		const location_t& location,
		const std::string& name,
		const lexical_scope_t& body
	){
		return statement_t(location, { benchmark_def_statement_t{ name, body } });
	}


	//////////////////////////////////////		test_def_statement_t


	struct test_def_statement_t {
		bool operator==(const test_def_statement_t& other) const {
			return
				function_name == other.function_name
				&& scenario == other.scenario
				&& _body == other._body
			;
		}

		std::string function_name;
		std::string scenario;
		lexical_scope_t _body;
	};

	public: static statement_t make__test_def_statement(
		const location_t& location,
		const std::string& function_name,
		const std::string& scenario,
		const lexical_scope_t& body
	){
		return statement_t(location, { test_def_statement_t{ function_name, scenario, body } });
	}


	//////////////////////////////////////		while_statement_t


	struct while_statement_t {
		bool operator==(const while_statement_t& other) const {
			return
				_condition == other._condition
				&& _body == other._body;
		}

		expression_t _condition;
		lexical_scope_t _body;
	};

	public: static statement_t make__while_statement(
		const location_t& location,
		const expression_t& condition,
		const lexical_scope_t& body
	){
		return statement_t(location, { while_statement_t{ condition, body } });
	}


	//////////////////////////////////////		expression_statement_t


	struct expression_statement_t {
		bool operator==(const expression_statement_t& other) const {
			return _expression == other._expression;
		}

		expression_t _expression;
	};
	public: static statement_t make__expression_statement(const location_t& location, const expression_t& expression){
		return statement_t(location, { expression_statement_t{ expression } });
	}


	//////////////////////////////////////		statement_t


	typedef std::variant<
		return_statement_t,
		bind_local_t,
		assign_t,
		assign2_t,
		init2_t,
		block_statement_t,
		ifelse_statement_t,
		for_statement_t,
		while_statement_t,
		expression_statement_t,
		software_system_statement_t,
		container_def_statement_t,
		benchmark_def_statement_t,
		test_def_statement_t
	> statement_variant_t;

	statement_t(/*const types_t& types,*/ const location_t& location, const statement_variant_t& contents) :
#if DEBUG_STATEMENT_DEBUG_STRING
		debug_string(""),
#endif
		location(location),
		_contents(contents)
	{
#if DEBUG_STATEMENT_DEBUG_STRING
		const auto json = statement_to_json(types, *this);
		debug_string = json_to_compact_string(json);
#endif
	}

	bool check_invariant() const {
		return true;
	}


	//////////////////////////////////////		STATE

#if DEBUG_STATEMENT_DEBUG_STRING
	std::string debug_string;
#endif
	location_t location;
	statement_variant_t _contents;
};

static bool operator==(const statement_t& lhs, const statement_t& rhs){
	return lhs.location == rhs.location && lhs._contents == rhs._contents;
}


const std::vector<statement_t> ast_json_to_statements(types_t& types, const json_t& p);
json_t statement_to_json(const types_t& types, const statement_t& e);



}	//	floyd


#endif /* parser_statement_hpp */
