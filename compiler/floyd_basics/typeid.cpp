//
//  typeid.cpp
//  Floyd
//
//  Created by Marcus Zetterquist on 2018-02-12.
//  Copyright © 2018 Marcus Zetterquist. All rights reserved.
//

#include "typeid.h"

#include "parser_primitives.h"
#include "json_support.h"
#include "utils.h"



namespace floyd {





//////////////////////////////////////////////////		base_type


std::string base_type_to_opcode(const base_type t){
	if(t == base_type::k_undefined){
		return "undef";
	}
	else if(t == base_type::k_any){
		return "any";
	}

	else if(t == base_type::k_void){
		return "void";
	}
	else if(t == base_type::k_bool){
		return "bool";
	}
	else if(t == base_type::k_int){
		return "int";
	}
	else if(t == base_type::k_double){
		return "double";
	}
	else if(t == base_type::k_string){
		return "string";
	}
	else if(t == base_type::k_json){
		return "json";
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
	else if(t == base_type::k_dict){
		return "dict";
	}
	else if(t == base_type::k_function){
		return "func";
	}
	else if(t == base_type::k_unresolved){
		return "unknown-identifier";
	}
	else if(t == base_type::k_resolved){
		return "resolved-identifier";
	}
	else{
		QUARK_ASSERT(false);
		quark::throw_exception();
	}
	return "**impossible**";
}

QUARK_TEST("", "base_type_to_opcode(base_type)", "", ""){
	QUARK_TEST_VERIFY(base_type_to_opcode(base_type::k_undefined) == "undef");
}
QUARK_TEST("", "base_type_to_opcode(base_type)", "", ""){
	QUARK_TEST_VERIFY(base_type_to_opcode(base_type::k_any) == "any");
}
QUARK_TEST("", "base_type_to_opcode(base_type)", "", ""){
	QUARK_TEST_VERIFY(base_type_to_opcode(base_type::k_void) == "void");
}

QUARK_TEST("", "base_type_to_opcode(base_type)", "", ""){
	QUARK_TEST_VERIFY(base_type_to_opcode(base_type::k_bool) == "bool");
}
QUARK_TEST("", "base_type_to_opcode(base_type)", "", ""){
	QUARK_TEST_VERIFY(base_type_to_opcode(base_type::k_int) == "int");
}
QUARK_TEST("", "base_type_to_opcode(base_type)", "", ""){
	QUARK_TEST_VERIFY(base_type_to_opcode(base_type::k_double) == "double");
}
QUARK_TEST("", "base_type_to_opcode(base_type)", "", ""){
	QUARK_TEST_VERIFY(base_type_to_opcode(base_type::k_string) == "string");
}
QUARK_TEST("", "base_type_to_opcode(base_type)", "", ""){
	QUARK_TEST_VERIFY(base_type_to_opcode(base_type::k_json) == "json");
}

QUARK_TEST("", "base_type_to_opcode(base_type)", "", ""){
	QUARK_TEST_VERIFY(base_type_to_opcode(base_type::k_typeid) == "typeid");
}

QUARK_TEST("", "base_type_to_opcode(base_type)", "", ""){
	QUARK_TEST_VERIFY(base_type_to_opcode(base_type::k_struct) == "struct");
}
QUARK_TEST("", "base_type_to_opcode(base_type)", "", ""){
	QUARK_TEST_VERIFY(base_type_to_opcode(base_type::k_vector) == "vector");
}
QUARK_TEST("", "base_type_to_opcode(base_type)", "", ""){
	QUARK_TEST_VERIFY(base_type_to_opcode(base_type::k_dict) == "dict");
}
QUARK_TEST("", "base_type_to_opcode(base_type)", "", ""){
	QUARK_TEST_VERIFY(base_type_to_opcode(base_type::k_function) == "func");
}

QUARK_TEST("", "base_type_to_opcode(base_type)", "", ""){
	QUARK_TEST_VERIFY(base_type_to_opcode(base_type::k_unresolved) == "unknown-identifier");
}
QUARK_TEST("", "base_type_to_opcode(base_type)", "", ""){
	QUARK_TEST_VERIFY(base_type_to_opcode(base_type::k_resolved) == "resolved-identifier");
}

base_type opcode_to_base_type(const std::string& s){
	QUARK_ASSERT(s != "");

	if(s == "undef"){
		return base_type::k_undefined;
	}
	else if(s == "any"){
		return base_type::k_any;
	}
	else if(s == "void"){
		return base_type::k_void;
	}
	else if(s == "bool"){
		return base_type::k_bool;
	}
	else if(s == "int"){
		return base_type::k_int;
	}
	else if(s == "double"){
		return base_type::k_double;
	}
	else if(s == "string"){
		return base_type::k_string;
	}
	else if(s == "typeid"){
		return base_type::k_typeid;
	}
	else if(s == "json"){
		return base_type::k_json;
	}

	else if(s == "struct"){
		return base_type::k_struct;
	}
	else if(s == "vector"){
		return base_type::k_vector;
	}
	else if(s == "dict"){
		return base_type::k_dict;
	}
	else if(s == "func"){
		return base_type::k_function;
	}
	else if(s == "unknown-identifier"){
		return base_type::k_unresolved;
	}
	else if(s == "resolved-identifier"){
		return base_type::k_resolved;
	}

	else{
		QUARK_ASSERT(false);
	}
	return base_type::k_unresolved;
}


void ut_verify(const quark::call_context_t& context, const base_type& result, const base_type& expected){
	ut_verify(
		context,
		base_type_to_opcode(result),
		base_type_to_opcode(expected)
	);
}



//////////////////////////////////////////////////		get_json_type()


int get_json_type(const json_t& value){
	QUARK_ASSERT(value.check_invariant());

	if(value.is_object()){
		return 1;
	}
	else if(value.is_array()){
		return 2;
	}
	else if(value.is_string()){
		return 3;
	}
	else if(value.is_number()){
		return 4;
	}
	else if(value.is_true()){
		return 5;
	}
	else if(value.is_false()){
		return 6;
	}
	else if(value.is_null()){
		return 7;
	}
	else{
		QUARK_ASSERT(false);
		quark::throw_exception();
	}
	return 0;
}


//////////////////////////////////////////////////		typeid_t


bool typeid_t::check_invariant() const{
#if DEBUG_DEEP
	struct visitor_t {
		bool operator()(const undefined_t& e) const{
			return true;
		}
		bool operator()(const any_t& e) const{
			return true;
		}

		bool operator()(const void_t& e) const{
			return true;
		}
		bool operator()(const bool_t& e) const{
			return true;
		}
		bool operator()(const int_t& e) const{
			return true;
		}
		bool operator()(const double_t& e) const{
			return true;
		}
		bool operator()(const string_t& e) const{
			return true;
		}

		bool operator()(const json_type_t& e) const{
			return true;
		}
		bool operator()(const typeid_type_t& e) const{
			return true;
		}

		bool operator()(const struct_t& e) const{
			QUARK_ASSERT(e._struct_def);
			QUARK_ASSERT(e._struct_def->check_invariant());
			return true;
		}
		bool operator()(const vector_t& e) const{
			QUARK_ASSERT(e._parts.size() == 1);
			QUARK_ASSERT(e._parts[0].check_invariant());
			return true;
		}
		bool operator()(const dict_t& e) const{
			QUARK_ASSERT(e._parts.size() == 1);
			QUARK_ASSERT(e._parts[0].check_invariant());
			return true;
		}
		bool operator()(const function_t& e) const{
			//	If function returns a DYN, it must have a dyn_return.
			QUARK_ASSERT(e._parts[0].is_any() == false || e.dyn_return != return_dyn_type::none);

			QUARK_ASSERT(e._parts.size() >= 1);

			for(const auto& m: e._parts){
				QUARK_ASSERT(m.check_invariant());
			}
			return true;
		}
		bool operator()(const unresolved_t& e) const{
			QUARK_ASSERT(e._unresolved_type_identifier.empty() == false);
			return true;
		}
		bool operator()(const resolved_t& e) const{
			QUARK_ASSERT(e._resolved_type_identifier.empty() == false);
			return true;
		}
	};
	return std::visit(visitor_t{}, _contents);
#else
	return true;
#endif
}

void typeid_t::swap(typeid_t& other){
	QUARK_ASSERT(other.check_invariant());
	QUARK_ASSERT(check_invariant());

#if DEBUG_DEEP
	std::swap(_DEBUG_string, other._DEBUG_string);
#endif
	std::swap(_contents, other._contents);
	std::swap(_name, other._name);

	QUARK_ASSERT(other.check_invariant());
	QUARK_ASSERT(check_invariant());
}


QUARK_TESTQ("typeid_t", "make_undefined()"){
	ut_verify(QUARK_POS, typeid_t::make_undefined().get_base_type(), base_type::k_undefined);
}
QUARK_TESTQ("typeid_t", "is_undefined()"){
	ut_verify_auto(QUARK_POS, typeid_t::make_undefined().is_undefined(), true);
}
QUARK_TESTQ("typeid_t", "is_undefined()"){
	ut_verify_auto(QUARK_POS, typeid_t::make_bool().is_undefined(), false);
}


QUARK_TESTQ("typeid_t", "make_any()"){
	ut_verify(QUARK_POS, typeid_t::make_any().get_base_type(), base_type::k_any);
}
QUARK_TESTQ("typeid_t", "is_any()"){
	ut_verify_auto(QUARK_POS, typeid_t::make_any().is_any(), true);
}
QUARK_TESTQ("typeid_t", "is_any()"){
	ut_verify_auto(QUARK_POS, typeid_t::make_bool().is_any(), false);
}


QUARK_TESTQ("typeid_t", "make_void()"){
	ut_verify(QUARK_POS, typeid_t::make_void().get_base_type(), base_type::k_void);
}
QUARK_TESTQ("typeid_t", "is_void()"){
	ut_verify_auto(QUARK_POS, typeid_t::make_void().is_void(), true);
}
QUARK_TESTQ("typeid_t", "is_void()"){
	ut_verify_auto(QUARK_POS, typeid_t::make_bool().is_void(), false);
}


QUARK_TESTQ("typeid_t", "make_bool()"){
	ut_verify(QUARK_POS, typeid_t::make_bool().get_base_type(), base_type::k_bool);
}
QUARK_TESTQ("typeid_t", "is_bool()"){
	ut_verify_auto(QUARK_POS, typeid_t::make_bool().is_bool(), true);
}
QUARK_TESTQ("typeid_t", "is_bool()"){
	ut_verify_auto(QUARK_POS, typeid_t::make_void().is_bool(), false);
}


QUARK_TESTQ("typeid_t", "make_int()"){
	ut_verify(QUARK_POS, typeid_t::make_int().get_base_type(), base_type::k_int);
}
QUARK_TESTQ("typeid_t", "is_int()"){
	QUARK_UT_VERIFY(typeid_t::make_int().is_int() == true);
}
QUARK_TESTQ("typeid_t", "is_int()"){
	QUARK_UT_VERIFY(typeid_t::make_bool().is_int() == false);
}


QUARK_TESTQ("typeid_t", "make_double()"){
	QUARK_UT_VERIFY(typeid_t::make_double().is_double());
}
QUARK_TESTQ("typeid_t", "is_double()"){
	QUARK_UT_VERIFY(typeid_t::make_double().is_double() == true);
}
QUARK_TESTQ("typeid_t", "is_double()"){
	QUARK_UT_VERIFY(typeid_t::make_bool().is_double() == false);
}


QUARK_TESTQ("typeid_t", "make_string()"){
	QUARK_UT_VERIFY(typeid_t::make_string().get_base_type() == base_type::k_string);
}
QUARK_TESTQ("typeid_t", "is_string()"){
	QUARK_UT_VERIFY(typeid_t::make_string().is_string() == true);
}
QUARK_TESTQ("typeid_t", "is_string()"){
	QUARK_UT_VERIFY(typeid_t::make_bool().is_string() == false);
}


QUARK_TESTQ("typeid_t", "make_json()"){
	QUARK_UT_VERIFY(typeid_t::make_json().get_base_type() == base_type::k_json);
}
QUARK_TESTQ("typeid_t", "is_json()"){
	QUARK_UT_VERIFY(typeid_t::make_json().is_json() == true);
}
QUARK_TESTQ("typeid_t", "is_json()"){
	QUARK_UT_VERIFY(typeid_t::make_bool().is_json() == false);
}


QUARK_TESTQ("typeid_t", "make_typeid()"){
	QUARK_UT_VERIFY(typeid_t::make_typeid().get_base_type() == base_type::k_typeid);
}
QUARK_TESTQ("typeid_t", "is_typeid()"){
	QUARK_UT_VERIFY(typeid_t::make_typeid().is_typeid() == true);
}
QUARK_TESTQ("typeid_t", "is_typeid()"){
	QUARK_UT_VERIFY(typeid_t::make_bool().is_typeid() == false);
}




typeid_t make_empty_struct(){
	return typeid_t::make_struct2({});
}

const auto k_struct_test_members_b = std::vector<member_t>({
	{ typeid_t::make_int(), "x" },
	{ typeid_t::make_string(), "y" },
	{ typeid_t::make_bool(), "z" }
});

typeid_t make_test_struct_a(){
	return typeid_t::make_struct2(k_struct_test_members_b);
}

QUARK_TESTQ("typeid_t", "make_struct2()"){
	QUARK_UT_VERIFY(typeid_t::make_struct2({}).get_base_type() == base_type::k_struct);
}
QUARK_TESTQ("typeid_t", "make_struct2()"){
	QUARK_UT_VERIFY(make_empty_struct().get_base_type() == base_type::k_struct);
}
QUARK_TESTQ("typeid_t", "is_struct()"){
	QUARK_UT_VERIFY(make_empty_struct().is_struct() == true);
}
QUARK_TESTQ("typeid_t", "is_struct()"){
	QUARK_UT_VERIFY(typeid_t::make_bool().is_struct() == false);
}

QUARK_TESTQ("typeid_t", "make_struct2()"){
	const auto t = make_test_struct_a();
	QUARK_UT_VERIFY(t.get_struct() == k_struct_test_members_b);
}
QUARK_TESTQ("typeid_t", "get_struct()"){
	const auto t = make_test_struct_a();
	QUARK_UT_VERIFY(t.get_struct() == k_struct_test_members_b);
}
QUARK_TESTQ("typeid_t", "get_struct_ref()"){
	const auto t = make_test_struct_a();
	QUARK_UT_VERIFY(t.get_struct_ref()->_members == k_struct_test_members_b);
}





QUARK_TESTQ("typeid_t", "make_vector()"){
	QUARK_UT_VERIFY(typeid_t::make_vector(typeid_t::make_int()).get_base_type() == base_type::k_vector);
}
QUARK_TESTQ("typeid_t", "is_vector()"){
	QUARK_UT_VERIFY(typeid_t::make_vector(typeid_t::make_int()).is_vector() == true);
}
QUARK_TESTQ("typeid_t", "is_vector()"){
	QUARK_UT_VERIFY(typeid_t::make_bool().is_vector() == false);
}
QUARK_TESTQ("typeid_t", "get_vector_element_type()"){
	QUARK_UT_VERIFY(typeid_t::make_vector(typeid_t::make_int()).get_vector_element_type().is_int());
}
QUARK_TESTQ("typeid_t", "get_vector_element_type()"){
	QUARK_UT_VERIFY(typeid_t::make_vector(typeid_t::make_string()).get_vector_element_type().is_string());
}


QUARK_TESTQ("typeid_t", "make_dict()"){
	QUARK_UT_VERIFY(typeid_t::make_dict(typeid_t::make_int()).get_base_type() == base_type::k_dict);
}
QUARK_TESTQ("typeid_t", "is_dict()"){
	QUARK_UT_VERIFY(typeid_t::make_dict(typeid_t::make_int()).is_dict() == true);
}
QUARK_TESTQ("typeid_t", "is_dict()"){
	QUARK_UT_VERIFY(typeid_t::make_bool().is_dict() == false);
}
QUARK_TESTQ("typeid_t", "get_dict_value_type()"){
	QUARK_UT_VERIFY(typeid_t::make_dict(typeid_t::make_int()).get_dict_value_type().is_int());
}
QUARK_TESTQ("typeid_t", "get_dict_value_type()"){
	QUARK_UT_VERIFY(typeid_t::make_dict(typeid_t::make_string()).get_dict_value_type().is_string());
}



const auto k_test_function_args_a = std::vector<typeid_t>({
	{ typeid_t::make_int() },
	{ typeid_t::make_string() },
	{ typeid_t::make_bool() }
});

QUARK_TESTQ("typeid_t", "make_function()"){
	const auto t = typeid_t::make_function(typeid_t::make_void(), {}, epure::pure);
	QUARK_UT_VERIFY(t.get_base_type() == base_type::k_function);
}
QUARK_TESTQ("typeid_t", "is_function()"){
	const auto t = typeid_t::make_function(typeid_t::make_void(), {}, epure::pure);
	QUARK_UT_VERIFY(t.is_function() == true);
}
QUARK_TESTQ("typeid_t", "is_function()"){
	QUARK_UT_VERIFY(typeid_t::make_bool().is_function() == false);
}
QUARK_TESTQ("typeid_t", "get_function_return()"){
	const auto t = typeid_t::make_function(typeid_t::make_void(), {}, epure::pure);
	QUARK_UT_VERIFY(t.get_function_return().is_void());
}
QUARK_TESTQ("typeid_t", "get_function_return()"){
	const auto t = typeid_t::make_function(typeid_t::make_string(), {}, epure::pure);
	QUARK_UT_VERIFY(t.get_function_return().is_string());
}
QUARK_TESTQ("typeid_t", "get_function_args()"){
	const auto t = typeid_t::make_function(typeid_t::make_void(), k_test_function_args_a, epure::pure);
	QUARK_UT_VERIFY(t.get_function_args() == k_test_function_args_a);
}


QUARK_TESTQ("typeid_t", "make_unresolved_type_identifier()"){
	const auto t = typeid_t::make_unresolved_type_identifier("xyz");
	QUARK_UT_VERIFY(t.get_base_type() == base_type::k_unresolved);
}

QUARK_TESTQ("typeid_t", "is_unresolved_type_identifier()"){
	const auto t = typeid_t::make_unresolved_type_identifier("xyz");
	QUARK_UT_VERIFY(t.is_unresolved_type_identifier() == true);
}
QUARK_TESTQ("typeid_t", "is_unresolved_type_identifier()"){
	QUARK_UT_VERIFY(typeid_t::make_bool().is_unresolved_type_identifier() == false);
}

QUARK_TESTQ("typeid_t", "get_unresolved()"){
	const auto t = typeid_t::make_unresolved_type_identifier("xyz");
	QUARK_UT_VERIFY(t.get_unresolved_type_identifer() == "xyz");
}
QUARK_TESTQ("typeid_t", "get_unresolved()"){
	const auto t = typeid_t::make_unresolved_type_identifier("123");
	QUARK_UT_VERIFY(t.get_unresolved_type_identifer() == "123");
}










QUARK_TESTQ("typeid_t", "operator==()"){
	const auto a = typeid_t::make_vector(typeid_t::make_int());
	const auto b = typeid_t::make_vector(typeid_t::make_int());
	QUARK_UT_VERIFY(a == b);
}
QUARK_TESTQ("typeid_t", "operator==()"){
	const auto a = typeid_t::make_vector(typeid_t::make_int());
	const auto b = typeid_t::make_dict(typeid_t::make_int());
	QUARK_UT_VERIFY((a == b) == false);
}
QUARK_TESTQ("typeid_t", "operator==()"){
	const auto a = typeid_t::make_vector(typeid_t::make_int());
	const auto b = typeid_t::make_vector(typeid_t::make_string());
	QUARK_UT_VERIFY((a == b) == false);
}


QUARK_TESTQ("typeid_t", "operator=()"){
	const auto a = typeid_t::make_bool();
	const auto b = a;
	QUARK_UT_VERIFY(a == b);
}
QUARK_TESTQ("typeid_t", "operator=()"){
	const auto a = typeid_t::make_vector(typeid_t::make_int());
	const auto b = a;
	QUARK_UT_VERIFY(a == b);
}
QUARK_TESTQ("typeid_t", "operator=()"){
	const auto a = typeid_t::make_dict(typeid_t::make_int());
	const auto b = a;
	QUARK_UT_VERIFY(a == b);
}
QUARK_TESTQ("typeid_t", "operator=()"){
	const auto a = typeid_t::make_function(typeid_t::make_string(), { typeid_t::make_int(), typeid_t::make_double() }, epure::pure);
	const auto b = a;
	QUARK_UT_VERIFY(a == b);
}



//////////////////////////////////////		FORMATS



static std::string typeid_to_compact_string_int(const typeid_t& t){
//		QUARK_ASSERT(t.check_invariant());

	const auto basetype = t.get_base_type();

	if(basetype == floyd::base_type::k_unresolved){
		return std::string() + "unresolved: " + t.get_unresolved_type_identifer();
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
		const auto pure = t.get_function_pure();

		std::vector<std::string> args_str;
		for(const auto& a: args){
			args_str.push_back(typeid_to_compact_string(a));
		}

		return std::string() + "function " + typeid_to_compact_string(ret) + "(" + concat_strings_with_divider(args_str, ",") + ") " + (pure == epure::pure ? "pure" : "impure");
	}
	else{
		return base_type_to_opcode(basetype);
	}
}
std::string typeid_to_compact_string(const typeid_t& t){
//	return typeid_to_compact_string_int(t);
	if(t.get_name() == ""){
		return typeid_to_compact_string_int(t);
	}
	else{
		return t.get_name() + ":" + typeid_to_compact_string_int(t);
	}
}


struct typeid_str_test_t {
	typeid_t _typeid;
	std::string _ast_json;
	std::string _compact_str;
};


const std::vector<typeid_str_test_t> make_typeid_str_tests(){
	const auto s1 = typeid_t::make_struct2({});

	const auto tests = std::vector<typeid_str_test_t>{
		{ typeid_t::make_undefined(), quote(base_type_to_opcode(base_type::k_undefined)), base_type_to_opcode(base_type::k_undefined) },
		{ typeid_t::make_bool(), quote(base_type_to_opcode(base_type::k_bool)), base_type_to_opcode(base_type::k_bool) },
		{ typeid_t::make_int(), quote(base_type_to_opcode(base_type::k_int)), base_type_to_opcode(base_type::k_int) },
		{ typeid_t::make_double(), quote(base_type_to_opcode(base_type::k_double)), base_type_to_opcode(base_type::k_double) },
		{ typeid_t::make_string(), quote(base_type_to_opcode(base_type::k_string)), base_type_to_opcode(base_type::k_string) },

		//	Typeid
		{ typeid_t::make_typeid(), quote(base_type_to_opcode(base_type::k_typeid)), base_type_to_opcode(base_type::k_typeid) },
		{ typeid_t::make_typeid(), quote(base_type_to_opcode(base_type::k_typeid)), base_type_to_opcode(base_type::k_typeid) },


//??? vector
//??? dict

		//	Struct
		{ s1, R"(["struct", [[]]])", "struct {}" },
		{
			typeid_t::make_struct2(
				std::vector<member_t>{
					member_t(typeid_t::make_int(), "a"),
					member_t(typeid_t::make_double(), "b")
				}
			),
			R"(["struct", [[{ "type": "int", "name": "a"}, {"type": "double", "name": "b"}]]])",
			"struct {int a;double b;}"
		},


		//	Function
		{
			typeid_t::make_function(typeid_t::make_bool(), std::vector<typeid_t>{ typeid_t::make_int(), typeid_t::make_double() }, epure::pure),
			R"(["function", "bool", [ "int", "double"]])",
			"function bool(int,double)"
		},


		//	unknown_identifier
		{ typeid_t::make_unresolved_type_identifier("hello"), "\"hello\"", "hello" }
	};
	return tests;
}


#if 0
OFF_QUARK_TEST("typeid_to_ast_json()", "", "", ""){
	const auto f = make_typeid_str_tests();
	for(int i = 0 ; i < f.size() ; i++){
		QUARK_TRACE(std::to_string(i));
		const auto start_typeid = f[i]._typeid;
		const auto expected_ast_json = parse_json(seq_t(f[i]._ast_json)).first;

		//	Test typeid_to_ast_json().
		const auto result1 = typeid_to_ast_json(start_typeid, json_tags::k_tag_resolve_state);
		ut_verify(QUARK_POS, result1, expected_ast_json);
	}
}


OFF_QUARK_TEST("typeid_from_ast_json", "", "", ""){
	const auto f = make_typeid_str_tests();
	for(int i = 0 ; i < f.size() ; i++){
		QUARK_TRACE(std::to_string(i));
		const auto start_typeid = f[i]._typeid;
		const auto expected_ast_json = parse_json(seq_t(f[i]._ast_json)).first;

		//	Test typeid_from_ast_json();
		const auto result2 = typeid_from_ast_json(expected_ast_json);
		QUARK_UT_VERIFY(result2 == start_typeid);
	}
	QUARK_TRACE("OK!");
}


OFF_QUARK_TEST("typeid_to_compact_string", "", "", ""){
	const auto f = make_typeid_str_tests();
	for(int i = 0 ; i < f.size() ; i++){
		QUARK_TRACE(std::to_string(i));
		const auto start_typeid = f[i]._typeid;

		//	Test typeid_to_compact_string().
		const auto result3 = typeid_to_compact_string(start_typeid);
		ut_verify(QUARK_POS, result3, f[i]._compact_str);
	}
	QUARK_TRACE("OK!");
}
#endif



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


std::string to_compact_string(const struct_definition_t& v){
	auto s = std::string() + "struct {";
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
	std::vector<floyd::typeid_t> r;
	for(const auto& a: m){
		r.push_back(a._type);
	}
	return r;
}

std::string members_to_string(const std::vector<member_t>& m){
	std::string result;
	for(const auto& e: m){
		const std::string s = std::string("(") + typeid_to_compact_string(e._type) + " " + e._name + std::string(")");
		result = result + s;
	}

	//	Remove trailing ',' character, if any.
	return result == "" ? "" : result.substr(0, result.size() - 1);
}



////////////////////////			typeid_t



int count_function_dynamic_args(const std::vector<typeid_t>& args){
	int count = 0;
	for(const auto& e: args){
		if(e.is_any()){
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




QUARK_TEST("typeid_t", "operator==()", "", ""){
	const auto a = typeid_t::make_function(typeid_t::make_int(), {}, epure::pure);
	const auto b = typeid_t::make_function(typeid_t::make_int(), {}, epure::pure);
	QUARK_UT_VERIFY(a == b);
}






std::vector<json_t> typeids_to_json_array(const std::vector<typeid_t>& m);
std::vector<typeid_t> typeids_from_json_array(const std::vector<json_t>& m);



json_t struct_definition_to_ast_json(const struct_definition_t& v){
	QUARK_ASSERT(v.check_invariant());

	return json_t::make_array({
		members_to_json(v._members)
	});
}



static json_t typeid_to_json0(const typeid_t& t, json_tags tags){
	QUARK_ASSERT(t.check_invariant());

	const auto b = t.get_base_type();
	const auto basetype_str = base_type_to_opcode(b);
	const auto basetype_str_tagged = tags == json_tags::k_tag_resolve_state ? (std::string() + tag_resolved_type_char + basetype_str) : basetype_str;

	if(false
		|| b == base_type::k_undefined
		|| b == base_type::k_any

		|| b == base_type::k_void
		|| b == base_type::k_bool
		|| b == base_type::k_int
		|| b == base_type::k_double
		|| b == base_type::k_string
		|| b == base_type::k_json
		|| b == base_type::k_typeid
	){
		return basetype_str_tagged;
	}
	else if(b == base_type::k_struct){
		const auto struct_def = t.get_struct();
		return json_t::make_array({
			json_t(basetype_str_tagged),
			struct_definition_to_ast_json(struct_def)
		});
	}
	else if(b == base_type::k_vector){
		const auto d = t.get_vector_element_type();
		return json_t::make_array({
			json_t(basetype_str),
			typeid_to_ast_json(d, tags)
		});
	}
	else if(b == base_type::k_dict){
		const auto d = t.get_dict_value_type();
		return json_t::make_array({
			json_t(basetype_str),
			typeid_to_ast_json(d, tags)
		});
	}
	else if(b == base_type::k_function){
		return json_t::make_array({
			basetype_str,
			typeid_to_ast_json(t.get_function_return(), tags),
			typeids_to_json_array(t.get_function_args()),
			t.get_function_pure() == epure::pure ? true : false
		});
	}
	else if(b == base_type::k_unresolved){
		return std::string(1, tag_unresolved_type_char) + t.get_unresolved_type_identifer();
	}
	else{
		QUARK_ASSERT(false);
		quark::throw_logic_error();
	}
	return basetype_str_tagged;
}

json_t typeid_to_ast_json(const typeid_t& t, json_tags tags){
	QUARK_ASSERT(t.check_invariant());

	const auto type_desc = typeid_to_json0(t, tags);
	if(t.get_name() == ""){
		return type_desc;
	}
	else{
		return json_t::make_object({
			{ "name", t._name },
			{ "desc", type_desc }
		});
	}
}


QUARK_TEST("", "typeid_to_ast_json()", "", ""){
	const auto t = typeid_t::make_int();
	const auto r = json_to_compact_string_minimal_quotes(typeid_to_ast_json(t, json_tags::k_tag_resolve_state));
	QUARK_UT_VERIFY(r == "^int");
}

QUARK_TEST("", "typeid_to_ast_json()", "", ""){
	const auto t = typeid_t::make_int().name("coord_t");
	const auto r = json_to_compact_string_minimal_quotes(typeid_to_ast_json(t, json_tags::k_tag_resolve_state));
	QUARK_UT_VERIFY(r == "{ desc: ^int, name: coord_t }");
}

QUARK_TEST("", "typeid_to_ast_json()", "", ""){
	const auto t = typeid_t::make_undefined().name("coord_t");
	const auto r = json_to_compact_string_minimal_quotes(typeid_to_ast_json(t, json_tags::k_tag_resolve_state));
	QUARK_UT_VERIFY(r == "{ desc: ^undef, name: coord_t }");
}




static typeid_t typeid_from_json0(const json_t& t){
	QUARK_ASSERT(t.check_invariant());

	if(t.is_string()){
		const auto s0 = t.get_string();
		if(s0.front() == tag_resolved_type_char){
			const auto s = s0.substr(1);
			if(s == ""){
				return typeid_t::make_undefined();
			}

			const auto b = opcode_to_base_type(s);


			if(b == base_type::k_undefined){
				return typeid_t::make_undefined();
			}
			else if(b == base_type::k_any){
				return typeid_t::make_any();
			}
			else if(b == base_type::k_void){
				return typeid_t::make_void();
			}
			else if(b == base_type::k_bool){
				return typeid_t::make_bool();
			}
			else if(b == base_type::k_int){
				return typeid_t::make_int();
			}
			else if(b == base_type::k_double){
				return typeid_t::make_double();
			}
			else if(b == base_type::k_string){
				return typeid_t::make_string();
			}
			else if(b == base_type::k_typeid){
				return typeid_t::make_typeid();
			}
			else if(b == base_type::k_json){
				return typeid_t::make_json();
			}
			else{
				quark::throw_exception();
			}
		}
		else if(s0.front() == tag_unresolved_type_char){
			const auto s = s0.substr(1);
			return typeid_t::make_unresolved_type_identifier(s);
		}
		else {
			throw std::exception();
		}
	}
	else if(t.is_array()){
		const auto a = t.get_array();
		const auto s0 = a[0].get_string();
//		QUARK_ASSERT(s0.front() != '^');
		QUARK_ASSERT(s0.front() != '#');
		const auto s = s0.front() == '^' ? s0.substr(1) : s0;
/*
		if(s == "typeid"){
			const auto t3 = typeid_from_ast_json(json_t{a[1]});
			return typeid_t::make_typeid(t3);
		}
		else
*/
		if(s == "struct"){
			const auto struct_def_array = a[1].get_array();
			const auto member_array = struct_def_array[0].get_array();

			const std::vector<member_t> struct_members = members_from_json(member_array);
			return typeid_t::make_struct2(struct_members);
		}
		else if(s == "vector"){
			const auto element_type = typeid_from_ast_json(a[1]);
			return typeid_t::make_vector(element_type);
		}
		else if(s == "dict"){
			const auto value_type = typeid_from_ast_json(a[1]);
			return typeid_t::make_dict(value_type);
		}
		else if(s == "func"){
			const auto ret_type = typeid_from_ast_json(a[1]);
			const auto arg_types_array = a[2].get_array();
			const std::vector<typeid_t> arg_types = typeids_from_json_array(arg_types_array);

			if(a[3].is_true() == false && a[3].is_false() == false){
				quark::throw_exception();
			}
			const bool pure = a[3].is_true();
			return typeid_t::make_function(ret_type, arg_types, pure ? epure::pure : epure::impure);
		}
		else if(s == "unknown-identifier"){
			QUARK_ASSERT(false);
			quark::throw_exception();
		}
		else {
			QUARK_ASSERT(false);
			quark::throw_exception();
		}
	}
	else{
		quark::throw_runtime_error("Invalid typeid-json.");
	}
	return typeid_t::make_undefined();
}

typeid_t typeid_from_ast_json(const json_t& t2){
	QUARK_ASSERT(t2.check_invariant());

	if(t2.is_object()){
		const auto name = t2.get_object_element("name").get_string();
		const auto& t = t2.get_object_element("desc");
		const auto a = typeid_from_json0(t);
		return a.name(name);
	}
	else{
		const auto a = typeid_from_json0(t2);
		return a;
	}
}



QUARK_TEST("", "typeid_from_ast_json()", "", ""){
	const auto t = typeid_t::make_int();
	const auto j = typeid_to_ast_json(t, json_tags::k_tag_resolve_state);
	const auto r = typeid_from_ast_json(j);
	QUARK_ASSERT(r == t);
}

QUARK_TEST("", "typeid_from_ast_json()", "", ""){
	const auto t = typeid_t::make_int().name("coord_t");
	const auto j = typeid_to_ast_json(t, json_tags::k_tag_resolve_state);
	const auto r = typeid_from_ast_json(j);
	QUARK_ASSERT(r == t);
}

QUARK_TEST("", "typeid_from_ast_json()", "", ""){
	const auto t = typeid_t::make_undefined().name("coord_t");
	const auto j = typeid_to_ast_json(t, json_tags::k_tag_resolve_state);
	const auto r = typeid_from_ast_json(j);
	QUARK_ASSERT(r == t);
}




std::vector<json_t> typeids_to_json_array(const std::vector<typeid_t>& m){
	std::vector<json_t> r;
	for(const auto& a: m){
		r.push_back(typeid_to_ast_json(a, json_tags::k_tag_resolve_state));
	}
	return r;
}
std::vector<typeid_t> typeids_from_json_array(const std::vector<json_t>& m){
	std::vector<typeid_t> r;
	for(const auto& a: m){
		r.push_back(typeid_from_ast_json(a));
	}
	return r;
}



json_t members_to_json(const std::vector<member_t>& members){
	std::vector<json_t> r;
	for(const auto& i: members){
		const auto member = make_object({
			{ "type", typeid_to_ast_json(i._type, json_tags::k_tag_resolve_state) },
			{ "name", json_t(i._name) }
		});
		r.push_back(json_t(member));
	}
	return r;
}

std::vector<member_t> members_from_json(const json_t& members){
	QUARK_ASSERT(members.check_invariant());

	std::vector<member_t> r;
	for(const auto& i: members.get_array()){
		const auto m = member_t(
			typeid_from_ast_json(i.get_object_element("type")),
			i.get_object_element("name").get_string()
		);

		r.push_back(m);
	}
	return r;
}

void ut_verify(const quark::call_context_t& context, const typeid_t& result, const typeid_t& expected){
	QUARK_ASSERT(result.check_invariant());
	QUARK_ASSERT(expected.check_invariant());

	ut_verify(
		context,
		typeid_to_ast_json(result, json_tags::k_plain),
		typeid_to_ast_json(expected, json_tags::k_plain)
	);
}

}	//	floyd
