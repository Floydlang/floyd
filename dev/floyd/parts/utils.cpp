//
//  utils.cpp
//  Floyd
//
//  Created by Marcus Zetterquist on 11/08/16.
//  Copyright © 2016 Marcus Zetterquist. All rights reserved.
//

#include "utils.h"
#include "quark.h"
#include <string>


using std::vector;
using std::string;

//	template <typename T> bool compare_shared_values(const T& ptr_a, const T& ptr_b){


QUARK_UNIT_TEST("", "compare_shared_values()", "", ""){
	const auto a = std::make_shared<std::string>("a");
	const auto b = std::make_shared<std::string>("a");
	QUARK_TEST_VERIFY(compare_shared_values(a, b) == true);
}

QUARK_UNIT_TEST("", "compare_shared_values()", "", ""){
	const auto a = std::make_shared<std::string>("a");
	const auto b = a;
	QUARK_TEST_VERIFY(compare_shared_values(a, b) == true);
}


QUARK_UNIT_TEST("", "compare_shared_values()", "", ""){
	const auto a = std::make_shared<std::string>("a");
	const auto b = std::make_shared<std::string>("b");
	QUARK_TEST_VERIFY(compare_shared_values(a, b) == false);
}
QUARK_UNIT_TEST("", "compare_shared_values()", "", ""){
	const auto a = std::shared_ptr<std::string>();
	const auto b = std::make_shared<std::string>("b");
	QUARK_TEST_VERIFY(compare_shared_values(a, b) == false);
}
QUARK_UNIT_TEST("", "compare_shared_values()", "", ""){
	const auto a = std::make_shared<std::string>("a");
	const auto b = std::shared_ptr<std::string>();
	QUARK_TEST_VERIFY(compare_shared_values(a, b) == false);
}
QUARK_UNIT_TEST("", "compare_shared_values()", "", ""){
	const auto a = std::shared_ptr<std::string>();
	const auto b = std::shared_ptr<std::string>();
	QUARK_TEST_VERIFY(compare_shared_values(a, b) == true);
}




auto lambda_echo = [](int i ) { QUARK_TRACE_SS(i); };

std::vector<int> test_collection{20,24,37,42,23,45,37};

QUARK_UNIT_TEST("", "lambda_echo()", "", "") {
	for_each_col(test_collection, lambda_echo);
}




/*
https://stackoverflow.com/questions/14945223/map-function-with-c11-constructs

#include <vector>
#include <algorithm>
#include <type_traits>

//
// Takes an iterable, applies a function to every element,
// and returns a vector of the results
//
template <typename T, typename Func>
auto map_container(const T& iterable, Func&& func) ->
    std::vector<decltype(func(std::declval<typename T::value_type>()))>
{
    // Some convenience type definitions
    typedef decltype(func(std::declval<typename T::value_type>())) value_type;
    typedef std::vector<value_type> result_type;

    // Prepares an output vector of the appropriate size
    result_type res(iterable.size());

    // Let std::transform apply `func` to all elements
    // (use perfect forwarding for the function object)
    std::transform(
        begin(iterable), end(iterable), res.begin(),
        std::forward<Func>(func)
        );

    return res;
}
*/

QUARK_UNIT_TEST("", "mapf()", "", "") {
	auto result = mapf<int>(std::vector<int>{ 20, 21, 22, 23 }, [](int e){ return e + 100; });
	QUARK_UT_VERIFY(result == (std::vector<int>{ 120, 121, 122, 123 }));
}

QUARK_UNIT_TEST("", "mapf()", "", "") {
	auto result = mapf<string>(std::vector<int>{ 20, 21, 22, 23 }, [](int e){ return std::to_string(e + 100); });
	QUARK_UT_VERIFY(result == (std::vector<string>{ "120", "121", "122", "123" }));
}




/*
QUARK_UNIT_TEST("", "filter()", "", "") {
	auto filteredCol = filter(test_collection,[](int value){ return value > 30;});
	for_each(filteredCol,lambda_echo);
}
*/



QUARK_UNIT_TEST("", "reduce()", "", "") {
	const auto result = reduce(std::vector<int>{ 1, 2, 3 }, 1000, [](int acc, int e){ return acc + e; });
	QUARK_UT_VERIFY(result == (1000 + 1 + 2 + 3));
}
QUARK_UNIT_TEST("", "reduce()", "", "") {
	const auto result = reduce(std::vector<std::string>{ "a", "b", "c", "b", "d" }, 100, [](int acc, std::string e){ return e == "b" ? acc + 1 : acc; });
	QUARK_UT_VERIFY(result == 102);
}




#if 0
std::string float_to_string_no_trailing_zeros(float v){
    std::stringstream ss;
    ss << v;
    return ss.str();
/*
	char temp[200 + 1];//### Use C++ function instead.
	sprintf(temp, "%f", _float);
	return std::string(temp);
*/
}


QUARK_UNIT_TEST("", "", "", ""){
	QUARK_UT_VERIFY(float_to_string_no_trailing_zeros(0) == "0");
}
QUARK_UNIT_TEST("", "", "", ""){
	QUARK_UT_VERIFY(float_to_string_no_trailing_zeros(123) == "123");
}
QUARK_UNIT_TEST("", "", "", ""){
	QUARK_UT_VERIFY(float_to_string_no_trailing_zeros(1.123) == "1.123");
}
QUARK_UNIT_TEST("", "", "", ""){
	QUARK_UT_VERIFY(float_to_string_no_trailing_zeros(1.5) == "1.5");
}
#endif


#include <sstream>
