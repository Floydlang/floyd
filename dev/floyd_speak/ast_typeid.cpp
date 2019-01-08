//
//  ast_typeid.cpp
//  FloydSpeak
//
//  Created by Marcus Zetterquist on 2018-02-12.
//  Copyright © 2018 Marcus Zetterquist. All rights reserved.
//

#include "ast_typeid.h"

#include "floyd_parser/parser_primitives.h"
#include "floyd_basics.h"
#include "json_parser.h"
#include "utils.h"
#include "ast_typeid_helpers.h"

using std::string;
using std::vector;







namespace floyd {


#if 0
	struct typeid2_wrapper_t;

	typedef std::variant<
		void,
		void,

		void,

		void,
		void,
		void,
		void,
		void,

		void,

		std::shared_ptr<const struct_definition_t>,
		std::shared_ptr<const protocol_definition_t>,

		//	vector, element-type
		std::shared_ptr<typeid2_wrapper_t>
	> typeid2_t;


	struct typeid2_wrapper_t {
		typeid2_t t;
	};
#endif



//////////////////////////////////////////////////		base_type


string base_type_to_string(const base_type t){
	if(t == base_type::k_internal_undefined){
		return keyword_t::k_internal_undefined;
	}
	else if(t == base_type::k_internal_dynamic){
		return keyword_t::k_internal_dynamic;
	}

	else if(t == base_type::k_void){
		return keyword_t::k_void;
	}
	else if(t == base_type::k_bool){
		return keyword_t::k_bool;
	}
	else if(t == base_type::k_int){
		return keyword_t::k_int;
	}
	else if(t == base_type::k_double){
		return keyword_t::k_double;
	}
	else if(t == base_type::k_string){
		return keyword_t::k_string;
	}
	else if(t == base_type::k_json_value){
		return keyword_t::k_json_value;
	}

	else if(t == base_type::k_typeid){
		return keyword_t::k_typeid;
	}

	else if(t == base_type::k_struct){
		return keyword_t::k_struct;
	}
	else if(t == base_type::k_protocol){
		return keyword_t::k_protocol;
	}
	else if(t == base_type::k_vector){
		return "vector";
	}
	else if(t == base_type::k_dict){
		return "dict";
	}
	else if(t == base_type::k_function){
		return "fun";
	}
	else if(t == base_type::k_internal_unresolved_type_identifier){
		return "**unknown-identifier**";
	}
	else{
		QUARK_ASSERT(false);
		throw std::exception();
	}
}


//////////////////////////////////////		base_type_to_string(base_type)


QUARK_UNIT_TESTQ("base_type_to_string(base_type)", ""){
	QUARK_TEST_VERIFY(base_type_to_string(base_type::k_internal_undefined) == "**undef**");
	QUARK_TEST_VERIFY(base_type_to_string(base_type::k_internal_dynamic) == "**dyn**");
	QUARK_TEST_VERIFY(base_type_to_string(base_type::k_void) == keyword_t::k_void);

	QUARK_TEST_VERIFY(base_type_to_string(base_type::k_bool) == keyword_t::k_bool);
	QUARK_TEST_VERIFY(base_type_to_string(base_type::k_int) == keyword_t::k_int);
	QUARK_TEST_VERIFY(base_type_to_string(base_type::k_double) == keyword_t::k_double);
	QUARK_TEST_VERIFY(base_type_to_string(base_type::k_string) == keyword_t::k_string);
	QUARK_TEST_VERIFY(base_type_to_string(base_type::k_json_value) == keyword_t::k_json_value);

	QUARK_TEST_VERIFY(base_type_to_string(base_type::k_typeid) == "typeid");

	QUARK_TEST_VERIFY(base_type_to_string(base_type::k_struct) == keyword_t::k_struct);
	QUARK_TEST_VERIFY(base_type_to_string(base_type::k_protocol) == keyword_t::k_protocol);
	QUARK_TEST_VERIFY(base_type_to_string(base_type::k_vector) == "vector");
	QUARK_TEST_VERIFY(base_type_to_string(base_type::k_dict) == "dict");
	QUARK_TEST_VERIFY(base_type_to_string(base_type::k_function) == "fun");

	QUARK_TEST_VERIFY(base_type_to_string(base_type::k_internal_unresolved_type_identifier) == "**unknown-identifier**");
}


//////////////////////////////////////////////////		typeid_t


bool typeid_t::check_invariant() const{
//		QUARK_ASSERT(_DEBUG != "");

	if(_base_type == floyd::base_type::k_internal_undefined){
		QUARK_ASSERT(!_ext);
	}
	else if(_base_type == floyd::base_type::k_internal_dynamic){
		QUARK_ASSERT(!_ext);
	}

	else if(_base_type == floyd::base_type::k_void){
		QUARK_ASSERT(!_ext);
	}
	else if(_base_type == floyd::base_type::k_bool){
		QUARK_ASSERT(!_ext);
	}
	else if(_base_type == floyd::base_type::k_int){
		QUARK_ASSERT(!_ext);
	}
	else if(_base_type == floyd::base_type::k_double){
		QUARK_ASSERT(!_ext);
	}
	else if(_base_type == floyd::base_type::k_string){
		QUARK_ASSERT(!_ext);
	}
	else if(_base_type == floyd::base_type::k_json_value){
		QUARK_ASSERT(!_ext);
	}
	else if(_base_type == floyd::base_type::k_typeid){
		QUARK_ASSERT(!_ext);
	}
	else if(_base_type == floyd::base_type::k_struct){
		QUARK_ASSERT(_ext);
		QUARK_ASSERT(_ext->_parts.empty());
		QUARK_ASSERT(_ext->_unresolved_type_identifier.empty());
		QUARK_ASSERT(_ext->_struct_def && _ext->_struct_def->check_invariant());
		QUARK_ASSERT(!_ext->_protocol_def);
	}
	else if(_base_type == floyd::base_type::k_protocol){
		QUARK_ASSERT(_ext);
		QUARK_ASSERT(_ext->_parts.empty());
		QUARK_ASSERT(_ext->_unresolved_type_identifier.empty());
		QUARK_ASSERT(!_ext->_struct_def);
		QUARK_ASSERT(_ext->_protocol_def && _ext->_protocol_def->check_invariant());
	}
	else if(_base_type == floyd::base_type::k_vector){
		QUARK_ASSERT(_ext);
		QUARK_ASSERT(_ext->_parts.size() == 1);
		QUARK_ASSERT(_ext->_unresolved_type_identifier.empty());
		QUARK_ASSERT(!_ext->_struct_def);
		QUARK_ASSERT(!_ext->_protocol_def);

		QUARK_ASSERT(_ext->_parts[0].check_invariant());
	}
	else if(_base_type == floyd::base_type::k_dict){
		QUARK_ASSERT(_ext);
		QUARK_ASSERT(_ext->_parts.size() == 1);
		QUARK_ASSERT(_ext->_unresolved_type_identifier.empty());
		QUARK_ASSERT(!_ext->_struct_def);
		QUARK_ASSERT(!_ext->_protocol_def);

		QUARK_ASSERT(_ext->_parts[0].check_invariant());
	}
	else if(_base_type == floyd::base_type::k_function){
		QUARK_ASSERT(_ext);
		QUARK_ASSERT(_ext->_parts.size() >= 1);
		QUARK_ASSERT(_ext->_unresolved_type_identifier.empty());
		QUARK_ASSERT(!_ext->_struct_def);
		QUARK_ASSERT(!_ext->_protocol_def);

		for(const auto& e: _ext->_parts){
			QUARK_ASSERT(e.check_invariant());
		}
	}
	else if(_base_type == floyd::base_type::k_internal_unresolved_type_identifier){
		QUARK_ASSERT(_ext);
		QUARK_ASSERT(_ext->_parts.empty());
		QUARK_ASSERT(_ext->_unresolved_type_identifier.empty() == false);
		QUARK_ASSERT(!_ext->_struct_def);
		QUARK_ASSERT(!_ext->_protocol_def);
	}
	else{
		QUARK_ASSERT(false);
	}
	return true;
}

void typeid_t::swap(typeid_t& other){
	QUARK_ASSERT(other.check_invariant());
	QUARK_ASSERT(check_invariant());

#if DEBUG
	std::swap(_DEBUG, other._DEBUG);
#endif
	std::swap(_base_type, other._base_type);
	_ext.swap(other._ext);

	QUARK_ASSERT(other.check_invariant());
	QUARK_ASSERT(check_invariant());
}



QUARK_UNIT_TESTQ("typeid_t", "make_undefined()"){
	QUARK_UT_VERIFY(typeid_t::make_undefined().get_base_type() == base_type::k_internal_undefined);
}
QUARK_UNIT_TESTQ("typeid_t", "is_undefined()"){
	QUARK_UT_VERIFY(typeid_t::make_undefined().is_undefined() == true);
}
QUARK_UNIT_TESTQ("typeid_t", "is_undefined()"){
	QUARK_UT_VERIFY(typeid_t::make_bool().is_undefined() == false);
}

QUARK_UNIT_TESTQ("typeid_t", "make_internal_dynamic()"){
	QUARK_UT_VERIFY(typeid_t::make_internal_dynamic().get_base_type() == base_type::k_internal_dynamic);
}
QUARK_UNIT_TESTQ("typeid_t", "is_internal_dynamic()"){
	QUARK_UT_VERIFY(typeid_t::make_internal_dynamic().is_internal_dynamic() == true);
}
QUARK_UNIT_TESTQ("typeid_t", "is_internal_dynamic()"){
	QUARK_UT_VERIFY(typeid_t::make_bool().is_internal_dynamic() == false);
}

QUARK_UNIT_TESTQ("typeid_t", "make_void()"){
	QUARK_UT_VERIFY(typeid_t::make_void().get_base_type() == base_type::k_void);
}
QUARK_UNIT_TESTQ("typeid_t", "is_void()"){
	QUARK_UT_VERIFY(typeid_t::make_void().is_void() == true);
}
QUARK_UNIT_TESTQ("typeid_t", "is_void()"){
	QUARK_UT_VERIFY(typeid_t::make_bool().is_void() == false);
}

QUARK_UNIT_TESTQ("typeid_t", "make_bool()"){
	QUARK_UT_VERIFY(typeid_t::make_bool().get_base_type() == base_type::k_bool);
}
QUARK_UNIT_TESTQ("typeid_t", "is_bool()"){
	QUARK_UT_VERIFY(typeid_t::make_bool().is_bool() == true);
}
QUARK_UNIT_TESTQ("typeid_t", "is_bool()"){
	QUARK_UT_VERIFY(typeid_t::make_void().is_bool() == false);
}

QUARK_UNIT_TESTQ("typeid_t", "make_int()"){
	QUARK_UT_VERIFY(typeid_t::make_int().get_base_type() == base_type::k_int);
}
QUARK_UNIT_TESTQ("typeid_t", "is_int()"){
	QUARK_UT_VERIFY(typeid_t::make_int().is_int() == true);
}
QUARK_UNIT_TESTQ("typeid_t", "is_int()"){
	QUARK_UT_VERIFY(typeid_t::make_bool().is_int() == false);
}


QUARK_UNIT_TESTQ("typeid_t", "make_double()"){
	QUARK_UT_VERIFY(typeid_t::make_double().is_double());
}
QUARK_UNIT_TESTQ("typeid_t", "is_double()"){
	QUARK_UT_VERIFY(typeid_t::make_double().is_double() == true);
}
QUARK_UNIT_TESTQ("typeid_t", "is_double()"){
	QUARK_UT_VERIFY(typeid_t::make_bool().is_double() == false);
}


QUARK_UNIT_TESTQ("typeid_t", "make_string()"){
	QUARK_UT_VERIFY(typeid_t::make_string().get_base_type() == base_type::k_string);
}
QUARK_UNIT_TESTQ("typeid_t", "is_string()"){
	QUARK_UT_VERIFY(typeid_t::make_string().is_string() == true);
}
QUARK_UNIT_TESTQ("typeid_t", "is_string()"){
	QUARK_UT_VERIFY(typeid_t::make_bool().is_string() == false);
}


QUARK_UNIT_TESTQ("typeid_t", "make_json_value()"){
	QUARK_UT_VERIFY(typeid_t::make_json_value().get_base_type() == base_type::k_json_value);
}
QUARK_UNIT_TESTQ("typeid_t", "is_json_value()"){
	QUARK_UT_VERIFY(typeid_t::make_json_value().is_json_value() == true);
}
QUARK_UNIT_TESTQ("typeid_t", "is_json_value()"){
	QUARK_UT_VERIFY(typeid_t::make_bool().is_json_value() == false);
}


QUARK_UNIT_TESTQ("typeid_t", "make_typeid()"){
	QUARK_UT_VERIFY(typeid_t::make_typeid().get_base_type() == base_type::k_typeid);
}
QUARK_UNIT_TESTQ("typeid_t", "is_typeid()"){
	QUARK_UT_VERIFY(typeid_t::make_typeid().is_typeid() == true);
}
QUARK_UNIT_TESTQ("typeid_t", "is_typeid()"){
	QUARK_UT_VERIFY(typeid_t::make_bool().is_typeid() == false);
}



QUARK_UNIT_TESTQ("typeid_t", "make_struct2()"){
	QUARK_UT_VERIFY(typeid_t::make_struct2({}).get_base_type() == base_type::k_struct);
}
QUARK_UNIT_TESTQ("typeid_t", "is_struct()"){
	QUARK_UT_VERIFY(typeid_t::make_struct2({}).is_struct() == true);
}
QUARK_UNIT_TESTQ("typeid_t", "is_struct()"){
	QUARK_UT_VERIFY(typeid_t::make_bool().is_struct() == false);
}


QUARK_UNIT_TESTQ("typeid_t", "make_struct2()"){
	QUARK_UT_VERIFY(typeid_t::make_struct2({}).get_base_type() == base_type::k_struct);
}
QUARK_UNIT_TESTQ("typeid_t", "is_struct()"){
	QUARK_UT_VERIFY(typeid_t::make_struct2({}).is_struct() == true);
}
QUARK_UNIT_TESTQ("typeid_t", "is_struct()"){
	QUARK_UT_VERIFY(typeid_t::make_bool().is_struct() == false);
}


QUARK_UNIT_TESTQ("typeid_t", "make_protocol()"){
	QUARK_UT_VERIFY(typeid_t::make_protocol({}).get_base_type() == base_type::k_protocol);
}
QUARK_UNIT_TESTQ("typeid_t", "is_protocol()"){
	QUARK_UT_VERIFY(typeid_t::make_protocol({}).is_protocol() == true);
}
QUARK_UNIT_TESTQ("typeid_t", "is_protocol()"){
	QUARK_UT_VERIFY(typeid_t::make_bool().is_protocol() == false);
}



QUARK_UNIT_TESTQ("typeid_t", "make_vector()"){
	QUARK_UT_VERIFY(typeid_t::make_vector(typeid_t::make_int()).get_base_type() == base_type::k_vector);
}
QUARK_UNIT_TESTQ("typeid_t", "is_vector()"){
	QUARK_UT_VERIFY(typeid_t::make_vector(typeid_t::make_int()).is_vector() == true);
}
QUARK_UNIT_TESTQ("typeid_t", "is_vector()"){
	QUARK_UT_VERIFY(typeid_t::make_bool().is_vector() == false);
}


QUARK_UNIT_TESTQ("typeid_t", "make_dict()"){
	QUARK_UT_VERIFY(typeid_t::make_dict(typeid_t::make_int()).get_base_type() == base_type::k_dict);
}
QUARK_UNIT_TESTQ("typeid_t", "is_dict()"){
	QUARK_UT_VERIFY(typeid_t::make_dict(typeid_t::make_int()).is_dict() == true);
}
QUARK_UNIT_TESTQ("typeid_t", "is_dict()"){
	QUARK_UT_VERIFY(typeid_t::make_bool().is_dict() == false);
}



QUARK_UNIT_TESTQ("typeid_t", "make_function()"){
	QUARK_UT_VERIFY(typeid_t::make_function(typeid_t::make_void(), {}).get_base_type() == base_type::k_function);
}
QUARK_UNIT_TESTQ("typeid_t", "is_function()()"){
	QUARK_UT_VERIFY(typeid_t::make_function(typeid_t::make_void(), {}).is_function() == true);
}
QUARK_UNIT_TESTQ("typeid_t", "is_function()()"){
	QUARK_UT_VERIFY(typeid_t::make_bool().is_function() == false);
}




QUARK_UNIT_TESTQ("typeid_t", "make_unresolved_type_identifier()"){
	QUARK_UT_VERIFY(typeid_t::make_unresolved_type_identifier("xyz").get_base_type() == base_type::k_internal_unresolved_type_identifier);
}

QUARK_UNIT_TESTQ("typeid_t", "is_unresolved_type_identifier()()"){
	QUARK_UT_VERIFY(typeid_t::make_unresolved_type_identifier("xyz").is_unresolved_type_identifier() == true);
}
QUARK_UNIT_TESTQ("typeid_t", "is_unresolved_type_identifier()()"){
	QUARK_UT_VERIFY(typeid_t::make_bool().is_unresolved_type_identifier() == false);
}



//////////////////////////////////////		FORMATS


std::string typeid_to_compact_string_int(const typeid_t& t){
//		QUARK_ASSERT(t.check_invariant());

	const auto basetype = t.get_base_type();

	if(basetype == floyd::base_type::k_internal_unresolved_type_identifier){
		return std::string() + "•" + t.get_unresolved_type_identifier() + "•";
	}
/*
	else if(basetype == floyd::base_type::k_typeid){
		const auto t2 = t.get_typeid_typeid();
		return "typeid(" + typeid_to_compact_string(t2) + ")";
	}
*/
	else if(basetype == floyd::base_type::k_struct){
		const auto struct_def = t.get_struct();
		return floyd::to_compact_string(struct_def);
	}
	else if(basetype == floyd::base_type::k_protocol){
		const auto protocol_def = t.get_protocol();
		return floyd::to_compact_string(protocol_def);
	}
	else if(basetype == floyd::base_type::k_vector){
		const auto e = t.get_vector_element_type();
		return "[" + typeid_to_compact_string(e) + "]";
	}
	else if(basetype == floyd::base_type::k_dict){
		const auto e = t.get_dict_value_type();
		return "[string:" + typeid_to_compact_string(e) + "]";
	}
	else if(basetype == floyd::base_type::k_function){
		const auto ret = t.get_function_return();
		const auto args = t.get_function_args();

		vector<string> args_str;
		for(const auto& a: args){
			args_str.push_back(typeid_to_compact_string(a));
		}

		return string() + "function " + typeid_to_compact_string(ret) + "(" + concat_strings_with_divider(args_str, ",") + ")";
	}
	else{
		return base_type_to_string(basetype);
	}
}
std::string typeid_to_compact_string(const typeid_t& t){
	return typeid_to_compact_string_int(t);
//		return std::string() + "<" + typeid_to_compact_string_int(t) + ">";
}


struct typeid_str_test_t {
	typeid_t _typeid;
	string _ast_json;
	string _compact_str;
};


const vector<typeid_str_test_t> make_typeid_str_tests(){
	const auto s1 = typeid_t::make_struct2({});

	const auto tests = vector<typeid_str_test_t>{
		{ typeid_t::make_undefined(), quote(keyword_t::k_internal_undefined), keyword_t::k_internal_undefined },
		{ typeid_t::make_bool(), quote(keyword_t::k_bool), keyword_t::k_bool },
		{ typeid_t::make_int(), quote(keyword_t::k_int), keyword_t::k_int },
		{ typeid_t::make_double(), quote(keyword_t::k_double), keyword_t::k_double },
		{ typeid_t::make_string(), quote(keyword_t::k_string), keyword_t::k_string},

		//	Typeid
		{ typeid_t::make_typeid(), quote(keyword_t::k_typeid), keyword_t::k_typeid },
		{ typeid_t::make_typeid(), quote(keyword_t::k_typeid), keyword_t::k_typeid },


//??? vector
//??? dict

		//	Struct
		{ s1, R"(["struct", [[]]])", "struct {}" },
		{
			typeid_t::make_struct2(
				vector<member_t>{
					member_t(typeid_t::make_int(), "a"),
					member_t(typeid_t::make_double(), "b")
				}
			),
			R"(["struct", [[{ "type": "int", "name": "a"}, {"type": "double", "name": "b"}]]])",
			"struct {int a;double b;}"
		},


		//	Function
		{
			typeid_t::make_function(typeid_t::make_bool(), vector<typeid_t>{ typeid_t::make_int(), typeid_t::make_double() }),
			R"(["function", "bool", [ "int", "double"]])",
			"function bool(int,double)"
		},


		//	unknown_identifier
		{ typeid_t::make_unresolved_type_identifier("hello"), "\"hello\"", "hello" }
	};
	return tests;
}


OFF_QUARK_UNIT_TEST("typeid_to_ast_json()", "", "", ""){
	const auto f = make_typeid_str_tests();
	for(int i = 0 ; i < f.size() ; i++){
		QUARK_TRACE(std::to_string(i));
		const auto start_typeid = f[i]._typeid;
		const auto expected_ast_json = parse_json(seq_t(f[i]._ast_json)).first;

		//	Test typeid_to_ast_json().
		const auto result1 = typeid_to_ast_json(start_typeid, json_tags::k_tag_resolve_state);
		ut_compare_jsons(result1._value, expected_ast_json);
	}
}


OFF_QUARK_UNIT_TEST("typeid_from_ast_json", "", "", ""){
	const auto f = make_typeid_str_tests();
	for(int i = 0 ; i < f.size() ; i++){
		QUARK_TRACE(std::to_string(i));
		const auto start_typeid = f[i]._typeid;
		const auto expected_ast_json = parse_json(seq_t(f[i]._ast_json)).first;

		//	Test typeid_from_ast_json();
		const auto result2 = typeid_from_ast_json(ast_json_t{expected_ast_json});
		QUARK_UT_VERIFY(result2 == start_typeid);
	}
	QUARK_TRACE("OK!");
}


OFF_QUARK_UNIT_TEST("typeid_to_compact_string", "", "", ""){
	const auto f = make_typeid_str_tests();
	for(int i = 0 ; i < f.size() ; i++){
		QUARK_TRACE(std::to_string(i));
		const auto start_typeid = f[i]._typeid;

		//	Test typeid_to_compact_string().
		const auto result3 = typeid_to_compact_string(start_typeid);
		quark::ut_compare(result3, f[i]._compact_str);
	}
	QUARK_TRACE("OK!");
}



//////////////////////////////////////////////////		struct_definition_t


bool struct_definition_t::check_invariant() const{
//		QUARK_ASSERT(_struct!type.is_undefined() && _struct_type.check_invariant());

	for(const auto& m: _members){
		QUARK_ASSERT(m.check_invariant());
	}
	return true;
}

bool struct_definition_t::operator==(const struct_definition_t& other) const{
	QUARK_ASSERT(check_invariant());
	QUARK_ASSERT(other.check_invariant());

	return _members == other._members;
}

bool struct_definition_t::check_types_resolved() const{
	for(const auto& e: _members){
		bool result = e._type.check_types_resolved();
		if(result == false){
			return false;
		}
	}
	return true;
}


std::string to_compact_string(const struct_definition_t& v){
	auto s = string() + "struct {";
	for(const auto& e: v._members){
		s = s + typeid_to_compact_string(e._type) + " " + e._name + ";";
	}
	s = s + "}";
	return s;
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



//////////////////////////////////////////////////		protocol_definition_t


bool protocol_definition_t::check_invariant() const{
//		QUARK_ASSERT(_struct!type.is_undefined() && _struct_type.check_invariant());

	for(const auto& m: _members){
		QUARK_ASSERT(m.check_invariant());
	}
	return true;
}

bool protocol_definition_t::operator==(const protocol_definition_t& other) const{
	QUARK_ASSERT(check_invariant());
	QUARK_ASSERT(other.check_invariant());

	return _members == other._members;
}

bool protocol_definition_t::check_types_resolved() const{
	for(const auto& e: _members){
		bool result = e._type.check_types_resolved();
		if(result == false){
			return false;
		}
	}
	return true;
}


std::string to_compact_string(const protocol_definition_t& v){
	auto s = string() + "protocol {";
	for(const auto& e: v._members){
		s = s + typeid_to_compact_string(e._type) + " " + e._name + ";";
	}
	s = s + "}";
	return s;
}



/*	QUARK_UNIT_TEST("typeid_to_ast_json()", "", "", ""){
	const auto f = make_typeid_str_tests();
	for(int i = 0 ; i < f.size() ; i++){
		QUARK_TRACE(std::to_string(i));
		const auto start_typeid = f[i]._typeid;
		const auto expected_ast_json = parse_json(seq_t(f[i]._ast_json)).first;

		//	Test typeid_to_ast_json().
		const auto result1 = typeid_to_ast_json(start_typeid, json_tags::k_tag_resolve_state);
		ut_compare_jsons(result1._value, expected_ast_json);
	}
}
*/




////////////////////////			member_t


member_t::member_t(const floyd::typeid_t& type, const std::string& name) :
	_type(type),
	_name(name)
{
	QUARK_ASSERT(type.check_invariant());
//		QUARK_ASSERT(name.size() > 0);

	QUARK_ASSERT(check_invariant());
}

bool member_t::check_invariant() const{
	QUARK_ASSERT(_type.check_invariant());
//		QUARK_ASSERT(_name.size() > 0);
	return true;
}

bool member_t::operator==(const member_t& other) const{
	QUARK_ASSERT(check_invariant());
	QUARK_ASSERT(other.check_invariant());

	return (_type == other._type) && (_name == other._name);
}


std::vector<floyd::typeid_t> get_member_types(const std::vector<member_t>& m){
	vector<floyd::typeid_t> r;
	for(const auto& a: m){
		r.push_back(a._type);
	}
	return r;
}


bool typeid_t::check_types_resolved() const{
	if(is_unresolved_type_identifier()){
		return false;
	}
	else if(is_undefined()){
		return false;
	}
	else{
		if(_ext){
			for(const auto& e: _ext->_parts){
				bool result = e.check_types_resolved();
				if(result == false){
					return false;
				}
			}

			if(_ext->_struct_def){
				bool result = _ext->_struct_def->check_types_resolved();
				if(result == false){
					return false;
				}
			}
			else if(_ext->_protocol_def){
				bool result = _ext->_protocol_def->check_types_resolved();
				if(result == false){
					return false;
				}
			}
		}
	}
	return true;
}


int count_function_dynamic_args(const std::vector<typeid_t>& args){
	int count = 0;
	for(const auto& e: args){
		if(e.is_internal_dynamic()){
			count++;
		}
	}
	return count;
}
int count_function_dynamic_args(const typeid_t& function_type){
	QUARK_ASSERT(function_type.is_function());

	return count_function_dynamic_args(function_type.get_function_args());
}
bool is_dynamic_function(const typeid_t& function_type){
	QUARK_ASSERT(function_type.is_function());

	const auto count = count_function_dynamic_args(function_type);
	return count > 0;
}

}


