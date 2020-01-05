//
//  parser_evaluator.cpp
//  Floyd
//
//  Created by Marcus Zetterquist on 26/07/16.
//  Copyright © 2016 Marcus Zetterquist. All rights reserved.
//

#include "bytecode_execution_engine.h"

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

/*
REFS:

std::packaged_task

https://en.cppreference.com/w/cpp/thread/condition_variable/wait
*/

static void send(bc_process_t& process, const std::string& dest_process_id, const rt_pod_t& message0, const type_t& type);

//??? use process_def_t as member
//	NOTICE: Each process inbox has its own mutex + condition variable. No mutex protects cout.
struct bc_process_t : public runtime_process_i {
	bool check_invariant() const {
		QUARK_ASSERT(_ee != nullptr);
		QUARK_ASSERT(_name_key.empty() == false);
		QUARK_ASSERT(_bc_thread && _bc_thread->check_invariant());

		QUARK_ASSERT(_init_function.check_invariant());
		QUARK_ASSERT(_init_function._type.is_undefined() == false);

		QUARK_ASSERT(_msg_function.check_invariant());
		QUARK_ASSERT(_msg_function._type.is_undefined() == false);
		QUARK_ASSERT(_message_type.check_invariant());

		QUARK_ASSERT(_process_state.check_invariant());
		return true;
	}

	void runtime_process__on_send_message(const std::string& dest_process_id, const rt_pod_t& message0, const type_t& type) override {
		QUARK_ASSERT(check_invariant());

		send(*this, dest_process_id, message0, type);

		QUARK_ASSERT(check_invariant());
	}

	void runtime_process__on_exit_process() override {
		QUARK_ASSERT(check_invariant());

		_exiting_flag = true;
	}


	//////////////////////////////////////		STATE

	bc_execution_engine_t* _ee;
	std::condition_variable _inbox_condition_variable;
	std::mutex _inbox_mutex;
	std::deque<rt_value_t> _inbox;

	std::string _name_key;
	interpreter_t* _bc_thread;

	rt_value_t _init_function;
	rt_value_t _msg_function;
	rt_value_t _process_state;
	type_t _message_type;

	std::atomic<bool> _exiting_flag;
};





static interpreter_t& get_main_interpreter(bc_execution_engine_t& ee){
	QUARK_ASSERT(ee.check_invariant());

	return ee.main_bc_thread;
}


static value_t call_function_values(interpreter_t& vm, const value_t& f, const std::vector<value_t>& args){
	QUARK_ASSERT(vm.check_invariant());
	QUARK_ASSERT(f.check_invariant());
QUARK_ASSERT(f.is_function());
#if DEBUG
	for(const auto& i: args){ QUARK_ASSERT(i.check_invariant()); };
#endif

	const auto f2 = value_to_rt(vm._backend, f);
	std::vector<rt_value_t> args2;
	for(const auto& e: args){
		args2.push_back(value_to_rt(vm._backend, e));
	}

	const auto result = call_function_bc(vm, f2, &args2[0], static_cast<int>(args2.size()));
	return rt_to_value(vm._backend, result);
}

rt_value_t load_global(bc_execution_engine_t& ee, const module_symbol_t& name){
	QUARK_ASSERT(ee.check_invariant());

	return load_global(get_main_interpreter(ee), name);
}

value_t call_function(bc_execution_engine_t& ee, const value_t& f, const std::vector<value_t>& args){
	QUARK_ASSERT(ee.check_invariant());
	QUARK_ASSERT(f.check_invariant());
	QUARK_ASSERT(f.is_function());

	return call_function_values(get_main_interpreter(ee), f, args);
}



//////////////////////////////////////		bc_process_t





static void send(bc_process_t& process, const std::string& dest_process_id, const rt_pod_t& message0, const type_t& type){
	QUARK_ASSERT(process.check_invariant());
	QUARK_ASSERT(type.check_invariant());

	auto& backend = process._ee->backend;
	const auto& types = backend.types;
	const auto message = make_rt_value(backend, message0, type, rt_value_t::rc_mode::bump);

	const auto it = std::find_if(
		process._ee->_processes.begin(),
		process._ee->_processes.end(),
		[&](const std::shared_ptr<bc_process_t>& process){ return process->_name_key == dest_process_id; }
	);
	if(it == process._ee->_processes.end()){
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


//////////////////////////////////////		bc_execution_engine_t


void unwind_global_stack(bc_execution_engine_t& ee){
	QUARK_ASSERT(ee.check_invariant());

	get_main_interpreter(ee).unwind_stack();

	QUARK_ASSERT(ee.check_invariant());
}

bool bc_execution_engine_t::check_invariant() const {
	QUARK_ASSERT(check_invariant_thread_safe());

	QUARK_ASSERT(main_bc_thread.check_invariant());
/*
	//	Warning, cannot check invariant of this data since other OS threads mutate them concurrently.
	for(const auto& e: _processes){
		QUARK_ASSERT(e && e->check_invariant());
	}
*/
	return true;
}

bool bc_execution_engine_t::check_invariant_thread_safe() const {
	QUARK_ASSERT(_program->check_invariant());
	QUARK_ASSERT(backend.check_invariant());
	QUARK_ASSERT(handler != nullptr);
	QUARK_ASSERT(_main_thread_id != std::thread::id());
	for(const auto& e: _bc_threads){
		//	Warning, cannot check invariant of this data since other OS threads mutate them concurrently.
		QUARK_ASSERT(e && e->check_invariant_thread_safe());
	}
	return true;
}

bc_execution_engine_t::bc_execution_engine_t(const bc_program_t& program, const config_t& config, runtime_handler_i& runtime_handler) :
	_program(std::make_shared<bc_program_t>(program)),
	backend(link_functions(*_program), bc_make_struct_layouts(_program->_types), _program->_types, config),
	main_bc_thread(_program, backend, config, nullptr, runtime_handler, "main"),
	_main_thread_id(std::this_thread::get_id()),
	handler(&runtime_handler),
	_processes(),
	_bc_threads(),
	_os_threads()
{
	QUARK_ASSERT(program.check_invariant());

	QUARK_ASSERT(check_invariant());
}

std::unique_ptr<bc_execution_engine_t> make_bytecode_execution_engine(
	const bc_program_t& program,
	const config_t& config,
	runtime_handler_i& runtime_handler
){
	return std::make_unique<bc_execution_engine_t>(program, config, runtime_handler);
}

static std::string make_trace_process_header(const bc_process_t& process){
	QUARK_ASSERT(process.check_invariant());

	const auto process_name = process._name_key;
	const auto os_thread_name = get_current_thread_name();

	const auto header = std::string("[") + process_name + ", OS thread: " + os_thread_name + " ]";
	return header;
}

static void run_process(bc_execution_engine_t& ee, int process_id){
	QUARK_ASSERT(ee.check_invariant_thread_safe());
	QUARK_ASSERT(process_id >= 0 && process_id < ee._processes.size());

	auto& process = *ee._processes[process_id];

	QUARK_ASSERT(process.check_invariant());

	auto& backend = ee.backend;
	const auto& types = backend.types;
	const auto trace_header = make_trace_process_header(process);

	auto& interpreter = *process._bc_thread;

	//	Call process init()
	{
		process._process_state = call_function_bc(interpreter, process._init_function, {}, 0);
	}

	QUARK_ASSERT(ee.check_invariant_thread_safe());

	//	Handle process messages until exit.
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

		QUARK_ASSERT(ee.check_invariant_thread_safe());

		{
			const rt_value_t args[] = { process._process_state, message };
			process._process_state = call_function_bc(interpreter, process._msg_function, args, 2);
		}

		QUARK_ASSERT(ee.check_invariant_thread_safe());
	}

	QUARK_ASSERT(ee.check_invariant_thread_safe());
}

static void run_floyd_processes(bc_execution_engine_t& ee, const config_t& config){
	QUARK_ASSERT(ee.check_invariant());

	const auto& container_def = ee._program->_container_def;
	std::map<std::string, process_def_t> process_infos = reduce(
		container_def._clock_busses,
		std::map<std::string, process_def_t>(),
		[&](const std::map<std::string, process_def_t>& acc, const std::pair<std::string, clock_bus_t>& e){
			auto acc2 = acc;
			acc2.insert(e.second._processes.begin(), e.second._processes.end());
			return acc2;
		}
	);

	const auto process_infos2 = std::vector<std::pair<std::string, process_def_t>>(process_infos.begin(), process_infos.end());

	if(process_infos2.empty() == false){
		for(int process_id = 0 ; process_id < process_infos2.size() ; process_id++){
			const auto& t = process_infos2[process_id];

			auto process = std::make_shared<bc_process_t>();

			//	Floyd process 0 is run on the main OS/BC thread.
			if(process_id == 0){
				auto bc_thread = &ee.main_bc_thread;

				bc_thread->_process_handler = process.get();

				//??? dup
				process->_exiting_flag = false;
				process->_message_type = t.second.msg_type;
				process->_ee = &ee;
				process->_name_key = t.first;
				process->_bc_thread = bc_thread;
				process->_init_function = load_global(*process->_bc_thread, t.second.init_func_linkname);
				process->_msg_function = load_global(*process->_bc_thread, t.second.msg_func_linkname);

				if(process->_init_function._type.is_undefined()){
					throw std::runtime_error("Cannot link floyd init function: \"" + t.second.init_func_linkname.s + "\"");
				}
				if(process->_msg_function._type.is_undefined()){
					throw std::runtime_error("Cannot link floyd message function: \"" + t.second.msg_func_linkname.s + "\"");
				}

				ee._processes.push_back(process);
			}

			//	All other processes gets their own interpreter_t and OS thread.
			else{
				std::stringstream name_ss;
				name_ss << "floyd process " << process_id << " " << t.first;
				const auto name = name_ss.str();

				auto bc_thread = std::make_shared<interpreter_t>(ee._program, ee.backend, config, process.get(), *ee.handler, name);
				ee._bc_threads.push_back(bc_thread);

				//??? dup
				process->_exiting_flag = false;
				process->_message_type = t.second.msg_type;
				process->_ee = &ee;
				process->_name_key = t.first;
				process->_bc_thread = bc_thread.get();
				process->_init_function = load_global(*process->_bc_thread, t.second.init_func_linkname);
				process->_msg_function = load_global(*process->_bc_thread, t.second.msg_func_linkname);

				if(process->_init_function._type.is_undefined()){
					throw std::runtime_error("Cannot link floyd init function: \"" + t.second.init_func_linkname.s + "\"");
				}
				if(process->_msg_function._type.is_undefined()){
					throw std::runtime_error("Cannot link floyd message function: \"" + t.second.msg_func_linkname.s + "\"");
				}

				ee._processes.push_back(process);

				ee._os_threads.push_back(
					std::thread(
						[&](int process_id){
							set_current_threads_name(name);

							run_process(ee, process_id);
						},
						process_id
					)
				);
			}
		}

		//	Run process 0 using main thread.
		run_process(ee, 0);

		for(auto &t: ee._os_threads){
			t.join();
		}

		//	Delete the processes so we free up their resources and RCs
		ee._processes.clear();
	}

	QUARK_ASSERT(ee.check_invariant());
}

//??? Check main prototype earlier in pipeline!
static int64_t bc_call_main(bc_execution_engine_t& ee, const rt_value_t& f, const std::vector<std::string>& main_args){
	QUARK_ASSERT(ee.check_invariant());
	QUARK_ASSERT(f.check_invariant());
	QUARK_ASSERT(f._type.is_function());

	auto& interpreter = get_main_interpreter(ee);
	auto types = interpreter._program->_types;

	//	int main([string] args) impure
	if(f._type == get_main_signature_arg_impure(types) || f._type == get_main_signature_arg_pure(types)){
		const auto main_args2 = make_string_vector(ee.backend, main_args);

		const rt_value_t args[] = { main_args2 };
		const auto main_result = call_function_bc(interpreter, f, args, 1);
		const auto main_result_int = main_result.get_int_value();

		QUARK_ASSERT(ee.check_invariant());
		return main_result_int;
	}

	//	int main() impure
	else if(f._type == get_main_signature_no_arg_impure(types) || f._type == get_main_signature_no_arg_pure(types)){
		const auto main_result = call_function_bc(interpreter, f, {}, 0);
		const auto main_result_int = main_result.get_int_value();

		QUARK_ASSERT(ee.check_invariant());
		return main_result_int;
	}
	else{
		throw std::exception();
	}
}

run_output_t run_program_bc(bc_execution_engine_t& ee, const std::vector<std::string>& main_args, const config_t& config){
	QUARK_ASSERT(ee.check_invariant());

	const auto& main_function = load_global(get_main_interpreter(ee), module_symbol_t("main"));
	if(main_function._type.is_undefined() == false){
		const auto main_result_int = bc_call_main(ee, main_function, main_args);

		QUARK_ASSERT(ee.check_invariant());
		return { main_result_int, {} };
	}
	else{
		run_floyd_processes(ee, config);

		QUARK_ASSERT(ee.check_invariant());
		return run_output_t(0, {});
	}
}

std::vector<test_t> collect_tests(bc_execution_engine_t& ee){
	QUARK_ASSERT(ee.check_invariant());

	const auto& test_registry_bind = load_global(get_main_interpreter(ee), module_symbol_t(k_global_test_registry));
	QUARK_ASSERT(test_registry_bind._type.is_undefined() == false);

	const auto vec = rt_to_value(get_main_interpreter(ee)._backend, test_registry_bind);
	const auto vec2 = vec.get_vector_value();
	std::vector<test_t> a = unpack_test_registry(vec2);

	QUARK_ASSERT(ee.check_invariant());
	return a;
}

static std::string run_test(bc_execution_engine_t& ee, const test_t& test){
	QUARK_ASSERT(ee.check_invariant());

	const auto function_id = test.f;

	const auto f_type = type_t::make_function(
		ee._program->_types,
		type_t::make_void(),
		{},
		epure::pure
	);

	const auto f_value = rt_value_t::make_function_value(get_main_interpreter(ee)._backend, f_type, function_id);

	try {
		const std::vector<rt_value_t> args2;
		call_function_bc(get_main_interpreter(ee), f_value, &args2[0], static_cast<int>(args2.size()));
		QUARK_ASSERT(ee.check_invariant());

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
	bc_execution_engine_t& ee,
	const std::vector<test_t>& all_tests,
	const std::vector<test_id_t>& wanted
){
	QUARK_ASSERT(ee.check_invariant());

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
			const auto r = run_test(ee, test);
			const auto r2 = r == ""
				? test_result_t { test_result_t::type::success, "", test.test_id }
				: test_result_t { test_result_t::type::fail_with_string, r, test.test_id };
			result.push_back(r2);
		}
		else{
			result.push_back(test_result_t { test_result_t::type::not_run, "", test.test_id });
		}
	}

	QUARK_ASSERT(ee.check_invariant());

	return result;
}


bc_program_t compile_to_bytecode(const compilation_unit_t& cu){
	const auto sem_ast = compile_to_sematic_ast__errors(cu);
	const auto bc = generate_bytecode(sem_ast);

	QUARK_ASSERT(bc.check_invariant());
	return bc;
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
