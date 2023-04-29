#include <spdlog/spdlog.h>

#include <exception>
#include <iostream>
#include <string>

#include "src/emulator.h"
#include "src/keyboard.h"
#include "src/screen.h"
#include "src/speaker.h"

int main(int argc, char* argv[]) {
  std::vector<std::string> args(argv, argv + argc);
  if (args.size() < 3) {
    spdlog::error("must provide rom and beep sound file");
    return 1;
  }

  std::string sound_file{ args[2] };
  chip8::Speaker speaker{ sound_file };

  std::chrono::microseconds delay{ args.size() >= 4 ? std::atoi(args[3].c_str()) : 500 };

  chip8::Screen screen;
  std::string rom_file{ args[1] };
  chip8::Emulator emulator{ rom_file, screen, speaker, delay };

  try {
    emulator.StartExecutionLoop();
  } catch (const std::exception& ex) {
    spdlog::error("unexpected exception: {}", ex.what());
  }

  spdlog::info("bye!");
}

