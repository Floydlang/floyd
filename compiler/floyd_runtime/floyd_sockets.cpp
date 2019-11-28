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
#include "quark.h"


//#define QUARK_TEST QUARK_TEST_VIP

/*
	The gethostbyname*() and gethostbyaddr*() functions are obsolete. Applications should use getaddrinfo(3) and getnameinfo(3) instead.
	const char *gai_strerror(int errcode);
	gethostbyname2_r() is safer
*/

void throw_errno2(const std::string& s, int error) QUARK_NO_RETURN;
void throw_errno2(const std::string& s, int error) {
	// No such host is known in the database.
	if(error == HOST_NOT_FOUND){
		throw std::runtime_error(s + "HOST_NOT_FOUND");
	}

	// This condition happens when the name server could not be contacted. If you try again later, you may succeed then.
	else if(error == TRY_AGAIN){
		throw std::runtime_error(s + "TRY_AGAIN");
	}

	// A non-recoverable error occurred.
	else if(error == NO_RECOVERY){
		throw std::runtime_error(s + "NO_RECOVERY");
	}

	// The host database contains an entry for the name, but it doesn’t have an associated Internet address.
	else if(error == NO_ADDRESS){
		throw std::runtime_error(s + "NO_ADDRESS");
	}
	else{
		throw std::runtime_error(s);
	}
}




struct socket_t {
	bool check_invariant() const;
	socket_t(int af);
	~socket_t();


	int _fd;
};

bool socket_t::check_invariant() const{
	QUARK_ASSERT(_fd >= 0);
	return true;
}

socket_t::socket_t(int af){
	const auto fd = ::socket(af, SOCK_STREAM, 0);
	if (fd == -1){
		throw_errno2("Socket creation error", errno);
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



std::vector<std::string> unpack_vector_of_strings(char** vec){
	QUARK_ASSERT(vec != nullptr);
	std::vector<std::string> result;
	for(int i = 0 ; vec[i] != nullptr ; i++){
		const std::string s(vec[i]);
		result.push_back(s);
	}
	return result;
}


/*
The inet_ntoa() function converts the Internet host address in, given in network byte order, to a string in IPv4 dotted-decimal notation. The string is returned in a statically allocated buffer, which subsequent calls will overwrite.
*/
std::string to_string(const struct in_addr& a){
	const std::string s = inet_ntoa(a);
	return s;
}


json_t to_json(const hostent_t& value){
	return json_t::make_object({
		{ "official_host_name", value.official_host_name },
		{ "name_aliases", json_t::make_array(
			mapf<json_t>(value.name_aliases, [](const auto& e){ return json_t(e); })
		) },
		{ "addresses_IPv4", json_t::make_array(
			mapf<json_t>(value.addresses_IPv4, [](const auto& e){ return to_string(e) /*+ "(" + sockets_gethostbyaddr2(e, AF_INET).official_host_name + ")"*/; } )
		) }
	});
}



static std::string error_to_string(int errnum){
	char temp[1024 + 1];
	int r = strerror_r(errnum, temp, 1024);
	QUARK_ASSERT(r == 0);
	return std::string(temp);
}



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

	int flags = fcntl(fd, F_GETFL, 0);
	if (flags == -1){
		return false;
	}
	flags = blocking ? (flags & ~O_NONBLOCK) : (flags | O_NONBLOCK);
	const auto a = fcntl(fd, F_SETFL, flags);
	return a == 0 ? true : false;
}

#endif





//??? network-ordered bytes.
struct in_addr make_in_addr_from_ip_string(const std::string& s){
	struct in_addr result;
	int r = inet_aton(s.c_str(), &result);

/*
	// Convert IPv4 and IPv6 addresses from text to binary form
	if(inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr) <= 0){
		throw std::runtime_error("Invalid address - Address not supported");
	}
*/

	if(r == 0){
		QUARK_ASSERT(false);
		throw std::runtime_error("inet_aton() failed");
	}
	return result;
}


std::string read_socket(int socket){
	QUARK_ASSERT(socket >= 0);

	char buffer[1024] = { 0 };
	std::string result;
	while(true){
		const auto read_result = read(socket , buffer, 1024);
		if(read_result < 0){
			QUARK_ASSERT(read_result == -1);
			throw_errno2("read()", errno);
		}
		else {
			if(read_result == 0){
				return result;
			}
			else if(read_result < 1024){
				std::string s(buffer);
				result = result + s;
				return result;
			}
			else if(read_result == 1024){
				std::string s(buffer);
				result = result + s;

				//	Out input buffer was full, read more data.
			}
		}
	}
}

//??? Sockets should use bytes, not string.

void write_socket(int socket, const std::string& data){
//	::write(socket, data.c_str(), data.size());
	const auto send_result = ::send(socket , data.c_str() , data.size(), 0);
	if(send_result < 0){
		QUARK_ASSERT(send_result == -1);
		throw_errno2("send()", errno);
	}
}


hostent_t unpack_hostent(const struct hostent& e){
	std::vector<std::string> aliases = unpack_vector_of_strings(e.h_aliases);

	// ??? Add proper support for AF_INET or AF_INET6, with the latter being used for IPv6 hosts.
	const size_t address_length = e.h_length;
	if(address_length != 4){
		throw std::runtime_error("Unsupported adress length");
	}

	std::vector<struct in_addr> addresses;
	for(int i = 0 ; e.h_addr_list[i] != nullptr ; i++){
		const auto a = (struct in_addr *)e.h_addr_list[i];
		addresses.push_back(*a);
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




//	gethostbyname2_r() is safer
hostent_t sockets_gethostbyname2(const std::string& name, int af){
	//	Function: struct hostent * gethostbyname2 (const char *name, int af)
	const auto e = gethostbyname2(name.c_str(), af);
	if(e == nullptr){
		const auto host_error = ::h_errno;
		const auto error_str = hstrerror(host_error);
		throw std::runtime_error("gethostbyname2(): " + std::string(error_str) + ": " + std::to_string(host_error));
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
		const auto host_error = ::h_errno;
		const auto error_str = hstrerror(host_error);
		throw std::runtime_error("gethostbyaddr(): " + std::string(error_str) + ": " + std::to_string(host_error));
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


/*
	Will add CRLF after request_line, headers.
*/
std::string make_http_request_str(const std::string& request_line, const std::vector<std::string>& headers, const std::string& optional_body){
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



QUARK_TEST("socket-component", "make_http_request_str()","", ""){
	const auto r = make_http_request_str("GET /hello.htm HTTP/1.1", {}, "");
	QUARK_VERIFY(r == "GET /hello.htm HTTP/1.1\r\n\r\n");
}
QUARK_TEST("socket-component", "make_http_request_str()","", ""){
	const auto r = make_http_request_str("GET /hello.htm HTTP/1.1", { "Host: www.tutorialspoint.com", "Accept-Language: en-us" }, "licenseID=string&content=string&/paramsXML=string");
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

struct http_response_t {
	std::string status_line;
	std::vector<std::pair<std::string, std::string>> headers;
	std::string optional_body;
};
bool operator==(const http_response_t& lhs, const http_response_t& rhs){
	return lhs.status_line == rhs.status_line && lhs.headers == rhs.headers && lhs.optional_body == rhs.optional_body;
}



std::pair<std::string, seq_t> read_to_crlf(const seq_t& p){
	return read_until_str(p, kCRLF, true);
}

std::pair<std::string, seq_t> read_to_crlf_skip_leads(const seq_t& p){
	const auto a = read_until_str(p, kCRLF, true);
	const auto b = skip(seq_t(a.first), k_skip_leading_chars);
	return { b.str(), a.second };
}

std::string make_http_response_str(const std::string& status_line, const std::vector<std::string>& headers, const std::string& optional_body){
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

http_response_t unpack_response_string(const std::string& s){
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

QUARK_TEST("socket-component", "unpack_response_string()", "k_http_response1", ""){
	const auto r = unpack_response_string(k_http_response1);
	QUARK_VERIFY(r.status_line == "HTTP/1.1 301 Moved Permanently");
	QUARK_VERIFY(r.headers.size() == 14);
	QUARK_VERIFY(r.headers[0] == (std::pair<std::string, std::string>("Server", "Varnish")));
	QUARK_VERIFY(r.headers[13] == (std::pair<std::string, std::string>("X-Cache-Hits", "0")));
	QUARK_VERIFY(r.optional_body == "");
}

QUARK_TEST("socket-component", "unpack_response_string()", "k_http_response2", ""){
	const auto r = unpack_response_string(k_http_response2);
	QUARK_VERIFY(r.status_line == "HTTP/1.0 200 OK");
	QUARK_VERIFY(r.headers.size() == 7);
	QUARK_VERIFY(r.headers[0] == (std::pair<std::string, std::string>("Accept-Ranges", "bytes")));
	QUARK_VERIFY(r.headers[6] == (std::pair<std::string, std::string>("Connection", "close")));
	QUARK_VERIFY(r.optional_body == "\t<html><head><title>edgecastcdn.net</title></head><body><h1>edgecastcdn.net</h1></body></html>");
}




struct http_request_t {
	struct in_addr addr;
	int port;
	int af;
	std::string message;
};




std::string execute_request(const http_request_t& request){
	const auto socket = socket_t(request.af);

	struct sockaddr_in serv_addr;
	memset(&serv_addr, '0', sizeof(serv_addr));
	serv_addr.sin_family = (sa_family_t)request.af;
	serv_addr.sin_port = htons(request.port);
	serv_addr.sin_addr = request.addr;

	const auto connect_err = ::connect(socket._fd, (const struct sockaddr *)&serv_addr, sizeof(serv_addr));
	if (connect_err != 0){
		throw_errno2("connect()", errno);
	}

	write_socket(socket._fd , request.message);

	std::string response = read_socket(socket._fd);
	return response;
}

//??? store request as kv too!
http_request_t make_get_request(const std::string& addr, int port, int af, const std::string& command, const std::vector<std::string>& headers, const std::string& optional_body){
	const auto e = sockets_gethostbyname2(addr, af);
	QUARK_ASSERT(e.addresses_IPv4.size() >= 1);

	return http_request_t {
		e.addresses_IPv4[0],
		port,
		af,
		make_http_request_str(command, headers, optional_body)
	};
}


QUARK_TEST("socket-component", "", "", ""){
	const auto r = execute_request(make_get_request("cnn.com", 80, AF_INET, "GET / HTTP/1.1", { "Host: www.cnn.com" }, ""));
	QUARK_TRACE(r);
	QUARK_VERIFY(r.empty() == false);
}

QUARK_TEST("socket-component", "", "", ""){
	const auto r = execute_request(make_get_request("example.com", 80, AF_INET, "GET /index.html HTTP/1.0", { }, ""));
	QUARK_TRACE(r);
}

QUARK_TEST("socket-component", "", "", ""){
	const auto r = execute_request(make_get_request("google.com", 80, AF_INET, "GET /index.html HTTP/1.0", { }, ""));
	QUARK_TRACE(r);
}

QUARK_TEST("socket-component", "", "", ""){
	const auto r = execute_request(make_get_request("google.com", 80, AF_INET, "GET / HTTP/1.0", { "Host: www.google.com" }, ""));
	QUARK_TRACE(r);
}

QUARK_TEST("socket-component", "", "", ""){
//	const auto r = execute_request(http_request_t { sockets_gethostbyname2("stackoverflow.com", AF_INET).addresses_IPv4[0], 80, AF_INET, "GET / HTTP/1.0" "\r\n" "Host: stackoverflow.com" "\r\n" "\r\n" });
	const auto r = execute_request(make_get_request("stackoverflow.com", 80, AF_INET, "GET /index.html HTTP/1.0", { "Host: www.stackoverflow.com" }, ""));
	QUARK_TRACE(r);
}

//	http://localhost:8080/info.html

void execute_http_server(){
	socket_t socket(AF_INET);

	const int PORT = 8080; //Where the clients can reach at

	/* htonl converts a long integer (e.g. address) to a network representation */
	/* htons converts a short integer (e.g. port) to a network representation */
	struct sockaddr_in address;
	memset((char *)&address, 0, sizeof(address));
	address.sin_family = AF_INET;
	address.sin_addr.s_addr = htonl(INADDR_ANY);
	address.sin_port = htons(PORT);
	const auto bind_result = bind(socket._fd, (struct sockaddr *)&address, sizeof(address));
	if(bind_result != 0){
		throw_errno2("bind()", errno);
	}

	const auto listen_result = ::listen(socket._fd, 3);
	if (listen_result != 0){
		throw_errno2("listen()", errno);
	}

    int addrlen = sizeof(address);

	while(true){
		const auto new_socket = accept(socket._fd, (struct sockaddr *)&address, (socklen_t*)&addrlen);
		if (new_socket < 0){
			QUARK_ASSERT(new_socket == -1);
			throw_errno2("accept()", errno);
		}

		const auto read_data = read_socket(new_socket);
//		printf("%s\n",read_data.c_str() );

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

			std::string r = make_http_response_str(
				"HTTP/1.1 200 OK",
				{
					"Content-Type: text/html",
					"Content-Length: " + std::to_string(doc.size())
				},
				doc
			);
			write_socket(new_socket, r);
		}
		else {
			std::string r = make_http_response_str("HTTP/1.1 404 OK", {}, "");
			write_socket(new_socket, r);
		}

		close(new_socket);
	}
}

#if 0
QUARK_TEST_VIP("socket-component", "", "", ""){
	execute_http_server();
}
#endif



