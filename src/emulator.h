#pragma once

#include <array>
#include <atomic>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

#include "screen.h"
#include "speaker.h"

namespace chip8 {

class Emulator {
 public:
  static inline const uint16_t kChip8MemorySize{ 4096 };
  static inline const uint16_t kChip8ProgramStartAddress{ 0x200 };
  static inline const uint8_t kChip8ScreenWidth{ 64 };
  static inline const uint8_t kChip8ScreenHeight{ 32 };

  static inline const std::vector<uint8_t> kChip8FontSet = {
    0xF0, 0x90, 0x90, 0x90, 0xF0, 0x20, 0x60, 0x20, 0x20, 0x70, 0xF0, 0x10, 0xF0, 0x80, 0xF0, 0xF0,
    0x10, 0xF0, 0x10, 0xF0, 0x90, 0x90, 0xF0, 0x10, 0x10, 0xF0, 0x80, 0xF0, 0x10, 0xF0, 0xF0, 0x80,
    0xF0, 0x90, 0xF0, 0xF0, 0x10, 0x20, 0x40, 0x40, 0xF0, 0x90, 0xF0, 0x90, 0xF0, 0xF0, 0x90, 0xF0,
    0x10, 0xF0, 0xF0, 0x90, 0xF0, 0x90, 0x90, 0xE0, 0x90, 0xE0, 0x90, 0xE0, 0xF0, 0x80, 0x80, 0x80,
    0xF0, 0xE0, 0x90, 0x90, 0x90, 0xE0, 0xF0, 0x80, 0xF0, 0x80, 0xF0, 0xF0, 0x80, 0xF0, 0x80, 0x80
  };

  explicit Emulator(const std::string& filename, Screen& screen, Speaker& speaker,
                    std::chrono::microseconds delay);

  void StartExecutionLoop();

 private:
  struct Instruction {
    uint16_t raw;
    uint8_t first_nibble;
    uint8_t second_nibble;
    uint8_t third_nibble;
    uint8_t fourth_nibble;
  };

  void LoadProgramText(const std::string& filename);
  void LoadFontSet();

  std::optional<std::array<uint8_t, 2>> Fetch();
  static Instruction Decode(std::array<uint8_t, 2> instr);
  void Execute(const Instruction& instr);
  void OnTimersTick();

  void ClearScreen();
  void Jump(uint16_t address);
  void SetRegisterVx(uint8_t x, uint8_t value);
  void AddToRegisterVx(uint8_t x, uint8_t value);
  void SetIndexRegister(uint16_t value);
  void Display(uint8_t x, uint8_t y, uint8_t n);
  void CallSubroutine(uint16_t address);
  void ReturnFromSubroutine();
  void SkipInstructionIfVxEqual(uint8_t x, uint8_t value);
  void SkipInstructionIfVxNotEqual(uint8_t x, uint8_t value);
  void SkipInstructionIfVxEqualVy(uint8_t x, uint8_t y);
  void SkipInstructionIfVxNotEqualVy(uint8_t x, uint8_t y);
  void SkipInstructionIfPressed(uint8_t x);
  void SkipInstructionIfNotPressed(uint8_t x);
  void SetVy2Vx(uint8_t x, uint8_t y);
  void VxBinaryOrVy(uint8_t x, uint8_t y);
  void VxBinaryAndVy(uint8_t x, uint8_t y);
  void VxBinaryXorVy(uint8_t x, uint8_t y);
  void AddVy2Vx(uint8_t x, uint8_t y);
  void VxSubtractVy(uint8_t x, uint8_t y);
  void VySubtractVx(uint8_t x, uint8_t y);
  void ShiftVxRight(uint8_t x);
  void ShiftVxLeft(uint8_t x);
  void JumpWithOffset(uint16_t address);
  void VxBinaryAndRandom(uint8_t x, uint8_t value);
  void AddVx2IndexRegister(uint8_t x);
  void WaitForKeyPress(uint8_t x);
  void SetIndexRegisterForFont(uint8_t x);
  void HexInVxToDecimal(uint8_t x);
  void StoreRegistersInMemory(uint8_t x);
  void LoadRegistersFromMemory(uint8_t x);
  void SetDelayTimer(uint8_t x);
  void SetSoundTimer(uint8_t x);
  void SetDelayTimer2Vx(uint8_t x);

  std::array<uint8_t, 16> variable_registers_{};
  std::array<uint16_t, 16> stack_{};
  uint8_t stack_pointer_{ 0 };
  std::array<uint8_t, kChip8MemorySize> memory_{};
  int program_counter_{ kChip8ProgramStartAddress };
  std::array<std::array<uint8_t, kChip8ScreenWidth>, kChip8ScreenHeight> screen_matrix_;
  uint16_t index_register_{ 0 };
  uint8_t delay_timer_{ 0 };
  uint8_t sound_timer_{ 0 };

  std::chrono::microseconds delay_;
  Screen& screen_;
  Speaker& speaker_;
};

}  // namespace chip8
