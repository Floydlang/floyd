//
//  hardware_caps.h
//  floyd_speak
//
//  Created by Marcus Zetterquist on 2018-09-19.
//  Copyright © 2018 Marcus Zetterquist. All rights reserved.
//

#ifndef hardware_caps_h
#define hardware_caps_h


#include <cstdint>
#include <cstddef>

namespace floyd {

struct hardware_info_t {
	std::uint64_t _cpu_type;
	std::uint64_t _cpu_type_subtype;


	std::uint64_t _processor_packages;

	//	Core count
	std::uint32_t _physical_processor_count;

	//	Total hardware thread count (can be more than physical because of hyperthreading).
	std::uint32_t _logical_processor_count;

	std::uint64_t _cpu_freq_hz;
	std::uint64_t _bus_freq_hz;


	std::size_t _mem_size;
	std::size_t _page_size;
	std::size_t _cacheline_size;
	//	Usually depends on "long double".
	std::size_t _scalar_align;

	std::size_t _l1_data_cache_size;
	std::size_t _l1_instruction_cache_size;
	std::size_t _l2_cache_size;
	std::size_t _l3_cache_size;


};

hardware_info_t read_hardware_info();

}

#endif /* hardware_caps_h */
