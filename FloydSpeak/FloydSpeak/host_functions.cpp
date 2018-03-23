//
//  host_functions.cpp
//  FloydSpeak
//
//  Created by Marcus Zetterquist on 2018-02-23.
//  Copyright © 2018 Marcus Zetterquist. All rights reserved.
//

#include "host_functions.hpp"

#include "parser_primitives.h"
#include "parse_expression.h"
#include "parse_statement.h"
#include "statement.h"
#include "floyd_parser.h"
#include "ast_value.h"
#include "pass2.h"
#include "pass3.h"
#include "json_support.h"
#include "json_parser.h"

#include <cmath>
#include <sys/time.h>

#include <thread>
#include <chrono>
#include <algorithm>
#include <iostream>
#include <fstream>
#include "text_parser.h"

#include "floyd_interpreter.h"

namespace floyd {

using std::vector;
using std::string;
using std::pair;
using std::shared_ptr;
using std::unique_ptr;
using std::make_shared;


value_t flatten_to_json(const value_t& value){
	const auto j = value_to_ast_json(value);
	value_t json_value = value_t::make_json_value(j._value);
	return json_value;
}


value_t unflatten_json_to_specific_type(const json_t& v, const typeid_t& target_type){
	QUARK_ASSERT(v.check_invariant());

	if(target_type.is_null()){
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
	else if(target_type.is_float()){
		if(v.is_number()){
			return value_t::make_float((float)v.get_number());
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
			for(const auto member: struct_def._members){
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
			for(const auto member: source_obj){
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
value_t host__print(interpreter_t& vm, const std::vector<value_t>& args){
	QUARK_ASSERT(vm.check_invariant());

	if(args.size() != 1){
		throw std::runtime_error("assert() requires 1 argument!");
	}

	const auto& value = args[0];

#if 0
	if(value.is_struct() && value.get_struct_value()->_def == *json_value___struct_def){
		const auto s = json_value__to_compact_string(value);
		vm2._print_output.push_back(s);
	}
	else
#endif
	{
		const auto s = to_compact_string2(value);
		printf("%s\n", s.c_str());
		vm._print_output.push_back(s);
	}

	return value_t::make_null();
}

value_t host__assert(interpreter_t& vm, const std::vector<value_t>& args){
	QUARK_ASSERT(vm.check_invariant());

	if(args.size() != 1){
		throw std::runtime_error("assert() requires 1 argument!");
	}

	const auto& value = args[0];
	if(value.is_bool() == false){
		throw std::runtime_error("First argument to assert() must be of type bool.");
	}
	bool ok = value.get_bool_value();
	if(!ok){
		vm._print_output.push_back("Assertion failed.");
		throw std::runtime_error("Floyd assertion failed.");
	}
	return value_t::make_null();
}

//	string to_string(value_t)
value_t host__to_string(interpreter_t& vm, const std::vector<value_t>& args){
	QUARK_ASSERT(vm.check_invariant());

	if(args.size() != 1){
		throw std::runtime_error("to_string() requires 1 argument!");
	}

	const auto& value = args[0];
	const auto a = to_compact_string2(value);
	return value_t::make_string(a);
}
value_t host__to_pretty_string(interpreter_t& vm, const std::vector<value_t>& args){
	QUARK_ASSERT(vm.check_invariant());

	if(args.size() != 1){
		throw std::runtime_error("to_pretty_string() requires 1 argument!");
	}

	const auto& value = args[0];
	const auto json = value_to_ast_json(value);
	const auto s = json_to_pretty_string(json._value, 0, pretty_t{80, 4});
	return value_t::make_string(s);
}

value_t host__typeof(interpreter_t& vm, const std::vector<value_t>& args){
	QUARK_ASSERT(vm.check_invariant());

	if(args.size() != 1){
		throw std::runtime_error("typeof() requires 1 argument!");
	}

	const auto& value = args[0];
	const auto type = value.get_type();
	const auto result = value_t::make_typeid_value(type);
	return result;
}

value_t host__get_time_of_day(interpreter_t& vm, const std::vector<value_t>& args){
	QUARK_ASSERT(vm.check_invariant());

	if(args.size() != 0){
		throw std::runtime_error("get_time_of_day() requires 0 arguments!");
	}

	std::chrono::time_point<std::chrono::high_resolution_clock> t = std::chrono::high_resolution_clock::now();
	std::chrono::duration<double> elapsed_seconds = t - vm._imm->_start_time;
	const auto ms = elapsed_seconds.count() * 1000.0;
	return value_t::make_int(int(ms));
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





	value_t update_struct_member_shallow(interpreter_t& vm, const value_t& obj, const std::string& member_name, const value_t& new_value){
		QUARK_ASSERT(obj.check_invariant());
		QUARK_ASSERT(member_name.empty() == false);
		QUARK_ASSERT(new_value.check_invariant());

		const auto s = obj.get_struct_value();
		const auto def = s->_def;

		int member_index = find_struct_member_index(*def, member_name);
		if(member_index == -1){
			throw std::runtime_error("Unknown member.");
		}

		const auto struct_typeid = obj.get_type();
		const auto values = s->_member_values;


		QUARK_TRACE(typeid_to_compact_string(new_value.get_type()));
		QUARK_TRACE(typeid_to_compact_string(def->_members[member_index]._type));

		const auto dest_member_entry = def->_members[member_index];
		auto dest_member_resolved_type = dest_member_entry._type;

		//?? why is runtime resolve needed?
		dest_member_resolved_type = find_type_by_name(vm, dest_member_entry._type);

		if(!(new_value.get_type() == dest_member_resolved_type)){
			throw std::runtime_error("Value type not matching struct member type.");
		}

		auto values2 = values;
		values2[member_index] = new_value;

		auto s2 = value_t::make_struct_value(struct_typeid, values2);
		return s2;
	}

	value_t update_struct_member_deep(interpreter_t& vm, const value_t& obj, const std::vector<std::string>& path, const value_t& new_value){
		QUARK_ASSERT(obj.check_invariant());
		QUARK_ASSERT(path.empty() == false);
		QUARK_ASSERT(new_value.check_invariant());

		if(path.size() == 1){
			return update_struct_member_shallow(vm, obj, path[0], new_value);
		}
		else{
			vector<string> subpath = path;
			subpath.erase(subpath.begin());

			const auto s = obj.get_struct_value();
			const auto def = s->_def;
			int member_index = find_struct_member_index(*def, path[0]);
			if(member_index == -1){
				throw std::runtime_error("Unknown member.");
			}

			const auto child_value = s->_member_values[member_index];
			if(child_value.is_struct() == false){
				throw std::runtime_error("Value type not matching struct member type.");
			}

			const auto child2 = update_struct_member_deep(vm, child_value, subpath, new_value);
			const auto obj2 = update_struct_member_shallow(vm, obj, path[0], child2);
			return obj2;
		}
	}

value_t host__update(interpreter_t& vm, const std::vector<value_t>& args){
	QUARK_ASSERT(vm.check_invariant());

	QUARK_TRACE(json_to_pretty_string(interpreter_to_json(vm)));

	const auto& obj1 = args[0];
	const auto& lookup_key = args[1];
	const auto& new_value = args[2];

	if(args.size() != 3){
		throw std::runtime_error("update() needs 3 arguments.");
	}
	else{
		if(obj1.is_string()){
			if(lookup_key.is_int() == false){
				throw std::runtime_error("String lookup using integer index only.");
			}
			else{
				const auto obj = obj1;
				const auto v = obj.get_string_value();

				if((new_value.get_type().is_string() && new_value.get_string_value().size() == 1) == false){
					throw std::runtime_error("Update element must be a 1-character string.");
				}
				else{
					const int lookup_index = lookup_key.get_int_value();
					if(lookup_index < 0 || lookup_index >= v.size()){
						throw std::runtime_error("String lookup out of bounds.");
					}
					else{
						string v2 = v;
						v2[lookup_index] = new_value.get_string_value()[0];
						const auto s2 = value_t::make_string(v2);
						return s2;
					}
				}
			}
		}
		else if(obj1.is_json_value()){
			const auto json_value0 = obj1.get_json_value();
			if(json_value0.is_array()){
				assert(false);
			}
			else if(json_value0.is_object()){
				assert(false);
			}
			else{
				throw std::runtime_error("Can only update string, vector, dict or struct.");
			}
		}
		else if(obj1.is_vector()){
			if(lookup_key.is_int() == false){
				throw std::runtime_error("Vector lookup using integer index only.");
			}
			else{
				const auto obj = obj1;
				auto v2 = obj.get_vector_value();
				const auto element_type = obj.get_type().get_vector_element_type();

				if(element_type != new_value.get_type()){
					throw std::runtime_error("Update element must match vector type.");
				}
				else{
					const int lookup_index = lookup_key.get_int_value();
					if(lookup_index < 0 || lookup_index >= v2.size()){
						throw std::runtime_error("Vector lookup out of bounds.");
					}
					else{
						v2[lookup_index] = new_value;
						const auto s2 = value_t::make_vector_value(element_type, v2);
						return s2;
					}
				}
			}
		}
		else if(obj1.is_dict()){
			if(lookup_key.is_string() == false){
				throw std::runtime_error("Dict lookup using string key only.");
			}
			else{
				const auto obj = obj1;
				const auto entries = obj.get_dict_value();
				const auto value_type = obj.get_type().get_dict_value_type();
				if(value_type != new_value.get_type()){
					throw std::runtime_error("Update element must match dict value type.");
				}
				else{
					const string key = lookup_key.get_string_value();
					auto entries2 = entries;
					entries2[key] = new_value;
					const auto value2 = value_t::make_dict_value(value_type, entries2);
					return value2;
				}
			}
		}
		else if(obj1.is_struct()){
			if(lookup_key.is_string() == false){
				throw std::runtime_error("You must specify structure member using string.");
			}
			else{
				const auto obj = obj1;

				//### Does simple check for "." -- we should use vector of strings instead.
				const auto nodes = split_on_chars(seq_t(lookup_key.get_string_value()), ".");
				if(nodes.empty()){
					throw std::runtime_error("You must specify structure member using string.");
				}

				const auto s2 = update_struct_member_deep(vm, obj, nodes, new_value);
				return s2;
			}
		}
		else {
			throw std::runtime_error("Can only update string, vector, dict or struct.");
		}
	}
}

value_t host__size(interpreter_t& vm, const std::vector<value_t>& args){
	QUARK_ASSERT(vm.check_invariant());

	if(args.size() != 1){
		throw std::runtime_error("find requires 2 arguments");
	}

	const auto obj = args[0];
	if(obj.is_string()){
		const auto size = obj.get_string_value().size();
		return value_t::make_int(static_cast<int>(size));
	}
	else if(obj.is_json_value()){
		const auto value = obj.get_json_value();
		if(value.is_object()){
			const auto size = value.get_object_size();
			return value_t::make_int(static_cast<int>(size));
		}
		else if(value.is_array()){
			const auto size = value.get_array_size();
			return value_t::make_int(static_cast<int>(size));
		}
		else if(value.is_string()){
			const auto size = value.get_string().size();
			return value_t::make_int(static_cast<int>(size));
		}
		else{
			throw std::runtime_error("Calling size() on unsupported type of value.");
		}
	}
	else if(obj.is_vector()){
		const auto size = obj.get_vector_value().size();
		return value_t::make_int(static_cast<int>(size));
	}
	else if(obj.is_dict()){
		const auto size = obj.get_dict_value().size();
		return value_t::make_int(static_cast<int>(size));
	}
	else{
		throw std::runtime_error("Calling size() on unsupported type of value.");
	}
}

value_t host__find(interpreter_t& vm, const std::vector<value_t>& args){
	QUARK_ASSERT(vm.check_invariant());

	if(args.size() != 2){
		throw std::runtime_error("find requires 2 arguments");
	}

	const auto obj = args[0];
	const auto wanted = args[1];

	if(obj.is_string()){
		const auto str = obj.get_string_value();
		const auto wanted2 = wanted.get_string_value();

		const auto r = str.find(wanted2);
		if(r == std::string::npos){
			return value_t::make_int(static_cast<int>(-1));
		}
		else{
			return value_t::make_int(static_cast<int>(r));
		}
	}
	else if(obj.is_vector()){
		const auto vec = obj.get_vector_value();
		const auto element_type = obj.get_type().get_vector_element_type();
		if(wanted.get_type() != element_type){
			throw std::runtime_error("Type mismatch.");
		}
		const auto size = vec.size();
		int index = 0;
		while(index < size && vec[index] != wanted){
			index++;
		}
		if(index == size){
			return value_t::make_int(static_cast<int>(-1));
		}
		else{
			return value_t::make_int(static_cast<int>(index));
		}
	}
	else{
		throw std::runtime_error("Calling find() on unsupported type of value.");
	}
}

value_t host__exists(interpreter_t& vm, const std::vector<value_t>& args){
	QUARK_ASSERT(vm.check_invariant());

	if(args.size() != 2){
		throw std::runtime_error("find requires 2 arguments");
	}

	const auto obj = args[0];
	const auto key = args[1];

	if(obj.is_dict()){
		if(key.get_type().is_string() == false){
			throw std::runtime_error("Key must be string.");
		}
		const auto key_string = key.get_string_value();
		const auto entries = obj.get_dict_value();

		const auto found_it = entries.find(key_string);
		const bool exists = found_it != entries.end();
		return value_t::make_bool(exists);
	}
	else{
		throw std::runtime_error("Calling exist() on unsupported type of value.");
	}
}

value_t host__erase(interpreter_t& vm, const std::vector<value_t>& args){
	QUARK_ASSERT(vm.check_invariant());

	if(args.size() != 2){
		throw std::runtime_error("find requires 2 arguments");
	}

	const auto obj = args[0];
	const auto key = args[1];

	if(obj.is_dict()){
		if(key.get_type().is_string() == false){
			throw std::runtime_error("Key must be string.");
		}
		const auto key_string = key.get_string_value();
		const auto entries = obj.get_dict_value();

		std::map<string, value_t> entries2 = entries;
		entries2.erase(key_string);
		const auto value_type = obj.get_type().get_dict_value_type();
		const auto value2 = value_t::make_dict_value(value_type, entries2);
		return value2;
	}
	else{
		throw std::runtime_error("Calling exist() on unsupported type of value.");
	}
}




//	assert(push_back(["one","two"], "three") == ["one","two","three"])
value_t host__push_back(interpreter_t& vm, const std::vector<value_t>& args){
	QUARK_ASSERT(vm.check_invariant());

	if(args.size() != 2){
		throw std::runtime_error("find requires 2 arguments");
	}

	const auto obj = args[0];
	const auto element = args[1];
	if(obj.is_string()){
		const auto str = obj.get_string_value();
		const auto ch = element.get_string_value();

		auto str2 = str + ch;
		const auto v = value_t::make_string(str2);
		return v;
	}
	else if(obj.is_vector()){
		const auto vec = obj.get_vector_value();
		const auto element_type = obj.get_type().get_vector_element_type();
		if(element.get_type() != element_type){
			throw std::runtime_error("Type mismatch.");
		}
		auto elements2 = vec;
		elements2.push_back(element);
		const auto v = value_t::make_vector_value(element_type, elements2);
		return v;
	}
	else{
		throw std::runtime_error("Calling push_back() on unsupported type of value.");
	}
}

//	assert(subset("abc", 1, 3) == "bc");
value_t host__subset(interpreter_t& vm, const std::vector<value_t>& args){
	QUARK_ASSERT(vm.check_invariant());

	if(args.size() != 3){
		throw std::runtime_error("subset() requires 3 arguments");
	}

	const auto obj = args[0];

	if(args[1].is_int() == false || args[2].is_int() == false){
		throw std::runtime_error("subset() requires start and end to be integers.");
	}

	const auto start = args[1].get_int_value();
	const auto end = args[2].get_int_value();
	if(start < 0 || end < 0){
		throw std::runtime_error("subset() requires start and end to be non-negative.");
	}

	if(obj.is_string()){
		const auto str = obj.get_string_value();
		const auto start2 = std::min(start, static_cast<int>(str.size()));
		const auto end2 = std::min(end, static_cast<int>(str.size()));

		string str2;
		for(int i = start2 ; i < end2 ; i++){
			str2.push_back(str[i]);
		}
		const auto v = value_t::make_string(str2);
		return v;
	}
	else if(obj.is_vector()){
		const auto vec = obj.get_vector_value();
		const auto element_type = obj.get_type().get_vector_element_type();
		const auto start2 = std::min(start, static_cast<int>(vec.size()));
		const auto end2 = std::min(end, static_cast<int>(vec.size()));
		vector<value_t> elements2;
		for(int i = start2 ; i < end2 ; i++){
			elements2.push_back(vec[i]);
		}
		const auto v = value_t::make_vector_value(element_type, elements2);
		return v;
	}
	else{
		throw std::runtime_error("Calling push_back() on unsupported type of value.");
	}
}



//	assert(replace("One ring to rule them all", 4, 7, "rabbit") == "One rabbit to rule them all");
value_t host__replace(interpreter_t& vm, const std::vector<value_t>& args){
	QUARK_ASSERT(vm.check_invariant());

	if(args.size() != 4){
		throw std::runtime_error("replace() requires 4 arguments");
	}

	const auto obj = args[0];

	if(args[1].is_int() == false || args[2].is_int() == false){
		throw std::runtime_error("replace() requires start and end to be integers.");
	}

	const auto start = args[1].get_int_value();
	const auto end = args[2].get_int_value();
	if(start < 0 || end < 0){
		throw std::runtime_error("replace() requires start and end to be non-negative.");
	}
	if(args[3].get_type() != args[0].get_type()){
		throw std::runtime_error("replace() requires 4th arg to be same as argument 0.");
	}

	if(obj.is_string()){
		const auto str = obj.get_string_value();
		const auto start2 = std::min(start, static_cast<int>(str.size()));
		const auto end2 = std::min(end, static_cast<int>(str.size()));
		const auto new_bits = args[3].get_string_value();

		string str2 = str.substr(0, start2) + new_bits + str.substr(end2);
		const auto v = value_t::make_string(str2);
		return v;
	}
	else if(obj.is_vector()){
		const auto vec = obj.get_vector_value();
		const auto element_type = obj.get_type().get_vector_element_type();
		const auto start2 = std::min(start, static_cast<int>(vec.size()));
		const auto end2 = std::min(end, static_cast<int>(vec.size()));
		const auto new_bits = args[3].get_vector_value();


		const auto string_left = vector<value_t>(vec.begin(), vec.begin() + start2);
		const auto string_right = vector<value_t>(vec.begin() + end2, vec.end());

		auto result = string_left;
		result.insert(result.end(), new_bits.begin(), new_bits.end());
		result.insert(result.end(), string_right.begin(), string_right.end());

		const auto v = value_t::make_vector_value(element_type, result);
		return v;
	}
	else{
		throw std::runtime_error("Calling replace() on unsupported type of value.");
	}
}

value_t host__get_env_path(interpreter_t& vm, const std::vector<value_t>& args){
	QUARK_ASSERT(vm.check_invariant());

	if(args.size() != 0){
		throw std::runtime_error("get_env_path() requires 0 arguments!");
	}

    const char *homeDir = getenv("HOME");
    const std::string env_path(homeDir);
//	const std::string env_path = "~/Desktop/";

	return value_t::make_string(env_path);
}

value_t host__read_text_file(interpreter_t& vm, const std::vector<value_t>& args){
	QUARK_ASSERT(vm.check_invariant());

	if(args.size() != 1){
		throw std::runtime_error("read_text_file() requires 1 arguments!");
	}
	if(args[0].is_string() == false){
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
	return value_t::make_string(file_contents);
}

value_t host__write_text_file(interpreter_t& vm, const std::vector<value_t>& args){
	QUARK_ASSERT(vm.check_invariant());

	if(args.size() != 2){
		throw std::runtime_error("write_text_file() requires 2 arguments!");
	}
	else if(args[0].is_string() == false){
		throw std::runtime_error("write_text_file() requires a file path as a string.");
	}
	else if(args[1].is_string() == false){
		throw std::runtime_error("write_text_file() requires a file path as a string.");
	}
	else{
		const string path = args[0].get_string_value();
		const string file_contents = args[1].get_string_value();

		std::ofstream outputFile;
		outputFile.open(path);
		outputFile << file_contents;
		outputFile.close();
		return value_t();
	}
}

/*
	Reads json from a text string, returning an unpacked json_value.
*/
value_t host__decode_json(interpreter_t& vm, const std::vector<value_t>& args){
	QUARK_ASSERT(vm.check_invariant());

	if(args.size() != 1){
		throw std::runtime_error("decode_json_value() requires 1 argument!");
	}
	else if(args[0].is_string() == false){
		throw std::runtime_error("decode_json_value() requires string argument.");
	}
	else{
		const string s = args[0].get_string_value();
		std::pair<json_t, seq_t> result = parse_json(seq_t(s));
		value_t json_value = value_t::make_json_value(result.first);
		return json_value;
	}
}

value_t host__encode_json(interpreter_t& vm, const std::vector<value_t>& args){
	QUARK_ASSERT(vm.check_invariant());

	if(args.size() != 1){
		throw std::runtime_error("encode_json() requires 1 argument!");
	}
	else if(args[0].is_json_value()== false){
		throw std::runtime_error("encode_json() requires argument to be json_value.");
	}
	else{
		const auto value0 = args[0].get_json_value();
		const string s = json_to_compact_string(value0);

		return value_t::make_string(s);
	}
}



value_t host__flatten_to_json(interpreter_t& vm, const std::vector<value_t>& args){
	QUARK_ASSERT(vm.check_invariant());

	if(args.size() != 1){
		throw std::runtime_error("flatten_to_json() requires 1 argument!");
	}
	else{
		const auto value = args[0];
		const auto result = flatten_to_json(value);
		return result;
	}
}

value_t host__unflatten_from_json(interpreter_t& vm, const std::vector<value_t>& args){
	QUARK_ASSERT(vm.check_invariant());

	if(args.size() != 2){
		throw std::runtime_error("unflatten_from_json() requires 2 argument!");
	}
	else if(args[0].is_json_value() == false){
		throw std::runtime_error("unflatten_from_json() requires string argument.");
	}
	else if(args[1].is_typeid()== false){
		throw std::runtime_error("unflatten_from_json() requires string argument.");
	}
	else{
		const auto json_value = args[0].get_json_value();
		const auto target_type = args[1].get_typeid_value();
		const auto value = unflatten_json_to_specific_type(json_value, target_type);
		return value;
	}
}


value_t host__get_json_type(interpreter_t& vm, const std::vector<value_t>& args){
	QUARK_ASSERT(vm.check_invariant());

	if(args.size() != 1){
		throw std::runtime_error("get_json_type() requires 1 argument!");
	}
	else if(args[0].is_json_value() == false){
		throw std::runtime_error("get_json_type() requires json_value argument.");
	}
	else{
		const auto json_value = args[0].get_json_value();

		if(json_value.is_object()){
			return value_t::make_int(1);
//			return { vm, value_t::make_typeid_value(typeid_t::make_dict(typeid_t::make_json_value())) };
		}
		else if(json_value.is_array()){
			return value_t::make_int(2);
//			return { vm, value_t::make_typeid_value(typeid_t::make_vector(typeid_t::make_json_value())) };
		}
		else if(json_value.is_string()){
			return value_t::make_int(3);
//			return { vm, value_t::make_typeid_value(typeid_t::make_string()) };
		}
		else if(json_value.is_number()){
			return value_t::make_int(4);
//			return { vm, value_t::make_typeid_value(typeid_t::make_float()) };
		}
		else if(json_value.is_true()){
			return value_t::make_int(5);
//			return { vm, value_t::make_typeid_value(typeid_t::make_bool()) };
		}
		else if(json_value.is_false()){
			return value_t::make_int(6);
//			return { vm, value_t::make_typeid_value(typeid_t::make_bool()) };
		}
		else if(json_value.is_null()){
			return value_t::make_int(7);
//			return { vm, value_t::make_typeid_value(typeid_t::make_null()) };
		}
		else{
			QUARK_ASSERT(false);
			throw std::exception();
		}
	}
}


/*
	o: typeid,
	1... -- arguments to constructor.
*/
value_t host__instantiate_from_typeid(interpreter_t& vm, const std::vector<value_t>& args){
	QUARK_ASSERT(vm.check_invariant());

	if(args.size() == 0){
		throw std::runtime_error("instantiate_from_typeid() requires 1 argument or more!");
	}
	else if(args[0].is_typeid() == false){
		throw std::runtime_error("instantiate_from_typeid() requires argument 1 to be the typeid of type to instantiate!");
	}
	else{
		const auto type_to_construct = args[0].get_typeid_value();
		const std::vector<value_t> init_args(args.begin() + 1, args.end());
		const auto result = construct_value_from_typeid(vm, type_to_construct, type_to_construct, values_to_bcs(init_args));
		return bc_to_value(result, type_to_construct);
	}
}



std::map<std::string, host_function_signature_t> get_host_function_signatures(){
	const std::map<std::string, host_function_signature_t> temp {
		{ "print", host_function_signature_t{ 1000, typeid_t::make_function(typeid_t::make_null(), { typeid_t::make_null()}) } },
		{ "assert", host_function_signature_t{ 1001, typeid_t::make_function(typeid_t::make_null(), {typeid_t::make_null()}) } },
		{ "to_string", host_function_signature_t{ 1002, typeid_t::make_function(typeid_t::make_string(), {typeid_t::make_null()}) } },
		{ "to_pretty_string", host_function_signature_t{ 1003, typeid_t::make_function(typeid_t::make_string(), {typeid_t::make_null()}) }},
		{ "typeof", host_function_signature_t{1004, typeid_t::make_function(typeid_t::make_typeid(), {typeid_t::make_null()}) } },
		{ "get_time_of_day", host_function_signature_t{ 1005, typeid_t::make_function(typeid_t::make_int(), {}) }},

		{ "update", host_function_signature_t{ 1006, typeid_t::make_function(typeid_t::make_null(), {typeid_t::make_null(),typeid_t::make_null(),typeid_t::make_null()}) }},
		{ "size", host_function_signature_t{ 1007, typeid_t::make_function(typeid_t::make_int(), {typeid_t::make_null()}) } },
		{ "find", host_function_signature_t{ 1008, typeid_t::make_function(typeid_t::make_int(), {typeid_t::make_null(), typeid_t::make_null()}) }},
		{ "exists", host_function_signature_t{ 1009, typeid_t::make_function(typeid_t::make_bool(), {typeid_t::make_null(),typeid_t::make_null()}) }},
		{ "erase", host_function_signature_t{ 1010, typeid_t::make_function(typeid_t::make_null(), {typeid_t::make_null(),typeid_t::make_null()}) } },
		{ "push_back", host_function_signature_t{ 1011, typeid_t::make_function(typeid_t::make_null(), {typeid_t::make_null(),typeid_t::make_null()}) }},
		{ "subset", host_function_signature_t{ 1012, typeid_t::make_function(typeid_t::make_null(), {typeid_t::make_null(),typeid_t::make_null(),typeid_t::make_null()}) }},
		{ "replace", host_function_signature_t{ 1013, typeid_t::make_function(typeid_t::make_null(), {typeid_t::make_null(),typeid_t::make_null(),typeid_t::make_null(),typeid_t::make_null()}) }},

		{ "get_env_path", host_function_signature_t{ 1014, typeid_t::make_function(typeid_t::make_string(), {}) }},
		{ "read_text_file", host_function_signature_t{ 1015, typeid_t::make_function(typeid_t::make_string(), {typeid_t::make_null()}) }},
		{ "write_text_file", host_function_signature_t{ 1016, typeid_t::make_function(typeid_t::make_null(), {typeid_t::make_null(), typeid_t::make_null()}) }},

		{ "decode_json", host_function_signature_t{ 1017, typeid_t::make_function(typeid_t::make_json_value(), {typeid_t::make_string()}) }},
		{ "encode_json", host_function_signature_t{ 1018, typeid_t::make_function(typeid_t::make_string(), {typeid_t::make_json_value()}) }},
		{ "flatten_to_json", host_function_signature_t{ 1019, typeid_t::make_function(typeid_t::make_json_value(), {typeid_t::make_null()}) }},
		{ "unflatten_from_json", host_function_signature_t{ 1020, typeid_t::make_function(typeid_t::make_null(), {typeid_t::make_null(), typeid_t::make_null()}) }},
		{ "get_json_type", host_function_signature_t{ 1021, typeid_t::make_function(typeid_t::make_int(), {typeid_t::make_json_value()}) }},

		{ "instantiate_from_typeid", host_function_signature_t{ 1022, typeid_t::make_function(typeid_t::make_null(), {typeid_t::make_null()}) }}

	};
	return temp;
}


/*
	struct host_function_t {
		host_function_signature_t _signature;
		HOST_FUNCTION_PTR _f;
	}
*/



std::map<int,  host_function_t> get_host_functions(){
	const auto host_functions = get_host_function_signatures();

	const std::map<string, HOST_FUNCTION_PTR> lookup0 = {
		{ "print", host__print },
		{ "assert", host__assert },
		{ "to_string", host__to_string },
		{ "to_pretty_string", host__to_pretty_string },
		{ "typeof", host__typeof },
		{ "get_time_of_day", host__get_time_of_day },

		{ "update", host__update },
		{ "size", host__size },
		{ "find", host__find },
		{ "exists", host__exists },
		{ "erase", host__erase },
		{ "push_back", host__push_back },
		{ "subset", host__subset },
		{ "replace", host__replace },

		{ "get_env_path", host__get_env_path },
		{ "read_text_file", host__read_text_file },
		{ "write_text_file", host__write_text_file },

		{ "decode_json", host__decode_json },
		{ "encode_json", host__encode_json },
		{ "flatten_to_json", host__flatten_to_json },
		{ "unflatten_from_json", host__unflatten_from_json },
		{ "get_json_type", host__get_json_type },
		{ "instantiate_from_typeid", host__instantiate_from_typeid }
	};

	const auto lookup = [&](){
		std::map<int,  host_function_t> result;
		for(const auto e: lookup0){
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
