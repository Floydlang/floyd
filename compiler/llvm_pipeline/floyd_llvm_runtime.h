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
#include "floyd_runtime.h"
#include "floyd_llvm_types.h"
#include <llvm/IR/Function.h>

#include <string>
#include <vector>
#include <thread>
#include <deque>
#include <condition_variable>

namespace llvm {
	struct ExecutionEngine;
}

namespace floyd {

struct llvm_ir_program_t;
struct run_output_t;
struct runtime_t;
struct llvm_instance_t;
struct llvm_execution_engine_t;




////////////////////////////////		llvm_bind_t

struct llvm_bind_t {
	module_symbol_t link_name;
	void* address;
	type_t type;
};



////////////////////////////////		llvm_function_bind_t



struct llvm_function_bind_t {
	module_symbol_t name;
	llvm::FunctionType* llvm_function_type;
	void* native_f;
};



//	NOTICE: Each process inbox has its own mutex + condition variable.
//	No mutex protects cout.
struct llvm_process_t : public runtime_basics_i, runtime_process_i {
	public: bool check_invariant() const {
		QUARK_ASSERT(ee != nullptr);
		QUARK_ASSERT(_init_function != nullptr);
		QUARK_ASSERT(_init_function->address != nullptr);
		QUARK_ASSERT(_msg_function != nullptr);
		QUARK_ASSERT(_msg_function->address != nullptr);
		return true;
	}


	//////////////////////////////////////		STATE - INIT


	void runtime_basics__on_print(const std::string& s) override;
	type_t runtime_basics__get_global_symbol_type(const std::string& s) override;
	rt_value_t runtime_basics__call_thunk(const rt_value_t& f, const rt_value_t args[], int arg_count) override;

	void runtime_process__on_send_message(const std::string& dest_process_id, const rt_pod_t& message, const type_t& message_type) override;
	void runtime_process__on_exit_process() override;



	//////////////////////////////////////		STATE - INIT

	llvm_execution_engine_t* ee;
	std::string _name_key;
	process_def_t _process_def;
	std::thread::id _thread_id;

	std::shared_ptr<llvm_bind_t> _init_function;
	std::shared_ptr<llvm_bind_t> _msg_function;
	type_t _state_type;
	type_t _message_type;


	//////////////////////////////////////		STATE - MUTATING

	std::condition_variable _inbox_condition_variable;
	std::mutex _inbox_mutex;
	std::deque<rt_pod_t> _inbox;

	//	Notice: before init() is called, this value is an undefined.
	value_t _process_state;
	std::atomic<bool> _exiting_flag;
};


////////////////////////////////		llvm_execution_engine_t

struct llvm_execution_engine_t;

//https://en.wikipedia.org/wiki/Hexspeak
const uint64_t k_debug_magic = 0xFACEFEED05050505;

struct route_t : public runtime_basics_i, runtime_process_i {
	route_t(runtime_handler_i* runtime_handler, const symbol_table_t* symbol_table) :
		_runtime_handler(runtime_handler),
		_symbol_table(symbol_table)
	{
	}



	void runtime_basics__on_print(const std::string& s) override {
		QUARK_ASSERT(_runtime_handler != nullptr);
		QUARK_ASSERT(_symbol_table != nullptr);

		_runtime_handler->on_print(s);
	}

	type_t runtime_basics__get_global_symbol_type(const std::string& s) override {
		QUARK_ASSERT(_runtime_handler != nullptr);
		QUARK_ASSERT(_symbol_table != nullptr);

		const auto sym = find_symbol_required(*_symbol_table, s);
		return sym._value_type;
	}

	void runtime_process__on_send_message(const std::string& dest_process_id, const rt_pod_t& message, const type_t& message_type) override {
		QUARK_ASSERT(_runtime_handler != nullptr);
		QUARK_ASSERT(_symbol_table != nullptr);
	}
	void runtime_process__on_exit_process() override {
		QUARK_ASSERT(_runtime_handler != nullptr);
		QUARK_ASSERT(_symbol_table != nullptr);
	}
	rt_value_t runtime_basics__call_thunk(const rt_value_t& f, const rt_value_t args[], int arg_count) override {
		quark::throw_defective_request();
	}



	public: runtime_handler_i* _runtime_handler;
	public: const symbol_table_t* _symbol_table;
};

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
	std::vector<func_link_t> function_link_map;

	route_t _handler_router;

	public: const std::chrono::time_point<std::chrono::high_resolution_clock> _start_time;


	llvm_bind_t main_function;
	bool inited;
	config_t config;


	std::map<std::string, process_def_t> _process_infos;
	std::thread::id _main_thread_id;

	std::vector<std::shared_ptr<llvm_process_t>> _processes;
	std::vector<std::thread> _worker_threads;
	bool _trace_processes;
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



////////////////////////////////		FUNCTION POINTERS


typedef int64_t (*FLOYD_RUNTIME_INIT)(runtime_t* frp);
typedef int64_t (*FLOYD_RUNTIME_DEINIT)(runtime_t* frp);

//??? remove
typedef void (*FLOYD_RUNTIME_HOST_FUNCTION)(runtime_t* frp, int64_t arg);


//	func int main([string] args) impure
typedef int64_t (*FLOYD_RUNTIME_MAIN_ARGS_IMPURE)(runtime_t* frp, rt_pod_t args);

//	func int main() impure
typedef int64_t (*FLOYD_RUNTIME_MAIN_NO_ARGS_IMPURE)(runtime_t* frp);

//	func int main([string] args) pure
typedef int64_t (*FLOYD_RUNTIME_MAIN_ARGS_PURE)(runtime_t* frp, rt_pod_t args);

//	func int main() impure
typedef int64_t (*FLOYD_RUNTIME_MAIN_NO_ARGS_PURE)(runtime_t* frp);


//		func my_gui_state_t my_gui__init() impure { }
typedef rt_pod_t (*FLOYD_RUNTIME_PROCESS_INIT)(runtime_t* frp);

//		func my_gui_state_t my_gui__msg(my_gui_state_t state, T message) impure{
typedef rt_pod_t (*FLOYD_RUNTIME_PROCESS_MESSAGE)(runtime_t* frp, rt_pod_t state, rt_pod_t message);

typedef rt_pod_t (*FLOYD_BENCHMARK_F)(runtime_t* frp);
typedef void (*FLOYD_TEST_F)(runtime_t* frp);



////////////////////////////////	CLIENT ACCESS OF RUNNING PROGRAM



void* get_global_ptr(const llvm_execution_engine_t& c, const module_symbol_t& name);


std::pair<void*, type_t> bind_global(const llvm_execution_engine_t& ee, const module_symbol_t& name);
value_t load_global(const llvm_context_t& c, const std::pair<void*, type_t>& v);

void store_via_ptr(llvm_context_t& c, const type_t& member_type, void* value_ptr, const value_t& value);

llvm_bind_t bind_function2(llvm_execution_engine_t& ee, const module_symbol_t& name);



runtime_t make_runtime_ptr(llvm_context_t* p);



////////////////////////////////		HIGH LEVEL



//	Returns a complete list of all functions: programmed in floyd, runtime functions, init() deinit().
std::vector<func_link_t> make_function_link_map1(
	llvm::LLVMContext& context,
	const llvm_type_lookup& type_lookup,
	const std::vector<floyd::function_definition_t>& ast_function_defs,
	const intrinsic_signatures_t& intrinsic_signatures
);


int64_t llvm_call_main(llvm_execution_engine_t& ee, const llvm_bind_t& f, const std::vector<std::string>& main_args);

void deinit_program(llvm_execution_engine_t& ee);



//	Calls init() and will perform deinit() when engine is destructed later.
std::unique_ptr<llvm_execution_engine_t> init_llvm_jit(llvm_ir_program_t& program, runtime_handler_i& handler, bool trace_processes);


//	Calls main() if it exists, else runs the floyd processes. Returns when execution is done.
run_output_t run_program(llvm_execution_engine_t& ee, const std::vector<std::string>& main_args);



////////////////////////////////		BENCHMARKS


std::vector<bench_t> collect_benchmarks(llvm_execution_engine_t& ee);
std::vector<benchmark_result2_t> run_benchmarks(llvm_execution_engine_t& ee, const std::vector<bench_t>& tests);



////////////////////////////////		TESTS


std::vector<test_t> collect_tests(llvm_execution_engine_t& ee);



}	//	namespace floyd


#endif /* floyd_llvm_runtime_hpp */
