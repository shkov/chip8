#pragma once

#include <spdlog/spdlog.h>

#include <iostream>

namespace Chip8 {

class Emulator {
 public:
  Emulator() = default;

  void Start() {
    spdlog::info("Chip8Emulator");
    spdlog::info("hello from {}", "ivan");
  };

 private:
};

}  // namespace Chip8
