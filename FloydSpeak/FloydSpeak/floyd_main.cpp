//
//  floyd_main.cpp
//  FloydSpeak
//
//  Created by Marcus Zetterquist on 10/04/16.
//  Copyright © 2016 Marcus Zetterquist. All rights reserved.
//

#include <stdio.h>

#include "quark.h"
#include "game_of_life1.h"
#include "game_of_life2.h"
#include "game_of_life3.h"

//////////////////////////////////////////////////		main()



int main(int argc, const char * argv[]) {
	try {
#if QUARK_UNIT_TESTS_ON
		quark::run_tests();
#endif
	}
	catch(...){
		QUARK_TRACE("Error");
		return -1;
	}

  return 0;
}
