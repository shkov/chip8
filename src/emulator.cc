#include "emulator.h"

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

namespace chip8
{

Emulator::Emulator(const std::string &filename, Window &w) : window_{ w }
{
    ClearScreen();
    LoadFontSet();
    LoadProgramText(filename);
};

void Emulator::StartExecutionLoop()
{
    while (window_.IsOpen())
    {
        auto raw{ Fetch() };
        if (!raw.has_value())
        {
            return;
        }

        Instruction instr{ Decode(*raw) };

        Execute(instr);
    }
};

void Emulator::LoadProgramText(const std::string &filename)
{
    std::ifstream rom_file{ filename, std::ios::in | std::ios::binary | std::ios::ate };
    if (!rom_file)
    {
        throw std::invalid_argument{ "failed to open rom file" };
    }
    rom_file.unsetf(std::ios::skipws);

    int64_t file_size{ rom_file.tellg() };
    if (file_size > kChip8MemorySize)
    {
        throw std::invalid_argument{ "rom file is too large to fit into chip8 memory" };
    }

    rom_file.seekg(0, std::ios::beg);

    rom_file.read(reinterpret_cast<char *>(memory_.data() + kChip8ProgramStartAddress), file_size);
    if (!rom_file.good())
    {
        throw std::runtime_error{ "Error reading file" };
    }
};

void Emulator::LoadFontSet()
{
    for (size_t i = 0; i < kChip8FontSet.size(); i++)
    {
        memory_[i] = kChip8FontSet[i];
    }
};

std::optional<std::array<uint8_t, 2>> Emulator::Fetch()
{
    uint8_t first{ memory_[program_counter_++] };
    uint8_t sec{ memory_[program_counter_++] };
    return std::array{ first, sec };
};

Emulator::Instruction Emulator::Decode(std::array<uint8_t, 2> instr)
{
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

void Emulator::Execute(const Instruction &instr)
{
    switch (instr.first_nibble)
    {
    case 0x0:
        switch (instr.third_nibble)
        {
        case 0xE:
            switch (instr.fourth_nibble)
            {
            case 0x0:
                ClearScreen();
                return;

            case 0xE:
                ReturnFromSubroutine();
                return;
            }
        }
        return;

    case 0x1:
        Jump(instr);
        return;

    case 0x2:
        CallSubroutine(instr);
        return;

    case 0x3:
        SkipInstructionIfVXEqual(instr);
        return;

    case 0x4:
        SkipInstructionIfVXNotEqual(instr);
        return;

    case 0x5:
        SkipInstructionIfVXEqualVY(instr);
        return;

    case 0x6:
        SetRegisterVX(instr);
        return;

    case 0x7:
        AddToRegisterVX(instr);
        return;

    case 0x9:
        SkipInstructionIfVXNotEqualVY(instr);
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
};

void Emulator::ClearScreen()
{
    for (int y = 0; y < kChip8ScreenHeight; ++y)
    {
        for (int x = 0; x < kChip8ScreenWidth; ++x)
        {
            screen_[y][x] = 0;
        }
    }
    window_.Draw(screen_);
};

void Emulator::Jump(const Instruction &instr)
{
    program_counter_ = instr.raw & 0x0FFF;
};

void Emulator::SetRegisterVX(const Instruction &instr)
{
    size_t register_idx{ instr.second_nibble };
    variable_registers_[register_idx] = instr.raw & 0x00FF;
};

void Emulator::AddToRegisterVX(const Instruction &instr)
{
    size_t register_idx{ instr.second_nibble };
    variable_registers_[register_idx] += instr.raw & 0x00FF;
};

void Emulator::SetIndexRegister(const Instruction &instr)
{
    index_register_ = instr.raw & 0x0FFF;
};

void Emulator::Display(const Instruction &instr)
{
    uint8_t start_from_y = variable_registers_[instr.third_nibble] % kChip8ScreenHeight;
    uint8_t start_from_x = variable_registers_[instr.second_nibble] % kChip8ScreenWidth;

    variable_registers_[0xF] = 0;

    for (size_t y = 0; y < instr.fourth_nibble; ++y)
    {
        size_t target_y{ start_from_y + y };
        if (target_y >= kChip8ScreenHeight)
        {
            break;
        }

        uint8_t pixel = memory_[index_register_ + y];

        for (size_t x = 0; x < 8; ++x)
        {
            size_t target_x{ start_from_x + x };
            if (target_x >= kChip8ScreenWidth)
            {
                break;
            }

            if ((pixel & (0x80 >> x)) != 0)
            { // sprite bit is set
                uint8_t &location{ screen_[target_y][target_x] };
                if (location != 0)
                { // screen bit is set
                    variable_registers_[0xF] = 1;
                }
                location ^= 1;
            }
        }

        window_.Draw(screen_);
    }
};

void Emulator::CallSubroutine(const Instruction &instr)
{
    ++stack_pointer_;
    stack_[stack_pointer_] = program_counter_;
    Jump(instr);
}

void Emulator::ReturnFromSubroutine()
{
    uint16_t addr{ stack_[stack_pointer_--] };
    program_counter_ = addr;
}

void Emulator::SkipInstructionIfVXEqual(const Instruction &instr)
{
    size_t register_idx{ instr.second_nibble };
    if (variable_registers_[register_idx] == (instr.raw & 0x00FF))
        program_counter_ += 2;
}

void Emulator::SkipInstructionIfVXNotEqual(const Instruction &instr)
{
    size_t register_idx{ instr.second_nibble };
    if (variable_registers_[register_idx] != (instr.raw & 0x00FF))
        program_counter_ += 2;
}

void Emulator::SkipInstructionIfVXEqualVY(const Instruction &instr)
{
    size_t register_vx{ instr.second_nibble };
    size_t register_vy{ instr.third_nibble };
    if (variable_registers_[register_vx] == variable_registers_[register_vy])
        program_counter_ += 2;
}

void Emulator::SkipInstructionIfVXNotEqualVY(const Instruction &instr)
{
    size_t register_vx{ instr.second_nibble };
    size_t register_vy{ instr.third_nibble };
    if (variable_registers_[register_vx] != variable_registers_[register_vy])
        program_counter_ += 2;
}

void Emulator::VXBinaryOrVY(const Instruction &instr)
{

    size_t register_vx{ instr.second_nibble };
    size_t register_vy{ instr.third_nibble };
    variable_registers_[register_vx] = variable_registers_[register_vx] | variable_registers_[register_vy];
}

void Emulator::VXBinaryAndVY(const Instruction &instr)
{
    size_t register_vx{ instr.second_nibble };
    size_t register_vy{ instr.third_nibble };
    variable_registers_[register_vx] = variable_registers_[register_vx] & variable_registers_[register_vy];
}

void Emulator::VXBinaryXorVY(const Instruction &instr)
{
    size_t register_vx{ instr.second_nibble };
    size_t register_vy{ instr.third_nibble };
    variable_registers_[register_vx] = variable_registers_[register_vx] ^ variable_registers_[register_vy];
}

void Emulator::AddVY2VX(const Instruction &instr)
{
    size_t register_vx{ instr.second_nibble };
    size_t register_vy{ instr.third_nibble };
    int result{ variable_registers_[register_vx] + variable_registers_[register_vy] };
    if (result > 255)
        variable_registers_[0xF] = 1;
    else
        variable_registers_[0xF] = 0;

    variable_registers_[register_vx] = result;
}

void Emulator::VXSubtractVY(const Instruction &instr)
{
}

void Emulator::VYSubtractVX(const Instruction &instr)
{
}

void Emulator::ShiftVXRight(const Instruction &instr)
{
}

void Emulator::ShiftVXLeft(const Instruction &instr)
{
}

void Emulator::JumpWithOffset(const Instruction &instr)
{
}

void Emulator::VXBinaryAndRandom(const Instruction &instr)
{
}

void Emulator::SkipIfPressed(const Instruction &instr)
{
}

void Emulator::SkipIfNotPressed(const Instruction &instr)
{
}

void Emulator::AddVX2IndexRegister(const Instruction &instr)
{
}

void Emulator::WaitForKeyPress(const Instruction &instr)
{
}

void Emulator::SetIndexRegisterForFont(const Instruction &instr)
{
}

void Emulator::HexToDecimal(const Instruction &instr)
{
}

void Emulator::StoreRegistersInMemory(const Instruction &instr)
{
}

void Emulator::ReadRegistersFromMemory(const Instruction &instr)
{
}

} // namespace chip8

