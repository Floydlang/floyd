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



///////////////////////////////		HTTP REQUESTS


/*
	Will add CRLF after request_line, headers etc.
*/
std::string make_http_request_string(
	const std::string& request_line,
	const std::vector<std::string>& headers,
	const std::string& optional_body
);

struct http_request_t {
	id_address_and_port_t addr;
	std::string message;
};

//	Lookups up addr, uses the first IP. Always IPv4 (for now).
http_request_t make_http_request_helper(
	const std::string& addr,
	int port,
	int af,
	const std::string& message
);


struct http_request_line_t {
	std::string method;
	std::string uri;
	std::string http_version;
};
inline bool operator==(const http_request_line_t& lhs, const http_request_line_t& rhs){
	return lhs.method == rhs.method && lhs.uri == rhs.uri && lhs.http_version == rhs.http_version;
}

std::string pack_http_request_line(const http_request_line_t& request_line);

//	Input string is the first line of the message only, no CRLF allowed.
http_request_line_t unpack_http_request_line(const std::string& request_line);


///////////////////////////////		HTTP RESPONSES


struct http_response_t {
	std::string status_line;
	std::vector<std::pair<std::string, std::string>> headers;
	std::string optional_body;
};

bool operator==(const http_response_t& lhs, const http_response_t& rhs);

std::string make_http_response_string(
	const std::string& status_line,
	const std::vector<std::string>& headers,
	const std::string& optional_body
);

http_response_t unpack_http_response_string(const std::string& s);



struct http_response_status_line_t {
	std::string http_version;
	std::string status_code;
};
inline bool operator==(const http_response_status_line_t& lhs, const http_response_status_line_t& rhs){
	return lhs.http_version == rhs.http_version && lhs.status_code == rhs.status_code;
}

std::string pack_http_response_status_line(const http_response_status_line_t& value);

//	Input string is the first line of the message only, no CRLF allowed.
http_response_status_line_t unpack_http_response_status_line(const std::string& s);


///////////////////////////////		EXECUTE


std::string execute_http_request(const http_request_t& request);


void execute_http_server(const server_params_t& params, connection_i& connection);

#endif /* floyd_http_hpp */
