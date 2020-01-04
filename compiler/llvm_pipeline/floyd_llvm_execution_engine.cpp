//
//  floyd_llvm_runtime.cpp
//  Floyd
//
//  Created by Marcus Zetterquist on 2019-04-24.
//  Copyright © 2019 Marcus Zetterquist. All rights reserved.
//

static const bool k_trace_function_link_map = false;

#include "floyd_llvm_execution_engine.h"

//??? temp
#include "floyd_llvm_codegen.h"
#include "floyd_llvm_codegen_basics.h"

#include "value_backend.h"
#include "floyd_runtime.h"
#include "value_features.h"
#include "floyd_llvm_runtime_functions.h"
#include "floyd_llvm_intrinsics.h"

//??? temp
#include "floyd_network_component.h"

#include "floyd_corelib.h"
#include "text_parser.h"
#include "os_process.h"
#include "compiler_helpers.h"
#include "format_table.h"
#include "utils.h"

#include <llvm/ExecutionEngine/ExecutionEngine.h>
#include <llvm/ExecutionEngine/GenericValue.h>
#include <llvm/ExecutionEngine/MCJIT.h>
#include <llvm/Support/TargetSelect.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/IR/DataLayout.h>

//#include "llvm/Bitcode/BitstreamWriter.h"

#include <condition_variable>
#include <iostream>

namespace floyd {


////////////////////////////////	BASICS


/*
@variable = global i32 21
define i32 @main() {
%1 = load i32, i32* @variable ; load the global variable
%2 = mul i32 %1, 2
store i32 %2, i32* @variable ; store instruction to write to global variable
ret i32 %2
}
*/


////////////////////////////////	CLIENT ACCESS OF RUNNING PROGRAM


void* get_global_ptr(const llvm_execution_engine_t& ee, const module_symbol_t& name){
	QUARK_ASSERT(ee.check_invariant());
	QUARK_ASSERT(name.s.empty() == false);

	const auto addr = ee.ee->getGlobalValueAddress(name.s);
	return  (void*)addr;
}

static void* get_function_ptr(const llvm_execution_engine_t& ee, const module_symbol_t& name){
	QUARK_ASSERT(ee.check_invariant());
	QUARK_ASSERT(name.s.empty() == false);

	const auto addr = ee.ee->getFunctionAddress(name.s);
	return (void*)addr;
}

std::pair<void*, type_t> bind_global(const llvm_execution_engine_t& ee, const module_symbol_t& name){
	QUARK_ASSERT(ee.check_invariant());
	QUARK_ASSERT(name.s.empty() == false);

	const auto global_ptr = get_global_ptr(ee, name);
	if(global_ptr != nullptr){
		auto symbol = find_symbol(ee.global_symbols, name.s);
		QUARK_ASSERT(symbol != nullptr);
		return { global_ptr, symbol->get_value_type() };
	}
	else{
		return { nullptr, type_t::make_undefined() };
	}
}

static value_t load_via_ptr(const llvm_context_t& c, const void* value_ptr, const type_t& type){
	QUARK_ASSERT(c.check_invariant());
	QUARK_ASSERT(value_ptr != nullptr);
	QUARK_ASSERT(type.check_invariant());

	const auto result = load_via_ptr2(c.ee->backend.types, value_ptr, type);
	const auto result2 = from_runtime_value2(c.ee->backend, result, type);
	return result2;
}

value_t load_global(const llvm_context_t& c, const std::pair<void*, type_t>& v){
	QUARK_ASSERT(c.check_invariant());
	QUARK_ASSERT(v.first != nullptr);
	QUARK_ASSERT(v.second.is_undefined() == false);

	return load_via_ptr(c, v.first, v.second);
}

void store_via_ptr(llvm_context_t& c, const type_t& member_type, void* value_ptr, const value_t& value){
	QUARK_ASSERT(c.check_invariant());
	QUARK_ASSERT(member_type.check_invariant());
	QUARK_ASSERT(value_ptr != nullptr);
	QUARK_ASSERT(value.check_invariant());

	const auto value2 = to_runtime_value2(c.ee->backend, value);
	store_via_ptr2(c.ee->backend.types, value_ptr, member_type, value2);
}


llvm_bind_t bind_function2(llvm_execution_engine_t& ee, const module_symbol_t& name){
	QUARK_ASSERT(ee.check_invariant());
	QUARK_ASSERT(name.s.empty() == false);

	const auto f = get_function_ptr(ee, name);
	if(f != nullptr){
		const auto def = lookup_func_link_by_symbol(ee.backend, name);
		if(def == nullptr){
			quark::throw_defective_request();
		}
		const auto function_type = def->function_type_optional;
		return llvm_bind_t {
			name,
			f,
			function_type
		};
	}
	else{
		return llvm_bind_t {
			name,
			nullptr,
			type_t::make_undefined()
		};
	}
}


////////////////////////////////	INTERNALS FOR EXECUTION ENGINE


static std::vector<func_link_t> make_init_deinit_link_map(llvm::LLVMContext& context, const llvm_type_lookup& type_lookup){
	QUARK_ASSERT(type_lookup.check_invariant());

	std::vector<func_link_t> result;
	auto& types = type_lookup.state.types;

	//	init()
	{
		const auto link_name = module_symbol_t { "init" };
		llvm::FunctionType* function_type = llvm::FunctionType::get(
			llvm::Type::getInt64Ty(context),
			{
				make_frp_type(type_lookup)
			},
			false
		);
		const auto def = func_link_t {
			"runtime",
			link_name,
			type_t::make_undefined(),
			func_link_t::eexecution_model::k_native__ccc,
			nullptr,
			{},
			(native_type_t*)function_type
		};
		result.push_back(def);
	}

	//	deinit()
	{
		const auto link_name = module_symbol_t { "deinit" };
		llvm::FunctionType* function_type = llvm::FunctionType::get(
			llvm::Type::getInt64Ty(context),
			{
				make_frp_type(type_lookup)
			},
			false
		);
		const auto def = func_link_t {
			"runtime",
			link_name,
			type_t::make_undefined(),
			func_link_t::eexecution_model::k_native__ccc,
			nullptr,
			{},
			(native_type_t*)function_type
		};
		result.push_back(def);
	}

	if(k_trace_function_link_map){
		trace_function_link_map(types, result);
	}

	return result;
}


static std::map<std::string, void*> get_corelib_and_network_binds(){
	const std::map<std::string, void*> corelib_binds = get_unified_corelib_binds();
	const auto network_binds = get_network_component_binds();

	std::map<std::string, void*> merge = corelib_binds;
	merge.insert(network_binds.begin(), network_binds.end());

	return merge;
}



//	IMPORTANT: The corelib function function prototypes & types lives in floyd source code, thus is inside ast_function_defs.
static std::vector<func_link_t> make_floyd_code_and_corelib_link_map(llvm::LLVMContext& context, const llvm_type_lookup& type_lookup, const std::vector<floyd::function_definition_t>& ast_function_defs){
	QUARK_ASSERT(type_lookup.check_invariant());

	const auto& types = type_lookup.state.types;

	std::vector<func_link_t> prototypes;

	//	Make llvm_codegen_function_type_t for all functions inside the floyd program (floyd source code).
	//	This includes corelib function prototypes and user's functions.
	//	Only makes prototypes, no implementations.
	{
		for(const auto& function_def: ast_function_defs){
			const auto link_name = module_symbol_t(function_def._definition_name);
			const auto function_type = get_llvm_function_type(type_lookup, function_def._function_type);

			const auto def = func_link_t {
				"program",
				link_name,
				function_def._function_type,
				func_link_t::eexecution_model::k_native__floydcc,
				nullptr,
				get_member_names(function_def._named_args),
				(native_type_t*)function_type
			};
			prototypes.push_back(def);
		}
	}

	std::map<module_symbol_t, void*> binds0;

	// Corelib function binds. The prototypes are provided in input AST already.
	{
		const auto corelib_function_map0 = get_corelib_and_network_binds();
		std::map<module_symbol_t, void*> corelib_function_map;
		for(const auto& e: corelib_function_map0){
			corelib_function_map.insert({ module_symbol_t(e.first), e.second });
		}
		binds0.insert(corelib_function_map.begin(), corelib_function_map.end());
	}

	//	Attempt to resolve prototypes to real native functions, if possible.
	std::vector<func_link_t> result;
	for(const auto& e: prototypes){
		const auto it = binds0.find(e.module_symbol);
		if(it != binds0.end()){
			const auto def2 = set_f(e, it->second);
			result.push_back(def2);
		}
		else{
			result.push_back(e);
		}
	}

	if(k_trace_function_link_map){
		trace_function_link_map(types, result);
	}

	return result;
}

std::vector<func_link_t> make_function_link_map1(llvm::LLVMContext& context, const llvm_type_lookup& type_lookup, const std::vector<floyd::function_definition_t>& ast_function_defs, const intrinsic_signatures_t& intrinsic_signatures){
	QUARK_ASSERT(type_lookup.check_invariant());

	const auto runtime_functions_link_map = make_runtime_function_link_map(context, type_lookup);
	const auto intrinsics_link_map = make_intrinsics_link_map(context, type_lookup, intrinsic_signatures);
	const auto init_deinit_link_map = make_init_deinit_link_map(context, type_lookup);
	const auto floyd_code_and_corelib_link_map = make_floyd_code_and_corelib_link_map(context, type_lookup, ast_function_defs);

	std::vector<func_link_t> acc;
	acc = concat(acc, runtime_functions_link_map);
	acc = concat(acc, intrinsics_link_map);
	acc = concat(acc, init_deinit_link_map);
	acc = concat(acc, floyd_code_and_corelib_link_map);
	return acc;
}



int64_t llvm_call_main(llvm_execution_engine_t& ee, const llvm_bind_t& f, const std::vector<std::string>& main_args){
	QUARK_ASSERT(f.address != nullptr);

	auto context = llvm_context_t { &ee, nullptr };
	auto& types = ee.type_lookup.state.types;
	auto runtime_ptr = make_runtime_ptr(&context);

	//??? Check this earlier.
	if(f.type == get_main_signature_arg_impure(types) || f.type == get_main_signature_arg_pure(types)){
		const auto f2 = reinterpret_cast<FLOYD_RUNTIME_MAIN_ARGS_IMPURE>(f.address);

		//??? Slow path via value_t
		std::vector<value_t> main_args2;
		for(const auto& e: main_args){
			main_args2.push_back(value_t::make_string(e));
		}
		const auto main_args3 = value_t::make_vector_value(types, type_t::make_string(), main_args2);
		const auto main_args4 = to_runtime_value2(context.ee->backend, main_args3);
		const auto main_result_int = (*f2)(&runtime_ptr, main_args4);

		const auto return_itype = type_t::make_vector(types, type_t::make_string());
		release_value(context.ee->backend, main_args4, return_itype);
		return main_result_int;
	}
	else if(f.type == get_main_signature_no_arg_impure(types) || f.type == get_main_signature_no_arg_pure(types)){
		const auto f2 = reinterpret_cast<FLOYD_RUNTIME_MAIN_NO_ARGS_IMPURE>(f.address);
		const auto main_result_int = (*f2)(&runtime_ptr);
		return main_result_int;
	}
	else{
		throw std::exception();
	}
}

#if DEBUG && 1
//	Verify that all global functions can be accessed. If *one* is unresolved, then all return NULL!?
static void check_nulls(llvm_execution_engine_t& ee2, const llvm_ir_program_t& p){
	int index = 0;
	for(const auto& e: p.debug_globals._symbols){
		const auto t = e.second.get_value_type();
		if(peek2(ee2.type_lookup.state.types, t).is_function()){
			const auto global_var = (FLOYD_RUNTIME_HOST_FUNCTION*)floyd::get_global_ptr(ee2, module_symbol_t(e.first));
			if(global_var != nullptr){
				QUARK_ASSERT(global_var != nullptr);

				const auto f = *global_var;
	//				QUARK_ASSERT(f != nullptr);

				const std::string suffix = f == nullptr ? " NULL POINTER" : "";
	//			const uint64_t addr = reinterpret_cast<uint64_t>(f);
	//			QUARK_TRACE_SS(index << " " << e.first << " " << addr << suffix);
			}
			else{
			}
		}
		else{
		}
		index++;
	}
}
#endif

static int64_t floyd_llvm_intrinsic__dummy(runtime_t* frp){
	auto& r = get_backend(frp);
	(void)r;
	quark::throw_runtime_error("Attempting to calling unimplemented function.");
}


////////////////////////////////		llvm_execution_engine_t




llvm_execution_engine_t::~llvm_execution_engine_t(){
	QUARK_ASSERT(check_invariant());
}

bool llvm_execution_engine_t::check_invariant() const {
	QUARK_ASSERT(debug_magic == k_debug_magic);

	QUARK_ASSERT(ee);
	QUARK_ASSERT(backend.check_invariant());
	return true;
}

//	NOTICE: LLVM strips out unused functions = nullptr = not all functions in our link map gets a native function pointer.
static std::vector<func_link_t> resolve_function_ptrs(const types_t& types, llvm::ExecutionEngine& ee, const std::vector<func_link_t>& function_link_map){
	QUARK_ASSERT(types.check_invariant());

	std::vector<func_link_t> result;
	for(const auto& e: function_link_map){
		if(e.function_type_optional.is_function()){
			const auto f = (void*)ee.getFunctionAddress(e.module_symbol.s);
			result.push_back(set_f(e, f));
		}
	}

	if(false) trace_function_link_map(types, result);

/*
	if(false){
		QUARK_SCOPED_TRACE("linked functions");
		for(const auto& e: result){
			QUARK_TRACE_SS(e.first.s << " = " << (e.second == nullptr ? "nullptr" : ptr_to_hexstring(e.second)));
		}
	}
*/
	return result;
}


//??? LLVM codegen unlinks functions not called: need to mark functions external.


static std::vector<std::pair<type_t, struct_layout_t>> make_struct_layouts(const llvm_type_lookup& type_lookup, const llvm::DataLayout& data_layout){
	QUARK_ASSERT(type_lookup.check_invariant());

	const auto& types = type_lookup.state.types;
	std::vector<std::pair<type_t, struct_layout_t>> result;

	for(int i = 0 ; i < type_lookup.state.type_entries.size() ; i++){
		const auto& type = lookup_type_from_index(types, i);
		const auto peek_type = peek2(types, type);
		if(peek_type.is_struct() && is_fully_defined(types, peek_type)){
			auto t2 = get_exact_struct_type_byvalue(type_lookup, peek_type);
			const llvm::StructLayout* layout = data_layout.getStructLayout(t2);


			const auto& source_struct_def = peek_type.get_struct(types);

			const auto struct_bytes = layout->getSizeInBytes();
			std::vector<member_info_t> member_infos;
			for(int member_index = 0 ; member_index < source_struct_def._members.size() ; member_index++){
				const auto& member = source_struct_def._members[member_index];
				const auto offset = layout->getElementOffset(member_index);
				member_infos.push_back(member_info_t { offset, member._type } );
			}

			result.push_back( { type, struct_layout_t{ member_infos, struct_bytes } } );
		}
	}
	return result;
}

#if QUARK_MAC
static std::string strip_link_name(const std::string& s){
	QUARK_ASSERT(s.empty() == false);
	QUARK_ASSERT(s[0] == '_');
	const auto s2 = s.substr(1);
	return s2;
}
#else
static std::string strip_link_name(const std::string& platform_link_name){
	QUARK_ASSERT(platform_link_name.empty() == false);
	return platform_link_name;
}
#endif



//	Destroys program_breaks, can only called once!
static std::unique_ptr<llvm_execution_engine_t> make_engine_no_init(llvm_instance_t& instance, llvm_ir_program_t& program_breaks, runtime_handler_i& runtime_handler, bool trace_processes){
	QUARK_ASSERT(instance.check_invariant());
	QUARK_ASSERT(program_breaks.check_invariant());

	if(k_trace_function_link_map){
		const auto& types = program_breaks.type_lookup.state.types;
		trace_function_link_map(types, program_breaks.function_link_map);
	}

	std::string collectedErrors;

	//	WARNING: Destroys p -- uses std::move().
	llvm::ExecutionEngine* exeEng = llvm::EngineBuilder(std::move(program_breaks.module))
		.setErrorStr(&collectedErrors)
		.setOptLevel(llvm::CodeGenOpt::Level::None)
		.setVerifyModules(true)
		.setEngineKind(llvm::EngineKind::JIT)
		.create();

	if (exeEng == nullptr){
		std::string error = "Unable to construct execution engine: " + collectedErrors;
		perror(error.c_str());
		throw std::exception();
	}
	QUARK_ASSERT(collectedErrors.empty());

	const auto start_time = std::chrono::high_resolution_clock::now();

	auto ee1 = std::shared_ptr<llvm::ExecutionEngine>(exeEng);

	//	LINK. Resolve all unresolved functions.
	{
		//	https://stackoverflow.com/questions/33328562/add-mapping-to-c-lambda-from-llvm
		auto lambda = [&](const std::string& s) -> void* {
			const auto s2 = strip_link_name(s);

			const auto& function_link_map = program_breaks.function_link_map;
			const auto found = lookup_func_link_by_symbol(function_link_map, module_symbol_t(s2));
			if(found != nullptr && found->f != nullptr){
				return found->f;
			}
			else {
				return (void*)&floyd_llvm_intrinsic__dummy;
			}
		};
		std::function<void*(const std::string&)> on_lazy_function_creator2 = lambda;

		//	NOTICE! Patch during finalizeObject() only, then restore!
		ee1->InstallLazyFunctionCreator(on_lazy_function_creator2);
		ee1->finalizeObject();
		ee1->InstallLazyFunctionCreator(nullptr);

	//	ee2.ee->DisableGVCompilation(false);
	//	ee2.ee->DisableSymbolSearching(false);
	}

	std::vector<func_link_t> final_link_map = resolve_function_ptrs(program_breaks.type_lookup.state.types, *ee1, program_breaks.function_link_map);

	auto ee2 = std::unique_ptr<llvm_execution_engine_t>(
		new llvm_execution_engine_t {
			k_debug_magic,
			value_backend_t(
				final_link_map,
				make_struct_layouts(program_breaks.type_lookup, ee1->getDataLayout()),
				program_breaks.type_lookup.state.types,
				program_breaks.settings.config
			),
			program_breaks.type_lookup,
			program_breaks.container_def,
			&instance,
			ee1,
			program_breaks.debug_globals,
			route_t(&runtime_handler, nullptr),
			start_time,
			llvm_bind_t{ k_no_module_symbol, nullptr, type_t::make_undefined() },
			false,
			program_breaks.settings.config,

			{},
			std::this_thread::get_id(),
			{},
			{},
			trace_processes
		}
	);
	ee2->_handler_router._symbol_table = &ee2->global_symbols;
	QUARK_ASSERT(ee2->check_invariant());

#if DEBUG
	check_nulls(*ee2, program_breaks);
#endif

	if(k_trace_function_link_map){
		const auto& types = program_breaks.type_lookup.state.types;
		trace_function_link_map(types, ee2->backend.func_link_lookup);
	}

	return ee2;
}

//	Destroys program_breaks, can only run it once!
//	Also runs floyd_runtime_init() - to execute Floyd's global functions and initialize global constants.
std::unique_ptr<llvm_execution_engine_t> init_llvm_jit(llvm_ir_program_t& program_breaks, runtime_handler_i& handler, bool trace_processes){
	QUARK_ASSERT(program_breaks.check_invariant());

	auto ee = make_engine_no_init(*program_breaks.instance, program_breaks, handler, trace_processes);
	auto context = llvm_context_t{ ee.get(), nullptr };

//	trace_heap(ee->backend.heap);

	//	Make sure linking went well - test that by trying to resolve a function we know exists.
#if DEBUG
	{
		{
			const auto print_global_ptr_ptr = (FLOYD_RUNTIME_HOST_FUNCTION*)floyd::get_global_ptr(*context.ee, module_symbol_t("init"));
			QUARK_ASSERT(print_global_ptr_ptr != nullptr);
			const auto print_ptr = *print_global_ptr_ptr;
			QUARK_ASSERT(print_ptr != nullptr);
		}
	}
#endif

	ee->main_function = bind_function2(*ee, module_symbol_t("main"));

	auto a_func = reinterpret_cast<FLOYD_RUNTIME_INIT>(get_function_ptr(*context.ee, module_symbol_t("init")));
	QUARK_ASSERT(a_func != nullptr);

	auto runtime_ptr = make_runtime_ptr(&context);

	int64_t init_result = (*a_func)(&runtime_ptr);
	QUARK_ASSERT(init_result == 667);

	QUARK_ASSERT(init_result == 667);
	ee->inited = true;
	return ee;
}

/*
	Try using C++ multithreading maps etc?
	Explore std::packaged_task
*/
//	https://en.cppreference.com/w/cpp/thread/condition_variable/wait



void llvm_process_t::runtime_basics__on_print(const std::string& s){
	//??? needs mutex.
	ee->_handler_router._runtime_handler->on_print(s);
}

type_t llvm_process_t::runtime_basics__get_global_symbol_type(const std::string& s){
	return find_symbol_required(ee->global_symbols, s)._value_type;
}

rt_value_t llvm_process_t::runtime_basics__call_thunk(const rt_value_t& f, const rt_value_t args[], int arg_count){
	quark::throw_defective_request();
}


static std::string make_trace_process_header(const llvm_process_t& process){
	const auto process_name = process._name_key;
	const auto os_thread_name = get_current_thread_name();

//	const auto header = std::string("[") + process_name + ", OS thread: " + os_thread_name + " ]";
	const auto header = std::string("[") + process_name + "]";
	return header;
}


static void send(llvm_process_t& process, const std::string& dest_process_id, const rt_pod_t& message, const type_t& message_type){
	QUARK_ASSERT(process.check_invariant());
	auto& backend = process.ee->backend;
	const auto& types = backend.types;

	const auto trace = process.ee->_trace_processes;
	const auto trace_header = trace ? make_trace_process_header(process) : "";
	QUARK_SCOPED_TRACE_OPTIONAL(trace_header + ": Send message to dest process ID: " + dest_process_id, trace);

	if(trace){
		QUARK_SCOPED_TRACE("Message:");
		const auto v = from_runtime_value2(process.ee->backend, message, message_type);
		const auto message2 = value_to_json(types, v);
		QUARK_TRACE_SS(json_to_pretty_string(message2));
	}

	const auto dest_process_it = std::find_if(
		process.ee->_processes.begin(),
		process.ee->_processes.end(),
		[&](const std::shared_ptr<llvm_process_t>& process){ return process->_name_key == dest_process_id; }
	);
	if(dest_process_it == process.ee->_processes.end()){
		if(trace){
			QUARK_TRACE("Cannot find floyd process: \"" + dest_process_id + "\", message not sent.");
		}
	}
	else{
		auto& dest_process = **dest_process_it;

		if(message_type != dest_process._message_type){
			const auto send_message_type_str = type_to_compact_string(
				types,
				message_type,
				enamed_type_mode::short_names
			);

			const auto msg_message_type_str = type_to_compact_string(
				types,
				dest_process._message_type,
				enamed_type_mode::short_names
			);

			quark::throw_runtime_error(
				"[Floyd runtime] Message type to send() is <" + send_message_type_str + ">"
				+ " but ___msg() requires message type <" + msg_message_type_str + ">"
				+ "."
			);
		}

		{
			rt_value_t message2(backend, message, message_type, rt_value_t::rc_mode::bump);
			std::lock_guard<std::mutex> lk(dest_process._inbox_mutex);
			dest_process._inbox.push_front(message2);
		}
		dest_process._inbox_condition_variable.notify_one();
	//    dest_process._inbox_condition_variable.notify_all();
	}
}

void llvm_process_t::runtime_process__on_send_message(const std::string& dest_process_id, const rt_pod_t& message, const type_t& message_type){
	return send(*this, dest_process_id, message, message_type);
}

void llvm_process_t::runtime_process__on_exit_process(){
	_exiting_flag = true;
}

static void run_process(llvm_execution_engine_t& ee, int process_id){
	auto& backend = ee.backend;
	auto& types = backend.types;

	auto& process = *ee._processes[process_id];

	const auto trace = ee._trace_processes;
	const auto trace_header = trace ? make_trace_process_header(process) : "";

	auto context = llvm_context_t{ &ee, &process };
	auto runtime_ptr = make_runtime_ptr(&context);

	//	Call process init()
	{
		auto f = reinterpret_cast<FLOYD_RUNTIME_PROCESS_INIT>(process._init_function->address);
		const auto init_state = (*f)(&runtime_ptr);
		process._process_state = rt_value_t(backend, init_state, process._state_type, rt_value_t::rc_mode::adopt);
	}

	//	Handle process messages until exit.
	while(process._exiting_flag == false){
		//	Block until we get a message
		rt_value_t message;
		{
			std::unique_lock<std::mutex> lk(process._inbox_mutex);

			if(trace){
        		QUARK_TRACE_SS(trace_header << ": waiting for message");
			}
			process._inbox_condition_variable.wait(lk, [&]{ return process._inbox.empty() == false; });

			//	Pop message.
			QUARK_ASSERT(process._inbox.empty() == false);
			auto temp = process._inbox.back();
			process._inbox.pop_back();

			temp.swap(message);
		}

		//	Handle message.
		QUARK_SCOPED_TRACE_OPTIONAL(trace_header + ": Received message, calling floyd process f()", trace);

		if(trace){
			{
				QUARK_SCOPED_TRACE("Input message");
				const auto v = from_runtime_value2(backend, message._pod, message._type);
				const auto message2 = value_to_json(types, v);
				QUARK_TRACE_SS(json_to_pretty_string(message2));
			}
			{
				QUARK_SCOPED_TRACE("Input state");
				const auto s = value_to_json(types, rt_to_value(backend, process._process_state));
				QUARK_TRACE_SS(json_to_pretty_string(s));
			}
		}

		rt_value_t state2;

		{
			QUARK_SCOPED_TRACE_OPTIONAL("Call msg handler", trace);
			auto f = reinterpret_cast<FLOYD_RUNTIME_PROCESS_MESSAGE>(process._msg_function->address);
			auto s1 = (*f)(&runtime_ptr, process._process_state._pod, message._pod);
			auto s2 = rt_value_t(backend, s1, process._state_type, rt_value_t::rc_mode::adopt);
			state2.swap(s2);
		}

		process._process_state = state2;

		if(trace){
			QUARK_SCOPED_TRACE("Output state");
			const auto s = value_to_json(types, rt_to_value(backend, process._process_state));
			QUARK_TRACE_SS(json_to_pretty_string(s));
		}
	}
}


/*
	There is only one LLVM execution engine, shared by all Floyd processes.
	When client code calls  send() or print() we need to figure out which Floyd process they call from.
*/
static std::map<std::string, value_t> run_processes(llvm_execution_engine_t& ee){
	if(ee.container_def._clock_busses.empty() == true){
		return {};
	}
	else{
		ee._process_infos = reduce(
			ee.container_def._clock_busses,
			std::map<std::string, process_def_t>(), [](const std::map<std::string, process_def_t>& acc, const std::pair<std::string, clock_bus_t>& e){
				auto acc2 = acc;
				acc2.insert(e.second._processes.begin(), e.second._processes.end());
				return acc2;
			}
		);

		for(const auto& t: ee._process_infos){
			auto process = std::make_shared<llvm_process_t>();
			process->ee = &ee;
			process->_name_key = t.first;
			process->_message_type = t.second.msg_type;
			process->_state_type = t.second.state_type;
			process->_exiting_flag = false;
			process->_process_def = t.second;
			process->_init_function = std::make_shared<llvm_bind_t>(
				bind_function2(ee, t.second.init_func_linkname)
			);
			process->_msg_function = std::make_shared<llvm_bind_t>( 
				bind_function2(ee, t.second.msg_func_linkname)
			);

			if(process->_init_function->address == nullptr){
				throw std::runtime_error("Cannot link floyd init function: \"" + t.second.init_func_linkname.s + "\"");
			}
			if(process->_msg_function->address == nullptr){
				throw std::runtime_error("Cannot link floyd message function: \"" + t.second.msg_func_linkname.s + "\"");
			}


			ee._processes.push_back(process);
		}

		//	Start all Floyd processes, each in their own OS thread.
		//	Remember that current thread (main) is also a thread, no need to create a worker thread for one process.
		{
			ee._processes[0]->_thread_id = ee._main_thread_id;
			for(int process_id = 1 ; process_id < ee._processes.size() ; process_id++){
				ee._worker_threads.push_back(
					std::thread([&](int process_id){
			//			const auto native_thread = thread::native_handle();

						std::stringstream thread_name;
						thread_name << std::string() << "floyd process " << process_id << " thread";
						set_current_threads_name(thread_name.str());

						run_process(ee, process_id);
					},
					process_id)
				);
			}
			run_process(ee, 0);
		}

		for(auto &t: ee._worker_threads){
			t.join();
		}

		//	Delete the processes so we free up their resources and RCs
		ee._processes.clear();
		return {};
	}
}

run_output_t run_program(llvm_execution_engine_t& ee, const std::vector<std::string>& main_args){
	if(ee.main_function.address != nullptr){
		const auto main_result_int = llvm_call_main(ee, ee.main_function, main_args);
		return { main_result_int, {} };
	}
	else{
		const auto result = run_processes(ee);
		return { 0, result };
	}
}

void deinit_program(llvm_execution_engine_t& ee){
	if(ee.inited){
		auto f = reinterpret_cast<FLOYD_RUNTIME_INIT>(get_function_ptr(ee, module_symbol_t("deinit")));
		QUARK_ASSERT(f != nullptr);

		auto context = llvm_context_t { &ee, nullptr };
		auto runtime_ptr = make_runtime_ptr(&context);
		int64_t result = (*f)(&runtime_ptr);
		QUARK_ASSERT(result == 668);
		ee.inited = false;
	};
}


runtime_t make_runtime_ptr(llvm_context_t* p){
	QUARK_ASSERT(p != nullptr && p->check_invariant());

	if(p->process){
		return {
			&p->ee->backend,
			p->process,
			p->process
		};
	}
	else{
		return {
			&p->ee->backend,
			&p->ee->_handler_router,
			&p->ee->_handler_router
		};
	}
}



////////////////////////////////		BENCHMARKS


std::vector<bench_t> collect_benchmarks(llvm_execution_engine_t& ee){
	auto context = llvm_context_t { &ee, nullptr };

	std::pair<void*, type_t> benchmark_registry_bind = bind_global(ee, module_symbol_t(k_global_benchmark_registry));
	QUARK_ASSERT(benchmark_registry_bind.first != nullptr);

	const value_t reg = load_global(context, benchmark_registry_bind);
	const auto v = reg.get_vector_value();

	std::vector<bench_t> result;
	for(const auto& e: v){
		const auto s = e.get_struct_value();
		const auto name = s->_member_values[0].get_string_value();
		const auto f_link_name = s->_member_values[1].get_function_value();
		result.push_back(bench_t{ benchmark_id_t { "", name }, f_link_name });
	}
	return result;
}

std::vector<benchmark_result2_t> run_benchmarks(llvm_execution_engine_t& ee, const std::vector<bench_t>& tests){
	QUARK_ASSERT(ee.check_invariant());

	const auto& types = ee.backend.types;
	auto context = llvm_context_t { &ee, nullptr };

	const auto benchmark_result_type = find_symbol_required(ee.global_symbols, "benchmark_result_t")._value_type;
	const auto benchmark_result_vec_type = type_t::make_vector(types, benchmark_result_type);

	std::vector<benchmark_result2_t> result;
	for(const auto& b: tests){
		const auto name = b.benchmark_id.test;
		const auto f_link_name = b.f;

		const auto f_bind = bind_function2(ee, f_link_name);
		QUARK_ASSERT(f_bind.address != nullptr);
		auto f2 = reinterpret_cast<FLOYD_BENCHMARK_F>(f_bind.address);

		auto runtime_ptr = make_runtime_ptr(&context);
		const auto bench_result = (*f2)(&runtime_ptr);
		const auto result2 = from_runtime_value2(context.ee->backend, bench_result, benchmark_result_vec_type);

//			QUARK_TRACE(value_and_type_to_string(result2));

		std::vector<benchmark_result2_t> test_result;
		const auto& vec_result = result2.get_vector_value();
		for(const auto& m: vec_result){
			const auto& struct_result = m.get_struct_value();
			const auto result3 = benchmark_result_t {
				struct_result->_member_values[0].get_int_value(),
				struct_result->_member_values[1].get_json()
			};
			const auto x = benchmark_result2_t { b.benchmark_id, result3 };
			test_result.push_back(x);
		}
		result = concat(result, test_result);
	}

	return result;
}



////////////////////////////////		TESTS



std::vector<test_t> collect_tests(llvm_execution_engine_t& ee){
	auto context = llvm_context_t { &ee, nullptr };

	std::pair<void*, type_t> test_registry_bind = bind_global(ee, module_symbol_t(k_global_test_registry));
	QUARK_ASSERT(test_registry_bind.first != nullptr);

	const value_t reg = load_global(context, test_registry_bind);
	const auto v = reg.get_vector_value();
	return unpack_test_registry(v);
}


}	//	namespace floyd
