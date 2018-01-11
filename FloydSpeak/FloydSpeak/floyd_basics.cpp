//
//  floyd_basics.cpp
//  FloydSpeak
//
//  Created by Marcus Zetterquist on 2017-08-09.
//  Copyright © 2017 Marcus Zetterquist. All rights reserved.
//

#include "floyd_basics.h"


using std::string;

namespace floyd_basics {

	//////////////////////////////////////////////////		base_type


	string base_type_to_string(const base_type t){
		if(t == base_type::k_null){
			return "null";
		}
		else if(t == base_type::k_bool){
			return "bool";
		}
		else if(t == base_type::k_int){
			return "int";
		}
		else if(t == base_type::k_float){
			return "float";
		}
		else if(t == base_type::k_string){
			return "string";
		}
		else if(t == base_type::k_struct){
			return "struct";
		}
		else if(t == base_type::k_vector){
			return "vector";
		}
		else if(t == base_type::k_function){
			return "function";
		}
		else{
			QUARK_ASSERT(false);
		}
	}




	//////////////////////////////////////		base_type_to_string(base_type)


	QUARK_UNIT_TESTQ("base_type_to_string(base_type)", ""){
		QUARK_TEST_VERIFY(base_type_to_string(base_type::k_bool) == "bool");
		QUARK_TEST_VERIFY(base_type_to_string(base_type::k_int) == "int");
		QUARK_TEST_VERIFY(base_type_to_string(base_type::k_float) == "float");
		QUARK_TEST_VERIFY(base_type_to_string(base_type::k_string) == "string");
		QUARK_TEST_VERIFY(base_type_to_string(base_type::k_struct) == "struct");
		QUARK_TEST_VERIFY(base_type_to_string(base_type::k_vector) == "vector");
		QUARK_TEST_VERIFY(base_type_to_string(base_type::k_function) == "function");
	}



//??? Make separate constant strings for "@" etc and use them all over code instead of "@".

//??? Use "f()" for functions.
//??? Use "[n]" for lookups.

static std::map<expression_type, string> operation_to_string_lookup = {
	{ expression_type::k_arithmetic_add__2, "+" },
	{ expression_type::k_arithmetic_subtract__2, "-" },
	{ expression_type::k_arithmetic_multiply__2, "*" },
	{ expression_type::k_arithmetic_divide__2, "/" },
	{ expression_type::k_arithmetic_remainder__2, "%" },

	{ expression_type::k_comparison_smaller_or_equal__2, "<=" },
	{ expression_type::k_comparison_smaller__2, "<" },
	{ expression_type::k_comparison_larger_or_equal__2, ">=" },
	{ expression_type::k_comparison_larger__2, ">" },

	{ expression_type::k_logical_equal__2, "==" },
	{ expression_type::k_logical_nonequal__2, "!=" },
	{ expression_type::k_logical_and__2, "&&" },
	{ expression_type::k_logical_or__2, "||" },
//	{ expression_type::k_logical_not, "!" },
	{ expression_type::k_arithmetic_unary_minus__1, "unary_minus" },

	{ expression_type::k_literal, "k" },

	{ expression_type::k_conditional_operator3, "?:" },
	{ expression_type::k_call, "call" },

	{ expression_type::k_variable, "@" },
	{ expression_type::k_resolve_member, "->" },

	{ expression_type::k_lookup_element, "[-]" }
};

std::map<string, expression_type> make_reverse(const std::map<expression_type, string>& m){
	std::map<string, expression_type> temp;
	for(const auto e: m){
		temp[e.second] = e.first;
	}
	return temp;
}

static std::map<string, expression_type> string_to_operation_lookip = make_reverse(operation_to_string_lookup);




string expression_type_to_token(const expression_type& op){
	const auto r = operation_to_string_lookup.find(op);
	QUARK_ASSERT(r != operation_to_string_lookup.end());
	return r->second;
}

expression_type token_to_expression_type(const string& op){
	const auto r = string_to_operation_lookip.find(op);
	QUARK_ASSERT(r != string_to_operation_lookip.end());
	return r->second;
}




	//////////////////////////////////////////////////		typeid_t



	//??? use json
	void trace(const typeid_t& t, const std::string& label){
		QUARK_ASSERT(t.check_invariant());

		const auto type = t.get_base_type();
		if(type == floyd_basics::base_type::k_bool){
			QUARK_TRACE("<" + base_type_to_string(type) + "> " + label);
		}
		else if(type == floyd_basics::base_type::k_int){
			QUARK_TRACE("<" + base_type_to_string(type) + "> " + label);
		}
		else if(type == floyd_basics::base_type::k_float){
			QUARK_TRACE("<" + base_type_to_string(type) + "> " + label);
		}
		else if(type == floyd_basics::base_type::k_string){
			QUARK_TRACE("<" + base_type_to_string(type) + "> " + label);
		}
		else if(type == floyd_basics::base_type::k_struct){
			QUARK_SCOPED_TRACE("<" + base_type_to_string(type) + "> " + label);
//			trace(t.get_struct_def());
		}
		else if(type == floyd_basics::base_type::k_vector){
			QUARK_SCOPED_TRACE("<" + base_type_to_string(type) + "> " + label);
//			trace(*t._vector_def->_value_type, "");
		}
		else if(type == floyd_basics::base_type::k_function){
			QUARK_SCOPED_TRACE("<" + base_type_to_string(type) + "> " + label);
//			trace(t.get_function_def());
		}
		else{
			QUARK_ASSERT(false);
		}
	}



	////////////////////////			typeid_t


	bool typeid_t::check_invariant() const{
		if (_unresolved_type_symbol.empty() == false){
				QUARK_ASSERT(_base_type == floyd_basics::base_type::k_null);
				QUARK_ASSERT(_parts.empty());
				QUARK_ASSERT(_struct_def_id == "");
		}
		else{
			if(_base_type == floyd_basics::base_type::k_null){
				QUARK_ASSERT(_parts.empty());
				QUARK_ASSERT(_struct_def_id.empty());
			}
			else if(_base_type == floyd_basics::base_type::k_bool){
				QUARK_ASSERT(_parts.empty());
				QUARK_ASSERT(_struct_def_id.empty());
			}
			else if(_base_type == floyd_basics::base_type::k_int){
				QUARK_ASSERT(_parts.empty());
				QUARK_ASSERT(_struct_def_id.empty());
			}
			else if(_base_type == floyd_basics::base_type::k_float){
				QUARK_ASSERT(_parts.empty());
				QUARK_ASSERT(_struct_def_id.empty());
			}
			else if(_base_type == floyd_basics::base_type::k_string){
				QUARK_ASSERT(_parts.empty());
				QUARK_ASSERT(_struct_def_id.empty());
			}
			else if(_base_type == floyd_basics::base_type::k_struct){
				QUARK_ASSERT(_parts.empty() == true);
				QUARK_ASSERT(_struct_def_id.empty() == false);
			}
			else if(_base_type == floyd_basics::base_type::k_vector){
				QUARK_ASSERT(_parts.empty() == false);
				QUARK_ASSERT(_struct_def_id.empty() == true);
			}
			else if(_base_type == floyd_basics::base_type::k_function){
				QUARK_ASSERT(_parts.empty() == false);
				QUARK_ASSERT(_struct_def_id.empty() == true);
			}
			else{
				QUARK_ASSERT(false);
			}
		}
		return true;
	}

	void typeid_t::swap(typeid_t& other){
		QUARK_ASSERT(other.check_invariant());
		QUARK_ASSERT(check_invariant());

		std::swap(_base_type, other._base_type);
		_parts.swap(other._parts);
		_struct_def_id.swap(other._struct_def_id);
		_unresolved_type_symbol.swap(other._unresolved_type_symbol);

		QUARK_ASSERT(other.check_invariant());
		QUARK_ASSERT(check_invariant());
	}

	std::string typeid_t::to_string() const {
		QUARK_ASSERT(check_invariant());

		if(_unresolved_type_symbol != ""){
			return /*"unresolved:" +*/ _unresolved_type_symbol;
		}
		else if(_base_type == floyd_basics::base_type::k_struct){
			return "[" + _parts[0].to_string() + "]";
		}
		else if(_base_type == floyd_basics::base_type::k_vector){
			return "[" + _parts[0].to_string() + "]";
		}
		else if(_base_type == floyd_basics::base_type::k_function){
			auto s = _parts[0].to_string() + " (";
			if(_parts.size() > 2){
				for(int i = 1 ; i < _parts.size() - 1 ; i++){
					s = s + _parts[i].to_string() + ",";
				}
				s = s + _parts[_parts.size() - 1].to_string();
			}
			s = s + ")";
			return s;
		}
		else{
			return base_type_to_string(_base_type);
		}
//		return json_to_compact_string(typeid_to_json(*this));
	}

	json_t typeid_to_json(const typeid_t& t){
		if(t._parts.empty() && t._struct_def_id.empty()){
			return base_type_to_string(t.get_base_type());
		}
		else{
			auto parts = json_t::make_array();
			for(const auto e: t._parts){
				parts = push_back(parts, typeid_to_json(e));
			}
			return make_object({
				{ "base_type", json_t(base_type_to_string(t.get_base_type())) },
				{ "parts", parts },
				{ "struct_def_id", t._struct_def_id.empty() == false ? t._struct_def_id : json_t() }
			});
		}
	}


	QUARK_UNIT_TESTQ("typeid_t{}", ""){
	}



}
