#include "stdafx.h"
#include "gff.h"

class CBitStream {
public:
  CBitStream(uint8_t* data_ptr, uint32_t data_len)
    : mask_(0x80),
      data_ptr_(data_ptr),
      data_len_(data_len) {}

  bool HasData() const { return mask_ != 0 || data_len_ != 0; }
  uint32_t DataSize() const { return data_len_; }
  bool ReadBit() {
    if (!mask_) {
      NextMask();
    }
    bool result = (*data_ptr_ & mask_) != 0;
    mask_ >>= 1;
    return result;
  }

private:
  void NextMask() {
    if (data_len_) {
      mask_ = 0x80;
      data_ptr_++;
      data_len_--;
    }
  }

  uint8_t  mask_;
  uint8_t* data_ptr_;
  uint32_t data_len_;
};

static uint8_t gff_decode_nibble(CBitStream* bit_stream) {
  uint8_t zero_cnt = 0;
  bool bit;

  do {
    if (!bit_stream->HasData()) return 0xf0;

    bit = bit_stream->ReadBit();
    if (bit) break;
    zero_cnt++;
  } while (zero_cnt < 6);

  if (zero_cnt > 5) {
    if (bit_stream->DataSize() == 1) {
      return 0xf0;
    }
    return 0xff;
  }

  switch (zero_cnt) {
  case 0:
    return 0;

  case 1:
    bit = bit_stream->ReadBit();
    return (bit) ? 0xf : 1;

  case 2:
    bit = bit_stream->ReadBit();
    return (bit) ? 8 : 2;

  case 3:
    bit = bit_stream->ReadBit();
    return (bit) ? 7 : 0xc;

  case 4:
    bit = bit_stream->ReadBit();
    if (bit) {
      bit = bit_stream->ReadBit();
      return (bit) ? 9 : 4;
    } else {
      bit = bit_stream->ReadBit();
      if (bit) {
        bit = bit_stream->ReadBit();
        return (bit) ? 5 : 0xa;
      } else {
        bit = bit_stream->ReadBit();
        return (bit) ? 0xb : 3;
      }
    }
    break;

  case 5:
    bit = bit_stream->ReadBit();
    if (bit) {
      bit = bit_stream->ReadBit();
      return (bit) ? 0xd : 0xe;
    } else {
      bit = bit_stream->ReadBit();
      return (bit) ? 6 : 0xff;
    }
    break;
  }
  return 0xff;
}

uint32_t ComputeGffDecodedSize(uint8_t* data_ptr, uint32_t data_len) {
  CBitStream bs(data_ptr, data_len);
  uint32_t cnt = 0;
  while (bs.HasData()) {
    uint8_t b = gff_decode_nibble(&bs);
    if (b == 0xff) return 0;
    if (b == 0xf0) return cnt;  // End of file

    b = gff_decode_nibble(&bs);
    if (b > 0xf) return 0;  // Error or odd number of nibbles
    cnt++;
  }
  return cnt;
}

bool DecodeGff(uint8_t* data_ptr, uint32_t data_len, uint8_t* dest) {
  CBitStream bs(data_ptr, data_len);
  while (bs.HasData()) {
    uint8_t n1 = gff_decode_nibble(&bs);
    if (n1 == 0xf0) return true;  // End of file
    if (n1 == 0xff) return false;

    uint8_t n2 = gff_decode_nibble(&bs);
    if (n2 > 0xf) return false;

    uint8_t byte = (n1 << 4) | n2;
    *dest++ = byte;
  }
  return true;
}