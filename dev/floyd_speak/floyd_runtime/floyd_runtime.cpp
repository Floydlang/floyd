//
//  floyd_runtime.cpp
//  floyd_speak
//
//  Created by Marcus Zetterquist on 2019-02-17.
//  Copyright © 2019 Marcus Zetterquist. All rights reserved.
//

#include "floyd_runtime.h"


#include "quark.h"

#if 0
//#include <celero/Celero.h>

#include <random>

#ifndef WIN32
#include <cmath>
#include <cstdlib>
#endif


//#define LOCAL_TRACE_SS QUARK_TRACE_SS
#define LOCAL_TRACE_SS(x)



#if 0
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
	LOCAL_TRACE_SS(std::endl << "•••• make_test_elements(): const" << count);

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




static const int SamplesCount = 8;
static const int IterationsCount = 3'000'000;

//	Chosen because my L3 caches is 8 MB.
static const int k_element_count = 400'000;

const auto totalNumberOfTests = 16;

std::vector<celero::TestFixture::ExperimentValue> getExperimentValues(){
	std::vector<celero::TestFixture::ExperimentValue> problemSpace;
	for(auto i = 0; i < totalNumberOfTests; i++){
		/// An arbitrary integer value which can be used or translated for use by the test fixture.
//		const int64_t value_for_fixture = k_element_count + i * k_element_count;
		const int64_t value_for_fixture = 1 << (8 + i);

		/// The number of iterations to do with this test value.  0 (default) indicates that the default number of iterations set up for the test
		/// case should be used.
		const int64_t iterations = static_cast<int64_t>(totalNumberOfTests - i);

		// ExperimentValues is part of the base class and allows us to specify
		// some values to control various test runs to end up building a nice graph.
		const auto e = celero::TestFixture::ExperimentValue{ value_for_fixture, iterations };
		problemSpace.push_back(e);

		LOCAL_TRACE_SS("getExperimentValues: " << i << " value_for_fixture: " << e.Value << " iterations: " << e.Iterations);
	}

	return problemSpace;
}

std::vector<int32_t> g_test_elements;


	class Fix1 : public celero::TestFixture {
		public: ~Fix1(){
			LOCAL_TRACE_SS("~Fix1()" << this->elements.size());
		}

		public: std::vector<celero::TestFixture::ExperimentValue> getExperimentValues() const override {
			return ::getExperimentValues();
		}

		public: void setUp(const celero::TestFixture::ExperimentValue& x) override {
			LOCAL_TRACE_SS("setUp() x.Value: " << x.Value << " this->elements.size(): " <<this->elements.size());

			if(g_test_elements.size() == x.Value){
				elements = g_test_elements;
			}
			else{
				g_test_elements = make_test_elements(x.Value);
				elements = g_test_elements;
			}
		}
		void tearDown() override {
			LOCAL_TRACE_SS("tearDown()" << this->elements.size());
//			std::vector<int32_t>().swap(elements);
		}

		std::vector<int32_t> elements;
	};


// Run an automatic baseline.
BASELINE_F(vector_int32_read_test, std_vector_int, Fix1, SamplesCount, IterationsCount){
	LOCAL_TRACE_SS(__FUNCTION__ << " element count: " << this->elements.size());

	int32_t sum = 0;
	for(int i = 0 ; i < this->elements.size() ; i++){
		const auto e = this->elements[i];
		sum = sum + e;
	}
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




#endif

