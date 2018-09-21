//
//  software_system.hpp
//  floyd_speak
//
//  Created by Marcus Zetterquist on 2018-09-13.
//  Copyright © 2018 Marcus Zetterquist. All rights reserved.
//

#ifndef software_system_hpp
#define software_system_hpp


#include "quark.h"
#include <vector>
#include <string>
#include <map>

#include "json_support.h"

struct person_t {
	std::string _name_key;
	std::string _desc;
};
struct connection_t {
	std::string _source_key;
	std::string _dest_key;
	std::string _interaction_desc;
	std::string _tech_desc;
};

struct clock_bus_t {
	//	Right now an process is the name of the process-function, will probably get more members.
	std::map<std::string, std::string> _processes;
};

struct container_t {
	std::string _desc;
	std::string _tech;
	std::map<std::string, clock_bus_t> _clock_busses;
	std::vector<connection_t> _connections;
	std::vector<std::string> _components;
};

struct software_system_t {
	std::string _name;
	std::string _desc;
	std::vector<person_t> _people;
	std::vector<connection_t> _connections;
	std::map<std::string, container_t> _containers;
};

software_system_t parse_software_system_json(const json_t& value);



#endif /* software_system_hpp */
