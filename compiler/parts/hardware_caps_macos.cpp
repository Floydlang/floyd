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
//#include <thread>
#include <sstream>

//#include <sys/types.h>
//#include <cpuid.h>
#include <sys/sysctl.h>
#include <mach/machine.h>

#include "unix_helpers.h"
#include "quark.h"


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



static std::pair<bool, uint64_t> sysctlbyname_uint64(const std::string& key){
	uint64_t result = -1;
	size_t size = -1;
	int error = ::sysctlbyname(key.c_str(), &result, &size, NULL, 0);
	if(error != 0){
		const auto err = get_unix_err();
		return { false, -1 };
	}
	QUARK_ASSERT(size == 8);
	return { true, result };
}

static uint64_t sysctlbyname_uint64_def(const std::string& key, uint64_t def){
	const auto r = sysctlbyname_uint64(key);
	return r.first ? r.second : def;
}

static std::pair<bool, uint32_t> sysctlbyname_uint32(const std::string& key){
	//	Reserve 8 bytes here.
	uint64_t result64 = -1;
	size_t size = -1;
	const int error = ::sysctlbyname(key.c_str(), &result64, &size, NULL, 0);
	if(error != 0){
		const auto err = get_unix_err();
		return { false, -1 };
	}

	//	CTL_HW_NAMES sometimes lies and says this key is a CTLTYPE_INT, then returns 8 bytes.
	if(size == 8){
		const auto result2 = sysctlbyname_uint64(key);
		return { result2.first, static_cast<uint32_t>(result2.second) };
	}
	else if(size == 4){
		return { true, static_cast<uint32_t>(result64) };
	}
	else{
		quark::throw_defective_request();
	}
}

static uint32_t sysctlbyname_uint32_def(const std::string& key, uint32_t def){
	const auto r = sysctlbyname_uint32(key);
	return r.first ? r.second : def;
}

static std::pair<bool, std::string> sysctlbyname_string(const std::string& key){
	std::vector<char> temp(200, 'x');
	size_t out_size = temp.size();
	int error = ::sysctlbyname(key.c_str(), &temp[0], &out_size, nullptr, 0);
	if(error != 0){
		const auto err = get_unix_err();
		return { false, "*error*" };
	}

	const std::string result(temp.begin(), temp.begin() + out_size - 1);
	return { true, result };
}

static std::string sysctlbyname_string_def(const std::string& key, const std::string& def){
	const auto r = sysctlbyname_string(key);
	return r.first ? r.second : def;
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


//	https://opensource.apple.com/source/xnu/xnu-792/libkern/libkern/sysctl.h

hardware_caps_t read_hardware_caps(){
	return {
		._machdep_cpu_brand_string = sysctlbyname_string_def("machdep.cpu.brand_string", ""),

/*
machdep.cpu.features
machdep.cpu.leaf7_features
machdep.cpu.extfeatures
*/

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
	};
}

QUARK_TEST("hardware_caps", "read_hardware_caps", "", ""){
	const auto r = read_hardware_caps();
}



////////////////////////////////		AFFINITY


#define SYSCTL_CORE_COUNT   "machdep.cpu.core_count"

typedef struct cpu_set {
  uint32_t    count;
} cpu_set_t;

static inline void CPU_ZERO(cpu_set_t *cs) { cs->count = 0; }

static inline void CPU_SET(int num, cpu_set_t *cs) { cs->count |= (1 << num); }

static inline int CPU_ISSET(int num, cpu_set_t *cs) { return (cs->count & (1 << num)); }

static int sched_getaffinity(pid_t pid, size_t cpu_size, cpu_set_t *cpu_set){
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
