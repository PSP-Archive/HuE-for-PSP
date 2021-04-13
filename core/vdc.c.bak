// 
// SuperGrafx対応済み
// 一部機能が正常に実装されていないけど
// 実装されていない機能が使われてなければ問題なし
// 

#include "stdinc.h"
#include "pce.h"
#include "pg.h"
#include "string.h"

#define	VRR	2

#define MinLine(vdc)    io.vdcregs[vdc].minline
#define MaxLine(vdc)    io.vdcregs[vdc].maxline


//=============================================================================
// 
// VDC INITIALIZE
// 
//=============================================================================
void VDC_init(void)
{
    frame_counter = 0;

    memset(io.vpc,0,sizeof(io.vpc));
    io.vpc[0] = io.vpc[1] = 0x11;

    io.vdcregs[0].status  = io.vdcregs[1].status  = 0;
    io.vdcregs[0].inc     = io.vdcregs[1].inc     = 1;
    io.vdcregs[0].minline = io.vdcregs[1].minline = 0;
    io.vdcregs[0].maxline = io.vdcregs[1].maxline = 255;

    io.scanline = 0;
    io.prevline = 0;

    memset(VRAMS,0,sizeof(VRAMS));
    memset(VRAM2,0,sizeof(VRAM2));
    memset(vchanges,1,sizeof(vchanges));
    memset(vchange,1,sizeof(vchange));
}

//-------------------------------------------------------------------
// ST_0 / ST_1 / ST_2
// 
// VPCレジスタ 0x0E の内容でアクセス対象が変化する
// 
//-------------------------------------------------------------------
void VDC_d_write(word A,byte V)
{
    A |= (io.vpc[6]&1)<<4;
    VDC_write(A,V);
}

//-------------------------------------------------------------------
// 
// VDC WRITE ACCESS
// 
//-------------------------------------------------------------------
void VDC_write(word A,byte V)
{
    VDC_REG* pV=0;
    int v=0;
    int vdc=0;

    A &= 0x1f;

         if(A<0x08) { vdc=0; }
    else if(A<0x10) { io.vpc[A&7]=V; return; }
    else if(A<0x18) { vdc=1; }
    else             return;
    
    pV = &io.vdcregs[vdc];
    v = pV->reg;
    
    switch(A&3){
      case 0: pV->reg = V&31; return;
      case 1:                     return;
      case 2: // VDC 下位バイト書込み

        pV->VDC[v].B.l = V;

        switch(v){
          case VWR:
            //io.VDC[VWR].B.l = V;
            return;
          case HDR:
            pV->screen_w = ((V&0x7f)+1) * 8;// (V+1)*8;
            break;
          case MWR: {
              static byte bgw[]={32,64,128,128};
              pV->bg_h=(V&0x40)?64:32;
              pV->bg_w=bgw[(V>>4)&3];

              memset(vchange[vdc],1,VRAMSIZE/32);
              memset(vchanges[vdc],1,VRAMSIZE/128);
          }
            //TRACE("bg:%dx%d, V:%X\n",io.bg_w,io.bg_h, V);
            //TRACE("MWRl: %02X\n", V);
            break;
          case BYR:
/*            if (!scroll) {
                oldScrollX = ScrollX;
                oldScrollY = ScrollY;
                oldScrollYDiff = ScrollYDiff;
            }
            //io.VDC[BYR].B.l = V;
            scroll=1;
            ScrollYDiff=scanline-1;
*/
            return;
          case BXR:
/*
            if (!scroll) {
                oldScrollX = ScrollX;
                oldScrollY = ScrollY;
                oldScrollYDiff = ScrollYDiff;
            }
            //io.VDC[BXR].B.l = V;
            scroll=1;
*/
            return;
        }
        
        return;
        
        //-------------------------------------------
        // VDC 上位バイト書込み
        //-------------------------------------------
      case 3:
        pV->VDC[v].B.h = V;

        //printf("vdc_h%d,%02x ",io.vdc_reg,V);
        switch(pV->reg){
          case VWR:
            pV->VRAM[pV->VDC[MAWR].W*2+0]=pV->VDC[VWR].B.l;
            pV->VRAM[pV->VDC[MAWR].W*2+1]=pV->VDC[VWR].B.h;
            
            vchange[vdc][pV->VDC[MAWR].W/16]=1;
            vchanges[vdc][pV->VDC[MAWR].W/64]=1;
            pV->VDC[MAWR].W+=pV->inc;
            return;
          case VDW:
            //io.VDC[VDW].B.l = io.VDC_ratch[VDW];
            //io.VDC[VDW].B.h = V;
            pV->screen_h = (pV->VDC[VDW].W&511)+1;

            MaxLine(vdc) = pV->screen_h-1;
            
            //TRACE("VDWh: %X\n", io.VDC[VDW].W);
            return;
          case LENR: // 12
            //io.VDC[LENR].B.l = io.VDC_ratch[LENR];
            //io.VDC[LENR].B.h = V;
            //TRACE("DMA:%04x %04x %04x\n",io.VDC[DISTR].W,io.VDC[SOUR].W,io.VDC[LENR].W);
            /* VRAM to VRAM DMA */
            
            // スプライトのコピー処理が実行されるぽい？
            memcpy(pV->VRAM+pV->VDC[DISTR].W*2,pV->VRAM+pV->VDC[SOUR].W*2,(pV->VDC[LENR].W+1)*2);

            //変更フラグをたてるっぽい
            memset(vchange[vdc]+pV->VDC[DISTR].W/16,1,(pV->VDC[LENR].W+1)/16);
            memset(vchange[vdc]+pV->VDC[DISTR].W/64,1,(pV->VDC[LENR].W+1)/64);
            pV->VDC[DISTR].W += pV->VDC[LENR].W+1;
            pV->VDC[SOUR].W  += pV->VDC[LENR].W+1;
            pV->VDC[LENR].W = 0;
            
            pV->status|=VDC_DMAfinish;
            return;
            
          case CR :{
              static byte incsize[]={1,32,64,128};
              pV->inc = incsize[(V>>3)&3];
              //TRACE("CRh: %02X\n", V);
          } break;
          case HDR:
            //io.screen_w = (io.VDC_ratch[HDR]+1)*8;
            //TRACE0("HDRh\n");
            break;
          case BYR:
            if (!pV->scroll) {
                pV->oldScrollX = ScrollX(vdc);
                pV->oldScrollY = ScrollY(vdc);
                pV->oldScrollYDiff = pV->ScrollYDiff;
            }
            pV->VDC[BYR].B.h = V&1;
            pV->scroll=1;
            pV->ScrollYDiff=io.scanline-1;
            return;

          case SATB: // 13
            //io.VDC[SATB].B.h = V;
            //TRACE("SATB=%X,scanline=%d\n", io.VDC[SATB].W, scanline);
            pV->satb=1;
            pV->status&=~VDC_SATBfinish;
            return;

          case BXR:
            if(!pV->scroll) {
                pV->oldScrollX = ScrollX(vdc);
                pV->oldScrollY = ScrollY(vdc);
                pV->oldScrollYDiff = pV->ScrollYDiff;
            }
            pV->VDC[BXR].B.h = V & 3;
            pV->scroll=1;
            //			ScrollX = io.VDC[BXR].W;
            //			TRACE("BXRh = %d, scanline = %d\n", io.VDC[BXR].W, scanline);
            //			io.VDC[BXR].W = 256;
            return;
        }
        
        //io.VDC[io.vdc_reg].B.l = io.VDC_ratch[io.vdc_reg];
        //io.VDC[io.vdc_reg].B.h = V;
        //		if (io.vdc_reg != CR)
        //			TRACE("vdc_h: %02X,%02X\n", io.vdc_reg, V);
        //if (io.vdc_reg>19) {
        //    //TRACE("ignore write hi vdc%d,%02x\n",io.vdc_reg,V);
        //}
        return;
    }
}

//-------------------------------------------------------------------
// 
// VDC READ ACCESS
// 
//-------------------------------------------------------------------
byte VDC_read(word A)
{
    byte ret;
    VDC_REG* pV=0;
    int vdc = 0;

    A &= 0x1f;

         if(A<0x08) { vdc=0; }
    else if(A<0x10) { return io.vpc[A&7]; }
    else if(A<0x18) { vdc=1; }
    else             return 0xFF;

    pV = &io.vdcregs[vdc];
    
    switch(A&3){
      case 0:
        ret = pV->status;
        pV->status=0;//&=VDC_InVBlank;//&=~VDC_BSY;
        return ret;
      case 1:
        return 0;
      case 2:
        if (pV->reg==VRR) 
          return pV->VRAM[pV->VDC[MARR].W*2];
        //else return io.VDC[io.vdc_reg].B.l;
      case 3:
        if (pV->reg==VRR) {
            ret = pV->VRAM[pV->VDC[MARR].W*2+1];
            pV->VDC[MARR].W+=pV->inc;
            return ret;
        }
        //else {
        //    return io.VDC[io.vdc_reg].B.h;
        //}
    }
    
    return 0;
}




//=============================================================================
// 
// VDC_SATB_DMA_CHECK
// 
//=============================================================================
int VDC_SATB_DMA_CHECK(void)
{
    int vdc=0;
    
    if(io.vdcregs[vdc].satb_dma_counter ) {
        
        io.vdcregs[vdc].satb_dma_counter--;

        if(io.vdcregs[vdc].satb_dma_counter==0) {
            if(SATBIntON(vdc)) {
                io.vdcregs[vdc].status |= VDC_SATBfinish;
                return INT_IRQ;
            }
        }
    }
    return 0;
}

//=============================================================================
// 
// VDC_SATB_DMA
// 
//=============================================================================
void VDC_SATB_DMA(void)
{
    int vdc;
    VDC_REG *pV;
    
    vdc = 0;
    pV = &io.vdcregs[vdc];
    
    if( pV->satb==1 || pV->VDC[DCR].W&0x0010) {
        memcpy4(pV->SPRAM,pV->VRAM+pV->VDC[SATB].W*2,512/4);
        pV->satb = 1;
        pV->status &= ~VDC_SATBfinish;
        pV->satb_dma_counter = 4;
    }

    vdc = 1;
    pV = &io.vdcregs[vdc];
    
    if( pV->satb==1 || pV->VDC[DCR].W&0x0010) {
        memcpy4(pV->SPRAM,pV->VRAM+pV->VDC[SATB].W*2,512/4);
        pV->satb = 1;
        pV->status &= ~VDC_SATBfinish;
        pV->satb_dma_counter = 4;
    }

}

/*
	Hit Chesk Sprite#0 and others
*/
int CheckSprites(void)
{
    register int i,x,y,w,h;
    register SPR *spr;
    register int x0,y0,w0,h0;

    int vdc = 0;

    spr = io.vdcregs[vdc].SPRAM;
    x0 = spr->x;
    y0 = spr->y;
    w0 = (((spr->atr>>8 )&1)+1)*16;
	h0 = (((spr->atr>>12)&3)+1)*16;
    
	spr++;
    
	for(i=1;i<64;i++) {
		x = spr->x;
		y = spr->y;
		w = (((spr->atr>>8 )&1)+1)*16;
		h = (((spr->atr>>12)&3)+1)*16;
        
        if( (x<x0+w0) && (x+w>x0) && (y<y0+h0) && (y+h>y0) ){
            return TRUE;
        }
        spr++;
	}
    return FALSE;
}
