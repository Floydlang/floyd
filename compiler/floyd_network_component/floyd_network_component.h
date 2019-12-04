//
//  floyd_network_component.hpp
//  floyd
//
//  Created by Marcus Zetterquist on 2019-12-03.
//  Copyright © 2019 Marcus Zetterquist. All rights reserved.
//

#ifndef floyd_network_component_hpp
#define floyd_network_component_hpp

#include <string>
#include <map>
#include <vector>

#include "compiler_basics.h"
#include "floyd_sockets.h"
#include "floyd_http.h"



//??? NOTICE: We don't have a module system yet. The network component is merged into corelib for now.
namespace floyd {



extern const std::string k_network_component_header;




//######################################################################################################################
//	NETWORK COMPONENT
//######################################################################################################################


//??? make_type__dddd_t()
type_t make__network_component_t__type(types_t& types);
type_t make__id_address_and_port_t__type(types_t& types);
type_t make__host_info_t__type(types_t& types);
type_t make__header_t__type(types_t& types);
type_t make__http_request_line_t__type(types_t& types);
type_t make__http_request_t__type(types_t& types);
type_t make__http_response_status_line_t__type(types_t& types);
type_t make__http_response_t__type(types_t& types);



/*
std::string read_socket(int socket);
void write_socket(int socket, const std::string& data);


hostent_t lookup_host_from_ip(ip_address_t addr);
hostent_t lookup_host_from_name(const std::string& name);

std::string to_ipv4_dotted_decimal_string(const ip_address_t& a);
ip_address_t from_ipv4_dotted_decimal_string(const std::string& s);


//	func int connect_to_server(network_component_t c, id_address_and_port_t server_addr)
//	func void close_socket(int socket)


///////////////////////////////		HTTP


std::string pack_http_request(const http_request_t& r);
http_request_t unpack_http_request(const std::string& r);

std::string pack_http_response(const http_response_t& r);
http_response_t unpack_http_response(const std::string& s);


///////////////////////////////		EXECUTE HTTP


//	Blocks for reply.
std::string execute_http_request(id_address_and_port_t addr, const std::string& request);
*/


} // floyd

#endif /* floyd_network_component_hpp */
