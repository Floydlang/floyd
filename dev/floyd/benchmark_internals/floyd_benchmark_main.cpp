//
//  main.cpp
//  megastruct
//
//  Created by Marcus Zetterquist on 2019-01-28.
//  Copyright © 2019 Marcus Zetterquist. All rights reserved.
//

#include "gtest/gtest.h"
#include "benchmark/benchmark.h"
#include "hardware_caps.h"

#include <iostream>
#include <vector>

#if 0
///////////////////////////		CSoundEngine

#define CAtomicFIFO std::vector
typedef long TSpeakerID;
struct CSoundEngine;

struct TRealTimeSpeaker {
	float fFractionPos;
	float fRadius;
	float fPitch;
	float fX;
	float fY;
	float fLevel;
	float fFilteredMic1Gain;
	float fFilteredMic2Gain;
	float fGainChangeK;
	bool fPlayFlag;
	bool fLoopFlag;

	const std::int16_t* fSampleData;
	long fChannels;
	std::uint32_t fFrameCount;
};

class CSpeakerRef {
	CSoundEngine* fSoundEngine;
	TSpeakerID fID;
};

class CSoundEngine {
	float fVolume;
	CAtomicFIFO<TSpeakerID> fFreeSpeakerIDs;
	std::list<TSpeakerID> fReferencedSpeakersIDs;

	CAtomicFIFO<TRealTimeSpeaker> fRealTimeSpeakers;

	int32_t fFlushCount;
	int32_t fRenderCount;

	float fMic1X;
	float fMic1Y;

	float fMic2X;
	float fMic2Y;
	long fMagic;
};

/*
	GOAL: Manage memory accesses. Move things together, pack data (use CPU instructions to save memory accesses)

	- Tag hot members = put them at top, in the same cache-line

	Store data in unused bits of floats
	We want to know exact ranges of integers so e can store only those bits.

	- Vector of pixels, compress non-lossy! Key frames.
	- Audio as vector<int16> -- compress with key frames. Skip silent sections,
	- ZIP.
*/

/*
	FloydNative__TRealTimeSpeaker

	Stores 1.33 speakers per CL.
	Checking playflag of 1024 speakers accessess 768 CLs. Worst case = 246 cycles * 768 = 188928 cycles for memory accesses
	Checking distance to 1024 speakers accesses 768 CLs

	CACHE LINE
	BYTE 0   1        2        3	    4        5        6        7        8        9        10       11       12       13       14       15
	|<------------------------------------------------------------------->| |<------------------------------------------------------------------->|
	-------- -------- -------- -------- -------- -------- -------- -------- -------- -------- -------- -------- -------- -------- -------- --------
	AAAAAAAA AAAAAAAA AAAAAAAA AAAAAAAA BBBBBBBB BBBBBBBB BBBBBBBB BBBBBBBB CCCCCCCC CCCCCCCC CCCCCCCC CCCCCCCC DDD00000 00000000 00000000 00000000
 
	16       17       18       19       20       21       22       23       24       25       26       27       28       29       30       31
	|<------------------------------------------------------------------->| |<------------------------------------------------------------------->|
	-------- -------- -------- -------- -------- -------- -------- -------- -------- -------- -------- -------- -------- -------- -------- --------
	EEEEEEEE EEEEEEEE EEEEEEEE EEEEEEEE FFFFFFFF FFFFFFFF FFFFFFFF FFFFFFFF GGGGGGGG GGGGGGGG GGGGGGGG GGGGGGGG HHHHHHHH HHHHHHHH HHHHHHHH HHHHHHHH

	32       33       34       35       36       37       38       39       40       41       42       43       44       45       46       47
	|<------------------------------------------------------------------->| |<------------------------------------------------------------------->|
	-------- -------- -------- -------- -------- -------- -------- -------- -------- -------- -------- -------- -------- -------- -------- --------
	IIIIIIII IIIIIIII IIIIIIII IIIIIIII JJJJJJJJ JJJJJJJJ JJJJJJJJ JJJJJJJJ KKKKKKKK KKKKKKKK KKKKKKKK KKKKKKKK LLLLLLLL LLLLLLLL LLLLLLLL LLLLLLLL

	48       49       50       51       52       53       54       55       56       57       58       59       60       61       62       63
	|<------------------------------------------------------------------->| |<------------------------------------------------------------------->|
	-------- -------- -------- -------- -------- -------- -------- -------- -------- -------- -------- -------- -------- -------- -------- --------
	00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000
*/
struct FloydNative__TRealTimeSpeaker {
	float fX;	//	HOT	A
	float fRadius;	//	HOT	B
	float fY;	//	HOT C

	//	D (3bits) HOT
		bool fPlayFlag;	//	HOT
		bool fLoopFlag;
		bool fChannels;

	float fFractionPos;	//	E
	float fPitch;	// F
	float fLevel;	// G
	float fFilteredMic1Gain;	//	H
	float fFilteredMic2Gain;	//	I
	float fGainChangeK;		//	J

	//	[int16_t] fSampleData;	//	K
	int16_t* fSampleData__ptr;	//	K
	int64_t fSampleData__count;	//	L

//	long fChannels;	//	1 or 2
};


//	Faster to have struct of arrays of structs!

//	NOTE: Reference & RC or deepcopy. No need for RC if copied.

struct FloydNative__CSoundEngine {
	float fVolume;
	CAtomicFIFO<TSpeakerID> fFreeSpeakerIDs;
	std::list<TSpeakerID> fReferencedSpeakersIDs;

	CAtomicFIFO<TRealTimeSpeaker> fRealTimeSpeakers;

	int32_t fFlushCount;
	int32_t fRenderCount;

	float fMic1X;
	float fMic1Y;

	float fMic2X;
	float fMic2Y;
	long fMagic;
};


//	??? There can be several memory layouts for [TRealTimeSpeaker] -- need to convert between them, generate function-instanced for them etc.


/*
	FloydNativeGenerated__TRealTimeSpeaker2__hot
	Stores ONE bit per speaker = 512 speakers per cache line
	Checking fPlayFlag of 1024 speakers accesses 2 CLs. Worst case = 246 cycles * 2 = 492 cycles for memory accesses.


	CACHE LINE
	BYTE 0   1        2        3	    4        5        6        7        8        9        10       11       12       13       14       15
	|<------------------------------------------------------------------->| |<------------------------------------------------------------------->|
	-------- -------- -------- -------- -------- -------- -------- -------- -------- -------- -------- -------- -------- -------- -------- --------
 
	16       17       18       19       20       21       22       23       24       25       26       27       28       29       30       31
	|<------------------------------------------------------------------->| |<------------------------------------------------------------------->|
	-------- -------- -------- -------- -------- -------- -------- -------- -------- -------- -------- -------- -------- -------- -------- --------

	32       33       34       35       36       37       38       39       40       41       42       43       44       45       46       47
	|<------------------------------------------------------------------->| |<------------------------------------------------------------------->|
	-------- -------- -------- -------- -------- -------- -------- -------- -------- -------- -------- -------- -------- -------- -------- --------

	48       49       50       51       52       53       54       55       56       57       58       59       60       61       62       63
	|<------------------------------------------------------------------->| |<------------------------------------------------------------------->|
	-------- -------- -------- -------- -------- -------- -------- -------- -------- -------- -------- -------- -------- -------- -------- --------
*/
struct FloydNativeGenerated__TRealTimeSpeaker2__hot {
	bool fPlayFlag;	//	HOT	D1
};

/*
	FloydNativeGenerated__TRealTimeSpeaker2__roomtemp
	Stores 12 bytes per speaker = 5.333 speakers per cache line
	Checking distance to 1024 speakers accesses 192 CLs. IMPORTANT: Number will be smaller since we only check distance of ENABLED speakers.


	CACHE LINE
	BYTE 0   1        2        3	    4        5        6        7        8        9        10       11       12       13       14       15
	|<------------------------------------------------------------------->| |<------------------------------------------------------------------->|
	-------- -------- -------- -------- -------- -------- -------- -------- -------- -------- -------- -------- -------- -------- -------- --------
	AAAAAAAA AAAAAAAA AAAAAAAA AAAAAAAA BBBBBBBB BBBBBBBB BBBBBBBB BBBBBBBB CCCCCCCC CCCCCCCC CCCCCCCC CCCCCCCC -------- -------- -------- --------
 
	16       17       18       19       20       21       22       23       24       25       26       27       28       29       30       31
	|<------------------------------------------------------------------->| |<------------------------------------------------------------------->|
	-------- -------- -------- -------- -------- -------- -------- -------- -------- -------- -------- -------- -------- -------- -------- --------

	32       33       34       35       36       37       38       39       40       41       42       43       44       45       46       47
	|<------------------------------------------------------------------->| |<------------------------------------------------------------------->|
	-------- -------- -------- -------- -------- -------- -------- -------- -------- -------- -------- -------- -------- -------- -------- --------

	48       49       50       51       52       53       54       55       56       57       58       59       60       61       62       63
	|<------------------------------------------------------------------->| |<------------------------------------------------------------------->|
	-------- -------- -------- -------- -------- -------- -------- -------- -------- -------- -------- -------- -------- -------- -------- --------
*/
struct FloydNativeGenerated__TRealTimeSpeaker2__roomtemp {
	float fX;	//	HOT	A
	float fRadius;	//	HOT	B
	float fY;	//	HOT C
};

struct FloydNativeGenerated__TRealTimeSpeaker2__cold {
	bool fLoopFlag;	//	D2
	bool fChannels;	//	D3

	float fFractionPos;	//	E
	float fPitch;	// F
	float fLevel;	// G
	float fFilteredMic1Gain;	//	H
	float fFilteredMic2Gain;	//	I
	float fGainChangeK;		//	J

	//	[int16_t] fSampleData;	//	K
	int16_t* fSampleData__ptr;	//	K
	int64_t fSampleData__count;	//	L
};


//	Faster to have struct of arrays of structs!

//	NOTE: Reference & RC or deepcopy. No need for RC if copied.

struct FloydNative__CSoundEngine2 {
	float fVolume;
	CAtomicFIFO<TSpeakerID> fFreeSpeakerIDs;
	std::list<TSpeakerID> fReferencedSpeakersIDs;

	CAtomicFIFO<TRealTimeSpeaker> fRealTimeSpeakers;

	int32_t fFlushCount;
	int32_t fRenderCount;

	float fMic1X;
	float fMic1Y;

	float fMic2X;
	float fMic2Y;
	long fMagic;
};
#endif





/*
	Generate special create/delete/copy for each struct. It deeply bumps reference counters - no nesting. Flat number of RC bumps.
*/

/*
Cache line latency:

https://stackoverflow.com/questions/3928995/how-do-cache-lines-work


What every programmer should know about memory, Part 1
https://lwn.net/Articles/250967/
*/


/*
	CACHE LINE
	BYTE 0   1        2        3	    4        5        6        7        8        9        10       11       12       13       14       15
	|<------------------------------------------------------------------->| |<------------------------------------------------------------------->|
	-------- -------- -------- -------- -------- -------- -------- -------- -------- -------- -------- -------- -------- -------- -------- --------
	AAAAAAAA AAAAAAAA AAAAAAAA AAAAAAAA BBBBBBBB BBBBBBBB BBBBBBBB BBBBBBBB CCCCCCCC CCCCCCCC CCCCCCCC CCCCCCCC DDD00000 00000000 00000000 00000000
 
	16       17       18       19       20       21       22       23       24       25       26       27       28       29       30       31
	|<------------------------------------------------------------------->| |<------------------------------------------------------------------->|
	-------- -------- -------- -------- -------- -------- -------- -------- -------- -------- -------- -------- -------- -------- -------- --------
	EEEEEEEE EEEEEEEE EEEEEEEE EEEEEEEE FFFFFFFF FFFFFFFF FFFFFFFF FFFFFFFF GGGGGGGG GGGGGGGG GGGGGGGG GGGGGGGG HHHHHHHH HHHHHHHH HHHHHHHH HHHHHHHH

	32       33       34       35       36       37       38       39       40       41       42       43       44       45       46       47
	|<------------------------------------------------------------------->| |<------------------------------------------------------------------->|
	-------- -------- -------- -------- -------- -------- -------- -------- -------- -------- -------- -------- -------- -------- -------- --------
	IIIIIIII IIIIIIII IIIIIIII IIIIIIII JJJJJJJJ JJJJJJJJ JJJJJJJJ JJJJJJJJ KKKKKKKK KKKKKKKK KKKKKKKK KKKKKKKK LLLLLLLL LLLLLLLL LLLLLLLL LLLLLLLL

	48       49       50       51       52       53       54       55       56       57       58       59       60       61       62       63
	|<------------------------------------------------------------------->| |<------------------------------------------------------------------->|
	-------- -------- -------- -------- -------- -------- -------- -------- -------- -------- -------- -------- -------- -------- -------- --------
	00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000
*/

/*
	CACHE LINE

	BYTE 0   1        2        3	    4        5        6        7
	|<------------------------------------------------------------------->|
	-------- -------- -------- -------- -------- -------- -------- --------
	00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000

	8        9        10       11       12       13       14       15
	|<------------------------------------------------------------------->|
	-------- -------- -------- -------- -------- -------- -------- --------
	00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000

	16       17       18       19       20       21       22       23
	|<------------------------------------------------------------------->|
	-------- -------- -------- -------- -------- -------- -------- --------
	00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000

	24       25       26       27       28       29       30       31
	|<------------------------------------------------------------------->|
	-------- -------- -------- -------- -------- -------- -------- --------
	00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000

	32       33       34       35       36       37       38       39
	|<------------------------------------------------------------------->|
	-------- -------- -------- -------- -------- -------- -------- --------
	00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000

	40       41       42       43       44       45       46       47
	|<------------------------------------------------------------------->|
	-------- -------- -------- -------- -------- -------- -------- --------
	00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000

	48       49       50       51       52       53       54       55
	|<------------------------------------------------------------------->|
	-------- -------- -------- -------- -------- -------- -------- --------
	00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000

	56       57       58       59       60       61       62       63
	|<------------------------------------------------------------------->|
	-------- -------- -------- -------- -------- -------- -------- --------
	00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000
*/



////////////////////////////////		BENCHMARK -- GOOGLE EXAMPLES

#if 0

int Factorial(int n)
{
    if (n < 0 ) {
        return 0;
    }
    return !n ? 1 : n * Factorial(n - 1);
}

//	IDEA: Only have load/store of entire cache lines.


// Tests factorial of 0.
TEST(FactorialTest, HandlesZeroInput) {
  EXPECT_EQ(Factorial(0), 1);
}

// Tests factorial of positive numbers.
TEST(FactorialTest, HandlesPositiveInput) {
  EXPECT_EQ(Factorial(1), 1);
  EXPECT_EQ(Factorial(2), 2);
  EXPECT_EQ(Factorial(3), 6);
  EXPECT_EQ(Factorial(8), 40320);
}


namespace {


double CalculatePi(int depth) {
  double pi = 0.0;
  for (int i = 0; i < depth; ++i) {
    double numerator = static_cast<double>(((i % 2) * 2) - 1);
    double denominator = static_cast<double>((2 * i) - 1);
    pi += numerator / denominator;
  }
  return (pi - 1.0) * 4;
}

std::set<int64_t> ConstructRandomSet(int64_t size) {
  std::set<int64_t> s;
  for (int i = 0; i < size; ++i) s.insert(s.end(), i);
  return s;
}


}  // end namespace

static void BM_Factorial(benchmark::State& state) {
  int fac_42 = 0;
  for (auto _ : state) fac_42 = Factorial(8);
  // Prevent compiler optimizations
  std::stringstream ss;
  ss << fac_42;
  state.SetLabel(ss.str());
}
BENCHMARK(BM_Factorial);
BENCHMARK(BM_Factorial)->UseRealTime();

static void BM_CalculatePiRange(benchmark::State& state) {
  double pi = 0.0;
  for (auto _ : state) pi = CalculatePi(static_cast<int>(state.range(0)));
  std::stringstream ss;
  ss << pi;
  state.SetLabel(ss.str());
}
BENCHMARK_RANGE(BM_CalculatePiRange, 1, 1024 * 1024);

static void BM_CalculatePi(benchmark::State& state) {
  static const int depth = 1024;
  for (auto _ : state) {
    benchmark::DoNotOptimize(CalculatePi(static_cast<int>(depth)));
  }
}
BENCHMARK(BM_CalculatePi)->Threads(8);
BENCHMARK(BM_CalculatePi)->ThreadRange(1, 32);
BENCHMARK(BM_CalculatePi)->ThreadPerCpu();

static void BM_SetInsert(benchmark::State& state) {
  std::set<int64_t> data;
  for (auto _ : state) {
    state.PauseTiming();
    data = ConstructRandomSet(state.range(0));
    state.ResumeTiming();
    for (int j = 0; j < state.range(1); ++j) data.insert(rand());
  }
  state.SetItemsProcessed(state.iterations() * state.range(1));
  state.SetBytesProcessed(state.iterations() * state.range(1) * sizeof(int));
}

// Test many inserts at once to reduce the total iterations needed. Otherwise, the slower,
// non-timed part of each iteration will make the benchmark take forever.
BENCHMARK(BM_SetInsert)->Ranges({{1 << 10, 8 << 10}, {128, 512}});
#endif


#if 0


template <typename Container,
          typename ValueType = typename Container::value_type>
static void BM_Sequential(benchmark::State& state) {
  ValueType v = 42;
  for (auto _ : state) {
    Container c;
    for (int64_t i = state.range(0); --i;) c.push_back(v);
  }
  const int64_t items_processed = state.iterations() * state.range(0);
  state.SetItemsProcessed(items_processed);
  state.SetBytesProcessed(items_processed * sizeof(v));
}
BENCHMARK_TEMPLATE2(BM_Sequential, std::vector<int>, int)
    ->Range(1 << 0, 1 << 10);
BENCHMARK_TEMPLATE(BM_Sequential, std::list<int>)->Range(1 << 0, 1 << 10);
// Test the variadic version of BENCHMARK_TEMPLATE in C++11 and beyond.
#ifdef BENCHMARK_HAS_CXX11
BENCHMARK_TEMPLATE(BM_Sequential, std::vector<int>, int)->Arg(512);
#endif



static void BM_StringCompare(benchmark::State& state) {
  size_t len = static_cast<size_t>(state.range(0));
  std::string s1(len, '-');
  std::string s2(len, '-');
  for (auto _ : state) benchmark::DoNotOptimize(s1.compare(s2));
}
BENCHMARK(BM_StringCompare)->Range(1, 1 << 20);



template <class... Args>
void BM_with_args(benchmark::State& state, Args&&...) {
  for (auto _ : state) {
  }
}
BENCHMARK_CAPTURE(BM_with_args, int_test, 42, 43, 44);
BENCHMARK_CAPTURE(BM_with_args, string_and_pair_test, std::string("abc"),
                  std::pair<int, double>(42, 3.8));



void BM_non_template_args(benchmark::State& state, int, double) {
  while(state.KeepRunning()) {}
}
BENCHMARK_CAPTURE(BM_non_template_args, basic_test, 0, 0);




static void BM_DenseThreadRanges(benchmark::State& st) {
  switch (st.range(0)) {
    case 1:
      assert(st.threads == 1 || st.threads == 2 || st.threads == 3);
      break;
    case 2:
      assert(st.threads == 1 || st.threads == 3 || st.threads == 4);
      break;
    case 3:
      assert(st.threads == 5 || st.threads == 8 || st.threads == 11 ||
             st.threads == 14);
      break;
    default:
      assert(false && "Invalid test case number");
  }
  while (st.KeepRunning()) {
  }
}
BENCHMARK(BM_DenseThreadRanges)->Arg(1)->DenseThreadRange(1, 3);
BENCHMARK(BM_DenseThreadRanges)->Arg(2)->DenseThreadRange(1, 4, 2);
BENCHMARK(BM_DenseThreadRanges)->Arg(3)->DenseThreadRange(5, 14, 3);

#endif








int main(int argc, char** argv) {
	const auto caps = read_hardware_caps();
	const auto caps_string = get_hardware_caps_string(caps);
	std::cout << caps_string;

	::benchmark::Initialize(&argc, argv);
	if (::benchmark::ReportUnrecognizedArguments(argc, argv)) return 1;
	::benchmark::RunSpecifiedBenchmarks();
}
