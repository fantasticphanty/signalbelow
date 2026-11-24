#ifndef SIGNAL_GAME_H
#define SIGNAL_GAME_H
#include <stdbool.h>
enum { GW=16, GH=10, EXIT_X=14, EXIT_Y=2, ECHO_TURNS=8, COOLDOWN=3 };
typedef enum { DIVE, ESCAPED, CAUGHT } Result;
typedef struct {
    int x,y, ex,ey, origin_x,origin_y, moves, cooldown, echo, wake_moves, pulses, facing;
    bool mapped;
    Result result;
} Game;
void game_reset(Game *g);
bool wall(int x,int y);
bool game_pulse(Game *g);
bool game_move(Game *g,int dx,int dy);
#endif
