#include <spdlog/spdlog.h>

#include <exception>
#include <iostream>

#include "src/emulator.h"
#include "src/window.h"

int main(int argc, char *argv[])
{
    std::vector<std::string> args(argv, argv + argc);
    if (args.size() < 2)
    {
        spdlog::error("must provide rom file");
        return 1;
    }

    std::string rom_file{ args[1] };
    chip8::Window window;
    chip8::Emulator emulator{ rom_file, window };

    try
    {
        emulator.StartExecutionLoop();
    }
    catch (const std::exception &ex)
    {
        spdlog::error("unexpected exception: {}", ex.what());
    }

    spdlog::info("bye!");
}

