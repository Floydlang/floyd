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


struct json_t;


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
	std::vector<struct in_addr> addresses_IPv4;
};


hostent_t lookup_host(const struct in_addr& addr_v4);
hostent_t lookup_host(const std::string& name);

std::string to_ipv4_dotted_decimal_string(const struct in_addr& a);
struct in_addr from_ipv4_dotted_decimal_string(const std::string& s);

json_t to_json(const hostent_t& value);

#endif /* floyd_sockets_hpp */
