add_library(src emulator.cc screen.h keyboard.h)
target_link_libraries(src spdlog sfml-graphics)
