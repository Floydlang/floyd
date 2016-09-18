//
//  parser_value.cpp
//  FloydSpeak
//
//  Created by Marcus Zetterquist on 26/07/16.
//  Copyright © 2016 Marcus Zetterquist. All rights reserved.
//

#include "parser_value.h"

#include "statements.h"
#include "ast_utils.h"


namespace floyd_parser {
	using std::string;
	using std::make_shared;



	//////////////////////////////////////////////////		struct_instance_t

		bool struct_instance_t::check_invariant() const{
			QUARK_ASSERT(_struct_type && _struct_type->check_invariant());

			QUARK_ASSERT(_struct_type->get_struct_def()->_members.size() == _member_values.size());

			for(const auto m: _member_values){
				QUARK_ASSERT(m.second.check_invariant());

//				const member_t& def_member = find_struct_member_throw(__def, m.first);

//				QUARK_ASSERT(m.second.get_type().to_string() == def_member._type->to_string());
			}
			return true;
		}

		bool struct_instance_t::operator==(const struct_instance_t& other){
			QUARK_ASSERT(check_invariant());
			QUARK_ASSERT(other.check_invariant());

			return _struct_type == other._struct_type && _member_values == other._member_values;
		}


		std::string to_preview(const struct_instance_t& instance){
			string r;
			for(const auto m: instance._member_values){
				r = r + (string("<") + m.second.get_type()->to_string() + ">" + m.first + "=" + m.second.plain_value_to_string());
			}
			return string("{") + r + "}";
		}




	//////////////////////////////////////////////////		vector_instance_t


		bool vector_instance_t::check_invariant() const{
			for(const auto m: _elements){
				QUARK_ASSERT(m.check_invariant());
				//??? check type of member value is the same as in the type_def.
			}
			return true;
		}

		bool vector_instance_t::operator==(const vector_instance_t& other){
			QUARK_ASSERT(check_invariant());
			QUARK_ASSERT(other.check_invariant());

			return *_vector_type == *other._vector_type && _elements == other._elements;
		}


		std::string to_preview(const vector_instance_t& instance){
			string r;
			for(const auto m: instance._elements){
				r = r + m.plain_value_to_string() + " ";
			}
			return /*string("<") + instance._vector_type->_name.to_string() + ">*/ std::string("[") + instance._vector_type->get_vector_def()->_element_type->to_string() + "][" + r + "]";
		}



	value_t make_vector_instance(const std::shared_ptr<const vector_def_t>& def, const std::vector<value_t>& elements){
		QUARK_ASSERT(def && def->check_invariant());

		auto instance = make_shared<vector_instance_t>();
		instance->_vector_type = make_shared<type_def_t>(type_def_t::make_vector_def(def));
		instance->_elements = elements;
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
		int diff = compare_value_true_deep(a_it->second, b_it->second);
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
	QUARK_ASSERT(*left.get_type() == *right.get_type());

	const auto type = left.get_base_type();
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

	QUARK_TEST_VERIFY(a == value_t(true));
	QUARK_TEST_VERIFY(a != value_t(false));
	QUARK_TEST_VERIFY(a.plain_value_to_string() == "true");
	QUARK_TEST_VERIFY(a.value_and_type_to_string() == "<bool>true");
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

	QUARK_TEST_VERIFY(a == value_t(false));
	QUARK_TEST_VERIFY(a != value_t(true));
	QUARK_TEST_VERIFY(a.plain_value_to_string() == "false");
	QUARK_TEST_VERIFY(a.value_and_type_to_string() == "<bool>false");
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

	QUARK_TEST_VERIFY(a == value_t(13));
	QUARK_TEST_VERIFY(a != value_t(14));
	QUARK_TEST_VERIFY(a.plain_value_to_string() == "13");
	QUARK_TEST_VERIFY(a.value_and_type_to_string() == "<int>13");
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

	QUARK_TEST_VERIFY(a == value_t(13.5f));
	QUARK_TEST_VERIFY(a != value_t(14.0f));
	QUARK_TEST_VERIFY(a.plain_value_to_string() == "13.500000");
	QUARK_TEST_VERIFY(a.value_and_type_to_string() == "<float>13.500000");
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

	QUARK_TEST_VERIFY(a == value_t("xyz"));
	QUARK_TEST_VERIFY(a != value_t("xyza"));
	QUARK_TEST_VERIFY(a.plain_value_to_string() == "\"xyz\"");
	QUARK_TEST_VERIFY(a.value_and_type_to_string() == "<string>\"xyz\"");
}

#if false
QUARK_UNIT_TESTQ("value_t()", "struct"){
	struct_fixture_t f;
	const auto a = f._struct6_instance0;
	QUARK_TEST_VERIFY(!a.is_null());
	QUARK_TEST_VERIFY(!a.is_bool());
	QUARK_TEST_VERIFY(!a.is_int());
	QUARK_TEST_VERIFY(!a.is_float());
	QUARK_TEST_VERIFY(!a.is_string());
	QUARK_TEST_VERIFY(a.is_struct());
	QUARK_TEST_VERIFY(!a.is_vector());

	QUARK_TEST_VERIFY(a == f._struct6_instance1);
	QUARK_TEST_VERIFY(a != value_t("xyza"));
	quark::ut_compare(a.plain_value_to_string(), "{<bool>_bool_false=false<bool>_bool_true=true<int>_int=111<pixel>_pixel={<int>blue=77<int>green=66<int>red=55}<string>_string=\"test 123\"}");
	quark::ut_compare(a.value_and_type_to_string(), "<struct6>{<bool>_bool_false=false<bool>_bool_true=true<int>_int=111<pixel>_pixel={<int>blue=77<int>green=66<int>red=55}<string>_string=\"test 123\"}");
}
#endif

QUARK_UNIT_TESTQ("value_t()", "vector"){
	const auto vector_def = make_shared<const vector_def_t>(vector_def_t::make2(type_identifier_t::make("my_vec"), make_shared<type_def_t>(type_def_t::make_int())));
	const auto a = make_vector_instance(vector_def, {});
	const auto b = make_vector_instance(vector_def, {});

	QUARK_TEST_VERIFY(!a.is_null());
	QUARK_TEST_VERIFY(!a.is_bool());
	QUARK_TEST_VERIFY(!a.is_int());
	QUARK_TEST_VERIFY(!a.is_float());
	QUARK_TEST_VERIFY(!a.is_string());
	QUARK_TEST_VERIFY(!a.is_struct());
	QUARK_TEST_VERIFY(a.is_vector());

	QUARK_TEST_VERIFY(a == b);
	QUARK_TEST_VERIFY(a != value_t("xyza"));
	quark::ut_compare(a.plain_value_to_string(), "[int][]");
	quark::ut_compare(a.value_and_type_to_string(), "<my_vec>[int][]");
}


QUARK_UNIT_TESTQ("value_t()", "vector"){
	const auto vector_def = make_shared<const vector_def_t>(vector_def_t::make2(type_identifier_t::make("my_vec"), make_shared<type_def_t>(type_def_t::make_int())));
	const auto a = make_vector_instance(vector_def, { 3, 4, 5});
	const auto b = make_vector_instance(vector_def, { 3, 4 });

	QUARK_TEST_VERIFY(a != b);
	QUARK_TEST_VERIFY(a != value_t("xyza"));
	QUARK_TEST_VERIFY(a.get_vector()->_elements[0] == 3);
	QUARK_TEST_VERIFY(a.get_vector()->_elements[1] == 4);
	QUARK_TEST_VERIFY(a.get_vector()->_elements[2] == 5);
}



#if false

struct_fixture_t::struct_fixture_t() :
	_struct6_def(make_struct6(_ast._global_scope))
{
	auto pixel_def = scope_def_t::make_struct(
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


}	//	floyd_parser
