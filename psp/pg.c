#include "syscall.h"
#include "pg.h"
#include "pspstd.h"
#include "psp_main.h"
#include "string.h"

#include <stdio.h>

unsigned short g_bgBitmap[480*272];

// EDRAM TOP ADDRESS 
static char* pg_vramtop = (char*)0x04000000;

static long pg_screenmode;
static long pg_showframe;
static long pg_drawframe;

static char pg_mypath[MAX_PATH];
static char pg_workdir[MAX_PATH];

static void null_func() {  }

int pgaOutBlocking(unsigned long channel,unsigned long vol1,unsigned long vol2,void *buf);


//==================================================================
//  
//==================================================================
void pgWaitV(void)
{
    if(PSP_Is()) {
#if 0
        sceDisplayWaitVblankCB();
#else
        static int pv = 0;
        int        cv = sceDisplayGetVcount();
        if(pv==cv) {
            sceDisplayWaitVblankCB();
        }
        pv = sceDisplayGetVcount();
#endif        
    } else {
        sceDisplayWaitVblankStart();
    }
}

//==================================================================
// Wait vsync [count] times
// 確実にVblankStartを待ちたい
//==================================================================
void pgWaitVn(unsigned int count)
{
	for(; count>0; --count) {
        if(PSP_Is()) {
            sceDisplayWaitVblankStartCB();
        } else {
            sceDisplayWaitVblankStart();
        }
        // pgWaitV();
    }
}


//==================================================================
//  
//==================================================================
void* pgVramAddrOfs(unsigned int ofs)
{
    if(PSP_Is()) {
        return (void*)( 0x04000000UL + ofs );
    }
    
    return (void*)( 0x44000000UL + ofs);
}

//==================================================================
// 
//==================================================================
char *pgGetVramAddr(unsigned long x,unsigned long y)
{
    if(PSP_Is()) {
        return pg_vramtop+(pg_drawframe?FRAMESIZE:0)+x*PIXELSIZE*2+y*LINESIZE*2;
    }
    
    return (char*)((int)(pg_vramtop+(pg_drawframe?FRAMESIZE:0)+x*PIXELSIZE*2+y*LINESIZE*2) | 0x40000000);
}

//==================================================================
// 
//==================================================================
void pgPset(int px,int py,int color)
{
    unsigned short* pSet = (unsigned short*)pgGetVramAddr(px,py);

    *pSet = (unsigned short)color;
}


//==================================================================
// 線を描く
// 始点(sx,sy)と終点(ex,ey)
// (1) 長辺を基準として描画する。
// 
// 
//==================================================================
void pgDrawLine(int sx,int sy,int ex,int ey,int color)
{
    int vx=1,vy=1;
    int x,y,lx,ly;
    unsigned short * p;
    int px,py;

    /* 傾きを求める */
    vx = ex - sx;
    vy = ey - sy;

    /* 長さを求める */
    if(vx<0) lx=-vx; else lx=vx;
    if(vy<0) ly=-vy; else ly=vy;

    /* 長い軸を基準にして線を描く */
    if(lx>=ly) {
        /* x軸が長い */
        for(x=0;x<lx;x++) {
            /* 座標 */
            px = sx + (vx/lx) * x;
            py = sy + (vy/ly) * (ly*x/lx) ;

            if(px>0 && px<480 && py>0 && py<272) {
                p = (unsigned short*)pgGetVramAddr(px,py);
                *p = color;
            }
        }
    } else {
        // OK っぽい
        /* y軸が長い */
        for(y=0;y<ly;y++) {
            /* 座標 */
            py = sy + (vy/ly) * y;
            px = sx + (vx/lx) * (lx*y/ly) ;

            if(px>0 && px<480 && py>0 && py<272) {
                p = (unsigned short*)pgGetVramAddr(px,py);
                *p = color;
            }
        }
    }
}
    
//==================================================================
// 
//==================================================================
void pgDrawFrame(unsigned long x1, unsigned long y1, unsigned long x2, unsigned long y2, unsigned long color)
{
	unsigned char *vptr0;		//pointer to vram
	unsigned long i;

	vptr0=pgGetVramAddr(0,0);
	for(i=x1; i<=x2; i++){
		((unsigned short *)vptr0)[i*PIXELSIZE + y1*LINESIZE] = (unsigned short)color;
		((unsigned short *)vptr0)[i*PIXELSIZE + y2*LINESIZE] = (unsigned short)color;
	}
	for(i=y1; i<=y2; i++){
		((unsigned short *)vptr0)[x1*PIXELSIZE + i*LINESIZE] = (unsigned short)color;
		((unsigned short *)vptr0)[x2*PIXELSIZE + i*LINESIZE] = (unsigned short)color;
	}
}

//==================================================================
// 
//==================================================================
void pgFillBox(unsigned long x1, unsigned long y1, unsigned long x2, unsigned long y2, unsigned long color)
{
	unsigned char *vptr0;		//pointer to vram
	unsigned long i, j;

	vptr0=pgGetVramAddr(0,0);
	for(i=y1; i<=y2; i++){
		for(j=x1; j<=x2; j++){
			((unsigned short *)vptr0)[j*PIXELSIZE + i*LINESIZE] = (unsigned short)color;
		}
	}
}

//-----------------------------------------------------------------------------
// Load Menu Bitmap
//-----------------------------------------------------------------------------
void load_menu_bg(char* name)
{
	unsigned char *menu_bg;
	unsigned short *vptr;
    int    menu_bg_siz = 480*272*3+0x36;
    char * menu_bg_buf = 0;
	char BgPath[MAX_PATH];
 	unsigned short x,y,yy,r,g,b;
    int nRead;
    FILE* fp;// int fd;

    strcpy(BgPath,pg_workdir);
    strcat(BgPath,name);

    memset(g_bgBitmap,0,sizeof(g_bgBitmap));
    
    if( (menu_bg_buf = malloc(menu_bg_siz)) ) {
        if((fp=fopen(BgPath,"r"))>0) {
            nRead = fread(menu_bg_buf,1,menu_bg_siz,fp);
            fclose(fp);
            
            if(nRead==menu_bg_siz) {
                menu_bg = menu_bg_buf + 0x36;
                vptr=g_bgBitmap;
                for(y=0; y<272; y++){
                    for(x=0; x<480; x++){
                        yy = 271 - y;
                        r = *(menu_bg + (yy*480 + x)*3 + 2);
                        g = *(menu_bg + (yy*480 + x)*3 + 1);
                        b = *(menu_bg + (yy*480 + x)*3);
                        *vptr++ = RGB(r,g,b);
                    }
                }
            }
        }
        
        free(menu_bg_buf);
    }
}


//==================================================================
// color: 
//==================================================================
void pgFillBmp(int color)
{
    // GPUで処理統一する場合は、ここでGPUを使うとよろしい
    int i;

    for(i=0;i<272;i++) {
        memcpy4(pgGetVramAddr(0,i),&g_bgBitmap[480*i],480/4*2);
    }
//    for(i=0;i<272;i++) {
//        memcpy2(pgGetVramAddr(0,i),&g_bgBitmap[480*i],480/2*2);
//    }
}



//==================================================================
// 
//==================================================================
void pgFillvram(unsigned long color)
{
	unsigned char *vptr0;		//pointer to vram
	unsigned long i;
	vptr0=pgGetVramAddr(0,0);
	for (i=0; i<FRAMESIZE/2; i++) {
		*(unsigned short *)vptr0=(unsigned short)color;
		vptr0+=PIXELSIZE*2;
	}
}

//==================================================================
// 
//==================================================================
void pgCls(unsigned long color)
{
    pgFillvram(color);
    pgScreenFlip();
    pgFillvram(color);
    pgScreenFlip();
}


//==================================================================
// 
//==================================================================
void pgScreenFrame(long mode,long frame)
{
	pg_screenmode=mode;
	frame=(frame?1:0);
	pg_showframe=frame;
	if (mode==0) {
		//screen off
		pg_drawframe=frame;
		sceDisplaySetFrameBuf(0,0,0,1);
	} else if (mode==1) {
		//show/draw same
        pg_drawframe=frame;
        sceDisplaySetFrameBuf(pg_vramtop+(pg_showframe?FRAMESIZE:0),LINESIZE,PIXELSIZE,1);
	} else if (mode==2) {
        //show/draw different
		pg_drawframe=(frame?0:1);
        sceDisplaySetFrameBuf(pg_vramtop+(pg_showframe?FRAMESIZE:0),LINESIZE,PIXELSIZE,1);
	}
}


//==================================================================
// 
//==================================================================
void pgScreenFlip()
{
	pg_showframe=1-pg_showframe;
	pg_drawframe=1-pg_drawframe;
    
	sceDisplaySetFrameBuf(pg_vramtop+(pg_showframe?FRAMESIZE:0),LINESIZE,PIXELSIZE,0);
}


//==================================================================
// 
//==================================================================
void pgScreenFlipV()
{
	pgWaitV();
	pgScreenFlip();
}

enum PadEnum {
    pad_New = 0,
    pad_Old = 1,
    pad_Now = 2
};

//==================================================================
// 
//==================================================================
inline int readpad0(enum PadEnum kind)
{
    static unsigned int pad[3]={0,0,0};
    
	static int n=0;
	ctrl_data_t paddata;

    if(PSP_Is()) {
        sceCtrlPeekBufferPositive(&paddata, 1);
        if (paddata.Ly == 0xff) paddata.buttons|=CTRL_DOWN; else
        if (paddata.Ly == 0x00) paddata.buttons|=CTRL_UP;  
        if (paddata.Lx == 0x00) paddata.buttons|=CTRL_LEFT; else
        if (paddata.Lx == 0xff) paddata.buttons|=CTRL_RIGHT;
    } else {
        sceCtrlReadBufferPositive(&paddata, 1);
    }

    pad[pad_Now] = paddata.buttons;
    pad[pad_New] = pad[pad_Now] & ~pad[pad_Old]; // now_pad & ~old_pad;
    
	if(pad[pad_Old]==pad[pad_Now]){
		n++;
		if(n>=25){
			pad[pad_New]=pad[pad_Now];
			n = 20;
		}
	}else{
		n=0;
        pad[pad_Old] = pad[pad_Now];
	}

    
    return pad[kind];
}

int readpad_now(void) { return readpad0(pad_Now); }
int readpad_old(void) { return readpad0(pad_Old); }
int readpad_new(void) { return readpad0(pad_New); }


/******************************************************************************/


#define PGA_CHANNELS 1
#define PGA_SAMPLES (512)	//256
#define MAXVOLUME 0x8000

int pga_ready=0;
int pga_handle[PGA_CHANNELS];

short pga_sndnul[PGA_CHANNELS][PGA_SAMPLES][2];
void* (*pga_channel_callback[PGA_CHANNELS])(void *buf, unsigned long reqn);
int pga_threadhandle[PGA_CHANNELS];
volatile int pga_terminate=0;


//==================================================================
// 
//==================================================================
static int pga_channel_thread(int args, void *argp)
{
	int channel=*(int *)argp;
    //void *pLast=0;

    memset(pga_sndnul,0,sizeof(pga_sndnul));

	while (pga_terminate==0) {
		void *bufptr=pga_sndnul; // &pga_sndbuf[channel][bufidx];
		void* (*callback)(void *buf, unsigned long reqn);
		callback=pga_channel_callback[channel];
		if (callback) {
			bufptr = callback(bufptr,PGA_SAMPLES);
            if(bufptr==0) { bufptr=pga_sndnul; }
		} else {
             bufptr = pga_sndnul;
        }
		pgaOutBlocking(channel,0x8000,0x8000,bufptr);
        //bufidx=(bufidx?0:1);
	}
	sceKernelExitThread(0);
	return 0;
}

//==================================================================
// 
//==================================================================
void pgaSetChannelCallback(int channel, void *callback)
{
	pga_channel_callback[channel]=callback;
}


//==================================================================
// 
//==================================================================
int pgaInit()
{
	int i,ret;
	int failed=0;
	char str[32];

	pga_terminate=0;
	pga_ready=0;
    
	for (i=0; i<PGA_CHANNELS; i++) {
		pga_handle[i]=-1;
		pga_threadhandle[i]=-1;
		pga_channel_callback[i]=0;
	}
	for (i=0; i<PGA_CHANNELS; i++) {
        // ３番目のパラメータを0x10にするとモノラルになる
        if ((pga_handle[i]=sceAudioChReserve(-1,PGA_SAMPLES,0))<0) failed=1;
	}
	if (failed) {
		for (i=0; i<PGA_CHANNELS; i++) {
			if (pga_handle[i]!=-1) sceAudioChRelease(pga_handle[i]);
			pga_handle[i]=-1;
		}
		return -1;
	}
	pga_ready=1;

	strcpy(str,"pgasnd0");
	for (i=0; i<PGA_CHANNELS; i++) {
		str[6]='0'+i;
		pga_threadhandle[i]=sceKernelCreateThread(str,(pg_threadfunc_t)&pga_channel_thread,0x12,0x10000,0,NULL);
		if (pga_threadhandle[i]<0) {
			pga_threadhandle[i]=-1;
			failed=1;
			break;
		}
		ret=sceKernelStartThread(pga_threadhandle[i],sizeof(i),&i);
		if (ret!=0) {
			failed=1;
			break;
		}
	}
	if (failed) {
		pga_terminate=1;
		for (i=0; i<PGA_CHANNELS; i++) {
			if (pga_threadhandle[i]!=-1) {
				sceKernelWaitThreadEnd(pga_threadhandle[i],NULL);
				sceKernelDeleteThread(pga_threadhandle[i]);
			}
			pga_threadhandle[i]=-1;
		}
		pga_ready=0;
		return -1;
	}
	return 0;
}


//==================================================================
// 
//==================================================================
void pgaTermPre()
{
	pga_ready=0;
	pga_terminate=1;
}


//==================================================================
// 
//==================================================================
void pgaTerm()
{
	int i;
	pga_ready=0;
	pga_terminate=1;

	for (i=0; i<PGA_CHANNELS; i++) {
		if (pga_threadhandle[i]!=-1) {
			sceKernelWaitThreadEnd(pga_threadhandle[i],NULL);
			sceKernelDeleteThread(pga_threadhandle[i]);
		}
		pga_threadhandle[i]=-1;
	}

	for (i=0; i<PGA_CHANNELS; i++) {
		if (pga_handle[i]!=-1) {
			sceAudioChRelease(pga_handle[i]);
			pga_handle[i]=-1;
		}
	}
}

//==================================================================
// 
//==================================================================
int pgaOutBlocking(unsigned long channel,unsigned long vol1,unsigned long vol2,void *buf)
{
	if (!pga_ready) return -1;
	if (channel>=PGA_CHANNELS) return -1;
	if (vol1>MAXVOLUME) vol1=MAXVOLUME;
	if (vol2>MAXVOLUME) vol2=MAXVOLUME;
    return sceAudioOutputPannedBlocking(pga_handle[channel],vol1,vol2,buf);
}


//==================================================================
// 
//==================================================================
void pgMain(unsigned long args, void *argp)
{
    int n;
    
    //-------------------------------------------
    // Work Directory情報を構築する              
    //-------------------------------------------
    strcpy(pg_mypath,argp);
    strcpy(pg_workdir,pg_mypath);
	for (n=strlen(pg_workdir); n>0 && pg_workdir[n-1]!='/'; --n) pg_workdir[n-1]=0;

    //
    if(PSP_Is()) {
        unsigned int *gu_list = (unsigned int*)0x041c0000;
        //pgWaitV = sceDisplayWaitVblankStart;
        sceGuInit();
        // setup
        sceGuStart(GU_DIRECT,gu_list);
        sceGuDrawBuffer(GU_PSM_5551,(void*)0,512);
        sceGuDispBuffer(480,272,(void*)0x44000,512);
        sceGuDepthBuffer((void*)0x90000,512);
        sceGuOffset(2048 - (480/2),2048 - (272/2));
        sceGuViewport(2048,2048,480,272);
        sceGuDepthRange(0xc350,0x2710);
        sceGuScissor(0,0,480,272);
        sceGuEnable(GU_SCISSOR_TEST);
        sceGuEnable(GU_TEXTURE_2D);
        sceGuFrontFace(GU_CW);
        sceGuClear(GU_COLOR_BUFFER_BIT|GU_DEPTH_BUFFER_BIT);
        sceGuFinish();
        sceGuSync(0,0);
        
        sceDisplayWaitVblankStart();
        sceGuDisplay(1);
    }

    // init graphics
    sceDisplaySetMode(0,SCREEN_WIDTH,SCREEN_HEIGHT);
	pgScreenFrame(2,0);
    
    // init input
    sceCtrlSetSamplingCycle(0);
    sceCtrlSetSamplingMode(PSP_Is()?1:0);

    // re-init graphics
    sceDisplaySetMode(1,SCREEN_WIDTH,SCREEN_HEIGHT);
    pgScreenFrame(2,0);

#ifdef SOUND
    pgaInit();
#endif//SOUND
}


//==================================================================
// 
//==================================================================
const char *pguGetMypath()
{
	return pg_mypath;
}

//==================================================================
// 
//==================================================================
const char *pguGetWorkdir()
{
	return pg_workdir;
}


//==================================================================
// 
//==================================================================
void pgSetClock(int clock)
{
    if(PSP_Is()){
        scePowerSetClockFrequency(clock,clock,clock/2);
    }
}

void memcpy4(void* d, void* s, int c) { for(;c>0;--c) *((int  *)d)++ = *((int  *)s)++; }
void memcpy2(void* d, void* s, int c) { for(;c>0;--c) *((short*)d)++ = *((short*)s)++; }
void memset4(void* d, int   v, int c) { for(;c>0;--c) *((int  *)d)++ = v;    }


