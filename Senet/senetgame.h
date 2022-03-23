#ifndef senetgame_h
#define senetgame_h

#define SENET_WAITING_ROLL1 1
#define SENET_WAITING_P1 2
#define SENET_WAITING_ROLL2 3
#define SENET_WAITING_P2 4
#define SENET_GAME_OVER 5

typedef struct
{
	int from;
	int to;
} MOVE;

typedef struct
{
	unsigned char board[3][10];
	int togo;
	int roll;
	int state;
} SENETGAME;

SENETGAME *senetgame(int playertogo);
int senet_resetgame(SENETGAME *sg, int togo);
int senet_stuck(SENETGAME *game);
int senet_choosemove(SENETGAME *sg, MOVE *move, int dicethrow);
MOVE *senet_listmoves(SENETGAME *sg, int *N, int dicethrow);
int senet_moveallowed(SENETGAME *game, MOVE *move);
int senet_applymove(SENETGAME *game, MOVE *move);
int senet_pawnshome(SENETGAME *game, int *Nwhite, int *Nblack);
int senet_xytoindex(int x, int y);
int senet_indextoxy(int index, int *x, int *y);
int senet_throwsticks(int *a, int *b, int *c, int *d);
int senet_applyroll(SENETGAME *game, int roll);

#endif