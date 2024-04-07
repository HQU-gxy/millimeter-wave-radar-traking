#include "hlk.h"
#include <stdint.h>

int16_t hlk_msb_signed_short(uint16_t value) {
  int8_t positive = (value & 0x8000) >= 1;
  uint16_t r = value & 0x7FFF;
  return positive ? r : -r;
}

/**
 * @brief check if all the bytes in the data are empty (0x00)
 * @param [in] data
 * @param [in] len
 * @return int
 */
int is_all_empty(const uint8_t *data, size_t len) {
  for (int i = 0; i < len; i++) {
    if (data[i] != 0) {
      return 0;
    }
  }
  return 1;
}

/**
 * @brief check the endian of the system
 * @return int 1: little endian, 0: big endian
 */
int is_little_endian_() {
  uint16_t x = 0x0001;
  return *((uint8_t *)&x) == 0x01;
}

/**
 * @brief check the endian of the system and cache the result
 * @return int 1: little endian, 0: big endian
 */
int8_t is_little_endian() {
  static int8_t little_endian = -1;
  if (little_endian == -1) {
    little_endian = is_little_endian_();
  }
  return little_endian;
}

/**
 * @brief like `htons` but for little endian
 * @param x
 * @return uint16_t
 */
uint16_t ltohs(uint16_t x) {
  return is_little_endian() ? x : ((x >> 8) | (x << 8));
}

/**
 * @brief like `ntohs` but for little endian
 * @param x
 * @return uint16_t
 */
uint16_t htols(uint16_t x) {
  return is_little_endian() ? x : ((x >> 8) | (x << 8));
}

int hlk_unmarshal_target(const uint8_t *data, size_t len,
                         hlk_target_t *target) {
  if (len < 8) {
    return -1;
  }
  if (is_all_empty(data, len)) {
    target->en = 0;
    target->x = 0;
    target->y = 0;
    target->speed = 0;
    target->resolution = 0;
    return 0;
  }
  int offset = 0;
  uint16_t x_ = *((uint16_t *)(data + offset));
  uint16_t x__ = ltohs(x_);
  int16_t x = hlk_msb_signed_short(x__);
  offset += 2;
  uint16_t y_ = *((uint16_t *)(data + offset));
  uint16_t y__ = ltohs(y_);
  int16_t y = hlk_msb_signed_short(y__);
  offset += 2;
  uint16_t speed_ = *((uint16_t *)(data + offset));
  uint16_t speed__ = ltohs(speed_);
  int16_t speed = hlk_msb_signed_short(speed__);
  offset += 2;
  uint16_t resolution_ = *((uint16_t *)(data + offset));
  uint16_t resolution = ltohs(resolution_);
  offset += 2;

  target->en = 1;
  target->x = x;
  target->y = y;
  target->speed = speed;
  target->resolution = resolution;
  return 0;
}

const uint8_t start_magic[] = {0xaa, 0xff, 0x03, 0x00};
const uint8_t end_magic[] = {0x55, 0xcc};

int hlk_unmarshal_result(const uint8_t *data, size_t size,
                         hlk_result_t *result) {
  size_t offset = 0;
  int ok = memcmp(data, start_magic, sizeof(start_magic));
  if (ok != 0) {
    return -1;
  }
  offset += sizeof(start_magic);
  const int CNT = sizeof(result->targets) / sizeof(result->targets[0]);
  for (int i = 0; i < CNT; i++) {
    hlk_target_t *target = &result->targets[i];
    int ret = hlk_unmarshal_target(data + offset, 8, target);
    if (ret != 0) {
      return -1;
    }
    offset += 8;
  }
  return 0;
}