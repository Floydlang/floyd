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
			_frame(nullptr)
		{
		}
		frame_pos_t(int frame_pos, const bc_frame_t* frame) :
			_frame_pos(frame_pos),
			_frame(frame)
		{
		}
*/

		int _frame_pos;
		const bc_frame_t* _frame;
	};

	inline bool operator==(const frame_pos_t& lhs, const frame_pos_t& rhs){
		return lhs._frame_pos == rhs._frame_pos && lhs._frame == rhs._frame;
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
			_current_frame(nullptr),
			_global_frame(global_frame)
		{
			_value_stack.reserve(1024);

			QUARK_ASSERT(check_invariant());
		}

		public: ~interpreter_stack_t(){
			QUARK_ASSERT(check_invariant());
		}


		public: bool check_invariant() const {
			QUARK_ASSERT(_current_frame_pos >= 0);
			QUARK_ASSERT(_current_frame_pos <= _value_stack.size());
			QUARK_ASSERT(_debug_types.size() == _value_stack.size());
			for(int i = 0 ; i < _value_stack.size() ; i++){
				QUARK_ASSERT(_debug_types[i].check_invariant());
			}
//			QUARK_ASSERT(_global_frame != nullptr);
			return true;
		}

		public: interpreter_stack_t(const interpreter_stack_t& other) = delete;

/*		private: save_for_later___interpreter_stack_t(const interpreter_stack_t& other) :
			_value_stack(other._value_stack),
			_debug_types(other._debug_types),
			_current_stack_frame(other._current_stack_frame),
			_global_frame(other._global_frame)
		{
			QUARK_ASSERT(other.check_invariant());

			//??? Cannot bump RC:s because we don't know which values!
			QUARK_ASSERT(false);
		}
*/

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

			other._value_stack.swap(_value_stack);
#if DEBUG
			other._debug_types.swap(_debug_types);
#endif
			std::swap(_current_frame_pos, other._current_frame_pos);
			std::swap(_current_frame, other._current_frame);
			std::swap(_global_frame, other._global_frame);

			QUARK_ASSERT(check_invariant());
			QUARK_ASSERT(other.check_invariant());
		}

		public: inline int size() const {
			QUARK_ASSERT(check_invariant());

			return static_cast<int>(_value_stack.size());
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
					push_inplace(local);
				}
			}
			_current_frame_pos = new_frame_pos;
			_current_frame = &frame;
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

		public: BC_INLINE frame_pos_t find_frame_pos(int parent_step) const{
			QUARK_ASSERT(check_invariant());

			QUARK_ASSERT(parent_step == 0 || parent_step == -1);
			if(parent_step == 0){
				return { _current_frame_pos, _current_frame};
			}
			else if(parent_step == -1){
				return frame_pos_t{k_frame_overhead, _global_frame};
			}
			else{
				QUARK_ASSERT(false);
			}
		}

		//	Returns stack position of the reg. Can be any stack frame.
		public: int resolve_register(const variable_address_t& reg) const{
			QUARK_ASSERT(check_invariant());
			QUARK_ASSERT(reg.check_invariant());

			const auto frame_pos = find_frame_pos(reg._parent_steps);

			QUARK_ASSERT(reg._index >= 0 && reg._index < frame_pos._frame->_body._symbols.size());
			const auto pos = frame_pos._frame_pos + reg._index;
			return pos;
		}

		public: const std::pair<std::string, symbol_t>* get_register_info(const variable_address_t& reg) const{
			QUARK_ASSERT(check_invariant());
			QUARK_ASSERT(reg.check_invariant());

			const auto frame_pos = find_frame_pos(reg._parent_steps);
			QUARK_ASSERT(reg._index >= 0 && reg._index < frame_pos._frame->_body._symbols.size());
			const auto symbol_ptr = &frame_pos._frame->_body._symbols[reg._index];
			return symbol_ptr;
		}

		public: const std::pair<std::string, symbol_t>* get_global_info(int global) const{
			QUARK_ASSERT(check_invariant());
			QUARK_ASSERT(global >= 0 && global < _global_frame->_body._symbols.size());

			return &_global_frame->_body._symbols[global];
		}

		public: bool check_reg(int reg) const{
			QUARK_ASSERT(reg >= 0 && reg < (size() - _current_frame_pos));
			return true;
		}

		public: const std::pair<std::string, symbol_t>* get_register_info2(int reg) const{
			QUARK_ASSERT(check_invariant());
			QUARK_ASSERT(check_reg(reg));
			QUARK_ASSERT(reg >= 0 && reg < _current_frame->_body._symbols.size());

			return &_current_frame->_body._symbols[reg];
		}


		public: bc_value_t read_register(const int reg) const{
			QUARK_ASSERT(check_invariant());
			QUARK_ASSERT(check_reg(reg));

			const auto pos = _current_frame_pos + reg;
			QUARK_ASSERT(_debug_types[pos] == _current_frame->_body._symbols[reg].second._value_type);

			bool is_ext = _current_frame->_exts[reg];
#if DEBUG
			const auto result = bc_value_t(_current_frame->_body._symbols[reg].second._value_type, _value_stack[pos], is_ext);
#else
			const auto result = bc_value_t(_value_stack[pos], is_ext);
#endif
			return result;
		}
		public: void write_register(const int reg, const bc_value_t& value){
			QUARK_ASSERT(check_invariant());
			QUARK_ASSERT(check_reg(reg));
			QUARK_ASSERT(value.check_invariant());

			const auto pos = _current_frame_pos + reg;
			QUARK_ASSERT(_debug_types[pos] == _current_frame->_body._symbols[reg].second._value_type);
			bool is_ext = _current_frame->_exts[reg];
			if(is_ext){
				auto prev_copy = _value_stack[pos];
				value._pod._ext->_rc++;
				_value_stack[pos] = value._pod;
				bc_value_t::debump(prev_copy);
			}
			else{
				_value_stack[pos] = value._pod;
			}

			QUARK_ASSERT(check_invariant());
		}



		public: bc_value_t read_register_inplace(const int reg) const{
			QUARK_ASSERT(check_invariant());
			QUARK_ASSERT(check_reg(reg));

			const auto info = get_register_info2(reg);
			QUARK_ASSERT(bc_value_t::is_bc_ext(info->second._value_type.get_base_type()) == false);

			const auto pos = _current_frame_pos + reg;
			return load_inline_value(pos);
		}
		public: void write_register_inplace(const int reg, const bc_value_t& value){
			QUARK_ASSERT(check_invariant());
			QUARK_ASSERT(check_reg(reg));
			QUARK_ASSERT(value.check_invariant());
		#if DEBUG
			const auto info = get_register_info2(reg);
			QUARK_ASSERT(info->second._value_type == value._debug_type);
		#endif

			const auto pos = _current_frame_pos + reg;
			replace_inline(pos, value);
		}

		public: bc_value_t read_register_obj(const int reg) const{
			QUARK_ASSERT(check_invariant());
			QUARK_ASSERT(check_reg(reg));
		#if DEBUG
			const auto info = get_register_info2(reg);
			QUARK_ASSERT(bc_value_t::is_bc_ext(info->second._value_type.get_base_type()) == true);
		#endif

			const auto pos = _current_frame_pos + reg;
			return load_obj(pos);
		}
		public: void write_register_obj(const int reg, const bc_value_t& value){
			QUARK_ASSERT(check_invariant());
			QUARK_ASSERT(check_reg(reg));
			QUARK_ASSERT(value.check_invariant());
		#if DEBUG
			const auto info = get_register_info2(reg);
			QUARK_ASSERT(info->second._value_type == value._debug_type);
		#endif

			const auto pos = _current_frame_pos + reg;
			replace_obj(pos, value);
		}

		public: bool read_register_bool(const int reg) const{
			QUARK_ASSERT(check_invariant());
			QUARK_ASSERT(check_reg(reg));
		#if DEBUG
			const auto info = get_register_info2(reg);
			QUARK_ASSERT(info->second._value_type == typeid_t::make_bool());
		#endif

			const auto pos = _current_frame_pos + reg;
			return load_inline_value(pos).get_bool_value();
		}
		public: void write_register_bool(const int reg, bool value){
			QUARK_ASSERT(check_invariant());
			QUARK_ASSERT(check_reg(reg));
		#if DEBUG
			const auto info = get_register_info2(reg);
			QUARK_ASSERT(info->second._value_type == typeid_t::make_bool());
		#endif

			const auto pos = _current_frame_pos + reg;
			const auto value2 = bc_value_t::make_bool(value);
			replace_inline(pos, value2);
		}


		public: int read_register_int(const int reg) const{
			QUARK_ASSERT(check_invariant());
			QUARK_ASSERT(check_reg(reg));
		#if DEBUG
			const auto info = get_register_info2(reg);
			QUARK_ASSERT(info->second._value_type == typeid_t::make_int());
		#endif

			const auto pos = _current_frame_pos + reg;
			return load_intq(pos);
		}
		public: void write_register_int(const int reg, int value){
			QUARK_ASSERT(check_invariant());
			QUARK_ASSERT(check_reg(reg));
		#if DEBUG
			const auto info = get_register_info2(reg);
			QUARK_ASSERT(info->second._value_type == typeid_t::make_int());
		#endif

			const auto pos = _current_frame_pos + reg;
			const auto value2 = bc_value_t::make_int(value);
			replace_inline(pos, value2);
		}

		public: void write_register_float(const int reg, float value){
			QUARK_ASSERT(check_invariant());
			QUARK_ASSERT(check_reg(reg));
		#if DEBUG
			const auto info = get_register_info2(reg);
			QUARK_ASSERT(info->second._value_type == typeid_t::make_float());
		#endif

			const auto pos = _current_frame_pos + reg;
			const auto value2 = bc_value_t::make_float(value);
			replace_inline(pos, value2);
		}

		public: const std::string& read_register_string(const int reg) const{
			QUARK_ASSERT(check_invariant());
			QUARK_ASSERT(check_reg(reg));
		#if DEBUG
			const auto info = get_register_info2(reg);
			QUARK_ASSERT(info->second._value_type == typeid_t::make_string());
		#endif

			const auto pos = _current_frame_pos + reg;
			return _value_stack[pos]._ext->_string;
		}

		public: void write_register_string(const int reg, const std::string& value){
			QUARK_ASSERT(check_invariant());
			QUARK_ASSERT(check_reg(reg));
		#if DEBUG
			const auto info = get_register_info2(reg);
			QUARK_ASSERT(info->second._value_type == typeid_t::make_string());
		#endif

			const auto pos = _current_frame_pos + reg;
			const auto value2 = bc_value_t::make_string(value);
			replace_obj(pos, value2);
		}

		public: inline int read_register_function(const int reg) const{
			QUARK_ASSERT(check_invariant());
			QUARK_ASSERT(check_reg(reg));
		#if DEBUG
			const auto info = get_register_info2(reg);
			QUARK_ASSERT(info->second._value_type.get_base_type() == base_type::k_function);
		#endif

			return _value_stack[_current_frame_pos + reg]._function_id;
		}

		public: inline const bc_pod_value_t& peek_register(const int reg) const{
			QUARK_ASSERT(check_invariant());
			QUARK_ASSERT(check_reg(reg));

			return _value_stack[_current_frame_pos + reg];
		}


		public: const std::vector<bc_value_t>* read_register_vector(const int reg) const{
			QUARK_ASSERT(check_invariant());
			QUARK_ASSERT(check_reg(reg));
		#if DEBUG
			const auto info = get_register_info2(reg);
			QUARK_ASSERT(info->second._value_type.get_base_type() == base_type::k_vector);
		#endif

			const auto pos = _current_frame_pos + reg;
			const auto value = load_obj(pos);
			return value.get_vector_value();
		}


		friend std::shared_ptr<value_entry_t> find_global_symbol2(const interpreter_t& vm, const std::string& s);



		//////////////////////////////////////		STACK


		public: void save_frame(){
			const auto frame_pos = bc_value_t::make_int(_current_frame_pos);
			push_inplace(frame_pos);

			const auto frame_ptr = bc_value_t(_current_frame);
			push_inplace(frame_ptr);
		}
		public: void restore_frame(){
			const auto stack_size = size();
			bc_pod_value_t frame_ptr_pod = load_pod(stack_size - 1);
			bc_pod_value_t frame_pos_pod = load_pod(stack_size - 2);

			_current_frame = frame_ptr_pod._frame_ptr;
			_current_frame_pos = frame_pos_pod._int;

			pop(false);
			pop(false);
		}



		//////////////////////////////////////		STACK


		public: BC_INLINE void push_value(const bc_value_t& value, bool is_ext){
			QUARK_ASSERT(check_invariant());
			QUARK_ASSERT(value.check_invariant());
#if DEBUG
			QUARK_ASSERT(bc_value_t::is_bc_ext(value._debug_type.get_base_type()) == is_ext);
#endif

			if(is_ext){
				value._pod._ext->_rc++;
			}
			_value_stack.push_back(value._pod);
#if DEBUG
			_debug_types.push_back(value._debug_type);
#endif

			QUARK_ASSERT(check_invariant());
		}
		private: BC_INLINE void push_intq(int value){
			QUARK_ASSERT(check_invariant());

			bc_pod_value_t e;
			e._int = value;
			_value_stack.push_back(e);
#if DEBUG
			_debug_types.push_back(typeid_t::make_int());
#endif

			QUARK_ASSERT(check_invariant());
		}
		private: BC_INLINE void push_obj(const bc_value_t& value){
			QUARK_ASSERT(check_invariant());
			QUARK_ASSERT(value.check_invariant());
#if DEBUG
			QUARK_ASSERT(bc_value_t::is_bc_ext(value._debug_type.get_base_type()) == true);
#endif

			value._pod._ext->_rc++;
			_value_stack.push_back(value._pod);
#if DEBUG
			_debug_types.push_back(value._debug_type);
#endif

			QUARK_ASSERT(check_invariant());
		}
		public: BC_INLINE void push_inplace(const bc_value_t& value){
			QUARK_ASSERT(check_invariant());
			QUARK_ASSERT(value.check_invariant());
#if DEBUG
			QUARK_ASSERT(bc_value_t::is_bc_ext(value._debug_type.get_base_type()) == false);
#endif

			_value_stack.push_back(value._pod);
#if DEBUG
			_debug_types.push_back(value._debug_type);
#endif

			QUARK_ASSERT(check_invariant());
		}


		//	returned value will have ownership of obj, if it's an obj.
		public: BC_INLINE bc_value_t load_value_slow(int pos, const typeid_t& type) const{
			QUARK_ASSERT(check_invariant());
			QUARK_ASSERT(pos >= 0 && pos < _value_stack.size());
			QUARK_ASSERT(type.check_invariant());
			QUARK_ASSERT(type == _debug_types[pos]);

			const auto& e = _value_stack[pos];
			bool is_ext = bc_value_t::is_bc_ext(type.get_base_type());

#if DEBUG
			const auto result = bc_value_t(type, e, is_ext);
#else
			const auto result = bc_value_t(e, is_ext);
#endif
			return result;
		}

		public: BC_INLINE const bc_pod_value_t& load_pod(int pos) const{
			QUARK_ASSERT(check_invariant());
			QUARK_ASSERT(pos >= 0 && pos < _value_stack.size());
			return _value_stack[pos];
		}
		public: BC_INLINE bc_value_t load_inline_value(int pos) const{
			QUARK_ASSERT(check_invariant());
			QUARK_ASSERT(pos >= 0 && pos < _value_stack.size());
			QUARK_ASSERT(bc_value_t::is_bc_ext(_debug_types[pos].get_base_type()) == false);

#if DEBUG
			const auto result = bc_value_t(_debug_types[pos], _value_stack[pos], false);
#else
			const auto result = bc_value_t(_value_stack[pos], false);
#endif
			return result;
		}

		public: BC_INLINE bc_value_t load_obj(int pos) const{
			QUARK_ASSERT(check_invariant());
			QUARK_ASSERT(pos >= 0 && pos < _value_stack.size());
			QUARK_ASSERT(bc_value_t::is_bc_ext(_debug_types[pos].get_base_type()) == true);

#if DEBUG
			const auto result = bc_value_t(_debug_types[pos], _value_stack[pos], true);
#else
			const auto result = bc_value_t(_value_stack[pos], true);
#endif
			return result;
		}

		public: BC_INLINE int load_intq(int pos) const{
			QUARK_ASSERT(check_invariant());
			QUARK_ASSERT(pos >= 0 && pos < _value_stack.size());
			QUARK_ASSERT(bc_value_t::is_bc_ext(_debug_types[pos].get_base_type()) == false);

			return _value_stack[pos]._int;
		}
		private: BC_INLINE const bc_frame_t* load_frame_ptr(int pos) const{
			QUARK_ASSERT(check_invariant());
			QUARK_ASSERT(pos >= 0 && pos < _value_stack.size());
			QUARK_ASSERT(bc_value_t::is_bc_ext(_debug_types[pos].get_base_type()) == false);

			return _value_stack[pos]._frame_ptr;
		}
		private: BC_INLINE void push_frame_ptr(const bc_frame_t* frame){
			QUARK_ASSERT(check_invariant());
			QUARK_ASSERT(frame != nullptr && frame->check_invariant());

			const auto value = bc_value_t(frame);
			_value_stack.push_back(value._pod);
#if DEBUG
			_debug_types.push_back(value._debug_type);
#endif

		}

		//??? We could have support simple sumtype called DYN that holds a value_t at runtime.


		public: BC_INLINE void replace_inline(int pos, const bc_value_t& value){
			QUARK_ASSERT(check_invariant());
			QUARK_ASSERT(value.check_invariant());
			QUARK_ASSERT(pos >= 0 && pos < _value_stack.size());
#if FLOYD_BC_VALUE_DEBUG_TYPE
			QUARK_ASSERT(bc_value_t::is_bc_ext(value._debug_type.get_base_type()) == false);
#endif
			QUARK_ASSERT(_debug_types[pos] == value._debug_type);

			_value_stack[pos] = value._pod;

			QUARK_ASSERT(check_invariant());
		}
		public: BC_INLINE void replace_pod(int pos, const bc_pod_value_t& pod){
			QUARK_ASSERT(check_invariant());
			QUARK_ASSERT(pos >= 0 && pos < _value_stack.size());

			_value_stack[pos] = pod;

			QUARK_ASSERT(check_invariant());
		}

		public: BC_INLINE void replace_obj(int pos, const bc_value_t& value){
			QUARK_ASSERT(check_invariant());
			QUARK_ASSERT(value.check_invariant());
			QUARK_ASSERT(pos >= 0 && pos < _value_stack.size());
#if FLOYD_BC_VALUE_DEBUG_TYPE
			QUARK_ASSERT(bc_value_t::is_bc_ext(value._debug_type.get_base_type()) == true);
#endif
			QUARK_ASSERT(_debug_types[pos] == value._debug_type);

			auto prev_copy = _value_stack[pos];
			value._pod._ext->_rc++;
			_value_stack[pos] = value._pod;
			bc_value_t::debump(prev_copy);

			QUARK_ASSERT(check_invariant());
		}

		private: BC_INLINE void replace_int(int pos, const int& value){
			QUARK_ASSERT(check_invariant());
			QUARK_ASSERT(pos >= 0 && pos < _value_stack.size());
			QUARK_ASSERT(_debug_types[pos] == typeid_t::make_int());

			_value_stack[pos]._int = value;

			QUARK_ASSERT(check_invariant());
		}

		//	extbits[0] tells if the first popped value has ext. etc.
		//	bit 0 maps to the next value to be popped from stack
		//	Max 32 can be popped.
		public: BC_INLINE void pop_batch(int count, uint32_t extbits){
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
		public: BC_INLINE void pop_batch(const std::vector<bool>& exts){
			QUARK_ASSERT(check_invariant());
			QUARK_ASSERT(_value_stack.size() >= exts.size());

			auto flag_index = exts.size() - 1;
			for(int i = 0 ; i < exts.size() ; i++){
				pop(exts[flag_index]);
				flag_index--;
			}
			QUARK_ASSERT(check_invariant());
		}
		public: BC_INLINE void pop(bool ext){
			QUARK_ASSERT(check_invariant());
			QUARK_ASSERT(_value_stack.empty() == false);
			QUARK_ASSERT(bc_value_t::is_bc_ext(_debug_types.back().get_base_type()) == ext);

			auto copy = _value_stack.back();
			_value_stack.pop_back();
#if DEBUG
			_debug_types.pop_back();
#endif
			if(ext){
				bc_value_t::debump(copy);
			}

			QUARK_ASSERT(check_invariant());
		}

#if DEBUG
		public: bool is_ext(int pos) const{
			QUARK_ASSERT(check_invariant());
			QUARK_ASSERT(pos >= 0 && pos < _value_stack.size());
			return bc_value_t::is_bc_ext(_debug_types[pos].get_base_type());
		}
#endif

#if DEBUG
		public: typeid_t debug_get_type(int pos) const{
			QUARK_ASSERT(check_invariant());
			QUARK_ASSERT(pos >= 0 && pos < _value_stack.size());
			return _debug_types[pos];
		}
#endif
/*
		BC_INLINE void pop_value_slow(const typeid_t& type){
			QUARK_ASSERT(_value_stack.empty() == false);

			auto prev_copy = _value_stack.back();
			_value_stack.pop_back();
			bc_value_t::debump(prev_copy);
		}
*/

		public: frame_pos_t get_current_frame_pos() const {
			QUARK_ASSERT(check_invariant());

			return frame_pos_t{_current_frame_pos, _current_frame};
		}

		public: json_t stack_to_json() const;


		public: std::vector<bc_pod_value_t> _value_stack;

		//	These are DEEP copies = do not share RC with non-debug values.
#if DEBUG
		public: std::vector<typeid_t> _debug_types;
#endif

		public: int _current_frame_pos;
		public: const bc_frame_t* _current_frame;
		private: const bc_frame_t* _global_frame;
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
		public: interpreter_stack_t _stack;
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
