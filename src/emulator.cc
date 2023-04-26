#include "emulator.h"

#include <spdlog/spdlog.h>

#include <array>
#include <atomic>
#include <cstdint>
#include <cstdlib>
#include <ctime>
#include <exception>
#include <fstream>
#include <iomanip>
#include <ios>
#include <iostream>
#include <optional>
#include <random>
#include <string>
#include <vector>

#include "keyboard.h"

namespace chip8
{

Emulator::Emulator(const std::string& filename, Window& w) : window_{ w }
{
    std::srand(std::time(nullptr));
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

        if (delay_timer_ > 0)
            --delay_timer_;
        if (sound_timer_ > 0)
            --sound_timer_;
        if (sound_timer_ > 0)
            printf("%c", 7);
    }
};

void Emulator::LoadProgramText(const std::string& filename)
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

    rom_file.read(reinterpret_cast<char*>(memory_.data() + kChip8ProgramStartAddress), file_size);
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

void Emulator::Execute(const Instruction& instr)
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
            default:
                throw std::invalid_argument{ "unknown opcode" };
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
        SkipInstructionIfVxEqual(instr);
        return;

    case 0x4:
        SkipInstructionIfVxNotEqual(instr);
        return;

    case 0x5:
        SkipInstructionIfVxEqualVy(instr);
        return;

    case 0x6:
        SetRegisterVx(instr);
        return;

    case 0x7:
        AddToRegisterVx(instr);
        return;

    case 0x8:
        switch (instr.fourth_nibble)
        {
        case 0x0:
            SetVy2Vx(instr);
            return;
        case 0x1:
            VxBinaryOrVy(instr);
            return;
        case 0x2:
            VxBinaryAndVy(instr);
            return;
        case 0x3:
            VxBinaryXorVy(instr);
            return;
        case 0x4:
            AddVy2Vx(instr);
            return;
        case 0x5:
            VxSubtractVy(instr);
            return;
        case 0x6:
            ShiftVxRight(instr);
            return;
        case 0x7:
            VySubtractVx(instr);
            return;
        case 0xE:
            ShiftVxLeft(instr);
            return;
        default:
            throw std::invalid_argument{ "unknown opcode" };
        }
        return;

    case 0x9:
        SkipInstructionIfVxNotEqualVy(instr);
        return;

    case 0xA:
        SetIndexRegister(instr);
        return;

    case 0xB:
        JumpWithOffset(instr);
        return;

    case 0xC:
        VxBinaryAndRandom(instr);
        return;

    case 0xD:
        Display(instr);
        return;

    case 0xE:
        if (instr.third_nibble == 0x9 && instr.fourth_nibble == 0xE)
            SkipInstructionIfPressed(instr);
        else if (instr.third_nibble == 0xA && instr.fourth_nibble == 0x1)
            SkipInstructionIfNotPressed(instr);
        else
            throw std::invalid_argument{ "unknown opcode" };
        return;

    case 0xF:
        if (instr.third_nibble == 0x1 && instr.fourth_nibble == 0xE)
            AddVx2IndexRegister(instr);
        else if (instr.third_nibble == 0x0 && instr.fourth_nibble == 0xA)
            WaitForKeyPress(instr);
        else if (instr.third_nibble == 0x2 && instr.fourth_nibble == 0x9)
            SetIndexRegisterForFont(instr);
        else if (instr.third_nibble == 0x3 && instr.fourth_nibble == 0x3)
            HexToDecimal(instr);
        else if (instr.third_nibble == 0x5 && instr.fourth_nibble == 0x5)
            StoreRegistersInMemory(instr);
        else if (instr.third_nibble == 0x6 && instr.fourth_nibble == 0x5)
            LoadRegistersFromMemory(instr);
        else if (instr.third_nibble == 0x0 && instr.fourth_nibble == 0x7)
            SetDelayTimer2Vx(instr);
        else if (instr.third_nibble == 0x1 && instr.fourth_nibble == 0x5)
            SetDelayTimer(instr);
        else if (instr.third_nibble == 0x1 && instr.fourth_nibble == 0x8)
            SetSoundTimer(instr);
        else
            throw std::invalid_argument{ "unknown opcode" };
        return;

    default:
        throw std::invalid_argument{ "unknown opcode" };
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

void Emulator::Jump(const Instruction& instr)
{
    program_counter_ = instr.raw & 0x0FFF;
};

void Emulator::SetRegisterVx(const Instruction& instr)
{
    size_t register_idx{ instr.second_nibble };
    variable_registers_[register_idx] = instr.raw & 0x00FF;
};

void Emulator::AddToRegisterVx(const Instruction& instr)
{
    size_t register_idx{ instr.second_nibble };
    variable_registers_[register_idx] += instr.raw & 0x00FF;
};

void Emulator::SetIndexRegister(const Instruction& instr)
{
    index_register_ = instr.raw & 0x0FFF;
};

void Emulator::Display(const Instruction& instr)
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
                uint8_t& location{ screen_[target_y][target_x] };
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

void Emulator::CallSubroutine(const Instruction& instr)
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

void Emulator::SkipInstructionIfVxEqual(const Instruction& instr)
{
    size_t register_idx{ instr.second_nibble };
    if (variable_registers_[register_idx] == (instr.raw & 0x00FF))
        program_counter_ += 2;
}

void Emulator::SkipInstructionIfVxNotEqual(const Instruction& instr)
{
    size_t register_idx{ instr.second_nibble };
    if (variable_registers_[register_idx] != (instr.raw & 0x00FF))
        program_counter_ += 2;
}

void Emulator::SkipInstructionIfVxEqualVy(const Instruction& instr)
{
    size_t register_vx{ instr.second_nibble };
    size_t register_vy{ instr.third_nibble };
    if (variable_registers_[register_vx] == variable_registers_[register_vy])
        program_counter_ += 2;
}

void Emulator::SkipInstructionIfVxNotEqualVy(const Instruction& instr)
{
    size_t register_vx{ instr.second_nibble };
    size_t register_vy{ instr.third_nibble };
    if (variable_registers_[register_vx] != variable_registers_[register_vy])
        program_counter_ += 2;
}

void Emulator::SkipInstructionIfPressed(const Instruction& instr)
{
    size_t register_vx{ instr.second_nibble };
    uint8_t key{ variable_registers_[register_vx] };
    if (Keyboard::IsKeyPressed(key))
        program_counter_ += 2;
}

void Emulator::SkipInstructionIfNotPressed(const Instruction& instr)
{
    size_t register_vx{ instr.second_nibble };
    uint8_t key{ variable_registers_[register_vx] };
    if (!Keyboard::IsKeyPressed(key))
        program_counter_ += 2;
}

void Emulator::SetVy2Vx(const Instruction& instr)
{

    size_t register_vx{ instr.second_nibble };
    size_t register_vy{ instr.third_nibble };
    variable_registers_[register_vx] = variable_registers_[register_vy];
}

void Emulator::VxBinaryOrVy(const Instruction& instr)
{

    size_t register_vx{ instr.second_nibble };
    size_t register_vy{ instr.third_nibble };
    variable_registers_[register_vx] = variable_registers_[register_vx] | variable_registers_[register_vy];
}

void Emulator::VxBinaryAndVy(const Instruction& instr)
{
    size_t register_vx{ instr.second_nibble };
    size_t register_vy{ instr.third_nibble };
    variable_registers_[register_vx] = variable_registers_[register_vx] & variable_registers_[register_vy];
}

void Emulator::VxBinaryXorVy(const Instruction& instr)
{
    size_t register_vx{ instr.second_nibble };
    size_t register_vy{ instr.third_nibble };
    variable_registers_[register_vx] = variable_registers_[register_vx] ^ variable_registers_[register_vy];
}

void Emulator::AddVy2Vx(const Instruction& instr)
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

void Emulator::VxSubtractVy(const Instruction& instr)
{
    size_t register_vx{ instr.second_nibble };
    size_t register_vy{ instr.third_nibble };
    if (register_vx > register_vy)
        variable_registers_[0xF] = 1;
    else
        variable_registers_[0xF] = 0;
    variable_registers_[register_vx] = variable_registers_[register_vx] - variable_registers_[register_vy];
}

void Emulator::VySubtractVx(const Instruction& instr)
{
    size_t register_vx{ instr.second_nibble };
    size_t register_vy{ instr.third_nibble };
    if (register_vy > register_vx)
        variable_registers_[0xF] = 1;
    else
        variable_registers_[0xF] = 0;
    variable_registers_[register_vx] = variable_registers_[register_vy] - variable_registers_[register_vx];
}

void Emulator::ShiftVxRight(const Instruction& instr)
{
    size_t register_vx{ instr.second_nibble };
    int shifted{ variable_registers_[register_vx] & 0x1 };
    if (shifted > 0)
        variable_registers_[0xF] = 1;
    else
        variable_registers_[0xF] = 0;
    variable_registers_[register_vx] >>= 1;
}

void Emulator::ShiftVxLeft(const Instruction& instr)
{
    size_t register_vx{ instr.second_nibble };
    int shifted{ variable_registers_[register_vx] & 0x8 };
    if (shifted > 0)
        variable_registers_[0xF] = 1;
    else
        variable_registers_[0xF] = 0;
    variable_registers_[register_vx] <<= 1;
}

void Emulator::JumpWithOffset(const Instruction& instr)
{

    program_counter_ = instr.raw & 0x0FFF;
    program_counter_ += variable_registers_[0x0];
}

void Emulator::VxBinaryAndRandom(const Instruction& instr)
{

    size_t register_vx{ instr.second_nibble };
    int random = std::rand() % 8;
    variable_registers_[register_vx] = random & (instr.raw & 0x00FF);
}

void Emulator::AddVx2IndexRegister(const Instruction& instr)
{
    size_t register_vx{ instr.second_nibble };
    index_register_ += variable_registers_[register_vx];
}

void Emulator::SetIndexRegisterForFont(const Instruction& instr)
{
    size_t register_vx{ instr.second_nibble };
    index_register_ = variable_registers_[register_vx];
}

void Emulator::HexToDecimal(const Instruction& instr)
{
    size_t register_vx{ instr.second_nibble };
    uint8_t hex_number{ variable_registers_[register_vx] };
    memory_[index_register_ + 2] = hex_number % 10;
    hex_number /= 10;
    memory_[index_register_ + 1] = hex_number % 10;
    hex_number /= 10;
    memory_[index_register_] = hex_number;
}

void Emulator::StoreRegistersInMemory(const Instruction& instr)
{
    size_t x{ instr.second_nibble };
    for (size_t i = 0; i <= x; ++i)
    {
        memory_[index_register_ + i] = variable_registers_[i];
    }
}

void Emulator::LoadRegistersFromMemory(const Instruction& instr)
{

    size_t x{ instr.second_nibble };
    for (size_t i = 0; i <= x; ++i)
    {
        variable_registers_[i] = memory_[index_register_ + i];
    }
}

void Emulator::WaitForKeyPress(const Instruction& instr)
{
    std::optional<uint8_t> key{ Keyboard::WaitForKeyPress(window_) };
    if (!key.has_value())
        return;

    size_t register_vx{ instr.second_nibble };
    variable_registers_[register_vx] = *key;
}

void Emulator::SetDelayTimer(const Instruction& instr)
{
    size_t register_vx{ instr.second_nibble };
    delay_timer_ = variable_registers_[register_vx];
}

void Emulator::SetSoundTimer(const Instruction& instr)
{
    size_t register_vx{ instr.second_nibble };
    sound_timer_ = variable_registers_[register_vx];
}

void Emulator::SetDelayTimer2Vx(const Instruction& instr)
{
    size_t register_vx{ instr.second_nibble };
    variable_registers_[register_vx] = delay_timer_;
}

} // namespace chip8
