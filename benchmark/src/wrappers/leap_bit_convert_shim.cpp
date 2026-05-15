#include <cstdint>
#include <cstring>

void c_convert2bit(char *str, int length, uint8_t *bits) {
  std::memset(bits, 0, 128);
  for (int i = 0; i < length; ++i) {
    const int byte = (i * 2) / 8;
    const int shift = (i * 2) % 8;
    uint8_t code = 0;
    if (str[i] == 'C') code = 1;
    else if (str[i] == 'G') code = 2;
    else if (str[i] == 'T') code = 3;
    bits[byte] |= static_cast<uint8_t>(code << shift);
  }
}

void sse_convert2bit(char *str, uint8_t *bits0, uint8_t *bits1) {
  std::memset(bits0, 0, 16);
  std::memset(bits1, 0, 16);
  for (int i = 0; i < 128; ++i) {
    if (str[i] == 'C' || str[i] == 'T') bits0[i / 8] |= static_cast<uint8_t>(1u << (i % 8));
    if (str[i] == 'G' || str[i] == 'T') bits1[i / 8] |= static_cast<uint8_t>(1u << (i % 8));
  }
}

void avx_convert2bit(char *str, uint8_t *bits0, uint8_t *bits1) {
  std::memset(bits0, 0, 32);
  std::memset(bits1, 0, 32);
  for (int i = 0; i < 256; ++i) {
    if (str[i] == 'C' || str[i] == 'T') bits0[i / 8] |= static_cast<uint8_t>(1u << (i % 8));
    if (str[i] == 'G' || str[i] == 'T') bits1[i / 8] |= static_cast<uint8_t>(1u << (i % 8));
  }
}

void avx512_convert2bit(char *str, uint8_t *bits0, uint8_t *bits1) {
  std::memset(bits0, 0, 64);
  std::memset(bits1, 0, 64);
  for (int i = 0; i < 512; ++i) {
    if (str[i] == 'C' || str[i] == 'T') bits0[i / 8] |= static_cast<uint8_t>(1u << (i % 8));
    if (str[i] == 'G' || str[i] == 'T') bits1[i / 8] |= static_cast<uint8_t>(1u << (i % 8));
  }
}

