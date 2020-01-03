//
//  parser_evaluator.h
//  Floyd
//
//  Created by Marcus Zetterquist on 26/07/16.
//  Copyright © 2016 Marcus Zetterquist. All rights reserved.
//

#ifndef floyd_interpreter_hpp
#define floyd_interpreter_hpp

/*
	High-level access to the byte code interpreter.
	Implements process / inbox mechanisms.
*/

#include "quark.h"

#include "bytecode_interpreter.h"
#include "compiler_helpers.h"

#include <string>
#include <vector>

namespace floyd {
struct value_t;
struct semantic_ast_t;
struct compilation_unit_t;



//////////////////////////////////////		bc_execution_engine_t


struct bc_process_t;


struct bc_execution_engine_t {

	bool check_invariant() const;


	//////////////////////////////////////		STATE
	interpreter_t main_temp;

	//??? Instead, store bc_program_t.
	container_t _container;

	runtime_handler_i* handler;
	std::map<std::string, process_def_t> _process_infos;
	std::thread::id _main_thread_id;

	std::vector<std::shared_ptr<bc_process_t>> _processes;
	std::vector<std::thread> _worker_threads;
};

std::unique_ptr<bc_execution_engine_t> make_bytecode_execution_engine(const bc_program_t& program, const config_t& config, runtime_handler_i& runtime_handler);



//////////////////////////////////////		Free functions


value_t find_global_symbol(interpreter_t& vm, const module_symbol_t& s);
value_t call_function(interpreter_t& vm, const floyd::value_t& f, const std::vector<value_t>& args);
bc_program_t compile_to_bytecode(const compilation_unit_t& cu);

run_output_t run_program_bc(bc_execution_engine_t& ee, const std::vector<std::string>& main_args, const config_t& config);

std::vector<test_t> collect_tests(interpreter_t& vm);

//	Returns one element for every test in the program.
std::vector<test_result_t> run_tests_bc(
	interpreter_t& vm,
	const std::vector<test_t>& all_tests,
	const std::vector<test_id_t>& wanted
);


} //	floyd


#endif /* floyd_interpreter_hpp */
