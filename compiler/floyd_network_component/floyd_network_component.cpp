//
//  floyd_network_component.cpp
//  floyd
//
//  Created by Marcus Zetterquist on 2019-12-03.
//  Copyright © 2019 Marcus Zetterquist. All rights reserved.
//

#include "floyd_network_component.h"

#include "floyd_corelib.h"

#include "compiler_basics.h"
#include "floyd_sockets.h"
#include "floyd_http.h"


//??? temporary
#include "bytecode_interpreter.h"


namespace floyd {


static const std::string k_network_component_header = R"(

	//######################################################################################################################
	//	NETWORK COMPONENT
	//######################################################################################################################

	struct network_component_t {
		int internal
	}


	//////////////////////////////////////		BASICS


	struct ip_address_and_port_t {
		ip_address_t addr
		int port
	}

//	func string read_socket(int socket)
//	func void write_socket(int socket, string data)

	struct host_info_t {
		string official_host_name
		[string] name_aliases
		[ip_address_t] addresses_IPv4
	}

	func host_info_t lookup_host_from_ip(ip_address_t addr) impure
	func host_info_t lookup_host_from_name(string name) impure

//	func string to_ipv4_dotted_decimal_string(ip_address_t a)
//	func ip_address_t from_ipv4_dotted_decimal_string(string s)


	//////////////////////////////////////		TCP CLIENT


//	func int connect_to_server(network_component_t c, ip_address_and_port_t server_addr) impure
//	func void close_socket(int socket) impure



	//////////////////////////////////////		TCP SERVER


/*
	struct server_params_t {
		int port
	}

	//	http://localhost:8080/info.html
	//	Will call the process to handle take care of each client-session, uses message of type string.
	func void execute_server(network_component_t c, server_params_t params, string process_id)
*/


	///////////////////////////////		HTTP


	struct http_header_t {
		string key
		string value
	}

	struct http_request_line_t {
		string method
		string uri
		string http_version
	}

	struct http_request_t {
		http_request_line_t request_line
		[http_header_t] headers
		string optional_body
	}

	func string pack_http_request(http_request_t r)
	func http_request_t unpack_http_request(string r)

	func string execute_http_request(network_component_t c, ip_address_and_port_t addr, string request) impure


	struct http_response_status_line_t {
		string http_version
		string status_code
	}
	struct http_response_t {
		http_response_status_line_t status_line
		[http_header_t] headers
		string optional_body
	}

/*
	func int get_time_ns() impure {
		return get_time_of_day() * 1000
	}
*/

//	func string pack_http_response(http_response_t r)
	func http_response_t unpack_http_response(string s)


	///////////////////////////////		EXECUTE HTTP


	//	Blocks for reply.

	//	Blocks forever. ??? how to ask it to stop?
//	func void execute_http_server(network_component_t c, server_params_t params, string process_id) impure

)";


std::string get_network_component_header(){
	return k_network_component_header;
}



//////////////////////////////////////////		TYPES




static type_t make__network_component_t__type(types_t& types){
	return type_t::make_struct(
		types,
		struct_type_desc_t({
			{ type_t::make_int(), "internal" }
		})
	);
}

static type_t make__ip_address_and_port_t__type(types_t& types){
	return type_t::make_struct(
		types,
		struct_type_desc_t({
			{ make__ip_address_t__type(types), "addr" },
			{ type_t::make_int(), "port" }
		})
	);
}

static type_t make__host_info_t__type(types_t& types){
	return type_t::make_struct(
		types,
		struct_type_desc_t({
			{ type_t::make_string(), "official_host_name" },
			{ type_t::make_vector(types, type_t::make_string()), "name_aliases" },
			{ type_t::make_vector(types, make__ip_address_t__type(types)), "addresses_IPv4" }
		})
	);
}

static type_t make__http_header_t__type(types_t& types){
	return type_t::make_struct(
		types,
		struct_type_desc_t({
			{ type_t::make_string(), "key" },
			{ type_t::make_string(), "value" }
		})
	);
}

static type_t make__http_request_line_t__type(types_t& types){
	return type_t::make_struct(
		types,
		struct_type_desc_t({
			{ type_t::make_string(), "method" },
			{ type_t::make_string(), "uri" },
			{ type_t::make_string(), "http_version" }
		})
	);
}

static type_t make__http_request_t__type(types_t& types){
	return type_t::make_struct(
		types,
		struct_type_desc_t({
			{ make__http_request_line_t__type(types), "request_line" },
			{ type_t::make_vector(types, make__http_header_t__type(types)), "headers" },
			{ type_t::make_string(), "optional_body" }
		})
	);
}

static type_t make__http_response_status_line_t__type(types_t& types){
	return type_t::make_struct(
		types,
		struct_type_desc_t({
			{ type_t::make_string(), "http_version" },
			{ type_t::make_string(), "status_code" }
		})
	);
}


static type_t make__http_response_t__type(types_t& types){
	return type_t::make_struct(
		types,
		struct_type_desc_t({
			{ make__http_response_status_line_t__type(types), "status_line" },
			{ type_t::make_vector(types, make__http_header_t__type(types)), "headers" },
			{ type_t::make_string(), "optional_body" }
		})
	);
}


//////////////////////////////////////////		PACK & UNPACK





static rt_value_t to_runtime__ip_address_t(value_backend_t& backend, const ip_address_t& value){
	const auto& types = backend.types;
	const auto ip_address_t__type = lookup_type_from_name(types, type_name_t{{ "global_scope", "ip_address_t" }});

	const auto str4 = unmake_ipv4(value);
	const std::vector<rt_value_t> values = { rt_value_t::make_string(backend, str4) };
	return rt_value_t::make_struct_value(backend, ip_address_t__type, values);
}

static ip_address_t from_runtime_ip_address_t(value_backend_t& backend, rt_pod_t ip_address){
	const auto& types = backend.types;

	const auto members = from_runtime_struct(backend, ip_address, make__ip_address_t__type(types));
	const auto ip_str = members[0].get_string_value(backend);
	const auto addr2 = ip_address_t { make_ipv4(ip_str) };
	return addr2;
}

static rt_value_t to_runtime__host_info_t(value_backend_t& backend, const hostent_t& value){
	const auto& types = backend.types;

	const auto name_aliases = mapf<rt_value_t>(value.name_aliases, [&](const auto& e){ return rt_value_t::make_string(backend, e); });

	const auto addresses_IPv4 = mapf<rt_value_t>(value.addresses_IPv4, [&](const auto& e){ return to_runtime__ip_address_t(backend, e); });
	const auto ip_address_t__type = peek2(types, lookup_type_from_name(types, type_name_t{{ "global_scope", "ip_address_t" }}));

	const auto host_info_t__type = peek2(types, lookup_type_from_name(types, type_name_t{{ "global_scope", "host_info_t" }}));
	const auto result = rt_value_t::make_struct_value(
		backend,
		host_info_t__type,
		{
			rt_value_t::make_string(backend, value.official_host_name),
			make_vector_value(backend, type_t::make_string(), { name_aliases.begin(), name_aliases.end() }),
			make_vector_value(backend, ip_address_t__type, { addresses_IPv4.begin(), addresses_IPv4.end() }),
		}
	);
	return result;
}


static http_request_t from_runtime__http_request_t(value_backend_t& backend, rt_pod_t s){
	auto& types = backend.types;

	const auto http_request_t__type = lookup_type_from_name(types, type_name_t{{ "global_scope", "http_request_t" }});

	//??? Avoid passing via value_t
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
	return req;
}

static rt_value_t to_runtime__http_request_t(value_backend_t& backend, const http_request_t& req){
	auto& types = backend.types;

	const auto http_request_t__type = lookup_type_from_name(types, type_name_t{{ "global_scope", "http_request_t" }});
	const auto http_request_line_t___type = lookup_type_from_name(types, type_name_t{{ "global_scope", "http_request_line_t" }});
	const auto http_header_t__type = lookup_type_from_name(types, type_name_t{{ "global_scope", "http_header_t" }});

	const auto headers2 = mapf<rt_value_t>(req.headers, [&](const auto& e){
		return rt_value_t::make_struct_value(
			backend,
			http_header_t__type,
			{
				rt_value_t::make_string(backend, e.key),
				rt_value_t::make_string(backend, e.value)
			}
		);
	});

	return rt_value_t::make_struct_value(
		backend,
		http_request_t__type,
		{
			rt_value_t::make_struct_value(
				backend,
				http_request_line_t___type,
				{
					rt_value_t::make_string(backend, req.request_line.method),
					rt_value_t::make_string(backend, req.request_line.uri),
					rt_value_t::make_string(backend, req.request_line.http_version)
				}
			),
			make_vector_value(backend, http_header_t__type, immer::vector<rt_value_t>(headers2.begin(), headers2.end())),
			rt_value_t::make_string(backend, req.optional_body)
		}
	);
}



static ip_address_and_port_t from_runtime__ip_address_and_port_t(value_backend_t& backend, rt_pod_t addr){
	auto& types = backend.types;

	const auto ip_address_and_port_t__type = lookup_type_from_name(types, type_name_t{{ "global_scope", "ip_address_and_port_t" }});

	const auto addr1 = from_runtime_value2(backend, addr, ip_address_and_port_t__type).get_struct_value();
	const auto addr2 = ip_address_t { make_ipv4(addr1->_member_values[0].get_struct_value()->_member_values[0].get_string_value()) };
	return ip_address_and_port_t { addr2, (int)addr1->_member_values[1].get_int_value() };
}





//??? Split into several pack/unpack functions.
static rt_value_t to_runtime__http_response_t(value_backend_t& backend, const http_response_t& response){
	QUARK_ASSERT(backend.check_invariant());

	const auto& types = backend.types;

	const auto http_header_t__type = lookup_type_from_name(types, type_name_t{{ "global_scope", "http_header_t" }});
	const auto http_response_t__type = lookup_type_from_name(types, type_name_t{{ "global_scope", "http_response_t" }});
	const auto http_response_status_line_t__type = lookup_type_from_name(types, type_name_t{{ "global_scope", "http_response_status_line_t" }});

	const auto headers2 = mapf<rt_value_t>(response.headers, [&](const auto& e){
		return rt_value_t::make_struct_value(
			backend,
			http_header_t__type,
			{
				rt_value_t::make_string(backend, e.key),
				rt_value_t::make_string(backend, e.value)
			}
		);
	});

	return rt_value_t::make_struct_value(
		backend,
		http_response_t__type,
		{
			rt_value_t::make_struct_value(
				backend,
				http_response_status_line_t__type,
				{
					rt_value_t::make_string(backend, response.status_line.http_version),
					rt_value_t::make_string(backend, response.status_line.status_code),
				}
			),
			make_vector_value(backend, http_header_t__type, immer::vector<rt_value_t>(headers2.begin(), headers2.end())),
			rt_value_t::make_string(backend, response.optional_body)
		}
	);
}


//////////////////////////////////////////		PACK & UNPACK


/*
static rt_value_t bc_corelib__read_socket(interpreter_t& vm, const rt_value_t args[], int arg_count){
	QUARK_ASSERT(vm.check_invariant());
	QUARK_ASSERT(arg_count == 1);

	auto& backend = vm._backend;
	const auto& types = backend.types;

	QUARK_ASSERT(peek2(types, args[0]._type).is_int());

	const auto socket_id = args[0].get_int_value();
	const auto r = read_socket_string((int)socket_id);
	return rt_value_t::make_string(backend, r);
}
*/
static void unified_corelib__read_socket(runtime_t* frp){
}


/*
static rt_value_t bc_corelib__write_socket(interpreter_t& vm, const rt_value_t args[], int arg_count){
	QUARK_ASSERT(vm.check_invariant());
	QUARK_ASSERT(arg_count == 1);
	QUARK_ASSERT(arg_count == 2);

	auto& backend = vm._backend;
	const auto& types = backend.types;

	QUARK_ASSERT(peek2(types, args[0]._type).is_int());
	QUARK_ASSERT(peek2(types, args[1]._type).is_string());

	const auto socket_id = args[0].get_int_value();
	const auto& data = args[1].get_string_value(backend);
	write_socket_string((int)socket_id, data);
	return rt_value_t::make_void();
}
*/
static void unified_corelib__write_socket(runtime_t* frp){
}


static rt_pod_t unified_corelib__lookup_host_from_ip(runtime_t* frp, rt_pod_t ip){
	auto& backend = get_backend(frp);

	const auto ip2 = from_runtime_ip_address_t(backend, ip);
	const auto info = lookup_host(ip2);
	const auto info2 = to_runtime__host_info_t(backend, info);
	info2.retain();
	return info2._pod;
}

static rt_pod_t unified_corelib__lookup_host_from_name(runtime_t* frp, rt_pod_t name_str){
	auto& backend = get_backend(frp);

	const auto name = from_runtime_string2(backend, name_str);
	const auto info = lookup_host(name);
	const auto info2 = to_runtime__host_info_t(backend, info);
	info2.retain();
	return info2._pod;
}

static rt_pod_t unified_corelib__pack_http_request(runtime_t* frp, rt_pod_t s){
	auto& backend = get_backend(frp);

	const auto req = from_runtime__http_request_t(backend, s);
	const auto request_string = pack_http_request(req);
	return to_runtime_string2(backend, request_string);
}

/*
static rt_value_t bc_corelib__unpack_http_request(interpreter_t& vm, const rt_value_t args[], int arg_count){
	QUARK_ASSERT(vm.check_invariant());
	QUARK_ASSERT(arg_count == 1);

	auto& backend = vm._backend;
	const auto& types = backend.types;

	QUARK_ASSERT(peek2(types, args[0]._type).is_string());

	const auto http_request_t__type = lookup_type_from_name(types, type_name_t{{ "global_scope", "http_request_t_" }});
	const auto http_request_line_t___type = lookup_type_from_name(types, type_name_t{{ "global_scope", "http_request_line_t" }});
	const auto http_header_t__type = lookup_type_from_name(types, type_name_t{{ "global_scope", "http_header_t" }});

	const auto s = args[0].get_string_value(backend);
	const http_request_t req = unpack_http_request(s);

	const auto headers2 = mapf<rt_value_t>(req.headers, [&](const auto& e){
		return rt_value_t::make_struct_value(
			backend,
			http_header_t__type,
			{
				rt_value_t::make_string(backend, e.key),
				rt_value_t::make_string(backend, e.value)
			}
		);
	});

	return rt_value_t::make_struct_value(
		backend,
		http_request_t__type,
		{
			rt_value_t::make_struct_value(
				backend,
				http_request_line_t___type,
				{
					rt_value_t::make_string(backend, req.request_line.method),
					rt_value_t::make_string(backend, req.request_line.uri),
					rt_value_t::make_string(backend, req.request_line.http_version)
				}
			),
			make_vector_value(backend, http_header_t__type, immer::vector<rt_value_t>(headers2.begin(), headers2.end())),
			rt_value_t::make_string(backend, req.optional_body)
		}
	);
}
*/
static rt_pod_t unified_corelib__unpack_http_request(runtime_t* frp, rt_pod_t s){
	auto& backend = get_backend(frp);

	const auto s2 = from_runtime_string2(backend, s);
	const auto r = unpack_http_request(s2);
	const auto result = to_runtime__http_request_t(backend, r);
	result.retain();
	return result._pod;
}

/*
//??? Split into several pack/unpack functions.
static rt_value_t bc_corelib__pack_http_response(interpreter_t& vm, const rt_value_t args[], int arg_count){
	QUARK_ASSERT(vm.check_invariant());
	QUARK_ASSERT(arg_count == 1);

	auto& backend = vm._backend;
	const auto& types = backend.types;

	const auto http_response_t__type = lookup_type_from_name(types, type_name_t{{ "global_scope", "http_response_t" }});
	QUARK_ASSERT(args[0]._type == http_response_t__type);

	const auto status_line = args[0].get_struct_value(backend);
	const auto headers = get_vector_elements(backend, args[1]);
	const auto optional_body = args[2].get_string_value(backend);

	const auto headers2 = mapf<http_header_t>(headers, [&](const auto& e){
		const auto& header = e.get_struct_value(backend);
		return http_header_t { header[0].get_string_value(backend), header[1].get_string_value(backend) };
	});
	const auto req = http_response_t {
		http_response_status_line_t {
			status_line[0].get_string_value(backend),
			status_line[1].get_string_value(backend)
		},
		headers2,
		optional_body
	};
	const auto r = pack_http_response(req);
	return rt_value_t::make_string(backend, r);
}
*/
static void unified_corelib__pack_http_response(runtime_t* frp){
}

static rt_pod_t unified_corelib__unpack_http_response(runtime_t* frp, rt_pod_t response_str){
	auto& backend = get_backend(frp);
	const auto response = from_runtime_string2(backend, response_str);
	const auto a = unpack_http_response(response);
	const auto result = to_runtime__http_response_t(backend, a);
	result.retain();
	return result._pod;
}

static rt_pod_t unified_corelib__execute_http_request(runtime_t* frp, rt_pod_t c, rt_pod_t addr, rt_pod_t request_string){
	auto& backend = get_backend(frp);

	const auto addr2 = from_runtime__ip_address_and_port_t(backend, addr);
	const auto request2 = from_runtime_string2(backend, request_string);
	const auto response = execute_http_request(addr2, request2);
	return to_runtime_string2(backend, response);
}

//??? rename unified_corelib__ -> network_compeonent_*

std::map<std::string, void*> get_network_component_binds(){
	const std::map<std::string, void*> host_functions_map = {
//		{ "read_socket", reinterpret_cast<void *>(&unified_corelib__read_socket) },
//		{ "write_socket", reinterpret_cast<void *>(&unified_corelib__write_socket) },
		{ "lookup_host_from_ip", reinterpret_cast<void *>(&unified_corelib__lookup_host_from_ip) },
		{ "lookup_host_from_name", reinterpret_cast<void *>(&unified_corelib__lookup_host_from_name) },
//		{ "to_ipv4_dotted_decimal_string", nullptr },
//		{ "from_ipv4_dotted_decimal_string", nullptr },
		{ "pack_http_request", reinterpret_cast<void *>(&unified_corelib__pack_http_request) },
		{ "unpack_http_request", reinterpret_cast<void *>(&unified_corelib__unpack_http_request) },
//		{ "pack_http_response", reinterpret_cast<void *>(&unified_corelib__pack_http_response) },
		{ "unpack_http_response", reinterpret_cast<void *>(&unified_corelib__unpack_http_response) },
		{ "execute_http_request", reinterpret_cast<void *>(&unified_corelib__execute_http_request) }
	};
	return host_functions_map;
}


} // floyd
