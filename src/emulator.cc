#include "emulator.h"

#include <spdlog/spdlog.h>

#include <array>
#include <atomic>
#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <exception>
#include <fstream>
#include <iomanip>
#include <ios>
#include <iostream>
#include <optional>
#include <random>
#include <string>
#include <thread>
#include <vector>

#include "keyboard.h"
#include "screen.h"
#include "speaker.h"

namespace chip8 {

Emulator::Emulator(const std::string& filename, Screen& screen, Speaker& speaker,
                   std::chrono::microseconds delay)
    : screen_{ screen }, speaker_(speaker), delay_{ delay } {
  std::srand(std::time(nullptr));
  ClearScreen();
  LoadFontSet();
  LoadProgramText(filename);
};

void Emulator::StartExecutionLoop() {
  while (screen_.IsOpen()) {
    const auto raw{ Fetch() };
    if (!raw.has_value()) {
      return;
    }

    Instruction instr{ Decode(*raw) };

    Execute(instr);

    OnTimersTick();
  }
};

void Emulator::LoadProgramText(const std::string& filename) {
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

void Emulator::LoadFontSet() {
  for (size_t i = 0; i < kChip8FontSet.size(); i++) {
    memory_[i] = kChip8FontSet[i];
  }
};

std::optional<std::array<uint8_t, 2>> Emulator::Fetch() {
  uint8_t first{ memory_[program_counter_++] };
  uint8_t sec{ memory_[program_counter_++] };
  return std::array{ first, sec };
};

Emulator::Instruction Emulator::Decode(std::array<uint8_t, 2> instr) {
  uint8_t high_byte = instr[0];
  uint8_t low_byte = instr[1];

  return {
    .raw = static_cast<uint16_t>(static_cast<uint16_t>(high_byte << 8) | low_byte),
    .first_nibble = static_cast<uint8_t>(high_byte >> 4),
    .second_nibble = static_cast<uint8_t>(high_byte & 0x0F),
    .third_nibble = static_cast<uint8_t>(low_byte >> 4),
    .fourth_nibble = static_cast<uint8_t>(low_byte & 0x0F),
  };
};

void Emulator::Execute(const Instruction& instr) {
  switch (instr.first_nibble) {
    case 0x0:
      switch (instr.third_nibble) {
        case 0xE:
          switch (instr.fourth_nibble) {
            case 0x0:
              ClearScreen();
              return;

            case 0xE:
              ReturnFromSubroutine();
              return;

            default:
              throw std::invalid_argument{ "unknown opcode" };
          }
      }
      return;

    case 0x1:
      Jump(instr.raw & 0x0FFF);
      return;

    case 0x2:
      CallSubroutine(instr.raw & 0x0FFF);
      return;

    case 0x3:
      SkipInstructionIfVxEqual(instr.second_nibble, instr.raw & 0x00FF);
      return;

    case 0x4:
      SkipInstructionIfVxNotEqual(instr.second_nibble, instr.raw & 0x00FF);
      return;

    case 0x5:
      SkipInstructionIfVxEqualVy(instr.second_nibble, instr.third_nibble);
      return;

    case 0x6:
      SetRegisterVx(instr.second_nibble, instr.raw & 0x00FF);
      return;

    case 0x7:
      AddToRegisterVx(instr.second_nibble, instr.raw & 0x00FF);
      return;

    case 0x8:
      switch (instr.fourth_nibble) {
        case 0x0:
          SetVy2Vx(instr.second_nibble, instr.third_nibble);
          return;
        case 0x1:
          VxBinaryOrVy(instr.second_nibble, instr.third_nibble);
          return;
        case 0x2:
          VxBinaryAndVy(instr.second_nibble, instr.third_nibble);
          return;
        case 0x3:
          VxBinaryXorVy(instr.second_nibble, instr.third_nibble);
          return;
        case 0x4:
          AddVy2Vx(instr.second_nibble, instr.third_nibble);
          return;
        case 0x5:
          VxSubtractVy(instr.second_nibble, instr.third_nibble);
          return;
        case 0x6:
          ShiftVxRight(instr.second_nibble);
          return;
        case 0x7:
          VySubtractVx(instr.second_nibble, instr.third_nibble);
          return;
        case 0xE:
          ShiftVxLeft(instr.second_nibble);
          return;
        default:
          throw std::invalid_argument{ "unknown opcode" };
      }

      return;

    case 0x9:
      SkipInstructionIfVxNotEqualVy(instr.second_nibble, instr.third_nibble);
      return;

    case 0xA:
      SetIndexRegister(instr.raw & 0x0FFF);
      return;

    case 0xB:
      JumpWithOffset(instr.raw & 0x0FFF);
      return;

    case 0xC:
      VxBinaryAndRandom(instr.second_nibble, instr.raw & 0x00FF);
      return;

    case 0xD:
      Display(instr.second_nibble, instr.third_nibble, instr.fourth_nibble);
      return;

    case 0xE:
      if (instr.third_nibble == 0x9 && instr.fourth_nibble == 0xE)
        SkipInstructionIfPressed(instr.second_nibble);

      else if (instr.third_nibble == 0xA && instr.fourth_nibble == 0x1)
        SkipInstructionIfNotPressed(instr.second_nibble);

      else
        throw std::invalid_argument{ "unknown opcode" };

      return;

    case 0xF:
      if (instr.third_nibble == 0x1 && instr.fourth_nibble == 0xE)
        AddVx2IndexRegister(instr.second_nibble);

      else if (instr.third_nibble == 0x0 && instr.fourth_nibble == 0xA)
        WaitForKeyPress(instr.second_nibble);

      else if (instr.third_nibble == 0x2 && instr.fourth_nibble == 0x9)
        SetIndexRegisterForFont(instr.second_nibble);

      else if (instr.third_nibble == 0x3 && instr.fourth_nibble == 0x3)
        HexInVxToDecimal(instr.second_nibble);

      else if (instr.third_nibble == 0x5 && instr.fourth_nibble == 0x5)
        StoreRegistersInMemory(instr.second_nibble);

      else if (instr.third_nibble == 0x6 && instr.fourth_nibble == 0x5)
        LoadRegistersFromMemory(instr.second_nibble);

      else if (instr.third_nibble == 0x0 && instr.fourth_nibble == 0x7)
        SetDelayTimer2Vx(instr.second_nibble);

      else if (instr.third_nibble == 0x1 && instr.fourth_nibble == 0x5)
        SetDelayTimer(instr.second_nibble);

      else if (instr.third_nibble == 0x1 && instr.fourth_nibble == 0x8)
        SetSoundTimer(instr.second_nibble);

      else
        throw std::invalid_argument{ "unknown opcode" };

      return;

    default:
      throw std::invalid_argument{ "unknown opcode" };
  }
};

void Emulator::OnTimersTick() {
  if (delay_timer_ > 0) --delay_timer_;
  if (sound_timer_ > 0) --sound_timer_;
  if (sound_timer_ > 0) {
    speaker_.Play();
  }
  std::this_thread::sleep_for(std::chrono::microseconds{ delay_ });
};

void Emulator::ClearScreen() {
  for (int y = 0; y < kChip8ScreenHeight; ++y) {
    for (int x = 0; x < kChip8ScreenWidth; ++x) {
      screen_matrix_[y][x] = 0;
    }
  }
  screen_.Draw(screen_matrix_);
};

void Emulator::Jump(uint16_t address) { program_counter_ = address; };

void Emulator::SetRegisterVx(uint8_t x, uint8_t value) { variable_registers_[x] = value; };

void Emulator::AddToRegisterVx(uint8_t x, uint8_t value) { variable_registers_[x] += value; };

void Emulator::SetIndexRegister(uint16_t value) { index_register_ = value; };

void Emulator::Display(uint8_t x, uint8_t y, uint8_t n) {
  uint8_t start_from_y = variable_registers_[y] % kChip8ScreenHeight;
  uint8_t start_from_x = variable_registers_[x] % kChip8ScreenWidth;

  variable_registers_[0xF] = 0;

  for (size_t y = 0; y < n; ++y) {
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
        uint8_t& location{ screen_matrix_[target_y][target_x] };
        if (location != 0) {  // screen bit is set
          variable_registers_[0xF] = 1;
        }
        location ^= 1;
      }
    }
  }

  screen_.Draw(screen_matrix_);
};

void Emulator::CallSubroutine(uint16_t address) {
  ++stack_pointer_;
  stack_[stack_pointer_] = program_counter_;
  program_counter_ = address;
}

void Emulator::ReturnFromSubroutine() {
  uint16_t addr{ stack_[stack_pointer_--] };
  program_counter_ = addr;
}

void Emulator::SkipInstructionIfVxEqual(uint8_t x, uint8_t value) {
  if (variable_registers_[x] == (value)) program_counter_ += 2;
}

void Emulator::SkipInstructionIfVxNotEqual(uint8_t x, uint8_t value) {
  if (variable_registers_[x] != (value)) program_counter_ += 2;
}

void Emulator::SkipInstructionIfVxEqualVy(uint8_t x, uint8_t y) {
  if (variable_registers_[x] == variable_registers_[y]) program_counter_ += 2;
}

void Emulator::SkipInstructionIfVxNotEqualVy(uint8_t x, uint8_t y) {
  if (variable_registers_[x] != variable_registers_[y]) program_counter_ += 2;
}

void Emulator::SkipInstructionIfPressed(uint8_t x) {
  uint8_t key{ variable_registers_[x] };
  if (Keyboard::IsKeyPressed(key)) program_counter_ += 2;
}

void Emulator::SkipInstructionIfNotPressed(uint8_t x) {
  uint8_t key{ variable_registers_[x] };
  if (!Keyboard::IsKeyPressed(key)) program_counter_ += 2;
}

void Emulator::SetVy2Vx(uint8_t x, uint8_t y) { variable_registers_[x] = variable_registers_[y]; }

void Emulator::VxBinaryOrVy(uint8_t x, uint8_t y) {
  variable_registers_[x] = variable_registers_[x] | variable_registers_[y];
}

void Emulator::VxBinaryAndVy(uint8_t x, uint8_t y) {
  variable_registers_[x] = variable_registers_[x] & variable_registers_[y];
}

void Emulator::VxBinaryXorVy(uint8_t x, uint8_t y) {
  variable_registers_[x] = variable_registers_[x] ^ variable_registers_[y];
}

void Emulator::AddVy2Vx(uint8_t x, uint8_t y) {
  int result{ variable_registers_[x] + variable_registers_[y] };
  if (result > 255)
    variable_registers_[0xF] = 1;
  else
    variable_registers_[0xF] = 0;

  variable_registers_[x] = result;
}

void Emulator::VxSubtractVy(uint8_t x, uint8_t y) {
  if (variable_registers_[x] > variable_registers_[y])
    variable_registers_[0xF] = 1;
  else
    variable_registers_[0xF] = 0;
  variable_registers_[x] = variable_registers_[x] - variable_registers_[y];
}

void Emulator::VySubtractVx(uint8_t x, uint8_t y) {
  if (variable_registers_[y] > variable_registers_[x])
    variable_registers_[0xF] = 1;
  else
    variable_registers_[0xF] = 0;
  variable_registers_[x] = variable_registers_[y] - variable_registers_[x];
}

void Emulator::ShiftVxRight(uint8_t x) {
  int shifted{ variable_registers_[x] & 0x1 };
  if (shifted > 0)
    variable_registers_[0xF] = 1;
  else
    variable_registers_[0xF] = 0;
  variable_registers_[x] >>= 1;
}

void Emulator::ShiftVxLeft(uint8_t x) {
  int shifted{ variable_registers_[x] & 0x8 };
  if (shifted > 0)
    variable_registers_[0xF] = 1;
  else
    variable_registers_[0xF] = 0;
  variable_registers_[x] <<= 1;
}

void Emulator::JumpWithOffset(uint16_t address) {
  program_counter_ = address;
  program_counter_ += variable_registers_[0x0];
}

void Emulator::VxBinaryAndRandom(uint8_t x, uint8_t value) {
  int random = std::rand() % 8;
  variable_registers_[x] = random & value;
}

void Emulator::AddVx2IndexRegister(uint8_t x) { index_register_ += variable_registers_[x]; }

void Emulator::SetIndexRegisterForFont(uint8_t x) { index_register_ = variable_registers_[x]; }

void Emulator::HexInVxToDecimal(uint8_t x) {
  uint8_t hex_number{ variable_registers_[x] };
  memory_[index_register_] = hex_number / 100;
  memory_[index_register_ + 1] = (hex_number / 10) % 10;
  memory_[index_register_ + 2] = (hex_number % 100) % 10;
}

void Emulator::StoreRegistersInMemory(uint8_t x) {
  for (size_t i = 0; i <= x; ++i) {
    memory_[index_register_ + i] = variable_registers_[i];
  }
}

void Emulator::LoadRegistersFromMemory(uint8_t x) {
  for (size_t i = 0; i <= x; ++i) {
    variable_registers_[i] = memory_[index_register_ + i];
  }
}

void Emulator::WaitForKeyPress(uint8_t x) {
  std::optional<uint8_t> key{ Keyboard::WaitForKeyPress(screen_) };
  if (!key.has_value()) return;

  variable_registers_[x] = *key;
}

void Emulator::SetDelayTimer(uint8_t x) { delay_timer_ = variable_registers_[x]; }

void Emulator::SetSoundTimer(uint8_t x) { sound_timer_ = variable_registers_[x]; }

void Emulator::SetDelayTimer2Vx(uint8_t x) { variable_registers_[x] = delay_timer_; }

}  // namespace chip8
