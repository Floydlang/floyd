//
//  parser_value.cpp
//  FloydSpeak
//
//  Created by Marcus Zetterquist on 26/07/16.
//  Copyright © 2016 Marcus Zetterquist. All rights reserved.
//

#include "parser_value.h"

#include "statements.h"
#include "parser_primitives.h"

using std::string;
using std::make_shared;


namespace floyd {





	//////////////////////////////////////////////////		struct_instance_t


	bool struct_instance_t::check_invariant() const{
//		QUARK_ASSERT(_struct_type._base_type != base_type::k_null && _struct_type.check_invariant());

		QUARK_ASSERT(_def.check_invariant());

		for(const auto m: _member_values){
			QUARK_ASSERT(m.check_invariant());
		}
		return true;
	}

	bool struct_instance_t::operator==(const struct_instance_t& other) const{
		QUARK_ASSERT(check_invariant());
		QUARK_ASSERT(other.check_invariant());

		return _def == other._def && _member_values == other._member_values;
	}

	std::string to_string(const struct_instance_t& v){
		auto s = "struct " + v._def._name + " {";
		for(int i = 0 ; i < v._def._members.size() ; i++){
			const auto& def = v._def._members[i];
			const auto& value = v._member_values[i];

			const auto m = def._type.to_string2() + " " + def._name + "=" + value.to_string()  + ",";
			s = s + m;
		}
		if(s.back() == ','){
			s.pop_back();
		}
		s = s + "}";
		return s;
	}

	json_t to_json(const struct_instance_t& t){
		return json_t::make_object(
			{
				{ "def", to_json(t._def) },
				{ "member_values", values_to_json_array(t._member_values) }
			}
		);
	}


	//////////////////////////////////////////////////		vector_instance_t


		bool vector_instance_t::check_invariant() const{
			for(const auto m: _elements){
				QUARK_ASSERT(m.check_invariant());
				//??? check type of member value is the same as in the type_def.
			}
			return true;
		}

		bool vector_instance_t::operator==(const vector_instance_t& other) const{
			QUARK_ASSERT(check_invariant());
			QUARK_ASSERT(other.check_invariant());

			return _vector_type == other._vector_type && _elements == other._elements;
		}


		std::string to_preview(const vector_instance_t& instance){
			string r;
			for(const auto m: instance._elements){
				r = r + m.plain_value_to_string() + " ";
			}
			return /*"[" + instance._vector_type._parts[0].to_string() + "]*/ "[" + r + "]";
		}




	//////////////////////////////////////		vector_def_t



	vector_def_t vector_def_t::make2(
		const floyd::typeid_t& element_type)
	{
		QUARK_ASSERT(element_type._base_type != floyd::base_type::k_null && element_type.check_invariant());

		vector_def_t result(element_type);

		QUARK_ASSERT(result.check_invariant());
		return result;
	}

	bool vector_def_t::check_invariant() const{
		QUARK_ASSERT(_element_type._base_type != floyd::base_type::k_null && _element_type.check_invariant());
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
		QUARK_TRACE_SS("element_type: " << e._element_type.to_string2());
	}

	json_t vector_def_to_json(const vector_def_t& s){
		return {
		};
	}


/*

QUARK_UNIT_TESTQ("host_function_t", "null"){

	struct dummy_t : public host_function_t {
		public: virtual value_t host_function_call(const std::vector<value_t> args){
			QUARK_ASSERT(true);
			return value_t(13);
		};
	};

	dummy_t t;
	host_function_t* t2 = &t;

	host_function_t* const t3 = t2;
//	t3 = t3;

	QUARK_TEST_VERIFY(t.host_function_call({}) == value_t(13));
	QUARK_TEST_VERIFY(t2->host_function_call({}) == value_t(13));
	QUARK_TEST_VERIFY(t3->host_function_call({}) == value_t(13));
}
*/


	//////////////////////////////////////////////////		function_instance_t



	bool function_instance_t::check_invariant() const {
		return true;
	}

	bool function_instance_t::operator==(const function_instance_t& other) const{
		QUARK_ASSERT(check_invariant());
		QUARK_ASSERT(other.check_invariant());

		return _def == other._def;
	}





	value_t make_vector_instance(const std::shared_ptr<const vector_def_t>& def, const std::vector<value_t>& elements){
		QUARK_ASSERT(def && def->check_invariant());

		const auto vector_type = typeid_t::make_vector(def->_element_type);
		auto instance = make_shared<vector_instance_t>(vector_type, elements);
		return value_t(instance);
	}



int limit(int value, int min, int max){
	if(value < min){
		return min;
	}
	else if(value > max){
		return max;
	}
	else{
		return value;
	}
}

int compare_string(const std::string& left, const std::string& right){
	// ### Better if it doesn't use c_ptr since that is non-pure string handling.
	return limit(std::strcmp(left.c_str(), right.c_str()), -1, 1);
}

QUARK_UNIT_TESTQ("compare_string()", ""){
	QUARK_TEST_VERIFY(compare_string("", "") == 0);
}
QUARK_UNIT_TESTQ("compare_string()", ""){
	QUARK_TEST_VERIFY(compare_string("aaa", "aaa") == 0);
}
QUARK_UNIT_TESTQ("compare_string()", ""){
	QUARK_TEST_VERIFY(compare_string("b", "a") == 1);
}



int value_t::compare_value_true_deep(const struct_instance_t& left, const struct_instance_t& right){
	QUARK_ASSERT(left.check_invariant());
	QUARK_ASSERT(right.check_invariant());

	auto a_it = left._member_values.begin();
	auto b_it = right._member_values.begin();

	while(a_it !=left._member_values.end()){
		int diff = compare_value_true_deep(*a_it, *b_it);
		if(diff != 0){
			return diff;
		}

		a_it++;
		b_it++;
	}
	return 0;
}

int value_t::compare_value_true_deep(const value_t& left, const value_t& right){
	QUARK_ASSERT(left.check_invariant());
	QUARK_ASSERT(right.check_invariant());
	QUARK_ASSERT(left.get_type() == right.get_type());

	const auto type = left._typeid.get_base_type();
	if(type == base_type::k_null){
		return 0;
	}
	else if(type == base_type::k_bool){
		return (left.get_bool() ? 1 : 0) - (right.get_bool() ? 1 : 0);
	}
	else if(type == base_type::k_int){
		return limit(left.get_int() - right.get_int(), -1, 1);
	}
	else if(type == base_type::k_float){
		const auto a = left.get_float();
		const auto b = right.get_float();
		if(a > b){
			return 1;
		}
		else if(a < b){
			return -1;
		}
		else{
			return 0;
		}
	}
	else if(type == base_type::k_string){
		return compare_string(left.get_string(), right.get_string());
	}
	else if(type == base_type::k_struct){
		//	Shortcut: same obejct == we know values are same without having to check them.
		if(left.get_struct() == right.get_struct()){
			return 0;
		}
		else{
			return compare_value_true_deep(*left.get_struct(), *right.get_struct());
		}
	}
	else if(type == base_type::k_vector){
		QUARK_ASSERT(false);
		return 0;
	}
	else if(type == base_type::k_function){
		QUARK_ASSERT(false);
		return 0;
	}
	else{
		QUARK_ASSERT(false);
	}
}





	//////////////////////////////////////////////////		value_t


void trace(const value_t& e){
	QUARK_ASSERT(e.check_invariant());

	QUARK_TRACE("value_t: " + e.value_and_type_to_string());
}

//??? swap(), operator=, copy-constructor.

QUARK_UNIT_TESTQ("value_t()", "null"){
	const auto a = value_t();
	QUARK_TEST_VERIFY(a.is_null());
	QUARK_TEST_VERIFY(!a.is_bool());
	QUARK_TEST_VERIFY(!a.is_int());
	QUARK_TEST_VERIFY(!a.is_float());
	QUARK_TEST_VERIFY(!a.is_string());
	QUARK_TEST_VERIFY(!a.is_struct());
	QUARK_TEST_VERIFY(!a.is_vector());
	QUARK_TEST_VERIFY(!a.is_function());

	QUARK_TEST_VERIFY(a == value_t());
	QUARK_TEST_VERIFY(a != value_t("test"));
	QUARK_TEST_VERIFY(a.plain_value_to_string() == "<null>");
	QUARK_TEST_VERIFY(a.value_and_type_to_string() == "<null>");
}

QUARK_UNIT_TESTQ("value_t()", "bool - true"){
	const auto a = value_t(true);
	QUARK_TEST_VERIFY(!a.is_null());
	QUARK_TEST_VERIFY(a.is_bool());
	QUARK_TEST_VERIFY(!a.is_int());
	QUARK_TEST_VERIFY(!a.is_float());
	QUARK_TEST_VERIFY(!a.is_string());
	QUARK_TEST_VERIFY(!a.is_struct());
	QUARK_TEST_VERIFY(!a.is_vector());
	QUARK_TEST_VERIFY(!a.is_function());

	QUARK_TEST_VERIFY(a == value_t(true));
	QUARK_TEST_VERIFY(a != value_t(false));
	QUARK_TEST_VERIFY(a.plain_value_to_string() == "true");
	QUARK_TEST_VERIFY(a.value_and_type_to_string() == "\"bool\": true");
}

QUARK_UNIT_TESTQ("value_t()", "bool - false"){
	const auto a = value_t(false);
	QUARK_TEST_VERIFY(!a.is_null());
	QUARK_TEST_VERIFY(a.is_bool());
	QUARK_TEST_VERIFY(!a.is_int());
	QUARK_TEST_VERIFY(!a.is_float());
	QUARK_TEST_VERIFY(!a.is_string());
	QUARK_TEST_VERIFY(!a.is_struct());
	QUARK_TEST_VERIFY(!a.is_vector());
	QUARK_TEST_VERIFY(!a.is_function());

	QUARK_TEST_VERIFY(a == value_t(false));
	QUARK_TEST_VERIFY(a != value_t(true));
	QUARK_TEST_VERIFY(a.plain_value_to_string() == "false");
	QUARK_TEST_VERIFY(a.value_and_type_to_string() == "\"bool\": false");
}

QUARK_UNIT_TESTQ("value_t()", "int"){
	const auto a = value_t(13);
	QUARK_TEST_VERIFY(!a.is_null());
	QUARK_TEST_VERIFY(!a.is_bool());
	QUARK_TEST_VERIFY(a.is_int());
	QUARK_TEST_VERIFY(!a.is_float());
	QUARK_TEST_VERIFY(!a.is_string());
	QUARK_TEST_VERIFY(!a.is_struct());
	QUARK_TEST_VERIFY(!a.is_vector());
	QUARK_TEST_VERIFY(!a.is_function());

	QUARK_TEST_VERIFY(a == value_t(13));
	QUARK_TEST_VERIFY(a != value_t(14));
	QUARK_TEST_VERIFY(a.plain_value_to_string() == "13");
	QUARK_TEST_VERIFY(a.value_and_type_to_string() == "\"int\": 13");
}

QUARK_UNIT_TESTQ("value_t()", "float"){
	const auto a = value_t(13.5f);
	QUARK_TEST_VERIFY(!a.is_null());
	QUARK_TEST_VERIFY(!a.is_bool());
	QUARK_TEST_VERIFY(!a.is_int());
	QUARK_TEST_VERIFY(a.is_float());
	QUARK_TEST_VERIFY(!a.is_string());
	QUARK_TEST_VERIFY(!a.is_struct());
	QUARK_TEST_VERIFY(!a.is_vector());
	QUARK_TEST_VERIFY(!a.is_function());

	QUARK_TEST_VERIFY(a == value_t(13.5f));
	QUARK_TEST_VERIFY(a != value_t(14.0f));
	QUARK_TEST_VERIFY(a.plain_value_to_string() == "13.500000");
	QUARK_TEST_VERIFY(a.value_and_type_to_string() == "\"float\": 13.500000");
}

QUARK_UNIT_TESTQ("value_t()", "string"){
	const auto a = value_t("xyz");
	QUARK_TEST_VERIFY(!a.is_null());
	QUARK_TEST_VERIFY(!a.is_bool());
	QUARK_TEST_VERIFY(!a.is_int());
	QUARK_TEST_VERIFY(!a.is_float());
	QUARK_TEST_VERIFY(a.is_string());
	QUARK_TEST_VERIFY(!a.is_struct());
	QUARK_TEST_VERIFY(!a.is_vector());
	QUARK_TEST_VERIFY(!a.is_function());

	QUARK_TEST_VERIFY(a == value_t("xyz"));
	QUARK_TEST_VERIFY(a != value_t("xyza"));
	QUARK_TEST_VERIFY(a.plain_value_to_string() == "xyz");
	QUARK_TEST_VERIFY(a.value_and_type_to_string() == "\"string\": \"xyz\"");
}

#if 0
QUARK_UNIT_TESTQ("value_t()", "struct"){
	const auto struct_scope_ref = lexical_scope_t::make_struct_object(
		std::vector<member_t>{
			{ typeid_t::make_string(), "x" }
		}
	);
	const auto struct_type = typeid_t::make_struct("xxx"/*struct_scope_ref*/);
	const auto instance = make_shared<struct_instance_t>(struct_instance_t(struct_type, std::map<std::string, value_t>{
		{ "x", value_t("skalman")}
	}));
	const auto a = value_t(instance);

	QUARK_TEST_VERIFY(!a.is_null());
	QUARK_TEST_VERIFY(!a.is_bool());
	QUARK_TEST_VERIFY(!a.is_int());
	QUARK_TEST_VERIFY(!a.is_float());
	QUARK_TEST_VERIFY(!a.is_string());
	QUARK_TEST_VERIFY(a.is_struct());
	QUARK_TEST_VERIFY(!a.is_vector());
	QUARK_TEST_VERIFY(!a.is_function());

	QUARK_TEST_VERIFY(a != value_t("xyza"));
}
#endif


QUARK_UNIT_TESTQ("value_t()", "vector"){
	const auto vector_def = make_shared<const vector_def_t>(vector_def_t::make2(typeid_t::make_int()));
	const auto a = make_vector_instance(vector_def, {});
	const auto b = make_vector_instance(vector_def, {});

	QUARK_TEST_VERIFY(!a.is_null());
	QUARK_TEST_VERIFY(!a.is_bool());
	QUARK_TEST_VERIFY(!a.is_int());
	QUARK_TEST_VERIFY(!a.is_float());
	QUARK_TEST_VERIFY(!a.is_string());
	QUARK_TEST_VERIFY(!a.is_struct());
	QUARK_TEST_VERIFY(a.is_vector());
	QUARK_TEST_VERIFY(!a.is_function());

	QUARK_TEST_VERIFY(a == b);
	QUARK_TEST_VERIFY(a != value_t("xyza"));
	quark::ut_compare(a.plain_value_to_string(), "[]");
	quark::ut_compare(a.value_and_type_to_string(), "{ \"base_type\": \"vector\", \"parts\": [\"int\"] }: []");
}


QUARK_UNIT_TESTQ("value_t()", "vector"){
	const auto vector_def = make_shared<const vector_def_t>(vector_def_t::make2(typeid_t::make_int()));
	const auto a = make_vector_instance(vector_def, { 3, 4, 5});
	const auto b = make_vector_instance(vector_def, { 3, 4 });

	QUARK_TEST_VERIFY(a != b);
	QUARK_TEST_VERIFY(a != value_t("xyza"));
	QUARK_TEST_VERIFY(a.get_vector()->_elements[0] == 3);
	QUARK_TEST_VERIFY(a.get_vector()->_elements[1] == 4);
	QUARK_TEST_VERIFY(a.get_vector()->_elements[2] == 5);
}

#if false
value_t make_test_func(){
	const auto function_scope_ref = lexical_scope_t::make_function_object(
		type_identifier_t::make("my_func"),
		std::vector<member_t>{
			{ typeid_t::make_int(), "a" },
			{ typeid_t::make_string(), "b" }
		},
		{},
		{},
		typeid_t::make_bool()
	);

	const auto function_type = typeid_t::make_function_type_def(function_scope_ref);
	const auto a = value_t(function_type);
	return a;
}

QUARK_UNIT_TESTQ("value_t()", "function"){
	const auto a = make_test_func();

	QUARK_TEST_VERIFY(!a.is_null());
	QUARK_TEST_VERIFY(!a.is_bool());
	QUARK_TEST_VERIFY(!a.is_int());
	QUARK_TEST_VERIFY(!a.is_float());
	QUARK_TEST_VERIFY(!a.is_string());
	QUARK_TEST_VERIFY(!a.is_struct());
	QUARK_TEST_VERIFY(!a.is_vector());
	QUARK_TEST_VERIFY(a.is_function());

	QUARK_TEST_VERIFY(a != value_t("xyza"));
}
#endif

#if false

struct_fixture_t::struct_fixture_t() :
	_struct6_def(make_struct6(_ast._global_scope))
{
	auto pixel_def = lexical_scope_t::make_struct(
		type_identifier_t::make("pixel"),
		std::vector<member_t>(
			{
				member_t(type_identifier_t::make_int(), "red", value_t(55)),
				member_t(type_identifier_t::make_int(), "green", value_t(66)),
				member_t(type_identifier_t::make_int(), "blue", value_t(77))
			}
		));

	_ast._global_scope = _ast._global_scope->set_types(define_struct_type(_ast._global_scope->_types_collector, "pixel", pixel_def));

	_struct6_instance0 = make_struct_instance(make_resolved_root(_ast), _struct6_def);
	_struct6_instance1 = make_struct_instance(make_resolved_root(_ast), _struct6_def);
}
#endif



json_t value_to_json(const value_t& v){
	if(v.is_null()){
		return json_t();
	}
	else if(v.is_bool()){
		return json_t(v.get_bool());
	}
	else if(v.is_int()){
		return json_t(static_cast<double>(v.get_int()));
	}
	else if(v.is_float()){
		return json_t(static_cast<double>(v.get_float()));
	}
	else if(v.is_string()){
		return json_t(v.get_string());
	}
	else if(v.is_typeid()){
//		return json_t(v.get_typeid().to_string2());
		return typeid_to_ast_json(v.get_typeid());
	}
	else if(v.is_struct()){
		const auto value = v.get_struct();
		return to_json(*value);
	}
	else if(v.is_vector()){
		const auto value = v.get_vector();
		std::vector<json_t> result;
		for(int i = 0 ; i < value->_elements.size() ; i++){
			const auto element_value = value->_elements[i];
			result.push_back(value_to_json(element_value));
		}
		return result;
	}
	else if(v.is_function()){
		const auto value = v.get_function();
		return json_t::make_object(
			{
				{ "function_type", typeid_to_json(get_function_type(value->_def)) }
			}
		);
	}
	else{
		QUARK_ASSERT(false);
	}
}

#if false
QUARK_UNIT_TESTQ("value_to_json()", "Nested struct to nested JSON objects"){
	struct_fixture_t f;
	const value_t value = f._struct6_instance0;

	const auto result = value_to_json(value);

	QUARK_UT_VERIFY(result.is_object());
	const auto obj = result.get_object();

	QUARK_UT_VERIFY(obj.at("_bool_true").is_true());
	QUARK_UT_VERIFY(obj.at("_bool_false").is_false());
	QUARK_UT_VERIFY(obj.at("_int").get_number() == 111.0);
	QUARK_UT_VERIFY(obj.at("_string").get_string() == "test 123");
	QUARK_UT_VERIFY(obj.at("_pixel").is_object());
	QUARK_UT_VERIFY(obj.at("_pixel").get_object_element("red").get_number() == 55.0);
	QUARK_UT_VERIFY(obj.at("_pixel").get_object_element("green").get_number() == 66.0);
}
#endif

#if 0
QUARK_UNIT_TESTQ("value_to_json()", "Vector"){
	const auto vector_def = make_shared<const vector_def_t>(vector_def_t::make2(type_identifier_t::make("my_vec"), type_identifier_t::make_int()));
	const auto a = make_vector_instance(vector_def, { 10, 11, 12 });
	const auto b = make_vector_instance(vector_def, { 10, 4 });

	const auto result = value_to_json(a);

	QUARK_UT_VERIFY(result.is_array());
	const auto array = result.get_array();

	QUARK_UT_VERIFY(array[0] == 10);
	QUARK_UT_VERIFY(array[1] == 11);
	QUARK_UT_VERIFY(array[2] == 12);
}
#endif

QUARK_UNIT_TESTQ("value_to_json()", ""){
	quark::ut_compare(value_to_json(value_t("hello")), json_t("hello"));
}

QUARK_UNIT_TESTQ("value_to_json()", ""){
	quark::ut_compare(value_to_json(value_t(123)), json_t(123.0));
}

QUARK_UNIT_TESTQ("value_to_json()", ""){
	quark::ut_compare(value_to_json(value_t(true)), json_t(true));
}

QUARK_UNIT_TESTQ("value_to_json()", ""){
	quark::ut_compare(value_to_json(value_t(false)), json_t(false));
}

QUARK_UNIT_TESTQ("value_to_json()", ""){
	quark::ut_compare(value_to_json(value_t()), json_t());
}









	//////////////////////////////////////////////////		function_definition_t


	function_definition_t::function_definition_t(
		const std::vector<member_t>& args,
		const std::vector<std::shared_ptr<statement_t>> statements,
		const typeid_t& return_type
	)
	:
		_args(args),
		_statements(statements),
		_host_function(0),
		_return_type(return_type)
	{
	}

	function_definition_t::function_definition_t(
		const std::vector<member_t>& args,
		const int host_function,
		const typeid_t& return_type
	)
	:
		_args(args),
		_host_function(host_function),
		_return_type(return_type)
	{
	}

	json_t function_definition_t::to_json() const {
		typeid_t function_type = get_function_type(*this);
		return json_t::make_array({
			"func-def",
			typeid_to_json(function_type),
			members_to_json(_args),
			statements_to_json(_statements),
			typeid_to_json(_return_type)
		});
	}

	bool operator==(const function_definition_t& lhs, const function_definition_t& rhs){
		return
			lhs._args == rhs._args
			&& compare_shared_value_vectors(lhs._statements, rhs._statements)
			&& lhs._host_function == rhs._host_function
			&& lhs._return_type == rhs._return_type;
	}

	typeid_t get_function_type(const function_definition_t& f){
		return typeid_t::make_function(f._return_type, get_member_types(f._args));
	}

	std::string to_string(const function_definition_t& v){
		return "???missing impl for to_string(function_definition_t)";
/*
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
*/
	}


}	//	floyd
