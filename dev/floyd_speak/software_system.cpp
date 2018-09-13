//
//  software_system.cpp
//  floyd_speak
//
//  Created by Marcus Zetterquist on 2018-09-13.
//  Copyright © 2018 Marcus Zetterquist. All rights reserved.
//

#include "software_system.h"


using std::string;
using std::vector;



vector<person_t> unpack_persons(const json_t& persons_obj){
	vector<person_t> result;
	const auto temp = persons_obj.get_object();
	for(const auto& person_pairs: temp){
		const auto name = person_pairs.first;
		const auto desc = person_pairs.second.get_string();
		result.push_back(person_t { ._name_key = name, ._desc = desc} );
	}
	return result;
}


clock_bus_t unpack_clock_bus(const json_t& clock_bus_obj){
	std::map<std::string, std::string> actors;

	const auto actors_map = clock_bus_obj.get_object();
	for(const auto& actor_pair: actors_map){
		const auto name_key = actor_pair.first;
		const auto actor_function_key = actor_pair.second.get_string();
		actors.insert({name_key, actor_function_key} );
	}
	return clock_bus_t{._actors = actors};
}

std::map<std::string, clock_bus_t> unpack_clock_busses(const json_t& clocks_obj){
	std::map<std::string, clock_bus_t> result;
	const auto temp = clocks_obj.get_object();
	for(const auto& clock_pair: temp){
		const auto name = clock_pair.first;
		const auto clock_bus = unpack_clock_bus(clock_pair.second);
		result.insert({name, clock_bus});
	}
	return result;
}
container_t unpack_container(const json_t& container_obj){
	return container_t{
		._tech = container_obj.get_object_element("tech").get_string(),
		._desc = container_obj.get_object_element("desc").get_string(),
		._clock_busses = unpack_clock_busses(container_obj.get_object_element("clocks")),
		._connections = {},
		._components = {}
	};
}

std::map<string, container_t> unpack_containers(const json_t& containers_obj){
	std::map<string, container_t> result;
	const auto containers_map = containers_obj.get_object();
	for(const auto& container_pair: containers_map){
		const auto name = container_pair.first;
		const auto container = unpack_container(container_pair.second);
		result.insert({name, container});
	}
	return result;
}

software_system_t parse_software_system_json(const json_t& value){
	const auto name = value.get_object_element("name").get_string();
	const auto desc = value.get_object_element("desc").get_string();
	const auto people = unpack_persons(value.get_object_element("people"));
	const auto connections = value.get_object_element("connections");
	const auto containers = unpack_containers(value.get_object_element("containers"));
	return software_system_t{
		._name = name,
		._desc = desc,
		._people = people,
		._connections = {},
		._containers = containers
	};
}
