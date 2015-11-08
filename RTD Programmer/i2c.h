#pragma once

#include <stdint.h>

bool InitI2C();
void CloseI2C();

void SetI2CAddr(uint8_t value);

bool WriteReg(uint8_t reg, uint8_t value);
uint8_t ReadReg(uint8_t reg);
void ReadBytesFromAddr(uint8_t reg, uint8_t* dest, uint8_t len);
bool WriteBytesToAddr(uint8_t reg, uint8_t* values, uint8_t len);