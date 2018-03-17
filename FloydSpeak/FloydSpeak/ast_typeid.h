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

			//All above are included in code-types. They don't need further info.
			k_core_count,

		k_struct,
		k_vector,
		k_dict,
		k_function,

		//	We have an identifier, like "pixel" or "print" but haven't resolved it to an actual type yet.
		k_unresolved_type_identifier
	};

	std::string base_type_to_string(const base_type t);


	struct struct_definition_t;

	struct typeid_ext_imm_t {
		public: bool operator==(const typeid_ext_imm_t& other) const{
			return _parts == other._parts && _unresolved_type_identifier == other._unresolved_type_identifier && compare_shared_values(_struct_def, other._struct_def);
		}


		public: const std::vector<typeid_t> _parts;

		//	Used for k_unresolved_type_identifier.
		public: const std::string _unresolved_type_identifier;

		public: const std::shared_ptr<const struct_definition_t> _struct_def;
	};


	//////////////////////////////////////		typeid_t



	struct typeid_t {

		public: static typeid_t make_null(){
			return { floyd::base_type::k_null, {} };
		}

		public: bool is_null() const {
			QUARK_ASSERT(check_invariant());

			return _base_type == floyd::base_type::k_null;
		}

		public: static typeid_t make_bool(){
			return { floyd::base_type::k_bool, {} };
		}

		public: bool is_bool() const {
			QUARK_ASSERT(check_invariant());

			return _base_type == base_type::k_bool;
		}

		public: static typeid_t make_int(){
			return { floyd::base_type::k_int, {} };
		}

		public: bool is_int() const {
			QUARK_ASSERT(check_invariant());

			return _base_type == base_type::k_int;
		}

		public: static typeid_t make_float(){
			return { floyd::base_type::k_float, {} };
		}

		public: bool is_float() const {
			QUARK_ASSERT(check_invariant());

			return _base_type == base_type::k_float;
		}

		public: static typeid_t make_string(){
			return { floyd::base_type::k_string, {} };
		}

		public: bool is_string() const {
			QUARK_ASSERT(check_invariant());

			return _base_type == base_type::k_string;
		}

		public: static typeid_t make_json_value(){
			return { floyd::base_type::k_json_value, {} };
		}

		public: bool is_json_value() const {
			QUARK_ASSERT(check_invariant());

			return _base_type == base_type::k_json_value;
		}

		public: static typeid_t make_typeid(){
			return { floyd::base_type::k_typeid, {} };
		}

		public: bool is_typeid() const {
			QUARK_ASSERT(check_invariant());

			return _base_type == base_type::k_typeid;
		}

		public: static typeid_t make_struct(const std::shared_ptr<const struct_definition_t>& def){
			const auto ext = std::make_shared<const typeid_ext_imm_t>(typeid_ext_imm_t{ {}, "", def });
			return { floyd::base_type::k_struct, ext };
		}

		public: bool is_struct() const {
			QUARK_ASSERT(check_invariant());

			return _base_type == base_type::k_struct;
		}

		public: const struct_definition_t& get_struct() const{
			QUARK_ASSERT(get_base_type() == base_type::k_struct);

			return *_ext->_struct_def;
		}
		public: const std::shared_ptr<const struct_definition_t>& get_struct_ref() const{
			QUARK_ASSERT(get_base_type() == base_type::k_struct);

			return _ext->_struct_def;
		}


		public: static typeid_t make_vector(const typeid_t& element_type){
			const auto ext = std::make_shared<const typeid_ext_imm_t>(typeid_ext_imm_t{ { element_type }, "", {} });
			return { floyd::base_type::k_vector, ext };
		}

		public: bool is_vector() const {
			QUARK_ASSERT(check_invariant());

			return _base_type == base_type::k_vector;
		}

		public: const typeid_t& get_vector_element_type() const{
			QUARK_ASSERT(get_base_type() == base_type::k_vector);

			return _ext->_parts[0];
		}



		public: static typeid_t make_dict(const typeid_t& value_type){
			const auto ext = std::make_shared<const typeid_ext_imm_t>(typeid_ext_imm_t{ { value_type }, "", {} });
			return { floyd::base_type::k_dict, ext };
		}

		public: bool is_dict() const {
			QUARK_ASSERT(check_invariant());

			return _base_type == base_type::k_dict;
		}

		public: const typeid_t& get_dict_value_type() const{
			QUARK_ASSERT(get_base_type() == base_type::k_dict);

			return _ext->_parts[0];
		}


		public: static typeid_t make_function(const typeid_t& ret, const std::vector<typeid_t>& args){
			//	Functions use _parts[0] for return type always. _parts[1] is first argument, if any.
			std::vector<typeid_t> parts = { ret };
			parts.insert(parts.end(), args.begin(), args.end());
			const auto ext = std::make_shared<const typeid_ext_imm_t>(typeid_ext_imm_t{ parts, "", {} });

			return { floyd::base_type::k_function, ext };
		}

		public: bool is_function() const {
			QUARK_ASSERT(check_invariant());

			return _base_type == base_type::k_function;
		}

		public: typeid_t get_function_return() const{
			QUARK_ASSERT(get_base_type() == base_type::k_function);

			return _ext->_parts[0];
		}
		public: std::vector<typeid_t> get_function_args() const{
			QUARK_ASSERT(check_invariant());
			QUARK_ASSERT(get_base_type() == base_type::k_function);

			auto r = _ext->_parts;
			r.erase(r.begin());
			return r;
		}

		public: static typeid_t make_unresolved_type_identifier(const std::string& s){
			const auto ext = std::make_shared<const typeid_ext_imm_t>(typeid_ext_imm_t{ {}, s, {} });
			return { floyd::base_type::k_unresolved_type_identifier, ext };
		}

		public: bool is_unresolved_type_identifier() const {
			QUARK_ASSERT(check_invariant());

			return _base_type == base_type::k_unresolved_type_identifier;
		}

		public: std::string get_unresolved_type_identifier() const{
			QUARK_ASSERT(check_invariant());
			QUARK_ASSERT(get_base_type() == base_type::k_unresolved_type_identifier);

			return _ext->_unresolved_type_identifier;
		}

/*
		public: typeid_t(const typeid_t& other) :
			_DEBUG(other._DEBUG),
			_base_type(other._base_type),
			_ext(other._ext)
		{
//			for(const auto e: parts){ QUARK_ASSERT(e.check_invariant()); }
//			QUARK_ASSERT(struct_def == nullptr || struct_def->check_invariant());

			_DEBUG = typeid_to_compact_string(*this);

			QUARK_ASSERT(check_invariant());
		}
*/



		public: bool operator==(const typeid_t& other) const{
			QUARK_ASSERT(check_invariant());
			QUARK_ASSERT(other.check_invariant());

			return true
#if DEBUG
				&& _DEBUG == other._DEBUG
#endif
				&& _base_type == other._base_type
				&& compare_shared_values(_ext, other._ext);
		}
		public: bool operator!=(const typeid_t& other) const{ return !(*this == other);}

		public: bool check_invariant() const;

		public: void swap(typeid_t& other);


		public: floyd::base_type get_base_type() const{
			return _base_type;
		}

		private: typeid_t(floyd::base_type base_type, const std::shared_ptr<const typeid_ext_imm_t>& ext) :
			_base_type(base_type),
			_ext(ext)
		{
//			for(const auto e: parts){ QUARK_ASSERT(e.check_invariant()); }
//			QUARK_ASSERT(struct_def == nullptr || struct_def->check_invariant());

#if DEBUG
			_DEBUG = typeid_to_compact_string(*this);
#endif
			QUARK_ASSERT(check_invariant());
		}


		/////////////////////////////		STATE
#if DEBUG
		private: std::string _DEBUG;
#endif
		private: floyd::base_type _base_type;
		private: std::shared_ptr<const typeid_ext_imm_t> _ext;
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

	/*
		int -> "int"
		[int] -> [ "vector", "int" ]
	*/
	ast_json_t typeid_to_ast_json(const typeid_t& t);
	typeid_t typeid_from_ast_json(const ast_json_t& t);

	std::string typeid_to_compact_string(const typeid_t& t);


	std::vector<json_t> typeids_to_json_array(const std::vector<typeid_t>& m);
	std::vector<typeid_t> typeids_from_json_array(const std::vector<json_t>& m);


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




	//////////////////////////////////////		interned_typeids_t





	struct itypeid_t;

	struct interned_typeids_t {
		//	Index is used as the intern-id.
		//	Never contains duplicates.
		std::vector<typeid_t> _interns;


		interned_typeids_t() :
			_interns((int)base_type::k_core_count, typeid_t::make_null())
		{
			_interns[(int)base_type::k_null] = typeid_t::make_null();
			_interns[(int)base_type::k_bool] = typeid_t::make_bool();
			_interns[(int)base_type::k_int] = typeid_t::make_int();
			_interns[(int)base_type::k_float] = typeid_t::make_float();
			_interns[(int)base_type::k_string] = typeid_t::make_string();
			_interns[(int)base_type::k_json_value] = typeid_t::make_json_value();
			_interns[(int)base_type::k_typeid] = typeid_t::make_typeid();
		}

		public: int intern_typeid(const typeid_t& type){
			const auto it = std::find_if(_interns.begin(), _interns.end(), [&type](const typeid_t& e) { return e == type; });
			if(it != _interns.end()){
				return static_cast<int>(it - _interns.begin());
			}
			else{
				_interns.push_back(type);
				return static_cast<int>(_interns.size() - 1 + 10000);
			}
		}
	};

	struct itypeid_t {
		public: bool operator==(const itypeid_t& other){
			return _intern_id == other._intern_id;
		}

		public: static itypeid_t make_null(){
			return { (int)base_type::k_null };
		}
		public: static itypeid_t make_bool(){
			return { (int)base_type::k_bool };
		}
		public: static itypeid_t make_int(){
			return { (int)base_type::k_int };
		}
		public: static itypeid_t make_float(){
			return { (int)base_type::k_float };
		}
		public: static itypeid_t make_string(){
			return { (int)base_type::k_string };
		}
		public: static itypeid_t make_json_value(){
			return { (int)base_type::k_json_value };
		}
		public: static itypeid_t make_typeid(){
			return { (int)base_type::k_typeid };
		}



		public: int _intern_id;
	};

	inline const typeid_t& resolve(const interned_typeids_t& interned, const itypeid_t& type){
		const auto& i = interned._interns[type._intern_id];
		return i;
	}
	inline itypeid_t intern_typeid(interned_typeids_t& interned, const typeid_t& type){
		const auto i = interned.intern_typeid(type);
		return itypeid_t{i};
	}


}

#endif /* ast_typeid_hpp */
