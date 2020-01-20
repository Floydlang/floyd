//
//  floyd_network_component.cpp
//  floyd
//
//  Created by Marcus Zetterquist on 2019-12-03.
//  Copyright © 2019 Marcus Zetterquist. All rights reserved.
//

#include "floyd_network_component.h"

#include "floyd_corelib.h"
#include "floyd_runtime.h"
#include "floyd_http.h"
#include "test_helpers.h"

#include "compiler_helpers.h"
#include "semantic_ast.h"
#include "text_parser.h"

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

	func string read_socket(int socket) impure
	func void write_socket(int socket, string data) impure

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



	struct http_response_status_line_t {
		string http_version
		string status_code
	}
	struct http_response_t {
		http_response_status_line_t status_line
		[http_header_t] headers
		string optional_body
	}

	func string pack_http_response(http_response_t r)
	func http_response_t unpack_http_response(string s)


	///////////////////////////////		EXECUTE HTTP


	//	Blocks for reply.
	func string execute_http_request(network_component_t c, ip_address_and_port_t addr, string request) impure

	//	Blocks forever.
	func void execute_http_server(network_component_t c, int port, any f, any context) impure

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

static http_response_t from_runtime__http_response_t(value_backend_t& backend, const rt_pod_t& response){
	auto& types = backend.types;

	const auto http_response_t__type = lookup_type_from_name(types, type_name_t{{ "global_scope", "http_response_t" }});
	const auto response2 = rt_value_t(backend, response, http_response_t__type, rt_value_t::rc_mode::bump);

	const auto members = response2.get_struct_value(backend);

	const auto status_line = members[0].get_struct_value(backend);
	const auto headers = get_vector_elements(backend, members[1]);
	const auto optional_body = members[2].get_string_value(backend);

	const auto headers2 = mapf<http_header_t>(headers, [&](const auto& e){
		const auto& header = e.get_struct_value(backend);
		return http_header_t { header[0].get_string_value(backend), header[1].get_string_value(backend) };
	});
	const auto response3 = http_response_t {
		http_response_status_line_t {
			status_line[0].get_string_value(backend),
			status_line[1].get_string_value(backend)
		},
		headers2,
		optional_body
	};
	return response3;
}




//////////////////////////////////////////		PACK & UNPACK


// func string read_socket(int socket)
static rt_pod_t network_component__read_socket(runtime_t* frp, rt_pod_t socket_id){
	auto& backend = get_backend(frp);

	const auto socket_id2 = (int)socket_id.int_value;
	const auto r = read_socket_string(*frp->sockets, socket_id2);
	const auto r2 = rt_value_t::make_string(backend, r);
	r2.retain();
	return r2._pod;
}

// func void write_socket(int socket, string data)
static void network_component__write_socket(runtime_t* frp, rt_pod_t socket_id, rt_pod_t data){
	auto& backend = get_backend(frp);

	const auto socket_id2 = (int)socket_id.int_value;
	const auto& data2 = from_runtime_string2(backend, data);
	write_socket_string(*frp->sockets, socket_id2, data2);
}


static rt_pod_t network_component__lookup_host_from_ip(runtime_t* frp, rt_pod_t ip){
	auto& backend = get_backend(frp);

	const auto ip2 = from_runtime_ip_address_t(backend, ip);
	const auto info = frp->sockets->sockets_i__lookup_host(ip2);
	const auto info2 = to_runtime__host_info_t(backend, info);
	info2.retain();
	return info2._pod;
}

static rt_pod_t network_component__lookup_host_from_name(runtime_t* frp, rt_pod_t name_str){
	auto& backend = get_backend(frp);

	const auto name = from_runtime_string2(backend, name_str);
	const auto info = frp->sockets->sockets_i__lookup_host(name);
	const auto info2 = to_runtime__host_info_t(backend, info);
	info2.retain();
	return info2._pod;
}

static rt_pod_t network_component__pack_http_request(runtime_t* frp, rt_pod_t s){
	auto& backend = get_backend(frp);

	const auto req = from_runtime__http_request_t(backend, s);
	const auto request_string = pack_http_request(req);
	return to_runtime_string2(backend, request_string);
}

static rt_pod_t network_component__unpack_http_request(runtime_t* frp, rt_pod_t s){
	auto& backend = get_backend(frp);

	const auto s2 = from_runtime_string2(backend, s);
	const auto r = unpack_http_request(s2);
	const auto result = to_runtime__http_request_t(backend, r);
	result.retain();
	return result._pod;
}

static rt_pod_t network_component__pack_http_response(runtime_t* frp, rt_pod_t s){
	auto& backend = get_backend(frp);

	const auto response = from_runtime__http_response_t(backend, s);
	const auto response_string = pack_http_response(response);
	return to_runtime_string2(backend, response_string);
}

static rt_pod_t network_component__unpack_http_response(runtime_t* frp, rt_pod_t response_str){
	auto& backend = get_backend(frp);
	const auto response = from_runtime_string2(backend, response_str);
	const auto a = unpack_http_response(response);
	const auto result = to_runtime__http_response_t(backend, a);
	result.retain();
	return result._pod;
}

static rt_pod_t network_component__execute_http_request(runtime_t* frp, rt_pod_t c, rt_pod_t addr, rt_pod_t request_string){
	auto& backend = get_backend(frp);

	const auto addr2 = from_runtime__ip_address_and_port_t(backend, addr);
	const auto request2 = from_runtime_string2(backend, request_string);
	const auto response = execute_http_request(*frp->sockets, addr2, request2);
	return to_runtime_string2(backend, response);
}



typedef bool (*HTTP_SERVER_F)(runtime_t* frp, rt_pod_t socket_id, rt_pod_t context);

struct server_connection_t : public connection_i {
	server_connection_t(runtime_t* frp, rt_pod_t f, rt_type_t f_type, rt_pod_t context, rt_type_t context_type) :
		frp(frp),
		f(f),
		f_type(f_type),
		context(context),
		context_type(context_type)
	{
	}

	bool connection_i__on_accept(int socket2) override {
		auto& backend = get_backend(frp);
		const auto& func_link = lookup_func_link_by_pod_required(backend, f);

		// ??? This thunking must be moved of inner loop. Use ffi to make bridge for k_native__floydcc?
		if(func_link.execution_model == func_link_t::eexecution_model::k_bytecode__floydcc){
			const rt_value_t f_args[] = {
				rt_value_t::make_int(socket2),
				rt_value_t(backend, context, type_t(context_type), rt_value_t::rc_mode::bump)
			};
			const auto continue_flag = call_thunk(frp, rt_value_t(backend, f, func_link.function_type_optional, rt_value_t::rc_mode::bump), f_args, 2);
			QUARK_ASSERT(continue_flag._type.is_bool());
			return continue_flag._pod.bool_value == 1;
		}
		else if(func_link.execution_model == func_link_t::eexecution_model::k_native__floydcc){
			const auto f2 = reinterpret_cast<HTTP_SERVER_F>(func_link.f);
			const uint8_t continue_flag = (*f2)(frp, make_runtime_int(socket2), context);
			return continue_flag == 1;
		}
		else{
			quark::throw_defective_request();
		}
	}


	///////////////////////////////		STATE

	runtime_t* frp;
	rt_pod_t f;
	rt_type_t f_type;
	rt_pod_t context;
	rt_type_t context_type;
};

//	??? This is the first-non intrinsic function that uses ANY and an arg of function-using-any. It's type checking is sloppy.
//	Blocks forever.
//	func void execute_http_server(network_component_t c, int port, func void f(int socket)) impure
static void network_component__execute_http_server(runtime_t* frp, rt_pod_t c, rt_pod_t port, rt_pod_t f, rt_type_t f_type, rt_pod_t context, rt_type_t context_type){
	const auto port2 = port.int_value;

	const auto f2 = type_t(f_type);
	const auto& types = frp->backend->types;
	if(f2.is_function() && f2.get_function_return(types).is_bool() && f2.get_function_args(types).size() == 2 && f2.get_function_args(types)[0].is_int() && f2.get_function_args(types)[1] == type_t(context_type)){
	}
	else{
		quark::throw_runtime_error("execute_http_server() - the function has wrong type");
	}

	server_connection_t connection { frp, f, f_type, context, context_type };
	execute_http_server(*frp->sockets, server_params_t { (int)port2 }, connection);
}





std::map<std::string, void*> get_network_component_binds(){
	const std::map<std::string, void*> host_functions_map = {
		{ "read_socket", reinterpret_cast<void *>(&network_component__read_socket) },
		{ "write_socket", reinterpret_cast<void *>(&network_component__write_socket) },
		{ "lookup_host_from_ip", reinterpret_cast<void *>(&network_component__lookup_host_from_ip) },
		{ "lookup_host_from_name", reinterpret_cast<void *>(&network_component__lookup_host_from_name) },
//		{ "to_ipv4_dotted_decimal_string", nullptr },
//		{ "from_ipv4_dotted_decimal_string", nullptr },
		{ "pack_http_request", reinterpret_cast<void *>(&network_component__pack_http_request) },
		{ "unpack_http_request", reinterpret_cast<void *>(&network_component__unpack_http_request) },
		{ "pack_http_response", reinterpret_cast<void *>(&network_component__pack_http_response) },
		{ "unpack_http_response", reinterpret_cast<void *>(&network_component__unpack_http_response) },
		{ "execute_http_request", reinterpret_cast<void *>(&network_component__execute_http_request) },
		{ "execute_http_server", reinterpret_cast<void *>(&network_component__execute_http_server) }
	};
	return host_functions_map;
}


} // floyd








using namespace floyd;


//######################################################################################################################
//	NETWORK COMPONENT TESTS
//######################################################################################################################

//#define QUARK_TEST QUARK_TEST_VIP

QUARK_TEST("network component", "network_component_t()", "", ""){
	ut_run_closed_lib(
		QUARK_POS,
		R"(

			let c = network_component_t(100)
			assert(c.internal == 100)

		)"
	);
}
QUARK_TEST("network component", "ip_address_and_port_t()", "", ""){
	ut_run_closed_lib(
		QUARK_POS,
		R"(

			let a = ip_address_and_port_t(ip_address_t("abcd"), 8080)
			assert(a.addr.data == "abcd")
			assert(a.port == 8080)

		)"
	);
}

QUARK_TEST("network component", "host_info_t()", "", ""){
	ut_run_closed_lib(
		QUARK_POS,
		R"(

			let a = host_info_t(
				"example.com",
				[ "example2.com", "example3.com"],
				[ ip_address_t("abcd"), ip_address_t("zxyz") ]
			)
			assert(a.official_host_name == "example.com")
			assert(a.name_aliases[0] == "example2.com")
			assert(a.name_aliases[1] == "example3.com")
			assert(a.addresses_IPv4[0].data == "abcd")
			assert(a.addresses_IPv4[1].data == "zxyz")

		)"
	);
}

QUARK_TEST("network component", "lookup_host_from_ip()", "", ""){
	ut_run_closed_lib(
		QUARK_POS,
		R"(

			let x = ip_address_t("abcd")
//			let a = lookup_host_from_ip(x)

		)"
	);
}





void ut_run_closed_libXXX(const quark::call_context_t& context, const std::string& program){
	test_floyd2(context, make_compilation_unit(program, "", compilation_unit_mode::k_include_core_lib), make_default_compiler_settings(), {}, check_nothing(), false);
}

QUARK_TEST_VIP("network component", "lookup_host_from_name()", "", ""){
	ut_run_closed_libXXX(
		QUARK_POS,
		R"(

			let a = lookup_host_from_name("example.com")

			print(a)

			assert(a.official_host_name == "example.com")
			assert(a.name_aliases == [ "a", "b" ])

//			assert(size(a.addresses_IPv4) == 0)

		)"
	);
}



QUARK_TEST("network component", "lookup_host_from_name()", "", ""){
	ut_run_closed_lib(
		QUARK_POS,
		R"(

			let a = lookup_host_from_name("example.com")

			print(a)
			assert(a.official_host_name == "example.com")
//			assert(size(a.name_aliases[0]) > 0)
			assert(size(a.addresses_IPv4) > 0)
//			assert(a.addresses_IPv4[0].data == "abcd")

		)"
	);
}

QUARK_TEST("network component", "http_header_t()", "", ""){
	ut_run_closed_lib(
		QUARK_POS,
		R"(

			let a = http_header_t("Content-Type", "text/html")
			assert(a.key == "Content-Type")
			assert(a.value == "text/html")

		)"
	);
}

QUARK_TEST("network component", "http_request_line_t()", "", ""){
	ut_run_closed_lib(
		QUARK_POS,
		R"(

			let a = http_request_line_t("POST", "/test/demo_form.php", "HTTP/1.1")
			assert(a.method == "POST")
			assert(a.uri == "/test/demo_form.php")
			assert(a.http_version == "HTTP/1.1")

		)"
	);
}

QUARK_TEST("network component", "pack_http_request()", "", ""){
	ut_run_closed_lib(
		QUARK_POS,
		R"(

			let request_line = http_request_line_t ( "GET", "/index.html", "HTTP/1.0" )
			let a = http_request_t ( request_line, [], "" )
			let r = pack_http_request(a)
			print(r)
			assert(r == "GET /index.html HTTP/1.0\r\n\r\n")

		)"
	);
}

QUARK_TEST("network component", "unpack_http_request()", "", ""){
	ut_run_closed_lib(
		QUARK_POS,
		R"(

			let r = unpack_http_request("GET /test.html HTTP/1.1")

			assert(r.request_line.method == "GET")
			assert(r.request_line.uri == "/test.html")
			assert(r.request_line.http_version == "HTTP/1.1")
			assert(r.headers == [])
			assert(r.optional_body == "")

		)"
	);
}

QUARK_TEST("network component", "pack_http_response()", "", ""){
	ut_run_closed_lib(
		QUARK_POS,
		R"(

			let r = pack_http_response(
				http_response_t(
					http_response_status_line_t("HTTP/1.1", "301 Moved Permanently"),
					[ http_header_t("Server", "Varnish"), http_header_t("X-Cache-Hits", "0") ],
					""
				)
			)
			assert(r == "HTTP/1.1 301 Moved Permanently\r\nServer: Varnish\r\nX-Cache-Hits: 0\r\n\r\n")

		)"
	);
}


QUARK_TEST("network component", "unpack_http_response()", "", ""){
	ut_run_closed_lib(
		QUARK_POS,
		R"(

			let r = unpack_http_response("HTTP/1.1 301 Moved Permanently\r\n	Server: Varnish\r\n	X-Cache-Hits: 0\r\n\r\n")
			assert(r.status_line == http_response_status_line_t ( "HTTP/1.1", "301 Moved Permanently" ))
			assert(size(r.headers) == 2)
			assert(r.headers[0] == http_header_t ( "Server", "Varnish" ))
			assert(r.headers[1] == http_header_t ( "X-Cache-Hits", "0" ))
			assert(r.optional_body == "")

		)"
	);
}


QUARK_TEST("network component", "execute_http_request()", "", ""){
	ut_run_closed_lib(
		QUARK_POS,
		R"(

			let c = network_component_t(666)

			let request_line = http_request_line_t ( "GET", "/index.html", "HTTP/1.0" )
			let a = http_request_t ( request_line, [], "" )
			let ip = lookup_host_from_name("example.com").addresses_IPv4[0]
			let dest = ip_address_and_port_t(ip, 80)
			let r = execute_http_request(c, dest, pack_http_request(a))
			print(r)

		)"
	);
}


#if 0
//	WARNING: This test never completes + is impure.
QUARK_TEST("network component", "execute_http_server()", "", ""){
	ut_run_closed_lib(
		QUARK_POS,
		R"(

			let c = network_component_t(666)

			func string make_webpage(){
				let doc = "
					<head>
						<title>Hello Floyd Server</title>
					</head>
					<body>
						<h1>Hello Floyd Server</h1>
						This document may be found <a HREF=\"https://stackoverflow.com/index.html\">here</a>
					</body>
				"
				return doc
			}

			func bool f(int socket_id, string context) impure{
				assert(context == "my-context")

				let read_data = read_socket(socket_id)
				if(read_data == ""){
					print("empty request\n")
				}
				else{
					let request = unpack_http_request(read_data)

					if(request.request_line == http_request_line_t( "GET", "/info.html", "HTTP/1.1" )){
						print("Serving page\n")
						let doc = make_webpage()
						let r = pack_http_response(
							http_response_t (
								http_response_status_line_t ( "HTTP/1.1", "200 OK" ),
								[
									http_header_t( "Content-Type", "text/html" ),
									http_header_t( "Content-Length", to_string(size(doc)) )
								],
								doc
							)
						)
						write_socket(socket_id, r)
					}
					else {
						let r = pack_http_response(http_response_t ( http_response_status_line_t ( "HTTP/1.1", "404 OK" ), {}, "" ))
						write_socket(socket_id, r)
					}
				}
				return true
			}

			execute_http_server(c, 8080, f, "my-context")

		)"
	);
}
#endif


QUARK_TEST("regression test", "map()", "map() from inside another map()", ""){
	ut_run_closed_nolib(QUARK_POS, R"(

		struct context_t { int a string b }

		func int f2(int v, string c){
			return v + 3000
		}

		func int f1(int v, context_t context){
			assert(context.a == 2000)
			assert(context.b == "twenty")

			let r = map([ 100, 200, 300, v ], f2, "xyz")

			return r[3]
		}

		let r = map([ 10, 11, 12 ], f1, context_t( 2000, "twenty"))
//		print(r) ; print("\n")
		assert(r == [ 3010, 3011, 3012 ])

	)");
}

QUARK_TEST("regression test", "call BC function from 2nd thread -- used to crash in d98c84ce9cd5ddd8ccbab350aebf4108482d18e0", "", ""){
	ut_run_closed_lib(
		QUARK_POS,
		R"___(

			let c = network_component_t(666)

			container-def {
				"name": "",
				"tech": "",
				"desc": "",
				"clocks": {
					"main_clock": {
						"sss-main": "my_main"
					},
					"http-server": {
						"mmm-server": "my_server"
					}
				}
			}


			////////////////////////////////	SERVER


			func void f(int socket_id) impure {
				print("f()")
			}

			func double my_server__init() impure {
//				f(44)
				exit()
				return 123.456
			}

			func double my_server__msg(double state, int m) impure {
				return state
			}


			////////////////////////////////	MAIN


			func int my_main__init() impure {
				f(44)
				exit()
				return 13
			}

			func int my_main__msg(int state, int m) impure {
				return state
			}

		)___"
	);
}

QUARK_TEST("network component", "execute_http_server()", "Make sure f's type is checked", ""){
	ut_verify_exception_lib(
		QUARK_POS,
		R"(

			func bool f(int socket_id) impure {
				return true
			}

			let c = network_component_t(666)
			execute_http_server(c, 8080, f, 0)

		)",
		"execute_http_server() - the function has wrong type"
	);
}
