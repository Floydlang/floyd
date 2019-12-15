//
//  floyd_llvm_runtime.cpp
//  Floyd
//
//  Created by Marcus Zetterquist on 2019-04-24.
//  Copyright © 2019 Marcus Zetterquist. All rights reserved.
//

#include "floyd_llvm_corelib.h"

#include "floyd_llvm_runtime.h"
#include "floyd_runtime.h"
#include "floyd_corelib.h"
#include "text_parser.h"
#include "file_handling.h"
#include "os_process.h"


#include "floyd_sockets.h"
#include "floyd_http.h"
#include "floyd_network_component.h"


#include <iostream>
#include <fstream>

namespace floyd {





struct native_binary_t {
	VECTOR_CARRAY_T* ascii40;
};

struct native_sha1_t {
	VECTOR_CARRAY_T* ascii40;
};



//		func string make_benchmark_report([benchmark_result2_t] results)
static runtime_value_t llvm_corelib__make_benchmark_report(floyd_runtime_t* frp, const runtime_value_t b){
	auto& r = get_floyd_runtime(frp);
	auto& backend = r.ee->backend;
	const auto& types = backend.types;

	const auto s = find_symbol_required(r.ee->global_symbols, "benchmark_result2_t");
	const auto benchmark_result2_vec_type = type_t::make_vector(types, s._value_type);

	const auto b2 = from_runtime_value(r, b, benchmark_result2_vec_type);

	auto temp_types = types;
	const auto test_results = unpack_vec_benchmark_result2_t(temp_types, b2);
	QUARK_ASSERT(types.nodes.size() == temp_types.nodes.size());

	const auto report = make_benchmark_report(test_results);
	auto result = to_runtime_string(r, report);
	return result;
}




static DICT_CPPMAP_T* llvm_corelib__detect_hardware_caps(floyd_runtime_t* frp){
	auto& r = get_floyd_runtime(frp);
	auto& backend = r.ee->backend;

	const auto& types = backend.types;
	const std::vector<std::pair<std::string, json_t>> caps = corelib_detect_hardware_caps();

	std::map<std::string, value_t> caps_map;
	for(const auto& e: caps){
  		caps_map.insert({ e.first, value_t::make_json(e.second) });
	}

	const auto a = value_t::make_dict_value(types, type_t::make_json(), caps_map);
	auto result = to_runtime_value(r, a);
	return result.dict_cppmap_ptr;
}

runtime_value_t llvm_corelib__make_hardware_caps_report(floyd_runtime_t* frp, runtime_value_t caps0){
	auto& r = get_floyd_runtime(frp);
	auto& backend = r.ee->backend;
	auto& types = backend.types;

	const auto type = type_t::make_dict(types, type_t::make_json());
	const auto b2 = from_runtime_value(r, caps0, type);
	const auto m = b2.get_dict_value();
	std::vector<std::pair<std::string, json_t>> caps;
	for(const auto& e: m){
		caps.push_back({ e.first, e.second.get_json() });
	}
	const auto s = corelib_make_hardware_caps_report(caps);
	return to_runtime_string(r, s);
}
runtime_value_t llvm_corelib__make_hardware_caps_report_brief(floyd_runtime_t* frp, runtime_value_t caps0){
	auto& r = get_floyd_runtime(frp);
	auto& backend = r.ee->backend;
	auto& types = backend.types;

	const auto b2 = from_runtime_value(r, caps0, type_t::make_dict(types, type_t::make_json()));
	const auto m = b2.get_dict_value();
	std::vector<std::pair<std::string, json_t>> caps;
	for(const auto& e: m){
		caps.push_back({ e.first, e.second.get_json() });
	}
	const auto s = corelib_make_hardware_caps_report_brief(caps);
	return to_runtime_string(r, s);
}
runtime_value_t llvm_corelib__get_current_date_and_time_string(floyd_runtime_t* frp){
	auto& r = get_floyd_runtime(frp);

	const auto s = get_current_date_and_time_string();
	return to_runtime_string(r, s);
}








static STRUCT_T* llvm_corelib__calc_string_sha1(floyd_runtime_t* frp, runtime_value_t s0){
	auto& r = get_floyd_runtime(frp);
	auto& backend = r.ee->backend;
	auto& types = backend.types;

	const auto& s = from_runtime_string(r, s0);
	const auto ascii40 = corelib_calc_string_sha1(s);

	const auto a = value_t::make_struct_value(
		types,
		type_t::make_struct(types, struct_type_desc_t({ member_t{ type_t::make_string(), "ascii40" } }) ),
		{ value_t::make_string(ascii40) }
	);

	auto result = to_runtime_value(r, a);
	return result.struct_ptr;
}

static STRUCT_T* llvm_corelib__calc_binary_sha1(floyd_runtime_t* frp, STRUCT_T* binary_ptr){
	auto& r = get_floyd_runtime(frp);
	auto& backend = r.ee->backend;
	QUARK_ASSERT(binary_ptr != nullptr);
	auto& types = backend.types;

	const auto& binary = *reinterpret_cast<const native_binary_t*>(binary_ptr->get_data_ptr());

	const auto& s = from_runtime_string(r, make_runtime_vector_carray(binary.ascii40));
	const auto ascii40 = corelib_calc_string_sha1(s);

	const auto a = value_t::make_struct_value(
		types,
		type_t::make_struct(types, struct_type_desc_t({ member_t{ type_t::make_string(), "ascii40" } })),
		{ value_t::make_string(ascii40) }
	);

	auto result = to_runtime_value(r, a);
	return result.struct_ptr;
}



static int64_t llvm_corelib__get_time_of_day(floyd_runtime_t* frp){
	get_floyd_runtime(frp);

	return corelib__get_time_of_day();
}



static runtime_value_t llvm_corelib__read_text_file(floyd_runtime_t* frp, runtime_value_t arg){
	auto& r = get_floyd_runtime(frp);

	const auto path = from_runtime_string(r, arg);
	const auto file_contents = 	corelib_read_text_file(path);
	return to_runtime_string(r, file_contents);
}

static void llvm_corelib__write_text_file(floyd_runtime_t* frp, runtime_value_t path0, runtime_value_t data0){
	auto& r = get_floyd_runtime(frp);

	const auto path = from_runtime_string(r, path0);
	const auto file_contents = from_runtime_string(r, data0);
	corelib_write_text_file(path, file_contents);
}

static runtime_value_t llvm_corelib__read_line_stdin(floyd_runtime_t* frp){
	auto& r = get_floyd_runtime(frp);
	const auto s = 	corelib_read_line_stdin();
	return to_runtime_string(r, s);
}




static VECTOR_CARRAY_T* llvm_corelib__get_fsentries_shallow(floyd_runtime_t* frp, runtime_value_t path0){
	auto& r = get_floyd_runtime(frp);
	auto& backend = r.ee->backend;
	auto& types = backend.types;

	const auto path = from_runtime_string(r, path0);

	const auto a = corelib_get_fsentries_shallow(path);

	const auto elements = directory_entries_to_values(types, a);
	const auto k_fsentry_t__type = make__fsentry_t__type(types);
	const auto vec2 = value_t::make_vector_value(types, k_fsentry_t__type, elements);

#if 1
	const auto debug = value_and_type_to_json(types, vec2);
	QUARK_TRACE(json_to_pretty_string(debug));
#endif

	const auto v = to_runtime_value(r, vec2);
	return v.vector_carray_ptr;
}

static VECTOR_CARRAY_T* llvm_corelib__get_fsentries_deep(floyd_runtime_t* frp, runtime_value_t path0){
	auto& r = get_floyd_runtime(frp);
	auto& backend = r.ee->backend;
	auto& types = backend.types;

	const auto path = from_runtime_string(r, path0);

	const auto a = corelib_get_fsentries_deep(path);

	const auto elements = directory_entries_to_values(types, a);
	const auto k_fsentry_t__type = make__fsentry_t__type(types);
	const auto vec2 = value_t::make_vector_value(types, k_fsentry_t__type, elements);

#if 1
	const auto debug = value_and_type_to_json(types, vec2);
	QUARK_TRACE(json_to_pretty_string(debug));
#endif

	const auto v = to_runtime_value(r, vec2);
	return v.vector_carray_ptr;
}

static STRUCT_T* llvm_corelib__get_fsentry_info(floyd_runtime_t* frp, runtime_value_t path0){
	auto& r = get_floyd_runtime(frp);
	auto& backend = r.ee->backend;
	auto& types = backend.types;
	const auto path = from_runtime_string(r, path0);

	const auto info = corelib_get_fsentry_info(path);

	const auto info2 = pack_fsentry_info(types, info);

	const auto v = to_runtime_value(r, info2);
	return v.struct_ptr;
}

static runtime_value_t llvm_corelib__get_fs_environment(floyd_runtime_t* frp){
	auto& r = get_floyd_runtime(frp);
	auto& backend = r.ee->backend;
	auto& types = backend.types;

	const auto env = corelib_get_fs_environment();

	const auto result = pack_fs_environment_t(types, env);
	const auto v = to_runtime_value(r, result);
	return v;
}




static uint8_t llvm_corelib__does_fsentry_exist(floyd_runtime_t* frp, runtime_value_t path0){
	auto& r = get_floyd_runtime(frp);
	auto& backend = r.ee->backend;
	auto& types = backend.types;
	const auto path = from_runtime_string(r, path0);

	bool exists = corelib_does_fsentry_exist(path);

	const auto result = value_t::make_bool(exists);
#if 1
	const auto debug = value_and_type_to_json(types, result);
	QUARK_TRACE(json_to_pretty_string(debug));
#endif
	return exists ? 0x01 : 0x00;
}

static void llvm_corelib__create_directory_branch(floyd_runtime_t* frp, runtime_value_t path0){
	auto& r = get_floyd_runtime(frp);
	const auto path = from_runtime_string(r, path0);

	corelib_create_directory_branch(path);
}

static void llvm_corelib__delete_fsentry_deep(floyd_runtime_t* frp, runtime_value_t path0){
	auto& r = get_floyd_runtime(frp);

	const auto path = from_runtime_string(r, path0);

	corelib_delete_fsentry_deep(path);
}

static void llvm_corelib__rename_fsentry(floyd_runtime_t* frp, runtime_value_t path0, runtime_value_t name0){
	auto& r = get_floyd_runtime(frp);
	const auto path = from_runtime_string(r, path0);
	const auto n = from_runtime_string(r, name0);

	corelib_rename_fsentry(path, n);
}




//######################################################################################################################
//	NETWORK COMPONENT
//######################################################################################################################



static void llvm_corelib__read_socket(floyd_runtime_t* frp){
}

static void llvm_corelib__write_socket(floyd_runtime_t* frp){
}

static void llvm_corelib__lookup_host_from_ip(floyd_runtime_t* frp){
}

	static value_t make__ip_address_t(llvm_context_t& r, const types_t& types, const ip_address_t& value){
		const auto ip_address_t__type = lookup_type_from_name(types, type_name_t{{ "global_scope", "ip_address_t" }});

		const auto result = value_t::make_struct_value(
			types,
			peek2(types, ip_address_t__type),
			{
				value_t::make_string(unmake_ipv4(value))
			}
		);
		return result;
	}

	static value_t make__host_info_t(llvm_context_t& r, const types_t& types, const hostent_t& value){
		const auto name_aliases = mapf<value_t>(value.name_aliases, [](const auto& e){ return value_t::make_string(e); });
		const auto addresses_IPv4 = mapf<value_t>(value.addresses_IPv4, [&](const auto& e){ return make__ip_address_t(r, types, e); });

//		trace_types(types);
		const auto host_info_t__type = peek2(types, lookup_type_from_name(types, type_name_t{{ "global_scope", "host_info_t" }}));
		const auto ip_address_t__type = peek2(types, lookup_type_from_name(types, type_name_t{{ "global_scope", "ip_address_t" }}));
		const auto result = value_t::make_struct_value(
			types,
			peek2(types, host_info_t__type),
			{
				value_t::make_string(value.official_host_name),
				value_t::make_vector_value(types, type_t::make_string(), name_aliases),
				value_t::make_vector_value(types, ip_address_t__type, addresses_IPv4),
			}
		);
		return result;
	}

static runtime_value_t llvm_corelib__lookup_host_from_name(floyd_runtime_t* frp, runtime_value_t name_str){
	auto& r = get_floyd_runtime(frp);
	auto& backend = r.ee->backend;
	auto& types = backend.types;

//	trace_types(types);
//	trace_llvm_type_lookup(r.ee->type_lookup);

	const auto name = from_runtime_string(r, name_str);
	const auto result = lookup_host(name);
	const auto info = make__host_info_t(r, types, result);
	const auto v = to_runtime_value(r, info);
	return v;
}

static runtime_value_t llvm_corelib__pack_http_request(floyd_runtime_t* frp, runtime_value_t s){
	auto& r = get_floyd_runtime(frp);
	auto& backend = r.ee->backend;
	auto& types = backend.types;

	const auto http_request_t__type = lookup_type_from_name(types, type_name_t{{ "global_scope", "http_request_t" }});

	const auto request = from_runtime_value2(backend, s, http_request_t__type).get_struct_value();

	const auto request_line = request->_member_values[0].get_struct_value();
	const auto headers = request->_member_values[1].get_vector_value();
	const auto optional_body = request->_member_values[2].get_string_value();

	const auto headers2 = mapf<http_header_t>(headers, [](const auto& e){
		const auto& header = e.get_struct_value();
		return http_header_t {
			header->_member_values[0].get_string_value(),
			header->_member_values[1].get_string_value()
		};
	});
	const auto req = http_request_t {
		http_request_line_t {
			request_line->_member_values[0].get_string_value(),
			request_line->_member_values[1].get_string_value(),
			request_line->_member_values[2].get_string_value()
		},
		headers2,
		optional_body
	};
	const auto request_string = pack_http_request(req);
	return to_runtime_string2(backend, request_string);
}

static void llvm_corelib__unpack_http_request(floyd_runtime_t* frp){
}

static void llvm_corelib__pack_http_response(floyd_runtime_t* frp){
}

static void llvm_corelib__unpack_http_response(floyd_runtime_t* frp){
}

static runtime_value_t llvm_corelib__execute_http_request(floyd_runtime_t* frp, runtime_value_t c, runtime_value_t addr, runtime_value_t request_string){
	auto& r = get_floyd_runtime(frp);
	auto& backend = r.ee->backend;
	auto& types = backend.types;

	const auto network_component_t__type = lookup_type_from_name(types, type_name_t{{ "global_scope", "network_component_t" }});
	const auto ip_address_and_port_t__type = lookup_type_from_name(types, type_name_t{{ "global_scope", "ip_address_and_port_t" }});

	const auto addr1 = from_runtime_value2(backend, addr, ip_address_and_port_t__type).get_struct_value();
	const auto request = from_runtime_string2(backend, request_string);

	const auto addr2 = ip_address_t { make_ipv4(addr1->_member_values[0].get_struct_value()->_member_values[0].get_string_value()) };
	const auto response = execute_http_request(ip_address_and_port_t { addr2, (int)addr1->_member_values[1].get_int_value() }, request);
	return to_runtime_string2(backend, response);
}




std::map<std::string, void*> get_corelib_binds(){

	////////////////////////////////		CORE FUNCTIONS AND HOST FUNCTIONS
	const std::map<std::string, void*> host_functions_map = {
		{ "make_benchmark_report", reinterpret_cast<void *>(&llvm_corelib__make_benchmark_report) },

		{ "detect_hardware_caps", reinterpret_cast<void *>(&llvm_corelib__detect_hardware_caps) },
		{ "make_hardware_caps_report", reinterpret_cast<void *>(&llvm_corelib__make_hardware_caps_report) },
		{ "make_hardware_caps_report_brief", reinterpret_cast<void *>(&llvm_corelib__make_hardware_caps_report_brief) },
		{ "get_current_date_and_time_string", reinterpret_cast<void *>(&llvm_corelib__get_current_date_and_time_string) },

		{ "calc_string_sha1", reinterpret_cast<void *>(&llvm_corelib__calc_string_sha1) },
		{ "calc_binary_sha1", reinterpret_cast<void *>(&llvm_corelib__calc_binary_sha1) },

		{ "get_time_of_day", reinterpret_cast<void *>(&llvm_corelib__get_time_of_day) },

		{ "read_text_file", reinterpret_cast<void *>(&llvm_corelib__read_text_file) },
		{ "write_text_file", reinterpret_cast<void *>(&llvm_corelib__write_text_file) },
		{ "read_line_stdin", reinterpret_cast<void *>(&llvm_corelib__read_line_stdin) },

		{ "get_fsentries_shallow", reinterpret_cast<void *>(&llvm_corelib__get_fsentries_shallow) },
		{ "get_fsentries_deep", reinterpret_cast<void *>(&llvm_corelib__get_fsentries_deep) },
		{ "get_fsentry_info", reinterpret_cast<void *>(&llvm_corelib__get_fsentry_info) },
		{ "get_fs_environment", reinterpret_cast<void *>(&llvm_corelib__get_fs_environment) },

		{ "does_fsentry_exist", reinterpret_cast<void *>(&llvm_corelib__does_fsentry_exist) },
		{ "create_directory_branch", reinterpret_cast<void *>(&llvm_corelib__create_directory_branch) },
		{ "delete_fsentry_deep", reinterpret_cast<void *>(&llvm_corelib__delete_fsentry_deep) },
		{ "rename_fsentry", reinterpret_cast<void *>(&llvm_corelib__rename_fsentry) },

		{ "read_socket", reinterpret_cast<void *>(&llvm_corelib__read_socket) },
		{ "write_socket", reinterpret_cast<void *>(&llvm_corelib__write_socket) },
		{ "lookup_host_from_ip", reinterpret_cast<void *>(&llvm_corelib__lookup_host_from_ip) },
		{ "lookup_host_from_name", reinterpret_cast<void *>(&llvm_corelib__lookup_host_from_name) },
		{ "to_ipv4_dotted_decimal_string", nullptr },
		{ "from_ipv4_dotted_decimal_string", nullptr },
		{ "pack_http_request", reinterpret_cast<void *>(&llvm_corelib__pack_http_request) },
		{ "unpack_http_request", reinterpret_cast<void *>(&llvm_corelib__unpack_http_request) },
		{ "pack_http_response", reinterpret_cast<void *>(&llvm_corelib__pack_http_response) },
		{ "unpack_http_response", reinterpret_cast<void *>(&llvm_corelib__unpack_http_response) },
		{ "execute_http_request", reinterpret_cast<void *>(&llvm_corelib__execute_http_request) }
	};
	return host_functions_map;
}


}	//	namespace floyd



