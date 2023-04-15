#include <spdlog/spdlog.h>

#include <csignal>
#include <cstdlib>
#include <iostream>

#include "src/emulator.h"

std::function<void(int)> shutdown_handler;
void signal_handler(int signal) { shutdown_handler(signal); }

int main(int argc, char* argv[]) {
  std::vector<std::string> args(argv, argv + argc);
  if (args.size() < 2) {
    spdlog::error("must provide rom file");
    return 1;
  }

  std::string rom_file{ args[1] };
  chip8::Emulator emulator{ rom_file };

  shutdown_handler = [&emulator](int signum) {
    spdlog::info("Interrupt signal ({}) received. Bye", signum);
    emulator.Stop();
  };

  auto _ = std::signal(SIGINT, signal_handler);

  emulator.StartExecutionLoop();

  spdlog::info("bye!");
}

