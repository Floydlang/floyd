//
//  parser_evaluator.h
//  FloydSpeak
//
//  Created by Marcus Zetterquist on 26/07/16.
//  Copyright © 2016 Marcus Zetterquist. All rights reserved.
//

#ifndef floyd_interpreter_hpp
#define floyd_interpreter_hpp


#include "quark.h"

#include <string>
#include <vector>
#include <map>
#include "ast_value.h"
#include "host_functions.hpp"
#include "bytecode_gen.h"

namespace floyd {
	struct bc_value_t;
	struct interpreter_t;
	struct bc_program_t;
	struct bc_instruction_t;


	bc_value_t construct_value_from_typeid(interpreter_t& vm, const typeid_t& type, const typeid_t& arg0_type, const std::vector<bc_value_t>& arg_values);



	#define DEBUG_STACK (DEBUG & FLOYD_BD_DEBUG)



	//////////////////////////////////////		interpreter_stack_t



	struct interpreter_stack_t {
		public: interpreter_stack_t() :
			_internal_placeholder_object(bc_value_t::make_string("Internal placeholder object")),
			_current_stack_frame(0)
		{
			_value_stack.reserve(1024);

			QUARK_ASSERT(check_invariant());
		}

		public: ~interpreter_stack_t(){
			QUARK_ASSERT(check_invariant());
		}


		public: bool check_invariant() const {
			QUARK_ASSERT(_current_stack_frame >= 0);
			QUARK_ASSERT(_current_stack_frame <= _value_stack.size());
			QUARK_ASSERT(_internal_placeholder_object.check_invariant());
			QUARK_ASSERT(_exts.size() == _value_stack.size());
			for(int i = 0 ; i < _value_stack.size() ; i++){
//				QUARK_ASSERT(_value_stack[i].check_invariant());
			}
			return true;
		}


		public: interpreter_stack_t(const interpreter_stack_t& other) :
			_value_stack(other._value_stack),
			_exts(other._exts),
			_internal_placeholder_object(other._internal_placeholder_object),
			_current_stack_frame(other._current_stack_frame)
		{
			QUARK_ASSERT(other.check_invariant());

			//??? Cannot bump RC:s because we don't know which values!
			QUARK_ASSERT(false);
		}

		public: interpreter_stack_t& operator=(const interpreter_stack_t& other){
			QUARK_ASSERT(check_invariant());
			QUARK_ASSERT(other.check_invariant());

			interpreter_stack_t temp = other;
			temp.swap(*this);
			return *this;
		}

		public: void swap(interpreter_stack_t& other) throw() {
			QUARK_ASSERT(check_invariant());
			QUARK_ASSERT(other.check_invariant());

			other._value_stack.swap(_value_stack);
			other._exts.swap(_exts);
			_internal_placeholder_object.swap(other._internal_placeholder_object);
			std::swap(_current_stack_frame, other._current_stack_frame);

			QUARK_ASSERT(check_invariant());
			QUARK_ASSERT(other.check_invariant());
		}

		int size() const {
			QUARK_ASSERT(check_invariant());

			return static_cast<int>(_value_stack.size());
		}


		static bool is_compatible(bool is_ext, const typeid_t& stack_info){
			if(stack_info.is_unresolved_type_identifier()){
				const auto tag = stack_info.get_unresolved_type_identifier();
				if(tag == "OBJ" && is_ext == false){
					QUARK_ASSERT(false);
				}
				else if(tag == "INLINE" && is_ext == true){
					QUARK_ASSERT(false);
				}
				else{
					return true;
				}
			}
			else{
				bool is_ext_rhs = bc_value_t::is_bc_ext(stack_info.get_base_type());
				QUARK_ASSERT(is_ext == is_ext_rhs);
			}
			return true;
		}

		static bool is_compatible(const typeid_t& type, const typeid_t& stack_info){
			if(stack_info.is_unresolved_type_identifier()){
				const auto tag = stack_info.get_unresolved_type_identifier();
				bool is_ext = bc_value_t::is_bc_ext(type.get_base_type());
				if(tag == "OBJ" && is_ext == false){
					QUARK_ASSERT(false);
				}
				else if(tag == "INLINE" && is_ext == true){
					QUARK_ASSERT(false);
				}
				else{
					return true;
				}
			}
			else{
				QUARK_ASSERT(type == stack_info);
			}
			return true;
		}


		BC_INLINE void push_value(const bc_value_t& value, bool is_ext){
			QUARK_ASSERT(check_invariant());
			QUARK_ASSERT(value.check_invariant());
			QUARK_ASSERT(bc_value_t::is_bc_ext(value._debug_type.get_base_type()) == is_ext);

			if(is_ext){
				value._pod._ext->_rc++;
			}
			_value_stack.push_back(value._pod);
			_exts.push_back(is_ext);

			QUARK_ASSERT(check_invariant());
		}
		BC_INLINE void push_intq(int value){
			QUARK_ASSERT(check_invariant());

			bc_pod_value_t e;
			e._int = value;
			_value_stack.push_back(e);
			_exts.push_back(false);

			QUARK_ASSERT(check_invariant());
		}
		BC_INLINE void push_obj(const bc_value_t& value){
			QUARK_ASSERT(check_invariant());
			QUARK_ASSERT(value.check_invariant());
			QUARK_ASSERT(bc_value_t::is_bc_ext(value._debug_type.get_base_type()) == true);

			value._pod._ext->_rc++;
			_value_stack.push_back(value._pod);
			_exts.push_back(true);

			QUARK_ASSERT(check_invariant());
		}

/*
		BC_INLINE void push_values_no_rc_bump(const bc_pod_value_t* values, int value_count){
			QUARK_ASSERT(check_invariant());
			QUARK_ASSERT(values != nullptr);
			QUARK_ASSERT(value_count >= 0);

			_value_stack.insert(_value_stack.end(), values, values + value_count);
			for(int i = 0 ; i < value_count ; i++){
				_exts.push_back(false);
			}

			QUARK_ASSERT(check_invariant());
		}
*/


		//	returned value will have ownership of obj, if it's an obj.
		BC_INLINE bc_value_t load_value_slow(int pos, const typeid_t& type) const{
			QUARK_ASSERT(check_invariant());
			QUARK_ASSERT(pos >= 0 && pos < _value_stack.size());
			QUARK_ASSERT(type.check_invariant());

			const auto& e = _value_stack[pos];
			bool is_ext = bc_value_t::is_bc_ext(type.get_base_type());

			QUARK_ASSERT(is_ext == _exts[pos]);

#if DEBUG && FLOYD_BD_DEBUG
			const auto result = bc_value_t(type, e, is_ext);
#else
			const auto result = bc_value_t(e, is_ext);
#endif
			return result;
		}

		BC_INLINE bc_value_t load_inline_value(int pos) const{
			QUARK_ASSERT(check_invariant());
			QUARK_ASSERT(pos >= 0 && pos < _value_stack.size());
			QUARK_ASSERT(false == _exts[pos]);

#if DEBUG && FLOYD_BD_DEBUG
			const auto result = bc_value_t(typeid_t::make_unresolved_type_identifier("INLINE"), _value_stack[pos], false);
#else
			const auto result = bc_value_t(_value_stack[pos], false);
#endif
			return result;
		}

		BC_INLINE bc_value_t load_obj(int pos) const{
			QUARK_ASSERT(check_invariant());
			QUARK_ASSERT(pos >= 0 && pos < _value_stack.size());
			QUARK_ASSERT(true == _exts[pos]);

#if DEBUG && FLOYD_BD_DEBUG
			const auto result = bc_value_t(typeid_t::make_unresolved_type_identifier("OBJ"), _value_stack[pos], true);
#else
			const auto result = bc_value_t(_value_stack[pos], true);
#endif
			return result;
		}

		BC_INLINE int load_intq(int pos) const{
			QUARK_ASSERT(check_invariant());
			QUARK_ASSERT(pos >= 0 && pos < _value_stack.size());
			QUARK_ASSERT(false == _exts[pos]);

			return _value_stack[pos]._int;
		}
		BC_INLINE const bc_body_optimized_t* load_symbol_ptr(int pos) const{
			QUARK_ASSERT(check_invariant());
			QUARK_ASSERT(pos >= 0 && pos < _value_stack.size());
			QUARK_ASSERT(false == _exts[pos]);

			return _value_stack[pos]._symbol_table;
		}
		BC_INLINE void push_symbol_ptr(const bc_body_optimized_t* symbols){
			QUARK_ASSERT(check_invariant());
			QUARK_ASSERT(symbols != nullptr && symbols->check_invariant());

			bc_pod_value_t e;
			e._symbol_table = symbols;
			_value_stack.push_back(e);
			_exts.push_back(false);
		}

		//??? We could have support simple sumtype called DYN that holds a value_t at runtime.

		BC_INLINE void replace_value_same_type_SLOW(int pos, const bc_value_t& value, const typeid_t& type){
			QUARK_ASSERT(check_invariant());
			QUARK_ASSERT(value.check_invariant());
			QUARK_ASSERT(type.check_invariant());
			QUARK_ASSERT(pos >= 0 && pos < _value_stack.size());
			QUARK_ASSERT(value._debug_type == type);

			if(bc_value_t::is_bc_ext(type.get_base_type())){
				QUARK_ASSERT(true == _exts[pos]);
				replace_obj(pos, value);
			}
			else{
				QUARK_ASSERT(false == _exts[pos]);
				replace_inline(pos, value);
			}

			QUARK_ASSERT(check_invariant());
		}

		BC_INLINE void replace_inline(int pos, const bc_value_t& value){
			QUARK_ASSERT(check_invariant());
			QUARK_ASSERT(value.check_invariant());
			QUARK_ASSERT(pos >= 0 && pos < _value_stack.size());
			QUARK_ASSERT(bc_value_t::is_bc_ext(value._debug_type.get_base_type()) == false);
			QUARK_ASSERT(false == _exts[pos]);

			_value_stack[pos] = value._pod;

			QUARK_ASSERT(check_invariant());
		}

		BC_INLINE void replace_obj(int pos, const bc_value_t& value){
			QUARK_ASSERT(check_invariant());
			QUARK_ASSERT(value.check_invariant());
			QUARK_ASSERT(pos >= 0 && pos < _value_stack.size());
			QUARK_ASSERT(bc_value_t::is_bc_ext(value._debug_type.get_base_type()) == true);
			QUARK_ASSERT(true == _exts[pos]);

			auto prev_copy = _value_stack[pos];
			value._pod._ext->_rc++;
			_value_stack[pos] = value._pod;
			bc_value_t::debump(prev_copy);

			QUARK_ASSERT(check_invariant());
		}

		BC_INLINE void replace_int(int pos, const int& value){
			QUARK_ASSERT(check_invariant());
			QUARK_ASSERT(pos >= 0 && pos < _value_stack.size());
			QUARK_ASSERT(false == _exts[pos]);

			_value_stack[pos]._int = value;

			QUARK_ASSERT(check_invariant());
		}

		//	extbits[0] tells if the first popped value has ext. etc.
		//	bit 0 maps to the next value to be popped from stack
		//	Max 32 can be popped.
		BC_INLINE void pop_batch(int count, uint32_t extbits){
			QUARK_ASSERT(check_invariant());
			QUARK_ASSERT(_value_stack.size() >= count);
			QUARK_ASSERT(count >= 0);
			QUARK_ASSERT(count <= 32);

			uint32_t bits = extbits;
			for(int i = 0 ; i < count ; i++){
				bool ext = (bits & 1) ? true : false;
				pop(ext);
				bits = bits >> 1;
			}
			QUARK_ASSERT(check_invariant());
		}

		//	exts[exts.size() - 1] maps to the closed value on stack, the next to be popped.
		BC_INLINE void pop_batch(const std::vector<bool>& exts){
			QUARK_ASSERT(check_invariant());
			QUARK_ASSERT(_value_stack.size() >= exts.size());

			auto flag_index = exts.size() - 1;
			for(int i = 0 ; i < exts.size() ; i++){
				pop(exts[flag_index]);
				flag_index--;
			}
			QUARK_ASSERT(check_invariant());
		}
		BC_INLINE void pop(bool ext){
			QUARK_ASSERT(check_invariant());
			QUARK_ASSERT(_value_stack.empty() == false);
			QUARK_ASSERT(_exts.back() == ext);

			auto copy = _value_stack.back();
			_value_stack.pop_back();
			_exts.pop_back();
			if(ext){
				bc_value_t::debump(copy);
			}

			QUARK_ASSERT(check_invariant());
		}

		public: bool is_ext(int pos) const{
			QUARK_ASSERT(check_invariant());
			QUARK_ASSERT(pos >= 0 && pos < _value_stack.size());
			return _exts[pos];
		}
/*
		BC_INLINE void pop_value_slow(const typeid_t& type){
			QUARK_ASSERT(_value_stack.empty() == false);

			auto prev_copy = _value_stack.back();
			_value_stack.pop_back();
			bc_value_t::debump(prev_copy);
		}
*/

		public: bc_value_t _internal_placeholder_object;
		private: std::vector<bc_pod_value_t> _value_stack;
		private: std::vector<bool> _exts;
		public: int _current_stack_frame;
	};



	//////////////////////////////////////		interpreter_context_t



	struct interpreter_context_t {
		public: quark::trace_context_t _tracer;
	};


	//////////////////////////////////////		execution_result_t


	struct execution_result_t {
		enum output_type {

			//	_output != nullptr
			k_returning,

			//	_output != nullptr
			k_passive_expression_output,


			//	_output == nullptr
			k_complete_without_result_value
		};

		public: static execution_result_t make_return_unwind(const bc_value_t& return_value){
			return { execution_result_t::k_returning, return_value };
		}
		public: static execution_result_t make_passive_expression_output(const bc_value_t& output_value){
			return { execution_result_t::k_passive_expression_output, output_value };
		}
		public: static execution_result_t make__complete_without_value(){
			return { execution_result_t::k_complete_without_result_value, bc_value_t::make_undefined() };
		}

		private: execution_result_t(output_type type, const bc_value_t& output) :
			_type(type),
			_output(output)
		{
		}

		public: output_type _type;
		public: bc_value_t _output;
	};


	//////////////////////////////////////		interpreter_imm_t


	struct interpreter_imm_t {
		////////////////////////		STATE
		public: const std::chrono::time_point<std::chrono::high_resolution_clock> _start_time;
		public: const bc_program_t _program;
		public: const std::map<int, HOST_FUNCTION_PTR> _host_functions;
	};


	//////////////////////////////////////		interpreter_t

	/*
		Complete runtime state of the interpreter.
		MUTABLE
	*/

	struct interpreter_t {
		public: explicit interpreter_t(const bc_program_t& program);
		public: interpreter_t(const interpreter_t& other) = delete;
		public: const interpreter_t& operator=(const interpreter_t& other)= delete;
#if DEBUG
		public: bool check_invariant() const;
#endif
		public: void swap(interpreter_t& other) throw();

		////////////////////////		STATE
		public: std::shared_ptr<interpreter_imm_t> _imm;



		//	Holds all values for all environments.
		//	Notice: stack holds refs to RC-counted objects!
		public: interpreter_stack_t _value_stack;
		public: std::vector<std::string> _print_output;
	};


	value_t call_host_function(interpreter_t& vm, int function_id, const std::vector<value_t>& args);
	value_t call_function(interpreter_t& vm, const value_t& f, const std::vector<value_t>& args);
	json_t interpreter_to_json(const interpreter_t& vm);


	/*
		Return value:
			null = instructions were all executed through.
			value = return instruction returned a value.
	*/
	execution_result_t execute_instructions(interpreter_t& vm, const std::vector<bc_instruction_t>& instructions);



	//////////////////////////		run_main()

	struct value_entry_t {
		bc_value_t _value;
		std::string _symbol_name;
		symbol_t _symbol;
		variable_address_t _address;
	};

	value_t find_global_symbol(const interpreter_t& vm, const std::string& s);
	value_t get_global(const interpreter_t& vm, const std::string& name);
	std::shared_ptr<value_entry_t> find_global_symbol2(const interpreter_t& vm, const std::string& s);

	/*
		Quickie that compiles a program and calls its main() with the args.
	*/
	std::pair<std::shared_ptr<interpreter_t>, value_t> run_main(
		const interpreter_context_t& context,
		const std::string& source,
		const std::vector<value_t>& args
	);

	std::pair<std::shared_ptr<interpreter_t>, value_t> run_program(const interpreter_context_t& context, const bc_program_t& program, const std::vector<floyd::value_t>& args);

	bc_program_t program_to_ast2(const interpreter_context_t& context, const std::string& program);


	std::shared_ptr<interpreter_t> run_global(const interpreter_context_t& context, const std::string& source);

	void print_vm_printlog(const interpreter_t& vm);

} //	floyd


#endif /* floyd_interpreter_hpp */
