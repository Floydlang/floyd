//
//  floyd_sockets.hpp
//  floyd
//
//  Created by Marcus Zetterquist on 2019-11-28.
//  Copyright © 2019 Marcus Zetterquist. All rights reserved.
//

#ifndef floyd_sockets_hpp
#define floyd_sockets_hpp

#include <stdio.h>
#include <vector>
#include <string>

struct hostent_t {
	std::string official_host_name;
	std::vector<std::string> name_aliases;
	std::vector<struct in_addr> addresses_IPv4;
};


hostent_t sockets_gethostbyaddr2(const struct in_addr& addr_v4, int af);


#endif /* floyd_sockets_hpp */
