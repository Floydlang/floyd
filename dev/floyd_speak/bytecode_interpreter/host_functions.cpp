//
//  host_functions.cpp
//  FloydSpeak
//
//  Created by Marcus Zetterquist on 2018-02-23.
//  Copyright © 2018 Marcus Zetterquist. All rights reserved.
//

#include "host_functions.h"

#include "json_support.h"
#include "json_parser.h"
#include "ast_typeid_helpers.h"

#include <cmath>
#include <sys/time.h>

#include <thread>
#include <chrono>
#include <algorithm>
#include <iostream>
#include <fstream>
#include "text_parser.h"

namespace floyd {

using std::vector;
using std::string;
using std::pair;
using std::shared_ptr;
using std::make_shared;

//??? Remove usage of value_t
value_t flatten_to_json(const value_t& value){
	const auto j = value_to_ast_json(value, json_tags::k_plain);
	value_t json_value = value_t::make_json_value(j._value);
	return json_value;
}






extern const std::string k_tiny_prefix = R"(
	let double cmath_pi = 3.14159265358979323846

	struct cpu_address_t {
		int address
	}

	struct size_t {
		int address
	}

	struct file_pos_t {
		int pos
	}

	struct time_ms_t {
		int pos
	}

	struct uuid_t {
		int high64
		int low64
	}


	struct ip_address_t {
		int high64
		int low_64_bits
	}


	struct url_t {
		string absolute_url
	}

	struct url_parts_t {
		string protocol
		string domain
		string path
		[string:string] query_strings
		int port
	}

	struct quick_hash_t {
		int hash
	}

	struct key_t {
		quick_hash_t hash
	}

	struct date_t {
		string utd_date
	}

	struct sha1_t {
		string ascii40
	}

	struct relative_path_t {
		string relative_path
	}

	struct absolute_path_t {
		string absolute_path
	}

	struct binary_t {
		string bytes
	}

	struct text_location_t {
		absolute_path_t source_file
		int line_number
		int pos_in_line
	}

	struct seq_t {
		string str
		size_t pos
	}

	struct text_t {
		binary_t data
	}

	struct text_resource_id {
		quick_hash_t id
	}

	struct image_id_t {
		int id
	}

	struct color_t {
		double red
		double green
		double blue
		double alpha
	}

	struct vector2_t {
		double x
		double y
	}



	let color__black = color_t(0.0, 0.0, 0.0, 1.0)
	let color__white = color_t(1.0, 1.0, 1.0, 1.0)


	func color_t add_colors(color_t a, color_t b){
		return color_t(
			a.red + b.red,
			a.green + b.green,
			a.blue + b.blue,
			a.alpha + b.alpha
		)
	}

)";










//??? removeusage of value_t
value_t unflatten_json_to_specific_type(const json_t& v, const typeid_t& target_type){
	QUARK_ASSERT(v.check_invariant());

	if(target_type.is_undefined()){
		throw std::runtime_error("Invalid json schema, found null - unsupported by Floyd.");
	}
	else if(target_type.is_bool()){
		if(v.is_true()){
			return value_t::make_bool(true);
		}
		else if(v.is_false()){
			return value_t::make_bool(false);
		}
		else{
			throw std::runtime_error("Invalid json schema, expected true or false.");
		}
	}
	else if(target_type.is_int()){
		if(v.is_number()){
			return value_t::make_int((int)v.get_number());
		}
		else{
			throw std::runtime_error("Invalid json schema, expected number.");
		}
	}
	else if(target_type.is_double()){
		if(v.is_number()){
			return value_t::make_double((double)v.get_number());
		}
		else{
			throw std::runtime_error("Invalid json schema, expected number.");
		}
	}
	else if(target_type.is_string()){
		if(v.is_string()){
			return value_t::make_string(v.get_string());
		}
		else{
			throw std::runtime_error("Invalid json schema, expected string.");
		}
	}
	else if(target_type.is_json_value()){
		return value_t::make_json_value(v);
	}
	else if(target_type.is_typeid()){
		const auto typeid_value = typeid_from_ast_json(ast_json_t({v}));
		return value_t::make_typeid_value(typeid_value);
	}
	else if(target_type.is_struct()){
		if(v.is_object()){
			const auto struct_def = target_type.get_struct();
			vector<value_t> members2;
			for(const auto& member: struct_def._members){
				const auto member_value0 = v.get_object_element(member._name);
				const auto member_value1 = unflatten_json_to_specific_type(member_value0, member._type);
				members2.push_back(member_value1);
			}
			const auto result = value_t::make_struct_value(target_type, members2);
			return result;
		}
		else{
			throw std::runtime_error("Invalid json schema for Floyd struct, expected JSON object.");
		}
	}

	else if(target_type.is_vector()){
		if(v.is_array()){
			const auto target_element_type = target_type.get_vector_element_type();
			vector<value_t> elements2;
			for(int i = 0 ; i < v.get_array_size() ; i++){
				const auto member_value0 = v.get_array_n(i);
				const auto member_value1 = unflatten_json_to_specific_type(member_value0, target_element_type);
				elements2.push_back(member_value1);
			}
			const auto result = value_t::make_vector_value(target_element_type, elements2);
			return result;
		}
		else{
			throw std::runtime_error("Invalid json schema for Floyd vector, expected JSON array.");
		}
	}
	else if(target_type.is_dict()){
		if(v.is_object()){
			const auto value_type = target_type.get_dict_value_type();
			const auto source_obj = v.get_object();
			std::map<std::string, value_t> obj2;
			for(const auto& member: source_obj){
				const auto member_name = member.first;
				const auto member_value0 = member.second;
				const auto member_value1 = unflatten_json_to_specific_type(member_value0, value_type);
				obj2[member_name] = member_value1;
			}
			const auto result = value_t::make_dict_value(value_type, obj2);
			return result;
		}
		else{
			throw std::runtime_error("Invalid json schema, expected JSON object.");
		}
	}
	else if(target_type.is_function()){
		throw std::runtime_error("Invalid json schema, cannot unflatten functions.");
	}
	else if(target_type.is_unresolved_type_identifier()){
		QUARK_ASSERT(false);
		throw std::exception();
//		throw std::runtime_error("Invalid json schema, cannot unflatten functions.");
	}
	else{
		QUARK_ASSERT(false);
		throw std::exception();
	}
}


//	Records all output to interpreter
bc_value_t host__print(interpreter_t& vm, const bc_value_t args[], int arg_count){
	QUARK_ASSERT(vm.check_invariant());

	if(arg_count != 1){
		throw std::runtime_error("assert() requires 1 argument!");
	}

	const auto& value = args[0];
	const auto s = to_compact_string2(bc_to_value(value));
//	printf("%s\n", s.c_str());
	vm._print_output.push_back(s);

	return bc_value_t::make_undefined();
}
bc_value_t host__send(interpreter_t& vm, const bc_value_t args[], int arg_count){
	QUARK_ASSERT(vm.check_invariant());

	if(arg_count != 2){
		throw std::runtime_error("send() requires 2 arguments!");
	}

	const auto& process_id = args[0].get_string_value();
	const auto& message_json = args[1].get_json_value();

	QUARK_TRACE_SS("send(\"" << process_id << "\"," << json_to_pretty_string(message_json) <<")");


	vm._handler->on_send(process_id, message_json);

	return bc_value_t::make_undefined();
}

bc_value_t host__assert(interpreter_t& vm, const bc_value_t args[], int arg_count){
	QUARK_ASSERT(vm.check_invariant());

	if(arg_count != 1){
		throw std::runtime_error("assert() requires 1 argument!");
	}

	const auto& value = args[0];
	if(value._type.is_bool() == false){
		throw std::runtime_error("First argument to assert() must be of type bool.");
	}
	bool ok = value.get_bool_value();
	if(!ok){
		vm._print_output.push_back("Assertion failed.");
		throw std::runtime_error("Floyd assertion failed.");
	}
	return bc_value_t::make_undefined();
}

//	string to_string(value_t)
bc_value_t host__to_string(interpreter_t& vm, const bc_value_t args[], int arg_count){
	QUARK_ASSERT(vm.check_invariant());

	if(arg_count != 1){
		throw std::runtime_error("to_string() requires 1 argument!");
	}

	const auto& value = args[0];
	const auto a = to_compact_string2(bc_to_value(value));
	return bc_value_t::make_string(a);
}
bc_value_t host__to_pretty_string(interpreter_t& vm, const bc_value_t args[], int arg_count){
	QUARK_ASSERT(vm.check_invariant());

	if(arg_count != 1){
		throw std::runtime_error("to_pretty_string() requires 1 argument!");
	}

	const auto& value = args[0];
	const auto json = bcvalue_to_json(value);
	const auto s = json_to_pretty_string(json, 0, pretty_t{80, 4});
	return bc_value_t::make_string(s);
}

bc_value_t host__typeof(interpreter_t& vm, const bc_value_t args[], int arg_count){
	QUARK_ASSERT(vm.check_invariant());

	if(arg_count != 1){
		throw std::runtime_error("typeof() requires 1 argument!");
	}

	const auto& value = args[0];
	const auto type = value._type;
	const auto result = value_t::make_typeid_value(type);
	return value_to_bc(result);
}

bc_value_t host__get_time_of_day(interpreter_t& vm, const bc_value_t args[], int arg_count){
	QUARK_ASSERT(vm.check_invariant());

	if(arg_count != 0){
		throw std::runtime_error("get_time_of_day() requires 0 arguments!");
	}

	std::chrono::time_point<std::chrono::high_resolution_clock> t = std::chrono::high_resolution_clock::now();
	std::chrono::duration<double> elapsed_seconds = t - vm._imm->_start_time;
	const auto ms = elapsed_seconds.count() * 1000.0;
	const auto result = value_t::make_int(int(ms));
	return value_to_bc(result);
}

QUARK_UNIT_TESTQ("sizeof(int)", ""){
	QUARK_TRACE(std::to_string(sizeof(int)));
	QUARK_TRACE(std::to_string(sizeof(int64_t)));
}

QUARK_UNIT_TESTQ("get_time_of_day_ms()", ""){
	const auto a = std::chrono::high_resolution_clock::now();
    std::this_thread::sleep_for(std::chrono::milliseconds(7));
	const auto b = std::chrono::high_resolution_clock::now();

	std::chrono::duration<double> elapsed_seconds = b - a;
	const int ms = static_cast<int>((static_cast<double>(elapsed_seconds.count()) * 1000.0));

	QUARK_UT_VERIFY(ms >= 7)
}


bc_value_t host__update(interpreter_t& vm, const bc_value_t args[], int arg_count){
	QUARK_ASSERT(vm.check_invariant());

	QUARK_TRACE(json_to_pretty_string(interpreter_to_json(vm)));

	const auto& obj1 = args[0];
	const auto& lookup_key = args[1];
	const auto& new_value = args[2];

	if(arg_count != 3){
		throw std::runtime_error("update() needs 3 arguments.");
	}
	else{
		return update_element(vm, obj1, lookup_key, new_value);
	}
}

bc_value_t host__size(interpreter_t& vm, const bc_value_t args[], int arg_count){
	QUARK_ASSERT(vm.check_invariant());

		QUARK_ASSERT(false);

	if(arg_count != 1){
		throw std::runtime_error("find requires 2 arguments");
	}

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
			throw std::runtime_error("Calling size() on unsupported type of value.");
		}
	}
	else if(obj._type.is_vector()){
		if(encode_as_vector_pod64(obj._type)){
			const auto size = obj._pod._ext->_vector_pod64.size();
			return bc_value_t::make_int(static_cast<int>(size));
		}
		else{
			const auto size = get_vector_value(obj)->size();
			return bc_value_t::make_int(static_cast<int>(size));
		}
	}
	else if(obj._type.is_dict()){
		if(encode_as_dict_pod64(obj._type)){
			const auto size = obj._pod._ext->_dict_pod64.size();
			return bc_value_t::make_int(static_cast<int>(size));
		}
		else{
			const auto size = get_dict_value(obj).size();
			return bc_value_t::make_int(static_cast<int>(size));
		}
	}
	else{
		throw std::runtime_error("Calling size() on unsupported type of value.");
	}
}

bc_value_t host__find(interpreter_t& vm, const bc_value_t args[], int arg_count){
	QUARK_ASSERT(vm.check_invariant());

	if(arg_count != 2){
		throw std::runtime_error("find requires 2 arguments");
	}

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
			throw std::runtime_error("Type mismatch.");
		}
		else if(obj._type.get_vector_element_type().is_bool()){
			const auto& vec = obj._pod._ext->_vector_pod64;
			int index = 0;
			const auto size = vec.size();
			while(index < size && vec[index]._bool != wanted._pod._pod64._bool){
				index++;
			}
			int result = index == size ? -1 : static_cast<int>(index);
			return bc_value_t::make_int(result);
		}
		else if(obj._type.get_vector_element_type().is_int()){
			const auto& vec = obj._pod._ext->_vector_pod64;
			int index = 0;
			const auto size = vec.size();
			while(index < size && vec[index]._int64 != wanted._pod._pod64._int64){
				index++;
			}
			int result = index == size ? -1 : static_cast<int>(index);
			return bc_value_t::make_int(result);
		}
		else if(obj._type.get_vector_element_type().is_double()){
			const auto& vec = obj._pod._ext->_vector_pod64;
			int index = 0;
			const auto size = vec.size();
			while(index < size && vec[index]._double != wanted._pod._pod64._double){
				index++;
			}
			int result = index == size ? -1 : static_cast<int>(index);
			return bc_value_t::make_int(result);
		}
		else{
			const auto& vec = *get_vector_value(obj);
			const auto size = vec.size();
			int index = 0;
			while(index < size && bc_compare_value_exts(vec[index], bc_object_handle_t(wanted), element_type) != 0){
				index++;
			}
			int result = index == size ? -1 : static_cast<int>(index);
			return bc_value_t::make_int(result);
		}
	}
	else{
		throw std::runtime_error("Calling find() on unsupported type of value.");
	}
}

bc_value_t host__exists(interpreter_t& vm, const bc_value_t args[], int arg_count){
	QUARK_ASSERT(vm.check_invariant());

	if(arg_count != 2){
		throw std::runtime_error("find requires 2 arguments");
	}

	const auto obj = args[0];
	const auto key = args[1];

	if(obj._type.is_dict()){
		if(key._type.is_string() == false){
			throw std::runtime_error("Key must be string.");
		}
		const auto key_string = key.get_string_value();

		if(encode_as_dict_pod64(obj._type)){
			const auto found_ptr = obj._pod._ext->_dict_pod64.find(key_string);
			return bc_value_t::make_bool(found_ptr != nullptr);
		}
		else{
			const auto entries = get_dict_value(obj);
			const auto found_ptr = entries.find(key_string);
			return bc_value_t::make_bool(found_ptr != nullptr);
		}
	}
	else{
		throw std::runtime_error("Calling exist() on unsupported type of value.");
	}
}

bc_value_t host__erase(interpreter_t& vm, const bc_value_t args[], int arg_count){
	QUARK_ASSERT(vm.check_invariant());

	if(arg_count != 2){
		throw std::runtime_error("find requires 2 arguments");
	}

	const auto obj = args[0];
	const auto key = args[1];

	if(obj._type.is_dict()){
		if(key._type.is_string() == false){
			throw std::runtime_error("Key must be string.");
		}
		const auto key_string = key.get_string_value();

		const auto value_type = obj._type.get_dict_value_type();
		if(encode_as_dict_pod64(obj._type)){
			auto entries2 = obj._pod._ext->_dict_pod64.erase(key_string);
			const auto value2 = make_dict_value(value_type, entries2);
			return value2;
		}
		else{
			auto entries2 = get_dict_value(obj);
			entries2 = entries2.erase(key_string);
			const auto value2 = make_dict_value(value_type, entries2);
			return value2;
		}
	}
	else{
		throw std::runtime_error("Calling exist() on unsupported type of value.");
	}
}


//	assert(push_back(["one","two"], "three") == ["one","two","three"])
bc_value_t host__push_back(interpreter_t& vm, const bc_value_t args[], int arg_count){
	QUARK_ASSERT(vm.check_invariant());

	QUARK_ASSERT(false);
	if(arg_count != 2){
		throw std::runtime_error("find requires 2 arguments");
	}

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
			throw std::runtime_error("Type mismatch.");
		}
		else if(encode_as_vector_pod64(obj._type)){
			auto elements2 = obj._pod._ext->_vector_pod64.push_back(element._pod._pod64);
			const auto v = make_vector_int64_value(element_type, elements2);
			return v;
		}
		else{
			const auto vec = *get_vector_value(obj);
			auto elements2 = vec.push_back(bc_object_handle_t(element));
			const auto v = make_vector_value(element_type, elements2);
			return v;
		}
	}
	else{
		throw std::runtime_error("Calling push_back() on unsupported type of value.");
	}
}

//	assert(subset("abc", 1, 3) == "bc");
bc_value_t host__subset(interpreter_t& vm, const bc_value_t args[], int arg_count){
	QUARK_ASSERT(vm.check_invariant());

	if(arg_count != 3){
		throw std::runtime_error("subset() requires 3 arguments");
	}

	const auto obj = args[0];

	if(args[1]._type.is_int() == false || args[2]._type.is_int() == false){
		throw std::runtime_error("subset() requires start and end to be integers.");
	}

	const auto start = args[1].get_int_value();
	const auto end = args[2].get_int_value();
	if(start < 0 || end < 0){
		throw std::runtime_error("subset() requires start and end to be non-negative.");
	}

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
		if(encode_as_vector_pod64(obj._type)){
			const auto& element_type = obj._type.get_vector_element_type();
			const auto& vec = obj._pod._ext->_vector_pod64;
			const auto start2 = std::min(start, static_cast<int64_t>(vec.size()));
			const auto end2 = std::min(end, static_cast<int64_t>(vec.size()));
			immer::vector<bc_pod64_t> elements2;
			for(auto i = start2 ; i < end2 ; i++){
				elements2 = elements2.push_back(vec[i]);
			}
			const auto v = make_vector_int64_value(element_type, elements2);
			return v;
		}
		else{
			const auto vec = *get_vector_value(obj);
			const auto element_type = obj._type.get_vector_element_type();
			const auto start2 = std::min(start, static_cast<int64_t>(vec.size()));
			const auto end2 = std::min(end, static_cast<int64_t>(vec.size()));
			immer::vector<bc_object_handle_t> elements2;
			for(auto i = start2 ; i < end2 ; i++){
				elements2 = elements2.push_back(vec[i]);
			}
			const auto v = make_vector_value(element_type, elements2);
			return v;
		}
	}
	else{
		throw std::runtime_error("Calling push_back() on unsupported type of value.");
	}
}


//	assert(replace("One ring to rule them all", 4, 7, "rabbit") == "One rabbit to rule them all");
bc_value_t host__replace(interpreter_t& vm, const bc_value_t args[], int arg_count){
	QUARK_ASSERT(vm.check_invariant());

	if(arg_count != 4){
		throw std::runtime_error("replace() requires 4 arguments");
	}

	const auto obj = args[0];

	if(args[1]._type.is_int() == false || args[2]._type.is_int() == false){
		throw std::runtime_error("replace() requires start and end to be integers.");
	}

	const auto start = args[1].get_int_value();
	const auto end = args[2].get_int_value();
	if(start < 0 || end < 0){
		throw std::runtime_error("replace() requires start and end to be non-negative.");
	}
	if(args[3]._type != args[0]._type){
		throw std::runtime_error("replace() requires 4th arg to be same as argument 0.");
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
		if(encode_as_vector_pod64(obj._type)){
			const auto& vec = obj._pod._ext->_vector_pod64;
			const auto element_type = obj._type.get_vector_element_type();
			const auto start2 = std::min(start, static_cast<int64_t>(vec.size()));
			const auto end2 = std::min(end, static_cast<int64_t>(vec.size()));
			const auto& new_bits = args[3]._pod._ext->_vector_pod64;

			auto result = immer::vector<bc_pod64_t>(vec.begin(), vec.begin() + start2);
			for(int i = 0 ; i < new_bits.size() ; i++){
				result = result.push_back(new_bits[i]);
			}
			for(int i = 0 ; i < (vec.size( ) - end2) ; i++){
				result = result.push_back(vec[end2 + i]);
			}
			const auto v = make_vector_int64_value(element_type, result);
			return v;
		}
		else{
			const auto& vec = *get_vector_value(obj);
			const auto element_type = obj._type.get_vector_element_type();
			const auto start2 = std::min(start, static_cast<int64_t>(vec.size()));
			const auto end2 = std::min(end, static_cast<int64_t>(vec.size()));
			const auto& new_bits = *get_vector_value(args[3]);

			auto result = immer::vector<bc_object_handle_t>(vec.begin(), vec.begin() + start2);
			for(int i = 0 ; i < new_bits.size() ; i++){
				result = result.push_back(new_bits[i]);
			}
			for(int i = 0 ; i < (vec.size( ) - end2) ; i++){
				result = result.push_back(vec[end2 + i]);
			}
			const auto v = make_vector_value(element_type, result);
			return v;
		}
	}
	else{
		throw std::runtime_error("Calling replace() on unsupported type of value.");
	}
}

bc_value_t host__get_env_path(interpreter_t& vm, const bc_value_t args[], int arg_count){
	QUARK_ASSERT(vm.check_invariant());

	if(arg_count != 0){
		throw std::runtime_error("get_env_path() requires 0 arguments!");
	}

    const char *homeDir = getenv("HOME");
    const std::string env_path(homeDir);
//	const std::string env_path = "~/Desktop/";

	const auto v = bc_value_t::make_string(env_path);
	return v;
}

bc_value_t host__read_text_file(interpreter_t& vm, const bc_value_t args[], int arg_count){
	QUARK_ASSERT(vm.check_invariant());

	if(arg_count != 1){
		throw std::runtime_error("read_text_file() requires 1 arguments!");
	}
	if(args[0]._type.is_string() == false){
		throw std::runtime_error("read_text_file() requires a file path as a string.");
	}

	const string source_path = args[0].get_string_value();
	std::string file_contents;
	{
		std::ifstream f (source_path);
		if (f.is_open() == false){
			throw std::runtime_error("Cannot read source file.");
		}
		std::string line;
		while ( getline(f, line) ) {
			file_contents.append(line + "\n");
		}
		f.close();
	}
	const auto v = bc_value_t::make_string(file_contents);
	return v;
}

bc_value_t host__write_text_file(interpreter_t& vm, const bc_value_t args[], int arg_count){
	QUARK_ASSERT(vm.check_invariant());

	if(arg_count != 2){
		throw std::runtime_error("write_text_file() requires 2 arguments!");
	}
	else if(args[0]._type.is_string() == false){
		throw std::runtime_error("write_text_file() requires a file path as a string.");
	}
	else if(args[1]._type.is_string() == false){
		throw std::runtime_error("write_text_file() requires a file path as a string.");
	}
	else{
		const string path = args[0].get_string_value();
		const string file_contents = args[1].get_string_value();

		std::ofstream outputFile;
		outputFile.open(path);
		outputFile << file_contents;
		outputFile.close();
		return bc_value_t();
	}
}

/*
	Reads json from a text string, returning an unpacked json_value.
*/
bc_value_t host__decode_json(interpreter_t& vm, const bc_value_t args[], int arg_count){
	QUARK_ASSERT(vm.check_invariant());

	if(arg_count != 1){
		throw std::runtime_error("decode_json_value() requires 1 argument!");
	}
	else if(args[0]._type.is_string() == false){
		throw std::runtime_error("decode_json_value() requires string argument.");
	}
	else{
		const string s = args[0].get_string_value();
		std::pair<json_t, seq_t> result = parse_json(seq_t(s));
		const auto json_value = bc_value_t::make_json_value(result.first);
		return json_value;
	}
}

bc_value_t host__encode_json(interpreter_t& vm, const bc_value_t args[], int arg_count){
	QUARK_ASSERT(vm.check_invariant());

	if(arg_count != 1){
		throw std::runtime_error("encode_json() requires 1 argument!");
	}
	else if(args[0]._type.is_json_value()== false){
		throw std::runtime_error("encode_json() requires argument to be json_value.");
	}
	else{
		const auto value0 = args[0].get_json_value();
		const string s = json_to_compact_string(value0);
		return bc_value_t::make_string(s);
	}
}


bc_value_t host__flatten_to_json(interpreter_t& vm, const bc_value_t args[], int arg_count){
	QUARK_ASSERT(vm.check_invariant());

	if(arg_count != 1){
		throw std::runtime_error("flatten_to_json() requires 1 argument!");
	}
	else{
		const auto value = args[0];
		const auto value2 = bc_to_value(args[0]);
		const auto result = flatten_to_json(value2);
		return value_to_bc(result);
	}
}

bc_value_t host__unflatten_from_json(interpreter_t& vm, const bc_value_t args[], int arg_count){
	QUARK_ASSERT(vm.check_invariant());

	if(arg_count != 2){
		throw std::runtime_error("unflatten_from_json() requires 2 argument!");
	}
	else if(args[0]._type.is_json_value() == false){
		throw std::runtime_error("unflatten_from_json() requires string argument.");
	}
	else if(args[1]._type.is_typeid()== false){
		throw std::runtime_error("unflatten_from_json() requires string argument.");
	}
	else{
		const auto json_value = args[0].get_json_value();
		const auto target_type = args[1].get_typeid_value();
		const auto result = unflatten_json_to_specific_type(json_value, target_type);
		return value_to_bc(result);
	}
}

bc_value_t host__get_json_type(interpreter_t& vm, const bc_value_t args[], int arg_count){
	QUARK_ASSERT(vm.check_invariant());

	if(arg_count != 1){
		throw std::runtime_error("get_json_type() requires 1 argument!");
	}
	else if(args[0]._type.is_json_value() == false){
		throw std::runtime_error("get_json_type() requires json_value argument.");
	}
	else{
		const auto json_value = args[0].get_json_value();
		if(json_value.is_object()){
			return bc_value_t::make_int(1);
		}
		else if(json_value.is_array()){
			return bc_value_t::make_int(2);
		}
		else if(json_value.is_string()){
			return bc_value_t::make_int(3);
		}
		else if(json_value.is_number()){
			return bc_value_t::make_int(4);
		}
		else if(json_value.is_true()){
			return bc_value_t::make_int(5);
		}
		else if(json_value.is_false()){
			return bc_value_t::make_int(6);
		}
		else if(json_value.is_null()){
			return bc_value_t::make_int(7);
		}
		else{
			QUARK_ASSERT(false);
			throw std::exception();
		}
	}
}

std::map<std::string, host_function_signature_t> get_host_function_signatures(){
	const auto VOID = typeid_t::make_void();
	const auto DYN = typeid_t::make_internal_dynamic();

	const std::map<std::string, host_function_signature_t> temp {
		{ "print", host_function_signature_t{ 1000, typeid_t::make_function(VOID, { DYN }, epure::pure) } },

		{ "assert", host_function_signature_t{ 1001, typeid_t::make_function(VOID, { DYN }, epure::pure) } },
		{ "to_string", host_function_signature_t{ 1002, typeid_t::make_function(typeid_t::make_string(), { DYN }, epure::pure) } },
		{ "to_pretty_string", host_function_signature_t{ 1003, typeid_t::make_function(typeid_t::make_string(), { DYN }, epure::pure) }},
		{ "typeof", host_function_signature_t{1004, typeid_t::make_function(typeid_t::make_typeid(), { DYN }, epure::pure) } },

		{ "update", host_function_signature_t{ 1006, typeid_t::make_function(DYN, { DYN, DYN, DYN }, epure::pure) }},
		{ "size", host_function_signature_t{ 1007, typeid_t::make_function(typeid_t::make_int(), { DYN }, epure::pure) } },
		{ "find", host_function_signature_t{ 1008, typeid_t::make_function(typeid_t::make_int(), { DYN, DYN }, epure::pure) }},
		{ "exists", host_function_signature_t{ 1009, typeid_t::make_function(typeid_t::make_bool(), { DYN, DYN }, epure::pure) }},
		{ "erase", host_function_signature_t{ 1010, typeid_t::make_function(DYN, { DYN, DYN }, epure::pure) } },
		{ "push_back", host_function_signature_t{ 1011, typeid_t::make_function(DYN, { DYN, DYN }, epure::pure) }},
		{ "subset", host_function_signature_t{ 1012, typeid_t::make_function(DYN, { DYN, DYN, DYN}, epure::pure) }},
		{ "replace", host_function_signature_t{ 1013, typeid_t::make_function(DYN, { DYN, DYN, DYN, DYN }, epure::pure) }},


		{ "decode_json", host_function_signature_t{ 1017, typeid_t::make_function(typeid_t::make_json_value(), {typeid_t::make_string()}, epure::pure) }},
		{ "encode_json", host_function_signature_t{ 1018, typeid_t::make_function(typeid_t::make_string(), {typeid_t::make_json_value()}, epure::pure) }},
		{ "flatten_to_json", host_function_signature_t{ 1019, typeid_t::make_function(typeid_t::make_json_value(), { DYN }, epure::pure) }},
		{ "unflatten_from_json", host_function_signature_t{ 1020, typeid_t::make_function(DYN, { typeid_t::make_json_value(), typeid_t::make_typeid() }, epure::pure) }},
		{ "get_json_type", host_function_signature_t{ 1021, typeid_t::make_function(typeid_t::make_int(), {typeid_t::make_json_value()}, epure::pure) }},

		{ "send", host_function_signature_t{ 1022, typeid_t::make_function(VOID, { typeid_t::make_string(), typeid_t::make_json_value() }, epure::impure) } },
		{ "get_time_of_day", host_function_signature_t{ 1005, typeid_t::make_function(typeid_t::make_int(), {}, epure::impure) }},
		{ "get_env_path", host_function_signature_t{ 1014, typeid_t::make_function(typeid_t::make_string(), {}, epure::impure) }},
		{ "read_text_file", host_function_signature_t{ 1015, typeid_t::make_function(typeid_t::make_string(), { DYN }, epure::impure) }},
		{ "write_text_file", host_function_signature_t{ 1016, typeid_t::make_function(VOID, { DYN, DYN }, epure::impure) }}
	};
	return temp;
}


std::map<int,  host_function_t> get_host_functions(){
	const auto host_functions = get_host_function_signatures();

	const std::map<string, HOST_FUNCTION_PTR> lookup0 = {
		{ "print", host__print },
		{ "assert", host__assert },
		{ "to_string", host__to_string },
		{ "to_pretty_string", host__to_pretty_string },
		{ "typeof", host__typeof },

		{ "update", host__update },
		{ "size", host__size },
		{ "find", host__find },
		{ "exists", host__exists },
		{ "erase", host__erase },
		{ "push_back", host__push_back },
		{ "subset", host__subset },
		{ "replace", host__replace },

		{ "decode_json", host__decode_json },
		{ "encode_json", host__encode_json },
		{ "flatten_to_json", host__flatten_to_json },
		{ "unflatten_from_json", host__unflatten_from_json },
		{ "get_json_type", host__get_json_type },

		{ "send", host__send },
		{ "get_time_of_day", host__get_time_of_day },
		{ "get_env_path", host__get_env_path },
		{ "read_text_file", host__read_text_file },
		{ "write_text_file", host__write_text_file }
	};

	const auto lookup = [&](){
		std::map<int,  host_function_t> result;
		for(const auto& e: lookup0){
			const auto function_name = e.first;
			const auto f_ptr = e.second;
			const auto hf = host_functions.at(function_name);
			const host_function_t hf2 = { hf, function_name, f_ptr };
			result.insert({ hf._function_id, hf2});
		}
		return result;
	}();

	return lookup;
}


}
