//
//  parser_evaluator.cpp
//  Floyd
//
//  Created by Marcus Zetterquist on 26/07/16.
//  Copyright © 2016 Marcus Zetterquist. All rights reserved.
//

#include "bytecode_interpreter.h"


#include "floyd_runtime.h"
#include "types.h"
#include "format_table.h"
#include <ffi.h>


namespace floyd {

static const bool k_trace_stepping = true;

static bool should_trace(const std::string& name){
	return true && name == "floyd process 1 server";
}

//	Check memory layouts.
QUARK_TEST("", "", "", ""){
	const auto int_size = sizeof(int);
	QUARK_ASSERT(int_size == 4);

	const auto pod_size = sizeof(rt_pod_t);
	QUARK_ASSERT(pod_size == 8);

	const auto bcvalue_size = sizeof(rt_value_t);
	(void)bcvalue_size;
//	QUARK_ASSERT(bcvalue_size == 72);

	const auto instruction2_size = sizeof(bc_instruction_t);
	QUARK_ASSERT(instruction2_size == 8);


	const auto immer_vec_bool_size = sizeof(immer::vector<bool>);
	const auto immer_vec_int_size = sizeof(immer::vector<int>);
	const auto immer_vec_string_size = sizeof(immer::vector<std::string>);

	QUARK_ASSERT(immer_vec_bool_size == 32);
	QUARK_ASSERT(immer_vec_int_size == 32);
	QUARK_ASSERT(immer_vec_string_size == 32);


	const auto immer_stringkey_map_size = sizeof(immer::map<std::string, double>);
	QUARK_ASSERT(immer_stringkey_map_size == 16);

	const auto immer_intkey_map_size = sizeof(immer::map<uint32_t, double>);
	QUARK_ASSERT(immer_intkey_map_size == 16);
}



static type_t lookup_full_type(const interpreter_t& vm, const bc_typeid_t& type){
	const auto& backend = vm._backend;
	const auto& types = backend.types;
	return lookup_type_from_index(types, type);
}

static int get_global_n_pos(int n){
	return k_frame_overhead + n;
}

static int get_local_n_pos(int frame_pos, int n){
	return frame_pos + n;
}


//	??? Make stub bc_static_frame_t for each host function to make call conventions same as Floyd functions.
struct cif_t {
	ffi_cif cif;
	std::vector<ffi_type*> arg_types;

	ffi_type* return_type;

	//	Those type we need to malloc are owned by this vector so we can delete them.
	std::vector<ffi_type*> args_types_owning;
};

//	??? To add support for complex types, we need to wrap ffi_type in a struct that also owns sub-types.
static ffi_type* make_ffi_type(const type_t& type){
	if(type.is_void()){
		return &ffi_type_void;
	}
	else if(type.is_bool()){
		return &ffi_type_uint8;
	}
	else if(type.is_int()){
		return &ffi_type_sint64;
	}
	else if(type.is_double()){
		return &ffi_type_double;
	}
	else if(type.is_string()){
		return &ffi_type_pointer;
	}
	else if(type.is_json()){
		return &ffi_type_pointer;
	}
	else if(type.is_typeid()){
		return &ffi_type_sint64;
	}
	else if(type.is_struct()){
		return &ffi_type_pointer;
	}
	else if(type.is_vector()){
		return &ffi_type_pointer;
	}
	else if(type.is_dict()){
		return &ffi_type_pointer;
	}
	else if(type.is_function()){
		return &ffi_type_pointer;
	}
	else{
		quark::throw_defective_request();
	}
}

static cif_t make_cif(const value_backend_t& backend, const type_t& function_type){
	QUARK_ASSERT(backend.check_invariant());
	const auto& types = backend.types;

	const auto function_type_peek = peek2(types, function_type);
	const auto function_type_args = function_type_peek.get_function_args(types);
	const auto return_type = function_type_peek.get_function_return(types);

	cif_t result;

	//	This is the runtime pointer, passed as first argument to all functions.
	result.arg_types.push_back(&ffi_type_pointer);

	const auto function_type_args_size = function_type_args.size();
	for(int arg_index = 0 ; arg_index < function_type_args_size ; arg_index++){
		const auto func_arg_type = function_type_args[arg_index];

		//	Notice that ANY will represent this function type arg as TWO elements in BC stack and in FFI argument list
		if(func_arg_type.is_any()){
			//	rt_pod_t
			result.arg_types.push_back(&ffi_type_uint64);

			//	rt_type_t
			result.arg_types.push_back(&ffi_type_uint64);
		}
		else{
			const auto f = make_ffi_type(peek2(types, func_arg_type));
			result.arg_types.push_back(f);
		}
	}

	const auto return_type_peek = peek2(types, return_type);
	result.return_type = return_type_peek.is_any() ? make_ffi_type(type_t::make_int()) : make_ffi_type(return_type_peek);

	if (ffi_prep_cif(&result.cif, FFI_DEFAULT_ABI, (int)result.arg_types.size(), result.return_type, &result.arg_types[0]) != FFI_OK) {
		quark::throw_defective_request();
	}
	return result;
}

static void call_via_libffi(interpreter_t& vm, int target_reg, const func_link_t& func_link, int callee_arg_count){
	QUARK_ASSERT(vm.check_invariant());
	QUARK_ASSERT(func_link.f != nullptr);

	auto& backend = vm._backend;
	const auto& types = backend.types;

	auto cif = make_cif(backend, func_link.function_type_optional);

	interpreter_stack_t& stack = vm._stack;

	const auto function_type_peek = peek2(types, func_link.function_type_optional);

	const auto function_type_args = function_type_peek.get_function_args(types);
	const auto function_def_dynamic_arg_count = count_dyn_args(types, func_link.function_type_optional);

	const int arg0_stack_pos = (int)(stack.size() - (function_def_dynamic_arg_count + callee_arg_count));
	int stack_pos = arg0_stack_pos;

	const auto runtime_ptr = runtime_t { vm._name, &backend, &vm, vm._process_handler };

	std::vector<void*> values;

	//	This is the runtime pointer, passed as first argument to all functions.
	auto runtime_ptr_ptr = &runtime_ptr;
	values.push_back((void*)&runtime_ptr_ptr);

	const auto function_type_args_size = function_type_args.size();

	std::vector<rt_value_t> storage;
	storage.reserve(function_type_args_size + function_def_dynamic_arg_count);

	for(int arg_index = 0 ; arg_index < function_type_args_size ; arg_index++){
		const auto func_arg_type = function_type_args[arg_index];

		//	Notice that ANY will represent this function type arg as TWO elements in BC stack and in FFI argument list
		if(func_arg_type.is_any()){
			const auto arg_itype = stack._entries[stack_pos + 0].int_value;
			const auto arg_pod = stack._entries[stack_pos + 1];

			const auto arg_type = lookup_full_type(vm, static_cast<int16_t>(arg_itype));

			storage.push_back(rt_value_t::make_int(arg_type.get_data()));

			//	Store the value as a 64 bit integer always.
			storage.push_back(rt_value_t::make_int(arg_pod.int_value));

			//	Notice: in ABI, any is passed like: [ rt_pod_t value, rt_type_t value_type ], notice swapped order!
			//	Use a pointer directly into storage. This is tricky. Requires reserve().
			values.push_back(&storage[storage.size() - 1]._pod);
			values.push_back(&storage[storage.size() - 2]._pod);

			stack_pos++;
			stack_pos++;
		}
		else{
			const auto arg_value = stack.load_value(stack_pos + 0, func_arg_type);
			storage.push_back(arg_value);

			//	Use a pointer directly into storage. This is tricky. Requires reserve().
			values.push_back(&storage[storage.size() - 1]._pod);
			stack_pos++;
		}
	}

	ffi_arg return_value;

	ffi_call(&cif.cif, FFI_FN(func_link.f), &return_value, &values[0]);

	const auto& function_return_type = function_type_peek.get_function_return(types);
	const auto function_return_type_peek = peek2(types, function_return_type);
	if(function_return_type_peek.is_void() == true){
	}
	else{
		const auto result2 = *(rt_pod_t*)&return_value;

		//	Cast the result to the destination symbol's type.
		const auto dest_type = vm._current_static_frame->_symbol_effective_type[target_reg];
		const auto result3 = rt_value_t(backend, result2, dest_type, rt_value_t::rc_mode::adopt);
		vm.write_register(target_reg, result3);
	}
}

//	This is a floyd function, with a frame_ptr to execute.
//	The arguments are already on the floyd stack.
//	Returns the call's return value or void.
static rt_value_t open_frame_and_make_call(interpreter_t& vm, const func_link_t& func_link, int callee_arg_count){
	QUARK_ASSERT(vm.check_invariant());
	const auto& backend = vm._backend;
	const auto& types = backend.types;
	QUARK_ASSERT(func_link.f != nullptr);
	QUARK_ASSERT(func_link.function_type_optional.get_function_args(types).size() == callee_arg_count);

	QUARK_ASSERT(count_dyn_args(types, func_link.function_type_optional) == 0);

	//	We need to remember the global pos where to store return value, since we're switching frame to call function.
	auto frame_ptr = (const bc_static_frame_t*)func_link.f;

	vm.open_frame_except_args(*frame_ptr, callee_arg_count);
	const auto& result = execute_instructions(vm, frame_ptr->_instructions);
	QUARK_ASSERT(result.second.check_invariant());
	vm.close_frame(*frame_ptr);

	const auto& return_type = peek2(types, func_link.function_type_optional).get_function_return(types);
	const auto return_type_peek = peek2(types, return_type);
	if(return_type_peek.is_void()){
		return rt_value_t::make_void();
	}
	else{
		return result.second;
	}
}

//	This is a floyd function, with a frame_ptr to execute.
//	The arguments are already on the floyd stack.
static void call_bc(interpreter_t& vm, int target_reg, const func_link_t& func_link, int callee_arg_count){
	QUARK_ASSERT(vm.check_invariant());
	const auto& backend = vm._backend;
	const auto& types = backend.types;
	QUARK_ASSERT(func_link.f != nullptr);
	QUARK_ASSERT(func_link.function_type_optional.get_function_args(types).size() == callee_arg_count);

	interpreter_stack_t& stack = vm._stack;

	const auto& return_type = peek2(types, func_link.function_type_optional).get_function_return(types);

	QUARK_ASSERT(count_dyn_args(types, func_link.function_type_optional) == 0);

	//	We need to remember the global pos where to store return value, since we're switching frame to call function.
	int result_reg_pos = static_cast<int>(vm._current_frame_start_ptr - &stack._entries[0]) + target_reg;

	const auto result = open_frame_and_make_call(vm, func_link, callee_arg_count);

	const auto return_type_peek = peek2(types, return_type);
	if(return_type_peek.is_void() == false){
		//	Cannot store via register, we store on absolute stack pos (relative to start of
		//	stack. We have not yet executed k_pop_frame_ptr that restores our frame.
		stack.replace_external_value(result_reg_pos, result);
	}
}

static void do_call_instruction(interpreter_t& vm, int target_reg, const rt_pod_t callee, int callee_arg_count){
	QUARK_ASSERT(vm.check_invariant());
	const auto& backend = vm._backend;

	const auto func_link_ptr = lookup_func_link_by_pod(backend, callee);
	if(func_link_ptr == nullptr || func_link_ptr->f == nullptr){
		quark::throw_runtime_error("Attempting to calling unimplemented function.");
	}
	const auto& func_link = *func_link_ptr;

	if(func_link.execution_model == func_link_t::eexecution_model::k_bytecode__floydcc){
		call_bc(vm, target_reg, func_link, callee_arg_count);
	}
	else if(func_link.execution_model == func_link_t::eexecution_model::k_native__floydcc){
		call_via_libffi(vm, target_reg, func_link, callee_arg_count);
	}
	else{
		quark::throw_defective_request();
	}
}

rt_value_t call_function_bc(interpreter_t& vm, const rt_value_t& f, const rt_value_t args[], int callee_arg_count){
	QUARK_ASSERT(vm.check_invariant());
	QUARK_ASSERT(f.check_invariant());

	const auto& backend = vm._backend;
	const auto& types = backend.types;

#if DEBUG
	for(int i = 0 ; i < callee_arg_count ; i++){ QUARK_ASSERT(args[i].check_invariant()); };
	QUARK_ASSERT(peek2(types, f._type).is_function());

	const auto& arg_types = peek2(types, f._type).get_function_args(types);

	//	arity
	QUARK_ASSERT(callee_arg_count == arg_types.size());

	for(int i = 0 ; i < callee_arg_count; i++){
		if(peek2(types, args[i]._type) != peek2(types, arg_types[i])){
			quark::throw_defective_request();
		}
	}
#endif

	const auto func_link_ptr = lookup_func_link_by_pod(backend, f._pod);
	if(func_link_ptr == nullptr || func_link_ptr->f == nullptr){
		quark::throw_runtime_error("Attempting to calling unimplemented function.");
	}
	const auto& func_link = *func_link_ptr;

	if(func_link.execution_model == func_link_t::eexecution_model::k_bytecode__floydcc){
		const auto& return_type = peek2(types, func_link.function_type_optional).get_function_return(types);

		vm.save_frame();

		//	We push the values to the stack = the stack will take RC ownership of the values.
		std::vector<type_t> arg_types2;
		for(int i = 0 ; i < callee_arg_count ; i++){
			const auto& bc = args[i];
			const auto is_ext = args[i]._type;
			arg_types2.push_back(is_ext);
			vm._stack.push_external_value(bc);
		}

		const auto result = open_frame_and_make_call(vm, func_link, callee_arg_count);

		vm._stack.pop_batch(arg_types2);
		vm.restore_frame();

		const auto return_type_peek = peek2(types, return_type);
		if(return_type_peek.is_void() == false){
			QUARK_ASSERT(result.check_invariant());
			return result;
		}
		else{
			return rt_value_t::make_void();
		}
	}
	else{
		quark::throw_defective_request();
	}
}

void interpreter_t::runtime_basics__on_print(const std::string& s){
	_runtime_handler->on_print(s);
}

type_t interpreter_t::runtime_basics__get_global_symbol_type(const std::string& s){
	const auto& symbols = _program->_globals._symbols;

	const auto it = std::find_if(symbols._symbols.begin(), symbols._symbols.end(), [&s](const auto& e){ return e.first == s; });
	if(it == symbols._symbols.end()){
		quark::throw_defective_request();
	}

	return it->second._value_type;
}

rt_value_t interpreter_t::runtime_basics__call_thunk(const rt_value_t& f, const rt_value_t args[], int arg_count){
	return call_function_bc(*this, f, args, arg_count);
}

void interpreter_t::save_frame(){
	const auto frame_pos = rt_value_t::make_int(get_current_frame_pos());
	_stack.push_inplace_value(frame_pos);

	const auto frame_ptr = rt_value_t(_current_static_frame);
	_stack.push_inplace_value(frame_ptr);
}

void interpreter_t::restore_frame(){
	QUARK_ASSERT(check_invariant());
	QUARK_ASSERT(_stack._stack_size >= k_frame_overhead);

	const auto frame_pos = _stack._entries[_stack._stack_size - k_frame_overhead + 0].int_value;
	const auto frame_ptr = (const bc_static_frame_t*)_stack._entries[_stack._stack_size - k_frame_overhead + 1].frame_ptr;
	_stack._stack_size -= k_frame_overhead;
	_stack._entry_types.pop_back();
	_stack._entry_types.pop_back();
	_current_static_frame = frame_ptr;
	_current_frame_start_ptr = &_stack._entries[frame_pos];
}

//	Function arguments MUST ALREADY have been pushed on the stack!! Only handles locals.
//	??? Faster: This function should just allocate a block for frame, then have a list of writes.
//	???	ALTERNATIVELY: generate instructions to do this in the VM? Nah, that's always slower.
void interpreter_t::open_frame_except_args(const bc_static_frame_t& frame, int pushed_arg_count){
	QUARK_ASSERT(check_invariant());
	QUARK_ASSERT(frame.check_invariant());
	QUARK_ASSERT(frame._arg_count == pushed_arg_count);

	const auto pos_with_args_already_pushed = _stack.size();

	//	Carefully position the new stack frame so its includes the parameters that already sits in the stack.
	//	The stack frame already has symbols/registers mapped for those parameters.
	const auto new_frame_start_pos = pos_with_args_already_pushed - pushed_arg_count;

	for(int i = 0 ; i < frame._locals_count ; i++){
		const int symbol_index = i + frame._arg_count;
		const auto& symbol_kv = frame._symbols._symbols[symbol_index];
		const auto& symbol = symbol_kv.second;
		const auto type = symbol._value_type;
		const bool ext = is_rc_value(_backend.types, type);

		if(symbol._symbol_type == symbol_t::symbol_type::immutable_reserve){
			if(ext){
				_stack.push_external_value_blank(type);
			}
			else{
				_stack.push_inplace_value(value_to_rt(_backend, make_default_value(type)));
			}
		}
		else if(symbol._symbol_type == symbol_t::symbol_type::immutable_arg){
			quark::throw_defective_request();
		}
		else if(symbol._symbol_type == symbol_t::symbol_type::immutable_precalc){
//				QUARK_ASSERT(ext == false || (type.is_json() && symbol._init.get_json().is_null()));
			if(ext){
				_stack.push_external_value(value_to_rt(_backend, symbol._init));
			}
			else {
				_stack.push_inplace_value(value_to_rt(_backend, symbol._init));
			}
		}
		else if(symbol._symbol_type == symbol_t::symbol_type::named_type){
			const auto v = rt_value_t::make_typeid_value(symbol._value_type);
			_stack.push_inplace_value(v);
		}
		else if(symbol._symbol_type == symbol_t::symbol_type::mutable_reserve){
			if(ext){
				_stack.push_external_value_blank(type);
			}
			else{
				_stack.push_inplace_value(value_to_rt(_backend, make_default_value(type)));
			}
		}
		else {
			quark::throw_defective_request();
		}
	}
	_current_static_frame = &frame;
	_current_frame_start_ptr = &_stack._entries[new_frame_start_pos];

	QUARK_ASSERT(check_invariant());
}

//	Pops all locals, decrementing RC when needed.
//	Only handles locals, not parameters.
void interpreter_t::close_frame(const bc_static_frame_t& frame){
	QUARK_ASSERT(check_invariant());
	QUARK_ASSERT(frame.check_invariant());

//		const auto type = frame._symbols[i + frame._arg_count].second._value_type;
	const auto count = frame._symbols._symbols.size() - frame._arg_count;
	for(auto i = count ; i > 0 ; i--){
		const auto symbol_index = frame._arg_count + i - 1;
		const auto& type = frame._symbol_effective_type[symbol_index];
		_stack.pop(type);
	}
	QUARK_ASSERT(check_invariant());
}

frame_pos_t interpreter_t::read_frame_info(size_t pos) const{
//	QUARK_ASSERT(check_invariant());

	const auto v = _stack.load_intq((int)(pos + 0));
	QUARK_ASSERT(v >= 0);

	const auto ptr = (const bc_static_frame_t*)_stack._entries[pos + 1].frame_ptr;
	QUARK_ASSERT(ptr == nullptr || ptr->check_invariant());

	return frame_pos_t{ (size_t)v, ptr };
}

bool interpreter_t::check_stack_frame(const frame_pos_t& in_frame) const{
	QUARK_ASSERT(in_frame._frame_ptr != nullptr && in_frame._frame_ptr->check_invariant());

	if(in_frame._frame_pos == 0){
		return true;
	}
	else{
		QUARK_ASSERT(in_frame._frame_ptr->_symbols.check_invariant());

		for(int i = 0 ; i < in_frame._frame_ptr->_symbols._symbols.size() ; i++){
//			const auto& symbol = in_frame._frame_ptr->_symbols._symbols[i];

//			bool symbol_ext = encode_as_external(_types, symbol.second._value_type);
//			int local_pos = get_local_n_pos(in_frame._frame_pos, i);

//			bool stack_ext = debug_is_ext(local_pos);
//			QUARK_ASSERT(symbol_ext == stack_ext);
		}

		const auto prev_frame = read_frame_info(in_frame._frame_pos - k_frame_overhead);
		QUARK_ASSERT(prev_frame._frame_pos >= 0 && prev_frame._frame_pos < in_frame._frame_pos);
		if(prev_frame._frame_pos > k_frame_overhead){
			return check_stack_frame(prev_frame);
		}
		else{
			return true;
		}
	}
}

//	Returned elements are sorted with smaller stack positions first.
//	Walks the stack from the active frame towards the start of the stack, the globals frame.
std::vector<active_frame_t> interpreter_t::get_stack_frames_noci() const{
//	QUARK_ASSERT(check_invariant());

	std::vector<active_frame_t> result;

	const auto frame_start0 = get_current_frame_pos();
	if(frame_start0 > 0){
		const auto stack_size = static_cast<int>(_stack._stack_size);

		{
			const auto frame_size0 = _current_static_frame->_symbols._symbols.size();
			const auto frame_end0 = frame_start0 + frame_size0;

			//??? Notice: during a call sequence: push_frame_ptr, push arguments, call, popn, pop_frame_ptr -- the
			//	stack size vs frames is slighly out of sync and gives us negative temporary stack entries.
			const auto temp_count0 = stack_size >= frame_end0 ? stack_size - frame_end0 : 0;
			const auto e = active_frame_t { frame_start0, frame_end0, _current_static_frame, temp_count0 };
			result.push_back(e);
		}

		auto info_pos = frame_start0 - k_frame_overhead;
		auto frame = read_frame_info(info_pos);
		while(frame._frame_ptr != nullptr){
			const auto frame_start = frame._frame_pos;
			const auto frame_size = frame._frame_ptr->_symbols._symbols.size();
			const auto frame_end = frame_start + frame_size;
			const auto temp_count = info_pos >= frame_end ? info_pos - frame_end : 0;

			const auto e = active_frame_t { frame_start, frame_end, frame._frame_ptr, temp_count };
			result.push_back(e);

			info_pos = frame_start - k_frame_overhead;
			frame = read_frame_info(info_pos);
		}

		std::reverse(result.begin(), result.end());
	}
	return result;
}


//???std::chrono::high_resolution_clock::now()

interpreter_t::interpreter_t(
	const std::shared_ptr<bc_program_t>& program,
	value_backend_t& backend,
	const config_t& config,
	runtime_process_i* process_handler,
	runtime_handler_i& runtime_handler,
	const std::string& name
) :
	_program(program),
	_process_handler(process_handler),
	_runtime_handler(&runtime_handler),
	_backend(backend),
	_name(name),
	_stack { &_backend },
	_current_frame_start_ptr(nullptr),
	_current_static_frame(nullptr)
{
	QUARK_ASSERT(program && program->check_invariant());
	QUARK_ASSERT(backend.check_invariant());

	_current_frame_start_ptr = &_stack._entries[0];

	{
		save_frame();
		open_frame_except_args(_program->_globals, 0);

		QUARK_ASSERT(check_invariant());
		if(false) trace_interpreter(*this, 0);

		//	Run static intialization (basically run global instructions before calling main()).
		execute_instructions(*this, _program->_globals._instructions);

		if(false) trace_interpreter(*this, 0);

		//	close_frame() is called in destructor. This allows clients to read globals etc before interpreter is destructed.
//		_stack.close_frame(_program->_globals);
	}
	QUARK_ASSERT(check_invariant());
}

interpreter_t::~interpreter_t(){
	QUARK_ASSERT(check_invariant());
}

void interpreter_t::swap(interpreter_t& other) throw(){
	other._program.swap(this->_program);
	std::swap(other._process_handler, this->_process_handler);
	std::swap(other._runtime_handler, this->_runtime_handler);
	other._stack.swap(this->_stack);
	std::swap(_current_frame_start_ptr, other._current_frame_start_ptr);
	std::swap(_current_static_frame, other._current_static_frame);
}

bool interpreter_t::check_invariant() const {
	QUARK_ASSERT(check_invariant_thread_safe());

	//	Also check per-thread state:

	QUARK_ASSERT(_stack.check_invariant());
	QUARK_ASSERT(_current_frame_start_ptr >= &_stack._entries[0]);
	QUARK_ASSERT(_current_frame_start_ptr <= &_stack._entries[_stack._stack_size]);
	QUARK_ASSERT(_current_static_frame == 0 || _current_static_frame->check_invariant());

	//??? Traverse all stack frames and check them.

	return true;
}
bool interpreter_t::check_invariant_thread_safe() const {
	QUARK_ASSERT(_program && _program->check_invariant());
	QUARK_ASSERT(_backend.check_invariant());

//	QUARK_ASSERT(_process_handler != nullptr);
	QUARK_ASSERT(_runtime_handler != nullptr);
	return true;
}


void interpreter_t::unwind_stack(){
	QUARK_ASSERT(check_invariant());

	QUARK_ASSERT(_current_static_frame == &_program->_globals);
	close_frame(_program->_globals);

	QUARK_ASSERT(check_invariant());
}





//??? Unify this debug code with trace_value_backend()
static std::vector<std::string> make(
	value_backend_t& backend,
	size_t i,
	rt_pod_t bc_pod,
	const type_t& debug_type,
	const active_frame_t& frame,
	int frame_index,
	int symbol_index,
	bool is_symbol
){
	const bool is_rc = is_rc_value(backend.types, debug_type);

	std::string value_str = "";
	std::string rc_str = "";
	std::string alloc_id_str = "";

	std::string symbol_effective_type = type_to_compact_string(backend.types, frame.static_frame->_symbol_effective_type[symbol_index]);

	if(is_rc){
		//??? Related to k_init_local
		const bool illegal_rc_value = is_rc && (bc_pod.int_value == UNINITIALIZED_RUNTIME_VALUE);

		if(illegal_rc_value){
			value_str = "***RC VALUE: UNINITIALIZED***";
		}
		else{
			//	Hack for now, gets the alloc struct for any type of RC-value, but uses struct_ptr.
			const auto& alloc = bc_pod.struct_ptr->alloc;
			const auto alloc_id = alloc.alloc_id;
			const int32_t rc = alloc.rc;

			rc_str = std::to_string(rc);
			alloc_id_str = std::to_string(alloc_id);

			if(rc < 0){
				value_str = "***RC VALUE: NEGATIVE RC???***";
			}
			else if(rc == 0){
				value_str = "***RC VALUE: DELETED***";
			}
			else {
				const auto bc_that_owns_rc = rt_value_t(backend, bc_pod, debug_type, rt_value_t::rc_mode::bump);
				value_str = json_to_compact_string(bcvalue_to_json(backend, bc_that_owns_rc));
			}
		}
	}
	else{
		const auto bc_that_owns_rc = rt_value_t(backend, bc_pod, debug_type, rt_value_t::rc_mode::bump);
		value_str = json_to_compact_string(bcvalue_to_json(backend, bc_that_owns_rc));
	}

	const std::string frame_str = std::to_string(frame_index) + (frame_index == 0 ? " GLOBAL" : "");
	if(is_symbol){
		const auto& symbol = frame.static_frame->_symbols._symbols[symbol_index];
		const std::string symname = symbol.first;
		const std::string symbol_value_type = type_to_compact_string(backend.types, symbol.second._value_type);

		const auto line = std::vector<std::string> {
			std::to_string(i) + ":" + std::to_string(symbol_index),
			type_to_compact_string(backend.types, debug_type),
			value_str,
			rc_str,
			alloc_id_str,

			frame_str,
			symname,
			symbol_value_type,
			symbol_effective_type
		};
		return line;
	}
	else{
		const std::string symname = std::string() + "stack-temp" + std::to_string(symbol_index);
		const std::string symbol_value_type = "";

		const auto line = std::vector<std::string> {
			std::to_string(i) + ":" + std::to_string(symbol_index),
			type_to_compact_string(backend.types, debug_type),
			value_str,
			rc_str,
			alloc_id_str,

			frame_str,
			symname,
			symbol_value_type,
			symbol_effective_type
		};
		return line;
	}
}

void trace_interpreter(interpreter_t& vm, int pc){
	if(should_trace(vm._name) == false){
		return;
	}

	QUARK_SCOPED_TRACE(vm._name);

	auto& backend = vm._backend;
	const auto& stack = vm._stack;

	{
		QUARK_SCOPED_TRACE("STACK");

		const auto stack_frames = vm.get_stack_frames_noci();

		std::vector<std::vector<std::string>> matrix;

		size_t stack_pos = 0;
		for(int frame_index = 0 ; frame_index < stack_frames.size() ; frame_index++){
			const auto line0 = std::vector<std::string> {
				std::to_string(stack_pos),
				"",
				std::to_string(stack._entries[stack_pos].int_value),
				"",
				"",

				"",
				"prev frame pos",
				"",
				""
			};
			matrix.push_back(line0);
			stack_pos++;

			const auto line1 = std::vector<std::string> {
				std::to_string(stack_pos),
				"",
				ptr_to_hexstring(stack._entries[stack_pos].frame_ptr),
				"",
				"",

				"",
				"prev frame ptr",
				"",
				""
			};
			matrix.push_back(line1);
			stack_pos++;

			const auto& frame = stack_frames[frame_index];

			for(int symbol_index = 0 ; symbol_index < (frame.end_pos - frame.start_pos) ; symbol_index++){
				const auto bc_pod = stack._entries[stack_pos];
				const auto debug_type = stack._entry_types[stack_pos];
				const auto line = make(backend, stack_pos, bc_pod, debug_type, frame, frame_index, symbol_index, true);
				matrix.push_back(line);
				stack_pos++;
			}
			for(int temp_index = 0 ; temp_index < frame.temp_count ; temp_index++){
				const auto bc_pod = stack._entries[stack_pos];
				const auto debug_type = stack._entry_types[stack_pos];
				const auto line = make(backend, stack_pos, bc_pod, debug_type, frame, frame_index, temp_index, false);
				matrix.push_back(line);
				stack_pos++;
			}
		}

		QUARK_SCOPED_TRACE("STACK ENTRIES");
		const auto result = generate_table_type1(
			{ "#", "debug type", "value", "RC", "alloc ID", "frame", "symname", "sym.value type", "sym.effective type" },
			matrix
		);
		QUARK_TRACE(result);
	}

	trace_value_backend_dynamic(backend);

	const auto& frame = *vm._current_static_frame;
	{
		std::vector<json_t> instructions;
		int pos = 0;
		for(const auto& e: frame._instructions){
			const auto cursor = pos == pc ? "===>" : "    ";

			const auto i = json_t::make_array({
				cursor,
				pos,
				opcode_to_string(e._opcode),
				json_t(e._a),
				json_t(e._b),
				json_t(e._c)
			});
			instructions.push_back(i);
			pos++;
		}

		QUARK_SCOPED_TRACE("INSTRUCTIONS");
		QUARK_TRACE(json_to_pretty_string(json_t::make_array(instructions)));
	}
}



//////////////////////////////////////////		INSTRUCTIONS



//	IMPORTANT: NO arguments are passed as DYN arguments.
static void execute_new_1(interpreter_t& vm, int16_t dest_reg, int16_t target_itype, int16_t source_itype){
	QUARK_ASSERT(vm.check_invariant());

	auto& backend = vm._backend;
	const auto& types = backend.types;

	const auto& target_type = lookup_full_type(vm, target_itype);
	const auto target_peek = peek2(types, target_type);
	QUARK_ASSERT(target_peek.is_vector() == false && target_peek.is_dict() == false && target_peek.is_struct() == false);

	const int arg0_stack_pos = vm._stack.size() - 1;
	const auto source_itype2 = lookup_type_from_index(types, source_itype);
	const auto input_value = vm._stack.load_value(arg0_stack_pos + 0, source_itype2);

	const rt_value_t result = [&]{
		if(target_peek.is_bool() || target_peek.is_int() || target_peek.is_double() || target_peek.is_typeid()){
			return input_value;
		}

		//	Automatically transform a json::string => string at runtime?
		else if(target_peek.is_string() && peek2(types, source_itype2).is_json()){
			if(input_value.get_json().is_string()){
				return rt_value_t::make_string(backend, input_value.get_json().get_string());
			}
			else{
				quark::throw_runtime_error("Attempting to assign a non-string JSON to a string.");
			}
		}
		else if(target_peek.is_json()){
			const auto arg = bcvalue_to_json(backend, input_value);
			return rt_value_t::make_json(backend, arg);
		}
		else{
			return input_value;
		}
	}();

	vm.write_register(dest_reg, result);
}

//	IMPORTANT: No arguments are passed as DYN arguments.
static void execute_new_vector_obj(interpreter_t& vm, int16_t dest_reg, int16_t target_itype, int16_t arg_count){
	QUARK_ASSERT(vm.check_invariant());

	auto& backend = vm._backend;
	const auto& types = backend.types;
	if(false) trace_types(types);

	const auto& target_type = lookup_full_type(vm, target_itype);
	const auto target_peek = peek2(types, target_type);
	QUARK_ASSERT(target_peek.is_vector());

	const auto& element_type = target_peek.get_vector_element_type(types);
	QUARK_ASSERT(element_type.is_undefined() == false);
	QUARK_ASSERT(target_type.is_undefined() == false);

	const int arg0_stack_pos = vm._stack.size() - arg_count;

	immer::vector<rt_value_t> elements2;
	for(int i = 0 ; i < arg_count ; i++){
		const auto pos = arg0_stack_pos + i;
		QUARK_ASSERT(peek2(types, vm._stack._entry_types[pos]) == type_t(peek2(types, element_type)));
		const auto& e = vm._stack._entries[pos];
		const auto e2 = rt_value_t(backend, e, element_type, rt_value_t::rc_mode::bump);
		elements2 = elements2.push_back(e2);
	}

	const auto result = make_vector_value(backend, element_type, elements2);
	vm.write_register(dest_reg, result);
}

static void execute_new_dict_obj(interpreter_t& vm, int16_t dest_reg, int16_t target_itype, int16_t arg_count){
	QUARK_ASSERT(vm.check_invariant());

	auto& backend = vm._backend;
	const auto& types = backend.types;

	const auto& target_type = lookup_full_type(vm, target_itype);
	const auto target_peek = peek2(types, target_type);
	QUARK_ASSERT(target_peek.is_dict());

	const int arg0_stack_pos = vm._stack.size() - arg_count;

	const auto& element_type = target_peek.get_dict_value_type(types);
	QUARK_ASSERT(target_type.is_undefined() == false);
	QUARK_ASSERT(element_type.is_undefined() == false);

	const auto string_type = type_t::make_string();

	immer::map<std::string, rt_value_t> elements2;
	int dict_element_count = arg_count / 2;
	for(auto i = 0 ; i < dict_element_count ; i++){
		const auto key = vm._stack.load_value(arg0_stack_pos + i * 2 + 0, string_type);
		const auto value = vm._stack.load_value(arg0_stack_pos + i * 2 + 1, element_type);
		const auto key2 = key.get_string_value(backend);
		elements2 = elements2.insert({ key2, value });
	}

	const auto result = make_dict_value(backend, element_type, elements2);
	vm.write_register(dest_reg, result);
}

static void execute_new_dict_pod64(interpreter_t& vm, int16_t dest_reg, int16_t target_itype, int16_t arg_count){
	QUARK_ASSERT(vm.check_invariant());

	execute_new_dict_obj(vm, dest_reg, target_itype, arg_count);
/*
	const auto& types = vm._imm->_program._types;
	const auto& target_type = lookup_full_type(vm, target_itype);
	const auto target_peek = peek2(types, target_type);
	QUARK_ASSERT(target_peek.is_dict());

	const int arg0_stack_pos = vm._stack.size() - arg_count;

	const auto& element_type = target_peek.get_dict_value_type(types);
	QUARK_ASSERT(target_type.is_undefined() == false);
	QUARK_ASSERT(element_type.is_undefined() == false);

	const auto string_type = type_t::make_string();

	immer::map<std::string, bc_inplace_value_t> elements2;
	int dict_element_count = arg_count / 2;
	for(auto i = 0 ; i < dict_element_count ; i++){
		const auto key = vm._stack.load_value(arg0_stack_pos + i * 2 + 0, string_type);
		const auto value = vm._stack.load_value(arg0_stack_pos + i * 2 + 1, element_type);
		const auto key2 = key.get_string_value();
		elements2 = elements2.insert({ key2, value._pod._inplace });
	}

	const auto result = make_dict(types, element_type, elements2);
	vm.write_register(dest_reg, result);
*/
}

static void execute_new_struct(interpreter_t& vm, int16_t dest_reg, int16_t target_itype, int16_t arg_count){
	QUARK_ASSERT(vm.check_invariant());

	auto& backend = vm._backend;
	const auto& types = backend.types;

	const auto& target_type = lookup_full_type(vm, target_itype);
	const auto& target_peek = peek2(types, target_type);
	QUARK_ASSERT(target_peek.is_struct());

	const int arg0_stack_pos = vm._stack.size() - arg_count;

	const auto& struct_def = target_peek.get_struct(types);
	std::vector<rt_value_t> elements2;
	for(int i = 0 ; i < arg_count ; i++){
		const auto member_type0 = struct_def._members[i]._type;
		const auto member_type = peek2(types, member_type0);
		const auto value = vm._stack.load_value(arg0_stack_pos + i, member_type);
		elements2.push_back(value);
	}

	const auto result = rt_value_t::make_struct_value(backend, target_type, elements2);
	QUARK_ASSERT(result.check_invariant());

	vm.write_register(dest_reg, result);
}

inline void write_reg_rc(value_backend_t& backend, rt_pod_t regs[], int dest_reg, const type_t& dest_type, rt_pod_t value){
	QUARK_ASSERT(backend.check_invariant());
	QUARK_ASSERT(dest_type.check_invariant());

	release_value_safe(backend, regs[dest_reg], dest_type);
	retain_value(backend, value, dest_type);
	regs[dest_reg] = value;
}

std::pair<bc_typeid_t, rt_value_t> execute_instructions(interpreter_t& vm, const std::vector<bc_instruction_t>& instructions){
	QUARK_ASSERT(vm.check_invariant());
	QUARK_ASSERT(
		instructions.empty() == true
		|| (instructions.back()._opcode == bc_opcode::k_return || instructions.back()._opcode == bc_opcode::k_stop)
	);

	auto& backend = vm._backend;
	const auto& types = backend.types;

	interpreter_stack_t& stack = vm._stack;
	const bc_static_frame_t* frame_ptr = vm._current_static_frame;
	auto* regs = vm._current_frame_start_ptr;
	auto* globals = &stack._entries[k_frame_overhead];

//	QUARK_TRACE_SS("STACK:  " << json_to_pretty_string(stack.stack_to_json()));

	int pc = 0;

	if(k_trace_stepping){
		trace_interpreter(vm, pc);
	}

	while(true){
		QUARK_ASSERT(pc >= 0);
		QUARK_ASSERT(pc < instructions.size());
		const auto i = instructions[pc];

		QUARK_ASSERT(vm.check_invariant());
		QUARK_ASSERT(i.check_invariant());

		QUARK_ASSERT(frame_ptr == vm._current_static_frame);
		QUARK_ASSERT(regs == vm._current_frame_start_ptr);


		const auto opcode = i._opcode;

		switch(opcode){

		case bc_opcode::k_nop:
			break;


		//////////////////////////////////////////		ACCESS GLOBALS


		case bc_opcode::k_load_global_external_value: {
			QUARK_ASSERT(vm.check_reg__external_value(i._a));
			QUARK_ASSERT(vm.check_global_access_obj(i._b));

			const auto& type = frame_ptr->_symbols._symbols[i._a].second._value_type;
			release_value_safe(backend, regs[i._a], type);
			const auto& new_value_pod = globals[i._b];
			regs[i._a] = new_value_pod;
			retain_value(backend, new_value_pod, type);
			break;
		}
		case bc_opcode::k_load_global_inplace_value: {
			QUARK_ASSERT(vm.check_reg__inplace_value(i._a));

			regs[i._a] = globals[i._b];
			break;
		}

		case bc_opcode::k_init_local: {
			const auto& type = frame_ptr->_symbols._symbols[i._a].second._value_type;
			const auto& new_value_pod = regs[i._b];
			regs[i._a] = new_value_pod;
			retain_value(backend, new_value_pod, type);
			break;
		}

		case bc_opcode::k_store_global_external_value: {
			QUARK_ASSERT(vm.check_global_access_obj(i._a));
			QUARK_ASSERT(vm.check_reg__external_value(i._b));

			const auto& type = frame_ptr->_symbols._symbols[i._b].second._value_type;
			release_value_safe(backend, globals[i._a], type);
			const auto& new_value_pod = regs[i._b];
			globals[i._a] = new_value_pod;
			retain_value(backend, new_value_pod, type);
			break;
		}
		case bc_opcode::k_store_global_inplace_value: {
			QUARK_ASSERT(vm.check_global_access_intern(i._a));
			QUARK_ASSERT(vm.check_reg__inplace_value(i._b));

			globals[i._a] = regs[i._b];
			break;
		}


		//////////////////////////////////////////		ACCESS LOCALS


		case bc_opcode::k_copy_reg_inplace_value: {
			QUARK_ASSERT(vm.check_reg__inplace_value(i._a));
			QUARK_ASSERT(vm.check_reg__inplace_value(i._b));

			regs[i._a] = regs[i._b];
			break;
		}
		case bc_opcode::k_copy_reg_external_value: {
			QUARK_ASSERT(vm.check_reg__external_value(i._a));
			QUARK_ASSERT(vm.check_reg__external_value(i._b));

			const auto& type = frame_ptr->_symbols._symbols[i._a].second._value_type;
			release_value_safe(backend, regs[i._a], type);
			const auto& new_value_pod = regs[i._b];
			regs[i._a] = new_value_pod;
			retain_value(backend, new_value_pod, type);
			break;
		}


		//////////////////////////////////////////		STACK


		case bc_opcode::k_return: {
			const auto& type = frame_ptr->_symbols._symbols[i._a].second._value_type;
			return { true, rt_value_t(backend, regs[i._a], type, rt_value_t::rc_mode::bump) };
		}

		case bc_opcode::k_stop: {
			return { false, rt_value_t::make_undefined() };
		}

		case bc_opcode::k_push_frame_ptr: {
			QUARK_ASSERT(vm.check_invariant());
			QUARK_ASSERT((stack._stack_size + k_frame_overhead) < stack._allocated_count)

			stack._entries[stack._stack_size + 0].int_value = static_cast<int64_t>(vm._current_frame_start_ptr - &stack._entries[0]);
			stack._entries[stack._stack_size + 1].frame_ptr = (void*)frame_ptr;
			stack._stack_size += k_frame_overhead;
			stack._entry_types.push_back(type_t::make_int());
			stack._entry_types.push_back(type_t::make_void());
			QUARK_ASSERT(vm.check_invariant());
			break;
		}

		case bc_opcode::k_pop_frame_ptr: {
			QUARK_ASSERT(vm.check_invariant());
			QUARK_ASSERT(stack._stack_size >= k_frame_overhead);

			const auto frame_pos = stack._entries[stack._stack_size - k_frame_overhead + 0].int_value;
			frame_ptr = (bc_static_frame_t*)stack._entries[stack._stack_size - k_frame_overhead + 1].frame_ptr;
			stack._stack_size -= k_frame_overhead;

			stack._entry_types.pop_back();
			stack._entry_types.pop_back();

			regs = &stack._entries[frame_pos];

			//??? do we need vm._current_static_frame, stack._current_frame_pos? Only use local variables to track these?
			vm._current_static_frame = frame_ptr;
			vm._current_frame_start_ptr = regs;

			QUARK_ASSERT(frame_ptr == vm._current_static_frame);
			QUARK_ASSERT(regs == vm._current_frame_start_ptr);
			QUARK_ASSERT(vm.check_invariant());
			break;
		}

		case bc_opcode::k_push_inplace_value: {
			QUARK_ASSERT(vm.check_reg__inplace_value(i._a));
			const auto type = stack._entry_types[vm.get_current_frame_pos() + i._a];
			stack._entries[stack._stack_size] = regs[i._a];
			stack._stack_size++;
			stack._entry_types.push_back(type);
			QUARK_ASSERT(stack.check_invariant());
			break;
		}
		case bc_opcode::k_push_external_value: {
			QUARK_ASSERT(vm.check_reg__external_value(i._a));
			const auto& type = frame_ptr->_symbols._symbols[i._a].second._value_type;
			const auto& new_value_pod = regs[i._a];
			retain_value(backend, new_value_pod, type);
			stack._entries[stack._stack_size] = new_value_pod;
			stack._stack_size++;
			stack._entry_types.push_back(type);
			break;
		}

		//??? Idea: popn specifies *where* we can find a list with the types to pop.
		//??? popn with a = 0 is a NOP ==> never emit these!
		case bc_opcode::k_popn: {
			QUARK_ASSERT(vm.check_invariant());

			const uint32_t n = i._a;
			//const uint32_t extbits = i._b;

			QUARK_ASSERT(stack._stack_size >= n);
			QUARK_ASSERT(n >= 0);
			QUARK_ASSERT(n <= 32);

			int pos = static_cast<int>(stack._stack_size) - 1;
			for(int m = 0 ; m < n ; m++){
				const auto type = stack._entry_types.back();
				release_value_safe(backend, stack._entries[pos], type);
				pos--;
				stack._entry_types.pop_back();
			}
			stack._stack_size -= n;

			QUARK_ASSERT(vm.check_invariant());
			break;
		}


		//////////////////////////////////////////		BRANCHING


		case bc_opcode::k_branch_false_bool: {
			QUARK_ASSERT(vm.check_reg_bool(i._a));

			//	Notice that pc will be incremented too, hence the - 1.
			pc = regs[i._a].bool_value ? pc : pc + i._b - 1;
			break;
		}
		case bc_opcode::k_branch_true_bool: {
			QUARK_ASSERT(vm.check_reg_bool(i._a));

			//	Notice that pc will be incremented too, hence the - 1.
			pc = regs[i._a].bool_value ? pc + i._b - 1: pc;
			break;
		}
		case bc_opcode::k_branch_zero_int: {
			QUARK_ASSERT(vm.check_reg_int(i._a));

			//	Notice that pc will be incremented too, hence the - 1.
			pc = regs[i._a].int_value == 0 ? pc + i._b - 1 : pc;
			break;
		}
		case bc_opcode::k_branch_notzero_int: {
			QUARK_ASSERT(vm.check_reg_int(i._a));

			//	Notice that pc will be incremented too, hence the - 1.
			pc = regs[i._a].int_value == 0 ? pc : pc + i._b - 1;
			break;
		}
		case bc_opcode::k_branch_smaller_int: {
			QUARK_ASSERT(vm.check_reg_int(i._a));
			QUARK_ASSERT(vm.check_reg_int(i._b));

			//	Notice that pc will be incremented too, hence the - 1.
			pc = regs[i._a].int_value < regs[i._b].int_value ? pc + i._c - 1 : pc;
			break;
		}
		case bc_opcode::k_branch_smaller_or_equal_int: {
			QUARK_ASSERT(vm.check_reg_int(i._a));
			QUARK_ASSERT(vm.check_reg_int(i._b));

			//	Notice that pc will be incremented too, hence the - 1.
			pc = regs[i._a].int_value <= regs[i._b].int_value ? pc + i._c - 1 : pc;
			break;
		}
		case bc_opcode::k_branch_always: {
			//	Notice that pc will be incremented too, hence the - 1.
			pc = pc + i._a - 1;
			break;
		}


		//////////////////////////////////////////		COMPLEX


		case bc_opcode::k_get_struct_member: {
			QUARK_ASSERT(vm.check_invariant());
			QUARK_ASSERT(vm.check_reg_any(i._a));
			QUARK_ASSERT(vm.check_reg_struct(i._b));

			const auto& struct_type = frame_ptr->_symbols._symbols[i._b].second._value_type;
			const auto data_ptr = regs[i._b].struct_ptr->get_data_ptr();
			const auto member_index = i._c;
			const auto r = load_struct_member(backend, data_ptr, struct_type, member_index);
			const auto& member_type = r.second;

			release_value_safe(backend, regs[i._a], member_type);
			retain_value(backend, r.first, member_type);
			regs[i._a] = r.first;

			QUARK_ASSERT(vm.check_invariant());
			break;
		}

		case bc_opcode::k_lookup_element_string: {
			QUARK_ASSERT(vm.check_invariant());
			QUARK_ASSERT(vm.check_reg_int(i._a));
			QUARK_ASSERT(vm.check_reg_string(i._b));
			QUARK_ASSERT(vm.check_reg_int(i._c));

			//	??? faster to lookup via char* directly.
			const auto s = from_runtime_string2(backend, regs[i._b]);
			const auto lookup_index = regs[i._c].int_value;
			if(lookup_index < 0 || lookup_index >= s.size()){
				quark::throw_runtime_error("Lookup in string: out of bounds.");
			}
			else{
				const char ch = s[lookup_index];
				regs[i._a].int_value = ch;
			}
			QUARK_ASSERT(vm.check_invariant());
			break;
		}

		//	??? Simple JSON-values should not require ext. null, int, bool, empty object, empty array.
		case bc_opcode::k_lookup_element_json: {
			QUARK_ASSERT(vm.check_invariant());
			QUARK_ASSERT(vm.check_reg_json(i._a));
			QUARK_ASSERT(vm.check_reg_json(i._b));

			// reg c points to different types depending on the runtime-type of the json.
			QUARK_ASSERT(vm.check_reg_any(i._c));

			const auto& parent_json = regs[i._b].json_ptr->get_json();

			if(parent_json.is_object()){
				QUARK_ASSERT(vm.check_reg_string(i._c));

				const auto& lookup_key = from_runtime_string2(backend, regs[i._c]);

				//	get_object_element() throws if key can't be found.
				const auto& value = parent_json.get_object_element(lookup_key);

				//??? no need to create full rt_value_t here! We only need pod.
				const auto value2 = rt_value_t::make_json(backend, value);

				release_value_safe(backend, regs[i._a], value2._type);
				retain_value(backend, value2._pod, value2._type);
				regs[i._a] = value2._pod;
			}
			else if(parent_json.is_array()){
				QUARK_ASSERT(vm.check_reg_int(i._c));

				const auto lookup_index = regs[i._c].int_value;
				if(lookup_index < 0 || lookup_index >= parent_json.get_array_size()){
					quark::throw_runtime_error("Lookup in json array: out of bounds.");
				}
				else{
					const auto& value = parent_json.get_array_n(lookup_index);

					//??? value2 will soon go out of scope - avoid creating rt_value_t all together.
					//??? no need to create full rt_value_t here! We only need pod.
					const auto value2 = rt_value_t::make_json(backend, value);

					release_value_safe(backend, regs[i._a], value2._type);
					retain_value(backend, value2._pod, value2._type);
					regs[i._a] = value2._pod;
				}
			}
			else{
				quark::throw_runtime_error("Lookup using [] on json only works on objects and arrays.");
			}
			QUARK_ASSERT(vm.check_invariant());
			break;
		}

		case bc_opcode::k_lookup_element_vector_w_external_elements:
		case bc_opcode::k_lookup_element_vector_w_inplace_elements:
		{
			QUARK_ASSERT(vm.check_invariant());
			QUARK_ASSERT(vm.check_reg__external_value(i._a));
			QUARK_ASSERT(vm.check_reg_int(i._c));

			const auto& vector_type = frame_ptr->_symbols._symbols[i._b].second._value_type;
			const auto lookup_index = regs[i._c].int_value;
			const auto size = get_vector_size(backend, vector_type, regs[i._b]);
			if(lookup_index < 0 || lookup_index >= size){
				quark::throw_runtime_error("Lookup in vector: out of bounds.");
			}
			else{
				const auto element_type = peek2(types, vector_type).get_vector_element_type(types);
				const auto e = lookup_vector_element(backend, vector_type, regs[i._b], lookup_index);
				write_reg_rc(backend, regs, i._a, element_type, e);
			}
			QUARK_ASSERT(vm.check_invariant());
			break;
		}

		case bc_opcode::k_lookup_element_dict_w_external_values:
		case bc_opcode::k_lookup_element_dict_w_inplace_values:
		{
			QUARK_ASSERT(vm.check_reg__external_value(i._a));
			QUARK_ASSERT(vm.check_reg_dict_w_external_values(i._b));
			QUARK_ASSERT(vm.check_reg_string(i._c));

			const auto& dict_type = frame_ptr->_symbols._symbols[i._b].second._value_type;
			const auto key = regs[i._c];
//				quark::throw_runtime_error("Lookup in dict: key not found.");
			const auto element_type = peek2(types, dict_type).get_dict_value_type(types);
			const auto e = lookup_dict(backend, regs[i._b], dict_type, key);
			write_reg_rc(backend, regs, i._a, element_type, e);
			break;
		}

		case bc_opcode::k_get_size_vector_w_external_elements: {
			QUARK_ASSERT(vm.check_invariant());
			QUARK_ASSERT(vm.check_reg_int(i._a));
			QUARK_ASSERT(vm.check_reg_vector_w_external_elements(i._b));
			QUARK_ASSERT(i._c == 0);

			const auto& type_b = frame_ptr->_symbols._symbols[i._b].second._value_type;
			regs[i._a].int_value = get_vector_size(backend, type_b, regs[i._b]);
			QUARK_ASSERT(vm.check_invariant());
			break;
		}

		case bc_opcode::k_get_size_vector_w_inplace_elements: {
			QUARK_ASSERT(vm.check_invariant());
			QUARK_ASSERT(vm.check_reg_int(i._a));
			QUARK_ASSERT(vm.check_reg_vector_w_inplace_elements(i._b));
			QUARK_ASSERT(i._c == 0);

			const auto& type_b = frame_ptr->_symbols._symbols[i._b].second._value_type;
			regs[i._a].int_value = get_vector_size(backend, type_b, regs[i._b]);
			QUARK_ASSERT(vm.check_invariant());
			break;
		}

		case bc_opcode::k_get_size_dict_w_external_values: {
			QUARK_ASSERT(vm.check_invariant());
			QUARK_ASSERT(vm.check_reg_int(i._a));
			QUARK_ASSERT(vm.check_reg_dict_w_external_values(i._b));
			QUARK_ASSERT(i._c == 0);

			const auto& type_b = frame_ptr->_symbols._symbols[i._b].second._value_type;
			regs[i._a].int_value = get_dict_size(backend, type_b, regs[i._b]);
			QUARK_ASSERT(vm.check_invariant());
			break;
		}
		case bc_opcode::k_get_size_dict_w_inplace_values: {
			QUARK_ASSERT(vm.check_invariant());
			QUARK_ASSERT(vm.check_reg_int(i._a));
			QUARK_ASSERT(vm.check_reg_dict_w_inplace_values(i._b));
			QUARK_ASSERT(i._c == 0);

			const auto& type_b = frame_ptr->_symbols._symbols[i._b].second._value_type;
			regs[i._a].int_value = get_dict_size(backend, type_b, regs[i._b]);
			QUARK_ASSERT(vm.check_invariant());
			break;
		}

		case bc_opcode::k_get_size_string: {
			QUARK_ASSERT(vm.check_invariant());
			QUARK_ASSERT(vm.check_reg_int(i._a));
			QUARK_ASSERT(vm.check_reg_string(i._b));
			QUARK_ASSERT(i._c == 0);

			regs[i._a].int_value = get_vector_size(backend, type_t::make_string(), regs[i._b]);
			QUARK_ASSERT(vm.check_invariant());
			break;
		}
		case bc_opcode::k_get_size_jsonvalue: {
			QUARK_ASSERT(vm.check_invariant());
			QUARK_ASSERT(vm.check_reg_int(i._a));
			QUARK_ASSERT(vm.check_reg_json(i._b));
			QUARK_ASSERT(i._c == 0);

			const auto& json = regs[i._b].json_ptr->get_json();
			if(json.is_object()){
				regs[i._a].int_value = json.get_object_size();
			}
			else if(json.is_array()){
				regs[i._a].int_value = json.get_array_size();
			}
			else if(json.is_string()){
				regs[i._a].int_value = json.get_string().size();
			}
			else{
				quark::throw_runtime_error("Calling size() on unsupported type of value.");
			}
			QUARK_ASSERT(vm.check_invariant());
			break;
		}

		case bc_opcode::k_pushback_vector_w_external_elements: {
			QUARK_ASSERT(vm.check_invariant());
			QUARK_ASSERT(vm.check_reg_vector_w_external_elements(i._a));
			QUARK_ASSERT(vm.check_reg_vector_w_external_elements(i._b));
			QUARK_ASSERT(vm.check_reg__external_value(i._c));

			const auto type_b = frame_ptr->_symbols._symbols[i._b].second._value_type;
			write_reg_rc(backend, regs, i._a, type_b, push_back_vector_element(backend, regs[i._b], type_b, regs[i._c]));

			QUARK_ASSERT(vm.check_invariant());
			break;
		}

		case bc_opcode::k_pushback_vector_w_inplace_elements: {
			QUARK_ASSERT(vm.check_invariant());
			QUARK_ASSERT(vm.check_reg_vector_w_inplace_elements(i._a));
			QUARK_ASSERT(vm.check_reg_vector_w_inplace_elements(i._b));
			QUARK_ASSERT(vm.check_reg(i._c));

			const auto type_b = frame_ptr->_symbols._symbols[i._b].second._value_type;
			write_reg_rc(backend, regs, i._a, type_b, push_back_vector_element(backend, regs[i._b], type_b, regs[i._c]));

			QUARK_ASSERT(vm.check_invariant());
			break;
		}

		case bc_opcode::k_pushback_string: {
			QUARK_ASSERT(vm.check_invariant());
			QUARK_ASSERT(vm.check_reg_string(i._a));
			QUARK_ASSERT(vm.check_reg_string(i._b));
			QUARK_ASSERT(vm.check_reg_int(i._c));

			const auto type_b = frame_ptr->_symbols._symbols[i._b].second._value_type;
			write_reg_rc(backend, regs, i._a, type_b, push_back_vector_element(backend, regs[i._b], type_b, regs[i._c]));

			QUARK_ASSERT(vm.check_invariant());
			break;
		}


		case bc_opcode::k_call: {
			QUARK_ASSERT(vm.check_invariant());
			QUARK_ASSERT(vm.check_reg_function(i._b));

			do_call_instruction(vm, i._a, regs[i._b], i._c);

			frame_ptr = vm._current_static_frame;
			regs = vm._current_frame_start_ptr;

			QUARK_ASSERT(frame_ptr == vm._current_static_frame);
			QUARK_ASSERT(regs == vm._current_frame_start_ptr);
			QUARK_ASSERT(vm.check_invariant());
			break;
		}

		case bc_opcode::k_new_1: {
			QUARK_ASSERT(vm.check_reg(i._a));

			const auto dest_reg = i._a;
			const auto target_itype = i._b;
			const auto source_itype = i._c;
			const auto& target_type = lookup_full_type(vm, target_itype);
			QUARK_ASSERT(
				peek2(types, target_type).is_vector() == false
				&& peek2(types, target_type).is_dict() == false
				&& peek2(types, target_type).is_struct() == false
			);
			execute_new_1(vm, dest_reg, target_itype, source_itype);
			break;
		}

		case bc_opcode::k_new_vector_w_external_elements: {
			QUARK_ASSERT(vm.check_reg_vector_w_external_elements(i._a));
			QUARK_ASSERT(i._b >= 0);
			QUARK_ASSERT(i._c >= 0);

			const auto dest_reg = i._a;
			const auto target_itype = i._b;
			const auto arg_count = i._c;
			const auto& vector_type = lookup_full_type(vm, target_itype);
			QUARK_ASSERT(peek2(types, vector_type).is_vector());

			execute_new_vector_obj(vm, dest_reg, target_itype, arg_count);
			break;
		}

		case bc_opcode::k_new_vector_w_inplace_elements: {
			QUARK_ASSERT(vm.check_invariant());
			QUARK_ASSERT(vm.check_reg_vector_w_inplace_elements(i._a));
			QUARK_ASSERT(i._b == 0);
			QUARK_ASSERT(i._c >= 0);

			const auto dest_reg = i._a;
			const auto target_itype = i._b;
			const auto arg_count = i._c;
			execute_new_vector_obj(vm, dest_reg, target_itype, arg_count);

			QUARK_ASSERT(vm.check_invariant());
			break;
		}

		case bc_opcode::k_new_dict_w_external_values: {
			const auto dest_reg = i._a;
			const auto target_itype = i._b;
			const auto arg_count = i._c;
			const auto& target_type = lookup_full_type(vm, target_itype);
			const auto peek = peek2(types, target_type);
			QUARK_ASSERT(peek.is_dict());
			execute_new_dict_obj(vm, dest_reg, target_itype, arg_count);
			break;
		}
		case bc_opcode::k_new_dict_w_inplace_values: {
			const auto dest_reg = i._a;
			const auto target_itype = i._b;
			const auto arg_count = i._c;
			const auto& target_type = lookup_full_type(vm, target_itype);
			QUARK_ASSERT(peek2(types, target_type).is_dict());
			execute_new_dict_pod64(vm, dest_reg, target_itype, arg_count);
			break;
		}
		case bc_opcode::k_new_struct: {
			const auto dest_reg = i._a;
			const auto target_itype = i._b;
			const auto arg_count = i._c;
			const auto& target_type = lookup_full_type(vm, target_itype);
			QUARK_ASSERT(peek2(types, target_type).is_struct());
			execute_new_struct(vm, dest_reg, target_itype, arg_count);
			break;
		}


		//////////////////////////////		COMPARISON


		case bc_opcode::k_comparison_smaller_or_equal: {
			QUARK_ASSERT(vm.check_reg_bool(i._a));
			QUARK_ASSERT(vm.check_reg_any(i._b));
			QUARK_ASSERT(vm.check_reg_any(i._c));

			const auto& type = frame_ptr->_symbols._symbols[i._b].second._value_type;
			QUARK_ASSERT(peek2(types, type).is_int() == false);

			const auto left = vm.read_register(i._b);
			const auto right = vm.read_register(i._c);
			long diff = compare_value_true_deep(backend, left, right, type);

			regs[i._a].bool_value = diff <= 0;
			break;
		}
		case bc_opcode::k_comparison_smaller_or_equal_int: {
			QUARK_ASSERT(vm.check_reg_bool(i._a));
			QUARK_ASSERT(vm.check_reg_int(i._b));
			QUARK_ASSERT(vm.check_reg_int(i._c));

			regs[i._a].bool_value = regs[i._b].int_value <= regs[i._c].int_value;
			break;
		}

		case bc_opcode::k_comparison_smaller: {
			QUARK_ASSERT(vm.check_reg_bool(i._a));
			QUARK_ASSERT(vm.check_reg_any(i._b));
			QUARK_ASSERT(vm.check_reg_any(i._c));

			const auto& type = frame_ptr->_symbols._symbols[i._b].second._value_type;
			QUARK_ASSERT(peek2(types, type).is_int() == false);
			const auto left = vm.read_register(i._b);
			const auto right = vm.read_register(i._c);
			long diff = compare_value_true_deep(backend, left, right, type);

			regs[i._a].bool_value = diff < 0;
			break;
		}
		case bc_opcode::k_comparison_smaller_int:
			QUARK_ASSERT(vm.check_reg_bool(i._a));
			QUARK_ASSERT(vm.check_reg_int(i._b));
			QUARK_ASSERT(vm.check_reg_int(i._c));

			regs[i._a].bool_value = regs[i._b].int_value < regs[i._c].int_value;
			break;

		case bc_opcode::k_logical_equal: {
			QUARK_ASSERT(vm.check_reg_bool(i._a));
			QUARK_ASSERT(vm.check_reg_any(i._b));
			QUARK_ASSERT(vm.check_reg_any(i._c));

			const auto& type = frame_ptr->_symbols._symbols[i._b].second._value_type;
			QUARK_ASSERT(peek2(types, type).is_int() == false);
			const auto left = vm.read_register(i._b);
			const auto right = vm.read_register(i._c);
			long diff = compare_value_true_deep(backend, left, right, type);

			regs[i._a].bool_value = diff == 0;
			break;
		}
		case bc_opcode::k_logical_equal_int: {
			QUARK_ASSERT(vm.check_reg_bool(i._a));
			QUARK_ASSERT(vm.check_reg_int(i._b));
			QUARK_ASSERT(vm.check_reg_int(i._c));

			regs[i._a].bool_value = regs[i._b].int_value == regs[i._c].int_value;
			break;
		}

		case bc_opcode::k_logical_nonequal: {
			QUARK_ASSERT(vm.check_reg_bool(i._a));
			QUARK_ASSERT(vm.check_reg_any(i._b));
			QUARK_ASSERT(vm.check_reg_any(i._c));

			const auto& type = frame_ptr->_symbols._symbols[i._b].second._value_type;
			QUARK_ASSERT(peek2(types, type).is_int() == false);
			const auto left = vm.read_register(i._b);
			const auto right = vm.read_register(i._c);
			long diff = compare_value_true_deep(backend, left, right, type);

			regs[i._a].bool_value = diff != 0;
			break;
		}
		case bc_opcode::k_logical_nonequal_int: {
			QUARK_ASSERT(vm.check_reg_bool(i._a));
			QUARK_ASSERT(vm.check_reg_int(i._b));
			QUARK_ASSERT(vm.check_reg_int(i._c));

			regs[i._a].bool_value = regs[i._b].int_value != regs[i._c].int_value;
			break;
		}


		//////////////////////////////		ARITHMETICS


		//??? Replace by a | b opcode.
		case bc_opcode::k_add_bool: {
			QUARK_ASSERT(vm.check_reg_bool(i._a));
			QUARK_ASSERT(vm.check_reg_bool(i._b));
			QUARK_ASSERT(vm.check_reg_bool(i._c));

			regs[i._a].bool_value = regs[i._b].bool_value + regs[i._c].bool_value;
			break;
		}
		case bc_opcode::k_add_int: {
			QUARK_ASSERT(vm.check_reg_int(i._a));
			QUARK_ASSERT(vm.check_reg_int(i._b));
			QUARK_ASSERT(vm.check_reg_int(i._c));

			regs[i._a].int_value = regs[i._b].int_value + regs[i._c].int_value;
			break;
		}
		case bc_opcode::k_add_double: {
			QUARK_ASSERT(vm.check_reg_double(i._a));
			QUARK_ASSERT(vm.check_reg_double(i._b));
			QUARK_ASSERT(vm.check_reg_double(i._c));

			regs[i._a].double_value = regs[i._b].double_value + regs[i._c].double_value;
			break;
		}

		case bc_opcode::k_concat_strings: {
			QUARK_ASSERT(vm.check_reg_string(i._a));
			QUARK_ASSERT(vm.check_reg_string(i._b));
			QUARK_ASSERT(vm.check_reg_string(i._c));

			const auto type = type_t::make_string();

			const auto s2 = concat_strings(backend, regs[i._b], regs[i._c]);
			release_value_safe(backend, regs[i._a], type);
			regs[i._a] = s2;
			break;
		}

		case bc_opcode::k_concat_vectors_w_external_elements: {
			QUARK_ASSERT(vm.check_reg_vector_w_external_elements(i._a));
			QUARK_ASSERT(vm.check_reg_vector_w_external_elements(i._b));
			QUARK_ASSERT(vm.check_reg_vector_w_external_elements(i._c));

			const auto& vector_type = frame_ptr->_symbols._symbols[i._a].second._value_type;
			const auto r = concatunate_vectors(backend, vector_type, regs[i._b], regs[i._c]);
			const auto& result = rt_value_t(backend, r, vector_type, rt_value_t::rc_mode::adopt);
			vm.write_register(i._a, result);
			break;
		}

		case bc_opcode::k_subtract_double: {
			QUARK_ASSERT(vm.check_reg_double(i._a));
			QUARK_ASSERT(vm.check_reg_double(i._b));
			QUARK_ASSERT(vm.check_reg_double(i._c));

			regs[i._a].double_value = regs[i._b].double_value - regs[i._c].double_value;
			break;
		}
		case bc_opcode::k_subtract_int: {
			QUARK_ASSERT(vm.check_reg_int(i._a));
			QUARK_ASSERT(vm.check_reg_int(i._b));
			QUARK_ASSERT(vm.check_reg_int(i._c));

			regs[i._a].int_value = regs[i._b].int_value - regs[i._c].int_value;
			break;
		}
		case bc_opcode::k_multiply_double: {
			QUARK_ASSERT(vm.check_reg_double(i._a));
			QUARK_ASSERT(vm.check_reg_double(i._c));
			QUARK_ASSERT(vm.check_reg_double(i._c));

			regs[i._a].double_value = regs[i._b].double_value * regs[i._c].double_value;
			break;
		}
		case bc_opcode::k_multiply_int: {
			QUARK_ASSERT(vm.check_reg_int(i._a));
			QUARK_ASSERT(vm.check_reg_int(i._c));
			QUARK_ASSERT(vm.check_reg_int(i._c));

			regs[i._a].int_value = regs[i._b].int_value * regs[i._c].int_value;
			break;
		}
		case bc_opcode::k_divide_double: {
			QUARK_ASSERT(vm.check_reg_double(i._a));
			QUARK_ASSERT(vm.check_reg_double(i._b));
			QUARK_ASSERT(vm.check_reg_double(i._c));

			const auto right = regs[i._c].double_value;
			if(right == 0.0f){
				quark::throw_runtime_error("EEE_DIVIDE_BY_ZERO");
			}
			regs[i._a].double_value = regs[i._b].double_value / right;
			break;
		}
		case bc_opcode::k_divide_int: {
			QUARK_ASSERT(vm.check_reg_int(i._a));
			QUARK_ASSERT(vm.check_reg_int(i._b));
			QUARK_ASSERT(vm.check_reg_int(i._c));

			const auto right = regs[i._c].int_value;
			if(right == 0.0f){
				quark::throw_runtime_error("EEE_DIVIDE_BY_ZERO");
			}
			regs[i._a].int_value = regs[i._b].int_value / right;
			break;
		}
		case bc_opcode::k_remainder_int: {
			QUARK_ASSERT(vm.check_reg_int(i._a));
			QUARK_ASSERT(vm.check_reg_int(i._b));
			QUARK_ASSERT(vm.check_reg_int(i._c));

			const auto right = regs[i._c].int_value;
			if(right == 0){
				quark::throw_runtime_error("EEE_DIVIDE_BY_ZERO");
			}
			regs[i._a].int_value = regs[i._b].int_value % right;
			break;
		}


		case bc_opcode::k_logical_and_bool: {
			QUARK_ASSERT(vm.check_reg_bool(i._a));
			QUARK_ASSERT(vm.check_reg_bool(i._b));
			QUARK_ASSERT(vm.check_reg_bool(i._c));

			regs[i._a].bool_value = regs[i._b].bool_value  && regs[i._c].bool_value;
			break;
		}
		case bc_opcode::k_logical_and_int: {
			QUARK_ASSERT(vm.check_reg_bool(i._a));
			QUARK_ASSERT(vm.check_reg_int(i._b));
			QUARK_ASSERT(vm.check_reg_int(i._c));

			regs[i._a].bool_value = (regs[i._b].int_value != 0) && (regs[i._c].int_value != 0);
			break;
		}
		case bc_opcode::k_logical_and_double: {
			QUARK_ASSERT(vm.check_reg_bool(i._a));
			QUARK_ASSERT(vm.check_reg_double(i._b));
			QUARK_ASSERT(vm.check_reg_double(i._c));

			regs[i._a].bool_value = (regs[i._b].double_value != 0) && (regs[i._c].double_value != 0);
			break;
		}

		case bc_opcode::k_logical_or_bool: {
			QUARK_ASSERT(vm.check_reg_bool(i._a));
			QUARK_ASSERT(vm.check_reg_bool(i._b));
			QUARK_ASSERT(vm.check_reg_bool(i._c));

			regs[i._a].bool_value = regs[i._b].bool_value || regs[i._c].bool_value;
			break;
		}
		case bc_opcode::k_logical_or_int: {
			QUARK_ASSERT(vm.check_reg_bool(i._a));
			QUARK_ASSERT(vm.check_reg_int(i._b));
			QUARK_ASSERT(vm.check_reg_int(i._c));

			regs[i._a].bool_value = (regs[i._b].int_value != 0) || (regs[i._c].int_value != 0);
			break;
		}
		case bc_opcode::k_logical_or_double: {
			QUARK_ASSERT(vm.check_reg_bool(i._a));
			QUARK_ASSERT(vm.check_reg_double(i._b));
			QUARK_ASSERT(vm.check_reg_double(i._c));

			regs[i._a].bool_value = (regs[i._b].double_value != 0.0f) || (regs[i._c].double_value != 0.0f);
			break;
		}


		//////////////////////////////		NONE


		default:
			quark::throw_defective_request();
		}
		pc++;

		if(k_trace_stepping){
			trace_interpreter(vm, pc);
		}
	}
	return { false, rt_value_t::make_undefined() };
}


//////////////////////////////////////////		FUNCTIONS

	

rt_value_t load_global(interpreter_t& vm, const module_symbol_t& s){
	QUARK_ASSERT(vm.check_invariant());
	QUARK_ASSERT(s.s.size() > 0);

	const auto& symbols = vm._program->_globals._symbols;
    const auto& it = std::find_if(
    	symbols._symbols.begin(),
    	symbols._symbols.end(),
    	[&s](const std::pair<std::string, symbol_t>& e) { return e.first == s.s; }
	);
	if(it != symbols._symbols.end()){
		const auto index = static_cast<int>(it - symbols._symbols.begin());
		const auto pos = get_global_n_pos(index);
		QUARK_ASSERT(pos >= 0 && pos < vm._stack.size());

		const auto r = vm._stack.load_value(pos, it->second._value_type);

        QUARK_ASSERT(r.check_invariant());
        return r;
	}
	else{
		return {};
	}
}

json_t interpreter_to_json(interpreter_t& vm){
	std::vector<json_t> callstack;
	QUARK_ASSERT(vm.check_invariant());

	const auto stack = stack_to_json(vm._stack, vm._backend);

	return json_t::make_object({
		{ "program", bcprogram_to_json(*vm._program) },
		{ "callstack", stack }
	});
}

static json_t types_to_json(const types_t& types, const std::vector<type_t>& types2){
	std::vector<json_t> r;
	int id = 0;
	for(const auto& e: types2){
		const auto i = json_t::make_array({
			id,
			type_to_json(types, e)
		});
		r.push_back(i);
		id++;
	}
	return json_t::make_array(r);
}

}	//	floyd
