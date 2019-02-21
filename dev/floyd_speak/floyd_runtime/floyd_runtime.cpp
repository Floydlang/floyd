//
//  floyd_runtime.cpp
//  floyd_speak
//
//  Created by Marcus Zetterquist on 2019-02-17.
//  Copyright © 2019 Marcus Zetterquist. All rights reserved.
//

#include "floyd_runtime.h"


#include "quark.h"


#include <celero/Celero.h>

#include <random>

#ifndef WIN32
#include <cmath>
#include <cstdlib>
#endif

#if 1
///
/// This is the main(int argc, char** argv) for the entire celero program.
/// You can write your own, or use this macro to insert the standard one into the project.
///
//CELERO_MAIN

std::random_device g_random_device;
std::uniform_int_distribution<int> UniformDistribution(0, 1024);

///
/// In reality, all of the "Complex" cases take the same amount of time to run.
/// The difference in the results is a product of measurement error.
///
/// Interestingly, taking the sin of a constant number here resulted in a
/// great deal of optimization in clang and gcc.
///
BASELINE(DemoSimple, Baseline, 10, 1000000)
{
    celero::DoNotOptimizeAway(static_cast<float>(sin(UniformDistribution(g_random_device))));
}

///
/// Run a test consisting of 1 sample of 710000 operations per measurement.
/// There are not enough samples here to likely get a meaningful result.
///
BENCHMARK(DemoSimple, Complex1, 1, 710000)
{
    celero::DoNotOptimizeAway(static_cast<float>(sin(fmod(UniformDistribution(g_random_device), 3.14159265))));
}

///
/// Run a test consisting of 30 samples of 710000 operations per measurement.
/// There are not enough samples here to get a reasonable measurement
/// It should get a Basline number lower than the previous test.
///
BENCHMARK(DemoSimple, Complex2, 30, 710000)
{
    celero::DoNotOptimizeAway(static_cast<float>(sin(fmod(UniformDistribution(g_random_device), 3.14159265))));
}

///
/// Run a test consisting of 60 samples of 710000 operations per measurement.
/// There are not enough samples here to get a reasonable measurement
/// It should get a Basline number lower than the previous test.
///
BENCHMARK(DemoSimple, Complex3, 60, 710000)
{
    celero::DoNotOptimizeAway(static_cast<float>(sin(fmod(UniformDistribution(g_random_device), 3.14159265))));
}

#endif



////////////////////////////////	PARTICLE TEST

#if 0

static int randSeed = 10;
inline float randF()
{
	return 0.01f * (float)((randSeed++) % 100);
}

static const size_t UPDATES = 1000;
static const float DELTA_TIME = 1.0f / 60.0f;

template <int BufSize = 1>
class TParticle
{
private:
	float pos[4];
	float acc[4];
	float vel[4];
	float col[4];
	float rot;
	float time;
	float buf[BufSize];

public:
	void generate()
	{
		acc[0] = randF();
		acc[1] = randF();
		acc[2] = randF();
		acc[3] = randF();
		pos[0] = pos[1] = pos[2] = pos[3] = 0.0f;
		vel[0] = randF();
		vel[1] = randF();
		vel[2] = randF();
		vel[3] = vel[1] + vel[2];
		rot = 0.0f;
		time = 2.0f + randF();
	}

	void update(float dt)
	{
		vel[0] += acc[0] * dt;
		vel[1] += acc[1] * dt;
		vel[2] += acc[2] * dt;
		vel[3] += acc[3] * dt;
		pos[0] += vel[0] * dt;
		pos[1] += vel[1] * dt;
		pos[2] += vel[2] * dt;
		pos[3] += vel[3] * dt;
		col[0] = pos[0] * 0.001f;
		col[1] = pos[1] * 0.001f;
		col[2] = pos[2] * 0.001f;
		col[3] = pos[3] * 0.001f;
		rot += vel[3] * dt;
		time -= dt;

		for(int i = 0; i < BufSize; ++i)
		{
			buf[i] = vel[i % 4] + vel[i % 4] + vel[2] + pos[0] + pos[1] + pos[i % 4];
		}

		if(time < 0.0f)
		{
			generate();
		}
	}
};

using Particle = TParticle<1>;	 // sizeof 19*float = 76bytes
using Particle160 = TParticle<22>; // sizeof (18 + 22)*float = 160 bytes


class ParticlesFixture : public celero::TestFixture
{
public:
	virtual std::vector<celero::TestFixture::ExperimentValue> getExperimentValues() const override
	{
		std::vector<celero::TestFixture::ExperimentValue> problemSpace;
		const auto totalNumberOfTests = 10;

		for(auto i = 0; i < totalNumberOfTests; i++)
		{
			// ExperimentValues is part of the base class and allows us to specify
			// some values to control various test runs to end up building a nice graph.
			problemSpace.push_back({1000 + i * 1000, static_cast<int64_t>(totalNumberOfTests - i)});
		}

		return problemSpace;
	}
};

class ParticlesObjVectorFixture : public ParticlesFixture
{
public:
	/// Before each run, build a vector of random integers.
	virtual void setUp(const celero::TestFixture::ExperimentValue& x) override
	{
		this->particles = std::vector<Particle>(x.Value);

		for(auto& p : this->particles)
		{
			p.generate();
		}
	}

	/// After each run, clear the vector
	virtual void tearDown() override
	{
		this->particles.clear();
	}

	std::vector<Particle> particles;
};

static const int SamplesCount = 100;
static const int IterationsCount = 1;

// Run an automatic baseline.
// Celero will help make sure enough samples are taken to get a reasonable measurement
BASELINE_F(ParticlesTest, ObjVector, ParticlesObjVectorFixture, SamplesCount, IterationsCount)
{
	for(size_t u = 0; u < UPDATES; ++u)
	{
		for(auto& p : particles)
		{
			p.update(DELTA_TIME);
		}
	}
}

class ParticlesPtrVectorFixture : public ParticlesFixture
{
public:
	virtual bool randomizeAddresses()
	{
		return true;
	}

	/// Before each run, build a vector of random integers.
	virtual void setUp(const celero::TestFixture::ExperimentValue& experimentValue) override
	{
		this->particles = std::vector<std::shared_ptr<Particle>>(experimentValue.Value);

		for(auto& p : this->particles)
		{
			p = std::make_shared<Particle>();
		}

		if(this->randomizeAddresses() == true)
		{
			for(int64_t i = 0; i < experimentValue.Value / 2; ++i)
			{
				const auto a = celero::Random() % experimentValue.Value;
				const auto b = celero::Random() % experimentValue.Value;

				if(a != b)
				{
					std::swap(this->particles[a], this->particles[b]);
				}
			}
		}

		for(auto& p : this->particles)
		{
			p->generate();
		}
	}

	/// After each run, clear the vector
	virtual void tearDown() override
	{
		this->particles.clear();
	}

	std::vector<std::shared_ptr<Particle>> particles;
};

BENCHMARK_F(ParticlesTest, PtrVector, ParticlesPtrVectorFixture, SamplesCount, IterationsCount)
{
	for(size_t u = 0; u < UPDATES; ++u)
	{
		for(auto& p : this->particles)
		{
			p->update(DELTA_TIME);
		}
	}
}

class ParticlesPtrVectorNoRandFixture : public ParticlesPtrVectorFixture
{
public:
	virtual bool randomizeAddresses()
	{
		return false;
	}
};

BENCHMARK_F(ParticlesTest, PtrVectorNoRand, ParticlesPtrVectorNoRandFixture, SamplesCount, IterationsCount)
{
	for(size_t u = 0; u < UPDATES; ++u)
	{
		for(auto& p : this->particles)
		{
			p->update(DELTA_TIME);
		}
	}
}

#endif




////////////////////////////////	VECTOR TEST





std::vector<int32_t> make_test_elements(int64_t count){
	std::random_device random_device;
	std::uniform_int_distribution<int> d(0, 1024);

	std::vector<int32_t> result;
	result.reserve(count);

	for(auto i = 0 ; i < count ; i++){
		int value = d(random_device);
		result.push_back(value);
	}
	return result;
}



static const int SamplesCount = 100;
static const int IterationsCount = 1;

std::vector<celero::TestFixture::ExperimentValue> getExperimentValues(){
	std::vector<celero::TestFixture::ExperimentValue> problemSpace;
	const auto totalNumberOfTests = 10;

	for(auto i = 0; i < totalNumberOfTests; i++)
	{
		// ExperimentValues is part of the base class and allows us to specify
		// some values to control various test runs to end up building a nice graph.
		problemSpace.push_back({1000 + i * 1000, static_cast<int64_t>(totalNumberOfTests - i)});
	}

	return problemSpace;
}

	class Fix1 : public celero::TestFixture {
		public: std::vector<celero::TestFixture::ExperimentValue> getExperimentValues() const override {
			return ::getExperimentValues();
		}

		public: void setUp(const celero::TestFixture::ExperimentValue& x) override {
			elements = make_test_elements(x.Value);
		}
		void tearDown() override {
			std::vector<int32_t>().swap(elements);
		}

		std::vector<int32_t> elements;
	};

// Run an automatic baseline.
BASELINE_F(vector_int32_read_test, std_vector_int, Fix1, SamplesCount, IterationsCount){
	int32_t sum = 0;
	for(int i = 0 ; i < SamplesCount ; i++){
		const auto e = this->elements[i];
		sum = sum + e;
	}
}


////////////////////////////////	REFERENCE IMPLEMENTATION


std::vector<uint8_t> WriteVarLen(unsigned long value)
{
   std::vector<uint8_t> result;
   unsigned long buffer;
   buffer = value & 0x7F;

   while ( (value >>= 7) )
   {
     buffer <<= 8;
     buffer |= ((value & 0x7F) | 0x80);
   }

   while (true)
   {
      result.push_back(buffer & 0xff);
//      putc(buffer, outfile);
      if (buffer & 0x80)
          buffer >>= 8;
      else
          break;
   }
   return result;
}

uint8_t getc2(uint8_t data[], uint32_t& io_pos){
	const auto result = data[io_pos];
	io_pos++;
	return result;
}

unsigned long ReadVarLen(uint8_t data[], uint32_t pos)
{
    unsigned long value;
    unsigned char c;

    if ( (value = getc2(data, pos)) & 0x80 )
    {
       value &= 0x7F;
       do
       {
         value = (value << 7) + ((c = getc2(data, pos)) & 0x7F);
       } while (c & 0x80);
    }

    return(value);
}



const std::vector<std::pair<uint32_t, std::vector<uint8_t>>> test_data = {
	{ 0x00000000, { 0x00 } },
	{ 0x00000040, { 0x40 } },
	{ 0x0000007F, { 0x7F } },

	{ 0x00000080, { 0x81, 0x00 } },
	{ 0x00002000, { 0xC0, 0x00 } },
	{ 0x00003FFF, { 0xFF, 0x7F } },

	{ 0x00004000, { 0x81, 0x80, 0x00 } },
	{ 0x00100000, { 0xC0, 0x80, 0x00 } },
	{ 0x001FFFFF, { 0xFF, 0xFF, 0x7F } },

	{ 0x00200000, { 0x81, 0x80, 0x80, 0x00 } },
	{ 0x08000000, { 0xC0, 0x80, 0x80, 0x00 } },
	{ 0x0FFFFFFF, { 0xFF, 0xFF, 0xFF, 0x7F } },

	{ 0x1FFFFFFF, { 0x10, 0xFF, 0xFF, 0xFF, 0x7F } },
	{ 0x1FFFFFFF, { 0x10, 0xFF, 0xFF, 0xFF, 0x7F } }
};


/*
		BIT CACHE LINE
		|<-0---- 1------- 2------- 3------- 4------- 5------- 6------- 7----->|
		EEEEDDDD DDDCCCCC CCBBBBBB BAAAAAAA
*/

//http://midi.teragonaudio.com/tech/midifile/vari.htm
std::vector<uint8_t> pack7bit(uint32_t v){
	const uint64_t a = (v & 0b00000000'00000000'00000000'01111111) >> 0;
	const uint64_t b = (v & 0b00000000'00000000'00111111'10000000) >> 7;
	const uint64_t c = (v & 0b00000000'00011111'11000000'00000000) >> 14;
	const uint64_t d = (v & 0b00001111'11100000'00000000'00000000) >> 21;
	const uint64_t e = (v & 0b11110000'00000000'00000000'00000000) >> 28;

	//	This is 5 * 8 = 40 bits.
	uint64_t r = (e << 32) | (d << 24) | (c << 16) | (b << 8) | (a << 0);

/*
		BIT CACHE LINE
		|<-0---- 1------- 2------- 3------- 4------- 5------- 6------- 7----->|
		1000EEEE 1DDDDDDD 1CCCCCCC 1BBBBBBB 1AAAAAAA
*/
	if(r < (1 << 8)){
		return { uint8_t(a & 0xff) };
	}
	else if(r < (1 << 16)){
		return { uint8_t((b & 0xff) | 0x80), uint8_t(a & 0xff) };
	}
	else if(r < (1 << 24)){
		return { uint8_t((c & 0xff) | 0x80), uint8_t((b & 0xff) | 0x80), uint8_t(a & 0xff) };
	}
	else if(r < (1L << 32)){
		return { uint8_t((d & 0xff) | 0x80), uint8_t((c & 0xff) | 0x80), uint8_t((b & 0xff) | 0x80), uint8_t(a & 0xff) };
	}
	else{
		return { uint8_t((e & 0xff) | 0x80), uint8_t((d & 0xff) | 0x80), uint8_t((c & 0xff) | 0x80), uint8_t((b & 0xff) | 0x80), uint8_t(a & 0xff) };
	}
}

//	Wrapper that controls every result against reference implementation.
std::vector<uint8_t> pack7bit__verified(uint32_t v){
	const auto a = pack7bit(v);
	const auto b = WriteVarLen(v);

	QUARK_ASSERT(a == b);
	return a;
}

QUARK_UNIT_TEST_VIP("", "pack7bit()", "", ""){
	std::vector<uint32_t> test_values;

	test_values.push_back(0);
	test_values.push_back(UINT_MAX);
	test_values.push_back(0b00000000'00000000'00000000'00000000);
	test_values.push_back(0b10101010'10101010'10101010'10101010);
	test_values.push_back(0b01010101'01010101'01010101'01010101);
	test_values.push_back(0b11111111'00000000'00000000'00000000);
	test_values.push_back(0b00000000'11111111'00000000'00000000);
	test_values.push_back(0b00000000'00000000'11111111'00000000);
	test_values.push_back(0b00000000'00000000'00000000'11111111);

	for(uint32_t v = 1 ; v < 32 ; v++){
		test_values.push_back(1 << v);
		test_values.push_back((1 << v) - 1);
		test_values.push_back((1 << v) + 1);
	}

	for(const auto e: test_values){
		pack7bit__verified(e);
	}
}

QUARK_UNIT_TEST_VIP("", "", "", ""){
	QUARK_UT_VERIFY(pack7bit__verified(test_data[0].first) == test_data[0].second);
	QUARK_UT_VERIFY(pack7bit__verified(test_data[1].first) == test_data[1].second);
	QUARK_UT_VERIFY(pack7bit__verified(test_data[2].first) == test_data[2].second);

	QUARK_UT_VERIFY(pack7bit__verified(test_data[3].first) == test_data[3].second);
	QUARK_UT_VERIFY(pack7bit__verified(test_data[4].first) == test_data[4].second);
	QUARK_UT_VERIFY(pack7bit__verified(test_data[5].first) == test_data[5].second);

	QUARK_UT_VERIFY(pack7bit__verified(test_data[6].first) == test_data[6].second);
	QUARK_UT_VERIFY(pack7bit__verified(test_data[7].first) == test_data[7].second);
	QUARK_UT_VERIFY(pack7bit__verified(test_data[8].first) == test_data[8].second);

	QUARK_UT_VERIFY(pack7bit__verified(test_data[9].first) == test_data[9].second);
	QUARK_UT_VERIFY(pack7bit__verified(test_data[10].first) == test_data[10].second);
	QUARK_UT_VERIFY(pack7bit__verified(test_data[11].first) == test_data[11].second);
}



	class Fix2 : public celero::TestFixture {
		public: std::vector<celero::TestFixture::ExperimentValue> getExperimentValues() const override {
			return ::getExperimentValues();
		}

		public: void setUp(const celero::TestFixture::ExperimentValue& x) override {
			elements = make_test_elements(x.Value);
		}
		void tearDown() override {
			std::vector<int32_t>().swap(elements);
		}

		std::vector<int32_t> elements;
	};

#if 0
BENCHMARK_F(ParticlesTest, PtrVector, ParticlesPtrVectorFixture, SamplesCount, IterationsCount)
{
	for(size_t u = 0; u < UPDATES; ++u)
	{
		for(auto& p : this->particles)
		{
			p->update(DELTA_TIME);
		}
	}
}

class ParticlesPtrVectorNoRandFixture : public ParticlesPtrVectorFixture
{
public:
	virtual bool randomizeAddresses()
	{
		return false;
	}
};

BENCHMARK_F(ParticlesTest, PtrVectorNoRand, ParticlesPtrVectorNoRandFixture, SamplesCount, IterationsCount)
{
	for(size_t u = 0; u < UPDATES; ++u)
	{
		for(auto& p : this->particles)
		{
			p->update(DELTA_TIME);
		}
	}
}
#endif

