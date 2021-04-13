//-----------------------------------------------------------------------------
// ピクセルフォーマットは RGBA(5551)でAはMSB
// ３種類(SP0/SP1/BG)のピクセル属性を判別するため以下の条件を付ける。
// ※ピクセル属性は別領域で実装可能だがメモリ帯域を考慮した上で決定です。
//
// SP描画Pixel : MSB=1
// BG描画Pixel : LSB=1
// LSBは画素の色情報を保持する有効BITであるが最大誤差+1なので無視する。
//-----------------------------------------------------------------------------

#include "stdinc.h"
#include "pce.h"

#define uint unsigned int

static PALFMT pal_lut[512]; // Color Lookup Table
static uint bCvtTbl[65536]; // Bit Convert Table


#define B_TBL(a,b)    (bCvtTbl[(word)a] | (bCvtTbl[(word)b]<<2))
#define B_TBL00(a,b)  (bCvtTbl[(word)a] | (bCvtTbl[(word)b]<<2))
#define B_TBL16(a,b)  (bCvtTbl[(word)(a>>16)] | (bCvtTbl[(word)(b>>16)]<<2))


//-----------------------------------------------------------------------------
// 
// VRAMからBGをエンコードする
// 
//-----------------------------------------------------------------------------
void plane2pixel(int vdc,int no)
{
    register DWORD L0,L1;
    register DWORD* C = (DWORD*)((word*)io.vdcregs[vdc].VRAM + no*16);
    register DWORD* C2= (DWORD*)&VRAM2[vdc][no*8];

    L0=C[0]; L1=C[4];  C2[0]=B_TBL00(L0,L1);  C2[1]=B_TBL16(L0,L1);
    L0=C[1]; L1=C[5];  C2[2]=B_TBL00(L0,L1);  C2[3]=B_TBL16(L0,L1);
    L0=C[2]; L1=C[6];  C2[4]=B_TBL00(L0,L1);  C2[5]=B_TBL16(L0,L1);
    L0=C[3]; L1=C[7];  C2[6]=B_TBL00(L0,L1);  C2[7]=B_TBL16(L0,L1);
}

//-----------------------------------------------------------------------------
//
// VRAMからBGをエンコードする
//
//-----------------------------------------------------------------------------
void sprite2pixel(int vdc,int no)
{
    register DWORD L0,L1,L2,L3;
    register byte La,Lb,Lc,Ld;
    register word Wa,Wb;
    register int i;

    register DWORD* C  = (DWORD*)((word*)io.vdcregs[vdc].VRAM + no*64);
    register DWORD* C2 = (DWORD*)&VRAMS[vdc][no*32];

    for(i=0;i<8;i++) {
        L0=C[i]; L1=C[i+8]; L2=C[i+16]; L3=C[i+24];
        
        La=L0;      Lb=L1;      Lc=L2;     Ld=L3;
        Wa=(word)Lb<<8|La; Wb=(word)Ld<<8|Lc;  *C2++ = B_TBL00(Wa,Wb);
        
        La=L0>> 8;  Lb=L1>> 8;  Lc=L2>> 8; Ld=L3>> 8;
        Wa=(word)Lb<<8|La; Wb=(word)Ld<<8|Lc;  *C2++ = B_TBL00(Wa,Wb);
        
        La=L0>>16;  Lb=L1>>16;  Lc=L2>>16; Ld=L3>>16;
        Wa=(word)Lb<<8|La; Wb=(word)Ld<<8|Lc;  *C2++ = B_TBL00(Wa,Wb);
        
        La=L0>>24;  Lb=L1>>24;  Lc=L2>>24; Ld=L3>>24;
        Wa=(word)Lb<<8|La; Wb=(word)Ld<<8|Lc;  *C2++ = B_TBL00(Wa,Wb);
    }
}



//-------------------------------------------------------------------
// 
// VDC INITIALIZE
// 
//-------------------------------------------------------------------
void VCE_init(void)
{
    uint i,r,g,b;
    uint lut[8] = {0,4,9,13,18,22,27,31};

    for(i=0;i<512;i++) {
        b = 7 &  i;
        r = 7 & (i>>3);
        g = 7 & (i>>6);
        pal_lut[i] = (lut[b]<<10) | (lut[g]<<5) | lut[r];

        // SPとBGのマスク処理のため上下1bitをワークとして使う
        pal_lut[i]&= 0x7ffe;
    }
    

    // FAST BIT CONVERT TABLE
    {
        for(i=0;i<256;i++) {
            bCvtTbl[i] = ((i&0x80)<<21)|((i&0x40)<<18)|((i&0x20)<<15)|((i&0x10)<<12)
              | ((i&0x08)<< 9)|((i&0x04)<< 6)|((i&0x02)<< 3)|((i&0x01)    );
            
        }
        
        for(i=256;i<65536;i++) {
            bCvtTbl[i] = (bCvtTbl[(i>>8)]<<1) | (bCvtTbl[(i&0xff)]<<0);
        }
    }

    
    io.vce_cr = 0;
    io.vce_reg.W = 0;
}

//-------------------------------------------------------------------
// 
// VCE WRITE ACCESS
// 
//-------------------------------------------------------------------
void VCE_write(word A, byte V)
{
    A = A & 7;
    
    switch(A) {
      case 0: io.vce_cr = V&0x87;            break;
      case 1: /*TRACE("VCE 1, V=%X\n", V);*/ break;
      case 2: io.vce_reg.B.l = V;            break;
      case 3: io.vce_reg.B.h = V&1;          break;
      case 4: io.VCE[io.vce_reg.W].B.l= V;   break;
      case 5: {
          register int i;
          register int n = io.vce_reg.W;
          register int c;
          
          io.VCE[n].B.h = V;
          c = io.VCE[n].W & 0x1ff;
          
          // update palette
          if(n==0)        for(i=  0;i<256;i+=16)  io.Pal[i]=pal_lut[c];        // BG blank
          else if(n==256) for(i=256;i<512;i+=16)  io.Pal[i]=pal_lut[c];        // SP blank
          else if(n&0x100)                        io.Pal[n]=pal_lut[c]|0x8000; // SP color
          else if(n&15)                           io.Pal[n]=pal_lut[c]|0x0001; // BG color
          
          io.vce_reg.W=(io.vce_reg.W+1)&0x1FF;
      }
        return;
      case 6:	/*TRACE("VCE 6, V=%X\n", V);*/ break;
      case 7:	/*TRACE("VCE 7, V=%X\n", V);*/ break;
    }
    
}

//-------------------------------------------------------------------
// 
// VCE READ ACCESS
// 
//-------------------------------------------------------------------
byte VCE_read(word A)
{
    A = A & 7;

    if(A==4) {
        return io.VCE[io.vce_reg.W].B.l;
    } else
    if(A==5) {
        byte v = io.VCE[io.vce_reg.W].B.h;
        io.vce_reg.W = (io.vce_reg.W+1) & 0x1ff;
        return v;
    }

    return 0;
}

