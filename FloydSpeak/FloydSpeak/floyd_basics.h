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

	const std::vector<std::string> basic_types {
		"bool",
		"char",
		"-code_point",
		"-double",
		"float",
		"float32",
		"float80",
		"-hash",
		"int",
		"int16",
		"int32",
		"int64",
		"int8",
		"-path",
		"string",
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
		"struct",
		"-typedef",
		"-vector"
	};

	const std::vector<std::string> keywords {
		"assert",
		"-catch",
		"-deserialize()",
		"-diff()",
		"else",
		"-ensure",
		"false",
		"foreach",
		"-hash()",
		"if",
		"-invariant",
		"log",
		"mutable",
		"-namespace",
		"-null",
		"-private",
		"-property",
		"-prove",
		"-require",
		"return",
		"-serialize()",
		"-swap",
		"-switch",
		"-tag",
		"-test",
		"-this",
		"true",
		"-try",
		"-typecast",
		"-typeof",
		"while"
	};


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

		//	c99: a[b]			token: "[-]"
		k_lookup_element,

		//???	use k_literal for function values?
		k_define_function
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

		k_typeid,

		k_struct,
		k_vector,
		k_function,

		k_unknown_identifier
	};

	std::string base_type_to_string(const base_type t);



	//////////////////////////////////////		typeid_t

	/*
		in-code						base					more									notes
		================================================================================================================
		bool						k_bool
		int							k_int
		float						k_float
		string						k_string

									k_typeid
		struct						k_struct				"coord_t/8000", struct_definition_t
		struct						k_struct_type			"coord_t/8000"

		[int]						k_vector				typeid_t(k_int)

		int ()
		int (float, [string])		k_function				k_int, k_float, k_vector

		randomize_player			k_unknown_identifier	"randomize_player"

		- When parsing we find identifiers that we don't know what they mean. Stored as k_unknown_identifier with identifier
	*/


	struct struct_definition_t;

	struct typeid_t {

		public: bool is_null() const {
			return _base_type == floyd::base_type::k_null;
		}

		public: static typeid_t make_null(){
			return { floyd::base_type::k_null, {}, {}, {}, {} };
		}

		public: static typeid_t make_bool(){
			return { floyd::base_type::k_bool, {}, {}, {}, {} };
		}

		public: static typeid_t make_int(){
			return { floyd::base_type::k_int, {}, {}, {}, {} };
		}

		public: static typeid_t make_float(){
			return { floyd::base_type::k_float, {}, {}, {}, {} };
		}

		public: static typeid_t make_string(){
			return { floyd::base_type::k_string, {}, {}, {}, {} };
		}

		public: static typeid_t make_typeid(const typeid_t& type){
			return { floyd::base_type::k_typeid, { type }, {}, {}, {} };
		}

		public: static typeid_t make_struct(const std::shared_ptr<struct_definition_t>& def){
			return { floyd::base_type::k_struct, {}, {}, {}, def };
		}

		public: static typeid_t make_vector(const typeid_t& element_type){
			return { floyd::base_type::k_vector, { element_type }, {}, {}, {} };
		}

		public: static typeid_t make_function(const typeid_t& ret, const std::vector<typeid_t>& args){
			//	Functions use _parts[0] for return type always. _parts[1] is first argument, if any.
			std::vector<typeid_t> parts = { ret };
			parts.insert(parts.end(), args.begin(), args.end());
			return { floyd::base_type::k_function, parts, {}, {}, {} };
		}

		public: static typeid_t make_unknown_identifier(const std::string& s){
			return { floyd::base_type::k_unknown_identifier, {}, {}, s, {} };
		}



		public: bool operator==(const typeid_t& other) const{
			return
				_base_type == other._base_type
				&& _parts == other._parts
				&& _unique_type_id == other._unique_type_id
				&& _unknown_identifier == other._unknown_identifier
				&& compare_shared_values(_struct_def, other._struct_def);
		}

		public: bool check_invariant() const;

		public: void swap(typeid_t& other);


		//	Supports non-lossy round trip between to_string() and from_string(). ??? make it so and test!
		//	Compatible with Floyd sources.

		/*
			"int"
			"[int]"
			"int f(float b)"
			"typeid(int)"

			### Store as compact JSON instead? Then we can't use [ and {".
		*/
		public: std::string to_string() const;
		public: static typeid_t from_string(const std::string& s);


		public: floyd::base_type get_base_type() const{
			return _base_type;
		}


		/////////////////////////////		STATE


		public: floyd::base_type _base_type;
		public: std::vector<typeid_t> _parts;
		public: std::string _unique_type_id;

		//	Used for k_unknown_identifier.
		public: std::string _unknown_identifier;

		//??? Add path to environment when struct was defined = make it unqiue.
		public: std::shared_ptr<struct_definition_t> _struct_def;
	};

	json_t typeid_to_json(const typeid_t& t);



	struct value_t;
	struct statement_t;


	//////////////////////////////////////		member_t

	/*
		Definition of a struct-member.
	*/

	struct member_t {
		public: member_t(const floyd::typeid_t& type, const std::string& name);
		public: member_t(const floyd::typeid_t& type, const std::shared_ptr<value_t>& value, const std::string& name);
		bool operator==(const member_t& other) const;
		public: bool check_invariant() const;


		/////////////////////////////		STATE
		public: floyd::typeid_t _type;

		//	Optional -- must have same type as _type.
		public: std::shared_ptr<const value_t> _value;

		public: std::string _name;
	};

	void trace(const member_t& member);


	void trace(const std::vector<std::shared_ptr<statement_t>>& e);

	std::vector<floyd::typeid_t> get_member_types(const std::vector<member_t>& m);
	json_t members_to_json(const std::vector<member_t>& members);






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

		public: json_t to_json() const;


		public: std::string _name;
		public: std::vector<member_t> _members;
	};

	std::string to_string(const struct_definition_t& v);


	//	Returns -1 if not found.
	int find_struct_member_index(const struct_definition_t& def, const std::string& name);

}


#endif /* floyd_basics_hpp */
