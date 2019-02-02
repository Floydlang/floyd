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

#ifdef __APPLE__

namespace floyd {


/*
	Can't be used, they are compiler-time constants.
	std::hardware_destructive_interference_size, std::hardware_constructive_interference_size

	https://www.unix.com/man-page/osx/3/sysconf/

	https://www.freebsd.org/cgi/man.cgi?query=sysctlbyname&apropos=0&sektion=0&manpath=FreeBSD+10.1-RELEASE&arch=default&format=html
*/



/*
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
*/

/*
sysctl -n machdep.cpu.brand_string
Intel(R) Core(TM) i7-4790K CPU @ 4.00GHz

https://ark.intel.com/products/80807/Intel-Core-i7-4790K-Processor-8M-Cache-up-to-4_40-GHz

*/


//int	sysctl(int *, u_int, void *, size_t *, void *, size_t);
//int	sysctlbyname(const char *, void *, size_t *, void *, size_t);
uint32_t sysctlbyname_uint32(const std::string& key){
	uint32_t result = -1;
	size_t size = -1;
	int error = sysctlbyname(key.c_str(), &result, &size, NULL, 0);
	if(error != 0){
		throw std::exception();
	}
	QUARK_ASSERT(size == 4);
	return result;
}
uint64_t sysctlbyname_uint64(const std::string& key){
	uint64_t result = -1;
	size_t size = -1;
	int error = sysctlbyname(key.c_str(), &result, &size, NULL, 0);
	if(error != 0){
		throw std::exception();
	}
	QUARK_ASSERT(size == 8);
	return result;
}


//	SEE sysctl.h

#include <mach/machine.h>

  
hardware_info_t read_hardware_info(){
	return {
		._cpu_type = sysctlbyname_uint32("hw.cputype"),
		._cpu_type_subtype = sysctlbyname_uint32("hw.cpusubtype"),

		._processor_packages = sysctlbyname_uint32("hw.packages"),

		._physical_processor_count = sysctlbyname_uint32("hw.physicalcpu_max"),
		._logical_processor_count = sysctlbyname_uint32("hw.logicalcpu_max"),

		._cpu_freq_hz = sysctlbyname_uint64("hw.cpufrequency"),
		._bus_freq_hz = sysctlbyname_uint64("hw.busfrequency"),

		._mem_size = sysctlbyname_uint64("hw.memsize"),
		._page_size = sysctlbyname_uint64("hw.pagesize"),
		._cacheline_size = sysctlbyname_uint64("hw.cachelinesize"),
		._scalar_align = alignof(std::max_align_t),

		._l1_data_cache_size = sysctlbyname_uint64("hw.l1dcachesize"),
		._l1_instruction_cache_size = sysctlbyname_uint64("hw.l1icachesize"),
		._l2_cache_size = sysctlbyname_uint64("hw.l2cachesize"),
		._l3_cache_size = sysctlbyname_uint64("hw.l3cachesize")

	};
}


QUARK_UNIT_TEST("","", "", ""){
	const auto a = read_hardware_info();
	QUARK_UT_VERIFY(a._cacheline_size >= 16);
	QUARK_UT_VERIFY(a._scalar_align >= 4);
}


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
	}


}


/*
unsigned int get_freq() {
	int mib[2];
	unsigned int freq;
	size_t len;

	mib[0] = CTL_HW;
	mib[1] = HW_CPU_FREQ;
	len = sizeof(freq);
	sysctl(mib, 2, &freq, &len, NULL, 0);

	printf("%u\n", freq);

	return freq;
}

QUARK_UNIT_TEST("","", "", ""){
	const auto a = get_freq();
	QUARK_UT_VERIFY(a > 0);
}
*/



#define SYSCTL_CORE_COUNT   "machdep.cpu.core_count"

typedef struct cpu_set {
  uint32_t    count;
} cpu_set_t;

static inline void
CPU_ZERO(cpu_set_t *cs) { cs->count = 0; }

static inline void
CPU_SET(int num, cpu_set_t *cs) { cs->count |= (1 << num); }

static inline int
CPU_ISSET(int num, cpu_set_t *cs) { return (cs->count & (1 << num)); }

int sched_getaffinity(pid_t pid, size_t cpu_size, cpu_set_t *cpu_set)
{
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
#endif

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

