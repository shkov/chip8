#include <spdlog/spdlog.h>

#include <iostream>

#include "src/emulator.h"

int main() {
  Chip8::Emulator emulator;
  emulator.Start();
}
