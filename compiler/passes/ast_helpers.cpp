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




bool check_types_resolved(const type_interner_t& interner, const expression_t& e){
	QUARK_ASSERT(interner.check_invariant());
	QUARK_ASSERT(e.check_invariant());

	if(e.is_annotated_shallow() == false){
		return false;
	}

	const auto output_type = e.get_output_type();
	if(check_types_resolved(interner, output_type) == false){
		return false;
	}


	struct visitor_t {
		const type_interner_t& interner;


		bool operator()(const expression_t::literal_exp_t& e) const{
			return check_types_resolved(interner, e.value.get_type());
		}
		bool operator()(const expression_t::arithmetic_t& e) const{
			return check_types_resolved(interner, *e.lhs) && check_types_resolved(interner, *e.rhs);
		}
		bool operator()(const expression_t::comparison_t& e) const{
			return check_types_resolved(interner, *e.lhs) && check_types_resolved(interner, *e.rhs);
		}
		bool operator()(const expression_t::unary_minus_t& e) const{
			return check_types_resolved(interner, *e.expr);
		}
		bool operator()(const expression_t::conditional_t& e) const{
			return check_types_resolved(interner, *e.condition) && check_types_resolved(interner, *e.a) && check_types_resolved(interner, *e.b);
		}

		bool operator()(const expression_t::call_t& e) const{
			return check_types_resolved(interner, *e.callee) && check_types_resolved(interner, e.args);
		}
		bool operator()(const expression_t::intrinsic_t& e) const{
			return check_types_resolved(interner, e.args);
		}


		bool operator()(const expression_t::struct_definition_expr_t& e) const{
			return check_types_resolved(interner, *e.def);
		}
		bool operator()(const expression_t::function_definition_expr_t& e) const{
			return check_types_resolved(interner, e.def);
		}
		bool operator()(const expression_t::load_t& e) const{
			return false;
		}
		bool operator()(const expression_t::load2_t& e) const{
			return true;
		}

		bool operator()(const expression_t::resolve_member_t& e) const{
			return check_types_resolved(interner, *e.parent_address);
		}
		bool operator()(const expression_t::update_member_t& e) const{
			return check_types_resolved(interner, *e.parent_address);
		}
		bool operator()(const expression_t::lookup_t& e) const{
			return check_types_resolved(interner, *e.parent_address) && check_types_resolved(interner, *e.lookup_key);
		}
		bool operator()(const expression_t::value_constructor_t& e) const{
			if(check_types_resolved(interner, e.value_type) == false){
				return false;
			}
			return check_types_resolved(interner, e.elements);
		}
		bool operator()(const expression_t::benchmark_expr_t& e) const{
			return check_types_resolved(interner, *e.body);
		}
	};

	bool result = std::visit(visitor_t{ interner }, e._expression_variant);
	if(result == false){
		return false;
	}

	if(check_types_resolved(interner, e.get_output_type()) == false){
		return false;
	}

	return result;
}

bool check_types_resolved(const type_interner_t& interner, const std::vector<expression_t>& expressions){
	QUARK_ASSERT(interner.check_invariant());

	for(const auto& e: expressions){
		if(floyd::check_types_resolved(interner, e) == false){
			return false;
		}
	}
	return true;
}


bool check_types_resolved(const type_interner_t& interner, const function_definition_t& def){
	QUARK_ASSERT(interner.check_invariant());
	QUARK_ASSERT(def.check_invariant());

	bool result = floyd::check_types_resolved(interner, def._function_type);
	if(result == false){
		return false;
	}

	for(const auto& e: def._named_args){
		bool result2 = floyd::check_types_resolved(interner, e._type);
		if(result2 == false){
			return false;
		}
	}

	if(def._optional_body){
		if(check_types_resolved(interner, *def._optional_body) == false){
			return false;
		}
	}

	return true;
}

bool check_types_resolved(const type_interner_t& interner, const body_t& body){
	QUARK_ASSERT(interner.check_invariant());
	QUARK_ASSERT(body.check_invariant());

	for(const auto& e: body._statements){
		if(check_types_resolved(interner, e) == false){
			return false;
		}
	}
	for(const auto& s: body._symbol_table._symbols){
		QUARK_ASSERT(s.second.check_invariant());

		const auto value_type0 = s.second._value_type;
		const auto value_type = lookup_type_from_itype(interner, value_type0);

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

		if(value_type.is_undefined() == false && check_types_resolved(interner, value_type) == false){
			return false;
		}

		if(s.first != base_type_to_opcode(base_type::k_undefined) && s.second._init.is_undefined() == false && check_types_resolved(interner, s.second._init.get_type()) == false){
			return false;
		}
	}
	return true;
}





bool check_types_resolved(const type_interner_t& interner, const std::vector<std::shared_ptr<statement_t>>& s){
	QUARK_ASSERT(interner.check_invariant());

	for(const auto& e: s){
		if(floyd::check_types_resolved(interner, *e) == false){
			return false;
		}
	}
	return true;
}

bool check_types_resolved(const type_interner_t& interner, const statement_t& s){
	QUARK_ASSERT(interner.check_invariant());
	QUARK_ASSERT(s.check_invariant());

	struct visitor_t {
		const type_interner_t& interner;


		bool operator()(const statement_t::return_statement_t& s) const{
			return floyd::check_types_resolved(interner, s._expression);
		}

		bool operator()(const statement_t::bind_local_t& s) const{
			return true
				&& check_types_resolved(interner, s._bindtype)
				&& floyd::check_types_resolved(interner, s._expression)
				;
		}
		bool operator()(const statement_t::assign_t& s) const{
			return floyd::check_types_resolved(interner, s._expression);
		}
		bool operator()(const statement_t::assign2_t& s) const{
			return floyd::check_types_resolved(interner, s._expression);
		}
		bool operator()(const statement_t::init2_t& s) const{
			return floyd::check_types_resolved(interner, s._expression);
		}
		bool operator()(const statement_t::block_statement_t& s) const{
			return floyd::check_types_resolved(interner, s._body);
		}

		bool operator()(const statement_t::ifelse_statement_t& s) const{
			return true
				&& floyd::check_types_resolved(interner, s._condition)
				&& floyd::check_types_resolved(interner, s._then_body)
				&& floyd::check_types_resolved(interner, s._else_body)
				;
		}
		bool operator()(const statement_t::for_statement_t& s) const{
			return true
				&& floyd::check_types_resolved(interner, s._start_expression)
				&& floyd::check_types_resolved(interner, s._end_expression)
				&& floyd::check_types_resolved(interner, s._body)
				;
		}
		bool operator()(const statement_t::while_statement_t& s) const{
			return true
				&& floyd::check_types_resolved(interner, s._condition)
				&&floyd:: check_types_resolved(interner, s._body)
				;
		}

		bool operator()(const statement_t::expression_statement_t& s) const{
			return floyd::check_types_resolved(interner, s._expression);
		}
		bool operator()(const statement_t::software_system_statement_t& s) const{
			return true;
		}
		bool operator()(const statement_t::container_def_statement_t& s) const{
			return true;
		}
		bool operator()(const statement_t::benchmark_def_statement_t& s) const{
			return check_types_resolved(interner, s._body);
		}
	};

	return std::visit(visitor_t { interner }, s._contents);
}


bool check_types_resolved(const type_interner_t& interner, const struct_definition_t& s){
	QUARK_ASSERT(s.check_invariant());

	for(const auto& e: s._members){
		bool result = check_types_resolved(interner, e._type);
		if(result == false){
			return false;
		}
	}
	return true;
}


bool check_types_resolved__type_vector(const type_interner_t& interner, const std::vector<typeid_t>& elements){
	QUARK_ASSERT(interner.check_invariant());

	for(const auto& e: elements){
		if(check_types_resolved(interner, e) == false){
			return false;
		}
	}
	return true;
}

bool check_types_resolved(const type_interner_t& interner, const ast_type_t& type){
	QUARK_ASSERT(interner.check_invariant());
	QUARK_ASSERT(type.check_invariant());

	if(std::holds_alternative<typeid_t>(type._contents)){
		const auto& typeid0 = std::get<typeid_t>(type._contents);
		try {
			const auto itype = lookup_itype_from_typeid(interner, typeid0);
			return check_types_resolved(interner, typeid0);
		}
		catch(...){
			return false;
		}
	}
	else if(std::holds_alternative<itype_t>(type._contents)){
		const auto& itype0 = std::get<itype_t>(type._contents);
		const auto typeid0 = lookup_type_from_itype(interner, itype0);
		return check_types_resolved(interner, typeid0);
	}
	else{
		QUARK_ASSERT(false);
		throw std::exception();
	}
}

bool check_types_resolved(const type_interner_t& interner, const typeid_t& t){
	QUARK_ASSERT(interner.check_invariant());
	QUARK_ASSERT(t.check_invariant());

	struct visitor_t {
		const type_interner_t interner;


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
			return check_types_resolved(interner, *e._struct_def);
		}
		bool operator()(const typeid_t::vector_t& e) const{
			return check_types_resolved__type_vector(interner, e._parts);
		}
		bool operator()(const typeid_t::dict_t& e) const{
			return check_types_resolved__type_vector(interner, e._parts);
		}
		bool operator()(const typeid_t::function_t& e) const{
			return check_types_resolved__type_vector(interner, e._parts);
		}
		bool operator()(const typeid_t::identifier_t& e) const {

//???
			return true;
		}
	};
	return std::visit(visitor_t { interner }, t._contents);
}


bool check_types_resolved(const type_interner_t& interner){
	QUARK_ASSERT(interner.check_invariant());

	for(auto index = 0 ; index < interner.interned2.size() ; index++){
	}

	return true;
}

bool check_types_resolved(const general_purpose_ast_t& ast){
	QUARK_ASSERT(ast.check_invariant());

	if(check_types_resolved(ast._interned_types) == false){
		return false;
	}

	if(check_types_resolved(ast._interned_types, ast._globals) == false){
		return false;
	}
	for(const auto& e: ast._function_defs){
		const auto result = check_types_resolved(ast._interned_types, e);
		if(result == false){
			return false;
		}
	}
	return true;
}


}	//	floyd
