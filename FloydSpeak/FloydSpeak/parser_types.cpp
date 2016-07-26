
//
//  parser_types.cpp
//  FloydSpeak
//
//  Created by Marcus Zetterquist on 24/07/16.
//  Copyright © 2016 Marcus Zetterquist. All rights reserved.
//

#include "parser_types.h"

#include "quark.h"
#include <vector>
#include <string>
#include <map>

#include "parser_primitives.h"


using std::make_shared;
using std::string;


namespace floyd_parser {














	//////////////////////////////////////////////////		type_identifier_t


	type_identifier_t type_identifier_t::make_type(std::string s){
		const type_identifier_t result(s);

		QUARK_ASSERT(result.check_invariant());
		return result;
	}

	bool type_identifier_t::operator==(const type_identifier_t& other) const{
		QUARK_ASSERT(check_invariant());
		QUARK_ASSERT(other.check_invariant());

		return other._type_magic == _type_magic;
	}

	bool type_identifier_t::operator!=(const type_identifier_t& other) const{
		return !(*this == other);
	}

/*
	type_identifier_t::type_identifier_t() :
		_type_magic("")
	{
		QUARK_ASSERT(check_invariant());
	}
*/

	type_identifier_t::type_identifier_t(const char s[]) :
		_type_magic(s)
	{
		QUARK_ASSERT(s != nullptr);

		QUARK_ASSERT(check_invariant());
	}

	type_identifier_t::type_identifier_t(const std::string& s) :
		_type_magic(s)
	{
		QUARK_ASSERT(check_invariant());
	}

	void type_identifier_t::swap(type_identifier_t& other){
		QUARK_ASSERT(check_invariant());
		QUARK_ASSERT(other.check_invariant());

		_type_magic.swap(other._type_magic);
	}

	std::string type_identifier_t::to_string() const {
		QUARK_ASSERT(check_invariant());

		return _type_magic;
	}

	bool type_identifier_t::check_invariant() const {
		QUARK_ASSERT(_type_magic != "");
//		QUARK_ASSERT(_type_magic == "" || _type_magic == "string" || _type_magic == "int" || _type_magic == "float" || _type_magic == "value_type");
		return true;
	}



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




	bool is_valid_type_identifier(const std::string& s){
		return true;
	}





	////////////////////////			member_t


	member_t::member_t(const std::string& name, const std::string& type_identifier) :
		_name(name),
		_type_identifier(type_identifier)
	{
		QUARK_ASSERT(!type_identifier.empty());

		QUARK_ASSERT(check_invariant());
	}

	bool member_t::check_invariant() const{
		QUARK_ASSERT(!_type_identifier.empty());
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
				QUARK_TRACE("<" + it._type_identifier + "> " + it._name);

//				trace_frontend_type(*it._type, it._name);
			}
		}
		else if(t._base_type == k_vector){
			QUARK_SCOPED_TRACE("<" + to_string(t._base_type) + "> " + label);
//			trace_frontend_type(*t._vector_def->_value_type, "");
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
//				const string member_type = typedef_s.empty() ? to_signature(*member._type) : "<" + typedef_s + ">";
				const string member_type = "<" + typedef_s + ">";

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
//			const auto vector_value_s = t._vector_def->_value_type_identifier.empty() ? to_signature(*t._vector_def->_value_type) : "<" + t._vector_def->_value_type_identifier + ">";
			const auto vector_value_s = "";
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

	std::vector<byte_range_t> calc_struct_default_memory_layout(const frontend_types_collector_t& types, const type_definition_t& s){
		QUARK_ASSERT(types.check_invariant());
		QUARK_ASSERT(s.check_invariant());

		std::vector<byte_range_t> result;
		std::size_t pos = 0;
		for(const auto& member : s._struct_def->_members) {
			const auto identifier_data = types.lookup_identifier_deep(member._type_identifier);
			const auto type_def = identifier_data->_optional_def;
			QUARK_ASSERT(type_def);

			if(type_def->_base_type == k_int32){
				pos = align_pos(pos, 4);
				result.push_back(byte_range_t(pos, 4));
				pos += 4;
			}
			else if(type_def->_base_type == k_bool){
				result.push_back(byte_range_t(pos, 1));
				pos += 1;
			}
			else if(type_def->_base_type == k_string){
				pos = align_pos(pos, 8);
				result.push_back(byte_range_t(pos, 8));
				pos += 8;
			}
			else if(type_def->_base_type == k_struct){
				pos = align_pos(pos, 8);
				result.push_back(byte_range_t(pos, 8));
				pos += 8;
			}
			else if(type_def->_base_type == k_vector){
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


	////////////////////////			frontend_types_collector_t


	/*
		Holds all types of program.
	*/

	frontend_types_collector_t::frontend_types_collector_t(){
		QUARK_ASSERT(check_invariant());


		//	int32
		{
			auto def = make_shared<type_definition_t>();
			def->_base_type = k_int32;
			QUARK_ASSERT(def->check_invariant());

			_identifiers["int32"] = { "", def };
			_type_definitions[to_signature(*def)] = def;
		}

		//	bool
		{
			auto def = make_shared<type_definition_t>();
			def->_base_type = k_bool;
			QUARK_ASSERT(def->check_invariant());

			_identifiers["bool"] = { "", def };
			_type_definitions[to_signature(*def)] = def;
		}

		//	string
		{
			auto def = make_shared<type_definition_t>();
			def->_base_type = k_string;
			QUARK_ASSERT(def->check_invariant());

			_identifiers["string"] = { "", def };
			_type_definitions[to_signature(*def)] = def;
		}
	}
	
	bool frontend_types_collector_t::check_invariant() const{
		for(const auto it: _identifiers){
			QUARK_ASSERT(it.first != "");
		}

		for(const auto it: _type_definitions){
			const auto signature = to_signature(*it.second);
			QUARK_ASSERT(it.first == signature);
			QUARK_ASSERT(it.second->check_invariant());
		}

		//	Make sure all types referenced from _identifiers are also stored inside _type_definition.
		for(const auto identifiers_it: _identifiers){
			if(identifiers_it.second._optional_def){
				auto defs_it = _type_definitions.begin();
				while(defs_it != _type_definitions.end() && defs_it->second != identifiers_it.second._optional_def){
					 defs_it++;
				}

				QUARK_ASSERT(defs_it != _type_definitions.end());
			}
		}
		return true;
	}

	frontend_types_collector_t frontend_types_collector_t::define_alias_identifier(const std::string& new_identifier, const std::string& existing_identifier) const{
		QUARK_ASSERT(check_invariant());
		QUARK_ASSERT(!new_identifier.empty());
		QUARK_ASSERT(!existing_identifier.empty());

		if(_identifiers.find(existing_identifier) == _identifiers.end()){
			throw std::runtime_error("unknown type identifier to base alias on");
		}

		if(is_type_identifier_fully_defined(new_identifier)){
			throw std::runtime_error("new type identifier already defined");
		}

		auto result = *this;
		result._identifiers[new_identifier] = { existing_identifier, {} };

		QUARK_ASSERT(result.check_invariant());
		return result;
	}


	frontend_types_collector_t frontend_types_collector_t::define_type_identifier(const std::string& new_identifier, const std::shared_ptr<type_definition_t>& type_def) const{
		QUARK_ASSERT(check_invariant());
		QUARK_ASSERT(!new_identifier.empty());

		//	### Be quite if existing identifier matches new one 100% == same type_def.
		if(is_type_identifier_fully_defined(new_identifier)){
			throw std::runtime_error("new type identifier already defined");
		}

		auto result = *this;
		result._identifiers[new_identifier] = { "", type_def };

		QUARK_ASSERT(result.check_invariant());
		return result;
	}


	std::pair<std::shared_ptr<type_definition_t>, frontend_types_collector_t> frontend_types_collector_t::define_struct_type(const std::vector<member_t>& members) const{
		QUARK_ASSERT(check_invariant());

		auto type_def = make_shared<type_definition_t>();
		type_def->_base_type = k_struct;
		type_def->_struct_def = make_shared<struct_def_t>();
		type_def->_struct_def->_members = members;

		const string signature = to_signature(*type_def);

		const auto existing_it = _type_definitions.find(signature);
		if(existing_it != _type_definitions.end()){
			return { existing_it->second, *this };
		}
		else{
			auto result = *this;
			result._type_definitions.insert(std::pair<std::string, std::shared_ptr<type_definition_t>>(signature, type_def));
			return { type_def, result };
		}
	}

	bool frontend_types_collector_t::is_type_identifier_fully_defined(const std::string& type_identifier) const{
		QUARK_ASSERT(check_invariant());
		QUARK_ASSERT(!type_identifier.empty());

		const auto existing_it = _identifiers.find(type_identifier);
		if(existing_it != _identifiers.end() && (!existing_it->second._alias_type_identifier.empty() || existing_it->second._optional_def)){
			return true;
		}
		else{
			return false;
		}
	}


	frontend_types_collector_t frontend_types_collector_t::define_struct_type(const std::string& new_identifier, std::vector<member_t> members) const{
		QUARK_ASSERT(check_invariant());

		//	Make struct def, if not already done.
		const auto a = frontend_types_collector_t::define_struct_type(members);
		const auto type_def = a.first;
		const auto collector2 = a.second;

		if(new_identifier.empty()){
			return collector2;
		}
		else{
			//	Make a type-identifier too.
			const auto collector3 = collector2.define_type_identifier(new_identifier, type_def);

			return collector3;
		}
	}

	std::shared_ptr<type_indentifier_data_ref> frontend_types_collector_t::lookup_identifier_shallow(const std::string& s) const{
		QUARK_ASSERT(check_invariant());

		const auto it = _identifiers.find(s);
		return it == _identifiers.end() ? std::shared_ptr<type_indentifier_data_ref>() : make_shared<type_indentifier_data_ref>(it->second);
	}

	std::shared_ptr<type_indentifier_data_ref> frontend_types_collector_t::lookup_identifier_deep(const std::string& s) const{
		QUARK_ASSERT(check_invariant());

		const auto it = _identifiers.find(s);
		if(it == _identifiers.end()){
			return {};
		}
		else {
			const auto alias = it->second._alias_type_identifier;
			if(!alias.empty()){
				return lookup_identifier_deep(alias);
			}
			else{
				return make_shared<type_indentifier_data_ref>(it->second);
			}
		}
	}

	std::shared_ptr<type_definition_t> frontend_types_collector_t::resolve_identifier(const std::string& s) const{
		QUARK_ASSERT(check_invariant());

		const auto identifier_data = lookup_identifier_deep(s);
		if(!identifier_data){
			return {};
		}
		else{
			return identifier_data->_optional_def;
		}
	}



	std::shared_ptr<type_definition_t> frontend_types_collector_t::lookup_signature(const std::string& s) const{
		QUARK_ASSERT(check_invariant());

		const auto it = _type_definitions.find(s);
		return it == _type_definitions.end() ? std::shared_ptr<type_definition_t>() : it->second;
	}


} //	floyd_parser;




using namespace floyd_parser;



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
frontend_types_collector_t define_test_struct_0(const frontend_types_collector_t& types){
	const auto a = types.define_struct_type("", {});
	return a;
}


/*
	struct struct_1 {
		int32 a
		string b
	}
*/
frontend_types_collector_t define_test_struct_1(const frontend_types_collector_t& types){
	const auto a = types.define_struct_type("struct_1",
		{
			member_t("a", "int32"),
			member_t("b", "string")
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
frontend_types_collector_t define_test_struct_2(const frontend_types_collector_t& types){
	const auto a = types.define_struct_type("struct_2",
		{
			member_t("x", "string"),
			member_t("y", "struct_1"),
			member_t("z", "string")
		}
	);
	return a;
}
//??? check for duplicate member names.
frontend_types_collector_t define_test_struct_3(const frontend_types_collector_t& types){
	const auto a = types.define_struct_type("struct_3",
		{
			member_t("a", "bool"),
			// pad
			// pad
			// pad
			member_t("b", "int32"),
			member_t("c", "bool"),
			member_t("d", "bool"),
			member_t("e", "bool"),
			member_t("f", "bool"),
			member_t("g", "string"),
			member_t("h", "bool")
		}
	);
	return a;
}


//////////////////////////////////////		to_signature()


QUARK_UNIT_TESTQ("to_signature()", "empty unnamed struct"){
	const auto a = frontend_types_collector_t();
	const auto b = define_test_struct_0(a);
	const auto t1 = b.lookup_signature("<struct>{}");
	QUARK_TEST_VERIFY(t1);
	QUARK_TEST_VERIFY(to_signature(*t1) == "<struct>{}");
}

QUARK_UNIT_TESTQ("to_signature()", "struct 1"){
	const auto a = frontend_types_collector_t();
	const auto b = define_test_struct_1(a);
	const auto t1 = b.resolve_identifier("struct_1");
	const auto s1 = to_signature(*t1);
	QUARK_TEST_VERIFY(s1 == "<struct>{<int32>a,<string>b}");
}


QUARK_UNIT_TESTQ("to_signature()", "struct 2"){
	const auto a = frontend_types_collector_t();
	const auto b = define_test_struct_1(a);
	const auto c = define_test_struct_2(b);
	const auto t2 = c.resolve_identifier("struct_2");
	const auto s2 = to_signature(*t2);
	QUARK_TEST_VERIFY(s2 == "<struct>{<string>x,<struct_1>y,<string>z}");
}



//////////////////////////////////////		frontend_types_collector_t



QUARK_UNIT_TESTQ("frontend_types_collector_t::frontend_types_collector_t()", "default construction"){
	const auto a = frontend_types_collector_t();
	QUARK_TEST_VERIFY(a.check_invariant());

	QUARK_TEST_VERIFY(a._identifiers.size() == 3);
	QUARK_TEST_VERIFY(a._type_definitions.size() == 3);

	const auto b = a.resolve_identifier("int32");
	QUARK_TEST_VERIFY(b);
	QUARK_TEST_VERIFY(b->_base_type == k_int32);

	const auto d = a.resolve_identifier("bool");
	QUARK_TEST_VERIFY(d);
	QUARK_TEST_VERIFY(d->_base_type == k_bool);

	const auto c = a.resolve_identifier("string");
	QUARK_TEST_VERIFY(c);
	QUARK_TEST_VERIFY(c->_base_type == k_string);
}


QUARK_UNIT_TESTQ("frontend_types_collector_t::resolve_identifier()", "not found"){
	const auto a = frontend_types_collector_t();
	const auto b = a.resolve_identifier("xyz");
	QUARK_TEST_VERIFY(!b);
}

QUARK_UNIT_TESTQ("frontend_types_collector_t::resolve_identifier()", "int32 found"){
	const auto a = frontend_types_collector_t();
	const auto b = a.resolve_identifier("int32");
	QUARK_TEST_VERIFY(b);
}


QUARK_UNIT_TESTQ("frontend_types_collector_t::define_alias_identifier()", "int32 => my_int"){
	auto a = frontend_types_collector_t();
	a = a.define_alias_identifier("my_int", "int32");
	const auto b = a.resolve_identifier("my_int");
	QUARK_TEST_VERIFY(b);
	QUARK_TEST_VERIFY(b->_base_type == k_int32);
}


QUARK_UNIT_TESTQ("calc_struct_default_memory_layout()", "struct 2"){
	const auto a = frontend_types_collector_t();
	const auto b = define_test_struct_3(a);
	const auto t = b.resolve_identifier("struct_3");
	const auto layout = calc_struct_default_memory_layout(a, *t);
	int i = 0;
	for(const auto it: layout){
		const string name = i == 0 ? "struct" : t->_struct_def->_members[i - 1]._name;
		QUARK_TRACE_SS(it.first << "--" << (it.first + it.second) << ": " + name);
		i++;
	}
	QUARK_TEST_VERIFY(true);
//	QUARK_TEST_VERIFY(s2 == "<struct>{<string>x,<struct_1>y,<string>z}");
}






