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
#include <memory> 
#include <netinet/in.h>

#include "utils.h"
struct json_t;


//??? Make this a variant with ipv4 and ipv6.
struct ip_address_t {
	struct in_addr ipv4;
};

ip_address_t make_ipv4(const std::string& s);
std::string unmake_ipv4(const ip_address_t& s);


struct ip_address_and_port_t {
	ip_address_t addr;
	int port;
};


struct socket_t {
	bool check_invariant() const;
	socket_t(int af);
	~socket_t();


	int _fd;
};

struct hostent_t {
	std::string official_host_name;
	std::vector<std::string> name_aliases;
	std::vector<ip_address_t> addresses_IPv4;
};

std::string to_ipv4_dotted_decimal_string(const ip_address_t& a);
ip_address_t from_ipv4_dotted_decimal_string(const std::string& s);

json_t to_json(const hostent_t& value);

struct connection_to_server_t {
	std::shared_ptr<socket_t> socket;
};

struct server_params_t {
	int port;
};

struct connection_i {
	public: virtual ~connection_i(){};

	public: virtual bool connection_i__on_accept(int socket2_fd) = 0;
};

namespace socket_impl {

std::vector<uint8_t> read_socket_binary(int socket);
void write_socket_binary(int socket, const std::vector<uint8_t>& data);

hostent_t lookup_host(const ip_address_t& addr);
hostent_t lookup_host(const std::string& name);

//	Client uses this to open a connection to server. When connection_to_server_t is destructed, connection is closed.
connection_to_server_t connect_to_server(const ip_address_and_port_t& server_addr);

//	http://localhost:8080/info.html
void execute_server(const server_params_t& params, connection_i& connection);

} // socket_impl



struct sockets_i {
	public: virtual ~sockets_i(){};

	public: virtual std::vector<uint8_t> sockets_i__read_socket(int socket) = 0;
	public: virtual void sockets_i__write_socket(int socket, const std::vector<uint8_t>& data) = 0;

	public: virtual hostent_t sockets_i__lookup_host(const ip_address_t& addr) = 0;
	public: virtual hostent_t sockets_i__lookup_host(const std::string& name) = 0;


	//////////////////////////////////////		CLIENT


	//	Client uses this to open a connection to server. When connection_to_server_t is destructed, connection is closed.
	public: virtual connection_to_server_t sockets_i__connect_to_server(const ip_address_and_port_t& server_addr) = 0;


	//////////////////////////////////////		SERVER
	public: virtual void sockets_i__execute_server(const server_params_t& params, connection_i& connection) = 0;
};

struct sockets_t : public sockets_i {
	public: ~sockets_t() override {
	}

	public: std::vector<uint8_t> sockets_i__read_socket(int socket) override {
		return socket_impl::read_socket_binary(socket);
	}
	public: void sockets_i__write_socket(int socket, const std::vector<uint8_t>& data) override {
		return socket_impl::write_socket_binary(socket, data);
	}

	public: hostent_t sockets_i__lookup_host(const ip_address_t& addr) override {
		return socket_impl::lookup_host(addr);
	}
	public: hostent_t sockets_i__lookup_host(const std::string& name) override {
		return socket_impl::lookup_host(name);
	}

	public: connection_to_server_t sockets_i__connect_to_server(const ip_address_and_port_t& server_addr) override {
		return socket_impl::connect_to_server(server_addr);
	}
	public: void sockets_i__execute_server(const server_params_t& params, connection_i& connection) override {
		socket_impl::execute_server(params, connection);
	}
};



std::string read_socket_string(sockets_i& sockets, int socket);
void write_socket_string(sockets_i& sockets, int socket, const std::string& data);


#endif /* floyd_sockets_hpp */
