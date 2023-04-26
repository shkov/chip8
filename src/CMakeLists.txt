add_library(src emulator.cc window.h keyboard.h)
target_link_libraries(src spdlog sfml-graphics)