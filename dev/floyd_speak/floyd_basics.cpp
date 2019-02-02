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

const location_t k_no_location(std::numeric_limits<std::size_t>::max());

//////////////////////////////////////////////////		keywords


const std::string keyword_t::k_return = "return";
const std::string keyword_t::k_while = "while";
const std::string keyword_t::k_for = "for";
const std::string keyword_t::k_if = "if";
const std::string keyword_t::k_else = "else";
const std::string keyword_t::k_func = "func";
const std::string keyword_t::k_impure = "impure";

const std::string keyword_t::k_internal_undefined = "**undef**";
const std::string keyword_t::k_internal_dynamic = "**dyn**";
const std::string keyword_t::k_void = "void";
const std::string keyword_t::k_false = "false";
const std::string keyword_t::k_true = "true";
const std::string keyword_t::k_bool = "bool";

const std::string keyword_t::k_int = "int";
const std::string keyword_t::k_double = "double";
const std::string keyword_t::k_string = "string";
const std::string keyword_t::k_typeid = "typeid";
const std::string keyword_t::k_json_value = "json_value";
const std::string keyword_t::k_struct = "struct";
const std::string keyword_t::k_protocol = "protocol";

const std::string keyword_t::k_mutable = "mutable";
const std::string keyword_t::k_let = "let";

const std::string keyword_t::k_software_system = "software-system";
const std::string keyword_t::k_container_def = "container-def";

const std::string keyword_t::k_json_object = "json_object";
const std::string keyword_t::k_json_array = "json_array";
const std::string keyword_t::k_json_string = "json_string";
const std::string keyword_t::k_json_number = "json_number";
const std::string keyword_t::k_json_true = "json_true";
const std::string keyword_t::k_json_false = "json_false";
const std::string keyword_t::k_json_null = "json_null";

}
