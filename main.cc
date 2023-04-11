#include <spdlog/spdlog.h>

#include <csignal>
#include <cstdlib>
#include <iostream>

#include "src/emulator.h"

std::function<void(int)> shutdown_handler;
void signal_handler(int signal) { shutdown_handler(signal); }

int main() {
  const char* rom_file = std::getenv("ROM_FILE");
  if (rom_file == nullptr) {
    spdlog::error("must provide ROM_FILE via env");
    return 1;
  }

  Chip8::Emulator emulator{rom_file};

  shutdown_handler = [&emulator](int signum) {
    spdlog::info("Interrupt signal ({}) received. Bye", signum);
    emulator.Stop();
  };

  auto _ = std::signal(SIGINT, signal_handler);

  emulator.StartExecutionLoop();

  spdlog::info("bye!");
}
