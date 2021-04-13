//------------------------------------------------------------------------------
//
// キー入力を処理するためのファイル
//
//------------------------------------------------------------------------------

#include "stdinc.h"
#include "syscall.h"
#include "pg.h"
#include "main.h"
#include "pce.h"

#define CTRL_EXIT(bt) ((CTRL_SELECT|CTRL_LTRIGGER)==((bt)&(CTRL_SELECT|CTRL_LTRIGGER)))

extern int menu_flag;
extern EmuRuntime eRun;
extern EmuConfig  eConf;

//--------------------------------------------------------------------------
// 
// Key Input
// 
//--------------------------------------------------------------------------
int JoyStick(short* JS)
{
	register int bt = 0;
    ctrl_data_t paddata;
    static int old_bt=0;

    JS[0] = JS[1] = JS[2] = JS[3] = JS[4] = 0;

    if(PSP_Is()) {
        sceCtrlPeekBufferPositive(&paddata, 1);
        
        bt = paddata.buttons;
        if(paddata.Lx < 43) bt |= CTRL_A_LEFT ;
        if(paddata.Lx >211) bt |= CTRL_A_RIGHT;
        if(paddata.Ly < 43) bt |= CTRL_A_UP   ;
        if(paddata.Ly >211) bt |= CTRL_A_DOWN ;
    } else {
        sceCtrlReadBufferPositive(&paddata, 1);
        bt = paddata.buttons;
    }

    // AUTOFIRE [ON/OFF] Toggle Change
    if(bt & eConf.autoflag) {
        int newbt = (bt & ~old_bt) & eConf.autoflag;
        if(newbt) {
            int msk = (newbt & eConf.autoflag);
            eConf.autofire = eConf.autofire ^ msk;
            old_bt = bt;

            core_error("Autofire Toggled");
        }
    } else {
        old_bt = 0;
    }
    
    bt |= eConf.autofire;

    if(bt) {
        register short js = 0;
        register int mask = 0;
        register int fc = (frame_counter&31);
        register unsigned int *rapm = eConf.rapm[eConf.button6];
        register unsigned int *pKey = eConf.key[eConf.button6];
        
        if( CTRL_EXIT(bt)      ) mask |= 0x80000000; 
        if( bt & CTRL_UP       ) mask |= pKey[0x0] * ((rapm[0x0]>>fc)&1);
        if( bt & CTRL_RIGHT    ) mask |= pKey[0x1] * ((rapm[0x1]>>fc)&1);
        if( bt & CTRL_DOWN     ) mask |= pKey[0x2] * ((rapm[0x2]>>fc)&1);
        if( bt & CTRL_LEFT     ) mask |= pKey[0x3] * ((rapm[0x3]>>fc)&1);
        if( bt & CTRL_A_UP     ) mask |= pKey[0x4] * ((rapm[0x4]>>fc)&1);
        if( bt & CTRL_A_RIGHT  ) mask |= pKey[0x5] * ((rapm[0x5]>>fc)&1);
        if( bt & CTRL_A_DOWN   ) mask |= pKey[0x6] * ((rapm[0x6]>>fc)&1);
        if( bt & CTRL_A_LEFT   ) mask |= pKey[0x7] * ((rapm[0x7]>>fc)&1);
        if( bt & CTRL_TRIANGLE ) mask |= pKey[0x8] * ((rapm[0x8]>>fc)&1);
        if( bt & CTRL_CIRCLE   ) mask |= pKey[0x9] * ((rapm[0x9]>>fc)&1);
        if( bt & CTRL_CROSS    ) mask |= pKey[0xa] * ((rapm[0xa]>>fc)&1);
        if( bt & CTRL_SQUARE   ) mask |= pKey[0xb] * ((rapm[0xb]>>fc)&1);
        if( bt & CTRL_LTRIGGER ) mask |= pKey[0xc] * ((rapm[0xc]>>fc)&1);
        if( bt & CTRL_RTRIGGER ) mask |= pKey[0xd] * ((rapm[0xd]>>fc)&1);
        if( bt & CTRL_SELECT   ) mask |= pKey[0xe] * ((rapm[0xe]>>fc)&1);
        if( bt & CTRL_START    ) mask |= pKey[0xf] * ((rapm[0xf]>>fc)&1);

        // go to Menu
        if( mask & 0x80000000 ) {
            switch(menu_Main()){
              case STATE_QUIT:   PSP_GoHome();             return 0x10000;
              case STATE_RESET:  menu_flag = STATE_RESET;  return 0x10000;
              case STATE_ROM:    menu_flag = STATE_ROM;    return 0x10000;
              default:           js = 0;                   break;
            }
        }
        else {
            js = (mask&0x0ffff) | (eConf.button6 * JOY_SIX);
        }
        
        JS[eConf.padno] = js;
    }

    if(PSP_IsEsc()) {
        return 0x10000;
    }
    
    return 0;
}

