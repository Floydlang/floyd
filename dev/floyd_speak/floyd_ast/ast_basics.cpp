//
//  ast_basics.cpp
//  FloydSpeak
//
//  Created by Marcus Zetterquist on 2018-02-12.
//  Copyright © 2018 Marcus Zetterquist. All rights reserved.
//

#include "ast_basics.h"

#include "quark.h"

#include <string>


namespace floyd {


}


QUARK_UNIT_TEST("", "", "", ""){
	const floyd::variable_address_t dummy;
	const auto copy(dummy);
	floyd::variable_address_t b;
	b = dummy;
}
