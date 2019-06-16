//
//  host_functions.cpp
//  FloydSpeak
//
//  Created by Marcus Zetterquist on 2018-02-23.
//  Copyright © 2018 Marcus Zetterquist. All rights reserved.
//

#include "bytecode_host_functions.h"

#include "json_support.h"
#include "ast_typeid_helpers.h"

#include "text_parser.h"
#include "file_handling.h"
#include "sha1_class.h"
#include "ast_value.h"
#include "ast_json.h"

#include <cmath>
#include <sys/time.h>

#include <thread>
#include <chrono>
#include <algorithm>
#include <iostream>
#include <fstream>

#include "floyd_runtime.h"
#include "floyd_filelib.h"


namespace floyd {

using std::vector;
using std::string;
using std::pair;
using std::shared_ptr;
using std::make_shared;




struct host_function_record_t {
	std::string _name;
	BC_HOST_FUNCTION_PTR _f;

	function_id_t _function_id;

	floyd::typeid_t _function_type;
};



//??? Remove usage of value_t
value_t value_to_jsonvalue(const value_t& value){
	const auto j = value_to_ast_json(value, json_tags::k_plain);
	value_t json_value = value_t::make_json_value(j);
	return json_value;
}





bc_value_t host__assert(interpreter_t& vm, const bc_value_t args[], int arg_count){
	QUARK_ASSERT(vm.check_invariant());
	QUARK_ASSERT(arg_count == 1);
	QUARK_ASSERT(args[0]._type.is_bool());

	const auto& value = args[0];
	bool ok = value.get_bool_value();
	if(!ok){
		vm._print_output.push_back("Assertion failed.");
		quark::throw_runtime_error("Floyd assertion failed.");
	}
	return bc_value_t::make_undefined();
}

//	string to_string(value_t)
bc_value_t host__to_string(interpreter_t& vm, const bc_value_t args[], int arg_count){
	QUARK_ASSERT(vm.check_invariant());
	QUARK_ASSERT(arg_count == 1);

	const auto& value = args[0];
	const auto a = to_compact_string2(bc_to_value(value));
	return bc_value_t::make_string(a);
}
bc_value_t host__to_pretty_string(interpreter_t& vm, const bc_value_t args[], int arg_count){
	QUARK_ASSERT(vm.check_invariant());
	QUARK_ASSERT(arg_count == 1);

	const auto& value = args[0];
	const auto json = bcvalue_to_json(value);
	const auto s = json_to_pretty_string(json, 0, pretty_t{ 80, 4 });
	return bc_value_t::make_string(s);
}

bc_value_t host__typeof(interpreter_t& vm, const bc_value_t args[], int arg_count){
	QUARK_ASSERT(vm.check_invariant());
	QUARK_ASSERT(arg_count == 1);

	const auto& value = args[0];
	const auto type = value._type;
	const auto result = value_t::make_typeid_value(type);
	return value_to_bc(result);
}



bc_value_t host__update(interpreter_t& vm, const bc_value_t args[], int arg_count){
	QUARK_ASSERT(vm.check_invariant());
	QUARK_ASSERT(arg_count == 3);
//	QUARK_TRACE(json_to_pretty_string(interpreter_to_json(vm)));

	const auto& obj1 = args[0];
	const auto& lookup_key = args[1];
	const auto& new_value = args[2];
	return update_element(vm, obj1, lookup_key, new_value);
}

/*bc_value_t host__size(interpreter_t& vm, const bc_value_t args[], int arg_count){
	QUARK_ASSERT(vm.check_invariant());
	QUARK_ASSERT(arg_count == 1);

		QUARK_ASSERT(false);

	const auto obj = args[0];
	if(obj._type.is_string()){
		const auto size = obj.get_string_value().size();
		return bc_value_t::make_int(static_cast<int>(size));
	}
	else if(obj._type.is_json_value()){
		const auto value = obj.get_json_value();
		if(value.is_object()){
			const auto size = value.get_object_size();
			return bc_value_t::make_int(static_cast<int>(size));
		}
		else if(value.is_array()){
			const auto size = value.get_array_size();
			return bc_value_t::make_int(static_cast<int>(size));
		}
		else if(value.is_string()){
			const auto size = value.get_string().size();
			return bc_value_t::make_int(static_cast<int>(size));
		}
		else{
			quark::throw_runtime_error("Calling size() on unsupported type of value.");
		}
	}
	else if(obj._type.is_vector()){
		if(encode_as_vector_w_inplace_elements(obj._type)){
			const auto size = obj._pod._external->_vector_w_inplace_elements.size();
			return bc_value_t::make_int(static_cast<int>(size));
		}
		else{
			const auto size = get_vector_value(obj)->size();
			return bc_value_t::make_int(static_cast<int>(size));
		}
	}
	else if(obj._type.is_dict()){
		if(encode_as_dict_w_inplace_values(obj._type)){
			const auto size = obj._pod._external->_dict_w_inplace_values.size();
			return bc_value_t::make_int(static_cast<int>(size));
		}
		else{
			const auto size = get_dict_value(obj).size();
			return bc_value_t::make_int(static_cast<int>(size));
		}
	}
	else{
		quark::throw_runtime_error("Calling size() on unsupported type of value.");
	}
}
*/

bc_value_t host__find(interpreter_t& vm, const bc_value_t args[], int arg_count){
	QUARK_ASSERT(vm.check_invariant());
	QUARK_ASSERT(arg_count == 2);

	const auto obj = args[0];
	const auto wanted = args[1];

	if(obj._type.is_string()){
		const auto str = obj.get_string_value();
		const auto wanted2 = wanted.get_string_value();

		const auto r = str.find(wanted2);
		int result = r == std::string::npos ? -1 : static_cast<int>(r);
		return bc_value_t::make_int(result);
	}
	else if(obj._type.is_vector()){
		const auto element_type = obj._type.get_vector_element_type();
		if(wanted._type != element_type){
			QUARK_ASSERT(false);
			quark::throw_runtime_error("Type mismatch.");
		}
		else if(obj._type.get_vector_element_type().is_bool()){
			const auto& vec = obj._pod._external->_vector_w_inplace_elements;
			int index = 0;
			const auto size = vec.size();
			while(index < size && vec[index]._bool != wanted._pod._inplace._bool){
				index++;
			}
			int result = index == size ? -1 : static_cast<int>(index);
			return bc_value_t::make_int(result);
		}
		else if(obj._type.get_vector_element_type().is_int()){
			const auto& vec = obj._pod._external->_vector_w_inplace_elements;
			int index = 0;
			const auto size = vec.size();
			while(index < size && vec[index]._int64 != wanted._pod._inplace._int64){
				index++;
			}
			int result = index == size ? -1 : static_cast<int>(index);
			return bc_value_t::make_int(result);
		}
		else if(obj._type.get_vector_element_type().is_double()){
			const auto& vec = obj._pod._external->_vector_w_inplace_elements;
			int index = 0;
			const auto size = vec.size();
			while(index < size && vec[index]._double != wanted._pod._inplace._double){
				index++;
			}
			int result = index == size ? -1 : static_cast<int>(index);
			return bc_value_t::make_int(result);
		}
		else{
			const auto& vec = *get_vector_external_elements(obj);
			const auto size = vec.size();
			int index = 0;
			while(index < size && bc_compare_value_exts(vec[index], bc_external_handle_t(wanted), element_type) != 0){
				index++;
			}
			int result = index == size ? -1 : static_cast<int>(index);
			return bc_value_t::make_int(result);
		}
	}
	else{
		quark::throw_runtime_error("Calling find() on unsupported type of value.");
	}
}

//??? user function type overloading and create several different functions, depending on the DYN argument.


bc_value_t host__exists(interpreter_t& vm, const bc_value_t args[], int arg_count){
	QUARK_ASSERT(vm.check_invariant());
	QUARK_ASSERT(arg_count == 2);

	const auto obj = args[0];
	const auto key = args[1];

	if(obj._type.is_dict()){
		if(key._type.is_string() == false){
			quark::throw_runtime_error("Key must be string.");
		}

		const auto key_string = key.get_string_value();

		if(encode_as_dict_w_inplace_values(obj._type)){
			const auto found_ptr = obj._pod._external->_dict_w_inplace_values.find(key_string);
			return bc_value_t::make_bool(found_ptr != nullptr);
		}
		else{
			const auto entries = get_dict_value(obj);
			const auto found_ptr = entries.find(key_string);
			return bc_value_t::make_bool(found_ptr != nullptr);
		}
	}
	else{
		quark::throw_runtime_error("Calling exist() on unsupported type of value.");
	}
}

bc_value_t host__erase(interpreter_t& vm, const bc_value_t args[], int arg_count){
	QUARK_ASSERT(vm.check_invariant());
	QUARK_ASSERT(arg_count == 2);

	const auto obj = args[0];
	const auto key = args[1];

	if(obj._type.is_dict()){
		if(key._type.is_string() == false){
			quark::throw_runtime_error("Key must be string.");
		}
		const auto key_string = key.get_string_value();

		const auto value_type = obj._type.get_dict_value_type();
		if(encode_as_dict_w_inplace_values(obj._type)){
			auto entries2 = obj._pod._external->_dict_w_inplace_values.erase(key_string);
			const auto value2 = make_dict(value_type, entries2);
			return value2;
		}
		else{
			auto entries2 = get_dict_value(obj);
			entries2 = entries2.erase(key_string);
			const auto value2 = make_dict(value_type, entries2);
			return value2;
		}
	}
	else{
		quark::throw_runtime_error("Calling exist() on unsupported type of value.");
	}
}

/*
//	assert(push_back(["one","two"], "three") == ["one","two","three"])
bc_value_t host__push_back(interpreter_t& vm, const bc_value_t args[], int arg_count){
	QUARK_ASSERT(vm.check_invariant());
	QUARK_ASSERT(arg_count == 2);

	QUARK_ASSERT(false);

	const auto obj = args[0];
	const auto element = args[1];
	if(obj._type.is_string()){
		const auto str = obj.get_string_value();
		const auto ch = element.get_string_value();
		auto str2 = str + ch;
		return bc_value_t::make_string(str2);
	}
	else if(obj._type.is_vector()){
		QUARK_ASSERT(false);
		const auto element_type = obj._type.get_vector_element_type();
		if(element._type != element_type){
			quark::throw_runtime_error("Type mismatch.");
		}
		else if(encode_as_vector_w_inplace_elements(obj._type)){
			auto elements2 = obj._pod._external->_vector_w_inplace_elements.push_back(element._pod._pod64);
			const auto v = make_vector(element_type, elements2);
			return v;
		}
		else{
			const auto vec = *get_vector_value(obj);
			auto elements2 = vec.push_back(bc_external_handle_t(element));
			const auto v = make_vector_value(element_type, elements2);
			return v;
		}
	}
	else{
		quark::throw_runtime_error("Calling push_back() on unsupported type of value.");
	}
}
*/

//	assert(subset("abc", 1, 3) == "bc");
bc_value_t host__subset(interpreter_t& vm, const bc_value_t args[], int arg_count){
	QUARK_ASSERT(vm.check_invariant());
	QUARK_ASSERT(arg_count == 3);
	QUARK_ASSERT(args[1]._type.is_int());
	QUARK_ASSERT(args[2]._type.is_int());

	const auto obj = args[0];

	const auto start = args[1].get_int_value();
	const auto end = args[2].get_int_value();
	if(start < 0 || end < 0){
		quark::throw_runtime_error("subset() requires start and end to be non-negative.");
	}

	//??? Move functionallity into seprate function.
	if(obj._type.is_string()){
		const auto str = obj.get_string_value();
		const auto start2 = std::min(start, static_cast<int64_t>(str.size()));
		const auto end2 = std::min(end, static_cast<int64_t>(str.size()));

		string str2;
		for(auto i = start2 ; i < end2 ; i++){
			str2.push_back(str[i]);
		}
		const auto v = bc_value_t::make_string(str2);
		return v;
	}
	else if(obj._type.is_vector()){
		if(encode_as_vector_w_inplace_elements(obj._type)){
			const auto& element_type = obj._type.get_vector_element_type();
			const auto& vec = obj._pod._external->_vector_w_inplace_elements;
			const auto start2 = std::min(start, static_cast<int64_t>(vec.size()));
			const auto end2 = std::min(end, static_cast<int64_t>(vec.size()));
			immer::vector<bc_inplace_value_t> elements2;
			for(auto i = start2 ; i < end2 ; i++){
				elements2 = elements2.push_back(vec[i]);
			}
			const auto v = make_vector(element_type, elements2);
			return v;
		}
		else{
			const auto& vec = obj._pod._external->_vector_w_external_elements;
			const auto element_type = obj._type.get_vector_element_type();
			const auto start2 = std::min(start, static_cast<int64_t>(vec.size()));
			const auto end2 = std::min(end, static_cast<int64_t>(vec.size()));
			immer::vector<bc_external_handle_t> elements2;
			for(auto i = start2 ; i < end2 ; i++){
				elements2 = elements2.push_back(vec[i]);
			}
			const auto v = make_vector(element_type, elements2);
			return v;
		}
	}
	else{
		quark::throw_runtime_error("Calling push_back() on unsupported type of value.");
	}
}


//	assert(replace("One ring to rule them all", 4, 7, "rabbit") == "One rabbit to rule them all");
bc_value_t host__replace(interpreter_t& vm, const bc_value_t args[], int arg_count){
	QUARK_ASSERT(vm.check_invariant());
	QUARK_ASSERT(arg_count == 4);
	QUARK_ASSERT(args[1]._type.is_int());
	QUARK_ASSERT(args[2]._type.is_int());

	const auto obj = args[0];

	const auto start = args[1].get_int_value();
	const auto end = args[2].get_int_value();
	if(start < 0 || end < 0){
		quark::throw_runtime_error("replace() requires start and end to be non-negative.");
	}
	if(start > end){
		quark::throw_runtime_error("replace() requires start <= end.");
	}
	if(args[3]._type != args[0]._type){
		quark::throw_runtime_error("replace() requires argument 4 to be same type of collection.");
	}

	if(obj._type.is_string()){
		const auto str = obj.get_string_value();
		const auto start2 = std::min(start, static_cast<int64_t>(str.size()));
		const auto end2 = std::min(end, static_cast<int64_t>(str.size()));
		const auto new_bits = args[3].get_string_value();

		string str2 = str.substr(0, start2) + new_bits + str.substr(end2);
		const auto v = bc_value_t::make_string(str2);
		return v;
	}
	else if(obj._type.is_vector()){
		if(encode_as_vector_w_inplace_elements(obj._type)){
			const auto& vec = obj._pod._external->_vector_w_inplace_elements;
			const auto element_type = obj._type.get_vector_element_type();
			const auto start2 = std::min(start, static_cast<int64_t>(vec.size()));
			const auto end2 = std::min(end, static_cast<int64_t>(vec.size()));
			const auto& new_bits = args[3]._pod._external->_vector_w_inplace_elements;

			auto result = immer::vector<bc_inplace_value_t>(vec.begin(), vec.begin() + start2);
			for(int i = 0 ; i < new_bits.size() ; i++){
				result = result.push_back(new_bits[i]);
			}
			for(int i = 0 ; i < (vec.size( ) - end2) ; i++){
				result = result.push_back(vec[end2 + i]);
			}
			const auto v = make_vector(element_type, result);
			return v;
		}
		else{
			const auto& vec = obj._pod._external->_vector_w_external_elements;
			const auto element_type = obj._type.get_vector_element_type();
			const auto start2 = std::min(start, static_cast<int64_t>(vec.size()));
			const auto end2 = std::min(end, static_cast<int64_t>(vec.size()));
			const auto& new_bits = args[3]._pod._external->_vector_w_external_elements;

			auto result = immer::vector<bc_external_handle_t>(vec.begin(), vec.begin() + start2);
			for(int i = 0 ; i < new_bits.size() ; i++){
				result = result.push_back(new_bits[i]);
			}
			for(int i = 0 ; i < (vec.size( ) - end2) ; i++){
				result = result.push_back(vec[end2 + i]);
			}
			const auto v = make_vector(element_type, result);
			return v;
		}
	}
	else{
		quark::throw_runtime_error("Calling replace() on unsupported type of value.");
	}
}
/*
	Reads json from a text string, returning an unpacked json_value.
*/
bc_value_t host__script_to_jsonvalue(interpreter_t& vm, const bc_value_t args[], int arg_count){
	QUARK_ASSERT(vm.check_invariant());
	QUARK_ASSERT(arg_count == 1);
	QUARK_ASSERT(args[0]._type.is_string());

	const string s = args[0].get_string_value();
	std::pair<json_t, seq_t> result = parse_json(seq_t(s));
	const auto json_value = bc_value_t::make_json_value(result.first);
	return json_value;
}

bc_value_t host__jsonvalue_to_script(interpreter_t& vm, const bc_value_t args[], int arg_count){
	QUARK_ASSERT(vm.check_invariant());
	QUARK_ASSERT(arg_count == 1);
	QUARK_ASSERT(args[0]._type.is_json_value());

	const auto value0 = args[0].get_json_value();
	const string s = json_to_compact_string(value0);
	return bc_value_t::make_string(s);
}


bc_value_t host__value_to_jsonvalue(interpreter_t& vm, const bc_value_t args[], int arg_count){
	QUARK_ASSERT(vm.check_invariant());
	QUARK_ASSERT(arg_count == 1);

	const auto value = args[0];
	const auto value2 = bc_to_value(args[0]);
	const auto result = value_to_jsonvalue(value2);
	return value_to_bc(result);
}

bc_value_t host__jsonvalue_to_value(interpreter_t& vm, const bc_value_t args[], int arg_count){
	QUARK_ASSERT(vm.check_invariant());
	QUARK_ASSERT(arg_count == 2);
	QUARK_ASSERT(args[0]._type.is_json_value());
	QUARK_ASSERT(args[1]._type.is_typeid());

	const auto json_value = args[0].get_json_value();
	const auto target_type = args[1].get_typeid_value();
	const auto result = unflatten_json_to_specific_type(json_value, target_type);
	return value_to_bc(result);
}


bc_value_t host__get_json_type(interpreter_t& vm, const bc_value_t args[], int arg_count){
	QUARK_ASSERT(vm.check_invariant());
	QUARK_ASSERT(arg_count == 1);
	QUARK_ASSERT(args[0]._type.is_json_value());


	const auto json_value = args[0].get_json_value();
	return bc_value_t::make_int(get_json_type(json_value));
}


/////////////////////////////////////////		PURE -- SHA1


/*
# FUTURE -- BUILT-IN SHA1 FUNCTIONS
```

sha1_t calc_sha1(string s)
sha1_t calc_sha1(binary_t data)

//	SHA1 is 20 bytes.
//	String representation uses hex, so is 40 characters long.
//	"1234567890123456789012345678901234567890"
let sha1_bytes = 20
let sha1_chars = 40

string sha1_to_string(sha1_t s)
sha1_t sha1_from_string(string s)

```
*/


bc_value_t host__calc_string_sha1(interpreter_t& vm, const bc_value_t args[], int arg_count){
	QUARK_ASSERT(vm.check_invariant());
	QUARK_ASSERT(arg_count == 1);
	QUARK_ASSERT(args[0]._type.is_string());

	const auto& s = args[0].get_string_value();
	const auto sha1 = CalcSHA1(s);
	const auto ascii40 = SHA1ToStringPlain(sha1);

	const auto result = value_t::make_struct_value(
		make__sha1_t__type(),
		{
			value_t::make_string(ascii40)
		}
	);

#if 1
	const auto debug = value_and_type_to_ast_json(result);
	QUARK_TRACE(json_to_pretty_string(debug));
#endif

	const auto v = value_to_bc(result);
	return v;
}



bc_value_t host__calc_binary_sha1(interpreter_t& vm, const bc_value_t args[], int arg_count){
	QUARK_ASSERT(vm.check_invariant());
	QUARK_ASSERT(arg_count == 1);
	QUARK_ASSERT(args[0]._type == make__binary_t__type());

	const auto& sha1_struct = args[0].get_struct_value();
	QUARK_ASSERT(sha1_struct.size() == make__binary_t__type().get_struct()._members.size());
	QUARK_ASSERT(sha1_struct[0]._type.is_string());

	const auto& sha1_string = sha1_struct[0].get_string_value();
	const auto sha1 = CalcSHA1(sha1_string);
	const auto ascii40 = SHA1ToStringPlain(sha1);

	const auto result = value_t::make_struct_value(
		make__sha1_t__type(),
		{
			value_t::make_string(ascii40)
		}
	);

#if 1
	const auto debug = value_and_type_to_ast_json(result);
	QUARK_TRACE(json_to_pretty_string(debug));
#endif

	const auto v = value_to_bc(result);
	return v;
}

/////////////////////////////////////////		PURE -- FUNCTIONAL

/////////////////////////////////////////		PURE -- MAP()

//	[R] map([E], R f(E e))
//??? need to provide context property to map() and pass to f().
bc_value_t host__map(interpreter_t& vm, const bc_value_t args[], int arg_count){
	QUARK_ASSERT(vm.check_invariant());
	QUARK_ASSERT(arg_count == 2);

	if(args[0]._type.is_vector() == false){
		quark::throw_runtime_error("map() arg 1 must be a vector.");
	}
	const auto e_type = args[0]._type.get_vector_element_type();

	if(args[1]._type.is_function() == false){
		quark::throw_runtime_error("map() requires start and end to be integers.");
	}
	const auto f = args[1];
	const auto f_arg_types = f._type.get_function_args();
	const auto r_type = f._type.get_function_return();

	if(f_arg_types.size() != 1){
		quark::throw_runtime_error("map() function f requries 1 argument.");
	}

	if(f_arg_types[0] != e_type){
		quark::throw_runtime_error("map() function f must accept collection elements as its argument.");
	}

	const auto input_vec = get_vector(args[0]);
	immer::vector<bc_value_t> vec2;
	for(const auto& e: input_vec){
		const bc_value_t f_args[1] = { e };
		const auto result1 = call_function_bc(vm, f, f_args, 1);
		vec2 = vec2.push_back(result1);
	}

	const auto result = make_vector(r_type, vec2);

#if 1
	const auto debug = value_and_type_to_ast_json(bc_to_value(result));
	QUARK_TRACE(json_to_pretty_string(debug));
#endif

	return result;
}


/////////////////////////////////////////		PURE -- map_string()

//	If input collection is a string, call f() with one character at a time, output is a new string.
//	string map(string, string f(string:char e))
bc_value_t host__map_string(interpreter_t& vm, const bc_value_t args[], int arg_count){
	QUARK_ASSERT(vm.check_invariant());
	QUARK_ASSERT(arg_count == 2);
	QUARK_ASSERT(args[0]._type.is_string());
	QUARK_ASSERT(args[1]._type.is_function());

	const auto f = args[1];
	const auto f_arg_types = f._type.get_function_args();
	const auto r_type = f._type.get_function_return();

	if(f_arg_types.size() != 1){
		quark::throw_runtime_error("map_string() function f requries 1 argument.");
	}

	if(f_arg_types[0] != typeid_t::make_string()){
		quark::throw_runtime_error("map_string() function f must accept collection elements as its argument.");
	}

	const auto input_vec = args[0].get_string_value();
	std::string vec2;
	for(const auto& e: input_vec){
		const bc_value_t f_args[1] = { bc_value_t::make_string(std::string(1, e)) };
		const auto result1 = call_function_bc(vm, f, f_args, 1);
		QUARK_ASSERT(result1._type.is_string());
		vec2.append(result1.get_string_value());
	}

	const auto result = bc_value_t::make_string(vec2);

#if 1
	const auto debug = value_and_type_to_ast_json(bc_to_value(result));
	QUARK_TRACE(json_to_pretty_string(debug));
#endif

	return result;
}





/////////////////////////////////////////		PURE -- REDUCE()


//	R map([E] elements, R init, R f(R acc, E e))

bc_value_t host__reduce(interpreter_t& vm, const bc_value_t args[], int arg_count){
	QUARK_ASSERT(vm.check_invariant());
	QUARK_ASSERT(arg_count == 3);

	//	Check topology.
	if(args[0]._type.is_vector() == false || args[2]._type.is_function() == false || args[2]._type.get_function_args().size () != 2){
		quark::throw_runtime_error("reduce() requires 3 arguments.");
	}

	const auto& elements = args[0];
	const auto& init = args[1];
	const auto& f = args[2];

	if(
		elements._type.get_vector_element_type() != f._type.get_function_args()[1]
		&& init._type != f._type.get_function_args()[0]
	)
	{
		quark::throw_runtime_error("R reduce([E] elements, R init_value, R (R acc, E element) f");
	}

	const auto input_vec = get_vector(elements);

	bc_value_t acc = init;
	for(const auto& e: input_vec){
		const bc_value_t f_args[2] = { acc, e };
		const auto result1 = call_function_bc(vm, f, f_args, 2);
		acc = result1;
	}

	const auto result = acc;

#if 1
	const auto debug = value_and_type_to_ast_json(bc_to_value(result));
	QUARK_TRACE(json_to_pretty_string(debug));
#endif

	return result;
}




/////////////////////////////////////////		PURE -- filter()


//	[E] filter([E], bool f(E e))


bc_value_t host__filter(interpreter_t& vm, const bc_value_t args[], int arg_count){
	QUARK_ASSERT(vm.check_invariant());
	QUARK_ASSERT(arg_count == 2);

	//	Check topology.
	if(args[0]._type.is_vector() == false || args[1]._type.is_function() == false || args[1]._type.get_function_args().size () != 1){
		quark::throw_runtime_error("filter() requires 2 arguments.");
	}

	const auto& elements = args[0];
	const auto& f = args[1];
	const auto& e_type = elements._type.get_vector_element_type();

	if(
		elements._type.get_vector_element_type() != f._type.get_function_args()[0]
	)
	{
		quark::throw_runtime_error("[E] filter([E], bool f(E e))");
	}

	const auto input_vec = get_vector(elements);
	immer::vector<bc_value_t> vec2;

	for(const auto& e: input_vec){
		const bc_value_t f_args[1] = { e };
		const auto result1 = call_function_bc(vm, f, f_args, 1);
		QUARK_ASSERT(result1._type.is_bool());

		if(result1.get_bool_value()){
			vec2 = vec2.push_back(e);
		}
	}

	const auto result = make_vector(e_type, vec2);

#if 1
	const auto debug = value_and_type_to_ast_json(bc_to_value(result));
	QUARK_TRACE(json_to_pretty_string(debug));
#endif

	return result;
}






/////////////////////////////////////////		PURE -- SUPERMAP()


//	[R] supermap([E] values, [int] parents, R (E, [R]) f)

bc_value_t host__supermap(interpreter_t& vm, const bc_value_t args[], int arg_count){
	QUARK_ASSERT(vm.check_invariant());
	QUARK_ASSERT(arg_count == 3);

	//	Check topology.
	if(args[0]._type.is_vector() && args[1]._type == typeid_t::make_vector(typeid_t::make_int()) && args[2]._type.is_function() && args[2]._type.get_function_args().size () == 2){
	}
	else{
		quark::throw_runtime_error("supermap() requires 3 arguments.");
	}

	const auto& elements = args[0];
	const auto& e_type = elements._type.get_vector_element_type();
	const auto& parents = args[1];
	const auto& f = args[2];
	const auto& r_type = args[2]._type.get_function_return();
	if(
		e_type == f._type.get_function_args()[0]
		&& r_type == f._type.get_function_args()[1].get_vector_element_type()
	){
	}
	else {
		quark::throw_runtime_error("R supermap([E] elements, R init_value, R (R acc, E element) f");
	}

	const auto elements2 = get_vector(elements);
	const auto parents2 = get_vector(parents);

	if(elements2.size() != parents2.size()) {
		quark::throw_runtime_error("supermap() requires elements and parents be the same count.");
	}

	auto elements_todo = elements2.size();
	std::vector<int> rcs(elements2.size(), 0);

	immer::vector<bc_value_t> complete(elements2.size(), bc_value_t());

	for(const auto& e: parents2){
		const auto parent_index = e.get_int_value();

		const auto count = static_cast<int64_t>(elements2.size());
		QUARK_ASSERT(parent_index >= -1);
		QUARK_ASSERT(parent_index < count);

		if(parent_index != -1){
			rcs[parent_index]++;
		}
	}

	while(elements_todo > 0){
		std::vector<int> pass_ids;
		for(int i = 0 ; i < elements2.size() ; i++){
			const auto rc = rcs[i];
			if(rc == 0){
				pass_ids.push_back(i);
				rcs[i] = -1;
			}
		}

		if(pass_ids.empty()){
			quark::throw_runtime_error("supermap() dependency cycle error.");
		}

		for(const auto element_index: pass_ids){
			const auto& e = elements2[element_index];

			//	Make list of the element's inputs -- the must all be complete now.
			immer::vector<bc_value_t> solved_deps;
			for(int element_index2 = 0 ; element_index2 < parents2.size() ; element_index2++){
				const auto& p = parents2[element_index2];
				const auto parent_index = p.get_int_value();
				if(parent_index == element_index){
					QUARK_ASSERT(element_index2 != -1);
					QUARK_ASSERT(element_index2 >= -1 && element_index2 < elements2.size());
					QUARK_ASSERT(rcs[element_index2] == -1);
					QUARK_ASSERT(complete[element_index2]._type.is_undefined() == false);
					const auto& solved = complete[element_index2];
					solved_deps = solved_deps.push_back(solved);
				}
			}

			const bc_value_t f_args[2] = { e, make_vector(r_type, solved_deps) };
			const auto result1 = call_function_bc(vm, f, f_args, 2);

			const auto parent_index = parents2[element_index].get_int_value();
			if(parent_index != -1){
				rcs[parent_index]--;
			}
			complete = complete.set(element_index, result1);
			elements_todo--;
		}
	}

	const auto result = make_vector(r_type, complete);

#if 1
	const auto debug = value_and_type_to_ast_json(bc_to_value(result));
	QUARK_TRACE(json_to_pretty_string(debug));
#endif

	return result;
}





/////////////////////////////////////////		PURE -- SUPERMAP2()

//	Input dependencies are specified for as 1... many integers per E, in order. [-1] or [a, -1] or [a, b, -1 ] etc.
//
//	[R] supermap([E] values, [int] parents, R (E, [R]) f)

struct dep_t {
	int64_t incomplete_count;
	std::vector<int64_t> depends_in_element_index;
};


bc_value_t host__supermap2(interpreter_t& vm, const bc_value_t args[], int arg_count){
	QUARK_ASSERT(vm.check_invariant());
	QUARK_ASSERT(arg_count == 3);

	//	Check topology.
	if(args[0]._type.is_vector() && args[1]._type == typeid_t::make_vector(typeid_t::make_int()) && args[2]._type.is_function() && args[2]._type.get_function_args().size () == 2){
	}
	else{
		quark::throw_runtime_error("supermap() requires 3 arguments.");
	}

	const auto& elements = args[0];
	const auto& e_type = elements._type.get_vector_element_type();
	const auto& dependencies = args[1];
	const auto& f = args[2];
	const auto& r_type = args[2]._type.get_function_return();
	if(
		e_type == f._type.get_function_args()[0]
		&& r_type == f._type.get_function_args()[1].get_vector_element_type()
	){
	}
	else {
		quark::throw_runtime_error("R supermap([E] elements, R init_value, R (R acc, E element) f");
	}

	const auto elements2 = get_vector(elements);
	const auto dependencies2 = get_vector(dependencies);


	immer::vector<bc_value_t> complete(elements2.size(), bc_value_t());

	std::vector<dep_t> element_dependencies(elements2.size(), dep_t{ 0, {} });
	{
		const auto element_count = static_cast<int64_t>(elements2.size());
		const auto dep_index_count = static_cast<int64_t>(dependencies2.size());
		int element_index = 0;
		int dep_index = 0;
		while(element_index < element_count){
			std::vector<int64_t> deps;
			const auto& e = dependencies2[dep_index];
			auto e_int = e.get_int_value();
			while(e_int != -1){
				deps.push_back(e_int);

				dep_index++;
				QUARK_ASSERT(dep_index < dep_index_count);

				e_int = dependencies2[dep_index].get_int_value();
			}
			QUARK_ASSERT(element_index < element_count);
			element_dependencies[element_index] = dep_t{ static_cast<int64_t>(deps.size()), deps };
			element_index++;
		}
		QUARK_ASSERT(element_index == element_count);
		QUARK_ASSERT(dep_index == dep_index_count);
	}

	auto elements_todo = elements2.size();
	while(elements_todo > 0){

		//	Make list of elements that should be processed this pass.
		std::vector<int> pass_ids;
		for(int i = 0 ; i < elements2.size() ; i++){
			const auto rc = element_dependencies[i].incomplete_count;
			if(rc == 0){
				pass_ids.push_back(i);
				element_dependencies[i].incomplete_count = -1;
			}
		}

		if(pass_ids.empty()){
			quark::throw_runtime_error("supermap() dependency cycle error.");
		}

		for(const auto element_index: pass_ids){
			const auto& e = elements2[element_index];

			immer::vector<bc_value_t> ready_elements;
			for(const auto& dep_e: element_dependencies[element_index].depends_in_element_index){
				const auto& ready = complete[dep_e];
				ready_elements = ready_elements.push_back(ready);
			}
			const auto ready_elements2 = make_vector(r_type, ready_elements);
			const bc_value_t f_args[2] = { e, ready_elements2 };

			const auto result1 = call_function_bc(vm, f, f_args, 2);

			//	Decrement incomplete_count for every element that specifies *element_index* as a input dependency.
			for(auto& x: element_dependencies){
				const auto it = std::find(x.depends_in_element_index.begin(), x.depends_in_element_index.end(), element_index);
				if(it != x.depends_in_element_index.end()){
					x.incomplete_count--;
				}
			}

			//	Copy value to output.
			complete = complete.set(element_index, result1);
			elements_todo--;
		}
	}

	const auto result = make_vector(r_type, complete);

#if 1
	const auto debug = value_and_type_to_ast_json(bc_to_value(result));
	QUARK_TRACE(json_to_pretty_string(debug));
#endif

	return result;
}





/////////////////////////////////////////		IMPURE -- MISC




//	Records all output to interpreter
bc_value_t host__print(interpreter_t& vm, const bc_value_t args[], int arg_count){
	QUARK_ASSERT(vm.check_invariant());
	QUARK_ASSERT(arg_count == 1);
//	QUARK_ASSERT(args[0]._type.is_string());

	const auto& value = args[0];
	const auto s = to_compact_string2(bc_to_value(value));
	printf("%s\n", s.c_str());
	vm._print_output.push_back(s);

	return bc_value_t::make_undefined();
}
bc_value_t host__send(interpreter_t& vm, const bc_value_t args[], int arg_count){
	QUARK_ASSERT(vm.check_invariant());
	QUARK_ASSERT(arg_count == 2);
	QUARK_ASSERT(args[0]._type.is_string());
	QUARK_ASSERT(args[1]._type.is_json_value());

	const auto& process_id = args[0].get_string_value();
	const auto& message_json = args[1].get_json_value();

	QUARK_TRACE_SS("send(\"" << process_id << "\"," << json_to_pretty_string(message_json) <<")");


	vm._handler->on_send(process_id, message_json);

	return bc_value_t::make_undefined();
}


bc_value_t host__get_time_of_day(interpreter_t& vm, const bc_value_t args[], int arg_count){
	QUARK_ASSERT(vm.check_invariant());
	QUARK_ASSERT(arg_count == 0);

	std::chrono::time_point<std::chrono::high_resolution_clock> t = std::chrono::high_resolution_clock::now();
	std::chrono::duration<double> elapsed_seconds = t - vm._imm->_start_time;
	const auto ms = elapsed_seconds.count() * 1000.0;
	const auto result = value_t::make_int(int(ms));
	return value_to_bc(result);
}

QUARK_UNIT_TEST("sizeof(int)", "", "", ""){
	QUARK_TRACE(std::to_string(sizeof(int)));
	QUARK_TRACE(std::to_string(sizeof(int64_t)));
}

QUARK_UNIT_TEST("get_time_of_day_ms()", "", "", ""){
	const auto a = std::chrono::high_resolution_clock::now();
	std::this_thread::sleep_for(std::chrono::milliseconds(7));
	const auto b = std::chrono::high_resolution_clock::now();

	std::chrono::duration<double> elapsed_seconds = b - a;
	const int ms = static_cast<int>((static_cast<double>(elapsed_seconds.count()) * 1000.0));

	QUARK_UT_VERIFY(ms >= 7)
}





/////////////////////////////////////////		IMPURE -- FILE SYSTEM




bc_value_t host__read_text_file(interpreter_t& vm, const bc_value_t args[], int arg_count){
	QUARK_ASSERT(vm.check_invariant());
	QUARK_ASSERT(arg_count == 1);
	QUARK_ASSERT(args[0]._type.is_string());

	const string source_path = args[0].get_string_value();
	std::string file_contents = read_text_file(source_path);
	const auto v = bc_value_t::make_string(file_contents);
	return v;
}


void write_text_file(const std::string& abs_path, const std::string& data){
	const auto up = UpDir2(abs_path);

	MakeDirectoriesDeep(up.first);

	std::ofstream outputFile;
	outputFile.open(abs_path);
	if (outputFile.fail()) {
		quark::throw_exception();
	}

	outputFile << data;
	outputFile.close();
}

bc_value_t host__write_text_file(interpreter_t& vm, const bc_value_t args[], int arg_count){
	QUARK_ASSERT(vm.check_invariant());
	QUARK_ASSERT(arg_count == 2);
	QUARK_ASSERT(args[0]._type.is_string());
	QUARK_ASSERT(args[1]._type.is_string());

	const string path = args[0].get_string_value();
	const string file_contents = args[1].get_string_value();

	write_text_file(path, file_contents);

	return bc_value_t();
}



std::vector<value_t> directory_entries_to_values(const std::vector<TDirEntry>& v){
	const auto k_fsentry_t__type = make__fsentry_t__type();
	const auto elements = mapf<value_t>(
		v,
		[&k_fsentry_t__type](const auto& e){
//			const auto t = value_t::make_string(e.fName);
			const auto type_string = e.fType == TDirEntry::kFile ? "file": "dir";
			const auto t2 = value_t::make_struct_value(
				k_fsentry_t__type,
				{
					value_t::make_string(type_string),
					value_t::make_string(e.fNameOnly),
					value_t::make_string(e.fParent)
				}
			);
			return t2;
		}
	);
	return elements;
}




bc_value_t host__get_fsentries_shallow(interpreter_t& vm, const bc_value_t args[], int arg_count){
	QUARK_ASSERT(vm.check_invariant());
	QUARK_ASSERT(arg_count == 1);
	QUARK_ASSERT(args[0]._type.is_string());

	const string path = args[0].get_string_value();
	if(is_valid_absolute_dir_path(path) == false){
		quark::throw_runtime_error("get_fsentries_shallow() illegal input path.");
	}

	const auto a = GetDirItems(path);
	const auto elements = directory_entries_to_values(a);
	const auto k_fsentry_t__type = make__fsentry_t__type();
	const auto vec2 = value_t::make_vector_value(k_fsentry_t__type, elements);

#if 1
	const auto debug = value_and_type_to_ast_json(vec2);
	QUARK_TRACE(json_to_pretty_string(debug));
#endif

	const auto v = value_to_bc(vec2);

	return v;
}

bc_value_t host__get_fsentries_deep(interpreter_t& vm, const bc_value_t args[], int arg_count){
	QUARK_ASSERT(vm.check_invariant());
	QUARK_ASSERT(arg_count == 1);
	QUARK_ASSERT(args[0]._type.is_string());

	const string path = args[0].get_string_value();
	if(is_valid_absolute_dir_path(path) == false){
		quark::throw_runtime_error("get_fsentries_deep() illegal input path.");
	}

	const auto a = GetDirItemsDeep(path);
	const auto elements = directory_entries_to_values(a);
	const auto k_fsentry_t__type = make__fsentry_t__type();
	const auto vec2 = value_t::make_vector_value(k_fsentry_t__type, elements);

#if 0
	const auto debug = value_and_type_to_ast_json(vec2);
	QUARK_TRACE(json_to_pretty_string(debug._value));
#endif

	const auto v = value_to_bc(vec2);

	return v;
}


//??? implement
std::string posix_timespec__to__utc(const time_t& t){
	return std::to_string(t);
}


value_t impl__get_fsentry_info(const std::string& path){
	if(is_valid_absolute_dir_path(path) == false){
		quark::throw_runtime_error("get_fsentry_info() illegal input path.");
	}

	TFileInfo info;
	bool ok = GetFileInfo(path, info);
	QUARK_ASSERT(ok);
	if(ok == false){
		quark::throw_exception();
	}

	const auto parts = SplitPath(path);
	const auto parent = UpDir2(path);

	const auto type_string = info.fDirFlag ? "dir" : "string";
	const auto name = info.fDirFlag ? parent.second : parts.fName;
	const auto parent_path = info.fDirFlag ? parent.first : parts.fPath;

	const auto creation_date = posix_timespec__to__utc(info.fCreationDate);
	const auto modification_date = posix_timespec__to__utc(info.fModificationDate);
	const auto file_size = info.fFileSize;

	const auto result = value_t::make_struct_value(
		make__fsentry_info_t__type(),
		{
			value_t::make_string(type_string),
			value_t::make_string(name),
			value_t::make_string(parent_path),

			value_t::make_string(creation_date),
			value_t::make_string(modification_date),

			value_t::make_int(file_size)
		}
	);

#if 1
	const auto debug = value_and_type_to_ast_json(result);
	QUARK_TRACE(json_to_pretty_string(debug));
#endif

	return result;
}


bc_value_t host__get_fsentry_info(interpreter_t& vm, const bc_value_t args[], int arg_count){
	QUARK_ASSERT(vm.check_invariant());
	QUARK_ASSERT(arg_count == 1);
	QUARK_ASSERT(args[0]._type.is_string());

	const string path = args[0].get_string_value();
	const auto result = impl__get_fsentry_info(path);
	const auto v = value_to_bc(result);
	return v;
}


bc_value_t host__get_fs_environment(interpreter_t& vm, const bc_value_t args[], int arg_count){
	QUARK_ASSERT(vm.check_invariant());
	QUARK_ASSERT(arg_count == 0);

	const auto dirs = GetDirectories();

	const auto result = value_t::make_struct_value(
		make__fs_environment_t__type(),
		{
			value_t::make_string(dirs.home_dir),
			value_t::make_string(dirs.documents_dir),
			value_t::make_string(dirs.desktop_dir),

			value_t::make_string(dirs.application_support),
			value_t::make_string(dirs.preferences_dir),
			value_t::make_string(dirs.cache_dir),
			value_t::make_string(dirs.temp_dir),

			value_t::make_string(dirs.process_dir)
		}
	);

#if 1
	const auto debug = value_and_type_to_ast_json(result);
	QUARK_TRACE(json_to_pretty_string(debug));
#endif

	const auto v = value_to_bc(result);
	return v;
}

//??? refactor common code.

bc_value_t host__does_fsentry_exist(interpreter_t& vm, const bc_value_t args[], int arg_count){
	QUARK_ASSERT(vm.check_invariant());
	QUARK_ASSERT(arg_count == 1);
	QUARK_ASSERT(args[0]._type.is_string());

	const string path = args[0].get_string_value();
	if(is_valid_absolute_dir_path(path) == false){
		quark::throw_runtime_error("does_fsentry_exist() illegal input path.");
	}

	bool exists = DoesEntryExist(path);
	const auto result = value_t::make_bool(exists);
#if 1
	const auto debug = value_and_type_to_ast_json(result);
	QUARK_TRACE(json_to_pretty_string(debug));
#endif

	const auto v = value_to_bc(result);
	return v;
}


bc_value_t host__create_directory_branch(interpreter_t& vm, const bc_value_t args[], int arg_count){
	QUARK_ASSERT(vm.check_invariant());
	QUARK_ASSERT(arg_count == 1);
	QUARK_ASSERT(args[0]._type.is_string());

	const string path = args[0].get_string_value();
	if(is_valid_absolute_dir_path(path) == false){
		quark::throw_runtime_error("create_directory_branch() illegal input path.");
	}

	MakeDirectoriesDeep(path);
	return bc_value_t::make_void();
}

bc_value_t host__delete_fsentry_deep(interpreter_t& vm, const bc_value_t args[], int arg_count){
	QUARK_ASSERT(vm.check_invariant());
	QUARK_ASSERT(arg_count == 1);
	QUARK_ASSERT(args[0]._type.is_string());

	const string path = args[0].get_string_value();
	if(is_valid_absolute_dir_path(path) == false){
		quark::throw_runtime_error("delete_fsentry_deep() illegal input path.");
	}

	DeleteDeep(path);

	return bc_value_t::make_void();
}


bc_value_t host__rename_fsentry(interpreter_t& vm, const bc_value_t args[], int arg_count){
	QUARK_ASSERT(vm.check_invariant());
	QUARK_ASSERT(arg_count == 2);
	QUARK_ASSERT(args[0]._type.is_string());
	QUARK_ASSERT(args[1]._type.is_string());

	const string path = args[0].get_string_value();
	if(is_valid_absolute_dir_path(path) == false){
		quark::throw_runtime_error("rename_fsentry() illegal input path.");
	}
	const string n = args[1].get_string_value();
	if(n.empty()){
		quark::throw_runtime_error("rename_fsentry() illegal input name.");
	}

	RenameEntry(path, n);

	return bc_value_t::make_void();
}



/*
std::string GetTemporaryFile(const std::string& name){
	//	char *tmpnam(char *str)
	//	tempnam()

	char* temp_buffer = std::tempnam(const char *dir, const char *pfx);
	std::free(temp_buffer);
	temp_buffer = nullptr;


}


## make\_temporary\_path()

Returns a path where you can write a file or a directory and it goes into the file system. The name will be randomized. The operating system will delete this data at some point.

	string make_temporary_path() impure

Example:

	"/var/folders/kw/7178zwsx7p3_g10y7rp2mt5w0000gn/T/g10y/7178zwsxp3_g10y7rp2mt

There is never a file extension. You could add one if you want too.
*/


host_function_record_t make_rec(const std::string& name, BC_HOST_FUNCTION_PTR f, function_id_t function_id, typeid_t function_type){
	return host_function_record_t { name, f, function_id, function_type };
}





std::vector<host_function_record_t> get_host_function_records(){
	const auto VOID = typeid_t::make_void();
	const auto DYN = typeid_t::make_any();

	const auto k_fsentry_t__type = make__fsentry_t__type();

	const std::vector<host_function_record_t> result = {
		make_rec("assert", host__assert, 1001, typeid_t::make_function(VOID, { typeid_t::make_bool() }, epure::pure)),
		make_rec("to_string", host__to_string, 1002, typeid_t::make_function(typeid_t::make_string(), { DYN }, epure::pure)),
		make_rec("to_pretty_string", host__to_pretty_string, 1003, typeid_t::make_function(typeid_t::make_string(), { DYN }, epure::pure)),
		make_rec("typeof", host__typeof, 1004, typeid_t::make_function(typeid_t::make_typeid(), { DYN }, epure::pure)),

		make_rec("update", host__update, 1006, typeid_t::make_function_dyn_return({ DYN, DYN, DYN }, epure::pure, typeid_t::return_dyn_type::arg0)),

		//	size() is translated to bc_opcode::k_get_size_vector_w_external_elements() etc.
		make_rec("size", nullptr, 1007, typeid_t::make_function(typeid_t::make_int(), { DYN }, epure::pure)),

		make_rec("find", host__find, 1008, typeid_t::make_function(typeid_t::make_int(), { DYN, DYN }, epure::pure)),
		make_rec("exists", host__exists, 1009, typeid_t::make_function(typeid_t::make_bool(), { DYN, DYN }, epure::pure)),
		make_rec("erase", host__erase, 1010, typeid_t::make_function_dyn_return({ DYN, DYN }, epure::pure, typeid_t::return_dyn_type::arg0)),

		//	push_back() is translated to bc_opcode::k_pushback_vector_w_inplace_elements() etc.
		make_rec("push_back", nullptr, 1011, typeid_t::make_function_dyn_return({ DYN, DYN }, epure::pure, typeid_t::return_dyn_type::arg0)),

		make_rec("subset", host__subset, 1012, typeid_t::make_function_dyn_return({ DYN, typeid_t::make_int(), typeid_t::make_int()}, epure::pure, typeid_t::return_dyn_type::arg0)),
		make_rec("replace", host__replace, 1013, typeid_t::make_function_dyn_return({ DYN, typeid_t::make_int(), typeid_t::make_int(), DYN }, epure::pure, typeid_t::return_dyn_type::arg0)),


		make_rec("script_to_jsonvalue", host__script_to_jsonvalue, 1017, typeid_t::make_function(typeid_t::make_json_value(), {typeid_t::make_string()}, epure::pure)),
		make_rec("jsonvalue_to_script", host__jsonvalue_to_script, 1018, typeid_t::make_function(typeid_t::make_string(), {typeid_t::make_json_value()}, epure::pure)),
		make_rec("value_to_jsonvalue", host__value_to_jsonvalue, 1019, typeid_t::make_function(typeid_t::make_json_value(), { DYN }, epure::pure)),

		//	??? Tricky. How to we compute the return type from the input arguments?
		make_rec("jsonvalue_to_value", host__jsonvalue_to_value, 1020, typeid_t::make_function_dyn_return({ typeid_t::make_json_value(), typeid_t::make_typeid() }, epure::pure, typeid_t::return_dyn_type::arg1_typeid_constant_type)),

		make_rec("get_json_type", host__get_json_type, 1021, typeid_t::make_function(typeid_t::make_int(), {typeid_t::make_json_value()}, epure::pure)),

		make_rec("calc_string_sha1", host__calc_string_sha1, 1031, typeid_t::make_function(make__sha1_t__type(), { typeid_t::make_string() }, epure::pure)),
		make_rec("calc_binary_sha1", host__calc_binary_sha1, 1032, typeid_t::make_function(make__sha1_t__type(), { make__binary_t__type() }, epure::pure)),

		make_rec("map", host__map, 1033, typeid_t::make_function_dyn_return({ DYN, DYN}, epure::pure, typeid_t::return_dyn_type::vector_of_arg1func_return)),
		make_rec(
			"map_string",
			host__map_string,
			1034,
			typeid_t::make_function(
				typeid_t::make_string(),
				{
					typeid_t::make_string(),
					typeid_t::make_function(typeid_t::make_string(), { typeid_t::make_string() }, epure::pure)
				},
				epure::pure
			)
		),
		make_rec("filter", host__filter, 1036, typeid_t::make_function_dyn_return({ DYN, DYN }, epure::pure, typeid_t::return_dyn_type::arg0)),
		make_rec("reduce", host__reduce, 1035, typeid_t::make_function_dyn_return({ DYN, DYN, DYN }, epure::pure, typeid_t::return_dyn_type::arg1)),
		make_rec("supermap", host__supermap, 1037, typeid_t::make_function_dyn_return({ DYN, DYN, DYN }, epure::pure, typeid_t::return_dyn_type::vector_of_arg2func_return)),

		//	print = impure!
		make_rec("print", host__print, 1000, typeid_t::make_function(VOID, { DYN }, epure::pure)),
		make_rec("send", host__send, 1022, typeid_t::make_function(VOID, { typeid_t::make_string(), typeid_t::make_json_value() }, epure::impure)),
		make_rec("get_time_of_day", host__get_time_of_day, 1005, typeid_t::make_function(typeid_t::make_int(), {}, epure::impure)),


		make_rec("read_text_file", host__read_text_file, 1015, typeid_t::make_function(typeid_t::make_string(), { typeid_t::make_string() }, epure::impure)),
		make_rec("write_text_file", host__write_text_file, 1016, typeid_t::make_function(VOID, { typeid_t::make_string(), typeid_t::make_string() }, epure::impure)),
		make_rec(
			"get_fsentries_shallow",
			host__get_fsentries_shallow,
			1023,
			typeid_t::make_function(typeid_t::make_vector(k_fsentry_t__type), { typeid_t::make_string() }, epure::impure)
		),
		make_rec(
			"get_fsentries_deep",
			host__get_fsentries_deep,
			1024,
			typeid_t::make_function(typeid_t::make_vector(k_fsentry_t__type), { typeid_t::make_string() }, epure::impure)
		),
		make_rec(
			"get_fsentry_info",
			host__get_fsentry_info,
			1025,
			typeid_t::make_function(
				make__fsentry_info_t__type(),
				{ typeid_t::make_string() },
				epure::impure
			)
		),
		make_rec(
			"get_fs_environment",
			host__get_fs_environment,
			1026,
			typeid_t::make_function(
				make__fs_environment_t__type(),
				{},
				epure::impure
			)
		),
		make_rec(
			"does_fsentry_exist",
			host__does_fsentry_exist,
			1027,
			typeid_t::make_function(
				typeid_t::make_bool(),
				{ typeid_t::make_string() },
				epure::impure
			)
		),
		make_rec(
			"create_directory_branch",
			host__create_directory_branch,
			1028,
			typeid_t::make_function(
				typeid_t::make_void(),
				{ typeid_t::make_string() },
				epure::impure
			)
		),
		make_rec(
			"delete_fsentry_deep",
			host__delete_fsentry_deep,
			1029,
			typeid_t::make_function(
				typeid_t::make_void(),
				{ typeid_t::make_string() },
				epure::impure
			)
		),
		make_rec(
			"rename_fsentry",
			host__rename_fsentry,
			1030,
			typeid_t::make_function(
				typeid_t::make_void(),
				{ typeid_t::make_string(), typeid_t::make_string() },
				epure::impure
			)
		)

	};
	return result;
}



std::map<std::string, host_function_signature_t> get_host_function_signatures(){
	const auto a = get_host_function_records();

	std::map<std::string, host_function_signature_t> result;
	for(const auto& e: a){
		result.insert({ e._name, { e._function_id, e._function_type } });
	}
	return result;
}

std::map<int, bc_host_function_t> get_host_functions(){
	const auto a = get_host_function_records();

	std::map<int, bc_host_function_t> result;
	for(const auto& e: a){
		const auto sign = host_function_signature_t{ e._function_id, e._function_type };
		result.insert(
			{ e._function_id, bc_host_function_t{ sign, e._name, e._f } }
		);
	}
	return result;
}


}
