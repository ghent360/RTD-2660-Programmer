// becker_read.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <stdio.h>

static CCyUSBDevice USBDevice;
static CCyControlEndPoint *ept = USBDevice.ControlEndPt;

static BYTE ReadNakCnt() 
{
	ept->Target = TGT_DEVICE;
	ept->ReqType = REQ_VENDOR;
	ept->Direction = DIR_FROM_DEVICE;
	ept->ReqCode = 0xA3;
	ept->Value = 0;
	ept->Index = 0;

  UCHAR buf[1];
	LONG buflen = sizeof(buf);
  ept->XferData(buf, buflen);
  return buf[0];
}

static void SetI2CAddr(BYTE value) 
{
	ept->Target = TGT_DEVICE;
	ept->ReqType = REQ_VENDOR;
	ept->Direction = DIR_TO_DEVICE;
	ept->ReqCode = 0xA5;
	ept->Value = 0;
	ept->Index = 0;

  UCHAR buf[1];
  LONG buflen =  1;
	buf[0] = value;
  ept->XferData(buf, buflen);
}

static void WriteBytesToAddr(BYTE reg, BYTE* values, BYTE len)
{
	ept->Target    = TGT_DEVICE;
  ept->ReqType   = REQ_VENDOR;
  ept->Direction = DIR_TO_DEVICE;
  ept->ReqCode   = 0xA2;
  ept->Value     = 0;
  ept->Index     = 0;

  UCHAR buf[64];
  if (len > 63)
  {
    len = 63;
  }
  LONG buflen =  len + 1;
	buf[0] = reg;
  for(int idx = 0; idx < len; idx++)
  {
    buf[1 + idx] = values[idx];
  }

  ept->XferData(buf, buflen);
}

static void ReadBytes(BYTE *dest) 
{
	ept->Target = TGT_DEVICE;
	ept->ReqType = REQ_VENDOR;
	ept->Direction = DIR_FROM_DEVICE;
	ept->ReqCode = 0xA4;
	ept->Value = 0;
	ept->Index = 0;

  UCHAR buf[64];
	LONG buflen = sizeof(buf);
  ept->XferData(buf, buflen);
  memcpy(dest, buf, buflen);
}

static void ReadBytesFromAddr(BYTE reg, BYTE* dest, BYTE len)
{
	ept->Target    = TGT_DEVICE;
  ept->ReqType   = REQ_VENDOR;
  ept->Direction = DIR_TO_DEVICE;
  ept->ReqCode   = 0xA1;
  ept->Value     = 0;
  ept->Index     = 0;

  UCHAR buf[2];
  LONG buflen = sizeof(buf);
	buf[0] = reg;
	buf[1] = len;
  ept->XferData(buf, buflen);
  ReadBytes(dest);
}

static BYTE ReadReg(BYTE reg)
{
  BYTE result;
  ReadBytesFromAddr(reg, &result, 1);
  return result;
}

static bool WriteReg(BYTE reg, BYTE value)
{
  WriteBytesToAddr(reg, &value, 1);
  return ReadNakCnt() == 0;
}

int _tmain(int argc, _TCHAR* argv[])
{
  static char buffer[128];
  if (!USBDevice.IsOpen()) {
    printf("Can't connect to the USB device. Check the cable.\n");
    return -1;
  }
  printf("Ready\n");
  USBDevice.Close();
	return 0;
}
