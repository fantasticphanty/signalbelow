#include <nds.h>
#include <stdint.h>
#include "game.h"

#define C(r,g,b) ((uint16_t)(0x8000|((r)&31)|(((g)&31)<<5)|(((b)&31)<<10)))
enum { W=256,H=192, TS=14, X0=16, Y0=26 };
static uint16_t top[W*H] __attribute__((aligned(32))), bottom[W*H] __attribute__((aligned(32)));
static uint16_t *dst;
static int8_t ping[4096] __attribute__((aligned(32)));
static const uint8_t glyph[][5] = {
 {0x7e,0x09,0x09,0x09,0x7e},{0x7f,0x49,0x49,0x49,0x36},{0x3e,0x41,0x41,0x41,0x22},
 {0x7f,0x41,0x41,0x22,0x1c},{0x7f,0x49,0x49,0x49,0x41},{0x7f,0x09,0x09,0x09,0x01},
 {0x3e,0x41,0x49,0x49,0x7a},{0x7f,0x08,0x08,0x08,0x7f},{0x41,0x41,0x7f,0x41,0x41},
 {0x20,0x40,0x41,0x3f,0x01},{0x7f,0x08,0x14,0x22,0x41},{0x7f,0x40,0x40,0x40,0x40},
 {0x7f,0x02,0x0c,0x02,0x7f},{0x7f,0x04,0x08,0x10,0x7f},{0x3e,0x41,0x41,0x41,0x3e},
 {0x7f,0x09,0x09,0x09,0x06},{0x3e,0x41,0x51,0x21,0x5e},{0x7f,0x09,0x19,0x29,0x46},
 {0x26,0x49,0x49,0x49,0x32},{0x01,0x01,0x7f,0x01,0x01},{0x3f,0x40,0x40,0x40,0x3f},
 {0x1f,0x20,0x40,0x20,0x1f},{0x3f,0x40,0x38,0x40,0x3f},{0x63,0x14,0x08,0x14,0x63},
 {0x07,0x08,0x70,0x08,0x07},{0x61,0x51,0x49,0x45,0x43},
 {0x3e,0x51,0x49,0x45,0x3e},{0x00,0x42,0x7f,0x40,0x00},{0x42,0x61,0x51,0x49,0x46},
 {0x21,0x41,0x45,0x4b,0x31},{0x18,0x14,0x12,0x7f,0x10},{0x27,0x45,0x45,0x45,0x39},
 {0x3c,0x4a,0x49,0x49,0x30},{0x01,0x71,0x09,0x05,0x03},{0x36,0x49,0x49,0x49,0x36},
 {0x06,0x49,0x49,0x29,0x1e},{0,8,8,8,0},{0,0x60,0x60,0,0},{0x20,0x10,8,4,2}
};
static void box(int x,int y,int w,int h,uint16_t c) {
    int x1=x+w,y1=y+h;
    if(x<0)x=0;
    if(y<0)y=0;
    if(x1>W)x1=W;
    if(y1>H)y1=H;
    for(int j=y;j<y1;j++) for(int i=x;i<x1;i++) dst[j*W+i]=c;
}
static void edge(int x,int y,int w,int h,uint16_t c) {
    box(x,y,w,1,c); box(x,y+h-1,w,1,c); box(x,y,1,h,c); box(x+w-1,y,1,h,c);
}
static void pix(int x,int y,uint16_t c) {if((unsigned)x<W&&(unsigned)y<H)dst[y*W+x]=c;}
static void label(int x,int y,int s,uint16_t c,const char* word) {
    for(;*word;word++,x+=6*s) {
        char ch=*word;
        int k=ch>='A'&&ch<='Z'?ch-'A':ch>='0'&&ch<='9'?ch-'0'+26:ch=='-'?36:ch=='.'?37:ch=='/'?38:-1;
        if(k>=0)for(int a=0;a<5;a++)for(int b=0;b<7;b++)if(glyph[k][a]&(1<<b))box(x+a*s,y+b*s,s,s,c);
    }
}
static uint32_t noise(unsigned x,unsigned y) {
    uint32_t n=x*295729u+y*1580030173u+2999u;
    n=(n^(n>>16))*1597334677u;
    return n^(n>>15);
}
static int mag(int x){return x<0?-x:x;}
static void ring(int cx,int cy,int r,uint16_t c){
    int x=r,y=0,e=1-r;
    while(x>=y){
        pix(cx+x,cy+y,c);pix(cx+y,cy+x,c);pix(cx-y,cy+x,c);pix(cx-x,cy+y,c);
        pix(cx-x,cy-y,c);pix(cx-y,cy-x,c);pix(cx+y,cy-x,c);pix(cx+x,cy-y,c);
        ++y;if(e<0)e+=2*y+1;else{--x;e+=2*(y-x)+1;}
    }
}
static void diver(const Game*g,int x,int y){
    const uint16_t dark=C(9,10,7), amber=C(31,26,13), ochre=C(23,14,6), glass=C(12,30,28);
    box(x+1,y+5,2,6,ochre);box(x+3,y+2,7,8,amber);box(x+4,y+1,5,1,amber);
    box(x+(g->facing>0?6:3),y+3,4,3,glass);box(x+4,y+8,5,3,ochre);
    box(x+3,y+10,2,2,amber);box(x+8,y+10,2,2,amber);
    pix(x+11,y+2,dark);pix(x+12,y+1,glass);pix(x+11,y-1,glass);
}
static void creature(int x,int y,bool seen){
    uint16_t body=seen?C(19,4,8):C(10,2,6), eye=seen?C(31,13,15):C(21,7,10);
    box(x+2,y+4,10,6,body);box(x+4,y+2,6,3,body);
    box(x+1,y+3,3,1,body);box(x+10,y+3,3,1,body);
    pix(x+4,y+6,eye);pix(x+9,y+6,eye);
    box(x+2,y+10,2,2,body);box(x+6,y+9,2,3,body);box(x+10,y+10,2,2,body);
}
static void world(const Game*g) {
    dst=top;box(0,0,W,H,C(1,3,5));box(0,0,W,22,C(2,5,8));box(0,21,W,1,C(5,13,16));
    label(9,7,1,C(12,29,26),"SIGNAL BELOW");label(174,7,1,C(7,16,20),"SECTOR 01");
    edge(X0-2,Y0-2,GW*TS+4,GH*TS+4,C(5,14,17));
    for(int y=0;y<GH;y++)for(int x=0;x<GW;x++){
        int px=X0+x*TS, py=Y0+y*TS;
        int d=mag(g->x-x)+mag(g->y-y), od=mag(g->origin_x-x)+mag(g->origin_y-y);
        int lit=d<4?5-d:0;
        bool on=g->echo>0&&od<=(1+2*(ECHO_TURNS-g->echo));
        uint32_t n=noise(x,y);
        if(wall(x,y)){
            box(px,py,TS,TS,C(2+lit/2,5+lit,7+lit));
            box(px+1,py+1,12,3,C(3+lit/2,7+lit,9+lit));
            box(px+1,py+4,2,8,C(3+lit/2,6+lit,9+lit));
            box(px+7,py+8,5,3,C(2,4+lit/2,6+lit/2));
        }else{
            box(px,py,TS,TS,C(1+lit/3,3+lit+(on?2:0),6+lit+(on?3:0)));
            if(on)edge(px,py,TS,TS,C(2,8,11));
        }
        for(int i=0;i<6;i++){n=n*1664525u+1013904223u;pix(px+1+(n>>26)%12,py+1+(n>>20)%12,C(2+lit/3,5+lit+(n&1),8+lit));}
        if(!wall(x,y)&&((n>>8)&15)==0){box(px+10,py+9,1,3,C(4,9+lit,11+lit));pix(px+11,py+8,C(4,10+lit,12+lit));}
    }
    if(g->mapped) {
        int x=X0+EXIT_X*TS,y=Y0+EXIT_Y*TS;
        edge(x+1,y+1,12,12,C(14,31,24));box(x+4,y+3,6,8,C(6,19,16));
        box(x+5,y+5,4,4,C(14,30,23));pix(x+8,y+7,C(31,31,21));
    }
    if(g->mapped&&(g->echo||mag(g->x-g->ex)+mag(g->y-g->ey)<=3))
        creature(X0+g->ex*TS,Y0+g->ey*TS,g->echo!=0);
    diver(g,X0+g->x*TS,Y0+g->y*TS);
    label(10,176,1,g->mapped?C(14,28,23):C(21,22,14),
          g->mapped?"HATCH MAPPED / REACH IT":"HATCH HIDDEN / PING TO MAP");
    if(g->result!=DIVE) {
        const bool won=g->result==ESCAPED;
        box(22,63,212,61,C(2,5,7));edge(22,63,212,61,won?C(10,24,19):C(23,8,12));
        label(won?44:55,75,2,won?C(16,31,24):C(31,14,16),won?"YOU MADE IT OUT":"SIGNAL LOST");
        label(won?58:52,104,1,C(15,22,23),won?"SALVAGE DIVE COMPLETE":"IT FOLLOWED YOUR ECHO");
    }
}
static void sonar(const Game*g) {
    dst=bottom;box(0,0,W,H,C(1,4,6));box(0,0,W,22,C(2,6,8));box(0,21,W,1,C(5,13,16));
    label(9,7,1,C(12,29,26),"ACOUSTIC CONSOLE");label(181,7,1,C(7,16,20),"DIVE UNIT");
    edge(11,29,130,124,C(5,15,16));box(12,30,128,122,C(1,5,7));
    int cx=76,cy=91;
    for(int yy=-51;yy<=51;yy++)for(int xx=-51;xx<=51;xx++)if(xx*xx+yy*yy<51*51){
        uint32_t n=noise(xx+60,yy+61);
        if((n&63)==0)pix(cx+xx,cy+yy,C(4,13,13));
        if(!xx||!yy)pix(cx+xx,cy+yy,C(3,11,12));
    }
    for(int r=13;r<=51;r+=12)ring(cx,cy,r,C(3,12,13));
    if(g->echo)ring(cx,cy,13+5*(ECHO_TURNS-g->echo),C(8,30,24));
    box(cx-1,cy-1,3,3,C(15,31,25));
    if(g->mapped){
        int dx=(EXIT_X-g->x)*3,dy=(EXIT_Y-g->y)*3;
        edge(cx+dx-2,cy+dy-2,5,5,C(16,31,20));
    }
    if(g->echo&&g->mapped){
        int dx=(g->ex-g->x)*3,dy=(g->ey-g->y)*3;
        box(cx+dx-1,cy+dy-1,3,3,C(31,11,12));
    }
    label(150,38,1,C(10,20,22),"HATCH STATUS");
    label(150,52,1,g->mapped?C(12,29,24):C(27,23,12),g->mapped?"MAPPED":"UNKNOWN");
    label(150,74,1,C(10,20,22),"SONAR CHARGE");
    for(int i=0;i<3;i++)box(150+i*31,89,27,5,i<3-g->cooldown?C(8,26,22):C(4,10,12));
    if(g->result==DIVE) {
        box(148,105,95,43,g->cooldown?C(4,9,11):C(4,16,16));
        edge(148,105,95,43,g->cooldown?C(8,12,13):C(9,27,22));
        label(172,115,2,g->cooldown?C(10,14,15):C(13,31,24),g->cooldown?"WAIT":"PING");
        label(158,137,1,C(9,20,20),g->cooldown?"KEEP MOVING":"TAP PANEL / A");
        label(10,169,1,C(17,23,23),"D-PAD MOVE   TOUCH PING");
        label(10,181,1,C(8,16,18),"START RESTART / PINGS ATTRACT IT");
    }else{
        label(151,112,1,C(14,28,23),"DIVE ENDED");
        label(151,130,1,C(14,20,23),"START RETRY");
        label(10,175,1,C(14,22,23),"PRESS START TO DIVE AGAIN");
    }
}
static void blit(uint16_t *vtop,uint16_t *vbottom){
    DC_FlushRange(top,sizeof(top)); DC_FlushRange(bottom,sizeof(bottom));
    dmaCopy(top,vtop,sizeof(top)); dmaCopy(bottom,vbottom,sizeof(bottom));
}
int main(void) {
    videoSetMode(MODE_5_2D); videoSetModeSub(MODE_5_2D);
    vramSetBankA(VRAM_A_MAIN_BG); vramSetBankC(VRAM_C_SUB_BG); lcdMainOnTop();
    const int a=bgInit(3,BgType_Bmp16,BgSize_B16_256x256,0,0);
    const int b=bgInitSub(3,BgType_Bmp16,BgSize_B16_256x256,0,0);
    uint16_t *vtop=bgGetGfxPtr(a), *vbottom=bgGetGfxPtr(b);
    for(unsigned i=0;i<sizeof(ping);i++){
        int phase=(i*13+i*i/900)&255;
        int wave=(phase<128?phase:255-phase)-64;
        ping[i]=(int8_t)(wave*(4096-(int)i)/4096);
    }
    DC_FlushRange(ping,sizeof(ping)); soundEnable();
    Game g;game_reset(&g); bool redraw=true; int delay=0,old_dir=0;
    while(1){
        if(redraw){world(&g);sonar(&g);}
        swiWaitForVBlank();
        if(redraw){blit(vtop,vbottom);redraw=false;}
        scanKeys();
        int down=keysDown(),held=keysHeld();
        if(down&KEY_START){game_reset(&g);delay=0;old_dir=0;redraw=true;continue;}
        if(g.result!=DIVE)continue;
        bool fire=(down&KEY_A)!=0;
        if(down&KEY_TOUCH){touchPosition t;touchRead(&t);fire|=t.px>=11&&t.px<244&&t.py>=29&&t.py<153;}
        if(fire&&game_pulse(&g)){
            redraw=true;soundPlaySample(ping,SoundFormat_8Bit,sizeof(ping),16000,80,64,false,0);
        }
        int dir=held&KEY_UP?1:held&KEY_DOWN?2:held&KEY_LEFT?3:held&KEY_RIGHT?4:0;
        if(dir!=old_dir)delay=0;
        if(dir&&delay==0){
            Result prior=g.result;
            redraw|=game_move(&g,dir==4?1:dir==3?-1:0,dir==2?1:dir==1?-1:0);
            delay=8;
            if(g.result!=prior)
                soundPlaySample(ping,SoundFormat_8Bit,sizeof(ping),g.result==ESCAPED?20000:6600,85,64,false,0);
        }
        if(delay)--delay;
        old_dir=dir;
    }
}
