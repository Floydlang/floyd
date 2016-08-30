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

/*
QUARK_UNIT_TESTQ("ast_path_t()", ""){
	ast_path_t a;
	QUARK_UT_VERIFY(a.check_invariant());
	QUARK_UT_VERIFY(a._names.empty());
}
*/


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

resolved_path_t go_down(const resolved_path_t& path, const scope_ref_t child){
	QUARK_ASSERT(path.check_invariant());
	QUARK_ASSERT(child && child->check_invariant());

	resolved_path_t result = path;
	result._scopes.push_back(child);
	return result;
}

	/*
		Resolves a path by step down each scope and searching for a subscope with the node name.
		Can traverse any scope: global scope, structs, functions, local scopes etc.
		Normally you don't need this function but build the path while traversing down tree, adding new scopes as we go down.
	*/
/*
resolved_path_t resolve_path(const ast_t& ast, const floyd_parser::ast_path_t& path){
return {};
#if 0
	QUARK_ASSERT(ast.check_invariant());
	QUARK_ASSERT(path.check_invariant());

	std::vector<scope_ref_t> scopes;
	scopes.push_back(ast._global_scope);
	scope_ref_t current = scopes.back();

	for(int i = 1 ; i < path._names.size() ; i++){
		const auto wanted_name = type_identifier_t(path._names[i]);
		const auto found_it = find_if(
			current->_types_collector._types.begin(),
			current->_types_collector._types.end(),
			[&] (const std::pair<std::string, type_name_entry_t>& e) {
				const type_name_entry_t& type_name_entry = e.second;

				//??? Don't scan, use all types found at each node.
				for(const auto& e2: type_name_entry._defs){
					if(e2->is_subscope() && e2->get_subscope()->_name == wanted_name){
						return true;
					}
				}
				return false;
			}
		);

		if(found_it == current->_types_collector._types.end()){
			return {};
		}
		else{
				//??? Can be meny types at each node.
			const scope_ref_t next_scope = found_it->second->get_subscope();
			scopes.push_back(next_scope);
			current = next_scope;
		}
	}
	return { scopes };
#endif
}

QUARK_UNIT_TESTQ("resolve_path()", ""){
	ast_t a;
	const auto path = make_root(a);
	QUARK_UT_VERIFY(path.check_invariant());
}
*/


/*
floyd_parser::ast_path_t unresolve_path(const resolved_path_t& path){
	QUARK_ASSERT(path.check_invariant());

	std::vector<std::string> names;
	for(const auto i: path._scopes){
		names.push_back(i->_name.to_string());
	}
	return floyd_parser::ast_path_t{ names };
}
*/


/*
floyd_parser::ast_path_t go_down(const floyd_parser::ast_path_t& path, const std::string& sub_scope_name){
	QUARK_ASSERT(path.check_invariant());
	QUARK_ASSERT(sub_scope_name.size() > 0);

	return { path._names + sub_scope_name };
}

QUARK_UNIT_TESTQ("go_down()", ""){
	const auto a = ast_path_t();
	const auto b = go_down(a, "xyz");
	QUARK_UT_VERIFY(b.check_invariant());
	QUARK_UT_VERIFY(b._names.size() == 1);
	QUARK_UT_VERIFY(b._names[0] == "xyz");
}

QUARK_UNIT_TESTQ("go_down()", ""){
	const auto b = go_down(go_down(ast_path_t(), "xyz"), "abc");
	QUARK_UT_VERIFY(b.check_invariant());
	QUARK_UT_VERIFY(b._names.size() == 2);
	QUARK_UT_VERIFY(b._names[0] == "xyz");
	QUARK_UT_VERIFY(b._names[1] == "abc");
}



floyd_parser::ast_path_t go_up(const floyd_parser::ast_path_t& path){
	QUARK_ASSERT(path.check_invariant());
	QUARK_ASSERT(path._names.size() > 0);

	return { std::vector<string>(path._names.begin(), path._names.end() - 1) };
}

QUARK_UNIT_TESTQ("go_up()", ""){
	const auto a = go_down(go_down(ast_path_t(), "xyz"), "abc");
	const auto b = go_up(a);
	QUARK_UT_VERIFY(b.check_invariant());
	QUARK_UT_VERIFY(b._names.size() == 1);
	QUARK_UT_VERIFY(b._names[0] == "xyz");
}
*/

bool compare_scopes(const scope_ref_t& a, const scope_ref_t& b){
	QUARK_ASSERT(a && a->check_invariant());
	QUARK_ASSERT(b && b->check_invariant());

	return a->_name == b->_name && a->_type == b->_type;
}

/*
resolved_path_t find_subpath(const ast_t& ast, const floyd_parser::resolved_path_t& path, const scope_ref_t& wanted_scope){
	QUARK_ASSERT(ast.check_invariant());
	QUARK_ASSERT(path.check_invariant());
	QUARK_ASSERT(path._scopes.size() > 0);
	QUARK_ASSERT(wanted_scope && wanted_scope->check_invariant());

	//	Explore all sub-scopes.
	const auto s = path._scopes.back();
	for(const auto i: s->_types_collector._type_definitions){
		if(i.second->is_subscope()){
			const auto scope = i.second->get_subscope();
			const auto child_path = std::vector<scope_ref_t>{ path._scopes + std::vector<scope_ref_t>{ scope } };
			if(scope->_name == wanted_scope->_name && scope->_type == wanted_scope->_type){
				return { child_path };
			}
			else{
				return find_subpath(ast, { child_path }, wanted_scope);
			}
		}
	}
	return {};
}
resolved_path_t find_resolved_path_slow(const ast_t& ast, const scope_ref_t& wanted_scope){
	resolved_path_t path;
	path._scopes.push_back(ast._global_scope);

	if(compare_scopes(ast._global_scope, wanted_scope)){
		return path;
	}
	else{
		return find_subpath(ast, path, wanted_scope);
	}
}
*/




std::pair<scope_ref_t, int> resolve_scoped_variable(const ast_t& ast, const floyd_parser::resolved_path_t& path2, const std::string& s){
	QUARK_ASSERT(ast.check_invariant());
	QUARK_ASSERT(path2.check_invariant());
	QUARK_ASSERT(s.size() > 0);

	//	Look s in each scope's members, one at a time until we searched the global scope.
	for(auto i = path2._scopes.size() ; i > 0 ; i--){
		const scope_ref_t scope_def = path2._scopes[i - 1];

		for(int index = 0 ; index < scope_def->_members.size() ; index++){
			const auto& member = scope_def->_members[index];
			if(member._name == s){
				return std::pair<scope_ref_t, int>(scope_def, index);
			}
		}
	}
	return {{}, -1};
}

std::shared_ptr<const floyd_parser::type_def_t> resolve_type_to_def(const ast_t& ast, const floyd_parser::resolved_path_t& path, const floyd_parser::scope_ref_t scope_def, const type_identifier_t& s){
	QUARK_ASSERT(ast.check_invariant());
	QUARK_ASSERT(path.check_invariant());
	QUARK_ASSERT(scope_def && scope_def->check_invariant());
	QUARK_ASSERT(s.check_invariant());

	if(s.is_resolved()){
		return s.get_resolved();
	}
	else{
		for(auto i = path._scopes.size() ; i > 0 ; i--){
			const scope_ref_t scope = path._scopes[i - 1];
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

floyd_parser::type_identifier_t resolve_type_to_id(const ast_t& ast, const floyd_parser::resolved_path_t& path, const floyd_parser::scope_ref_t scope_def, const type_identifier_t& s){
	QUARK_ASSERT(ast.check_invariant());
	QUARK_ASSERT(path.check_invariant());
	QUARK_ASSERT(scope_def && scope_def->check_invariant());
	QUARK_ASSERT(s.check_invariant());

	const auto a = resolve_type_to_def(ast, path, scope_def, s);
	if(a){
		return type_identifier_t::resolve(a);
	}
	else{
		return s;
	}
}



//////////////////////////////////////////////////		make_default_value()



value_t make_default_value(const ast_t& ast, const floyd_parser::resolved_path_t& path, const scope_ref_t scope_def, const floyd_parser::type_identifier_t& type){
	QUARK_ASSERT(ast.check_invariant());
	QUARK_ASSERT(path.check_invariant());
	QUARK_ASSERT(scope_def->check_invariant());
	QUARK_ASSERT(type.check_invariant());

	const auto t = resolve_type_to_def(ast, path, scope_def, type);
	if(!t){
		throw std::runtime_error("Undefined type!");
	}
	const auto r = make_default_value(ast, path, *t);
	return r;
}

//??? Each struct_def should store its path?
value_t make_default_struct_value(const ast_t& ast, const resolved_path_t& path, scope_ref_t t){
	QUARK_ASSERT(ast.check_invariant());
	QUARK_ASSERT(path.check_invariant());
	QUARK_ASSERT(t && t->check_invariant());

	return make_struct_instance(ast, path, t);
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



type_identifier_t resolve_type_throw(const ast_t& ast, const resolved_path_t& path, const scope_ref_t& scope_def, const floyd_parser::type_identifier_t& s){
	QUARK_ASSERT(ast.check_invariant());
	QUARK_ASSERT(path.check_invariant());
	QUARK_ASSERT(scope_def && scope_def->check_invariant());
	QUARK_ASSERT(s.check_invariant());

	if(s.is_resolved()){
		return s;
	}
	else{
		const auto a = floyd_parser::resolve_type_to_id(ast, path, scope_def, s);
		if(!a.is_resolved()){
			throw std::runtime_error("Undefined type \"" + s.to_string() + "\"");
		}
		return a;
	}
}






}	//	floyd_parser


