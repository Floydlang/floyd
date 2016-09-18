//
//  parser_statement.cpp
//  FloydSpeak
//
//  Created by Marcus Zetterquist on 26/07/16.
//  Copyright © 2016 Marcus Zetterquist. All rights reserved.
//

#include "statements.h"

#include "parser_ast.h"
#include "parse_expression.h"
#include "parse_function_def.h"
#include "parse_struct_def.h"
#include "parser_primitives.h"
#include "json_support.h"
#include "json_writer.h"


namespace floyd_parser {

	using std::string;
	using std::vector;
	using std::pair;
	using std::make_shared;
	using std::shared_ptr;



	statement_t make__bind_statement(const std::shared_ptr<const type_def_t>& type, const std::string& identifier, const expression_t& e){
		QUARK_ASSERT(type && type->check_invariant());
		QUARK_ASSERT(identifier.size () > 0);
		QUARK_ASSERT(e.check_invariant());

		return statement_t(bind_statement_t{ type, identifier, std::make_shared<expression_t>(e) });
	}

	statement_t make__return_statement(const return_statement_t& value){
		return statement_t(value);
	}


	statement_t make__return_statement(const expression_t& expression){
		return statement_t(return_statement_t{make_shared<expression_t>(expression)});
	}

	void trace(const statement_t& s){
		if(s._bind_statement){
			std::string t = "bind_statement_t: \"" + s._bind_statement->_identifier + "\"";
			QUARK_SCOPED_TRACE(t);
			trace(*s._bind_statement->_expression);
		}

		else if(s._define_struct){
			QUARK_SCOPED_TRACE("define_struct_statement_t: \"" + s._define_struct->_struct_def->_name.to_string());
			trace(s._define_struct->_struct_def);
		}
		else if(s._define_function){
			QUARK_SCOPED_TRACE("define_function_statement_t: \"" + s._define_function->_function_def->_name.to_string());
			trace(s._define_function->_function_def);
		}

		else if(s._return_statement){
			QUARK_SCOPED_TRACE("return_statement_t");
			trace(*s._return_statement->_expression);
		}
		else{
			QUARK_ASSERT(false);
		}
	}



	//////////////////////////////////////		statement_t



	bool statement_t::check_invariant() const {
		if(_bind_statement){
			QUARK_ASSERT(_bind_statement);
			QUARK_ASSERT(!_define_struct);
			QUARK_ASSERT(!_define_function);
			QUARK_ASSERT(!_return_statement);
		}
		else if(_define_struct){
			QUARK_ASSERT(!_bind_statement);
			QUARK_ASSERT(_define_struct);
			QUARK_ASSERT(!_define_function);
			QUARK_ASSERT(!_return_statement);
		}
		else if(_define_function){
			QUARK_ASSERT(!_bind_statement);
			QUARK_ASSERT(!_define_struct);
			QUARK_ASSERT(_define_function);
			QUARK_ASSERT(!_return_statement);
		}
		else if(_return_statement){
			QUARK_ASSERT(!_bind_statement);
			QUARK_ASSERT(!_define_struct);
			QUARK_ASSERT(!_define_function);
			QUARK_ASSERT(_return_statement);
		}
		else{
			QUARK_ASSERT(false);
		}
		return true;
	}



	json_value_t statement_to_json(const statement_t& e){
		QUARK_ASSERT(e.check_invariant());

		if(e._bind_statement){
			return json_value_t::make_array2({
				json_value_t("bind"),
				json_value_t(e._bind_statement->_identifier),
				expression_to_json(*e._bind_statement->_expression)
			});
		}
		else if(e._define_struct){
			return json_value_t::make_array2({
				json_value_t("defstruct"),
				scope_def_to_json(*e._define_struct->_struct_def)
			});
		}
		else if(e._define_function){
			return json_value_t::make_array2({
				json_value_t("deffunc"),
				scope_def_to_json(*e._define_function->_function_def)
			});
		}
		else if(e._return_statement){
			return json_value_t::make_array2({
				json_value_t("return"),
				expression_to_json(*e._return_statement->_expression)
			});
		}
		else{
			QUARK_ASSERT(false);
		}
	}


	QUARK_UNIT_TESTQ("statement_to_json", "bind"){
		quark::ut_compare(
			json_to_compact_string(
				statement_to_json(
					make__bind_statement(
						make_shared<type_def_t>(type_def_t::make_int()),
						"a",
						expression_t::make_constant(400)
					)
				)
			)
			,
			R"(["bind", "a", ["k", 400, "<int>"]])"
		);
	}

#if false
	QUARK_UNIT_TESTQ("statement_to_json", "defstruct"){
		const auto global = scope_def_t::make_global_scope();
		const auto result = statement_to_json(define_struct_statement_t{ make_struct1(global) });

		quark::ut_compare(result.get_array_n(0).get_string(), "defstruct");
	}
#endif

#if false
	QUARK_UNIT_TESTQ("statement_to_json", "deffunc"){
		const auto global = scope_def_t::make_global_scope();
		const auto result = statement_to_json(define_function_statement_t{ make_test_function2() });

		quark::ut_compare(result.get_array_n(0).get_string(), "deffunc");
	}
#endif

	QUARK_UNIT_TESTQ("statement_to_json", "return"){
		quark::ut_compare(
			json_to_compact_string(
				statement_to_json(make__return_statement(expression_t::make_constant("abc")))
			)
			,
			R"(["return", ["k", "abc", "<string>"]])"
		);
	}

}	//	floyd_parser
