#include "stdinc.h"
#include "string.h"
#include "pce.h"

#include "main.h"

#define AC_ENABLE_OFFSET_BASE_6 0x40
#define AC_ENABLE_OFFSET_BASE_A 0x20
#define AC_INCREMENT_BASE 0x10
#define AC_USE_OFFSET 0x02
#define AC_ENABLE_INC 0x01

//
//
//
void ACD_init(void)
{
    memset(&acd,0,sizeof(acd));
}


//
//
//
void ACD_write(word A,byte V)
{
  if ((A&0x1AE0)==0x1AE0) {
        switch (A & 15) {
          case 0: acd.ac_shift = (acd.ac_shift & 0xffffff00) | V;          break;
          case 1: acd.ac_shift = (acd.ac_shift & 0xffff00ff) | (V << 8);   break;
          case 2: acd.ac_shift = (acd.ac_shift & 0xff00ffff) | (V << 16);  break;
          case 3: acd.ac_shift = (acd.ac_shift & 0x00ffffff) | (V << 24);  break;
          case 4: acd.ac_shiftbits = V & 0x0f;
                  if(acd.ac_shiftbits != 0) {
                    if(acd.ac_shiftbits < 8) {
                      acd.ac_shift <<= acd.ac_shiftbits;
                    }
                    else {
                      acd.ac_shift >>= (16 - acd.ac_shiftbits);
                    }
                  }
          case 5:
            acd.ac_unknown4 = V;
          default: break;
        }
        return;
    }
    else {
        byte ac_port = (A >> 4) & 3;
        switch (A & 15) {
          case 0:
          case 1:
            if (acd.ac_control[ac_port] & AC_USE_OFFSET) {
                // Log("Write %d to 0x%04X (base + offset)\n",V,
                //           acd.ac_offset[ac_port] + acd.ac_base[ac_port]);
//                acd.ac_extra_mem[((acd.ac_base[ac_port] + acd.ac_offset[ac_port]) & 0x1fffff)] = V;
                acd.ac_extra_mem[((acd.ac_base[ac_port] + acd.ac_offset[ac_port]) & 0x1fffff)] = V;
            }
            else {
                // Log("Write %d to 0x%04X (base)\n",V, acd.ac_base[ac_port]);
//                acd.ac_extra_mem[((acd.ac_base[ac_port]) & 0x1fffff)] = V;
                acd.ac_extra_mem[((acd.ac_base[ac_port]) & 0x1fffff)] = V;
            }
            
            if (acd.ac_control[ac_port] & AC_ENABLE_INC) {
                if (acd.ac_control[ac_port] & AC_INCREMENT_BASE)
                  acd.ac_base[ac_port] = (acd.ac_base[ac_port] + acd.ac_incr[ac_port]) & 0xffffff;
                else
                  acd.ac_offset[ac_port] = (acd.ac_offset[ac_port] + acd.ac_incr[ac_port]) & 0xffffff;
            }
            
            //diffdiff
            return ;
            //diffdiff
          case  2:  acd.ac_base[ac_port] = (acd.ac_base[ac_port] & 0xffff00) | V;          return;
          case  3:  acd.ac_base[ac_port] = (acd.ac_base[ac_port] & 0xff00ff) | (V << 8);   return;
          case  4:  acd.ac_base[ac_port] = (acd.ac_base[ac_port] & 0x00ffff) | (V << 16);  return;
          case  5:  acd.ac_offset[ac_port] = (acd.ac_offset[ac_port] & 0xff00) | V;        return;
          case  6:  acd.ac_offset[ac_port] = (acd.ac_offset[ac_port] & 0x00ff) | (V << 8);
                    if(acd.ac_control[ac_port] & (AC_ENABLE_OFFSET_BASE_6))
                       acd.ac_base[ac_port] = (acd.ac_base[ac_port] + acd.ac_offset[ac_port]) & 0xffffff;
                    return;
          case  7:  acd.ac_incr[ac_port] = (acd.ac_incr[ac_port] & 0xff00) | V;            return;
          case  8:  acd.ac_incr[ac_port] = (acd.ac_incr[ac_port] & 0x00ff) | (V << 8);     return;
          case  9:  acd.ac_control[ac_port] = V;                                          return;
          case 10:  if (acd.ac_control[ac_port] & (AC_ENABLE_OFFSET_BASE_A | AC_ENABLE_OFFSET_BASE_6) == (AC_ENABLE_OFFSET_BASE_A | AC_ENABLE_OFFSET_BASE_6))
                        acd.ac_base[ac_port] = (acd.ac_base[ac_port] + acd.ac_offset[ac_port]) & 0xffffff;
            return;
          default:
            //Log ("\nUnknown AC write %d into 0x%04X\n", V, A);
			  ;
        }
        
    }
}


//
//
//
byte ACD_read(word A)
{
    if((A&0x1AE0)==0x1AE0) {
		switch(A&0x0f) {
		case  0: return (byte) (acd.ac_shift);
		case  1: return (byte) (acd.ac_shift >> 8);
		case  2: return (byte) (acd.ac_shift >> 16);
	    case  3: return (byte) (acd.ac_shift >> 24);
		case  4: return acd.ac_shiftbits;
		case  5: return acd.ac_unknown4;
		case 14: return 0x10;
	    case 15: return 0x51;
	    default: return 0xff;
		}
	}
	else {
        byte ac_port = (A >> 4) & 3;
        byte ret = 0;
        switch (A & 15) {
          case 0x00:
          case 0x01:
            if(acd.ac_control[ac_port] & AC_USE_OFFSET)
//              ret = acd.ac_extra_mem[((acd.ac_base[ac_port] + acd.ac_offset[ac_port]) & 0x1fffff)];
              ret = acd.ac_extra_mem[((acd.ac_base[ac_port] + acd.ac_offset[ac_port]) & 0x1fffff)];
            else
//              ret = acd.ac_extra_mem[((acd.ac_base[ac_port]) & 0x1fffff)];
              ret = acd.ac_extra_mem[((acd.ac_base[ac_port]) & 0x1fffff)];
            
            if(acd.ac_control[ac_port] & AC_ENABLE_INC) {
                if (acd.ac_control[ac_port] & AC_INCREMENT_BASE)
                  acd.ac_base[ac_port] = (acd.ac_base[ac_port] + acd.ac_incr[ac_port]) & 0xffffff;
                else
                  acd.ac_offset[ac_port] = (acd.ac_offset[ac_port] + acd.ac_incr[ac_port]) & 0xffff;
            }
            return ret;
          case 0x02: return (byte) (acd.ac_base[ac_port]);
          case 0x03: return (byte) (acd.ac_base[ac_port] >> 8);
          case 0x04: return (byte) (acd.ac_base[ac_port] >> 16);
          case 0x05: return (byte) (acd.ac_offset[ac_port]);
          case 0x06: return (byte) (acd.ac_offset[ac_port] >> 8);
          case 0x07: return (byte) (acd.ac_incr[ac_port]);
          case 0x08: return (byte) (acd.ac_incr[ac_port] >> 8);
          case 0x09: return acd.ac_control[ac_port];
          case 0x0a: return 0;
          default: return 0xff;
		}
    }

	return 0xff;
}


