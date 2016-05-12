//  http://codereview.stackexchange.com/questions/47167/conways-game-of-life-in-c

#include "quark.h"
#include <iostream>
#include <cstdlib>

namespace {
//const int gridsize = 75; //Making this a global constant to avoid array issues.
const int gridsize = 32;

void Display(bool grid[gridsize+1][gridsize+1]){
    for(int a = 1; a < gridsize; a++){
        for(int b = 1; b < gridsize; b++){
            if(grid[a][b] == true){
                std::cout << " *";
            }
            else{
                std::cout << "  ";
            }
            if(b == gridsize-1){
                std::cout << std::endl;
            }
        }
    }
}
//This copy's the grid for comparision purposes.
void CopyGrid (bool grid[gridsize+1][gridsize+1],bool grid2[gridsize+1][gridsize+1]){
    for(int a =0; a < gridsize; a++){
        for(int b = 0; b < gridsize; b++){grid2[a][b] = grid[a][b];}
    }
}
//Calculates Life or Death
void liveOrDie(bool grid[gridsize+1][gridsize+1]){
    bool grid2[gridsize+1][gridsize+1] = {};
    CopyGrid(grid, grid2);
    for(int a = 1; a < gridsize; a++){
        for(int b = 1; b < gridsize; b++){
            int life = 0;
        for(int c = -1; c < 2; c++){
            for(int d = -1; d < 2; d++){
                if(!(c == 0 && d == 0)){
                    if(grid2[a+c][b+d]) {++life;}
                }
            }
        }
            if(life < 2) {grid[a][b] = false;}
            else if(life == 3){grid[a][b] = true;}
            else if(life > 3){grid[a][b] = false;}
        }
    }
}

}
void game_of_life1(int generations){

    //const int gridsize = 50;
    bool grid[gridsize+1][gridsize+1] = {};

    //Still have to manually enter the starting cells.
    grid[gridsize/2][gridsize/2] = true;
    grid[gridsize/2-1][gridsize/2] = true;
    grid[gridsize/2][gridsize/2+1] = true;
    grid[gridsize/2][gridsize/2-1] = true;
    grid[gridsize/2+1][gridsize/2+1] = true;

    for(int i = 0 ; i < generations ; i++){
        //The following copies our grid.

        Display(grid);     //This is our display.
        liveOrDie(grid); //calculate if it lives or dies.
    }
}

QUARK_UNIT_TESTQ("game_of_life1", "10 generations"){
	game_of_life1(10);
}
