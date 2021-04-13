//*****************************************************************************
// 
// 
// 
//*****************************************************************************
#include "stdinc.h"
#include "syscall.h"
#include "pg.h"
#include "pce.h"
#include "main.h"
#include "string.h"
#include "hue_image.h"
#include "pspstd.h"
#include "psp_main.h"

// external
extern EmuConfig eConf;
extern EmuRuntime eRun;

static void blt_normal(int x,int y,int w,int h);
static void blt_gpu(int x,int y,int w,int h);

int og_x=0,og_y=0,og_w=0,og_h=0;

// CPU描画時のFrame buffer 
static void *img_buffer[2]={ 0, 0 };

///////////////////////////////////////////
#define ERNUM  25
int  errPos = 0;
char errMsg[ERNUM][256];
int  errSw[ERNUM];



//
void core_error(char* errmsg)
{
    int len = strlen(errmsg);

    if(len>255) len=255;

    memcpy(errMsg[errPos],errmsg,len);
    errMsg[errPos][len]=0;
    errSw[errPos] = 255;
    errPos = (errPos+1)%ERNUM;
}

void print_error(void)
{
    int i,c;
    
    for(i=0;i<ERNUM;i++) {
        if((c = errSw[i])>-2) {
            if(c<0) c=0;
            mh_print(0,(i+1)*10,errMsg[i],RGB(c,c,c));
            errSw[i]--;
        }
    }
}

//---------------------------------------------------------------
// フレームスキップ処理関数
//---------------------------------------------------------------
void frame_skip(int counter)
{
    if( (counter % (eConf.skip))==0) {
        g_skip_next_frame = 1;
    }
}

//---------------------------------------------------------------
//
//---------------------------------------------------------------
void image_waitv(void)
{
    if(eConf.vsync && !g_skip_next_vsync) {
        pgWaitV();
    }
}

//---------------------------------------------------------------
//
//---------------------------------------------------------------
void image_debug(void)
{
    // fps計算
    static unsigned int lasttick=0,framerate = 0;
    register DWORD curtick = sceKernelLibcClock();
    int fps;
    
    if(lasttick > curtick) {
        lasttick = curtick;
    }
    
    framerate = (framerate + (curtick-lasttick))/2;
    if(framerate==0) fps = 0;
    else             fps = ((10000000/(framerate))+9)/10;
    lasttick = curtick;

    if(eConf.debug) {
        mh_print_dec(0,0,fps,-1);
        
#if defined(SGX)
        if(io.vpc[0]!=0x11 || io.vpc[1]!=0x11) {
            int w1 = ((word)(io.vpc[3]&3)<<8) | io.vpc[2];
            int w2 = ((word)(io.vpc[5]&3)<<8) | io.vpc[4];
            
            mh_print_hex4(430,  0,io.vpc[0],-1);
            mh_print_hex4(430, 10,io.vpc[1],-1);
            mh_print_hex4(430, 20,io.vpc[2],-1);
            mh_print_hex4(430, 30,io.vpc[3],-1);
            mh_print_hex4(430, 40,io.vpc[4],-1);
            mh_print_hex4(430, 50,io.vpc[5],-1);
            mh_print_hex4(430, 60,io.vpc[6],-1);
            mh_print_hex4(430, 70,io.vpc[7],-1);
            
            mh_print_hex4(430, 90,w1,-1);
            mh_print_hex4(430,100,w2,-1);
        }
#endif
    }

    print_error();
}

//-----------------------------------------------------------------------------
// CPU bitblt
//-----------------------------------------------------------------------------
static void blt_normal(int x,int y,int w,int h)
{
    register int sx = (SCREEN_WIDTH - w)/2;
    register int sy = (SCREEN_HEIGHT- h)/2;
    register int i,j;

    og_x=x;
    og_y=y;
    og_w=w;
    og_h=h;
    

    for(j=0;j<h;j++) {
        register DWORD* dst = (DWORD*)pgGetVramAddr(sx,sy+j);
        register DWORD* src = (DWORD*)&XBuf[WIDTH*(j+y)+x];
        for(i=0;i<w/2;i++) {
            *dst++ = *src++;
        }
    }

    image_debug();
    image_waitv();
	pgScreenFlip();
}


#ifdef GPU_ENABLE

typedef struct Vertex16 {
    unsigned short u, v;
	short x, y, z;
} Vertex16;


//-----------------------------------------------------------------------------
// GPU関連
//-----------------------------------------------------------------------------
static int   gpu_qid=-1;
static int   gpu_back=0;

unsigned int *gu_list = (unsigned int*)0x041c0000;

//-----------------------------------------------------------------------------
// GPU bitblt
//-----------------------------------------------------------------------------
static void blt_gpu(int x,int y,int w,int h)
{
    int ScreenX,ScreenY;
    int ScreenW = w;
    int ScreenH = h;
    unsigned short *screen = (unsigned short*)XBuf;
    Vertex16* ScreenVertex;

    og_x=x;
    og_y=y;
    og_w=w;
    og_h=h;
    
#if 0
    if(gpu_qid>=0) {
        sceGuSync(0,0);
        image_debug();
		image_waitv();
        sceKernelDcacheWritebackAll();
        sceGuSwapBuffers();
    }
#endif

    // XBufをキャッシュから追い出す
    sceKernelDcacheWritebackAll();
    
    switch(eConf.video) {
      case 2: /* 4:3 */
        ScreenW = 362;
        ScreenH = 272;
        break;
      case 3: /* FULL SCREEN */
        ScreenW = SCREEN_WIDTH;
        ScreenH = SCREEN_HEIGHT;
        break;
      default:
        ScreenW = w;
        ScreenH = h;
        break;
    }
    
    ScreenX = (SCREEN_WIDTH-ScreenW)/2;
    ScreenY = (SCREEN_HEIGHT-ScreenH)/2;

    sceGuStart(GU_DIRECT,gu_list);

//    sceGuClearColor(0); // black
//    sceGuClear(GU_COLOR_BUFFER_BIT);
    
    sceGuTexMode(GU_PSM_5551,0,0,0);
    sceGuTexImage(0,512,256,512,screen);
    sceGuTexFunc(GU_TFX_REPLACE,GU_TCC_RGBA);
    sceGuTexFilter(GU_LINEAR,GU_LINEAR);

    sceGuTexScale(1.0f,1.0f);
    sceGuTexOffset(0.0f,0.0f);
    sceGuAmbientColor(0xffffffff);
    
    ScreenVertex = (Vertex16*)sceGuGetMemory(2 * sizeof(Vertex16));
    ScreenVertex[0].u = x;
    ScreenVertex[0].v = y;
    ScreenVertex[1].u = x+w;
    ScreenVertex[1].v = y+h;

    ScreenVertex[0].x = ScreenX;
    ScreenVertex[0].y = ScreenY;
    ScreenVertex[1].x = ScreenX+ScreenW;
    ScreenVertex[1].y = ScreenY+ScreenH;
    
    ScreenVertex[0].z = 0;
    ScreenVertex[1].z = 0;
    
    sceGuDrawArray( GU_SPRITES,
                    GU_TEXTURE_16BIT|GU_VERTEX_16BIT|GU_TRANSFORM_2D,
                    2,
                    NULL,
                    ScreenVertex
                    );
    
    sceGuFinish();

    gpu_qid = 1;
    
#if 1
    if(gpu_qid>=0) {
        sceGuSync(0,0);
        image_debug();
		image_waitv();
        sceKernelDcacheWritebackAll();
        sceGuSwapBuffers();
    }
#endif
    gpu_back = 1-gpu_back;
    XBuf = img_buffer[gpu_back];
}
#endif//GPU_ENABLE

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
void image_config_update(void)
{
#ifndef GPU_ENABLE
    eConf.video = 0;
#endif//GPU_ENABLE

    img_buffer[0] = pgVramAddrOfs(0x00100000);
    img_buffer[1] = pgVramAddrOfs(0x00140000);
    XBuf = img_buffer[0];
    
    if(!PSP_Is()) eConf.video=0;
    
    if(eConf.video==0) {
        pImageFunc = (void*)blt_normal;
    } else {
        pImageFunc = (void*)blt_gpu;
    }

    memset(img_buffer[0],0,0x40000);
    memset(img_buffer[1],0,0x40000);

    pgCls(0);

    {
        int i;
        for(i=0;i<ERNUM;i++) {
            errSw[i] = -2;
        }
    }
}

