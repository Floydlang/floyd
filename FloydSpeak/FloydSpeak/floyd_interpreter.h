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

	struct value_entry_t;


	enum {
		//	We store prev-frame-pos & symbol-ptr.
		k_frame_overhead = 2
	};


	//////////////////////////////////////		frame_pos_t



	struct frame_pos_t {
/*
		frame_pos_t() :
			_frame_pos(0),
			_frame_ptr(nullptr)
		{
		}
		frame_pos_t(int frame_pos, const bc_frame_t* frame) :
			_frame_pos(frame_pos),
			_frame_ptr(frame)
		{
		}
*/

		int _frame_pos;
		const bc_frame_t* _frame_ptr;
	};

	inline bool operator==(const frame_pos_t& lhs, const frame_pos_t& rhs){
		return lhs._frame_pos == rhs._frame_pos && lhs._frame_ptr == rhs._frame_ptr;
	}

	//////////////////////////////////////		interpreter_stack_t


/*
	0	[int = 0] 		previous stack frame pos, 0 = global
	1	[symbols_ptr frame #0]
	2	[local0]		<- stack frame #0
	3	[local1]
	4	[local2]

	5	[int = 1] //	prev stack frame pos
	1	[symbols_ptr frame #1]
	7	[local1]		<- stack frame #1
	8	[local2]
	9	[local3]
*/



	struct interpreter_stack_t {
		public: interpreter_stack_t(const bc_frame_t* global_frame) :
			_current_frame_pos(0),
			_current_frame_ptr(nullptr),
			_current_frame_entry_ptr(nullptr),
			_global_frame(global_frame),
			_entries(nullptr),
			_allocated_count(0),
			_stack_size(0)
		{
			_entries = new bc_pod_value_t[8192];
			_allocated_count = 8192;
			_current_frame_entry_ptr = &_entries[_current_frame_pos];

			QUARK_ASSERT(check_invariant());
		}

		public: ~interpreter_stack_t(){
			QUARK_ASSERT(check_invariant());

			delete[] _entries;
			_entries = nullptr;
		}


		public: bool check_invariant() const {
			QUARK_ASSERT(_entries != nullptr);
			QUARK_ASSERT(_stack_size >= 0 && _stack_size <= _allocated_count);
			QUARK_ASSERT(_current_frame_pos >= 0);
			QUARK_ASSERT(_current_frame_pos <= _stack_size);

			QUARK_ASSERT(_current_frame_entry_ptr == &_entries[_current_frame_pos]);

			QUARK_ASSERT(_debug_types.size() == _stack_size);
			for(int i = 0 ; i < _stack_size ; i++){
				QUARK_ASSERT(_debug_types[i].check_invariant());
			}
//			QUARK_ASSERT(_global_frame != nullptr);
			return true;
		}

		public: interpreter_stack_t(const interpreter_stack_t& other) = delete;

		public: interpreter_stack_t& operator=(const interpreter_stack_t& other) = delete;

/*
		public: interpreter_stack_t& operator=(const interpreter_stack_t& other){
			QUARK_ASSERT(check_invariant());
			QUARK_ASSERT(other.check_invariant());

			interpreter_stack_t temp = other;
			temp.swap(*this);
			return *this;
		}
*/

		public: void swap(interpreter_stack_t& other) throw() {
			QUARK_ASSERT(check_invariant());
			QUARK_ASSERT(other.check_invariant());

			std::swap(other._entries, _entries);
			std::swap(other._allocated_count, _allocated_count);
			std::swap(other._stack_size, _stack_size);
#if DEBUG
			other._debug_types.swap(_debug_types);
#endif
			std::swap(_current_frame_pos, other._current_frame_pos);
			std::swap(_current_frame_ptr, other._current_frame_ptr);
			std::swap(_current_frame_entry_ptr, other._current_frame_entry_ptr);

			std::swap(_global_frame, other._global_frame);

			QUARK_ASSERT(check_invariant());
			QUARK_ASSERT(other.check_invariant());
		}

		public: inline int size() const {
			QUARK_ASSERT(check_invariant());

			return static_cast<int>(_stack_size);
		}




		public: bool check_global_access_obj(const int global_index) const{
			QUARK_ASSERT(check_invariant());
			QUARK_ASSERT(global_index >= 0 && global_index < (k_frame_overhead + _global_frame->_symbols.size()));
			QUARK_ASSERT(_global_frame->_exts[global_index] == true);
			return true;
		}
		public: bool check_global_access_intern(const int global_index) const{
			QUARK_ASSERT(check_invariant());
			QUARK_ASSERT(global_index >= 0 && global_index < (k_frame_overhead + _global_frame->_symbols.size()));
			QUARK_ASSERT(_global_frame->_exts[global_index] == false);
			return true;
		}




		//////////////////////////////////////		FRAME


		public: int read_prev_frame_pos(int frame_pos) const;
		public: frame_pos_t read_prev_frame_pos2(int frame_pos) const;

		public: const bc_frame_t* read_prev_frame(int frame_pos) const;
#if DEBUG
		public: bool check_stack_frame(int frame_pos, const bc_frame_t* frame) const;
#endif

		//	??? DYN values /returns needs TWO registers.
		//	??? This function should just allocate a block for frame, then have a list of writes. ALTERNATIVELY: generatet instructions to do this in the VM?
		//	Returns new frame-pos, same as vm._current_stack_frame.
		public: void open_frame(const bc_frame_t& frame, int values_already_on_stack){
			QUARK_ASSERT(check_invariant());
			QUARK_ASSERT(frame.check_invariant());
			QUARK_ASSERT(values_already_on_stack == frame._args.size());

			const auto stack_end = size();
			const auto parameter_count = static_cast<int>(frame._args.size());

			//	Carefully position the new stack frame so its includes the parameters that already sits in the stack.
			//	The stack frame already has symbols/registers mapped for those parameters.
			const auto new_frame_pos = stack_end - parameter_count;

			for(int i = 0 ; i < frame._locals.size() ; i++){
				bool ext = frame._locals_exts[i];
				const auto& local = frame._locals[i];
				if(ext){
					push_obj(local);
				}
				else{
					push_intern(local);
				}
			}
			_current_frame_pos = new_frame_pos;
			_current_frame_ptr = &frame;
			_current_frame_entry_ptr = &_entries[_current_frame_pos];
		}


		//	Pops all locals, decrementing RC when needed.
		//	Decrements all stack frame object RCs.
		//	Caller handles RC for parameters, this function don't.
		public: void close_frame(const bc_frame_t& frame){
			QUARK_ASSERT(check_invariant());
			QUARK_ASSERT(frame.check_invariant());

			//	Using symbol table to figure out which stack-frame values needs RC. Decrement them all.
			pop_batch(frame._locals_exts);
		}

		public: std::vector<std::pair<int, int>> get_stack_frames(int frame_pos) const;

		public: inline frame_pos_t find_frame_pos(int parent_step) const{
			QUARK_ASSERT(check_invariant());

			QUARK_ASSERT(parent_step == 0 || parent_step == -1);
			if(parent_step == 0){
				return { _current_frame_pos, _current_frame_ptr};
			}
			else if(parent_step == -1){
				return frame_pos_t{k_frame_overhead, _global_frame};
			}
			else{
				QUARK_ASSERT(false);
				throw std::exception();
			}
		}

/*
		//	Returns stack position of the reg. Can be any stack frame.
		public: int resolve_register(const variable_address_t& reg) const{
			QUARK_ASSERT(check_invariant());
			QUARK_ASSERT(reg.check_invariant());

			const auto frame_pos = find_frame_pos(reg._parent_steps);

			QUARK_ASSERT(reg._index >= 0 && reg._index < frame_pos._frame_ptr->_body._symbols.size());
			const auto pos = frame_pos._frame_pos + reg._index;
			return pos;
		}

		public: const std::pair<std::string, symbol_t>* get_register_info(const variable_address_t& reg) const{
			QUARK_ASSERT(check_invariant());
			QUARK_ASSERT(reg.check_invariant());

			const auto frame_pos = find_frame_pos(reg._parent_steps);
			QUARK_ASSERT(reg._index >= 0 && reg._index < frame_pos._frame_ptr->_body._symbols.size());
			const auto symbol_ptr = &frame_pos._frame_ptr->_body._symbols[reg._index];
			return symbol_ptr;
		}

		public: const std::pair<std::string, symbol_t>* get_global_info(int global) const{
			QUARK_ASSERT(check_invariant());
			QUARK_ASSERT(global >= 0 && global < _global_frame->_body._symbols.size());

			return &_global_frame->_body._symbols[global];
		}
*/

		public: bool check_reg(int reg) const{
			QUARK_ASSERT(reg >= 0 && reg < _current_frame_ptr->_symbols.size());
			return true;
		}

		public: const std::pair<std::string, symbol_t>* get_register_info2(int reg) const{
			QUARK_ASSERT(check_invariant());
			QUARK_ASSERT(check_reg(reg));
			QUARK_ASSERT(reg >= 0 && reg < _current_frame_ptr->_symbols.size());

			return &_current_frame_ptr->_symbols[reg];
		}



		public: inline const bc_pod_value_t& peek_register(const int reg) const{
			QUARK_ASSERT(check_invariant());
			QUARK_ASSERT(check_reg(reg));

			return _current_frame_entry_ptr[reg];
		}
		public: inline const void write_register_pod(const int reg, const bc_pod_value_t& pod) const{
			QUARK_ASSERT(check_invariant());
			QUARK_ASSERT(check_reg(reg));

			_current_frame_entry_ptr[reg] = pod;
		}

		public: bc_value_t read_register(const int reg) const{
			QUARK_ASSERT(check_invariant());
			QUARK_ASSERT(check_reg(reg));

			QUARK_ASSERT(_debug_types[_current_frame_pos + reg] == _current_frame_ptr->_symbols[reg].second._value_type);

			bool is_ext = _current_frame_ptr->_exts[reg];
#if DEBUG
			const auto result = bc_value_t(_current_frame_ptr->_symbols[reg].second._value_type, _current_frame_entry_ptr[reg], is_ext);
#else
			const auto result = bc_value_t(_current_frame_entry_ptr[reg], is_ext);
#endif
			return result;
		}
		public: void write_register(const int reg, const bc_value_t& value){
			QUARK_ASSERT(check_invariant());
			QUARK_ASSERT(check_reg(reg));
			QUARK_ASSERT(value.check_invariant());

			QUARK_ASSERT(_debug_types[_current_frame_pos + reg] == _current_frame_ptr->_symbols[reg].second._value_type);
			bool is_ext = _current_frame_ptr->_exts[reg];
			if(is_ext){
				auto prev_copy = _current_frame_entry_ptr[reg];
				value._pod._ext->_rc++;
				_current_frame_entry_ptr[reg] = value._pod;
				bc_value_t::release_ext_pod(prev_copy);
			}
			else{
				_current_frame_entry_ptr[reg] = value._pod;
			}

			QUARK_ASSERT(check_invariant());
		}


		public: bc_value_t read_register_intern(const int reg) const{
			QUARK_ASSERT(check_invariant());
			QUARK_ASSERT(check_reg(reg));

			const auto info = get_register_info2(reg);
			QUARK_ASSERT(bc_value_t::is_bc_ext(info->second._value_type.get_base_type()) == false);

#if DEBUG
			return bc_value_t(_debug_types[_current_frame_pos + reg], _current_frame_entry_ptr[reg], false);
#else
			return bc_value_t(_current_frame_entry_ptr[reg], false);
#endif
		}
		public: void write_register_intern(const int reg, const bc_value_t& value){
			QUARK_ASSERT(check_invariant());
			QUARK_ASSERT(check_reg(reg));
			QUARK_ASSERT(value.check_invariant());
		#if DEBUG
			const auto info = get_register_info2(reg);
			QUARK_ASSERT(info->second._value_type == value._debug_type);
		#endif

			_current_frame_entry_ptr[reg] = value._pod;
		}


		//??? use const bc_value_object_t* peek_register_obj()
		public: bc_value_t read_register_obj(const int reg) const{
			QUARK_ASSERT(check_invariant());
			QUARK_ASSERT(check_reg(reg));
		#if DEBUG
			const auto info = get_register_info2(reg);
			QUARK_ASSERT(bc_value_t::is_bc_ext(info->second._value_type.get_base_type()) == true);
		#endif

#if DEBUG
			return bc_value_t(_debug_types[_current_frame_pos + reg], _current_frame_entry_ptr[reg], true);
#else
			return bc_value_t(_current_frame_entry_ptr[reg], true);
#endif
		}
		public: void write_register_obj(const int reg, const bc_value_t& value){
			QUARK_ASSERT(check_invariant());
			QUARK_ASSERT(check_reg(reg));
			QUARK_ASSERT(value.check_invariant());
		#if DEBUG
			const auto info = get_register_info2(reg);
			QUARK_ASSERT(info->second._value_type == value._debug_type);
		#endif
			QUARK_ASSERT(_debug_types[_current_frame_pos + reg] == value._debug_type);

			auto prev_copy = _current_frame_entry_ptr[reg];
			value._pod._ext->_rc++;
			_current_frame_entry_ptr[reg] = value._pod;
			bc_value_t::release_ext_pod(prev_copy);

			QUARK_ASSERT(check_invariant());
		}

/*
		public: bool read_register_bool(const int reg) const{
			QUARK_ASSERT(check_invariant());
			QUARK_ASSERT(check_reg(reg));
		#if DEBUG
			const auto info = get_register_info2(reg);
			QUARK_ASSERT(info->second._value_type == typeid_t::make_bool());
		#endif

			return _current_frame_entry_ptr[reg]._bool;
		}
		public: void write_register_bool(const int reg, bool value){
			QUARK_ASSERT(check_invariant());
			QUARK_ASSERT(check_reg(reg));
		#if DEBUG
			const auto info = get_register_info2(reg);
			QUARK_ASSERT(info->second._value_type == typeid_t::make_bool());
		#endif

			_current_frame_entry_ptr[reg]._bool = value;
		}
*/

		#if DEBUG
		public: bool check_register_access_any(const int reg) const{
			QUARK_ASSERT(check_invariant());
			QUARK_ASSERT(check_reg(reg));
			return true;
		}
		#endif

		#if DEBUG
		public: bool check_register_access_bool(const int reg) const{
			QUARK_ASSERT(check_invariant());
			QUARK_ASSERT(check_reg(reg));
			QUARK_ASSERT(_debug_types[_current_frame_pos + reg].is_bool());
			return true;
		}
		#endif

		#if DEBUG
		public: bool check_register_access_int(const int reg) const{
			QUARK_ASSERT(check_invariant());
			QUARK_ASSERT(check_reg(reg));
			QUARK_ASSERT(_debug_types[_current_frame_pos + reg].is_int());
			return true;
		}
		#endif

		#if DEBUG
		public: bool check_register_access_string(const int reg) const{
			QUARK_ASSERT(check_invariant());
			QUARK_ASSERT(check_reg(reg));
			QUARK_ASSERT(_debug_types[_current_frame_pos + reg].is_string());
			return true;
		}
		#endif

		#if DEBUG
		public: bool check_register_access_json_value(const int reg) const{
			QUARK_ASSERT(check_invariant());
			QUARK_ASSERT(check_reg(reg));
			QUARK_ASSERT(_debug_types[_current_frame_pos + reg].is_json_value());
			return true;
		}
		#endif

		#if DEBUG
		public: bool check_register_access_vector(const int reg) const{
			QUARK_ASSERT(check_invariant());
			QUARK_ASSERT(check_reg(reg));
			QUARK_ASSERT(_debug_types[_current_frame_pos + reg].is_vector());
			return true;
		}
		#endif

		#if DEBUG
		public: bool check_register_access_dict(const int reg) const{
			QUARK_ASSERT(check_invariant());
			QUARK_ASSERT(check_reg(reg));
			QUARK_ASSERT(_debug_types[_current_frame_pos + reg].is_dict());
			return true;
		}
		#endif

		#if DEBUG
		public: bool check_register_access_struct(const int reg) const{
			QUARK_ASSERT(check_invariant());
			QUARK_ASSERT(check_reg(reg));
			QUARK_ASSERT(_debug_types[_current_frame_pos + reg].is_struct());
			return true;
		}
		#endif


		#if DEBUG
		public: bool check_register_access_obj(const int reg) const{
			QUARK_ASSERT(check_invariant());
			QUARK_ASSERT(check_reg(reg));
			QUARK_ASSERT(_current_frame_ptr->_exts[reg] == true);
			return true;
		}
		#endif

		#if DEBUG
		public: bool check_register_access_intern(const int reg) const{
			QUARK_ASSERT(check_invariant());
			QUARK_ASSERT(check_reg(reg));
			QUARK_ASSERT(_current_frame_ptr->_exts[reg] == false);
			return true;
		}
		#endif

/*
		public: int read_register_int_xxx(const int reg) const{
			QUARK_ASSERT(check_register_int_access(reg));

			return _current_frame_entry_ptr[reg]._int;
		}
		public: void write_register_int_xxx(const int reg, int value){
			QUARK_ASSERT(check_invariant());
			QUARK_ASSERT(check_reg(reg));
		#if DEBUG
			const auto info = get_register_info2(reg);
			QUARK_ASSERT(info->second._value_type == typeid_t::make_int());
		#endif

			_current_frame_entry_ptr[reg]._int = value;
		}
*/

		public: void write_register_float(const int reg, float value){
			QUARK_ASSERT(check_invariant());
			QUARK_ASSERT(check_reg(reg));
		#if DEBUG
			const auto info = get_register_info2(reg);
			QUARK_ASSERT(info->second._value_type == typeid_t::make_float());
		#endif

			_current_frame_entry_ptr[reg]._float = value;
		}

		public: const std::string& peek_register_string(const int reg) const{
			QUARK_ASSERT(check_invariant());
			QUARK_ASSERT(check_reg(reg));
		#if DEBUG
			const auto info = get_register_info2(reg);
			QUARK_ASSERT(info->second._value_type == typeid_t::make_string());
		#endif

			return _current_frame_entry_ptr[reg]._ext->_string;
		}

		public: void write_register_string(const int reg, const std::string& value){
			QUARK_ASSERT(check_invariant());
			QUARK_ASSERT(check_reg(reg));
			QUARK_ASSERT(_debug_types[_current_frame_pos + reg].is_string());

			const auto value2 = bc_value_t::make_string(value);

			auto prev_copy = _current_frame_entry_ptr[reg];
			value2._pod._ext->_rc++;
			_current_frame_entry_ptr[reg] = value2._pod;
			bc_value_t::release_ext_pod(prev_copy);

			QUARK_ASSERT(check_invariant());
		}

		public: inline int read_register_function(const int reg) const{
			QUARK_ASSERT(check_invariant());
			QUARK_ASSERT(check_reg(reg));
		#if DEBUG
			const auto info = get_register_info2(reg);
			QUARK_ASSERT(info->second._value_type.get_base_type() == base_type::k_function);
		#endif

			return _current_frame_entry_ptr[reg]._function_id;
		}

		public: const std::vector<bc_value_t>* peek_register_vector(const int reg) const{
			QUARK_ASSERT(check_invariant());
			QUARK_ASSERT(check_reg(reg));
		#if DEBUG
			const auto info = get_register_info2(reg);
			QUARK_ASSERT(info->second._value_type.get_base_type() == base_type::k_vector);
		#endif

			return &_current_frame_entry_ptr[reg]._ext->_vector_elements;
		}


		friend std::shared_ptr<value_entry_t> find_global_symbol2(const interpreter_t& vm, const std::string& s);





		//////////////////////////////////////		STACK




		public: void save_frame(){
			const auto frame_pos = bc_value_t::make_int(_current_frame_pos);
			push_intern(frame_pos);

			const auto frame_ptr = bc_value_t(_current_frame_ptr);
			push_intern(frame_ptr);
		}
		public: void restore_frame(){
			const auto stack_size = size();
			bc_pod_value_t frame_ptr_pod = load_pod(stack_size - 1);
			bc_pod_value_t frame_pos_pod = load_pod(stack_size - 2);

			_current_frame_ptr = frame_ptr_pod._frame_ptr;
			_current_frame_pos = frame_pos_pod._int;
			_current_frame_entry_ptr = &_entries[_current_frame_pos];

			pop(false);
			pop(false);
		}



		//////////////////////////////////////		STACK


		public: inline void push_value(const bc_value_t& value, bool is_ext){
			QUARK_ASSERT(check_invariant());
			QUARK_ASSERT(value.check_invariant());
#if DEBUG
			QUARK_ASSERT(bc_value_t::is_bc_ext(value._debug_type.get_base_type()) == is_ext);
#endif

			if(is_ext){
				value._pod._ext->_rc++;
			}
			_entries[_stack_size] = value._pod;
			_stack_size++;
#if DEBUG
			_debug_types.push_back(value._debug_type);
#endif

			QUARK_ASSERT(check_invariant());
		}
		private: inline void push_intq(int value){
			QUARK_ASSERT(check_invariant());

			//??? Can write directly into entry, no need to construct e.
			bc_pod_value_t e;
			e._int = value;
			_entries[_stack_size] = e;
			_stack_size++;
#if DEBUG
			_debug_types.push_back(typeid_t::make_int());
#endif

			QUARK_ASSERT(check_invariant());
		}
		private: inline void push_obj(const bc_value_t& value){
			QUARK_ASSERT(check_invariant());
			QUARK_ASSERT(value.check_invariant());
#if DEBUG
			QUARK_ASSERT(bc_value_t::is_bc_ext(value._debug_type.get_base_type()) == true);
#endif

			value._pod._ext->_rc++;
			_entries[_stack_size] = value._pod;
			_stack_size++;
#if DEBUG
			_debug_types.push_back(value._debug_type);
#endif

			QUARK_ASSERT(check_invariant());
		}
		public: inline void push_intern(const bc_value_t& value){
			QUARK_ASSERT(check_invariant());
			QUARK_ASSERT(value.check_invariant());
#if DEBUG
			QUARK_ASSERT(bc_value_t::is_bc_ext(value._debug_type.get_base_type()) == false);
#endif

			_entries[_stack_size] = value._pod;
			_stack_size++;
#if DEBUG
			_debug_types.push_back(value._debug_type);
#endif

			QUARK_ASSERT(check_invariant());
		}


		//	returned value will have ownership of obj, if it's an obj.
		public: inline bc_value_t load_value_slow(int pos, const typeid_t& type) const{
			QUARK_ASSERT(check_invariant());
			QUARK_ASSERT(pos >= 0 && pos < _stack_size);
			QUARK_ASSERT(type.check_invariant());
			QUARK_ASSERT(type == _debug_types[pos]);

			const auto& e = _entries[pos];
			bool is_ext = bc_value_t::is_bc_ext(type.get_base_type());

#if DEBUG
			const auto result = bc_value_t(type, e, is_ext);
#else
			const auto result = bc_value_t(e, is_ext);
#endif
			return result;
		}

		public: inline const bc_pod_value_t& load_pod(int pos) const{
			QUARK_ASSERT(check_invariant());
			QUARK_ASSERT(pos >= 0 && pos < _stack_size);
			return _entries[pos];
		}
		public: inline bc_value_t load_intern_value(int pos) const{
			QUARK_ASSERT(check_invariant());
			QUARK_ASSERT(pos >= 0 && pos < _stack_size);
			QUARK_ASSERT(bc_value_t::is_bc_ext(_debug_types[pos].get_base_type()) == false);

#if DEBUG
			const auto result = bc_value_t(_debug_types[pos], _entries[pos], false);
#else
			const auto result = bc_value_t(_entries[pos], false);
#endif
			return result;
		}

		public: inline bc_value_t load_obj(int pos) const{
			QUARK_ASSERT(check_invariant());
			QUARK_ASSERT(pos >= 0 && pos < _stack_size);
			QUARK_ASSERT(bc_value_t::is_bc_ext(_debug_types[pos].get_base_type()) == true);

#if DEBUG
			const auto result = bc_value_t(_debug_types[pos], _entries[pos], true);
#else
			const auto result = bc_value_t(_entries[pos], true);
#endif
			return result;
		}

		public: inline int load_intq(int pos) const{
			QUARK_ASSERT(check_invariant());
			QUARK_ASSERT(pos >= 0 && pos < _stack_size);
			QUARK_ASSERT(bc_value_t::is_bc_ext(_debug_types[pos].get_base_type()) == false);

			return _entries[pos]._int;
		}
		private: inline const bc_frame_t* load_frame_ptr(int pos) const{
			QUARK_ASSERT(check_invariant());
			QUARK_ASSERT(pos >= 0 && pos < _stack_size);
			QUARK_ASSERT(bc_value_t::is_bc_ext(_debug_types[pos].get_base_type()) == false);

			return _entries[pos]._frame_ptr;
		}
		private: inline void push_frame_ptr(const bc_frame_t* frame_ptr){
			QUARK_ASSERT(check_invariant());
			QUARK_ASSERT(frame_ptr != nullptr && frame_ptr->check_invariant());

			const auto value = bc_value_t(frame_ptr);
			_entries[_stack_size] = value._pod;
			_stack_size++;
#if DEBUG
			_debug_types.push_back(value._debug_type);
#endif

		}

		//??? We could have support simple sumtype called DYN that holds a value_t at runtime.


		public: inline void replace_intern(int pos, const bc_value_t& value){
			QUARK_ASSERT(check_invariant());
			QUARK_ASSERT(value.check_invariant());
			QUARK_ASSERT(pos >= 0 && pos < _stack_size);
#if FLOYD_BC_VALUE_DEBUG_TYPE
			QUARK_ASSERT(bc_value_t::is_bc_ext(value._debug_type.get_base_type()) == false);
#endif
			QUARK_ASSERT(_debug_types[pos] == value._debug_type);

			_entries[pos] = value._pod;

			QUARK_ASSERT(check_invariant());
		}
		public: inline void replace_pod(int pos, const bc_pod_value_t& pod){
			QUARK_ASSERT(check_invariant());
			QUARK_ASSERT(pos >= 0 && pos < _stack_size);

			_entries[pos] = pod;

			QUARK_ASSERT(check_invariant());
		}

		public: inline void replace_obj(int pos, const bc_value_t& value){
			QUARK_ASSERT(check_invariant());
			QUARK_ASSERT(value.check_invariant());
			QUARK_ASSERT(pos >= 0 && pos < _stack_size);
#if FLOYD_BC_VALUE_DEBUG_TYPE
			QUARK_ASSERT(bc_value_t::is_bc_ext(value._debug_type.get_base_type()) == true);
#endif
			QUARK_ASSERT(_debug_types[pos] == value._debug_type);

			auto prev_copy = _entries[pos];
			value._pod._ext->_rc++;
			_entries[pos] = value._pod;
			bc_value_t::release_ext_pod(prev_copy);

			QUARK_ASSERT(check_invariant());
		}

		private: inline void replace_int(int pos, const int& value){
			QUARK_ASSERT(check_invariant());
			QUARK_ASSERT(pos >= 0 && pos < _stack_size);
			QUARK_ASSERT(_debug_types[pos] == typeid_t::make_int());

			_entries[pos]._int = value;

			QUARK_ASSERT(check_invariant());
		}

		//	extbits[0] tells if the first popped value has ext. etc.
		//	bit 0 maps to the next value to be popped from stack
		//	Max 32 can be popped.
		public: inline void pop_batch(int count, uint32_t extbits){
			QUARK_ASSERT(check_invariant());
			QUARK_ASSERT(_stack_size >= count);
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
		public: inline void pop_batch(const std::vector<bool>& exts){
			QUARK_ASSERT(check_invariant());
			QUARK_ASSERT(_stack_size >= exts.size());

			auto flag_index = exts.size() - 1;
			for(int i = 0 ; i < exts.size() ; i++){
				pop(exts[flag_index]);
				flag_index--;
			}
			QUARK_ASSERT(check_invariant());
		}
		public: inline void pop(bool ext){
			QUARK_ASSERT(check_invariant());
			QUARK_ASSERT(_stack_size > 0);
			QUARK_ASSERT(bc_value_t::is_bc_ext(_debug_types.back().get_base_type()) == ext);

			auto copy = _entries[_stack_size - 1];
			_stack_size--;
#if DEBUG
			_debug_types.pop_back();
#endif
			if(ext){
				bc_value_t::release_ext_pod(copy);
			}

			QUARK_ASSERT(check_invariant());
		}

#if DEBUG
		public: bool is_ext(int pos) const{
			QUARK_ASSERT(check_invariant());
			QUARK_ASSERT(pos >= 0 && pos < _stack_size);
			return bc_value_t::is_bc_ext(_debug_types[pos].get_base_type());
		}
#endif

#if DEBUG
		public: typeid_t debug_get_type(int pos) const{
			QUARK_ASSERT(check_invariant());
			QUARK_ASSERT(pos >= 0 && pos < _stack_size);
			return _debug_types[pos];
		}
#endif

		public: frame_pos_t get_current_frame_pos() const {
			QUARK_ASSERT(check_invariant());

			return frame_pos_t{_current_frame_pos, _current_frame_ptr};
		}

		public: json_t stack_to_json() const;


		public: bc_pod_value_t* _entries;
		public: size_t _allocated_count;
		public: size_t _stack_size;

		//	These are DEEP copies = do not share RC with non-debug values.
#if DEBUG
		public: std::vector<typeid_t> _debug_types;
#endif

		public: int _current_frame_pos;
		public: const bc_frame_t* _current_frame_ptr;
		public: bc_pod_value_t* _current_frame_entry_ptr;

		public: const bc_frame_t* _global_frame;
	};





	//////////////////////////////////////		interpreter_context_t



	struct interpreter_context_t {
		public: quark::trace_context_t _tracer;
	};


	//////////////////////////////////////		execution_result_t


	struct execution_result_t {
		enum output_type {

			//	_output != bc_value_t::make_undefined()
			k_returning,

			//	_output != bc_value_t::make_undefined()
			k_passive_expression_output,


			//	_output == bc_value_t::make_undefined()
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
		public: interpreter_stack_t _stack;
		public: std::vector<std::string> _print_output;
	};


	value_t call_host_function(interpreter_t& vm, int function_id, const std::vector<value_t>& args);
	value_t call_function(interpreter_t& vm, const value_t& f, const std::vector<value_t>& args);
	json_t interpreter_to_json(const interpreter_t& vm);


	std::pair<bool, bc_value_t> execute_instructions(interpreter_t& vm, const std::vector<bc_instruction2_t>& instructions);



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
