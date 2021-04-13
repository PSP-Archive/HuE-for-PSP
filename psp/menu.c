//-----------------------------------------------------------------------------
//
//
//-----------------------------------------------------------------------------
#include <stdio.h>

#include "stdinc.h"
#include "main.h"
#include "pg.h"
#include "string.h"
#include "filer.h"
#include "hue_image.h"
#include "pce.h"

#include "pspstd.h"
#include "psp_main.h"
#include "syscall.h"

int StateSave(int slot,void* buf,int size);
int StateLoad(int slot,void* buf,int size);

typedef struct {
    int img_tag;
    int img_size;
    short w,h;
    short pic[256*256];

    int io_tag;
    int io_size;
    IO  io;
} SCRN;

typedef struct {
    int   key;
    char* name;
} KEYDEF;

typedef struct {
    unsigned char r,g,b;
} COLOR;

typedef struct {
    int x,y,c;
} POINT;

#define QSLOT_MAX  15

// quick slot
static SCRN* qslot[QSLOT_MAX];

#define MSG_YES  1
#define MSG_NO   0

//-----------------------------------------------------------------------------
// 
// YES/NO 
// 
//-----------------------------------------------------------------------------
int MsgBox(char* msg,int def_yn)
{
    int len = strlen(msg)*10;
    int sx,sy;
    int npad,bUp=1;
    int sel = def_yn;

    sx = (SCREEN_WIDTH - len)/2;
    sy = (SCREEN_HEIGHT - len - 10)/2 - 20;

    while(readpad_now()){
        pgWaitVn(1);
    }

    while(1) {

        if((npad = readpad_new())==0 && !bUp){
            pgWaitVn(1);
            bUp=0;
            continue;
        }

        if(npad & (CTRL_LEFT|CTRL_RIGHT)) sel=1-sel;
        if(npad & (CTRL_CIRCLE)) break;
           
        pgFillBox  (sx-5,sy-5,sx+len+5,sy+20,RGB_BLUE);
        pgDrawFrame(sx-5,sy-5,sx+len+5,sy+20,RGB_WHITE);
        mh_print(sx,sy,msg,-1);
        mh_print(sx   ,sy+10,"YES",(sel==1)?RGB_RED:RGB_WHITE);
        mh_print(sx+50,sy+10,"NO ",(sel==0)?RGB_RED:RGB_WHITE);

        if(PSP_Is()){
            sceKernelDcacheWritebackAll();
        }
        pgScreenFlipV();
    }
    return sel;
}


//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
void make_screen(SCRN* p)
{
    // XBuf Draw
    extern int og_x,og_y,og_w,og_h;
    int i,j;
    short* ptr=p->pic;

    p->w = (og_w>256)? 256: og_w;
    p->h = (og_h>256)? 256: og_h;
    memset(p->pic,0,sizeof(p->pic));
    
    for(j=0;j<p->h;j++) {
        register WORD* src = (WORD*)&XBuf[WIDTH*(j+og_y)+og_x];
        for(i=0;i<p->w;i++) {
            *ptr++ = src[i];
        }
    }
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
void qinit(void)
{
    memset(qslot,0,sizeof(qslot));
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
SCRN* qsave2(void)
{
    SCRN* p;
    
    if((p = malloc(sizeof(SCRN)))) {
        p->img_tag = 0x00000008;
        p->img_size= sizeof(short)*(2+256*256);
        make_screen(p);
        
        p->io_tag  = 0x00000010;
        p->io_size = sizeof(io);
        memcpy(&p->io,&io,sizeof(io));

        return p;
    }
    return 0;
}

//-----------------------------------------------------------------------------
// 
//-----------------------------------------------------------------------------
int qload2(SCRN* pSlot)
{
    int i;
    
    if(pSlot) {
        
        memcpy(&io,&pSlot->io,sizeof(io));
        
        for(i=0;i<8;i++) {
            bank_set(i,io.m6502.MPR[i]);
        }
        
        memset(vchange, 1,sizeof(vchange));
        memset(vchanges,1,sizeof(vchanges));
        memset(VRAM2,0,sizeof(VRAM2));
        memset(VRAMS,0,sizeof(VRAMS));
        return 0;
    }
    return -1;
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
int qfree2(SCRN* pSlot)
{
    if(pSlot) {
        free(pSlot);
        return 0;
    }
    return -1;
}

/*
int qsave(int slot)
{
    if(qslot[slot]) {
        qfree(slot);
    }
    if(!qslot[slot] && (qslot[slot] = qsave2())) {
        return 0;
    }
    return -1;
}
*/

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
int qload(int slot)
{
    return qload2(qslot[slot]);
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
int qfree(int slot)
{
    if(qfree2(qslot[slot])==0) {
        qslot[slot]=0;
    }

    return -1;
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
int qclear(void)
{
    int i;
    for(i=0;i<QSLOT_MAX;i++) {
        if(qslot[i]) {
            free(qslot[i]);
            qslot[i]=0;
        }
    }

    for(i=0;i<QSLOT_MAX;i++) {
        qload_disk(i);
    }

    return 0;
}


//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
int qsave_disk(int slot)
{
    if(qslot[slot]) {
        free(qslot[slot]);
        qslot[slot]=0;
    }

    if((qslot[slot] = qsave2())) {
        if(StateSave(slot,qslot[slot],sizeof(SCRN))) {
            return 0;
        }
    }
    
    return -1;
}


//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
int qload_disk(int slot)
{
    SCRN* p;
    
    if(qslot[slot]) {
        free(qslot[slot]);
        qslot[slot]=0;
    }

    if((p = (SCRN*)malloc(sizeof(SCRN)))){
        if(StateLoad(slot,p,sizeof(SCRN))) {
            qslot[slot]=p;
            return 0;
        }
        if(p){
            free(p);
            p=0;
        }
    }
    return -1;
}


// global
EmuConfig eConf= {
    0,            // cpu clock
    1,            // vsync wait
    1,            // sound [off/stereo]
    0x00,         // frame skip timing
    0,            // debug
    0,            // pad number
    "",           // cdrom name
    0,            // video
    0,            // 6 button
    {
        // key config
        { // 2Button Mode
            JOY_UP    , // D-UP      
            JOY_RIGHT , // D-RIGHT   
            JOY_DOWN  , // D-DOWN    
            JOY_LEFT  , // D-LEFT    
            JOY_UP    , // A-UP      
            JOY_RIGHT , // A-RIGHT   
            JOY_DOWN  , // A-DOWN    
            JOY_LEFT  , // A-LEFT    
            0         , // △        
            JOY_1     , // ○        
            JOY_2     , // ×        
            0         , // □        
            0         , // L-Trigger 
            0         , // R-Trigger 
            JOY_SELECT, // Select    
            JOY_RUN     // Start     
          },
        { // 6Button Mode
            JOY_UP    , // D-UP      
            JOY_RIGHT , // D-RIGHT   
            JOY_DOWN  , // D-DOWN    
            JOY_LEFT  , // D-LEFT    
            JOY_UP    , // A-UP      
            JOY_RIGHT , // A-RIGHT   
            JOY_DOWN  , // A-DOWN    
            JOY_LEFT  , // A-LEFT    
            JOY_5     , // △        
            JOY_1     , // ○        
            JOY_2     , // ×        
            JOY_6     , // □        
            JOY_3     , // L-Trigger 
            JOY_4     , // R-Trigger 
            JOY_SELECT, // Select    
            JOY_RUN     // Start     
          }
    },
    {   // Rapid
        {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
        {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}
    },
    {
        {-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,},
        {-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,}
    },
    0,  // autofire conf
    0,  // autofire 
    
}, pConf;


typedef struct {
//    int  num;
    char menu[128];
    char data[128];
    char **msg;
    int  min;
    int  max;
    int  *pObj;
} MENU_STRUCT;


static char *msgOnOff[3]={"Off","On",0};
static char *msgClock[4]={"333MHz","266MHz","222MHz",0};
static char *msgVideo[16]={"cpu","gpu","gpu 4:3","gpu(full)",0};
static char *msgPadNo[6]={ "Player 1", "Player 2", "Player 3", "Player 4", "Player 5",0 };
static char *msgSkip[] = {"no skip","auto","skip 4/8","skip 3/8","skip 2/8","skip 1/8",0};
static char *msgDebug[]= {"off","normal"};


static unsigned int default_rapid_map[4]={
    0xffffffff, // 11111111 11111111 11111111 11111111
    0x55555555, // 01010101 01010101 01010101 01010101
    0x49249249, // 01001001 00100100 10010010 01001001
    0x11111111  // 00010001 00010001 00010001 00010001
  };


#define    MENU_CPUCLOCK    0
#define    MENU_VSYNC       1
#define    MENU_VIDEO       2
#define    MENU_PADNO       3
#define    MENU_KEYCONFIG   4
#define    MENU_SOUND       5
#define    MENU_SKIP        6
#define    MENU_DEBUG       7
#define    MENU_ROM         8
#define    MENU_CDROM       9
#define    MENU_RESET      10
#define    MENU_QUIT       11
#define    MENU_CONT       12
#define    MENU_STRING     13

// 
static MENU_STRUCT psp_menu[] = {
    { "CPU Clock"          ,""                 , msgClock  , 0, 2, &eConf.clock },
    { "Vsync Wait"         ,""                 , msgOnOff  , 0, 1, &eConf.vsync },
    { "Video"              ,""                 , msgVideo  , 0, 3, &eConf.video },
    { "Control Pad No"     ,""                 , msgPadNo  , 0, 4, &eConf.padno },
    { "Key Config"         ,""                 , 0         , 0, 0, 0            },
    { "Sound"              ,""                 , msgOnOff  , 0, 1, &eConf.sound },
    { "Frameskip"          ,""                 , msgSkip   , 0, 5, &eConf.skip  },
    { "debug"              ,""                 , msgDebug  , 0, 1, &eConf.debug },
    { "rom select"         ,"ROM select"       , 0         , 0, 0, 0},
    { "cdrom select"       ,"CDROM select"     , 0         , 0, 0, 0},
    { "Reset"              ,"Reset"            , 0         , 0, 0, 0},
    { "exit"               ,"Quit HuE for PSP" , 0         , 0, 0, 0},
    { "continue"           ,"back to game"     , 0         , 0, 0, 0},
    {"","",0,0}
};

// 最大最小で止める
static int _round(int cur, int change,int min, int max)
{
    cur+=change;
    if(cur<min) { cur=min; }
    if(cur>max) { cur=max; }
    return cur;
}

// 最大最小で反対側へいく
static int _loop(int cur, int change,int min, int max)
{
    cur+=change;
    if(cur<min) { cur=max; }
    if(cur>max) { cur=min; }
    return cur;
}

extern void enable_sound(int);
extern void image_config_update(void);

extern unsigned short g_bgBitmap[480*272];

#define PSP_KEYMAX 16
#define PCE_KEYMAX 14
COLOR PSPMAP[PSP_KEYMAX];
COLOR PCEMAP[PCE_KEYMAX];
POINT PSPPOINT[PSP_KEYMAX];
POINT PCEPOINT[PCE_KEYMAX];


unsigned char g_bgMask[480*272];
int bg_SX=108;
int bg_SY=85;
int bg_SXSY=0;

//-----------------------------------------------------------------------------
// Load Menu Bitmap
//-----------------------------------------------------------------------------
void load_mask(char* name)
{
	unsigned char *menu_bg;
	unsigned char *vptr;
    int    menu_bg_siz = 480*272*3+0x36;
    char * menu_bg_buf = 0;
	char BgPath[MAX_PATH];
 	unsigned short x,y,yy;
    int nRead;
    FILE* fp;
    int i;
    unsigned char log_r[3]={0,0,0};
    unsigned char log_g[3]={0,0,0};
    unsigned char log_b[3]={0,0,0};
    unsigned r,g,b;
    
    memset(PSPPOINT,0,sizeof(PSPPOINT));
    memset(PCEPOINT,0,sizeof(PCEPOINT));

    strcpy(BgPath,pguGetWorkdir());
    strcat(BgPath,name);

    memset(PSPMAP,0,sizeof(PSPMAP));

    if( (menu_bg_buf = malloc(menu_bg_siz)) ) {
        if((fp=fopen(BgPath,"r"))>0) {
            nRead = fread(menu_bg_buf,1,menu_bg_siz,fp);
            fclose(fp);
            
            if(nRead==menu_bg_siz) {
                menu_bg = menu_bg_buf + 0x36;
                vptr=g_bgMask;
#if 1
                // PSPコントローラのMASK値を取得
                for(i=0;i<PSP_KEYMAX;i++) {
                    PSPMAP[i].r = menu_bg[(271*480+i)*3+2];
                    PSPMAP[i].g = menu_bg[(271*480+i)*3+1];
                    PSPMAP[i].b = menu_bg[(271*480+i)*3+0];
                }

                // PCEコントローラのMASK値を取得
                for(i=0;i<PCE_KEYMAX;i++) {
                    PCEMAP[i].r = menu_bg[(270*480+i)*3+2];
                    PCEMAP[i].g = menu_bg[(270*480+i)*3+1];
                    PCEMAP[i].b = menu_bg[(270*480+i)*3+0];
                }
#endif
                for(y=0;y<272;y++){
                    for(x=0;x<480;x++){
                        yy = 271 - y;
                        *vptr++ = 0x10;

                        if(y<2) continue;

                        r = menu_bg[(yy*480+x)*3+2];
                        g = menu_bg[(yy*480+x)*3+1];
                        b = menu_bg[(yy*480+x)*3+0];

                        if(!r&&!g&&!b) continue;

                        if(!bg_SXSY) {
                            log_r[2]=log_r[1]; log_r[1]=log_r[0]; log_r[0] = r;
                            log_g[2]=log_g[1]; log_g[1]=log_g[0]; log_g[0] = g;
                            log_b[2]=log_b[1]; log_b[1]=log_b[0]; log_b[0] = b;

                            // 描画開始ポイントを取得
                            if((log_r[2]==255 && log_g[2]==0   && log_b[2]==0  ) &&
                               (log_r[1]==0   && log_g[1]==255 && log_b[1]==0  ) &&
                               (log_r[0]==0   && log_g[0]==0   && log_b[0]==255) ) {
                                bg_SX = x-3;
                                bg_SY = y;
                                bg_SXSY = 1;
                            }
                        }

                        for(i=0;i<PCE_KEYMAX;i++){
                            if( PCEMAP[i].r==r && PCEMAP[i].g==g && PCEMAP[i].b==b && y) {
                                *(vptr-1)=i + 0x20;
                                PCEPOINT[i].x+=x;
                                PCEPOINT[i].y+=y;
                                PCEPOINT[i].c++;
                                break;
                            }
                        }
                        
                        for(i=0;i<PSP_KEYMAX;i++) {
                            if( PSPMAP[i].r==r && PSPMAP[i].g==g && PSPMAP[i].b==b && y) {
                                *(vptr-1)=i;
                                PSPPOINT[i].x+=x;
                                PSPPOINT[i].y+=y;
                                PSPPOINT[i].c++;
                                break;
                            }
                        }
                    }
                }
            }

            for(i=0;i<PCE_KEYMAX;i++) {
                if(PCEPOINT[i].c) {
                    PCEPOINT[i].x /= PCEPOINT[i].c;
                    PCEPOINT[i].y /= PCEPOINT[i].c;
                }
            }

            for(i=0;i<PSP_KEYMAX;i++) {
                if(PSPPOINT[i].c) {
                    PSPPOINT[i].x /= PSPPOINT[i].c;
                    PSPPOINT[i].y /= PSPPOINT[i].c;
                }
            }
        }
        
        free(menu_bg_buf);
    }
}

KEYDEF keydef[] = {
    { JOY_UP    , ": UP     "},
    { JOY_RIGHT , ": RIGHT  "},
    { JOY_DOWN  , ": DOWN   "},
    { JOY_LEFT  , ": LEFT   "},
    { JOY_1     , ": I      "},
    { JOY_2     , ": II     "},
    { JOY_3     , ": II     "},
    { JOY_4     , ": IV     "},
    { JOY_5     , ": V      "},
    { JOY_6     , ": VI     "},
    { JOY_RUN   , ": RUN    "},
    { JOY_SELECT, ": SELECT "},
    { 0x80000000, ": menu   "},
    { 0         , ": -------"}
};

#define MODE_RAPIDMAX  (sizeof(rpdCvt)/sizeof(rpdCvt[0]))
char * rpdCvt[8] = {"x0","x1","x2","x3","a0","a1","a2","a3"};

char* PSP_KEY[] = {
    "D-UP      ", "D-RIGHT   ", "D-DOWN    " , "D-LEFT    " ,
    "A-UP      ", "A-RIGHT   ", "A-DOWN    ", "A-LEFT    ",
    "△        ", "○        ", "×        ", "□        ",
    "L-Trigger ", "R-Trigger ", "Select    ", "Start     " 
};

// KEY IDからINDEX値を取得
int findKeyDef(int key)
{
    int i;
    
    for(i=0;i<PCE_KEYMAX;i++) {
        if(keydef[i].key==key) {
            return i;
        }
    }
    return 0;
}

//-----------------------------------------------------------------------------
// KeyConfig
// トリガーキーで２／６ボタンモードの切り替え
//-----------------------------------------------------------------------------
void KeyConf(void)
{
    int i,j,m,x,y;
    int select=0,cursor=0;
    int npad,fst=0;
    int mode = eConf.button6;
    unsigned short * bgf = malloc(2*480*272);
    int pspX,pspY,pceX,pceY;
    
    load_mask("KEYm.bmp");
    load_menu_bg("KEY.bmp");
    
    while(1) {
        npad = readpad_new();
        if(!npad && fst) {
            pgWaitV();
            fst=1;
            continue;
        }
        
        // STARTでESCAPE
        if(npad & CTRL_START) { break; }
        
        // TRIGGERでモード変更
        if(npad & (CTRL_LTRIGGER|CTRL_RTRIGGER)) {
            mode = !mode;
        }
        
        // 十字キーで選択変更
        if(npad & (CTRL_UP|CTRL_DOWN|CTRL_LEFT|CTRL_RIGHT)) {
            if(npad & CTRL_LEFT )  select--;
            if(npad & CTRL_RIGHT)  select++;
            if(npad & CTRL_UP   )  select-=2;
            if(npad & CTRL_DOWN )  select+=2;
            
            if(select< 0) select=15;
            if(select>15) select=0;
        }
        
        // ○×で割り当て変更
        cursor = findKeyDef(eConf.key[mode][select]);
        if(npad & (CTRL_CIRCLE|CTRL_CROSS)) {
            if(npad & CTRL_CROSS ) cursor--;
            if(npad & CTRL_CIRCLE) cursor++;
            
            if(cursor<0)  cursor=PCE_KEYMAX-1;
            if(cursor>(PCE_KEYMAX-1)) cursor=0;
        }
        eConf.key[mode][select]=keydef[cursor].key;
        
        // TRIANGLEで連射切り替え
        if(npad & CTRL_TRIANGLE) {
            eConf.rap[mode][select] = (eConf.rap[mode][select]+1) % MODE_RAPIDMAX;
        }

        // 
        pgFillBmp(0);

        pspX = PSPPOINT[select].x;
        pspY = PSPPOINT[select].y;
        pceX = PCEPOINT[cursor].x;
        pceY = PCEPOINT[cursor].y;

        if(pspX || pspY) {
            /* 選択されたキーの塗り潰しポイントを塗り潰す */
            for(i=0;i<272;i++){
                for(j=0;j<480;j++){
                    m=g_bgMask[j+i*480];
                    
                    if(m==select) {
                        *(short*)pgGetVramAddr(j,i) = RGB_RED;
                    }
                    
                    if(m<0x20) continue;
                    
                    if(m==(cursor+0x20)) {
                        *(short*)pgGetVramAddr(j,i) = RGB_BLUE;
                    } else {
                        *(short*)pgGetVramAddr(j,i) = RGB_WHITE;
                    }
                }
            }
        }
        
        pgPrint(0, 0,-1,(mode==0)? "2 BUTTON MODE(EXIT:START)":"6 BUTTON MODE(EXIT:START)");
        mh_print(0,10,"TRIGGER         : BUTON MODE",-1);
        mh_print(0,20,"TRIANGLE        : RAPID ",-1);
        mh_print(0,30,"CIRCLE or CROSS : CHANGE ",-1);
        
        for(i=0;i<PSP_KEYMAX;i++) {
            char* KeyName= "";
            char* Rapid  = "";
            char  string[80];
            
            KeyName = keydef[findKeyDef(eConf.key[mode][i])].name;
            Rapid   = rpdCvt[eConf.rap[mode][i]];
            
            strcpy(string,PSP_KEY[i]);
            strcat(string,KeyName);
            strcat(string,Rapid);

            x = bg_SX+(i&1)*140; // x = 105+(i&1)*140;
            y = bg_SY+i/2*15;  // y = 46+i/2*15;
            
            if(i==select) {
                mh_print(x,y,string,RGB_RED);
                pgDrawLine(x,y+10,x+114,y+10,RGB_RED);
            } else {
                mh_print(x,y,string,RGB_WHITE);
            }
        }
        x = bg_SX+(i&1)*150;
        y = bg_SY+i/2*15;
        
        mh_print(x,y,"SPECIAL MENU : L-Trigger+SELECT",RGB_RED);
        
        {
            x = bg_SX+(select&1)*140;
            y = bg_SY+select/2*15 + 10;
            if(!(pspX<x)){ x=x+114; }
            
            if(pspX || pspY || pceX || pceY) {
                pgDrawLine(x,y,pspX,pspY,RGB_YELLOW);
                pgDrawLine(pspX,pspY,pceX,pceY,RGB_GREEN);
            }
        }
        
        pgScreenFlipV();
    }

    if(bgf) {
        free(bgf);
    }

    // KeyConfig Update
    eConf.button6 = mode;
    eConf.autoflag= 0;
    eConf.autofire= 0;

    if(eConf.rap[mode][0x0]&4) eConf.autoflag |= CTRL_UP; 
    if(eConf.rap[mode][0x1]&4) eConf.autoflag |= CTRL_RIGHT;
    if(eConf.rap[mode][0x2]&4) eConf.autoflag |= CTRL_DOWN;
    if(eConf.rap[mode][0x3]&4) eConf.autoflag |= CTRL_LEFT;
    if(eConf.rap[mode][0x4]&4) eConf.autoflag |= CTRL_A_UP;
    if(eConf.rap[mode][0x5]&4) eConf.autoflag |= CTRL_A_RIGHT;
    if(eConf.rap[mode][0x6]&4) eConf.autoflag |= CTRL_A_DOWN;
    if(eConf.rap[mode][0x7]&4) eConf.autoflag |= CTRL_A_LEFT;
    if(eConf.rap[mode][0x8]&4) eConf.autoflag |= CTRL_TRIANGLE;
    if(eConf.rap[mode][0x9]&4) eConf.autoflag |= CTRL_CIRCLE;
    if(eConf.rap[mode][0xa]&4) eConf.autoflag |= CTRL_CROSS;
    if(eConf.rap[mode][0xb]&4) eConf.autoflag |= CTRL_SQUARE;
    if(eConf.rap[mode][0xc]&4) eConf.autoflag |= CTRL_LTRIGGER;
    if(eConf.rap[mode][0xd]&4) eConf.autoflag |= CTRL_RTRIGGER;
    if(eConf.rap[mode][0xe]&4) eConf.autoflag |= CTRL_SELECT;
    if(eConf.rap[mode][0xf]&4) eConf.autoflag |= CTRL_START;

    // エミュレーション高速化のため、ここで計算する
    for(i=0;i<2;i++){
        for(j=0;j<16;j++) {
            eConf.rapm[i][j] = default_rapid_map[ eConf.rap[i][j]&3 ];
        }
    }
}

int getFreeMemory(void)
{
    int i = 1024;
    void *mem;
    
    while((mem = malloc(i))) {
        free(mem);
        i+=1024;
    }

    return i;
}

//--------------------------------------------------------------------------
// 
// ステートセーブに関する処理
// 
//--------------------------------------------------------------------------
int menu_State(int new_pad)
{
    // ステートセーブ関係
    static int chg=0;
    int i;
    char msg[80],*p;
    static int select=0;
    
    if(new_pad & CTRL_LEFT)  { chg-=10; }
    if(new_pad & CTRL_RIGHT) { chg+=10; }
    if(new_pad & CTRL_UP  )  { chg-=1; }
    if(new_pad & CTRL_DOWN)  { chg+=1; }
    
    select = _loop(select,chg,0,QSLOT_MAX-1);
    chg = 0;
    
    mh_print(0,0,"STATE MENU",-1);
    mh_print(0,10,"○ : SAVE / △ : LOAD",-1);


    // SAVE
    if(new_pad & CTRL_CIRCLE) {
        if(MsgBox("SAVE OK?",MSG_YES)==MSG_YES) {
            qsave_disk(select);
        }
    }

    // LOAD
    if(new_pad & CTRL_TRIANGLE) {
        if(MsgBox("LOAD OK?",MSG_YES)==MSG_YES) {
            qload(select);
            return STATE_CONT;
        }
    }
    
    
    for(i=0;i<QSLOT_MAX;i++) {
        int px,py,c;
        px = (i / 15) * 70 + 50;
        py = (i % 15) * 12 + 50;
        
        strcpy(msg,"SLOT ");
        p = &msg[5];
        *p++ = ((i+1)/10)+0x30;
        *p++ = ((i+1)%10)+0x30;
        *p++ = ' ';
        
        if(qslot[i]==0) { strcpy(p,"OFF");  p+=3; }
        else            { strcpy(p,"ON ");  p+=3; }
        
        *p++ = 0;
        
        // DISKにあるデータは黄色にする等の工夫を!
        if(i==select) c = RGB_RED;
        else {
            if(qslot[i]==0) c = RGB(127,127,127);
            else            c = RGB_WHITE;
        }
        mh_print(px,py,msg,c);
    }
    
    // XBuf Draw
    {
        int sx = 215;
        int sy =  30;
        SCRN* p = 0;
        
        if((p=qslot[select])) {
            int y,x;
            short * dst;
            short * src = p->pic;
            for(y=0;y<p->h;y++) {
                dst = (short*)pgGetVramAddr(sx,sy+y);
                for(x=0;x<p->w;x++) {
                    *dst++ = *src++;
                }
            }
            
            pgDrawFrame(sx-1,sy-1,sx+p->w+1,sy+p->h+1,-1);
        }
    }
    
    return 0;
}



//--------------------------------------------------------------------------
// 
// メニューのメイン処理
// 
//--------------------------------------------------------------------------
int menu_Main(void)
{
    int select=0,val;
    int i,sx,sy;
    DWORD retcode = STATE_PLAY;
    EmuConfig inConf;
    int new_pad = 0;
    int menu_page=0;
    int bUp = 1;

    load_menu_bg("menu.bmp");
    memcpy(&inConf,&eConf,sizeof(EmuConfig)); // backup
    pgCls(0);
    clockDown();

#ifdef SOUND
    enable_sound(0);
#endif

    if( strlen(eConf.cdrom)>0 ) {
        strcpy(psp_menu[MENU_CDROM].data,eConf.cdrom);
    }
    
    while(1) {

        new_pad = readpad_new();

        if(!new_pad && bUp==0) {
            pgWaitVn(1);
            bUp=0;
            continue;
        }

        if(new_pad & CTRL_START) { retcode=STATE_CONT; break; }
        if(new_pad & CTRL_RTRIGGER) {
            menu_page = _loop(menu_page,1,0,1);
            select = 0;
        }

        pgFillBmp(0);
        
        if(menu_page==1) {
            val=0;
            if(new_pad & CTRL_UP  )  { select--; }
            if(new_pad & CTRL_DOWN)  { select++; }
            
            if(select<0) select=MENU_STRING-1;
            if(select==MENU_STRING) select=0;
            
            // 操作
            if(new_pad & (CTRL_CIRCLE|CTRL_CROSS)){
                int change=-1;
                if(new_pad&CTRL_CIRCLE) change=1;
                
                if(select==MENU_CPUCLOCK)  eConf.clock  = _loop(eConf.clock,change  ,0,2);
                if(select==MENU_VSYNC   )  eConf.vsync  = _loop(eConf.vsync,change  ,0,1);
                if(select==MENU_VIDEO   )  eConf.video  = _loop(eConf.video,change  ,0,3);
                if(select==MENU_PADNO   )  eConf.padno  = _loop(eConf.padno,change  ,0,4);
                if(select==MENU_SOUND   )  eConf.sound  = _loop(eConf.sound,change  ,0,1);
                if(select==MENU_SKIP    )  eConf.skip   = _loop(eConf.skip,change   ,0,5); 
                if(select==MENU_DEBUG   )  eConf.debug  = _loop(eConf.debug,change  ,0,1);
                
                if(select==MENU_KEYCONFIG) {
                    KeyConf();
                    load_menu_bg("menu.bmp");
                }
                
                if(select==MENU_CONT) { retcode=STATE_CONT;  break; }
                if(select==MENU_ROM)  { retcode=STATE_ROM;   break; }
                if(select==MENU_RESET){ retcode=STATE_RESET; break; }
                
                if(select==MENU_CDROM) {
                    int ext[2]={EXT_TOC,EXT_NULL};
                    getFilePath(eConf.cdrom,eRun.hue_path,ext);
                    strcpy(psp_menu[MENU_CDROM].data,eConf.cdrom);
                }
                if(select==MENU_QUIT){ retcode=STATE_QUIT;   break; }
            }
            
            if(new_pad & (CTRL_LEFT|CTRL_RIGHT)) {
                int change=-1;
                if(new_pad&CTRL_RIGHT) change=1;
            }
            
            strcpy(psp_menu[MENU_CPUCLOCK].data,psp_menu[MENU_CPUCLOCK].msg[eConf.clock]  );
            strcpy(psp_menu[MENU_VSYNC   ].data,psp_menu[MENU_VSYNC   ].msg[eConf.vsync]  );
            strcpy(psp_menu[MENU_VIDEO   ].data,psp_menu[MENU_VIDEO   ].msg[eConf.video]  );
            strcpy(psp_menu[MENU_PADNO   ].data,psp_menu[MENU_PADNO   ].msg[eConf.padno]  );
            strcpy(psp_menu[MENU_SOUND   ].data,psp_menu[MENU_SOUND   ].msg[eConf.sound]  );
            strcpy(psp_menu[MENU_SKIP    ].data,psp_menu[MENU_SKIP    ].msg[eConf.skip]   );
            strcpy(psp_menu[MENU_DEBUG   ].data,psp_menu[MENU_DEBUG   ].msg[eConf.debug]  );
            
            sx = 30;
            sy = 30;
            
            for(i=0;i<MENU_STRING;i++) {
                if(i==select)  mh_print(sx-10 ,sy,"⇒",RGB_WHITE);
                mh_print(sx    ,sy,psp_menu[i].menu,(i==select)?RGB_RED:RGB_WHITE);
                mh_print(sx+100,sy,psp_menu[i].data,RGB_WHITE);
                sy += 10;
            }
        }
        else {
            if((retcode = menu_State(new_pad))) {
                break;
            }
            bUp = 1;
        }
        
        // free memory を表示
        mh_print(0,260,"free memory ",-1);
        mh_print_dec(60,260,getFreeMemory(),-1);

        if(PSP_Is()){
            sceKernelDcacheWritebackAll();
        }
        pgScreenFlipV();
        if(PSP_IsEsc()) { retcode=STATE_QUIT; break; }
    }
    
    // クロック変更
    changeClock();
    
    // 画面を真っ黒にする
    pgCls(0);

#ifdef SOUND
    enable_sound(eConf.sound);
#endif

    image_config_update();

    return retcode;
}

