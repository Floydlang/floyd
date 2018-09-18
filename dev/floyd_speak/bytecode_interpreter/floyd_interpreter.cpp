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
		const auto& struct_def = type.get_struct();
		const auto& members = value.get_struct_value();
		vector<value_t> members2;
		for(int i = 0 ; i < members.size() ; i++){
			const auto& member_type = struct_def._members[i]._type;
			const auto& member_value = members[i];
			const auto& member_value2 = bc_to_value(member_value);
			members2.push_back(member_value2);
		}
		return value_t::make_struct_value(type, members2);
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
		throw std::runtime_error("Cannot find global.");
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

	const auto& pass1 = parse_program2(context2, program);
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




#include <thread>
#incude <cstddef>

// Can't be used, they are compiler-time constants.
std::hardware_destructive_interference_size, std::hardware_constructive_interference_size




const auto a = hardware_info_t {
	._l1_cache_line_size = xxx,
	._scalar_align = alignof(std::max_align_t),
	._hardware_thread_count = hardware_concurrency()
};



https://www.unix.com/man-page/osx/3/sysconf/

https://www.freebsd.org/cgi/man.cgi?query=sysctlbyname&apropos=0&sektion=0&manpath=FreeBSD+10.1-RELEASE&arch=default&format=html

*/



/*
	Model Name:	iMac
	Model Identifier:	iMac15,1
	Processor Name:	Intel Core i7
	Processor Speed:	4 GHz
	Number of Processors:	1
	Total Number of Cores:	4
	L2 Cache (per Core):	256 KB
	L3 Cache:	8 MB
	Memory:	16 GB
	Boot ROM Version:	IM151.0217.B00
	SMC Version (system):	2.23f11
	Serial Number (system):	C02NR292FY14
	Hardware UUID:	6CC8669B-7ADB-583F-BEB7-251C0199F0EE
*/

/*
sysctl -n machdep.cpu.brand_string
Intel(R) Core(TM) i7-4790K CPU @ 4.00GHz

https://ark.intel.com/products/80807/Intel-Core-i7-4790K-Processor-8M-Cache-up-to-4_40-GHz

*/

struct hardware_info_t {
	std::size_t _l1_cacheline_size;

	//	Usually depends on "long double".
	std::size_t _scalar_align;
	unsigned int _hardware_thread_count;
};

hardware_info_t read_hardware_info(){
	return {
		._l1_cacheline_size = 64,
		._scalar_align = 16,
		._hardware_thread_count = 4 * 2,
	};
}



#if 0
https://en.cppreference.com/w/cpp/thread/condition_variable/wait

std::condition_variable cv;
std::mutex cv_m; // This mutex is used for three purposes:
                 // 1) to synchronize accesses to i
                 // 2) to synchronize accesses to std::cerr
                 // 3) for the condition variable cv
int i = 0;
	
void waits()
{
    std::unique_lock<std::mutex> lk(cv_m);
    std::cerr << "Waiting... \n";
    cv.wait(lk, []{return i == 1;});
    std::cerr << "...finished waiting. i == 1\n";
}
	
void signals()
{
    std::this_thread::sleep_for(std::chrono::seconds(1));
    {
        std::lock_guard<std::mutex> lk(cv_m);
        std::cerr << "Notifying...\n";
    }
    cv.notify_all();
	
    std::this_thread::sleep_for(std::chrono::seconds(1));
	
    {
        std::lock_guard<std::mutex> lk(cv_m);
        i = 1;
        std::cerr << "Notifying again...\n";
    }
    cv.notify_all();
}
	
int main()
{
    std::thread t1(waits), t2(waits), t3(waits), t4(signals);
    t1.join();
    t2.join();
    t3.join();
    t4.join();
}

#endif

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


struct actor_processor_interface {
	virtual ~actor_processor_interface(){};
	virtual void on_message(const json_t& message) = 0;
	virtual void on_init() = 0;
};


//	NOTICE: Each actor inbox has its own mutex + condition variable. No mutex protects cout.
struct actor_t {
	std::condition_variable _inbox_condition_variable;
	std::mutex _inbox_mutex;
	std::deque<json_t> _inbox;

	string _name_key;
	string _function_key;
	std::thread::id _thread_id;

	std::shared_ptr<interpreter_t> _interpreter;
	std::shared_ptr<value_entry_t> _init_function;
	std::shared_ptr<value_entry_t> _process_function;
	value_t _actor_state;


	std::shared_ptr<actor_processor_interface> _processor;
};

struct actor_runtime_t {
	container_t _container;
	std::map<std::string, std::string> _actor_infos;
	std::thread::id _main_thread_id;

	vector<std::shared_ptr<actor_t>> _actors;
	vector<std::thread> _worker_threads;
};

/*
??? have ONE runtime PER computer or one per interpreter?
??? Separate system-interpreter (all actors and many clock busses) vs ONE thread of execution?
*/

void send_message(actor_runtime_t& runtime, int actor_id, const json_t& message){
	auto& actor = *runtime._actors[actor_id];

    {
        std::lock_guard<std::mutex> lk(actor._inbox_mutex);
        actor._inbox.push_front(message);
        std::cout << "Notifying...\n";
    }
    actor._inbox_condition_variable.notify_one();
//    actor._inbox_condition_variable.notify_all();
}

void process_actor(actor_runtime_t& runtime, int actor_id){
	auto& actor = *runtime._actors[actor_id];
	bool stop = false;

	const auto thread_name = get_current_thread_name();

	if(actor._processor){
		actor._processor->on_init();
	}

	if(actor._init_function != nullptr){
		const vector<value_t> args = {};
		actor._actor_state = call_function(*actor._interpreter, bc_to_value(actor._init_function->_value), args);
	}

	while(stop == false){
		json_t message;
		{
			std::unique_lock<std::mutex> lk(actor._inbox_mutex);

			std::cout << thread_name << ": waiting... \n";
			actor._inbox_condition_variable.wait(lk, [&]{ return actor._inbox.empty() == false; });
			std::cout << thread_name << ": continue... \n";

			//	Pop message.
			QUARK_ASSERT(actor._inbox.empty() == false);
			message = actor._inbox.back();
			actor._inbox.pop_back();
		}
		QUARK_TRACE_SS("RECEIVED: " << json_to_pretty_string(message));

		if(message.is_string() && message.get_string() == "stop"){
			stop = true;
			std::cout << thread_name << ": STOP \n";
		}
		else{
			if(actor._processor){
				actor._processor->on_message(message);
			}

			if(actor._process_function != nullptr){
				const vector<value_t> args = { actor._actor_state, value_t::make_json_value(message) };
				const auto& state2 = call_function(*actor._interpreter, bc_to_value(actor._process_function->_value), args);
				actor._actor_state = state2;
			}
		}
	}
}

void run_container(const interpreter_context_t& context, const string& source, const vector<floyd::value_t>& args, const std::string& container_key){
	actor_runtime_t runtime;
	runtime._main_thread_id = std::this_thread::get_id();

	auto program = compile_to_bytecode(context, source);
	if(program._software_system._name == ""){
		throw std::exception();
	}
	runtime._container = program._software_system._containers.at(container_key);
	runtime._actor_infos = fold(runtime._container._clock_busses, std::map<std::string, std::string>(), [](const std::map<std::string, std::string>& acc, const pair<string, clock_bus_t>& e){
		auto acc2 = acc;
		acc2.insert(e.second._actors.begin(), e.second._actors.end());
		return acc2;
	});

	for(const auto& t: runtime._actor_infos){
		auto actor = std::make_shared<actor_t>();
		actor->_name_key = t.first;
		actor->_function_key = t.second;
		actor->_interpreter = make_shared<interpreter_t>(program);
		actor->_init_function = find_global_symbol2(*actor->_interpreter, t.second + "__init");
		actor->_process_function = find_global_symbol2(*actor->_interpreter, t.second);

		runtime._actors.push_back(actor);
	}

/*
	struct my_processor : public actor_processor_interface {
		my_processor(actor_runtime_t& runtime) : _runtime(runtime) {}

		actor_runtime_t& _runtime;

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

	runtime._actors[2]->_processor = std::make_shared<my_processor>(runtime);
*/

	//	Remember that current thread (main) is also a thread, no need to create a worker thread for one actor.
	runtime._actors[0]->_thread_id = runtime._main_thread_id;

	for(int actor_id = 1 ; actor_id < runtime._actors.size() ; actor_id++){
		runtime._worker_threads.push_back(std::thread([&](int actor_id){

//			const auto native_thread = thread::native_handle();

			std::stringstream thread_name;
			thread_name << string() << "actor " << actor_id << " thread";
			pthread_setname_np(/*pthread_self(),*/ thread_name.str().c_str());

			process_actor(runtime, actor_id);
		}, actor_id));
	}

	send_message(runtime, 0, json_t("inc"));
	send_message(runtime, 0, json_t("inc"));
	send_message(runtime, 0, json_t("dec"));
	send_message(runtime, 0, json_t("inc"));
	send_message(runtime, 0, json_t("stop"));

	process_actor(runtime, 0);

	for(auto &t: runtime._worker_threads){
		t.join();
	}

	QUARK_UT_VERIFY(runtime._actors[0]->_actor_state.get_struct_value()->_member_values[0].get_int_value() == 1002);
}




}	//	floyd
