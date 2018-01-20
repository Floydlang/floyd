//
//  floyd_basics.hpp
//  FloydSpeak
//
//  Created by Marcus Zetterquist on 2017-08-09.
//  Copyright © 2017 Marcus Zetterquist. All rights reserved.
//

#ifndef floyd_basics_hpp
#define floyd_basics_hpp

#include <string>
#include <vector>

#include "json_support.h"
#include "utils.h"


namespace floyd {
	struct value_t;
	struct typeid_t;
	std::string typeid_to_compact_string(const typeid_t& t);


	struct keyword_t {
		static const std::string k_return;
		static const std::string k_while;
		static const std::string k_if;
		static const std::string k_else;

		static const std::string k_false;
		static const std::string k_true;
		static const std::string k_bool;

		static const std::string k_int;
		static const std::string k_float;
		static const std::string k_string;
		static const std::string k_struct;
/*
		"mutable",
		"null",

		const std::string keyword_t::k_typeid = "typeid";

		"assert",
		"catch",

		"print",
		"to_string",
		"update",
		"size",

*/

/*
		"deserialize()",
		"diff()",
		"ensure",
		"foreach",
		"hash()",
		"invariant",
		"log",
		"namespace",
		"private",
		"property",
		"prove",
		"require",
		"serialize()",
		"swap",
		"switch",
		"tag",
		"test",
		"this",
		"try",
		"typecast",
		"typeof",
*/
	};

/*
	const std::vector<std::string> basic_types {
		"char",
		"-code_point",
		"-double",
		"float32",
		"float80",
		"-hash",
		"int16",
		"int32",
		"int64",
		"int8",
		"-path",
		"-text"
	};
	const std::vector<std::string> advanced_types {
		"-clock",
		"-defect_exception",
		"-dyn",
		"-dyn**<>",
		"-enum",
		"-exception",
		"map",
		"-protocol",
		"-rights",
		"-runtime_exception",
		"seq",
		"-typedef",
	};
*/


	//////////////////////////////////////		base_type



	//??? Split and categories better. Logic vs equality vs math.

	//	Number at end of name tells number of input expressions operation has.
	enum class expression_type {

		//	c99: a + b			token: "+"
		k_arithmetic_add__2 = 10,

		//	c99: a - b			token: "-"
		k_arithmetic_subtract__2,

		//	c99: a * b			token: "*"
		k_arithmetic_multiply__2,

		//	c99: a / b			token: "/"
		k_arithmetic_divide__2,

		//	c99: a % b			token: "%"
		k_arithmetic_remainder__2,


		//	c99: a <= b			token: "<="
		k_comparison_smaller_or_equal__2,

		//	c99: a < b			token: "<"
		k_comparison_smaller__2,

		//	c99: a >= b			token: ">="
		k_comparison_larger_or_equal__2,

		//	c99: a > b			token: ">"
		k_comparison_larger__2,


		//	c99: a == b			token: "=="
		k_logical_equal__2,

		//	c99: a != b			token: "!="
		k_logical_nonequal__2,


		//	c99: a && b			token: "&&"
		k_logical_and__2,

		//	c99: a || b			token: "||"
		k_logical_or__2,

		//	c99: !a				token: "!"
//			k_logical_not,

		//	c99: 13				token: "k"
		k_literal,

		//	c99: -a				token: "unary_minus"
		k_arithmetic_unary_minus__1,

		//	c99: cond ? a : b	token: "?:"
		k_conditional_operator3,

		//	c99: a(b, c)		token: "call"
		k_call,

		//	c99: a				token: "@"
		k_variable,

		//	c99: a.b			token: "->"
		k_resolve_member,

		//	c99: a[b]			token: "[]"
		k_lookup_element,

		//???	use k_literal for function values?
		k_define_function,

		k_vector_definition
	};

	expression_type token_to_expression_type(const std::string& op);
	std::string expression_type_to_token(const expression_type& op);



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

		//	This is a type that specifies another type.
		k_typeid,

		k_struct,
		k_vector,
		k_function,

		//	We have an identifier, like "pixel" or "print" but haven't resolved it to an actual type yet.
		k_unknown_identifier
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

		public: static typeid_t make_typeid(const typeid_t& type){
			return { floyd::base_type::k_typeid, { type }, {}, {}, {} };
		}

		public: bool is_typeid() const {
			QUARK_ASSERT(check_invariant());

			return _base_type == base_type::k_typeid;
		}

		public: const typeid_t& get_typeid_typeid() const{
			QUARK_ASSERT(get_base_type() == base_type::k_typeid);
			return _parts[0];
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


		public: static typeid_t make_unknown_identifier(const std::string& s){
			return { floyd::base_type::k_unknown_identifier, {}, {}, s, {} };
		}

		public: bool is_unknown_identifier() const {
			QUARK_ASSERT(check_invariant());

			return _base_type == base_type::k_unknown_identifier;
		}

		public: std::string get_unknown_identifier() const{
			QUARK_ASSERT(get_base_type() == base_type::k_unknown_identifier);

			return _unknown_identifier;
		}



		public: bool operator==(const typeid_t& other) const{
			return
				_DEBUG == other._DEBUG
				&& _base_type == other._base_type
				&& _parts == other._parts
				&& _unique_type_id == other._unique_type_id
				&& _unknown_identifier == other._unknown_identifier
				&& compare_shared_values(_struct_def, other._struct_def);
		}

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
			_unique_type_id(unique_type_id),
			_unknown_identifier(unknown_identifier),
			_struct_def(struct_def)
		{
			_DEBUG = typeid_to_compact_string(*this);
		}


		/////////////////////////////		STATE
		private: std::string _DEBUG;
		private: floyd::base_type _base_type;
		private: std::vector<typeid_t> _parts;
		private: std::string _unique_type_id;

		//	Used for k_unknown_identifier.
		private: std::string _unknown_identifier;

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


		-							k_typeid				[target type id]

		struct red { int x;}		k_struct				struct_definition_t (name = "red", { "x", k_int })

		[int]						k_vector				k_int

		int ()						k_function				return = k_int, args = []

		int (float, [string])		k_function				return = k_int, args = [ k_float, typeid_t(k_vector, string) ]

		randomize_player			k_unknown_identifier	"randomize_player"
		- When parsing we find identifiers that we don't know what they mean. Stored as k_unknown_identifier with identifier




		NORMALIZED JSON
		This is the JSON format we use to pass AST around. The normalized format means it supports roundtrip. Use
				typeid_to_normalized_json() and typeid_from_normalized_json().

		COMPACT_STRING
		This is a nice user-visible representation of the typeid_t. It may be lossy. It's for REPLs etc. UI.


		SOURCE CODE TYPE
		Use read_required_type()


		??? Remove concept of typeid_t make_unknown_identifier, instead use typeid_t OR identifier-string.
	*/

	json_t typeid_to_normalized_json(const typeid_t& t);
	typeid_t typeid_from_normalized_json(const json_t& t);

	std::string typeid_to_compact_string(const typeid_t& t);




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

	void trace(const member_t& member);


	std::vector<floyd::typeid_t> get_member_types(const std::vector<member_t>& m);
	json_t members_to_json(const std::vector<member_t>& members);
	std::vector<member_t> members_from_json(const json_t& members);




	json_t values_to_json_array(const std::vector<value_t>& values);


	std::vector<json_t> typeids_to_json_array(const std::vector<typeid_t>& m);
	std::vector<typeid_t> typeids_from_json_array(const std::vector<json_t>& m);



	//////////////////////////////////////////////////		struct_definition_t


	struct struct_definition_t {
		public: struct_definition_t(const std::string& name, const std::vector<member_t>& members) :
			_name(name),
			_members(members)
		{
			QUARK_ASSERT(name.size() > 0);
			QUARK_ASSERT(check_invariant());
		}
		public: bool check_invariant() const;
		public: bool operator==(const struct_definition_t& other) const;


		public: std::string _name;
		public: std::vector<member_t> _members;
	};

	std::string to_compact_string(const struct_definition_t& v);
	json_t typeid_to_normalized_json(const struct_definition_t& v);

	//	Returns -1 if not found.
	int find_struct_member_index(const struct_definition_t& def, const std::string& name);

}


#endif /* floyd_basics_hpp */
