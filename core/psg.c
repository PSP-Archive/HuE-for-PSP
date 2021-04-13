#ifdef SOUND

#include "stdinc.h"
#include "pce.h"
#include "pg.h"
#include "string.h"

#define   CHANNEL             io.psg_ch
#define   PSGreg(ch,num)      io.PSG[(ch)][(num)]
#define   WAVreg(ch,num)      io.wave[(ch)][(num)]

//-------------------------------------------------------------------
// 
// 
// 
//-------------------------------------------------------------------
void PSG_init(void)
{
    int i;

    for(i=0;i<8;i++) {
        io.PSG[i][4] = 0x80;
    }
    
    io.psg_volume = 0;
    io.psg_ch = 0;
}


//-------------------------------------------------------------------
// 
// PSGに対するWRITEアクセスを記述する
// 
//-------------------------------------------------------------------
void PSG_write(word A,byte V)
{
    if(io.psg_ch>5) {
        if((A&15)==0) io.psg_ch = V&7;
    }
    else {
        // 後方のswitchで音に関する変更をしない場合はスキップする
        if (io.psg_ch<6 && (A&15)>0 && (A&15)<8 ) {
            if ((A&15)==1) {
                write_psg(0);
                write_psg(1);
                write_psg(2);
                write_psg(3);
                write_psg(4);
                write_psg(5);
            } else {
                write_psg(io.psg_ch);
            }
        }
        
        switch(A&15){
          case 0: io.psg_ch = V&7;             return; // 音に影響なし
          case 1: io.psg_volume = V;           break;
          case 2: io.PSG[io.psg_ch][2] = V;    break;
          case 3: io.PSG[io.psg_ch][3] = V&15; break;
          case 4: io.PSG[io.psg_ch][4] = V;    break;
          case 5: io.PSG[io.psg_ch][5] = V;    break;
          case 6:
        if (io.PSG[io.psg_ch][4]&0x40){
            io.wave[io.psg_ch][0]=V&31;
        }else {
            io.wave[io.psg_ch][io.wavofs[io.psg_ch]]=V&31;
            io.wavofs[io.psg_ch]=(io.wavofs[io.psg_ch]+1)&31;
        } break;
          case 7: io.PSG[io.psg_ch][7] = V;    break;
          case 8: io.psg_lfo_freq = V;         return; // 音に影響なし
          case 9: io.psg_lfo_ctrl = V;         return; // 音に影響なし
          default: //TRACE("ignored PSG write\n");
            return; // 音に影響なし
            break;
        }
    }
}

//-------------------------------------------------------------------
// 
// PSGに対するREADアクセスを記述する
// 
//-------------------------------------------------------------------
byte PSG_read(word A)
{
    if((A&15)==0) return io.psg_ch;

    if(io.psg_ch>5) return NODATA;

    switch(A&15){
      case 0: return io.psg_ch;
      case 1: return io.psg_volume;
      case 2: return io.PSG[io.psg_ch][2];
      case 3: return io.PSG[io.psg_ch][3];
      case 4: return io.PSG[io.psg_ch][4];
      case 5: return io.PSG[io.psg_ch][5];
      case 6: return io.wave[io.psg_ch][io.wavofs[CHANNEL]];
      case 7: return io.PSG[io.psg_ch][7];
      case 8: return io.psg_lfo_freq;
      case 9: return io.psg_lfo_ctrl;
      default:
        return NODATA;
    }
    return NODATA;
}




#endif
