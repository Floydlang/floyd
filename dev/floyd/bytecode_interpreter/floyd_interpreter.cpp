//
//  parser_evaluator.cpp
//  Floyd
//
//  Created by Marcus Zetterquist on 26/07/16.
//  Copyright © 2016 Marcus Zetterquist. All rights reserved.
//

#include "floyd_interpreter.h"

#include "floyd_runtime.h"

#include "bytecode_generator.h"
#include "os_process.h"
#include "bytecode_helpers.h"
#include "semantic_ast.h"

#include <thread>
#include <deque>
#include <future>

#include <condition_variable>

namespace floyd {

static const bool k_trace_messaging = false;






floyd::value_t find_global_symbol(const interpreter_t& vm, const std::string& s){
	QUARK_ASSERT(vm.check_invariant());

	return get_global(vm, s);
}

value_t get_global(const interpreter_t& vm, const std::string& name){
	QUARK_ASSERT(vm.check_invariant());

	const auto& result = find_global_symbol2(vm, name);
	if(result == nullptr){
		quark::throw_runtime_error(std::string() + "Cannot find global \"" + name + "\".");
	}
	else{
		return bc_to_value(result->_value);
	}
}

value_t call_function(interpreter_t& vm, const floyd::value_t& f, const std::vector<value_t>& args){
#if DEBUG
	QUARK_ASSERT(vm.check_invariant());
	QUARK_ASSERT(f.check_invariant());
	for(const auto& i: args){ QUARK_ASSERT(i.check_invariant()); };
	QUARK_ASSERT(f.is_function());
#endif

	const auto f2 = value_to_bc(f);
	std::vector<bc_value_t> args2;
	for(const auto& e: args){
		args2.push_back(value_to_bc(e));
	}

	const auto result = call_function_bc(vm, f2, &args2[0], static_cast<int>(args2.size()));
	return bc_to_value(result);
}




bc_program_t compile_to_bytecode(const compilation_unit_t& cu){
	const auto sem_ast = compile_to_sematic_ast__errors(cu);
	const auto bc = generate_bytecode(sem_ast);
	return bc;
}




std::shared_ptr<interpreter_t> run_global(const compilation_unit_t& cu){
	auto program = compile_to_bytecode(cu);
	auto vm = std::make_shared<interpreter_t>(program);
//	QUARK_TRACE(json_to_pretty_string(interpreter_to_json(vm)));
	print_vm_printlog(*vm);
	return vm;
}

std::pair<std::shared_ptr<interpreter_t>, value_t> run_main(const compilation_unit_t& cu, const std::vector<floyd::value_t>& args){
	auto program = compile_to_bytecode(cu);

	//	Runs global code.
	auto interpreter = std::make_shared<interpreter_t>(program);

	const auto& main_function = find_global_symbol2(*interpreter, "main");
	if(main_function != nullptr){
		const auto& result = call_function(*interpreter, bc_to_value(main_function->_value), args);
		return { interpreter, result };
	}
	else{
		return { interpreter, value_t::make_undefined() };
	}
}

void print_vm_printlog(const interpreter_t& vm){
	QUARK_ASSERT(vm.check_invariant());

#if 0
	if(vm._print_output.empty() == false){
		QUARK_SCOPED_TRACE("print output:");
		for(const auto& line: vm._print_output){
			QUARK_TRACE_SS(line);
		}
	}
#endif
}


//////////////////////////////////////		container_runner_t


/*

std::packaged_task
*/

//	https://en.cppreference.com/w/cpp/thread/condition_variable/wait


struct process_interface {
	virtual ~process_interface(){};
	virtual void on_message(const json_t& message) = 0;
	virtual void on_init() = 0;
};


//	NOTICE: Each process inbox has its own mutex + condition variable. No mutex protects cout.
struct bc_process_t {
	std::condition_variable _inbox_condition_variable;
	std::mutex _inbox_mutex;
	std::deque<json_t> _inbox;

	std::string _name_key;
	std::string _function_key;
	std::thread::id _thread_id;

	std::shared_ptr<interpreter_t> _interpreter;
	std::shared_ptr<value_entry_t> _init_function;
	std::shared_ptr<value_entry_t> _process_function;
	value_t _process_state;


	std::shared_ptr<process_interface> _processor;
};

struct bc_process_runtime_t {
	container_t _container;
	std::map<std::string, std::string> _process_infos;
	std::thread::id _main_thread_id;

	std::vector<std::shared_ptr<bc_process_t>> _processes;
	std::vector<std::thread> _worker_threads;
};


/*
??? have ONE runtime PER computer or one per interpreter?
??? Separate system-interpreter (all processes and many clock busses) vs ONE thread of execution?
*/

static void send_message(bc_process_runtime_t& runtime, int process_id, const json_t& message){
	auto& process = *runtime._processes[process_id];

    {
        std::lock_guard<std::mutex> lk(process._inbox_mutex);
        process._inbox.push_front(message);
        if(k_trace_messaging){
        	QUARK_TRACE("Notifying...");
		}
    }
    process._inbox_condition_variable.notify_one();
//    process._inbox_condition_variable.notify_all();
}

static void process_process(bc_process_runtime_t& runtime, int process_id){
	auto& process = *runtime._processes[process_id];
	bool stop = false;

	const auto thread_name = get_current_thread_name();

	if(process._processor){
		process._processor->on_init();
	}

	if(process._init_function != nullptr){
		const std::vector<value_t> args = {};
		process._process_state = call_function(*process._interpreter, bc_to_value(process._init_function->_value), args);
	}

	while(stop == false){
		json_t message;
		{
			std::unique_lock<std::mutex> lk(process._inbox_mutex);

			if(k_trace_messaging){
				QUARK_TRACE_SS(thread_name << ": waiting......");
			}
			process._inbox_condition_variable.wait(lk, [&]{ return process._inbox.empty() == false; });
			if(k_trace_messaging){
				QUARK_TRACE_SS(thread_name << ": continue");
			}

			//	Pop message.
			QUARK_ASSERT(process._inbox.empty() == false);
			message = process._inbox.back();
			process._inbox.pop_back();
		}
		if(k_trace_messaging){
			QUARK_TRACE_SS("RECEIVED: " << json_to_pretty_string(message));
		}

		if(message.is_string() && message.get_string() == "stop"){
			stop = true;
			if(k_trace_messaging){
        		QUARK_TRACE_SS(thread_name << ": STOP");
			}
		}
		else{
			if(process._processor){
				process._processor->on_message(message);
			}

			if(process._process_function != nullptr){
				const std::vector<value_t> args = { process._process_state, value_t::make_json(message) };
				const auto& state2 = call_function(*process._interpreter, bc_to_value(process._process_function->_value), args);
				process._process_state = state2;
			}
		}
	}
}

static std::map<std::string, value_t> run_floyd_processes(const interpreter_t& vm, const std::vector<std::string>& args){
	const auto& container_def = vm._imm->_program._container_def;

	if(container_def._clock_busses.empty()){
		return {};
	}
	else{
		bc_process_runtime_t runtime;
		runtime._main_thread_id = std::this_thread::get_id();

	/*
		if(program._software_system._name == ""){
			quark::throw_exception();
		}
	*/

		runtime._container = container_def;

		runtime._process_infos = reduce(runtime._container._clock_busses, std::map<std::string, std::string>(), [](const std::map<std::string, std::string>& acc, const std::pair<std::string, clock_bus_t>& e){
			auto acc2 = acc;
			acc2.insert(e.second._processes.begin(), e.second._processes.end());
			return acc2;
		});

		struct my_interpreter_handler_t : public runtime_handler_i {
			my_interpreter_handler_t(bc_process_runtime_t& runtime) : _runtime(runtime) {}

			virtual void on_send(const std::string& process_id, const json_t& message){
				const auto it = std::find_if(_runtime._processes.begin(), _runtime._processes.end(), [&](const std::shared_ptr<bc_process_t>& process){ return process->_name_key == process_id; });
				if(it != _runtime._processes.end()){
					const auto process_index = it - _runtime._processes.begin();
					send_message(_runtime, static_cast<int>(process_index), message);
				}
			}

			bc_process_runtime_t& _runtime;
		};
		auto my_interpreter_handler = my_interpreter_handler_t{runtime};


		for(const auto& t: runtime._process_infos){
			auto process = std::make_shared<bc_process_t>();
			process->_name_key = t.first;
			process->_function_key = t.second;
			process->_interpreter = std::make_shared<interpreter_t>(vm._imm->_program, &my_interpreter_handler);
			process->_init_function = find_global_symbol2(*process->_interpreter, t.second + "__init");
			process->_process_function = find_global_symbol2(*process->_interpreter, t.second);

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

				process_process(runtime, process_id);
			}, process_id));
		}

		process_process(runtime, 0);

		for(auto &t: runtime._worker_threads){
			t.join();
		}

	#if 0
		const auto result_vec = mapf<pair<string, value_t>>(
			runtime._processes,
			[](const auto& process){ return pair<string, value_t>{ process->_name_key, process->_process_state };}
		);
		std::map<string, value_t> result_map;
		for(const auto& e: result_vec){
			const pair<string, value_t> v(e.first, e.second);
			result_map.insert(v);
		}

		return result_map;
	#endif
		return {};
	//	QUARK_UT_VERIFY(runtime._processes[0]->_process_state.get_struct_value()->_member_values[0].get_int_value() == 998);
	}
}



static int64_t bc_call_main(interpreter_t& interpreter, const floyd::value_t& f, const std::vector<std::string>& main_args){
	QUARK_ASSERT(interpreter.check_invariant());
	QUARK_ASSERT(f.check_invariant());

	//??? Check this earlier.
	if(f.get_type() == get_main_signature_arg_impure() || f.get_type() == get_main_signature_arg_pure()){
		const auto main_args2 = mapf<value_t>(main_args, [](auto& e){ return value_t::make_string(e); });
		const auto main_args3 = value_t::make_vector_value(typeid_t::make_string(), main_args2);
		const auto main_result = call_function(interpreter, f, { main_args3 });
		const auto main_result_int = main_result.get_int_value();
		return main_result_int;
	}
	else if(f.get_type() == get_main_signature_no_arg_impure() || f.get_type() == get_main_signature_no_arg_pure()){
		const auto main_result = call_function(interpreter, f, {});
		const auto main_result_int = main_result.get_int_value();
		return main_result_int;
	}
	else{
		throw std::exception();
	}
}

run_output_t run_program_bc(interpreter_t& vm, const std::vector<std::string>& main_args){
	const auto& main_function = find_global_symbol2(vm, "main");
	if(main_function != nullptr){
		const auto main_result_int = bc_call_main(vm, bc_to_value(main_function->_value), main_args);
		print_vm_printlog(vm);
		return { main_result_int, {} };
	}
	else{
		const auto output = run_floyd_processes(vm, main_args);
		print_vm_printlog(vm);
		return run_output_t(0, output);
	}
}






}	//	floyd
#ifdef  __EMSCRIPTEN__

#include <emscripten/bind.h>

using namespace emscripten;


// C++ code callable from javascript
int runFloyd(std::string inStr) {

    //vector<floyd::value_t> args("");
    //const std::string& container_key;

    floyd::run_container2(inStr,{},"");


    return 1;
}

// Expose function to JS
EMSCRIPTEN_BINDINGS(my_module) {
    function("runFloyd", &runFloyd);
}

#endif

