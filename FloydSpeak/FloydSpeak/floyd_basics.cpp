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

		else if(t == base_type::k_typeid){
			return "typeid";
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
		else if(t == base_type::k_unknown_identifier){
			return "unknown-identifier";
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
		QUARK_TEST_VERIFY(base_type_to_string(base_type::k_typeid) == "typeid");
		QUARK_TEST_VERIFY(base_type_to_string(base_type::k_struct) == "struct");
		QUARK_TEST_VERIFY(base_type_to_string(base_type::k_vector) == "vector");
		QUARK_TEST_VERIFY(base_type_to_string(base_type::k_function) == "function");
		QUARK_TEST_VERIFY(base_type_to_string(base_type::k_unknown_identifier) == "unknown-identifier");
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



	void trace(const typeid_t& t, const std::string& label){
		QUARK_ASSERT(t.check_invariant());
		json_to_pretty_string(typeid_to_json(t));
	}

	bool typeid_t::check_invariant() const{
		if(_base_type == floyd::base_type::k_null){
			QUARK_ASSERT(_parts.empty());
			QUARK_ASSERT(_unique_type_id.empty());
			QUARK_ASSERT(_unknown_identifier.empty());
			QUARK_ASSERT(!_struct_def);
		}
		else if(_base_type == floyd::base_type::k_bool){
			QUARK_ASSERT(_parts.empty());
			QUARK_ASSERT(_unique_type_id.empty());
			QUARK_ASSERT(_unknown_identifier.empty());
			QUARK_ASSERT(!_struct_def);
		}
		else if(_base_type == floyd::base_type::k_int){
			QUARK_ASSERT(_parts.empty());
			QUARK_ASSERT(_unique_type_id.empty());
			QUARK_ASSERT(_unknown_identifier.empty());
			QUARK_ASSERT(!_struct_def);
		}
		else if(_base_type == floyd::base_type::k_float){
			QUARK_ASSERT(_parts.empty());
			QUARK_ASSERT(_unique_type_id.empty());
			QUARK_ASSERT(_unknown_identifier.empty());
			QUARK_ASSERT(!_struct_def);
		}
		else if(_base_type == floyd::base_type::k_string){
			QUARK_ASSERT(_parts.empty());
			QUARK_ASSERT(_unique_type_id.empty());
			QUARK_ASSERT(_unknown_identifier.empty());
			QUARK_ASSERT(!_struct_def);
		}
		else if(_base_type == floyd::base_type::k_typeid){
			QUARK_ASSERT(_parts.size() == 1);
			QUARK_ASSERT(_parts[0].check_invariant());
			QUARK_ASSERT(_unique_type_id.empty());
			QUARK_ASSERT(_unknown_identifier.empty());
			QUARK_ASSERT(!_struct_def);
		}
		else if(_base_type == floyd::base_type::k_struct){
			QUARK_ASSERT(_parts.empty() == true);
//			QUARK_ASSERT(_unique_type_id.empty() == false);
			QUARK_ASSERT(_unknown_identifier.empty());
			QUARK_ASSERT(_struct_def);
			QUARK_ASSERT(_struct_def->check_invariant());
		}
		else if(_base_type == floyd::base_type::k_vector){
			QUARK_ASSERT(_parts.size() == 1);
			QUARK_ASSERT(_unique_type_id.empty() == true);
			QUARK_ASSERT(_unknown_identifier.empty());
			QUARK_ASSERT(!_struct_def);
		}
		else if(_base_type == floyd::base_type::k_function){
			QUARK_ASSERT(_parts.empty() == false);
			QUARK_ASSERT(_parts[0].check_invariant());
			QUARK_ASSERT(_unique_type_id.empty() == true);
			QUARK_ASSERT(_unknown_identifier.empty());
			QUARK_ASSERT(!_struct_def);
		}
		else if(_base_type == floyd::base_type::k_unknown_identifier){
			QUARK_ASSERT(_parts.empty());
			QUARK_ASSERT(_unique_type_id == "");
			QUARK_ASSERT(_unknown_identifier.empty() == false);
			QUARK_ASSERT(!_struct_def);
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
		_unknown_identifier.swap(other._unknown_identifier);
		_struct_def.swap(other._struct_def);

		QUARK_ASSERT(other.check_invariant());
		QUARK_ASSERT(check_invariant());
	}


	std::string typeid_t::to_string2() const {
		QUARK_ASSERT(check_invariant());

		if(_base_type == floyd::base_type::k_unknown_identifier){
//			return "[unknown-identifier:" + _unknown_identifier + "]";
			return _unknown_identifier;
		}
		else if(_base_type == floyd::base_type::k_typeid){
			return "typeid(" + _parts[0].to_string2() + ")";
		}
		else if(_base_type == floyd::base_type::k_struct){
			return floyd::to_string(*_struct_def);
		}
		else if(_base_type == floyd::base_type::k_vector){
			return "[" + _parts[0].to_string2() + "]";
		}
		else if(_base_type == floyd::base_type::k_function){
			auto s = _parts[0].to_string2() + "(";
			if(_parts.size() > 1){
				for(int i = 1 ; i < _parts.size() - 1 ; i++){
					s = s + _parts[i].to_string2() + ",";
				}
				s = s + _parts[_parts.size() - 1].to_string2();
			}
			s = s + ")";
			return s;
		}
		else{
			return base_type_to_string(_base_type);
		}
	}

	typeid_t typeid_t::from_string(const std::string& s){
		const auto a = read_required_type(seq_t(s));

		return a.first;
	}


	json_t typeid_to_json(const typeid_t& t){
		QUARK_ASSERT(t.check_invariant());

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

/*
	json_t typeid_to_json(const typeid_t& t){
		const auto s = t.to_string();
		return json_t::make_array({
			json_t("typeid"),
			json_t(s)
		});
	}
*/


/*
TODO

		//	Supports non-lossy round trip between to_string() and from_string(). ??? make it so and test!
		//	Compatible with Floyd sources.
			### Store as compact JSON instead? Then we can't use [ and {".

			//??? How to encode typeid in floyd source code?
//??? Remove concept of typeid_t make_unknown_identifier, instead use typeid_t OR identifier-string.
*/

	QUARK_UNIT_TESTQ("typeid_t", "null"){
		QUARK_UT_VERIFY(typeid_t::make_null().is_null());
	}


	QUARK_UNIT_TESTQ("typeid_t", "string"){
		QUARK_UT_VERIFY(typeid_t::make_string().get_base_type() == base_type::k_string);
	}


	QUARK_UNIT_TESTQ("typeid_t", "unknown_identifier"){
		QUARK_UT_VERIFY(typeid_t::make_unknown_identifier("hello").get_base_type() == base_type::k_unknown_identifier);
	}




	QUARK_UNIT_TESTQ("to_string2", ""){
		QUARK_UT_VERIFY(typeid_t::make_null().to_string2() == "null");
	}
	QUARK_UNIT_TESTQ("to_string2", ""){
		QUARK_UT_VERIFY(typeid_t::make_bool().to_string2() == "bool");
	}
	QUARK_UNIT_TESTQ("to_string2", ""){
		QUARK_UT_VERIFY(typeid_t::make_string().to_string2() == "string");
	}
	QUARK_UNIT_TESTQ("to_string2", ""){
		const auto struct_def = std::make_shared<struct_definition_t>(struct_definition_t("file", {}));
		QUARK_UT_VERIFY(typeid_t::make_struct(struct_def).to_string2() == "struct file {}");
	}
	QUARK_UNIT_TESTQ("to_string2", ""){
		QUARK_UT_VERIFY(typeid_t::make_unknown_identifier("hello").to_string2() == "hello");
//		QUARK_UT_VERIFY(typeid_t::make_unknown_identifier("hello").to_string() == "[unknown-identifier:hello]");
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

	bool member_t::check_invariant() const{
		QUARK_ASSERT(_type._base_type != floyd::base_type::k_null && _type.check_invariant());
		QUARK_ASSERT(_name.size() > 0);
		return true;
	}

	bool member_t::operator==(const member_t& other) const{
		QUARK_ASSERT(check_invariant());
		QUARK_ASSERT(other.check_invariant());

		return (_type == other._type) && (_name == other._name);
	}


	void trace(const member_t& member){
		QUARK_TRACE("<member> type: <" + member._type.to_string2() + "> name: \"" + member._name + "\"");
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
				{ "type", typeid_to_ast_json(i._type) },
				{ "name", json_t(i._name) }
			});
			r.push_back(json_t(member));
		}
		return r;
	}


	json_t values_to_json_array(const std::vector<value_t>& values){
		std::vector<json_t> r;
		for(const auto i: values){
			const auto j = value_to_json(i);
			r.push_back(j);
		}
		return json_t::make_array(r);
	}




	//////////////////////////////////////////////////		struct_definition_t




	bool struct_definition_t::check_invariant() const{
//		QUARK_ASSERT(_struct_type._base_type != floyd::base_type::k_null && _struct_type.check_invariant());

		for(const auto m: _members){
			QUARK_ASSERT(m.check_invariant());
		}
		return true;
	}

	bool struct_definition_t::operator==(const struct_definition_t& other) const{
		QUARK_ASSERT(check_invariant());
		QUARK_ASSERT(other.check_invariant());

		return _name == other._name && _members == other._members;
	}

	std::string to_string(const struct_definition_t& v){
		auto s = "struct " + v._name + " {";
		for(const auto e: v._members){
			s = s + e._type.to_string2() + " " + e._name + ",";
		}
		if(s.back() == ','){
			s.pop_back();
		}
		s = s + "}";
		return s;
	}

	json_t to_json(const struct_definition_t& v){
		QUARK_ASSERT(v.check_invariant());

		return json_t::make_array({
			"struct-def",
			json_t(v._name),
			members_to_json(v._members)
		});
	}


	int find_struct_member_index(const struct_definition_t& def, const std::string& name){
			int index = 0;
			while(index < def._members.size() && def._members[index]._name != name){
				index++;
			}
			if(index == def._members.size()){
				return -1;
			}
			else{
				return index;
			}
	}


}
