//
//  ast_helpers.cpp
//  floyd
//
//  Created by Marcus Zetterquist on 2019-07-30.
//  Copyright © 2019 Marcus Zetterquist. All rights reserved.
//

#include "ast_helpers.h"

#include "expression.h"
#include "statement.h"



namespace floyd {


bool check_types_resolved(const expression_t& e){
	QUARK_ASSERT(e.check_invariant());

	if(e.is_annotated_shallow() == false){
		return false;
	}

	const auto output_type = e.get_output_type();
	if(output_type.check_types_resolved() == false){
		return false;
	}


	struct visitor_t {
		bool operator()(const expression_t::literal_exp_t& e) const{
			return e.value.get_type().check_types_resolved();
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
			return check_types_resolved(*e.callee) && expression_t::check_types_resolved(e.args);
		}
		bool operator()(const expression_t::corecall_t& e) const{
			return expression_t::check_types_resolved(e.args);
		}


		bool operator()(const expression_t::struct_definition_expr_t& e) const{
			return e.def->check_types_resolved();
		}
		bool operator()(const expression_t::function_definition_expr_t& e) const{
			return e.def->check_types_resolved();
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
			return expression_t::check_types_resolved(e.elements);
		}
		bool operator()(const expression_t::benchmark_expr_t& e) const{
			return e.body->check_types_resolved();
		}
	};

	bool result = std::visit(visitor_t{}, e._expression_variant);
	return result;
}





}	//	floyd
