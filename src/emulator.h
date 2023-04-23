#pragma once

#include <array>
#include <atomic>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

#include "window.h"

namespace chip8
{

class Emulator
{
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

    explicit Emulator(const std::string& filename, Window& window);

    void StartExecutionLoop();

  private:
    struct Instruction
    {
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

    void ClearScreen();
    void Jump(const Instruction& instr);
    void SetRegisterVx(const Instruction& instr);
    void AddToRegisterVx(const Instruction& instr);
    void SetIndexRegister(const Instruction& instr);
    void Display(const Instruction& instr);
    void CallSubroutine(const Instruction& instr);
    void ReturnFromSubroutine();
    void SkipInstructionIfVxEqual(const Instruction& instr);
    void SkipInstructionIfVxNotEqual(const Instruction& instr);
    void SkipInstructionIfVxEqualVy(const Instruction& instr);
    void SkipInstructionIfVxNotEqualVy(const Instruction& instr);
    void SkipInstructionIfPressed(const Instruction& instr);
    void SkipInstructionIfNotPressed(const Instruction& instr);
    void SetVy2Vx(const Instruction& instr);
    void VxBinaryOrVy(const Instruction& instr);
    void VxBinaryAndVy(const Instruction& instr);
    void VxBinaryXorVy(const Instruction& instr);
    void AddVy2Vx(const Instruction& instr);
    void VxSubtractVy(const Instruction& instr);
    void VySubtractVx(const Instruction& instr);
    void ShiftVxRight(const Instruction& instr);
    void ShiftVxLeft(const Instruction& instr);
    void JumpWithOffset(const Instruction& instr);
    void VxBinaryAndRandom(const Instruction& instr);
    void AddVx2IndexRegister(const Instruction& instr);
    void WaitForKeyPress(const Instruction& instr);
    void SetIndexRegisterForFont(const Instruction& instr);
    void HexToDecimal(const Instruction& instr);
    void StoreRegistersInMemory(const Instruction& instr);
    void LoadRegistersFromMemory(const Instruction& instr);
    void SetDelayTimer(const Instruction& instr);
    void SetSoundTimer(const Instruction& instr);
    void SetDelayTimer2Vx(const Instruction& instr);

    std::array<uint8_t, 16> variable_registers_{};
    std::array<uint16_t, 16> stack_{};
    uint8_t stack_pointer_{ 0 };
    std::array<uint8_t, kChip8MemorySize> memory_{};
    int program_counter_{ kChip8ProgramStartAddress };
    std::array<std::array<uint8_t, kChip8ScreenWidth>, kChip8ScreenHeight> screen_;
    uint16_t index_register_{ 0 };

    Window& window_;
};

} // namespace chip8
