//
// pceregs.h
//
// ïœêîíËã`Ç∆é¿ëÃíËã`
//
#ifndef __PCEREGS_H_DEFINED__
#define __PCEREGS_H_DEFINED__

#ifndef EXTERN
#define EXTERN extern
#define INIT_VALUE(value)
#endif//EXTERN

#ifndef INIT_VALUE
#define INIT_VALUE(value) =(value)
#endif

EXTERN IO  io;
EXTERN CD  cd;
EXTERN ACD acd;
EXTERN byte  WRAM[0x2000];
EXTERN byte* RAM            INIT_VALUE(io.RAM);

EXTERN int g_skip_next_vsync  INIT_VALUE(0) ;
EXTERN int g_skip_next_frame  INIT_VALUE(0);
EXTERN int frame_counter    INIT_VALUE(0);
EXTERN int BaseClock        INIT_VALUE(7160000);
EXTERN int UPeriod          INIT_VALUE(0);

EXTERN byte IOAREA[0x2000];

EXTERN unsigned int VRAM2[2][VRAMSIZE / sizeof(int)];
EXTERN unsigned int VRAMS[2][VRAMSIZE / sizeof(int)];
EXTERN byte vchange[2][VRAMSIZE / 32];
EXTERN byte vchanges[2][VRAMSIZE/128];

EXTERN int TimerCount;
//EXTERN int CycleOld;
EXTERN int TimerPeriod;
EXTERN int scanlines_per_frame INIT_VALUE(263);
//EXTERN int prevline;
//EXTERN int scanline;

EXTERN byte populus;
EXTERN byte *ROM;
EXTERN int ROM_size;
EXTERN int Country;
EXTERN int IPeriod;


EXTERN byte PopRAM[0x10000];
EXTERN byte *Page[8];
EXTERN byte *ROMMap[256];

EXTERN PALFMT * XBuf;


#endif
