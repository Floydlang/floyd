//
//  ast_typeid.cpp
//  FloydSpeak
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
#include <cpuid.h>

#include <stdio.h>
#include <sys/types.h>
#include <sys/sysctl.h>

using namespace std;

#include "quark.h"


#include <mach/machine.h>



#ifdef __APPLE__

namespace floyd {


/*
Can't be used, they are compiler-time constants:
std::hardware_destructive_interference_size, std::hardware_constructive_interference_size

https://www.unix.com/man-page/osx/3/sysconf/

https://www.freebsd.org/cgi/man.cgi?query=sysctlbyname&apropos=0&sektion=0&manpath=FreeBSD+10.1-RELEASE&arch=default&format=html


Model Name:	iMac
Model Identifier:	iMac15,1
Processor Name:	Intel Core i7
Processor Speed:	4 GHz
Number of Processors:	1
Total Number of Cores:	4
L2 Cache (per Core):	256 KB
L3 Cache:	8 MB
Memory:	16 GB
Boot ROM Version:	IM151.0217.B00
SMC Version (system):	2.23f11
Serial Number (system):	C02NR292FY14
Hardware UUID:	6CC8669B-7ADB-583F-BEB7-251C0199F0EE


SEE sysctl.h

sysctl -n machdep.cpu.brand_string
Intel(R) Core(TM) i7-4790K CPU @ 4.00GHz

https://ark.intel.com/products/80807/Intel-Core-i7-4790K-Processor-8M-Cache-up-to-4_40-GHz
*/



uint64_t sysctlbyname_uint64(const std::string& key){
	uint64_t result = -1;
	size_t size = -1;
	int error = sysctlbyname(key.c_str(), &result, &size, NULL, 0);
	if(error != 0){
		quark::throw_exception();
	}
	QUARK_ASSERT(size == 8);
	return result;
}

uint64_t sysctlbyname_uint64_def(const std::string& key, uint64_t def){
	try{
		return sysctlbyname_uint64(key);
	}
	catch(...){
		return def;
	}
}

uint32_t sysctlbyname_uint32(const std::string& key){
	//	Reserve 8 bytes here.
	uint64_t result = -1;

	size_t size = -1;
	int error = sysctlbyname(key.c_str(), &result, &size, NULL, 0);
	if(error != 0){
		quark::throw_exception();
	}

	//	CTL_HW_NAMES sometimes lies and says this key is a CTLTYPE_INT, then returns 8 bytes.
	if(size == 8){
		const auto result2 = sysctlbyname_uint64(key);
		return static_cast<uint32_t>(result2);
	}
	else{
		QUARK_ASSERT(size == 4);
		return static_cast<uint32_t>(result);
	}
}

uint32_t sysctlbyname_uint32_def(const std::string& key, uint32_t def){
	try{
		return sysctlbyname_uint32(key);
	}
	catch(...){
		return def;
	}
}

std::string sysctlbyname_string(const std::string& key){
	std::vector<char> temp(200, 'x');
	size_t out_size = temp.size();
	int error = sysctlbyname(key.c_str(), &temp[0], &out_size, nullptr, 0);
	if(error != 0){
		quark::throw_exception();
	}

	const std::string result(temp.begin(), temp.begin() + out_size - 1);
	return result;
}

std::string sysctlbyname_string_def(const std::string& key, const std::string& def){
	try{
		return sysctlbyname_string(key);
	}
	catch(...){
		return def;
	}
}






#if 0
FUTURE: More caps about how many caches and how they are shared.

Below copied from Darwin code.

/* For each cache level (0 is memory) */
void test(){
	std::size_t size = 0;
	int i = 0;
	int memsize = 666;

	if (!sysctlbyname("hw.cacheconfig", NULL, &size, NULL, 0)) {
		unsigned n = size / sizeof(uint32_t);
		uint64_t cacheconfig[n];
		uint64_t cachesize[n];
		uint32_t cacheconfig32[n];

		if ((!sysctlbyname("hw.cacheconfig", cacheconfig, &size, NULL, 0))) {
			/* Yeech. Darwin seemingly has changed from 32bit to 64bit integers for
			 * cacheconfig, with apparently no way for detection. Assume the machine
			 * won't have more than 4 billion cpus */
			if (cacheconfig[0] > 0xFFFFFFFFUL) {
				memcpy(cacheconfig32, cacheconfig, size);
				for (i = 0 ; i < size / sizeof(uint32_t); i++)
					cacheconfig[i] = cacheconfig32[i];
			}

			memset(cachesize, 0, sizeof(uint64_t) * n);
			size = sizeof(uint64_t) * n;
			if (sysctlbyname("hw.cachesize", cachesize, &size, NULL, 0)) {
				if (n > 0)
					cachesize[0] = memsize;
				if (n > 1)
					cachesize[1] = l1dcachesize;
				if (n > 2)
					cachesize[2] = l2cachesize;
				if (n > 3)
					cachesize[3] = l3cachesize;
			}

			hwloc_debug("%s", "caches");
			for (i = 0; i < n && cacheconfig[i]; i++)
				hwloc_debug(" %"PRIu64"(%"PRIu64"kB)", cacheconfig[i], cachesize[i] / 1024);

			/* Now we know how many caches there are */
			n = i;
			hwloc_debug("\n%u cache levels\n", n - 1);

			/* For each cache level (0 is memory) */
			for (i = 0; i < n; i++) {

		if (hwloc_filter_check_keep_object_type(topology, obj->type))
			hwloc_insert_object_by_cpuset(topology, obj);
		else
			hwloc_free_unlinked_object(obj); /* FIXME: don't built at all, just build the cpuset in case l1i needs it */
				}
			}
		}
	}
}
#endif



hardware_caps_t read_hardware_caps(){
	return {
		._hw_machine = sysctlbyname_string_def("hw.machine", ""),
		._hw_model = sysctlbyname_string_def("hw.model", ""),
		._hw_ncpu = sysctlbyname_uint32_def("hw.ncpu", -1),

		._hw_byteorder = sysctlbyname_uint32_def("hw.byteorder", 0),
		._hw_physmem = sysctlbyname_uint32_def("hw.physmem", 0),
		._hw_usermem = sysctlbyname_uint32_def("hw.usermem", 0),

		._hw_epoch = sysctlbyname_uint32_def("hw.epoch", -1),
		._hw_floatingpoint = sysctlbyname_uint32_def("hw.floatingpoint", 0),

		._hw_machinearch = sysctlbyname_string_def("hw.machinearch", ""),

		._hw_vectorunit = sysctlbyname_uint32_def("hw.vectorunit", -1),
		._hw_tbfrequency = sysctlbyname_uint32_def("hw.tbfrequency", -1),
		._hw_availcpu = sysctlbyname_uint32_def("hw.availcpu", -1),


		._hw_cpu_type = sysctlbyname_uint32_def("hw.cputype", -1),

		._hw_cpu_type_subtype = sysctlbyname_uint32_def("hw.cpusubtype", -1),

		._hw_packaged = sysctlbyname_uint32_def("hw.packages", -1),

		._hw_physical_processor_count = sysctlbyname_uint32_def("hw.physicalcpu_max", -1),
		._hw_logical_processor_count = sysctlbyname_uint32_def("hw.logicalcpu_max", -1),

		._hw_cpu_freq_hz = sysctlbyname_uint64_def("hw.cpufrequency", -1),
		._hw_bus_freq_hz = sysctlbyname_uint64_def("hw.busfrequency", -1),

		._hw_mem_size = sysctlbyname_uint64_def("hw.memsize", -1),
		._hw_page_size = sysctlbyname_uint64_def("hw.pagesize", -1),
		._hw_cacheline_size = sysctlbyname_uint64_def("hw.cachelinesize", -1),
		._hw_scalar_align = alignof(std::max_align_t),

		._hw_l1_data_cache_size = sysctlbyname_uint64_def("hw.l1dcachesize", -1),
		._hw_l1_instruction_cache_size = sysctlbyname_uint64_def("hw.l1icachesize", -1),
		._hw_l2_cache_size = sysctlbyname_uint64_def("hw.l2cachesize", -1),
		._hw_l3_cache_size = sysctlbyname_uint64_def("hw.l3cachesize", -1)

	};
}

QUARK_UNIT_TEST("","", "", ""){
	const auto a = read_hardware_caps();
	QUARK_UT_VERIFY(a._hw_cacheline_size >= 16);
	QUARK_UT_VERIFY(a._hw_scalar_align >= 4);
}

void trace_hardware_caps(const hardware_caps_t& caps){
	QUARK_SCOPED_TRACE("Hardware caps");

	QUARK_TRACE_SS("machine" << ":\t" << caps._hw_machine);
	QUARK_TRACE_SS("model" << ":\t" << caps._hw_model);
	QUARK_TRACE_SS("ncpu" << ":\t" << caps._hw_ncpu);

	QUARK_TRACE_SS("byteorder" << ":\t" << caps._hw_byteorder);
	QUARK_TRACE_SS("physmem" << ":\t" << caps._hw_physmem);
	QUARK_TRACE_SS("usermem" << ":\t" << caps._hw_usermem);


	QUARK_TRACE_SS("epoch" << ":\t" << caps._hw_epoch);
	QUARK_TRACE_SS("floatingpoint" << ":\t" << caps._hw_floatingpoint);
	QUARK_TRACE_SS("machinearch" << ":\t" << caps._hw_machinearch);

	QUARK_TRACE_SS("vectorunit" << ":\t" << caps._hw_vectorunit);
	QUARK_TRACE_SS("tbfrequency" << ":\t" << caps._hw_tbfrequency);
	QUARK_TRACE_SS("availcpu" << ":\t" << caps._hw_availcpu);


	QUARK_TRACE_SS("cpu_type" << ":\t" << caps._hw_cpu_type);
	QUARK_TRACE_SS("cpu_type_subtype" << ":\t" << caps._hw_cpu_type_subtype);

	QUARK_TRACE_SS("packaged" << ":\t" << caps._hw_packaged);

	QUARK_TRACE_SS("physical_processor_count" << ":\t" << caps._hw_physical_processor_count);


	QUARK_TRACE_SS("logical_processor_count" << ":\t" << caps._hw_logical_processor_count);


	QUARK_TRACE_SS("cpu_freq_hz" << ":\t" << caps._hw_cpu_freq_hz);
	QUARK_TRACE_SS("bus_freq_hz" << ":\t" << caps._hw_bus_freq_hz);

	QUARK_TRACE_SS("mem_size" << ":\t" << caps._hw_mem_size);
	QUARK_TRACE_SS("page_size" << ":\t" << caps._hw_page_size);
	QUARK_TRACE_SS("cacheline_size" << ":\t" << caps._hw_cacheline_size);


	QUARK_TRACE_SS("scalar_align" << ":\t" << caps._hw_scalar_align);

	QUARK_TRACE_SS("l1_data_cache_size" << ":\t" << caps._hw_l1_data_cache_size);
	QUARK_TRACE_SS("l1_instruction_cache_size" << ":\t" << caps._hw_l1_instruction_cache_size);
	QUARK_TRACE_SS("l2_cache_size" << ":\t" << caps._hw_l2_cache_size);
	QUARK_TRACE_SS("l3_cache_size" << ":\t" << caps._hw_l3_cache_size);
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



#define SYSCTL_CORE_COUNT   "machdep.cpu.core_count"

typedef struct cpu_set {
  uint32_t    count;
} cpu_set_t;

static inline void CPU_ZERO(cpu_set_t *cs) { cs->count = 0; }

static inline void CPU_SET(int num, cpu_set_t *cs) { cs->count |= (1 << num); }

static inline int CPU_ISSET(int num, cpu_set_t *cs) { return (cs->count & (1 << num)); }

int sched_getaffinity(pid_t pid, size_t cpu_size, cpu_set_t *cpu_set){
  int32_t core_count = 0;
  size_t  len = sizeof(core_count);
  int ret = sysctlbyname(SYSCTL_CORE_COUNT, &core_count, &len, 0, 0);
  if (ret) {
    printf("error while get core count %d\n", ret);
    return -1;
  }
  cpu_set->count = 0;
  for (int i = 0; i < core_count; i++) {
    cpu_set->count |= (1 << i);
  }

  return 0;
}

/*
int pthread_setaffinity_np(pthread_t thread, size_t cpu_size,
                           cpu_set_t *cpu_set)
{
  thread_port_t mach_thread;
  int core = 0;

  for (core = 0; core < 8 * cpu_size; core++) {
    if (CPU_ISSET(core, cpu_set)) break;
  }
  printf("binding to core %d\n", core);
  thread_affinity_policy_data_t policy = { core };
  mach_thread = pthread_mach_thread_np(thread);
  thread_policy_set(mach_thread, THREAD_AFFINITY_POLICY,
                    (thread_policy_t)&policy, 1);
  return 0;
}
*/

}

#endif
