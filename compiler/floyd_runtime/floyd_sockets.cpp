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
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

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


static std::string error_to_string(int errnum){
	char temp[1024 + 1];
	int r = strerror_r(errnum, temp, 1024);
	QUARK_ASSERT(r == 0);
	return std::string(temp);
}






/*
The inet_ntoa() function converts the Internet host address in, given in network byte order, to a string in IPv4 dotted-decimal notation. The string is returned in a statically allocated buffer, which subsequent calls will overwrite.
*/
std::string to_string(const struct in_addr& a){
	const std::string s = inet_ntoa(a);
	return s;
}

//??? network-ordered bytes.
struct in_addr make_in_addr_from_ip_string(const std::string& s){
	struct in_addr result;
	int error = inet_aton(s.c_str(), &result);

/*
	// Convert IPv4 and IPv6 addresses from text to binary form
	if(inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr) <= 0){
		throw std::runtime_error("Invalid address - Address not supported");
	}
*/

	if(error == 0){
		QUARK_ASSERT(false);
		throw std::exception();
	}
	return result;
}


std::string read_socket(int socket){
	char buffer[1024] = { 0 };
	std::string result;
	while(true){
		const auto read_result = read(socket , buffer, 1024);
		if(read_result == -1){
			// errno
			throw std::runtime_error("read() failed");
		}
		else if(read_result == 0){
			return result;
		}
		else if(read_result > 0){
			std::string s(buffer);
			result = result + s;
		}
		else{
			throw std::runtime_error("read() failed");
		}
	}
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

QUARK_TEST("socket-component", "gethostbyname2_r()","google.com", ""){
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

QUARK_TEST("socket-component", "sockets_gethostbyaddr2()","google.com", ""){
	const std::string k_addr = "172.217.21.174";
	const auto a = sockets_gethostbyaddr2(make_in_addr_from_ip_string(k_addr), AF_INET);
	QUARK_VERIFY(a.official_host_name != "");
	QUARK_VERIFY(a.addresses_IPv4.size() == 1);
	QUARK_VERIFY(to_string(a.addresses_IPv4[0]) == k_addr);
}



int test(){
	#define PORT 8080

    int sock = 0;
    long valread;
    struct sockaddr_in serv_addr;
    const char *hello = "Hello from client";
    char buffer[1024] = {0};
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        printf("\n Socket creation error \n");
        return -1;
    }
    
    memset(&serv_addr, '0', sizeof(serv_addr));
    
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);
    
    // Convert IPv4 and IPv6 addresses from text to binary form
    if(inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr)<=0)
    {
        printf("\nInvalid address/ Address not supported \n");
        return -1;
    }
    
    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
    {
        printf("\nConnection Failed \n");
        return -1;
    }
    send(sock , hello , strlen(hello) , 0 );
    printf("Hello message sent\n");
    valread = read( sock , buffer, 1024);
    printf("%s\n",buffer );
    return 0;
}


static const std::string k_http_request_test =
	"GET /hello.htm HTTP/1.1"
	"User-Agent: Mozilla/4.0 (compatible; MSIE5.01; Windows NT)"
	"Host: www.tutorialspoint.com"
	"Accept-Language: en-us"
	"Accept-Encoding: gzip, deflate"
	"Connection: Keep-Alive"
;

static const std::string k_http_request_test2 =
	"GET /docs/index.html HTTP/1.1"
	"Host: www.nowhere123.com"
	"Accept: image/gif, image/jpeg, */*"
	"Accept-Language: en-us"
	"Accept-Encoding: gzip, deflate"
	"User-Agent: Mozilla/4.0 (compatible; MSIE 6.0; Windows NT 5.1)"
	"\r\n"
;

static const std::string k_http_get_minimal = "GET / HTTP/1.1\r\nHost: www.cnn.com\r\n\r\n";

std::string test2(const struct in_addr& addr, int port, const std::string& request){
	const auto sock = socket(AF_INET, SOCK_STREAM, 0);
	if (sock < 0){
		throw std::runtime_error("Socket creation error");
	}

	struct sockaddr_in serv_addr;
	memset(&serv_addr, '0', sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(port);
	serv_addr.sin_addr = addr;

	const auto connect_err = connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr));
	if (connect_err < 0){
		throw std::runtime_error("Connection Failed");
	}

	const auto send_result = send(sock , request.c_str() , request.size() , 0 );
	if(send_result == -1 || send_result != request.size()){
		// errno
		throw std::runtime_error("send() failed");
	}

	std::string reply = read_socket(sock);
	return reply;
}


QUARK_TEST_VIP("socket-component", "","", ""){
//	int error = test2(make_in_addr_from_ip_string("http://httpbin.org/"), 80, k_http_request_test2);
//	int error = test2(sockets_gethostbyname2("google.com", AF_INET).addresses_IPv4[0], 80, k_http_request_test2);


	const auto reply = test2(sockets_gethostbyname2("cnn.com", AF_INET).addresses_IPv4[0], 80, k_http_get_minimal);
	QUARK_TRACE(reply);
}




