//
//  ast_typeid.hpp
//  FloydSpeak
//
//  Created by Marcus Zetterquist on 2018-02-12.
//  Copyright © 2018 Marcus Zetterquist. All rights reserved.
//

#ifndef ast_typeid_hpp
#define ast_typeid_hpp

/*
	# base_type

	An enum with all the base-types of types. Int, struct etc.
	Struct needs more information to define its type but Int is done.


	# typeid_t

	Specifies an exact Floyd type. Both for base types like "int" and "string" and composite types like "struct { [float] p; string s }
	It can hold *any Floyd type*. It can also hold unresolved type identifiers and a few types internal to compiler.

	typeid_t can be convert to/from JSON and is written in source code according to Floyd source syntax, see table below.

	Immutable value object.
	The values are normalized and can be compared.
	Composite types can form trees of types,
		like:
			"[string: struct {int x; int y}]"
		This is a dictionary with structs, each holding two integers.


	Source code						base_type								AST JSON
	================================================================================================================
	null							k_internal_undefined					"null"
	bool							k_bool									"bool"
	int								k_int									"int"
	double							k_double								"double"
	string							k_string								"string"
	json_value						k_json_value							"json_value"
	"typeid"						k_typeid								"typeid"
	struct red { int x;float y}		k_struct								["struct", [{"type": "in", "name": "x"}, {"type": "float", "name": "y"}]]
	protocol reader {
		[int] read()
		int get_size()
  	}								k_protocol								["protocol", [{"type": ["function", ["vector, "int"]]], "name": "read"}, {"type": "["function", []]", "name": "get_size"}]]
	[int]							k_vector								["vector", "int"]
	[string: int]					k_dict									["dict", "int"]
	int ()							k_function								["function", "int", []]
	int (double, [string])			k_function								["function", "int", ["double", ["vector", "string"]]]
	randomize_player			k_internal_unresolved_type_identifier		["internal_unresolved_type_identifier", "randomize_player"]



	AST JSON
	This is the JSON format we use to pass AST around. Use typeid_to_ast_json() and typeid_from_ast_json().

	COMPACT_STRING
	This is a nice user-visible representation of the typeid_t. It may be lossy. It's for REPLs etc. UI.

	SOURCE CODE TYPE
	Use read_type(), read_required_type()
*/

#include "quark.h"
#include "utils.h"

#include <string>
#include <vector>
#include <variant>
#include "floyd_syntax.h"


namespace floyd {
	struct struct_definition_t;
	struct ast_json_t;
	struct typeid_t;
	struct member_t;


	const char tag_unresolved_type_char = '#';
	const char tag_resolved_type_char = '^';


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
		public: bool check_types_resolved() const;


		public: std::vector<member_t> _members;
	};

	std::string to_compact_string(const struct_definition_t& v);


	//	Returns -1 if not found.
	int find_struct_member_index(const struct_definition_t& def, const std::string& name);



	//////////////////////////////////////////////////		protocol_definition_t

	//	Protocol members are all function protoypes only.

	struct protocol_definition_t {
		public: protocol_definition_t(const std::vector<member_t>& members) :
			_members(members)
		{
			QUARK_ASSERT(check_invariant());
		}
		public: bool check_invariant() const;
		public: bool operator==(const protocol_definition_t& other) const;
		public: bool check_types_resolved() const;


		public: std::vector<member_t> _members;
	};

	std::string to_compact_string(const protocol_definition_t& v);

	




	enum class epure {
		pure,
		impure
	};


	//////////////////////////////////////		typeid_ext_imm_t


	//	Stores extra information for those types that need more than just a base_type.

	struct typeid_ext_imm_t {
		public: bool operator==(const typeid_ext_imm_t& other) const{
			return true
				&& _parts == other._parts
				&& _unresolved_type_identifier == other._unresolved_type_identifier
				&& compare_shared_values(_struct_def, other._struct_def)
				&& compare_shared_values(_protocol_def, other._protocol_def)
				&& _pure == other._pure
				;
		}


		public: const std::vector<typeid_t> _parts;

		//	Used for k_internal_unresolved_type_identifier.
		public: const std::string _unresolved_type_identifier;

		public: const std::shared_ptr<const struct_definition_t> _struct_def;
		public: const std::shared_ptr<const protocol_definition_t> _protocol_def;
		public: epure _pure;
	};



	//////////////////////////////////////		typeid_t



	struct typeid_t {

		public: static typeid_t make_undefined(){
			return { floyd::base_type::k_internal_undefined, {} };
		}

		public: bool is_undefined() const {
			QUARK_ASSERT(check_invariant());

			return _base_type == floyd::base_type::k_internal_undefined;
		}


		public: static typeid_t make_internal_dynamic(){
			return { floyd::base_type::k_internal_dynamic, {} };
		}

		public: bool is_internal_dynamic() const {
			QUARK_ASSERT(check_invariant());

			return _base_type == floyd::base_type::k_internal_dynamic;
		}


		public: static typeid_t make_void(){
			return { floyd::base_type::k_void, {} };
		}

		public: bool is_void() const {
			QUARK_ASSERT(check_invariant());

			return _base_type == floyd::base_type::k_void;
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


		public: static typeid_t make_double(){
			return { floyd::base_type::k_double, {} };
		}

		public: bool is_double() const {
			QUARK_ASSERT(check_invariant());

			return _base_type == base_type::k_double;
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


		public: static typeid_t make_struct1(const std::shared_ptr<const struct_definition_t>& def){
			QUARK_ASSERT(def);

			const auto ext = std::make_shared<const typeid_ext_imm_t>(typeid_ext_imm_t{ {}, "", def, {}, epure::pure});
			return { floyd::base_type::k_struct, ext };
		}

		public: static typeid_t make_struct2(const std::vector<member_t>& members){
			auto def = std::make_shared<const struct_definition_t>(members);
			const auto ext = std::make_shared<const typeid_ext_imm_t>(typeid_ext_imm_t{ {}, "", def, {}, epure::pure});
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



		public: static typeid_t make_protocol(const std::vector<member_t>& members){
			const auto def = std::make_shared<protocol_definition_t>(protocol_definition_t(members));
			const auto ext = std::make_shared<const typeid_ext_imm_t>(typeid_ext_imm_t{ {}, "", {}, def, epure::pure });
			return { floyd::base_type::k_protocol, ext };
		}

		public: bool is_protocol() const {
			QUARK_ASSERT(check_invariant());

			return _base_type == base_type::k_protocol;
		}

		public: const protocol_definition_t& get_protocol() const{
			QUARK_ASSERT(get_base_type() == base_type::k_protocol);
			QUARK_ASSERT(_ext->_protocol_def);
			return *_ext->_protocol_def;
		}
		public: const std::shared_ptr<const protocol_definition_t>& get_protocol_ref() const{
			QUARK_ASSERT(get_base_type() == base_type::k_protocol);

			return _ext->_protocol_def;
		}



		public: static typeid_t make_vector(const typeid_t& element_type){
			const auto ext = std::make_shared<const typeid_ext_imm_t>(typeid_ext_imm_t{ { element_type }, "", {}, {}, epure::pure });
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
			const auto ext = std::make_shared<const typeid_ext_imm_t>(typeid_ext_imm_t{ { value_type }, "", {}, {}, epure::pure });
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



		public: static typeid_t make_function(const typeid_t& ret, const std::vector<typeid_t>& args, epure pure){
			//	Functions use _parts[0] for return type always. _parts[1] is first argument, if any.
			std::vector<typeid_t> parts = { ret };
			parts.insert(parts.end(), args.begin(), args.end());
			const auto ext = std::make_shared<const typeid_ext_imm_t>(typeid_ext_imm_t{ parts, "", {}, {}, pure});

			return { floyd::base_type::k_function, ext };
		}

		public: bool is_function() const {
			QUARK_ASSERT(check_invariant());

			return _base_type == base_type::k_function;
		}

		public: const typeid_t& get_function_return() const{
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

		public: epure get_function_pure() const {
			QUARK_ASSERT(check_invariant());
			QUARK_ASSERT(get_base_type() == base_type::k_function);

			return _ext->_pure;
		}


		public: static typeid_t make_unresolved_type_identifier(const std::string& s){
			const auto ext = std::make_shared<const typeid_ext_imm_t>(typeid_ext_imm_t{ {}, s, {}, {}, epure::pure});
			return { floyd::base_type::k_internal_unresolved_type_identifier, ext };
		}

		public: bool is_unresolved_type_identifier() const {
			QUARK_ASSERT(check_invariant());

			return _base_type == base_type::k_internal_unresolved_type_identifier;
		}

		public: std::string get_unresolved_type_identifier() const{
			QUARK_ASSERT(check_invariant());
			QUARK_ASSERT(get_base_type() == base_type::k_internal_unresolved_type_identifier);

			return _ext->_unresolved_type_identifier;
		}




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


		public: bool check_types_resolved() const;

		public: inline floyd::base_type get_base_type() const{
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

	std::string typeid_to_compact_string(const typeid_t& t);




	//////////////////////////////////////		dynamic function



	//	A dynamic function has one or several arguments of type k_internal_dynamic.
	//	The argument types for k_internal_dynamic arguments must be supplied for each function invocation.
	bool is_dynamic_function(const typeid_t& function_type);

	int count_function_dynamic_args(const std::vector<typeid_t>& args);
	int count_function_dynamic_args(const typeid_t& function_type);




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
}

#endif /* ast_typeid_hpp */
