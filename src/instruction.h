#pragma once

#include <cstdint>

namespace chip8 {

struct Instruction {
  uint16_t raw;
  uint8_t first_nibble;
  uint8_t second_nibble;
  uint8_t third_nibble;
  uint8_t fourth_nibble;
};

}  // namespace chip8

