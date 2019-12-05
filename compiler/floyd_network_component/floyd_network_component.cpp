//
//  floyd_network_component.cpp
//  floyd
//
//  Created by Marcus Zetterquist on 2019-12-03.
//  Copyright © 2019 Marcus Zetterquist. All rights reserved.
//

#include "floyd_network_component.h"

#include "floyd_corelib.h"

namespace floyd {



extern const std::string k_network_component_header = R"(

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

	func string read_socket(int socket)
	func void write_socket(int socket, string data)

	struct host_info_t {
		string official_host_name
		[string] name_aliases
		[ip_address_t] addresses_IPv4
	}

	func host_info_t lookup_host_from_ip(ip_address_t addr) impure
	func host_info_t lookup_host_from_name(string name) impure

	func string to_ipv4_dotted_decimal_string(ip_address_t a)
	func ip_address_t from_ipv4_dotted_decimal_string(string s)


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


	struct http_response_status_line_t {
		string http_version
		string status_code
	}
	struct http_response_t {
		http_response_status_line_t status_line
		[http_header_t] headers
		string optional_body
	}

	func int get_time_ns() impure {
		return get_time_of_day() * 1000
	}

	func string pack_http_request(http_request_t r)
	func http_request_t unpack_http_request(string r)

	func string pack_http_response(http_response_t r)
	func http_response_t unpack_http_response(string s)



	///////////////////////////////		EXECUTE HTTP


	//	Blocks for reply.
	func string execute_http_request(network_component_t c, ip_address_and_port_t addr, string request) impure

	//	Blocks forever. ??? how to ask it to stop?
//	func void execute_http_server(network_component_t c, server_params_t params, string process_id) impure

)";





//######################################################################################################################
//	NETWORK COMPONENT
//######################################################################################################################


type_t make__network_component_t__type(types_t& types){
	return make_struct(
		types,
		struct_type_desc_t({
			{ type_t::make_int(), "internal" }
		})
	);
}

type_t make__ip_address_and_port_t__type(types_t& types){
	return make_struct(
		types,
		struct_type_desc_t({
			{ make__ip_address_t__type(types), "addr" },
			{ type_t::make_int(), "port" }
		})
	);
}

type_t make__host_info_t__type(types_t& types){
	return make_struct(
		types,
		struct_type_desc_t({
			{ type_t::make_string(), "official_host_name" },
			{ make_vector(types, type_t::make_string()), "name_aliases" },
			{ make_vector(types, make__ip_address_t__type(types)), "addresses_IPv4" }
		})
	);
}

type_t make__http_header_t__type(types_t& types){
	return make_struct(
		types,
		struct_type_desc_t({
			{ type_t::make_string(), "key" },
			{ type_t::make_string(), "value" }
		})
	);
}

type_t make__http_request_line_t__type(types_t& types){
	return make_struct(
		types,
		struct_type_desc_t({
			{ type_t::make_string(), "method" },
			{ type_t::make_string(), "uri" },
			{ type_t::make_string(), "http_version" }
		})
	);
}

type_t make__http_request_t__type(types_t& types){
	return make_struct(
		types,
		struct_type_desc_t({
			{ make__http_request_line_t__type(types), "request_line" },
			{ make_vector(types, make__http_header_t__type(types)), "headers" },
			{ type_t::make_string(), "optional_body" }
		})
	);
}

type_t make__http_response_status_line_t__type(types_t& types){
	return make_struct(
		types,
		struct_type_desc_t({
			{ type_t::make_string(), "http_version" },
			{ type_t::make_string(), "status_code" }
		})
	);
}


type_t make__http_response_t__type(types_t& types){
	return make_struct(
		types,
		struct_type_desc_t({
			{ make__http_response_status_line_t__type(types), "status_line" },
			{ make_vector(types, make__http_header_t__type(types)), "headers" },
			{ type_t::make_string(), "optional_body" }
		})
	);
}



} // floyd
