//
//  unix_helpers.hpp
//  floyd
//
//  Created by Marcus Zetterquist on 2019-12-01.
//  Copyright © 2019 Marcus Zetterquist. All rights reserved.
//

#ifndef unix_helpers_hpp
#define unix_helpers_hpp

#include <string>
#include <vector>
#include "quark.h"

std::vector<std::string> unpack_vector_of_strings(char** vec);



////////////////////////////////		UNIX ERRORS()


struct unix_errno_t { int value; };
unix_errno_t get_unix_err();
std::string unix_err_to_string(const unix_errno_t& error);
void throw_errno2(const std::string& header, const unix_errno_t& error) QUARK_NO_RETURN;


#endif /* unix_helpers_hpp */
