//
//  unix_helpers.cpp
//  floyd
//
//  Created by Marcus Zetterquist on 2019-12-01.
//  Copyright © 2019 Marcus Zetterquist. All rights reserved.
//

#include "unix_helpers.h"



std::vector<std::string> unpack_vector_of_strings(char** vec){
	QUARK_ASSERT(vec != nullptr);
	std::vector<std::string> result;
	for(int i = 0 ; vec[i] != nullptr ; i++){
		const std::string s(vec[i]);
		result.push_back(s);
	}
	return result;
}





unix_errno_t get_unix_err(){
	return unix_errno_t { errno };
}


#if QUARK_MAC || QUARK_WINDOWS

std::string unix_err_to_string(const unix_errno_t& error){
	char temp[200 + 1];
	int err = strerror_r(error.value, temp, 200);
	if(err != 0){
		throw_errno2("strerror_r()", get_unix_err());
	}
	return std::string(temp);
}

#endif

#if QUARK_LINUX

// Gnu specific? return string describing error number
std::string unix_err_to_string(const unix_errno_t& error){
	char temp[200 + 1];
	char* ptr = strerror_r(error.value, temp, 200);
	if(ptr == nullptr){
		throw_errno2("strerror_r()", get_unix_err());
	}
	return std::string(temp);
}

#endif

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
