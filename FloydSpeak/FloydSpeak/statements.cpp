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
	statement_t make__block_statement(const std::vector<std::shared_ptr<statement_t>>& statements){
/*
		vector<shared_ptr<statement_t>> statements2;
		for(const auto e: statements){
			statements2.push_back(std::make_shared<statement_t>(e));
		}
*/
		return statement_t(block_statement_t{ statements });
	}

	statement_t make__ifelse_statement(
		const expression_t& condition,
		std::vector<std::shared_ptr<statement_t>> then_statements,
		std::vector<std::shared_ptr<statement_t>> else_statements
	){
		return statement_t(ifelse_statement_t{ condition, then_statements, else_statements });
	}


	statement_t make__for_statement(
		const std::vector<std::shared_ptr<statement_t>> init,
		const expression_t& condition,
		const expression_t& post,
		const std::vector<std::shared_ptr<statement_t>> body
	){
		return statement_t(for_statement_t{ init, condition, post, body });
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
		if(_return != nullptr){
			QUARK_ASSERT(_return != nullptr);
			QUARK_ASSERT(_bind == nullptr);
			QUARK_ASSERT(_block == nullptr);
			QUARK_ASSERT(_if == nullptr);
			QUARK_ASSERT(_for == nullptr);
		}
		else if(_bind){
			QUARK_ASSERT(_return == nullptr);
			QUARK_ASSERT(_bind != nullptr);
			QUARK_ASSERT(_block == nullptr);
			QUARK_ASSERT(_if == nullptr);
			QUARK_ASSERT(_for == nullptr);
		}
		else if(_block){
			QUARK_ASSERT(_return == nullptr);
			QUARK_ASSERT(_bind == nullptr);
			QUARK_ASSERT(_block != nullptr);
			QUARK_ASSERT(_if == nullptr);
			QUARK_ASSERT(_for == nullptr);
		}
		else if(_if){
			QUARK_ASSERT(_return == nullptr);
			QUARK_ASSERT(_bind == nullptr);
			QUARK_ASSERT(_block == nullptr);
			QUARK_ASSERT(_if != nullptr);
			QUARK_ASSERT(_for == nullptr);
		}
		else if(_for){
			QUARK_ASSERT(_return == nullptr);
			QUARK_ASSERT(_bind == nullptr);
			QUARK_ASSERT(_block == nullptr);
			QUARK_ASSERT(_if == nullptr);
			QUARK_ASSERT(_for != nullptr);
		}
		else{
			QUARK_ASSERT(false);
		}
		return true;
	}


	std::vector<json_t> statements_shared_to_json(const std::vector<std::shared_ptr<statement_t>>& a){
		vector<json_t> result;
		for(const auto& e: a){
			result.push_back(statement_to_json(*e));
		}
		return result;
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
		else if(e._block){
			return json_t::make_array({
				json_t("block"),
				json_t::make_array(statements_shared_to_json(e._block->_statements))
			});
		}
		else if(e._if){
			return json_t::make_array({
				json_t("if"),
				expression_to_json(e._if->_condition),
				json_t::make_array(statements_shared_to_json(e._if->_then_statements)),
				json_t::make_array(statements_shared_to_json(e._if->_else_statements))
			});
		}
		else if(e._for){
			return json_t::make_array({
				json_t("for"),
				statements_to_json(e._for->_init),
				expression_to_json(e._for->_condition),
				expression_to_json(e._for->_post),
				statements_to_json(e._for->_body)
			});
		}
		else{
			QUARK_ASSERT(false);
		}
	}

	json_t statements_to_json(const std::vector<std::shared_ptr<statement_t>>& e){
		std::vector<json_t> statements;
		for(const auto& i: e){
			statements.push_back(statement_to_json(*i));
		}
		return json_t::make_array(statements);
	}


	QUARK_UNIT_TESTQ("statement_to_json", "return"){
		quark::ut_compare(
			json_to_compact_string(
				statement_to_json(make__return_statement(expression_t::make_literal_string("abc")))
			)
			,
			R"(["return", ["k", "abc", "string"]])"
		);
	}


statement_t make_function_statement(const string name, const function_definition_t def){
	const auto function_typeid = typeid_t::make_function(def._return_type, get_member_types(def._args));
	const auto function_def_expr = expression_t::make_function_definition(def);
	return make__bind_statement(name, function_typeid, function_def_expr);
}


}	//	floyd_ast
