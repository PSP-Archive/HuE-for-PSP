#include "stdinc.h"
#include "pce.h"
#include "main.h"

//=============================================================================
//
//
//=============================================================================
void IRQ_init(void)
{
	io.irq_mask = 0;
    io.irq_status = 0;
}


//=============================================================================
//
//
//=============================================================================
byte IRQ_read(word A)
{
    register byte ret;
    
    switch(A&3){
      case 2: return io.irq_mask;
      case 3: ret = io.irq_status;
        io.irq_status=0;
        return ret;
    }

    return 0xff;
}

//=============================================================================
//
//
//=============================================================================
void IRQ_write(word A,byte V)
{
    switch(A&3){
      case 2: io.irq_mask = V;/*TRACE("irq_mask = %02X\n", V);*/ return;
      case 3: io.irq_status= (io.irq_status&~TIRQ)|(V&0xF8); return;
    }
}
