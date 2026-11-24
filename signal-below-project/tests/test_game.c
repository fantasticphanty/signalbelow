#include <assert.h>
#include <stdio.h>
#include "game.h"
static void play(Game *g,const char *s){
    for(;*s;s++){
        assert(game_move(g,*s=='R'?1:*s=='L'?-1:0,*s=='D'?1:*s=='U'?-1:0));
    }
}
int main(void){
    Game g;
    game_reset(&g);
    assert(!g.mapped && g.moves==0 && !game_move(&g,1,1));
    play(&g,"D");
    assert(!game_move(&g,0,1) && g.moves==1 && !g.mapped);
    game_reset(&g);
    assert(game_pulse(&g) && g.mapped && !game_pulse(&g));
    play(&g,"UU");
    assert(!game_pulse(&g) && g.cooldown==1);
    play(&g,"U");
    assert(g.cooldown==0 && game_pulse(&g));
    game_reset(&g);
    assert(game_pulse(&g));
    play(&g,"UUURRRRUURRRRRRRR");
    assert(g.result==ESCAPED && g.echo==0 && g.mapped && g.moves==17);
    assert(!game_move(&g,1,0) && !game_pulse(&g));
    game_reset(&g);
    assert(game_pulse(&g));
    play(&g,"RRRRURRR");
    assert(g.result==CAUGHT && g.moves==8);
    game_reset(&g);
    play(&g,"UUURRRRUURRRRRRRR");
    assert(g.result==DIVE && !g.mapped && g.ex==12 && g.ey==7);
    puts("Native core: walls, ping, cooldown, win, loss, and restart passed.");
}
