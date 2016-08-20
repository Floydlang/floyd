//
//  ast_utils.cpp
//  FloydSpeak
//
//  Created by Marcus Zetterquist on 17/08/16.
//  Copyright © 2016 Marcus Zetterquist. All rights reserved.
//

#include "ast_utils.h"


#include "parser_statement.h"
#include "parser_value.h"


namespace floyd_parser {
	using std::string;
	using std::make_shared;




// ??? move to ast_utils
std::pair<scope_ref_t, int> resolve_scoped_variable(floyd_parser::scope_ref_t scope_def, const std::string& s){
	QUARK_ASSERT(scope_def && scope_def->check_invariant());
	QUARK_ASSERT(s.size() > 0);

	for(int index = 0 ; index < scope_def->_members.size() ; index++){
		const auto& member = scope_def->_members[index];
		if(member._name == s){
			return std::pair<scope_ref_t, int>(scope_def, index);
		}
	}

	//	Not, found - try parent scope.
	const auto parent = scope_def->_parent_scope.lock();
	if(parent){
		return resolve_scoped_variable(parent, s);
	}
	else{
		return {};
	}
}

/*
std::pair<scope_ref_t, std::shared_ptr<type_def_t> > resolve_type(const floyd_parser::scope_ref_t scope_def, const type_identifier_t& s){
	QUARK_ASSERT(scope_def && scope_def->check_invariant());
	QUARK_ASSERT(s.check_invariant());

	if(s.is_resolved()){
		return s.get_resolved();
	}
	else{
		const auto a = scope_def->_types_collector.resolve_identifier(s.to_string());
		if(a){
			return { scope_def, a };
		}

		//	Not, found - try parent scope.
		const auto parent = scope_def->_parent_scope.lock();
		if(parent){
			return resolve_type(parent, s);
		}
		else{
			return {};
		}
	}
}

std::pair<scope_ref_t, floyd_parser::type_identifier_t> resolve_type2(const floyd_parser::scope_ref_t scope_def, const type_identifier_t& s){
	const auto a = resolve_type(scope_def, s);
	return { a.first, type_identifier_t::resolve(a.second) };
}
*/
// ??? move to ast_utils
std::shared_ptr<floyd_parser::type_def_t> resolve_type(const floyd_parser::scope_ref_t scope_def, const type_identifier_t& s){
	QUARK_ASSERT(scope_def && scope_def->check_invariant());
	QUARK_ASSERT(s.check_invariant());

	if(s.is_resolved()){
		return s.get_resolved();
	}
	else{
		const auto a = scope_def->_types_collector.resolve_identifier(s.to_string());
		if(a){
			return a;
		}

		//	Not, found - try parent scope.
		const auto parent = scope_def->_parent_scope.lock();
		if(parent){
			return resolve_type(parent, s);
		}
		else{
			return {};
		}
	}
}

// ??? move to ast_utils
floyd_parser::type_identifier_t resolve_type2(const floyd_parser::scope_ref_t scope_def, const type_identifier_t& s){
	const auto a = resolve_type(scope_def, s);
	if(a){
		return type_identifier_t::resolve(a);
	}
	else{
		return s;
	}
}



/*
//////////////////////////////////////////////////		resolve_type()





std::shared_ptr<type_def_t> resolve_type(const scope_ref_t scope_def, const type_identifier_t& s){
	QUARK_ASSERT(scope_def->check_invariant());
	QUARK_ASSERT(s.check_invariant());

	const auto t = scope_def->_types_collector.resolve_identifier(s.to_string());
	if(t){
		return t;
	}
	auto parent_scope = scope_def->_parent_scope.lock();
	if(parent_scope){
		return resolve_type(parent_scope, s);
	}
	else{
		return {};
	}
}
*/




//////////////////////////////////////////////////		make_default_value()



value_t make_default_value(const scope_ref_t scope_def, const floyd_parser::type_identifier_t& type){
	QUARK_ASSERT(scope_def->check_invariant());
	QUARK_ASSERT(type.check_invariant());

	const auto t = resolve_type(scope_def, type);
	if(!t){
		throw std::runtime_error("Undefined type!");
	}
	const auto r = make_default_value(*t);
	return r;
}



	value_t make_default_value(scope_ref_t t){
		QUARK_ASSERT(t && t->check_invariant());
		return make_struct_instance(t);
	}

	value_t make_default_value(const type_def_t& t){
		QUARK_ASSERT(t.check_invariant());

		const auto type = t.get_type();
		if(type == k_int){
			return value_t(0);
		}
		else if(type == k_bool){
			return value_t(false);
		}
		else if(type == k_string){
			return value_t("");
		}
		else if(type == k_struct){
			return make_default_value(t.get_struct_def());
		}
		else if(type == k_vector){
			QUARK_ASSERT(false);
		}
		else if(type == k_function){
			QUARK_ASSERT(false);
		}
		else{
			QUARK_ASSERT(false);
		}
	}







member_t find_struct_member_throw(const scope_ref_t& struct_ref, const std::string& member_name){
	QUARK_ASSERT(struct_ref && struct_ref->check_invariant());
	QUARK_ASSERT(member_name.size() > 0);

	const auto found_it = find_if(
		struct_ref->_members.begin(),
		struct_ref->_members.end(),
		[&] (const member_t& member) { return member._name == member_name; }
	);
	if(found_it == struct_ref->_members.end()){
		throw std::runtime_error("Unresolved member \"" + member_name + "\"");
	}

	return *found_it;
}



type_identifier_t resolve_type_throw(const scope_ref_t& scope_def, const floyd_parser::type_identifier_t& s){
	QUARK_ASSERT(scope_def && scope_def->check_invariant());
	QUARK_ASSERT(s.check_invariant());

	if(s.is_resolved()){
		return s;
	}
	else{
		const auto a = floyd_parser::resolve_type2(scope_def, s);
		if(!a.is_resolved()){
			throw std::runtime_error("Undefined type \"" + s.to_string() + "\"");
		}
		return a;
	}
}






}	//	floyd_parser


