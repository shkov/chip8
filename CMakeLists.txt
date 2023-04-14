cmake_minimum_required(VERSION 3.20)
project(chip8)

set(CMAKE_CXX_STANDARD 20)

add_executable(${PROJECT_NAME} main.cc)

add_subdirectory(contrib/spdlog)
add_subdirectory(contrib/googletest)
add_subdirectory(contrib/benchmark)
add_subdirectory(contrib/sdl EXCLUDE_FROM_ALL)

target_link_libraries(${PROJECT_NAME} spdlog SDL3::SDL3)
