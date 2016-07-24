//
//  runtime_value.hpp
//  FloydSpeak
//
//  Created by Marcus Zetterquist on 24/07/16.
//  Copyright © 2016 Marcus Zetterquist. All rights reserved.
//

#ifndef runtime_value_hpp
#define runtime_value_hpp

#include <cstdio>
#include <vector>

#include "runtime_core.h"



namespace runtime_value {


	//////////////////////////////////////		frontend_value_ref


	/*
		Value type, immutable, reference counted.
		Always uses 8 bytes, optionally owns external memory too.

		Can hold *any* frontend value, ints, vectors, structs and composites etc.
		The type is external to the value.
		
	*/
	struct frontend_value_ref {
		public: frontend_value_ref();
		public: ~frontend_value_ref();
		public: bool check_invariant() const;

		public: uint8_t _data[8];
	};


	frontend_value_ref make_ref();


}	//	runtime_value



#endif /* runtime_value_hpp */
