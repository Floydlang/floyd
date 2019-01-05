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


//??? How can server code server many clients in parallell?
//??? How can a client have many pending requests?

//////////////////////////////////////		URLS

//??? needed? Better to hide all escaping etc in highlevel function.
std::string escape_string_to_url_string(const std::string& s);
std::string unescape_string_from_url_string(const std::string& s);


struct url_t {
	std::string complete_url_path;
};

bool is_valid_url(const std::string& s);
url_t make_url(const std::string& s);

struct url_parts_t {
	std::string protocol;
	std::string path;
	std::string parameters;
};


url_parts_t split_url(const url_t& url);
url_t make_urls(const url_parts_t& parts);


//////////////////////////////////////		REST
/*
	404 för resource not found
	406 för felformaterad input
	304 not modified är det
*/

#endif /* network_api_hpp */
