//
//  parser_evaluator.h
//  Floyd
//
//  Created by Marcus Zetterquist on 26/07/16.
//  Copyright © 2016 Marcus Zetterquist. All rights reserved.
//

#ifndef bytecode_execution_engine_hpp
#define bytecode_execution_engine_hpp

/*
	The bytecode execution engine runs Floyd programs and threaded floyd processes.
*/

#include "quark.h"

#include "bytecode_interpreter.h"
#include "compiler_helpers.h"

#include <string>
#include <vector>

namespace floyd {
struct value_t;
struct compilation_unit_t;


//////////////////////////////////////		bc_execution_engine_t


struct bc_process_t;

struct bc_thread_t {
	bool check_invariant() const {
		QUARK_ASSERT(_interpreter != nullptr && _interpreter->check_invariant());
		return true;
	}

	std::thread os_thread;
	std::shared_ptr<interpreter_t> _interpreter;
};

struct bc_execution_engine_t {
	bc_execution_engine_t(const bc_program_t& program, const config_t& config, runtime_handler_i& runtime_handler);
	bool check_invariant() const;


	//////////////////////////////////////		STATE
	std::shared_ptr<bc_program_t> _program;
	value_backend_t backend;
	interpreter_t main_bc_thread;
	std::thread::id _main_thread_id;

	runtime_handler_i* handler;

	//	One for each floyd process when using processes. #0 will use main-thread, #1 -> will use worker threads.
	//	When just using global scope and main, this vector is empty.
	std::vector<std::shared_ptr<bc_process_t>> _processes;


	//	One for each floyd process EXCEPT process 0. Process 0 runs on main thread.
	std::vector<std::shared_ptr<interpreter_t>> _bc_threads;
	std::vector<std::thread> _os_threads;
};

std::unique_ptr<bc_execution_engine_t> make_bytecode_execution_engine(
	const bc_program_t& program,
	const config_t& config,
	runtime_handler_i& runtime_handler
);


//////////////////////////////////////		Free functions


void unwind_global_stack(bc_execution_engine_t& ee);

rt_value_t load_global(bc_execution_engine_t& ee, const module_symbol_t& s);
value_t call_function(bc_execution_engine_t& ee, const floyd::value_t& f, const std::vector<value_t>& args);

run_output_t run_program_bc(bc_execution_engine_t& ee, const std::vector<std::string>& main_args, const config_t& config);

std::vector<test_t> collect_tests(bc_execution_engine_t& ee);

//	Returns one element for every test in the program.
std::vector<test_result_t> run_tests_bc(
	bc_execution_engine_t& ee,
	const std::vector<test_t>& all_tests,
	const std::vector<test_id_t>& wanted
);


//	Helper
bc_program_t compile_to_bytecode(const compilation_unit_t& cu);

} //	floyd

#endif /* bytecode_execution_engine_hpp */
