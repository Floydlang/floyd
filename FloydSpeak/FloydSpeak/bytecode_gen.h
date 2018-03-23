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

namespace floyd {
	struct expression_t;
	struct value_t;
	struct statement_t;
	struct bgenerator_t;

	struct bc_body_t;
	struct bc_value_t;


	inline value_t bc_to_value(const bc_value_t& value);
	inline bc_value_t value_to_bc(const value_t& value);


	inline std::vector<bc_value_t> values_to_bcs(const std::vector<value_t>& values);
	inline std::vector<value_t> bcs_to_values(const std::vector<bc_value_t>& values);



	//////////////////////////////////////		bc_value_t



	struct bc_value_t {
		public: value_t _backstore;

		public: bc_value_t(){
		}

		private: explicit bc_value_t(const value_t& value) :
			_backstore(value)
		{
		}

		public: bool check_invariant() const {
			return _backstore.check_invariant();
		}
		public: typeid_t get_type() const {
			return _backstore.get_type();
		}
		public: base_type get_basetype() const {
			return _backstore.get_basetype();
		}

		public: bool operator==(const bc_value_t& other) const{
			return _backstore == other._backstore;
		}

		private: bool is_null() const {
			return _backstore.is_null();
		}

		public: static bc_value_t make_null(){
			return {};
		}

		private: bool is_bool() const {
			return _backstore.is_bool();
		}
		public: static bc_value_t make_bool(bool v){
			return bc_value_t{ value_t::make_bool(v) };
		}
		public: bool get_bool_value_quick() const {
			return _backstore.get_bool_value_quick();
		}

		private: bool is_int() const {
			return _backstore.is_int();
		}
		public: static bc_value_t make_int(int v){
			return bc_value_t{ value_t::make_int(v) };
		}
		public: int get_int_value() const {
			return _backstore.get_int_value();
		}
		public: int get_int_value_quick() const {
			return _backstore.get_int_value_quick();
		}

		private: bool is_float() const {
			return _backstore.is_float();
		}
		public: static bc_value_t make_float(float v){
			return bc_value_t{ value_t::make_float(v) };
		}
		public: float get_float_value() const {
			return _backstore.get_float_value();
		}

		private: bool is_string() const {
			return _backstore.is_string();
		}
		public: static bc_value_t make_string(const std::string& v){
			return bc_value_t{ value_t::make_string(v) };
		}
		public: std::string get_string_value() const{
			return _backstore.get_string_value();
		}


		private: bool is_function() const {
			return _backstore.is_function();
		}
		public: int get_function_value() const{
			return _backstore.get_function_value();
		}

		private: bool is_vector() const {
			return _backstore.is_vector();
		}
		private: bool is_struct() const {
			return _backstore.is_struct();
		}
		private: bool is_dict() const {
			return _backstore.is_dict();
		}

		private: bool is_typeid() const {
			return _backstore.is_typeid();
		}
		public: typeid_t get_typeid_value() const {
			return _backstore.get_typeid_value();
		}
		public: std::shared_ptr<struct_instance_t> get_struct_value() const {
			return _backstore.get_struct_value();
		}

		public: const std::vector<bc_value_t> get_vector_value() const{
			return values_to_bcs(_backstore.get_vector_value());
		}

		public: static bc_value_t make_struct_value(const typeid_t& struct_type, const std::vector<bc_value_t>& values){
			return bc_value_t{ value_t::make_struct_value(struct_type, bcs_to_values(values)) };
		}
		public: static bc_value_t make_struct_value(const typeid_t& struct_type, const std::vector<value_t>& values){
			return bc_value_t{ value_t::make_struct_value(struct_type, values) };
		}

		public: static bc_value_t make_vector_value(const typeid_t& element_type, const std::vector<bc_value_t>& elements){
			return bc_value_t{value_t::make_vector_value(element_type, bcs_to_values(elements))};
		}

		public: static bc_value_t make_dict_value(const typeid_t& value_type, const std::map<std::string, bc_value_t>& entries){
			std::map<std::string, value_t> elements2;
			for(const auto e: entries){
				elements2.insert({e.first, bc_to_value(e.second)});
			}
			return bc_value_t{value_t::make_dict_value(value_type, elements2)};
		}

		public: const std::map<std::string, bc_value_t> get_dict_value() const{
			std::map<std::string, bc_value_t> result;
			const auto& elements = _backstore.get_dict_value();
			for(const auto e: elements){
				result.insert({e.first, value_to_bc(e.second)});
			}
			return result;
		}

		public: static int compare_value_true_deep(const bc_value_t& left, const bc_value_t& right){
			return value_t::compare_value_true_deep(left._backstore, right._backstore);
		}

		private: bool is_json_value() const {
			return _backstore.is_json_value();
		}
		public: static bc_value_t make_json_value(const json_t& v){
			return bc_value_t{ value_t::make_json_value(v) };
		}
		public: json_t get_json_value() const{
			return _backstore.get_json_value();
		}



		friend bc_value_t value_to_bc(const value_t& value);

	};



	//////////////////////////////////////		value helpers



	inline std::vector<bc_value_t> values_to_bcs(const std::vector<value_t>& values){
		std::vector<bc_value_t> result;
		for(const auto e: values){
			result.push_back(value_to_bc(e));
		}
		return result;
	}

	inline std::vector<value_t> bcs_to_values(const std::vector<bc_value_t>& values){
		std::vector<value_t> result;
		for(const auto e: values){
			result.push_back(bc_to_value(e));
		}
		return result;
	}

	inline value_t bc_to_value(const bc_value_t& value){
		return value._backstore;
	}
	inline bc_value_t value_to_bc(const value_t& value){
		return bc_value_t{value};
	}

	inline std::string to_compact_string2(const bc_value_t& value) {
		return to_compact_string2(value._backstore);
	}



	//////////////////////////////////////		bc_struct_instance_t



	struct bc_struct_instance_t {
		public: std::vector<bc_value_t> _member_values;
	};




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

	enum bc_statement_opcode {
		k_nop,

		//	Store _e[0] -> _v
		k_statement_store,

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



	enum bc_expression_opcode {
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



	//////////////////////////////////////		get_fulltype2()



	inline std::vector<typeid_t> get_fulltype2(const typeid_t& type){
		if(type.is_null()){
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



	//////////////////////////////////////		bc_typeid_t



	struct bc_typeid_t {
		bc_typeid_t() :
			_basetype(base_type::k_null),
			_fulltype()
		{
		}
		explicit bc_typeid_t(const typeid_t& full) :
			_basetype(full.get_base_type()),
			_fulltype(get_fulltype2(full))
		{
		}
		base_type _basetype;


		public: typeid_t get_fulltype() const {
			if(_fulltype.empty()){
				if(_basetype == base_type::k_null){
					return typeid_t::make_null();
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



	//////////////////////////////////////		bc_expression_t



	struct bc_expression_t {
		bc_expression_opcode _opcode;

		bc_typeid_t _type;

		std::vector<bc_expression_t> _e;
		variable_address_t _address;
		bc_value_t _value;
		bc_typeid_t _input_type;

		public: bool check_invariant() const { return true; }
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


		public: bool check_invariant() const {
			return true;
		}
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
		public: bool check_invariant() const {
			return true;
		}


		typeid_t _function_type;
		std::vector<member_t> _args;
		bc_body_t _body;
		int _host_function_id;
	};



	//////////////////////////////////////		bc_program_t



	struct bc_program_t {
		public: bool check_invariant() const {
//			QUARK_ASSERT(_globals.check_invariant());
			return true;
		}

		public: const bc_body_t _globals;
		public: std::vector<const bc_function_definition_t> _function_defs;
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
	};


	//////////////////////////		run_bggen()


	bc_program_t run_bggen(const quark::trace_context_t& tracer, const ast_t& pass3);

	json_t bcprogram_to_json(const bc_program_t& program);

} //	floyd


#endif /* bytecode_gen_h */
