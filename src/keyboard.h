#pragma once

#include <spdlog/spdlog.h>

#include <SFML/Graphics.hpp>
#include <cstdint>
#include <optional>

#include "screen.h"

namespace chip8 {

class Keyboard {
 public:
  static bool IsKeyPressed(uint8_t key) { return sf::Keyboard::isKeyPressed(ConvertChip8ToQwerty(key)); };

  static std::optional<uint8_t> WaitForKeyPress(Screen& screen) {
    while (true) {
      std::optional<sf::Keyboard::Key> key{ screen.WaitKeyPress() };
      if (!key.has_value()) return std::nullopt;

      std::optional<uint8_t> chip8_key{ ConvertQwertyToChip8(*key) };
      if (chip8_key.has_value()) return *chip8_key;
    }
  };

 private:
  static sf::Keyboard::Key ConvertChip8ToQwerty(uint8_t key) {
    switch (key) {
      case 0x1:
        return sf::Keyboard::Num1;
      case 0x2:
        return sf::Keyboard::Num2;
      case 0x3:
        return sf::Keyboard::Num3;
      case 0xC:
        return sf::Keyboard::Num4;

      case 0x4:
        return sf::Keyboard::Q;
      case 0x5:
        return sf::Keyboard::W;
      case 0x6:
        return sf::Keyboard::E;
      case 0xD:
        return sf::Keyboard::R;

      case 0x7:
        return sf::Keyboard::A;
      case 0x8:
        return sf::Keyboard::S;
      case 0x9:
        return sf::Keyboard::D;
      case 0xE:
        return sf::Keyboard::F;

      case 0xA:
        return sf::Keyboard::Z;
      case 0x0:
        return sf::Keyboard::X;
      case 0xB:
        return sf::Keyboard::C;
      case 0xF:
        return sf::Keyboard::V;
    }
    assert(false);
    return sf::Keyboard::Unknown;
  };

  static std::optional<uint8_t> ConvertQwertyToChip8(sf::Keyboard::Key key) {
    switch (key) {
      case sf::Keyboard::Num1:
        return 0x1;
      case sf::Keyboard::Num2:
        return 0x2;
      case sf::Keyboard::Num3:
        return 0x3;
      case sf::Keyboard::Num4:
        return 0xC;

      case sf::Keyboard::Q:
        return 0x4;
      case sf::Keyboard::W:
        return 0x5;
      case sf::Keyboard::E:
        return 0x6;
      case sf::Keyboard::R:
        return 0xD;

      case sf::Keyboard::A:
        return 0x7;
      case sf::Keyboard::S:
        return 0x8;
      case sf::Keyboard::D:
        return sf::Keyboard::D;
      case sf::Keyboard::F:
        return 0xE;

      case sf::Keyboard::Z:
        return 0xA;
      case sf::Keyboard::X:
        return 0x0;
      case sf::Keyboard::C:
        return 0xB;
      case sf::Keyboard::V:
        return 0xF;

      default:
        return std::nullopt;
    }
  }
};

}  // namespace chip8
