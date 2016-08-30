//
//  ast_utils.cpp
//  FloydSpeak
//
//  Created by Marcus Zetterquist on 17/08/16.
//  Copyright © 2016 Marcus Zetterquist. All rights reserved.
//

#include "ast_utils.h"


#include "statements.h"
#include "parser_value.h"
#include "utils.h"


namespace floyd_parser {
	using std::string;
	using std::make_shared;


//////////////////////////////////////////////////		resolved_path_t


scope_ref_t resolved_path_t::get_leaf() const{
	QUARK_ASSERT(check_invariant());
	QUARK_ASSERT(!_scopes.empty());

	return _scopes.back();
}

bool resolved_path_t::check_invariant() const {
	for(const auto i: _scopes){
		QUARK_ASSERT(i && i->check_invariant());
	}
	return true;
};



//////////////////////////////////////////////////		free


resolved_path_t go_down(const resolved_path_t& path, const scope_ref_t& child){
	QUARK_ASSERT(path.check_invariant());
	QUARK_ASSERT(child && child->check_invariant());

	resolved_path_t result = path;
	result._scopes.push_back(child);
	return result;
}

bool compare_scopes(const scope_ref_t& a, const scope_ref_t& b){
	QUARK_ASSERT(a && a->check_invariant());
	QUARK_ASSERT(b && b->check_invariant());

	return a->_name == b->_name && a->_type == b->_type;
}



std::pair<scope_ref_t, int> resolve_scoped_variable(const ast_t& ast, const floyd_parser::resolved_path_t& path2, const std::string& s){
	QUARK_ASSERT(ast.check_invariant());
	QUARK_ASSERT(path2.check_invariant());
	QUARK_ASSERT(s.size() > 0);

	//	Look s in each scope's members, one at a time until we searched the global scope.
	for(auto i = path2._scopes.size() ; i > 0 ; i--){
		const scope_ref_t& scope_def = path2._scopes[i - 1];

		for(int index = 0 ; index < scope_def->_members.size() ; index++){
			const auto& member = scope_def->_members[index];
			if(member._name == s){
				return std::pair<scope_ref_t, int>(scope_def, index);
			}
		}
	}
	return {{}, -1};
}

std::shared_ptr<const floyd_parser::type_def_t> resolve_type_to_def(const ast_t& ast, const floyd_parser::resolved_path_t& path, const type_identifier_t& s){
	QUARK_ASSERT(ast.check_invariant());
	QUARK_ASSERT(path.check_invariant());
	QUARK_ASSERT(s.check_invariant());

	if(s.is_resolved()){
		return s.get_resolved();
	}
	else{
		for(auto i = path._scopes.size() ; i > 0 ; i--){
			const scope_ref_t& scope = path._scopes[i - 1];
			const auto a = scope->_types_collector.resolve_identifier(s.to_string());
			if(a.size() > 1){
				throw std::runtime_error("Multiple definitions for type-identifier \"" + s.to_string() + "\".");
			}
			if(!a.empty()){
				return a[0];
			}
		}
		return {};
	}
}

floyd_parser::type_identifier_t resolve_type_to_id(const ast_t& ast, const floyd_parser::resolved_path_t& path, const type_identifier_t& s){
	QUARK_ASSERT(ast.check_invariant());
	QUARK_ASSERT(path.check_invariant());
	QUARK_ASSERT(s.check_invariant());

	const auto a = resolve_type_to_def(ast, path, s);
	if(a){
		return type_identifier_t::resolve(a);
	}
	else{
		return s;
	}
}



//////////////////////////////////////////////////		make_default_value()



value_t make_default_value(const ast_t& ast, const floyd_parser::resolved_path_t& path, const floyd_parser::type_identifier_t& type){
	QUARK_ASSERT(ast.check_invariant());
	QUARK_ASSERT(path.check_invariant());
	QUARK_ASSERT(type.check_invariant());

	const auto t = resolve_type_to_def(ast, path, type);
	if(!t){
		throw std::runtime_error("Undefined type!");
	}
	const auto r = make_default_value(ast, path, *t);
	return r;
}

//??? Each struct_def should store its path?
value_t make_default_struct_value(const ast_t& ast, const resolved_path_t& path, scope_ref_t struct_def){
	QUARK_ASSERT(ast.check_invariant());
	QUARK_ASSERT(path.check_invariant());
	QUARK_ASSERT(struct_def && struct_def->check_invariant());

	return make_struct_instance(ast, path, struct_def);
}

value_t make_default_value(const ast_t& ast, const resolved_path_t& path, const type_def_t& t){
	QUARK_ASSERT(ast.check_invariant());
	QUARK_ASSERT(path.check_invariant());
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
		return make_default_struct_value(ast, path, t.get_struct_def());
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

type_identifier_t resolve_type_throw(const ast_t& ast, const resolved_path_t& path, const floyd_parser::type_identifier_t& s){
	QUARK_ASSERT(ast.check_invariant());
	QUARK_ASSERT(path.check_invariant());
	QUARK_ASSERT(s.check_invariant());

	if(s.is_resolved()){
		return s;
	}
	else{
		const auto a = floyd_parser::resolve_type_to_id(ast, path, s);
		if(!a.is_resolved()){
			throw std::runtime_error("Undefined type \"" + s.to_string() + "\"");
		}
		return a;
	}
}


}	//	floyd_parser
