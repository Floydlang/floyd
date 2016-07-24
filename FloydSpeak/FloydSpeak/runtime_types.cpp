
//
//  runtime_types.cpp
//  FloydSpeak
//
//  Created by Marcus Zetterquist on 24/07/16.
//  Copyright © 2016 Marcus Zetterquist. All rights reserved.
//

#include "runtime_types.h"

#include "quark.h"
#include <vector>
#include <string>
#include <map>

using std::make_shared;
using std::string;


namespace runtime_types {




	string to_string(const frontend_base_type t){
		if(t == k_int32){
			return "int32";
		}
		else if(t == k_string){
			return "string";
		}
		else if(t == k_struct){
			return "struct";
		}
		else if(t == k_vector){
			return "vector";
		}
		else{
			QUARK_ASSERT(false);
		}
	}


	////////////////////////			member_t


	member_t::member_t(const std::string& name, const std::shared_ptr<frontend_type_t>& type) :
		_name(name),
		_type(type)
	{
		QUARK_ASSERT(type);
		QUARK_ASSERT(type->check_invariant());

		QUARK_ASSERT(check_invariant());
	}

	bool member_t::check_invariant() const{
		QUARK_ASSERT(_type);
		QUARK_ASSERT(_type->check_invariant());
		return true;
	}





	////////////////////////			frontend_type_t


	bool frontend_type_t::check_invariant() const{
		if(_base_type == k_int32){
			QUARK_ASSERT(_members.empty());
			QUARK_ASSERT(!_value_type);
			QUARK_ASSERT(!_key_type);
		}
		else if(_base_type == k_string){
			QUARK_ASSERT(_members.empty());
			QUARK_ASSERT(!_value_type);
			QUARK_ASSERT(!_key_type);
		}
		else if(_base_type == k_struct){
//			QUARK_ASSERT(_members.empty());
			QUARK_ASSERT(!_value_type);
			QUARK_ASSERT(!_key_type);
		}
		else if(_base_type == k_vector){
			QUARK_ASSERT(_members.empty());
			QUARK_ASSERT(_value_type);
			QUARK_ASSERT(!_key_type);
		}
		else{
			QUARK_ASSERT(false);
		}
		return true;
	}

	frontend_type_t frontend_type_t::make_int32(const std::string& name){
		frontend_type_t result;
		result._typedef = name;
		result._base_type = k_int32;
		QUARK_ASSERT(result.check_invariant());
		return result;
	}

	frontend_type_t frontend_type_t::make_string(const std::string& name){
		frontend_type_t result;
		result._typedef = name;
		result._base_type = k_string;
		QUARK_ASSERT(result.check_invariant());
		return result;
	}

	frontend_type_t frontend_type_t::make_struct(const std::string& name, const std::vector<member_t>& members){
		frontend_type_t result;
		result._typedef = name;
		result._base_type = k_struct;
		result._members = members;
		QUARK_ASSERT(result.check_invariant());
		return result;
	}






	void trace_frontend_type(const frontend_type_t& t){
		QUARK_ASSERT(t.check_invariant());

		QUARK_TRACE("frontend_type_t <" + t._typedef + ">");

		const auto s = to_string(t._base_type);
	}


	std::string to_signature(const frontend_type_t& t){
		QUARK_ASSERT(t.check_invariant());

		const auto base_type = to_string(t._base_type);

		const string label = t._typedef.empty() ? "" : t._typedef + ": ";
		if(t._base_type == k_struct){
			string body;
			for(const auto& member : t._members) {
				const string member_label = member._name;
				const string typedef_s = member._type->_typedef;
				const string member_type = typedef_s.empty() ? to_signature(*member._type) : "<" + typedef_s + ">";

				//	"<string>first_name"
				const string member_result = member_type + member_label;

				body = body + member_result + ",";
			}

			//	Remove trailing comma, if any.
			if(body.size() > 1 && body.back() == ','){
				body.pop_back();
			}

			return label + "<struct>" + "{" + body + "}";
		}
		else if(t._base_type == k_vector){
			const auto vector_value_s = t._value_type->_typedef.empty() ? to_signature(*t._value_type) : "<" + t._value_type->_typedef + ">";
			return label + "<vector>" + "[" + vector_value_s + "]";
		}
		else{
			return label + "<" + base_type + ">";
		}
	}


} //	runtime_types;



using namespace runtime_types;



//////////////////////////////////////		rt_memory_manager


QUARK_UNIT_TESTQ("to_string()", ""){
	QUARK_TEST_VERIFY(to_string(k_int32) == "int32");
	QUARK_TEST_VERIFY(to_string(k_string) == "string");
	QUARK_TEST_VERIFY(to_string(k_struct) == "struct");
	QUARK_TEST_VERIFY(to_string(k_vector) == "vector");
}


/*
	struct {
		int32 a
		string b
	}
*/
frontend_type_t make_struct_1(){
	const auto a = frontend_type_t::make_struct("",
		{
			member_t("a", make_shared<frontend_type_t>(frontend_type_t::make_int32(""))),
			member_t("b", make_shared<frontend_type_t>(frontend_type_t::make_string("")))
		}
	);
	return a;
}

/*
	struct {
		string x
		<struct>{<int32>a,<string>b} y
		string z
	}
*/
frontend_type_t make_struct_2(){
	const auto a = frontend_type_t::make_struct("",
		{
			member_t("x", make_shared<frontend_type_t>(frontend_type_t::make_string(""))),
			member_t("y", make_shared<frontend_type_t>(make_struct_1())),
			member_t("z", make_shared<frontend_type_t>(frontend_type_t::make_string("")))
		}
	);
	return a;
}



QUARK_UNIT_TESTQ("to_signature()", ""){
	QUARK_TEST_VERIFY(to_signature(frontend_type_t::make_int32("")) == "<int32>");
	QUARK_TEST_VERIFY(to_signature(frontend_type_t::make_string("")) == "<string>");
	QUARK_TEST_VERIFY(to_signature(frontend_type_t::make_struct("", {})) == "<struct>{}");

	const auto t1 = make_struct_1();
	QUARK_TEST_VERIFY(to_signature(t1) == "<struct>{<int32>a,<string>b}");

	const auto t2 = make_struct_2();
	const auto s2 = to_signature(t2);
	QUARK_TEST_VERIFY(s2 == "<struct>{<string>x,<struct>{<int32>a,<string>b}y,<string>z}");
}



