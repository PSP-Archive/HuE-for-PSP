#include "stdinc.h"
#include "string.h"
#include "pce.h"
#include "main.h"

//
//
//
void JOY_init(void)
{
    int i;
    for(i=0;i<5;i++) {
        io.shiftmode[i] = 0;
    }
}


byte JOY_read(word A)
{
    byte ret = (io.JOY[io.joy_counter]>>io.shiftmode[io.joy_counter])^0xff;
    if (io.joy_select&1) ret>>=4;
    else {
        ret&=15;
        if(io.JOY[io.joy_counter] & 0xf000) {
            io.shiftmode[io.joy_counter] = 8 - io.shiftmode[io.joy_counter];
        }
        io.joy_counter=(io.joy_counter+1)%5;
    }
    return ret; // |Country; /* country 0:JPN 1=US */
}



void JOY_write(word A,byte V)
{
    io.joy_select = V&1;
    if (V&2) io.joy_counter = 0;
}



