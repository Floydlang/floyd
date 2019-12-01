//
//  floyd_sockets.cpp
//  floyd
//
//  Created by Marcus Zetterquist on 2019-11-28.
//  Copyright © 2019 Marcus Zetterquist. All rights reserved.
//

#include "floyd_sockets.h"

/*
https://en.wikipedia.org/wiki/Berkeley_sockets
https://www.ntu.edu.sg/home/ehchua/programming/webprogramming/HTTP_Basics.html
http://httpbin.org/#/
*/

#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/un.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>

#include "json_support.h"
#include "utils.h"
#include "text_parser.h"
#include "unix_helpers.h"
#include "quark.h"


//#define QUARK_TEST QUARK_TEST_VIP

/*
	The gethostbyname*() and gethostbyaddr*() functions are obsolete. Applications should use
	getaddrinfo(3) and getnameinfo(3) instead.

	const char *gai_strerror(int errcode);
	gethostbyname2_r() is safer
	gethostbyaddr() is safer

	??? network-ordered bytes.

	/??? Sockets should use bytes, not string.
	??? Add proper support for AF_INET or AF_INET6, with the latter being used for IPv6 hosts.
*/


bool socket_t::check_invariant() const{
	QUARK_ASSERT(_fd >= 0);
	return true;
}

socket_t::socket_t(int af){
	const auto fd = ::socket(af, SOCK_STREAM, 0);
	if (fd == -1){
		throw_errno2("Socket creation error", get_unix_err());
	}
	_fd = fd;

	QUARK_ASSERT(check_invariant());
}

socket_t::~socket_t(){
	QUARK_ASSERT(check_invariant());

	const int close_result = ::close(_fd);
	QUARK_ASSERT(close_result == 0);

	_fd = -1;;
}

//??? setsockopt()

/** Returns true on success, or false if there was an error */
#if QUARK_WIN

bool SetSocketBlockingEnabled(int fd, bool blocking){
	QUARK_ASSERT(fd >= 0);

	unsigned long mode = blocking ? 0 : 1;
	const auto a = ioctlsocket(fd, FIONBIO, &mode);
	return a == 0 ? true : false;
}

#endif

#if QUARK_MAC || QUARK_LINUX

bool SetSocketBlockingEnabled(int fd, bool blocking){
	QUARK_ASSERT(fd >= 0);

	int flags = ::fcntl(fd, F_GETFL, 0);
	if (flags == -1){
		return false;
	}
	flags = blocking ? (flags & ~O_NONBLOCK) : (flags | O_NONBLOCK);
	const auto a = ::fcntl(fd, F_SETFL, flags);
	return a == 0 ? true : false;
}

#endif


std::vector<uint8_t> read_socket_binary(int socket){
	QUARK_ASSERT(socket >= 0);

	uint8_t buffer[1024] = { 0 };
	std::vector<uint8_t> result;
	while(true){
		const auto read_result = ::read(socket , buffer, 1024);
		if(read_result < 0){
			QUARK_ASSERT(read_result == -1);
			throw_errno2("read()", get_unix_err());
		}
		else {
			if(read_result == 0){
				return result;
			}
			else if(read_result < 1024){
				std::vector<uint8_t> s(&buffer[0], &buffer[read_result]);
				result = concat(result, s);
				return result;
			}
			else if(read_result == 1024){
				std::vector<uint8_t> s(&buffer[0], &buffer[read_result]);
				result = concat(result, s);

				//	Out input buffer was full = read more data.
			}
		}
	}
}

void write_socket_binary(int socket, const std::vector<uint8_t>& data){
//	::write(socket, data.c_str(), data.size());
	const auto send_result = ::send(socket , data.data() , data.size(), 0);
	if(send_result < 0){
		QUARK_ASSERT(send_result == -1);
		throw_errno2("send()", get_unix_err());
	}
}

std::string read_socket_string(int socket){
	const auto binary = read_socket_binary(socket);
	return std::string(binary.begin(), binary.end());
}

void write_socket_string(int socket, const std::string& data){
	const std::vector<uint8_t> temp(data.begin(), data.end());
	write_socket_binary(socket, temp);
}


/*
The inet_ntoa() function converts the Internet host address in, given in network byte order, to a
string in IPv4 dotted-decimal notation. The string is returned in a statically allocated buffer,
which subsequent calls will overwrite.
*/
std::string to_ipv4_dotted_decimal_string(const ip_address_t& a){
	const std::string s = ::inet_ntoa(a.ipv4);
	return s;
}

/*
	// Convert IPv4 and IPv6 addresses from text to binary form
	if(inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr) <= 0){
		throw std::runtime_error("Invalid address - Address not supported");
	}
*/
ip_address_t from_ipv4_dotted_decimal_string(const std::string& s){
	struct in_addr result;
	int r = ::inet_aton(s.c_str(), &result);

	if(r == 0){
		QUARK_ASSERT(false);
		throw std::runtime_error("inet_aton() failed");
	}
	return ip_address_t { result };
}

json_t to_json(const hostent_t& value){
	return json_t::make_object({
		{ "official_host_name", value.official_host_name },
		{ "name_aliases", json_t::make_array(
			mapf<json_t>(value.name_aliases, [](const auto& e){ return json_t(e); })
		) },
		{ "addresses_IPv4", json_t::make_array(
			mapf<json_t>(value.addresses_IPv4, [](const auto& e){ return to_ipv4_dotted_decimal_string(e); } )
		) }
	});
}



static hostent_t unpack_hostent(const struct hostent& e){
	std::vector<std::string> aliases = unpack_vector_of_strings(e.h_aliases);

	const size_t address_length = e.h_length;
	if(address_length != 4){
		throw std::runtime_error("Unsupported adress length");
	}

	std::vector<ip_address_t> addresses;
	for(int i = 0 ; e.h_addr_list[i] != nullptr ; i++){
		const auto a = (struct in_addr *)e.h_addr_list[i];
		const auto a2 = ip_address_t { *a };
		addresses.push_back(a2);
	}

	const auto r = hostent_t { e.h_name, aliases, addresses };

#if DEBUG
	{
		const json_t j = to_json(r);
		QUARK_TRACE(json_to_pretty_string(j));
	}
#endif

	return r;
}

hostent_t lookup_host(const std::string& name){
	//	Function: struct hostent * gethostbyname2 (const char *name, int af)
	const auto e = ::gethostbyname2(name.c_str(), AF_INET);
	if(e == nullptr){
		const auto host_error = ::h_errno;
		const auto error_str = ::hstrerror(host_error);
		throw std::runtime_error(
			"gethostbyname2(): " + std::string(error_str) + ": " + std::to_string(host_error)
		);
	}
	else{
		return unpack_hostent(*e);
	}
}

QUARK_TEST("socket-component", "gethostbyname2_r()","google.com", ""){
	const auto a = lookup_host("google.com");
	QUARK_VERIFY(a.official_host_name == "google.com");
	QUARK_VERIFY(a.addresses_IPv4.size() == 1);

	const auto addr = a.addresses_IPv4[0];
	const auto addr_str = to_ipv4_dotted_decimal_string(addr);
//	QUARK_VERIFY(to_string(a.addresses_IPv4[0]) == "216.58.207.238");
}

hostent_t lookup_host(const ip_address_t& addr, int af){
	const auto e = ::gethostbyaddr(&addr.ipv4, 4, af);
	if(e == nullptr){
		const auto host_error = ::h_errno;
		const auto error_str = ::hstrerror(host_error);
		throw std::runtime_error(
			"gethostbyaddr(): " + std::string(error_str) + ": " + std::to_string(host_error)
		);
	}
	else{
		return unpack_hostent(*e);
	}
}

QUARK_TEST("socket-component", "lookup_host()","google.com", ""){
	const std::string k_addr = "172.217.21.174";
	const auto a = lookup_host(from_ipv4_dotted_decimal_string(k_addr), AF_INET);
	QUARK_VERIFY(a.official_host_name != "");
	QUARK_VERIFY(a.addresses_IPv4.size() == 1);
	QUARK_VERIFY(to_ipv4_dotted_decimal_string(a.addresses_IPv4[0]) == k_addr);
}


