#pragma once

#include <stdint.h>

uint32_t ComputeGffDecodedSize(uint8_t* data_ptr, uint32_t data_len);
bool DecodeGff(uint8_t* data_ptr, uint32_t data_len, uint8_t* dest);
