//
//  floyd_llvm_runtime.cpp
//  Floyd
//
//  Created by Marcus Zetterquist on 2019-04-24.
//  Copyright © 2019 Marcus Zetterquist. All rights reserved.
//

static const bool k_trace_function_link_map = false;

#include "floyd_llvm_runtime.h"

#include "floyd_llvm_codegen.h"
#include "floyd_runtime.h"
#include "floyd_llvm_corelib.h"
#include "value_features.h"
#include "floyd_llvm_runtime_functions.h"
#include "floyd_llvm_intrinsics.h"

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


#include <thread>
#include <deque>
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


floyd_runtime_t* make_runtime_ptr(llvm_execution_engine_t* ee){
	return reinterpret_cast<floyd_runtime_t*>(ee);
}


////////////////////////////////	CLIENT ACCESS OF RUNNING PROGRAM


const function_link_entry_t& find_function_def_from_link_name(const std::vector<function_link_entry_t>& function_link_map, const link_name_t& link_name){
	auto it = std::find_if(function_link_map.begin(), function_link_map.end(), [&] (const function_link_entry_t& e) { return e.link_name == link_name; } );
	QUARK_ASSERT(it != function_link_map.end());

	QUARK_ASSERT(it->llvm_codegen_f != nullptr);
	return *it;
}

void* get_global_ptr(const llvm_execution_engine_t& ee, const std::string& name){
	QUARK_ASSERT(ee.check_invariant());
	QUARK_ASSERT(name.empty() == false);

	const auto addr = ee.ee->getGlobalValueAddress(name);
	return  (void*)addr;
}

static void* get_function_ptr(const llvm_execution_engine_t& ee, const link_name_t& name){
	QUARK_ASSERT(ee.check_invariant());
	QUARK_ASSERT(name.s.empty() == false);

	const auto addr = ee.ee->getFunctionAddress(name.s);
	return (void*)addr;
}


std::pair<void*, type_t> bind_global(const llvm_execution_engine_t& ee, const std::string& name){
	QUARK_ASSERT(ee.check_invariant());
	QUARK_ASSERT(name.empty() == false);

	const auto global_ptr = get_global_ptr(ee, name);
	if(global_ptr != nullptr){
		auto symbol = find_symbol(ee.global_symbols, name);
		QUARK_ASSERT(symbol != nullptr);
		return { global_ptr, symbol->get_value_type() };
	}
	else{
		return { nullptr, make_undefined() };
	}
}

static value_t load_via_ptr(const llvm_execution_engine_t& runtime, const void* value_ptr, const type_t& type){
	QUARK_ASSERT(runtime.check_invariant());
	QUARK_ASSERT(value_ptr != nullptr);
	QUARK_ASSERT(type.check_invariant());

	const auto result = load_via_ptr2(runtime.backend.types, value_ptr, type);
	const auto result2 = from_runtime_value(runtime, result, type);
	return result2;
}

value_t load_global(const llvm_execution_engine_t& ee, const std::pair<void*, type_t>& v){
	QUARK_ASSERT(v.first != nullptr);
	QUARK_ASSERT(v.second.is_undefined() == false);

	return load_via_ptr(ee, v.first, v.second);
}

void store_via_ptr(llvm_execution_engine_t& runtime, const type_t& member_type, void* value_ptr, const value_t& value){
	QUARK_ASSERT(runtime.check_invariant());
	QUARK_ASSERT(member_type.check_invariant());
	QUARK_ASSERT(value_ptr != nullptr);
	QUARK_ASSERT(value.check_invariant());

	const auto value2 = to_runtime_value(runtime, value);
	store_via_ptr2(runtime.backend.types, value_ptr, member_type, value2);
}


llvm_bind_t bind_function2(llvm_execution_engine_t& ee, const link_name_t& name){
	QUARK_ASSERT(ee.check_invariant());
	QUARK_ASSERT(name.s.empty() == false);

	const auto f = get_function_ptr(ee, name);
	if(f != nullptr){
		const auto def = find_function_def_from_link_name(ee.function_link_map, name);
		const auto function_type = def.function_type_or_undef;
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
			make_undefined()
		};
	}
}


////////////////////////////////	INTERNALS FOR EXECUTION ENGINE





//	Make link entries for all runtime functions, like floydrt_retain_vec().
//	These have no floyd-style function type, only llvm function type, since they use parameters not expressable with type_t.
static std::vector<function_link_entry_t> make_runtime_function_link_map(llvm::LLVMContext& context, const llvm_type_lookup& type_lookup){
	QUARK_ASSERT(type_lookup.check_invariant());

	const auto runtime_function_binds = get_runtime_function_binds(context, type_lookup);
	auto& types = type_lookup.state.types;

	std::vector<function_link_entry_t> result;
	for(const auto& e: runtime_function_binds){
		const auto link_name = encode_runtime_func_link_name(e.name);
		const auto def = function_link_entry_t{ "runtime", link_name, e.llvm_function_type, nullptr, make_undefined(), {}, e.native_f };
		result.push_back(def);
	}

	if(k_trace_function_link_map){
		trace_function_link_map(types, result);
	}

	return result;
}

static std::vector<function_link_entry_t> make_init_deinit_link_map(llvm::LLVMContext& context, const llvm_type_lookup& type_lookup){
	QUARK_ASSERT(type_lookup.check_invariant());

	std::vector<function_link_entry_t> result;
	auto& types = type_lookup.state.types;

	//	init()
	{
		const auto link_name = encode_runtime_func_link_name("init");
		llvm::FunctionType* function_type = llvm::FunctionType::get(
			llvm::Type::getInt64Ty(context),
			{
				make_frp_type(type_lookup)
			},
			false
		);
		const auto def = function_link_entry_t{ "runtime", link_name, function_type, nullptr, make_undefined(), {}, nullptr };
		result.push_back(def);
	}

	//	deinit()
	{
		const auto link_name = encode_runtime_func_link_name("deinit");
		llvm::FunctionType* function_type = llvm::FunctionType::get(
			llvm::Type::getInt64Ty(context),
			{
				make_frp_type(type_lookup)
			},
			false
		);
		const auto def = function_link_entry_t{ "runtime", link_name, function_type, nullptr, make_undefined(), {}, nullptr };
		result.push_back(def);
	}

	if(k_trace_function_link_map){
		trace_function_link_map(types, result);
	}

	return result;
}

//	IMPORTANT: The corelib function function prototypes & types lives in floyd source code, thus is inside ast_function_defs.
static std::vector<function_link_entry_t> make_floyd_code_and_corelib_link_map(llvm::LLVMContext& context, const llvm_type_lookup& type_lookup, const std::vector<floyd::function_definition_t>& ast_function_defs){
	QUARK_ASSERT(type_lookup.check_invariant());

	std::vector<function_link_entry_t> result0;
	std::map<link_name_t, void*> binds0;
	const auto& types = type_lookup.state.types;

	//	Make function def for all functions inside the floyd program (floyd source code).
	{
		for(const auto& function_def: ast_function_defs){
			const auto link_name = encode_floyd_func_link_name(function_def._definition_name);
			const auto function_type = function_def._function_type;
			llvm::Type* function_ptr_type = get_llvm_type_as_arg(type_lookup, function_type);
			const auto function_byvalue_type = deref_ptr(function_ptr_type);
			const auto def = function_link_entry_t{ "program", link_name, (llvm::FunctionType*)function_byvalue_type, nullptr, function_type, function_def._named_args, nullptr };
			result0.push_back(def);
		}
	}

	////////	Corelib
	{
		const auto corelib_function_map0 = get_corelib_binds();
		std::map<link_name_t, void*> corelib_function_map;
		for(const auto& e: corelib_function_map0){
			corelib_function_map.insert({ encode_floyd_func_link_name(e.first), e.second });
		}
		binds0.insert(corelib_function_map.begin(), corelib_function_map.end());
	}

	std::vector<function_link_entry_t> result;
	for(const auto& e: result0){
		const auto it = binds0.find(e.link_name);
		if(it != binds0.end()){
			const auto def2 = function_link_entry_t{ e.module, e.link_name, e.llvm_function_type, e.llvm_codegen_f, e.function_type_or_undef, e.arg_names_or_empty, it->second };
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



std::vector<function_link_entry_t> make_function_link_map1(llvm::LLVMContext& context, const llvm_type_lookup& type_lookup, const std::vector<floyd::function_definition_t>& ast_function_defs, const intrinsic_signatures_t& intrinsic_signatures){
	QUARK_ASSERT(type_lookup.check_invariant());

	const auto runtime_functions_link_map = make_runtime_function_link_map(context, type_lookup);
	const auto intrinsics_link_map = make_intrinsics_link_map(context, type_lookup, intrinsic_signatures);
	const auto init_deinit_link_map = make_init_deinit_link_map(context, type_lookup);
	const auto floyd_code_and_corelib_link_map = make_floyd_code_and_corelib_link_map(context, type_lookup, ast_function_defs);

	std::vector<function_link_entry_t> acc;
	acc = concat(acc, runtime_functions_link_map);
	acc = concat(acc, intrinsics_link_map);
	acc = concat(acc, init_deinit_link_map);
	acc = concat(acc, floyd_code_and_corelib_link_map);
	return acc;
}


void trace_function_link_map(const types_t& types, const std::vector<function_link_entry_t>& defs){
	QUARK_SCOPED_TRACE("FUNCTION LINK MAP");

	std::vector<std::vector<std::string>> matrix;
	for(const auto& e: defs){
		const auto f0 = e.function_type_or_undef.is_undefined() ? "" : json_to_compact_string(type_to_compact_string(types, e.function_type_or_undef));

		std::string arg_names;
		for(const auto& m: e.arg_names_or_empty){
			arg_names = m._name + ",";
		}
		arg_names = arg_names.empty() ? "" : arg_names.substr(0, arg_names.size() - 1);


		const auto f1 = f0.substr(0, 100);
		const auto f = f1.size() != f0.size() ? (f1 + "...") : f1;

		const auto line = std::vector<std::string>{
			e.link_name.s,
			e.module,
			print_type(e.llvm_function_type),
			e.llvm_codegen_f != nullptr ? ptr_to_hexstring(e.llvm_codegen_f) : "",
			f,
			arg_names,
			e.native_f != nullptr ? ptr_to_hexstring(e.native_f) : "",
		};
		matrix.push_back(line);
	}

	const auto result = generate_table_type1(
		{ "LINK-NAME", "MODULE", "LLVM_FUNCTION_TYPE", "LLVM_CODEGEN_F", "FUNCTION TYPE", "ARG NAMES", "NATIVE_F" },
		matrix
	);
	QUARK_TRACE(result);
}





int64_t llvm_call_main(llvm_execution_engine_t& ee, const llvm_bind_t& f, const std::vector<std::string>& main_args){
	QUARK_ASSERT(f.address != nullptr);

	auto& types = ee.type_lookup.state.types;

	//??? Check this earlier.
	if(f.type == get_main_signature_arg_impure(types) || f.type == get_main_signature_arg_pure(types)){
		const auto f2 = reinterpret_cast<FLOYD_RUNTIME_MAIN_ARGS_IMPURE>(f.address);

		//??? Slow path via value_t
		std::vector<value_t> main_args2;
		for(const auto& e: main_args){
			main_args2.push_back(value_t::make_string(e));
		}
		const auto main_args3 = value_t::make_vector_value(types, type_t::make_string(), main_args2);
		const auto main_args4 = to_runtime_value(ee, main_args3);
		const auto main_result_int = (*f2)(make_runtime_ptr(&ee), main_args4);

		const auto return_itype = make_vector(types, type_t::make_string());
		if(is_rc_value(peek2(types, return_itype))){
			release_value(ee.backend, main_args4, return_itype);
		}
		return main_result_int;
	}
	else if(f.type == get_main_signature_no_arg_impure(types) || f.type == get_main_signature_no_arg_pure(types)){
		const auto f2 = reinterpret_cast<FLOYD_RUNTIME_MAIN_NO_ARGS_IMPURE>(f.address);
		const auto main_result_int = (*f2)(make_runtime_ptr(&ee));
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
			const auto global_var = (FLOYD_RUNTIME_HOST_FUNCTION*)floyd::get_global_ptr(ee2, e.first);
			QUARK_ASSERT(global_var != nullptr);

			const auto f = *global_var;
//				QUARK_ASSERT(f != nullptr);

			const std::string suffix = f == nullptr ? " NULL POINTER" : "";
//			const uint64_t addr = reinterpret_cast<uint64_t>(f);
//			QUARK_TRACE_SS(index << " " << e.first << " " << addr << suffix);
		}
		else{
		}
		index++;
	}
}
#endif


static int64_t floyd_llvm_intrinsic__dummy(floyd_runtime_t* frp){
	auto& r = get_floyd_runtime(frp);
	(void)r;
	quark::throw_runtime_error("Attempting to calling unimplemented function.");
}





////////////////////////////////		llvm_execution_engine_t



llvm_execution_engine_t::~llvm_execution_engine_t(){
	QUARK_ASSERT(check_invariant());

	if(inited){
		auto f = reinterpret_cast<FLOYD_RUNTIME_INIT>(get_function_ptr(*this, encode_runtime_func_link_name("deinit")));
		QUARK_ASSERT(f != nullptr);

		int64_t result = (*f)(make_runtime_ptr(this));
		QUARK_ASSERT(result == 668);
		inited = false;
	};

//	const auto leaks = heap.count_used();
//	QUARK_ASSERT(leaks == 0);

//	detect_leaks(heap);
}

bool llvm_execution_engine_t::check_invariant() const {
	QUARK_ASSERT(ee);
	QUARK_ASSERT(backend.check_invariant());
	return true;
}

static std::vector<std::pair<link_name_t, void*>> collection_native_func_ptrs(llvm::ExecutionEngine& ee, const std::vector<function_link_entry_t>& function_link_map){
	std::vector<std::pair<link_name_t, void*>> result;
	for(const auto& e: function_link_map){
		const auto f = (void*)ee.getFunctionAddress(e.link_name.s);
//		auto f = get_function_ptr(runtime, e.link_name);
		result.push_back({ e.link_name, f });
	}

	if(k_trace_process_messaging){
		QUARK_SCOPED_TRACE("linked functions");
		for(const auto& e: result){
			QUARK_TRACE_SS(e.first.s << " = " << (e.second == nullptr ? "nullptr" : ptr_to_hexstring(e.second)));
		}
	}

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
		if(peek_type.is_struct() && is_wellformed(types, peek_type)){
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
std::string strip_link_name(const std::string& s){
	QUARK_ASSERT(s.empty() == false);
	QUARK_ASSERT(s[0] == '_');
	const auto s2 = s.substr(1);
	return s2;
}
#else
std::string strip_link_name(const std::string& platform_link_name){
	QUARK_ASSERT(platform_link_name.empty() == false);
	return platform_link_name;
}
#endif



//	Destroys program, can only called once!
static std::unique_ptr<llvm_execution_engine_t> make_engine_no_init(llvm_instance_t& instance, llvm_ir_program_t& program_breaks){
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
			const auto it = std::find_if(function_link_map.begin(), function_link_map.end(), [&](const function_link_entry_t& def){ return def.link_name.s == s2; });
			if(it != function_link_map.end() && it->native_f != nullptr){
				return it->native_f;
			}
			else {
				return (void*)&floyd_llvm_intrinsic__dummy;
//				throw std::exception();
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

	//	NOTICE: LLVM strips out unused functions = not all functions in our link map gets a native function pointer.
	std::vector<function_link_entry_t> final_link_map;
	for(const auto& e: program_breaks.function_link_map){
		const auto addr = (void*)ee1->getFunctionAddress(e.link_name.s);

		//??? null llvm_codegen_f pointer, which makes no sense now?
		const auto e2 = function_link_entry_t{ e.module, e.link_name, e.llvm_function_type, e.llvm_codegen_f, e.function_type_or_undef, e.arg_names_or_empty, addr };
		final_link_map.push_back(e2);
	}


	auto ee2 = std::unique_ptr<llvm_execution_engine_t>(
		new llvm_execution_engine_t{
			k_debug_magic,
			value_backend_t(
				collection_native_func_ptrs(*ee1, final_link_map),
				make_struct_layouts(program_breaks.type_lookup, ee1->getDataLayout()),
				program_breaks.type_lookup.state.types,
				program_breaks.settings.config
			),
			program_breaks.type_lookup,
			program_breaks.container_def,
			&instance,
			ee1,
			program_breaks.debug_globals,
			final_link_map,
			{},
			nullptr,
			start_time,
			llvm_bind_t{ link_name_t {}, nullptr, make_undefined() },
			false,
			program_breaks.settings.config
		}
	);
	QUARK_ASSERT(ee2->check_invariant());

#if DEBUG
	check_nulls(*ee2, program_breaks);
#endif

	if(k_trace_function_link_map){
		const auto& types = program_breaks.type_lookup.state.types;
		trace_function_link_map(types, ee2->function_link_map);
	}

	return ee2;
}

//	Destroys program, can only run it once!
//	Automatically runs floyd_runtime_init() to execute Floyd's global functions and initialize global constants.
std::unique_ptr<llvm_execution_engine_t> init_llvm_jit(llvm_ir_program_t& program_breaks){
	QUARK_ASSERT(program_breaks.check_invariant());

	auto ee = make_engine_no_init(*program_breaks.instance, program_breaks);

	trace_heap(ee->backend.heap);

	//	Make sure linking went well - test that by trying to resolve a function we know exists.
#if DEBUG
	{
		{
			const auto print_global_ptr_ptr = (FLOYD_RUNTIME_HOST_FUNCTION*)floyd::get_global_ptr(*ee, encode_runtime_func_link_name("init").s);
			QUARK_ASSERT(print_global_ptr_ptr != nullptr);
			const auto print_ptr = *print_global_ptr_ptr;
			QUARK_ASSERT(print_ptr != nullptr);
		}
	}
#endif

	ee->main_function = bind_function2(*ee, encode_floyd_func_link_name("main"));

	auto a_func = reinterpret_cast<FLOYD_RUNTIME_INIT>(get_function_ptr(*ee, encode_runtime_func_link_name("init")));
	QUARK_ASSERT(a_func != nullptr);

	int64_t init_result = (*a_func)(make_runtime_ptr(ee.get()));
	QUARK_ASSERT(init_result == 667);


	QUARK_ASSERT(init_result == 667);
	ee->inited = true;

	trace_heap(ee->backend.heap);
//	const auto leaks = ee->heap.count_used();
//	QUARK_ASSERT(leaks == 0);

	return ee;
}




//////////////////////////////////////		llvm_process_runtime_t



/*
	We use only one LLVM execution engine to run main() and all Floyd processes.
	They each have a separate llvm_process_t-instance and their own runtime-pointer.
*/

/*
	Try using C++ multithreading maps etc?
	Explore std::packaged_task
*/
//	https://en.cppreference.com/w/cpp/thread/condition_variable/wait


struct process_interface {
	virtual ~process_interface(){};
	virtual void on_message(const json_t& message) = 0;
	virtual void on_init() = 0;
};


//	NOTICE: Each process inbox has its own mutex + condition variable.
//	No mutex protects cout.
struct llvm_process_t {
	std::condition_variable _inbox_condition_variable;
	std::mutex _inbox_mutex;
	std::deque<json_t> _inbox;

	std::string _name_key;
	std::string _function_key;
	std::thread::id _thread_id;

//	std::shared_ptr<interpreter_t> _interpreter;
	std::shared_ptr<llvm_bind_t> _init_function;
	std::shared_ptr<llvm_bind_t> _process_function;
	value_t _process_state;
	std::shared_ptr<process_interface> _processor;
};

struct llvm_process_runtime_t {
	container_t _container;
	std::map<std::string, std::string> _process_infos;
	std::thread::id _main_thread_id;

	llvm_execution_engine_t* ee;

	std::vector<std::shared_ptr<llvm_process_t>> _processes;
	std::vector<std::thread> _worker_threads;
};

/*
??? have ONE runtime PER computer or one per interpreter?
??? Separate system-interpreter (all processes and many clock busses) vs ONE thread of execution?
*/

static void send_message(llvm_process_runtime_t& runtime, int process_id, const json_t& message){
	auto& process = *runtime._processes[process_id];

    {
        std::lock_guard<std::mutex> lk(process._inbox_mutex);
        process._inbox.push_front(message);
        if(k_trace_process_messaging){
        	QUARK_TRACE("Notifying...");
		}
    }
    process._inbox_condition_variable.notify_one();
//    process._inbox_condition_variable.notify_all();
}

static void run_process(llvm_process_runtime_t& runtime, int process_id){
	auto& process = *runtime._processes[process_id];
	bool stop = false;
	auto& types = runtime.ee->backend.types;

	const auto thread_name = get_current_thread_name();

	const type_t process_state_type = process._init_function != nullptr ? peek2(types, process._init_function->type).get_function_return(types) : make_undefined();

	if(process._processor){
		process._processor->on_init();
	}

	if(process._init_function != nullptr){
		//	!!! This validation should be done earlier in the startup process / compilation process.
		if(process._init_function->type != make_process_init_type(types, process_state_type)){
			quark::throw_runtime_error("Invalid function prototype for process-init");
		}

		auto f = reinterpret_cast<FLOYD_RUNTIME_PROCESS_INIT>(process._init_function->address);
		const auto result = (*f)(make_runtime_ptr(runtime.ee));
		process._process_state = from_runtime_value(*runtime.ee, result, peek2(types, process._init_function->type).get_function_return(types));
	}

	while(stop == false){
		json_t message;
		{
			std::unique_lock<std::mutex> lk(process._inbox_mutex);

			if(k_trace_process_messaging){
        		QUARK_TRACE_SS(thread_name << ": waiting......");
			}
			process._inbox_condition_variable.wait(lk, [&]{ return process._inbox.empty() == false; });
			if(k_trace_process_messaging){
        		QUARK_TRACE_SS(thread_name << ": continue");
			}

			//	Pop message.
			QUARK_ASSERT(process._inbox.empty() == false);
			message = process._inbox.back();
			process._inbox.pop_back();
		}

		if(k_trace_process_messaging){
			QUARK_TRACE_SS("RECEIVED: " << json_to_pretty_string(message));
		}

		if(message.is_string() && message.get_string() == "stop"){
			stop = true;
			if(k_trace_process_messaging){
        		QUARK_TRACE_SS(thread_name << ": STOP");
			}
		}
		else{
			if(process._processor){
				process._processor->on_message(message);
			}

			if(process._process_function != nullptr){
				//	!!! This validation should be done earlier in the startup process / compilation process.
				if(process._process_function->type != make_process_message_handler_type(types, process_state_type)){
					quark::throw_runtime_error("Invalid function prototype for process message handler");
				}

				auto f = reinterpret_cast<FLOYD_RUNTIME_PROCESS_MESSAGE>(process._process_function->address);
				const auto state2 = to_runtime_value(*runtime.ee, process._process_state);
				const auto message2 = to_runtime_value(*runtime.ee, value_t::make_json(message));
				const auto result = (*f)(make_runtime_ptr(runtime.ee), state2, message2);
				process._process_state = from_runtime_value(*runtime.ee, result, peek2(types, process._process_function->type).get_function_return(types));
			}
		}
	}
}


static std::map<std::string, value_t> run_processes(llvm_execution_engine_t& ee){
	if(ee.container_def._clock_busses.empty() == true){
		return {};
	}
	else{
		llvm_process_runtime_t runtime;
		runtime._main_thread_id = std::this_thread::get_id();
		runtime.ee = &ee;

		runtime._container = ee.container_def;

		runtime._process_infos = reduce(runtime._container._clock_busses, std::map<std::string, std::string>(), [](const std::map<std::string, std::string>& acc, const std::pair<std::string, clock_bus_t>& e){
			auto acc2 = acc;
			acc2.insert(e.second._processes.begin(), e.second._processes.end());
			return acc2;
		});

		struct my_interpreter_handler_t : public runtime_handler_i {
			my_interpreter_handler_t(llvm_process_runtime_t& runtime) : _runtime(runtime) {}

			virtual void on_send(const std::string& process_id, const json_t& message){
				const auto it = std::find_if(_runtime._processes.begin(), _runtime._processes.end(), [&](const std::shared_ptr<llvm_process_t>& process){ return process->_name_key == process_id; });
				if(it != _runtime._processes.end()){
					const auto process_index = it - _runtime._processes.begin();
					send_message(_runtime, static_cast<int>(process_index), message);
				}
			}

			llvm_process_runtime_t& _runtime;
		};
		auto my_interpreter_handler = my_interpreter_handler_t{runtime};

	//??? We need to pass unique runtime-object to each Floyd process -- not the ee!

		ee._handler = &my_interpreter_handler;

		for(const auto& t: runtime._process_infos){
			auto process = std::make_shared<llvm_process_t>();
			process->_name_key = t.first;
			process->_function_key = t.second;
	//		process->_interpreter = std::make_shared<interpreter_t>(program, &my_interpreter_handler);

			process->_init_function = std::make_shared<llvm_bind_t>(bind_function2(*runtime.ee, encode_floyd_func_link_name(t.second + "__init")));
			process->_process_function = std::make_shared<llvm_bind_t>(bind_function2(*runtime.ee, encode_floyd_func_link_name(t.second)));

			runtime._processes.push_back(process);
		}

		//	Remember that current thread (main) is also a thread, no need to create a worker thread for one process.
		runtime._processes[0]->_thread_id = runtime._main_thread_id;

		for(int process_id = 1 ; process_id < runtime._processes.size() ; process_id++){
			runtime._worker_threads.push_back(std::thread([&](int process_id){

	//			const auto native_thread = thread::native_handle();

				std::stringstream thread_name;
				thread_name << std::string() << "process " << process_id << " thread";
	#if QUARK_MAC
				pthread_setname_np(/*pthread_self(),*/ thread_name.str().c_str());
	#endif

				run_process(runtime, process_id);
			}, process_id));
		}

		run_process(runtime, 0);

		for(auto &t: runtime._worker_threads){
			t.join();
		}

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



////////////////////////////////		BENCHMARKS


//	??? should lookup structs via their name in symbol table!
//	??? This requires resolving symbols, which is too late at runtime. Resolve these earlier?
std::vector<bench_t> collect_benchmarks(llvm_execution_engine_t& ee){
	std::pair<void*, type_t> benchmark_registry_bind = bind_global(ee, k_global_benchmark_registry);
	QUARK_ASSERT(benchmark_registry_bind.first != nullptr);

	const value_t reg = load_global(ee, benchmark_registry_bind);
	const auto v = reg.get_vector_value();

	std::vector<bench_t> result;
	for(const auto& e: v){
		const auto s = e.get_struct_value();
		const auto name = s->_member_values[0].get_string_value();
		const auto f_link_name_str = s->_member_values[1].get_function_value().name;
		const auto f_link_name = link_name_t{ f_link_name_str };
		result.push_back(bench_t{ benchmark_id_t { "", name }, f_link_name });
	}
	return result;
}

///??? should lookup structs via their name in symbol table!
std::vector<benchmark_result2_t> run_benchmarks(llvm_execution_engine_t& ee, const std::vector<bench_t>& tests){
	QUARK_ASSERT(ee.check_invariant());

	const auto benchmark_result_vec_type_symbol = find_symbol_required(ee.global_symbols, "benchmark_result_vec_t");
	const auto benchmark_result_vec_type = benchmark_result_vec_type_symbol._value_type;

	std::vector<benchmark_result2_t> result;
	for(const auto& b: tests){
		const auto name = b.benchmark_id.test;
		const auto f_link_name = b.f;

		const auto f_bind = bind_function2(ee, f_link_name);
		QUARK_ASSERT(f_bind.address != nullptr);
		auto f2 = reinterpret_cast<FLOYD_BENCHMARK_F>(f_bind.address);
		const auto bench_result = (*f2)(make_runtime_ptr(&ee));
		const auto result2 = from_runtime_value(ee, bench_result, benchmark_result_vec_type);

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



//	??? should lookup structs via their name in symbol table!
//	??? This requires resolving symbols, which is too late at runtime. Resolve these earlier?
std::vector<test_t> collect_tests(llvm_execution_engine_t& ee){
	std::pair<void*, type_t> test_registry_bind = bind_global(ee, k_global_test_registry);
	QUARK_ASSERT(test_registry_bind.first != nullptr);

	const value_t reg = load_global(ee, test_registry_bind);
	const auto v = reg.get_vector_value();
	return unpack_test_registry(v);
}


}	//	namespace floyd
