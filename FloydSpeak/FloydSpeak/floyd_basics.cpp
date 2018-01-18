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
#include "json_parser.h"
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

	{ expression_type::k_lookup_element, "[]" }
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

	QUARK_UNIT_TESTQ("typeid_t", "null"){
		QUARK_UT_VERIFY(typeid_t::make_null().is_null());
	}

	QUARK_UNIT_TESTQ("typeid_t", "string"){
		QUARK_UT_VERIFY(typeid_t::make_string().get_base_type() == base_type::k_string);
	}

	QUARK_UNIT_TESTQ("typeid_t", "unknown_identifier"){
		QUARK_UT_VERIFY(typeid_t::make_unknown_identifier("hello").get_base_type() == base_type::k_unknown_identifier);
	}



	//////////////////////////////////////		FORMATS





	json_t typeid_to_normalized_json(const typeid_t& t){
		QUARK_ASSERT(t.check_invariant());

		const auto b = t.get_base_type();
		const auto basetype_str = base_type_to_string(b);

		if(b == base_type::k_null || b == base_type::k_bool || b == base_type::k_int || b == base_type::k_float || b == base_type::k_string){
			return basetype_str;
		}
		else if(b == base_type::k_typeid){
			const auto typeid2 = t.get_typeid_typeid();
			return json_t::make_array({
				json_t(basetype_str),
				typeid_to_normalized_json(typeid2)
			});
		}
		else if(b == base_type::k_struct){
			const auto struct_def = t.get_struct();
			return json_t::make_array({
				json_t(basetype_str),
				typeid_to_normalized_json(struct_def)
			});
		}
		else if(b == base_type::k_vector){
			const auto d = t.get_vector_element_type();
			return json_t::make_array({
				json_t(basetype_str),
				typeid_to_normalized_json(d)
			});
		}
		else if(b == base_type::k_function){
			return json_t::make_array({
				basetype_str,
				typeid_to_normalized_json(t.get_function_return()),
				typeids_to_json_array(t.get_function_args())
			});
		}
		else if(b == base_type::k_unknown_identifier){
			return t.get_unknown_identifier();
		}
		else{
			QUARK_ASSERT(false);
		}
	}

	typeid_t typeid_from_normalized_json(const json_t& t){
		if(t.is_string()){
			const auto s = t.get_string();
			if(s == "null"){
				return typeid_t::make_null();
			}
			else if(s == "bool"){
				return typeid_t::make_bool();
			}
			else if(s == "int"){
				return typeid_t::make_int();
			}
			else if(s == "float"){
				return typeid_t::make_float();
			}
			else if(s == "string"){
				return typeid_t::make_string();
			}
			else{
				return typeid_t::make_unknown_identifier(s);
			}
		}
		else if(t.is_array()){
			const auto a = t.get_array();
			const auto s = a[0].get_string();
			if(s == "typeid"){
				const auto t2 = typeid_from_normalized_json(a[1]);
				return typeid_t::make_typeid(t2);
			}
			else if(s == "struct"){
				const auto struct_def_array = a[1].get_array();
				const auto struct_name = struct_def_array[0].get_string();
				const auto member_array = struct_def_array[1].get_array();

				const vector<member_t> struct_members = members_from_json(member_array);
				return typeid_t::make_struct(
					std::make_shared<struct_definition_t>(struct_definition_t(struct_name, struct_members))
				);
			}
			else if(s == "vector"){
				const auto element_type = typeid_from_normalized_json(a[1]);
				return typeid_t::make_vector(element_type);
			}
			else if(s == "function"){
				const auto ret_type = typeid_from_normalized_json(a[1]);
				const auto arg_types_array = a[2].get_array();
				const vector<typeid_t> arg_types = typeids_from_json_array(arg_types_array);
				return typeid_t::make_function(ret_type, arg_types);
			}
			else if(s == "unknown-identifier"){
				QUARK_ASSERT(false);
				return typeid_t::make_null();
			}
			else {
				QUARK_ASSERT(false);
				return typeid_t::make_null();
			}
		}
		else{
			QUARK_ASSERT(false);
			return typeid_t::make_null();
		}
	}

	std::string typeid_to_compact_string(const typeid_t& t){
		QUARK_ASSERT(t.check_invariant());

		const auto basetype = t.get_base_type();

		if(basetype == floyd::base_type::k_unknown_identifier){
			return t.get_unknown_identifier();
		}
		else if(basetype == floyd::base_type::k_typeid){
			const auto t2 = t.get_typeid_typeid();
			return "typeid(" + typeid_to_compact_string(t2) + ")";
		}
		else if(basetype == floyd::base_type::k_struct){
			const auto struct_def = t.get_struct();
			return floyd::to_compact_string(struct_def);
		}
		else if(basetype == floyd::base_type::k_vector){
			const auto e = t.get_vector_element_type();
			return "[" + typeid_to_compact_string(e) + "]";
		}
		else if(basetype == floyd::base_type::k_function){
			const auto ret = t.get_function_return();
			const auto args = t.get_function_args();

			vector<string> args_str;
			for(const auto a: args){
				args_str.push_back(typeid_to_compact_string(a));
			}

			return string() + "function " + typeid_to_compact_string(ret) + "(" + concat_strings_with_divider(args_str, ",") + ")";
		}
		else{
			return base_type_to_string(basetype);
		}
	}




	struct typeid_str_test_t {
		typeid_t _typeid;
		string _normalized_json;
		string _compact_str;
	};


	const vector<typeid_str_test_t> make_typeid_str_tests(){
		const auto s1 = typeid_t::make_struct(
			std::make_shared<struct_definition_t>(struct_definition_t("file", {}))
		);

		const auto tests = vector<typeid_str_test_t>{
			{ typeid_t::make_null(), "\"null\"", "null" },
			{ typeid_t::make_bool(), "\"bool\"", "bool" },
			{ typeid_t::make_int(), "\"int\"", "int" },
			{ typeid_t::make_float(), "\"float\"", "float" },
			{ typeid_t::make_string(), "\"string\"", "string" },

			//	Typeid
			{ typeid_t::make_typeid(typeid_t::make_float()), R"([ "typeid" , "float"])", "typeid(float)" },
			{ typeid_t::make_typeid(typeid_t::make_string()), R"([ "typeid" , "string"])", "typeid(string)" },
			{
				typeid_t::make_typeid(s1),
				R"(
					[
						"typeid",
						["struct", ["file", []]]
					]
				)",
				"typeid(struct file {})"
			},

			//	Struct
			{ s1, R"(["struct", ["file", []]])", "struct file {}" },
			{
				typeid_t::make_struct(
					std::make_shared<struct_definition_t>(struct_definition_t(
						"file",
						vector<member_t>{
							member_t(typeid_t::make_int(), "a"),
							member_t(typeid_t::make_float(), "b")
						}
					))
				),
				R"(["struct", ["file", [{ "type": "int", "name": "a"}, {"type": "float", "name": "b"}]]])",
				"struct file {int a;float b;}"
			},


			//	Function
			{
				typeid_t::make_function(typeid_t::make_bool(), vector<typeid_t>{ typeid_t::make_int(), typeid_t::make_float() }),
				R"(["function", "bool", [ "int", "float"]])",
				"function bool(int,float)"
			},



			//	unknown_identifier
			{ typeid_t::make_unknown_identifier("hello"), "\"hello\"", "hello" }
		};
		return tests;
	}


	QUARK_UNIT_TESTQ("typeid_to_normalized_json()", ""){
		const auto f = make_typeid_str_tests();
		for(int i = 0 ; i < f.size() ; i++){
			const auto start_typeid = f[i]._typeid;
			const auto expected_normalized_json = parse_json(seq_t(f[i]._normalized_json)).first;

			//	Test typeid_to_normalized_json().
			const auto result1 = typeid_to_normalized_json(start_typeid);
			ut_compare_jsons(result1, expected_normalized_json);
		}
	}


	QUARK_UNIT_TESTQ("typeid_from_normalized_json", ""){
		const auto f = make_typeid_str_tests();
		for(int i = 0 ; i < f.size() ; i++){
			const auto start_typeid = f[i]._typeid;
			const auto expected_normalized_json = parse_json(seq_t(f[i]._normalized_json)).first;

 			//	Test typeid_from_normalized_json();
 			const auto result2 = typeid_from_normalized_json(expected_normalized_json);
			QUARK_UT_VERIFY(result2 == start_typeid);
		}
		QUARK_TRACE("OK!");
	}


	QUARK_UNIT_TESTQ("typeid_to_compact_string", ""){
		const auto f = make_typeid_str_tests();
		for(int i = 0 ; i < f.size() ; i++){
			const auto start_typeid = f[i]._typeid;

			//	Test typeid_to_compact_string().
			const auto result3 = typeid_to_compact_string(start_typeid);
			quark::ut_compare(result3, f[i]._compact_str);
		}
		QUARK_TRACE("OK!");
	}





	////////////////////////			member_t


	member_t::member_t(const floyd::typeid_t& type, const std::string& name) :
		_type(type),
		_name(name)
	{
		QUARK_ASSERT(!type.is_null() && type.check_invariant());
		QUARK_ASSERT(name.size() > 0);

		QUARK_ASSERT(check_invariant());
	}

	bool member_t::check_invariant() const{
		QUARK_ASSERT(!_type.is_null() && _type.check_invariant());
		QUARK_ASSERT(_name.size() > 0);
		return true;
	}

	bool member_t::operator==(const member_t& other) const{
		QUARK_ASSERT(check_invariant());
		QUARK_ASSERT(other.check_invariant());

		return (_type == other._type) && (_name == other._name);
	}

	void trace(const member_t& member){
		QUARK_TRACE("<member> type: <" + typeid_to_compact_string(member._type) + "> name: \"" + member._name + "\"");
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
				{ "type", typeid_to_normalized_json(i._type) },
				{ "name", json_t(i._name) }
			});
			r.push_back(json_t(member));
		}
		return r;
	}

	std::vector<member_t> members_from_json(const json_t& members){
		QUARK_ASSERT(members.check_invariant());

		std::vector<member_t> r;
		for(const auto i: members.get_array()){
			const auto m = member_t(
				typeid_from_normalized_json(i.get_object_element("type")),
				i.get_object_element("name").get_string()
			);

			r.push_back(m);
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


	std::vector<json_t> typeids_to_json_array(const std::vector<typeid_t>& m){
		vector<json_t> r;
		for(const auto a: m){
			r.push_back(typeid_to_normalized_json(a));
		}
		return r;
	}
	std::vector<typeid_t> typeids_from_json_array(const std::vector<json_t>& m){
		vector<typeid_t> r;
		for(const auto a: m){
			r.push_back(typeid_from_normalized_json(a));
		}
		return r;
	}



	//////////////////////////////////////////////////		struct_definition_t




	bool struct_definition_t::check_invariant() const{
//		QUARK_ASSERT(_struct!type.is_null() && _struct_type.check_invariant());

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

	std::string to_compact_string(const struct_definition_t& v){
		auto s = "struct " + v._name + " {";
		for(const auto e: v._members){
			s = s + typeid_to_compact_string(e._type) + " " + e._name + ";";
		}
		s = s + "}";
		return s;
	}

	json_t typeid_to_normalized_json(const struct_definition_t& v){
		QUARK_ASSERT(v.check_invariant());

		return json_t::make_array({
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
