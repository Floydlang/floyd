//
//  cpp_experiments.cpp
//  FloydSpeak
//
//  Created by Marcus Zetterquist on 2017-07-23.
//  Copyright © 2017 Marcus Zetterquist. All rights reserved.
//

#include <memory>
#include <string>
#include <thread>
#include <future>
#include "immer/vector.hpp"
//#include "parts/xcode-libcxx-xcode9/variant"	//	https://github.com/youknowone/xcode-libcxx
#include <variant>
#include <cmath>

using namespace std;

#include "quark.h"



QUARK_UNIT_TEST("", "", "", ""){
	double a = 10.0f;
	double b = 23.3f;

	bool r = a && b;
	QUARK_UT_VERIFY(r == true);
}

struct test_cpp_value_class_a {
	int _a = 10;
	int _b = 10;

	bool operator==(const test_cpp_value_class_a& other){
		return _a == other._a && _b == other._b;
	}
};

QUARK_UNIT_TESTQ("test_cpp_value_class_a", "what is needed for basic operations"){
	test_cpp_value_class_a a;
	test_cpp_value_class_a b = a;

	QUARK_TEST_VERIFY(b._a == 10);
	QUARK_TEST_VERIFY(a == b);
}


QUARK_UNIT_TESTQ("C++ bool", ""){
	quark::ut_compare(true, true);
	quark::ut_compare(true, !false);
	quark::ut_compare(false, false);
	quark::ut_compare(!false, true);

	const auto x = false + false;
	const auto y = false - false;

	QUARK_UT_VERIFY(x == false);
	QUARK_UT_VERIFY(y == false);
}

QUARK_UNIT_TEST("","", "", ""){
	const auto size = sizeof(std::make_shared<int>(13));
	QUARK_UT_VERIFY(size == 16);
}




QUARK_UNIT_TEST("C++17 variant", "", "", ""){
    std::variant<int,std::string> foo;
	
    //Assign a value to the integer member
    foo = 69;
    //Extract the last assigned value:
    if( std::get<int>(foo) == 69 ) {
        std::cout << "Let's do it!\n";
    }
	
    //Assign a value to the std::string member
    foo = "Ciao!";
	
    std::cout << std::get<std::string>(foo) << std::endl;

	const auto x = std::get<std::string>(foo);
	QUARK_UT_VERIFY(x == "Ciao!");
}

/*
struct no_op_equal {
	float x;
	private: bool operator==(const no_op_equal& other){return false;};
};

QUARK_UNIT_TEST("C++17 variant", "operator==", "", ""){
    std::variant<int, std::string, no_op_equal> a = no_op_equal{ 3.14f };
    std::variant<int, std::string, no_op_equal> b = no_op_equal{ 3.14f };
	QUARK_UT_VERIFY(a == b);
}
*/




struct expression_t {
	std::string _source_code;
};

struct return_t {
	expression_t _expression;
};
struct store_t {
	std::string _variable_name;
	expression_t _expression;
};
struct bind_t {
	std::string _variable_name;
	expression_t _expression;
};
struct expression_statement_t {
	expression_t _expression;
};

struct test_statement_t {
    std::variant<return_t, store_t, bind_t, expression_statement_t> _contents;

	std::string _debug_str;
};


QUARK_UNIT_TEST("C++17 variant", "", "", ""){
	test_statement_t a{bind_t{"myval", {"1+3"}}, "debug myval = 1+3"};

	const auto s = sizeof(a);

	const auto bind = std::get<bind_t>(a._contents);
	QUARK_UT_VERIFY(bind._variable_name == "myval");
	QUARK_UT_VERIFY(bind._expression._source_code == "1+3");


	QUARK_UT_VERIFY(a._contents.index() == 2);
	const auto bind2 = std::get<2>(a._contents);
	QUARK_UT_VERIFY(bind2._variable_name == "myval");
	QUARK_UT_VERIFY(bind2._expression._source_code == "1+3");



	QUARK_UT_VERIFY(holds_alternative<bind_t>(a._contents) == true);
	QUARK_UT_VERIFY(holds_alternative<return_t>(a._contents) == false);


// 	const auto b = std::get_if<int>(&a._contents);
//	QUARK_UT_VERIFY(b != nullptr);

 	const auto c = std::get_if<bind_t>(&a._contents);
	QUARK_UT_VERIFY(c != nullptr);
}




/////////////////        Multi-threading example



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
	
	
	////////////////    State
	
	float _red = 0.0f;
	float _green = 0.0f;
	float _blue = 0.0f;
	float _alpha = 0.0f;
};

struct image {
	int _width;
	int _height;
	immer::vector<pixel> _pixels;
};

image make_image(int width, int height){
	immer::vector<pixel> pixels;
	
	for(auto y = 0 ; y < height ; y++){
		for(auto x = 0 ; x < width ; x++){
			float s = std::sin(static_cast<float>(M_PI) * 2.0f * (float)x);
			pixel p(0.5f + s, 0.5f, 0.0f, 1.0f);
			pixels.push_back(p);
		}
	}
	
	image result;
	result._width = width;
	result._height = height;
	result._pixels = pixels;
	return result;
}


//    There is no way to trip-up caller because image is a copy.
image worker8(image img) {
	const size_t count = std::min<size_t>(300, img._pixels.size());
	for(size_t i = 0 ; i < count ; i++){
		auto pixel = img._pixels[i];
		pixel._red = 1.0f - pixel._red;
		img._pixels = img._pixels.set(i, pixel);
	}
	return img;
}

//    Demonstates it is safe and easy and efficent to use immer::vector when sharing data between threads.
void example8(){
	QUARK_SCOPED_TRACE(__FUNCTION__);
	
	const auto a = make_image(700, 700);
	
//    const auto inodes1 = steady::get_inode_count<pixel>();
//    const auto leaf_nodes1 = steady::get_leaf_count<pixel>();
//    QUARK_TRACE_SS("When a exists: inodes: " << inodes1 << ", leaf nodes: " << leaf_nodes1);
	
	//    Get a workerthread process image a in the background.
	//    We give the worker a copy of a. This is free.
	//    There is no way for worker thread to have side effects on image a.
	auto future = std::async(worker8, a);
	
	//    ...meanwhile, do some processing of our own on mage a!
	auto b = a;
	for(int i = 0 ; i < 200 ; i++){
		auto pixel = b._pixels[i];
		pixel._red = 1.0f - pixel._red;
		b._pixels = b._pixels.set(i, pixel);
	}
	
	//    Block on worker finishing.
	image c = future.get();
	
	
//    const auto inodes2 = steady::get_inode_count<pixel>();
//    const auto leaf_nodes2 = steady::get_leaf_count<pixel>();
	//QUARK_TRACE_SS("When b + c exists: inodes: " << inodes2 << ", leaf nodes: " << leaf_nodes2);
	
//    const auto extra_inodes = inodes2 - inodes1;
//    const auto extra_leaf_nodes = leaf_nodes2 - leaf_nodes1;
	
//    QUARK_TRACE_SS("Extra storage requried for a + b: inodes: " << extra_inodes << ", leaf nodes: " << extra_leaf_nodes);
}

QUARK_UNIT_TEST("","", "", ""){
	example8();
}


static const int k_hardware_thread_count = 8;



void call_from_thread(int tid) {
	std::cout << tid << std::endl;
}

struct runtime_t {
	std::vector<std::thread> _worker_threads;


	runtime_t(){
		//	Remember that current thread (main) is also a thread.
		for(int i = 0 ; i < k_hardware_thread_count - 1 ; i++){
			_worker_threads.push_back(std::thread(call_from_thread, i));
		}
	}
	~runtime_t(){
		for(auto &t: _worker_threads){
			t.join();
		}
	}
};


QUARK_UNIT_TEST("","", "", ""){
	std::cout << "A" << std::endl;
	{
		runtime_t test_runtime;
		std::cout << "B" << std::endl;
	}
	std::cout << "C" << std::endl;
}


