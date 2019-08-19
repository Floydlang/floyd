//
//  floyd_llvm_runtime.hpp
//  Floyd
//
//  Created by Marcus Zetterquist on 2019-04-24.
//  Copyright © 2019 Marcus Zetterquist. All rights reserved.
//

#ifndef floyd_llvm_runtime_hpp
#define floyd_llvm_runtime_hpp

#include "floyd_llvm_values.h"
#include "floyd_llvm_value_thunking.h"

#include <string>
#include <vector>

namespace llvm {
	struct ExecutionEngine;
}

//??? make floyd_llvm-namespace. Reduces collisions with byte code interpreter.

namespace floyd {

struct llvm_ir_program_t;
struct runtime_handler_i;
struct run_output_t;
struct floyd_runtime_t;
struct llvm_instance_t;


////////////////////////////////		function_bind_t

struct function_bind_t {
	std::string name;
	llvm::FunctionType* function_type;
	void* implementation_f;
};


////////////////////////////////		llvm_bind_t

struct llvm_bind_t {
	link_name_t link_name;
	void* address;
	typeid_t type;
};


////////////////////////////////		function_def_t



struct function_def_t {
	link_name_t link_name;

	//	Only valid during codegen
	llvm::Function* llvm_codegen_f;

	function_definition_t floyd_fundef;
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

	value_mgr_t value_mgr;

	container_t container_def;

	llvm_instance_t* instance;
	std::shared_ptr<llvm::ExecutionEngine> ee;
	symbol_table_t global_symbols;
	std::vector<function_def_t> function_defs;
	public: std::vector<std::string> _print_output;

	public: runtime_handler_i* _handler;

	public: const std::chrono::time_point<std::chrono::high_resolution_clock> _start_time;


	llvm_bind_t main_function;
	bool inited;
};



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


////////////////////////////////		ENGINE GLOBALS


void* get_global_ptr(const llvm_execution_engine_t& ee, const std::string& name);

const function_def_t& find_function_def_from_link_name(const std::vector<function_def_t>& function_defs, const link_name_t& link_name);

std::pair<void*, typeid_t> bind_global(const llvm_execution_engine_t& ee, const std::string& name);
value_t load_global(const llvm_execution_engine_t& ee, const std::pair<void*, typeid_t>& v);

llvm_bind_t bind_function2(llvm_execution_engine_t& ee, const link_name_t& name);


llvm_execution_engine_t& get_floyd_runtime(floyd_runtime_t* frp);


////////////////////////////////		VALUES



inline value_t from_runtime_value(const llvm_execution_engine_t& runtime, const runtime_value_t encoded_value, const typeid_t& type){
	return from_runtime_value2(runtime.value_mgr, encoded_value, type);
}

inline runtime_value_t to_runtime_value(llvm_execution_engine_t& runtime, const value_t& value){
	return to_runtime_value2(runtime.value_mgr, value);
}

inline std::string from_runtime_string(const llvm_execution_engine_t& runtime, runtime_value_t encoded_value){
	return from_runtime_string2(runtime.value_mgr, encoded_value);
}
inline runtime_value_t to_runtime_string(llvm_execution_engine_t& runtime, const std::string& s){
	return to_runtime_string2(runtime.value_mgr, s);
}


////////////////////////////////		HIGH LEVEL



//	These are the support function built into the runtime, like RC primitives.
std::vector<function_bind_t> get_runtime_functions(llvm::LLVMContext& context, const llvm_type_lookup& type_lookup);

int64_t llvm_call_main(llvm_execution_engine_t& ee, const llvm_bind_t& f, const std::vector<std::string>& main_args);



//	Calls init() and will perform deinit() when engine is destructed later.
std::unique_ptr<llvm_execution_engine_t> init_program(llvm_ir_program_t& program);


//	Calls main() if it exists, else runs the floyd processes. Returns when execution is done.
run_output_t run_program(llvm_execution_engine_t& ee, const std::vector<std::string>& main_args);



struct bench_t {
	benchmark_id_t benchmark_id;
	link_name_t f;
};
inline bool operator==(const bench_t& lhs, const bench_t& rhs){ return lhs.benchmark_id == rhs.benchmark_id && lhs.f == rhs.f; }

std::vector<bench_t> collect_benchmarks(const llvm_execution_engine_t& ee);
std::vector<benchmark_result2_t> run_benchmarks(llvm_execution_engine_t& ee, const std::vector<bench_t>& tests);
std::vector<bench_t> filter_benchmarks(const std::vector<bench_t>& b, const std::vector<std::string>& run_tests);


}	//	namespace floyd


#endif /* floyd_llvm_runtime_hpp */
