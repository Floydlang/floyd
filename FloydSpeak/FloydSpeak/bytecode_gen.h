//
//  parser_evaluator.h
//  FloydSpeak
//
//  Created by Marcus Zetterquist on 26/07/16.
//  Copyright © 2016 Marcus Zetterquist. All rights reserved.
//

#ifndef bytecode_gen_h
#define bytecode_gen_h

#include "quark.h"

#include <string>
#include <vector>
#include "ast.h"
#include "ast_typeid.h"
#include "ast_value.h"


#define FLOYD_BD_DEBUG 1


namespace floyd {
	struct expression_t;
	struct value_t;
	struct statement_t;
	struct bgenerator_t;

	struct bc_body_t;
	struct bc_value_t;
	struct semantic_ast_t;
	struct bc_typeid2_t;

	inline value_t bc_to_value(const bc_value_t& value, const typeid_t& type);
	inline bc_value_t value_to_bc(const value_t& value);


	inline std::vector<bc_value_t> values_to_bcs(const std::vector<value_t>& values);
	inline std::vector<value_t> bcs_to_values__same_types(const std::vector<bc_value_t>& values, const typeid_t& shared_type);



	//////////////////////////////////////		bc_value_object_t



	struct bc_value_object_t {
#if DEBUG
		public: bool check_invariant() const{
			QUARK_ASSERT(_rc > 0);
			QUARK_ASSERT(_type.check_invariant());
			QUARK_ASSERT(_typeid_value.check_invariant());

			const auto base_type = _type.get_base_type();
			if(base_type == base_type::k_string){
//				QUARK_ASSERT(_string);
				QUARK_ASSERT(_json_value == nullptr);
				QUARK_ASSERT(_typeid_value == typeid_t::make_undefined());
				QUARK_ASSERT(_struct_members.empty());
				QUARK_ASSERT(_vector_elements.empty());
				QUARK_ASSERT(_dict_entries.empty());
				QUARK_ASSERT(_function_id == -1);
			}
			else if(base_type == base_type::k_json_value){
				QUARK_ASSERT(_string.empty());
				QUARK_ASSERT(_json_value != nullptr);
				QUARK_ASSERT(_typeid_value == typeid_t::make_undefined());
				QUARK_ASSERT(_struct_members.empty());
				QUARK_ASSERT(_vector_elements.empty());
				QUARK_ASSERT(_dict_entries.empty());
				QUARK_ASSERT(_function_id == -1);

				QUARK_ASSERT(_json_value->check_invariant());
			}

			else if(base_type == base_type::k_typeid){
				QUARK_ASSERT(_string.empty());
				QUARK_ASSERT(_json_value == nullptr);
		//		QUARK_ASSERT(_typeid_value != typeid_t::make_undefined());
				QUARK_ASSERT(_struct_members.empty());
				QUARK_ASSERT(_vector_elements.empty());
				QUARK_ASSERT(_dict_entries.empty());
				QUARK_ASSERT(_function_id == -1);

				QUARK_ASSERT(_typeid_value.check_invariant());
			}
			else if(base_type == base_type::k_struct){
				QUARK_ASSERT(_string.empty());
				QUARK_ASSERT(_json_value == nullptr);
				QUARK_ASSERT(_typeid_value == typeid_t::make_undefined());
//				QUARK_ASSERT(_struct != nullptr);
				QUARK_ASSERT(_vector_elements.empty());
				QUARK_ASSERT(_dict_entries.empty());
				QUARK_ASSERT(_function_id == -1);

//				QUARK_ASSERT(_struct && _struct->check_invariant());
			}
			else if(base_type == base_type::k_vector){
				QUARK_ASSERT(_string.empty());
				QUARK_ASSERT(_json_value == nullptr);
				QUARK_ASSERT(_typeid_value == typeid_t::make_undefined());
				QUARK_ASSERT(_struct_members.empty());
		//		QUARK_ASSERT(_vector_elements.empty());
				QUARK_ASSERT(_dict_entries.empty());
				QUARK_ASSERT(_function_id == -1);
			}
			else if(base_type == base_type::k_dict){
				QUARK_ASSERT(_string.empty());
				QUARK_ASSERT(_json_value == nullptr);
				QUARK_ASSERT(_typeid_value == typeid_t::make_undefined());
				QUARK_ASSERT(_struct_members.empty());
				QUARK_ASSERT(_vector_elements.empty());
		//		QUARK_ASSERT(_dict_entries.empty());
				QUARK_ASSERT(_function_id == -1);
			}
			else if(base_type == base_type::k_function){
				QUARK_ASSERT(_string.empty());
				QUARK_ASSERT(_json_value == nullptr);
				QUARK_ASSERT(_typeid_value == typeid_t::make_undefined());
				QUARK_ASSERT(_struct_members.empty());
				QUARK_ASSERT(_vector_elements.empty());
				QUARK_ASSERT(_dict_entries.empty());
				QUARK_ASSERT(_function_id != -1);
			}
			else {
				QUARK_ASSERT(false);
			}
			return true;
		}
#endif

		public: bool operator==(const bc_value_object_t& other) const;



		public: bc_value_object_t(const std::string& s) :
			_rc(1),
#if DEBUG && FLOYD_BD_DEBUG
			_type(typeid_t::make_string()),
#endif
			_string(s)
		{
			QUARK_ASSERT(check_invariant());
		}

		public: bc_value_object_t(const std::shared_ptr<json_t>& s) :
			_rc(1),
#if DEBUG && FLOYD_BD_DEBUG
			_type(typeid_t::make_json_value()),
#endif
			_json_value(s)
		{
			QUARK_ASSERT(check_invariant());
		}

		public: bc_value_object_t(const typeid_t& s) :
			_rc(1),
#if DEBUG && FLOYD_BD_DEBUG
			_type(typeid_t::make_typeid()),
#endif
			_typeid_value(s)
		{
			QUARK_ASSERT(check_invariant());
		}

		public: bc_value_object_t(const typeid_t& type, const std::vector<bc_value_t>& s, bool struct_tag) :
			_rc(1),
#if DEBUG && FLOYD_BD_DEBUG
			_type(type),
#endif
			_struct_members(s)
		{
			QUARK_ASSERT(check_invariant());
		}
		public: bc_value_object_t(const typeid_t& type, const std::vector<bc_value_t>& s) :
			_rc(1),
#if DEBUG && FLOYD_BD_DEBUG
			_type(type),
#endif
			_vector_elements(s)
		{
			QUARK_ASSERT(check_invariant());
		}
		public: bc_value_object_t(const typeid_t& type, const std::map<std::string, bc_value_t>& s) :
			_rc(1),
#if DEBUG && FLOYD_BD_DEBUG
			_type(type),
#endif
			_dict_entries(s)
		{
			QUARK_ASSERT(check_invariant());
		}
		public: bc_value_object_t(const typeid_t& type, int function_id) :
			_rc(1),
#if DEBUG && FLOYD_BD_DEBUG
			_type(type),
#endif
			_function_id(function_id)
		{
			QUARK_ASSERT(check_invariant());
		}



		public: int _rc;
#if DEBUG
		public: typeid_t _type;
#endif
		//	Holds ALL variants of objects right now -- optimize!
		public: std::string _string;
		public: std::shared_ptr<json_t> _json_value;
		public: typeid_t _typeid_value = typeid_t::make_undefined();
		public: std::vector<bc_value_t> _struct_members;


		public: std::vector<bc_value_t> _vector_elements;
		public: std::map<std::string, bc_value_t> _dict_entries;
		public: int _function_id = -1;
	};




	//////////////////////////////////////		bc_value_t


	inline bool is_ext_slow2(base_type basetype){
		return false
			|| basetype == base_type::k_string
			|| basetype == base_type::k_json_value
			|| basetype == base_type::k_typeid
			|| basetype == base_type::k_struct
			|| basetype == base_type::k_vector
			|| basetype == base_type::k_dict
			|| basetype == base_type::k_function;
	}



	struct bc_value_t {
		public: bc_value_t() :
#if DEBUG && FLOYD_BD_DEBUG
			_debug_type(typeid_t::make_undefined()),
#endif
			_is_ext(false)
		{
			QUARK_ASSERT(check_invariant());
		}
		private: explicit bc_value_t(const typeid_t& type, bool dummy) :
#if DEBUG && FLOYD_BD_DEBUG
			_debug_type(type),
#endif
			_is_ext(false)
		{
			QUARK_ASSERT(check_invariant());
		}

		public: ~bc_value_t(){
			QUARK_ASSERT(check_invariant());

			if(_is_ext){
				_value_internals._ext->_rc--;
				if(_value_internals._ext->_rc == 0){
					delete _value_internals._ext;
					_value_internals._ext = nullptr;
				}
			}
		}

		public: typeid_t get_debug_type() const {
#if DEBUG && FLOYD_BD_DEBUG
			return _debug_type;
#else
			return typeid_t::make_undefined();
#endif
		}

		public: bc_value_t(const bc_value_t& other) :
#if DEBUG && FLOYD_BD_DEBUG
			_debug_type(other._debug_type),
#endif
			_is_ext(other._is_ext),
			_value_internals(other._value_internals)
		{
			QUARK_ASSERT(other.check_invariant());

			if(_is_ext){
				_value_internals._ext->_rc++;
			}

			QUARK_ASSERT(check_invariant());
		}

		public: bc_value_t& operator=(const bc_value_t& other){
			QUARK_ASSERT(other.check_invariant());
			QUARK_ASSERT(check_invariant());

			bc_value_t temp(other);
			temp.swap(*this);

			QUARK_ASSERT(other.check_invariant());
			QUARK_ASSERT(check_invariant());
			return *this;
		}

/*
		public: bool operator==(const bc_value_t& other) const{
			QUARK_ASSERT(check_invariant());
			QUARK_ASSERT(other.check_invariant());

			if(_is_ext != other._is_ext){
				return false;
			}

			if(_basetype == base_type::k_internal_undefined){
				return true;
			}
			else if(_basetype == base_type::k_bool){
				return _value_internals._bool == other._value_internals._bool;
			}
			else if(_basetype == base_type::k_int){
				return _value_internals._int == other._value_internals._int;
			}
			else if(_basetype == base_type::k_float){
				return _value_internals._float == other._value_internals._float;
			}
			else{
				QUARK_ASSERT(is_ext(_basetype));
				return compare_shared_values(_value_internals._ext, other._value_internals._ext);
			}
		}

		public: bool operator!=(const bc_value_t& other) const{
			return !(*this == other);
		}
*/

		public: void swap(bc_value_t& other){
			QUARK_ASSERT(other.check_invariant());
			QUARK_ASSERT(check_invariant());

#if DEBUG && FLOYD_BD_DEBUG
			std::swap(_debug_type, other._debug_type);
#endif

			std::swap(_is_ext, other._is_ext);

			std::swap(_value_internals, other._value_internals);

			QUARK_ASSERT(other.check_invariant());
			QUARK_ASSERT(check_invariant());
		}






		private: static bc_value_t from_value(const value_t& value){
			QUARK_ASSERT(value.check_invariant());

			const auto basetype = value.get_basetype();
			if(basetype == base_type::k_internal_undefined){
				return make_undefined();
			}
			else if(basetype == base_type::k_internal_dynamic){
				return make_internal_dynamic();
			}
			else if(basetype == base_type::k_void){
				return make_void();
			}
			else if(basetype == base_type::k_bool){
				return make_bool(value.get_bool_value());
			}
			else if(basetype == base_type::k_bool){
				return make_bool(value.get_bool_value());
			}
			else if(basetype == base_type::k_int){
				return make_int(value.get_int_value());
			}
			else if(basetype == base_type::k_float){
				return make_float(value.get_float_value());
			}

			else if(basetype == base_type::k_string){
				return make_string(value.get_string_value());
			}
			else if(basetype == base_type::k_json_value){
				return make_json_value(value.get_json_value());
			}
			else if(basetype == base_type::k_typeid){
				return make_typeid_value(value.get_typeid_value());
			}
			else if(basetype == base_type::k_struct){
				return make_struct_value(value.get_type(), values_to_bcs(value.get_struct_value()->_member_values));
			}

			else if(basetype == base_type::k_vector){
				return make_vector_value(value.get_type().get_vector_element_type(), values_to_bcs(value.get_vector_value()));
			}
			else if(basetype == base_type::k_dict){
				const auto elements = value.get_dict_value();
				std::map<std::string, bc_value_t> entries2;
				for(const auto e: elements){
					entries2.insert({e.first, value_to_bc(e.second)});
				}
				return make_dict_value(value.get_type().get_dict_value_type(), entries2);
			}
			else if(basetype == base_type::k_function){
				return make_function_value(value.get_type(), value.get_function_value());
			}
			else{
				QUARK_ASSERT(false);
				throw std::exception();
			}
		}



#if DEBUG
		public: bool check_invariant() const {
			QUARK_ASSERT(_debug_type.check_invariant());
			return true;
		}
#endif

/*
		public: bool operator==(const bc_value_t& other) const{
			return compare_value_true_deep(*this, other) == 0;
		}
*/


		public: static int compare_value_true_deep(const bc_value_t& left, const bc_value_t& right, const bc_typeid2_t& type);



		//////////////////////////////////////		internal-undefined type


/*
#if DEBUG
		public: bool is_undefined() const {
			QUARK_ASSERT(check_invariant());

			return _debug_type.is_undefined();
		}
#endif
*/

		public: static bc_value_t make_undefined(){
			return bc_value_t(typeid_t::make_undefined(), true);
		}

		//////////////////////////////////////		internal-dynamic type

/*
#if DEBUG
		public: bool is_internal_dynamic() const {
			QUARK_ASSERT(check_invariant());

			return _debug_type.is_internal_dynamic();
		}
#endif
*/
		public: static bc_value_t make_internal_dynamic(){
			return bc_value_t(typeid_t::make_internal_dynamic(), true);
		}

		//////////////////////////////////////		void


/*
#if DEBUG
		public: bool is_void() const {
			QUARK_ASSERT(check_invariant());

			return _debug_type.is_void();
		}
#endif
*/
		public: static bc_value_t make_void(){
			return bc_value_t(typeid_t::make_void(), true);
		}


		//////////////////////////////////////		bool


/*
#if DEBUG
		public: bool is_bool() const {
			QUARK_ASSERT(check_invariant());

			return _debug_type.is_bool();
		}
#endif
*/
		public: static bc_value_t make_bool(bool v){
			return bc_value_t(v);
		}
		public: bool get_bool_value() const {
			QUARK_ASSERT(check_invariant());

			return _value_internals._bool;
		}
		public: bool get_bool_value_quick() const {
			QUARK_ASSERT(check_invariant());

			return _value_internals._bool;
		}
		private: explicit bc_value_t(bool value) :
#if DEBUG && FLOYD_BD_DEBUG
			_debug_type(typeid_t::make_bool()),
#endif
			_is_ext(false)
		{
			_value_internals._bool = value;
			QUARK_ASSERT(check_invariant());
		}


		//////////////////////////////////////		int


/*
#if DEBUG
		public: bool is_int() const {
			QUARK_ASSERT(check_invariant());

			return _debug_type.is_int();
		}
#endif
*/

		public: static bc_value_t make_int(int v){
			return bc_value_t{ v };
		}
		public: int get_int_value() const {
			QUARK_ASSERT(check_invariant());

			return _value_internals._int;
		}
		public: int get_int_value_quick() const {
			QUARK_ASSERT(check_invariant());

			return _value_internals._int;
		}
		private: explicit bc_value_t(int value) :
#if DEBUG && FLOYD_BD_DEBUG
			_debug_type(typeid_t::make_int()),
#endif
			_is_ext(false)
		{
			_value_internals._int = value;
			QUARK_ASSERT(check_invariant());
		}


		//////////////////////////////////////		float


/*
#if DEBUG
		public: bool is_float() const {
			QUARK_ASSERT(check_invariant());

			return _debug_type.is_float();
		}
#endif
*/
		public: static bc_value_t make_float(float v){
			return bc_value_t{ v };
		}
		public: float get_float_value() const {
			QUARK_ASSERT(check_invariant());

			return _value_internals._float;
		}
		private: explicit bc_value_t(float value) :
#if DEBUG && FLOYD_BD_DEBUG
			_debug_type(typeid_t::make_float()),
#endif
			_is_ext(false)
		{
			_value_internals._float = value;
			QUARK_ASSERT(check_invariant());
		}


		//////////////////////////////////////		string


/*
#if DEBUG
		public: bool is_string() const {
			QUARK_ASSERT(check_invariant());

			return _debug_type.is_string();
		}
#endif
*/
		public: static bc_value_t make_string(const std::string& v){
			return bc_value_t{ v };
		}
		public: std::string get_string_value() const{
			QUARK_ASSERT(check_invariant());

			return _value_internals._ext->_string;
		}
		private: explicit bc_value_t(const std::string value) :
#if DEBUG && FLOYD_BD_DEBUG
			_debug_type(typeid_t::make_string()),
#endif
			_is_ext(true)
		{
			_value_internals._ext = new bc_value_object_t{value};
			QUARK_ASSERT(check_invariant());
		}


		//////////////////////////////////////		json_value


/*
		private: bool is_json_value() const {
			QUARK_ASSERT(check_invariant());

			return _debug_type.is_json_value();
		}
*/
		public: static bc_value_t make_json_value(const json_t& v){
			return bc_value_t{ std::make_shared<json_t>(v) };
		}
		public: json_t get_json_value() const{
			QUARK_ASSERT(check_invariant());

			return *_value_internals._ext->_json_value.get();
		}
		private: explicit bc_value_t(const std::shared_ptr<json_t>& value) :
#if DEBUG && FLOYD_BD_DEBUG
			_debug_type(typeid_t::make_json_value()),
#endif
			_is_ext(true)
		{
			_value_internals._ext = new bc_value_object_t{value};
			QUARK_ASSERT(check_invariant());
		}


		//////////////////////////////////////		typeid



/*
		private: bool is_typeid() const {
			QUARK_ASSERT(check_invariant());

			return _debug_type.is_typeid();
		}
*/
		public: static bc_value_t make_typeid_value(const typeid_t& type_id){
			return bc_value_t{ type_id };
		}
		public: typeid_t get_typeid_value() const {
			QUARK_ASSERT(check_invariant());

			return _value_internals._ext->_typeid_value;
		}
		private: explicit bc_value_t(const typeid_t& type_id) :
#if DEBUG && FLOYD_BD_DEBUG
			_debug_type(typeid_t::make_typeid()),
#endif
			_is_ext(true)
		{
			_value_internals._ext = new bc_value_object_t{type_id};
			QUARK_ASSERT(check_invariant());
		}


		//////////////////////////////////////		struct


/*
		private: bool is_struct() const {
			QUARK_ASSERT(check_invariant());

			return _debug_type.is_struct();
		}
*/
		public: static bc_value_t make_struct_value(const typeid_t& struct_type, const std::vector<bc_value_t>& values){
			return bc_value_t{ struct_type, values, true };
		}
		public: const std::vector<bc_value_t>& get_struct_value() const {
			return _value_internals._ext->_struct_members;
		}
		private: explicit bc_value_t(const typeid_t& struct_type, const std::vector<bc_value_t>& values, bool struct_tag) :
#if DEBUG && FLOYD_BD_DEBUG
			_debug_type(struct_type),
#endif
			_is_ext(true)
		{
			_value_internals._ext = new bc_value_object_t{struct_type, values, true};
			QUARK_ASSERT(check_invariant());
		}


		//////////////////////////////////////		vector


/*
		private: bool is_vector() const {
			QUARK_ASSERT(check_invariant());

			return _debug_type.is_vector();
		}
*/

		public: static bc_value_t make_vector_value(const typeid_t& element_type, const std::vector<bc_value_t>& elements){
			return bc_value_t{element_type, elements};
		}
		public: const std::vector<bc_value_t>& get_vector_value() const{
			QUARK_ASSERT(check_invariant());

			return _value_internals._ext->_vector_elements;
		}
		private: explicit bc_value_t(const typeid_t& element_type, const std::vector<bc_value_t>& values) :
#if DEBUG && FLOYD_BD_DEBUG
			_debug_type(typeid_t::make_vector(element_type)),
#endif
			_is_ext(true)
		{
			_value_internals._ext = new bc_value_object_t{typeid_t::make_vector(element_type), values};
			QUARK_ASSERT(check_invariant());
		}



		//////////////////////////////////////		dict


/*
		private: bool is_dict() const {
			QUARK_ASSERT(check_invariant());

			return _debug_type.is_dict();
		}
*/
		public: static bc_value_t make_dict_value(const typeid_t& value_type, const std::map<std::string, bc_value_t>& entries){
			return bc_value_t{value_type, entries};
		}

		public: const std::map<std::string, bc_value_t>& get_dict_value() const{
			QUARK_ASSERT(check_invariant());

			return _value_internals._ext->_dict_entries;
		}
		private: explicit bc_value_t(const typeid_t& value_type, const std::map<std::string, bc_value_t>& entries) :
#if DEBUG && FLOYD_BD_DEBUG
			_debug_type(typeid_t::make_dict(value_type)),
#endif
			_is_ext(true)
		{
			_value_internals._ext = new bc_value_object_t{typeid_t::make_dict(value_type), entries};
			QUARK_ASSERT(check_invariant());
		}


		//////////////////////////////////////		function


/*
		//??? no need to ext for functions!
		private: bool is_function() const {
			QUARK_ASSERT(check_invariant());

			return _debug_type.is_function();
		}
*/
		public: static bc_value_t make_function_value(const typeid_t& function_type, int function_id){
			return bc_value_t{ function_type, function_id, true };
		}
		public: int get_function_value() const{
			QUARK_ASSERT(check_invariant());

			return _value_internals._ext->_function_id;
		}
		private: explicit bc_value_t(const typeid_t& function_type, int function_id, bool dummy) :
#if DEBUG && FLOYD_BD_DEBUG
			_debug_type(function_type),
#endif
			_is_ext(true)
		{
			_value_internals._ext = new bc_value_object_t{function_type, function_id};
			QUARK_ASSERT(check_invariant());
		}



		friend bc_value_t value_to_bc(const value_t& value);



		//////////////////////////////////////		STATE


		private: union value_internals_t {
			bool _bool;
			int _int;
			float _float;
			bc_value_object_t* _ext;
		};


#if DEBUG && FLOYD_BD_DEBUG
		private: typeid_t _debug_type;
#endif
		private: bool _is_ext = false;
		private: value_internals_t _value_internals;
	};




	//////////////////////////////////////		value helpers



	inline std::vector<bc_value_t> values_to_bcs(const std::vector<value_t>& values){
		std::vector<bc_value_t> result;
		for(const auto e: values){
			result.push_back(value_to_bc(e));
		}
		return result;
	}

	inline std::vector<value_t> bcs_to_values__same_types(const std::vector<bc_value_t>& values, const typeid_t& shared_type){
		std::vector<value_t> result;
		for(const auto e: values){
			result.push_back(bc_to_value(e, shared_type));
		}
		return result;
	}

	inline value_t bc_to_value(const bc_value_t& value, const typeid_t& type){
		QUARK_ASSERT(value.check_invariant());

		const auto basetype = type.get_base_type();

		if(basetype == base_type::k_internal_undefined){
			return value_t::make_undefined();
		}
		else if(basetype == base_type::k_internal_dynamic){
			return value_t::make_internal_dynamic();
		}
		else if(basetype == base_type::k_void){
			return value_t::make_void();
		}
		else if(basetype == base_type::k_bool){
			return value_t::make_bool(value.get_bool_value_quick());
		}
		else if(basetype == base_type::k_int){
			return value_t::make_int(value.get_int_value_quick());
		}
		else if(basetype == base_type::k_float){
			return value_t::make_float(value.get_float_value());
		}
		else if(basetype == base_type::k_string){
			return value_t::make_string(value.get_string_value());
		}
		else if(basetype == base_type::k_json_value){
			return value_t::make_json_value(value.get_json_value());
		}
		else if(basetype == base_type::k_typeid){
			return value_t::make_typeid_value(value.get_typeid_value());
		}
		else if(basetype == base_type::k_struct){
			const auto& struct_def = type.get_struct();
			const auto& members = value.get_struct_value();
			std::vector<value_t> members2;
			for(int i = 0 ; i < members.size() ; i++){
				const auto& member_type = struct_def._members[i]._type;
				const auto& member_value = members[i];
				const auto& member_value2 = bc_to_value(member_value, member_type);
				members2.push_back(member_value2);
			}
			return value_t::make_struct_value(type, members2);
		}
		else if(basetype == base_type::k_vector){
			const auto& element_type  = type.get_vector_element_type();
			return value_t::make_vector_value(element_type, bcs_to_values__same_types(value.get_vector_value(), element_type));
		}
		else if(basetype == base_type::k_dict){
			const auto value_type = type.get_dict_value_type();
			const auto entries = value.get_dict_value();
			std::map<std::string, value_t> entries2;
			for(const auto& e: entries){
				entries2.insert({e.first, bc_to_value(e.second, value_type)});
			}
			return value_t::make_dict_value(value_type, entries2);
		}
		else if(basetype == base_type::k_function){
			return value_t::make_function_value(type, value.get_function_value());
		}
		else{
			QUARK_ASSERT(false);
			throw std::exception();
		}
	}

	inline bc_value_t value_to_bc(const value_t& value){
		return bc_value_t::from_value(value);
	}

	inline std::string to_compact_string2(const bc_value_t& value) {
		return "xxyyzz";
//		return to_compact_string2(value._backstore);
	}






	//////////////////////////////////////		bc_statement_opcode


//	typedef uint32_t bc_instruction_t;

	/*
		----------------------------------- -----------------------------------
		66665555 55555544 44444444 33333333 33222222 22221111 11111100 00000000
		32109876 54321098 76543210 98765432 10987654 32109876 54321098 76543210

		XXXXXXXX XXXXXXXX PPPPPPPP PPPPPPPP PPPPPPPP PPPPPPPP PPPPPPPP PPPPPppp
		48bit Intel x86_64 pointer. ppp = low bits, set to 0, X = bit 47

		-----------------------------------
		33222222 22221111 11111100 00000000
		10987654 32109876 54321098 76543210

		INSTRUCTION
		CCCCCCCC AAAAAAAA BBBBBBBB CCCCCCCC

		A = destination register.
		B = lhs register
		C = rhs register

		-----------------------------------------------------------------------
	*/

	enum class bc_statement_opcode: uint8_t {
		k_nop,

		//	Store _e[0] -> _v
		k_statement_store,

		//	Notice: instruction doesn't have to know value-type -- we can see type in symbol table
		//	A) index of local/global variable.
		//	B) Future: have BLOB as stack-frame, shift+mask data into it. env_index, word_offset, shift
		//	C) Instruction uses local-index and VM knows have to encode into stack BLOB.
		k_statement_store1bit,
		k_statement_store8bits,
		k_statement_store16bits,
		k_statement_store32bits,
		k_statement_store64bits,
		k_statement_store_objptr,

		//	Needed?
		//	execute body_x
		k_statement_block,

		//	_e[0]
		k_statement_return,

		//	if _e[0] execute _body_x, else execute _body_y
		k_statement_if,

		k_statement_for,

		k_statement_while,

		//	Not needed. Just use an expression and don't use its result.
		k_statement_expression
	};



	//////////////////////////////////////		bc_expression_opcode



	enum class bc_expression_opcode: uint8_t {
		k_expression_literal,
		k_expression_resolve_member,
		k_expression_lookup_element,
		k_expression_load,
		k_expression_call,
		k_expression_construct_value,

		k_expression_arithmetic_unary_minus,

		//	replace by k_statement_if.
		k_expression_conditional_operator3,

		k_expression_comparison_smaller_or_equal,
		k_expression_comparison_smaller,
		k_expression_comparison_larger_or_equal,
		k_expression_comparison_larger,

		k_expression_logical_equal,
		k_expression_logical_nonequal,

		k_expression_arithmetic_add,
		k_expression_arithmetic_subtract,
		k_expression_arithmetic_multiply,
		k_expression_arithmetic_divide,
		k_expression_arithmetic_remainder,

		k_expression_logical_and,
		k_expression_logical_or
	};




	//////////////////////////////////////		bc_typeid_t

	//??? not needed anymore - we use int:s in interpreter, we can use typeid_t as global types.
	struct bc_typeid2_t {
		static std::vector<typeid_t> get_fulltype2(const typeid_t& type){
			if(type.is_undefined()){
				return {};
			}
			else if(type.is_bool()){
				return {};
			}
			else if(type.is_int()){
				return {};
			}
			else if(type.is_float()){
				return {};
			}
			else{
				return {type};
			}
		}


		bc_typeid2_t() :
			_basetype(base_type::k_internal_undefined),
			_fulltype()
		{
		}
		explicit bc_typeid2_t(const typeid_t& full) :
			_basetype(full.get_base_type()),
			_fulltype(get_fulltype2(full))
		{
		}
		base_type _basetype;

		public: bool operator==(const bc_typeid2_t& other) const{
			return get_fulltype() == other.get_fulltype();
		}

		public: typeid_t get_fulltype() const {
			if(_fulltype.empty()){
				if(_basetype == base_type::k_internal_undefined){
					return typeid_t::make_undefined();
				}
				else if(_basetype == base_type::k_internal_dynamic){
					return typeid_t::make_internal_dynamic();
				}
				else if(_basetype == base_type::k_void){
					return typeid_t::make_void();
				}
				else if(_basetype == base_type::k_bool){
					return typeid_t::make_bool();
				}
				else if(_basetype == base_type::k_int){
					return typeid_t::make_int();
				}
				else if(_basetype == base_type::k_float){
					return typeid_t::make_float();
				}
				else{
					QUARK_ASSERT(false);
					throw std::exception();
				}
			}
			else{
				return _fulltype.front();
			}
		}


		//	0 or 1 elements. Contains an element if _basetype doesn't fully describe the type.
		std::vector<typeid_t> _fulltype;
	};

	typedef int16_t bc_typeid_t;


	//////////////////////////////////////		bc_expression_t



	struct bc_expression_t {
		bc_expression_t(
			bc_expression_opcode opcode,

			bc_typeid_t type,

			std::vector<bc_expression_t> e,
			variable_address_t address,
			bc_value_t value,
			bc_typeid_t input_type
		):
			_opcode(opcode),
			_type(type),
			_e(e),
			_address_parent_step(address._parent_steps),
			_address_index(address._index),
			_value(value),
			_input_type(input_type)
		{
			QUARK_ASSERT(check_invariant());
		}

#if DEBUG
		public: bool check_invariant() const { return true; }
#endif

		//////////////////////////////////////		STATE

		std::vector<bc_expression_t> _e;			//	24
		int16_t _address_parent_step;
		int16_t _address_index;
		bc_typeid_t _type;
		bc_typeid_t _input_type;
		bc_value_t _value;							//	16
		bc_expression_opcode _opcode;
	};

	//	A memory block. Addressed using index. Always 1 cache line big.
	//	Prefetcher likes bigger blocks than this.
	struct bc_node_t {
		uint32_t _words[64 / sizeof(uint32_t)];
	};



	//////////////////////////////////////		bc_instruction_t



	struct bc_instruction_t {
		bc_statement_opcode _opcode;
		bc_typeid_t _type;

		std::vector<bc_expression_t> _e;
		variable_address_t _v;
		std::vector<bc_body_t> _b;


#if DEBUG
		public: bool check_invariant() const {
			return true;
		}
#endif

	};



	//////////////////////////////////////		bc_body_t



	struct bc_body_t {
		const std::vector<std::pair<std::string, symbol_t>> _symbols;

		std::vector<bc_instruction_t> _statements;

		bc_body_t(const std::vector<bc_instruction_t>& s) :
			_statements(s),
			_symbols{}
		{
		}
		bc_body_t(const std::vector<bc_instruction_t>& statements, const std::vector<std::pair<std::string, symbol_t>>& symbols) :
			_statements(statements),
			_symbols(symbols)
		{
		}
	};



	//////////////////////////////////////		bc_function_definition_t


	struct bc_function_definition_t {
#if DEBUG
		public: bool check_invariant() const {
			return true;
		}
#endif

		typeid_t _function_type;
		std::vector<member_t> _args;
		bc_body_t _body;
		int _host_function_id;
	};



	//////////////////////////////////////		bc_program_t



	struct bc_program_t {
#if DEBUG
		public: bool check_invariant() const {
//			QUARK_ASSERT(_globals.check_invariant());
			return true;
		}
#endif

		public: const bc_body_t _globals;
		public: std::vector<const bc_function_definition_t> _function_defs;
		public: std::vector<const bc_typeid2_t> _types;
	};


	//////////////////////////////////////		bcgen_context_t


	struct bcgen_context_t {
		public: quark::trace_context_t _tracer;
	};


	//////////////////////////////////////		bcgen_environment_t

	/*
		Runtime scope, similar to a stack frame.
	*/

	struct bcgen_environment_t {
		public: const body_t* _body_ptr;
	};


	//////////////////////////////////////		bgenerator_imm_t


	struct bgenerator_imm_t {
		////////////////////////		STATE
		public: const ast_t _ast_pass3;
	};


	//////////////////////////////////////		bgenerator_t

	/*
		Complete runtime state of the bgenerator.
		MUTABLE
	*/

	struct bgenerator_t {
		public: explicit bgenerator_t(const ast_t& pass3);
		public: bgenerator_t(const bgenerator_t& other);
		public: const bgenerator_t& operator=(const bgenerator_t& other);
#if DEBUG
		public: bool check_invariant() const;
#endif
		public: void swap(bgenerator_t& other) throw();

		////////////////////////		STATE
		public: std::shared_ptr<bgenerator_imm_t> _imm;

		//	Holds all values for all environments.
		public: std::vector<bcgen_environment_t> _call_stack;

		public: std::vector<const bc_typeid2_t> _types;
	};


	//////////////////////////		run_bggen()


	bc_program_t run_bggen(const quark::trace_context_t& tracer, const semantic_ast_t& pass3);

	json_t bcprogram_to_json(const bc_program_t& program);

} //	floyd


#endif /* bytecode_gen_h */
