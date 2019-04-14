//
//  typeid_helpers.hpp
//  floyd_speak
//
//  Created by Marcus Zetterquist on 2019-01-07.
//  Copyright © 2019 Marcus Zetterquist. All rights reserved.
//

#ifndef typeid_helpers_hpp
#define typeid_helpers_hpp

/*
	Functions that packs and unpacks typeid_t.
*/

#include "quark.h"
#include <vector>

struct json_t;

namespace floyd {

struct struct_definition_t;
struct typeid_t;
struct member_t;


json_t struct_definition_to_ast_json(const struct_definition_t& v);

json_t members_to_json(const std::vector<member_t>& members);
std::vector<member_t> members_from_json(const json_t& members);


enum class json_tags{
	k_plain,

	//	Show in the string if the type has been resolved or not. Uses "^hello" or "#world"
	k_tag_resolve_state
};
const char tag_unresolved_type_char = '#';
const char tag_resolved_type_char = '^';

json_t typeid_to_ast_json(const typeid_t& t, json_tags tags);
typeid_t typeid_from_ast_json(const json_t& t);


void ut_verify(const quark::call_context_t& context, const typeid_t& result, const typeid_t& expected);

}	//	floyd

#endif /* typeid_helpers_hpp */
