#pragma once

#include <spdlog/spdlog.h>

#include <cstdint>
#include <exception>
#include <fstream>
#include <ios>
#include <iostream>
#include <string>

namespace chip8 {

class Emulator {
 public:
  static inline const uint16_t kChip8MemorySize = 4096;
  static inline const uint16_t kChip8ProgramStartAddress = 0x200;

  explicit Emulator(const std::string& filename) {
    std::ifstream rom_file{filename, std::ios::in | std::ios::binary | std::ios::ate};
    if (!rom_file) {
      throw std::invalid_argument{"failed to open rom file"};
    }
    rom_file.unsetf(std::ios::skipws);

    size_t file_size = rom_file.tellg();
    if (file_size > kChip8MemorySize) {
      throw std::invalid_argument{"rom file is too large to fit into chip8 memory"};
    }

    rom_file.seekg(0, std::ios::beg);
    program_text_.reserve(file_size);

    program_text_.insert(program_text_.begin(), std::istream_iterator<uint8_t>(rom_file),
                         std::istream_iterator<uint8_t>());

    if (program_text_.size() != file_size) {
      throw std::invalid_argument{"failed to read from rom file"};
    }
  };

  void StartExecutionLoop() {
    for (uint8_t byte : program_text_) {
      std::cout << byte;
    }
    std::cout << std::endl;
  }

  void Stop(){};

 private:
  std::vector<uint8_t> program_text_;
};

}  // namespace chip8
