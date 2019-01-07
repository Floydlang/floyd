//
//  typeid_helpers.hpp
//  floyd_speak
//
//  Created by Marcus Zetterquist on 2019-01-07.
//  Copyright © 2019 Marcus Zetterquist. All rights reserved.
//

#ifndef typeid_helpers_hpp
#define typeid_helpers_hpp

	struct json_t;

namespace floyd {

	struct ast_json_t;
	struct struct_definition_t;

	ast_json_t struct_definition_to_ast_json(const struct_definition_t& v);

}

#endif /* typeid_helpers_hpp */
