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
	struct expression_t;
	struct bc_value_t;
	struct statement_t;
	struct interpreter_t;
	struct bc_program_t;
	struct bc_instruction_t;


	bc_value_t construct_value_from_typeid(interpreter_t& vm, const typeid_t& type, const typeid_t& arg0_type, const std::vector<bc_value_t>& arg_values);







	//////////////////////////////////////		interpret_stack_element_t


	//	IMPORTANT: Has no constructor, destructor etc!! POD.
	//	??? rename to value_pod_t
	typedef bc_value_t::value_internals_t interpret_stack_element_t;



	//////////////////////////////////////		interpreter_stack_t



	struct interpreter_stack_t {
		public: interpreter_stack_t(){
			_value_stack.reserve(1024);
		}
		public: ~interpreter_stack_t(){
		}

		public: interpreter_stack_t(const interpreter_stack_t& other) :
			_value_stack(other._value_stack)
		{
			//??? Cannot bump RC:s because we don't know which values!
			QUARK_ASSERT(false);
		}

		public: interpreter_stack_t& operator=(const interpreter_stack_t& other){
			interpreter_stack_t temp = other;
			temp.swap(*this);
			return *this;
		}

		public: void swap(interpreter_stack_t& other) throw() {
			other._value_stack.swap(_value_stack);
		}

		int size() const {
			return static_cast<int>(_value_stack.size());
		}
		BC_INLINE void push_value(const bc_value_t& value, bool is_ext){
			if(is_ext){
				value._value_internals._ext->_rc++;
			}
			_value_stack.push_back(value._value_internals);
		}
		BC_INLINE void push_intq(int value){
			interpret_stack_element_t e;
			e._int = value;
			_value_stack.push_back(e);
		}
		BC_INLINE void push_obj(const bc_value_t& value){
			value._value_internals._ext->_rc++;
			_value_stack.push_back(value._value_internals);
		}

		BC_INLINE void push_values_no_rc_bump(const bc_value_t::value_internals_t* values, int value_count){
			QUARK_ASSERT(values != nullptr);

			_value_stack.insert(_value_stack.end(), values, values + value_count);
		}



		//	returned value will have ownership of obj, if it's an obj.
		BC_INLINE bc_value_t load_value_slow(int pos, const typeid_t& type) const{
			QUARK_ASSERT(pos >= 0 && pos < _value_stack.size());

			const auto& e = _value_stack[pos];
			bool is_ext = bc_value_t::is_bc_ext(type.get_base_type());
			return bc_value_t(e, is_ext);
		}

		BC_INLINE bc_value_t load_inline_value(int pos) const{
			QUARK_ASSERT(pos >= 0 && pos < _value_stack.size());

			return bc_value_t(_value_stack[pos], false);
		}

		BC_INLINE bc_value_t load_obj(int pos) const{
			QUARK_ASSERT(pos >= 0 && pos < _value_stack.size());

			return bc_value_t(_value_stack[pos], true);
		}

		BC_INLINE int load_intq(int pos) const{
			QUARK_ASSERT(pos >= 0 && pos < _value_stack.size());

			return _value_stack[pos]._int;
		}

/*
		BC_INLINE void replace_value_same_type_SLOW(int pos, const bc_value_t& value, const typeid_t& type){
			QUARK_ASSERT(pos >= 0 && pos < _value_stack.size());

			if(bc_value_t::is_bc_ext(type.get_base_type())){
				replace_obj(pos, value);
			}
			else{
				replace_inline(pos, value);
			}
		}
*/

		BC_INLINE void replace_inline(int pos, const bc_value_t& value){
			QUARK_ASSERT(pos >= 0 && pos < _value_stack.size());

			_value_stack[pos] = value._value_internals;
		}

		BC_INLINE void replace_obj(int pos, const bc_value_t& value){
			QUARK_ASSERT(pos >= 0 && pos < _value_stack.size());

			auto prev_copy = _value_stack[pos];
			value._value_internals._ext->_rc++;
			_value_stack[pos] = value._value_internals;
			bc_value_t::debump(prev_copy);
		}

		BC_INLINE void replace_int(int pos, const int& value){
			QUARK_ASSERT(pos >= 0 && pos < _value_stack.size());

			_value_stack[pos]._int = value;
		}

/*
		BC_INLINE void pop_value_slow(const typeid_t& type){
			QUARK_ASSERT(_value_stack.empty() == false);

			auto prev_copy = _value_stack.back();
			_value_stack.pop_back();
			bc_value_t::debump(prev_copy);
		}
*/

		public: std::vector<interpret_stack_element_t> _value_stack;
	};



	//////////////////////////////////////		statement_result_t



	struct interpreter_context_t {
		public: quark::trace_context_t _tracer;
	};


	//////////////////////////////////////		statement_result_t


	struct statement_result_t {
		enum output_type {

			//	_output != nullptr
			k_returning,

			//	_output != nullptr
			k_passive_expression_output,


			//	_output == nullptr
			k_complete_without_result_value
		};

		public: static statement_result_t make_return_unwind(const bc_value_t& return_value){
			return { statement_result_t::k_returning, return_value };
		}
		public: static statement_result_t make_passive_expression_output(const bc_value_t& output_value){
			return { statement_result_t::k_passive_expression_output, output_value };
		}
		public: static statement_result_t make__complete_without_value(){
			return { statement_result_t::k_complete_without_result_value, bc_value_t::make_undefined() };
		}

		private: statement_result_t(output_type type, const bc_value_t& output) :
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

		public: bc_value_t _internal_placeholder_object;


		//	Holds all values for all environments.
		//	Notice: stack holds refs to RC-counted objects!
		public: interpreter_stack_t _value_stack;
		public: int _current_stack_frame;
		public: std::vector<std::string> _print_output;
	};


	value_t call_host_function(interpreter_t& vm, int function_id, const std::vector<value_t>& args);
	value_t call_function(interpreter_t& vm, const value_t& f, const std::vector<value_t>& args);
	json_t interpreter_to_json(const interpreter_t& vm);


	/*
		Return value:
			null = statements were all executed through.
			value = return statement returned a value.
	*/
	statement_result_t execute_statements(interpreter_t& vm, const std::vector<bc_instruction_t>& statements);



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
