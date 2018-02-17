//
//  ast_typeid.hpp
//  FloydSpeak
//
//  Created by Marcus Zetterquist on 2018-02-12.
//  Copyright © 2018 Marcus Zetterquist. All rights reserved.
//

#ifndef ast_typeid_hpp
#define ast_typeid_hpp

#include "quark.h"
#include "utils.h"

#include <string>
#include <vector>

struct json_t;

namespace floyd {

	struct ast_json_t;
	struct typeid_t;
	struct member_t;
	std::string typeid_to_compact_string(const typeid_t& t);

	//////////////////////////////////////		base_type

	/*
		This type is tracked by compiler, not stored in the value-type.
	*/
	enum class base_type {
		k_null,
		k_bool,
		k_int,
		k_float,
		k_string,
		k_json_value,

		//	This is a type that specifies another type.
		k_typeid,

		k_struct,
		k_vector,
		k_dict,
		k_function,

		//	We have an identifier, like "pixel" or "print" but haven't resolved it to an actual type yet.
		k_unresolved_type_identifier
	};

	std::string base_type_to_string(const base_type t);



	//////////////////////////////////////		typeid_t


	struct struct_definition_t;

	struct typeid_t {

		public: static typeid_t make_null(){
			return { floyd::base_type::k_null, {}, {}, {}, {} };
		}

		public: bool is_null() const {
			QUARK_ASSERT(check_invariant());

			return _base_type == floyd::base_type::k_null;
		}

		public: static typeid_t make_bool(){
			return { floyd::base_type::k_bool, {}, {}, {}, {} };
		}

		public: bool is_bool() const {
			QUARK_ASSERT(check_invariant());

			return _base_type == base_type::k_bool;
		}

		public: static typeid_t make_int(){
			return { floyd::base_type::k_int, {}, {}, {}, {} };
		}

		public: bool is_int() const {
			QUARK_ASSERT(check_invariant());

			return _base_type == base_type::k_int;
		}

		public: static typeid_t make_float(){
			return { floyd::base_type::k_float, {}, {}, {}, {} };
		}

		public: bool is_float() const {
			QUARK_ASSERT(check_invariant());

			return _base_type == base_type::k_float;
		}

		public: static typeid_t make_string(){
			return { floyd::base_type::k_string, {}, {}, {}, {} };
		}

		public: bool is_string() const {
			QUARK_ASSERT(check_invariant());

			return _base_type == base_type::k_string;
		}

		public: static typeid_t make_json_value(){
			return { floyd::base_type::k_json_value, {}, {}, {}, {} };
		}

		public: bool is_json_value() const {
			QUARK_ASSERT(check_invariant());

			return _base_type == base_type::k_json_value;
		}

		public: static typeid_t make_typeid(){
			return { floyd::base_type::k_typeid, {}, {}, {}, {} };
		}

		public: bool is_typeid() const {
			QUARK_ASSERT(check_invariant());

			return _base_type == base_type::k_typeid;
		}

		public: static typeid_t make_struct(const std::shared_ptr<struct_definition_t>& def){
			return { floyd::base_type::k_struct, {}, {}, {}, def };
		}

		public: bool is_struct() const {
			QUARK_ASSERT(check_invariant());

			return _base_type == base_type::k_struct;
		}

		public: const struct_definition_t& get_struct() const{
			QUARK_ASSERT(get_base_type() == base_type::k_struct);

			return *_struct_def;
		}


		public: static typeid_t make_vector(const typeid_t& element_type){
			return { floyd::base_type::k_vector, { element_type }, {}, {}, {} };
		}

		public: bool is_vector() const {
			QUARK_ASSERT(check_invariant());

			return _base_type == base_type::k_vector;
		}

		public: const typeid_t& get_vector_element_type() const{
			QUARK_ASSERT(get_base_type() == base_type::k_vector);

			return _parts[0];
		}



		public: static typeid_t make_dict(const typeid_t& value_type){
			return { floyd::base_type::k_dict, { value_type }, {}, {}, {} };
		}

		public: bool is_dict() const {
			QUARK_ASSERT(check_invariant());

			return _base_type == base_type::k_dict;
		}

		public: const typeid_t& get_dict_value_type() const{
			QUARK_ASSERT(get_base_type() == base_type::k_dict);

			return _parts[0];
		}



		public: static typeid_t make_function(const typeid_t& ret, const std::vector<typeid_t>& args){
			//	Functions use _parts[0] for return type always. _parts[1] is first argument, if any.
			std::vector<typeid_t> parts = { ret };
			parts.insert(parts.end(), args.begin(), args.end());
			return { floyd::base_type::k_function, parts, {}, {}, {} };
		}

		public: bool is_function() const {
			QUARK_ASSERT(check_invariant());

			return _base_type == base_type::k_function;
		}

		public: typeid_t get_function_return() const{
			QUARK_ASSERT(get_base_type() == base_type::k_function);

			return _parts[0];
		}
		public: std::vector<typeid_t> get_function_args() const{
			QUARK_ASSERT(get_base_type() == base_type::k_function);

			auto r = _parts;
			r.erase(r.begin());
			return r;
		}


		public: static typeid_t make_unresolved_type_identifier(const std::string& s){
			return { floyd::base_type::k_unresolved_type_identifier, {}, {}, s, {} };
		}

		public: bool is_unresolved_type_identifier() const {
			QUARK_ASSERT(check_invariant());

			return _base_type == base_type::k_unresolved_type_identifier;
		}

		public: std::string get_unresolved_type_identifier() const{
			QUARK_ASSERT(get_base_type() == base_type::k_unresolved_type_identifier);

			return _unresolved_type_identifier;
		}



		public: bool operator==(const typeid_t& other) const{
			return
				_DEBUG == other._DEBUG
				&& _base_type == other._base_type
				&& _parts == other._parts
				&& _unresolved_type_identifier == other._unresolved_type_identifier
				&& compare_shared_values(_struct_def, other._struct_def);
		}
		public: bool operator!=(const typeid_t& other) const{ return !(*this == other);}

		public: bool check_invariant() const;

		public: void swap(typeid_t& other);


		public: floyd::base_type get_base_type() const{
			return _base_type;
		}


		private: typeid_t(
			floyd::base_type base_type,
			const std::vector<typeid_t>& parts,
			const std::string& unique_type_id,
			const std::string& unknown_identifier,
			const std::shared_ptr<struct_definition_t>& struct_def
		):
			_base_type(base_type),
			_parts(parts),
			_unresolved_type_identifier(unknown_identifier),
			_struct_def(struct_def)
		{
			_DEBUG = typeid_to_compact_string(*this);
		}


		/////////////////////////////		STATE
		private: std::string _DEBUG;
		private: floyd::base_type _base_type;
		private: std::vector<typeid_t> _parts;

		//	Used for k_unresolved_type_identifier.
		private: std::string _unresolved_type_identifier;

		//??? Add path to environment when struct was defined = make it unique.
		private: std::shared_ptr<struct_definition_t> _struct_def;
	};



	//////////////////////////////////////		FORMATS

	/*
		typeid_t --- formats: json, source code etc.

		in-code						base					More								notes
		================================================================================================================
		null						k_null
		bool						k_bool
		int							k_int
		float						k_float
		string						k_string
		json_value					k_json_value


		-							k_typeid				[target type id]

		struct red { int x;}		k_struct				struct_definition_t (name = "red", { "x", k_int })

		[int]						k_vector				k_int
		[string: int]				k_dict					k_int

		int ()						k_function				return = k_int, args = []

		int (float, [string])		k_function				return = k_int, args = [ k_float, typeid_t(k_vector, string) ]

		randomize_player			k_unresolved_type_identifier	"randomize_player"
		- When parsing we find identifiers that we don't know what they mean. Stored as k_unresolved_type_identifier with identifier




		AST JSON
		This is the JSON format we use to pass AST around. Use typeid_to_ast_json() and typeid_from_ast_json().

		COMPACT_STRING
		This is a nice user-visible representation of the typeid_t. It may be lossy. It's for REPLs etc. UI.


		SOURCE CODE TYPE
		Use read_required_type()


		??? Remove concept of typeid_t make_unresolved_type_identifier, instead use typeid_t OR identifier-string.
	*/

	ast_json_t typeid_to_ast_json(const typeid_t& t);
	typeid_t typeid_from_ast_json(const ast_json_t& t);

	std::string typeid_to_compact_string(const typeid_t& t);


	std::vector<json_t> typeids_to_json_array(const std::vector<typeid_t>& m);
	std::vector<typeid_t> typeids_from_json_array(const std::vector<json_t>& m);



	//////////////////////////////////////////////////		struct_definition_t


	struct struct_definition_t {
		public: struct_definition_t(const std::vector<member_t>& members) :
			_members(members)
		{
			QUARK_ASSERT(check_invariant());
		}
		public: bool check_invariant() const;
		public: bool operator==(const struct_definition_t& other) const;


		public: std::vector<member_t> _members;
	};

	std::string to_compact_string(const struct_definition_t& v);

	ast_json_t struct_definition_to_ast_json(const struct_definition_t& v);

	//	Returns -1 if not found.
	int find_struct_member_index(const struct_definition_t& def, const std::string& name);




	//////////////////////////////////////		member_t

	/*
		Definition of a struct-member.
	*/

	struct member_t {
		public: member_t(const floyd::typeid_t& type, const std::string& name);
		bool operator==(const member_t& other) const;
		public: bool check_invariant() const;


		/////////////////////////////		STATE
		public: floyd::typeid_t _type;

		public: std::string _name;
	};


	std::vector<floyd::typeid_t> get_member_types(const std::vector<member_t>& m);
	json_t members_to_json(const std::vector<member_t>& members);
	std::vector<member_t> members_from_json(const json_t& members);

}

#endif /* ast_typeid_hpp */
