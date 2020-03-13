/***
*
*	Copyright (c) 1996-2002, Valve LLC. All rights reserved.
*	
*	This product contains software technology licensed from Id 
*	Software, Inc. ("Id Technology").  Id Technology (c) 1996 Id Software, Inc. 
*	All Rights Reserved.
*
*   Use, distribution, and modification of this source code and/or resulting
*   object code is restricted to non-commercial enhancements to products from
*   Valve LLC.  All other use, distribution, or modification is prohibited
*   without written permission from Valve LLC.
*
****/
/* crc.h */
#pragma once
#include "osconf.h"

typedef unsigned int CRC32_t;


#ifdef __cplusplus
extern "C"
{
#endif

void CRC32_Init(CRC32_t *pulCRC);
CRC32_t CRC32_Final(CRC32_t pulCRC);
void CRC32_ProcessByte(CRC32_t *pulCRC, unsigned char ch);
void CRC32_ProcessBuffer(CRC32_t *pulCRC, void *pBuffer, int nBuffer);

#ifdef __cplusplus
}
#endif

unsigned char COM_BlockSequenceCRCByte(unsigned char*base, int length, int sequence);