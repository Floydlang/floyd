//
//  typeid.cpp
//  Floyd
//
//  Created by Marcus Zetterquist on 2018-02-12.
//  Copyright © 2018 Marcus Zetterquist. All rights reserved.
//

#include "hardware_caps.h"

#include <memory>
#include <cstddef>
#include <string>
#include <thread>
#include <cmath>
#include <fstream>
#include <iostream>
#include <sstream>

#include <cpuid.h>
#include <sys/types.h>
#include <sys/sysctl.h>
#include <unistd.h>
#include <stdio.h>

using namespace std;

#include "quark.h"




static hardware_caps_t read_caps_linux();




//	https://opensource.apple.com/source/xnu/xnu-792/libkern/libkern/sysctl.h

hardware_caps_t read_hardware_caps(){
	hardware_caps_t test = read_caps_linux();

	return {
		//._machdep_cpu_brand_string = sysctlbyname_string_def("machdep.cpu.brand_string", ""),

/*
machdep.cpu.features
machdep.cpu.leaf7_features
machdep.cpu.extfeatures
*/
/* 
		._hw_machine = sysctlbyname_string_def("hw.machine", ""),
		._hw_model = sysctlbyname_string_def("hw.model", ""),
		._hw_ncpu = sysctlbyname_uint32_def("hw.ncpu", -1),

		._hw_byteorder = sysctlbyname_uint32_def("hw.byteorder", 0),
		._hw_physmem = sysctlbyname_uint32_def("hw.physmem", 0),
		._hw_usermem = sysctlbyname_uint32_def("hw.usermem", 0),

		._hw_epoch = sysctlbyname_uint32_def("hw.epoch", -1),
		._hw_floatingpoint = sysctlbyname_uint32_def("hw.floatingpoint", 0),

		._hw_machinearch = sysctlbyname_string_def("hw.machinearch", ""),

		._hw_vectorunit = sysctlbyname_uint32_def("hw.vectorunit", 0),
		._hw_tbfrequency = sysctlbyname_uint32_def("hw.tbfrequency", 0),
		._hw_availcpu = sysctlbyname_uint32_def("hw.availcpu", 0),


		._hw_cpu_type = sysctlbyname_uint32_def("hw.cputype", 0),

		._hw_cpu_type_subtype = sysctlbyname_uint32_def("hw.cpusubtype", 0),

		._hw_packaged = sysctlbyname_uint32_def("hw.packages", 0),

		._hw_physical_processor_count = sysctlbyname_uint32_def("hw.physicalcpu_max", 0),
		._hw_logical_processor_count = sysctlbyname_uint32_def("hw.logicalcpu_max", 0),

		._hw_cpu_freq_hz = sysctlbyname_uint64_def("hw.cpufrequency", 0),
		._hw_bus_freq_hz = sysctlbyname_uint64_def("hw.busfrequency", 0),

		._hw_mem_size = sysctlbyname_uint64_def("hw.memsize", 0),
		._hw_page_size = sysctlbyname_uint64_def("hw.pagesize", 0),
		._hw_cacheline_size = sysctlbyname_uint64_def("hw.cachelinesize", 0),
		._hw_scalar_align = alignof(std::max_align_t),

		._hw_l1_data_cache_size = sysctlbyname_uint64_def("hw.l1dcachesize", 0),
		._hw_l1_instruction_cache_size = sysctlbyname_uint64_def("hw.l1icachesize", 0),
		._hw_l2_cache_size = sysctlbyname_uint64_def("hw.l2cachesize", 0),
		._hw_l3_cache_size = sysctlbyname_uint64_def("hw.l3cachesize", 0)
*/
	};
}

//	Experiement.
QUARK_UNIT_TEST("","", "", ""){
	unsigned num_cpus = std::thread::hardware_concurrency();

//	QUARK_TRACE_SS(num_cpus);

	//          std::cout << "Thread #" << i << ": on CPU " << sched_getcpu() << "\n";

	#define CPUID(INFO, LEAF, SUBLEAF) __cpuid_count(LEAF, SUBLEAF, INFO[0], INFO[1], INFO[2], INFO[3])

	#define GETCPU(CPU) {                              \
			uint32_t CPUInfo[4];                           \
			CPUID(CPUInfo, 1, 0);                          \
			/* CPUInfo[1] is EBX, bits 24-31 are APIC ID */ \
			if ( (CPUInfo[3] & (1 << 9)) == 0) {           \
			  CPU = -1;  /* no APIC on chip */             \
			}                                              \
			else {                                         \
			  CPU = (unsigned)CPUInfo[1] >> 24;                    \
			}                                              \
			if (CPU < 0) CPU = 0;                          \
		  }

		int cpu = -1;
		GETCPU(cpu);

//		QUARK_TRACE_SS(cpu);

	QUARK_UT_VERIFY(num_cpus > 0);
}



////////////////////////////////		AFFINITY


// https://en.wikipedia.org/wiki/CPUID#Calling_CPUID
int GetCacheSize(int cache_level)  
{
    // Intel stores it's cache information in eax4, with ecx as index
    // The information received is as following:
    // ebx[31:22] = Ways of associativity
    // ebx[21:12] = Physical line partitions
    // ebx[11: 0] = Line size
    unsigned int cpu_info[4];
    // Windows counterpart
    //__cpuidex(cpu_info, 4, cache_level); // Index 0 is L1 data cache

    cpu_info[0]=0;
    __get_cpuid (0,
		 &cpu_info[0],&cpu_info[1],&cpu_info[2],&cpu_info[3]);


    if (cpu_info[0]<4) {
      return 0;
    }
    

    // 
    cpu_info[0]=4;
    cpu_info[2]=cache_level;
    __get_cpuid (4,
		 &cpu_info[0],&cpu_info[1],&cpu_info[2],&cpu_info[3]);
    
    int ways = cpu_info[1] & 0xffc00000; // This receives bit 22 to 31 from the ebx register
    ways >>= 22; // Bitshift it 22 bits to get the real value, since we started reading from bit 22
    int partitions = cpu_info[1] & 0x3ff800; // This receives bit 12 to 21
    partitions >>= 12; // Same here, bitshift 12 bits
    int line_size = cpu_info[1] & 0x7ff; // This receives bit 0 to 11
    int sets = cpu_info[2]; // The sets are the value of the ecx register
    // All of these values needs one appended to them to get the real value
    return (ways + 1) * (partitions + 1) * (line_size + 1) * (sets + 1); // Calculate the cache size by multiplying the values
}


// Easier to use this
// /sys/devices/system/cpu/cpu0/cache/index0/size
static hardware_caps_t read_caps_linux()
{
	hardware_caps_t ret;
	//std::ifstream fileStat("/proc/cpuinfo");
	std::ifstream fileStat("/sys/devices/system/cpu/cpu0/cache/index0/size");


	std::string line;

	// Does not work properly :-(
	//ret._hw_cacheline_size = GetCacheSize(0);  
	//ret._hw_l2_cache_size = GetCacheSize(1);  
	//ret._hw_l3_cache_size = GetCacheSize(2);
	ret._hw_cacheline_size = 32000;   // "32K" 
	ret._hw_scalar_align = alignof(std::max_align_t);

#if 0	
	const std::string STR_CPU("cache size");
	const std::size_t LEN_STR_CPU = STR_CPU.size();
	const std::string STR_TOT("tot");
	while(std::getline(fileStat, line))
	{
		// cpu stats line found

	  /*
	r << "cacheline_size" << ":\t" << caps._hw_cacheline_size << std::endl;


	r << "scalar_align" << ":\t" << caps._hw_scalar_align << std::endl;

	r << "l1_data_cache_size" << ":\t" << caps._hw_l1_data_cache_size << std::endl;
	r << "l1_instruction_cache_size" << ":\t" << caps._hw_l1_instruction_cache_size << std::endl;
	r << "l2_cache_size" << ":\t" << caps._hw_l2_cache_size << std::endl;
	r << "l3_cache_size" << ":\t" << caps._hw_l3_cache_size << std::endl;
	  */
	  
	  /*
getconf -a | grep CACHE_SIZE

cat /proc/cpuinfo

processor	: 0
vendor_id	: GenuineIntel
cpu family	: 6
model		: 42
model name	: Intel(R) Core(TM) i7-2620M CPU @ 2.70GHz
stepping	: 7
microcode	: 0x1a
cpu MHz		: 1162.745
cache size	: 4096 KB
physical id	: 0
siblings	: 4
core id		: 0
cpu cores	: 2
apicid		: 0
initial apicid	: 0
fpu		: yes
fpu_exception	: yes
cpuid level	: 13
wp		: yes
flags		: fpu vme de pse tsc msr pae mce cx8 apic sep mtrr pge mca cmov pat pse36 clflush dts acpi mmx fxsr sse sse2 ss ht tm pbe syscall nx rdtscp lm constant_tsc arch_perfmon pebs bts nopl xtopology nonstop_tsc cpuid aperfmperf pni pclmulqdq dtes64 monitor ds_cpl vmx smx est tm2 ssse3 cx16 xtpr pdcm pcid sse4_1 sse4_2 x2apic popcnt tsc_deadline_timer aes xsave avx lahf_lm epb pti tpr_shadow vnmi flexpriority ept vpid xsaveopt dtherm ida arat pln pts
bugs		: cpu_meltdown spectre_v1 spectre_v2 spec_store_bypass l1tf mds swapgs
bogomips	: 5389.17
clflush size	: 64
cache_alignment	: 64
address sizes	: 36 bits physical, 48 bits virtual
power management:
	  */
		if(!line.compare(0, LEN_STR_CPU, STR_CPU))
		{
			std::istringstream ss(line);

			// store entry
			entries.emplace_back(CPUData());
			CPUData & entry = entries.back();

			// read cpu label
			ss >> entry.cpu;

			// remove "cpu" from the label when it's a processor number
			if(entry.cpu.size() > LEN_STR_CPU)
				entry.cpu.erase(0, LEN_STR_CPU);
			// replace "cpu" with "tot" when it's total values
			else
				entry.cpu = STR_TOT;

		}
#endif
	return ret;
}
