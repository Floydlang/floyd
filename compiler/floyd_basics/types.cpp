//
//  compiler_basics.cpp
//  Floyd
//
//  Created by Marcus Zetterquist on 2019-02-05.
//  Copyright © 2019 Marcus Zetterquist. All rights reserved.
//

#include "types.h"

#include "parser_primitives.h"
#include "json_support.h"
#include "utils.h"
#include "ast_helpers.h"
#include "json_support.h"
#include "format_table.h"
#include "compiler_basics.h"
#include "parser_primitives.h"

#include <limits.h>




namespace floyd {


std::string pack_type_name(const type_name_t& tag){
	if(tag.lexical_path.empty()){
		return "/";
	}
	else{
		std::string acc;
		for(const auto& e: tag.lexical_path){
			acc = acc + "/" + e;
		}
		return acc;
	}
}

QUARK_TEST("", "pack_type_name()", "", ""){
	QUARK_TEST_VERIFY(pack_type_name(type_name_t{{ "hello" }} ) == "/hello");
}
QUARK_TEST("", "pack_type_name()", "", ""){
	QUARK_TEST_VERIFY(pack_type_name(type_name_t{{ "hello", "goodbye" }} ) == "/hello/goodbye");
}
QUARK_TEST("", "pack_type_name()", "", ""){
	QUARK_TEST_VERIFY(pack_type_name(type_name_t{{ "" }} ) == "/");
}


bool is_type_name(const std::string& s){
	return s.size() > 0 && s[0] == '/';
}

QUARK_TEST("", "is_type_name()", "", ""){
	QUARK_TEST_VERIFY(is_type_name("hello") == false);
}
QUARK_TEST("", "is_type_name()", "", ""){
	QUARK_TEST_VERIFY(is_type_name("/hello") == true);
}
QUARK_TEST("", "is_type_name()", "", ""){
	QUARK_TEST_VERIFY(is_type_name("/hello/goodbye") == true);
}


type_name_t unpack_type_name(const std::string& tag){
	QUARK_ASSERT(is_type_name(tag));

	if(tag == "/"){
		return type_name_t { } ;
	}
	else{
		std::vector<std::string> path;
		size_t pos = 0;
		while(pos != tag.size()){
			const auto next_pos = tag.find("/", pos + 1);
			const auto next_pos2 = next_pos == std::string::npos ? tag.size() : next_pos;
			const auto s = tag.substr(pos + 1, next_pos2 - 1);
			path.push_back(s);
			pos = next_pos2;
		}
		return type_name_t { path } ;
	}
}

QUARK_TEST("", "pack_type_name()", "", ""){
	QUARK_TEST_VERIFY(unpack_type_name("/hello") == type_name_t{{ "hello" }} );
}
QUARK_TEST("", "pack_type_name()", "", ""){
	QUARK_TEST_VERIFY(unpack_type_name("/hello/goodbye") == ( type_name_t{{ "hello", "goodbye" }} ) );
}
QUARK_TEST("", "pack_type_name()", "", ""){
	QUARK_TEST_VERIFY(unpack_type_name("/") == type_name_t{{ }} );
}


type_name_t make_empty_type_name(){
	return type_name_t { };
}



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
	else if(t == base_type::k_symbol_ref){
		return "symbol-ref";
	}
	else if(t == base_type::k_named_type){
		return "named-type";
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
	QUARK_TEST_VERIFY(base_type_to_opcode(base_type::k_symbol_ref) == "symbol-ref");
}
QUARK_TEST("", "base_type_to_opcode(base_type)", "", ""){
	QUARK_TEST_VERIFY(base_type_to_opcode(base_type::k_named_type) == "named-type");
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
	else if(s == "symbol"){
		return base_type::k_symbol_ref;
	}
	else if(s == "named-type"){
		return base_type::k_named_type;
	}

	else{
		QUARK_ASSERT(false);
	}
	return base_type::k_undefined;
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


//////////////////////////////////////////////////		type_t

#if 0
bool type_t::check_invariant() const{
#if DEBUG_DEEP_TYPEID_T
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
		bool operator()(const identifier_t& e) const {
			QUARK_ASSERT(e.name != "");
			return true;
		}
	};
	return std::visit(visitor_t{}, _contents);
#else
	return true;
#endif
}

void type_t::swap(type_t& other){
	QUARK_ASSERT(other.check_invariant());
	QUARK_ASSERT(check_invariant());

#if DEBUG_DEEP_TYPEID_T
	std::swap(_DEBUG_string, other._DEBUG_string);
#endif
	std::swap(_contents, other._contents);

	QUARK_ASSERT(other.check_invariant());
	QUARK_ASSERT(check_invariant());
}


QUARK_TESTQ("type_t", "make_undefined()"){
	ut_verify(QUARK_POS, type_t::make_undefined().get_base_type(), base_type::k_undefined);
}
QUARK_TESTQ("type_t", "is_undefined()"){
	ut_verify_auto(QUARK_POS, type_t::make_undefined().is_undefined(), true);
}
QUARK_TESTQ("type_t", "is_undefined()"){
	ut_verify_auto(QUARK_POS, type_t::make_bool().is_undefined(), false);
}


QUARK_TESTQ("type_t", "make_any()"){
	ut_verify(QUARK_POS, type_t::make_any().get_base_type(), base_type::k_any);
}
QUARK_TESTQ("type_t", "is_any()"){
	ut_verify_auto(QUARK_POS, type_t::make_any().is_any(), true);
}
QUARK_TESTQ("type_t", "is_any()"){
	ut_verify_auto(QUARK_POS, type_t::make_bool().is_any(), false);
}


QUARK_TESTQ("type_t", "make_void()"){
	ut_verify(QUARK_POS, type_t::make_void().get_base_type(), base_type::k_void);
}
QUARK_TESTQ("type_t", "is_void()"){
	ut_verify_auto(QUARK_POS, type_t::make_void().is_void(), true);
}
QUARK_TESTQ("type_t", "is_void()"){
	ut_verify_auto(QUARK_POS, type_t::make_bool().is_void(), false);
}


QUARK_TESTQ("type_t", "make_bool()"){
	ut_verify(QUARK_POS, type_t::make_bool().get_base_type(), base_type::k_bool);
}
QUARK_TESTQ("type_t", "is_bool()"){
	ut_verify_auto(QUARK_POS, type_t::make_bool().is_bool(), true);
}
QUARK_TESTQ("type_t", "is_bool()"){
	ut_verify_auto(QUARK_POS, type_t::make_void().is_bool(), false);
}


QUARK_TESTQ("type_t", "make_int()"){
	ut_verify(QUARK_POS, type_t::make_int().get_base_type(), base_type::k_int);
}
QUARK_TESTQ("type_t", "is_int()"){
	QUARK_UT_VERIFY(type_t::make_int().is_int() == true);
}
QUARK_TESTQ("type_t", "is_int()"){
	QUARK_UT_VERIFY(type_t::make_bool().is_int() == false);
}


QUARK_TESTQ("type_t", "make_double()"){
	QUARK_UT_VERIFY(type_t::make_double().is_double());
}
QUARK_TESTQ("type_t", "is_double()"){
	QUARK_UT_VERIFY(type_t::make_double().is_double() == true);
}
QUARK_TESTQ("type_t", "is_double()"){
	QUARK_UT_VERIFY(type_t::make_bool().is_double() == false);
}


QUARK_TESTQ("type_t", "make_string()"){
	QUARK_UT_VERIFY(type_t::make_string().get_base_type() == base_type::k_string);
}
QUARK_TESTQ("type_t", "is_string()"){
	QUARK_UT_VERIFY(type_t::make_string().is_string() == true);
}
QUARK_TESTQ("type_t", "is_string()"){
	QUARK_UT_VERIFY(type_t::make_bool().is_string() == false);
}


QUARK_TESTQ("type_t", "make_json()"){
	QUARK_UT_VERIFY(type_t::make_json().get_base_type() == base_type::k_json);
}
QUARK_TESTQ("type_t", "is_json()"){
	QUARK_UT_VERIFY(type_t::make_json().is_json() == true);
}
QUARK_TESTQ("type_t", "is_json()"){
	QUARK_UT_VERIFY(type_t::make_bool().is_json() == false);
}


QUARK_TESTQ("type_t", "make_typeid()"){
	QUARK_UT_VERIFY(type_t::make_typeid().get_base_type() == base_type::k_typeid);
}
QUARK_TESTQ("type_t", "is_typeid()"){
	QUARK_UT_VERIFY(type_t::make_typeid().is_typeid() == true);
}
QUARK_TESTQ("type_t", "is_typeid()"){
	QUARK_UT_VERIFY(type_t::make_bool().is_typeid() == false);
}




type_t make_empty_struct(){
	return make_struct({});
}

const auto k_struct_test_members_b = std::vector<member_t>({
	{ type_t::make_int(), "x" },
	{ type_t::make_string(), "y" },
	{ type_t::make_bool(), "z" }
});

type_t make_test_struct_a(){
	return type_t::make_struct2(k_struct_test_members_b);
}

QUARK_TESTQ("type_t", "make_struct2()"){
	QUARK_UT_VERIFY(type_t::make_struct2({}).get_base_type() == base_type::k_struct);
}
QUARK_TESTQ("type_t", "make_struct2()"){
	QUARK_UT_VERIFY(make_empty_struct().get_base_type() == base_type::k_struct);
}
QUARK_TESTQ("type_t", "is_struct()"){
	QUARK_UT_VERIFY(make_empty_struct().is_struct() == true);
}
QUARK_TESTQ("type_t", "is_struct()"){
	QUARK_UT_VERIFY(type_t::make_bool().is_struct() == false);
}

QUARK_TESTQ("type_t", "make_struct2()"){
	const auto t = make_test_struct_a();
	QUARK_UT_VERIFY(t.get_struct() == k_struct_test_members_b);
}
QUARK_TESTQ("type_t", "get_struct()"){
	const auto t = make_test_struct_a();
	QUARK_UT_VERIFY(t.get_struct() == k_struct_test_members_b);
}
QUARK_TESTQ("type_t", "get_struct_ref()"){
	const auto t = make_test_struct_a();
	QUARK_UT_VERIFY(t.get_struct_ref()->_members == k_struct_test_members_b);
}





QUARK_TESTQ("type_t", "make_vector()"){
	QUARK_UT_VERIFY(type_t::make_vector(type_t::make_int()).get_base_type() == base_type::k_vector);
}
QUARK_TESTQ("type_t", "is_vector()"){
	QUARK_UT_VERIFY(type_t::make_vector(type_t::make_int()).is_vector() == true);
}
QUARK_TESTQ("type_t", "is_vector()"){
	QUARK_UT_VERIFY(type_t::make_bool().is_vector() == false);
}
QUARK_TESTQ("type_t", "get_vector_element_type()"){
	QUARK_UT_VERIFY(type_t::make_vector(type_t::make_int()).get_vector_element_type().is_int());
}
QUARK_TESTQ("type_t", "get_vector_element_type()"){
	QUARK_UT_VERIFY(type_t::make_vector(type_t::make_string()).get_vector_element_type().is_string());
}


QUARK_TESTQ("type_t", "make_dict()"){
	QUARK_UT_VERIFY(type_t::make_dict(type_t::make_int()).get_base_type() == base_type::k_dict);
}
QUARK_TESTQ("type_t", "is_dict()"){
	QUARK_UT_VERIFY(type_t::make_dict(type_t::make_int()).is_dict() == true);
}
QUARK_TESTQ("type_t", "is_dict()"){
	QUARK_UT_VERIFY(type_t::make_bool().is_dict() == false);
}
QUARK_TESTQ("type_t", "get_dict_value_type()"){
	QUARK_UT_VERIFY(type_t::make_dict(type_t::make_int()).get_dict_value_type().is_int());
}
QUARK_TESTQ("type_t", "get_dict_value_type()"){
	QUARK_UT_VERIFY(type_t::make_dict(type_t::make_string()).get_dict_value_type().is_string());
}



const auto k_test_function_args_a = std::vector<type_t>({
	{ type_t::make_int() },
	{ type_t::make_string() },
	{ type_t::make_bool() }
});

QUARK_TESTQ("type_t", "make_function()"){
	const auto t = make_function(type_t::make_void(), {}, epure::pure);
	QUARK_UT_VERIFY(t.get_base_type() == base_type::k_function);
}
QUARK_TESTQ("type_t", "is_function()"){
	const auto t = make_function(type_t::make_void(), {}, epure::pure);
	QUARK_UT_VERIFY(t.is_function() == true);
}
QUARK_TESTQ("type_t", "is_function()"){
	QUARK_UT_VERIFY(type_t::make_bool().is_function() == false);
}
QUARK_TESTQ("type_t", "get_function_return()"){
	const auto t = make_function(type_t::make_void(), {}, epure::pure);
	QUARK_UT_VERIFY(t.get_function_return().is_void());
}
QUARK_TESTQ("type_t", "get_function_return()"){
	const auto t = make_function(type_t::make_string(), {}, epure::pure);
	QUARK_UT_VERIFY(t.get_function_return().is_string());
}
QUARK_TESTQ("type_t", "get_function_args()"){
	const auto t = make_function(type_t::make_void(), k_test_function_args_a, epure::pure);
	QUARK_UT_VERIFY(t.get_function_args() == k_test_function_args_a);
}


QUARK_TESTQ("type_t", "is_identifier()"){
	const auto t = type_t::make_identifier("xyz");
	QUARK_UT_VERIFY(t.is_identifier() == true);
}
QUARK_TESTQ("type_t", "is_identifier()"){
	QUARK_UT_VERIFY(type_t::make_bool().is_identifier() == false);
}

QUARK_TESTQ("type_t", "get_unresolved()"){
	const auto t = type_t::make_identifier("xyz");
	QUARK_UT_VERIFY(t.get_identifier() == "xyz");
}
QUARK_TESTQ("type_t", "get_unresolved()"){
	const auto t = type_t::make_identifier("123");
	QUARK_UT_VERIFY(t.get_identifier() == "123");
}










QUARK_TESTQ("type_t", "operator==()"){
	const auto a = type_t::make_vector(type_t::make_int());
	const auto b = type_t::make_vector(type_t::make_int());
	QUARK_UT_VERIFY(a == b);
}
QUARK_TESTQ("type_t", "operator==()"){
	const auto a = type_t::make_vector(type_t::make_int());
	const auto b = type_t::make_dict(type_t::make_int());
	QUARK_UT_VERIFY((a == b) == false);
}
QUARK_TESTQ("type_t", "operator==()"){
	const auto a = type_t::make_vector(type_t::make_int());
	const auto b = type_t::make_vector(type_t::make_string());
	QUARK_UT_VERIFY((a == b) == false);
}


QUARK_TESTQ("type_t", "operator=()"){
	const auto a = type_t::make_bool();
	const auto b = a;
	QUARK_UT_VERIFY(a == b);
}
QUARK_TESTQ("type_t", "operator=()"){
	const auto a = type_t::make_vector(type_t::make_int());
	const auto b = a;
	QUARK_UT_VERIFY(a == b);
}
QUARK_TESTQ("type_t", "operator=()"){
	const auto a = type_t::make_dict(type_t::make_int());
	const auto b = a;
	QUARK_UT_VERIFY(a == b);
}
QUARK_TESTQ("type_t", "operator=()"){
	const auto a = make_function(type_t::make_string(), { type_t::make_int(), type_t::make_double() }, epure::pure);
	const auto b = a;
	QUARK_UT_VERIFY(a == b);
}



//////////////////////////////////////		FORMATS





std::string typeid_to_compact_string(const type_t& t){
//	QUARK_ASSERT(t.check_invariant());

	const auto basetype = t.get_base_type();
	if(basetype == floyd::base_type::k_struct){
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

		return std::string() + "func " + typeid_to_compact_string(ret) + "(" + concat_strings_with_divider(args_str, ",") + ") " + (pure == epure::pure ? "pure" : "impure");
	}
	else if(basetype == floyd::base_type::k_identifier){
		return t.get_identifier();
	}
	else{
		return base_type_to_opcode(basetype);
	}
}




struct typeid_str_test_t {
	type_t _typeid;
	std::string _ast_json;
	std::string _compact_str;
};


const std::vector<typeid_str_test_t> make_typeid_str_tests(){
	const auto s1 = type_t::make_struct2({});

	const auto tests = std::vector<typeid_str_test_t>{
		{ type_t::make_undefined(), quote(base_type_to_opcode(base_type::k_undefined)), base_type_to_opcode(base_type::k_undefined) },
		{ type_t::make_bool(), quote(base_type_to_opcode(base_type::k_bool)), base_type_to_opcode(base_type::k_bool) },
		{ type_t::make_int(), quote(base_type_to_opcode(base_type::k_int)), base_type_to_opcode(base_type::k_int) },
		{ type_t::make_double(), quote(base_type_to_opcode(base_type::k_double)), base_type_to_opcode(base_type::k_double) },
		{ type_t::make_string(), quote(base_type_to_opcode(base_type::k_string)), base_type_to_opcode(base_type::k_string) },

		//	Typeid
		{ type_t::make_typeid(), quote(base_type_to_opcode(base_type::k_typeid)), base_type_to_opcode(base_type::k_typeid) },
		{ type_t::make_typeid(), quote(base_type_to_opcode(base_type::k_typeid)), base_type_to_opcode(base_type::k_typeid) },


//??? vector
//??? dict

		//	Struct
		{ s1, R"(["struct", [[]]])", "struct {}" },
		{
			type_t::make_struct2(
				std::vector<member_t>{
					member_t(type_t::make_int(), "a"),
					member_t(type_t::make_double(), "b")
				}
			),
			R"(["struct", [[{ "type": "int", "name": "a"}, {"type": "double", "name": "b"}]]])",
			"struct {int a;double b;}"
		},


		//	Function
		{
			make_function(type_t::make_bool(), std::vector<type_t>{ type_t::make_int(), type_t::make_double() }, epure::pure),
			R"(["func", "bool", [ "int", "double"]])",
			"func bool(int,double)"
		},


		//	unknown_identifier
		{ type_t::make_identifier("hello"), "\"hello\"", "hello" }
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


member_t::member_t(const floyd::type_t& type, const std::string& name) :
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


std::vector<floyd::type_t> get_member_types(const std::vector<member_t>& m){
	std::vector<floyd::type_t> r;
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



////////////////////////			type_t



int count_function_dynamic_args(const std::vector<type_t>& args){
	int count = 0;
	for(const auto& e: args){
		if(e.is_any()){
			count++;
		}
	}
	return count;
}
int count_function_dynamic_args(const type_t& function_type){
	QUARK_ASSERT(function_type.is_function());

	return count_function_dynamic_args(function_type.get_function_args());
}
bool is_dynamic_function(const type_t& function_type){
	QUARK_ASSERT(function_type.is_function());

	const auto count = count_function_dynamic_args(function_type);
	return count > 0;
}




QUARK_TEST("type_t", "operator==()", "", ""){
	const auto a = make_function(type_t::make_int(), {}, epure::pure);
	const auto b = make_function(type_t::make_int(), {}, epure::pure);
	QUARK_UT_VERIFY(a == b);
}






std::vector<json_t> typeids_to_json_array(const std::vector<type_t>& m);
std::vector<type_t> typeids_from_json_array(const std::vector<json_t>& m);



json_t struct_definition_to_ast_json(const struct_definition_t& v){
	QUARK_ASSERT(v.check_invariant());

	return json_t::make_array({
		members_to_json(v._members)
	});
}



json_t typeid_to_ast_json(const type_t& t){
	QUARK_ASSERT(t.check_invariant());

	const auto b = t.get_base_type();
	const auto basetype_str = base_type_to_opcode(b);

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
		return basetype_str;
	}
	else if(b == base_type::k_struct){
		const auto struct_def = t.get_struct();
		return json_t::make_array({
			json_t(basetype_str),
			struct_definition_to_ast_json(struct_def)
		});
	}
	else if(b == base_type::k_vector){
		const auto d = t.get_vector_element_type();
		return json_t::make_array({
			json_t(basetype_str),
			typeid_to_ast_json(d)
		});
	}
	else if(b == base_type::k_dict){
		const auto d = t.get_dict_value_type();
		return json_t::make_array({
			json_t(basetype_str),
			typeid_to_ast_json(d)
		});
	}
	else if(b == base_type::k_function){
		//	Only include dyn-type it it's different than return_dyn_type::none.
		const auto d = t.get_function_dyn_return_type();
		const auto dyn_type = d != type_t::return_dyn_type::none ? json_t(static_cast<int>(d)) : json_t();
		return make_array_skip_nulls({
			basetype_str,
			typeid_to_ast_json(t.get_function_return()),
			typeids_to_json_array(t.get_function_args()),
			t.get_function_pure() == epure::pure ? true : false,
			dyn_type
		});
	}
	else if(b == base_type::k_identifier){
		const auto identifier = t.get_identifier();
		if(is_type_name(identifier)){
			return json_t(t.get_identifier());
		}
		else {
			return json_t(std::string("%") + t.get_identifier());
		}
	}
	else{
		QUARK_ASSERT(false);
		quark::throw_logic_error();
	}
	return basetype_str;
}



QUARK_TEST("", "typeid_to_ast_json()", "", ""){
	const auto t = type_t::make_int();
	const auto r = json_to_compact_string_minimal_quotes(typeid_to_ast_json(t));
	QUARK_UT_VERIFY(r == "^int");
}

QUARK_TEST("", "typeid_to_ast_json()", "", ""){
	const auto t = type_t::make_int();
	const auto r = json_to_compact_string_minimal_quotes(typeid_to_ast_json(t));
	QUARK_UT_VERIFY(r == "{ desc: ^int, name: coord_t }");
}

QUARK_TEST("", "typeid_to_ast_json()", "", ""){
	const auto t = type_t::make_undefined();
	const auto r = json_to_compact_string_minimal_quotes(typeid_to_ast_json(t));
	QUARK_UT_VERIFY(r == "#coord_t");
}




static type_t typeid_from_json0(const json_t& t){
	QUARK_ASSERT(t.check_invariant());

	if(t.is_string()){
		const auto s0 = t.get_string();

		//	Identifier.
		if(s0.front() == '%'){
			const auto s = s0.substr(1);
			return type_t::make_identifier(s);
		}

		//	Tagged type.
		else if(is_type_name(s0)){
			return type_t::make_identifier(s0);
		}

		//	Other types.
		else {
			const auto s = s0;
			if(s0 == ""){
				return type_t::make_undefined();
			}

			const auto b = opcode_to_base_type(s);


			if(b == base_type::k_undefined){
				return type_t::make_undefined();
			}
			else if(b == base_type::k_any){
				return type_t::make_any();
			}
			else if(b == base_type::k_void){
				return type_t::make_void();
			}
			else if(b == base_type::k_bool){
				return type_t::make_bool();
			}
			else if(b == base_type::k_int){
				return type_t::make_int();
			}
			else if(b == base_type::k_double){
				return type_t::make_double();
			}
			else if(b == base_type::k_string){
				return type_t::make_string();
			}
			else if(b == base_type::k_typeid){
				return type_t::make_typeid();
			}
			else if(b == base_type::k_json){
				return type_t::make_json();
			}
			else{
				quark::throw_exception();
			}
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
			return type_t::make_typeid(t3);
		}
		else
*/
		if(s == "struct"){
			const auto struct_def_array = a[1].get_array();
			const auto member_array = struct_def_array[0].get_array();

			const std::vector<member_t> struct_members = members_from_json(member_array);
			return type_t::make_struct(struct_type_desc_t { struct_members });
		}
		else if(s == "vector"){
			const auto element_type = typeid_from_ast_json(a[1]);
			return type_t::make_vector(element_type);
		}
		else if(s == "dict"){
			const auto value_type = typeid_from_ast_json(a[1]);
			return type_t::make_dict(value_type);
		}
		else if(s == "func"){
			const auto ret_type = typeid_from_ast_json(a[1]);
			const auto arg_types_array = a[2].get_array();
			const std::vector<type_t> arg_types = typeids_from_json_array(arg_types_array);

			if(a[3].is_true() == false && a[3].is_false() == false){
				quark::throw_exception();
			}
			const bool pure = a[3].is_true();

			if(a.size() > 4){
				const auto dyn = static_cast<type_t::return_dyn_type>(a[4].get_number());
				if(dyn == type_t::return_dyn_type::none){
					return make_function(ret_type, arg_types, pure ? epure::pure : epure::impure);
				}
				else{
					return make_function_dyn_return(arg_types, pure ? epure::pure : epure::impure, dyn);
				}
			}
			else{
				return make_function(ret_type, arg_types, pure ? epure::pure : epure::impure);
			}
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
	return type_t::make_undefined();
}

type_t typeid_from_ast_json(const json_t& t2){
	QUARK_ASSERT(t2.check_invariant());

	return typeid_from_json0(t2);
}



QUARK_TEST("", "typeid_from_ast_json()", "", ""){
	const auto t = type_t::make_int();
	const auto j = typeid_to_ast_json(t);
	const auto r = typeid_from_ast_json(j);
	QUARK_ASSERT(r == t);
}

QUARK_TEST("", "typeid_from_ast_json()", "", ""){
	const auto t = type_t::make_int();
	const auto j = typeid_to_ast_json(t);
	const auto r = typeid_from_ast_json(j);
	QUARK_ASSERT(r == t);
}

QUARK_TEST("", "typeid_from_ast_json()", "", ""){
	const auto t = type_t::make_undefined();
	const auto j = typeid_to_ast_json(t);
	const auto r = typeid_from_ast_json(j);
	QUARK_ASSERT(r == t);
}




std::vector<json_t> typeids_to_json_array(const std::vector<type_t>& m){
	std::vector<json_t> r;
	for(const auto& a: m){
		r.push_back(typeid_to_ast_json(a));
	}
	return r;
}
std::vector<type_t> typeids_from_json_array(const std::vector<json_t>& m){
	std::vector<type_t> r;
	for(const auto& a: m){
		r.push_back(typeid_from_ast_json(a));
	}
	return r;
}



json_t members_to_json(const std::vector<member_t>& members){
	std::vector<json_t> r;
	for(const auto& i: members){
		const auto member = make_object({
			{ "type", typeid_to_ast_json(i._type) },
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

void ut_verify(const quark::call_context_t& context, const type_t& result, const type_t& expected){
	QUARK_ASSERT(result.check_invariant());
	QUARK_ASSERT(expected.check_invariant());

	ut_verify(
		context,
		typeid_to_ast_json(result),
		typeid_to_ast_json(expected)
	);
}

#endif





































static type_t lookup_type_from_index_it(const type_interner_t& interner, size_t type_index){
	QUARK_ASSERT(interner.check_invariant());
	QUARK_ASSERT(type_index >= 0 && type_index < interner.interned2.size());

	return lookup_type_from_index(interner, static_cast<type_lookup_index_t>(type_index));
}


std::vector<type_t> get_member_types(const std::vector<member_t>& m){
	std::vector<type_t> r;
	for(const auto& a: m){
		r.push_back(a._type);
	}
	return r;
}


json_t members_to_json(const type_interner_t& interner, const std::vector<member_t>& members){
	std::vector<json_t> r;
	for(const auto& i: members){
		const auto member = make_object({
			{ "type", type_to_json(interner, i._type) },
			{ "name", json_t(i._name) }
		});
		r.push_back(json_t(member));
	}
	return r;
}

std::vector<member_t> members_from_json(type_interner_t& interner, const json_t& members){
	QUARK_ASSERT(members.check_invariant());

	std::vector<member_t> r;
	for(const auto& i: members.get_array()){
		const auto m = member_t {
			type_from_json(interner, i.get_object_element("type")),
			i.get_object_element("name").get_string()
		};

		r.push_back(m);
	}
	return r;
}


type_variant_t get_type_variant(const type_interner_t& interner, const type_t& type){
	QUARK_ASSERT(interner.check_invariant());
	QUARK_ASSERT(type.check_invariant());

	if(type.is_undefined()){
		return undefined_t {};
	}
	else if(type.is_any()){
		return any_t {};
	}
	else if(type.is_void()){
		return void_t {};
	}


	else if(type.is_bool()){
		return bool_t {};
	}
	else if(type.is_int()){
		return int_t {};
	}
	else if(type.is_double()){
		return double_t {};
	}
	else if(type.is_string()){
		return string_t {};
	}
	else if(type.is_json()){
		return json_type_t {};
	}
	else if(type.is_typeid()){
		return typeid_type_t {};
	}

	else if(type.is_struct()){
		const auto& info = lookup_typeinfo_from_type(interner, type);
		return struct_t { info.struct_def };
	}
	else if(type.is_vector()){
		const auto& info = lookup_typeinfo_from_type(interner, type);
		return vector_t { info.child_types };
	}
	else if(type.is_dict()){
		const auto& info = lookup_typeinfo_from_type(interner, type);
		return dict_t { info.child_types };
	}
	else if(type.is_function()){
		const auto& info = lookup_typeinfo_from_type(interner, type);
		return function_t { info.child_types };
	}
	else if(type.is_symbol_ref()){
		const auto& info = lookup_typeinfo_from_type(interner, type);
		return symbol_ref_t { info.identifier_str };
	}
	else if(type.is_named_type()){
		const auto& info = lookup_typeinfo_from_type(interner, type);
		return named_type_t { info.child_types[0] };
	}
	else{
		QUARK_ASSERT(false);
		throw std::exception();
	}
}




//////////////////////////////////////////////////		type_t





type_t type_t::get_vector_element_type(const type_interner_t& interner) const{
	QUARK_ASSERT(check_invariant());
	QUARK_ASSERT(interner.check_invariant());
	QUARK_ASSERT(is_vector());

	const auto& info = lookup_typeinfo_from_type(interner, *this);
	return info.child_types[0];
}
type_t type_t::get_dict_value_type(const type_interner_t& interner) const{
	QUARK_ASSERT(check_invariant());
	QUARK_ASSERT(interner.check_invariant());
	QUARK_ASSERT(is_dict());

	const auto& info = lookup_typeinfo_from_type(interner, *this);
	return info.child_types[0];
}




struct_type_desc_t type_t::get_struct(const type_interner_t& interner) const{
	QUARK_ASSERT(check_invariant());
	QUARK_ASSERT(interner.check_invariant());
	QUARK_ASSERT(is_struct());

	const auto& info = lookup_typeinfo_from_type(interner, *this);
	return info.struct_def;
}



type_t type_t::get_function_return(const type_interner_t& interner) const{
	QUARK_ASSERT(check_invariant());
	QUARK_ASSERT(is_function());
	QUARK_ASSERT(interner.check_invariant());

	const auto& info = lookup_typeinfo_from_type(interner, *this);
	return info.child_types[0];
}

std::vector<type_t> type_t::get_function_args(const type_interner_t& interner) const{
	QUARK_ASSERT(check_invariant());
	QUARK_ASSERT(is_function());
	QUARK_ASSERT(interner.check_invariant());

	const auto& info = lookup_typeinfo_from_type(interner, *this);
	return std::vector<type_t>(info.child_types.begin() + 1, info.child_types.end());
}

return_dyn_type type_t::get_function_dyn_return_type(const type_interner_t& interner) const{
	QUARK_ASSERT(check_invariant());
	QUARK_ASSERT(is_function());
	QUARK_ASSERT(interner.check_invariant());

	const auto& info = lookup_typeinfo_from_type(interner, *this);
	return info.func_return_dyn_type;
}

epure type_t::get_function_pure(const type_interner_t& interner) const{
	QUARK_ASSERT(check_invariant());
	QUARK_ASSERT(is_function());
	QUARK_ASSERT(interner.check_invariant());

	const auto& info = lookup_typeinfo_from_type(interner, *this);
	return info.func_pure;
}



std::string type_t::get_symbol_ref(const type_interner_t& interner) const {
	QUARK_ASSERT(check_invariant());
	QUARK_ASSERT(is_symbol_ref());

	const auto& info = lookup_typeinfo_from_type(interner, *this);
	return info.identifier_str;
}


type_name_t type_t::get_named_type(const type_interner_t& interner) const {
	QUARK_ASSERT(check_invariant());
	QUARK_ASSERT(is_named_type());

	const auto& info = lookup_typeinfo_from_type(interner, *this);
	return info.optional_name;
}



int find_struct_member_index(const struct_type_desc_t& def, const std::string& name){
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




/////////////////////////////////////////////////		type_interner_t



static type_node_t make_entry(const base_type& bt){
	return type_node_t{
		make_empty_type_name(),
		bt,
		std::vector<type_t>{},
		{},
		epure::pure,
		return_dyn_type::none,
		""
	};
}

type_interner_t::type_interner_t(){
	//	Order is designed to match up the interned2[] with base_type indexes.
	interned2.push_back(make_entry(base_type::k_undefined));
	interned2.push_back(make_entry(base_type::k_any));
	interned2.push_back(make_entry(base_type::k_void));


	interned2.push_back(make_entry(base_type::k_bool));
	interned2.push_back(make_entry(base_type::k_int));
	interned2.push_back(make_entry(base_type::k_double));
	interned2.push_back(make_entry(base_type::k_string));
	interned2.push_back(make_entry(base_type::k_json));

	interned2.push_back(make_entry(base_type::k_typeid));

	//	These are complex types and are undefined. We need them to take up space in the interned2-vector.
	interned2.push_back(make_entry(base_type::k_undefined));
	interned2.push_back(make_entry(base_type::k_undefined));
	interned2.push_back(make_entry(base_type::k_undefined));
	interned2.push_back(make_entry(base_type::k_undefined));

	interned2.push_back(make_entry(base_type::k_symbol_ref));
	interned2.push_back(make_entry(base_type::k_undefined));

	QUARK_ASSERT(check_invariant());
}

bool type_interner_t::check_invariant() const {
	QUARK_ASSERT(interned2.size() < INT_MAX);

	QUARK_ASSERT(interned2[(type_lookup_index_t)base_type::k_undefined] == make_entry(base_type::k_undefined));
	QUARK_ASSERT(interned2[(type_lookup_index_t)base_type::k_any] == make_entry(base_type::k_any));
	QUARK_ASSERT(interned2[(type_lookup_index_t)base_type::k_void] == make_entry(base_type::k_void));

	QUARK_ASSERT(interned2[(type_lookup_index_t)base_type::k_bool] == make_entry(base_type::k_bool));
	QUARK_ASSERT(interned2[(type_lookup_index_t)base_type::k_int] == make_entry(base_type::k_int));
	QUARK_ASSERT(interned2[(type_lookup_index_t)base_type::k_double] == make_entry(base_type::k_double));
	QUARK_ASSERT(interned2[(type_lookup_index_t)base_type::k_string] == make_entry(base_type::k_string));
	QUARK_ASSERT(interned2[(type_lookup_index_t)base_type::k_json] == make_entry(base_type::k_json));

	QUARK_ASSERT(interned2[(type_lookup_index_t)base_type::k_typeid] == make_entry(base_type::k_typeid));

	QUARK_ASSERT(interned2[(type_lookup_index_t)base_type::k_struct] == make_entry(base_type::k_undefined));
	QUARK_ASSERT(interned2[(type_lookup_index_t)base_type::k_vector] == make_entry(base_type::k_undefined));
	QUARK_ASSERT(interned2[(type_lookup_index_t)base_type::k_dict] == make_entry(base_type::k_undefined));
	QUARK_ASSERT(interned2[(type_lookup_index_t)base_type::k_function] == make_entry(base_type::k_undefined));

	QUARK_ASSERT(interned2[(type_lookup_index_t)base_type::k_symbol_ref] == make_entry(base_type::k_symbol_ref));
	QUARK_ASSERT(interned2[(type_lookup_index_t)base_type::k_named_type] == make_entry(base_type::k_undefined));


	//??? Add other common combinations, like vectors with each atomic type, dict with each atomic
	//	type. This allows us to hardcoded their type indexes.
	return true;
}



static type_t lookup_node(const type_interner_t& interner, const type_node_t& node){
	QUARK_ASSERT(interner.check_invariant());

	const auto it = std::find_if(
		interner.interned2.begin(),
		interner.interned2.end(),
		[&](const auto& e){ return e == node; }
	);

	if(it != interner.interned2.end()){
		return lookup_type_from_index_it(interner, it - interner.interned2.begin());
	}
	else{
		throw std::exception();
//		return type_t::make_undefined();
	}
}



static type_t intern_node(type_interner_t& interner, const type_node_t& node){
	QUARK_ASSERT(interner.check_invariant());

	const auto it = std::find_if(
		interner.interned2.begin(),
		interner.interned2.end(),
		[&](const auto& e){ return e == node; }
	);

	if(it != interner.interned2.end()){
		return lookup_type_from_index_it(interner, it - interner.interned2.begin());
	}

	//	New type, store it.
	else{

		//	All child type are guaranteed to have types already since those are specified using types_t:s.
		interner.interned2.push_back(node);
		return lookup_type_from_index_it(interner, interner.interned2.size() - 1);
	}
}


type_t new_tagged_type(type_interner_t& interner, const type_name_t& tag){
	QUARK_ASSERT(interner.check_invariant());
	QUARK_ASSERT(tag.check_invariant());

	const auto type = type_t::make_undefined();
	const auto result = new_tagged_type(interner, tag, type);
	return result;
}

type_t new_tagged_type(type_interner_t& interner, const type_name_t& tag, const type_t& type){
	QUARK_ASSERT(interner.check_invariant());
	QUARK_ASSERT(tag.check_invariant());
	QUARK_ASSERT(type.check_invariant());

	if(false) trace_type_interner(interner);

	const auto it = std::find_if(
		interner.interned2.begin(),
		interner.interned2.end(),
		[&](const auto& e){ return e.optional_name == tag; }
	);
	if(it != interner.interned2.end()){
		throw std::exception();
	}

	const auto node = type_node_t{
		tag,
		base_type::k_named_type,
		{ type },
		{},
		epure::pure,
		return_dyn_type::none,
		""
	};

	QUARK_ASSERT(node.child_types.size() == 1);

	//	Can't use intern_node() since we have a tag.
	interner.interned2.push_back(node);
	return lookup_type_from_index_it(interner, interner.interned2.size() - 1);
}

type_t update_tagged_type(type_interner_t& interner, const type_t& named, const type_t& type){
	QUARK_ASSERT(interner.check_invariant());
	QUARK_ASSERT(named.check_invariant());
	QUARK_ASSERT(type.check_invariant());

	auto& node = lookup_typeinfo_from_type(interner, named);
	QUARK_ASSERT(node.bt == base_type::k_named_type);
	QUARK_ASSERT(node.child_types.size() == 1);

	node.child_types = { type };

	//	Returns a new type for the named tag, so it contains the updated byte_type info.
	return lookup_type_from_index(interner, named.get_lookup_index());
}

type_t get_tagged_type2(const type_interner_t& interner, const type_name_t& tag){
	QUARK_ASSERT(interner.check_invariant());
	QUARK_ASSERT(tag.check_invariant());

	const auto it = std::find_if(
		interner.interned2.begin(),
		interner.interned2.end(),
		[&](const auto& e){ return e.optional_name == tag; }
	);
	if(it == interner.interned2.end()){
		throw std::exception();
	}
	QUARK_ASSERT(it->child_types.size() == 1);
	return lookup_type_from_index_it(interner, it - interner.interned2.begin());
}


type_t peek(const type_interner_t& interner, const type_t& type){
	QUARK_ASSERT(interner.check_invariant());
	QUARK_ASSERT(type.check_invariant());

	if(false) trace_type_interner(interner);

	const auto& info = lookup_typeinfo_from_type(interner, type);
	if(info.bt == base_type::k_named_type){
		QUARK_ASSERT(info.child_types.size() == 1);
		const auto dest = info.child_types[0];

		//	Support many linked tags.
		return peek(interner, dest);
	}
	else{
		return type;
	}
}

type_t refresh_type(const type_interner_t& interner, const type_t& type){
	const auto lookup_index = type.get_lookup_index();
	QUARK_ASSERT(lookup_index >= 0);
	QUARK_ASSERT(lookup_index < interner.interned2.size());

	return lookup_type_from_index(interner, type.get_lookup_index());
}



type_t lookup_type_from_index(const type_interner_t& interner, type_lookup_index_t type_index){
	QUARK_ASSERT(interner.check_invariant());
	QUARK_ASSERT(type_index >= 0 && type_index < interner.interned2.size());

	const auto& node = interner.interned2[type_index];

	if(node.bt == base_type::k_struct){
		return type_t(type_t::assemble(type_index, base_type::k_struct, base_type::k_undefined));
	}
	else if(node.bt == base_type::k_vector){
		const auto element_bt = node.child_types[0].get_base_type();
		return type_t(type_t::assemble(type_index, base_type::k_vector, element_bt));
	}
	else if(node.bt == base_type::k_dict){
		const auto value_bt = node.child_types[0].get_base_type();
		return type_t(type_t::assemble(type_index, base_type::k_dict, value_bt));
	}
	else if(node.bt == base_type::k_function){
		//??? We could keep the return type inside the type
		return type_t(type_t::assemble(type_index, base_type::k_function, base_type::k_undefined));
	}
	else if(node.bt == base_type::k_symbol_ref){
		return type_t::assemble2(type_index, base_type::k_symbol_ref, base_type::k_undefined);
	}
	else if(node.bt == base_type::k_named_type){
		return type_t::assemble2(type_index, base_type::k_named_type, base_type::k_undefined);
	}
	else{
		const auto bt = node.bt;
		return type_t::assemble2((type_lookup_index_t)type_index, bt, base_type::k_undefined);
	}
}

type_t lookup_type_from_name(const type_interner_t& interner, const type_name_t& tag){
	QUARK_ASSERT(interner.check_invariant());

	if(tag.lexical_path.empty()){
		throw std::exception();
	}
	else{
		const auto it = std::find_if(
			interner.interned2.begin(),
			interner.interned2.end(),
			[&](const auto& e){ return e.optional_name == tag; }
		);
		if(it == interner.interned2.end()){
			throw std::exception();
		}

		return lookup_type_from_index_it(interner, it - interner.interned2.begin());
	}
}



void trace_type_interner(const type_interner_t& interner){
	QUARK_ASSERT(interner.check_invariant());

	{
		QUARK_SCOPED_TRACE("TYPES");

		{
			QUARK_SCOPED_TRACE("TYPES");
			std::vector<std::vector<std::string>> matrix;
			for(auto i = 0 ; i < interner.interned2.size() ; i++){
				const auto& type = lookup_type_from_index(interner, i);

				const auto& e = interner.interned2[i];
				const auto contents = e.bt == base_type::k_named_type
					? std::to_string(e.child_types[0].get_lookup_index())
					: type_to_compact_string(interner, type, resolve_named_types::dont_resolve);

				const auto line = std::vector<std::string>{
					std::to_string(i),
					pack_type_name(e.optional_name),
					base_type_to_opcode(e.bt),
					contents,
				};
				matrix.push_back(line);
			}

			const auto result = generate_table_type1({ "type_t", "name-tag", "base_type", "contents" }, matrix);
			QUARK_TRACE(result);
		}
	}
}



json_t type_to_json_shallow(const type_t& type){
	const auto s = std::string("type:") + std::to_string(type.get_lookup_index());
	return json_t(s);
}

static json_t struct_definition_to_json(const type_interner_t& interner, const struct_type_desc_t& v){
	QUARK_ASSERT(v.check_invariant());

	return json_t::make_array({
		members_to_json(interner, v._members)
	});
}

static std::vector<json_t> typeids_to_json_array(const type_interner_t& interner, const std::vector<type_t>& m){
	std::vector<json_t> r;
	for(const auto& a: m){
		r.push_back(type_to_json(interner, a));
	}
	return r;
}
static std::vector<type_t> typeids_from_json_array(type_interner_t& interner, const std::vector<json_t>& m){
	std::vector<type_t> r;
	for(const auto& a: m){
		r.push_back(type_from_json(interner, a));
	}
	return r;
}

json_t type_to_json(const type_interner_t& interner, const type_t& type){
	QUARK_ASSERT(interner.check_invariant());
	QUARK_ASSERT(type.check_invariant());

	const auto bt = type.get_base_type();
	const auto basetype_str = base_type_to_opcode(bt);

	struct visitor_t {
		const type_interner_t& interner;
		const type_t& type;
		const std::string basetype_str;


		json_t operator()(const undefined_t& e) const{
			return basetype_str;
		}
		json_t operator()(const any_t& e) const{
			return basetype_str;
		}

		json_t operator()(const void_t& e) const{
			return basetype_str;
		}
		json_t operator()(const bool_t& e) const{
			return basetype_str;
		}
		json_t operator()(const int_t& e) const{
			return basetype_str;
		}
		json_t operator()(const double_t& e) const{
			return basetype_str;
		}
		json_t operator()(const string_t& e) const{
			return basetype_str;
		}

		json_t operator()(const json_type_t& e) const{
			return basetype_str;
		}
		json_t operator()(const typeid_type_t& e) const{
			return basetype_str;
		}

		json_t operator()(const struct_t& e) const{
			return json_t::make_array(
				{
					json_t(basetype_str),
					struct_definition_to_json(interner, e.def)
				}
			);
		}
		json_t operator()(const vector_t& e) const{
			const auto d = type.get_vector_element_type(interner);
			return json_t::make_array( { json_t(basetype_str), type_to_json(interner, d) });
		}
		json_t operator()(const dict_t& e) const{
			const auto d = type.get_dict_value_type(interner);
			return json_t::make_array( { json_t(basetype_str), type_to_json(interner, d) });
		}
		json_t operator()(const function_t& e) const{
			const auto ret = type.get_function_return(interner);
			const auto args = type.get_function_args(interner);
			const auto pure = type.get_function_pure(interner);
			const auto dyn = type.get_function_dyn_return_type(interner);

			//	Only include dyn-type it it's different than return_dyn_type::none.
			const auto dyn_type = dyn != return_dyn_type::none ? json_t(static_cast<int>(dyn)) : json_t();
			return make_array_skip_nulls({
				basetype_str,
				type_to_json(interner, ret),
				typeids_to_json_array(interner, args),
				pure == epure::pure ? true : false,
				dyn_type
			});

			std::vector<std::string> args_str;
			for(const auto& a: args){
				args_str.push_back(type_to_compact_string(interner, a, resolve_named_types::dont_resolve));
			}

			return std::string() + "func " + type_to_compact_string(interner, ret, resolve_named_types::dont_resolve) + "(" + concat_strings_with_divider(args_str, ",") + ") " + (pure == epure::pure ? "pure" : "impure");
		}
		json_t operator()(const symbol_ref_t& e) const {
			const auto identifier = e.s;
			return json_t(std::string("%") + identifier);
		}
		json_t operator()(const named_type_t& e) const {
			const auto& tag = type.get_named_type(interner);
			return pack_type_name(tag);
//			return type_to_json_shallow(e.destination_type);
		}
	};

	const auto result = std::visit(visitor_t{ interner, type, basetype_str }, get_type_variant(interner, type));
	return result;
}

type_t type_from_json(type_interner_t& interner, const json_t& t){
	QUARK_ASSERT(interner.check_invariant());
	QUARK_ASSERT(t.check_invariant());

	if(t.is_string()){
		const auto s = t.get_string();

		//	Identifier.
		if(s.front() == '%'){
			return make_symbol_ref(interner, s.substr(1));
		}

		//	Tagged type.
		else if(is_type_name(s)){
			return make_named_type(interner, unpack_type_name(s));
		}

		//	Other types.
		else {
			if(s == ""){
				return type_t::make_undefined();
			}
			const auto b = opcode_to_base_type(s);

			if(b == base_type::k_undefined){
				return type_t::make_undefined();
			}
			else if(b == base_type::k_any){
				return type_t::make_any();
			}
			else if(b == base_type::k_void){
				return type_t::make_void();
			}
			else if(b == base_type::k_bool){
				return type_t::make_bool();
			}
			else if(b == base_type::k_int){
				return type_t::make_int();
			}
			else if(b == base_type::k_double){
				return type_t::make_double();
			}
			else if(b == base_type::k_string){
				return type_t::make_string();
			}
			else if(b == base_type::k_typeid){
				return type_t::make_typeid();
			}
			else if(b == base_type::k_json){
				return type_t::make_json();
			}
			else{
				quark::throw_exception();
			}
		}
	}
	else if(t.is_array()){
		const auto a = t.get_array();
		const auto s = a[0].get_string();
		QUARK_ASSERT(s.front() != '#');
		QUARK_ASSERT(s.front() != '^');
		QUARK_ASSERT(s.front() != '%');
		QUARK_ASSERT(s.front() != '/');

		if(s == "struct"){
			const auto struct_def_array = a[1].get_array();
			const auto member_array = struct_def_array[0].get_array();

			const std::vector<member_t> struct_members = members_from_json(interner, member_array);
			return make_struct(interner, struct_members);
		}
		else if(s == "vector"){
			const auto element_type = type_from_json(interner, a[1]);
			return make_vector(interner, element_type);
		}
		else if(s == "dict"){
			const auto value_type = type_from_json(interner, a[1]);
			return make_dict(interner, value_type);
		}
		else if(s == "func"){
			const auto ret_type = type_from_json(interner, a[1]);
			const auto arg_types_array = a[2].get_array();
			const std::vector<type_t> arg_types = typeids_from_json_array(interner, arg_types_array);

			if(a[3].is_true() == false && a[3].is_false() == false){
				quark::throw_exception();
			}
			const bool pure = a[3].is_true();

			if(a.size() > 4){
				const auto dyn = static_cast<return_dyn_type>(a[4].get_number());
				if(dyn == return_dyn_type::none){
					return make_function(interner, ret_type, arg_types, pure ? epure::pure : epure::impure);
				}
				else{
					return make_function_dyn_return(interner, arg_types, pure ? epure::pure : epure::impure, dyn);
				}
			}
			else{
				return make_function(interner, ret_type, arg_types, pure ? epure::pure : epure::impure);
			}
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
	return type_t::make_undefined();
}



//??? General note: port tests from typeid.h


type_t type_from_json_shallow(const json_t& j){
	QUARK_ASSERT(j.check_invariant());

	if(j.is_string() && j.get_string().substr(0, 6) == "type:"){
		const auto n_str = j.get_string().substr(6);
		const auto i = std::stoi(n_str);
		const auto type = type_t(i);
		return type;
	}
	else{
		throw std::exception();
	}
}

std::string type_to_debug_string(const type_t& type){
	std::stringstream s;
	s << "{ " << type.get_lookup_index() << ": " << base_type_to_opcode(type.get_base_type()) << " }";
	return s.str();
}

std::string type_to_compact_string(const type_interner_t& interner, const type_t& type, resolve_named_types resolve){
	QUARK_ASSERT(interner.check_invariant());
	QUARK_ASSERT(type.check_invariant());

	struct visitor_t {
		const type_interner_t& interner;
		const type_t& type;
		const resolve_named_types resolve;


		std::string operator()(const undefined_t& e) const{
			return "undef";
		}
		std::string operator()(const any_t& e) const{
			return "any";
		}

		std::string operator()(const void_t& e) const{
			return "void";
		}
		std::string operator()(const bool_t& e) const{
			return "bool";
		}
		std::string operator()(const int_t& e) const{
			return "int";
		}
		std::string operator()(const double_t& e) const{
			return "double";
		}
		std::string operator()(const string_t& e) const{
			return "string";
		}

		std::string operator()(const json_type_t& e) const{
			return "json";
		}
		std::string operator()(const typeid_type_t& e) const{
			return "typeid";
		}

		std::string operator()(const struct_t& e) const{
			std::string members_acc;
			for(const auto& m: e.def._members){
				members_acc = type_to_compact_string(interner, m._type, resolve) + " " + members_acc + m._name + ";";
			}
			return "struct {" + members_acc + "}";
		}
		std::string operator()(const vector_t& e) const{
			QUARK_ASSERT(e._parts.size() == 1);

			return "[" + type_to_compact_string(interner, e._parts[0], resolve) + "]";
		}
		std::string operator()(const dict_t& e) const{
			QUARK_ASSERT(e._parts.size() == 1);

			return "[string:" + type_to_compact_string(interner, e._parts[0], resolve) + "]";
		}
		std::string operator()(const function_t& e) const{
			const auto ret = type.get_function_return(interner);
			const auto args = type.get_function_args(interner);
			const auto pure = type.get_function_pure(interner);

			std::vector<std::string> args_str;
			for(const auto& a: args){
				args_str.push_back(type_to_compact_string(interner, a, resolve));
			}

			return std::string() + "func " + type_to_compact_string(interner, ret, resolve) + "(" + concat_strings_with_divider(args_str, ",") + ") " + (pure == epure::pure ? "pure" : "impure");
		}
		std::string operator()(const symbol_ref_t& e) const {
//			QUARK_ASSERT(e.s != "");
			return std::string("%") + e.s;
		}
		std::string operator()(const named_type_t& e) const {
			if(resolve == resolve_named_types::resolve){
				const auto p = peek(interner, type);
				return type_to_compact_string(interner, p, resolve);
			}

			//	Return the name of the type.
			else if(resolve == resolve_named_types::dont_resolve){
				const auto& info = lookup_typeinfo_from_type(interner, type);
				return info.optional_name.lexical_path.back();
			}
			else{
				QUARK_ASSERT(false);
				throw std::exception();
			}
		}
	};

	const auto result = std::visit(visitor_t{ interner, type, resolve }, get_type_variant(interner, type));
	return result;
}


type_t make_struct(type_interner_t& interner, const struct_type_desc_t& struct_def){
	const auto node = type_node_t{
		make_empty_type_name(),
		base_type::k_struct,
		std::vector<type_t>{},
		struct_def,
		epure::pure,
		return_dyn_type::none,
		""
	};
	return intern_node(interner, node);
}

type_t make_struct(const type_interner_t& interner, const struct_type_desc_t& struct_def){
	const auto node = type_node_t{
		make_empty_type_name(),
		base_type::k_struct,
		std::vector<type_t>{},
		struct_def,
		epure::pure,
		return_dyn_type::none,
		""
	};
	return lookup_node(interner, node);
}


type_t make_vector(type_interner_t& interner, const type_t& element_type){
	const auto node = type_node_t{
		make_empty_type_name(),
		base_type::k_vector,
		{ element_type },
		{},
		epure::pure,
		return_dyn_type::none,
		""
	};
	return intern_node(interner, node);
}
type_t make_vector(const type_interner_t& interner, const type_t& element_type){
	const auto node = type_node_t{
		make_empty_type_name(),
		base_type::k_vector,
		{ element_type },
		{},
		epure::pure,
		return_dyn_type::none,
		""
	};
	return lookup_node(interner, node);
}

type_t make_dict(type_interner_t& interner, const type_t& value_type){
	const auto node = type_node_t{
		make_empty_type_name(),
		base_type::k_dict,
		{ value_type },
		{},
		epure::pure,
		return_dyn_type::none,
		""
	};
	return intern_node(interner, node);
}

type_t make_dict(const type_interner_t& interner, const type_t& value_type){
	const auto node = type_node_t{
		make_empty_type_name(),
		base_type::k_dict,
		{ value_type },
		{},
		epure::pure,
		return_dyn_type::none,
		""
	};
	return lookup_node(interner, node);
}

type_t make_function3(type_interner_t& interner, const type_t& ret, const std::vector<type_t>& args, epure pure, return_dyn_type dyn_return){
	const auto node = type_node_t{
		make_empty_type_name(),
		base_type::k_function,
		concat(
			std::vector<type_t>{ ret },
			args
		),
		{},
		pure,
		dyn_return,
		""
	};
	return intern_node(interner, node);
}


type_t make_function3(const type_interner_t& interner, const type_t& ret, const std::vector<type_t>& args, epure pure, return_dyn_type dyn_return){
	const auto node = type_node_t{
		make_empty_type_name(),
		base_type::k_function,
		concat(
			std::vector<type_t>{ ret },
			args
		),
		{},
		pure,
		dyn_return,
		""
	};
	return lookup_node(interner, node);
}

type_t make_function_dyn_return(type_interner_t& interner, const std::vector<type_t>& args, epure pure, return_dyn_type dyn_return){
	return make_function3(interner, type_t::make_any(), args, pure, dyn_return);
}

type_t make_function(type_interner_t& interner, const type_t& ret, const std::vector<type_t>& args, epure pure){
	QUARK_ASSERT(ret.is_any() == false);

	return make_function3(interner, ret, args, pure, return_dyn_type::none);
}

type_t make_function(const type_interner_t& interner, const type_t& ret, const std::vector<type_t>& args, epure pure){
	QUARK_ASSERT(ret.is_any() == false);

	return make_function3(interner, ret, args, pure, return_dyn_type::none);
}


type_t make_symbol_ref(type_interner_t& interner, const std::string& s){
	const auto node = type_node_t{
		make_empty_type_name(),
		base_type::k_symbol_ref,
		std::vector<type_t>{},
		{},
		epure::pure,
		return_dyn_type::none,
		s
	};
	return intern_node(interner, node);
}

type_t make_named_type(type_interner_t& interner, const type_name_t& type){
	const auto node = type_node_t{
		type,
		base_type::k_symbol_ref,
		std::vector<type_t>{},
		{},
		epure::pure,
		return_dyn_type::none,
		""
	};
	return intern_node(interner, node);
}


//??? Doesnt work. Needs to use type_t-style code that genreates contents of type
json_t type_interner_to_json(const type_interner_t& interner){
	std::vector<json_t> types;
	for(auto i = 0 ; i < interner.interned2.size() ; i++){
		const auto& type = lookup_type_from_index(interner, i);

		const auto& e = interner.interned2[i];
		const auto tag = pack_type_name(e.optional_name);
		const auto desc = type_to_json(interner, type);
		const auto x = json_t::make_object({
			{ "tag", tag },
			{ "desc", desc }
		});
		types.push_back(x);
	}

	//??? support tags!
	return types;
}


//??? Doesnt work. Needs to use type_t-style code that genreates contents of type
//??? support tags!
type_interner_t type_interner_from_json(const json_t& j){
	type_interner_t interner;
	for(const auto& t: j.get_array()){
		const auto tag = t.get_object_element("tag").get_string();
		const auto desc = t.get_object_element("desc");

		const auto tag2 = unpack_type_name(tag);
		const auto type2 = type_from_json(interner, desc);
		(void)type2;
//		types.push_back(type_node_t { tag2, type2 });
	}
	return interner;
}





const type_node_t& lookup_typeinfo_from_type(const type_interner_t& interner, const type_t& type){
	QUARK_ASSERT(interner.check_invariant());
	QUARK_ASSERT(type.check_invariant());

	const auto lookup_index = type.get_lookup_index();
	QUARK_ASSERT(lookup_index >= 0);
	QUARK_ASSERT(lookup_index < interner.interned2.size());

	const auto& result = interner.interned2[lookup_index];
	return result;
}

type_node_t& lookup_typeinfo_from_type(type_interner_t& interner, const type_t& type){
	QUARK_ASSERT(interner.check_invariant());
	QUARK_ASSERT(type.check_invariant());

	const auto lookup_index = type.get_lookup_index();
	QUARK_ASSERT(lookup_index >= 0);
	QUARK_ASSERT(lookup_index < interner.interned2.size());

	auto& result = interner.interned2[lookup_index];
	return result;
}


bool is_atomic_type(type_t type){
	const auto bt = type.get_base_type();
	if(
		bt == base_type::k_undefined
		|| bt == base_type::k_any
		|| bt == base_type::k_void

		|| bt == base_type::k_bool
		|| bt == base_type::k_int
		|| bt == base_type::k_double
		|| bt == base_type::k_string
		|| bt == base_type::k_json

		|| bt == base_type::k_typeid
		|| bt == base_type::k_symbol_ref
//		|| bt == base_type::k_named_type
	){
		return true;
	}
	else{
		return false;
	}
}



}	// floyd

