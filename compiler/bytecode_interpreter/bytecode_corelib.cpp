//
//  bytecode_corelib.cpp
//  Floyd
//
//  Created by Marcus Zetterquist on 2018-02-23.
//  Copyright © 2018 Marcus Zetterquist. All rights reserved.
//

#include "bytecode_corelib.h"

#include "json_support.h"
#include "ast_value.h"

#include "floyd_runtime.h"
#include "floyd_corelib.h"
#include "bytecode_helpers.h"


#include "floyd_sockets.h"
#include "floyd_http.h"
#include "floyd_network_component.h"


namespace floyd {


static const bool k_trace = false;


bc_value_t bc_corelib__make_benchmark_report(interpreter_t& vm, const bc_value_t args[], int arg_count){
	QUARK_ASSERT(vm.check_invariant());
	QUARK_ASSERT(arg_count == 1);

	auto temp_types = vm._imm->_program._types;

	const auto& symbols = vm._imm->_program._globals._symbols;
	const auto it = std::find_if(symbols.begin(), symbols.end(), [&](const auto& s){ return s.first == "benchmark_result2_t"; } );
	QUARK_ASSERT(it != symbols.end());

//	const auto benchmark_result2_vec_type = make_vector(temp_types, it->second._value_type);

//	QUARK_ASSERT(args[0]._type == benchmark_result2_t__type);

	const auto b2 = bc_to_value(temp_types, args[0]);
	const auto test_results = unpack_vec_benchmark_result2_t(temp_types, b2);
	const auto report = make_benchmark_report(test_results);
	return value_to_bc(temp_types, value_t::make_string(report));
}




bc_value_t bc_corelib__detect_hardware_caps(interpreter_t& vm, const bc_value_t args[], int arg_count){
	QUARK_ASSERT(vm.check_invariant());
	QUARK_ASSERT(arg_count == 0);

	const auto& types = vm._imm->_program._types;
	const std::vector<std::pair<std::string, json_t>> caps = corelib_detect_hardware_caps();

	std::map<std::string, value_t> caps_map;
	for(const auto& e: caps){
  		caps_map.insert({ e.first, value_t::make_json(e.second) });
	}

	const auto a = value_t::make_dict_value(types, type_t::make_json(), caps_map);
	return value_to_bc(types, a);
}



bc_value_t bc_corelib__make_hardware_caps_report(interpreter_t& vm, const bc_value_t args[], int arg_count){
	QUARK_ASSERT(vm.check_invariant());
	QUARK_ASSERT(arg_count == 1);

	auto temp_types = vm._imm->_program._types;
	QUARK_ASSERT(args[0]._type == make_dict(temp_types, type_t::make_json()));

	const auto b2 = bc_to_value(temp_types, args[0]);
	const auto m = b2.get_dict_value();
	std::vector<std::pair<std::string, json_t>> caps;
	for(const auto& e: m){
		caps.push_back({ e.first, e.second.get_json() });
	}
	const auto s = corelib_make_hardware_caps_report(caps);
	return value_to_bc(temp_types, value_t::make_string(s));
}
bc_value_t bc_corelib__make_hardware_caps_report_brief(interpreter_t& vm, const bc_value_t args[], int arg_count){
	QUARK_ASSERT(vm.check_invariant());
	QUARK_ASSERT(arg_count == 1);

	auto temp_types = vm._imm->_program._types;
	QUARK_ASSERT(args[0]._type == make_dict(temp_types, type_t::make_json()));

	const auto b2 = bc_to_value(temp_types, args[0]);
	const auto m = b2.get_dict_value();
	std::vector<std::pair<std::string, json_t>> caps;
	for(const auto& e: m){
		caps.push_back({ e.first, e.second.get_json() });
	}
	const auto s = corelib_make_hardware_caps_report_brief(caps);
	return value_to_bc(temp_types, value_t::make_string(s));
}
bc_value_t bc_corelib__get_current_date_and_time_string(interpreter_t& vm, const bc_value_t args[], int arg_count){
	QUARK_ASSERT(vm.check_invariant());
	QUARK_ASSERT(arg_count == 0);

	auto temp_types = vm._imm->_program._types;
	const auto s = get_current_date_and_time_string();
	return value_to_bc(temp_types, value_t::make_string(s));
}







/////////////////////////////////////////		PURE -- SHA1



bc_value_t bc_corelib__calc_string_sha1(interpreter_t& vm, const bc_value_t args[], int arg_count){
	QUARK_ASSERT(vm.check_invariant());
	QUARK_ASSERT(arg_count == 1);

	const auto& types = vm._imm->_program._types;
	QUARK_ASSERT(peek2(types, args[0]._type).is_string());

	auto temp_types = types;

	const auto& s = args[0].get_string_value();
	const auto ascii40 = corelib_calc_string_sha1(s);

	const auto result = value_t::make_struct_value(
		temp_types,
		make__sha1_t__type(temp_types),
		{
			value_t::make_string(ascii40)
		}
	);

	if(k_trace && false){
		const auto debug = value_and_type_to_json(vm._imm->_program._types, result);
		QUARK_TRACE(json_to_pretty_string(debug));
	}

	const auto v = value_to_bc(vm._imm->_program._types, result);
	return v;
}



bc_value_t bc_corelib__calc_binary_sha1(interpreter_t& vm, const bc_value_t args[], int arg_count){
	QUARK_ASSERT(vm.check_invariant());
	QUARK_ASSERT(arg_count == 1);
//	QUARK_ASSERT(args[0]._type == make__binary_t__type());

	const auto& types = vm._imm->_program._types;

	auto temp_types = types;
	const auto& sha1_struct = args[0].get_struct_value();
	QUARK_ASSERT(sha1_struct.size() == peek2(temp_types, make__binary_t__type(temp_types)).get_struct(temp_types)._members.size());
	QUARK_ASSERT(peek2(types, sha1_struct[0]._type).is_string());

	const auto& sha1_string = sha1_struct[0].get_string_value();
	const auto ascii40 = corelib_calc_string_sha1(sha1_string);

	const auto result = value_t::make_struct_value(
		temp_types,
		make__sha1_t__type(temp_types),
		{
			value_t::make_string(ascii40)
		}
	);

	if(k_trace && false){
		const auto debug = value_and_type_to_json(vm._imm->_program._types, result);
		QUARK_TRACE(json_to_pretty_string(debug));
	}

	const auto v = value_to_bc(vm._imm->_program._types, result);
	return v;
}



bc_value_t bc_corelib__get_time_of_day(interpreter_t& vm, const bc_value_t args[], int arg_count){
	QUARK_ASSERT(vm.check_invariant());
	QUARK_ASSERT(arg_count == 0);

	const auto result = corelib__get_time_of_day();
	const auto result2 = bc_value_t::make_int(result);
	return result2;
}




/////////////////////////////////////////		IMPURE -- FILE SYSTEM




bc_value_t bc_corelib__read_text_file(interpreter_t& vm, const bc_value_t args[], int arg_count){
	QUARK_ASSERT(vm.check_invariant());
	QUARK_ASSERT(arg_count == 1);

	const auto& types = vm._imm->_program._types;
	QUARK_ASSERT(peek2(types, args[0]._type).is_string());

	const std::string source_path = args[0].get_string_value();
	std::string file_contents = corelib_read_text_file(source_path);
	const auto v = bc_value_t::make_string(file_contents);
	return v;
}

bc_value_t bc_corelib__write_text_file(interpreter_t& vm, const bc_value_t args[], int arg_count){
	QUARK_ASSERT(vm.check_invariant());
	QUARK_ASSERT(arg_count == 2);
	const auto& types = vm._imm->_program._types;
	QUARK_ASSERT(peek2(types, args[0]._type).is_string());
	QUARK_ASSERT(peek2(types, args[1]._type).is_string());

	const std::string path = args[0].get_string_value();
	const std::string file_contents = args[1].get_string_value();

	corelib_write_text_file(path, file_contents);

	return bc_value_t();
}

bc_value_t bc_corelib__read_line_stdin(interpreter_t& vm, const bc_value_t args[], int arg_count){
	QUARK_ASSERT(vm.check_invariant());
	QUARK_ASSERT(arg_count == 0);

	std::string file_contents = corelib_read_line_stdin();
	const auto v = bc_value_t::make_string(file_contents);
	return v;
}




bc_value_t bc_corelib__get_fsentries_shallow(interpreter_t& vm, const bc_value_t args[], int arg_count){
	QUARK_ASSERT(vm.check_invariant());
	QUARK_ASSERT(arg_count == 1);
	const auto& types = vm._imm->_program._types;
	QUARK_ASSERT(peek2(types, args[0]._type).is_string());

	auto temp_types = vm._imm->_program._types;
	const std::string path = args[0].get_string_value();

	const auto a = corelib_get_fsentries_shallow(path);

	const auto elements = directory_entries_to_values(temp_types, a);
	const auto k_fsentry_t__type = make__fsentry_t__type(temp_types);
	const auto vec2 = value_t::make_vector_value(temp_types, k_fsentry_t__type, elements);

	if(k_trace && false){
		const auto debug = value_and_type_to_json(temp_types, vec2);
		QUARK_TRACE(json_to_pretty_string(debug));
	}

	const auto v = value_to_bc(vm._imm->_program._types, vec2);

	return v;
}

bc_value_t bc_corelib__get_fsentries_deep(interpreter_t& vm, const bc_value_t args[], int arg_count){
	QUARK_ASSERT(vm.check_invariant());
	QUARK_ASSERT(arg_count == 1);
	const auto& types = vm._imm->_program._types;
	QUARK_ASSERT(peek2(types, args[0]._type).is_string());

	auto temp_types = vm._imm->_program._types;
	const std::string path = args[0].get_string_value();

	const auto a = corelib_get_fsentries_deep(path);

	const auto elements = directory_entries_to_values(temp_types, a);
	const auto k_fsentry_t__type = make__fsentry_t__type(temp_types);
	const auto vec2 = value_t::make_vector_value(temp_types, k_fsentry_t__type, elements);

	if(k_trace && false){
		const auto debug = value_and_type_to_json(temp_types, vec2);
		QUARK_TRACE(json_to_pretty_string(debug));
	}

	const auto v = value_to_bc(vm._imm->_program._types, vec2);

	return v;
}

bc_value_t bc_corelib__get_fsentry_info(interpreter_t& vm, const bc_value_t args[], int arg_count){
	QUARK_ASSERT(vm.check_invariant());
	QUARK_ASSERT(arg_count == 1);
	const auto& types = vm._imm->_program._types;
	QUARK_ASSERT(peek2(types, args[0]._type).is_string());

	auto temp_types = vm._imm->_program._types;
	const std::string path = args[0].get_string_value();

	const auto info = corelib_get_fsentry_info(path);

	const auto info2 = pack_fsentry_info(temp_types, info);
	const auto v = value_to_bc(vm._imm->_program._types, info2);
	return v;
}


bc_value_t bc_corelib__get_fs_environment(interpreter_t& vm, const bc_value_t args[], int arg_count){
	QUARK_ASSERT(vm.check_invariant());
	QUARK_ASSERT(arg_count == 0);

	auto temp_types = vm._imm->_program._types;
	const auto env = corelib_get_fs_environment();

	const auto result = pack_fs_environment_t(temp_types, env);
	const auto v = value_to_bc(vm._imm->_program._types, result);
	return v;
}


bc_value_t bc_corelib__does_fsentry_exist(interpreter_t& vm, const bc_value_t args[], int arg_count){
	QUARK_ASSERT(vm.check_invariant());
	QUARK_ASSERT(arg_count == 1);
	const auto& types = vm._imm->_program._types;
	QUARK_ASSERT(peek2(types, args[0]._type).is_string());

	auto temp_types = vm._imm->_program._types;
	const std::string path = args[0].get_string_value();

	bool exists = corelib_does_fsentry_exist(path);

	const auto result = value_t::make_bool(exists);
	if(k_trace && false){
		const auto debug = value_and_type_to_json(vm._imm->_program._types, result);
		QUARK_TRACE(json_to_pretty_string(debug));
	}

	const auto v = value_to_bc(vm._imm->_program._types, result);
	return v;
}


bc_value_t bc_corelib__create_directory_branch(interpreter_t& vm, const bc_value_t args[], int arg_count){
	QUARK_ASSERT(vm.check_invariant());
	QUARK_ASSERT(arg_count == 1);
	const auto& types = vm._imm->_program._types;
	QUARK_ASSERT(peek2(types, args[0]._type).is_string());

	const std::string path = args[0].get_string_value();

	corelib_create_directory_branch(path);

	return bc_value_t::make_void();
}

bc_value_t bc_corelib__delete_fsentry_deep(interpreter_t& vm, const bc_value_t args[], int arg_count){
	QUARK_ASSERT(vm.check_invariant());
	QUARK_ASSERT(arg_count == 1);
	const auto& types = vm._imm->_program._types;
	QUARK_ASSERT(peek2(types, args[0]._type).is_string());

	const std::string path = args[0].get_string_value();

	corelib_delete_fsentry_deep(path);

	return bc_value_t::make_void();
}


bc_value_t bc_corelib__rename_fsentry(interpreter_t& vm, const bc_value_t args[], int arg_count){
	QUARK_ASSERT(vm.check_invariant());
	QUARK_ASSERT(arg_count == 2);
	const auto& types = vm._imm->_program._types;
	QUARK_ASSERT(peek2(types, args[0]._type).is_string());
	QUARK_ASSERT(peek2(types, args[1]._type).is_string());

	const std::string path = args[0].get_string_value();
	const std::string n = args[1].get_string_value();

	corelib_rename_fsentry(path, n);

	return bc_value_t::make_void();
}


//######################################################################################################################
//	NETWORK COMPONENT
//######################################################################################################################


bc_value_t bc_corelib__read_socket(interpreter_t& vm, const bc_value_t args[], int arg_count){
	QUARK_ASSERT(vm.check_invariant());
	QUARK_ASSERT(arg_count == 1);
	const auto& types = vm._imm->_program._types;
	QUARK_ASSERT(peek2(types, args[0]._type).is_int());

	const auto socket_id = args[0].get_int_value();
	const auto r = read_socket_string((int)socket_id);
	return bc_value_t::make_string(r);
}
bc_value_t bc_corelib__write_socket(interpreter_t& vm, const bc_value_t args[], int arg_count){
	QUARK_ASSERT(vm.check_invariant());
	QUARK_ASSERT(arg_count == 1);
	QUARK_ASSERT(arg_count == 2);
	const auto& types = vm._imm->_program._types;
	QUARK_ASSERT(peek2(types, args[0]._type).is_int());
	QUARK_ASSERT(peek2(types, args[1]._type).is_string());

	const auto socket_id = args[0].get_int_value();
	const auto& data = args[1].get_string_value();
	write_socket_string((int)socket_id, data);
	return bc_value_t::make_void();
}

static bc_value_t make__ip_address_t(const types_t& types, const ip_address_t& value){
	const auto ip_address_t__type = lookup_type_from_name(types, type_name_t{{ "global_scope", "ip_address_t" }});

	return bc_value_t::make_struct_value(
		ip_address_t__type,
		{
			bc_value_t::make_string(unmake_ipv4(value))
		}
	);
}

static bc_value_t make__host_info_t(const types_t& types, const hostent_t& value){
	const auto name_aliases = mapf<bc_value_t>(value.name_aliases, [](const auto& e){ return bc_value_t::make_string(e); });
	const auto addresses_IPv4 = mapf<bc_value_t>(value.addresses_IPv4, [types](const auto& e){ return make__ip_address_t(types, e); });

	const auto host_info_t__type = lookup_type_from_name(types, type_name_t{{ "global_scope", "host_info_t" }});
	const auto ip_address_t__type = lookup_type_from_name(types, type_name_t{{ "global_scope", "ip_address_t" }});
	return bc_value_t::make_struct_value(
		host_info_t__type,
		{
			bc_value_t::make_string(value.official_host_name),
			make_vector(types, type_t::make_string(), immer::vector<bc_value_t>(name_aliases.begin(), name_aliases.end())),
			make_vector(types, ip_address_t__type, immer::vector<bc_value_t>(addresses_IPv4.begin(), addresses_IPv4.end())),
		}
	);
}

bc_value_t bc_corelib__lookup_host_from_ip(interpreter_t& vm, const bc_value_t args[], int arg_count){
	QUARK_ASSERT(vm.check_invariant());
	QUARK_ASSERT(arg_count == 1);
	const auto& types = vm._imm->_program._types;
	QUARK_ASSERT(args[0]._type == make__ip_address_t__type(types));

	const auto addr = args[0].get_struct_value();
	const auto addr2 = make_ipv4(addr[0].get_string_value());
	const auto r = lookup_host(addr2);
	return make__host_info_t(types, r);
}

bc_value_t bc_corelib__lookup_host_from_name(interpreter_t& vm, const bc_value_t args[], int arg_count){
	QUARK_ASSERT(vm.check_invariant());
	QUARK_ASSERT(arg_count == 1);
	const auto& types = vm._imm->_program._types;
	QUARK_ASSERT(peek2(types, args[0]._type).is_string());

	const auto name = args[0].get_string_value();
	const auto r = lookup_host(name);

	return make__host_info_t(types, r);
}



bc_value_t bc_corelib__pack_http_request(interpreter_t& vm, const bc_value_t args[], int arg_count){
	QUARK_ASSERT(vm.check_invariant());
	QUARK_ASSERT(arg_count == 1);
	const auto& types = vm._imm->_program._types;

	const auto http_request_t__type = lookup_type_from_name(types, type_name_t{{ "global_scope", "http_request_t" }});
	QUARK_ASSERT(args[0]._type == http_request_t__type);

	const auto request = args[0].get_struct_value();

	const auto request_line = request[0].get_struct_value();
	const auto headers = get_vector(types, request[1]);
	const auto optional_body = request[2].get_string_value();

	const auto headers2 = mapf<http_header_t>(headers, [](const auto& e){
		const auto& header = e.get_struct_value();
		return http_header_t { header[0].get_string_value(), header[1].get_string_value() };
	});
	const auto req = http_request_t {
		http_request_line_t {
			request_line[0].get_string_value(),
			request_line[1].get_string_value(),
			request_line[2].get_string_value()
		},
		headers2,
		optional_body
	};
	const auto r = pack_http_request(req);
	return bc_value_t::make_string(r);
}

bc_value_t bc_corelib__unpack_http_request(interpreter_t& vm, const bc_value_t args[], int arg_count){
	QUARK_ASSERT(vm.check_invariant());
	QUARK_ASSERT(arg_count == 1);
	const auto& types = vm._imm->_program._types;
	QUARK_ASSERT(peek2(types, args[0]._type).is_string());

	const auto http_header_t__type = lookup_type_from_name(types, type_name_t{{ "global_scope", "http_header_t" }});
	const auto http_request_t__type = lookup_type_from_name(types, type_name_t{{ "global_scope", "http_request_t_" }});
	const auto http_request_line_t___type = lookup_type_from_name(types, type_name_t{{ "global_scope", "http_request_line_t" }});

	const auto s = args[0].get_string_value();
	const http_request_t req = unpack_http_request(s);

	const auto headers2 = mapf<bc_value_t>(req.headers, [&](const auto& e){
		return bc_value_t::make_struct_value(
			http_header_t__type,
			{
				bc_value_t::make_string(e.key),
				bc_value_t::make_string(e.value)
			}
		);
	});

	return bc_value_t::make_struct_value(
		http_request_t__type,
		{
			bc_value_t::make_struct_value(
				http_request_line_t___type,
				{
					bc_value_t::make_string(req.request_line.method),
					bc_value_t::make_string(req.request_line.uri),
					bc_value_t::make_string(req.request_line.http_version)
				}
			),
			make_vector(types, http_header_t__type, immer::vector<bc_value_t>(headers2.begin(), headers2.end())),
			bc_value_t::make_string(req.optional_body)
		}
	);
}



bc_value_t bc_corelib__pack_http_response(interpreter_t& vm, const bc_value_t args[], int arg_count){
	QUARK_ASSERT(vm.check_invariant());
	QUARK_ASSERT(arg_count == 1);
	const auto& types = vm._imm->_program._types;

	const auto http_response_t__type = lookup_type_from_name(types, type_name_t{{ "global_scope", "http_response_t" }});
	QUARK_ASSERT(args[0]._type == http_response_t__type);

	const auto status_line = args[0].get_struct_value();
	const auto headers = get_vector(types, args[1]);
	const auto optional_body = args[2].get_string_value();

	const auto headers2 = mapf<http_header_t>(headers, [](const auto& e){
		const auto& header = e.get_struct_value();
		return http_header_t { header[0].get_string_value(), header[1].get_string_value() };
	});
	const auto req = http_response_t {
		http_response_status_line_t {
			status_line[0].get_string_value(),
			status_line[1].get_string_value()
		},
		headers2,
		optional_body
	};
	const auto r = pack_http_response(req);
	return bc_value_t::make_string(r);
}

bc_value_t bc_corelib__unpack_http_response(interpreter_t& vm, const bc_value_t args[], int arg_count){
	QUARK_ASSERT(vm.check_invariant());
	QUARK_ASSERT(arg_count == 1);
	const auto& types = vm._imm->_program._types;
	QUARK_ASSERT(peek2(types, args[0]._type).is_string());

	const auto s = args[0].get_string_value();
	const http_response_t response = unpack_http_response(s);

	const auto http_header_t__type = lookup_type_from_name(types, type_name_t{{ "global_scope", "http_header_t" }});
	const auto http_response_t__type = lookup_type_from_name(types, type_name_t{{ "global_scope", "http_response_t" }});
	const auto http_response_status_line_t__type = lookup_type_from_name(types, type_name_t{{ "global_scope", "http_response_status_line_t" }});

	const auto headers2 = mapf<bc_value_t>(response.headers, [&](const auto& e){
		return bc_value_t::make_struct_value(
			http_header_t__type,
			{
				bc_value_t::make_string(e.key),
				bc_value_t::make_string(e.value)
			}
		);
	});

	return bc_value_t::make_struct_value(
		http_response_t__type,
		{
			bc_value_t::make_struct_value(
				http_response_status_line_t__type,
				{
					bc_value_t::make_string(response.status_line.http_version),
					bc_value_t::make_string(response.status_line.status_code),
				}
			),
			make_vector(types, http_header_t__type, immer::vector<bc_value_t>(headers2.begin(), headers2.end())),
			bc_value_t::make_string(response.optional_body)
		}
	);
}


bc_value_t bc_corelib__execute_http_request(interpreter_t& vm, const bc_value_t args[], int arg_count){
	QUARK_ASSERT(vm.check_invariant());
	const auto& types = vm._imm->_program._types;
	QUARK_ASSERT(arg_count == 3);

	const auto network_component_t__type = lookup_type_from_name(types, type_name_t{{ "global_scope", "network_component_t" }});
	const auto ip_address_and_port_t__type = lookup_type_from_name(types, type_name_t{{ "global_scope", "ip_address_and_port_t" }});

	QUARK_ASSERT(args[0]._type == network_component_t__type);
	QUARK_ASSERT(args[1]._type == ip_address_and_port_t__type);
	QUARK_ASSERT(peek2(types, args[2]._type).is_string());

	const auto addr = args[1].get_struct_value();
	const auto request = args[2].get_string_value();
	const auto addr2 = ip_address_t { make_ipv4(addr[0].get_struct_value()[0].get_string_value()) };
	const auto response = execute_http_request(ip_address_and_port_t { addr2, (int)addr[1].get_int_value() }, request);
	return bc_value_t::make_string(response);
}



/////////////////////////////////////////		REGISTRY



std::map<function_id_t, BC_NATIVE_FUNCTION_PTR> bc_get_corelib_calls(){

	const auto result = std::map<function_id_t, BC_NATIVE_FUNCTION_PTR>{
		{ { "make_benchmark_report" }, bc_corelib__make_benchmark_report },

		{ { "detect_hardware_caps" }, bc_corelib__detect_hardware_caps },
		{ { "make_hardware_caps_report" }, bc_corelib__make_hardware_caps_report },
		{ { "make_hardware_caps_report_brief" }, bc_corelib__make_hardware_caps_report_brief },
		{ { "get_current_date_and_time_string" }, bc_corelib__get_current_date_and_time_string },

		{ { "calc_string_sha1" }, bc_corelib__calc_string_sha1 },
		{ { "calc_binary_sha1" }, bc_corelib__calc_binary_sha1 },

		{ { "get_time_of_day" }, bc_corelib__get_time_of_day },

		{ { "read_text_file" }, bc_corelib__read_text_file },
		{ { "write_text_file" }, bc_corelib__write_text_file },
		{ { "read_line_stdin" }, bc_corelib__read_line_stdin },

		{ { "get_fsentries_shallow" }, bc_corelib__get_fsentries_shallow },
		{ { "get_fsentries_deep" }, bc_corelib__get_fsentries_deep },
		{ { "get_fsentry_info" }, bc_corelib__get_fsentry_info },
		{ { "get_fs_environment" }, bc_corelib__get_fs_environment },
		{ { "does_fsentry_exist" }, bc_corelib__does_fsentry_exist },
		{ { "create_directory_branch" }, bc_corelib__create_directory_branch },
		{ { "delete_fsentry_deep" }, bc_corelib__delete_fsentry_deep },
		{ { "rename_fsentry" }, bc_corelib__rename_fsentry },

		{ { "read_socket" }, bc_corelib__read_socket },
		{ { "write_socket" }, bc_corelib__write_socket },
		{ { "lookup_host_from_ip" }, bc_corelib__lookup_host_from_ip },
		{ { "lookup_host_from_name" }, bc_corelib__lookup_host_from_name },
		{ { "to_ipv4_dotted_decimal_string" }, nullptr },
		{ { "from_ipv4_dotted_decimal_string" }, nullptr },
		{ { "pack_http_request" }, bc_corelib__pack_http_request },
		{ { "unpack_http_request" }, bc_corelib__unpack_http_request },
		{ { "pack_http_response" }, bc_corelib__pack_http_response },
		{ { "unpack_http_response" }, bc_corelib__unpack_http_response },
		{ { "execute_http_request" }, bc_corelib__execute_http_request }
	};
	return result;
}


}	// floyd
