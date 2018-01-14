//
//  floyd_basics.cpp
//  FloydSpeak
//
//  Created by Marcus Zetterquist on 2017-08-09.
//  Copyright © 2017 Marcus Zetterquist. All rights reserved.
//

#include "floyd_basics.h"


#include "parser_primitives.h"
#include "parser_value.h"
#include "utils.h"

using std::string;
using std::vector;

namespace floyd {

	//////////////////////////////////////////////////		base_type


	string base_type_to_string(const base_type t){
		if(t == base_type::k_null){
			return "null";
		}
		else if(t == base_type::k_bool){
			return "bool";
		}
		else if(t == base_type::k_int){
			return "int";
		}
		else if(t == base_type::k_float){
			return "float";
		}
		else if(t == base_type::k_string){
			return "string";
		}
		else if(t == base_type::k_struct){
			return "struct";
		}
		else if(t == base_type::k_vector){
			return "vector";
		}
		else if(t == base_type::k_function){
			return "function";
		}
		else if(t == base_type::k_custom_type){
			return "custom";
		}
		else{
			QUARK_ASSERT(false);
		}
	}




	//////////////////////////////////////		base_type_to_string(base_type)


	QUARK_UNIT_TESTQ("base_type_to_string(base_type)", ""){
		QUARK_TEST_VERIFY(base_type_to_string(base_type::k_bool) == "bool");
		QUARK_TEST_VERIFY(base_type_to_string(base_type::k_int) == "int");
		QUARK_TEST_VERIFY(base_type_to_string(base_type::k_float) == "float");
		QUARK_TEST_VERIFY(base_type_to_string(base_type::k_string) == "string");
		QUARK_TEST_VERIFY(base_type_to_string(base_type::k_struct) == "struct");
		QUARK_TEST_VERIFY(base_type_to_string(base_type::k_vector) == "vector");
		QUARK_TEST_VERIFY(base_type_to_string(base_type::k_function) == "function");
	}



//??? Make separate constant strings for "@" etc and use them all over code instead of "@".

//??? Use "f()" for functions.
//??? Use "[n]" for lookups.

static std::map<expression_type, string> operation_to_string_lookup = {
	{ expression_type::k_arithmetic_add__2, "+" },
	{ expression_type::k_arithmetic_subtract__2, "-" },
	{ expression_type::k_arithmetic_multiply__2, "*" },
	{ expression_type::k_arithmetic_divide__2, "/" },
	{ expression_type::k_arithmetic_remainder__2, "%" },

	{ expression_type::k_comparison_smaller_or_equal__2, "<=" },
	{ expression_type::k_comparison_smaller__2, "<" },
	{ expression_type::k_comparison_larger_or_equal__2, ">=" },
	{ expression_type::k_comparison_larger__2, ">" },

	{ expression_type::k_logical_equal__2, "==" },
	{ expression_type::k_logical_nonequal__2, "!=" },
	{ expression_type::k_logical_and__2, "&&" },
	{ expression_type::k_logical_or__2, "||" },
//	{ expression_type::k_logical_not, "!" },
	{ expression_type::k_arithmetic_unary_minus__1, "unary_minus" },

	{ expression_type::k_literal, "k" },

	{ expression_type::k_conditional_operator3, "?:" },
	{ expression_type::k_call, "call" },

	{ expression_type::k_variable, "@" },
	{ expression_type::k_resolve_member, "->" },

	{ expression_type::k_lookup_element, "[-]" }
};

std::map<string, expression_type> make_reverse(const std::map<expression_type, string>& m){
	std::map<string, expression_type> temp;
	for(const auto e: m){
		temp[e.second] = e.first;
	}
	return temp;
}

static std::map<string, expression_type> string_to_operation_lookip = make_reverse(operation_to_string_lookup);




string expression_type_to_token(const expression_type& op){
	const auto r = operation_to_string_lookup.find(op);
	QUARK_ASSERT(r != operation_to_string_lookup.end());
	return r->second;
}

expression_type token_to_expression_type(const string& op){
	const auto r = string_to_operation_lookip.find(op);
	QUARK_ASSERT(r != string_to_operation_lookip.end());
	return r->second;
}




	//////////////////////////////////////////////////		typeid_t



	//??? use json
	void trace(const typeid_t& t, const std::string& label){
		QUARK_ASSERT(t.check_invariant());

		const auto type = t.get_base_type();
		if(type == floyd::base_type::k_bool){
			QUARK_TRACE("<" + base_type_to_string(type) + "> " + label);
		}
		else if(type == floyd::base_type::k_int){
			QUARK_TRACE("<" + base_type_to_string(type) + "> " + label);
		}
		else if(type == floyd::base_type::k_float){
			QUARK_TRACE("<" + base_type_to_string(type) + "> " + label);
		}
		else if(type == floyd::base_type::k_string){
			QUARK_TRACE("<" + base_type_to_string(type) + "> " + label);
		}
		else if(type == floyd::base_type::k_struct){
			QUARK_SCOPED_TRACE("<" + base_type_to_string(type) + "> " + label);
//			trace(t.get_struct_def());
		}
		else if(type == floyd::base_type::k_vector){
			QUARK_SCOPED_TRACE("<" + base_type_to_string(type) + "> " + label);
//			trace(*t._vector_def->_value_type, "");
		}
		else if(type == floyd::base_type::k_function){
			QUARK_SCOPED_TRACE("<" + base_type_to_string(type) + "> " + label);
//			trace(t.get_function_def());
		}
		else{
			QUARK_ASSERT(false);
		}
	}



	////////////////////////			typeid_t


	bool typeid_t::check_invariant() const{
		if(_base_type == floyd::base_type::k_null){
			QUARK_ASSERT(_parts.empty());
			QUARK_ASSERT(_unique_type_id.empty());
			QUARK_ASSERT(_unresolved_identifier.empty());
		}
		else if(_base_type == floyd::base_type::k_bool){
			QUARK_ASSERT(_parts.empty());
			QUARK_ASSERT(_unique_type_id.empty());
			QUARK_ASSERT(_unresolved_identifier.empty());
		}
		else if(_base_type == floyd::base_type::k_int){
			QUARK_ASSERT(_parts.empty());
			QUARK_ASSERT(_unique_type_id.empty());
			QUARK_ASSERT(_unresolved_identifier.empty());
		}
		else if(_base_type == floyd::base_type::k_float){
			QUARK_ASSERT(_parts.empty());
			QUARK_ASSERT(_unique_type_id.empty());
			QUARK_ASSERT(_unresolved_identifier.empty());
		}
		else if(_base_type == floyd::base_type::k_string){
			QUARK_ASSERT(_parts.empty());
			QUARK_ASSERT(_unique_type_id.empty());
			QUARK_ASSERT(_unresolved_identifier.empty());
		}
		else if(_base_type == floyd::base_type::k_struct){
			QUARK_ASSERT(_parts.empty() == true);
			QUARK_ASSERT(_unique_type_id.empty() == false);
			QUARK_ASSERT(_unresolved_identifier.empty());
		}
		else if(_base_type == floyd::base_type::k_vector){
			QUARK_ASSERT(_parts.empty() == false);
			QUARK_ASSERT(_unique_type_id.empty() == true);
			QUARK_ASSERT(_unresolved_identifier.empty());
		}
		else if(_base_type == floyd::base_type::k_function){
			QUARK_ASSERT(_parts.empty() == false);
			QUARK_ASSERT(_unique_type_id.empty() == true);
			QUARK_ASSERT(_unresolved_identifier.empty());
		}
		else if(_base_type == floyd::base_type::k_custom_type){
			QUARK_ASSERT(_parts.empty());
			QUARK_ASSERT(_unique_type_id == "");
			QUARK_ASSERT(_unresolved_identifier.empty() == false);
		}
		else{
			QUARK_ASSERT(false);
		}
		return true;
	}

	void typeid_t::swap(typeid_t& other){
		QUARK_ASSERT(other.check_invariant());
		QUARK_ASSERT(check_invariant());

		std::swap(_base_type, other._base_type);
		_parts.swap(other._parts);
		_unique_type_id.swap(other._unique_type_id);
		_unresolved_identifier.swap(other._unresolved_identifier);

		QUARK_ASSERT(other.check_invariant());
		QUARK_ASSERT(check_invariant());
	}

	std::string typeid_t::to_string() const {
		QUARK_ASSERT(check_invariant());

		if(_base_type == floyd::base_type::k_custom_type){
			QUARK_ASSERT(_unresolved_identifier != "");
			return /*"unresolved:" +*/ _unresolved_identifier;
		}
		else if(_base_type == floyd::base_type::k_struct){
			return "{[}" + _parts[0].to_string() + "}";
		}
		else if(_base_type == floyd::base_type::k_vector){
			return "[" + _parts[0].to_string() + "]";
		}
		else if(_base_type == floyd::base_type::k_function){
			auto s = _parts[0].to_string() + " (";

			//??? doesn't work when size() == 2, that is ONE argument.
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
	}

	typeid_t typeid_t::from_string(const std::string& s){
		const auto a = read_required_type_identifier2(seq_t(s));

		return a.first;
	}


	json_t typeid_to_json(const typeid_t& t){
		if(t._parts.empty() && t._unique_type_id.empty()){
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
				{ "struct_def_id", t._unique_type_id.empty() == false ? t._unique_type_id : json_t() }
			});
		}
	}


	QUARK_UNIT_TESTQ("typeid_t{}", ""){
	}







	////////////////////////			member_t


	member_t::member_t(const floyd::typeid_t& type, const std::string& name) :
		_type(type),
		_name(name)
	{
		QUARK_ASSERT(type._base_type != floyd::base_type::k_null && type.check_invariant());
		QUARK_ASSERT(name.size() > 0);

		QUARK_ASSERT(check_invariant());
	}

	member_t::member_t(const floyd::typeid_t& type, const std::shared_ptr<value_t>& value, const std::string& name) :
		_type(type),
		_value(value),
		_name(name)
	{
		QUARK_ASSERT(type._base_type != floyd::base_type::k_null && type.check_invariant());
		QUARK_ASSERT(name.size() > 0);

		QUARK_ASSERT(check_invariant());
	}

	bool member_t::check_invariant() const{
		QUARK_ASSERT(_type._base_type != floyd::base_type::k_null && _type.check_invariant());
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




	std::vector<floyd::typeid_t> get_member_types(const std::vector<member_t>& m){
		vector<floyd::typeid_t> r;
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

	//////////////////////////////////////////////////		struct_definition_t

	bool struct_definition_t::check_invariant() const{
		QUARK_ASSERT(_struct_type._base_type != floyd::base_type::k_null && _struct_type.check_invariant());

		for(const auto m: _members){
			QUARK_ASSERT(m.check_invariant());
		}
		return true;
	}

	bool struct_definition_t::operator==(const struct_definition_t& other) const{
		QUARK_ASSERT(check_invariant());
		QUARK_ASSERT(other.check_invariant());

		return _struct_type == other._struct_type && _members == other._members;
	}

	json_t struct_definition_t::to_json() const {
//		floyd::typeid_t function_type = get_function_type(*this);
		return json_t::make_array({
			"struct-def",
			members_to_json(_members)
		});
	}



}
