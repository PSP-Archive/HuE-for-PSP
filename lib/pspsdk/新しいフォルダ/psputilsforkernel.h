/*
 * PSP Software Development Kit - http://www.pspdev.org
 * -----------------------------------------------------------------------
 * Licensed under the BSD license, see LICENSE in PSPSDK root for details.
 *
 * psputilsforkernel.h - Include file for UtilsForKernel
 *
 * Copyright (c) 2005 John Kelley <ps2dev@kelley.ca>
 * Copyright (c) 2005 adresd
 *
 * $Id: psputilsforkernel.h 1095 2005-09-27 21:02:16Z jim $
 */

#ifndef __PSPUTILSFORKERNEL_H__
#define __PSPUTILSFORKERNEL_H__

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Decompress gzip'd data (requires kernel mode)
 *
 * @param dest - pointer to destination buffer
 * @param destSize - size of destination buffer
 * @param src - pointer to source (compressed) data
 * @param unknown - unknown, pass NULL
 * @return size decompressed on success, < 0 on error
 */
int sceKernelGzipDecompress(u8 *dest, u32 destSize, const u8 *src, u32 unknown);

#ifdef __cplusplus
}
#endif

#endif
