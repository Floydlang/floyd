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
#include "FileHandling.h"
#include "sha1_class.h"



namespace floyd {

using std::vector;
using std::string;
using std::pair;
using std::shared_ptr;
using std::make_shared;

//??? Remove usage of value_t
value_t value_to_jsonvalue(const value_t& value){
	const auto j = value_to_ast_json(value, json_tags::k_plain);
	value_t json_value = value_t::make_json_value(j._value);
	return json_value;
}






extern const std::string k_builtin_types_and_constants = R"(
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
		string utc_date
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


	////////////////////////////		FILE SYSTEM TYPES


	struct fsentry_t {
		string type	//	"dir" or "file"
		string abs_parent_path
		string name
	}

	struct fsentry_info_t {
		string type	//	"file" or "dir"
		string name
		string abs_parent_path

		string creation_date
		string modification_date
		int file_size
	}

	struct fs_environment_t {
		string home_dir
		string documents_dir
		string desktop_dir

		string hidden_persistence_dir
		string preferences_dir
		string cache_dir
		string temp_dir

		string executable_dir
	}
)";



/////////////////////////////////////////		STRUCT PACKERS



typeid_t make__fs_environment_t__type(){
	const auto temp = typeid_t::make_struct2({
		{ typeid_t::make_string(), "home_dir" },
		{ typeid_t::make_string(), "documents_dir" },
		{ typeid_t::make_string(), "desktop_dir" },

		{ typeid_t::make_string(), "hidden_persistence_dir" },
		{ typeid_t::make_string(), "preferences_dir" },
		{ typeid_t::make_string(), "cache_dir" },
		{ typeid_t::make_string(), "temp_dir" },

		{ typeid_t::make_string(), "executable_dir" }
	});
	return temp;
}

typeid_t make__fsentry_t__type(){
	const auto temp = typeid_t::make_struct2({
		{ typeid_t::make_string(), "type" },
		{ typeid_t::make_string(), "name" },
		{ typeid_t::make_string(), "parent_path" }
	});
	return temp;
}

/*
	struct date_t {
		string utd_date
	}
*/
typeid_t make__date_t__type(){
	const auto temp = typeid_t::make_struct2({
		{ typeid_t::make_string(), "utd_date" }
	});
	return temp;
}

/*
	struct sha1_t {
		string ascii40
	}
*/
typeid_t make__sha1_t__type(){
	const auto temp = typeid_t::make_struct2({
		{ typeid_t::make_string(), "ascii40" }
	});
	return temp;
}

/*
	struct binary_t {
		string bytes
	}
*/
typeid_t make__binary_t__type(){
	const auto temp = typeid_t::make_struct2({
		{ typeid_t::make_string(), "bytes" }
	});
	return temp;
}

/*
	struct absolute_path_t {
		string absolute_path
	}
*/
typeid_t make__absolute_path_t__type(){
	const auto temp = typeid_t::make_struct2({
		{ typeid_t::make_string(), "absolute_path" }
	});
	return temp;
}

/*
	struct file_pos_t {
		int pos
	}
*/
typeid_t make__file_pos_t__type(){
	const auto temp = typeid_t::make_struct2({
		{ typeid_t::make_int(), "pos" }
	});
	return temp;
}

/*
	struct fsentry_info_t {
		string type	//	"file" or "dir"
		string name
		absolute_path_t parent_path

		date_t creation_date
		date_t modification_date
		file_pos_t file_size
	}
*/
typeid_t make__fsentry_info_t__type(){
	const auto temp = typeid_t::make_struct2({
		{ typeid_t::make_string(), "type" },
		{ typeid_t::make_string(), "name" },
		{ typeid_t::make_string(), "parent_path" },

		{ typeid_t::make_string(), "creation_date" },
		{ typeid_t::make_string(), "modification_date" },
		{ typeid_t::make_int(), "file_size" }
	});
	return temp;
}












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
/*
	Reads json from a text string, returning an unpacked json_value.
*/
bc_value_t host__script_to_jsonvalue(interpreter_t& vm, const bc_value_t args[], int arg_count){
	QUARK_ASSERT(vm.check_invariant());

	if(arg_count != 1){
		throw std::runtime_error("script_to_jsonvalue_value() requires 1 argument!");
	}
	else if(args[0]._type.is_string() == false){
		throw std::runtime_error("script_to_jsonvalue_value() requires string argument.");
	}
	else{
		const string s = args[0].get_string_value();
		std::pair<json_t, seq_t> result = parse_json(seq_t(s));
		const auto json_value = bc_value_t::make_json_value(result.first);
		return json_value;
	}
}

bc_value_t host__jsonvalue_to_script(interpreter_t& vm, const bc_value_t args[], int arg_count){
	QUARK_ASSERT(vm.check_invariant());

	if(arg_count != 1){
		throw std::runtime_error("jsonvalue_to_script() requires 1 argument!");
	}
	else if(args[0]._type.is_json_value()== false){
		throw std::runtime_error("jsonvalue_to_script() requires argument to be json_value.");
	}
	else{
		const auto value0 = args[0].get_json_value();
		const string s = json_to_compact_string(value0);
		return bc_value_t::make_string(s);
	}
}


bc_value_t host__value_to_jsonvalue(interpreter_t& vm, const bc_value_t args[], int arg_count){
	QUARK_ASSERT(vm.check_invariant());

	if(arg_count != 1){
		throw std::runtime_error("value_to_jsonvalue() requires 1 argument!");
	}
	else{
		const auto value = args[0];
		const auto value2 = bc_to_value(args[0]);
		const auto result = value_to_jsonvalue(value2);
		return value_to_bc(result);
	}
}

bc_value_t host__jsonvalue_to_value(interpreter_t& vm, const bc_value_t args[], int arg_count){
	QUARK_ASSERT(vm.check_invariant());

	if(arg_count != 2){
		throw std::runtime_error("jsonvalue_to_value() requires 2 argument!");
	}
	else if(args[0]._type.is_json_value() == false){
		throw std::runtime_error("jsonvalue_to_value() requires string argument.");
	}
	else if(args[1]._type.is_typeid()== false){
		throw std::runtime_error("jsonvalue_to_value() requires string argument.");
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

	if(arg_count != 1){
		throw std::runtime_error("calc_string_sha1() requires 1 arguments!");
	}
	if(args[0]._type.is_string() == false){
		throw std::runtime_error("calc_string_sha1() requires a string.");
	}

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
	QUARK_TRACE(json_to_pretty_string(debug._value));
#endif

	const auto v = value_to_bc(result);
	return v;
}



bc_value_t host__calc_binary_sha1(interpreter_t& vm, const bc_value_t args[], int arg_count){
	QUARK_ASSERT(vm.check_invariant());

	if(arg_count != 1){
		throw std::runtime_error("calc_string_sha1() requires 1 arguments!");
	}
	if((args[0]._type == make__binary_t__type()) == false){
		throw std::runtime_error("calc_string_sha1() requires a sha1_t.");
	}

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
	QUARK_TRACE(json_to_pretty_string(debug._value));
#endif

	const auto v = value_to_bc(result);
	return v;
}

/////////////////////////////////////////		PURE -- FUNCTIONAL

//	[R] map([E], R f(E e))
//??? need to provice context property to map() and pass to f().
bc_value_t host__map(interpreter_t& vm, const bc_value_t args[], int arg_count){
	QUARK_ASSERT(vm.check_invariant());

	if(arg_count != 2){
		throw std::runtime_error("map() requires 2 arguments");
	}

	if(args[0]._type.is_vector() == false){
		throw std::runtime_error("map() arg 1 must be a vector.");
	}
	const auto& collection = args[0];
	const auto collection_element_type = args[0]._type.get_vector_element_type();

	if(args[1]._type.is_function() == false){
		throw std::runtime_error("map() requires start and end to be integers.");
	}
	const auto f = args[1];
	const auto f_arg_types = f._type.get_function_args();
	const auto f_return_type = f._type.get_function_return();

	if(f_arg_types.size() != 1){
		throw std::runtime_error("map() function f requries 1 argument.");
	}

	if(f_arg_types[0] != collection_element_type){
		throw std::runtime_error("map() function f must accept collection elements as its argument.");
	}

	const auto r_type = f_return_type;

	immer::vector<bc_value_t> vec2;
	if(is_encoded_as_ext(collection_element_type) == false){
		for(const auto& e: collection._pod._ext->_vector_pod64){
			const auto e2 = bc_value_t(collection_element_type, e);
			const bc_value_t f_args[1] = { e2 };
			const auto result1 = call_function_bc(vm, f, f_args, 1);
			vec2 = vec2.push_back(result1);
		}
	}
	else{
		QUARK_ASSERT(false);
	}

	const auto result = make_vector(f_return_type, vec2);

#if 1
	const auto debug = value_and_type_to_ast_json(bc_to_value(result));
	QUARK_TRACE(json_to_pretty_string(debug._value));
#endif

	return result;
}


/////////////////////////////////////////		IMPURE -- MISC




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





/////////////////////////////////////////		IMPURE -- FILE SYSTEM




bc_value_t host__read_text_file(interpreter_t& vm, const bc_value_t args[], int arg_count){
	QUARK_ASSERT(vm.check_invariant());

	if(arg_count != 1){
		throw std::runtime_error("read_text_file() requires 1 arguments!");
	}
	if(args[0]._type.is_string() == false){
		throw std::runtime_error("read_text_file() requires a file path as a string.");
	}

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
		throw std::exception();
	}

	outputFile << data;
	outputFile.close();
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

		write_text_file(path, file_contents);

		return bc_value_t();
	}
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



	//??? check path is valid dir
bool is_valid_absolute_dir_path(const std::string& s){
	if(s.empty()){
		return false;
	}
	else{
/*
		if(s.back() != '/'){
			return false;
		}
*/

	}
	return true;
}

//??? use absolute_path_t as argument!
bc_value_t host__get_fsentries_shallow(interpreter_t& vm, const bc_value_t args[], int arg_count){
	QUARK_ASSERT(vm.check_invariant());

	if(arg_count != 1){
		throw std::runtime_error("get_fsentries_shallow() requires 1 arguments!");
	}
	if(args[0]._type.is_string() == false){
		throw std::runtime_error("get_fsentries_shallow() requires a path, as a string.");
	}

	const string path = args[0].get_string_value();
	if(is_valid_absolute_dir_path(path) == false){
		throw std::runtime_error("get_fsentries_shallow() illegal input path.");
	}

	const auto a = GetDirItems(path);
	const auto elements = directory_entries_to_values(a);
	const auto k_fsentry_t__type = make__fsentry_t__type();
	const auto vec2 = value_t::make_vector_value(k_fsentry_t__type, elements);

#if 1
	const auto debug = value_and_type_to_ast_json(vec2);
	QUARK_TRACE(json_to_pretty_string(debug._value));
#endif

	const auto v = value_to_bc(vec2);

	return v;
}

bc_value_t host__get_fsentries_deep(interpreter_t& vm, const bc_value_t args[], int arg_count){
	QUARK_ASSERT(vm.check_invariant());

	if(arg_count != 1){
		throw std::runtime_error("get_fsentries_deep() requires 1 arguments!");
	}
	if(args[0]._type.is_string() == false){
		throw std::runtime_error("get_fsentries_deep() requires a path, as a string.");
	}

	const string path = args[0].get_string_value();
	if(is_valid_absolute_dir_path(path) == false){
		throw std::runtime_error("get_fsentries_shallow() illegal input path.");
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


bc_value_t host__get_fsentry_info(interpreter_t& vm, const bc_value_t args[], int arg_count){
	QUARK_ASSERT(vm.check_invariant());

	if(arg_count != 1){
		throw std::runtime_error("get_fsentry_info() requires 1 arguments!");
	}
	if(args[0]._type.is_string() == false){
		throw std::runtime_error("get_fsentry_info() requires a path, as a string.");
	}

	const string path = args[0].get_string_value();
	if(is_valid_absolute_dir_path(path) == false){
		throw std::runtime_error("get_fsentry_info() illegal input path.");
	}


	TFileInfo info;
	bool ok = GetFileInfo(path, info);
	QUARK_ASSERT(ok);
	if(ok == false){
		throw std::exception();
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
	QUARK_TRACE(json_to_pretty_string(debug._value));
#endif

	const auto v = value_to_bc(result);
	return v;
}


bc_value_t host__get_fs_environment(interpreter_t& vm, const bc_value_t args[], int arg_count){
	QUARK_ASSERT(vm.check_invariant());

	if(arg_count != 0){
		throw std::runtime_error("get_fs_environment() requires 0 arguments!");
	}
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
	QUARK_TRACE(json_to_pretty_string(debug._value));
#endif

	const auto v = value_to_bc(result);
	return v;
}

//??? refactor common code.

bc_value_t host__does_fsentry_exist(interpreter_t& vm, const bc_value_t args[], int arg_count){
	QUARK_ASSERT(vm.check_invariant());

	if(arg_count != 1){
		throw std::runtime_error("does_fsentry_exist() requires 1 arguments!");
	}
	if(args[0]._type.is_string() == false){
		throw std::runtime_error("does_fsentry_exist() requires a path, as a string.");
	}

	const string path = args[0].get_string_value();
	if(is_valid_absolute_dir_path(path) == false){
		throw std::runtime_error("does_fsentry_exist() illegal input path.");
	}

	bool exists = DoesEntryExist(path);
	const auto result = value_t::make_bool(exists);
#if 1
	const auto debug = value_and_type_to_ast_json(result);
	QUARK_TRACE(json_to_pretty_string(debug._value));
#endif

	const auto v = value_to_bc(result);
	return v;
}


bc_value_t host__create_directory_branch(interpreter_t& vm, const bc_value_t args[], int arg_count){
	QUARK_ASSERT(vm.check_invariant());

	if(arg_count != 1){
		throw std::runtime_error("create_directory_branch() requires 1 arguments!");
	}
	if(args[0]._type.is_string() == false){
		throw std::runtime_error("create_directory_branch() requires a path, as a string.");
	}

	const string path = args[0].get_string_value();
	if(is_valid_absolute_dir_path(path) == false){
		throw std::runtime_error("create_directory_branch() illegal input path.");
	}

	MakeDirectoriesDeep(path);
	return bc_value_t::make_void();
}

bc_value_t host__delete_fsentry_deep(interpreter_t& vm, const bc_value_t args[], int arg_count){
	QUARK_ASSERT(vm.check_invariant());

	if(arg_count != 1){
		throw std::runtime_error("delete_fsentry_deep() requires 1 arguments!");
	}
	if(args[0]._type.is_string() == false){
		throw std::runtime_error("delete_fsentry_deep() requires a path, as a string.");
	}

	const string path = args[0].get_string_value();
	if(is_valid_absolute_dir_path(path) == false){
		throw std::runtime_error("delete_fsentry_deep() illegal input path.");
	}

	DeleteDeep(path);

	return bc_value_t::make_void();
}


bc_value_t host__rename_fsentry(interpreter_t& vm, const bc_value_t args[], int arg_count){
	QUARK_ASSERT(vm.check_invariant());

	if(arg_count != 2){
		throw std::runtime_error("rename_fsentry() requires 2 arguments!");
	}
	if(args[0]._type.is_string() == false){
		throw std::runtime_error("rename_fsentry() requires a path, as a string.");
	}
	if(args[1]._type.is_string() == false){
		throw std::runtime_error("rename_fsentry() argument 2 is name name, as a string.");
	}

	const string path = args[0].get_string_value();
	if(is_valid_absolute_dir_path(path) == false){
		throw std::runtime_error("rename_fsentry() illegal input path.");
	}
	const string n = args[1].get_string_value();
	if(n.empty()){
		throw std::runtime_error("rename_fsentry() illegal input name.");
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







std::map<std::string, host_function_signature_t> get_host_function_signatures(){
	const auto VOID = typeid_t::make_void();
	const auto DYN = typeid_t::make_internal_dynamic();

	const auto k_fsentry_t__type = make__fsentry_t__type();

	const std::map<std::string, host_function_signature_t> temp {
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


		{ "script_to_jsonvalue", host_function_signature_t{ 1017, typeid_t::make_function(typeid_t::make_json_value(), {typeid_t::make_string()}, epure::pure) }},
		{ "jsonvalue_to_script", host_function_signature_t{ 1018, typeid_t::make_function(typeid_t::make_string(), {typeid_t::make_json_value()}, epure::pure) }},
		{ "value_to_jsonvalue", host_function_signature_t{ 1019, typeid_t::make_function(typeid_t::make_json_value(), { DYN }, epure::pure) }},
		{ "jsonvalue_to_value", host_function_signature_t{ 1020, typeid_t::make_function(DYN, { typeid_t::make_json_value(), typeid_t::make_typeid() }, epure::pure) }},
		{ "get_json_type", host_function_signature_t{ 1021, typeid_t::make_function(typeid_t::make_int(), {typeid_t::make_json_value()}, epure::pure) }},

		{ "calc_string_sha1", host_function_signature_t{ 1031, typeid_t::make_function(make__sha1_t__type(), { typeid_t::make_string() }, epure::pure) }},
		{ "calc_binary_sha1", host_function_signature_t{ 1032, typeid_t::make_function(make__sha1_t__type(), { make__binary_t__type() }, epure::pure) }},

		{ "map", host_function_signature_t{ 1033, typeid_t::make_function(DYN, { DYN, DYN}, epure::pure) }},


		//	print = impure!
		{ "print", host_function_signature_t{ 1000, typeid_t::make_function(VOID, { DYN }, epure::pure) } },
		{ "send", host_function_signature_t{ 1022, typeid_t::make_function(VOID, { typeid_t::make_string(), typeid_t::make_json_value() }, epure::impure) } },
		{ "get_time_of_day", host_function_signature_t{ 1005, typeid_t::make_function(typeid_t::make_int(), {}, epure::impure) }},


		{ "read_text_file", host_function_signature_t{ 1015, typeid_t::make_function(typeid_t::make_string(), { DYN }, epure::impure) }},
		{ "write_text_file", host_function_signature_t{ 1016, typeid_t::make_function(VOID, { DYN, DYN }, epure::impure) }},
		{
			"get_fsentries_shallow",
			host_function_signature_t{
				1023,
				typeid_t::make_function(typeid_t::make_vector(k_fsentry_t__type), { typeid_t::make_string() }, epure::impure)
			}
		},
		{
			"get_fsentries_deep",
			host_function_signature_t{
				1024,
				typeid_t::make_function(typeid_t::make_vector(k_fsentry_t__type), { typeid_t::make_string() }, epure::impure)
			}
		},
		{
			"get_fsentry_info",
			host_function_signature_t{
				1025,
				typeid_t::make_function(
					make__fsentry_info_t__type(),
					{ typeid_t::make_string() },
					epure::impure
				)
			}
		},
		{
			"get_fs_environment",
			host_function_signature_t{
				1026,
				typeid_t::make_function(
					make__fs_environment_t__type(),
					{},
					epure::impure
				)
			}
		},
		{
			"does_fsentry_exist",
			host_function_signature_t{
				1027,
				typeid_t::make_function(
					typeid_t::make_bool(),
					{ typeid_t::make_string() },
					epure::impure
				)
			}
		},
		{
			"create_directory_branch",
			host_function_signature_t{
				1028,
				typeid_t::make_function(
					typeid_t::make_void(),
					{ typeid_t::make_string() },
					epure::impure
				)
			}
		},
		{
			"delete_fsentry_deep",
			host_function_signature_t{
				1029,
				typeid_t::make_function(
					typeid_t::make_void(),
					{ typeid_t::make_string() },
					epure::impure
				)
			}
		},
		{
			"rename_fsentry",
			host_function_signature_t{
				1030,
				typeid_t::make_function(
					typeid_t::make_void(),
					{ typeid_t::make_string(), typeid_t::make_string() },
					epure::impure
				)
			}
		}

	};
	return temp;
}


std::map<int,  host_function_t> get_host_functions(){
	const auto host_functions = get_host_function_signatures();

	const std::map<string, HOST_FUNCTION_PTR> lookup0 = {
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

		{ "script_to_jsonvalue", host__script_to_jsonvalue },
		{ "jsonvalue_to_script", host__jsonvalue_to_script },
		{ "value_to_jsonvalue", host__value_to_jsonvalue },
		{ "jsonvalue_to_value", host__jsonvalue_to_value },
		{ "get_json_type", host__get_json_type },

		{ "calc_string_sha1", host__calc_string_sha1 },
		{ "calc_binary_sha1", host__calc_binary_sha1 },
		{ "map", host__map },

		{ "print", host__print },
		{ "send", host__send },
		{ "get_time_of_day", host__get_time_of_day },

		{ "read_text_file", host__read_text_file },
		{ "write_text_file", host__write_text_file },
		{ "get_fsentries_shallow", host__get_fsentries_shallow },
		{ "get_fsentries_deep", host__get_fsentries_deep },
		{ "get_fsentry_info", host__get_fsentry_info },
		{ "get_fs_environment", host__get_fs_environment },
		{ "does_fsentry_exist", host__does_fsentry_exist },
		{ "create_directory_branch", host__create_directory_branch },
		{ "delete_fsentry_deep", host__delete_fsentry_deep },
		{ "rename_fsentry", host__rename_fsentry }
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
