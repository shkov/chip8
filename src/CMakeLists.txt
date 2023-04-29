add_library(src emulator.cc screen.h keyboard.h speaker.h)
target_link_libraries(src spdlog sfml-graphics sfml-audio sfml-window)
