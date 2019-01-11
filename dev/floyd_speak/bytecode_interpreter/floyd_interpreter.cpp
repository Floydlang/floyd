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

#include "pass3.h"
#include "host_functions.h"
#include "bytecode_generator.h"

#include <thread>
#include <deque>
#include <future>

#include <pthread.h>
#include <condition_variable>

namespace floyd {

using std::vector;
using std::string;
using std::pair;
using std::shared_ptr;
using std::make_shared;




//////////////////////////////////////		value_t -- helpers



vector<bc_value_t> values_to_bcs(const vector<value_t>& values){
	vector<bc_value_t> result;
	for(const auto& e: values){
		result.push_back(value_to_bc(e));
	}
	return result;
}

value_t bc_to_value(const bc_value_t& value){
	QUARK_ASSERT(value.check_invariant());

	const auto& type = value._type;
	const auto basetype = value._type.get_base_type();

	if(basetype == base_type::k_internal_undefined){
		return value_t::make_undefined();
	}
	else if(basetype == base_type::k_internal_dynamic){
		return value_t::make_internal_dynamic();
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
		vector<value_t> members2;
		for(int i = 0 ; i < members.size() ; i++){
			const auto& member_value = members[i];
			const auto& member_value2 = bc_to_value(member_value);
			members2.push_back(member_value2);
		}
		return value_t::make_struct_value(type, members2);
	}
	else if(basetype == base_type::k_protocol){
		QUARK_ASSERT(false);
		return value_t::make_undefined();
	}
	else if(basetype == base_type::k_vector){
		const auto& element_type  = type.get_vector_element_type();
		vector<value_t> vec2;
		if(element_type.is_bool()){
			for(const auto e: value._pod._ext->_vector_pod64){
				vec2.push_back(value_t::make_bool(e._bool));
			}
		}
		else if(element_type.is_int()){
			for(const auto e: value._pod._ext->_vector_pod64){
				vec2.push_back(value_t::make_int(e._int64));
			}
		}
		else if(element_type.is_double()){
			for(const auto e: value._pod._ext->_vector_pod64){
				vec2.push_back(value_t::make_double(e._double));
			}
		}
		else{
			for(const auto& e: value._pod._ext->_vector_objects){
				vec2.push_back(bc_to_value(bc_value_t(element_type, e)));
			}
		}
		return value_t::make_vector_value(element_type, vec2);
	}
	else if(basetype == base_type::k_dict){
		const auto& value_type  = type.get_dict_value_type();
		std::map<string, value_t> entries2;
		if(value_type.is_bool()){
			for(const auto& e: value._pod._ext->_dict_pod64){
				entries2.insert({ e.first, value_t::make_bool(e.second._bool) });
			}
		}
		else if(value_type.is_int()){
			for(const auto& e: value._pod._ext->_dict_pod64){
				entries2.insert({ e.first, value_t::make_int(e.second._int64) });
			}
		}
		else if(value_type.is_double()){
			for(const auto& e: value._pod._ext->_dict_pod64){
				entries2.insert({ e.first, value_t::make_double(e.second._double) });
			}
		}
		else{
			for(const auto& e: value._pod._ext->_dict_objects){
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
		throw std::exception();
	}
}

bc_value_t value_to_bc(const value_t& value){
	QUARK_ASSERT(value.check_invariant());

	const auto basetype = value.get_basetype();
	if(basetype == base_type::k_internal_undefined){
		return bc_value_t::make_undefined();
	}
	else if(basetype == base_type::k_internal_dynamic){
		return bc_value_t::make_internal_dynamic();
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
	else if(basetype == base_type::k_protocol){
		QUARK_ASSERT(false);
		return bc_value_t::make_undefined();
	}

	else if(basetype == base_type::k_vector){
		const auto vector_type = value.get_type();
		const auto element_type = vector_type.get_vector_element_type();

		if(encode_as_vector_pod64(vector_type)){
			const auto& vec = value.get_vector_value();
			immer::vector<bc_pod64_t> vec2;
			if(element_type.is_bool()){
				for(const auto& e: vec){
					vec2.push_back(bc_pod64_t{._bool = e.get_bool_value()});
				}
			}
			else if(element_type.is_int()){
				for(const auto& e: vec){
					vec2.push_back(bc_pod64_t{._int64 = e.get_int_value()});
				}
			}
			else if(element_type.is_double()){
				for(const auto& e: vec){
					vec2.push_back(bc_pod64_t{._double = e.get_double_value()});
				}
			}
			return make_vector_int64_value(element_type, vec2);
		}
		else{
			const auto& vec = value.get_vector_value();
			immer::vector<bc_object_handle_t> vec2;
			for(const auto& e: vec){
				const auto bc = value_to_bc(e);
				const auto hand = bc_object_handle_t(bc);
				vec2.push_back(hand);
			}
			return make_vector_value(element_type, vec2);
		}
	}
	else if(basetype == base_type::k_dict){
		const auto dict_type = value.get_type();
		const auto value_type = dict_type.get_dict_value_type();
//??? add handling for int, bool, double
		const auto elements = value.get_dict_value();
		immer::map<string, bc_object_handle_t> entries2;
		for(const auto& e: elements){
			entries2.insert({e.first, bc_object_handle_t(value_to_bc(e.second))});
		}
		return make_dict_value(value_type, entries2);
	}
	else if(basetype == base_type::k_function){
		return bc_value_t::make_function_value(value.get_type(), value.get_function_value());
	}
	else{
		QUARK_ASSERT(false);
		throw std::exception();
	}
}

//??? add tests!





#if 0
bc_value_t construct_value_from_typeid(interpreter_t& vm, const typeid_t& type, const typeid_t& arg0_type, const vector<bc_value_t>& arg_values){
	QUARK_ASSERT(vm.check_invariant());
	QUARK_ASSERT(type.check_invariant());

	if(type.is_json_value()){
		QUARK_ASSERT(arg_values.size() == 1);

		const auto& arg0 = arg_values[0];
		const auto arg = bc_to_value(arg0, arg0_type);
		const auto value = value_to_ast_json(arg, json_tags::k_plain);
		return bc_value_t::make_json_value(value._value);
	}
	else if(type.is_bool() || type.is_int() || type.is_double() || type.is_string() || type.is_typeid()){
		QUARK_ASSERT(arg_values.size() == 1);

		const auto& arg = arg_values[0];
		if(type.is_string()){
			if(arg0_type.is_json_value() && arg.get_json_value().is_string()){
				return bc_value_t::make_string(arg.get_json_value().get_string());
			}
			else if(arg0_type.is_string()){
			}
		}
		else{
			if(arg0_type != type){
			}
		}
		return arg;
	}
	else if(type.is_struct()){
/*
	#if DEBUG
		const auto def = type.get_struct_ref();
		QUARK_ASSERT(arg_values.size() == def->_members.size());

		for(int i = 0 ; i < def->_members.size() ; i++){
			const auto v = arg_values[i];
			const auto a = def->_members[i];
			QUARK_ASSERT(v.check_invariant());
			QUARK_ASSERT(v.get_type().get_base_type() != base_type::k_internal_unresolved_type_identifier);
			QUARK_ASSERT(v.get_type() == a._type);
		}
	#endif
*/
		const auto instance = bc_value_t::make_struct_value(type, arg_values);
//		QUARK_TRACE(to_compact_string2(instance));

		return instance;
	}
	else if(type.is_vector()){
		const auto& element_type = type.get_vector_element_type();
		QUARK_ASSERT(element_type.is_undefined() == false);

		return bc_value_t::make_vector_value(element_type, arg_values);
	}
	else if(type.is_dict()){
		const auto& element_type = type.get_dict_value_type();
		QUARK_ASSERT(element_type.is_undefined() == false);

		std::map<string, bc_value_t> m;
		for(auto i = 0 ; i < arg_values.size() / 2 ; i++){
			const auto& key = arg_values[i * 2 + 0].get_string_value();
			const auto& value = arg_values[i * 2 + 1];
			m.insert({ key, value });
		}
		return bc_value_t::make_dict_value(element_type, m);
	}
	else if(type.is_function()){
	}
	else if(type.is_unresolved_type_identifier()){
	}
	else{
	}

	QUARK_ASSERT(false);
	throw std::exception();
}
#endif


floyd::value_t find_global_symbol(const interpreter_t& vm, const string& s){
	QUARK_ASSERT(vm.check_invariant());

	return get_global(vm, s);
}

value_t get_global(const interpreter_t& vm, const string& name){
	QUARK_ASSERT(vm.check_invariant());

	const auto& result = find_global_symbol2(vm, name);
	if(result == nullptr){
		throw std::runtime_error(std::string() + "Cannot find global \"" + name + "\".");
	}
	else{
		return bc_to_value(result->_value);
	}
}

value_t call_function(interpreter_t& vm, const floyd::value_t& f, const vector<value_t>& args){
#if DEBUG
	QUARK_ASSERT(vm.check_invariant());
	QUARK_ASSERT(f.check_invariant());
	for(const auto& i: args){ QUARK_ASSERT(i.check_invariant()); };
	QUARK_ASSERT(f.is_function());
#endif

	const auto f2 = value_to_bc(f);
	vector<bc_value_t> args2;
	for(const auto& e: args){
		args2.push_back(value_to_bc(e));
	}

	const auto result = call_function_bc(vm, f2, &args2[0], static_cast<int>(args2.size()));
	return bc_to_value(result);
}

bc_program_t compile_to_bytecode(const interpreter_context_t& context, const string& program){
	parser_context_t context2{ quark::trace_context_t(context._tracer._verbose, context._tracer._tracer) };
//	parser_context_t context{ quark::make_default_tracer() };
//	QUARK_CONTEXT_TRACE(context._tracer, "Hello");


#if 1
const auto pref = k_tiny_prefix;
const auto p = pref + "\n" + program;
#else
const auto p = program;
#endif

	const auto& pass1 = parse_program2(context2, p);
	const auto& pass2 = json_to_ast(context2._tracer, pass1);
	const auto& pass3 = run_semantic_analysis(context2._tracer, pass2);
	const auto bc = generate_bytecode(context2._tracer, pass3);

	return bc;
}

std::pair<std::shared_ptr<interpreter_t>, value_t> run_program(const interpreter_context_t& context, const bc_program_t& program, const vector<floyd::value_t>& args){
	auto vm = make_shared<interpreter_t>(program);

	const auto& main_func = find_global_symbol2(*vm, "main");
	if(main_func != nullptr){
		const auto& r = call_function(*vm, bc_to_value(main_func->_value), args);
		return { vm, r };
	}
	else{
		return { vm, value_t::make_undefined() };
	}
}

std::shared_ptr<interpreter_t> run_global(const interpreter_context_t& context, const string& source){
	auto program = compile_to_bytecode(context, source);
	auto vm = make_shared<interpreter_t>(program);
//	QUARK_TRACE(json_to_pretty_string(interpreter_to_json(vm)));
	print_vm_printlog(*vm);
	return vm;
}

std::pair<std::shared_ptr<interpreter_t>, value_t> run_main(const interpreter_context_t& context, const string& source, const vector<floyd::value_t>& args){
	auto program = compile_to_bytecode(context, source);

	//	Runs global code.
	auto interpreter = make_shared<interpreter_t>(program);

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
		std::cout << "print output:\n";
		for(const auto& line: vm._print_output){
			std::cout << line << "\n";
		}
	}
}




//////////////////////////////////////		container_runner_t



/*

std::packaged_task
*/




//	https://en.cppreference.com/w/cpp/thread/condition_variable/wait

string get_current_thread_name(){
	char name[16];

	//pthread_setname_np(pthread_self(), s.c_str()); // set the name (pthread_self() returns the pthread_t of the current thread)
	pthread_getname_np(pthread_self(), &name[0], sizeof(name));
	if(strlen(name) == 0){
		return "main";
	}
	else{
		return string(name);
	}
}


struct process_interface {
	virtual ~process_interface(){};
	virtual void on_message(const json_t& message) = 0;
	virtual void on_init() = 0;
};


//	NOTICE: Each process inbox has its own mutex + condition variable. No mutex protects cout.
struct process_t {
	std::condition_variable _inbox_condition_variable;
	std::mutex _inbox_mutex;
	std::deque<json_t> _inbox;

	string _name_key;
	string _function_key;
	std::thread::id _thread_id;

	std::shared_ptr<interpreter_t> _interpreter;
	std::shared_ptr<value_entry_t> _init_function;
	std::shared_ptr<value_entry_t> _process_function;
	value_t _process_state;


	std::shared_ptr<process_interface> _processor;
};

struct process_runtime_t {
	container_t _container;
	std::map<std::string, std::string> _process_infos;
	std::thread::id _main_thread_id;

	vector<std::shared_ptr<process_t>> _processes;
	vector<std::thread> _worker_threads;
};

/*
??? have ONE runtime PER computer or one per interpreter?
??? Separate system-interpreter (all processes and many clock busses) vs ONE thread of execution?
*/

void send_message(process_runtime_t& runtime, int process_id, const json_t& message){
	auto& process = *runtime._processes[process_id];

    {
        std::lock_guard<std::mutex> lk(process._inbox_mutex);
        process._inbox.push_front(message);
        std::cout << "Notifying...\n";
    }
    process._inbox_condition_variable.notify_one();
//    process._inbox_condition_variable.notify_all();
}

void process_process(process_runtime_t& runtime, int process_id){
	auto& process = *runtime._processes[process_id];
	bool stop = false;

	const auto thread_name = get_current_thread_name();

	if(process._processor){
		process._processor->on_init();
	}

	if(process._init_function != nullptr){
		const vector<value_t> args = {};
		process._process_state = call_function(*process._interpreter, bc_to_value(process._init_function->_value), args);
	}

	while(stop == false){
		json_t message;
		{
			std::unique_lock<std::mutex> lk(process._inbox_mutex);

			std::cout << thread_name << ": waiting... \n";
			process._inbox_condition_variable.wait(lk, [&]{ return process._inbox.empty() == false; });
			std::cout << thread_name << ": continue... \n";

			//	Pop message.
			QUARK_ASSERT(process._inbox.empty() == false);
			message = process._inbox.back();
			process._inbox.pop_back();
		}
		QUARK_TRACE_SS("RECEIVED: " << json_to_pretty_string(message));

		if(message.is_string() && message.get_string() == "stop"){
			stop = true;
			std::cout << thread_name << ": STOP \n";
		}
		else{
			if(process._processor){
				process._processor->on_message(message);
			}

			if(process._process_function != nullptr){
				const vector<value_t> args = { process._process_state, value_t::make_json_value(message) };
				const auto& state2 = call_function(*process._interpreter, bc_to_value(process._process_function->_value), args);
				process._process_state = state2;
			}
		}
	}
}

std::map<std::string, value_t> run_container(const interpreter_context_t& context, const string& source, const vector<floyd::value_t>& args, const std::string& container_key){
	process_runtime_t runtime;
	runtime._main_thread_id = std::this_thread::get_id();

	auto program = compile_to_bytecode(context, source);
	if(program._software_system._name == ""){
		throw std::exception();
	}
	runtime._container = program._software_system._containers.at(container_key);
	runtime._process_infos = fold(runtime._container._clock_busses, std::map<std::string, std::string>(), [](const std::map<std::string, std::string>& acc, const pair<string, clock_bus_t>& e){
		auto acc2 = acc;
		acc2.insert(e.second._processes.begin(), e.second._processes.end());
		return acc2;
	});

	struct my_interpreter_handler_t : public interpreter_handler_i {
		my_interpreter_handler_t(process_runtime_t& runtime) : _runtime(runtime) {}

		virtual void on_send(const std::string& process_id, const json_t& message){
			const auto it = std::find_if(_runtime._processes.begin(), _runtime._processes.end(), [&](const std::shared_ptr<process_t>& process){ return process->_name_key == process_id; });
			if(it != _runtime._processes.end()){
				const auto process_index = it - _runtime._processes.begin();
				send_message(_runtime, static_cast<int>(process_index), message);
			}
		}

		process_runtime_t& _runtime;
	};
	auto my_interpreter_handler = my_interpreter_handler_t{runtime};


	for(const auto& t: runtime._process_infos){
		auto process = std::make_shared<process_t>();
		process->_name_key = t.first;
		process->_function_key = t.second;
		process->_interpreter = make_shared<interpreter_t>(program, &my_interpreter_handler);
		process->_init_function = find_global_symbol2(*process->_interpreter, t.second + "__init");
		process->_process_function = find_global_symbol2(*process->_interpreter, t.second);

		runtime._processes.push_back(process);
	}

/*
	struct my_processor : public process_interface {
		my_processor(process_runtime_t& runtime) : _runtime(runtime) {}

		process_runtime_t& _runtime;

		virtual void on_message(const json_t& message){
			if(message.is_string() && message.get_string() == "hello me!"){
				send_message(_runtime, 0, json_t("stop"));
				send_message(_runtime, 1, json_t("stop"));
				send_message(_runtime, 2, json_t("stop"));
				send_message(_runtime, 3, json_t("stop"));
			}
			else{
				QUARK_ASSERT(false);
			}
		}
		virtual void on_init(){
    		std::this_thread::sleep_for(std::chrono::seconds(1));
			send_message(_runtime, 1, json_t("hello!"));
    		std::this_thread::sleep_for(std::chrono::seconds(1));
			send_message(_runtime, 2, json_t("hello me!"));
		}
	};

	runtime._processes[2]->_processor = std::make_shared<my_processor>(runtime);
*/


	//	Remember that current thread (main) is also a thread, no need to create a worker thread for one process.
	runtime._processes[0]->_thread_id = runtime._main_thread_id;

	for(int process_id = 1 ; process_id < runtime._processes.size() ; process_id++){
		runtime._worker_threads.push_back(std::thread([&](int process_id){

//			const auto native_thread = thread::native_handle();

			std::stringstream thread_name;
			thread_name << string() << "process " << process_id << " thread";
			pthread_setname_np(/*pthread_self(),*/ thread_name.str().c_str());

			process_process(runtime, process_id);
		}, process_id));
	}

/*
	send_message(runtime, 0, json_t("inc"));
	send_message(runtime, 0, json_t("inc"));
	send_message(runtime, 0, json_t("dec"));
	send_message(runtime, 0, json_t("inc"));
	send_message(runtime, 0, json_t("stop"));
*/

	process_process(runtime, 0);

	for(auto &t: runtime._worker_threads){
		t.join();
	}

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
//	QUARK_UT_VERIFY(runtime._processes[0]->_process_state.get_struct_value()->_member_values[0].get_int_value() == 998);
}




}	//	floyd
