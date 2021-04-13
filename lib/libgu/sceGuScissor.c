/*
 * PSP Software Development Kit - http://www.pspdev.org
 * -----------------------------------------------------------------------
 * Licensed under the BSD license, see LICENSE in PSPSDK root for details.
 *
 * Copyright (c) 2005 Jesper Svennevid
 */

#include "guInternal.h"

void sceGuScissor(int x, int y, int w, int h)
{
	GuContext* context = &gu_contexts[gu_curr_context];

	context->scissor_start[0] = x;
	context->scissor_start[1] = y;
	context->scissor_end[0] = (x+w)-1;
	context->scissor_end[1] = (y+h)-1;

	if (context->scissor_enable)
	{
		sendCommandi(212,(y << 10)|x);
		sendCommandi(213,(((y+h)-1) << 10)|(((x+w)-1)));
	}
}
