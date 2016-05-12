//
//  game_of_life1.h
//  FloydSpeak
//
//  Created by Marcus Zetterquist on 12/05/16.
//  Copyright © 2016 Marcus Zetterquist. All rights reserved.
//

#ifndef game_of_life3_h
#define game_of_life3_h

#include <string>

namespace game_of_life3 {
//	const int gridsize = 75;
	const int gridsize = 32;

	struct grid_t {
		grid_t(){
			for(int a = 0 ; a < (gridsize + 1) ; a++){
				for(int b = 0 ; b < (gridsize + 1) ; b++){
					_entries[a][b] = false;
				}
			}
		}
		bool _entries[gridsize + 1][gridsize + 1];
	};

	std::string to_string(const grid_t& grid);
	grid_t liveOrDie(const grid_t& grid0);
	grid_t make_init();
	void game_of_life(int generations);
}


#endif /* game_of_life1_h */
