// main.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <memory.h>
#include "crc.h"
#include "i2c.h"
#include "gff.h"

struct FlashDesc {
  const char* device_name;
  uint32_t    jedec_id;
  uint32_t    size_kb;
  uint32_t    page_size;
  uint32_t    block_size_kb;
};

static const FlashDesc FlashDevices[] = {
    // name,        Jedec ID,    sizeK, page size, block sizeK
    {"AT25DF041A" , 0x1F4401,      512,       256, 64},
    {"AT25DF161"  , 0x1F4602, 2 * 1024,       256, 64},
    {"AT26DF081A" , 0x1F4501, 1 * 1024,       256, 64},
    {"AT26DF0161" , 0x1F4600, 2 * 1024,       256, 64},
    {"AT26DF161A" , 0x1F4601, 2 * 1024,       256, 64},
    {"AT25DF321" ,  0x1F4701, 4 * 1024,       256, 64},
    {"AT25DF512B" , 0x1F6501,       64,       256, 32},
    {"AT25DF512B" , 0x1F6500,       64,       256, 32},
    {"AT25DF021"  , 0x1F3200,      256,       256, 64},
    {"AT26DF641" ,  0x1F4800, 8 * 1024,       256, 64},
    // Manufacturer: ST 
    {"M25P05"     , 0x202010,       64,       256, 32},
    {"M25P10"     , 0x202011,      128,       256, 32},
    {"M25P20"     , 0x202012,      256,       256, 64},
    {"M25P40"     , 0x202013,      512,       256, 64},
    {"M25P80"     , 0x202014, 1 * 1024,       256, 64},
    {"M25P16"     , 0x202015, 2 * 1024,       256, 64},
    {"M25P32"     , 0x202016, 4 * 1024,       256, 64},
    {"M25P64"     , 0x202017, 8 * 1024,       256, 64},
    // Manufacturer: Windbond 
    {"W25X10"     , 0xEF3011,      128,       256, 64},
    {"W25X20"     , 0xEF3012,      256,       256, 64},
    {"W25X40"     , 0xEF3013,      512,       256, 64},
    {"W25X80"     , 0xEF3014, 1 * 1024,       256, 64},
    // Manufacturer: Macronix 
    {"MX25L512"   , 0xC22010,       64,       256, 64},
    {"MX25L3205"  , 0xC22016, 4 * 1024,       256, 64},
    {"MX25L6405"  , 0xC22017, 8 * 1024,       256, 64},
    {"MX25L8005"  , 0xC22014,     1024,       256, 64},
    // Microchip
    {"SST25VF512" , 0xBF4800,       64,       256, 32},
    {"SST25VF032" , 0xBF4A00, 4 * 1024,       256, 32},
    {NULL , 0, 0, 0, 0}
};

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
  uint8_t reg_value = (cmd_type << 5) | 
    (num_writes << 3) |
    (num_reads << 1);

  WriteReg(0x60, reg_value);
  WriteReg(0x61, cmd_code);
  switch (num_writes) {
  case 3:
    WriteReg(0x64, write_value >> 16);
    WriteReg(0x65, write_value >> 8);
    WriteReg(0x66, write_value);
    break;
  case 2:
    WriteReg(0x64, write_value >> 8);
    WriteReg(0x65, write_value);
    break;
  case 1:
    WriteReg(0x64, write_value);
    break;
  }
  WriteReg(0x60, reg_value | 1); // Execute the command
  uint8_t b;
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
  uint8_t b;
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

static const FlashDesc* FindChip(uint32_t jedec_id) {
  const FlashDesc* chip = FlashDevices;
  while (chip->jedec_id != 0) {
    if (chip->jedec_id == jedec_id)
      return chip;
    chip++;
  }
  return NULL;
}

uint8_t SPIComputeCRC(uint32_t start, uint32_t end) {
  WriteReg(0x64, start >> 16);
  WriteReg(0x65, start >> 8);
  WriteReg(0x66, start);

  WriteReg(0x72, end >> 16);
  WriteReg(0x73, end >> 8);
  WriteReg(0x74, end);

  WriteReg(0x6f, 0x84);
  uint8_t b;
  do
  {
    b = ReadReg(0x6f);
  } while (!(b & 0x2));  // TODO: add timeout and reset the controller
  return ReadReg(0x75);
}

uint8_t GetManufacturerId(uint32_t jedec_id) {
  return jedec_id >> 16;
}

void SetupChipCommands(uint32_t jedec_id) {
  uint8_t manufacturer_id = GetManufacturerId(jedec_id);
  switch (manufacturer_id) {
  case 0xEF:
    // These are the codes for Winbond
    WriteReg(0x62, 0x6);  // Flash Write enable op code
    WriteReg(0x63, 0x50); // Flash Write register op code
    WriteReg(0x6a, 0x3);  // Flash Read op code.
    WriteReg(0x6b, 0xb);  // Flash Fast read op code.
    WriteReg(0x6d, 0x2);  // Flash program op code.
    WriteReg(0x6e, 0x5);  // Flash read status op code.
    break;
  default:
    printf("Can not handle manufacturer code %02x\n", manufacturer_id);
    exit(-6);
    break;
  }
}

bool SaveFlash(const char *output_file_name, uint32_t chip_size) {
  FILE *dump = fopen(output_file_name, "wb");
  uint32_t addr = 0;
  InitCRC();
  do {
    uint8_t buffer[1024];
    printf("Reading addr %x\r", addr);
    SPIRead(addr, buffer, sizeof(buffer));
    fwrite(buffer, 1, sizeof(buffer), dump);
    ProcessCRC(buffer, sizeof(buffer));
    addr += sizeof(buffer);
  } while (addr < chip_size);
  printf("done.\n");
  fclose(dump);
  uint8_t data_crc = GetCRC();
  uint8_t chip_crc = SPIComputeCRC(0, chip_size - 1);
  printf("Received data CRC %02x\n", data_crc);
  printf("Chip CRC %02x\n", chip_crc);
  return data_crc == chip_crc;
}

uint64_t GetFileSize(FILE* file) {
  uint64_t current_pos;
  uint64_t result;
  current_pos = _ftelli64(file);
  fseek(file, 0, SEEK_END);
  result = _ftelli64(file);
  _fseeki64(file, current_pos, SEEK_SET);
  return result;
}

static uint8_t* ReadFile(const char *file_name, uint32_t* size) {
  FILE *file = fopen(file_name, "rb");
  uint8_t* result = NULL;
  if (NULL == file) {
    printf("Can't open input file %s\n", file_name);
    return result;
  }
  uint64_t file_size64 = GetFileSize(file);
  if (file_size64 > 8*1024*1024) {
    printf("This file looks to big %lld\n", file_size64);
    fclose(file);
    return result;
  }
  uint32_t file_size = (uint32_t)file_size64;
  result = new uint8_t[file_size];
  if (NULL == result) {
    printf("Not enough RAM.\n");
    fclose(file);
    return result;
  }
  fread(result, 1, file_size, file);
  fclose(file);
  if (memcmp("GMI GFF V1.0", result, 12) == 0) {
    printf("Detected GFF image.\n");
    // Handle GFF file
    if (file_size < 256) {
      printf("This file looks to small %d\n", file_size);
      delete [] result;
      return NULL;
    }
    uint32_t gff_size = ComputeGffDecodedSize(result + 256,
      file_size - 256);
    if (gff_size == 0) {
      printf("GFF Decoding failed for this file\n");
      delete [] result;
      return NULL;
    }
    uint8_t* gff_data = new uint8_t[gff_size];
    if (NULL == gff_data) {
      printf("Not enough RAM.\n");
      delete [] result;
      return NULL;
    }
    DecodeGff(result + 256, file_size - 256, gff_data);
    // Replace the encoded buffer with the decoded data.
    delete [] result;
    result = gff_data;
    file_size = gff_size;
  }
  if (NULL != size) {
    *size = file_size;
  }
  return result;
}

static bool ShouldProgramPage(uint8_t* buffer, uint32_t size) {
  for (uint32_t idx = 0; idx < size; ++idx) {
    if (buffer[idx] != 0xff) return true;
  }
  return false;
}

bool ProgramFlash(const char *input_file_name, uint32_t chip_size) {
  uint32_t prog_size;
  uint8_t* prog = ReadFile(input_file_name, &prog_size);
  if (NULL == prog) {
    return false;
  }
  printf("Erasing...");fflush(stdout);
  SPICommonCommand(E_CC_WRITE_AFTER_EWSR, 1, 0, 1, 0); // Unprotect the Status Register
  SPICommonCommand(E_CC_WRITE_AFTER_WREN, 1, 0, 1, 0); // Unprotect the flash
  SPICommonCommand(E_CC_ERASE, 0xc7, 0, 0, 0);         // Chip Erase
  printf("done\n");

  //RTD266x can program only 256 bytes at a time.
  uint8_t buffer[256];
  uint8_t b;
  uint32_t addr = 0;
  uint8_t* data_ptr = prog;
  uint32_t data_len = prog_size;
  InitCRC();
  do
  {
    // Wait for programming cycle to finish
    do {
      b = ReadReg(0x6f);
    } while (b & 0x40);

    printf("Writing addr %x\r", addr);
    // Fill with 0xff in case we read a partial buffer.
    memset(buffer, 0xff, sizeof(buffer));
    uint32_t len = sizeof(buffer);
    if (len > data_len) {
      len = data_len;
    }
    memcpy(buffer, data_ptr, len);
    data_ptr += len;
    data_len -= len;

    if (ShouldProgramPage(buffer, sizeof(buffer))) {
      // Set program size-1
      WriteReg(0x71, 255);

      // Set the programming address
      WriteReg(0x64, addr >> 16);
      WriteReg(0x65, addr >> 8);
      WriteReg(0x66, addr);

      // Write the content to register 0x70
      // Out USB gizmo supports max 63 bytes at a time.
      WriteBytesToAddr(0x70, buffer, 63);
      WriteBytesToAddr(0x70, buffer + 63, 63);
      WriteBytesToAddr(0x70, buffer + 126, 63);
      WriteBytesToAddr(0x70, buffer + 189, 63);
      WriteBytesToAddr(0x70, buffer + 252, 4);

      WriteReg(0x6f, 0xa0); // Start Programing
    }
    ProcessCRC(buffer, sizeof(buffer));
    addr += 256;
  } while (addr < chip_size && data_len != 0);
  delete [] prog;

  // Wait for programming cycle to finish
  do {
    b = ReadReg(0x6f);
  } while (b & 0x40);

  SPICommonCommand(E_CC_WRITE_AFTER_EWSR, 1, 0, 1, 0x1c); // Unprotect the Status Register
  SPICommonCommand(E_CC_WRITE_AFTER_WREN, 1, 0, 1, 0x1c); // Protect the flash

  uint8_t data_crc = GetCRC();
  uint8_t chip_crc = SPIComputeCRC(0, addr - 1);
  printf("Received data CRC %02x\n", data_crc);
  printf("Chip CRC %02x\n", chip_crc);
  return data_crc == chip_crc;
}

int main(int argc, char* argv[])
{
  uint8_t b;
  if (!InitI2C()) {
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
  uint32_t jedec_id;
  jedec_id = SPICommonCommand(E_CC_READ, 0x9f, 3, 0, 0);
  printf("JEDEC ID: 0x%02x\n", jedec_id);
  const FlashDesc* chip = FindChip(jedec_id);
  if (NULL == chip) {
    printf("Unknown chip ID\n");
    return -4;
  }
  printf("Manufacturer ");
  PrintManufacturer(GetManufacturerId(chip->jedec_id));
  printf("\n");
  printf("Chip: %s\n", chip->device_name);
  printf("Size: %dKB\n", chip->size_kb);

  // Setup flash command codes
  SetupChipCommands(chip->jedec_id);

  b = SPICommonCommand(E_CC_READ, 0x5, 1, 0, 0);
  printf("Flash status register: 0x%02x\n", b);
  
#if 1
  SaveFlash("flash-test.bin", chip->size_kb * 1024);
#else
  ProgramFlash("1024x600.bin", chip->size_kb * 1024);
#endif
  CloseI2C();
	return 0;
}
