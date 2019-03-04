//
//  hardware_caps.h
//  floyd_speak
//
//  Created by Marcus Zetterquist on 2018-09-19.
//  Copyright © 2018 Marcus Zetterquist. All rights reserved.
//

#ifndef hardware_caps_h
#define hardware_caps_h

/*
	Getting information about the computer the program is running on at the moment.
*/

#include <cstdint>
#include <cstddef>
#include <string>

namespace floyd {

struct hardware_caps_t {
	std::string _hw_machine;
	std::string _hw_model;
	std::uint32_t _hw_ncpu;

	std::uint32_t _hw_byteorder;
	std::uint32_t _hw_physmem;
	std::uint32_t _hw_usermem;

	std::uint32_t _hw_epoch;
	std::uint32_t _hw_floatingpoint;
	std::string _hw_machinearch;

	std::uint32_t _hw_vectorunit;
	std::uint32_t _hw_tbfrequency;
	std::uint32_t _hw_availcpu;


	std::uint64_t _hw_cpu_type;
	std::uint64_t _hw_cpu_type_subtype;

	std::uint32_t _hw_packaged;

	//	Core count
	std::uint32_t _hw_physical_processor_count;

	//	Total hardware thread count (can be more than physical because of hyperthreading).
	std::uint32_t _hw_logical_processor_count;


	std::uint64_t _hw_cpu_freq_hz;
	std::uint64_t _hw_bus_freq_hz;


	std::size_t _hw_mem_size;
	std::size_t _hw_page_size;
	std::size_t _hw_cacheline_size;

	//	Usually depends on "long double".
	std::size_t _hw_scalar_align;

	std::size_t _hw_l1_data_cache_size;
	std::size_t _hw_l1_instruction_cache_size;
	std::size_t _hw_l2_cache_size;
	std::size_t _hw_l3_cache_size;
};

hardware_caps_t read_hardware_caps();
std::string get_hardware_caps_string(const hardware_caps_t& caps);
void trace_hardware_caps(const hardware_caps_t& caps);

}

#endif /* hardware_caps_h */
