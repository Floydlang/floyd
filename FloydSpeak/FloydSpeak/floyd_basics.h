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

namespace floyd_basics {

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

		k_define_function,

		k_define_struct
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

		k_struct,
		k_vector,
		k_function,

		k_custom_type
	};

	std::string base_type_to_string(const base_type t);



	//////////////////////////////////////		typeid_t


	struct typeid_t;
	json_t typeid_to_json(const typeid_t& t);

	struct typeid_t {

		public: static typeid_t make_null(){
			return { floyd_basics::base_type::k_null, {}, {}, {} };
		}

		public: static typeid_t make_bool(){
			return { floyd_basics::base_type::k_bool, {}, {}, {} };
		}

		public: static typeid_t make_int(){
			return { floyd_basics::base_type::k_int, {}, {}, {} };
		}

		public: static typeid_t make_float(){
			return { floyd_basics::base_type::k_float, {}, {}, {} };
		}

		public: static typeid_t make_string(){
			return { floyd_basics::base_type::k_string, {}, {}, {} };
		}

		public: static typeid_t make_custom_type(const std::string& s){
			return { floyd_basics::base_type::k_custom_type, {}, {}, s };
		}

		public: bool is_null() const {
			return _base_type == floyd_basics::base_type::k_null;
		}

		public: static typeid_t make_struct(const std::string& struct_def_id){
			return { floyd_basics::base_type::k_struct, {}, struct_def_id, {} };
		}

		public: static typeid_t make_vector(const typeid_t& element_type){
			return { floyd_basics::base_type::k_vector, { element_type }, {}, {} };
		}

		public: static typeid_t make_function(const typeid_t& ret, const std::vector<typeid_t>& args){
			//	Functions use _parts[0] for return type always. _parts[1] is first argument, if any.
			std::vector<typeid_t> parts = { ret };
			parts.insert(parts.end(), args.begin(), args.end());
			return { floyd_basics::base_type::k_function, parts, {}, {} };
		}

		public: bool operator==(const typeid_t& other) const{
			return _base_type == other._base_type && _parts == other._parts && _struct_def_id == other._struct_def_id;
		}

		public: bool check_invariant() const;

		public: void swap(typeid_t& other);

		/*
			"int"
			"[int]"
			"int (float, [int])"
			"coord_t/8000"
			??? use json instead.
		*/
		public: std::string to_string() const;
		public: static typeid_t from_string(const std::string& s);


		public: floyd_basics::base_type get_base_type() const{
			return _base_type;
		}


		/////////////////////////////		STATE


		/*
			"int"
			"coord_t"
			"coord_t/8000"
			"int (float a, float b)"
			"[string]"
			"[string([bool(float,string),pixel)])"
			"[coord_t/8000]"
			"pixel_coord_t = coord_t/8000"
		*/
		public: floyd_basics::base_type _base_type;
		public: std::vector<typeid_t> _parts;
		public: std::string _struct_def_id;

		//	This is used it overrides _base_type (which will be k_custom_type).
		public: std::string _unresolved_type_symbol;
	};

	json_t typeid_to_json(const typeid_t& t);



}


#endif /* floyd_basics_hpp */
