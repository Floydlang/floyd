//  http://codereview.stackexchange.com/questions/47167/conways-game-of-life-in-c

#include "quark.h"
#include <iostream>
#include <cstdlib>

#include "game_of_life1.h"
#include "game_of_life2.h"
#include "game_of_life3.h"

using std::string;

QUARK_UNIT_TESTQ("game_of_life3", "same result as game_of_life2"){
	game_of_life2::grid_t grid2 = game_of_life2::make_init();
	game_of_life3::grid_t grid3 = game_of_life3::make_init();

	for(int i = 0 ; i < 100 ; i++){
		const auto s2 = to_string(grid2);
		const auto s3 = to_string(grid3);
		QUARK_TEST_VERIFY(s2 == s3);


		grid2 = liveOrDie(grid2);
		grid3 = liveOrDie(grid3);
	}
}



