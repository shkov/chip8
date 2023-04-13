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

    program_text_.resize(file_size);

    rom_file.read(reinterpret_cast<char*>(program_text_.data()), file_size);
    if (!rom_file.good()) {
      throw std::runtime_error{"Error reading file"};
    }
  };

  void StartExecutionLoop() {
    while (running_) {
      auto raw{Fetch()};
      if (!raw.has_value()) {
        return;
      }

      Instruction instr{Decode(*raw)};

      Execute(instr);
    }
    std::cout << std::endl;
  };

  void Stop() { running_ = false; };

 private:
  std::optional<std::array<uint8_t, 2>> Fetch() {
    if (program_text_.size() < program_counter_) {
      return std::nullopt;
    }
    uint8_t first{program_text_[program_counter_++]};
    uint8_t sec{program_text_[program_counter_++]};
    return std::array{first, sec};
  };

  static Instruction Decode(std::array<uint8_t, 2> instr) {
    uint8_t high_byte = instr[0];
    uint8_t low_byte = instr[1];

    return Instruction{
        .raw = static_cast<uint16_t>(static_cast<uint16_t>(high_byte << 8) | low_byte),
        .first_nibble = static_cast<uint8_t>(high_byte >> 4),
        .second_nibble = static_cast<uint8_t>(high_byte & 0x0F),
        .third_nibble = static_cast<uint8_t>(low_byte >> 4),
        .fourth_nibble = static_cast<uint8_t>(low_byte & 0x0F),
    };
  };

  void Execute(const Instruction& instr) {
    switch (instr.first_nibble) {
      case 0x0:
        switch (instr.third_nibble) {
          case 0xE:
            ClearScreen();
            break;
        }
        break;

      case 0x1:
        Jump(instr);
        break;

      case 0x6:
        SetRegisterVX(instr);
        break;

      case 0x7:
        AddToRegisterVX(instr);
        break;

      case 0xA:
        SetIndexRegister(instr);
        break;

      case 0xD:
        Display(instr);
        break;
    }
  }

  static void ClearScreen() { spdlog::info("clear screen"); };

  void Jump(const Instruction& instr) {
    program_counter_ = (instr.raw & 0x0FFF) - kChip8ProgramStartAddress;
    spdlog::info("jump to: {}", program_counter_);
  };

  static void SetRegisterVX(const Instruction& instr) {
    spdlog::info("set register VX: {:#x}", instr.first_nibble);
  };

  static void AddToRegisterVX(const Instruction& instr) {
    spdlog::info("add value to register VX: {:#x}", instr.first_nibble);
  };

  static void SetIndexRegister(const Instruction& instr) {
    spdlog::info("set index register I: {:#x}", instr.first_nibble);
  };

  static void Display(const Instruction& instr) { spdlog::info("display/draw: {:#x}", instr.first_nibble); };

  std::atomic<bool> running_{true};
  std::vector<uint8_t> program_text_;
  int program_counter_{0};
  uint16_t index_register_{0};
};

}  // namespace chip8
