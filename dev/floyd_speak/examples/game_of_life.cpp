//
//  FloydSpeak
//
//  Created by Marcus Zetterquist on 26/07/16.
//  Copyright © 2016 Marcus Zetterquist. All rights reserved.
//

#include "benchmark_basics.h"

#include <iostream>


//////////////////////////////////////////		GAME OF LIFE - C

//	https://www.cs.utexas.edu/users/djimenez/utsa/cs1713-3/c/life.txt

/*
 * The Game of Life
 *
 * a cell is born, if it has exactly three neighbours
 * a cell dies of loneliness, if it has less than two neighbours
 * a cell dies of overcrowding, if it has more than three neighbours
 * a cell survives to the next generation, if it does not die of loneliness
 * or overcrowding
 *
 * In my version, a 2D array of ints is used.  A 1 cell is on, a 0 cell is off.
 * The game plays 100 rounds, printing to the screen each time.  'x' printed
 * means on, space means 0.
 *
	(copied this from some web page, hence the English spellings...)

	1.STASIS : If, for a given cell, the number of on neighbours is
	exactly two, the cell maintains its status quo into the next
	generation. If the cell is on, it stays on, if it is off, it stays off.

	2.GROWTH : If the number of on neighbours is exactly three, the cell
	will be on in the next generation. This is regardless of the cell's
	current state.

	3.DEATH : If the number of on neighbours is 0, 1, 4-8, the cell will
	be off in the next generation.
*/

/* dimensions of the screen */

#define BOARD_WIDTH		79
#define BOARD_HEIGHT	24

/* set everthing to zero */

void initialize_board (int board[][BOARD_HEIGHT]) {
	int	i, j;

	for (i=0; i<BOARD_WIDTH; i++) for (j=0; j<BOARD_HEIGHT; j++)
		board[i][j] = 0;
}

/* add to a width index, wrapping around like a cylinder */

int xadd (int i, int a) {
	i += a;
	while (i < 0) i += BOARD_WIDTH;
	while (i >= BOARD_WIDTH) i -= BOARD_WIDTH;
	return i;
}

/* add to a height index, wrapping around */

int yadd (int i, int a) {
	i += a;
	while (i < 0) i += BOARD_HEIGHT;
	while (i >= BOARD_HEIGHT) i -= BOARD_HEIGHT;
	return i;
}

/* return the number of on cells adjacent to the i,j cell */

int adjacent_to (int board[][BOARD_HEIGHT], int i, int j) {
	int	k, l, count;

	count = 0;

	/* go around the cell */

	for (k=-1; k<=1; k++) for (l=-1; l<=1; l++)

		/* only count if at least one of k,l isn't zero */

		if (k || l)
			if (board[xadd(i,k)][yadd(j,l)]) count++;
	return count;
}

void play (int board[][BOARD_HEIGHT]) {
	int	i, j, a, newboard[BOARD_WIDTH][BOARD_HEIGHT];

	/* for each cell, apply the rules of Life */

	for (i=0; i<BOARD_WIDTH; i++) for (j=0; j<BOARD_HEIGHT; j++) {
		a = adjacent_to (board, i, j);
		if (a == 2) newboard[i][j] = board[i][j];
		if (a == 3) newboard[i][j] = 1;
		if (a < 2) newboard[i][j] = 0;
		if (a > 3) newboard[i][j] = 0;
	}

	/* copy the new board back into the old board */

	for (i=0; i<BOARD_WIDTH; i++) for (j=0; j<BOARD_HEIGHT; j++) {
		board[i][j] = newboard[i][j];
	}
}

/* print the life board */

void print (int board[][BOARD_HEIGHT]) {
	int	i, j;

	/* for each row */

	for (j=0; j<BOARD_HEIGHT; j++) {

		/* print each column position... */

		for (i=0; i<BOARD_WIDTH; i++) {
			printf ("%c", board[i][j] ? 'x' : ' ');
		}

		/* followed by a carriage return */

		printf ("\n");
	}
}

/* main program */

int test_main (int argc, char *argv[]) {
	int	board[BOARD_WIDTH][BOARD_HEIGHT], i;

	initialize_board (board);
//	read_file (board, argv[1]);

	board[BOARD_WIDTH / 2][BOARD_HEIGHT / 2] = 'x';
	board[1 + BOARD_WIDTH / 2][1+ BOARD_HEIGHT / 2] = 'x';
	board[1 + BOARD_WIDTH / 2][BOARD_HEIGHT / 2] = 'x';
	board[2 + BOARD_WIDTH / 2][BOARD_HEIGHT / 2] = 'x';
	board[2 + BOARD_WIDTH / 2][1 + BOARD_HEIGHT / 2] = 'x';
	board[3 + BOARD_WIDTH / 2][BOARD_HEIGHT / 2] = 'x';
	board[3 + BOARD_WIDTH / 2][1 + BOARD_HEIGHT / 2] = 'x';


	/* play game of life 100 times */

	for (i=0; i<100; i++) {
		printf ("%d\n", i);
		print (board);
		play (board);

		/* clear the screen using VT100 escape codes */
//		puts ("\033[H\033[J");
	}
	return 0;
}

QUARK_UNIT_TEST("Basic performance", "Game of life", "", ""){
	test_main(0, nullptr);
}

