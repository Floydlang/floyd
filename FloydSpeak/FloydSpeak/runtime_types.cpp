
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



namespace runtime_types {



	using std::string;

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







	////////////////////////			frontend_type_t

		bool frontend_type_t::check_invariant() const{
			if(_base_type == k_int32){
				QUARK_ASSERT(!_output_type);
				QUARK_ASSERT(_input_types.empty());
			}
			else if(_base_type == k_string){
				QUARK_ASSERT(!_output_type);
				QUARK_ASSERT(_input_types.empty());
			}
			else if(_base_type == k_struct){
				QUARK_ASSERT(!_output_type);
			}
			else if(_base_type == k_vector){
				QUARK_ASSERT(_output_type);
				QUARK_ASSERT(_input_types.empty());
			}
			else{
				QUARK_ASSERT(false);
			}
			return true;
		}

		frontend_type_t frontend_type_t::make_int32(){
			frontend_type_t result;
			result._base_type = k_int32;
			QUARK_ASSERT(result.check_invariant());
			return result;
		}

		frontend_type_t frontend_type_t::make_struct(const std::string& name, const std::vector<std::shared_ptr<frontend_type_t> >& members){
			frontend_type_t result;
			result._name = name;
			result._base_type = k_struct;
			result._input_types = members;
			QUARK_ASSERT(result.check_invariant());
			return result;
		}




	void trace_frontend_type(const frontend_type_t& t){
		QUARK_ASSERT(t.check_invariant());

		QUARK_TRACE("frontend_type_t <" + t._name + ">");

		const auto s = to_string(t._base_type);
	}


	std::string to_signature(const frontend_type_t& t){
		QUARK_ASSERT(t.check_invariant());

		const auto base_type = to_string(t._base_type);

		const string label = t._name.empty() ? "" : t._name + ": ";
		if(t._base_type == k_struct){
			string body;
			for(const auto& member : t._input_types) {
				string member_s = member->_name.empty() ? to_signature(*member) : "<" + member->_name + ">";
				body = body + member_s;
			}
			return label + "<struct>" + "{" + body + "}";
		}
		else if(t._base_type == k_vector){
			const auto vector_value_s = t._output_type->_name.empty() ? to_signature(*t._output_type) : "<" + t._output_type->_name + ">";
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






QUARK_UNIT_TESTQ("to_signature()", ""){
	QUARK_TEST_VERIFY(to_signature(frontend_type_t::make_int32()) == "<int32>");
	QUARK_TEST_VERIFY(auto a = to_signature(frontend_type_t::make_struct("", {})) == "<struct>{}");
}



