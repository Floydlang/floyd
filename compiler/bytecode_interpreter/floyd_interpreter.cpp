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
#include "bytecode_interpreter.h"
#include "os_process.h"
#include "semantic_ast.h"
#include "utils.h"

#include <thread>
#include <deque>
#include <future>

#include <condition_variable>

namespace floyd {

static const bool k_trace_messaging = true;






floyd::value_t find_global_symbol(interpreter_t& vm, const module_symbol_t& s){
	QUARK_ASSERT(vm.check_invariant());

	return get_global(vm, s);
}

value_t get_global(interpreter_t& vm, const module_symbol_t& name){
	QUARK_ASSERT(vm.check_invariant());

	const auto& result = find_global_symbol2(vm, name);
	if(result == nullptr){
		quark::throw_runtime_error(std::string() + "Cannot find global \"" + name.s + "\".");
	}
	else{
		return rt_to_value(vm._backend, result->_value);
	}
}

value_t call_function(interpreter_t& vm, const floyd::value_t& f, const std::vector<value_t>& args){
#if DEBUG
	QUARK_ASSERT(vm.check_invariant());
	QUARK_ASSERT(f.check_invariant());
	for(const auto& i: args){ QUARK_ASSERT(i.check_invariant()); };
	QUARK_ASSERT(f.is_function());
#endif

	const auto f2 = value_to_rt(vm._backend, f);
	std::vector<rt_value_t> args2;
	for(const auto& e: args){
		args2.push_back(value_to_rt(vm._backend, e));
	}

	const auto result = call_function_bc(vm, f2, &args2[0], static_cast<int>(args2.size()));
	return rt_to_value(vm._backend, result);
}




bc_program_t compile_to_bytecode(const compilation_unit_t& cu){
	const auto sem_ast = compile_to_sematic_ast__errors(cu);
	const auto bc = generate_bytecode(sem_ast);
	return bc;
}





//////////////////////////////////////		container_runner_t

/*
REFS:

std::packaged_task
*/

//	https://en.cppreference.com/w/cpp/thread/condition_variable/wait



struct bc_process_t;

struct bc_processes_runtime_t {
	container_t _container;
	runtime_handler_i* handler;
	std::map<std::string, process_def_t> _process_infos;
	std::thread::id _main_thread_id;

	std::vector<std::shared_ptr<bc_process_t>> _processes;
	std::vector<std::thread> _worker_threads;
};


//	NOTICE: Each process inbox has its own mutex + condition variable. No mutex protects cout.
struct bc_process_t : public runtime_process_i {
	bool check_invariant() const {
		QUARK_ASSERT(_init_function != nullptr);
		QUARK_ASSERT(_msg_function != nullptr);
		return true;
	}


	void runtime_process__on_send_message(const std::string& dest_process_id, const rt_pod_t& message0, const type_t& type) override {
		auto& backend = _interpreter->_backend;
		const auto& types = backend.types;
		const auto message = make_rt_value(backend, message0, type, rt_value_t::rc_mode::bump);

		const auto it = std::find_if(
			_owning_runtime->_processes.begin(),
			_owning_runtime->_processes.end(),
			[&](const std::shared_ptr<bc_process_t>& process){ return process->_name_key == dest_process_id; }
		);
		if(it == _owning_runtime->_processes.end()){
		}
		else{
			auto& dest_process = **it;

			if(message._type != dest_process._message_type){
				const auto send_message_type_str = type_to_compact_string(
					types,
					message._type,
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
				std::lock_guard<std::mutex> lk(dest_process._inbox_mutex);
				dest_process._inbox.push_front(message);
				if(k_trace_messaging){
//					QUARK_TRACE("Notifying...");
				}
			}
			dest_process._inbox_condition_variable.notify_one();
		//    dest_process._inbox_condition_variable.notify_all();
		}
	}

	void runtime_process__on_exit_process() override {
		_exiting_flag = true;
	}


	//////////////////////////////////////		STATE

	bc_processes_runtime_t* _owning_runtime;
	std::condition_variable _inbox_condition_variable;
	std::mutex _inbox_mutex;
	std::deque<rt_value_t> _inbox;

	std::string _name_key;
	std::thread::id _thread_id;

	std::shared_ptr<interpreter_t> _interpreter;
	std::shared_ptr<value_entry_t> _init_function;
	std::shared_ptr<value_entry_t> _msg_function;
	value_t _process_state;
	type_t _message_type;

	std::atomic<bool> _exiting_flag;
};


static std::string make_trace_process_header(const bc_process_t& process){
	const auto process_name = process._name_key;
	const auto os_thread_name = get_current_thread_name();

	const auto header = std::string("[") + process_name + ", OS thread: " + os_thread_name + " ]";
	return header;
}


static void process_process(bc_processes_runtime_t& runtime, int process_id){
	auto& process = *runtime._processes[process_id];

	QUARK_ASSERT(process.check_invariant());

	auto& backend = process._interpreter->_backend;
	const auto& types = backend.types;
	const auto trace_header = make_trace_process_header(process);

	{
		const std::vector<value_t> args = {};
		process._process_state = call_function(*process._interpreter, rt_to_value(backend, process._init_function->_value), args);
	}

	while(process._exiting_flag == false){
		rt_value_t message;
		{
			std::unique_lock<std::mutex> lk(process._inbox_mutex);

			if(k_trace_messaging){
				QUARK_TRACE_SS(trace_header << ": waiting......");
			}
			process._inbox_condition_variable.wait(lk, [&]{ return process._inbox.empty() == false; });

			//	Pop message.
			QUARK_ASSERT(process._inbox.empty() == false);
			message = process._inbox.back();
			process._inbox.pop_back();
		}
		if(k_trace_messaging){
			const auto v = rt_to_value(backend, message);
			const auto message2 = value_to_json(types, v);
			QUARK_TRACE_SS(trace_header << ": received message: " << json_to_pretty_string(message2));
		}

		{
			const std::vector<value_t> args = { process._process_state, rt_to_value(backend, message) };
			const auto f = rt_to_value(backend, process._msg_function->_value);
			const auto& state2 = call_function(*process._interpreter, f, args);
			process._process_state = state2;
		}
	}
}

//	NOTICE: Will not run the input VM, it makes new VMs for every thread run(!?)
//??? No need for args
static std::map<std::string, value_t> run_floyd_processes(const interpreter_t& vm, const std::vector<std::string>& args, const config_t& config){
	const auto& container_def = vm._imm->_program._container_def;

	if(container_def._clock_busses.empty()){
		return {};
	}
	else{
		bc_processes_runtime_t runtime;
		runtime.handler = vm._runtime_handler;
		runtime._main_thread_id = std::this_thread::get_id();
		runtime._container = container_def;
		runtime._process_infos = reduce(
			runtime._container._clock_busses,
			std::map<std::string, process_def_t>(),
			[](const std::map<std::string, process_def_t>& acc, const std::pair<std::string, clock_bus_t>& e){
				auto acc2 = acc;
				acc2.insert(e.second._processes.begin(), e.second._processes.end());
				return acc2;
			}
		);

		for(const auto& t: runtime._process_infos){
			auto process = std::make_shared<bc_process_t>();
			process->_exiting_flag = false;
			process->_message_type = t.second.msg_type;
			process->_owning_runtime = &runtime;
			process->_name_key = t.first;
			process->_interpreter = std::make_shared<interpreter_t>(vm._imm->_program, config, process.get(), *runtime.handler);
			process->_init_function = find_global_symbol2(*process->_interpreter, t.second.init_func_linkname);
			process->_msg_function = find_global_symbol2(*process->_interpreter, t.second.msg_func_linkname);

			if(process->_init_function == nullptr){
				throw std::runtime_error("Cannot link floyd init function: \"" + t.second.init_func_linkname.s + "\"");
			}
			if(process->_msg_function == nullptr){
				throw std::runtime_error("Cannot link floyd message function: \"" + t.second.msg_func_linkname.s + "\"");
			}

			runtime._processes.push_back(process);
		}

		//	Remember that current thread (main) is also a thread, no need to create a worker thread for that process.
		runtime._processes[0]->_thread_id = runtime._main_thread_id;

		for(int process_id = 1 ; process_id < runtime._processes.size() ; process_id++){
			runtime._worker_threads.push_back(
				std::thread([&](int process_id){
		//			const auto native_thread = thread::native_handle();

					std::stringstream thread_name;
					thread_name << std::string() << "process " << process_id << " thread";
					set_current_threads_name(thread_name.str());

					process_process(runtime, process_id);
				},
				process_id
				)
			);
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
	//	QUARK_VERIFY(runtime._processes[0]->_process_state.get_struct_value()->_member_values[0].get_int_value() == 998);
	}
}



static int64_t bc_call_main(interpreter_t& interpreter, const floyd::value_t& f, const std::vector<std::string>& main_args){
	QUARK_ASSERT(interpreter.check_invariant());
	QUARK_ASSERT(f.check_invariant());

	auto types = interpreter._imm->_program._types;

	//??? Check this earlier.
	if(f.get_type() == get_main_signature_arg_impure(types) || f.get_type() == get_main_signature_arg_pure(types)){
		const auto main_args2 = mapf<value_t>(main_args, [](auto& e){ return value_t::make_string(e); });
		const auto main_args3 = value_t::make_vector_value(types, type_t::make_string(), main_args2);
		const auto main_result = call_function(interpreter, f, { main_args3 });
		const auto main_result_int = main_result.get_int_value();
		return main_result_int;
	}
	else if(f.get_type() == get_main_signature_no_arg_impure(types) || f.get_type() == get_main_signature_no_arg_pure(types)){
		const auto main_result = call_function(interpreter, f, {});
		const auto main_result_int = main_result.get_int_value();
		return main_result_int;
	}
	else{
		throw std::exception();
	}
}

run_output_t run_program_bc(interpreter_t& vm, const std::vector<std::string>& main_args, const config_t& config){
	QUARK_ASSERT(vm.check_invariant());

	const auto& main_function = find_global_symbol2(vm, module_symbol_t("main"));
	if(main_function != nullptr){
		const auto main_result_int = bc_call_main(vm, rt_to_value(vm._backend, main_function->_value), main_args);
		return { main_result_int, {} };
	}
	else{
		const auto output = run_floyd_processes(vm, main_args, config);
		return run_output_t(0, output);
	}
}

std::vector<test_t> collect_tests(interpreter_t& vm){
	QUARK_ASSERT(vm.check_invariant());

	const auto& test_registry_bind = find_global_symbol2(vm, module_symbol_t(k_global_test_registry));
	QUARK_ASSERT(test_registry_bind != nullptr);

	const auto vec = rt_to_value(vm._backend, test_registry_bind->_value);
	const auto vec2 = vec.get_vector_value();
	std::vector<test_t> a = unpack_test_registry(vec2);
	return a;
}

static std::string run_test(interpreter_t& vm, const test_t& test){
	QUARK_ASSERT(vm.check_invariant());

	const auto function_id = test.f;

	const auto f_type = type_t::make_function(
		vm._imm->_program._types,
		type_t::make_void(),
		{},
		epure::pure
	);

	const auto f_value = rt_value_t::make_function_value(vm._backend, f_type, function_id);

	try {
		const std::vector<rt_value_t> args2;
		call_function_bc(vm, f_value, &args2[0], static_cast<int>(args2.size()));

		return "";
	}
	catch(const std::runtime_error& e){
		return std::string(e.what());
	}
	catch(const std::exception& e){
		return "std::exception";
	}
	catch(...){
		return "*** unknown exception ***";
	}
}

std::vector<test_result_t> run_tests_bc(
	interpreter_t& vm,
	const std::vector<test_t>& all_tests,
	const std::vector<test_id_t>& wanted
){
	QUARK_ASSERT(vm.check_invariant());

	std::vector<test_result_t> result;
	for(int index = 0 ; index < all_tests.size() ; index++){
		const auto& test = all_tests[index];

		const auto it = std::find_if(
			wanted.begin(),
			wanted.end(),
			[&] (const test_id_t& e) {
				return
					//??? check module in the future.
					e.function_name == test.test_id.function_name
					&& e.scenario == test.test_id.scenario
					;
			}
		);
		if(it != wanted.end()){
			const auto r = run_test(vm, test);
			const auto r2 = r == ""
				? test_result_t { test_result_t::type::success, "", test.test_id }
				: test_result_t { test_result_t::type::fail_with_string, r, test.test_id };
			result.push_back(r2);
		}
		else{
			result.push_back(test_result_t { test_result_t::type::not_run, "", test.test_id });
		}
	}

	return result;
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

