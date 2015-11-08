#pragma once

#include <stdint.h>

void InitCRC();
void ProcessCRC(const uint8_t *data, int len);
uint8_t GetCRC();