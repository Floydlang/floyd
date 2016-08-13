//
//  parser_value.cpp
//  FloydSpeak
//
//  Created by Marcus Zetterquist on 26/07/16.
//  Copyright © 2016 Marcus Zetterquist. All rights reserved.
//

#include "parser_value.h"

#include "parser_statement.h"


namespace floyd_parser {
	using std::string;
	using std::make_shared;








	//////////////////////////////////////////////////		struct_instance_t

		bool struct_instance_t::check_invariant() const{
			for(const auto m: _member_values){
				QUARK_ASSERT(m.second.check_invariant());
				//??? check type of member value is the same as in the type_def.
			}
			return true;
		}

		bool struct_instance_t::operator==(const struct_instance_t& other){
			QUARK_ASSERT(check_invariant());
			QUARK_ASSERT(other.check_invariant());

			return __def == other.__def && _member_values == other._member_values;
		}


		std::string to_preview(const struct_instance_t& instance){
			string r;
			for(const auto m: instance._member_values){
				r = r + (string("<") + m.first + ">" + m.second.plain_value_to_string());
			}
			return string("{") + r + "}";
		}



	value_t make_struct_instance(const std::shared_ptr<const struct_def_t>& def){
		QUARK_ASSERT(def && def->check_invariant());

		auto instance = make_shared<struct_instance_t>();

		instance->__def = def;
		for(int i = 0 ; i < def->_members.size() ; i++){
			const auto& member_def = def->_members[i];

			const auto member_type = resolve_type(def->_struct_scope, member_def._type->to_string());
			if(!member_type){
				throw std::runtime_error("Undefined struct type!");
			}

			//	If there is an initial value for this member, use that. Else use default value for this type.
			value_t value;
			if(member_def._value){
				value = *member_def._value;
			}
			else{
				value = make_default_value(def->_struct_scope, *member_def._type);
			}
			instance->_member_values[member_def._name] = value;
		}
		return value_t(instance);
	}




	//////////////////////////////////////////////////		value_t


void trace(const value_t& e){
	QUARK_ASSERT(e.check_invariant());

	QUARK_TRACE("value_t: " + e.value_and_type_to_string());
}


QUARK_UNIT_TESTQ("value_t()", "null"){
	QUARK_TEST_VERIFY(value_t().plain_value_to_string() == "<null>");
}

QUARK_UNIT_TESTQ("value_t()", "bool"){
	QUARK_TEST_VERIFY(value_t(true).plain_value_to_string() == "true");
}

QUARK_UNIT_TESTQ("value_t()", "int"){
	QUARK_TEST_VERIFY(value_t(13).plain_value_to_string() == "13");
}

QUARK_UNIT_TESTQ("value_t()", "float"){
	QUARK_TEST_VERIFY(value_t(13.5f).plain_value_to_string() == "13.500000");
}

QUARK_UNIT_TESTQ("value_t()", "string"){
	QUARK_TEST_VERIFY(value_t("hello").plain_value_to_string() == "'hello'");
}

//??? more





//////////////////////////////////////////////////		resolve_type()





std::shared_ptr<type_def_t> resolve_type_deep(const scope_ref_t scope_def, const std::string& s){
	QUARK_ASSERT(scope_def->check_invariant());

	const auto t = scope_def->_types_collector.resolve_identifier(s);
	if(t){
		return t;
	}
	auto parent_scope = scope_def->_parent_scope.lock();
	if(parent_scope){
		return resolve_type_deep(parent_scope, s);
	}
	else{
		return {};
	}
}

std::shared_ptr<type_def_t> resolve_type(const scope_ref_t scope_def, const std::string& s){
	QUARK_ASSERT(scope_def->check_invariant());
	QUARK_ASSERT(s.size() > 0);

	return resolve_type_deep(scope_def, s);
}




//////////////////////////////////////////////////		make_default_value()



value_t make_default_value(const scope_ref_t scope_def, const floyd_parser::type_identifier_t& type){
	QUARK_ASSERT(scope_def->check_invariant());
	QUARK_ASSERT(type.check_invariant());

	const auto t = resolve_type(scope_def, type.to_string());
	if(!t){
		throw std::runtime_error("Undefined type!");
	}
	const auto r = make_default_value(*t);
	return r;
}



	//??? move to pass2 or interpreter
	value_t make_default_value(const std::shared_ptr<struct_def_t>& t){
		QUARK_ASSERT(t && t->check_invariant());
		return make_struct_instance(t);
	}

	value_t make_default_value(const type_def_t& t){
		QUARK_ASSERT(t.check_invariant());

		if(t._base_type == k_int){
			return value_t(0);
		}
		else if(t._base_type == k_bool){
			return value_t(false);
		}
		else if(t._base_type == k_string){
			return value_t("");
		}
		else if(t._base_type == k_struct){
			return make_default_value(t._struct_def);
		}
		else if(t._base_type == k_vector){
			QUARK_ASSERT(false);
		}
		else if(t._base_type == k_function){
			QUARK_ASSERT(false);
		}
		else{
			QUARK_ASSERT(false);
		}
	}







}	//	floyd_parser
