#pragma once

#include <spdlog/spdlog.h>

#include <fstream>
#include <iostream>
#include <string>

namespace Chip8 {

class Emulator {
 public:
  explicit Emulator(const std::string& filename) : rom_file{filename} {};

  void StartExecutionLoop(){};

  void Stop(){};

 private:
  std::ifstream rom_file;
};

}  // namespace Chip8
