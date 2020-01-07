//
//  bytecode_stack.hpp
//  floyd
//
//  Created by Marcus Zetterquist on 2020-01-05.
//  Copyright © 2020 Marcus Zetterquist. All rights reserved.
//

#ifndef bytecode_stack_hpp
#define bytecode_stack_hpp

#include "types.h"
#include "value_backend.h"
#include "quark.h"

#include <vector>

namespace floyd {

void release_value_safe(value_backend_t& backend, rt_pod_t value, type_t type);

struct interpreter_stack_t {
	public: interpreter_stack_t(value_backend_t* backend) :
		_backend(backend),
		_entries(nullptr),
		_allocated_count(0),
		_stack_size(0)
	{
		QUARK_ASSERT(backend != nullptr && backend->check_invariant());

		_entries = new rt_pod_t[8192];
		_allocated_count = 8192;

		QUARK_ASSERT(check_invariant());
	}

	public: ~interpreter_stack_t(){
		QUARK_ASSERT(check_invariant());

		delete[] _entries;
		_entries = nullptr;
	}

	public: bool check_invariant() const {
		QUARK_ASSERT(_backend != nullptr && _backend->check_invariant_light());
		QUARK_ASSERT(_entries != nullptr);
		QUARK_ASSERT(_stack_size >= 0 && _stack_size <= _allocated_count);

		QUARK_ASSERT(_entry_types.size() == _stack_size);
		for(size_t i = 0 ; i < _stack_size ; i++){
			QUARK_ASSERT(_entry_types[i].check_invariant());
		}

		return true;
	}

	public: interpreter_stack_t(const interpreter_stack_t& other) = delete;
	public: interpreter_stack_t& operator=(const interpreter_stack_t& other) = delete;

	public: void swap(interpreter_stack_t& other) throw() {
		QUARK_ASSERT(check_invariant());
		QUARK_ASSERT(other.check_invariant());

		std::swap(other._backend, _backend);
		std::swap(other._entries, _entries);
		std::swap(other._allocated_count, _allocated_count);
		std::swap(other._stack_size, _stack_size);
		other._entry_types.swap(_entry_types);

		QUARK_ASSERT(check_invariant());
		QUARK_ASSERT(other.check_invariant());
	}

	public: size_t size() const {
		QUARK_ASSERT(check_invariant());

		return _stack_size;
	}


	public: void push_external_value(const rt_value_t& value){
		QUARK_ASSERT(check_invariant());
		QUARK_ASSERT(value.check_invariant());

		retain_value(*_backend, value._pod, value._type);
		_entries[_stack_size] = value._pod;
		_stack_size++;
		_entry_types.push_back(value._type);

		QUARK_ASSERT(check_invariant());
	}

	public: void push_external_value_blank(const type_t& type){
		QUARK_ASSERT(check_invariant());
		QUARK_ASSERT(type.check_invariant());

		_entries[_stack_size] = make_uninitialized_magic();
		_stack_size++;
		_entry_types.push_back(type);

		QUARK_ASSERT(check_invariant());
	}

	public: void push_inplace_value(const rt_value_t& value){
		QUARK_ASSERT(check_invariant());
		QUARK_ASSERT(value.check_invariant());

		_entries[_stack_size] = value._pod;
		_stack_size++;
		_entry_types.push_back(value._type);

		QUARK_ASSERT(check_invariant());
	}

	public: rt_value_t load_value(size_t pos, const type_t& type) const {
		QUARK_ASSERT(check_invariant());
		QUARK_ASSERT(pos >= 0 && pos < _stack_size);
		QUARK_ASSERT(type.check_invariant());
		QUARK_ASSERT(peek2(_backend->types, type) == peek2(_backend->types, _entry_types[pos]));

		const auto& e = _entries[pos];
		const auto result = rt_value_t(*_backend, e, type, rt_value_t::rc_mode::bump);
		return result;
	}

	public: int64_t load_intq(size_t pos) const{
		QUARK_ASSERT(check_invariant());
		QUARK_ASSERT(pos >= 0 && pos < _stack_size);
		QUARK_ASSERT(peek2(_backend->types, _entry_types[pos]).is_int());

		return _entries[pos].int_value;
	}

	public: void replace_external_value(size_t pos, const rt_value_t& value){
		QUARK_ASSERT(check_invariant());
		QUARK_ASSERT(value.check_invariant());
		QUARK_ASSERT(pos >= 0 && pos < _stack_size);
		QUARK_ASSERT(_entry_types[pos] == value._type);

		auto prev_copy = _entries[pos];

		retain_value(*_backend, value._pod, value._type);
		_entries[pos] = value._pod;
		release_value_safe(*_backend, prev_copy, value._type);

		QUARK_ASSERT(check_invariant());
	}

	//	exts[exts.size() - 1] maps to the closed value on stack, the next to be popped.
	public: void pop_batch(const std::vector<type_t>& exts){
		QUARK_ASSERT(check_invariant());
		QUARK_ASSERT(_stack_size >= exts.size());

		auto flag_index = exts.size() - 1;
		for(size_t i = 0 ; i < exts.size() ; i++){
			pop(exts[flag_index]);
			flag_index--;
		}
		QUARK_ASSERT(check_invariant());
	}

	public: void pop(const type_t& type){
		QUARK_ASSERT(check_invariant());
		QUARK_ASSERT(_stack_size > 0);

		auto copy = _entries[_stack_size - 1];
		_stack_size--;
		_entry_types.pop_back();
		release_value_safe(*_backend, copy, type);

		QUARK_ASSERT(check_invariant());
	}


	////////////////////////		STATE

	public: value_backend_t* _backend;
	public: rt_pod_t* _entries;
	public: size_t _allocated_count;
	public: size_t _stack_size;

	//	These are parallell with _entries, one element for each entry on the stack.
	//??? Kill these - we should have the types in the static frames already.
	public: std::vector<type_t> _entry_types;
};

json_t stack_to_json(const interpreter_stack_t& stack, value_backend_t& backend);



//////////////////////////////////////		INLINES


//??? Remove need for this function! BC should overwrite registers by default = no need to release_value() on previous value.
inline void release_value_safe(value_backend_t& backend, rt_pod_t value, type_t type){
	QUARK_ASSERT(backend.check_invariant());
	QUARK_ASSERT(value.check_invariant());
	QUARK_ASSERT(type.check_invariant());

	const auto& peek = peek2(backend.types, type);

	if(is_rc_value(backend.types, peek) && value.int_value == UNINITIALIZED_RUNTIME_VALUE){
	}
	else{
		return release_value(backend, value, type);
	}
}

} //	floyd

#endif /* bytecode_stack_hpp */
