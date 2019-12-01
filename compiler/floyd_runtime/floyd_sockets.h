//
//  floyd_sockets.hpp
//  floyd
//
//  Created by Marcus Zetterquist on 2019-11-28.
//  Copyright © 2019 Marcus Zetterquist. All rights reserved.
//

#ifndef floyd_sockets_hpp
#define floyd_sockets_hpp

#include <stdio.h>
#include <vector>
#include <string>
#include <netinet/in.h>

struct json_t;


//??? Make this a variant with ipv4 and ipv6.
struct ip_address_t {
	struct in_addr ipv4;
};


struct id_address_and_port_t {
	ip_address_t addr;
	int port;
//	int af;
};


struct socket_t {
	bool check_invariant() const;
	socket_t(int af);
	~socket_t();


	int _fd;
};

std::vector<uint8_t> read_socket_binary(int socket);
void write_socket_binary(int socket, const std::vector<uint8_t>& data);

std::string read_socket_string(int socket);
void write_socket_string(int socket, const std::string& data);



struct hostent_t {
	std::string official_host_name;
	std::vector<std::string> name_aliases;
	std::vector<ip_address_t> addresses_IPv4;
};


hostent_t lookup_host(const ip_address_t& addr);
hostent_t lookup_host(const std::string& name);

std::string to_ipv4_dotted_decimal_string(const ip_address_t& a);
ip_address_t from_ipv4_dotted_decimal_string(const std::string& s);

json_t to_json(const hostent_t& value);




struct connection_to_server_t {
	std::shared_ptr<socket_t> socket;
};

//	Client uses this to open a connection to server. When connection_to_server_t is destructed, connection is closed.
connection_to_server_t connect_to_server(const id_address_and_port_t& server_addr);


#endif /* floyd_sockets_hpp */
