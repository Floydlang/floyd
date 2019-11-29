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
#include <sstream>
#include <iomanip>


using std::vector;
using std::string;

//	template <typename T> bool compare_shared_values(const T& ptr_a, const T& ptr_b){


QUARK_TEST("", "compare_shared_values()", "", ""){
	const auto a = std::make_shared<std::string>("a");
	const auto b = std::make_shared<std::string>("a");
	QUARK_VERIFY(compare_shared_values(a, b) == true);
}

QUARK_TEST("", "compare_shared_values()", "", ""){
	const auto a = std::make_shared<std::string>("a");
	const auto b = a;
	QUARK_VERIFY(compare_shared_values(a, b) == true);
}


QUARK_TEST("", "compare_shared_values()", "", ""){
	const auto a = std::make_shared<std::string>("a");
	const auto b = std::make_shared<std::string>("b");
	QUARK_VERIFY(compare_shared_values(a, b) == false);
}
QUARK_TEST("", "compare_shared_values()", "", ""){
	const auto a = std::shared_ptr<std::string>();
	const auto b = std::make_shared<std::string>("b");
	QUARK_VERIFY(compare_shared_values(a, b) == false);
}
QUARK_TEST("", "compare_shared_values()", "", ""){
	const auto a = std::make_shared<std::string>("a");
	const auto b = std::shared_ptr<std::string>();
	QUARK_VERIFY(compare_shared_values(a, b) == false);
}
QUARK_TEST("", "compare_shared_values()", "", ""){
	const auto a = std::shared_ptr<std::string>();
	const auto b = std::shared_ptr<std::string>();
	QUARK_VERIFY(compare_shared_values(a, b) == true);
}




auto lambda_echo = [](int i ) { QUARK_TRACE_SS(i); };

std::vector<int> test_collection{20,24,37,42,23,45,37};

QUARK_TEST("", "lambda_echo()", "", "") {
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

QUARK_TEST("", "mapf()", "", "") {
	auto result = mapf<int>(std::vector<int>{ 20, 21, 22, 23 }, [](int e){ return e + 100; });
	QUARK_VERIFY(result == (std::vector<int>{ 120, 121, 122, 123 }));
}

QUARK_TEST("", "mapf()", "", "") {
	auto result = mapf<string>(std::vector<int>{ 20, 21, 22, 23 }, [](int e){ return std::to_string(e + 100); });
	QUARK_VERIFY(result == (std::vector<string>{ "120", "121", "122", "123" }));
}




/*
QUARK_TEST("", "filter()", "", "") {
	auto filteredCol = filter(test_collection,[](int value){ return value > 30;});
	for_each(filteredCol,lambda_echo);
}
*/



QUARK_TEST("", "reduce()", "", "") {
	const auto result = reduce(std::vector<int>{ 1, 2, 3 }, 1000, [](int acc, int e){ return acc + e; });
	QUARK_VERIFY(result == (1000 + 1 + 2 + 3));
}
QUARK_TEST("", "reduce()", "", "") {
	const auto result = reduce(std::vector<std::string>{ "a", "b", "c", "b", "d" }, 100, [](int acc, std::string e){ return e == "b" ? acc + 1 : acc; });
	QUARK_VERIFY(result == 102);
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


QUARK_TEST("", "", "", ""){
	QUARK_VERIFY(float_to_string_no_trailing_zeros(0) == "0");
}
QUARK_TEST("", "", "", ""){
	QUARK_VERIFY(float_to_string_no_trailing_zeros(123) == "123");
}
QUARK_TEST("", "", "", ""){
	QUARK_VERIFY(float_to_string_no_trailing_zeros(1.123) == "1.123");
}
QUARK_TEST("", "", "", ""){
	QUARK_VERIFY(float_to_string_no_trailing_zeros(1.5) == "1.5");
}
#endif



std::string value_to_hex_string(uint64_t value, int hexchars){
	std::stringstream ss;
	ss << /*<< std::showbase*/ std::hex << std::setfill('0') << std::setw(hexchars) << value;
	const auto s2 = ss.str();
	return s2.substr(0, hexchars);
}

QUARK_TEST("", "value_to_hex_string()", "", "") {
	const auto s = value_to_hex_string(0xdeadbeef, 8);
	QUARK_VERIFY(s == "deadbeef");
}
QUARK_TEST("", "value_to_hex_string()", "", "") {
	const auto s = value_to_hex_string(0xdeadbeef, 0);
	QUARK_VERIFY(s == "");
}
QUARK_TEST("", "value_to_hex_string()", "", "") {
	const auto s = value_to_hex_string(0xdeadbeef, 16);
	QUARK_VERIFY(s == "00000000deadbeef");
}
QUARK_TEST("", "value_to_hex_string()", "", "") {
	const auto s = value_to_hex_string(0xdeadbeef00001234, 16);
	QUARK_VERIFY(s == "deadbeef00001234");
}
QUARK_TEST("", "value_to_hex_string()", "", "") {
	const auto s = value_to_hex_string(0x03, 8);
	QUARK_VERIFY(s == "00000003");
}
QUARK_TEST("", "value_to_hex_string()", "", "") {
	const auto s = value_to_hex_string(65535, 8);
	QUARK_VERIFY(s == "0000ffff");
}




std::string ptr_to_hexstring(const void* ptr){
	const auto v = reinterpret_cast<std::size_t>(ptr);
	return "0x" + value_to_hex_string(v, sizeof(void*) * 2);
}

QUARK_TEST("", "ptr_to_hexstring()", "", "") {
	const auto s = ptr_to_hexstring((void*)0xdeadbeef);
	QUARK_VERIFY(s == "0x00000000deadbeef");
}



std::string concat_string(const std::vector<std::string>& vec, const std::string& divider){
	std::string acc;

	if(vec.empty()){
		return "";
	}
	const auto count = vec.size() - 1;
	for(std::vector<std::string>::size_type i = 0 ; i < count ; i++){
		acc = acc + vec[i] + divider;
	}
	acc = acc + vec.back();
	return acc;
}



unix_errno_t get_unix_err(){
	return unix_errno_t { errno };
}


std::string unix_err_to_string(const unix_errno_t& error){
	char temp[200 + 1];
	int err = strerror_r(error.value, temp, 200);
	QUARK_ASSERT(err == 0);
	return std::string(temp);
}

QUARK_TEST("", "unix_err_to_string()", "", "") {
	QUARK_VERIFY(unix_err_to_string({ EPERM }) == "Operation not permitted");
}
QUARK_TEST("", "unix_err_to_string()", "", "") {
	QUARK_VERIFY(unix_err_to_string({ ENOENT }) == "No such file or directory");
}
QUARK_TEST("", "unix_err_to_string()", "", "") {
	QUARK_VERIFY(unix_err_to_string({ ENOMEM }) == "Cannot allocate memory");
}
QUARK_TEST("", "unix_err_to_string()", "", "") {
	QUARK_VERIFY(unix_err_to_string({ EINVAL }) == "Invalid argument");
}
QUARK_TEST("", "unix_err_to_string()", "", "") {
	QUARK_VERIFY(unix_err_to_string({ ERANGE }) == "Result too large");
}
QUARK_TEST("", "unix_err_to_string()", "", "") {
	QUARK_VERIFY(unix_err_to_string({ EPERM }) == "Operation not permitted");
}


static std::string make_what(const std::string& header, const unix_errno_t& error) {
	const auto error_str = unix_err_to_string(error);
	const auto what = header + "failed with unix error: \"" + error_str + "\" (errno: " + std::to_string(error.value) + ")";
	return what;
}

QUARK_TEST("", "make_what()", "", "") {
	const auto r = make_what("send()", { EPERM });
	QUARK_VERIFY(r == "send()failed with unix error: \"Operation not permitted\" (errno: 1)");
}

void throw_errno2(const std::string& header, const unix_errno_t& error) {
	const auto what = make_what(header, error);
	throw std::runtime_error(what);
}


/*
void default_runtime::runtime_i__on_assert(const source_code_location& location, const char expression[]){
	QUARK_TRACE_SS(std::string("Assertion failed ") << location._source_file << ", " << location._line_number << " \"" << expression << "\"");

	int error = errno;
	if(error != 0){
		perror("perror() says");
	}

	throw std::logic_error("assert");
}
*/




