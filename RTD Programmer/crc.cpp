#include "stdafx.h"
#include "crc.h"

static unsigned gCrc = 0;

void InitCRC() {
  gCrc = 0;
}

void ProcessCRC(const uint8_t *data, int len)
{
	int i, j;
	for (j = len; j; j--, data++) {
		gCrc ^= (*data << 8);
		for(i = 8; i; i--) {
			if (gCrc & 0x8000)
				gCrc ^= (0x1070 << 3);
			gCrc <<= 1;
		}
	}
}

uint8_t GetCRC() {
	return (uint8_t)(gCrc >> 8);
}
