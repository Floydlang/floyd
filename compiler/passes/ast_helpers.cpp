//
//  ast_helpers.cpp
//  floyd
//
//  Created by Marcus Zetterquist on 2019-07-30.
//  Copyright © 2019 Marcus Zetterquist. All rights reserved.
//

#include "ast_helpers.h"

#include "ast.h"
#include "expression.h"
#include "statement.h"
#include "types.h"



namespace floyd {




bool check_types_resolved(const types_t& types, const expression_t& e){
	QUARK_ASSERT(types.check_invariant());
	QUARK_ASSERT(e.check_invariant());

	if(e.is_annotated_shallow() == false){
		return false;
	}

	const auto output_type = e.get_output_type();
	if(check_types_resolved(types, output_type) == false){
		return false;
	}


	struct visitor_t {
		const types_t& types;


		bool operator()(const expression_t::literal_exp_t& e) const{
			return check_types_resolved(types, e.value.get_type());
		}
		bool operator()(const expression_t::arithmetic_t& e) const{
			return check_types_resolved(types, *e.lhs) && check_types_resolved(types, *e.rhs);
		}
		bool operator()(const expression_t::comparison_t& e) const{
			return check_types_resolved(types, *e.lhs) && check_types_resolved(types, *e.rhs);
		}
		bool operator()(const expression_t::unary_minus_t& e) const{
			return check_types_resolved(types, *e.expr);
		}
		bool operator()(const expression_t::conditional_t& e) const{
			return check_types_resolved(types, *e.condition) && check_types_resolved(types, *e.a) && check_types_resolved(types, *e.b);
		}

		bool operator()(const expression_t::call_t& e) const{
			return check_types_resolved(types, *e.callee) && check_types_resolved(types, e.args);
		}
		bool operator()(const expression_t::intrinsic_t& e) const{
			return check_types_resolved(types, e.args);
		}


		bool operator()(const expression_t::struct_definition_expr_t& e) const{
			return check_types_resolved(types, *e.def);
		}
		bool operator()(const expression_t::function_definition_expr_t& e) const{
			return check_types_resolved(types, e.def);
		}
		bool operator()(const expression_t::load_t& e) const{
			return false;
		}
		bool operator()(const expression_t::load2_t& e) const{
			return true;
		}

		bool operator()(const expression_t::resolve_member_t& e) const{
			return check_types_resolved(types, *e.parent_address);
		}
		bool operator()(const expression_t::update_member_t& e) const{
			return check_types_resolved(types, *e.parent_address);
		}
		bool operator()(const expression_t::lookup_t& e) const{
			return check_types_resolved(types, *e.parent_address) && check_types_resolved(types, *e.lookup_key);
		}
		bool operator()(const expression_t::value_constructor_t& e) const{
			if(check_types_resolved(types, e.value_type) == false){
				return false;
			}
			return check_types_resolved(types, e.elements);
		}
		bool operator()(const expression_t::benchmark_expr_t& e) const{
			return check_types_resolved(types, *e.body);
		}
	};

	bool result = std::visit(visitor_t{ types }, e._expression_variant);
	if(result == false){
		return false;
	}

	if(check_types_resolved(types, e.get_output_type()) == false){
		return false;
	}

	return result;
}

bool check_types_resolved(const types_t& types, const std::vector<expression_t>& expressions){
	QUARK_ASSERT(types.check_invariant());

	for(const auto& e: expressions){
		if(floyd::check_types_resolved(types, e) == false){
			return false;
		}
	}
	return true;
}


bool check_types_resolved(const types_t& types, const function_definition_t& def){
	QUARK_ASSERT(types.check_invariant());
	QUARK_ASSERT(def.check_invariant());

	bool result = floyd::check_types_resolved(types, def._function_type);
	if(result == false){
		return false;
	}

	for(const auto& e: def._named_args){
		bool result2 = floyd::check_types_resolved(types, e._type);
		if(result2 == false){
			return false;
		}
	}

	if(def._optional_body){
		if(check_types_resolved(types, *def._optional_body) == false){
			return false;
		}
	}

	return true;
}

bool check_types_resolved(const types_t& types, const body_t& body){
	QUARK_ASSERT(types.check_invariant());
	QUARK_ASSERT(body.check_invariant());

	for(const auto& e: body._statements){
		if(check_types_resolved(types, e) == false){
			return false;
		}
	}
	for(const auto& s: body._symbol_table._symbols){
		QUARK_ASSERT(s.second.check_invariant());

		const auto value_type = s.second._value_type;

/*
		if(s._symbol_type == symbol_t::symbol_type::immutable_reserve){
			QUARK_ASSERT(is_empty(_value_type) == false);
			QUARK_ASSERT(_init.is_undefined());
		}
		else if(s._symbol_type == symbol_t::symbol_type::immutable_arg){
			QUARK_ASSERT(is_empty(_value_type) == false);
			QUARK_ASSERT(_init.is_undefined());
		}
		else if(s._symbol_type == symbol_t::symbol_type::immutable_precalc){
			QUARK_ASSERT(is_empty(_value_type) == false);
			QUARK_ASSERT(_init.is_undefined() == false);
		}
		else if(s._symbol_type == symbol_t::symbol_type::named_type){
			QUARK_ASSERT(is_empty(_value_type) == false);
			QUARK_ASSERT(_init.is_undefined());
		}
		else if(s._symbol_type == symbol_t::symbol_type::mutable_reserve){
			QUARK_ASSERT(is_empty(_value_type) == false);
			QUARK_ASSERT(_init.is_undefined());
		}
		else{
			QUARK_ASSERT(false);
		}
*/

		if(value_type.is_undefined() == false && check_types_resolved(types, value_type) == false){
			return false;
		}

		if(s.first != base_type_to_opcode(base_type::k_undefined) && s.second._init.is_undefined() == false && check_types_resolved(types, s.second._init.get_type()) == false){
			return false;
		}
	}
	return true;
}





bool check_types_resolved(const types_t& types, const std::vector<std::shared_ptr<statement_t>>& s){
	QUARK_ASSERT(types.check_invariant());

	for(const auto& e: s){
		if(floyd::check_types_resolved(types, *e) == false){
			return false;
		}
	}
	return true;
}

bool check_types_resolved(const types_t& types, const statement_t& s){
	QUARK_ASSERT(types.check_invariant());
	QUARK_ASSERT(s.check_invariant());

	struct visitor_t {
		const types_t& types;


		bool operator()(const statement_t::return_statement_t& s) const{
			return floyd::check_types_resolved(types, s._expression);
		}

		bool operator()(const statement_t::bind_local_t& s) const{
			return true
				&& check_types_resolved(types, s._bindtype)
				&& floyd::check_types_resolved(types, s._expression)
				;
		}
		bool operator()(const statement_t::assign_t& s) const{
			return floyd::check_types_resolved(types, s._expression);
		}
		bool operator()(const statement_t::assign2_t& s) const{
			return floyd::check_types_resolved(types, s._expression);
		}
		bool operator()(const statement_t::init2_t& s) const{
			return floyd::check_types_resolved(types, s._expression);
		}
		bool operator()(const statement_t::block_statement_t& s) const{
			return floyd::check_types_resolved(types, s._body);
		}

		bool operator()(const statement_t::ifelse_statement_t& s) const{
			return true
				&& floyd::check_types_resolved(types, s._condition)
				&& floyd::check_types_resolved(types, s._then_body)
				&& floyd::check_types_resolved(types, s._else_body)
				;
		}
		bool operator()(const statement_t::for_statement_t& s) const{
			return true
				&& floyd::check_types_resolved(types, s._start_expression)
				&& floyd::check_types_resolved(types, s._end_expression)
				&& floyd::check_types_resolved(types, s._body)
				;
		}
		bool operator()(const statement_t::while_statement_t& s) const{
			return true
				&& floyd::check_types_resolved(types, s._condition)
				&&floyd:: check_types_resolved(types, s._body)
				;
		}

		bool operator()(const statement_t::expression_statement_t& s) const{
			return floyd::check_types_resolved(types, s._expression);
		}
		bool operator()(const statement_t::software_system_statement_t& s) const{
			return true;
		}
		bool operator()(const statement_t::container_def_statement_t& s) const{
			return true;
		}
		bool operator()(const statement_t::benchmark_def_statement_t& s) const{
			return check_types_resolved(types, s._body);
		}
	};

	return std::visit(visitor_t { types }, s._contents);
}


bool check_types_resolved(const types_t& types, const struct_type_desc_t& s){
	QUARK_ASSERT(s.check_invariant());

	for(const auto& e: s._members){
		bool result = check_types_resolved(types, e._type);
		if(result == false){
			return false;
		}
	}
	return true;
}


bool check_types_resolved__type_vector(const types_t& types, const std::vector<type_t>& elements){
	QUARK_ASSERT(types.check_invariant());

	for(const auto& e: elements){
		if(check_types_resolved(types, e) == false){
			return false;
		}
	}
	return true;
}

bool check_types_resolved(const types_t& types, const type_t& t){
	QUARK_ASSERT(types.check_invariant());
	QUARK_ASSERT(t.check_invariant());

	struct visitor_t {
		const types_t types;


		bool operator()(const undefined_t& e) const{
			return false;
		}
		bool operator()(const any_t& e) const{
			return true;
		}

		bool operator()(const void_t& e) const{
			return true;
		}
		bool operator()(const bool_t& e) const{
			return true;
		}
		bool operator()(const int_t& e) const{
			return true;
		}
		bool operator()(const double_t& e) const{
			return true;
		}
		bool operator()(const string_t& e) const{
			return true;
		}

		bool operator()(const json_type_t& e) const{
			return true;
		}
		bool operator()(const typeid_type_t& e) const{
			return true;
		}

		bool operator()(const struct_t& e) const{
			return check_types_resolved(types, e.desc);
		}
		bool operator()(const vector_t& e) const{
			return check_types_resolved__type_vector(types, e._parts);
		}
		bool operator()(const dict_t& e) const{
			return check_types_resolved__type_vector(types, e._parts);
		}
		bool operator()(const function_t& e) const{
			return check_types_resolved__type_vector(types, e._parts);
		}
		bool operator()(const symbol_ref_t& e) const {
			return true;
		}
		bool operator()(const named_type_t& e) const {
			return true;
		}
	};
	return std::visit(visitor_t { types }, get_type_variant(types, t));
}


bool check_types_resolved(const types_t& types){
	QUARK_ASSERT(types.check_invariant());

	for(auto index = 0 ; index < types.nodes.size() ; index++){
	}

	return true;
}

bool check_types_resolved(const general_purpose_ast_t& ast){
	QUARK_ASSERT(ast.check_invariant());

	if(check_types_resolved(ast._types) == false){
		return false;
	}

	if(check_types_resolved(ast._types, ast._globals) == false){
		return false;
	}
	for(const auto& e: ast._function_defs){
		const auto result = check_types_resolved(ast._types, e);
		if(result == false){
			return false;
		}
	}
	return true;
}


}	//	floyd
