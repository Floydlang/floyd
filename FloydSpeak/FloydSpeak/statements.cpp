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


namespace floyd_ast {

	using std::string;
	using std::vector;
	using std::pair;
	using std::shared_ptr;



	statement_t make__return_statement(const return_statement_t& value){
		return statement_t(value);
	}


	statement_t make__return_statement(const expression_t& expression){
		return statement_t(return_statement_t{ expression });
	}

	statement_t make__bind_statement(const std::string& new_variable_name, const typeid_t& bindtype, const expression_t& expression){
		return statement_t(bind_statement_t{ new_variable_name, bindtype, expression });
	}

	statement_t make__for_statement(const statement_t& init, const expression_t& condition, const expression_t& post_expression, int block_id){
		return statement_t(for_statement_t{ std::make_shared<statement_t>(init), condition, post_expression, block_id });
	}




	void trace(const statement_t& s){
		if(s._return){
			QUARK_SCOPED_TRACE("return_statement_t");
			trace(s._return->_expression);
		}
		else{
			QUARK_ASSERT(false);
		}
	}



	//////////////////////////////////////		statement_t



	bool statement_t::check_invariant() const {
		if(_return){
			QUARK_ASSERT(true);
		}
		else if(_bind){
			QUARK_ASSERT(true);
		}
		else if(_for){
			QUARK_ASSERT(true);
		}
		else{
			QUARK_ASSERT(false);
		}
		return true;
	}



	json_t statement_to_json(const statement_t& e){
		QUARK_ASSERT(e.check_invariant());

		if(e._return){
			return json_t::make_array({
				json_t("return"),
				expression_to_json(e._return->_expression)
			});
		}
		else if(e._bind){
			return json_t::make_array({
				json_t("bind"),
				e._bind->_new_variable_name,
				typeid_to_json(e._bind->_bindtype),
				expression_to_json(e._bind->_expression)
			});
		}
		else if(e._for){
			return json_t::make_array({
				json_t("for"),
				statement_to_json(*e._for->_init),
				expression_to_json(e._for->_condition),
				expression_to_json(e._for->_post_expression),
				json_t(double(e._for->_block_id))
			});
		}
		else{
			QUARK_ASSERT(false);
		}
	}


	QUARK_UNIT_TESTQ("statement_to_json", "return"){
		quark::ut_compare(
			json_to_compact_string(
				statement_to_json(make__return_statement(expression_t::make_constant_string("abc")))
			)
			,
			R"(["return", ["k", "abc", "string"]])"
		);
	}

}	//	floyd_ast
