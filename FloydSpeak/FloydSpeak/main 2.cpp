/*
	Copyright 2015 Marcus Zetterquist

	Licensed under the Apache License, Version 2.0 (the "License");
	you may not use this file except in compliance with the License.
	You may obtain a copy of the License at

		http://www.apache.org/licenses/LICENSE-2.0

	Unless required by applicable law or agreed to in writing, software
	distributed under the License is distributed on an "AS IS" BASIS,
	WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
	See the License for the specific language governing permissions and
	limitations under the License.

	steady::vector<> is a persistent vector class for C++
*/

#include "steady_vector.h"

#include <iostream>
#include <thread>
#include <future>
#include <cmath>
#include <algorithm>
#include <cassert>
#include "quark.h"

#ifdef _WIN32
#define M_PI 3.14159265358979323846
#endif


//	Make a vector of ints. Add a few numbers.
//	Notice that push_back() returns a new vector each time - you need to save the return value.
//	There are no side effects. This makes code very simple and solid.
//	It also makes it simple to design pure functions.
void example1(){
	steady::vector<int> a;
	a.push_back(3);
	a.push_back(8);
	a.push_back(11);

	//	Notice! a is still the empty vector! It has not changed!
	assert(a.size() == 0);

	//	Reuse variable b to keep the latest generation of the vector.
	steady::vector<int> b;
	b = b.push_back(3);
	b = b.push_back(8);
	b = b.push_back(11);

	assert(b.size() == 3);
	assert(b[2] == 11);
}


//	Demonstrate that "modifying" a vector leaves the original unchanged too.
//	Also: make the vector using initializer list (c++11)
void example2(){
	const steady::vector<int> a{ 10, 20, 30 };
	const auto b = a.push_back(40);
	const auto c = b.push_back(50);

	assert(a == (steady::vector<int>{ 10, 20, 30 }));
	assert(b == (steady::vector<int>{ 10, 20, 30, 40 }));
	assert(c == (steady::vector<int>{ 10, 20, 30, 40, 50 }));
	assert(c.size() == 5);
}


//	Replace values in the vector. This also leaves original vector unchanged.
void example3(){
	const steady::vector<int> a{ 10, 20, 30 };
	const auto b = a.store(0, 2010);
	const auto c = b.store(2, 2030);

	assert(a == (steady::vector<int>{ 10, 20, 30 }));
	assert(b == (steady::vector<int>{ 2010, 20, 30 }));
	assert(c == (steady::vector<int>{ 2010, 20, 2030 }));
}


//	Append two vectors.
void example4(){
	const steady::vector<int> a{ 1, 2, 3 };
	const steady::vector<int> b{ 4, 5 };
	const auto c = a + b;

	assert(a == (steady::vector<int>{ 1, 2, 3 }));
	assert(b == (steady::vector<int>{ 4, 5 }));
	assert(c == (steady::vector<int>{ 1, 2, 3, 4, 5 }));
}


//	Converting to and from std::vector<>.
void example5(){
	const std::vector<int> a{ 1, 2, 3 };

	//	Make a steady::vector<> from a std::vector<>:
	const steady::vector<int> b(a);

	//	Make a std::vector<> from a steady::vector<>
	const std::vector<int> c = b.to_vec();

	assert(a == (std::vector<int>{ 1, 2, 3 }));
	assert(b == (steady::vector<int>{ 1, 2, 3 }));
	assert(c == (std::vector<int>{ 1, 2, 3 }));
}


//	Try vectors of strings instead of ints.
void example6(){
	using std::string;

	const steady::vector<string> a{ "one", "two", "three" };
	const steady::vector<string> b{ "four", "five" };
	const auto c = a + b;

	assert(a == (steady::vector<string>{ "one", "two", "three" }));
	assert(b == (steady::vector<string>{ "four", "five" }));
	assert(c == (steady::vector<string>{ "one", "two", "three", "four", "five" }));
}


//	Build huge vector by appending repeatedly.
void example7(){
	const steady::vector<int> a{ 10, 20, 30, 40, 50, 60, 70, 80, 90, 100 };
	const auto b = a + a + a + a + a + a + a + a + a + a;
	assert(b.size() == 100);

	const auto c = b + b + b + b + b + b + b + b + b + b;
	assert(c.size() == 1000);

	const auto d = c + c + c + c + c + c + c + c + c + c;
	assert(d.size() == 10000);

	const auto e = d + d + d + d + d + d + d + d + d + d;
	assert(e.size() == 100000);
}



/////////////////		Multi-threading example



struct pixel {
	pixel(float red, float green, float blue, float alpha) :
		_red(red),
		_green(green),
		_blue(blue),
		_alpha(alpha)
	{
	}
	pixel() = default;
	pixel& operator=(const pixel& rhs) = default;
	pixel(const pixel& rhs) = default;


	////////////////	State

	float _red = 0.0f;
	float _green = 0.0f;
	float _blue = 0.0f;
	float _alpha = 0.0f;
};

struct image {
	int _width;
	int _height;
	steady::vector<pixel> _pixels;
};

image make_image(int width, int height){
	steady::vector<pixel> pixels;

	std::vector<pixel> line;
	line.reserve(width);
	for(auto y = 0 ; y < height ; y++){
		line.clear();
		for(auto x = 0 ; x < width ; x++){
			float s = std::sin(static_cast<float>(M_PI) * 2.0f * (float)x);
			pixel p(0.5f + s, 0.5f, 0.0f, 1.0f);
			line.push_back(p);
		}
		pixels = pixels.push_back(line);
	}

	image result;
	result._width = width;
	result._height = height;
	result._pixels = pixels;
	return result;
}


//	There is no way to trip-up caller because image is a copy.
image worker8(image img) {
	const size_t count = std::min<size_t>(300, img._pixels.size());
	for(size_t i = 0 ; i < count ; i++){
		auto pixel = img._pixels[i];
		pixel._red = 1.0f - pixel._red;
		img._pixels = img._pixels.store(i, pixel);
	}
	return img;
}

//	Demonstates it is safe and easy and efficent to use steady::vector when sharing data between threads.
void example8(){
	QUARK_SCOPED_TRACE(__FUNCTION__);

	//	Make a big image.
	const auto a = make_image(700, 700);

	//	How many nodes are allocated now?
	const auto inodes1 = steady::get_inode_count<pixel>();
	const auto leaf_nodes1 = steady::get_leaf_count<pixel>();
	QUARK_TRACE_SS("When a exists: inodes: " << inodes1 << ", leaf nodes: " << leaf_nodes1);

	//	Get a workerthread process image a in the background.
	//	We give the worker a copy of a. This is free.
	//	There is no way for worker thread to have side effects on image a.
	auto future = std::async(worker8, a);

	//	...meanwhile, do some processing of our own on mage a!
	auto b = a;
	for(int i = 0 ; i < 200 ; i++){
		auto pixel = b._pixels[i];
		pixel._red = 1.0f - pixel._red;
		b._pixels = b._pixels.store(i, pixel);
	}

	//	Block on worker finishing.
	image c = future.get();


	/*
		RESULT

		We now have 3 images: a, b, c. They are mostly identical.
		The modified images share most of their state = very litte memory is consumed.
		Copying the images was free.
		There was no way to get data races on the images.
		No need for locks.
		No need to duplicate data to avoid races.
	*/

	const auto inodes2 = steady::get_inode_count<pixel>();
	const auto leaf_nodes2 = steady::get_leaf_count<pixel>();
	QUARK_TRACE_SS("When b + c exists: inodes: " << inodes2 << ", leaf nodes: " << leaf_nodes2);

	const auto extra_inodes = inodes2 - inodes1;
	const auto extra_leaf_nodes = leaf_nodes2 - leaf_nodes1;

	QUARK_TRACE_SS("Extra storage requried for a + b: inodes: " << extra_inodes << ", leaf nodes: " << extra_leaf_nodes);
}









void worker9(int tid) {
	std::cout << "Launched by thread " << tid << std::endl;
}

//	Do some threading.
void example9(){
	const int num_threads = 10;
	std::vector<std::thread> threads;

	for (int i = 0; i < num_threads; ++i) {
		threads.push_back(std::thread(worker9, i));
	}

	std::cout << "Launched from the main\n";

	for (std::thread& t: threads) {
		t.join();
	}
}



void examples(){
	example1();
	example2();
	example3();
	example4();
	example5();
	example6();
	example7();
	example8();
//	example9();
}

int mainx(int argc, const char * argv[]){
	try {
#if QUARK_UNIT_TESTS_ON
		quark::run_tests();
#endif
		examples();
	}
	catch(...){
		QUARK_TRACE("Error");
		return -1;
	}

  return 0;
}
