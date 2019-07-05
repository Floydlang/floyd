//
//  parser_evaluator.cpp
//  FloydSpeak
//
//  Created by Marcus Zetterquist on 26/07/16.
//  Copyright © 2016 Marcus Zetterquist. All rights reserved.
//

#include "floyd_interpreter.h"

#include "parser_primitives.h"
#include "floyd_parser.h"
#include "floyd_runtime.h"

#include "pass3.h"
#include "bytecode_host_functions.h"
#include "bytecode_generator.h"
#include "compiler_helpers.h"
#include "os_process.h"

#include <thread>
#include <deque>
#include <future>

#include <condition_variable>

namespace floyd {




//////////////////////////////////////		value_t -- helpers



std::vector<bc_value_t> values_to_bcs(const std::vector<value_t>& values){
	std::vector<bc_value_t> result;
	for(const auto& e: values){
		result.push_back(value_to_bc(e));
	}
	return result;
}

value_t bc_to_value(const bc_value_t& value){
	QUARK_ASSERT(value.check_invariant());

	const auto& type = value._type;
	const auto basetype = value._type.get_base_type();

	if(basetype == base_type::k_undefined){
		return value_t::make_undefined();
	}
	else if(basetype == base_type::k_any){
		return value_t::make_any();
	}
	else if(basetype == base_type::k_void){
		return value_t::make_void();
	}
	else if(basetype == base_type::k_bool){
		return value_t::make_bool(value.get_bool_value());
	}
	else if(basetype == base_type::k_int){
		return value_t::make_int(value.get_int_value());
	}
	else if(basetype == base_type::k_double){
		return value_t::make_double(value.get_double_value());
	}
	else if(basetype == base_type::k_string){
		return value_t::make_string(value.get_string_value());
	}
	else if(basetype == base_type::k_json_value){
		return value_t::make_json_value(value.get_json_value());
	}
	else if(basetype == base_type::k_typeid){
		return value_t::make_typeid_value(value.get_typeid_value());
	}
	else if(basetype == base_type::k_struct){
		const auto& members = value.get_struct_value();
		std::vector<value_t> members2;
		for(int i = 0 ; i < members.size() ; i++){
			const auto& member_value = members[i];
			const auto& member_value2 = bc_to_value(member_value);
			members2.push_back(member_value2);
		}
		return value_t::make_struct_value(type, members2);
	}
	else if(basetype == base_type::k_vector){
		const auto& element_type  = type.get_vector_element_type();
		std::vector<value_t> vec2;
		const bool vector_w_inplace_elements = encode_as_vector_w_inplace_elements(type);
		if(vector_w_inplace_elements){
			for(const auto e: value._pod._external->_vector_w_inplace_elements){
				vec2.push_back(bc_to_value(bc_value_t(element_type, e)));
			}
		}
		else{
			for(const auto& e: value._pod._external->_vector_w_external_elements){
				QUARK_ASSERT(e.check_invariant());
				vec2.push_back(bc_to_value(bc_value_t(element_type, e)));
			}
		}
		return value_t::make_vector_value(element_type, vec2);
	}
	else if(basetype == base_type::k_dict){
		const auto& value_type  = type.get_dict_value_type();
		const bool dict_w_inplace_values = encode_as_dict_w_inplace_values(type);
		std::map<std::string, value_t> entries2;
		if(dict_w_inplace_values){
			for(const auto& e: value._pod._external->_dict_w_inplace_values){
				entries2.insert({ e.first, bc_to_value(bc_value_t(value_type, e.second)) });
			}
		}
		else{
			for(const auto& e: value._pod._external->_dict_w_external_values){
				entries2.insert({ e.first, bc_to_value(bc_value_t(value_type, e.second)) });
			}
		}
		return value_t::make_dict_value(value_type, entries2);
	}
	else if(basetype == base_type::k_function){
		return value_t::make_function_value(type, value.get_function_value());
	}
	else{
		QUARK_ASSERT(false);
		quark::throw_exception();
	}
}

bc_value_t value_to_bc(const value_t& value){
	QUARK_ASSERT(value.check_invariant());

	const auto basetype = value.get_basetype();
	if(basetype == base_type::k_undefined){
		return bc_value_t::make_undefined();
	}
	else if(basetype == base_type::k_any){
		return bc_value_t::make_any();
	}
	else if(basetype == base_type::k_void){
		return bc_value_t::make_void();
	}
	else if(basetype == base_type::k_bool){
		return bc_value_t::make_bool(value.get_bool_value());
	}
	else if(basetype == base_type::k_bool){
		return bc_value_t::make_bool(value.get_bool_value());
	}
	else if(basetype == base_type::k_int){
		return bc_value_t::make_int(value.get_int_value());
	}
	else if(basetype == base_type::k_double){
		return bc_value_t::make_double(value.get_double_value());
	}

	else if(basetype == base_type::k_string){
		return bc_value_t::make_string(value.get_string_value());
	}
	else if(basetype == base_type::k_json_value){
		return bc_value_t::make_json_value(value.get_json_value());
	}
	else if(basetype == base_type::k_typeid){
		return bc_value_t::make_typeid_value(value.get_typeid_value());
	}
	else if(basetype == base_type::k_struct){
		return bc_value_t::make_struct_value(value.get_type(), values_to_bcs(value.get_struct_value()->_member_values));
	}

	else if(basetype == base_type::k_vector){
		const auto vector_type = value.get_type();
		const auto element_type = vector_type.get_vector_element_type();

		if(encode_as_vector_w_inplace_elements(vector_type)){
			const auto& vec = value.get_vector_value();
			immer::vector<bc_inplace_value_t> vec2;
			for(const auto& e: vec){
				const auto bc = value_to_bc(e);
				vec2.push_back(bc._pod._inplace);
			}
			return make_vector(element_type, vec2);
		}
		else{
			const auto& vec = value.get_vector_value();
			immer::vector<bc_external_handle_t> vec2;
			for(const auto& e: vec){
				const auto bc = value_to_bc(e);
				const auto hand = bc_external_handle_t(bc);
				vec2 = vec2.push_back(hand);
			}
			return make_vector(element_type, vec2);
		}
	}
	else if(basetype == base_type::k_dict){
		const auto dict_type = value.get_type();
		const auto value_type = dict_type.get_dict_value_type();

		const auto elements = value.get_dict_value();
		immer::map<std::string, bc_external_handle_t> entries2;

		if(encode_as_dict_w_inplace_values(dict_type)){
			QUARK_ASSERT(false);//??? fix
		}
		else{
			for(const auto& e: elements){
				entries2 = entries2.insert({e.first, bc_external_handle_t(value_to_bc(e.second))});
			}
		}
		return make_dict(value_type, entries2);
	}
	else if(basetype == base_type::k_function){
		return bc_value_t::make_function_value(value.get_type(), value.get_function_value());
	}
	else{
		QUARK_ASSERT(false);
		quark::throw_exception();
	}
}

//??? add tests!



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
	const auto pass3 = compile_to_sematic_ast__errors(cu);
	const auto bc = generate_bytecode(pass3);
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

	if(vm._print_output.empty() == false){
		QUARK_SCOPED_TRACE("print output:");
		for(const auto& line: vm._print_output){
			QUARK_TRACE_SS(line);
		}
	}
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
        QUARK_TRACE("Notifying...");
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

        	QUARK_TRACE_SS(thread_name << ": waiting......");
			process._inbox_condition_variable.wait(lk, [&]{ return process._inbox.empty() == false; });
        	QUARK_TRACE_SS(thread_name << ": continue");

			//	Pop message.
			QUARK_ASSERT(process._inbox.empty() == false);
			message = process._inbox.back();
			process._inbox.pop_back();
		}
		QUARK_TRACE_SS("RECEIVED: " << json_to_pretty_string(message));

		if(message.is_string() && message.get_string() == "stop"){
			stop = true;
        	QUARK_TRACE_SS(thread_name << ": STOP");
		}
		else{
			if(process._processor){
				process._processor->on_message(message);
			}

			if(process._process_function != nullptr){
				const std::vector<value_t> args = { process._process_state, value_t::make_json_value(message) };
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
	#ifdef __APPLE__
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
		return run_output_t(-1, output);
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

