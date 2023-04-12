#pragma once

#include <spdlog/spdlog.h>

#include <cstdint>
#include <exception>
#include <fstream>
#include <iomanip>
#include <ios>
#include <iostream>
#include <optional>
#include <string>

#include "instruction.h"

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

    int64_t file_size{rom_file.tellg()};
    if (file_size > kChip8MemorySize) {
      throw std::invalid_argument{"rom file is too large to fit into chip8 memory"};
    }

    rom_file.seekg(0, std::ios::beg);

    program_text_.resize(file_size / sizeof(uint16_t));

    rom_file.read(reinterpret_cast<char*>(program_text_.data()), file_size);
    if (!rom_file.good()) {
      throw std::runtime_error{"Error reading file"};
    }
  };

  void StartExecutionLoop() {
    while (running_) {
      std::optional<uint16_t> raw = Fetch();
      if (!raw.has_value()) {
        return;
      }

      Instruction instr = Decode(*raw);

      Execute(instr);
    }
    std::cout << std::endl;
  };

  void Stop() { running_ = false; };

 private:
  std::optional<uint16_t> Fetch() {
    if (program_text_.size() < program_counter_) {
      return std::nullopt;
    }
    return program_text_[program_counter_++];
  };

  // The CHIP-8 architecture is big endian.
  static Instruction Decode(uint16_t instruction) {
    auto high_byte = static_cast<uint8_t>(instruction & 0xFF);
    auto low_byte = static_cast<uint8_t>(instruction >> 8);

    return Instruction{
        .high_byte = high_byte,
        .low_byte = low_byte,
    };
  };

  static void Execute(const Instruction& instr) {
    std::cout << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(instr.high_byte) << " ";
    std::cout << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(instr.low_byte) << " ";
  }

  std::atomic<bool> running_{true};
  std::vector<uint16_t> program_text_;
  int program_counter_{0};
};

}  // namespace chip8
