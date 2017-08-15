//
//  parser_ast.cpp
//  FloydSpeak
//
//  Created by Marcus Zetterquist on 10/08/16.
//  Copyright © 2016 Marcus Zetterquist. All rights reserved.
//

#include "parser_ast.h"

#include "statements.h"
#include "parser_value.h"
#include "text_parser.h"
#include "parser_primitives.h"

#include <string>
#include <memory>
#include <map>
#include <iostream>
#include <cmath>
#include "parts/sha1_class.h"
#include "utils.h"
#include "json_support.h"
#include "json_writer.h"


namespace floyd_ast {
	using std::vector;
	using std::string;
	using std::pair;
	using std::make_shared;




	//??? use json
	void trace(const typeid_t& t, const std::string& label){
		QUARK_ASSERT(t.check_invariant());

		const auto type = t.get_base_type();
		if(type == floyd_basics::base_type::k_bool){
			QUARK_TRACE("<" + base_type_to_string(type) + "> " + label);
		}
		else if(type == floyd_basics::base_type::k_int){
			QUARK_TRACE("<" + base_type_to_string(type) + "> " + label);
		}
		else if(type == floyd_basics::base_type::k_float){
			QUARK_TRACE("<" + base_type_to_string(type) + "> " + label);
		}
		else if(type == floyd_basics::base_type::k_string){
			QUARK_TRACE("<" + base_type_to_string(type) + "> " + label);
		}
		else if(type == floyd_basics::base_type::k_struct){
			QUARK_SCOPED_TRACE("<" + base_type_to_string(type) + "> " + label);
//			trace(t.get_struct_def());
		}
		else if(type == floyd_basics::base_type::k_vector){
			QUARK_SCOPED_TRACE("<" + base_type_to_string(type) + "> " + label);
//			trace(*t._vector_def->_value_type, "");
		}
		else if(type == floyd_basics::base_type::k_function){
			QUARK_SCOPED_TRACE("<" + base_type_to_string(type) + "> " + label);
//			trace(t.get_function_def());
		}
		else{
			QUARK_ASSERT(false);
		}
	}



	////////////////////////			typeid_t


	bool typeid_t::check_invariant() const{
		if (_unresolved_type_symbol.empty() == false){
				QUARK_ASSERT(_base_type == floyd_basics::base_type::k_null);
				QUARK_ASSERT(_parts.empty());
				QUARK_ASSERT(_struct_def_id == "");
		}
		else{
			if(_base_type == floyd_basics::base_type::k_null){
				QUARK_ASSERT(_parts.empty());
				QUARK_ASSERT(_struct_def_id.empty());
			}
			else if(_base_type == floyd_basics::base_type::k_bool){
				QUARK_ASSERT(_parts.empty());
				QUARK_ASSERT(_struct_def_id.empty());
			}
			else if(_base_type == floyd_basics::base_type::k_int){
				QUARK_ASSERT(_parts.empty());
				QUARK_ASSERT(_struct_def_id.empty());
			}
			else if(_base_type == floyd_basics::base_type::k_float){
				QUARK_ASSERT(_parts.empty());
				QUARK_ASSERT(_struct_def_id.empty());
			}
			else if(_base_type == floyd_basics::base_type::k_string){
				QUARK_ASSERT(_parts.empty());
				QUARK_ASSERT(_struct_def_id.empty());
			}
			else if(_base_type == floyd_basics::base_type::k_struct){
				QUARK_ASSERT(_parts.empty() == true);
				QUARK_ASSERT(_struct_def_id.empty() == false);
			}
			else if(_base_type == floyd_basics::base_type::k_vector){
				QUARK_ASSERT(_parts.empty() == false);
				QUARK_ASSERT(_struct_def_id.empty() == true);
			}
			else if(_base_type == floyd_basics::base_type::k_function){
				QUARK_ASSERT(_parts.empty() == false);
				QUARK_ASSERT(_struct_def_id.empty() == true);
			}
			else{
				QUARK_ASSERT(false);
			}
		}
		return true;
	}

	void typeid_t::swap(typeid_t& other){
		QUARK_ASSERT(other.check_invariant());
		QUARK_ASSERT(check_invariant());

		std::swap(_base_type, other._base_type);
		_parts.swap(other._parts);
		_struct_def_id.swap(other._struct_def_id);
		_unresolved_type_symbol.swap(other._unresolved_type_symbol);

		QUARK_ASSERT(other.check_invariant());
		QUARK_ASSERT(check_invariant());
	}

	std::string typeid_t::to_string() const {
		QUARK_ASSERT(check_invariant());

		if(_unresolved_type_symbol != ""){
			return "unresolved:" + _unresolved_type_symbol;
		}
		else if(_base_type == floyd_basics::base_type::k_struct){
			return "[" + _parts[0].to_string() + "]";
		}
		else if(_base_type == floyd_basics::base_type::k_vector){
			return "[" + _parts[0].to_string() + "]";
		}
		else if(_base_type == floyd_basics::base_type::k_function){
			auto s = _parts[0].to_string() + " (";
			if(_parts.size() > 2){
				for(int i = 1 ; i < _parts.size() - 1 ; i++){
					s = s + _parts[i].to_string() + ",";
				}
				s = s + _parts[_parts.size() - 1].to_string();
			}
			s = s + ")";
			return s;
		}
		else{
			return base_type_to_string(_base_type);
		}
//		return json_to_compact_string(typeid_to_json(*this));
	}

	json_t typeid_to_json(const typeid_t& t){
		if(t._parts.empty() && t._struct_def_id.empty()){
			return base_type_to_string(t.get_base_type());
		}
		else{
			auto parts = json_t::make_array();
			for(const auto e: t._parts){
				parts = push_back(parts, typeid_to_json(e));
			}
			return make_object({
				{ "base_type", json_t(base_type_to_string(t.get_base_type())) },
				{ "parts", parts },
				{ "struct_def_id", t._struct_def_id.empty() == false ? t._struct_def_id : json_t() }
			});
		}
	}


	QUARK_UNIT_TESTQ("typeid_t{}", ""){
	}




	//////////////////////////////////////////////////		lexical_scope_t



	std::shared_ptr<const lexical_scope_t> lexical_scope_t::make_struct_object(const std::vector<member_t>& members){
		auto r = std::make_shared<lexical_scope_t>(
			lexical_scope_t{
				etype::k_struct_scope,
				{},
				members,
				{},
				{},
				{},
				nullptr
			}
		);
		QUARK_ASSERT(r->check_invariant());
		return r;
	}

	std::shared_ptr<const lexical_scope_t> lexical_scope_t::make_global_scope(
		const std::vector<std::shared_ptr<statement_t> >& statements,
		const std::vector<member_t>& globals,
		const std::map<int, std::shared_ptr<const lexical_scope_t> > objects
	)
	{
		auto r = std::make_shared<lexical_scope_t>(
			lexical_scope_t{
				etype::k_global_scope,
				{},
				globals,
				statements,
				{},
				objects,
				nullptr
			}
		);

		QUARK_ASSERT(r->check_invariant());
		return r;
	}

	bool lexical_scope_t::shallow_check_invariant() const {
		return true;
	}

	bool lexical_scope_t::check_invariant() const {
		//??? Check for duplicates? Other things?
		for(const auto& m: _args){
			QUARK_ASSERT(m.check_invariant());
		}
		for(const auto& m: _state){
			QUARK_ASSERT(m.check_invariant());
		}


		if(_type == etype::k_function_scope){
			QUARK_ASSERT(_host_function == nullptr);
		}
		else if(_type == etype::k_struct_scope){
			QUARK_ASSERT(_return_type._base_type == floyd_basics::base_type::k_null);
			QUARK_ASSERT(_host_function == nullptr);
		}
		else if(_type == etype::k_global_scope){
			QUARK_ASSERT(_return_type._base_type == floyd_basics::base_type::k_null);
			QUARK_ASSERT(_host_function == nullptr);
		}
		else if(_type == etype::k_block){
			QUARK_ASSERT(_host_function == nullptr);
		}
		else if(_type == etype::k_host_function_scope){
			QUARK_ASSERT(_host_function != nullptr);
		}
		else{
			QUARK_ASSERT(false);
		}
		return true;
	}

	bool lexical_scope_t::operator==(const lexical_scope_t& other) const{
		QUARK_ASSERT(check_invariant());
		QUARK_ASSERT(other.check_invariant());

		if(_type != other._type){
			return false;
		}
		if(_args != other._args){
			return false;
		}
		if(_state != other._state){
			return false;
		}
		if(!(_statements == other._statements)){
			return false;
		}
		if(!(_return_type == other._return_type)){
			return false;
		}
		if(_objects != other._objects){
			return false;
		}
		if(_host_function != other._host_function){
			return false;
		}
		return true;
	}

	string scope_type_to_string(lexical_scope_t::etype type){
		if(type == lexical_scope_t::etype::k_function_scope){
			return "function";
		}
		else if(type == lexical_scope_t::etype::k_struct_scope){
			return "struct";
		}
		else if(type == lexical_scope_t::etype::k_global_scope){
			return "global";
		}
		else if(type == lexical_scope_t::etype::k_block){
			return "subscope";
		}
		else if(type == lexical_scope_t::etype::k_host_function_scope){
			return "host_function";
		}
		else{
			QUARK_ASSERT(false);
		}
	}

	json_t objects_to_json(const std::map<int, std::shared_ptr<const lexical_scope_t> >& s){
		std::map<string, json_t> r;
		for(const auto i: s){
			r[std::to_string(i.first)] = lexical_scope_to_json(*i.second);
		}
		return r;
	}

	json_t lexical_scope_to_json(const lexical_scope_t& scope_def){
		const auto args = members_to_json(scope_def._args);
		const auto state = members_to_json(scope_def._state);

		std::vector<json_t> statements;
		for(const auto i: scope_def._statements){
			statements.push_back(statement_to_json(*i));
		}
		json_t statements2(statements);

		const auto objects = objects_to_json(scope_def.get_objects());

		return make_object({
			{ "objtype", json_t(scope_type_to_string(scope_def._type)) },
			{ "args", args.get_array_size() == 0 ? json_t() : json_t(args) },
			{ "state", state.get_array_size() == 0 ? json_t() : json_t(state) },
			{ "statements", statements2.get_array_size() == 0 ? json_t() : json_t(statements2) },
			{ "return_type", scope_def._return_type.is_null() ? json_t() : scope_def._return_type.to_string() },
//			{ "symbols", symbols.get_object_size() == 0 ? json_t() : symbols },
			{ "objects", objects.get_object_size() == 0 ? json_t() : objects },
			{ "host_function",
				scope_def._host_function == nullptr ? json_t() : json_t("HOST FUNCTION")
			}
		});
	}

	std::shared_ptr<const lexical_scope_t> lexical_scope_t::make_function_object(
		const std::vector<member_t>& args,
		const std::vector<member_t>& locals,
		const std::vector<std::shared_ptr<statement_t> >& statements,
		const typeid_t& return_type,
		const std::map<int, std::shared_ptr<const lexical_scope_t> > objects
	)
	{
		for(const auto i: args){ QUARK_ASSERT(i.check_invariant()); };
		for(const auto i: locals){ QUARK_ASSERT(i.check_invariant()); };

		auto obj = make_shared<lexical_scope_t>(
			lexical_scope_t{
				lexical_scope_t::etype::k_function_scope,
				args,
				locals,
				statements,
				return_type,
				objects,
				nullptr
			}
		);
		return obj;
	}

	std::shared_ptr<const lexical_scope_t> lexical_scope_t::make_host_function_object(
		const std::vector<member_t>& args,
		const typeid_t& return_type,
		HOST_FUNCTION host_function
	)
	{
		for(const auto i: args){ QUARK_ASSERT(i.check_invariant()); };
		QUARK_ASSERT(return_type.check_invariant());
		QUARK_ASSERT(host_function != nullptr);

		auto obj = make_shared<lexical_scope_t>(
			lexical_scope_t{
				lexical_scope_t::etype::k_host_function_scope,
				args,
				{},
				{},
				return_type,
				{},
				host_function
			}
		);
		return obj;
	}

	std::shared_ptr<const lexical_scope_t> lexical_scope_t::make_block_object(
		const std::vector<member_t>& locals,
		const std::vector<std::shared_ptr<statement_t> >& statements,
		const std::map<int, std::shared_ptr<const lexical_scope_t> > objects
	)
	{
		for(const auto i: locals){ QUARK_ASSERT(i.check_invariant()); };

		auto obj = make_shared<lexical_scope_t>(
			lexical_scope_t{
				lexical_scope_t::etype::k_block,
				{},
				locals,
				statements,
				{},
				objects,
				nullptr
			}
		);
		return obj;
	}


	QUARK_UNIT_TESTQ("lexical_scope_t::operator==", ""){
		const auto a = lexical_scope_t::make_global_scope({}, {}, {});
		const auto b = lexical_scope_t::make_global_scope({}, {}, {});
		QUARK_TEST_VERIFY(*a == *b);
	}




	void trace(const std::vector<std::shared_ptr<statement_t>>& e){
		QUARK_SCOPED_TRACE("statements");
		for(const auto s: e){
			trace(*s);
		}
	}




	//////////////////////////////////////////////////		function_object_t





	bool function_object_t::shallow_check_invariant() const {
		return true;
	}

	bool function_object_t::check_invariant() const {
		//??? Check for duplicates? Other things?
		for(const auto& m: _args){
			QUARK_ASSERT(m.check_invariant());
		}
		for(const auto& m: _state){
			QUARK_ASSERT(m.check_invariant());
		}
		return true;
	}

	bool function_object_t::operator==(const function_object_t& other) const{
		QUARK_ASSERT(check_invariant());
		QUARK_ASSERT(other.check_invariant());

		if(_args != other._args){
			return false;
		}
		if(_state != other._state){
			return false;
		}
		if(!(_statements == other._statements)){
			return false;
		}
		if(!(_return_type == other._return_type)){
			return false;
		}
		if(_objects != other._objects){
			return false;
		}
		if(_host_function != other._host_function){
			return false;
		}
		return true;
	}

	std::shared_ptr<const function_object_t> function_object_t::make_function_object(
		const std::vector<member_t>& args,
		const std::vector<member_t>& locals,
		const std::vector<std::shared_ptr<statement_t> >& statements,
		const typeid_t& return_type,
		const std::map<int, std::shared_ptr<const function_object_t> > objects
	)
	{
		for(const auto i: args){ QUARK_ASSERT(i.check_invariant()); };
		for(const auto i: locals){ QUARK_ASSERT(i.check_invariant()); };

		auto obj = make_shared<function_object_t>(
			function_object_t{
				args,
				locals,
				statements,
				return_type,
				objects,
				nullptr
			}
		);
		return obj;
	}

	std::shared_ptr<const function_object_t> function_object_t::make_host_function_object(
		const std::vector<member_t>& args,
		const typeid_t& return_type,
		HOST_FUNCTION host_function
	)
	{
		for(const auto i: args){ QUARK_ASSERT(i.check_invariant()); };
		QUARK_ASSERT(return_type.check_invariant());
		QUARK_ASSERT(host_function != nullptr);

		auto obj = make_shared<function_object_t>(
			function_object_t{
				args,
				{},
				{},
				return_type,
				{},
				host_function
			}
		);
		return obj;
	}


	QUARK_UNIT_TESTQ("function_object_t::operator==", ""){
		const auto a = true;
		const auto b = true;
		QUARK_TEST_VERIFY(a == b);
	}



	json_t objects_to_json(const std::map<int, std::shared_ptr<const function_object_t> >& s){
		std::map<string, json_t> r;
		for(const auto i: s){
			r[std::to_string(i.first)] = function_object_to_json(*i.second);
		}
		return r;
	}

	json_t function_object_to_json(const function_object_t& scope_def){
		const auto args = members_to_json(scope_def._args);
		const auto state = members_to_json(scope_def._state);

		std::vector<json_t> statements;
		for(const auto i: scope_def._statements){
			statements.push_back(statement_to_json(*i));
		}
		json_t statements2(statements);

		const auto objects = objects_to_json(scope_def.get_objects());

		return make_object({
			{ "args", args.get_array_size() == 0 ? json_t() : json_t(args) },
			{ "state", state.get_array_size() == 0 ? json_t() : json_t(state) },
			{ "statements", statements2.get_array_size() == 0 ? json_t() : json_t(statements2) },
			{ "return_type", scope_def._return_type.is_null() ? json_t() : scope_def._return_type.to_string() },
			{ "objects", objects.get_object_size() == 0 ? json_t() : objects },
			{ "host_function",
				scope_def._host_function == nullptr ? json_t() : json_t("HOST FUNCTION")
			}
		});
	}





	////////////////////////			member_t


	member_t::member_t(const typeid_t& type, const std::string& name) :
		_type(type),
		_name(name)
	{
		QUARK_ASSERT(type._base_type != floyd_basics::base_type::k_null && type.check_invariant());
		QUARK_ASSERT(name.size() > 0);

		QUARK_ASSERT(check_invariant());
	}

	member_t::member_t(const typeid_t& type, const std::shared_ptr<value_t>& value, const std::string& name) :
		_type(type),
		_value(value),
		_name(name)
	{
		QUARK_ASSERT(type._base_type != floyd_basics::base_type::k_null && type.check_invariant());
		QUARK_ASSERT(name.size() > 0);

		QUARK_ASSERT(check_invariant());
	}

	bool member_t::check_invariant() const{
		QUARK_ASSERT(_type._base_type != floyd_basics::base_type::k_null && _type.check_invariant());
		QUARK_ASSERT(_name.size() > 0);
		QUARK_ASSERT(!_value || _value->check_invariant());
		if(_value){
			QUARK_ASSERT(_type == _value->get_type());
		}
		return true;
	}

	bool member_t::operator==(const member_t& other) const{
		QUARK_ASSERT(check_invariant());
		QUARK_ASSERT(other.check_invariant());

		return (_type == other._type)
			&& (_name == other._name)
			&& compare_shared_values(_value, other._value);
	}


	void trace(const member_t& member){
		QUARK_TRACE("<member> type: <" + member._type.to_string() + "> name: \"" + member._name + "\"");
	}


	//??? more. Use to_json().
	void trace(const std::shared_ptr<const lexical_scope_t>& e){
		QUARK_ASSERT(e && e->check_invariant());
/*
		QUARK_ASSERT(e.check_invariant());
		QUARK_SCOPED_TRACE("struct_def_t");
		trace_vec("members", e._members);
*/
	}


	std::vector<typeid_t> get_member_types(const vector<member_t>& m){
		vector<typeid_t> r;
		for(const auto a: m){
			r.push_back(a._type);
		}
		return r;
	}

	json_t members_to_json(const std::vector<member_t>& members){
		std::vector<json_t> r;
		for(const auto i: members){
			const auto member = make_object({
				{ "type", typeid_to_json(i._type) },
				{ "value", i._value ? value_to_json(*i._value) : json_t() },
				{ "name", json_t(i._name) }
			});
			r.push_back(json_t(member));
		}
		return r;
	}




	//////////////////////////////////////		vector_def_t



	vector_def_t vector_def_t::make2(
		const typeid_t& element_type)
	{
		QUARK_ASSERT(element_type._base_type != floyd_basics::base_type::k_null && element_type.check_invariant());

		vector_def_t result;
		result._element_type = element_type;

		QUARK_ASSERT(result.check_invariant());
		return result;
	}

	bool vector_def_t::check_invariant() const{
		QUARK_ASSERT(_element_type._base_type != floyd_basics::base_type::k_null && _element_type.check_invariant());
		return true;
	}

	bool vector_def_t::operator==(const vector_def_t& other) const{
		QUARK_ASSERT(check_invariant());
		QUARK_ASSERT(other.check_invariant());

		if(!(_element_type == other._element_type)){
			return false;
		}
		return true;
	}

	void trace(const vector_def_t& e){
		QUARK_ASSERT(e.check_invariant());
		QUARK_SCOPED_TRACE("vector_def_t");
		QUARK_TRACE_SS("element_type: " << e._element_type.to_string());
	}

	json_t vector_def_to_json(const vector_def_t& s){
		return {
		};
	}




	////////////////////////			ast_t


	ast_t::ast_t() :
		_global_scope(lexical_scope_t::make_global_scope({}, {}, {}))
	{
		QUARK_ASSERT(check_invariant());
	}

	ast_t::ast_t(
		std::shared_ptr<const lexical_scope_t> global_scope
	) :
		_global_scope(global_scope)
	{
		QUARK_ASSERT(check_invariant());
	}

	bool ast_t::check_invariant() const {
		QUARK_ASSERT(_global_scope && _global_scope->check_invariant());
		return true;
	}



	void trace(const ast_t& program){
		QUARK_ASSERT(program.check_invariant());
		QUARK_SCOPED_TRACE("program");

		const auto s = json_to_pretty_string(ast_to_json(program));
		QUARK_TRACE(s);
	}

	json_t ast_to_json(const ast_t& ast){
		QUARK_ASSERT(ast.check_invariant());

		return lexical_scope_to_json(*ast.get_global_scope());
	}




	function_reg_t make_host_function_reg(const typeid_t& return_type, const std::vector<member_t>& args, HOST_FUNCTION host_function, int id){
		const auto function_typeid = typeid_t::make_function(return_type, get_member_types(args));
		const auto s = lexical_scope_t::make_host_function_object(
			args,
			return_type,
			host_function
		);

		value_t f = make_function_value(function_typeid, id);
		return { function_typeid, s, id, make_shared<value_t>(f) };
	}




} //	floyd_ast
