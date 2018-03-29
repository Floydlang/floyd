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


#define FLOYD_BD_DEBUG 0
#define BC_INLINE	inline
//#define BC_INLINE


namespace floyd {
	struct expression_t;
	struct value_t;
	struct statement_t;
	struct bgenerator_t;

	struct bc_body_t;
	struct bc_value_t;
	struct semantic_ast_t;

	typedef int16_t bc_typeid_t;

	value_t bc_to_value(const bc_value_t& value, const typeid_t& type);
	bc_value_t value_to_bc(const value_t& value);


	std::vector<bc_value_t> values_to_bcs(const std::vector<value_t>& values);
	std::vector<value_t> bcs_to_values__same_types(const std::vector<bc_value_t>& values, const typeid_t& shared_type);



	//////////////////////////////////////		bc_value_object_t



	struct bc_value_object_t {
#if DEBUG
		public: bool check_invariant() const{
			QUARK_ASSERT(_rc > 0);
#if DEBUG && FLOYD_BD_DEBUG
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
			}
			else if(base_type == base_type::k_json_value){
				QUARK_ASSERT(_string.empty());
				QUARK_ASSERT(_json_value != nullptr);
				QUARK_ASSERT(_typeid_value == typeid_t::make_undefined());
				QUARK_ASSERT(_struct_members.empty());
				QUARK_ASSERT(_vector_elements.empty());
				QUARK_ASSERT(_dict_entries.empty());

				QUARK_ASSERT(_json_value->check_invariant());
			}

			else if(base_type == base_type::k_typeid){
				QUARK_ASSERT(_string.empty());
				QUARK_ASSERT(_json_value == nullptr);
		//		QUARK_ASSERT(_typeid_value != typeid_t::make_undefined());
				QUARK_ASSERT(_struct_members.empty());
				QUARK_ASSERT(_vector_elements.empty());
				QUARK_ASSERT(_dict_entries.empty());

				QUARK_ASSERT(_typeid_value.check_invariant());
			}
			else if(base_type == base_type::k_struct){
				QUARK_ASSERT(_string.empty());
				QUARK_ASSERT(_json_value == nullptr);
				QUARK_ASSERT(_typeid_value == typeid_t::make_undefined());
//				QUARK_ASSERT(_struct != nullptr);
				QUARK_ASSERT(_vector_elements.empty());
				QUARK_ASSERT(_dict_entries.empty());

//				QUARK_ASSERT(_struct && _struct->check_invariant());
			}
			else if(base_type == base_type::k_vector){
				QUARK_ASSERT(_string.empty());
				QUARK_ASSERT(_json_value == nullptr);
				QUARK_ASSERT(_typeid_value == typeid_t::make_undefined());
				QUARK_ASSERT(_struct_members.empty());
		//		QUARK_ASSERT(_vector_elements.empty());
				QUARK_ASSERT(_dict_entries.empty());
			}
			else if(base_type == base_type::k_dict){
				QUARK_ASSERT(_string.empty());
				QUARK_ASSERT(_json_value == nullptr);
				QUARK_ASSERT(_typeid_value == typeid_t::make_undefined());
				QUARK_ASSERT(_struct_members.empty());
				QUARK_ASSERT(_vector_elements.empty());
		//		QUARK_ASSERT(_dict_entries.empty());
			}
			else if(base_type == base_type::k_function){
				QUARK_ASSERT(_string.empty());
				QUARK_ASSERT(_json_value == nullptr);
				QUARK_ASSERT(_typeid_value == typeid_t::make_undefined());
				QUARK_ASSERT(_struct_members.empty());
				QUARK_ASSERT(_vector_elements.empty());
				QUARK_ASSERT(_dict_entries.empty());
			}
			else {
				QUARK_ASSERT(false);
			}
#endif
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


		public: mutable int _rc;
#if DEBUG && FLOYD_BD_DEBUG
//??? use bc_typeid_t instead
		public: typeid_t _type;
#endif
		//	Holds ALL variants of objects right now -- optimize!
		public: std::string _string;
		public: std::shared_ptr<json_t> _json_value;
		public: typeid_t _typeid_value = typeid_t::make_undefined();
		public: std::vector<bc_value_t> _struct_members;
		public: std::vector<bc_value_t> _vector_elements;
		public: std::map<std::string, bc_value_t> _dict_entries;
	};



	//////////////////////////////////////		bc_value_t

	//	IMPORTANT: Has no constructor, destructor etc!! POD.

	union bc_pod_value_t {
		bool _bool;
		int _int;
		float _float;
		int _function_id;
		bc_value_object_t* _ext;
	};




	//////////////////////////////////////		bc_value_t



	struct bc_value_t {


		static BC_INLINE void debump(bc_pod_value_t& value){
			QUARK_ASSERT(value._ext != nullptr);

			value._ext->_rc--;
			if(value._ext->_rc == 0){
				delete value._ext;
				value._ext = nullptr;
			}
		}

		//	??? very slow?
		BC_INLINE static bool is_bc_ext(base_type basetype){
			return false
				|| basetype == base_type::k_string
				|| basetype == base_type::k_json_value
				|| basetype == base_type::k_typeid
				|| basetype == base_type::k_struct
				|| basetype == base_type::k_vector
				|| basetype == base_type::k_dict
				;
		}


		public: bc_value_t() :
#if DEBUG && FLOYD_BD_DEBUG
			_debug_type(typeid_t::make_undefined()),
#endif
			_is_ext(false)
		{
			_pod._ext = nullptr;
			QUARK_ASSERT(check_invariant());
		}

		public: ~bc_value_t(){
			QUARK_ASSERT(check_invariant());

			if(_is_ext){
				_pod._ext->_rc--;
				if(_pod._ext->_rc == 0){
					delete _pod._ext;
					_pod._ext = nullptr;
				}
			}
		}

#if DEBUG && FLOYD_BD_DEBUG
		public: typeid_t get_debug_type() const {
			return _debug_type;
			return typeid_t::make_undefined();
		}
#endif

		public: bc_value_t(const bc_value_t& other) :
#if DEBUG && FLOYD_BD_DEBUG
			_debug_type(other._debug_type),
#endif
			_is_ext(other._is_ext),
			_pod(other._pod)
		{
			QUARK_ASSERT(other.check_invariant());

			if(_is_ext){
				_pod._ext->_rc++;
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
			return compare_value_true_deep(*this, other) == 0;
		}
*/
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
				return _pod._bool == other._pod._bool;
			}
			else if(_basetype == base_type::k_int){
				return _pod._int == other._pod._int;
			}
			else if(_basetype == base_type::k_float){
				return _pod._float == other._pod._float;
			}
			else{
				QUARK_ASSERT(is_ext(_basetype));
				return compare_shared_values(_pod._ext, other._pod._ext);
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
			std::swap(_pod, other._pod);

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
#if DEBUG && FLOYD_BD_DEBUG
			QUARK_ASSERT(_debug_type.check_invariant());
#endif
			return true;
		}
#endif



		public: static int compare_value_true_deep(const bc_value_t& left, const bc_value_t& right, const typeid_t& type);


		//////////////////////////////////////		internal-undefined type


		public: static bc_value_t make_undefined(){
			return bc_value_t();
		}


		//////////////////////////////////////		internal-dynamic type


		public: static bc_value_t make_internal_dynamic(){
			return bc_value_t();
		}

		//////////////////////////////////////		void


		public: static bc_value_t make_void(){
			return bc_value_t();
		}


		//////////////////////////////////////		bool


		public: BC_INLINE static bc_value_t make_bool(bool v){
			return bc_value_t(v);
		}
		public: BC_INLINE bool get_bool_value() const {
			QUARK_ASSERT(check_invariant());

			return _pod._bool;
		}
		private: BC_INLINE explicit bc_value_t(bool value) :
#if DEBUG && FLOYD_BD_DEBUG
			_debug_type(typeid_t::make_bool()),
#endif
			_is_ext(false)
		{
			_pod._bool = value;
			QUARK_ASSERT(check_invariant());
		}


		//////////////////////////////////////		int


		public: BC_INLINE static bc_value_t make_int(int v){
			return bc_value_t{ v };
		}
		public: BC_INLINE int get_int_value() const {
			QUARK_ASSERT(check_invariant());

			return _pod._int;
		}
		private: BC_INLINE explicit bc_value_t(int value) :
#if DEBUG && FLOYD_BD_DEBUG
			_debug_type(typeid_t::make_int()),
#endif
			_is_ext(false)
		{
			_pod._int = value;
			QUARK_ASSERT(check_invariant());
		}


		//////////////////////////////////////		float


		public: BC_INLINE  static bc_value_t make_float(float v){
			return bc_value_t{ v };
		}
		public: float get_float_value() const {
			QUARK_ASSERT(check_invariant());

			return _pod._float;
		}
		private: BC_INLINE  explicit bc_value_t(float value) :
#if DEBUG && FLOYD_BD_DEBUG
			_debug_type(typeid_t::make_float()),
#endif
			_is_ext(false)
		{
			_pod._float = value;
			QUARK_ASSERT(check_invariant());
		}


		//////////////////////////////////////		string


		public: BC_INLINE  static bc_value_t make_string(const std::string& v){
			return bc_value_t{ v };
		}
		public: BC_INLINE  std::string get_string_value() const{
			QUARK_ASSERT(check_invariant());

			return _pod._ext->_string;
		}
		private: BC_INLINE  explicit bc_value_t(const std::string value) :
#if DEBUG && FLOYD_BD_DEBUG
			_debug_type(typeid_t::make_string()),
#endif
			_is_ext(true)
		{
			_pod._ext = new bc_value_object_t{value};
			QUARK_ASSERT(check_invariant());
		}


		//////////////////////////////////////		json_value


		public: BC_INLINE  static bc_value_t make_json_value(const json_t& v){
			return bc_value_t{ std::make_shared<json_t>(v) };
		}
		public: BC_INLINE  json_t get_json_value() const{
			QUARK_ASSERT(check_invariant());

			return *_pod._ext->_json_value.get();
		}
		private: BC_INLINE  explicit bc_value_t(const std::shared_ptr<json_t>& value) :
#if DEBUG && FLOYD_BD_DEBUG
			_debug_type(typeid_t::make_json_value()),
#endif
			_is_ext(true)
		{
			_pod._ext = new bc_value_object_t{value};
			QUARK_ASSERT(check_invariant());
		}


		//////////////////////////////////////		typeid


		public: BC_INLINE  static bc_value_t make_typeid_value(const typeid_t& type_id){
			return bc_value_t{ type_id };
		}
		public: BC_INLINE  typeid_t get_typeid_value() const {
			QUARK_ASSERT(check_invariant());

			return _pod._ext->_typeid_value;
		}
		private: BC_INLINE  explicit bc_value_t(const typeid_t& type_id) :
#if DEBUG && FLOYD_BD_DEBUG
			_debug_type(typeid_t::make_typeid()),
#endif
			_is_ext(true)
		{
			_pod._ext = new bc_value_object_t{type_id};
			QUARK_ASSERT(check_invariant());
		}


		//////////////////////////////////////		struct


		public: BC_INLINE  static bc_value_t make_struct_value(const typeid_t& struct_type, const std::vector<bc_value_t>& values){
			return bc_value_t{ struct_type, values, true };
		}
		public: BC_INLINE  const std::vector<bc_value_t>& get_struct_value() const {
			return _pod._ext->_struct_members;
		}
		private: BC_INLINE  explicit bc_value_t(const typeid_t& struct_type, const std::vector<bc_value_t>& values, bool struct_tag) :
#if DEBUG && FLOYD_BD_DEBUG
			_debug_type(struct_type),
#endif
			_is_ext(true)
		{
			_pod._ext = new bc_value_object_t{struct_type, values, true};
			QUARK_ASSERT(check_invariant());
		}


		//////////////////////////////////////		vector

		//??? remove all use of typeid_t -- clients will allocate them even if we don't keep the instances!
		public: BC_INLINE static bc_value_t make_vector_value(const typeid_t& element_type, const std::vector<bc_value_t>& elements){
			return bc_value_t{element_type, elements};
		}
		public: BC_INLINE const std::vector<bc_value_t>& get_vector_value() const{
			QUARK_ASSERT(check_invariant());

			return _pod._ext->_vector_elements;
		}
		private: BC_INLINE explicit bc_value_t(const typeid_t& element_type, const std::vector<bc_value_t>& values) :
#if DEBUG && FLOYD_BD_DEBUG
			_debug_type(typeid_t::make_vector(element_type)),
#endif
			_is_ext(true)
		{
			_pod._ext = new bc_value_object_t{typeid_t::make_vector(element_type), values};
			QUARK_ASSERT(check_invariant());
		}


		//////////////////////////////////////		dict


		public: BC_INLINE static bc_value_t make_dict_value(const typeid_t& value_type, const std::map<std::string, bc_value_t>& entries){
			return bc_value_t{value_type, entries};
		}

		public: BC_INLINE const std::map<std::string, bc_value_t>& get_dict_value() const{
			QUARK_ASSERT(check_invariant());

			return _pod._ext->_dict_entries;
		}
		private: BC_INLINE explicit bc_value_t(const typeid_t& value_type, const std::map<std::string, bc_value_t>& entries) :
#if DEBUG && FLOYD_BD_DEBUG
			_debug_type(typeid_t::make_dict(value_type)),
#endif
			_is_ext(true)
		{
			_pod._ext = new bc_value_object_t{typeid_t::make_dict(value_type), entries};
			QUARK_ASSERT(check_invariant());
		}


		//////////////////////////////////////		function


		public: BC_INLINE static bc_value_t make_function_value(const typeid_t& function_type, int function_id){
			return bc_value_t{ function_type, function_id, true };
		}
		public: BC_INLINE int get_function_value() const{
			QUARK_ASSERT(check_invariant());

			return _pod._function_id;
		}
		private: BC_INLINE explicit bc_value_t(const typeid_t& function_type, int function_id, bool dummy) :
#if DEBUG && FLOYD_BD_DEBUG
			_debug_type(function_type),
#endif
			_is_ext(false)
		{
			_pod._function_id = function_id;
			QUARK_ASSERT(check_invariant());
		}



		friend bc_value_t value_to_bc(const value_t& value);


		//	Bumps RC.
		public: BC_INLINE static bc_value_t make_object(bc_value_object_t* ext){
			bc_value_t temp;
			temp._is_ext = true;
			ext->_rc++;
			temp._pod._ext = ext;
			return temp;
		}

		//	YES bump.
		public: BC_INLINE explicit bc_value_t(const bc_pod_value_t& internals, bool is_ext) :
			_pod(internals),
			_is_ext(is_ext)
		{
			if(_is_ext){
				_pod._ext->_rc++;
			}
		}


		//////////////////////////////////////		STATE




#if DEBUG && FLOYD_BD_DEBUG
		private: typeid_t _debug_type;
#endif
		private: bool _is_ext;
		public: bc_pod_value_t _pod;
	};


	std::string to_compact_string2(const bc_value_t& value);


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



/*
	enum class bc_opcode: uint8_t {
		k_opcode__return_a,
		k_opcode__branch_if_true,


		k_opcode__add_object_abc,
		k_opcode__add_int_abc,
		k_opcode__add_float_abc,
		k_opcode__add_string_abc,
		k_opcode__add_vector_abc,

		k_opcode__resolve_member_int_ab,
		k_opcode__lookup_element_int_ab,


		k_opcode__load_global_inline,

		//	"resolve" = local variable or parent env chain.
		k_opcode__load_resolve_inline,
		k_opcode__load_int,

		k_opcode__load_obj,


		k_opcode__call,
		k_opcode__construct_value,

//		k_opcode__arithmetic_unary_minus,

		//	replace by k_statement_if.
		k_opcode__conditional_operator3,

		k_opcode__comparison_smaller_or_equal,
		k_opcode__comparison_smaller_or_equal__int,
		k_opcode__comparison_smaller,
		k_opcode__comparison_larger_or_equal,
		k_opcode__comparison_larger,

		k_opcode__logical_equal,
		k_opcode__logical_nonequal,

		k_opcode__arithmetic_add,
		k_opcode__arithmetic_add__int,
		k_opcode__arithmetic_subtract,
		k_opcode__arithmetic_subtract__int,
		k_opcode__arithmetic_multiply,
		k_opcode__arithmetic_divide,
		k_opcode__arithmetic_remainder,

		k_opcode__logical_and,
		k_opcode__logical_or
	};
*/



/*	//////////////////////////////////////		bc_expression_opcode



	enum class bc_expression_opcode: uint8_t {
		k_expression_illegal_opcode = 0,

		k_expression_literal,
		k_expression_literal__int,
		k_expression_resolve_member,
		k_expression_lookup_element,



		k_expression_load_global_inline,

		//	"resolve" = local variable or parent env chain.
		k_expression_load_resolve_inline,
		k_expression_load_int,

		k_expression_load_obj,



		k_expression_call,
		k_expression_construct_value,

//		k_expression_arithmetic_unary_minus,

		//	replace by k_statement_if.
		k_expression_conditional_operator3,


		k_expression_comparison_smaller_or_equal,
		k_expression_comparison_smaller,
		k_expression_comparison_larger_or_equal,
		k_expression_comparison_larger,

		k_expression_logical_equal,
		k_expression_logical_nonequal,


		k_expression_arithmetic_add,
		k_expression_arithmetic_add__int,
		k_expression_arithmetic_subtract,
		k_expression_arithmetic_subtract__int,
		k_expression_arithmetic_multiply,
		k_expression_arithmetic_divide,
		k_expression_arithmetic_remainder,

		k_expression_logical_and,
		k_expression_logical_or
	};
*/

inline int bc_limit(int value, int min, int max){
	if(value < min){
		return min;
	}
	else if(value > max){
		return max;
	}
	else{
		return value;
	}
}


/*
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
*/



	//////////////////////////////////////		bc_expression_t


/*
	struct bc_expression_t {
		private: bc_expression_t() :
			_opcode(bc_expression_opcode::k_expression_illegal_opcode),
			_pad(0),
			_type(-1),
			_e(nullptr),
			_e_count(0),
			_address_parent_step(0),
			_address_index(0),
			_value(),
			_input_type(-1)
		{
			QUARK_ASSERT(check_invariant());
		}

		public: bc_expression_t(const bc_expression_t& other) :
			_opcode(other._opcode),
			_pad(other._pad),
			_type(other._type),
			_e(nullptr),
			_e_count(other._e_count),
			_address_parent_step(other._address_parent_step),
			_address_index(other._address_index),
			_value(other._value),
			_input_type(other._input_type)
		{
			QUARK_ASSERT(other.check_invariant());
			if(_e_count > 0){
				bc_expression_t* ptr = new bc_expression_t[_e_count];
				for( int i = 0; i < _e_count; ++i ) {
					ptr[i] = other._e[i];
				}
				_e = ptr;
			}
			QUARK_ASSERT(check_invariant());
		}

		public: bc_expression_t& operator=(const bc_expression_t& other){
			QUARK_ASSERT(check_invariant());
			QUARK_ASSERT(other.check_invariant());

			bc_expression_t temp = other;
			temp.swap(*this);

			QUARK_ASSERT(check_invariant());
			return *this;
		}

		public: void swap(bc_expression_t& other) throw() {
			QUARK_ASSERT(check_invariant());
			QUARK_ASSERT(other.check_invariant());

			std::swap(_opcode, other._opcode);
			std::swap(_pad, other._pad);
			std::swap(_type, other._type);
			std::swap(_e, other._e);
			std::swap(_e_count, other._e_count);
			std::swap(_address_parent_step, other._address_parent_step);
			std::swap(_address_index, other._address_index);
			_value.swap(other._value);
			std::swap(_input_type, other._input_type);

			QUARK_ASSERT(check_invariant());
			QUARK_ASSERT(other.check_invariant());
		}


		bc_expression_t(
			bc_expression_opcode opcode,

			bc_typeid_t type,

			std::vector<bc_expression_t> e,
			variable_address_t address,
			bc_value_t value,
			bc_typeid_t input_type
		):
			_opcode(opcode),
			_pad(0),
			_type(type),
			_e(nullptr),
			_e_count(static_cast<uint16_t>(e.size())),
			_address_parent_step(static_cast<int16_t>(address._parent_steps)),
			_address_index(static_cast<int16_t>(address._index)),
			_value(value),
			_input_type(input_type)
		{
			if(_e_count > 0){
				bc_expression_t* ptr = new bc_expression_t[_e_count];
				for( int i = 0; i < _e_count; ++i ) {
					ptr[i] = e[i];
				}
				_e = ptr;
			}

			QUARK_ASSERT(check_invariant());
		}

		~bc_expression_t(){
			if(_e != nullptr){
				delete[] _e;
				_e = nullptr;
			}
		}

#if DEBUG
		public: bool check_invariant() const { return true; }
#endif

		//////////////////////////////////////		STATE

		bc_expression_opcode _opcode;	//	1
		uint8_t _pad;	//	1
		bc_typeid_t _type;	//	2
		uint16_t _e_count;	//	2
		bc_typeid_t _input_type;	//	2

//pos = 8
		const bc_expression_t* _e;			//	8

//	pos=16
		bc_value_t _value;							//	16

		int16_t _address_parent_step;	//	2
		int16_t _address_index;	//	2

	};

	//	A memory block. Addressed using index. Always 1 cache line big.
	//	Prefetcher likes bigger blocks than this.
	struct bc_node_t {
		uint32_t _words[64 / sizeof(uint32_t)];
	};
*/


	//////////////////////////////////////		bc_instruction_t

	enum class bc_statement_opcode: uint8_t {
		k_nop,

		k_opcode_resolve_member,
		k_opcode_lookup_element,
		k_opcode_call,


		k_opcode_arithmetic_add,
		k_opcode_arithmetic_subtract,
		k_opcode_arithmetic_multiply,
		k_opcode_arithmetic_divide,
		k_opcode_arithmetic_remainder,

		k_opcode_logical_and,
		k_opcode_logical_or,

		k_opcode_comparison_smaller_or_equal,
		k_opcode_comparison_smaller,
		k_opcode_comparison_larger_or_equal,
		k_opcode_comparison_larger,

		k_opcode_logical_equal,
		k_opcode_logical_nonequal,


		k_statement_store_resolve,


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


	//	Replace by int when we have flattened local bodies.
	typedef variable_address_t reg_t;

	struct bc_instruction_t {
		bc_instruction_t(
			bc_statement_opcode opcode,
			bc_typeid_t type,
			variable_address_t reg1,
			variable_address_t reg2,
			variable_address_t reg3
		) :
			_opcode(opcode),
			_instr_type(type),
			_reg1(reg1),
			_reg2(reg2),
			_reg3(reg3)
		{
			QUARK_ASSERT(check_invariant());
		}


#if DEBUG
		public: bool check_invariant() const {
			return true;
		}
#endif


		//////////////////////////////////////		STATE

		bc_statement_opcode _opcode;
		variable_address_t _reg1;
		variable_address_t _reg2;
		variable_address_t _reg3;

		//??? temporary. Plan is to embedd this type into opcode.
		bc_typeid_t _instr_type;
	};



	//////////////////////////////////////		bc_body_t



	struct bc_body_t {
		std::vector<std::pair<std::string, symbol_t>> _symbols;

		//	True if equivalent symbol is an ext.
		std::vector<bool> _exts;

		std::vector<bc_instruction_t> _statements;



		bc_body_t(const std::vector<bc_instruction_t>& s) :
			_statements(s),
			_symbols{}
		{
			QUARK_ASSERT(check_invariant());
		}

		bc_body_t(const std::vector<bc_instruction_t>& statements, const std::vector<std::pair<std::string, symbol_t>>& symbols) :
			_statements(statements),
			_symbols(symbols)
		{
			for(int i = 0 ; i < _symbols.size() ; i++){
				const auto basetype = symbols[i].second._value_type.get_base_type();
				const bool ext = bc_value_t::is_bc_ext(basetype);
				_exts.push_back(ext);
			}
			QUARK_ASSERT(check_invariant());
		}

		public: bool check_invariant() const {
			QUARK_ASSERT(_symbols.size() == _exts.size());
			return true;
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
		public: std::vector<const typeid_t> _types;
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

		public: std::vector<const typeid_t> _types;
	};


	//////////////////////////		run_bggen()


	bc_program_t run_bggen(const quark::trace_context_t& tracer, const semantic_ast_t& pass3);

	json_t bcprogram_to_json(const bc_program_t& program);

} //	floyd


#endif /* bytecode_gen_h */
