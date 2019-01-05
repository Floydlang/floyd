//
//  network_api.hpp
//  floyd_speak
//
//  Created by Marcus Zetterquist on 2019-01-05.
//  Copyright © 2019 Marcus Zetterquist. All rights reserved.
//

#ifndef network_api_hpp
#define network_api_hpp

#include <string>
#include <map>

#include "core_types.h"

/*
??? How can server code server many clients in parallell?
??? How can a client have many pending requests it waits for.
*/


//////////////////////////////////////		URLS

//??? needed? Better to hide all escaping etc in highlevel function.
std::string escape_string_to_url(const std::string& s);
std::string unescape_string_from_url(const std::string& s);



bool is_valid_url(const std::string& s);
url_t make_url(const std::string& s);

struct url_parts_t {
	std::string protocol;
	std::string path;
	std::string parameters;
};


url_parts_t split_url(const url_t& url);
url_t make_urls(const url_parts_t& parts);



//////////////////////////////		SOCKETS


/*
	When performing a call that is IO-bound:

	1. Make a sync call -- your code will stop until the IO operation is completed.
		- the message has been sent via sockets
		- destination devices has processed or failed to handle the message
		- the destination has transmitted a reply back.

	2. Make an async call and privide a tag. Floyd will queue up your request then return immediately so your code can continue executing.
 	These functions are called "queue()". At a future time when there is a reply, your green-process will receive a special message in its INBOX.
*/

struct oauth2_token_t {
	std::string token_string;
};

struct message_t {
	binary_t payload;
};

struct socket_reply_t {
	binary_t payload;
};


struct server_socket_t {
	int id;
};

server_socket_t open_server_socket(const url_t& url, int port, const inbox_tag_t& inbox_tag);
void close_server_socket(const server_socket_t& s);




struct client_socket_t {
	int id;
};

client_socket_t open_client_socket(const url_t& url, int port);
void close_client_socket(const client_socket_t& s);

//	Blocks until IO is complete.
socket_reply_t send(const client_socket_t& s, const message_t& message);

//	Returns at once. When later a reply is received, you will get a message with a socket_reply_t in your green-process INBOX.
void queue(const client_socket_t& s, const message_t& message, const inbox_tag_t& inbox_tag);


//////////////////////////////////////		REST


/*
	404 för resource not found
	406 för felformaterad input
	304 not modified är det


	kOK = 1000,
	kTimeout,
	kMissingResponseDefect,	//	??? This is not a problem now that we use Parse.com, right?
	kNotFound,
	kUnknownError,
	kBadRequest
*/

struct rest_request_t {
	//	"GET", "POST", "PUT"
	std::string operation_type;

	std::map<std::string, std::string> headers;
	std::map<std::string, std::string> params;
	binary_t raw_data;

	oauth2_token_t session_token;
};

struct rest_reply_t {
	int html_status_code;
	std::string response;
	std::string internal_exception;
};

//	Blocks until IO is complete.
rest_reply_t send(const rest_request_t& request);

//	Returns at once. When later a reply is received, you will get a message with a rest_reply_t in your green-process INBOX.
void queue_rest(const rest_request_t& request, const inbox_tag_t& inbox_tag);


#endif /* network_api_hpp */
