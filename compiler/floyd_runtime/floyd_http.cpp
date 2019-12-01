//
//  floyd_http.cpp
//  floyd
//
//  Created by Marcus Zetterquist on 2019-12-01.
//  Copyright © 2019 Marcus Zetterquist. All rights reserved.
//

#include "floyd_http.h"

#include "floyd_sockets.h"

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
#include "unix_helpers.h"
#include "text_parser.h"
#include "quark.h"



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
static const std::string k_http_get_minimal2 = "GET / HTTP/1.1\r\nHost: www.example.com\r\n\r\n";


static const std::string kCRLF = "\r\n";


std::string make_http_request_string(
	const std::string& request_line,
	const std::vector<std::string>& headers,
	const std::string& optional_body
){
	std::string headers_str;
	for(const auto& e: headers){
		headers_str = headers_str + e + kCRLF;
	}

	return
		request_line + kCRLF
		+ headers_str
		+ kCRLF
		+ optional_body;
}


QUARK_TEST("socket-component", "make_http_request_string()","", ""){
	const auto r = make_http_request_string("GET /hello.htm HTTP/1.1", {}, "");
	QUARK_VERIFY(r == "GET /hello.htm HTTP/1.1\r\n\r\n");
}
QUARK_TEST("socket-component", "make_http_request_string()","", ""){
	const auto r = make_http_request_string("GET /hello.htm HTTP/1.1", { "Host: www.tutorialspoint.com", "Accept-Language: en-us" }, "licenseID=string&content=string&/paramsXML=string");
	ut_verify_string(
		QUARK_POS,
		r,
		"GET /hello.htm HTTP/1.1\r\n"
		"Host: www.tutorialspoint.com" "\r\n"
		"Accept-Language: en-us" "\r\n"
		"\r\n"
		"licenseID=string&content=string&/paramsXML=string"
	);
}

static const std::string k_http_response1 =
	R"___(	HTTP/1.1 301 Moved Permanently)___" "\r\n"
	R"___(	Server: Varnish)___" "\r\n"
	R"___(	Retry-After: 0)___" "\r\n"
	R"___(	Content-Length: 0)___" "\r\n"
	R"___(	Cache-Control: public, max-age=600)___" "\r\n"
	R"___(	Location: https://www.cnn.com/)___" "\r\n"
	R"___(	Accept-Ranges: bytes)___" "\r\n"
	R"___(	Date: Thu, 28 Nov 2019 19:07:13 GMT)___" "\r\n"
	R"___(	Via: 1.1 varnish)___" "\r\n"
	R"___(	Connection: close)___" "\r\n"
	R"___(	Set-Cookie: countryCode=SE; Domain=.cnn.com; Path=/)___" "\r\n"
	R"___(	Set-Cookie: geoData=stockholm|AB|117 37|SE|EU|100|broadband; Domain=.cnn.com; Path=/)___" "\r\n"
	R"___(	X-Served-By: cache-bma1631-BMA)___" "\r\n"
	R"___(	X-Cache: HIT)___" "\r\n"
	R"___(	X-Cache-Hits: 0)___" "\r\n"
	R"___()___" "\r\n"
	;

static const std::string k_http_response2 =
	R"___(	HTTP/1.0 200 OK)___" "\r\n"
	R"___(	Accept-Ranges: bytes)___" "\r\n"
	R"___(	Content-Type: text/html)___" "\r\n"
	R"___(	Date: Thu, 28 Nov 2019 19:07:13 GMT)___" "\r\n"
	R"___(	Last-Modified: Thu, 28 Nov 2019 18:12:02 GMT)___" "\r\n"
	R"___(	Server: ECS (nyb/1D10))___" "\r\n"
	R"___(	Content-Length: 94)___" "\r\n"
	R"___(	Connection: close)___" "\r\n"
	R"___()___" "\r\n"
	R"___(	<html><head><title>edgecastcdn.net</title></head><body><h1>edgecastcdn.net</h1></body></html>)___"
	;

static const std::string k_http_response3 =
	R"___(	HTTP/1.1 301 Moved Permanently)___" "\r\n"
	R"___(	Content-Type: text/html; charset=UTF-8)___" "\r\n"
	R"___(	Location: https://stackoverflow.com/index.html)___" "\r\n"
	R"___(	Accept-Ranges: bytes)___" "\r\n"
	R"___(	Content-Length: 159)___" "\r\n"
	R"___(	Accept-Ranges: bytes)___" "\r\n"
	R"___(	Date: Thu, 28 Nov 2019 19:07:13 GMT)___" "\r\n"
	R"___(	Via: 1.1 varnish)___" "\r\n"
	R"___(	Age: 0)___" "\r\n"
	R"___(	Connection: close)___" "\r\n"
	R"___(	X-Served-By: cache-bma1641-BMA)___" "\r\n"
	R"___(	X-Cache: MISS)___" "\r\n"
	R"___(	X-Cache-Hits: 0)___" "\r\n"
	R"___(	X-Timer: S1574968034.643929,VS0,VE112)___" "\r\n"
	R"___(	Vary: Fastly-SSL)___" "\r\n"
	R"___(	X-DNS-Prefetch-Control: off)___" "\r\n"
	R"___(	Set-Cookie: prov=9847efbd-0d62-5ab8-cd80-94d1972a043c; domain=.stackoverflow.com; expires=Fri, 01-Jan-2055 00:00:00 GMT; path=/; HttpOnly)___" "\r\n"
	R"___()___" "\r\n"
	R"___(	<head><title>Document Moved</title></head>)___" "\r\n"
	R"___(	<body><h1>Object Moved</h1>This document may be found <a HREF="https://stackoverflow.com/index.html">here</a></body>)___"
	;

static const std::string k_skip_leading_chars = " \t";


bool operator==(const http_response_t& lhs, const http_response_t& rhs){
	return
		lhs.status_line == rhs.status_line
		&& lhs.headers == rhs.headers
		&& lhs.optional_body == rhs.optional_body;
}


static std::pair<std::string, seq_t> read_to_crlf(const seq_t& p){
	return read_until_str(p, kCRLF, true);
}

static std::pair<std::string, seq_t> read_to_crlf_skip_leads(const seq_t& p){
	const auto a = read_until_str(p, kCRLF, true);
	const auto b = skip(seq_t(a.first), k_skip_leading_chars);
	return { b.str(), a.second };
}

std::string make_http_response_string(
	const std::string& status_line,
	const std::vector<std::string>& headers,
	const std::string& optional_body
){
	std::string headers_str;
	for(const auto& e: headers){
		headers_str = headers_str + e + kCRLF;
	}

	return
		status_line + kCRLF
		+ headers_str
		+ kCRLF
		+ optional_body;
}

http_response_t unpack_http_response_string(const std::string& s){
	seq_t p(s);
	const auto status_line_pos = read_to_crlf_skip_leads(p);
	p = status_line_pos.second;

	std::vector<std::pair<std::string, std::string>> headers;
	while(p.empty() == false && is_first(p, kCRLF) == false){
		const auto h = read_to_crlf_skip_leads(p);

		const auto key_kv = read_until_str(seq_t(h.first), ": ", true);
		const auto value = key_kv.second.str();
		headers.push_back(
			std::pair<std::string, std::string>(key_kv.first, value)
		);
		p = h.second;
	}

	if(is_first(p, kCRLF)){
		p = p.rest(kCRLF.size());
	}

	const auto body = p.first(p.size());

	return {
		status_line_pos.first,
		headers,
		body
	};
}

QUARK_TEST("socket-component", "unpack_http_response_string()", "k_http_response1", ""){
	const auto r = unpack_http_response_string(k_http_response1);
	QUARK_VERIFY(r.status_line == "HTTP/1.1 301 Moved Permanently");
	QUARK_VERIFY(r.headers.size() == 14);
	QUARK_VERIFY(r.headers[0] == (std::pair<std::string, std::string>("Server", "Varnish")));
	QUARK_VERIFY(r.headers[13] == (std::pair<std::string, std::string>("X-Cache-Hits", "0")));
	QUARK_VERIFY(r.optional_body == "");
}

QUARK_TEST("socket-component", "unpack_http_response_string()", "k_http_response2", ""){
	const auto r = unpack_http_response_string(k_http_response2);
	QUARK_VERIFY(r.status_line == "HTTP/1.0 200 OK");
	QUARK_VERIFY(r.headers.size() == 7);
	QUARK_VERIFY(r.headers[0] == (std::pair<std::string, std::string>("Accept-Ranges", "bytes")));
	QUARK_VERIFY(r.headers[6] == (std::pair<std::string, std::string>("Connection", "close")));
	QUARK_VERIFY(r.optional_body == "\t<html><head><title>edgecastcdn.net</title></head><body><h1>edgecastcdn.net</h1></body></html>");
}




std::string execute_http_request(const http_request_t& request){
	const auto socket = socket_t(request.af);

	struct sockaddr_in a;
	memset(&a, '0', sizeof(a));
	a.sin_family = (sa_family_t)request.af;
	a.sin_port = htons(request.port);
	a.sin_addr = request.addr;

	const auto connect_err = ::connect(socket._fd, (const struct sockaddr*)&a, sizeof(a));
	if (connect_err != 0){
		throw_errno2("connect()", get_unix_err());
	}

	write_socket(socket._fd , request.message);

	std::string response = read_socket(socket._fd);
	return response;
}

//??? store request as kv too!
http_request_t make_http_request_helper(
	const std::string& addr,
	int port,
	int af,
	const std::string& message
){
	const auto e = lookup_host(addr);
	QUARK_ASSERT(e.addresses_IPv4.size() >= 1);

	return http_request_t {
		e.addresses_IPv4[0],
		port,
		af,
		message
	};
}


QUARK_TEST("socket-component", "", "", ""){
	const auto r = execute_http_request(make_http_request_helper("cnn.com", 80, AF_INET, make_http_request_string("GET / HTTP/1.1", { "Host: www.cnn.com" }, "")));
	QUARK_TRACE(r);
	QUARK_VERIFY(r.empty() == false);
}

QUARK_TEST("socket-component", "", "", ""){
	const auto r = execute_http_request(make_http_request_helper("example.com", 80, AF_INET, make_http_request_string("GET /index.html HTTP/1.0", { }, "")));
	QUARK_TRACE(r);
}

QUARK_TEST("socket-component", "", "", ""){
	const auto r = execute_http_request(make_http_request_helper("google.com", 80, AF_INET, make_http_request_string("GET /index.html HTTP/1.0", { }, "")));
	QUARK_TRACE(r);
}

QUARK_TEST("socket-component", "", "", ""){
	const auto r = execute_http_request(make_http_request_helper("google.com", 80, AF_INET, make_http_request_string("GET / HTTP/1.0", { "Host: www.google.com" }, "")));
	QUARK_TRACE(r);
}

QUARK_TEST("socket-component", "", "", ""){
//	const auto r = execute_http_request(http_request_t { lookup_host("stackoverflow.com", AF_INET).addresses_IPv4[0], 80, AF_INET, "GET / HTTP/1.0" "\r\n" "Host: stackoverflow.com" "\r\n" "\r\n" });
	const auto r = execute_http_request(make_http_request_helper("stackoverflow.com", 80, AF_INET, make_http_request_string("GET /index.html HTTP/1.0", { "Host: www.stackoverflow.com" }, "")));
	QUARK_TRACE(r);
}





//	http://localhost:8080/info.html


void execute_http_server(const tcp_server_params_t& params){
	socket_t socket(params.af);

	/* htonl converts a long integer (e.g. address) to a network representation */
	/* htons converts a short integer (e.g. port) to a network representation */
	struct sockaddr_in address;
	memset((char *)&address, 0, sizeof(address));
	address.sin_family = (sa_family_t)params.af;
	address.sin_addr.s_addr = htonl(INADDR_ANY);
	address.sin_port = htons(params.port);
	const auto bind_result = ::bind(socket._fd, (struct sockaddr *)&address, sizeof(address));
	if(bind_result != 0){
		throw_errno2("bind()", get_unix_err());
	}

	const auto listen_result = ::listen(socket._fd, 3);
	if (listen_result != 0){
		throw_errno2("listen()", get_unix_err());
	}

    int addrlen = sizeof(address);

	while(true){
		const auto socket2 = accept(socket._fd, (struct sockaddr *)&address, (socklen_t*)&addrlen);
		if (socket2 < 0){
			QUARK_ASSERT(socket2 == -1);
			throw_errno2("accept()", get_unix_err());
		}

		const auto read_data = read_socket(socket2);

		auto pos = seq_t(read_data);
		const auto x = if_first(pos, "GET /info.html HTTP/1.1");
		if(x.first){
			const std::string doc0 = "<html><head><title>edgecastcdn.net</title></head><body><h1>edgecastcdn.net</h1></body></html>";
			const std::string doc = R"___(
				<head>
					<title>Hello Floyd Server</title>
				</head>
				<body>
					<h1>Hello Floyd Server</h1>
					This document may be found <a HREF="https://stackoverflow.com/index.html">here</a>
				</body>
			)___";

			std::string r = make_http_response_string(
				"HTTP/1.1 200 OK",
				{
					"Content-Type: text/html",
					"Content-Length: " + std::to_string(doc.size())
				},
				doc
			);
			write_socket(socket2, r);
		}
		else {
			std::string r = make_http_response_string("HTTP/1.1 404 OK", {}, "");
			write_socket(socket2, r);
		}

		close(socket2);
	}
}

#if 0
//	Warning: this is not a real unit test, it runs forever.
QUARK_TEST_VIP("socket-component", "", "", ""){
	execute_http_server(tcp_server_params_t { 8080, AF_INET });
}

#endif

