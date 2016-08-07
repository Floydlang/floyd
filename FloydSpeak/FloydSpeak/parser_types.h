//
//  parse_types.h
//  FloydSpeak
//
//  Created by Marcus Zetterquist on 24/07/16.
//  Copyright © 2016 Marcus Zetterquist. All rights reserved.
//

#ifndef parser_types_hpp
#define parser_types_hpp

#include "quark.h"

#include <vector>
#include <string>

struct TSHA1;

/*
	type signature:	a string the defines one level of any type. It can be
	typedef: a typesafe identifier for any time.
*/

namespace floyd_parser {
	struct type_def_t;
	struct types_collector_t;
	struct statement_t;
	struct struct_def_t;
	struct function_def_t;
	struct value_t;
	struct scope_def_t;
	struct struct_instance_t;


	//////////////////////////////////////		frontend_base_type

	/*
		This type is tracked by compiler, not stored in the value-type.
	*/
	enum frontend_base_type {
//		k_float,
		k_int,
		k_bool,
		k_string,

//		k_enum,
		k_struct,
//		k_map,
		k_vector,
//		k_seq,
//		k_dyn,
		k_function
	};

	std::string to_string(const frontend_base_type t);


	//////////////////////////////////////		type_identifier_t

	/*
		Textual specification of a type-identifier.
		It only contains valid characters.
		There is no guarantee this type actually exists.
	*/

	struct type_identifier_t {
		public: static type_identifier_t make_type(std::string s);

		public: type_identifier_t(const type_identifier_t& other);
		public: type_identifier_t operator=(const type_identifier_t& other);

		public: bool operator==(const type_identifier_t& other) const;
		public: bool operator!=(const type_identifier_t& other) const;

		private: type_identifier_t(){};
		public: explicit type_identifier_t(const char s[]);
		public: explicit type_identifier_t(const std::string& s);
		public: void swap(type_identifier_t& other);
		public: std::string to_string() const;
		public: bool check_invariant() const;


		///////////////////		STATE
		/*
			The name of the type, including its path using :
			"null"

			"bool"
			"int"
			"float"
			"function"

			//	Specifies a data type.
			"value_type"


			"metronome"
			"map<string, metronome>"
			"game_engine:sprite"
			"vector<game_engine:sprite>"
			"int (string, vector<game_engine:sprite>)"
		*/
		private: std::string _type_magic;
	};


	//////////////////////////////////////		arg_t

	/*
		Describes a function argument - it's type and the argument name.
	*/
	struct arg_t {
		bool check_invariant() const {
			QUARK_ASSERT(_type.check_invariant());
			QUARK_ASSERT(_identifier.size() > 0);
			return true;
		}
		bool operator==(const arg_t& other) const{
			return _type == other._type && _identifier == other._identifier;
		}

		type_identifier_t _type;
		std::string _identifier;
	};

	void trace(const arg_t& arg);


	//////////////////////////////////////		function_def_t


	struct host_data_i {
		public: virtual ~host_data_i(){};
	};

	typedef value_t (*hosts_function_t)(const std::shared_ptr<host_data_i>& param, const std::vector<value_t>& args);

	struct function_def_t {
		public: function_def_t(type_identifier_t return_type, const std::vector<arg_t>& args, const std::vector<std::shared_ptr<statement_t> > statements);
		public: function_def_t(type_identifier_t return_type, const std::vector<arg_t>& args, hosts_function_t f, std::shared_ptr<host_data_i> param);
		public: bool check_invariant() const;
		public: bool operator==(const function_def_t& other) const;

		public: const type_identifier_t _return_type;
		public: const std::vector<arg_t> _args;
		public: std::shared_ptr<scope_def_t> _scope_def;
	};

	void trace(const function_def_t& v);
	void trace(const std::vector<std::shared_ptr<statement_t>>& e);
	function_def_t make_function_def(type_identifier_t return_type, const std::vector<arg_t>& args, const std::vector<statement_t>& statements);
	TSHA1 calc_function_body_hash(const function_def_t& f);




	//////////////////////////////////////		member_t

	/*
		Definition of a struct-member.
		??? support expressions for default values.
	*/

	struct member_t {
		public: member_t(const type_identifier_t& type, const std::string& name, const value_t& init_value);
		public: member_t(const type_identifier_t& type, const std::string& name);
		bool operator==(const member_t& other) const;
		public: bool check_invariant() const;

		public: std::shared_ptr<type_identifier_t> _type;

		//	Optional -- must have same type as _type.
		public: std::shared_ptr<value_t> _value;

		public: std::string _name;
	};

	void trace(const member_t& member);


	//////////////////////////////////////		struct_def_t

	/*
		Defines a struct, including all members.

		TODO
		- Add member functions / vtable
		- Add memory layout calculation and storage
		- Add support for alternative layout.
		- Add support for optional value (using "?").
	*/
	typedef struct_instance_t (*construct_default_t)(const struct_def_t& def);

	struct struct_def_t {
		public: bool check_invariant() const;
		bool operator==(const struct_def_t& other) const;


		///////////////////		STATE
		public: std::vector<member_t> _members;
//		public: construct_default_struct_t _construct_default;
//		public: construct_default_struct_t _construct_default;
	};

	void trace(const struct_def_t& e);
	std::string to_signature(const struct_def_t& t);
	struct_def_t make_struct_def(const std::vector<member_t>& members);




	//////////////////////////////////////		vector_def_t


	struct vector_def_t {
		public: vector_def_t(){};
		public: bool check_invariant() const;


		///////////////////		STATE
		public: std::string _value_type_identifier;
		public: std::string _key_type_identifier;
	};


	////////////////////	Helpers for making tests.




	struct_def_t make_struct0();
	struct_def_t make_struct1();

	/*
		struct struct2 {
		}
	*/
	struct_def_t make_struct2();

	/*
		struct struct3 {
			int a
			string b
		}
	*/
	struct_def_t make_struct3();

	/*
		struct struct4 {
			string x
			<struct_1> y
			string z
		}
	*/
	struct_def_t make_struct4();

	struct_def_t make_struct5();


}	//	floyd_parser


#endif
