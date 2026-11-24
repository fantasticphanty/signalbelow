#include "game.h"
void game_reset(Game *g) {
    *g=(Game){.x=2,.y=7,.ex=12,.ey=7,.origin_x=2,.origin_y=7,.facing=1};
}
bool wall(int x,int y) {
    return x<=0 || x>=GW-1 || y<=0 || y>=GH-1 ||
        (x==5 && y<=6 && y!=4) || (x==10 && y>=3 && y!=6);
}
bool game_pulse(Game *g) {
    if (g->result!=DIVE || g->cooldown) return false;
    g->mapped=true; g->echo=ECHO_TURNS; g->cooldown=COOLDOWN;
    g->origin_x=g->x; g->origin_y=g->y; ++g->pulses;
    return true;
}
static void chase(Game *g) {
    static const int dx[]={1,0,-1,0},dy[]={0,-1,0,1};
    unsigned char dist[GW*GH]={0}, q[GW*GH];
    unsigned head=0,tail=0; int p=g->y*GW+g->x;
    dist[p]=1; q[tail++]=p;
    while (head<tail) {
        int i=q[head++], x=i%GW,y=i/GW;
        for (int k=0;k<4;k++) {
            int nx=x+dx[k],ny=y+dy[k],j=ny*GW+nx;
            if (!wall(nx,ny) && !dist[j]) {
                dist[j]=dist[i]+1; q[tail++]=j;
            }
        }
    }
    int best=dist[g->ey*GW+g->ex], bx=g->ex,by=g->ey;
    if (!best) return;
    for (int k=0;k<4;k++) {
        int x=g->ex+dx[k],y=g->ey+dy[k],j=y*GW+x;
        if (!wall(x,y) && dist[j] && dist[j]<best) {
            best=dist[j]; bx=x; by=y;
        }
    }
    g->ex=bx; g->ey=by;
}
bool game_move(Game *g,int dx,int dy) {
    if (g->result!=DIVE || ((dx<0?-dx:dx)+(dy<0?-dy:dy)!=1) ||
        wall(g->x+dx,g->y+dy)) return false;
    g->x+=dx; g->y+=dy; ++g->moves;
    if(dx) g->facing=dx;
    if(g->echo) --g->echo;
    if(g->cooldown) --g->cooldown;
    if(g->mapped && g->x==g->ex && g->y==g->ey) {g->result=CAUGHT;return true;}
    if(g->mapped && g->x==EXIT_X && g->y==EXIT_Y) {g->result=ESCAPED;return true;}
    if(g->mapped && !(++g->wake_moves&1)) chase(g);
    if(g->mapped && g->x==g->ex && g->y==g->ey) g->result=CAUGHT;
    return true;
}
