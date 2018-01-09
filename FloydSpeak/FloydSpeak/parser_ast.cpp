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





	void trace(const std::vector<std::shared_ptr<statement_t>>& e){
		QUARK_SCOPED_TRACE("statements");
		for(const auto s: e){
			trace(*s);
		}
	}




	////////////////////////			member_t


	member_t::member_t(const floyd_basics::typeid_t& type, const std::string& name) :
		_type(type),
		_name(name)
	{
		QUARK_ASSERT(type._base_type != floyd_basics::base_type::k_null && type.check_invariant());
		QUARK_ASSERT(name.size() > 0);

		QUARK_ASSERT(check_invariant());
	}

	member_t::member_t(const floyd_basics::typeid_t& type, const std::shared_ptr<value_t>& value, const std::string& name) :
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




	std::vector<floyd_basics::typeid_t> get_member_types(const vector<member_t>& m){
		vector<floyd_basics::typeid_t> r;
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
		const floyd_basics::typeid_t& element_type)
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


	ast_t::ast_t(){
		QUARK_ASSERT(check_invariant());
	}

	ast_t::ast_t(std::vector<std::shared_ptr<statement_t> > statements) :
		_statements(statements)
	{
		QUARK_ASSERT(check_invariant());
	}

	bool ast_t::check_invariant() const {
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

		return json_t::make_object(
			{
				{ "statements", statements_to_json(ast._statements) }
			}
		);
	}




} //	floyd_ast
