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


namespace floyd {

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

	statement_t make__define_struct_statement(const define_struct_statement_t& value){
		return statement_t(value);
	}

	statement_t make__bind_or_assign_statement(const std::string& new_variable_name, const typeid_t& bindtype, const expression_t& expression, bool bind_as_mutable_tag){
		return statement_t(bind_or_assign_statement_t{ new_variable_name, bindtype, expression, bind_as_mutable_tag });
	}
	statement_t make__block_statement(const std::vector<std::shared_ptr<statement_t>>& statements){
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
		const std::string iterator_name,
		const expression_t& start_expression,
		const expression_t& end_expression,
		const std::vector<std::shared_ptr<statement_t>> body
	){
		return statement_t(for_statement_t{ iterator_name, start_expression, end_expression, body });
	}

	statement_t make__while_statement(
		const expression_t& condition,
		const std::vector<std::shared_ptr<statement_t>> body
	){
		return statement_t(while_statement_t{ condition, body });
	}

	statement_t make__expression_statement(const expression_t& expression){
		return statement_t(expression_statement_t{ expression });
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
		int count = 0;
		count = count + (_return != nullptr ? 1 : 0);
		count = count + (_def_struct != nullptr ? 1 : 0);
		count = count + (_bind_or_assign != nullptr ? 1 : 0);
		count = count + (_block != nullptr ? 1 : 0);
		count = count + (_if != nullptr ? 1 : 0);
		count = count + (_for != nullptr ? 1 : 0);
		count = count + (_while != nullptr ? 1 : 0);
		count = count + (_expression != nullptr ? 1 : 0);
		QUARK_ASSERT(count == 1);

		if(_return != nullptr){
		}
		else if(_def_struct != nullptr){
		}
		else if(_bind_or_assign){
		}
		else if(_block){
		}
		else if(_if){
		}
		else if(_for){
		}
		else if(_while){
		}
		else if(_expression){
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
				json_t(keyword_t::k_return),
				expression_to_json(e._return->_expression)
			});
		}
		else if(e._def_struct){
			return json_t::make_array({
				json_t("def-struct"),
				json_t(e._def_struct->_name),
				typeid_to_normalized_json(e._def_struct->_def)
			});
		}
		else if(e._bind_or_assign){
			const auto meta = (e._bind_or_assign->_bind_as_mutable_tag) ? (json_t::make_object({pair<string,json_t>{"mutable", true}})) : json_t::make_object();

			return json_t::make_array({
				json_t("bind"),
				e._bind_or_assign->_new_variable_name,
				typeid_to_normalized_json(e._bind_or_assign->_bindtype),
				expression_to_json(e._bind_or_assign->_expression),
				meta
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
				json_t(keyword_t::k_if),
				expression_to_json(e._if->_condition),
				json_t::make_array(statements_shared_to_json(e._if->_then_statements)),
				json_t::make_array(statements_shared_to_json(e._if->_else_statements))
			});
		}
		else if(e._for){
			return json_t::make_array({
				json_t("for"),
				json_t("open_range"),
				expression_to_json(e._for->_start_expression),
				expression_to_json(e._for->_end_expression),
				statements_to_json(e._for->_body)
			});
		}
		else if(e._while){
			return json_t::make_array({
				json_t(keyword_t::k_while),
				expression_to_json(e._while->_condition),
				statements_to_json(e._while->_body)
			});
		}
		else if(e._expression){
			return json_t::make_array({
				json_t("expression-statement"),
				expression_to_json(e._expression->_expression)
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
	return make__bind_or_assign_statement(name, function_typeid, function_def_expr, false);
}


}	//	floyd
