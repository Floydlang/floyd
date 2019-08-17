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



namespace floyd {


bool check_types_resolved(const expression_t& e){
	QUARK_ASSERT(e.check_invariant());

	if(e.is_annotated_shallow() == false){
		return false;
	}

	const auto output_type = e.get_output_type();
	if(check_types_resolved(output_type) == false){
		return false;
	}


	struct visitor_t {
		bool operator()(const expression_t::literal_exp_t& e) const{
			return check_types_resolved(e.value.get_type());
		}
		bool operator()(const expression_t::arithmetic_t& e) const{
			return check_types_resolved(*e.lhs) && check_types_resolved(*e.rhs);
		}
		bool operator()(const expression_t::comparison_t& e) const{
			return check_types_resolved(*e.lhs) && check_types_resolved(*e.rhs);
		}
		bool operator()(const expression_t::unary_minus_t& e) const{
			return check_types_resolved(*e.expr);
		}
		bool operator()(const expression_t::conditional_t& e) const{
			return check_types_resolved(*e.condition) && check_types_resolved(*e.a) && check_types_resolved(*e.b);
		}

		bool operator()(const expression_t::call_t& e) const{
			return check_types_resolved(*e.callee) && check_types_resolved(e.args);
		}
		bool operator()(const expression_t::corecall_t& e) const{
			return check_types_resolved(e.args);
		}


		bool operator()(const expression_t::struct_definition_expr_t& e) const{
			return check_types_resolved(*e.def);
		}
		bool operator()(const expression_t::function_definition_expr_t& e) const{
			return check_types_resolved(e.def);
		}
		bool operator()(const expression_t::load_t& e) const{
			return false;
		}
		bool operator()(const expression_t::load2_t& e) const{
			return true;
		}

		bool operator()(const expression_t::resolve_member_t& e) const{
			return check_types_resolved(*e.parent_address);
		}
		bool operator()(const expression_t::update_member_t& e) const{
			return check_types_resolved(*e.parent_address);
		}
		bool operator()(const expression_t::lookup_t& e) const{
			return check_types_resolved(*e.parent_address) && check_types_resolved(*e.lookup_key);
		}
		bool operator()(const expression_t::value_constructor_t& e) const{
			return check_types_resolved(e.elements);
		}
		bool operator()(const expression_t::benchmark_expr_t& e) const{
			return check_types_resolved(*e.body);
		}
	};

	bool result = std::visit(visitor_t{}, e._expression_variant);
	return result;
}

bool check_types_resolved(const std::vector<expression_t>& expressions){
	for(const auto& e: expressions){
		if(floyd::check_types_resolved(e) == false){
			return false;
		}
	}
	return true;
}


bool check_types_resolved(const function_definition_t& def){
	QUARK_ASSERT(def.check_invariant());

	bool result = floyd::check_types_resolved(def._function_type);
	if(result == false){
		return false;
	}

	for(const auto& e: def._named_args){
		bool result2 = floyd::check_types_resolved(e._type);
		if(result2 == false){
			return false;
		}
	}

	if(def._optional_body){
		if(check_types_resolved(*def._optional_body) == false){
			return false;
		}
	}

	return true;
}

bool check_types_resolved(const body_t& body){
	QUARK_ASSERT(body.check_invariant());

	for(const auto& e: body._statements){
		if(check_types_resolved(e) == false){
			return false;
		}
	}
	for(const auto& s: body._symbol_table._symbols){
		if(s.first != "undef" && check_types_resolved(s.second._value_type) == false){
			return false;
		}
		if(s.first != "undef" && s.second._init.is_undefined() == false && check_types_resolved(s.second._init.get_type()) == false){
			return false;
		}
	}
	return true;
}





bool check_types_resolved(const std::vector<std::shared_ptr<statement_t>>& s){
	for(const auto& e: s){
		if(floyd::check_types_resolved(*e) == false){
			return false;
		}
	}
	return true;
}

bool check_types_resolved(const statement_t& s){
	QUARK_ASSERT(s.check_invariant());

	struct visitor_t {
		bool operator()(const statement_t::return_statement_t& s) const{
			return floyd::check_types_resolved(s._expression);
		}

		bool operator()(const statement_t::bind_local_t& s) const{
			return true
				&& check_types_resolved(s._bindtype)
				&& floyd::check_types_resolved(s._expression)
				;
		}
		bool operator()(const statement_t::assign_t& s) const{
			return floyd::check_types_resolved(s._expression);
		}
		bool operator()(const statement_t::assign2_t& s) const{
			return floyd::check_types_resolved(s._expression);
		}
		bool operator()(const statement_t::init2_t& s) const{
			return floyd::check_types_resolved(s._expression);
		}
		bool operator()(const statement_t::block_statement_t& s) const{
			return floyd::check_types_resolved(s._body);
		}

		bool operator()(const statement_t::ifelse_statement_t& s) const{
			return true
				&& floyd::check_types_resolved(s._condition)
				&& floyd::check_types_resolved(s._then_body)
				&& floyd::check_types_resolved(s._else_body)
				;
		}
		bool operator()(const statement_t::for_statement_t& s) const{
			return true
				&& floyd::check_types_resolved(s._start_expression)
				&& floyd::check_types_resolved(s._end_expression)
				&& floyd::check_types_resolved(s._body)
				;
		}
		bool operator()(const statement_t::while_statement_t& s) const{
			return true
				&& floyd::check_types_resolved(s._condition)
				&&floyd:: check_types_resolved(s._body)
				;
		}

		bool operator()(const statement_t::expression_statement_t& s) const{
			return floyd::check_types_resolved(s._expression);
		}
		bool operator()(const statement_t::software_system_statement_t& s) const{
			return true;
		}
		bool operator()(const statement_t::container_def_statement_t& s) const{
			return true;
		}
		bool operator()(const statement_t::benchmark_def_statement_t& s) const{
			return check_types_resolved(s._body);
		}
	};

	return std::visit(visitor_t {}, s._contents);
}


bool check_types_resolved(const struct_definition_t& s){
	QUARK_ASSERT(s.check_invariant());

	for(const auto& e: s._members){
		bool result = check_types_resolved(e._type);
		if(result == false){
			return false;
		}
	}
	return true;
}


bool check_types_resolved_int(const std::vector<typeid_t>& elements){
	for(const auto& e: elements){
		if(check_types_resolved(e) == false){
			return false;
		}
	}
	return true;
}

bool check_types_resolved(const typeid_t& t){
	QUARK_ASSERT(t.check_invariant());

	struct visitor_t {
		bool operator()(const typeid_t::undefined_t& e) const{
			return false;
		}
		bool operator()(const typeid_t::any_t& e) const{
			return true;
		}

		bool operator()(const typeid_t::void_t& e) const{
			return true;
		}
		bool operator()(const typeid_t::bool_t& e) const{
			return true;
		}
		bool operator()(const typeid_t::int_t& e) const{
			return true;
		}
		bool operator()(const typeid_t::double_t& e) const{
			return true;
		}
		bool operator()(const typeid_t::string_t& e) const{
			return true;
		}

		bool operator()(const typeid_t::json_type_t& e) const{
			return true;
		}
		bool operator()(const typeid_t::typeid_type_t& e) const{
			return true;
		}

		bool operator()(const typeid_t::struct_t& e) const{
			return check_types_resolved(*e._struct_def);
		}
		bool operator()(const typeid_t::vector_t& e) const{
			return check_types_resolved_int(e._parts);
		}
		bool operator()(const typeid_t::dict_t& e) const{
			return check_types_resolved_int(e._parts);
		}
		bool operator()(const typeid_t::function_t& e) const{
			return check_types_resolved_int(e._parts);
		}
		bool operator()(const typeid_t::unresolved_t& e) const{
			return false;
		}
	};
	return std::visit(visitor_t{}, t._contents);
}


bool check_types_resolved(const general_purpose_ast_t& ast){
	if(check_types_resolved(ast._globals) == false){
		return false;
	}
	for(const auto& e: ast._function_defs){
		const auto result = check_types_resolved(e);
		if(result == false){
			return false;
		}
	}
	return true;
}


}	//	floyd
