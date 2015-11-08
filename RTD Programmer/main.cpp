// main.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <stdio.h>
#include <stdint.h>

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

enum ECommondCommandType {
  E_CC_NOOP = 0,
  E_CC_WRITE = 1,
  E_CC_READ = 2,
  E_CC_WRITE_AFTER_WREN = 3,
  E_CC_WRITE_AFTER_EWSR = 4,
  E_CC_ERASE = 5
};

uint32_t SPICommonCommand(ECommondCommandType cmd_type,
    uint8_t cmd_code,
    uint8_t num_reads,
    uint8_t num_writes,
    uint32_t write_value) {
  num_reads &= 3;
  num_writes &= 3;
  write_value &= 0xFFFFFF;
  BYTE reg_value = (cmd_type << 5) | 
    (num_writes << 3) |
    (num_reads << 1);

  WriteReg(0x60, reg_value);
  WriteReg(0x61, cmd_code);
  if (num_writes > 0) {
    WriteReg(0x64, write_value >> 16);
    WriteReg(0x65, write_value >> 8);
    WriteReg(0x66, write_value);
  }
  WriteReg(0x60, reg_value | 1); // Execute the command
  BYTE b;
  do {
    b = ReadReg(0x60);
  } while (b & 1);  // TODO: add timeout and reset the controller
  switch (num_reads) {
  case 0: return 0;
  case 1: return ReadReg(0x67);
  case 2: return (ReadReg(0x67) << 8) | ReadReg(0x68);
  case 3: return (ReadReg(0x67) << 16) | (ReadReg(0x68) << 8) | ReadReg(0x69);
  }
  return 0;
}

void SPIRead(uint32_t address, uint8_t *data, int32_t len) {
  WriteReg(0x60, 0x46);
  WriteReg(0x61, 0x3);
  WriteReg(0x64, address>>16);
  WriteReg(0x65, address>>8);
  WriteReg(0x66, address);
  WriteReg(0x60, 0x47); // Execute the command
  BYTE b;
  do {
    b = ReadReg(0x60);
  } while (b & 1);  // TODO: add timeout and reset the controller
  while (len > 0) {
    int32_t read_len = len;
    if (read_len > 64)
      read_len = 64;
    ReadBytesFromAddr(0x70, data, read_len);
    data += read_len;
    len -= read_len;
  }
}

struct FlashDesc {
  const char* device_name;
  uint32_t    jdec_id;
  uint32_t    size_kb;
  uint32_t    page_size;
  uint32_t    block_size_kb;
};

static const FlashDesc FlashDevices[] = {
    // name,        Jedec ID,      sizeK, page size, block sizeK
    {"AT25DF041A" , 0x1F4401,      512, 256, 64},
    {"AT25DF161"  , 0x1F4602, 2 * 1024, 256, 64},
    {"AT26DF081A" , 0x1F4501, 1 * 1024, 256, 64},
    {"AT26DF0161" , 0x1F4600, 2 * 1024, 256, 64},
    {"AT26DF161A" , 0x1F4601, 2 * 1024, 256, 64},
    {"AT25DF321" ,  0x1F4701, 4 * 1024, 256, 64},
    {"AT25DF512B" , 0x1F6501,       64, 256, 32},
    {"AT25DF512B" , 0x1F6500,       64, 256, 32},
    {"AT25DF021"  , 0x1F3200,      256, 256, 64},
    {"AT26DF641" ,  0x1F4800, 8 * 1024, 256, 64},
    // Manufacturer: ST 
    {"M25P05"     , 0x202010,       64, 256, 32},
    {"M25P10"     , 0x202011,      128, 256, 32},
    {"M25P20"     , 0x202012,      256, 256, 64},
    {"M25P40"     , 0x202013,      512, 256, 64},
    {"M25P80"     , 0x202014, 1 * 1024, 256, 64},
    {"M25P16"     , 0x202015, 2 * 1024, 256, 64},
    {"M25P32"     , 0x202016, 4 * 1024, 256, 64},
    {"M25P64"     , 0x202017, 8 * 1024, 256, 64},
    // Manufacturer: Windbond 
    {"W25X10"     , 0xEF3011,      128, 256, 64},
    {"W25X20"     , 0xEF3012,      256, 256, 64},
    {"W25X40"     , 0xEF3013,      512, 256, 64},
    {"W25X80"     , 0xEF3014, 1 * 1024, 256, 64},
    // Manufacturer: Macronix 
    {"MX25L512"   , 0xC22010,       64, 256, 64},
    {"MX25L3205"  , 0xC22016, 4 * 1024, 256, 64},
    {"MX25L6405"  , 0xC22017, 8 * 1024, 256, 64},
    {"MX25L8005"  , 0xC22014,     1024, 256, 64},
    // Microchip
    {"SST25VF512" , 0xBF4800,       64, 256, 32},
    {"SST25VF032" , 0xBF4A00, 4 * 1024, 256, 32},
    {NULL , 0, 0, 0, 0}
};

void PrintManufacturer(uint32_t id) {
  switch (id) {
  case 0x20: printf("ST"); break;
  case 0xef: printf("Winbond"); break;
  case 0x1f: printf("Atmel"); break;
  case 0xc2: printf("Macronix"); break;
  case 0xbf: printf("Microchip"); break;
  default: printf("Unknown");break;
  }
}

const FlashDesc* FindChip(uint32_t jdec_id) {
  const FlashDesc* chip = FlashDevices;
  while (chip->jdec_id != 0) {
    if (chip->jdec_id == jdec_id)
      return chip;
    chip++;
  }
  return NULL;
}

static unsigned gCrc;

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

uint8_t SPIComputeCRC(uint32_t start, uint32_t end) {
  WriteReg(0x64, start >> 16);
  WriteReg(0x65, start >> 8);
  WriteReg(0x66, start);

  WriteReg(0x72, end >> 16);
  WriteReg(0x73, end >> 8);
  WriteReg(0x74, end);

  WriteReg(0x6f, 0x84);
  BYTE b;
  do
  {
    b = ReadReg(0x6f);
  } while (!(b & 0x2));
  return ReadReg(0x75);
}

int _tmain(int argc, _TCHAR* argv[])
{
  BYTE b;
  if (!USBDevice.IsOpen()) {
    printf("Can't connect to the USB device. Check the cable.\n");
    return -1;
  }
  printf("Ready\n");
  SetI2CAddr(0x4a);

  if (!WriteReg(0x6f, 0x80)) {  // Enter ISP mode
    printf("Write to 6F failed.\n");
    return -2;
  }
  b = ReadReg(0x6f);
  if (!(b & 0x80)) {
    printf("Can't enable ISP mode\n");
    return -3;
  }
  uint32_t jdec_id;
  jdec_id = SPICommonCommand(E_CC_READ, 0x9f, 3, 0, 0);
  printf("JDEC ID: 0x%02x\n", jdec_id);
  const FlashDesc* chip = FindChip(jdec_id);
  if (NULL == chip) {
    printf("Unknown chip ID\n");
    return -4;
  }
  printf("Manufacturer ");
  PrintManufacturer(jdec_id >> 16);
  printf("\n");
  printf("Chip: %s\n", chip->device_name);
  printf("Size: %dKB\n", chip->size_kb);

  // Setup flash command codes
  // These are the codes for Winbond, others may varry
  WriteReg(0x62, 0x6);  // Flash Write enable op code
  WriteReg(0x63, 0x50); // Flash Write register op code
  WriteReg(0x6a, 0x3);  // Flash Read op code.
  WriteReg(0x6b, 0xb);  // Flash Fast read op code.
  WriteReg(0x6d, 0x2);  // Flash program op code.
  WriteReg(0x6e, 0x5);  // Flash read status op code.

  b = SPICommonCommand(E_CC_READ, 0x5, 1, 0, 0);
  printf("Flash status register: 0x%02x\n", b);

  FILE *dump = fopen("flash.bin", "wb");
  int32_t addr = 0;
  InitCRC();
  do {
    BYTE buffer[1024];
    printf("Reading addr %x\r", addr);
    SPIRead(addr, buffer, sizeof(buffer));
    fwrite(buffer, 1, sizeof(buffer), dump);
    ProcessCRC(buffer, sizeof(buffer));
    addr += sizeof(buffer);
  } while (addr < chip->size_kb * 1024);
  printf("done.\n");
  fclose(dump);
  printf("Our CRC=0x%02x\n", GetCRC());
  printf("Chip CRC = %02x\n", SPIComputeCRC(0, chip->size_kb * 1024 - 1));
  USBDevice.Close();
	return 0;
}
