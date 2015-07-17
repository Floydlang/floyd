//
//  Experiments.cpp
//  Floyd
//
//  Created by Marcus Zetterquist on 16/07/15.
//  Copyright (c) 2015 Marcus Zetterquist. All rights reserved.
//

#include "cpp_extension.h"
#include "Experiments.h"

#include "FloydType.h"

namespace {

	struct TMyComposite {
		float oscillator;
	};

	struct ABC123 {
		TMyComposite c;
		float count;
	};
	ABC123 MockupOptimizedCPP(const float a, const float b, const TMyComposite& c){
		const float t = a * b + c.oscillator;

		TMyComposite c2 = c;
		c2.oscillator = t;

		ABC123 r;
		r.c = c2;
		r.count = a * b * 0.01f;
		return r;
		
	}

}




#include <iostream>
#include <thread>


namespace TestThreading {

	namespace {
		static const int num_threads = 10;

		//This function will be called from a thread

		void call_from_thread(int tid) {
			std::cout << "Launched by thread " << tid << std::endl;
		}

		void Test() {
			std::thread t[num_threads];

			//Launch a group of threads
			for (int i = 0; i < num_threads; ++i) {
				t[i] = std::thread(call_from_thread, i);
			}

			std::cout << "Launched from the main\n";

			//Join the threads with the main thread
			for (int i = 0; i < num_threads; ++i) {
				t[i].join();
			}
		}
	}
}


UNIT_TEST("Experiments", "TestExperiments()", "", ""){
	TestThreading::Test();
}
