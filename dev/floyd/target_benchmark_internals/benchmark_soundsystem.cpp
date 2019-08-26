//
//  benchmark_soundsystem.cpp
//  benchmark
//
//  Created by Marcus Zetterquist on 2019-08-26.
//  Copyright © 2019 Marcus Zetterquist. All rights reserved.
//

#include "benchmark_soundsystem.h"

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

