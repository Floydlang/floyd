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
*/


#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/un.h>
#include <arpa/inet.h>
#include <netdb.h>

#include "quark.h"


std::vector<std::string> unpack_vector_of_strings(char** vec){
	QUARK_ASSERT(vec != nullptr);
	std::vector<std::string> result;
	for(int i = 0 ; vec[i] != nullptr ; i++){
		const std::string s(vec[i]);
		result.push_back(s);
	}
	return result;
}


struct hostent_t {
	std::string official_host_name;
	std::vector<std::string> name_aliases;
	std::vector<struct in_addr> addresses_IPv4;
};



/*
The inet_ntoa() function converts the Internet host address in, given in network byte order, to a string in IPv4 dotted-decimal notation. The string is returned in a statically allocated buffer, which subsequent calls will overwrite.
*/
std::string to_string(const struct in_addr& a){
	const std::string s = inet_ntoa(a);
	return s;
}

//??? network-ordered bytes.
struct in_addr make_in_addr(const std::string& s){
	struct in_addr result;
	int error = inet_aton(s.c_str(), &result);
	if(error != 1){
		QUARK_ASSERT(false);
		throw std::exception();
	}
	return result;
}




hostent_t unpack_hostent(const struct hostent& e){
	std::vector<std::string> aliases = unpack_vector_of_strings(e.h_aliases);

	// ??? Add proper support for AF_INET or AF_INET6, with the latter being used for IPv6 hosts.
	const size_t address_length = e.h_length;
	if(address_length != 4){
		throw std::runtime_error("");
	}

	std::vector<struct in_addr> addresses;
	for(int i = 0 ; e.h_addr_list[i] != nullptr ; i++){
		const auto a = (struct in_addr *)e.h_addr_list[i];
		addresses.push_back(*a);
	}

	return hostent_t { e.h_name, aliases, addresses };
}



//	gethostbyname2_r() is safer
void throw_socket_err(int error) QUARK_NO_RETURN;
void throw_socket_err(int error) {
	// No such host is known in the database.
	if(error == HOST_NOT_FOUND){
		throw std::runtime_error("HOST_NOT_FOUND");
	}

	// This condition happens when the name server could not be contacted. If you try again later, you may succeed then.
	else if(error == TRY_AGAIN){
		throw std::runtime_error("TRY_AGAIN");
	}

	// A non-recoverable error occurred.
	else if(error == NO_RECOVERY){
		throw std::runtime_error("NO_RECOVERY");
	}

	// The host database contains an entry for the name, but it doesn’t have an associated Internet address.
	else if(error == NO_ADDRESS){
		throw std::runtime_error("NO_ADDRESS");
	}
	else{
		throw std::runtime_error("");
	}
}

//	gethostbyname2_r() is safer
hostent_t sockets_gethostbyname2(const std::string& name, int af){
	//	Function: struct hostent * gethostbyname2 (const char *name, int af)
	const auto e = gethostbyname2(name.c_str(), af);
	if(e == nullptr){
		//??? Is there are function to get h_errno?
		throw_socket_err(h_errno);
	}
	else{
		return unpack_hostent(*e);
	}
}

QUARK_TEST_VIP("socket-component", "gethostbyname2_r()","google.com", ""){
	const auto a = sockets_gethostbyname2("google.com", AF_INET);
	QUARK_VERIFY(a.official_host_name == "google.com");
	QUARK_VERIFY(a.addresses_IPv4.size() == 1);

	const auto addr = a.addresses_IPv4[0];
	const auto addr_str = to_string(addr);
//	QUARK_VERIFY(to_string(a.addresses_IPv4[0]) == "216.58.207.238");
}





//	gethostbyaddr() is safer
hostent_t sockets_gethostbyaddr2(const struct in_addr& addr_v4, int af){
	const auto e = gethostbyaddr(&addr_v4, 4, af);
	if(e == nullptr){
		//??? Is there are function to get h_errno?
		throw_socket_err(h_errno);
	}
	else{
		return unpack_hostent(*e);
	}
}

QUARK_TEST_VIP("socket-component", "sockets_gethostbyaddr2()","google.com", ""){
	const std::string k_addr = "172.217.21.174";
	const auto a = sockets_gethostbyaddr2(make_in_addr(k_addr), AF_INET);
	QUARK_VERIFY(a.official_host_name != "");
	QUARK_VERIFY(a.addresses_IPv4.size() == 1);
	QUARK_VERIFY(to_string(a.addresses_IPv4[0]) == k_addr);
}
