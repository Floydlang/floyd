//
//  parser_value.cpp
//  FloydSpeak
//
//  Created by Marcus Zetterquist on 26/07/16.
//  Copyright © 2016 Marcus Zetterquist. All rights reserved.
//

#include "ast_value.h"

#include "statement.h"
#include "text_parser.h"
#include "pass2.h"

using std::string;
using std::make_shared;

namespace floyd {




	//////////////////////////////////////////////////		struct_instance_t


	bool struct_instance_t::check_invariant() const{
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

	std::string struct_instance_to_compact_string(const struct_instance_t& v){
		std::vector<std::string> members;
		for(int i = 0 ; i < v._def._members.size() ; i++){
			const auto& def = v._def._members[i];
			const auto& value = v._member_values[i];

			const auto m = /*typeid_to_compact_string(def._type) + " " +*/ def._name + "=" + to_compact_string_quote_strings(value);
			members.push_back(m);
		}
//		return "struct {" + concat_strings_with_divider(members, ", ") + "}";
		return "{" + concat_strings_with_divider(members, ", ") + "}";
	}



	//////////////////////////////////////		vector_def_t



	bool vector_def_t::check_invariant() const{
		QUARK_ASSERT(!_element_type.is_null() && _element_type.check_invariant());
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


	//////////////////////////////////////////////////		vector_instance_t


	bool vector_instance_t::check_invariant() const{
		for(const auto m: _elements){
			QUARK_ASSERT(m.check_invariant());
			QUARK_ASSERT(m.get_type() == _element_type);
		}
		return true;
	}

	bool vector_instance_t::operator==(const vector_instance_t& other) const{
		QUARK_ASSERT(check_invariant());
		QUARK_ASSERT(other.check_invariant());

		return _element_type == other._element_type && _elements == other._elements;
	}


	std::string vector_instance_to_compact_string(const vector_instance_t& instance){
		std::vector<std::string> elements;
		for(const auto e: instance._elements){
			const auto es = to_compact_string_quote_strings(e);
			elements.push_back(es);
		}
//		return "[" + typeid_to_compact_string(instance._element_type) + "]" + "(" + concat_strings_with_divider(elements, ",") + ")";
		return "[" + concat_strings_with_divider(elements, ", ") + "]";
	}



	//////////////////////////////////////		dict_def_t



	bool dict_def_t::check_invariant() const{
		QUARK_ASSERT(_value_type.check_invariant());
		QUARK_ASSERT(_value_type.is_null() == false);
		return true;
	}

	bool dict_def_t::operator==(const dict_def_t& other) const{
		QUARK_ASSERT(check_invariant());
		QUARK_ASSERT(other.check_invariant());

		if(!(_value_type == other._value_type)){
			return false;
		}
		return true;
	}



	//////////////////////////////////////////////////		dict_instance_t


	bool dict_instance_t::check_invariant() const{
		for(const auto m: _elements){
			QUARK_ASSERT(m.second.check_invariant());
	//		QUARK_ASSERT(m.second.get_type() == _value_type);
		}
		return true;
	}

	bool dict_instance_t::operator==(const dict_instance_t& other) const{
		QUARK_ASSERT(check_invariant());
		QUARK_ASSERT(other.check_invariant());

		return _value_type == other._value_type && _elements == other._elements;
	}


	std::string dict_instance_to_compact_string(const dict_instance_t& instance){
		std::vector<std::string> elements;
		for(const auto e: instance._elements){
			const auto key_str = quote(e.first);
			const auto value_str = to_compact_string_quote_strings(e.second);
			const auto es = key_str + ": " + value_str;
			elements.push_back(es);
		}
//		return "[string:" + typeid_to_compact_string(instance._value_type) + "]" + "{" + concat_strings_with_divider(elements, ",") + "}";
		return "{" + concat_strings_with_divider(elements, ", ") + "}";
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

	ast_json_t function_def_to_ast_json(const function_definition_t& v) {
		typeid_t function_type = get_function_type(v);
		return ast_json_t{json_t::make_array({
			"func-def",
			typeid_to_ast_json(function_type)._value,
			members_to_json(v._args),
			statements_to_json(v._statements)._value,
			typeid_to_ast_json(v._return_type)._value
		})};
	}



	//////////////////////////////////////////////////		function_instance_t



	bool function_instance_t::check_invariant() const {
		return true;
	}

	bool function_instance_t::operator==(const function_instance_t& other) const{
		QUARK_ASSERT(check_invariant());
		QUARK_ASSERT(other.check_invariant());

		return _def == other._def;
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


int value_t::compare_struct_true_deep(const struct_instance_t& left, const struct_instance_t& right){
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

//	Compare vector element by element.
//	### Think more of equality when vectors have different size and shared elements are equal.
int compare_vector_true_deep(const vector_instance_t& left, const vector_instance_t& right){
	QUARK_ASSERT(left.check_invariant());
	QUARK_ASSERT(right.check_invariant());
	QUARK_ASSERT(left._element_type == right._element_type);

	const auto shared_count = std::min(left._elements.size(), right._elements.size());
	for(int i = 0 ; i < shared_count ; i++){
		const auto element_result = value_t::compare_value_true_deep(left._elements[i], right._elements[i]);
		if(element_result != 0){
			return element_result;
		}
	}
	if(left._elements.size() == right._elements.size()){
		return 0;
	}
	else if(left._elements.size() > right._elements.size()){
		return -1;
	}
	else{
		return +1;
	}
}

template <typename Map>
bool map_compare (Map const &lhs, Map const &rhs) {
    // No predicate needed because there is operator== for pairs already.
    return lhs.size() == rhs.size()
        && std::equal(lhs.begin(), lhs.end(),
                      rhs.begin());
}


int compare_dict_true_deep(const dict_instance_t& left, const dict_instance_t& right){
	QUARK_ASSERT(left.check_invariant());
	QUARK_ASSERT(right.check_invariant());
	QUARK_ASSERT(left._value_type == right._value_type);

	
	auto left_it = left._elements.begin();
	auto left_end_it = left._elements.end();

	auto right_it = right._elements.begin();
	auto right_end_it = right._elements.end();

	while(left_it != left_end_it && right_it != right_end_it && *left_it == *right_it){
		left_it++;
		right_it++;
	}

	if(left_it == left_end_it && right_it == right_end_it){
		return 0;
	}
	else if(left_it == left_end_it && right_it != right_end_it){
		return 1;
	}
	else if(left_it != left_end_it && right_it == right_end_it){
		return -1;
	}
	else if(left_it != left_end_it && right_it != right_end_it){
		int key_diff = compare_string(left_it->first, right_it->first);
		if(key_diff != 0){
			return key_diff;
		}
		else {
			return value_t::compare_value_true_deep(left_it->second, right_it->second);
		}
	}
	else{
		QUARK_ASSERT(false)
		throw std::exception();
	}
}


int compare_json_values(const json_t& lhs, const json_t& rhs){
	if(lhs == rhs){
		return 0;
	}
	else{
		// ??? implement compare.
		assert(false);
	}
}

int value_t::compare_value_true_deep(const value_t& left, const value_t& right){
	QUARK_ASSERT(left.check_invariant());
	QUARK_ASSERT(right.check_invariant());
	QUARK_ASSERT(left.get_type() == right.get_type());

	const auto type = left._typeid;
	if(type.is_null()){
		return 0;
	}
	else if(type.is_bool()){
		return (left.get_bool_value() ? 1 : 0) - (right.get_bool_value() ? 1 : 0);
	}
	else if(type.is_int()){
		return limit(left.get_int_value() - right.get_int_value(), -1, 1);
	}
	else if(type.is_float()){
		const auto a = left.get_float_value();
		const auto b = right.get_float_value();
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
	else if(type.is_string()){
		return compare_string(left.get_string_value(), right.get_string_value());
	}
	else if(type.is_json_value()){
		return compare_json_values(left.get_json_value(), right.get_json_value());
	}
	else if(type.is_typeid()){
	//???
		if(left.get_typeid_value() == right.get_typeid_value()){
			return 0;
		}
		else{
			return -1;//??? Hack -- should return +1 depending on values.
		}
	}
	else if(type.is_struct()){
		//	Make sure the EXACT struct types are the same -- not only that they are both structs
		if(left.get_type() != right.get_type()){
			throw std::runtime_error("Cannot compare structs of different type.");
		}
		else{
			//	Shortcut: same obejct == we know values are same without having to check them.
			if(left.get_struct_value() == right.get_struct_value()){
				return 0;
			}
			else{
				return compare_struct_true_deep(*left.get_struct_value(), *right.get_struct_value());
			}
		}
	}
	else if(type.is_vector()){
		//	Make sure the EXACT types are the same -- not only that they are both vectors.
		if(left.get_type() != right.get_type()){
			throw std::runtime_error("Cannot compare structs of different type.");
		}
		else{
			const auto left_vec = left.get_vector_value();
			const auto right_vec = right.get_vector_value();
			return compare_vector_true_deep(*left_vec, *right_vec);
		}
	}
	else if(type.is_dict()){
		//	Make sure the EXACT types are the same -- not only that they are both dicts.
		if(left.get_type() != right.get_type()){
			throw std::runtime_error("Cannot compare dicts of different type.");
		}
		else{
			const auto left2 = left.get_dict_value();
			const auto right2 = right.get_dict_value();
			return compare_dict_true_deep(*left2, *right2);
		}
	}
	else if(type.is_function()){
		QUARK_ASSERT(false);
		return 0;
	}
	else if(type.is_unresolved_type_identifier()){
		QUARK_ASSERT(false);
		return 0;
	}
	else{
		QUARK_ASSERT(false);
		throw std::exception();
	}
}






bool value_t::check_invariant() const{
	QUARK_ASSERT(_typeid.check_invariant());

	const auto base_type = _typeid.get_base_type();
	if(base_type == base_type::k_null){
		QUARK_ASSERT(_bool == false);
		QUARK_ASSERT(_int == 0);
		QUARK_ASSERT(_float == 0.0f);
		QUARK_ASSERT(_string == "");
		QUARK_ASSERT(_json_value == nullptr);
		QUARK_ASSERT(_typeid_value == typeid_t::make_null());
		QUARK_ASSERT(_struct == nullptr);
		QUARK_ASSERT(_vector == nullptr);
		QUARK_ASSERT(_dict == nullptr);
		QUARK_ASSERT(_function == nullptr);
	}
	else if(base_type == base_type::k_bool){
//				QUARK_ASSERT(_bool == false);
		QUARK_ASSERT(_int == 0);
		QUARK_ASSERT(_float == 0.0f);
		QUARK_ASSERT(_string == "");
		QUARK_ASSERT(_json_value == nullptr);
		QUARK_ASSERT(_typeid_value == typeid_t::make_null());
		QUARK_ASSERT(_struct == nullptr);
		QUARK_ASSERT(_vector == nullptr);
		QUARK_ASSERT(_dict == nullptr);
		QUARK_ASSERT(_function == nullptr);
	}
	else if(base_type == base_type::k_int){
		QUARK_ASSERT(_bool == false);
//				QUARK_ASSERT(_int == 0);
		QUARK_ASSERT(_float == 0.0f);
		QUARK_ASSERT(_string == "");
		QUARK_ASSERT(_json_value == nullptr);
		QUARK_ASSERT(_typeid_value == typeid_t::make_null());
		QUARK_ASSERT(_struct == nullptr);
		QUARK_ASSERT(_vector == nullptr);
		QUARK_ASSERT(_dict == nullptr);
		QUARK_ASSERT(_function == nullptr);
	}
	else if(base_type == base_type::k_float){
		QUARK_ASSERT(_bool == false);
		QUARK_ASSERT(_int == 0);
//				QUARK_ASSERT(_float == 0.0f);
		QUARK_ASSERT(_string == "");
		QUARK_ASSERT(_json_value == nullptr);
		QUARK_ASSERT(_typeid_value == typeid_t::make_null());
		QUARK_ASSERT(_struct == nullptr);
		QUARK_ASSERT(_vector == nullptr);
		QUARK_ASSERT(_dict == nullptr);
		QUARK_ASSERT(_function == nullptr);
	}
	else if(base_type == base_type::k_string){
		QUARK_ASSERT(_bool == false);
		QUARK_ASSERT(_int == 0);
		QUARK_ASSERT(_float == 0.0f);
//				QUARK_ASSERT(_string == "");
		QUARK_ASSERT(_json_value == nullptr);
		QUARK_ASSERT(_typeid_value == typeid_t::make_null());
		QUARK_ASSERT(_struct == nullptr);
		QUARK_ASSERT(_vector == nullptr);
		QUARK_ASSERT(_dict == nullptr);
		QUARK_ASSERT(_function == nullptr);
	}
	else if(base_type == base_type::k_json_value){
		QUARK_ASSERT(_bool == false);
		QUARK_ASSERT(_int == 0);
		QUARK_ASSERT(_float == 0.0f);
		QUARK_ASSERT(_string == "");
		QUARK_ASSERT(_json_value != nullptr);
		QUARK_ASSERT(_typeid_value == typeid_t::make_null());
		QUARK_ASSERT(_struct == nullptr);
		QUARK_ASSERT(_vector == nullptr);
		QUARK_ASSERT(_dict == nullptr);
		QUARK_ASSERT(_function == nullptr);

		QUARK_ASSERT(_json_value->check_invariant());
	}

	else if(base_type == base_type::k_typeid){
		QUARK_ASSERT(_bool == false);
		QUARK_ASSERT(_int == 0);
		QUARK_ASSERT(_float == 0.0f);
		QUARK_ASSERT(_string == "");
		QUARK_ASSERT(_json_value == nullptr);
//		QUARK_ASSERT(_typeid_value != typeid_t::make_null());
		QUARK_ASSERT(_struct == nullptr);
		QUARK_ASSERT(_vector == nullptr);
		QUARK_ASSERT(_dict == nullptr);
		QUARK_ASSERT(_function == nullptr);
	}
	else if(base_type == base_type::k_struct){
		QUARK_ASSERT(_bool == false);
		QUARK_ASSERT(_int == 0);
		QUARK_ASSERT(_float == 0.0f);
		QUARK_ASSERT(_string == "");
		QUARK_ASSERT(_json_value == nullptr);
		QUARK_ASSERT(_typeid_value == typeid_t::make_null());
		QUARK_ASSERT(_struct != nullptr);
		QUARK_ASSERT(_vector == nullptr);
		QUARK_ASSERT(_dict == nullptr);
		QUARK_ASSERT(_function == nullptr);

		QUARK_ASSERT(_struct && _struct->check_invariant());
	}
	else if(base_type == base_type::k_vector){
		QUARK_ASSERT(_bool == false);
		QUARK_ASSERT(_int == 0);
		QUARK_ASSERT(_float == 0.0f);
		QUARK_ASSERT(_string == "");
		QUARK_ASSERT(_json_value == nullptr);
		QUARK_ASSERT(_typeid_value == typeid_t::make_null());
		QUARK_ASSERT(_struct == nullptr);
		QUARK_ASSERT(_vector != nullptr);
		QUARK_ASSERT(_dict == nullptr);
		QUARK_ASSERT(_function == nullptr);

		QUARK_ASSERT(_vector && _vector->check_invariant());
		QUARK_ASSERT(_typeid.get_vector_element_type() == _vector->_element_type);
	}
	else if(base_type == base_type::k_dict){
		QUARK_ASSERT(_bool == false);
		QUARK_ASSERT(_int == 0);
		QUARK_ASSERT(_float == 0.0f);
		QUARK_ASSERT(_string == "");
		QUARK_ASSERT(_json_value == nullptr);
		QUARK_ASSERT(_typeid_value == typeid_t::make_null());
		QUARK_ASSERT(_struct == nullptr);
		QUARK_ASSERT(_vector == nullptr);
		QUARK_ASSERT(_dict != nullptr);
		QUARK_ASSERT(_function == nullptr);

		QUARK_ASSERT(_dict && _dict->check_invariant());
		QUARK_ASSERT(_typeid.get_dict_value_type() == _dict->_value_type);
	}
	else if(base_type == base_type::k_function){
		QUARK_ASSERT(_bool == false);
		QUARK_ASSERT(_int == 0);
		QUARK_ASSERT(_float == 0.0f);
		QUARK_ASSERT(_string == "");
		QUARK_ASSERT(_json_value == nullptr);
		QUARK_ASSERT(_typeid_value == typeid_t::make_null());
		QUARK_ASSERT(_struct == nullptr);
		QUARK_ASSERT(_vector == nullptr);
		QUARK_ASSERT(_dict == nullptr);
		QUARK_ASSERT(_function != nullptr);

		QUARK_ASSERT(_function && _function->check_invariant());
	}
	else if(base_type == base_type::k_unresolved_type_identifier){
		//	 Cannot have a value of unknown type.
		QUARK_ASSERT(false);
	}
	else {
		QUARK_ASSERT(false);
	}
	return true;
}







std::string to_compact_string2(const value_t& value) {
	QUARK_ASSERT(value.check_invariant());

	const auto base_type = value.get_type().get_base_type();
	if(base_type == base_type::k_null){
		return keyword_t::k_null;
	}
	else if(base_type == base_type::k_bool){
		return value.get_bool_value() ? keyword_t::k_true : keyword_t::k_false;
	}
	else if(base_type == base_type::k_int){
		char temp[200 + 1];//### Use C++ function instead.
		sprintf(temp, "%d", value.get_int_value());
		return std::string(temp);
	}
	else if(base_type == base_type::k_float){
		return float_to_string(value.get_float_value());
	}
	else if(base_type == base_type::k_string){
		return value.get_string_value();
	}
	else if(base_type == base_type::k_json_value){
		return json_to_compact_string(value.get_json_value());
	}
	else if(base_type == base_type::k_typeid){
		return floyd::typeid_to_compact_string(value.get_typeid_value());
	}
	else if(base_type == base_type::k_struct){
		return struct_instance_to_compact_string(*value.get_struct_value());
	}
	else if(base_type == base_type::k_vector){
		return vector_instance_to_compact_string(*value.get_vector_value());
	}
	else if(base_type == base_type::k_dict){
		return dict_instance_to_compact_string(*value.get_dict_value());
	}
	else if(base_type == base_type::k_function){
		return floyd::typeid_to_compact_string(value.get_type());
	}
	else if(base_type == base_type::k_unresolved_type_identifier){
		QUARK_ASSERT(false);
		return "";
	}

	else{
		return "??";
	}
}

std::string to_compact_string_quote_strings(const value_t& value) {
	const auto s = to_compact_string2(value);
	if(value.is_string()){
		return "\"" + s + "\"";
	}
	else{
		return s;
	}
}

std::string value_and_type_to_string(const value_t& value) {
	QUARK_ASSERT(value.check_invariant());

	std::string type_string = floyd::typeid_to_compact_string(value.get_type());
	if(value.is_null()){
		return type_string;
	}
	return type_string + ": " + to_compact_string_quote_strings(value);
}




//??? swap(), operator=, copy-constructor.

QUARK_UNIT_TESTQ("value_t::make_null()", "null"){
	const auto a = value_t::make_null();
	QUARK_TEST_VERIFY(a.is_null());
	QUARK_TEST_VERIFY(!a.is_bool());
	QUARK_TEST_VERIFY(!a.is_int());
	QUARK_TEST_VERIFY(!a.is_float());
	QUARK_TEST_VERIFY(!a.is_string());
	QUARK_TEST_VERIFY(!a.is_struct());
	QUARK_TEST_VERIFY(!a.is_vector());
	QUARK_TEST_VERIFY(!a.is_dict());
	QUARK_TEST_VERIFY(!a.is_function());

	QUARK_TEST_VERIFY(a == value_t::make_null());
	QUARK_TEST_VERIFY(a != value_t::make_string("test"));
	QUARK_TEST_VERIFY(to_compact_string2(a) == "null");
	QUARK_TEST_VERIFY(value_and_type_to_string(a) == "null");
}

QUARK_UNIT_TESTQ("value_t()", "bool - true"){
	const auto a = value_t::make_bool(true);
	QUARK_TEST_VERIFY(!a.is_null());
	QUARK_TEST_VERIFY(a.is_bool());
	QUARK_TEST_VERIFY(!a.is_int());
	QUARK_TEST_VERIFY(!a.is_float());
	QUARK_TEST_VERIFY(!a.is_string());
	QUARK_TEST_VERIFY(!a.is_struct());
	QUARK_TEST_VERIFY(!a.is_vector());
	QUARK_TEST_VERIFY(!a.is_dict());
	QUARK_TEST_VERIFY(!a.is_function());

	QUARK_TEST_VERIFY(a == value_t::make_bool(true));
	QUARK_TEST_VERIFY(a != value_t::make_bool(false));
	QUARK_TEST_VERIFY(to_compact_string2(a) == keyword_t::k_true);
	QUARK_TEST_VERIFY(value_and_type_to_string(a) == "bool: true");
}

QUARK_UNIT_TESTQ("value_t()", "bool - false"){
	const auto a = value_t::make_bool(false);
	QUARK_TEST_VERIFY(!a.is_null());
	QUARK_TEST_VERIFY(a.is_bool());
	QUARK_TEST_VERIFY(!a.is_int());
	QUARK_TEST_VERIFY(!a.is_float());
	QUARK_TEST_VERIFY(!a.is_string());
	QUARK_TEST_VERIFY(!a.is_struct());
	QUARK_TEST_VERIFY(!a.is_vector());
	QUARK_TEST_VERIFY(!a.is_dict());
	QUARK_TEST_VERIFY(!a.is_function());

	QUARK_TEST_VERIFY(a == value_t::make_bool(false));
	QUARK_TEST_VERIFY(a != value_t::make_bool(true));
	QUARK_TEST_VERIFY(to_compact_string2(a) == keyword_t::k_false);
	QUARK_TEST_VERIFY(value_and_type_to_string(a) == "bool: false");
}

QUARK_UNIT_TESTQ("value_t()", "int"){
	const auto a = value_t::make_int(13);
	QUARK_TEST_VERIFY(!a.is_null());
	QUARK_TEST_VERIFY(!a.is_bool());
	QUARK_TEST_VERIFY(a.is_int());
	QUARK_TEST_VERIFY(!a.is_float());
	QUARK_TEST_VERIFY(!a.is_string());
	QUARK_TEST_VERIFY(!a.is_struct());
	QUARK_TEST_VERIFY(!a.is_vector());
	QUARK_TEST_VERIFY(!a.is_dict());
	QUARK_TEST_VERIFY(!a.is_function());

	QUARK_TEST_VERIFY(a == value_t::make_int(13));
	QUARK_TEST_VERIFY(a != value_t::make_int(14));
	QUARK_TEST_VERIFY(to_compact_string2(a) == "13");
	QUARK_TEST_VERIFY(value_and_type_to_string(a) == "int: 13");
}

QUARK_UNIT_TESTQ("value_t()", "float"){
	const auto a = value_t::make_float(13.5f);
	QUARK_TEST_VERIFY(!a.is_null());
	QUARK_TEST_VERIFY(!a.is_bool());
	QUARK_TEST_VERIFY(!a.is_int());
	QUARK_TEST_VERIFY(a.is_float());
	QUARK_TEST_VERIFY(!a.is_string());
	QUARK_TEST_VERIFY(!a.is_struct());
	QUARK_TEST_VERIFY(!a.is_vector());
	QUARK_TEST_VERIFY(!a.is_dict());
	QUARK_TEST_VERIFY(!a.is_function());

	QUARK_TEST_VERIFY(a == value_t::make_float(13.5f));
	QUARK_TEST_VERIFY(a != value_t::make_float(14.0f));
	QUARK_TEST_VERIFY(to_compact_string2(a) == "13.5");
	QUARK_TEST_VERIFY(value_and_type_to_string(a) == "float: 13.5");
}

QUARK_UNIT_TESTQ("value_t()", "string"){
	const auto a = value_t::make_string("xyz");
	QUARK_TEST_VERIFY(!a.is_null());
	QUARK_TEST_VERIFY(!a.is_bool());
	QUARK_TEST_VERIFY(!a.is_int());
	QUARK_TEST_VERIFY(!a.is_float());
	QUARK_TEST_VERIFY(a.is_string());
	QUARK_TEST_VERIFY(!a.is_struct());
	QUARK_TEST_VERIFY(!a.is_vector());
	QUARK_TEST_VERIFY(!a.is_dict());
	QUARK_TEST_VERIFY(!a.is_function());

	QUARK_TEST_VERIFY(a == value_t::make_string("xyz"));
	QUARK_TEST_VERIFY(a != value_t::make_string("xyza"));
	QUARK_TEST_VERIFY(to_compact_string2(a) == "xyz");
	QUARK_TEST_VERIFY(value_and_type_to_string(a) == "string: \"xyz\"");
}


ast_json_t value_and_type_to_ast_json(const value_t& v){
	return ast_json_t{json_t::make_array({
		typeid_to_ast_json(v.get_type())._value,
		value_to_ast_json(v)._value
	})};
}

#if DEBUG
std::string make_value_debug_str(const value_t& value){
//	return value_and_type_to_string(v);

	std::string type_string = floyd::typeid_to_compact_string(value.get_type());
	return type_string;
}
#endif


ast_json_t value_to_ast_json(const value_t& v){
	if(v.is_null()){
		return ast_json_t{json_t()};
	}
	else if(v.is_bool()){
		return ast_json_t{json_t(v.get_bool_value())};
	}
	else if(v.is_int()){
		return ast_json_t{json_t(static_cast<double>(v.get_int_value()))};
	}
	else if(v.is_float()){
		return ast_json_t{json_t(static_cast<double>(v.get_float_value()))};
	}
	else if(v.is_string()){
		return ast_json_t{json_t(v.get_string_value())};
	}
	else if(v.is_json_value()){
		return ast_json_t{v.get_json_value()};
	}
	else if(v.is_typeid()){
		return typeid_to_ast_json(v.get_typeid_value());
	}
	else if(v.is_struct()){
		const auto struct_value = v.get_struct_value();
		std::map<string, json_t> obj2;
		for(int i = 0 ; i < struct_value->_def._members.size() ; i++){
			const auto member = struct_value->_def._members[i];
			const auto key = member._name;
			const auto type = member._type;
			const auto value = struct_value->_member_values[i];
			const auto value2 = value_to_ast_json(value);
			obj2[key] = value2._value;
		}
		return ast_json_t{json_t::make_object(obj2)};
/*

 }
		return ast_json_t{json_t::make_object(
			{
				{ "struct-def", struct_definition_to_ast_json(i->_def)._value },
				{ "member_values", values_to_json_array(i->_member_values) }
			}
		)};
*/
//		return ast_json_t{ values_to_json_array(value->_member_values) 	};
	}
	else if(v.is_vector()){
		const auto value = v.get_vector_value();
		return ast_json_t{ values_to_json_array(value->_elements) 	};
/*
		std::vector<json_t> result;
		for(int i = 0 ; i < value->_elements.size() ; i++){
			const auto element_value = value->_elements[i];
			result.push_back(value_to_ast_json(element_value)._value);
		}
		return ast_json_t{result};
*/

	}
	else if(v.is_dict()){
		const auto value = v.get_dict_value();
		std::map<string, json_t> result;
		for(const auto e: value->_elements){
			result[e.first] = value_to_ast_json(e.second)._value;
		}
		return ast_json_t{result};
	}
	else if(v.is_function()){
		const auto value = v.get_function_value();
		return ast_json_t{json_t::make_object(
			{
				{ "function_type", typeid_to_ast_json(get_function_type(value->_def))._value }
			}
		)};
	}
	else{
		QUARK_ASSERT(false);
		throw std::exception();
	}
}

QUARK_UNIT_TESTQ("value_to_ast_json()", ""){
	quark::ut_compare(value_to_ast_json(value_t::make_string("hello"))._value, json_t("hello"));
}

QUARK_UNIT_TESTQ("value_to_ast_json()", ""){
	quark::ut_compare(value_to_ast_json(value_t::make_int(123))._value, json_t(123.0));
}

QUARK_UNIT_TESTQ("value_to_ast_json()", ""){
	quark::ut_compare(value_to_ast_json(value_t::make_bool(true))._value, json_t(true));
}

QUARK_UNIT_TESTQ("value_to_ast_json()", ""){
	quark::ut_compare(value_to_ast_json(value_t::make_bool(false))._value, json_t(false));
}

QUARK_UNIT_TESTQ("value_to_ast_json()", ""){
	quark::ut_compare(value_to_ast_json(value_t::make_null())._value, json_t());
}






value_t value_t::make_null(){
	return value_t();
}


value_t value_t::make_bool(bool value){
	return value_t(value);
}

value_t value_t::make_int(int value){
	return value_t(value);
}

value_t value_t::make_float(float value){
	return value_t(value);
}

value_t value_t::make_string(const std::string& value){
	return value_t(value);
}

value_t value_t::make_json_value(const json_t& v){
	auto f = std::make_shared<json_t>(v);
	return value_t(f);
}

value_t value_t::make_typeid_value(const typeid_t& type_id){
	return value_t(type_id);
}

value_t value_t::make_struct_value(const typeid_t& struct_type, const std::vector<value_t>& values){
	QUARK_ASSERT(struct_type.check_invariant());
	QUARK_ASSERT(struct_type.get_base_type() != base_type::k_unresolved_type_identifier);

	auto instance = std::shared_ptr<struct_instance_t>(new struct_instance_t{struct_type.get_struct(), values});
	return value_t(struct_type, instance);
}

value_t value_t::make_vector_value(const typeid_t& element_type, const std::vector<value_t>& elements){
	auto f = std::shared_ptr<vector_instance_t>(new vector_instance_t{element_type, elements});
	return value_t(f);
}

value_t value_t::make_dict_value(const typeid_t& value_type, const std::map<std::string, value_t>& elements){
	auto f = std::shared_ptr<dict_instance_t>(new dict_instance_t{value_type, elements});
	return value_t(f);
}

value_t value_t::make_function_value(const function_definition_t& def){
	auto f = std::shared_ptr<function_instance_t>(new function_instance_t{def});
	return value_t(f);
}




}	//	floyd
