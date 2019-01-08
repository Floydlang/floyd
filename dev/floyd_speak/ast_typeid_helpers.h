//
//  typeid_helpers.hpp
//  floyd_speak
//
//  Created by Marcus Zetterquist on 2019-01-07.
//  Copyright © 2019 Marcus Zetterquist. All rights reserved.
//

#ifndef typeid_helpers_hpp
#define typeid_helpers_hpp

#include <vector>

	struct json_t;

namespace floyd {

	struct ast_json_t;
	struct struct_definition_t;
	struct protocol_definition_t;
	struct typeid_t;


	ast_json_t struct_definition_to_ast_json(const struct_definition_t& v);



	ast_json_t protocol_definition_to_ast_json(const protocol_definition_t& v);





	//////////////////////////////////////		JSON



	enum class json_tags{
		k_plain,

		//	Show in the string if the type has been resolved or not. Uses "^hello" or "#world"
		k_tag_resolve_state
	};

	ast_json_t typeid_to_ast_json(const typeid_t& t, json_tags tags);
	typeid_t typeid_from_ast_json(const ast_json_t& t);

	std::vector<json_t> typeids_to_json_array(const std::vector<typeid_t>& m);
	std::vector<typeid_t> typeids_from_json_array(const std::vector<json_t>& m);



}

#endif /* typeid_helpers_hpp */
