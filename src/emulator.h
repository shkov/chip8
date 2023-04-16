#pragma once

#include <array>
#include <atomic>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

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

  explicit Emulator(const std::string& filename);

  void StartExecutionLoop();
  void Stop();

 private:
  struct Instruction {
    uint16_t raw;
    uint8_t first_nibble;
    uint8_t second_nibble;
    uint8_t third_nibble;
    uint8_t fourth_nibble;
  };

  void InitMemory();
  void LoadProgramText(const std::string& filename);
  void LoadFontSet();
  void PrintScreen();

  std::optional<std::array<uint8_t, 2>> Fetch();
  static Instruction Decode(std::array<uint8_t, 2> instr);
  void Execute(const Instruction& instr);

  void ClearScreen();
  void Jump(const Instruction& instr);
  void SetRegisterVX(const Instruction& instr);
  void AddToRegisterVX(const Instruction& instr);
  void SetIndexRegister(const Instruction& instr);
  void Display(const Instruction& instr);

  std::atomic<bool> running_{ true };
  int program_counter_{ kChip8ProgramStartAddress };
  std::array<uint8_t, 16> variable_registers_{};
  uint16_t index_register_{ 0 };

  std::vector<uint8_t> memory_;
  std::array<std::array<uint8_t, kChip8ScreenWidth>, kChip8ScreenHeight> screen_;
};

}  // namespace chip8
