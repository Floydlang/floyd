//
//  floyd_basics.cpp
//  FloydSpeak
//
//  Created by Marcus Zetterquist on 2017-08-09.
//  Copyright © 2017 Marcus Zetterquist. All rights reserved.
//

#include "floyd_basics.h"

using std::string;
using std::vector;

namespace floyd {

//////////////////////////////////////////////////		keywords


const std::string keyword_t::k_return = "return";
const std::string keyword_t::k_while = "while";
const std::string keyword_t::k_for = "for";
const std::string keyword_t::k_if = "if";
const std::string keyword_t::k_else = "else";

const std::string keyword_t::k_false = "false";
const std::string keyword_t::k_true = "true";
const std::string keyword_t::k_bool = "bool";

const std::string keyword_t::k_int = "int";
const std::string keyword_t::k_float = "float";
const std::string keyword_t::k_string = "string";
const std::string keyword_t::k_typeid = "typeid";
const std::string keyword_t::k_json_value = "json_value";
const std::string keyword_t::k_struct = "struct";

const std::string keyword_t::k_mutable = "mutable";


}
