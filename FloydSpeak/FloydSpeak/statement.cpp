//
//  parser_statement.cpp
//  FloydSpeak
//
//  Created by Marcus Zetterquist on 26/07/16.
//  Copyright © 2016 Marcus Zetterquist. All rights reserved.
//

#include "statement.h"


bool floyd::body_t::check_invariant() const {
			for(const auto i: _statements){
				QUARK_ASSERT(i->check_invariant());
			};
			return true;
		}
