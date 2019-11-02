//
//  typeid.cpp
//  Floyd
//
//  Created by Marcus Zetterquist on 2018-02-12.
//  Copyright © 2018 Marcus Zetterquist. All rights reserved.
//

#include "hardware_caps.h"

#include <cstddef>
#include <string>
#include <sstream>


#include "quark.h"



QUARK_TEST("","", "", ""){
	const auto a = read_hardware_caps();
	QUARK_VERIFY(a._hw_cacheline_size >= 16);
	QUARK_VERIFY(a._hw_scalar_align >= 4);
}

std::string get_hardware_caps_string(const hardware_caps_t& caps){
	std::stringstream r;
	r << "machdep_cpu_brand_string" << ":\t" << caps._machdep_cpu_brand_string << std::endl;

	r << "machine" << ":\t" << caps._hw_machine << std::endl;
	r << "model" << ":\t" << caps._hw_model << std::endl;
	r << "ncpu" << ":\t" << caps._hw_ncpu << std::endl;

	r << "byteorder" << ":\t" << caps._hw_byteorder << std::endl;
	r << "physmem" << ":\t" << caps._hw_physmem << std::endl;
	r << "usermem" << ":\t" << caps._hw_usermem << std::endl;


	r << "epoch" << ":\t" << caps._hw_epoch << std::endl;
	r << "floatingpoint" << ":\t" << caps._hw_floatingpoint << std::endl;
	r << "machinearch" << ":\t" << caps._hw_machinearch << std::endl;

	r << "vectorunit" << ":\t" << caps._hw_vectorunit << std::endl;
	r << "tbfrequency" << ":\t" << caps._hw_tbfrequency << std::endl;
	r << "availcpu" << ":\t" << caps._hw_availcpu << std::endl;


	r << "cpu_type" << ":\t" << caps._hw_cpu_type << std::endl;
	r << "cpu_type_subtype" << ":\t" << caps._hw_cpu_type_subtype << std::endl;

	r << "packaged" << ":\t" << caps._hw_packaged << std::endl;

	r << "physical_processor_count" << ":\t" << caps._hw_physical_processor_count << std::endl;


	r << "logical_processor_count" << ":\t" << caps._hw_logical_processor_count << std::endl;


	r << "cpu_freq_hz" << ":\t" << caps._hw_cpu_freq_hz << std::endl;
	r << "bus_freq_hz" << ":\t" << caps._hw_bus_freq_hz << std::endl;

	r << "mem_size" << ":\t" << caps._hw_mem_size << std::endl;
	r << "page_size" << ":\t" << caps._hw_page_size << std::endl;
	r << "cacheline_size" << ":\t" << caps._hw_cacheline_size << std::endl;


	r << "scalar_align" << ":\t" << caps._hw_scalar_align << std::endl;

	r << "l1_data_cache_size" << ":\t" << caps._hw_l1_data_cache_size << std::endl;
	r << "l1_instruction_cache_size" << ":\t" << caps._hw_l1_instruction_cache_size << std::endl;
	r << "l2_cache_size" << ":\t" << caps._hw_l2_cache_size << std::endl;
	r << "l3_cache_size" << ":\t" << caps._hw_l3_cache_size << std::endl;
	return r.str();
}

void trace_hardware_caps(const hardware_caps_t& caps){
#if QUARK_TRACE_ON
	QUARK_SCOPED_TRACE("Hardware caps");
	const auto s = get_hardware_caps_string(caps);
	QUARK_TRACE(s);
#endif
}
