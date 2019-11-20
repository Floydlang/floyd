//
//  floyd_llvm_runtime.hpp
//  Floyd
//
//  Created by Marcus Zetterquist on 2019-04-24.
//  Copyright © 2019 Marcus Zetterquist. All rights reserved.
//

#ifndef floyd_llvm_runtime_hpp
#define floyd_llvm_runtime_hpp

#include "value_backend.h"
#include "floyd_llvm_types.h"
#include "value_thunking.h"
#include <llvm/IR/IRBuilder.h>

#include <string>
#include <vector>
#include <thread>
#include <deque>
#include <condition_variable>

namespace llvm {
	struct ExecutionEngine;
}

//??? make floyd_llvm-namespace. Reduces collisions with byte code interpreter.

namespace floyd {

struct llvm_ir_program_t;

struct run_output_t;
struct floyd_runtime_t;
struct llvm_instance_t;


static const bool k_trace_process_messaging = false;


////////////////////////////////		llvm_bind_t

struct llvm_bind_t {
	link_name_t link_name;
	void* address;
	type_t type;
};



////////////////////////////////		function_bind_t



struct function_bind_t {
	std::string name;
	llvm::FunctionType* llvm_function_type;
	void* native_f;
};



////////////////////////////////		function_link_entry_t



struct function_link_entry_t {
	std::string module;

	link_name_t link_name;

	llvm::FunctionType* llvm_function_type;

	//	Only valid during codegen
	//??? Rename llvm_function_node;
	llvm::Function* llvm_codegen_f;

	type_t function_type_or_undef;

	//??? better to use vector<string>
	std::vector<member_t> arg_names_or_empty;

	void* native_f;
};

std::string print_function_link_map(const types_t& types, const std::vector<function_link_entry_t>& defs);
void trace_function_link_map(const types_t& types, const std::vector<function_link_entry_t>& defs);




//////////////////////////////////////		llvm_runtime_handler_i


struct llvm_runtime_handler_i {
	virtual ~llvm_runtime_handler_i(){};
	virtual void on_print(const std::string& s) = 0;
};



//	NOTICE: Each process inbox has its own mutex + condition variable.
//	No mutex protects cout.
struct llvm_process_t {
	public: bool check_invariant() const {
		QUARK_ASSERT(_init_function != nullptr);
		QUARK_ASSERT(_process_function != nullptr);
		return true;
	}


	//////////////////////////////////////		STATE - INIT

	std::string _name_key;
	process_def_t _process_def;
	std::thread::id _thread_id;

	std::shared_ptr<llvm_bind_t> _init_function;
	std::shared_ptr<llvm_bind_t> _process_function;
	type_t _message_type;


	//////////////////////////////////////		STATE - MUTATING

	std::condition_variable _inbox_condition_variable;
	std::mutex _inbox_mutex;
	std::deque<runtime_value_t> _inbox;

	value_t _process_state;
	std::atomic<bool> _exiting_flag;
};


////////////////////////////////		llvm_execution_engine_t



//https://en.wikipedia.org/wiki/Hexspeak
const uint64_t k_debug_magic = 0xFACEFEED05050505;


struct llvm_execution_engine_t {
	~llvm_execution_engine_t();
	bool check_invariant() const;


	////////////////////////////////		STATE

	//	Must be first member, checked by LLVM code.
	uint64_t debug_magic;

	value_backend_t backend;
	llvm_type_lookup type_lookup;

	container_t container_def;

	llvm_instance_t* instance;
	std::shared_ptr<llvm::ExecutionEngine> ee;
	symbol_table_t global_symbols;
	std::vector<function_link_entry_t> function_link_map;

	public: llvm_runtime_handler_i* _handler;

	public: const std::chrono::time_point<std::chrono::high_resolution_clock> _start_time;


	llvm_bind_t main_function;
	bool inited;
	config_t config;


	std::map<std::string, process_def_t> _process_infos;
	std::thread::id _main_thread_id;

	std::vector<std::shared_ptr<llvm_process_t>> _processes;
	std::vector<std::thread> _worker_threads;
};

struct llvm_context_t {
	public: bool check_invariant() const {
		QUARK_ASSERT(ee != nullptr);
		QUARK_ASSERT(ee->check_invariant());
		QUARK_ASSERT(process == nullptr || process->check_invariant());
		return true;
	}

	llvm_execution_engine_t* ee;
	llvm_process_t* process;
};



void send_message(llvm_context_t& c, const std::string& dest_process_id, const runtime_value_t& message, const type_t& message_type);
void exit_process(llvm_context_t& c);


////////////////////////////////		FUNCTION POINTERS


typedef int64_t (*FLOYD_RUNTIME_INIT)(floyd_runtime_t* frp);
typedef int64_t (*FLOYD_RUNTIME_DEINIT)(floyd_runtime_t* frp);

//??? remove
typedef void (*FLOYD_RUNTIME_HOST_FUNCTION)(floyd_runtime_t* frp, int64_t arg);


//	func int main([string] args) impure
typedef int64_t (*FLOYD_RUNTIME_MAIN_ARGS_IMPURE)(floyd_runtime_t* frp, runtime_value_t args);

//	func int main() impure
typedef int64_t (*FLOYD_RUNTIME_MAIN_NO_ARGS_IMPURE)(floyd_runtime_t* frp);

//	func int main([string] args) pure
typedef int64_t (*FLOYD_RUNTIME_MAIN_ARGS_PURE)(floyd_runtime_t* frp, runtime_value_t args);

//	func int main() impure
typedef int64_t (*FLOYD_RUNTIME_MAIN_NO_ARGS_PURE)(floyd_runtime_t* frp);


//		func my_gui_state_t my_gui__init() impure { }
typedef runtime_value_t (*FLOYD_RUNTIME_PROCESS_INIT)(floyd_runtime_t* frp);

//		func my_gui_state_t my_gui(my_gui_state_t state, json message) impure{
typedef runtime_value_t (*FLOYD_RUNTIME_PROCESS_MESSAGE)(floyd_runtime_t* frp, runtime_value_t state, runtime_value_t message);

typedef runtime_value_t (*FLOYD_BENCHMARK_F)(floyd_runtime_t* frp);
typedef void (*FLOYD_TEST_F)(floyd_runtime_t* frp);



////////////////////////////////	CLIENT ACCESS OF RUNNING PROGRAM


const function_link_entry_t& find_function_def_from_link_name(const std::vector<function_link_entry_t>& function_link_map, const link_name_t& link_name);

void* get_global_ptr(const llvm_execution_engine_t& c, const std::string& name);


std::pair<void*, type_t> bind_global(const llvm_execution_engine_t& ee, const std::string& name);
value_t load_global(const llvm_context_t& c, const std::pair<void*, type_t>& v);

void store_via_ptr(llvm_context_t& c, const type_t& member_type, void* value_ptr, const value_t& value);

llvm_bind_t bind_function2(llvm_execution_engine_t& ee, const link_name_t& name);




inline floyd_runtime_t* make_runtime_ptr(llvm_context_t* p);
inline llvm_context_t& get_floyd_runtime(floyd_runtime_t* frp);



////////////////////////////////		VALUES



inline value_t from_runtime_value(const llvm_context_t& c, const runtime_value_t encoded_value, const type_t& type){
	return from_runtime_value2(c.ee->backend, encoded_value, type);
}

inline runtime_value_t to_runtime_value(llvm_context_t& c, const value_t& value){
	return to_runtime_value2(c.ee->backend, value);
}

inline std::string from_runtime_string(const llvm_context_t& c, runtime_value_t encoded_value){
	return from_runtime_string2(c.ee->backend, encoded_value);
}
inline runtime_value_t to_runtime_string(llvm_context_t& c, const std::string& s){
	return to_runtime_string2(c.ee->backend, s);
}


////////////////////////////////		HIGH LEVEL



//	Returns a complete list of all functions: programmed in floyd, runtime functions, init() deinit().
std::vector<function_link_entry_t> make_function_link_map1(
	llvm::LLVMContext& context,
	const llvm_type_lookup& type_lookup,
	const std::vector<floyd::function_definition_t>& ast_function_defs,
	const intrinsic_signatures_t& intrinsic_signatures
);


int64_t llvm_call_main(llvm_execution_engine_t& ee, const llvm_bind_t& f, const std::vector<std::string>& main_args);




//	Calls init() and will perform deinit() when engine is destructed later.
std::unique_ptr<llvm_execution_engine_t> init_llvm_jit(llvm_ir_program_t& program, llvm_runtime_handler_i& handler);


//	Calls main() if it exists, else runs the floyd processes. Returns when execution is done.
run_output_t run_program(llvm_execution_engine_t& ee, const std::vector<std::string>& main_args);



////////////////////////////////		BENCHMARKS


std::vector<bench_t> collect_benchmarks(llvm_execution_engine_t& ee);
std::vector<benchmark_result2_t> run_benchmarks(llvm_execution_engine_t& ee, const std::vector<bench_t>& tests);



////////////////////////////////		TESTS




std::vector<test_t> collect_tests(llvm_execution_engine_t& ee);



//	Inlines
////////////////////////////////////////////////////////////////////////////////////////////////////


inline floyd_runtime_t* make_runtime_ptr(llvm_context_t* p){
	return reinterpret_cast<floyd_runtime_t*>(p);
}

inline llvm_context_t& get_floyd_runtime(floyd_runtime_t* frp){
	QUARK_ASSERT(frp != nullptr);

	auto ptr = reinterpret_cast<llvm_context_t*>(frp);
	QUARK_ASSERT(ptr != nullptr);
	QUARK_ASSERT(ptr->ee != nullptr);
	QUARK_ASSERT(ptr->ee->debug_magic == k_debug_magic);
	QUARK_ASSERT(ptr->check_invariant());
	return *ptr;
}


}	//	namespace floyd


#endif /* floyd_llvm_runtime_hpp */
