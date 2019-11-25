//
//  compiler_basics.cpp
//  Floyd
//
//  Created by Marcus Zetterquist on 2019-02-05.
//  Copyright © 2019 Marcus Zetterquist. All rights reserved.
//

#include "compiler_basics.h"

#include "text_parser.h"
#include "parser_primitives.h"
#include "format_table.h"
#include "types.h"


namespace floyd {


const location_t k_no_location(std::numeric_limits<std::size_t>::max());





config_t make_default_config(){
	return config_t { vector_backend::hamt, dict_backend::hamt, false } ;
}
 
compiler_settings_t make_default_compiler_settings(){
	return { make_default_config(), eoptimization_level::g_no_optimizations_enable_debugging };
}



std::vector<benchmark_result2_t> unpack_vec_benchmark_result2_t(types_t& types, const value_t& value){
	QUARK_ASSERT(types.check_invariant());
//	QUARK_ASSERT(value.get_type() == make_vector(types, make_benchmark_result2_t(types)));

	std::vector<benchmark_result2_t> test_results;
	for(const auto& e: value.get_vector_value()){
		const auto& f = e.get_struct_value();

		const auto& test_id = f->_member_values[0].get_struct_value();
		const auto& result = f->_member_values[1].get_struct_value();

		const auto module = test_id->_member_values[0].get_string_value();
		const auto test = test_id->_member_values[1].get_string_value();

		const auto dur = result->_member_values[0].get_int_value();
		const auto more = result->_member_values[1].get_json();

		const auto r = benchmark_result2_t { benchmark_id_t { module, test }, benchmark_result_t { dur, more } };
		test_results.push_back(r);
	}
	return test_results;
}



std::vector<test_t> unpack_test_registry(const std::vector<value_t>& r){
	std::vector<test_t> result;
	for(const auto& e: r){
		const auto s = e.get_struct_value();

		const auto function_name = s->_member_values[0].get_string_value();
		const auto scenario = s->_member_values[1].get_string_value();
		const auto f_link_name_str = s->_member_values[2].get_function_value().name;
		const auto test = test_t{ test_id_t { "", function_name, scenario }, link_name_t{ f_link_name_str } };

		result.push_back(test);
	}
	return result;
}



size_t count_fails(const std::vector<test_result_t>& test_results){
	const auto failed = std::count_if(
		test_results.begin(),
		test_results.end(),
		[](const auto& e){ return e.type == test_result_t::type::fail_with_string; }
	);
	return failed;
}


static std::vector<std::vector<std::string>> make_test_result_matrix(const std::vector<test_result_t>& v){
	std::vector<std::vector<std::string>> matrix;
	for(const auto& e: v){
		const std::vector<std::string> matrix_row = { e.test_id.module, e.test_id.function_name, e.test_id.scenario, e.fail_string };
		matrix.push_back(matrix_row);
	}
	return matrix;
}

std::string make_report(const std::vector<test_result_t>& test_results){
//	const auto enabled_count = std::count_if(test_results.begin(), test_results.end(), [](const test_result_t& e){ return e.type != test_result_t::type::not_run; });
	const auto enabled = filterf<test_result_t>(test_results, [](const auto& e){ return e.type != test_result_t::type::not_run; });
	const auto failed = filterf<test_result_t>(test_results, [](const auto& e){ return e.type == test_result_t::type::fail_with_string; });

	if(enabled.size() == test_results.size()){
		std::stringstream ss;
		if(failed.empty() == true){
			ss << "All tests (" << test_results.size() << "): passed!" << std::endl;
		}
		else{
			ss << "All tests (" << test_results.size() << "): " << failed.size() << " failed!" << std::endl;

			std::vector<std::vector<std::string>> matrix = make_test_result_matrix(failed);
			ss << generate_table_type1({ "MODULE", "FUNCTION", "SCENARIO", "RESULT" }, matrix);
		}
		return ss.str();
	}
	else{
		std::stringstream ss;
		if(failed.empty() == true){
			ss << "Specified tests (" << enabled.size() << " of " << test_results.size() << "): passed!" << std::endl;
		}
		else{
			ss << "Specified tests (" << enabled.size() << " of " << test_results.size() << "): " << failed.size() << " failed!" << std::endl;

			std::vector<std::vector<std::string>> matrix = make_test_result_matrix(failed);
			ss << generate_table_type1({ "MODULE", "FUNCTION", "SCENARIO", "RESULT" }, matrix);
		}
		return ss.str();
	}
}

std::string pack_test_id(const test_id_t& id){
	return id.function_name + ":" + id.scenario;
}

std::optional<test_id_t> unpack_test_id(const std::string& test){
	const auto parts = split_on_chars(seq_t(test), ":");
	if(parts.size() != 2){
		return {};
	}

	const auto function_name = parts[0];
	const auto scenario = parts[1];
	return test_id_t { "", function_name, scenario };
}

std::vector<test_id_t> unpack_test_ids(const std::vector<std::string>& tests){
	const auto tests2 = mapf<test_id_t>(tests, [&](const auto& e){
		const auto opt = unpack_test_id(e);
		if(!opt){
			throw std::exception();
		}
		return *opt;
	});
	return tests2;
}

std::vector<int> filter_tests(const std::vector<test_t>& b, const std::vector<std::string>& wanted_tests){
	std::vector<int> filtered;

	for(int index = 0 ; index < wanted_tests.size() ; index++){
		const auto& wanted_test = wanted_tests[index];
		const auto test_id = unpack_test_id(wanted_test);
		if(!test_id){
			throw std::exception();
		}

		const auto it = std::find_if(
			b.begin(),
			b.end(),
			[&] (const test_t& b2) {
				return
					//??? check module in the future.
					b2.test_id.function_name == test_id->function_name
					&& b2.test_id.scenario == test_id->scenario
					;
			}
		);
		if(it != b.end()){
			filtered.push_back(index);
		}
	}

	return filtered;
}



//std::vector<test_t>
static std::vector<std::vector<std::string>> make_test_list_report(const std::vector<test_result_t>& v){
	std::vector<std::vector<std::string>> matrix;
	for(const auto& e: v){
		const std::vector<std::string> matrix_row = { e.test_id.module, e.test_id.function_name, e.test_id.scenario, e.fail_string };
		matrix.push_back(matrix_row);
	}
	return matrix;
}

std::string make_test_list_report(const std::vector<test_t>& tests){
	std::vector<std::vector<std::string>> matrix;
	for(const auto& e: tests){
		matrix.push_back({ e.test_id.function_name, e.test_id.scenario });
	}

	std::stringstream ss;
	ss << "Test registry:" << std::endl;
	ss << generate_table_type1({ "FUNCTION", "SCENARIO" }, matrix);
	return ss.str();
}




//////////////////////////////////////		container_t



static process_def_t unpack_process_def(types_t& types, const json_t& j){
	const auto name = j.get_object_element("name").get_string();
	const auto state_type = type_from_json(types, j.get_object_element("state_type"));
	const auto msg_type = type_from_json(types, j.get_object_element("msg_type"));

	const auto init_func_linkname = j.get_object_element("init_func_linkname").get_string();
	const auto msg_func_linkname = j.get_object_element("msg_func_linkname").get_string();

	return process_def_t { name, state_type, msg_type, init_func_linkname, msg_func_linkname };
}

static json_t pack_process_def(const types_t& types, const process_def_t& def){
	return json_t::make_object({
		{ "name", def.name_key },
		{ "state_type", type_to_json(types, def.state_type) },
		{ "msg_type", type_to_json(types, def.msg_type) },
		{ "init_func_linkname", def.init_func_linkname },
		{ "msg_func_linkname", def.msg_func_linkname }
	});
}




static clock_bus_t unpack_clock_bus(types_t& types, const json_t& clock_bus_obj){
	std::map<std::string, process_def_t> processes;

	const auto processes_map = clock_bus_obj.get_object();
	for(const auto& process_pair: processes_map){
		const auto name_key = process_pair.first;
		if(process_pair.second.is_string()){
			processes.insert({ name_key, process_def_t { process_pair.second.get_string(), make_undefined(), make_undefined(), "", "" } } );
		}
		else if(process_pair.second.is_object()){
			const auto process_def = unpack_process_def(types, process_pair.second);
			processes.insert({ name_key, process_def } );
		}
		else{
			throw std::exception();
		}
	}
	return clock_bus_t{._processes = processes};
}

static json_t pack_clock_bus(const types_t& types, const clock_bus_t& clock_bus){
	std::map<std::string, json_t> processes;
	for(const auto& e: clock_bus._processes){
		processes.insert({ e.first, pack_process_def(types, e.second) });
	}
	return json_t::make_object(processes);
}

container_t parse_container_def_json(types_t& types, const json_t& container_obj){
	if(container_obj.get_object_size() == 0){
		return {};
	}
	else {
		std::map<std::string, clock_bus_t> clock_busses;
		const auto temp = container_obj.get_object_element("clocks").get_object();
		for(const auto& clock_pair: temp){
			const auto name = clock_pair.first;
			const auto clock_bus = unpack_clock_bus(types, clock_pair.second);
			clock_busses.insert({name, clock_bus});
		}

		return container_t{
			._name = container_obj.get_object_element("name").get_string(),
			._desc = container_obj.get_object_element("desc").get_string(),
			._tech = container_obj.get_object_element("tech").get_string(),
			._clock_busses = clock_busses
		};
	}
}

json_t container_to_json(const types_t& types, const container_t& v){
	std::map<std::string, json_t> clock_busses_json;
	for(const auto& e: v._clock_busses){
		clock_busses_json.insert({ e.first, pack_clock_bus(types, e.second) });
	}

	return json_t::make_object({
		{ "name", v._name },
		{ "desc", v._desc },
		{ "tech", v._tech },
		{ "clocks", clock_busses_json }
	});
}



static const process_def_t make_test_process1(){
	return { "gui", type_t::make_double(), type_t::make_bool(), "link_gui_init", "link_gui_msg" };
};
static const process_def_t make_test_process2(){
	return { "mem", type_t::make_bool(), type_t::make_double(), "link_mem_init", "link_mem_msg" };
};
static const process_def_t make_test_process3(){
	return { "audio", type_t::make_string(), type_t::make_bool(), "link_audio_init", "link_audio_msg" };
};
static const process_def_t make_test_process4(){
	return { "audio_prefetch", type_t::make_double(), type_t::make_string(), "link_audio_prefetch_init", "link_audio_prefetch_msg" };
};



static container_t make_example_container_def(){
	const types_t types;

	container_t a = {
		"*name*",
		"*desc*",
		"*tech*",
		std::map<std::string, clock_bus_t>{
			{ "bus_a", clock_bus_t{{ { "x", make_test_process1() }, { "y", make_test_process2() } } } },
			{ "bus_b", clock_bus_t{{ { "u", make_test_process3() }, { "z", make_test_process4() } } } }
		}
	};
	return a;
}

static json_t make_example_container_def_json(){
	static const std::string expected_json = R"___(
		{
			"clocks" : {
				"bus_a" : {
					"x" : {
						"init_func_linkname" : "link_gui_init",
						"msg_func_linkname" : "link_gui_msg",
						"msg_type" : "bool",
						"name" : "gui",
						"state_type" : "double"
					},
					"y" : {
						"init_func_linkname" : "link_mem_init",
						"msg_func_linkname" : "link_mem_msg",
						"msg_type" : "double",
						"name" : "mem",
						"state_type" : "bool"
					}
				},
				"bus_b" : {
					"u" : {
						"init_func_linkname" : "link_audio_init",
						"msg_func_linkname" : "link_audio_msg",
						"msg_type" : "bool",
						"name" : "audio",
						"state_type" : "string"
					},
					"z" : {
						"init_func_linkname" : "link_audio_prefetch_init",
						"msg_func_linkname" : "link_audio_prefetch_msg",
						"msg_type" : "string",
						"name" : "audio_prefetch",
						"state_type" : "double"
					}
				}
			},
			"desc" : "*desc*",
			"name" : "*name*",
			"tech" : "*tech*"
		}

	)___";
	const auto j = parse_json(seq_t(expected_json)).first;
	return j;
}



QUARK_TEST("floyd_basics", "container_to_json()", "", ""){
	const types_t types;
	const auto a = container_to_json(types, make_example_container_def());
	const auto b = make_example_container_def_json();
	ut_verify_string(QUARK_POS, json_to_compact_string(a), json_to_compact_string(b));
}

QUARK_TEST("floyd_basics", "parse_container_def_json()", "", ""){
	types_t types;
	const auto a = parse_container_def_json(types, make_example_container_def_json());
	const auto b = make_example_container_def();
	QUARK_VERIFY(a == b);
}




//////////////////////////////////////		software_system_t


static std::vector<person_t> unpack_persons(const json_t& persons_obj){
	std::vector<person_t> result;
	const auto temp = persons_obj.get_object();
	for(const auto& person_pairs: temp){
		const auto name = person_pairs.first;
		const auto desc = person_pairs.second.get_string();
		result.push_back(person_t { ._name_key = name, ._desc = desc} );
	}
	return result;
}

json_t pack_persons(const std::vector<person_t>& persons){
	std::map<std::string, json_t> result;
	for(const auto& e: persons){
		result.insert({ e._name_key, e._desc });
	}
	return result;
}

software_system_t parse_software_system_json(const json_t& value){
	std::vector<std::string> containers;
	const auto container_array = value.get_object_element("containers").get_array();
	for(const auto& e: container_array){
		containers.push_back(e.get_string());
	}

	const auto name = value.get_object_element("name").get_string();
	const auto desc = value.get_object_element("desc").get_string();
	const auto people = unpack_persons(value.get_object_element("people"));
	const auto connections = value.get_object_element("connections");

	return software_system_t{
		._name = name,
		._desc = desc,
		._people = people,
		._containers = containers
	};
}

json_t software_system_to_json(const software_system_t& v){
	return json_t::make_object({
		{ "name", v._name },
		{ "desc", v._name },
		{ "people", pack_persons(v._people) },
	});
}








bool is_floyd_literal(const type_desc_t& desc){
	QUARK_ASSERT(desc.check_invariant());

	//??? json is allowed but only for json::null. We should have a null-type instead.
	if(
		desc.is_void()
		|| desc.is_int()
		|| desc.is_double()
		|| desc.is_string()
		|| desc.is_bool()
		|| desc.is_typeid()
		|| desc.is_any()
		|| desc.is_json()
		|| desc.is_function()
	){
		return true;
	}
	else{
		return false;
	}
}

bool is_preinitliteral(const type_desc_t& desc){
	QUARK_ASSERT(desc.check_invariant());

	//??? json is allowed but only for json::null. We should have a null-type instead.
	if(desc.is_int() || desc.is_double() || desc.is_bool() || desc.is_typeid() || desc.is_any() || desc.is_function()){
		return true;
	}
	else{
		return false;
	}
}


////////////////////////////////////////		main() init() and message handler




type_t make_process_init_type(types_t& types, const type_t& t){
	return make_function3(types, t, {}, epure::impure, return_dyn_type::none);
}

type_t make_process_message_handler_type(types_t& types, const type_t& t){
	return make_function3(types, t, { t, type_t::make_json() }, epure::impure, return_dyn_type::none);
}



//////////////////////////////////////		INTRINSICS




#define ANY_TYPE type_t::make_any()

static intrinsic_signature_t make_intrinsic(
	const std::string& name,
	floyd::type_t function_type
){
	return { name, function_type };
}


intrinsic_signature_t make_assert_signature(types_t& types){
	return make_intrinsic("assert", make_function(types, type_t::make_void(), { type_t::make_bool() }, epure::pure) );
}
intrinsic_signature_t make_to_string_signature(types_t& types){
	return make_intrinsic("to_string", make_function(types, type_t::make_string(), { ANY_TYPE }, epure::pure) );
}

intrinsic_signature_t make_to_pretty_string_signature(types_t& types){
	return make_intrinsic("to_pretty_string", make_function(types, type_t::make_string(), { ANY_TYPE }, epure::pure) );
}
intrinsic_signature_t make_typeof_signature(types_t& types){
	return make_intrinsic("typeof", make_function(types, type_desc_t::make_typeid(), { ANY_TYPE }, epure::pure) );
}


intrinsic_signature_t make_update_signature(types_t& types){
	return make_intrinsic("update", make_function_dyn_return(types, { ANY_TYPE, ANY_TYPE, ANY_TYPE }, epure::pure, return_dyn_type::arg0) );
}

intrinsic_signature_t make_size_signature(types_t& types){
	return make_intrinsic("size", make_function(types, type_t::make_int(), { ANY_TYPE }, epure::pure) );
}


intrinsic_signature_t make_find_signature(types_t& types){
	return make_intrinsic("find", make_function(types, type_t::make_int(), { ANY_TYPE, ANY_TYPE }, epure::pure) );
}
intrinsic_signature_t make_exists_signature(types_t& types){
	return make_intrinsic("exists", make_function(types, type_t::make_bool(), { ANY_TYPE, ANY_TYPE }, epure::pure) );
}

intrinsic_signature_t make_erase_signature(types_t& types){
	return make_intrinsic("erase", make_function_dyn_return(types, { ANY_TYPE, ANY_TYPE }, epure::pure, return_dyn_type::arg0) );
}
intrinsic_signature_t make_get_keys_signature(types_t& types){
	return make_intrinsic("get_keys", make_function(types, make_vector(types, type_t::make_string()), { ANY_TYPE }, epure::pure) );
}


intrinsic_signature_t make_push_back_signature(types_t& types){
	return make_intrinsic("push_back", make_function_dyn_return(types, { ANY_TYPE, ANY_TYPE }, epure::pure, return_dyn_type::arg0) );
}

intrinsic_signature_t make_subset_signature(types_t& types){
	return make_intrinsic("subset", make_function_dyn_return(types, { ANY_TYPE, type_t::make_int(), type_t::make_int() }, epure::pure, return_dyn_type::arg0) );
}
intrinsic_signature_t make_replace_signature(types_t& types){
	return make_intrinsic("replace", make_function_dyn_return(types, { ANY_TYPE, type_t::make_int(), type_t::make_int(), ANY_TYPE }, epure::pure, return_dyn_type::arg0) );
}

intrinsic_signature_t make_parse_json_script_signature(types_t& types){
	return make_intrinsic("parse_json_script", make_function(types, type_t::make_json(), { type_t::make_string() }, epure::pure) );
}

intrinsic_signature_t make_generate_json_script_signature(types_t& types){
	return make_intrinsic("generate_json_script", make_function(types, type_t::make_string(), {type_t::make_json() }, epure::pure) );
}


intrinsic_signature_t make_to_json_signature(types_t& types){
	return make_intrinsic("to_json", make_function(types, type_t::make_json(), { ANY_TYPE }, epure::pure) );
}


intrinsic_signature_t make_from_json_signature(types_t& types){
	//	??? Tricky. How to we compute the return type from the input arguments?
	return make_intrinsic("from_json", make_function_dyn_return(types, { type_t::make_json(), type_desc_t::make_typeid() }, epure::pure, return_dyn_type::arg1_typeid_constant_type) );
}


intrinsic_signature_t make_get_json_type_signature(types_t& types){
	return make_intrinsic("get_json_type", make_function(types, type_t::make_int(), { type_t::make_json() }, epure::pure) );
}




//////////////////////////////////////		HIGHER-ORDER INTRINSICS



intrinsic_signature_t make_map_signature(types_t& types){
	return make_intrinsic("map", make_function_dyn_return(types, { ANY_TYPE, ANY_TYPE, ANY_TYPE }, epure::pure, return_dyn_type::vector_of_arg1func_return) );
}
type_t harden_map_func_type(types_t& types, const type_t& resolved_call_type0){
	QUARK_ASSERT(resolved_call_type0.check_invariant());

	const auto resolved_call_type = peek2(types, resolved_call_type0);
	const auto sign = make_map_signature(types);

	const auto arg1_type = resolved_call_type.get_function_args(types)[0];
	const auto arg1_peek = peek2(types, arg1_type);
	if(arg1_peek.is_vector() == false){
		quark::throw_runtime_error("map() arg 1 must be a vector.");
	}
	const auto e_type = arg1_peek.get_vector_element_type(types);

	const auto arg2_type = resolved_call_type.get_function_args(types)[1];
	if(peek2(types, arg2_type).is_function() == false){
		quark::throw_runtime_error("map() arg 2 must be a function.");
	}

	const auto r_type = peek2(types, arg2_type).get_function_return(types);

	const auto context_type = resolved_call_type.get_function_args(types)[2];

	const auto expected = make_function(types,
		make_vector(types, r_type),
		{
			make_vector(types, e_type),
			make_function(types, r_type, { e_type, context_type }, epure::pure),
			context_type
		},
		epure::pure
	);
	return expected;
}

bool check_map_func_type(types_t& types, const type_t& elements, const type_t& f, const type_t& context){
	QUARK_ASSERT(peek2(types, elements).is_vector());
	QUARK_ASSERT(peek2(types, f).is_function());

	const auto f_peek = peek2(types, f);
	QUARK_ASSERT(f_peek.get_function_args(types).size() == 2);
	QUARK_ASSERT(f_peek.get_function_args(types)[0] == peek2(types, elements).get_vector_element_type(types));
	QUARK_ASSERT(f_peek.get_function_args(types)[1] == context);
	return true;
}



//	string map_string(string s, func int(int e, C context) f, C context)
intrinsic_signature_t make_map_string_signature(types_t& types){
	return make_intrinsic(
		"map_string",
		make_function(
			types,
			type_t::make_string(),
			{
				type_t::make_string(),
				ANY_TYPE,
				ANY_TYPE
			},
			epure::pure
		)
	);
}

type_t harden_map_string_func_type(types_t& types, const type_t& resolved_call_type0){
	QUARK_ASSERT(resolved_call_type0.check_invariant());

	const auto resolved_call_type = peek2(types, resolved_call_type0);
	const auto sign = make_map_string_signature(types);
	const auto context_type = resolved_call_type.get_function_args(types)[2];

	const auto expected = make_function(
		types,
		type_t::make_string(),
		{
			type_t::make_string(),
			make_function(types, type_t::make_int(), { type_t::make_int(), context_type }, epure::pure),
			context_type
		},
		epure::pure
	);
	return expected;
}

bool check_map_string_func_type(types_t& types, const type_t& elements, const type_t& f, const type_t& context){
	QUARK_ASSERT(peek2(types, elements).is_string());

	const auto f_peek = peek2(types, f);
	QUARK_ASSERT(f_peek.is_function());
	QUARK_ASSERT(f_peek.get_function_args(types).size() == 2);
	QUARK_ASSERT(f_peek.get_function_args(types)[0] == type_t::make_int());
	QUARK_ASSERT(f_peek.get_function_args(types)[1] == context);
	return true;
}




//	[R] map_dag([E] elements, [int] depends_on, func R (E, [R], C context) f, C context)
intrinsic_signature_t make_map_dag_signature(types_t& types){
	return make_intrinsic("map_dag", make_function_dyn_return(types, { ANY_TYPE, ANY_TYPE, ANY_TYPE, ANY_TYPE }, epure::pure, return_dyn_type::vector_of_arg2func_return) );
}

type_t harden_map_dag_func_type(types_t& types, const type_t& resolved_call_type0){
	QUARK_ASSERT(resolved_call_type0.check_invariant());

	const auto resolved_call_type = peek2(types, resolved_call_type0);
	const auto sign = make_map_dag_signature(types);

	const auto arg1_type = resolved_call_type.get_function_args(types)[0];
	if(peek2(types, arg1_type).is_vector() == false){
		quark::throw_runtime_error("map_dag() arg 1 must be a vector.");
	}
	const auto e_type = peek2(types, arg1_type).get_vector_element_type(types);

	const auto arg3_type = resolved_call_type.get_function_args(types)[2];
	if(peek2(types, arg3_type).is_function() == false){
		quark::throw_runtime_error("map_dag() arg 3 must be a function.");
	}

	const auto r_type = peek2(types, arg3_type).get_function_return(types);

	const auto context_type = resolved_call_type.get_function_args(types)[3];

	const auto expected = make_function(
		types,
		make_vector(types, r_type),
		{
			make_vector(types, e_type),
			make_vector(types, type_t::make_int()),
			make_function(types, r_type, { e_type, make_vector(types, r_type), context_type }, epure::pure),
			context_type
		},
		epure::pure
	);
	return expected;
}

bool check_map_dag_func_type(types_t& types, const type_t& elements, const type_t& depends_on, const type_t& f, const type_t& context){
	//	Check topology.
	QUARK_ASSERT(peek2(types, elements).is_vector());
	QUARK_ASSERT(depends_on == make_vector(types, type_t::make_int()));

	const auto f_peek = peek2(types, f);
	QUARK_ASSERT(f_peek.is_function() && f_peek.get_function_args(types).size () == 3);
	QUARK_ASSERT(f_peek.get_function_args(types)[0] == peek2(types, elements).get_vector_element_type(types));
	QUARK_ASSERT(f_peek.get_function_args(types)[1] == make_vector(types, f_peek.get_function_return(types)));
	QUARK_ASSERT(f_peek.get_function_args(types)[2] == context);
	return true;
}





//	[E] filter([E] elements, func bool (E e, C context) f, C context)
intrinsic_signature_t make_filter_signature(types_t& types){
	return make_intrinsic("filter", make_function_dyn_return(types, { ANY_TYPE, ANY_TYPE, ANY_TYPE }, epure::pure, return_dyn_type::arg0) );
}
type_t harden_filter_func_type(types_t& types, const type_t& resolved_call_type0){
	QUARK_ASSERT(resolved_call_type0.check_invariant());

	const auto resolved_call_type = peek2(types, resolved_call_type0);
	const auto sign = make_filter_signature(types);

	const auto arg1_type = resolved_call_type.get_function_args(types)[0];
	if(peek2(types, arg1_type).is_vector() == false){
		quark::throw_runtime_error("map() arg 1 must be a vector.");
	}
	const auto e_type = peek2(types, arg1_type).get_vector_element_type(types);

	const auto context_type = resolved_call_type.get_function_args(types)[2];

	const auto expected = make_function(
		types,
		make_vector(types, e_type),
		{
			make_vector(types, e_type),
			make_function(types, type_t::make_bool(), { e_type, context_type }, epure::pure),
			context_type
		},
		epure::pure
	);
	return expected;
}

bool check_filter_func_type(types_t& types, const type_t& elements, const type_t& f, const type_t& context){
	QUARK_ASSERT(peek2(types, elements).is_vector());

	const auto f_peek = peek2(types, f);
	QUARK_ASSERT(f_peek.is_function());
	QUARK_ASSERT(f_peek.get_function_args(types).size() == 2);
	QUARK_ASSERT(f_peek.get_function_args(types)[0] == peek2(types, elements).get_vector_element_type(types));
	QUARK_ASSERT(f_peek.get_function_args(types)[1] == context);
	return true;
}




intrinsic_signature_t make_reduce_signature(types_t& types){
	return make_intrinsic("reduce", make_function_dyn_return(types, { ANY_TYPE, ANY_TYPE, ANY_TYPE, ANY_TYPE }, epure::pure, return_dyn_type::arg1) );
}

type_t harden_reduce_func_type(types_t& types, const type_t& resolved_call_type0){
	QUARK_ASSERT(resolved_call_type0.check_invariant());

	const auto resolved_call_type = peek2(types, resolved_call_type0);
	const auto call_function_arg_type = resolved_call_type.get_function_args(types);

	const auto elements_arg_type = call_function_arg_type[0];
	if(peek2(types, elements_arg_type).is_vector() == false){
		quark::throw_runtime_error("map() arg 1 must be a vector.");
	}
	const auto e_type = peek2(types, elements_arg_type).get_vector_element_type(types);

	const auto r_type = call_function_arg_type[1];

	const auto context_type = call_function_arg_type[3];

	const auto expected = make_function(
		types,
		r_type,
		{
			make_vector(types, e_type),
			r_type,
			make_function(types, r_type, { r_type, e_type, context_type }, epure::pure),
			context_type
		},
		epure::pure
	);
	return expected;
}

bool check_reduce_func_type(types_t& types, const type_t& elements, const type_t& accumulator_init, const type_t& f, const type_t& context){
	QUARK_ASSERT(elements.check_invariant());
	QUARK_ASSERT(accumulator_init.check_invariant());
	QUARK_ASSERT(f.check_invariant());
	QUARK_ASSERT(context.check_invariant());

	const auto f_peek = peek2(types, f);

	QUARK_ASSERT(peek2(types, elements).is_vector());
	QUARK_ASSERT(f_peek.is_function());
	QUARK_ASSERT(f_peek.get_function_args(types).size () == 3);

	QUARK_ASSERT(f_peek.get_function_args(types)[1] == peek2(types, elements).get_vector_element_type(types));
	QUARK_ASSERT(f_peek.get_function_args(types)[0] == accumulator_init);

	return true;
}




//	[T] stable_sort([T] elements, func bool (T left, T right, C context) less, C context)
intrinsic_signature_t make_stable_sort_signature(types_t& types){
	return make_intrinsic("stable_sort", make_function_dyn_return(types, { ANY_TYPE, ANY_TYPE, ANY_TYPE }, epure::pure, return_dyn_type::arg0) );
}

type_t harden_stable_sort_func_type(types_t& types, const type_t& resolved_call_type0){
	QUARK_ASSERT(resolved_call_type0.check_invariant());

	const auto resolved_call_type = peek2(types, resolved_call_type0);
	const auto sign = make_stable_sort_signature(types);

	const auto arg1_type = resolved_call_type.get_function_args(types)[0];
	if(peek2(types, arg1_type).is_vector() == false){
		quark::throw_runtime_error("stable_sort() arg 1 must be a vector.");
	}
	const auto e_type = peek2(types, arg1_type).get_vector_element_type(types);

	const auto arg2_type = resolved_call_type.get_function_args(types)[1];
	if(peek2(types, arg2_type).is_function() == false){
		quark::throw_runtime_error("stable_sort() arg 2 must be a function.");
	}

	const auto context_type = resolved_call_type.get_function_args(types)[2];

	const auto expected = make_function(
		types,
		arg1_type,
		{
			arg1_type,
			make_function(types, type_t::make_bool(), { e_type, e_type, context_type }, epure::pure),
			context_type
		},
		epure::pure
	);
	return expected;
}

bool check_stable_sort_func_type(types_t& types, const type_t& elements, const type_t& f, const type_t& context){
	QUARK_ASSERT(peek2(types, elements).is_vector());
	QUARK_ASSERT(peek2(types, f).is_function());

	const auto f_peek = peek2(types, f);

	QUARK_ASSERT(f_peek.get_function_args(types).size() == 3);

	const auto& e_type = peek2(types, elements).get_vector_element_type(types);
	QUARK_ASSERT(f_peek.get_function_return(types) == type_t::make_bool());
	QUARK_ASSERT(f_peek.get_function_args(types)[0] == e_type);
	QUARK_ASSERT(f_peek.get_function_args(types)[1] == e_type);
	QUARK_ASSERT(f_peek.get_function_args(types)[2] == context);

	return true;
}





//////////////////////////////////////		IMPURE FUNCTIONS



intrinsic_signature_t make_print_signature(types_t& types){
	return make_intrinsic("print", make_function(types, type_t::make_void(), { ANY_TYPE }, epure::pure) );
}
intrinsic_signature_t make_send_signature(types_t& types){
	return make_intrinsic("send", make_function(types, type_t::make_void(), { type_t::make_string(), ANY_TYPE }, epure::impure) );
}
intrinsic_signature_t make_exit_signature(types_t& types){
	return make_intrinsic("exit", make_function(types, type_t::make_void(), { }, epure::impure) );
}



//////////////////////////////////////		BITWISE



intrinsic_signature_t make_bw_not_signature(types_t& types){
	return make_intrinsic("bw_not", make_function(types, type_t::make_int(), { type_t::make_int() }, epure::pure) );
}
intrinsic_signature_t make_bw_and_signature(types_t& types){
	return make_intrinsic("bw_and", make_function(types, type_t::make_int(), { type_t::make_int(), type_t::make_int() }, epure::pure) );
}
intrinsic_signature_t make_bw_or_signature(types_t& types){
	return make_intrinsic("bw_or", make_function(types, type_t::make_int(), { type_t::make_int(), type_t::make_int() }, epure::pure) );
}
intrinsic_signature_t make_bw_xor_signature(types_t& types){
	return make_intrinsic("bw_xor", make_function(types, type_t::make_int(), { type_t::make_int(), type_t::make_int() }, epure::pure) );
}
intrinsic_signature_t make_bw_shift_left_signature(types_t& types){
	return make_intrinsic("bw_shift_left", make_function(types, type_t::make_int(), { type_t::make_int(), type_t::make_int() }, epure::pure) );
}
intrinsic_signature_t make_bw_shift_right_signature(types_t& types){
	return make_intrinsic("bw_shift_right", make_function(types, type_t::make_int(), { type_t::make_int(), type_t::make_int() }, epure::pure) );
}
intrinsic_signature_t make_bw_shift_right_arithmetic_signature(types_t& types){
	return make_intrinsic("bw_shift_right_arithmetic", make_function(types, type_t::make_int(), { type_t::make_int(), type_t::make_int() }, epure::pure) );
}







std::string get_intrinsic_opcode(const intrinsic_signature_t& signature){
	return std::string() + "$" + signature.name;
}

intrinsic_signatures_t make_intrinsic_signatures(types_t& types){
	intrinsic_signatures_t result;

	result.assert = make_assert_signature(types);
	result.to_string = make_to_string_signature(types);
	result.to_pretty_string = make_to_pretty_string_signature(types);
	result.typeof_sign = make_typeof_signature(types);

	result.update = make_update_signature(types);
	result.size = make_size_signature(types);
	result.find = make_find_signature(types);
	result.exists = make_exists_signature(types);
	result.erase = make_erase_signature(types);
	result.get_keys = make_get_keys_signature(types);
	result.push_back = make_push_back_signature(types);
	result.subset = make_subset_signature(types);
	result.replace = make_replace_signature(types);

	result.parse_json_script = make_parse_json_script_signature(types);
	result.generate_json_script = make_generate_json_script_signature(types);
	result.to_json = make_to_json_signature(types);
	result.from_json = make_from_json_signature(types);

	result.get_json_type = make_get_json_type_signature(types);


	result.map = make_map_signature(types);
//	result.xxx = make_map_string_signature(types);
	result.filter = make_filter_signature(types);
	result.reduce = make_reduce_signature(types);
	result.map_dag = make_map_dag_signature(types);

	result.stable_sort = make_stable_sort_signature(types);

	result.print = make_print_signature(types);
	result.send = make_send_signature(types);
	result.exit = make_exit_signature(types);

	result.bw_not = make_bw_not_signature(types);
	result.bw_and = make_bw_and_signature(types);
	result.bw_or = make_bw_or_signature(types);
	result.bw_xor = make_bw_xor_signature(types);
	result.bw_shift_left = make_bw_shift_left_signature(types);
	result.bw_shift_right = make_bw_shift_right_signature(types);
	result.bw_shift_right_arithmetic = make_bw_shift_right_arithmetic_signature(types);


	const std::vector<intrinsic_signature_t> vec = {
		make_assert_signature(types),
		make_to_string_signature(types),
		make_to_pretty_string_signature(types),
		make_typeof_signature(types),

		make_update_signature(types),
		make_size_signature(types),
		make_find_signature(types),
		make_exists_signature(types),
		make_erase_signature(types),
		make_get_keys_signature(types),
		make_push_back_signature(types),
		make_subset_signature(types),
		make_replace_signature(types),

		make_parse_json_script_signature(types),
		make_generate_json_script_signature(types),
		make_to_json_signature(types),
		make_from_json_signature(types),

		make_get_json_type_signature(types),

		make_map_signature(types),
//		make_map_string_signature(types),
		make_filter_signature(types),
		make_reduce_signature(types),
		make_map_dag_signature(types),

		make_stable_sort_signature(types),

		make_print_signature(types),
		make_send_signature(types),
		make_exit_signature(types),

		make_bw_not_signature(types),
		make_bw_and_signature(types),
		make_bw_or_signature(types),
		make_bw_xor_signature(types),
		make_bw_shift_left_signature(types),
		make_bw_shift_right_signature(types),
		make_bw_shift_right_arithmetic_signature(types)
	};
	result.vec = vec;
	return result;
}





//////////////////////////////////////////////////		location_t





std::pair<json_t, location_t> unpack_loc(const json_t& s){
	QUARK_ASSERT(s.is_array());

	const bool has_location = s.get_array_n(0).is_number();
	if(has_location){
		const location_t source_offset = has_location ? location_t(static_cast<std::size_t>(s.get_array_n(0).get_number())) : k_no_location;

		const auto elements = s.get_array();
		const std::vector<json_t> elements2 = { elements.begin() + 1, elements.end() };
		const auto statement = json_t::make_array(elements2);

		return { statement, source_offset };
	}
	else{
		return { s, k_no_location };
	}
}

location_t unpack_loc2(const json_t& s){
	QUARK_ASSERT(s.is_array());

	const bool has_location = s.get_array_n(0).is_number();
	if(has_location){
		const location_t source_offset = has_location ? location_t(static_cast<std::size_t>(s.get_array_n(0).get_number())) : k_no_location;
		return source_offset;
	}
	else{
		return k_no_location;
	}
}



//////////////////////////////////////////////////		MACROS



void NOT_IMPLEMENTED_YET() {
	throw std::exception();
}

void UNSUPPORTED() {
	QUARK_ASSERT(false);
	throw std::exception();
}


//	Return one entry per source line PLUS one extra end-marker. int tells byte offset of files that maps to this line-start.
//	Never returns empty vector, at least 2 elements.
std::vector<int> make_location_lookup(const std::string& source){
	if(source.empty()){
		return { 0 };
	}
	else{
		std::vector<int> result;
		int char_index = 0;
		result.push_back(char_index);
		for(const auto& ch: source){
			char_index++;

			if(ch == '\n' || ch == '\r'){
				result.push_back(char_index);
			}
		}
		const auto back_char = source.back();
		if(back_char == '\r' || back_char == '\n'){
			return result;
		}
		else{
			int end_index = static_cast<int>(source.size());
			result.push_back(end_index);
			return result;
		}
	}
}

QUARK_TEST("", "make_location_lookup()", "", ""){
	const auto r = make_location_lookup("");
	QUARK_VERIFY((r == std::vector<int>{ 0 }));
}
QUARK_TEST("", "make_location_lookup()", "", ""){
	const auto r = make_location_lookup("aaa");
	QUARK_VERIFY((r == std::vector<int>{ 0, 3 }));
}
QUARK_TEST("", "make_location_lookup()", "", ""){
	const auto r = make_location_lookup("aaa\nbbb\n");
	QUARK_VERIFY((r == std::vector<int>{ 0, 4, 8 }));
}

const std::string cleanup_line_snippet(const std::string& s){
	const auto line1 = skip(seq_t(s), "\t ");
	const auto split = split_on_chars(line1, "\n\r");
	const auto line = split.size() > 0 ? split.front() : "";
	return line;
}
QUARK_TEST("", "cleanup_line_snippet()", "", ""){
	ut_verify_string(QUARK_POS, cleanup_line_snippet(" \tabc\n\a"), "abc");
}

location2_t make_loc2(const std::string& program, const std::vector<int>& lookup, const std::string& file, const location_t& loc, int line_index){
	const auto start = lookup[line_index];
	const auto end = lookup[line_index + 1];
	const auto line = cleanup_line_snippet(program.substr(start, end - start));
	const auto column = loc.offset - start;
	return location2_t(file, line_index, static_cast<int>(column), start, end, line, loc);
}


location2_t find_loc_info(const std::string& program, const std::vector<int>& lookup, const std::string& file, const location_t& loc){
	QUARK_ASSERT(lookup.size() >= 2);
	QUARK_ASSERT(loc.offset <= lookup.back());

	int last_line_offset = lookup.back();
	QUARK_ASSERT(last_line_offset >= loc.offset);

	//	 EOF? Use program's last non-blank line.
	if(last_line_offset == loc.offset){
		auto line_index = static_cast<int>(lookup.size()) - 2;
		auto loc2 = make_loc2(program, lookup, file, loc, line_index);
		while(line_index >= 0 && loc2.line ==""){
			line_index--;
			loc2 = make_loc2(program, lookup, file, loc, line_index);
		}
		return loc2;
	}
	else{
		int line_index = 0;
		while(lookup[line_index + 1] <= loc.offset){
			line_index++;
			QUARK_ASSERT(line_index < lookup.size());
		}
		return make_loc2(program, lookup, file, loc, line_index);
	}
}

QUARK_TEST("", "make_location_lookup()", "", ""){
	const std::vector<int> lookup = { 0, 1, 2, 18, 19, 21 };
	const std::string program = R"(

			{ let a = 10

		)";
	const auto r = find_loc_info(program, lookup, "path.txt", location_t(21));
}

location2_t find_source_line(const compilation_unit_t& cu, const location_t& loc){
	const auto program_lookup = make_location_lookup(cu.program_text);

	if(cu.prefix_source != ""){
		const auto corelib_lookup = make_location_lookup(cu.prefix_source);
		const auto corelib_end_offset = corelib_lookup.back();
		if(loc.offset < corelib_end_offset){
			return find_loc_info(cu.prefix_source, corelib_lookup, "corelib", loc);
		}
		else{
			const auto program_loc = location_t(loc.offset - corelib_end_offset);
			const auto loc2 = find_loc_info(cu.program_text, program_lookup, cu.source_file_path, program_loc);
			const auto result = location2_t(
				loc2.source_file_path,
				loc2.line_number,
				loc2.column,
				loc2.start,
				loc2.end,
				loc2.line,
				location_t(loc2.loc.offset + corelib_end_offset)
			);
			return result;
		}
	}
	else{
		return find_loc_info(cu.program_text, program_lookup, cu.source_file_path, loc);
	}
}



/*	const auto line_numbers2 = mapf<int>(
		statements_pos.line_numbers,
		[&](const auto& e){
			return e - pre_line_count;
		}
	);
*/



std::pair<location2_t, std::string> refine_compiler_error_with_loc2(const compilation_unit_t& cu, const compiler_error& e){
	const auto loc2 = find_source_line(cu, e.location);
	const auto what1 = std::string(e.what());

	std::stringstream what2;
	const auto line_snippet = parser::reverse(parser::skip_whitespace(parser::reverse(loc2.line)));
	what2 << what1 << " Line: " << std::to_string(loc2.line_number + 1) << " \"" << line_snippet << "\"";
	if(loc2.source_file_path.empty() == false){
		what2 << " file: " << loc2.source_file_path;
	}

	return { loc2, what2.str() };
}




void ut_verify_json_and_rest(const quark::call_context_t& context, const std::pair<json_t, seq_t>& result_pair, const std::string& expected_json, const std::string& expected_rest){
	ut_verify_json(
		context,
		result_pair.first,
		parse_json(seq_t(expected_json)).first
	);

	ut_verify_string(context, result_pair.second.str(), expected_rest);
}

void ut_verify_string_seq(const quark::call_context_t& context, const std::pair<std::string, seq_t>& result, const std::pair<std::string, seq_t>& expected){
	if(result == expected){
	}
	else{
		ut_verify_string(context, result.first, expected.first);
		ut_verify_string(context, result.second.str(), expected.second.str());
	}
}




////////////////////////////////	make_ast_node()



json_t make_ast_node(const location_t& location, const std::string& opcode, const std::vector<json_t>& params){
	if(location == k_no_location){
		std::vector<json_t> e = { json_t(opcode) };
		e.insert(e.end(), params.begin(), params.end());
		return json_t(e);
	}
	else{
		const auto offset = static_cast<double>(location.offset);
		std::vector<json_t> e = { json_t(offset), json_t(opcode) };
		e.insert(e.end(), params.begin(), params.end());
		return json_t(e);
	}
}

QUARK_TEST("", "make_ast_node()", "", ""){
	const auto r = make_ast_node(location_t(1234), "def-struct", std::vector<json_t>{});

	ut_verify_json(QUARK_POS, r, json_t::make_array({ 1234.0, "def-struct" }));
}




////////////////////////////////		ENCODE / DECODE LINK NAMES



static const std::string k_floyd_func_link_prefix = "floydf_";


link_name_t encode_floyd_func_link_name(const std::string& name){
	return link_name_t { k_floyd_func_link_prefix + name };
}
std::string decode_floyd_func_link_name(const link_name_t& name){
	const auto left = name.s. substr(0, k_floyd_func_link_prefix.size());
	const auto right = name.s.substr(k_floyd_func_link_prefix.size(), std::string::npos);
	QUARK_ASSERT(left == k_floyd_func_link_prefix);
	return right;
}



static const std::string k_runtime_func_link_prefix = "floyd_runtime_";

link_name_t encode_runtime_func_link_name(const std::string& name){
	return link_name_t { k_runtime_func_link_prefix + name };
}

std::string decode_runtime_func_link_name(const link_name_t& name){
	const auto left = name.s.substr(0, k_runtime_func_link_prefix.size());
	const auto right = name.s.substr(k_runtime_func_link_prefix.size(), std::string::npos);
	QUARK_ASSERT(left == k_runtime_func_link_prefix);
	return right;
}



static const std::string k_intrinsic_link_prefix = "floyd_intrinsic_";

link_name_t encode_intrinsic_link_name(const std::string& name){
	return link_name_t { k_intrinsic_link_prefix + name };
}
std::string decode_intrinsic_link_name(const link_name_t& name){
	const auto left = name.s. substr(0, k_intrinsic_link_prefix.size());
	const auto right = name.s.substr(k_intrinsic_link_prefix.size(), std::string::npos);
	QUARK_ASSERT(left == k_intrinsic_link_prefix);
	return right;
}


}	// floyd

