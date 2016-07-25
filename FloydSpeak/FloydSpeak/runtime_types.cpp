
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
		if(t == k_bool){
			return "bool";
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


	member_t::member_t(const std::string& name, const std::string& type_identifier, const std::shared_ptr<type_definition_t>& type) :
		_name(name),
		_type_identifier(type_identifier),
		_type(type)
	{
		QUARK_ASSERT(!type_identifier.empty());

		QUARK_ASSERT(type);
		QUARK_ASSERT(type->check_invariant());

		QUARK_ASSERT(check_invariant());
	}

	bool member_t::check_invariant() const{
		QUARK_ASSERT(!_type_identifier.empty());
		QUARK_ASSERT(_type);
		QUARK_ASSERT(_type->check_invariant());
		return true;
	}



	////////////////////////			struct_def_t


	bool struct_def_t::check_invariant() const{
		return true;
	}





	////////////////////////			type_definition_t


	bool type_definition_t::check_invariant() const{
		if(_base_type == k_int32){
			QUARK_ASSERT(!_struct_def);
			QUARK_ASSERT(!_vector_def);
		}
		else if(_base_type == k_bool){
			QUARK_ASSERT(!_struct_def);
			QUARK_ASSERT(!_vector_def);
		}
		else if(_base_type == k_string){
			QUARK_ASSERT(!_struct_def);
			QUARK_ASSERT(!_vector_def);
		}
		else if(_base_type == k_struct){
			QUARK_ASSERT(_struct_def);
			QUARK_ASSERT(!_vector_def);
		}
		else if(_base_type == k_vector){
			QUARK_ASSERT(!_struct_def);
			QUARK_ASSERT(_vector_def);
		}
		else{
			QUARK_ASSERT(false);
		}
		return true;
	}





	void trace_frontend_type(const type_definition_t& t, const std::string& label){
		QUARK_ASSERT(t.check_invariant());

		if(t._base_type == k_int32){
			QUARK_TRACE("<" + to_string(t._base_type) + "> " + label);
		}
		else if(t._base_type == k_bool){
			QUARK_TRACE("<" + to_string(t._base_type) + "> " + label);
		}
		else if(t._base_type == k_string){
			QUARK_TRACE("<" + to_string(t._base_type) + "> " + label);
		}
		else if(t._base_type == k_struct){
			QUARK_SCOPED_TRACE("<" + to_string(t._base_type) + "> " + label);
			for(const auto it: t._struct_def->_members){
				trace_frontend_type(*it._type, it._name);
			}
		}
		else if(t._base_type == k_vector){
			QUARK_SCOPED_TRACE("<" + to_string(t._base_type) + "> " + label);
			trace_frontend_type(*t._vector_def->_value_type, "");
		}
		else{
			QUARK_ASSERT(false);
		}
	}


	std::string to_signature(const type_definition_t& t){
		QUARK_ASSERT(t.check_invariant());

		const auto base_type = to_string(t._base_type);

		const string label = "";
		if(t._base_type == k_struct){
			string body;
			for(const auto& member : t._struct_def->_members) {
				const string member_label = member._name;
				const string typedef_s = member._type_identifier;
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
			const auto vector_value_s = t._vector_def->_value_type_identifier.empty() ? to_signature(*t._vector_def->_value_type) : "<" + t._vector_def->_value_type_identifier + ">";
			return label + "<vector>" + "[" + vector_value_s + "]";
		}
		else{
			return label + "<" + base_type + ">";
		}
	}

	/*
		alignment == 8: pos is roundet up untill nearest multiple of 8.
	*/
	std::size_t align_pos(std::size_t pos, std::size_t alignment){
		std::size_t rem = pos % alignment;
		std::size_t add = rem == 0 ? 0 : alignment - rem;
		return pos + add;
	}


QUARK_UNIT_TESTQ("align_pos()", ""){
	QUARK_TEST_VERIFY(align_pos(0, 8) == 0);
	QUARK_TEST_VERIFY(align_pos(1, 8) == 8);
	QUARK_TEST_VERIFY(align_pos(2, 8) == 8);
	QUARK_TEST_VERIFY(align_pos(3, 8) == 8);
	QUARK_TEST_VERIFY(align_pos(4, 8) == 8);
	QUARK_TEST_VERIFY(align_pos(5, 8) == 8);
	QUARK_TEST_VERIFY(align_pos(6, 8) == 8);
	QUARK_TEST_VERIFY(align_pos(7, 8) == 8);
	QUARK_TEST_VERIFY(align_pos(8, 8) == 8);
	QUARK_TEST_VERIFY(align_pos(9, 8) == 16);
}

	std::vector<byte_range_t> calc_struct_default_memory_layout(const type_definition_t& s){
		QUARK_ASSERT(s.check_invariant());

		std::vector<byte_range_t> result;
		std::size_t pos = 0;
		for(const auto& member : s._struct_def->_members) {
			if(member._type->_base_type == k_int32){
				pos = align_pos(pos, 4);
				result.push_back(byte_range_t(pos, 4));
				pos += 4;
			}
			else if(member._type->_base_type == k_bool){
				result.push_back(byte_range_t(pos, 1));
				pos += 1;
			}
			else if(member._type->_base_type == k_string){
				pos = align_pos(pos, 8);
				result.push_back(byte_range_t(pos, 8));
				pos += 8;
			}
			else if(member._type->_base_type == k_struct){
				pos = align_pos(pos, 8);
				result.push_back(byte_range_t(pos, 8));
				pos += 8;
			}
			else if(member._type->_base_type == k_vector){
				pos = align_pos(pos, 8);
				result.push_back(byte_range_t(pos, 8));
				pos += 8;
			}
			else{
				QUARK_ASSERT(false);
			}
		}
		pos = align_pos(pos, 8);
		result.insert(result.begin(), byte_range_t(0, pos));
		return result;
	}


	////////////////////////			frontend_types_t


	/*
		Holds all types of program.
	*/

	frontend_types_t::frontend_types_t(){
		QUARK_ASSERT(check_invariant());


		//	int32
		{
			auto def = make_shared<type_definition_t>();
			def->_base_type = k_int32;
			QUARK_ASSERT(def->check_invariant());

			_identifiers["int32"] = def;
			_type_definitions[to_signature(*def)] = def;
		}

		//	bool
		{
			auto def = make_shared<type_definition_t>();
			def->_base_type = k_bool;
			QUARK_ASSERT(def->check_invariant());

			_identifiers["bool"] = def;
			_type_definitions[to_signature(*def)] = def;
		}

		//	string
		{
			auto def = make_shared<type_definition_t>();
			def->_base_type = k_string;
			QUARK_ASSERT(def->check_invariant());

			_identifiers["string"] = def;
			_type_definitions[to_signature(*def)] = def;
		}
	}
	
	bool frontend_types_t::check_invariant() const{
		for(const auto it: _identifiers){
			QUARK_ASSERT(it.first != "");
		}

		for(const auto it: _type_definitions){
			const auto signature = to_signature(*it.second);
			QUARK_ASSERT(it.first == signature);
			QUARK_ASSERT(it.second->check_invariant());
		}

		//	Make sure all types referenced from _identifiers are also stored inside _type_definition.
		for(const auto it: _identifiers){
			auto defs_it = _type_definitions.begin();
			while(defs_it != _type_definitions.end() && defs_it->second != it.second){
				 defs_it++;
			}

			QUARK_ASSERT(defs_it != _type_definitions.end());
		}
		return true;
	}

	//??? What happens if existing_identifier isn't bound to def yet? Maybe we should actually chain the identifiers?
	frontend_types_t frontend_types_t::define_alias(const std::string& new_identifier, const std::string& existing_identifier) const{
		QUARK_ASSERT(check_invariant());
		QUARK_ASSERT(!lookup_identifier(new_identifier));

		auto it = _identifiers.find(existing_identifier);
		if(it == _identifiers.end()){
			throw std::runtime_error("unknown identifier to alias");
		}

		auto result = *this;
		result._identifiers[new_identifier] = it->second;

		QUARK_ASSERT(result.check_invariant());

		trace_frontend_type(*it->second, "");
		return result;
	}

	frontend_types_t frontend_types_t::define_struct_type(const std::string& new_identifier, std::vector<member_t> members) const{
		QUARK_ASSERT(check_invariant());
		QUARK_ASSERT(!lookup_identifier(new_identifier));

		auto def = make_shared<type_definition_t>();
		def->_base_type = k_struct;
		def->_struct_def = make_shared<struct_def_t>();
		def->_struct_def->_members = members;
		const string signature = to_signature(*def);

		//	Reuse existing type def if one exists with the correct signature.
		const auto existing_it = _type_definitions.find(signature);
		if(existing_it != _type_definitions.end()){
			def = existing_it->second;
		}

		auto result = *this;
		if(!new_identifier.empty()){
			result._identifiers[new_identifier] = def;
		}
		result._type_definitions[signature] = def;

		QUARK_ASSERT(result.check_invariant());

		trace_frontend_type(*def, new_identifier);

		return result;
	}

	member_t frontend_types_t::make_member(const std::string& name, const std::string& type_identifier) const{
		auto def = lookup_identifier(type_identifier);
		member_t result(name, type_identifier, def);
		return result;
	}

	std::shared_ptr<type_definition_t> frontend_types_t::lookup_identifier(const std::string& s) const{
		QUARK_ASSERT(check_invariant());

		const auto it = _identifiers.find(s);
		return it == _identifiers.end() ? std::shared_ptr<type_definition_t>() : it->second;
	}

	std::shared_ptr<type_definition_t> frontend_types_t::lookup_signature(const std::string& s) const{
		QUARK_ASSERT(check_invariant());

		const auto it = _type_definitions.find(s);
		return it == _type_definitions.end() ? std::shared_ptr<type_definition_t>() : it->second;
	}


} //	runtime_types;




using namespace runtime_types;



//////////////////////////////////////		to_string(frontend_base_type)


QUARK_UNIT_TESTQ("to_string(frontend_base_type)", ""){
	QUARK_TEST_VERIFY(to_string(k_int32) == "int32");
	QUARK_TEST_VERIFY(to_string(k_bool) == "bool");
	QUARK_TEST_VERIFY(to_string(k_string) == "string");
	QUARK_TEST_VERIFY(to_string(k_struct) == "struct");
	QUARK_TEST_VERIFY(to_string(k_vector) == "vector");
}


/*
	struct {
	}
*/
frontend_types_t define_test_struct_0(const frontend_types_t& types){
	const auto a = types.define_struct_type("", {});
	return a;
}


/*
	struct struct_1 {
		int32 a
		string b
	}
*/
frontend_types_t define_test_struct_1(const frontend_types_t& types){
	const auto a = types.define_struct_type("struct_1",
		{
			types.make_member("a", "int32"),
			types.make_member("b", "string")
		}
	);
	return a;
}

/*
	struct struct_2 {
		string x
		<struct_1> y
		string z
	}
*/
frontend_types_t define_test_struct_2(const frontend_types_t& types){
	const auto a = types.define_struct_type("struct_2",
		{
			types.make_member("x", "string"),
			types.make_member("y", "struct_1"),
			types.make_member("z", "string")
		}
	);
	return a;
}
//??? check for duplicate member names.
frontend_types_t define_test_struct_3(const frontend_types_t& types){
	const auto a = types.define_struct_type("struct_3",
		{
			types.make_member("a", "bool"),
			// pad
			// pad
			// pad
			types.make_member("b", "int32"),
			types.make_member("c", "bool"),
			types.make_member("d", "bool"),
			types.make_member("e", "bool"),
			types.make_member("f", "bool"),
			types.make_member("g", "string"),
			types.make_member("h", "bool")
		}
	);
	return a;
}


//////////////////////////////////////		to_signature()


QUARK_UNIT_TESTQ("to_signature()", "empty unnamed struct"){
	const auto a = frontend_types_t();
	const auto b = define_test_struct_0(a);
	const auto t1 = b.lookup_signature("<struct>{}");
	QUARK_TEST_VERIFY(t1);
	QUARK_TEST_VERIFY(to_signature(*t1) == "<struct>{}");
}

QUARK_UNIT_TESTQ("to_signature()", "struct 1"){
	const auto a = frontend_types_t();
	const auto b = define_test_struct_1(a);
	const auto t1 = b.lookup_identifier("struct_1");
	const auto s1 = to_signature(*t1);
	QUARK_TEST_VERIFY(s1 == "<struct>{<int32>a,<string>b}");
}


QUARK_UNIT_TESTQ("to_signature()", "struct 2"){
	const auto a = frontend_types_t();
	const auto b = define_test_struct_1(a);
	const auto c = define_test_struct_2(b);
	const auto t2 = c.lookup_identifier("struct_2");
	const auto s2 = to_signature(*t2);
	QUARK_TEST_VERIFY(s2 == "<struct>{<string>x,<struct_1>y,<string>z}");
}



//////////////////////////////////////		frontend_types_t



QUARK_UNIT_TESTQ("frontend_types_t::frontend_types_t()", "default construction"){
	const auto a = frontend_types_t();
	QUARK_TEST_VERIFY(a.check_invariant());

	QUARK_TEST_VERIFY(a._identifiers.size() == 3);
	QUARK_TEST_VERIFY(a._type_definitions.size() == 3);

	const auto b = a.lookup_identifier("int32");
	QUARK_TEST_VERIFY(b);
	QUARK_TEST_VERIFY(b->_base_type == k_int32);

	const auto d = a.lookup_identifier("bool");
	QUARK_TEST_VERIFY(d);
	QUARK_TEST_VERIFY(d->_base_type == k_bool);

	const auto c = a.lookup_identifier("string");
	QUARK_TEST_VERIFY(c);
	QUARK_TEST_VERIFY(c->_base_type == k_string);
}


QUARK_UNIT_TESTQ("frontend_types_t::lookup_identifier()", "not found"){
	const auto a = frontend_types_t();
	const auto b = a.lookup_identifier("xyz");
	QUARK_TEST_VERIFY(!b);
}

QUARK_UNIT_TESTQ("frontend_types_t::lookup_identifier()", "int32 found"){
	const auto a = frontend_types_t();
	const auto b = a.lookup_identifier("int32");
	QUARK_TEST_VERIFY(b);
}


QUARK_UNIT_TESTQ("frontend_types_t::define_alias()", "int32 => my_int"){
	auto a = frontend_types_t();
	a = a.define_alias("my_int", "int32");
	const auto b = a.lookup_identifier("my_int");
	QUARK_TEST_VERIFY(b);
	QUARK_TEST_VERIFY(b->_base_type == k_int32);
}


QUARK_UNIT_TESTQ("calc_struct_default_memory_layout()", "struct 2"){
	const auto a = frontend_types_t();
	const auto b = define_test_struct_3(a);
	const auto t = b.lookup_identifier("struct_3");
	const auto layout = calc_struct_default_memory_layout(*t);
	int i = 0;
	for(const auto it: layout){
		const string name = i == 0 ? "struct" : t->_struct_def->_members[i - 1]._name;
		QUARK_TRACE_SS(it.first << "--" << (it.first + it.second) << ": " + name);
		i++;
	}
	QUARK_TEST_VERIFY(true);
//	QUARK_TEST_VERIFY(s2 == "<struct>{<string>x,<struct_1>y,<string>z}");
}






