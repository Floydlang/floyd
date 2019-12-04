//
//  floyd_http.hpp
//  floyd
//
//  Created by Marcus Zetterquist on 2019-12-01.
//  Copyright © 2019 Marcus Zetterquist. All rights reserved.
//

#ifndef floyd_http_hpp
#define floyd_http_hpp

#include "floyd_sockets.h"
#include <string>
#include <vector>
#include <memory>


struct header_t {
	std::string key;
	std::string value;
};


inline bool operator==(const header_t& lhs, const header_t& rhs){
	return lhs.key == rhs.key && lhs.value == rhs.value;
}



///////////////////////////////		HTTP REQUESTS


struct http_request_line_t {
	std::string method;
	std::string uri;
	std::string http_version;
};
inline bool operator==(const http_request_line_t& lhs, const http_request_line_t& rhs){
	return lhs.method == rhs.method && lhs.uri == rhs.uri && lhs.http_version == rhs.http_version;
}

struct http_request_t {
	http_request_line_t request_line;
	std::vector<header_t> headers;
	std::string optional_body;
};

std::string pack_http_request(const http_request_t& r);
http_request_t unpack_http_request(const std::string& r);


///////////////////////////////		HTTP RESPONSES


struct http_response_status_line_t {
	std::string http_version;
	std::string status_code;
};
inline bool operator==(const http_response_status_line_t& lhs, const http_response_status_line_t& rhs){
	return lhs.http_version == rhs.http_version && lhs.status_code == rhs.status_code;
}

struct http_response_t {
	http_response_status_line_t status_line;
	std::vector<header_t> headers;
	std::string optional_body;
};

bool operator==(const http_response_t& lhs, const http_response_t& rhs);

std::string pack_http_response(const http_response_t& r);
http_response_t unpack_http_response(const std::string& s);



///////////////////////////////		EXECUTE



//	Lookups up addr, uses the first IP. Always IPv4 (for now).
id_address_and_port_t make_http_dest(const std::string& addr, int port, int af);

std::string execute_http_request(const id_address_and_port_t& addr, const std::string& message);


void execute_http_server(const server_params_t& params, connection_i& connection);

#endif /* floyd_http_hpp */
