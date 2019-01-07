//
//  typeid_helpers.cpp
//  floyd_speak
//
//  Created by Marcus Zetterquist on 2019-01-07.
//  Copyright © 2019 Marcus Zetterquist. All rights reserved.
//

#include "ast_typeid_helpers.h"

#include "quark.h"
#include "floyd_basics.h"
#include "ast_typeid.h"



namespace floyd {

ast_json_t struct_definition_to_ast_json(const struct_definition_t& v){
	QUARK_ASSERT(v.check_invariant());

	return ast_json_t{json_t::make_array({
		members_to_json(v._members)
	})};
}


}
