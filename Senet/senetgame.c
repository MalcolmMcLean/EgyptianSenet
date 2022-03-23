#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "senetgame.h"

#define coin() (rand() >= RAND_MAX/2 ? 1 : 0)


static int moveallowed(SENETGAME *sg, int from, int to);
static int score(SENETGAME* game, int* s1, int* s2);
static int pawnblocked(SENETGAME *game, int index);
static int passesthree(SENETGAME *game, int from, int to);

SENETGAME *senetgame(int playertogo)
{
	SENETGAME *sg = malloc(sizeof(SENETGAME));
	int oppositeplayer;
	int i;
	int x, y;

	if (playertogo == 1)
		oppositeplayer = 2;
	else
		oppositeplayer = 1;
	if (!sg)
		return 0;
	for (i = 0; i < 14; i++)
	{
		senet_indextoxy(i, &x, &y);
		sg->board[y][x] = (i % 2) ? oppositeplayer : playertogo;
	}
	for (i = 14; i < 30; i++)
	{
		senet_indextoxy(i, &x, &y);
		sg->board[y][x] = 0;
	}
	sg->togo = playertogo;
	sg->roll = 1;
	sg->state = SENET_WAITING_ROLL1;


	return sg;
}

void killsenetgame(SENETGAME *sg)
{
	if (sg)
	{
		free(sg);
	}
}

int senet_resetgame(SENETGAME *sg, int togo)
{
	SENETGAME *newgame;

	newgame = senetgame(togo);
	memcpy(sg, newgame, sizeof(SENETGAME));
	killsenetgame(newgame);

	return 0;
}

int senet_stuck(SENETGAME *game)
{
	MOVE *moves;
	int N;

	moves = senet_listmoves(game, &N, game->roll);
	free(moves);
	return (N == 0) ? 1 : 0;
}

int senet_choosemove(SENETGAME *sg, MOVE *move, int dicethrow)
{
	MOVE *moves;
	int N;
	int answer;
	int choice = 0;
	int best = -100000;
	int i;
	SENETGAME tempgame;
	int s1, s2;

	answer = 0;
	moves = senet_listmoves(sg, &N, dicethrow);
	if (N)
	{
		for (i = 0; i < N; i++)
		{
			memcpy(&tempgame, sg, sizeof(SENETGAME));
			senet_applymove(&tempgame, &moves[i]);
			score(&tempgame, &s1, &s2);
			if (s2 -s1 > best)
			{
				best = s1 - s2;
				choice = i;
			}	
		}
		*move = moves[choice];
	}
	else
	{
		move->from = 0;
		move->to = 0;
		answer = -1;
	}
	free(moves);
	return answer;
}

MOVE *senet_listmoves(SENETGAME *sg, int *N, int dicethrow)
{
	int y, x;
	MOVE move;
	void *temp;
	int Nmoves = 0;
	MOVE *answer = 0;

	for (y = 0; y < 3; y++)
	{
		for (x = 0; x < 10; x++)
		{
			if (sg->board[y][x] == sg->togo)
			{
				move.from = senet_xytoindex(x, y);
				if (y < 2 || x < 5 )
					move.to = move.from + dicethrow;
				else if (x == 5)
				{
					if (dicethrow > 4)
						continue;
					else
						move.to = move.from + dicethrow;
				}
				else if (x == 6)
				{
					continue;
				}
				else if (x == 7)
				{
					if (dicethrow == 2)
						move.to = move.from + dicethrow;
					else
						move.to = move.from - dicethrow;
				}
				else if (x == 8)
				{
					if (dicethrow == 1)
						move.to = move.from + dicethrow;
					else
						move.to - move.from - dicethrow;
				}
				if (senet_moveallowed(sg, &move))
				{
					temp = realloc(answer, (Nmoves + 1) * sizeof(MOVE));
					if (!temp)
						goto out_of_memory;
					answer = temp;
					answer[Nmoves++] = move;
				}
			}
		}
	}

	*N = Nmoves;
	return answer;
out_of_memory:
	*N = 0;
	free(answer);
	return 0;
}

int senet_applyroll(SENETGAME *game, int roll)
{
	game->roll = roll;
	if (game->state == SENET_WAITING_ROLL1)
		game->state = SENET_WAITING_P1;
	else if (game->state == SENET_WAITING_ROLL2)
		game->state = SENET_WAITING_P2;
	game->togo = game->state == SENET_WAITING_P1 ? 1 : 2;

	return 0;
}

int senet_applymove(SENETGAME *game, MOVE *move)
{
	int fromx, fromy;
	int tox, toy;
	int coloura;
	int colourb;
	int temp;
	int Nwhite, Nblack;

	if (move->from != 0 || move->to != 0)
	{
		senet_indextoxy(move->from, &fromx, &fromy);
		/* move backwards */
		if (move->to == 26)
		{
			temp = 15;
			do
			{
				temp--;
				senet_indextoxy(temp, &tox, &toy);
			} while (game->board[toy][tox]);
		}
		else 
			senet_indextoxy(move->to, &tox, &toy);

		coloura = game->board[fromy][fromx];
		colourb = game->board[toy][tox];
		game->board[fromy][fromx] = colourb;
		game->board[toy][tox] = coloura;
		if (move->to == 29)
			game->board[2][9] = 0;
	}

	if (game->state == SENET_WAITING_P1)
		game->state = SENET_WAITING_ROLL2;
	else if (game->state == SENET_WAITING_P2)
		game->state = SENET_WAITING_ROLL1;
	senet_pawnshome(game, &Nwhite, &Nblack);
	if (Nwhite == 7 || Nblack == 7)
		game->state = SENET_GAME_OVER;

	return 0;
}

int senet_moveallowed(SENETGAME *game, MOVE *move)
{
	if (move->from != 27 && move->from != 28 && move->to - move->from != game->roll)
		return 0;
	if (move->from == 27 && move->to == 29 && game->roll == 2)
		return 1;
	if (move->from == 28 && move->to == 29 && game->roll == 1)
		return 1;
	if ( (move->from == 27 || move->from == 28) && move->to != move->from - game->roll)
		return 0;
	return moveallowed(game, move->from, move->to);
}

int senet_pawnshome(SENETGAME *game, int *Nwhite, int *Nblack)
{
	int x, y;
	int Nw = 0;
	int Nb = 0;

	for (y = 0; y < 3; y++)
	{
		for (x = 0; x < 10; x++)
		{
			if (game->board[y][x] == 1)
				Nw++;
			else if (game->board[y][x] == 2)
				Nb++;
		}
	}
	if (Nwhite)
		*Nwhite = 7 - Nw;
	if (Nblack)
		*Nblack = 7 - Nb;

	return 0;
}

int senet_throwsticks(int *a, int *b, int *c, int *d)
{
	int answer;
	*a = coin();
	*b = coin();
	*c = coin();
	*d = coin();
	answer = *a + *b + *c + *d;
	if (answer == 0)
		answer = 5;
	return answer;
}

static int moveallowed(SENETGAME *sg, int from, int to)
{
	int fromx, fromy;
	int tox, toy;

	if (to >= 30)
		return 0;
	if (sg->state != SENET_WAITING_P1 && sg->state != SENET_WAITING_P2)
		return 0;
	senet_indextoxy(from, &fromx, &fromy);
	senet_indextoxy(to, &tox, &toy);
	if (sg->board[fromy][fromx] == sg->board[toy][tox])
		return 0;
	if (from < 25 && to > 26)
		return 0;
	if (from == 27 && to == 28)
		return 0;
	if (pawnblocked(sg, to))
		return 0;
	if (passesthree(sg, from, to))
		return 0;
	return 1;


	return 0;
}

int senet_xytoindex(int x, int y)
{
	if (y == 0)
		return x;
	if (y == 1)
		return 19 - x;
	if (y == 2)
		return x + 20;
	return -1;
}

int senet_indextoxy(int index, int *x, int *y)
{
	if (index < 10)
	{
		*x = index;
		*y = 0;
	}
	else if (index < 20)
	{
		*x = 19 - index;
		*y = 1;
	}
	else if (index < 30)
	{
		*x = index - 20;
		*y = 2;
	}
	return 0;
}

static int score(SENETGAME *game, int *s1, int *s2)
{
	int x, y;
	int score1 = 0;
	int score2 = 0;
	int blockscore;
	int hadpawn1 = 0;
	int hadpawn2 = 0;
	int Nhome1;
	int Nhome2;
	int i;

	for (i = 0; i < 30; i++)
	{
		senet_indextoxy(i, &x, &y);
		if (pawnblocked(game, i))
			blockscore = i;
		else
			blockscore = 0;
		if (game->board[y][x] == 1)
		{
			score1 += i + blockscore * hadpawn2;
			hadpawn1 = 1;
		}
		if (game->board[y][x] == 2)
		{
			score2 += i + blockscore * hadpawn1;
			hadpawn2 = 1;
		}
	}
	senet_pawnshome(game, &Nhome1, &Nhome2);
	score1 += Nhome1 * 100;
	score2 += Nhome2 * 100;
	*s1 = score1;
	*s2 = score2;

	return 0;
}
static int pawnblocked(SENETGAME *game, int index)
{
	int x, y;
	int colour;

	senet_indextoxy(index, &x, &y);
	colour = game->board[y][x];
	if (colour == 0)
		return 0;
	if (index > 0)
	{
		senet_indextoxy(index - 1, &x, &y);
		if (game->board[y][x] == colour)
			return 1;
	}
	if (index < 30 - 1)
	{
		senet_indextoxy(index + 1, &x, &y);
		if (game->board[y][x] == colour)
			return 1;
	}
	return 0;
}

static int passesthree(SENETGAME *game, int from, int to)
{
	int fromx, fromy;
	int x, y;
	int colour;
	int count = 0;
	int pos;

	senet_indextoxy(from, &fromx, &fromy);
	colour = game->board[fromy][fromx];
	for (pos = from + 1; pos <= to; pos++)
	{
		senet_indextoxy(pos, &x, &y);
		if (game->board[y][x] == 0)
			count = 0;
		else if (game->board[y][x] != colour)
			count++;
		if (count >= 3)
			return 1;
	}

	return 0;
}