#pragma once

#include <spdlog/spdlog.h>

#include <array>
#include <atomic>
#include <cstdint>
#include <exception>
#include <fstream>
#include <iomanip>
#include <ios>
#include <iostream>
#include <optional>
#include <string>
#include <vector>

#include "instruction.h"

namespace chip8 {

class Emulator {
 public:
  static inline const uint16_t kChip8MemorySize = 4096;
  static inline const uint16_t kChip8ProgramStartAddress = 0x200;
  static inline const uint8_t kChip8ScreenWidth = 64;
  static inline const uint8_t kChip8ScreenHeight = 32;

  static inline const std::vector<uint8_t> kChip8FontSet = {
    0xF0, 0x90, 0x90, 0x90, 0xF0, 0x20, 0x60, 0x20, 0x20, 0x70, 0xF0, 0x10, 0xF0, 0x80, 0xF0, 0xF0,
    0x10, 0xF0, 0x10, 0xF0, 0x90, 0x90, 0xF0, 0x10, 0x10, 0xF0, 0x80, 0xF0, 0x10, 0xF0, 0xF0, 0x80,
    0xF0, 0x90, 0xF0, 0xF0, 0x10, 0x20, 0x40, 0x40, 0xF0, 0x90, 0xF0, 0x90, 0xF0, 0xF0, 0x90, 0xF0,
    0x10, 0xF0, 0xF0, 0x90, 0xF0, 0x90, 0x90, 0xE0, 0x90, 0xE0, 0x90, 0xE0, 0xF0, 0x80, 0x80, 0x80,
    0xF0, 0xE0, 0x90, 0x90, 0x90, 0xE0, 0xF0, 0x80, 0xF0, 0x80, 0xF0, 0xF0, 0x80, 0xF0, 0x80, 0x80
  };

  explicit Emulator(const std::string& filename) {
    ClearScreen();
    InitMemory();
    LoadFontSet();
    LoadProgramText(filename);
  };

  void StartExecutionLoop() {
    while (running_) {
      auto raw{ Fetch() };
      if (!raw.has_value()) {
        return;
      }

      Instruction instr{ Decode(*raw) };

      Execute(instr);
    }
  };

  void Stop() { running_ = false; };

 private:
  void InitMemory() {
    memory_.resize(kChip8MemorySize);
    std::fill(memory_.begin(), memory_.end(), 0);
  };

  void LoadProgramText(const std::string& filename) {
    std::ifstream rom_file{ filename, std::ios::in | std::ios::binary | std::ios::ate };
    if (!rom_file) {
      throw std::invalid_argument{ "failed to open rom file" };
    }
    rom_file.unsetf(std::ios::skipws);

    int64_t file_size{ rom_file.tellg() };
    if (file_size > kChip8MemorySize) {
      throw std::invalid_argument{ "rom file is too large to fit into chip8 memory" };
    }

    rom_file.seekg(0, std::ios::beg);

    rom_file.read(reinterpret_cast<char*>(memory_.data() + kChip8ProgramStartAddress), file_size);
    if (!rom_file.good()) {
      throw std::runtime_error{ "Error reading file" };
    }
  };

  void LoadFontSet() {
    for (int i = 0; i < kChip8FontSet.size(); i++) {
      memory_[i] = kChip8FontSet[i];
    }
  };

  std::optional<std::array<uint8_t, 2>> Fetch() {
    uint8_t first{ memory_[program_counter_++] };
    uint8_t sec{ memory_[program_counter_++] };
    return std::array{ first, sec };
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
            return;
        }
        return;

      case 0x1:
        Jump(instr);
        return;

      case 0x6:
        SetRegisterVX(instr);
        return;

      case 0x7:
        AddToRegisterVX(instr);
        return;

      case 0xA:
        SetIndexRegister(instr);
        return;

      case 0xD:
        Display(instr);
        return;

      default:
        throw std::invalid_argument{ "not known instruction" };
    }
  }

  void PrintScreen() {
    for (int y = 0; y < kChip8ScreenHeight; ++y) {
      for (int x = 0; x < kChip8ScreenWidth; ++x) {
        std::cout << (screen_[y][x] ? "o" : " ");
      }
      std::cout << std::endl;
    }
  };

  void ClearScreen() {
    for (int y = 0; y < kChip8ScreenHeight; ++y) {
      for (int x = 0; x < kChip8ScreenWidth; ++x) {
        screen_[y][x] = 0;
      }
    }
  };

  void Jump(const Instruction& instr) { program_counter_ = instr.raw & 0x0FFF; };

  void SetRegisterVX(const Instruction& instr) {
    size_t register_idx{ instr.second_nibble };
    variable_registers_[register_idx] = instr.raw & 0x00FF;
  };

  void AddToRegisterVX(const Instruction& instr) {
    size_t register_idx{ instr.second_nibble };
    variable_registers_[register_idx] += instr.raw & 0x00FF;
  };

  void SetIndexRegister(const Instruction& instr) { index_register_ = instr.raw & 0x0FFF; };

  void Display(const Instruction& instr) {
    uint8_t start_from_y = variable_registers_[instr.third_nibble] % kChip8ScreenHeight;
    uint8_t start_from_x = variable_registers_[instr.second_nibble] % kChip8ScreenWidth;

    variable_registers_[0xF] = 0;

    for (size_t y = 0; y < instr.fourth_nibble; ++y) {
      size_t target_y{ start_from_y + y };
      if (target_y >= kChip8ScreenHeight) {
        break;
      }

      uint8_t pixel = memory_[index_register_ + y];

      for (size_t x = 0; x < 8; ++x) {
        size_t target_x{ start_from_x + x };
        if (target_x >= kChip8ScreenWidth) {
          break;
        }

        if ((pixel & (0x80 >> x)) != 0) {  // sprite bit is set
          uint8_t& location{ screen_[target_y][target_x] };
          if (location != 0) {  // screen bit is set
            variable_registers_[0xF] = 1;
          }
          location ^= 1;
        }
      }
    }
  };

  std::atomic<bool> running_{ true };
  int program_counter_{ kChip8ProgramStartAddress };
  std::array<uint8_t, 16> variable_registers_{};
  uint16_t index_register_{ 0 };

  std::vector<uint8_t> memory_;
  std::array<std::array<uint8_t, kChip8ScreenWidth>, kChip8ScreenHeight> screen_;
};

}  // namespace chip8
