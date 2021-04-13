/*
	Portable PC-Engine Emulator
	1998 by BERO bero@geocities.co.jp

    Modified 1998 by hmmx hmmx@geocities.co.jp
    Modified 2000 by U-TA npce@excite.co.jp
    Modified 2001 by BreezeKeeper npce@rising-force.net
*/

//#define DBG_SPRITE_ON

#define VDC1  0
#define VDC2  1
#define EXTERN 

#include "stdinc.h"
#include "string.h"
#include "pg.h"
#include "syscall.h"
#include "pce.h"
#include "m6502.h"
#include "sound.h"
#include "main.h"

#define MAXDISP	227
#define	VRR	2

#define MinLine	io.vdcregs[0].minline
#define MaxLine io.vdcregs[0].maxline
#define	FC_W	io.vdcregs[0].screen_w
#define	FC_H	256


/* callback function */
int nullfunc() { return 0; }
int (*pSoundFunc)(int control)             = nullfunc;
int (*pImageFunc)(int x,int y,int w,int h) = nullfunc;
int (*pCddaPlayFunc)(int track,int flag)   = nullfunc;
int (*pCddaStopFunc)(void)                 = nullfunc;
int (*pJoyStick)(short *JS)                = nullfunc;

int skip_frame = 0;

int CheckSprites(void);
int JoyStick(short *JS);

void  IO_write(word A,byte V);
byte  IO_read(word A);

void RefreshBG(int Y1,int Y2,int vdc,int b1st);
void RefreshSP(int Y1,int Y2,int vdc);

void RefreshScreen(int,int);

//=============================================================================
//
//
//=============================================================================
void bank_set(byte P,byte V)
{
    if (ROMMap[V]==IOAREA) {
        Page[P]=IOAREA;
    }
    else {
        Page[P]=ROMMap[V]-P*0x2000;
    }
}

//=============================================================================
//
//
//=============================================================================
byte _Rd6502(word A)
{
    if(Page[A>>13]!=IOAREA) {
        return Page[A>>13][A];
    }

    return IO_read(A);
}


//=============================================================================
// ROM_size > 256ならSF2CE
//
//=============================================================================
void _Wr6502(word A,byte V)
{
    if(Page[A>>13]==IOAREA) {
        IO_write(A,V);
    } else{
        if( ROM_size>256 ) { // SF2CE ? 
            if ((A & 0x1ffc)==0x1ff0) {
                /* support for SF2CE silliness */
                int i;
                ROMMap[0x40] = ROMMap[0] + 0x80000;
                ROMMap[0x40] += (A & 3) * 0x80000;
                
                for (i = 0x41; i <= 0x7f; i++) {
                    ROMMap[i] = ROMMap[i - 1] + 0x2000;
                }
            } else {
                Page[A >> 13][A] = V;
            }
        } else {
            Page[A>>13][A]=V;
        }
    }
}



//=============================================================================
//
//
//=============================================================================
void IO_write(word A,byte V)
{
	switch(A&0x1e00) {
      case 0x0000: VDC_write(A,V); break;
      case 0x0400: VCE_write(A,V); break;
      case 0x0800: PSG_write(A,V); break;
      case 0x0c00: TMR_write(A,V); break;
      case 0x1000: JOY_write(A,V); break;
      case 0x1400: IRQ_write(A,V); break;
      case 0x1800: CD_write(A,V);  break;
      case 0x1A00: ACD_write(A,V); break;
      default:
        break;
    }
}


//=============================================================================
// 
// IO READ
// 
//=============================================================================
byte IO_read(word A)
{
    switch(A&0x1e00){
      case 0x0000: return VDC_read(A);
      case 0x0400: return VCE_read(A);
      case 0x0800: return PSG_read(A);
      case 0x0c00: return TMR_read(A);
      case 0x1000: return JOY_read(A);
      case 0x1400: return IRQ_read(A);
      case 0x1800: return CD_read(A);
      case 0x1A00: return ACD_read(A);
    }
	return NODATA;
}


//=============================================================================
// スキャンライン描画処理関数
// 
// [ Normal ]
// VDC内でSP0,SP1,BGを SP0<BG<SP1 という優先順位で合成する。

// [ SuperGrafx ]
// VDC1とVDC2の出力を合成し、その結果をVPIで合成する。
// 
//=============================================================================
void DrawLine(int y1,int y2)
{
    if(skip_frame) return ;

    // NORMAL PC-Engine (VDC1 only)
    if(io.vpc[0]==0x11 && io.vpc[1]==0x11) {
        RefreshBG(y1,y2,VDC1,1);
        RefreshSP(y1,y2,VDC1);
    }
    // VDC2のみ(見たことない)
    else if(io.vpc[0]==0x22 && io.vpc[1]==0x22) {
        RefreshBG(y1,y2,VDC2,1);
        RefreshSP(y1,y2,VDC2);
    }
    // その他
    else {
        // 1941 Counter Attack(OK?)
        if(io.vpc[0]==0x77 && io.vpc[1]==0x77) {
            RefreshBG(y1,y2,VDC2,1);
            RefreshBG(y1,y2,VDC1,0);
            RefreshSP(y1,y2,VDC1);
            RefreshSP(y1,y2,VDC2);
        }
        // バトルエース(OK?) 魔道王グランゾート(opening)
        else if(io.vpc[0]==0x0 && io.vpc[1]==0x30) {
            RefreshBG(y1,y2,VDC2,1); // ここが！
            RefreshSP(y1,y2,VDC2);
            RefreshBG(y1,y2,VDC1,2);
            RefreshSP(y1,y2,VDC1);
        }
        // オルディネス(OK?)
        else if(io.vpc[0]==0x33 && io.vpc[1]==0x33) {
            // $08-$09 : 33 33
            RefreshBG(y1,y2,VDC2,1);
            RefreshSP(y1,y2,VDC2);
            RefreshBG(y1,y2,VDC1,0);
            RefreshSP(y1,y2,VDC1);
        }
        // 大魔界村(OK?)
        else if(io.vpc[0]==0x75 && io.vpc[1]==0x56) {
            // $08 : 75 1111b 1001b
            // $09 : 56 1001b 1010b
            // $0A-$0E : 00 ff 03 00
            RefreshBG(y1,y2,VDC2,1);
            RefreshBG(y1,y2,VDC1,0);
            RefreshSP(y1,y2,VDC1);
            RefreshSP(y1,y2,VDC2);
        }
        // 魔道王グランゾート(ゲーム)
        else if(io.vpc[0]==0x44 && io.vpc[1]==0x74) {
            // $08-$09: 44 74
            RefreshBG(y1,y2,VDC2,1);
            RefreshBG(y1,y2,VDC1,2);
            RefreshSP(y1,y2,VDC1);
            RefreshSP(y1,y2,VDC2);
        }
        // その他(該当なしのハズ)
        else {
            RefreshBG(y1,y2,VDC1,1);
            RefreshBG(y1,y2,VDC2,0);
            RefreshSP(y1,y2,VDC2);
            RefreshSP(y1,y2,VDC1);
        }
    }
}


//=============================================================================
//
//
//=============================================================================
byte Loop6502(M6502 *R)
{
	int ret = INT_NONE;
    int dispmin = (MaxLine-MinLine>MAXDISP ? MinLine+((MaxLine-MinLine-MAXDISP+1)>>1) : MinLine);
    int dispmax = (MaxLine-MinLine>MAXDISP ? MaxLine-((MaxLine-MinLine-MAXDISP+1)>>1) : MaxLine);

    io.scanline=(io.scanline+1)%scanlines_per_frame;

    io.vdcregs[0].status&=~VDC_RasHit;
    io.vdcregs[1].status&=~VDC_RasHit;
    
    ret = VDC_SATB_DMA_CHECK();

    /* io.scanline Match Interrupt */
    if( io.vdcregs[0].VDC[CR].W & 0x04 ) {
        if( io.scanline ==((io.vdcregs[0].VDC[RCR].W&1023)-64) ) {
            io.vdcregs[0].status |= VDC_RasHit;
            ret = INT_IRQ;
        }
    }
    
    // 表示する領域の１ライン目を書くときの処理
	if (io.scanline==MinLine) {
        frame_counter++;
        pSoundFunc(SOUND_PLAY);  // callback function
        if(eConf.skip==1) SoundStabilizer();
        else
          if(eConf.skip>1) frame_skip(frame_counter);

        io.vdcregs[0].status&=~VDC_InVBlank;
        io.vdcregs[1].status&=~VDC_InVBlank;
        
        io.prevline=dispmin;
        
        io.vdcregs[0].ScrollYDiff = 0;
        io.vdcregs[0].oldScrollYDiff = 0;
        io.vdcregs[1].ScrollYDiff = 0;
        io.vdcregs[1].oldScrollYDiff = 0;
        
		skip_frame = g_skip_next_frame;
		g_skip_next_frame = 0;
	}
    // 最終ラインを書くときの処理
    else if (io.scanline==MaxLine) {
        if (CheckSprites()) io.vdcregs[0].status|= VDC_SpHit;
        else                io.vdcregs[0].status&=~VDC_SpHit;
        
        if (io.prevline<dispmax) {
            DrawLine(io.prevline,dispmax+1);
        }
        io.prevline=dispmax+1;
        RefreshScreen(dispmin,dispmax);
    }
    // 描画スキャンライン範囲内の処理
    else if (io.scanline>=MinLine && io.scanline<=MaxLine) {
        if((io.vdcregs[0].status&VDC_RasHit)) {
            if(io.prevline<dispmax) {
                DrawLine(io.prevline,io.scanline);
            }
            io.prevline = io.scanline;
        }
    }
    
//    scroll=0;

    // 最終ラインを描画した次のタイミングで実行する部分
	if (io.scanline==MaxLine+1) {
        io.vdcregs[0].status|=VDC_InVBlank;
        if(0x10000 & pJoyStick(io.JOY)) {
			return INT_QUIT;
		}

        /* VRAM to SATB DMA */
        VDC_SATB_DMA();
        
        if (ret==INT_IRQ) {
			io.vdcregs[0].pendvsync = 1;
        }
        else if (VBlankON(0)) {
            ret = INT_IRQ;
        }
	}

    if(io.vdcregs[0].pendvsync && ret!=INT_IRQ) {
        io.vdcregs[0].pendvsync = 0;
        //io.vdc_status|=VDC_InVBlank;
        if (VBlankON(0)) {
            //TRACE("vsync=%d\n", io.scanline);
            ret = INT_IRQ;
        }
    }
    
	if(ret==INT_IRQ) {
		if (!(io.irq_mask&IRQ1)) {
			io.irq_status|=IRQ1;
			return ret;
		}
	}
	return INT_NONE;
}



//=============================================================================
// BG LINEを描画する関数でつ
// SPより先に描画するでゴワス
//=============================================================================
void RefreshBG(int Y1,int Y2,int vdc,int b1st)
{
    int i;
    int X1,XW,Line;
    int x,y,h,offset;
	PALFMT *PP;//,*ZP;
	Y2++;

    PP=(PALFMT*)XBuf+WIDTH*(HEIGHT-FC_H)/2+(WIDTH-FC_W)/2+WIDTH*Y1;

    if( !ScreenON(vdc) ) { // || !io.vdcregs[vdc].BGONSwitch ) {
        if(b1st==1) {
            WORD *dst = (WORD*)XBuf+((HEIGHT-FC_H)/2+Y1)*WIDTH;
            for(i=0;i<(Y2-Y1)*WIDTH;i++) {
                *dst++ = io.Pal[0];
            }
        }
        else if(b1st==2) {
            WORD *dst = (WORD*)XBuf+((HEIGHT-FC_H)/2+Y1)*WIDTH;
            for(i=0;i<(Y2-Y1)*WIDTH;i++) {
                *dst++ &= 0x7ffe;
            }
        }
    }
    else {
        //TRACE("ScrollY=%d,diff=%d\n", ScrollY, ScrollYDiff);
        //TRACE("ScrollX=%d\n", ScrollX);
        y = Y1+ScrollY(vdc)-io.vdcregs[vdc].ScrollYDiff;
        offset = y&7;
        h = 8-offset;
        if (h>Y2-Y1) h=Y2-Y1;
        y>>=3;
        PP-=ScrollX(vdc)&7;

        XW=io.vdcregs[vdc].screen_w/8+1;
        
        for(Line=Y1;Line<Y2;y++) {

            x = ScrollX(vdc)/8;
            y &= io.vdcregs[vdc].bg_h-1;

            for(X1=0;X1<XW;X1++){
                PALFMT *R,*P;
                //byte *C;//,*Z;
                unsigned int *C2;
                int no;

                x&=io.vdcregs[vdc].bg_w-1;
                no = ((word*)io.vdcregs[vdc].VRAM)[x+y*io.vdcregs[vdc].bg_w];
                
                R = &io.Pal[(no>>12)*16];
                no&=0xFFF;

                if(vchange[vdc][no]) {
                    vchange[vdc][no]=0;
                    plane2pixel(vdc,no);
                }
                C2 = &VRAM2[vdc][no*8+offset];
                //C = &VRAM[no*32+offset*2];
                P = PP;

                for(i=0;i<h;i++) {
                    unsigned int L=C2[0];

                    if(b1st==1) {
                        P[0] = R[(L>>28)   ];
                        P[1] = R[(L>>24)&15];
                        P[2] = R[(L>>20)&15];
                        P[3] = R[(L>>16)&15];
                        P[4] = R[(L>>12)&15];
                        P[5] = R[(L>> 8)&15];
                        P[6] = R[(L>> 4)&15];
                        P[7] = R[(L    )&15];
                    }
                    else if(b1st==2) {
                        if((L>>28)   ) P[0] = R[(L>>28)   ]; else P[0]&=0x7ffe;
                        if((L>>24)&15) P[1] = R[(L>>24)&15]; else P[1]&=0x7ffe;
                        if((L>>20)&15) P[2] = R[(L>>20)&15]; else P[2]&=0x7ffe;
                        if((L>>16)&15) P[3] = R[(L>>16)&15]; else P[3]&=0x7ffe;
                        if((L>>12)&15) P[4] = R[(L>>12)&15]; else P[4]&=0x7ffe;
                        if((L>> 8)&15) P[5] = R[(L>> 8)&15]; else P[5]&=0x7ffe;
                        if((L>> 4)&15) P[6] = R[(L>> 4)&15]; else P[6]&=0x7ffe;
                        if((L    )&15) P[7] = R[(L    )&15]; else P[7]&=0x7ffe;
                    }
                    else {
                        if((L>>28)   ) P[0] = R[(L>>28)   ];
                        if((L>>24)&15) P[1] = R[(L>>24)&15];
                        if((L>>20)&15) P[2] = R[(L>>20)&15];
                        if((L>>16)&15) P[3] = R[(L>>16)&15];
                        if((L>>12)&15) P[4] = R[(L>>12)&15];
                        if((L>> 8)&15) P[5] = R[(L>> 8)&15];
                        if((L>> 4)&15) P[6] = R[(L>> 4)&15];
                        if((L    )&15) P[7] = R[(L    )&15];
                    }
                    
                    P+=WIDTH;
                    C2++;
                }
                x++;
                PP+=8;
            }
            Line+=h;
            PP+=WIDTH*h-XW*8;
            offset = 0;
            h = Y2-Line;
            if (h>8) h=8;
        }
    }
}

inline void SP1_Put(PALFMT* pFb,PALFMT* pPalette,unsigned int L,int shift)
{
    if((L=(L>>shift)&15)) {
        // SPではない場合に描画する
        if( !((*pFb) & 0x8000) ) {
            *pFb = pPalette[L];// | 0x8000;
        }
    }
}

inline void SP0_Put(PALFMT* pFb,PALFMT* pPalette,unsigned int L,int shift)
{
    if((L = (L>>shift)&15)) {
        if( (*pFb&0x8001) ) *pFb = *pFb|0x8000;
        else                *pFb = pPalette[L];
    }
}

#define SPX_PUT_N( NAME, PP, RR, LL, BB ) \
  if(LL) { \
      NAME(&PP[0+BB],RR,LL,28); NAME(&PP[1+BB],RR,LL,24); NAME(&PP[2+BB],RR,LL,20); NAME(&PP[3+BB],RR,LL,16); \
      NAME(&PP[4+BB],RR,LL,12); NAME(&PP[5+BB],RR,LL, 8); NAME(&PP[6+BB],RR,LL, 4); NAME(&PP[7+BB],RR,LL, 0); \
  }

#define SPX_PUT_H( NAME, PP, RR, LL, BB ) \
  if(LL) { \
      NAME(&PP[0+BB],RR,LL, 0); NAME(&PP[1+BB],RR,LL, 4); NAME(&PP[2+BB],RR,LL, 8); NAME(&PP[3+BB],RR,LL,12); \
      NAME(&PP[4+BB],RR,LL,16); NAME(&PP[5+BB],RR,LL,20); NAME(&PP[6+BB],RR,LL,24); NAME(&PP[7+BB],RR,LL,28); \
  }


//=============================================================================
//
//
//=============================================================================
static void PutSprite(PALFMT *P,int *C2,PALFMT *R,int h,int inc,int hflip,int spbg)
{
	register int i,L0,L1;
    
    if(spbg) {
        for(i=0;i<h;i++) {
            L0 = C2[0]; L1 = C2[1];
            if(hflip) { SPX_PUT_H(SP1_Put,P,R,L0,0); SPX_PUT_H(SP1_Put,P,R,L1,8); }
            else      { SPX_PUT_N(SP1_Put,P,R,L1,0); SPX_PUT_N(SP1_Put,P,R,L0,8); }
            C2+=inc;
            P+=WIDTH;
        }
    } else {
        for(i=0;i<h;i++) {
            L0 = C2[0]; L1 = C2[1];
            if(hflip) { SPX_PUT_H(SP0_Put,P,R,L0,0); SPX_PUT_H(SP0_Put,P,R,L1,8); }
            else      { SPX_PUT_N(SP0_Put,P,R,L1,0); SPX_PUT_N(SP0_Put,P,R,L0,8); }
            C2+=inc;
            P+=WIDTH;
        }
    }
}

#define ATTR_SPBG(attr)  (((attr)>>7)&1)
#define ATTR_PAL(attr)   (((attr)&15)*16+256)
#define ATTR_HFLIP(attr) ((atr)&H_FLIP);

//=============================================================================
// BGが描画されたフレームにSPを
//
//=============================================================================
//
// FrameBufferにスプライトを描画する
//
void RefreshSP(int Y1,int Y2,int vdc)
{
	int n,inc,hflip,spbg;
	SPR *spr;
    PALFMT *pDst;
    unsigned int *C2;
    int h,t,i,j,pos,y_sum;
    int cx,cy,yoffset,xoffset,atr;
    int x,y,no,cgx,cgy;

    if(!SpriteON(vdc)) return;

    for(n=0;n<64;n++) {
        spr = &io.vdcregs[vdc].SPRAM[n];
        atr = spr->atr;
        y = (spr->y&1023)-64;       // x coordinate    (10bit)
        x = (spr->x&1023)-32;       // y coordinate    (10bit)
        cgx = (atr>>8)&1;           // SPRITE-W (0:16,1:32)
		cgy = (atr>>12)&3;          // SPRITE-H (00:16,01:32,10:Inv,11:64)
		cgy |= cgy>>1;              // 

        if (y>=Y2 || y+(cgy+1)*16<Y1 || x>=FC_W || x+(cgx+1)*16<0) continue;
        //y--; /* スプライトが１ドット下に描画されるっぽいので調整 */

        no= spr->no&0x7ff;           // Pattern address (10bit)

        // 512-1023はゴミデータ?
        // if((no/2)>511) { continue; }
        no = (no>>1)&~(cgy*2+cgx);

        // sprite cache
        for(i=0;i<cgy*2+cgx+1;i++) {
			if (vchanges[vdc][no+i]) {
				vchanges[vdc][no+i]=0;
				sprite2pixel(vdc,no+i);
			}
			if (!cgx) i++;
		}

		C2 = &VRAMS[vdc][no*32];

		pos = WIDTH*(HEIGHT-FC_H)/2+(WIDTH-FC_W)/2+WIDTH*y+x;
		inc = 2;
        
		if (atr&V_FLIP) {
            inc=-2;
            C2+=15*2+cgy*64;
        }
        
		y_sum = 0;

        cy = y; // スプライトはスクロール影響なし？ //cy = y+ScrollY-ScrollYDiff;
		yoffset = cy&7;
		cy>>=3;
        xoffset = x&7; // スプライトはスクロール影響なし？//xoffset=(x+ScrollX)&7;
        cx = x / 8; //スプライトはスクロール影響なし？ //cx = (x+ScrollX)/8;

        hflip= ATTR_HFLIP(atr);
        spbg = ATTR_SPBG(atr);
        
		for(i=0;i<=cgy;i++) {

            cy = cy&(io.vdcregs[vdc].bg_h-1);
            
            t = Y1-y-y_sum;
			h = 16;
			if (t>0) {
				C2+=t*inc;
				h-=t;
				pos+=t*WIDTH;
				cy+=(yoffset+t)>>3;
				yoffset=(yoffset+t)&7;
			}
			if (h>Y2-y-y_sum) h = Y2-y-y_sum;
            
            for(j=0;j<=cgx;j++) {
                if(hflip) pDst = XBuf+pos+(cgx-j)*16;
                else      pDst = XBuf+pos+j*16;
                PutSprite(pDst,C2+j*32,
                          &io.Pal[ATTR_PAL(atr)],
                          h,inc,hflip,
                          spbg
                          );
            }
            
            pos+=h*WIDTH;
            C2+=h*inc+16*inc;
            y_sum+=16;
            cy+=(yoffset+h)>>3;
			yoffset=(yoffset+h)&7;
        }
    }
}


//=============================================================================
//
//
//=============================================================================
void RefreshScreen(int dispmin,int dispmax)
{
    int s,e,w,h;
    s = (WIDTH-io.vdcregs[0].screen_w)/2;
    e = (HEIGHT-256)/2+io.vdcregs[0].minline+dispmin;
    w = io.vdcregs[0].screen_w;
    h = dispmax-dispmin+1;
    pImageFunc(s,e,w,h);
}


//=============================================================================
//
//
//=============================================================================
void ResetPCE(M6502* p6502)
{
    frame_counter = 0;
    
	memset(&io, 0, sizeof(IO));
	memset(IOAREA,0xFF,0x2000);
    memset(p6502,0,sizeof(M6502));

	TimerCount = TimerPeriod;
    p6502->IPeriod = IPeriod;
	p6502->TrapBadOps = 1;
//	CycleOld = 0;
    
	Reset6502(p6502);

    VDC_init();
    VCE_init();
    PSG_init();
    JOY_init();
    IRQ_init();
    CD_init();
    ACD_init();

}

//=============================================================================
//
//
//=============================================================================
int LoadROM(char *name)
{
	int i,ROMmask;
	memset(ROMMap,0,sizeof(ROMMap));
    IPeriod = BaseClock/(scanlines_per_frame*60);
	TimerPeriod = BaseClock/1000*3*1024/21480;

    if(!CartLoad(name)) {
        return 0;
    }
    populus = 0;
    
	ROMmask = 1;
	while(ROMmask<ROM_size) ROMmask<<=1;
	ROMmask--;
    //TRACE("ROMmask=%02X, ROM_size=%02X\n", ROMmask, ROM_size);

    for(i=0;i<0xF7;i++) {
        if (ROM_size == 0x30) {
			switch (i&0x70) {
			case 0x00:
			case 0x10:
			case 0x50:
				ROMMap[i]=ROM+(i&ROMmask)*0x2000;
				break;
			case 0x20:
			case 0x60:
				ROMMap[i]=ROM+((i-0x20)&ROMmask)*0x2000;;
				break;
			case 0x30:
			case 0x70:
				ROMMap[i]=ROM+((i-0x10)&ROMmask)*0x2000;
				break;
			case 0x40:
				ROMMap[i]=ROM+((i-0x20)&ROMmask)*0x2000;
				break;
			}
		}
        else {
			ROMMap[i]=ROM+(i&ROMmask)*0x2000;
        }
	}
    
//		ROMMap[i]=ROM+(i%ROM_size+i/ROM_size*0x10)*0x2000;
/*		if (((i&ROMmask)+i/(ROMmask+1)) < ROM_size)
			ROMMap[i]=ROM+((i&ROMmask)+i/(ROMmask+1)*0x20)*0x2000;
		else
			ROMMap[i]=ROM;
*///		ROMMap[i]=ROM+(i&ROMmask)*0x2000;
   
	if (populus) {
		ROMMap[0x40] = PopRAM + (0)*0x2000;
		ROMMap[0x41] = PopRAM + (1)*0x2000;
		ROMMap[0x42] = PopRAM + (2)*0x2000;
		ROMMap[0x43] = PopRAM + (3)*0x2000;
	}

#if 1
	if (1) {

        for(i=0x68;i<0x88;i++) {
            memcpy(&cd.cd_extra_mem[0x2000*(i-0x68)],ROMMap[i], 0x2000);
			ROMMap[i] = &cd.cd_extra_mem[0x2000*(i-0x68)];
            
		}
	}
#else
	if (1) {
		for(i=0;i<8;i++)
          memcpy(cd.cd_extra_mem + i*0x2000, ROMMap[0x80+i], 0x2000);
        
        ROMMap[0x80] = cd.cd_extra_mem;
        ROMMap[0x81] = cd.cd_extra_mem + 0x2000;
        ROMMap[0x82] = cd.cd_extra_mem + 0x4000;
        ROMMap[0x83] = cd.cd_extra_mem + 0x6000;
        ROMMap[0x84] = cd.cd_extra_mem + 0x8000;
        ROMMap[0x85] = cd.cd_extra_mem + 0xA000;
        ROMMap[0x86] = cd.cd_extra_mem + 0xC000;
        ROMMap[0x87] = cd.cd_extra_mem + 0xE000;

        for(i=0x68;i<0x80;i++) {
            memcpy(cd.cd_extra_mem+0xE000+0x2000*(i-0x68),ROMMap[i], 0x2000);
			ROMMap[i] = cd.cd_extra_mem+0xE000+0x2000*(i-0x68);
		}
	}
#endif

#if 0
	ROMMap[0x80] = PopRAM + (0)*0x2000;
	ROMMap[0x81] = PopRAM + (1)*0x2000;
	ROMMap[0x82] = PopRAM + (2)*0x2000;
	ROMMap[0x83] = PopRAM + (3)*0x2000;
	ROMMap[0x84] = PopRAM + (4)*0x2000;
	ROMMap[0x85] = PopRAM + (5)*0x2000;
	ROMMap[0x86] = PopRAM + (6)*0x2000;
	ROMMap[0x87] = PopRAM + (7)*0x2000;
#endif
   
	ROMMap[0xF7] = WRAM;
	ROMMap[0xF8] = RAM;
	ROMMap[0xF9] = RAM+0x2000;
	ROMMap[0xFA] = RAM+0x4000;
	ROMMap[0xFB] = RAM+0x6000;
	ROMMap[0xFF] = IOAREA; //NULL; /* NULL = I/O area */

	return 1;
}
   
void loadWRAM(void);
void saveWRAM(void);
   
//=============================================================================
//
//
//=============================================================================
int RunPCE(void)
{
	ResetPCE(&io.m6502);
	Run6502(&io.m6502);
    return 1;
}

