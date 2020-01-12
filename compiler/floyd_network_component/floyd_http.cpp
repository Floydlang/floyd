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

//#define QUARK_TEST QUARK_TEST_VIP

static std::string pack_http_request_line(const http_request_line_t& request_line);
static http_request_line_t unpack_http_request_line(const std::string& request_line);


static std::string pack_http_response_status_line(const http_response_status_line_t& value);
static http_response_status_line_t unpack_http_response_status_line(const std::string& s);




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


static std::pair<std::string, seq_t> read_to_crlf(const seq_t& p){
	return read_until_str(p, kCRLF, true);
}

static const std::string k_skip_leading_chars = " \t";

static std::pair<std::string, seq_t> read_to_crlf_skip_leads(const seq_t& p){
	const auto a = read_until_str(p, kCRLF, true);
	const auto b = skip(seq_t(a.first), k_skip_leading_chars);
	return { b.str(), a.second };
}

static std::pair<std::vector<http_header_t>, seq_t> unpack_headers(const seq_t& p0){
	auto p = p0;
	std::vector<http_header_t> headers;
	while(p.empty() == false && is_first(p, kCRLF) == false){
		const auto h = read_to_crlf_skip_leads(p);

		const auto key_kv = read_until_str(seq_t(h.first), ": ", true);
		const auto value = key_kv.second.str();
		headers.push_back(
			http_header_t { key_kv.first, value }
		);
		p = h.second;
	}
	return { headers, p };
}

static std::string pack_headers(const std::vector<http_header_t>& headers){
	std::string result;
	for(const auto& e: headers){
		const auto line = e.key + ": " + e.value;
		result = result + line + kCRLF;
	}
	return result;
}






std::string pack_http_request(const http_request_t& r){
	return
		pack_http_request_line(r.request_line) + kCRLF
		+ pack_headers(r.headers)
		+ kCRLF
		+ r.optional_body;
}

QUARK_TEST("http", "pack_http_request()","", ""){
	const auto r = pack_http_request({ http_request_line_t { "GET", "/hello.htm", "HTTP/1.1" }, {}, "" });
	QUARK_VERIFY(r == "GET /hello.htm HTTP/1.1\r\n\r\n");
}
QUARK_TEST("http", "pack_http_request()","", ""){
	const auto a = http_request_t {
		http_request_line_t { "GET", "/hello.htm", "HTTP/1.1" },
		std::vector<http_header_t>{{
			http_header_t{ "Host", "www.tutorialspoint.com" },
			http_header_t{ "Accept-Language", "en-us" }
		}},
		"licenseID=string&content=string&/paramsXML=string"
	};
	const auto r = pack_http_request(a);
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




http_request_t unpack_http_request(const std::string& s){
	seq_t p(s);
	const auto request_line_pos = read_to_crlf_skip_leads(p);
	p = request_line_pos.second;

	const auto headers = unpack_headers(p);
	p = headers.second;

	if(is_first(p, kCRLF)){
		p = p.rest(kCRLF.size());
	}

	const auto body = p.first(p.size());

	return {
		unpack_http_request_line(request_line_pos.first),
		headers.first,
		body
	};
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


std::string pack_http_response(const http_response_t& r){
	return
		pack_http_response_status_line(r.status_line) + kCRLF
		+ pack_headers(r.headers)
		+ kCRLF
		+ r.optional_body;
}


bool operator==(const http_response_t& lhs, const http_response_t& rhs){
	return
		lhs.status_line == rhs.status_line
		&& lhs.headers == rhs.headers
		&& lhs.optional_body == rhs.optional_body;
}

http_response_t unpack_http_response(const std::string& s){
	seq_t p(s);
	const auto status_line_pos = read_to_crlf_skip_leads(p);
	p = status_line_pos.second;

	const auto headers = unpack_headers(p);
	p = headers.second;

	if(is_first(p, kCRLF)){
		p = p.rest(kCRLF.size());
	}

	const auto body = p.first(p.size());

	return {
		unpack_http_response_status_line(status_line_pos.first),
		headers.first,
		body
	};
}

QUARK_TEST("http", "unpack_http_response()", "k_http_response1", ""){
	const auto r = unpack_http_response(k_http_response1);
	QUARK_VERIFY(r.status_line == (http_response_status_line_t { "HTTP/1.1", "301 Moved Permanently" }));
	QUARK_VERIFY(r.headers.size() == 14);
	QUARK_VERIFY(r.headers[0] == (http_header_t { "Server", "Varnish" }));
	QUARK_VERIFY(r.headers[13] == (http_header_t { "X-Cache-Hits", "0" }));
	QUARK_VERIFY(r.optional_body == "");
}

QUARK_TEST("http", "unpack_http_response()", "k_http_response2", ""){
	const auto r = unpack_http_response(k_http_response2);
	QUARK_VERIFY(r.status_line == (http_response_status_line_t { "HTTP/1.0", "200 OK" }) );
	QUARK_VERIFY(r.headers.size() == 7);
	QUARK_VERIFY(r.headers[0] == (http_header_t { "Accept-Ranges", "bytes" } ));
	QUARK_VERIFY(r.headers[6] == (http_header_t { "Connection", "close" }));
	QUARK_VERIFY(r.optional_body == "\t<html><head><title>edgecastcdn.net</title></head><body><h1>edgecastcdn.net</h1></body></html>");
}


ip_address_and_port_t make_http_dest(sockets_i& sockets, const std::string& addr, int port, int af){
	const auto e = sockets.sockets_i__lookup_host(addr);
	QUARK_ASSERT(e.addresses_IPv4.size() >= 1);

	return ip_address_and_port_t { e.addresses_IPv4[0], port };
}

//const std::string doc0 = "<html><head><title>edgecastcdn.net</title></head><body><h1>edgecastcdn.net</h1></body></html>";




/*
Format: request-method-name request-URI HTTP-version

Examples:
	GET /test.html HTTP/1.1
	HEAD /query.html HTTP/1.0
	POST /index.html HTTP/1.1

	GET /info.html HTTP/1.1
*/
//	Input string is the first line of the message only, no CRLF allowed.
std::string pack_http_request_line(const http_request_line_t& request_line){
	QUARK_ASSERT(request_line.method.find(" ") == std::string::npos);
	QUARK_ASSERT(request_line.uri.find(" ") == std::string::npos);
	QUARK_ASSERT(request_line.http_version == "HTTP/1.1" || request_line.http_version == "HTTP/1.0");

	return request_line.method + " " + request_line.uri + " " + request_line.http_version;
}

QUARK_TEST("http", "pack_http_request_line()", "", ""){
	QUARK_VERIFY(pack_http_request_line({ "GET", "/test.html", "HTTP/1.1" }) == "GET /test.html HTTP/1.1");
}
QUARK_TEST("http", "pack_http_request_line()", "", ""){
	QUARK_VERIFY(pack_http_request_line({ "HEAD", "/query.html", "HTTP/1.0" }) == "HEAD /query.html HTTP/1.0");
}
QUARK_TEST("http", "pack_http_request_line()", "", ""){
	QUARK_VERIFY(pack_http_request_line({ "POST", "/index.html", "HTTP/1.1" }) == "POST /index.html HTTP/1.1");
}


http_request_line_t unpack_http_request_line(const std::string& request_line){
	const auto p = seq_t(request_line);

	QUARK_ASSERT(does_substr_exist(p, kCRLF) == false);

	const auto pre = read_while(p, " \t");

	const auto method = read_until(pre.second, " ");
	const auto p2 = read_required(method.second, " ");
	const auto uri = read_until(p2, " ");
	const auto p3 = read_required(uri.second, " ");
	const auto http_version = p3.str();
	if(http_version != "HTTP/1.1" && http_version != "HTTP/1.0"){
		throw std::runtime_error(std::string() + "Illegal HTTP request list, HTTP-version: " + http_version);
	}

	return http_request_line_t { method.first, uri.first, http_version };
}

QUARK_TEST("http", "unpack_http_request_line()", "", ""){
	QUARK_VERIFY(unpack_http_request_line("GET /test.html HTTP/1.1") == (http_request_line_t{ "GET", "/test.html", "HTTP/1.1" }) );
}
QUARK_TEST("http", "unpack_http_request_line()", "", ""){
	QUARK_VERIFY(unpack_http_request_line("HEAD /query.html HTTP/1.0") == (http_request_line_t{ "HEAD", "/query.html", "HTTP/1.0" }) );
}
QUARK_TEST("http", "unpack_http_request_line()", "", ""){
	QUARK_VERIFY(unpack_http_request_line("POST /index.html HTTP/1.1") == (http_request_line_t{ "POST", "/index.html", "HTTP/1.1" }) );
}

QUARK_TEST("http", "unpack_http_request_line()", "two spaces", ""){
	try {
		unpack_http_request_line("GET  /test.html HTTP/1.1");
		QUARK_VERIFY(false);
	}
	catch(const std::runtime_error& r){
	}
}
QUARK_TEST("http", "unpack_http_request_line()", "Illegal http version", ""){
	try {
		unpack_http_request_line("GET /test.html HTTP/1.2");
		QUARK_VERIFY(false);
	}
	catch(const std::runtime_error& r){
	}
}
QUARK_TEST("http", "unpack_http_request_line()", "two spaces", ""){
	try {
		unpack_http_request_line("GET /test.html  HTTP/1.2");
		QUARK_VERIFY(false);
	}
	catch(const std::runtime_error& r){
	}
}

QUARK_TEST("http", "unpack_http_request_line()", "leading tab", ""){
	QUARK_VERIFY(unpack_http_request_line("\tGET /test.html HTTP/1.1") == (http_request_line_t{ "GET", "/test.html", "HTTP/1.1" }) );
}




/*
HTTP/1.1 200 OK
HTTP/1.1 400 Bad Request
HTTP/1.1 403 Forbidden
HTTP/1.1 404 Not Found
HTTP/1.1 501 Method Not Implemented
HTTP/1.1 200 OK
HTTP/1.0 404 Not Found

The first line of the response message (i.e., the status line) contains the response status code, which is generated by the server to indicate the outcome of the request.
*/

static std::string pack_http_response_status_line(const http_response_status_line_t& value){
	QUARK_ASSERT(value.http_version == "HTTP/1.1" || value.http_version == "HTTP/1.0");
	QUARK_ASSERT(value.status_code.size() > 0);

	return value.http_version + " " + value.status_code;
}

QUARK_TEST("http", "pack_http_response_status_line()", "", ""){
	QUARK_VERIFY(pack_http_response_status_line({ "HTTP/1.1", "200 OK" }) == "HTTP/1.1 200 OK");
}


static http_response_status_line_t unpack_http_response_status_line(const std::string& s){
	const auto p = seq_t(s);
	QUARK_ASSERT(does_substr_exist(p, kCRLF) == false);

	const auto http_version = read_until(p, " ");
	const auto p2 = read_required(http_version.second, " ");
	const auto status_code = p2.str();

	if(http_version.first != "HTTP/1.1" && http_version.first != "HTTP/1.0"){
		throw std::runtime_error(std::string() + "Illegal HTTP request list, HTTP-version: " + http_version.first);
	}

	return http_response_status_line_t { http_version.first, status_code };
}

QUARK_TEST("http", "unpack_http_response_status_line()", "", ""){
	QUARK_VERIFY(unpack_http_response_status_line("HTTP/1.1 200 OK") == (http_response_status_line_t{ "HTTP/1.1", "200 OK" }) );
}
QUARK_TEST("http", "unpack_http_response_status_line()", "", ""){
	QUARK_VERIFY(unpack_http_response_status_line("HTTP/1.1 400 Bad Request") == (http_response_status_line_t{ "HTTP/1.1", "400 Bad Request" }) );
}
QUARK_TEST("http", "unpack_http_response_status_line()", "Bad http version", ""){
	try {
		unpack_http_response_status_line("HTTP/1.2 Bad Request");
		QUARK_VERIFY(false);
	}
	catch(const std::runtime_error& e){
	}
}

QUARK_TEST("http", "unpack_http_response_status_line()", "leading whitespace", ""){
	try {
		unpack_http_response_status_line("\tHTTP/1.1 Bad Request");
		QUARK_VERIFY(false);
	}
	catch(const std::runtime_error& e){
	}
}
QUARK_TEST("http", "unpack_http_response_status_line()", "whitespace", ""){
	try {
		unpack_http_response_status_line("\tHTTP/1.1  Bad Request");
		QUARK_VERIFY(false);
	}
	catch(const std::runtime_error& e){
	}
}







/*


Uniform Resource Locator (URL)
A URL (Uniform Resource Locator) is used to uniquely identify a resource over the web. URL has the following syntax:

protocol://hostname:port/path-and-file-name
There are 4 parts in a URL:

Protocol: The application-level protocol used by the client and server, e.g., HTTP, FTP, and telnet.
Hostname: The DNS domain name (e.g., www.nowhere123.com) or IP address (e.g., 192.128.1.2) of the server.
Port: The TCP port number that the server is listening for incoming requests from the clients.
Path-and-file-name: The name and location of the requested resource, under the server document base directory.
For example, in the URL http://www.nowhere123.com/docs/index.html, the communication protocol is HTTP; the hostname is www.nowhere123.com. The port number was not specified in the URL, and takes on the default number, which is TCP port 80 for HTTP. The path and file name for the resource to be located is "/docs/index.html".

Other examples of URL are:

ftp://www.ftp.org/docs/test.txt
mailto:user@test101.com
news:soc.culture.Singapore
telnet://www.nowhere123.com/

struct uri_parts_t {
	std::string protocol;
	std::string hostname;
	int port;
	std::string path;
};

*/


std::string execute_http_request(sockets_i& sockets, const ip_address_and_port_t& addr, const std::string& message){
	const auto connection = sockets.sockets_i__connect_to_server(addr);
	write_socket_string(sockets, connection.socket->_fd, message);
	std::string response = read_socket_string(sockets, connection.socket->_fd);
	return response;
}

QUARK_TEST("http", "execute_http_request()", "", ""){
	sockets_t sockets;
	const auto a = http_request_t {
		http_request_line_t { "GET", "/", "HTTP/1.1" },
		std::vector<http_header_t>{ http_header_t{ "Host", "www.cnn.com" } },
		""
	};
	const auto r = execute_http_request(sockets, make_http_dest(sockets, "cnn.com", 80, AF_INET), pack_http_request(a));
	QUARK_TRACE(r);
	QUARK_VERIFY(r.empty() == false);
}

QUARK_TEST("http", "execute_http_request()", "", ""){
	sockets_t sockets;
	const auto a = http_request_t { http_request_line_t { "GET", "/index.html", "HTTP/1.0" }, { }, "" };
	const auto r = execute_http_request(sockets, make_http_dest(sockets, "example.com", 80, AF_INET), pack_http_request(a));
	QUARK_TRACE(r);
}

QUARK_TEST("http", "execute_http_request()", "", ""){
	sockets_t sockets;
	const auto a = http_request_t { http_request_line_t { "GET", "/index.html", "HTTP/1.0" }, { }, "" };
	const auto r = execute_http_request(sockets, make_http_dest(sockets, "google.com", 80, AF_INET), pack_http_request(a));
	QUARK_TRACE(r);
}

QUARK_TEST("http", "execute_http_request()", "", ""){
	sockets_t sockets;
	const auto a = http_request_t { http_request_line_t { "GET", "/", "HTTP/1.0" }, std::vector<http_header_t>{ http_header_t{ "Host", "www.google.com" } }, "" };
	const auto r = execute_http_request(sockets, make_http_dest(sockets, "google.com", 80, AF_INET), pack_http_request(a));
	QUARK_TRACE(r);
}

QUARK_TEST("http", "execute_http_request()", "", ""){
	sockets_t sockets;
//	const auto r = execute_http_request(http_request_t { lookup_host("stackoverflow.com", AF_INET).addresses_IPv4[0], 80, AF_INET, "GET / HTTP/1.0" "\r\n" "Host: stackoverflow.com" "\r\n" "\r\n" });
	const auto a = http_request_t { http_request_line_t { "GET", "/index.html", "HTTP/1.0" }, std::vector<http_header_t>{ http_header_t{ "Host", "www.stackoverflow.com" } }, "" };
	const auto r = execute_http_request(sockets, make_http_dest(sockets, "stackoverflow.com", 80, AF_INET), pack_http_request(a));
	QUARK_TRACE(r);
}







void execute_http_server(sockets_i& sockets, const server_params_t& params, connection_i& connection){
	sockets.sockets_i__execute_server(params, connection);
}





struct test_connection_t : public connection_i {
	test_connection_t(sockets_i* sockets) :
		sockets(sockets)
	{
	}

	public: bool connection_i__on_accept(int socket2) override {
		const auto read_data = read_socket_string(*sockets, socket2);
		if(read_data.empty()){
			QUARK_TRACE("empty");
		}
		else{
			auto pos = seq_t(read_data);
			const auto request = unpack_http_request(read_data);
			const auto t = pack_http_request_line(request.request_line);
			QUARK_TRACE_SS(t);
			if(request.request_line == http_request_line_t{ "GET", "/info.html", "HTTP/1.1" }){
				const std::string doc = R"___(
					<head>
						<title>Hello Floyd Server</title>
					</head>
					<body>
						<h1>Hello Floyd Server</h1>
						This document may be found <a HREF="https://stackoverflow.com/index.html">here</a>
					</body>
				)___";

				std::string r = pack_http_response(
					http_response_t {
						http_response_status_line_t { "HTTP/1.1", "200 OK" },
						{{
							{ "Content-Type", "text/html" },
							{ "Content-Length", std::to_string(doc.size()) }
						}},
						doc
					}
				);
				write_socket_string(*sockets, socket2, r);
			}
			else {
				std::string r = pack_http_response(http_response_t { http_response_status_line_t { "HTTP/1.1", "404 OK" }, {}, "" });
				write_socket_string(*sockets, socket2, r);
			}
		}
		return true;
	}


	sockets_i* sockets;
};

#if 0
//	Warning: this is not a real unit test, it runs forever.
QUARK_TEST_VIP("http", "execute_http_server", "", ""){
	sockets_t sockets;
	test_connection_t test_conn { &sockets };
	execute_http_server(sockets, server_params_t { 8080 }, test_conn);
}

#endif

