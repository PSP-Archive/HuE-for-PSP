
#include "stdinc.h"
#include "pce.h"

void TMR_init(void)
{
	io.timer_counter = 0;
	io.timer_reload  = 0;
	io.timer_start   = 0;

}

void TMR_write(word A,byte V)
{
	//TRACE("Timer Access: A=%X,V=%X\n", A, V);
	switch(A&1){
	case 0: io.timer_reload = V&127; return;
	case 1: 
		V&=1;
		if (V && !io.timer_start)
			io.timer_counter = io.timer_reload;
		io.timer_start = V;
		return;
	}
}


byte TMR_read(word A)
{
	switch(A&1){
    case  0: return io.timer_counter;
	default: return 0xff;
    }
	return 0xff;
}

byte TimerInt(M6502 *R)
{
	if (io.timer_start) {
		io.timer_counter--;
		if (io.timer_counter > 128) {
			io.timer_counter = io.timer_reload;
			//io.irq_status &= ~TIRQ;
			if (!(io.irq_mask&TIRQ)) {
				io.irq_status |= TIRQ;
				//TRACE("tirq=%d\n",scanline);
				//TRACE("tirq\n");
				return INT_TIMER;
			}
		}
	}
	return INT_NONE;
}

