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
	typeid_t

	This is a very central type in the Floyd compiler.

	It specifies an exact Floyd type. Both for base types like "int" and "string" and composite types
	like "struct { [float] p; string s }.
	It can hold *any Floyd type*. It can also hold unresolved type identifiers and a few types internal to compiler.

	typeid_t can be convert to/from JSON and is written in source code according to Floyd source syntax, see table below.

	typeid_t is an immutable value object.
	The typeid_t is normalized and can be compared with other typeid_t:s.

	Composite types can form trees of types,
		like:
			"[string: struct {int x; int y}]"
		This is a dictionary with structs, each holding two integers.

	Source code						base_type								AST JSON
	================================================================================================================
	null							k_undefined					"null"
	bool							k_bool									"bool"
	int								k_int									"int"
	double							k_double								"double"
	string							k_string								"string"
	json_value						k_json_value							"json_value"
	"typeid"						k_typeid								"typeid"
	struct red { int x;float y}		k_struct								["struct", [{"type": "in", "name": "x"}, {"type": "float", "name": "y"}]]
	[int]							k_vector								["vector", "int"]
	[string: int]					k_dict									["dict", "int"]
	int ()							k_function								["function", "int", []]
	int (double, [string])			k_function								["function", "int", ["double", ["vector", "string"]]]
	randomize_player			k_unresolved		["internal_unresolved_type_identifier", "randomize_player"]


	AST JSON
	This is the JSON format we use to pass AST around. Use typeid_to_ast_json() and typeid_from_ast_json().

	COMPACT_STRING
	This is a nice user-visible representation of the typeid_t. It may be lossy. It's for REPLs etc. UI.

	SOURCE CODE TYPE
	Use read_type(), read_required_type()
*/

#include <string>
#include <vector>
#include <variant>

#include "utils.h"
#include "floyd_syntax.h"

#include "quark.h"

struct json_t;

namespace floyd {
struct typeid_t;
struct member_t;



std::string typeid_to_compact_string(const typeid_t& t);


//////////////////////////////////////////////////		get_json_type()


int get_json_type(const json_t& value);


//////////////////////////////////////////////////		struct_definition_t


//??? This struct should be *separate* from the actual struct_definition_t,
//	which should go next to function_definition_t in expression.h

struct struct_definition_t {
	public: struct_definition_t(const std::vector<member_t>& members) :
		_members(members)
	{
		QUARK_ASSERT(check_invariant());
	}
	public: bool check_invariant() const;
	public: bool operator==(const struct_definition_t& other) const;
	public: bool check_types_resolved() const;


	////////////////////////////////////////		STATE
	public: std::vector<member_t> _members;
};


std::string to_compact_string(const struct_definition_t& v);

//	Returns -1 if not found.
int find_struct_member_index(const struct_definition_t& def, const std::string& name);




//////////////////////////////////////////////////		itype_t


struct itype_t {
	itype_t(int32_t itype) : itype(itype){}

	int32_t itype;
};



enum class epure {
	pure,
	impure
};


//////////////////////////////////////		typeid_t


struct typeid_t {
	public: enum class return_dyn_type {
		none,
		arg0,
		arg1,

		arg1_typeid_constant_type,

		//	x = make_vector(arg1.get_function_return());
		vector_of_arg1func_return,

		//	x = make_vector(arg2.get_function_return());
		vector_of_arg2func_return
	};



	struct undefined_t {
		bool operator==(const undefined_t& other) const{	return true; };
	};
	struct any_t {
		bool operator==(const any_t& other) const{	return true; };
	};
	struct void_t {
		bool operator==(const void_t& other) const{	return true; };
	};
	struct bool_t {
		bool operator==(const bool_t& other) const{	return true; };
	};
	struct int_t {
		bool operator==(const int_t& other) const{	return true; };
	};
	struct double_t {
		bool operator==(const double_t& other) const{	return true; };
	};
	struct string_t {
		bool operator==(const string_t& other) const{	return true; };
	};
	struct json_type_t {
		bool operator==(const json_type_t& other) const{	return true; };
	};
	struct typeid_type_t {
		bool operator==(const typeid_type_t& other) const{	return true; };
	};
	struct struct_t {
		bool operator==(const struct_t& other) const{	return *_struct_def == *other._struct_def; };

		std::shared_ptr<const struct_definition_t> _struct_def;
	};
	struct vector_t {
		bool operator==(const vector_t& other) const{	return _parts == other._parts; };

		std::vector<typeid_t> _parts;
	};
	struct dict_t {
		bool operator==(const dict_t& other) const{	return _parts == other._parts; };

		std::vector<typeid_t> _parts;
	};
	struct function_t {
		bool operator==(const function_t& other) const{	return _parts == other._parts && pure == other.pure && dyn_return == other.dyn_return; };

		std::vector<typeid_t> _parts;
		epure pure;

		//??? Make this property travel OK through JSON, print outs etc.
		return_dyn_type dyn_return;
	};
	struct unresolved_t {
		bool operator==(const unresolved_t& other) const{	return _unresolved_type_identifier == other._unresolved_type_identifier; };

		std::string _unresolved_type_identifier;
	};

	typedef std::variant<
		undefined_t,
		any_t,
		void_t,
		bool_t,
		int_t,
		double_t,
		string_t,
		json_type_t,
		typeid_type_t,

		struct_t,
		vector_t,
		dict_t,
		function_t,

		unresolved_t
	> type_variant_t;


	////////////////////////////////////////		FUNCTIONS FOR EACH BASE-TYPE.

	public: static typeid_t make_undefined(){
		return { undefined_t() };
	}
	public: bool is_undefined() const {
		QUARK_ASSERT(check_invariant());

		return std::holds_alternative<undefined_t>(_contents);
	}


	public: static typeid_t make_any(){
		return { any_t() };
	}
	public: bool is_any() const {
		QUARK_ASSERT(check_invariant());

		return std::holds_alternative<any_t>(_contents);
	}


	public: static typeid_t make_void(){
		return { void_t() };
	}
	public: bool is_void() const {
		QUARK_ASSERT(check_invariant());

		return std::holds_alternative<void_t>(_contents);
	}


	public: static typeid_t make_bool(){
		return { bool_t() };
	}
	public: bool is_bool() const {
		QUARK_ASSERT(check_invariant());

		return std::holds_alternative<bool_t>(_contents);
	}


	public: static typeid_t make_int(){
		return { int_t() };
	}
	public: bool is_int() const {
		QUARK_ASSERT(check_invariant());

		return std::holds_alternative<int_t>(_contents);
	}


	public: static typeid_t make_double(){
		return { double_t() };
	}
	public: bool is_double() const {
		QUARK_ASSERT(check_invariant());

		return std::holds_alternative<double_t>(_contents);
	}


	public: static typeid_t make_string(){
		return { string_t() };
	}
	public: bool is_string() const {
		QUARK_ASSERT(check_invariant());

		return std::holds_alternative<string_t>(_contents);
	}


	public: static typeid_t make_json_value(){
		return { json_type_t() };
	}
	public: bool is_json_value() const {
		QUARK_ASSERT(check_invariant());

		return std::holds_alternative<json_type_t>(_contents);
	}


	public: static typeid_t make_typeid(){
		return { typeid_type_t() };
	}
	public: bool is_typeid() const {
		QUARK_ASSERT(check_invariant());

		return std::holds_alternative<typeid_type_t>(_contents);
	}


	public: static typeid_t make_struct1(const std::shared_ptr<const struct_definition_t>& def){
		QUARK_ASSERT(def);

		return { struct_t{ def } };
	}
	public: static typeid_t make_struct2(const std::vector<member_t>& members){
		auto def = std::make_shared<const struct_definition_t>(members);
		return { struct_t{ def } };
	}
	public: bool is_struct() const {
		QUARK_ASSERT(check_invariant());

		return std::holds_alternative<struct_t>(_contents);
	}
	public: const struct_definition_t& get_struct() const{
		QUARK_ASSERT(get_base_type() == base_type::k_struct);

		return *std::get<struct_t>(_contents)._struct_def;
	}
	public: const std::shared_ptr<const struct_definition_t>& get_struct_ref() const{
		QUARK_ASSERT(get_base_type() == base_type::k_struct);

		return std::get<struct_t>(_contents)._struct_def;
	}


	public: static typeid_t make_vector(const typeid_t& element_type){
		return { vector_t{ { element_type } } };
	}
	public: bool is_vector() const {
		QUARK_ASSERT(check_invariant());

		return std::holds_alternative<vector_t>(_contents);
	}
	public: const typeid_t& get_vector_element_type() const{
		QUARK_ASSERT(check_invariant());

		return std::get<vector_t>(_contents)._parts[0];
	}


	public: static typeid_t make_dict(const typeid_t& value_type){
		return { dict_t{ { value_type } } };
	}
	public: bool is_dict() const {
		QUARK_ASSERT(check_invariant());

		return std::holds_alternative<dict_t>(_contents);
	}
	public: const typeid_t& get_dict_value_type() const{
		QUARK_ASSERT(get_base_type() == base_type::k_dict);

		return std::get<dict_t>(_contents)._parts[0];
	}

	public: static typeid_t make_function3(const typeid_t& ret, const std::vector<typeid_t>& args, epure pure, return_dyn_type dyn_return){
		QUARK_ASSERT(ret.is_any() == false || dyn_return != return_dyn_type::none);

		//	Functions use _parts[0] for return type always. _parts[1] is first argument, if any.
		std::vector<typeid_t> parts = { ret };
		parts.insert(parts.end(), args.begin(), args.end());

		return { function_t{ parts, pure, dyn_return } };
	}

	public: static typeid_t make_function_dyn_return(const std::vector<typeid_t>& args, epure pure, return_dyn_type dyn_return){
		return make_function3(typeid_t::make_any(), args, pure, dyn_return);
	}

	public: static typeid_t make_function(const typeid_t& ret, const std::vector<typeid_t>& args, epure pure){
		QUARK_ASSERT(ret.is_any() == false);

		return make_function3(ret, args, pure, return_dyn_type::none);
	}

	public: bool is_function() const {
		QUARK_ASSERT(check_invariant());

		return std::holds_alternative<function_t>(_contents);
	}
	public: const typeid_t& get_function_return() const{
		QUARK_ASSERT(get_base_type() == base_type::k_function);

		return std::get<function_t>(_contents)._parts[0];
	}
	public: std::vector<typeid_t> get_function_args() const{
		QUARK_ASSERT(check_invariant());

		auto r = std::get<function_t>(_contents)._parts;
		r.erase(r.begin());
		return r;
	}
	public: epure get_function_pure() const {
		QUARK_ASSERT(check_invariant());

		return std::get<function_t>(_contents).pure;
	}
	public: return_dyn_type get_function_dyn_return_type() const {
		QUARK_ASSERT(check_invariant());

		return std::get<function_t>(_contents).dyn_return;
	}


	public: static typeid_t make_unresolved_type_identifier(const std::string& s){
		return { unresolved_t{ s } };
	}
	public: bool is_unresolved_type_identifier() const {
		QUARK_ASSERT(check_invariant());

		return std::holds_alternative<unresolved_t>(_contents);
	}
	public: std::string get_unresolved() const{
		QUARK_ASSERT(check_invariant());

		return std::get<unresolved_t>(_contents)._unresolved_type_identifier;
	}


	////////////////////////////////////////		BASICS

	public: floyd::base_type get_base_type() const{
		struct visitor_t {
			base_type operator()(const undefined_t& e) const{
				return base_type::k_undefined;
			}
			base_type operator()(const any_t& e) const{
				return base_type::k_any;
			}

			base_type operator()(const void_t& e) const{
				return base_type::k_void;
			}
			base_type operator()(const bool_t& e) const{
				return base_type::k_bool;
			}
			base_type operator()(const int_t& e) const{
				return base_type::k_int;
			}
			base_type operator()(const double_t& e) const{
				return base_type::k_double;
			}
			base_type operator()(const string_t& e) const{
				return base_type::k_string;
			}

			base_type operator()(const json_type_t& e) const{
				return base_type::k_json_value;
			}
			base_type operator()(const typeid_type_t& e) const{
				return base_type::k_typeid;
			}

			base_type operator()(const struct_t& e) const{
				return base_type::k_struct;
			}
			base_type operator()(const vector_t& e) const{
				return base_type::k_vector;
			}
			base_type operator()(const dict_t& e) const{
				return base_type::k_dict;
			}
			base_type operator()(const function_t& e) const{
				return base_type::k_function;
			}
			base_type operator()(const unresolved_t& e) const{
				return base_type::k_unresolved;
			}
		};
		return std::visit(visitor_t{}, _contents);
	}

	public: bool check_types_resolved() const;

	public: bool operator==(const typeid_t& other) const{
		QUARK_ASSERT(check_invariant());
		QUARK_ASSERT(other.check_invariant());

		return true
#if DEBUG
			&& _DEBUG == other._DEBUG
#endif
			&& _contents == other._contents;
	}
	public: bool operator!=(const typeid_t& other) const{ return !(*this == other);}
	public: bool check_invariant() const;
	public: void swap(typeid_t& other);


	////////////////////////////////////////		INTERNALS


	private: typeid_t(const type_variant_t& contents) :
		_contents(contents)
	{

#if DEBUG
		_DEBUG = typeid_to_compact_string(*this);
#endif
		QUARK_ASSERT(check_invariant());
	}


	////////////////////////////////////////		STATE
#if DEBUG
	private: std::string _DEBUG;
#endif
	public: type_variant_t _contents;
};

std::string typeid_to_compact_string(const typeid_t& t);





//////////////////////////////////////		dynamic function

//	Dynamic values are "fat" values that also keep their type. This is used right now for Floyd's host functions
//	that accepts many/any type of arguments. Like size().
//	A dynamic function has one or several arguments of type k_any.
//	The argument types for k_any arguments must be supplied for each function invocation.
bool is_dynamic_function(const typeid_t& function_type);

int count_function_dynamic_args(const std::vector<typeid_t>& args);
int count_function_dynamic_args(const typeid_t& function_type);



//////////////////////////////////////		member_t

/*
	Definition of a struct-member, just a type + name. It is used for other purposes too, like defining function arguments.
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

}	//	floyd

#endif /* ast_typeid_hpp */
