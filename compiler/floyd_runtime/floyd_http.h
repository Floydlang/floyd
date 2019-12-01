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

/*
	Will add CRLF after request_line, headers etc.
*/
std::string make_http_request_string(
	const std::string& request_line,
	const std::vector<std::string>& headers,
	const std::string& optional_body
);


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

std::string execute_http_request(const http_request_t& request);



void execute_http_server(const server_params_t& params);

#endif /* floyd_http_hpp */
