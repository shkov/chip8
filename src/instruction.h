#pragma once

#include <cstdint>

namespace chip8 {

struct Instruction {
  uint8_t high_byte;
  uint8_t low_byte;
};

}  // namespace chip8

