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
		QUARK_ASSERT(_def && _def->check_invariant());

		for(const auto& m: _member_values){
			QUARK_ASSERT(m.check_invariant());
		}
		return true;
	}

	bool struct_instance_t::operator==(const struct_instance_t& other) const{
		QUARK_ASSERT(check_invariant());
		QUARK_ASSERT(other.check_invariant());

		return *_def == *other._def && _member_values == other._member_values;
	}

	std::string struct_instance_to_compact_string(const struct_instance_t& v){
		std::vector<std::string> members;
		for(int i = 0 ; i < v._def->_members.size() ; i++){
			const auto& def = v._def->_members[i];
			const auto& value = v._member_values[i];

			const auto m = /*typeid_to_compact_string(def._type) + " " +*/ def._name + "=" + to_compact_string_quote_strings(value);
			members.push_back(m);
		}
//		return "struct {" + concat_strings_with_divider(members, ", ") + "}";
		return "{" + concat_strings_with_divider(members, ", ") + "}";
	}



	std::string vector_instance_to_compact_string(const std::vector<value_t>& elements0){
		std::vector<std::string> elements;
		for(const auto& e: elements0){
			const auto es = to_compact_string_quote_strings(e);
			elements.push_back(es);
		}
//		return "[" + typeid_to_compact_string(instance._element_type) + "]" + "(" + concat_strings_with_divider(elements, ",") + ")";
		return "[" + concat_strings_with_divider(elements, ", ") + "]";
	}



	std::string dict_instance_to_compact_string(const std::map<std::string, value_t>& entries){
		std::vector<std::string> elements;
		for(const auto& e: entries){
			const auto& key_str = quote(e.first);
			const auto& value_str = to_compact_string_quote_strings(e.second);
			const auto& es = key_str + ": " + value_str;
			elements.push_back(es);
		}
//		return "[string:" + typeid_to_compact_string(instance._value_type) + "]" + "{" + concat_strings_with_divider(elements, ",") + "}";
		return "{" + concat_strings_with_divider(elements, ", ") + "}";
	}



	//////////////////////////////////////////////////		function_definition_t



	bool operator==(const function_definition_t& lhs, const function_definition_t& rhs){
		return true
			&& lhs._function_type == rhs._function_type
			&& lhs._args == rhs._args
			&& compare_shared_values(lhs._body, rhs._body)
			&& lhs._host_function_id == rhs._host_function_id
			;
	}

	const typeid_t& get_function_type(const function_definition_t& f){
		return f._function_type;
	}

	ast_json_t function_def_to_ast_json(const function_definition_t& v) {
		typeid_t function_type = get_function_type(v);
		return ast_json_t{json_t::make_array({
			"func-def",
			typeid_to_ast_json(function_type)._value,
			members_to_json(v._args),
			body_to_json(*v._body)._value,
			typeid_to_ast_json(v._function_type.get_function_return())._value
		})};
	}











		value_ext_t::value_ext_t(const typeid_t& s) :
			_type(typeid_t::make_typeid()),
			_typeid_value(s)
		{
			QUARK_ASSERT(check_invariant());
		}


		value_ext_t::value_ext_t(const typeid_t& type, std::shared_ptr<struct_instance_t>& s) :
			_type(type),
			_struct(s)
		{
			QUARK_ASSERT(check_invariant());
		}

		value_ext_t::value_ext_t(const typeid_t& type, const std::vector<value_t>& s) :
			_type(type),
			_vector_elements(s)
		{
			QUARK_ASSERT(check_invariant());
		}

		value_ext_t::value_ext_t(const typeid_t& type, const std::map<std::string, value_t>& s) :
			_type(type),
			_dict_entries(s)
		{
			QUARK_ASSERT(check_invariant());
		}

		value_ext_t::value_ext_t(const typeid_t& type, int function_id) :
			_type(type),
			_function_id(function_id)
		{
			QUARK_ASSERT(check_invariant());
		}




		bool value_ext_t::operator==(const value_ext_t& other) const{
			QUARK_ASSERT(check_invariant());
			QUARK_ASSERT(other.check_invariant());

			const auto base_type = _type.get_base_type();
			if(base_type == base_type::k_string){
				return _string == other._string;
			}
			else if(base_type == base_type::k_json_value){
				return *_json_value == *other._json_value;
			}
			else if(base_type == base_type::k_typeid){
				return _typeid_value == other._typeid_value;
			}
			else if(base_type == base_type::k_struct){
				return *_struct == *other._struct;
			}
			else if(base_type == base_type::k_vector){
				return _vector_elements == other._vector_elements;
			}
			else if(base_type == base_type::k_dict){
				return _dict_entries == other._dict_entries;
			}
			else if(base_type == base_type::k_function){
				return _function_id == other._function_id;
			}
			else {
				QUARK_ASSERT(false);
				throw std::exception();
			}
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


int compare_struct_true_deep(const struct_instance_t& left, const struct_instance_t& right){
	QUARK_ASSERT(left.check_invariant());
	QUARK_ASSERT(right.check_invariant());

	std::vector<value_t>::const_iterator a_it = left._member_values.begin();
	std::vector<value_t>::const_iterator b_it = right._member_values.begin();

	while(a_it !=left._member_values.end()){
		int diff = value_t::compare_value_true_deep(*a_it, *b_it);
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
int compare_vector_true_deep(const std::vector<value_t>& left, const std::vector<value_t>& right){
//	QUARK_ASSERT(left.check_invariant());
//	QUARK_ASSERT(right.check_invariant());
//	QUARK_ASSERT(left._element_type == right._element_type);

	const auto& shared_count = std::min(left.size(), right.size());
	for(int i = 0 ; i < shared_count ; i++){
		const auto element_result = value_t::compare_value_true_deep(left[i], right[i]);
		if(element_result != 0){
			return element_result;
		}
	}
	if(left.size() == right.size()){
		return 0;
	}
	else if(left.size() > right.size()){
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


int compare_dict_true_deep(const std::map<std::string, value_t>& left, const std::map<std::string, value_t>& right){
	auto left_it = left.begin();
	auto left_end_it = left.end();

	auto right_it = right.begin();
	auto right_end_it = right.end();

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

	if(left.is_null()){
		return 0;
	}
	else if(left.is_bool()){
		return (left.get_bool_value() ? 1 : 0) - (right.get_bool_value() ? 1 : 0);
	}
	else if(left.is_int()){
		return limit(left.get_int_value() - right.get_int_value(), -1, 1);
	}
	else if(left.is_float()){
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
	else if(left.is_string()){
		return compare_string(left.get_string_value(), right.get_string_value());
	}
	else if(left.is_json_value()){
		return compare_json_values(left.get_json_value(), right.get_json_value());
	}
	else if(left.is_typeid()){
	//???
		if(left.get_typeid_value() == right.get_typeid_value()){
			return 0;
		}
		else{
			return -1;//??? Hack -- should return +1 depending on values.
		}
	}
	else if(left.is_struct()){
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
	else if(left.is_vector()){
		//	Make sure the EXACT types are the same -- not only that they are both vectors.
		if(left.get_type() != right.get_type()){
			throw std::runtime_error("Cannot compare structs of different type.");
		}
		else{
			const auto& left_vec = left.get_vector_value();
			const auto& right_vec = right.get_vector_value();
			return compare_vector_true_deep(left_vec, right_vec);
		}
	}
	else if(left.is_dict()){
		//	Make sure the EXACT types are the same -- not only that they are both dicts.
		if(left.get_type() != right.get_type()){
			throw std::runtime_error("Cannot compare dicts of different type.");
		}
		else{
			const auto& left2 = left.get_dict_value();
			const auto& right2 = right.get_dict_value();
			return compare_dict_true_deep(left2, right2);
		}
	}
	else if(left.is_function()){
		QUARK_ASSERT(false);
		return 0;
	}
	else{
		QUARK_ASSERT(false);
		throw std::exception();
	}
}






bool value_t::check_invariant() const{
	const auto type_int = _type_int;
	if(type_int == type_int::k_null){
		QUARK_ASSERT(!_ext);
	}
	else if(type_int == type_int::k_bool){
		QUARK_ASSERT(!_ext);
	}
	else if(type_int == type_int::k_int){
		QUARK_ASSERT(!_ext);
	}
	else if(type_int == type_int::k_float){
		QUARK_ASSERT(!_ext);
	}
	else if(type_int == type_int::k_ext){
		QUARK_ASSERT(_ext && _ext->check_invariant());
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
		return vector_instance_to_compact_string(value.get_vector_value());
	}
	else if(base_type == base_type::k_dict){
		return dict_instance_to_compact_string(value.get_dict_value());
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




		std::string value_t::get_string_value() const{
			QUARK_ASSERT(check_invariant());
			if(!is_string()){
				throw std::runtime_error("Type mismatch!");
			}

			return _ext->_string;
		}


		json_t value_t::get_json_value() const{
			QUARK_ASSERT(check_invariant());
			if(!is_json_value()){
				throw std::runtime_error("Type mismatch!");
			}

			return *_ext->_json_value;
		}


		typeid_t value_t::get_typeid_value() const{
			QUARK_ASSERT(check_invariant());
			if(!is_typeid()){
				throw std::runtime_error("Type mismatch!");
			}

			return _ext->_typeid_value;
		}



		std::shared_ptr<struct_instance_t> value_t::get_struct_value() const{
			QUARK_ASSERT(check_invariant());
			if(!is_struct()){
				throw std::runtime_error("Type mismatch!");
			}

			return _ext->_struct;
		}


		const std::vector<value_t>& value_t::get_vector_value() const{
			QUARK_ASSERT(check_invariant());
			if(!is_vector()){
				throw std::runtime_error("Type mismatch!");
			}

			return _ext->_vector_elements;
		}


		const std::map<std::string, value_t>& value_t::get_dict_value() const{
			QUARK_ASSERT(check_invariant());
			if(!is_dict()){
				throw std::runtime_error("Type mismatch!");
			}

			return _ext->_dict_entries;
		}

		int value_t::get_function_value() const{
			QUARK_ASSERT(check_invariant());
			if(!is_function()){
				throw std::runtime_error("Type mismatch!");
			}

			return _ext->_function_id;
		}
















		value_t::value_t(const char s[]) :
			_type_int(type_int::k_ext),
			_ext(std::make_shared<value_ext_t>(value_ext_t{std::string(s)}))
		{
			QUARK_ASSERT(s != nullptr);

#if DEBUG
			DEBUG_STR = make_value_debug_str(*this);
#endif

			QUARK_ASSERT(check_invariant());
		}

		value_t::value_t(const std::string& s) :
			_type_int(type_int::k_ext),
			_ext(std::make_shared<value_ext_t>(value_ext_t{s}))
		{
#if DEBUG
			DEBUG_STR = make_value_debug_str(*this);
#endif

			QUARK_ASSERT(check_invariant());
		}

		value_t::value_t(const std::shared_ptr<json_t>& s) :
			_type_int(type_int::k_ext),
			_ext(std::make_shared<value_ext_t>(value_ext_t(s)))
		{
			QUARK_ASSERT(s != nullptr && s->check_invariant());
#if DEBUG
			DEBUG_STR = make_value_debug_str(*this);
#endif

			QUARK_ASSERT(check_invariant());
		}

		value_t::value_t(const typeid_t& type) :
			_type_int(type_int::k_ext),
			_ext(std::make_shared<value_ext_t>(value_ext_t(type)))
		{
			QUARK_ASSERT(type.check_invariant());

#if DEBUG
			DEBUG_STR = make_value_debug_str(*this);
#endif

			QUARK_ASSERT(check_invariant());
		}

		value_t::value_t(const typeid_t& struct_type, std::shared_ptr<struct_instance_t>& instance) :
			_type_int(type_int::k_ext),
			_ext(std::make_shared<value_ext_t>(value_ext_t(struct_type, instance)))
		{
			QUARK_ASSERT(struct_type.get_base_type() == base_type::k_struct);
			QUARK_ASSERT(instance && instance->check_invariant());

#if DEBUG
			DEBUG_STR = make_value_debug_str(*this);
#endif

			QUARK_ASSERT(check_invariant());
		}

		value_t::value_t(const typeid_t& element_type, const std::vector<value_t>& elements) :
			_type_int(type_int::k_ext),
			_ext(std::make_shared<value_ext_t>(value_ext_t(typeid_t::make_vector(element_type), elements)))
		{
#if DEBUG
			DEBUG_STR = make_value_debug_str(*this);
#endif

			QUARK_ASSERT(check_invariant());
		}

		value_t::value_t(const typeid_t& value_type, const std::map<std::string, value_t>& entries) :
			_type_int(type_int::k_ext),
			_ext(std::make_shared<value_ext_t>(value_ext_t(typeid_t::make_dict(value_type), entries)))
		{
#if DEBUG
			DEBUG_STR = make_value_debug_str(*this);
#endif

			QUARK_ASSERT(check_invariant());
		}

		value_t::value_t(const typeid_t& type, int function_id) :
			_type_int(type_int::k_ext),
			_ext(std::make_shared<value_ext_t>(value_ext_t(type, function_id)))
		{
#if DEBUG
			DEBUG_STR = make_value_debug_str(*this);
#endif

			QUARK_ASSERT(check_invariant());
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
		const auto& struct_value = v.get_struct_value();
		std::map<string, json_t> obj2;
		for(int i = 0 ; i < struct_value->_def->_members.size() ; i++){
			const auto& member = struct_value->_def->_members[i];
			const auto& key = member._name;
			const auto& type = member._type;
			const auto& value = struct_value->_member_values[i];
			const auto& value2 = value_to_ast_json(value);
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
		const auto& vec = v.get_vector_value();
		return ast_json_t{ values_to_json_array(vec) 	};
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
		const auto entries = v.get_dict_value();
		std::map<string, json_t> result;
		for(const auto& e: entries){
			result[e.first] = value_to_ast_json(e.second)._value;
		}
		return ast_json_t{result};
	}
	else if(v.is_function()){
		return ast_json_t{json_t::make_object(
			{
				{ "function_type", typeid_to_ast_json(v.get_type())._value }
			}
		)};
	}
	else{
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


		//	Used internally in check_invariant() -- don't call check_invariant().
		typeid_t value_t::get_type() const{
//			QUARK_ASSERT(check_invariant());

			if(_type_int == k_null){
				return typeid_t::make_null();
			}
			else if(_type_int == k_bool){
				return typeid_t::make_bool();
			}
			else if(_type_int == k_int){
				return typeid_t::make_int();
			}
			else if(_type_int == k_float){
				return typeid_t::make_float();
			}
			else if(_type_int == k_ext){
				QUARK_ASSERT(_ext);
				return _ext->_type;
			}
			else{
				QUARK_ASSERT(false);
				throw std::exception();
			}
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

	auto instance = std::make_shared<struct_instance_t>(struct_instance_t{struct_type.get_struct_ref(), values});
	return value_t(struct_type, instance);
}

value_t value_t::make_vector_value(const typeid_t& element_type, const std::vector<value_t>& elements){
	return value_t(element_type, elements);
}

value_t value_t::make_dict_value(const typeid_t& value_type, const std::map<std::string, value_t>& entries){
	return value_t(value_type, entries);
}

value_t value_t::make_function_value(const typeid_t& function_type, int function_id){
	QUARK_ASSERT(function_type.check_invariant());
	return value_t(function_type, function_id);
}




	json_t values_to_json_array(const std::vector<value_t>& values){
		std::vector<json_t> r;
		for(const auto& i: values){
			const auto& j = value_to_ast_json(i)._value;
			r.push_back(j);
		}
		return json_t::make_array(r);
	}


}	//	floyd
