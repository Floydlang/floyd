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
			return check_types_resolved(*e.callee) && check_types_resolved(e.args);
		}
		bool operator()(const expression_t::corecall_t& e) const{
			return check_types_resolved(e.args);
		}


		bool operator()(const expression_t::struct_definition_expr_t& e) const{
			return e.def->check_types_resolved();
		}
		bool operator()(const expression_t::function_definition_expr_t& e) const{
			return check_types_resolved(*e.def);
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
			return e.body->check_types_resolved();
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
	bool result = def._function_type.check_types_resolved();
	if(result == false){
		return false;
	}

	for(const auto& e: def._args){
		bool result2 = e._type.check_types_resolved();
		if(result2 == false){
			return false;
		}
	}

	struct visitor_t {
		bool operator()(const function_definition_t::empty_t& e) const{
			return true;
		}
		bool operator()(const function_definition_t::floyd_func_t& e) const{
			if(e._body){
				return e._body->check_types_resolved();
			}
			else{
				return true;
			}
		}
		bool operator()(const function_definition_t::host_func_t& e) const{
			return true;
		}
	};
	bool result3 = std::visit(visitor_t{}, def._contents);
	if(result3 == false){
		return false;
	}
	return true;
}




}	//	floyd
